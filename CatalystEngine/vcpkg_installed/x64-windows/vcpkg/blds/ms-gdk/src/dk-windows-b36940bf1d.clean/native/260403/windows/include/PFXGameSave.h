// Copyright(c) Microsoft Corporation. All rights reserved.

#if !defined(__cplusplus)
    #error C++11 required
#endif

#pragma once

#include <stdint.h>
#include <XAsync.h>
#include <XTaskQueue.h>
#include <XUser.h>
extern "C" 
{

/*
PFXGameSave is a set of API's that enable reading and writing data on behalf of a user / game to be
persisted across game sessions and synced with the PlayFab cloud to be used across multiple devices.

This is similar to the XGameSaveFiles API with the PlayFab cloud as the backend to allow for cross-platform progression.
*/

typedef struct PFXGameSaveConfig* PFXGameSaveConfigHandle;

enum class PFXGameSaveConfigRequestInitFlags : uint32_t
{
    InitFlagNone = 0x0,
    InitFlagRollbackToLastKnownGood = 0x1,
    InitFlagRollbackToLastConflict = 0x2,
    InitFlagUseEntityAuth = 0x4,
    InitFlagUseFileLocation = 0x8
};
DEFINE_ENUM_FLAG_OPERATORS(PFXGameSaveConfigRequestInitFlags);

struct PFXGameSaveConfigRequest
{
    XUserHandle requestingUser;
    _Field_z_ const char* titleId;
    _Field_z_ const char* apiUrl;
    uint32_t flags;
    _Field_z_ const char* entityId;
    _Field_z_ const char* entityToken;
    _Field_z_ const char* fileLocation;
};

static const HRESULT E_GS_PLAYFAB_LOGIN_FAILURE = 0x80832300;
static const HRESULT E_GS_PLAYFAB_SERVICE_FAILURE = 0x80832301;
static const HRESULT E_GS_PLAYFAB_SYNC_FAILURE = 0x80832302;
static const HRESULT E_GS_PLAYFAB_SYNC_CONFLICT = 0x80832303;
static const HRESULT E_GS_PLAYFAB_INVALID_RESPONSE = 0x80832304;
static const HRESULT E_GS_PLAYFAB_INVALID_LOCATION = 0x80832305;

STDAPI PFXGameSaveInitializeConfig(_In_ PFXGameSaveConfigRequest* configRequest, _Outptr_result_nullonfailure_ PFXGameSaveConfigHandle* config) noexcept;
void PFXGameSaveFreeConfig(_In_ PFXGameSaveConfigHandle config) noexcept;

STDAPI PFXGameSaveFilesGetFolderWithUiAsync(_In_ PFXGameSaveConfigHandle configHandle, _In_ XAsyncBlock* async) noexcept;
STDAPI PFXGameSaveFilesGetFolderWithUiResult(_In_ XAsyncBlock* async, _In_ size_t folderSize, _Out_writes_bytes_(folderSize) char* folderResult) noexcept;

// Quota checks
STDAPI PFXGameSaveFilesGetRemainingQuota(_In_ PFXGameSaveConfigHandle, _Out_ int64_t* remainingQuota) noexcept;

// Upload
enum class PFXGameSaveUploadOptions : uint32_t
{
    UploadKeepActive = 0x0,
    UploadReleaseActive
};
STDAPI PFXGameSaveFilesUploadWithUiAsync(_In_ PFXGameSaveConfigHandle configHandle, _In_ PFXGameSaveUploadOptions options, _In_ XAsyncBlock* async) noexcept;
STDAPI PFXGameSaveFilesUploadWithUiResult(_In_ XAsyncBlock* async) noexcept;

// Set a Short Save Description of the pending changes, can be shown in the Conflict or Device Contention UX or returned in the PFXGameSaveDescriptor
STDAPI PFXGameSaveFilesSetSaveDescriptionAsync(_In_ PFXGameSaveConfigHandle configHandle, _In_ const char* shortSaveDescription, _In_ XAsyncBlock* async) noexcept;
STDAPI PFXGameSaveFilesSetSaveDescriptionResult(_In_ XAsyncBlock* async) noexcept;

// TCUI API with callbacks for progress and dialog handling
// NOTE: These are nearly identical to the XGameSave versions, but with PlayFab specific types.
// Both can be active at the same time and need to be separate.
enum class PFXGameSaveSyncState : uint32_t
{
    NotStarted           = 0x0,
    PreparingForDownload = 0x1,
    Downloading          = 0x2,
    PreparingForUpload   = 0x3,
    Uploading            = 0x4,
    SyncComplete         = 0x5
};

struct PFXGameSaveDescriptor
{
    // deviceType is a platform identifier, e.g. Xbox, PC, etc.
    _Field_z_ const char* deviceType;
    // deviceId is a unique identifier for the device, e.g. serial number
    _Field_z_ const char* deviceId;
    // friendlyName is a user friendly name for the device, e.g. "My Xbox One"
    _Field_z_ const char* friendlyName;
    // pfthumbnail.png - if the game saves it at the root of the Folder, otherwise null.
    _Field_z_ const char* thumbnailUri;
    // short description of the save, can be shown in the Conflict or Device Contention UX along with the thumbnail.
    _Field_z_ const char* shortSaveDescription;
    // relevant time of the descriptor (can differ depending upon state and usage)
    time_t time;
    // total bytes of the save
    uint64_t totalBytes;
    // size of the pending upload, if any.
    uint64_t uploadedBytes;
};

typedef void CALLBACK PFXGameSaveProgressUiCallback(_In_ XUserHandle requestingUser, _In_ PFXGameSaveSyncState syncState, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveSyncFailedUiCallback(_In_ XUserHandle requestingUser, _In_ PFXGameSaveSyncState syncState, _In_ HRESULT error, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveActiveDeviceContentionUiCallback(_In_ XUserHandle requestingUser, _In_ time_t localTime, _In_ time_t remoteTime, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveActiveDeviceContentionWithDescriptorUiCallback(_In_ XUserHandle requestingUser, _In_ PFXGameSaveDescriptor* localGameSave, _In_ PFXGameSaveDescriptor* remoteGameSave, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveConflictUiCallback(_In_ XUserHandle requestingUser, _In_ time_t localModifiedTime, _In_ time_t remoteModifiedTime, _In_ uint64_t localSize, _In_ uint64_t remoteSize, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveConflictWithDescriptorUiCallback(_In_ XUserHandle requestingUser, _In_ PFXGameSaveDescriptor* localGameSave, _In_ PFXGameSaveDescriptor* remoteGameSave, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveOutOfStorageUiCallback(_In_ XUserHandle requestingUser, _In_ bool isUserPartition, uint64_t requiredBytes, _In_opt_ void* context);
typedef void CALLBACK PFXGameSaveActiveDeviceChangedCallback(_In_ XUserHandle requestingUser, _In_ PFXGameSaveDescriptor* activeDevice, _In_opt_ void* context);

struct PFXGameSaveUiCallbacks
{
    PFXGameSaveProgressUiCallback* progressCallback;
    void* progressContext;
    PFXGameSaveSyncFailedUiCallback* syncFailedCallback;
    void* syncFailedContext;
    PFXGameSaveActiveDeviceContentionUiCallback* activeDeviceContentionCallback;
    void* activeDeviceContentionContext;
    PFXGameSaveConflictUiCallback* conflictCallback;
    void* conflictContext;
    PFXGameSaveOutOfStorageUiCallback* outOfStorageCallback;
    void* outOfStorageContext;
    PFXGameSaveActiveDeviceContentionWithDescriptorUiCallback* activeDeviceContentionWithDescriptorCallback;
    void* activeDeviceContentionWithDescriptorContext;
    PFXGameSaveConflictWithDescriptorUiCallback* conflictWithDescriptorCallback;
    void* conflictWithDescriptorContext;
};

STDAPI PFXGameSaveSetUiCallbacks(_In_ PFXGameSaveUiCallbacks* callbacks) noexcept;

STDAPI PFXGameSaveSetActiveDeviceChangedCallback(_In_ PFXGameSaveActiveDeviceChangedCallback* callback, _In_opt_ void* context) noexcept;

STDAPI PFXGameSaveProgressUiGetProgress(_In_ XUserHandle requestingUser, _Out_opt_ PFXGameSaveSyncState* syncState, _Out_opt_ uint64_t* current, _Out_opt_ uint64_t* total) noexcept;

enum class PFXGameSaveProgressUiResponse : uint32_t
{
    Cancel = 0
};

STDAPI PFXGameSaveSetProgressUiResponse(_In_ XUserHandle requestingUser, _In_ PFXGameSaveProgressUiResponse response) noexcept;

enum class PFXGameSaveSyncFailedUiResponse : uint32_t
{
    Cancel = 0,
    Retry = 1,
    UseOffline = 2
};

STDAPI PFXGameSaveSetSyncFailedUiResponse(_In_ XUserHandle requestingUser, _In_ PFXGameSaveSyncFailedUiResponse response) noexcept;

enum class PFXGameSaveActiveDeviceContentionUiResponse : uint32_t
{
    Cancel = 0,
    Retry = 1,
    SyncLastSavedData = 2
};

STDAPI PFXGameSaveSetActiveDeviceContentionUiResponse(_In_ XUserHandle requestingUser, _In_ PFXGameSaveActiveDeviceContentionUiResponse response) noexcept;

enum class PFXGameSaveConflictUiResponse : uint32_t
{
    Cancel = 0,
    TakeLocal = 1,
    TakeRemote = 2
};

STDAPI PFXGameSaveSetConflictUiResponse(_In_ XUserHandle requestingUser, _In_ PFXGameSaveConflictUiResponse response) noexcept;

enum class PFXGameSaveOutOfStorageUiResponse : uint32_t
{
    Cancel = 0,
    Retry = 1
};

STDAPI PFXGameSaveSetOutOfStorageUiResponse(_In_ XUserHandle requestingUser, _In_ PFXGameSaveOutOfStorageUiResponse response) noexcept;

} // extern "C"
