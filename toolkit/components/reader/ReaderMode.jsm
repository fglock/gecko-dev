// -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

this.EXPORTED_SYMBOLS = ["ReaderMode"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.importGlobalProperties(["XMLHttpRequest"]);

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils", "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");

let ReaderMode = {
  // Version of the cache schema.
  CACHE_VERSION: 1,

  DEBUG: 0,

  // Don't try to parse the page if it has too many elements (for memory and
  // performance reasons)
  MAX_ELEMS_TO_PARSE: 3000,

  /**
   * Gets an article from a loaded browser's document. This method will parse the document
   * if it does not find the article in the cache.
   *
   * @param browser A browser with a loaded page.
   * @return {Promise}
   * @resolves JS object representing the article, or null if no article is found.
   */
  parseDocumentFromBrowser: Task.async(function* (browser) {
    let uri = browser.currentURI;
    if (!this._shouldCheckUri(uri)) {
      this.log("Reader mode disabled for URI");
      return null;
    }

    // First, try to find a parsed article in the cache.
    let article = yield this.getArticleFromCache(uri);
    if (article) {
      this.log("Page found in cache, return article immediately");
      return article;
    }

    let doc = browser.contentWindow.document;
    return yield this._readerParse(uri, doc);
  }),

  /**
   * Downloads and parses a document from a URL.
   *
   * @param url URL to download and parse.
   * @return {Promise}
   * @resolves JS object representing the article, or null if no article is found.
   */
  downloadAndParseDocument: Task.async(function* (url) {
    let uri = Services.io.newURI(url, null, null);
    let doc = yield this._downloadDocument(url);
    return yield this._readerParse(uri, doc);
  }),

  _downloadDocument: function (url) {
    return new Promise((resolve, reject) => {
      let xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      xhr.onerror = evt => reject(evt.error);
      xhr.responseType = "document";
      xhr.onload = evt => {
        if (xhr.status !== 200) {
          reject("Reader mode XHR failed with status: " + xhr.status);
          return;
        }

        let doc = xhr.responseXML;

        // Manually follow a meta refresh tag if one exists.
        let meta = doc.querySelector("meta[http-equiv=refresh]");
        if (meta) {
          let content = meta.getAttribute("content");
          if (content) {
            let urlIndex = content.indexOf("URL=");
            if (urlIndex > -1) {
              let url = content.substring(urlIndex + 4);
              this._downloadDocument(url).then((doc) => resolve(doc));
              return;
            }
          }
        }
        resolve(doc);
      }
      xhr.send();
    });
  },


  /**
   * Retrieves an article from the cache given an article URI.
   *
   * @param uri The article URI.
   * @return {Promise}
   * @resolves JS object representing the article, or null if no article is found.
   * @rejects OS.File.Error
   */
  getArticleFromCache: Task.async(function* (uri) {
    let path = this._toHashedPath(uri.specIgnoringRef);
    try {
      let array = yield OS.File.read(path);
      return JSON.parse(new TextDecoder().decode(array));
    } catch (e if e instanceof OS.File.Error && e.becauseNoSuchFile) {
      return null;
    }
  }),

  /**
   * Stores an article in the cache.
   *
   * @param article JS object representing article.
   * @return {Promise}
   * @resolves When the article is stored.
   * @rejects OS.File.Error
   */
  storeArticleInCache: Task.async(function* (article) {
    let array = new TextEncoder().encode(JSON.stringify(article));
    let path = this._toHashedPath(article.url);
    yield this._ensureCacheDir();
    yield OS.File.writeAtomic(path, array, { tmpPath: path + ".tmp" });
  }),

  /**
   * Removes an article from the cache given an article URI.
   *
   * @param uri The article URI.
   * @return {Promise}
   * @resolves When the article is removed.
   * @rejects OS.File.Error
   */
  removeArticleFromCache: Task.async(function* (uri) {
    let path = this._toHashedPath(uri.specIgnoringRef);
    yield OS.File.remove(path);
  }),

  log: function(msg) {
    if (this.DEBUG)
      dump("Reader: " + msg);
  },

  _shouldCheckUri: function (uri) {
    if ((uri.prePath + "/") === uri.spec) {
      this.log("Not parsing home page: " + uri.spec);
      return false;
    }

    if (!(uri.schemeIs("http") || uri.schemeIs("https") || uri.schemeIs("file"))) {
      this.log("Not parsing URI scheme: " + uri.scheme);
      return false;
    }

    return true;
  },

  /**
   * Attempts to parse a document into an article. Heavy lifting happens
   * in readerWorker.js.
   *
   * @param uri The article URI.
   * @param doc The document to parse.
   * @return {Promise}
   * @resolves JS object representing the article, or null if no article is found.
   */
  _readerParse: function (uri, doc) {
    return new Promise((resolve, reject) => {
      let numTags = doc.getElementsByTagName("*").length;
      if (numTags > this.MAX_ELEMS_TO_PARSE) {
        this.log("Aborting parse for " + uri.spec + "; " + numTags + " elements found");
        resolve(null);
        return;
      }

      let worker = new ChromeWorker("chrome://global/content/reader/readerWorker.js");
      worker.onmessage = evt => {
        let article = evt.data;

        if (!article) {
          this.log("Worker did not return an article");
          resolve(null);
          return;
        }

        // Append URL to the article data. specIgnoringRef will ignore any hash
        // in the URL.
        article.url = uri.specIgnoringRef;
        let flags = Ci.nsIDocumentEncoder.OutputSelectionOnly | Ci.nsIDocumentEncoder.OutputAbsoluteLinks;
        article.title = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils)
                                                        .convertToPlainText(article.title, flags, 0);
        resolve(article);
      };

      worker.onerror = evt => {
        reject("Error in worker: " + evt.message);
      };

      try {
        let serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                         createInstance(Ci.nsIDOMSerializer);
        worker.postMessage({
          uri: {
            spec: uri.spec,
            host: uri.host,
            prePath: uri.prePath,
            scheme: uri.scheme,
            pathBase: Services.io.newURI(".", null, uri).spec
          },
          doc: serializer.serializeToString(doc)
        });
      } catch (e) {
        reject("Reader: could not build Readability arguments: " + e);
      }
    });
  },

  get _cryptoHash() {
    delete this._cryptoHash;
    return this._cryptoHash = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  },

  get _unicodeConverter() {
    delete this._unicodeConverter;
    this._unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                              .createInstance(Ci.nsIScriptableUnicodeConverter);
    this._unicodeConverter.charset = "utf8";
    return this._unicodeConverter;
  },

  /**
   * Calculate the hashed path for a stripped article URL.
   *
   * @param url The article URL. This should have referrers removed.
   * @return The file path to the cached article.
   */
  _toHashedPath: function (url) {
    let value = this._unicodeConverter.convertToByteArray(url);
    this._cryptoHash.init(this._cryptoHash.MD5);
    this._cryptoHash.update(value, value.length);

    let hash = CommonUtils.encodeBase32(this._cryptoHash.finish(false));
    let fileName = hash.substring(0, hash.indexOf("=")) + ".json";
    return OS.Path.join(OS.Constants.Path.profileDir, "readercache", fileName);
  },

  /**
   * Ensures the cache directory exists.
   *
   * @return Promise
   * @resolves When the cache directory exists.
   * @rejects OS.File.Error
   */
  _ensureCacheDir: function () {
    let dir = OS.Path.join(OS.Constants.Path.profileDir, "readercache");
    return OS.File.exists(dir).then(exists => {
      if (!exists) {
        return OS.File.makeDir(dir);
      }
    });
  }
};
