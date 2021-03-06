/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_File_h
#define mozilla_dom_File_h

#include "mozilla/Attributes.h"

#include "mozilla/GuardObjects.h"
#include "mozilla/LinkedList.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/Date.h"
#include "mozilla/dom/indexedDB/FileInfo.h"
#include "mozilla/dom/indexedDB/FileManager.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIFile.h"
#include "nsIMutable.h"
#include "nsIXMLHttpRequest.h"
#include "nsString.h"
#include "nsTemporaryFileInputStream.h"
#include "nsWrapperCache.h"
#include "nsWeakReference.h"

class nsDOMMultipartFile;
class nsIFile;
class nsIInputStream;
class nsIClassInfo;

#define PIFILEIMPL_IID \
  { 0x218ee173, 0xf44f, 0x4d30, \
    { 0xab, 0x0c, 0xd6, 0x66, 0xea, 0xc2, 0x84, 0x47 } }

class PIFileImpl : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(PIFILEIMPL_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(PIFileImpl, PIFILEIMPL_IID)

namespace mozilla {
namespace dom {

namespace indexedDB {
class FileInfo;
};

struct BlobPropertyBag;
struct ChromeFilePropertyBag;
struct FilePropertyBag;
class FileImpl;
class OwningArrayBufferOrArrayBufferViewOrBlobOrString;

class File MOZ_FINAL : public nsIDOMFile
                     , public nsIXHRSendable
                     , public nsIMutable
                     , public nsSupportsWeakReference
                     , public nsWrapperCache
{
public:
  NS_DECL_NSIDOMBLOB
  NS_DECL_NSIDOMFILE
  NS_DECL_NSIXHRSENDABLE
  NS_DECL_NSIMUTABLE

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(File, nsIDOMFile)

  static already_AddRefed<File>
  Create(nsISupports* aParent, const nsAString& aName,
         const nsAString& aContentType, uint64_t aLength,
         uint64_t aLastModifiedDate);

  static already_AddRefed<File>
  Create(nsISupports* aParent, const nsAString& aName,
         const nsAString& aContentType, uint64_t aLength);

  static already_AddRefed<File>
  Create(nsISupports* aParent, const nsAString& aContentType,
         uint64_t aLength);

  static already_AddRefed<File>
  Create(nsISupports* aParent, const nsAString& aContentType, uint64_t aStart,
         uint64_t aLength);

  // The returned File takes ownership of aMemoryBuffer. aMemoryBuffer will be
  // freed by moz_free so it must be allocated by moz_malloc or something
  // compatible with it.
  static already_AddRefed<File>
  CreateMemoryFile(nsISupports* aParent, void* aMemoryBuffer, uint64_t aLength,
                   const nsAString& aName, const nsAString& aContentType,
                   uint64_t aLastModifiedDate);

  // The returned File takes ownership of aMemoryBuffer. aMemoryBuffer will be
  // freed by moz_free so it must be allocated by moz_malloc or something
  // compatible with it.
  static already_AddRefed<File>
  CreateMemoryFile(nsISupports* aParent, void* aMemoryBuffer, uint64_t aLength,
                   const nsAString& aContentType);

  static already_AddRefed<File>
  CreateTemporaryFileBlob(nsISupports* aParent, PRFileDesc* aFD,
                          uint64_t aStartPos, uint64_t aLength,
                          const nsAString& aContentType);

  static already_AddRefed<File>
  CreateFromFile(nsISupports* aParent, nsIFile* aFile, bool aTemporary = false);

  static already_AddRefed<File>
  CreateFromFile(nsISupports* aParent, const nsAString& aContentType,
                 uint64_t aLength, nsIFile* aFile,
                 indexedDB::FileInfo* aFileInfo);

  static already_AddRefed<File>
  CreateFromFile(nsISupports* aParent, const nsAString& aName,
                 const nsAString& aContentType, uint64_t aLength,
                 nsIFile* aFile, indexedDB::FileInfo* aFileInfo);

  static already_AddRefed<File>
  CreateFromFile(nsISupports* aParent, nsIFile* aFile,
                 indexedDB::FileInfo* aFileInfo);

  static already_AddRefed<File>
  CreateFromFile(nsISupports* aParent, nsIFile* aFile, const nsAString& aName,
                 const nsAString& aContentType);

  File(nsISupports* aParent, FileImpl* aImpl);

  FileImpl* Impl() const
  {
    return mImpl;
  }

  const nsTArray<nsRefPtr<FileImpl>>* GetSubBlobImpls() const;

  bool IsSizeUnknown() const;

  bool IsDateUnknown() const;

  bool IsFile() const;

  already_AddRefed<File>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType,
              ErrorResult& aRv);

  // WebIDL methods
  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  // Blob constructor
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  // Blob constructor
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal,
              const Sequence<OwningArrayBufferOrArrayBufferViewOrBlobOrString>& aData,
              const BlobPropertyBag& aBag,
              ErrorResult& aRv);

  // File constructor
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal,
              const Sequence<OwningArrayBufferOrArrayBufferViewOrBlobOrString>& aData,
              const nsAString& aName,
              const FilePropertyBag& aBag,
              ErrorResult& aRv);

  // File constructor - ChromeOnly
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal,
              File& aData,
              const ChromeFilePropertyBag& aBag,
              ErrorResult& aRv);

  // File constructor - ChromeOnly
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aData,
              const ChromeFilePropertyBag& aBag,
              ErrorResult& aRv);

  // File constructor - ChromeOnly
  static already_AddRefed<File>
  Constructor(const GlobalObject& aGlobal,
              nsIFile* aData,
              const ChromeFilePropertyBag& aBag,
              ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint64_t GetSize(ErrorResult& aRv);

  // XPCOM GetType is OK

  // XPCOM GetName is OK

  int64_t GetLastModified(ErrorResult& aRv);

  Date GetLastModifiedDate(ErrorResult& aRv);

  void GetMozFullPath(nsAString& aFilename, ErrorResult& aRv);

  already_AddRefed<File> Slice(const Optional<int64_t>& aStart,
                               const Optional<int64_t>& aEnd,
                               const nsAString& aContentType,
                               ErrorResult& aRv);

private:
  ~File() {};

  // The member is the real backend implementation of this File/Blob.
  // It's thread-safe and not CC-able and it's the only element that is moved
  // between threads.
  // Note: we should not store any other state in this class!
  const nsRefPtr<FileImpl> mImpl;

  nsCOMPtr<nsISupports> mParent;
};

// This is the abstract class for any File backend. It must be nsISupports
// because this class must be ref-counted and it has to work with IPC.
class FileImpl : public PIFileImpl
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  FileImpl() {}

  virtual void GetName(nsAString& aName) = 0;

  virtual nsresult GetPath(nsAString& aName) = 0;

  virtual int64_t GetLastModified(ErrorResult& aRv) = 0;

  virtual void GetMozFullPath(nsAString& aName, ErrorResult& aRv) = 0;

  virtual void GetMozFullPathInternal(nsAString& aFileName, ErrorResult& aRv) = 0;

  virtual uint64_t GetSize(ErrorResult& aRv) = 0;

  virtual void GetType(nsAString& aType) = 0;

  already_AddRefed<FileImpl>
  Slice(const Optional<int64_t>& aStart, const Optional<int64_t>& aEnd,
        const nsAString& aContentType, ErrorResult& aRv);

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType, ErrorResult& aRv) = 0;

  virtual const nsTArray<nsRefPtr<FileImpl>>*
  GetSubBlobImpls() const = 0;

  virtual nsresult GetInternalStream(nsIInputStream** aStream) = 0;

  virtual int64_t GetFileId() = 0;

  virtual void AddFileInfo(indexedDB::FileInfo* aFileInfo) = 0;

  virtual indexedDB::FileInfo*
  GetFileInfo(indexedDB::FileManager* aFileManager) = 0;

  virtual nsresult GetSendInfo(nsIInputStream** aBody,
                               uint64_t* aContentLength,
                               nsACString& aContentType,
                               nsACString& aCharset) = 0;

  virtual nsresult GetMutable(bool* aMutable) const = 0;

  virtual nsresult SetMutable(bool aMutable) = 0;

  virtual void SetLazyData(const nsAString& aName,
                           const nsAString& aContentType,
                           uint64_t aLength,
                           uint64_t aLastModifiedDate) = 0;

  virtual bool IsMemoryFile() const = 0;

  virtual bool IsSizeUnknown() const = 0;

  virtual bool IsDateUnknown() const = 0;

  virtual bool IsFile() const = 0;

  // These 2 methods are used when the implementation has to CC something.
  virtual void Unlink() = 0;
  virtual void Traverse(nsCycleCollectionTraversalCallback &aCb) = 0;

  virtual bool IsCCed() const
  {
    return false;
  }

protected:
  virtual ~FileImpl() {}
};

class FileImplBase : public FileImpl
{
public:
  FileImplBase(const nsAString& aName, const nsAString& aContentType,
               uint64_t aLength, uint64_t aLastModifiedDate)
    : mIsFile(true)
    , mImmutable(false)
    , mContentType(aContentType)
    , mName(aName)
    , mStart(0)
    , mLength(aLength)
    , mLastModificationDate(aLastModifiedDate)
  {
    // Ensure non-null mContentType by default
    mContentType.SetIsVoid(false);
  }

  FileImplBase(const nsAString& aName, const nsAString& aContentType,
               uint64_t aLength)
    : mIsFile(true)
    , mImmutable(false)
    , mContentType(aContentType)
    , mName(aName)
    , mStart(0)
    , mLength(aLength)
    , mLastModificationDate(UINT64_MAX)
  {
    // Ensure non-null mContentType by default
    mContentType.SetIsVoid(false);
  }

  FileImplBase(const nsAString& aContentType, uint64_t aLength)
    : mIsFile(false)
    , mImmutable(false)
    , mContentType(aContentType)
    , mStart(0)
    , mLength(aLength)
    , mLastModificationDate(UINT64_MAX)
  {
    // Ensure non-null mContentType by default
    mContentType.SetIsVoid(false);
  }

  FileImplBase(const nsAString& aContentType, uint64_t aStart,
               uint64_t aLength)
    : mIsFile(false)
    , mImmutable(false)
    , mContentType(aContentType)
    , mStart(aStart)
    , mLength(aLength)
    , mLastModificationDate(UINT64_MAX)
  {
    NS_ASSERTION(aLength != UINT64_MAX,
                 "Must know length when creating slice");
    // Ensure non-null mContentType by default
    mContentType.SetIsVoid(false);
  }

  virtual void GetName(nsAString& aName) MOZ_OVERRIDE;

  virtual nsresult GetPath(nsAString& aName) MOZ_OVERRIDE;

  virtual int64_t GetLastModified(ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void GetMozFullPath(nsAString& aName, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void GetMozFullPathInternal(nsAString& aFileName,
                                      ErrorResult& aRv) MOZ_OVERRIDE;

  virtual uint64_t GetSize(ErrorResult& aRv) MOZ_OVERRIDE
  {
    return mLength;
  }

  virtual void GetType(nsAString& aType) MOZ_OVERRIDE;

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType, ErrorResult& aRv) MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual const nsTArray<nsRefPtr<FileImpl>>*
  GetSubBlobImpls() const MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual nsresult GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  virtual int64_t GetFileId() MOZ_OVERRIDE;

  virtual void AddFileInfo(indexedDB::FileInfo* aFileInfo) MOZ_OVERRIDE;

  virtual indexedDB::FileInfo*
  GetFileInfo(indexedDB::FileManager* aFileManager) MOZ_OVERRIDE;

  virtual nsresult GetSendInfo(nsIInputStream** aBody,
                               uint64_t* aContentLength,
                               nsACString& aContentType,
                               nsACString& aCharset) MOZ_OVERRIDE;

  virtual nsresult GetMutable(bool* aMutable) const MOZ_OVERRIDE;

  virtual nsresult SetMutable(bool aMutable) MOZ_OVERRIDE;

  virtual void
  SetLazyData(const nsAString& aName, const nsAString& aContentType,
              uint64_t aLength, uint64_t aLastModifiedDate) MOZ_OVERRIDE
  {
    NS_ASSERTION(aLength, "must have length");

    mName = aName;
    mContentType = aContentType;
    mLength = aLength;
    mLastModificationDate = aLastModifiedDate;
    mIsFile = !aName.IsVoid();
  }

  virtual bool IsMemoryFile() const MOZ_OVERRIDE
  {
    return false;
  }

  virtual bool IsDateUnknown() const MOZ_OVERRIDE
  {
    return mIsFile && mLastModificationDate == UINT64_MAX;
  }

  virtual bool IsFile() const MOZ_OVERRIDE
  {
    return mIsFile;
  }

  virtual bool IsStoredFile() const
  {
    return false;
  }

  virtual bool IsWholeFile() const
  {
    NS_NOTREACHED("Should only be called on dom blobs backed by files!");
    return false;
  }

  virtual bool IsSnapshot() const
  {
    return false;
  }

  virtual bool IsSizeUnknown() const MOZ_OVERRIDE
  {
    return mLength == UINT64_MAX;
  }

  virtual void Unlink() MOZ_OVERRIDE {}
  virtual void Traverse(nsCycleCollectionTraversalCallback &aCb) MOZ_OVERRIDE {}

protected:
  virtual ~FileImplBase() {}

  indexedDB::FileInfo* GetFileInfo() const
  {
    NS_ASSERTION(IsStoredFile(), "Should only be called on stored files!");
    NS_ASSERTION(!mFileInfos.IsEmpty(), "Must have at least one file info!");

    return mFileInfos.ElementAt(0);
  }

  bool mIsFile;
  bool mImmutable;

  nsString mContentType;
  nsString mName;
  nsString mPath; // The path relative to a directory chosen by the user

  uint64_t mStart;
  uint64_t mLength;

  uint64_t mLastModificationDate;

  // Protected by IndexedDatabaseManager::FileMutex()
  nsTArray<nsRefPtr<indexedDB::FileInfo>> mFileInfos;
};

/**
 * This class may be used off the main thread, and in particular, its
 * constructor and destructor may not run on the same thread.  Be careful!
 */
class FileImplMemory MOZ_FINAL : public FileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  FileImplMemory(void* aMemoryBuffer, uint64_t aLength, const nsAString& aName,
                 const nsAString& aContentType, uint64_t aLastModifiedDate)
    : FileImplBase(aName, aContentType, aLength, aLastModifiedDate)
    , mDataOwner(new DataOwner(aMemoryBuffer, aLength))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  FileImplMemory(void* aMemoryBuffer, uint64_t aLength,
                 const nsAString& aContentType)
    : FileImplBase(aContentType, aLength)
    , mDataOwner(new DataOwner(aMemoryBuffer, aLength))
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
  }

  virtual nsresult GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual bool IsMemoryFile() const MOZ_OVERRIDE
  {
    return true;
  }

  class DataOwner MOZ_FINAL : public mozilla::LinkedListElement<DataOwner> {
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DataOwner)
    DataOwner(void* aMemoryBuffer, uint64_t aLength)
      : mData(aMemoryBuffer)
      , mLength(aLength)
    {
      mozilla::StaticMutexAutoLock lock(sDataOwnerMutex);

      if (!sDataOwners) {
        sDataOwners = new mozilla::LinkedList<DataOwner>();
        EnsureMemoryReporterRegistered();
      }
      sDataOwners->insertBack(this);
    }

  private:
    // Private destructor, to discourage deletion outside of Release():
    ~DataOwner() {
      mozilla::StaticMutexAutoLock lock(sDataOwnerMutex);

      remove();
      if (sDataOwners->isEmpty()) {
        // Free the linked list if it's empty.
        sDataOwners = nullptr;
      }

      moz_free(mData);
    }

  public:
    static void EnsureMemoryReporterRegistered();

    // sDataOwners and sMemoryReporterRegistered may only be accessed while
    // holding sDataOwnerMutex!  You also must hold the mutex while touching
    // elements of the linked list that DataOwner inherits from.
    static mozilla::StaticMutex sDataOwnerMutex;
    static mozilla::StaticAutoPtr<mozilla::LinkedList<DataOwner> > sDataOwners;
    static bool sMemoryReporterRegistered;

    void* mData;
    uint64_t mLength;
  };

private:
  // Create slice
  FileImplMemory(const FileImplMemory* aOther, uint64_t aStart,
                 uint64_t aLength, const nsAString& aContentType)
    : FileImplBase(aContentType, aOther->mStart + aStart, aLength)
    , mDataOwner(aOther->mDataOwner)
  {
    NS_ASSERTION(mDataOwner && mDataOwner->mData, "must have data");
    mImmutable = aOther->mImmutable;
  }

  ~FileImplMemory() {}

  // Used when backed by a memory store
  nsRefPtr<DataOwner> mDataOwner;
};

class FileImplTemporaryFileBlob MOZ_FINAL : public FileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  FileImplTemporaryFileBlob(PRFileDesc* aFD, uint64_t aStartPos,
                            uint64_t aLength, const nsAString& aContentType)
    : FileImplBase(aContentType, aLength)
    , mLength(aLength)
    , mStartPos(aStartPos)
    , mContentType(aContentType)
  {
    mFileDescOwner = new nsTemporaryFileInputStream::FileDescOwner(aFD);
  }

  virtual nsresult GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType, ErrorResult& aRv) MOZ_OVERRIDE;

private:
  FileImplTemporaryFileBlob(const FileImplTemporaryFileBlob* aOther,
                            uint64_t aStart, uint64_t aLength,
                            const nsAString& aContentType)
    : FileImplBase(aContentType, aLength)
    , mLength(aLength)
    , mStartPos(aStart)
    , mFileDescOwner(aOther->mFileDescOwner)
    , mContentType(aContentType) {}

  ~FileImplTemporaryFileBlob() {}

  uint64_t mLength;
  uint64_t mStartPos;
  nsRefPtr<nsTemporaryFileInputStream::FileDescOwner> mFileDescOwner;
  nsString mContentType;
};

class FileImplFile : public FileImplBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  // Create as a file
  explicit FileImplFile(nsIFile* aFile, bool aTemporary = false)
    : FileImplBase(EmptyString(), EmptyString(), UINT64_MAX, UINT64_MAX)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(false)
    , mIsTemporary(aTemporary)
  {
    NS_ASSERTION(mFile, "must have file");
    // Lazily get the content type and size
    mContentType.SetIsVoid(true);
    mFile->GetLeafName(mName);
  }

  FileImplFile(nsIFile* aFile, indexedDB::FileInfo* aFileInfo)
    : FileImplBase(EmptyString(), EmptyString(), UINT64_MAX, UINT64_MAX)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(true)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
    NS_ASSERTION(aFileInfo, "must have file info");
    // Lazily get the content type and size
    mContentType.SetIsVoid(true);
    mFile->GetLeafName(mName);

    mFileInfos.AppendElement(aFileInfo);
  }

  // Create as a file
  FileImplFile(const nsAString& aName, const nsAString& aContentType,
               uint64_t aLength, nsIFile* aFile)
    : FileImplBase(aName, aContentType, aLength, UINT64_MAX)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(false)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
  }

  FileImplFile(const nsAString& aName, const nsAString& aContentType,
               uint64_t aLength, nsIFile* aFile,
               uint64_t aLastModificationDate)
    : FileImplBase(aName, aContentType, aLength, aLastModificationDate)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(false)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
  }

  // Create as a file with custom name
  FileImplFile(nsIFile* aFile, const nsAString& aName,
               const nsAString& aContentType)
    : FileImplBase(aName, aContentType, UINT64_MAX, UINT64_MAX)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(false)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
    if (aContentType.IsEmpty()) {
      // Lazily get the content type and size
      mContentType.SetIsVoid(true);
    }
  }

  // Create as a stored file
  FileImplFile(const nsAString& aName, const nsAString& aContentType,
               uint64_t aLength, nsIFile* aFile,
               indexedDB::FileInfo* aFileInfo)
    : FileImplBase(aName, aContentType, aLength, UINT64_MAX)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(true)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
    mFileInfos.AppendElement(aFileInfo);
  }

  // Create as a stored blob
  FileImplFile(const nsAString& aContentType, uint64_t aLength,
               nsIFile* aFile, indexedDB::FileInfo* aFileInfo)
    : FileImplBase(aContentType, aLength)
    , mFile(aFile)
    , mWholeFile(true)
    , mStoredFile(true)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
    mFileInfos.AppendElement(aFileInfo);
  }

  // Create as a file to be later initialized
  FileImplFile()
    : FileImplBase(EmptyString(), EmptyString(), UINT64_MAX, UINT64_MAX)
    , mWholeFile(true)
    , mStoredFile(false)
    , mIsTemporary(false)
  {
    // Lazily get the content type and size
    mContentType.SetIsVoid(true);
    mName.SetIsVoid(true);
  }

  // Overrides
  virtual uint64_t GetSize(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void GetType(nsAString& aType) MOZ_OVERRIDE;
  virtual int64_t GetLastModified(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void GetMozFullPathInternal(nsAString& aFullPath,
                                      ErrorResult& aRv) MOZ_OVERRIDE;
  virtual nsresult GetInternalStream(nsIInputStream**) MOZ_OVERRIDE;

  void SetPath(const nsAString& aFullPath);

protected:
  virtual ~FileImplFile() {
    if (mFile && mIsTemporary) {
      NS_WARNING("In temporary ~FileImplFile");
      // Ignore errors if any, not much we can do. Clean-up will be done by
      // https://mxr.mozilla.org/mozilla-central/source/xpcom/io/nsAnonymousTemporaryFile.cpp?rev=6c1c7e45c902#127
#ifdef DEBUG
      nsresult rv =
#endif
      mFile->Remove(false);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to remove temporary DOMFile.");
    }
  }

private:
  // Create slice
  FileImplFile(const FileImplFile* aOther, uint64_t aStart,
               uint64_t aLength, const nsAString& aContentType)
    : FileImplBase(aContentType, aOther->mStart + aStart, aLength)
    , mFile(aOther->mFile)
    , mWholeFile(false)
    , mStoredFile(aOther->mStoredFile)
    , mIsTemporary(false)
  {
    NS_ASSERTION(mFile, "must have file");
    mImmutable = aOther->mImmutable;

    if (mStoredFile) {
      indexedDB::FileInfo* fileInfo;

      using indexedDB::IndexedDatabaseManager;

      if (IndexedDatabaseManager::IsClosed()) {
        fileInfo = aOther->GetFileInfo();
      }
      else {
        mozilla::MutexAutoLock lock(IndexedDatabaseManager::FileMutex());
        fileInfo = aOther->GetFileInfo();
      }

      mFileInfos.AppendElement(fileInfo);
    }
  }

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual bool IsStoredFile() const MOZ_OVERRIDE
  {
    return mStoredFile;
  }

  virtual bool IsWholeFile() const MOZ_OVERRIDE
  {
    return mWholeFile;
  }

  nsCOMPtr<nsIFile> mFile;
  bool mWholeFile;
  bool mStoredFile;
  bool mIsTemporary;
};

class FileList MOZ_FINAL : public nsIDOMFileList,
                           public nsWrapperCache
{
  ~FileList() {}

public:
  explicit FileList(nsISupports *aParent) : mParent(aParent)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FileList)

  NS_DECL_NSIDOMFILELIST

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  nsISupports* GetParentObject()
  {
    return mParent;
  }

  void Disconnect()
  {
    mParent = nullptr;
  }

  bool Append(File *aFile) { return mFiles.AppendElement(aFile); }

  bool Remove(uint32_t aIndex) {
    if (aIndex < mFiles.Length()) {
      mFiles.RemoveElementAt(aIndex);
      return true;
    }

    return false;
  }

  void Clear() { return mFiles.Clear(); }

  static FileList* FromSupports(nsISupports* aSupports)
  {
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMFileList> list_qi = do_QueryInterface(aSupports);

      // If this assertion fires the QI implementation for the object in
      // question doesn't use the nsIDOMFileList pointer as the nsISupports
      // pointer. That must be fixed, or we'll crash...
      NS_ASSERTION(list_qi == static_cast<nsIDOMFileList*>(aSupports),
                   "Uh, fix QI!");
    }
#endif

    return static_cast<FileList*>(aSupports);
  }

  File* Item(uint32_t aIndex)
  {
    return mFiles.SafeElementAt(aIndex);
  }
  File* IndexedGetter(uint32_t aIndex, bool& aFound)
  {
    aFound = aIndex < mFiles.Length();
    return aFound ? mFiles.ElementAt(aIndex) : nullptr;
  }
  uint32_t Length()
  {
    return mFiles.Length();
  }

private:
  nsTArray<nsRefPtr<File>> mFiles;
  nsISupports *mParent;
};

} // dom namespace
} // file namespace

#endif // mozilla_dom_File_h
