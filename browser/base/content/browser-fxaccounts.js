# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

let gFxAccounts = {

  PREF_SYNC_START_DOORHANGER: "services.sync.ui.showSyncStartDoorhanger",
  DOORHANGER_ACTIVATE_DELAY_MS: 5000,
  SYNC_MIGRATION_NOTIFICATION_TITLE: "fxa-migration",

  _initialized: false,
  _inCustomizationMode: false,

  get weave() {
    delete this.weave;
    return this.weave = Cc["@mozilla.org/weave/service;1"]
                          .getService(Ci.nsISupports)
                          .wrappedJSObject;
  },

  get topics() {
    // Do all this dance to lazy-load FxAccountsCommon.
    delete this.topics;
    return this.topics = [
      "weave:service:ready",
      "weave:service:sync:start",
      "weave:service:login:error",
      "weave:service:setup-complete",
      "fxa-migration:state-changed",
      this.FxAccountsCommon.ONVERIFIED_NOTIFICATION,
      this.FxAccountsCommon.ONLOGOUT_NOTIFICATION
    ];
  },

  get button() {
    delete this.button;
    return this.button = document.getElementById("PanelUI-fxa-status");
  },

  get strings() {
    delete this.strings;
    return this.strings = Services.strings.createBundle(
      "chrome://browser/locale/accounts.properties"
    );
  },

  get loginFailed() {
    // Referencing Weave.Service will implicitly initialize sync, and we don't
    // want to force that - so first check if it is ready.
    let service = Cc["@mozilla.org/weave/service;1"]
                  .getService(Components.interfaces.nsISupports)
                  .wrappedJSObject;
    if (!service.ready) {
      return false;
    }
    // LOGIN_FAILED_LOGIN_REJECTED explicitly means "you must log back in".
    // All other login failures are assumed to be transient and should go
    // away by themselves, so aren't reflected here.
    return Weave.Status.login == Weave.LOGIN_FAILED_LOGIN_REJECTED;
  },

  get isActiveWindow() {
    let fm = Services.focus;
    return fm.activeWindow == window;
  },

  init: function () {
    // Bail out if we're already initialized and for pop-up windows.
    if (this._initialized || !window.toolbar.visible) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.addObserver(this, topic, false);
    }

    addEventListener("activate", this);
    gNavToolbox.addEventListener("customizationstarting", this);
    gNavToolbox.addEventListener("customizationending", this);

    // Request the current Legacy-Sync-to-FxA migration status.  We'll be
    // notified of fxa-migration:state-changed in response if necessary.
    Services.obs.notifyObservers(null, "fxa-migration:state-request", null);

    this._initialized = true;

    this.updateUI();
  },

  uninit: function () {
    if (!this._initialized) {
      return;
    }

    for (let topic of this.topics) {
      Services.obs.removeObserver(this, topic);
    }

    this._initialized = false;
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case this.FxAccountsCommon.ONVERIFIED_NOTIFICATION:
        Services.prefs.setBoolPref(this.PREF_SYNC_START_DOORHANGER, true);
        break;
      case "weave:service:sync:start":
        this.onSyncStart();
        break;
      case "fxa-migration:state-changed":
        this.onMigrationStateChanged(data, subject);
        break;
      default:
        this.updateUI();
        break;
    }
  },

  onSyncStart: function () {
    if (!this.isActiveWindow) {
      return;
    }

    let showDoorhanger = false;

    try {
      showDoorhanger = Services.prefs.getBoolPref(this.PREF_SYNC_START_DOORHANGER);
    } catch (e) { /* The pref might not exist. */ }

    if (showDoorhanger) {
      Services.prefs.clearUserPref(this.PREF_SYNC_START_DOORHANGER);
      this.showSyncStartedDoorhanger();
    }
  },

  onMigrationStateChanged: function (newState, email) {
    this._migrationInfo = !newState ? null : {
      state: newState,
      email: email ? email.QueryInterface(Ci.nsISupportsString).data : null,
    };
    this.updateUI();
  },

  handleEvent: function (event) {
    if (event.type == "activate") {
      // Our window might have been in the background while we received the
      // sync:start notification. If still needed, show the doorhanger after
      // a short delay. Without this delay the doorhanger would not show up
      // or with a too small delay show up while we're still animating the
      // window.
      setTimeout(() => this.onSyncStart(), this.DOORHANGER_ACTIVATE_DELAY_MS);
    } else {
      this._inCustomizationMode = event.type == "customizationstarting";
      this.updateAppMenuItem();
    }
  },

  showDoorhanger: function (id) {
    let panel = document.getElementById(id);
    let anchor = document.getElementById("PanelUI-menu-button");

    let iconAnchor =
      document.getAnonymousElementByAttribute(anchor, "class",
                                              "toolbarbutton-icon");

    panel.hidden = false;
    panel.openPopup(iconAnchor || anchor, "bottomcenter topright");
  },

  showSyncStartedDoorhanger: function () {
    this.showDoorhanger("sync-start-panel");
  },

  showSyncFailedDoorhanger: function () {
    this.showDoorhanger("sync-error-panel");
  },

  updateUI: function () {
    this.updateAppMenuItem();
    this.updateMigrationNotification();
  },

  updateAppMenuItem: function () {
    if (this._migrationInfo) {
      this.updateAppMenuItemForMigration();
      return;
    }

    // Bail out if FxA is disabled.
    if (!this.weave.fxAccountsEnabled) {
      // When migration transitions from needs-verification to the null state,
      // fxAccountsEnabled is false because migration has not yet finished.  In
      // that case, hide the button.  We'll get another notification with a null
      // state once migration is complete.
      this.button.hidden = true;
      this.button.removeAttribute("fxastatus");
      return;
    }

    // FxA is enabled, show the widget.
    this.button.hidden = false;

    // Make sure the button is disabled in customization mode.
    if (this._inCustomizationMode) {
      this.button.setAttribute("disabled", "true");
    } else {
      this.button.removeAttribute("disabled");
    }

    let defaultLabel = this.button.getAttribute("defaultlabel");
    let errorLabel = this.button.getAttribute("errorlabel");

    // If the user is signed into their Firefox account and we are not
    // currently in customization mode, show their email address.
    let doUpdate = userData => {
      // Reset the button to its original state.
      this.button.setAttribute("label", defaultLabel);
      this.button.removeAttribute("tooltiptext");
      this.button.removeAttribute("fxastatus");

      if (!this._inCustomizationMode) {
        if (this.loginFailed) {
          this.button.setAttribute("fxastatus", "error");
          this.button.setAttribute("label", errorLabel);
        } else if (userData) {
          this.button.setAttribute("fxastatus", "signedin");
          this.button.setAttribute("label", userData.email);
          this.button.setAttribute("tooltiptext", userData.email);
        }
      }
    }
    fxAccounts.getSignedInUser().then(userData => {
      doUpdate(userData);
    }).then(null, error => {
      // This is most likely in tests, were we quickly log users in and out.
      // The most likely scenario is a user logged out, so reflect that.
      // Bug 995134 calls for better errors so we could retry if we were
      // sure this was the failure reason.
      doUpdate(null);
    });
  },

  updateAppMenuItemForMigration: Task.async(function* () {
    let status = null;
    let label = null;
    switch (this._migrationInfo.state) {
      case this.fxaMigrator.STATE_USER_FXA:
        status = "migrate-signup";
        label = this.strings.formatStringFromName("needUserShort",
          [this.button.getAttribute("fxabrandname")], 1);
        break;
      case this.fxaMigrator.STATE_USER_FXA_VERIFIED:
        status = "migrate-verify";
        label = this.strings.formatStringFromName("needVerifiedUserShort",
                                                  [this._migrationInfo.email],
                                                  1);
        break;
    }
    this.button.label = label;
    this.button.hidden = false;
    this.button.setAttribute("fxastatus", status);
  }),

  updateMigrationNotification: Task.async(function* () {
    if (!this._migrationInfo) {
      Weave.Notifications.removeAll(this.SYNC_MIGRATION_NOTIFICATION_TITLE);
      return;
    }
    if (gBrowser.currentURI.spec.split("?")[0] == "about:accounts") {
      // If the current tab is about:accounts, assume the user just completed a
      // migration step and don't bother them with a redundant notification.
      return;
    }
    let note = null;
    switch (this._migrationInfo.state) {
      case this.fxaMigrator.STATE_USER_FXA: {
        // There are 2 cases here - no email address means it is an offer on
        // the first device (so the user is prompted to create an account).
        // If there is an email address it is the "join the party" flow, so the
        // user is prompted to sign in with the address they previously used.
        let msg, upgradeLabel, upgradeAccessKey;
        if (this._migrationInfo.email) {
          msg = this.strings.formatStringFromName("signInAfterUpgradeOnOtherDevice.description",
                                                  [this._migrationInfo.email],
                                                  1);
          upgradeLabel = this.strings.GetStringFromName("signInAfterUpgradeOnOtherDevice.label");
          upgradeAccessKey = this.strings.GetStringFromName("signInAfterUpgradeOnOtherDevice.accessKey");
        } else {
          msg = this.strings.GetStringFromName("needUserLong");
          upgradeLabel = this.strings.GetStringFromName("upgradeToFxA.label");
          upgradeAccessKey = this.strings.GetStringFromName("upgradeToFxA.accessKey");
        }
        note = new Weave.Notification(
          undefined, msg, undefined, Weave.Notifications.PRIORITY_WARNING, [
            new Weave.NotificationButton(upgradeLabel, upgradeAccessKey, () => {
              this.fxaMigrator.createFxAccount(window);
            }),
          ]
        );
        break;
      }
      case this.fxaMigrator.STATE_USER_FXA_VERIFIED: {
        let msg =
          this.strings.formatStringFromName("needVerifiedUserLong",
                                            [this._migrationInfo.email], 1);
        let resendLabel =
          this.strings.GetStringFromName("resendVerificationEmail.label");
        let resendAccessKey =
          this.strings.GetStringFromName("resendVerificationEmail.accessKey");
        note = new Weave.Notification(
          undefined, msg, undefined, Weave.Notifications.PRIORITY_INFO, [
            new Weave.NotificationButton(resendLabel, resendAccessKey, () => {
              this.fxaMigrator.resendVerificationMail();
            }),
          ]
        );
        break;
      }
    }
    note.title = this.SYNC_MIGRATION_NOTIFICATION_TITLE;
    Weave.Notifications.replaceTitle(note);
  }),

  onMenuPanelCommand: function (event) {
    let button = event.originalTarget;

    switch (button.getAttribute("fxastatus")) {
    case "signedin":
      this.openPreferences();
      break;
    case "error":
      this.openSignInAgainPage("menupanel");
      break;
    case "migrate-signup":
    case "migrate-verify":
      // The migration flow calls for the menu item to open sync prefs rather
      // than requesting migration start immediately.
      this.openPreferences();
      break;
    default:
      this.openAccountsPage(null, { entryPoint: "menupanel" });
      break;
    }

    PanelUI.hide();
  },

  openPreferences: function () {
    openPreferences("paneSync");
  },

  openAccountsPage: function (action, urlParams={}) {
    // An entryPoint param is used for server-side metrics.  If the current tab
    // is UITour, assume that it initiated the call to this method and override
    // the entryPoint accordingly.
    if (UITour.originTabs.get(window) &&
        UITour.originTabs.get(window).has(gBrowser.selectedTab)) {
      urlParams.entryPoint = "uitour";
    }
    let params = new URLSearchParams();
    if (action) {
      params.set("action", action);
    }
    for (let name in urlParams) {
      if (urlParams[name] !== undefined) {
        params.set(name, urlParams[name]);
      }
    }
    let url = "about:accounts?" + params;
    switchToTabHavingURI(url, true, {
      replaceQueryString: true
    });
  },

  openSignInAgainPage: function (entryPoint) {
    this.openAccountsPage("reauth", { entryPoint: entryPoint });
  },
};

XPCOMUtils.defineLazyGetter(gFxAccounts, "FxAccountsCommon", function () {
  return Cu.import("resource://gre/modules/FxAccountsCommon.js", {});
});

XPCOMUtils.defineLazyModuleGetter(gFxAccounts, "fxaMigrator",
  "resource://services-sync/FxaMigrator.jsm");
