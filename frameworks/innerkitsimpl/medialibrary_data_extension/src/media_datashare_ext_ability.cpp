/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "Extension"

#include "media_datashare_ext_ability.h"

#include "app_mgr_client.h"
#include "cloud_media_asset_manager.h"
#include "cloud_sync_utils.h"
#include "dataobs_mgr_client.h"
#include "datashare_ext_ability_context.h"
#include "media_app_uri_permission_column.h"
#include "media_datashare_stub_impl.h"
#include "media_scanner_manager.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_data_manager.h"
#include "dfx_manager.h"
#include "dfx_timer.h"
#include "dfx_deprecated_perm_usage.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_uripermission_operations.h"
#include "heif_transcoding_check_utils.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "os_account_manager.h"
#include "runtime.h"
#include "singleton.h"
#include "uri_permission_manager_client.h"
#include "userfilemgr_uri.h"
#ifdef MEDIALIBRARY_SECURITY_OPEN
#include "sec_comp_kit.h"
#endif
#include "userfilemgr_uri.h"
#include "media_tool_permission_handler.h"
#include "grant_permission_handler.h"
#include "read_write_permission_handler.h"
#include "db_permission_handler.h"
#include "system_api_check_handler.h"
#include "userfilemgr_uri.h"
#ifdef MEDIALIBRARY_MTP_ENABLE
#include "media_mtp_manager.h"
#endif
#include "media_fuse_manager.h"
#include "highlight_column.h"
#include "mediatool_uri.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"
#include "media_permission_check.h"
#include "user_define_ipc.h"
#include "medialibrary_tracer.h"
#include "media_file_change_manager.h"
#include "product_info.h"
#include "photo_album_upload_status_operation.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::NativeRdb;
using namespace OHOS::DistributedKv;
using namespace OHOS::Media;
using namespace OHOS::DataShare;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::Media::IPC;

namespace OHOS {
namespace AbilityRuntime {
using namespace OHOS::AppExecFwk;
using DataObsMgrClient = OHOS::AAFwk::DataObsMgrClient;
const std::string PERM_CLOUD_SYNC_MANAGER = "ohos.permission.CLOUDFILE_SYNC_MANAGER";
constexpr const char *MTP_DISABLE = "persist.edm.mtp_server_disable";
static const set<OperationObject> PHOTO_ACCESS_HELPER_OBJECTS = {
    OperationObject::PAH_PHOTO,
    OperationObject::PAH_ALBUM,
    OperationObject::PAH_MAP,
    OperationObject::PAH_FORM_MAP,
    OperationObject::ANALYSIS_PHOTO_ALBUM,
    OperationObject::ANALYSIS_PHOTO_MAP,
    OperationObject::VISION_OCR,
    OperationObject::VISION_AESTHETICS,
    OperationObject::VISION_VIDEO_AESTHETICS,
    OperationObject::VISION_LABEL,
    OperationObject::VISION_VIDEO_LABEL,
    OperationObject::VISION_IMAGE_FACE,
    OperationObject::VISION_VIDEO_FACE,
    OperationObject::VISION_FACE_TAG,
    OperationObject::VISION_OBJECT,
    OperationObject::VISION_RECOMMENDATION,
    OperationObject::VISION_SEGMENTATION,
    OperationObject::VISION_COMPOSITION,
    OperationObject::VISION_SALIENCY,
    OperationObject::VISION_HEAD,
    OperationObject::VISION_AFFECTIVE,
    OperationObject::VISION_POSE,
    OperationObject::VISION_TOTAL,
    OperationObject::VISION_VIDEO_TOTAL,
    OperationObject::VISION_PET_FACE,
    OperationObject::VISION_PET_TAG,
    OperationObject::VISION_ANALYSIS_ALBUM_TOTAL,
    OperationObject::GEO_DICTIONARY,
    OperationObject::GEO_KNOWLEDGE,
    OperationObject::GEO_PHOTO,
    OperationObject::PAH_MULTISTAGES_CAPTURE,
    OperationObject::STORY_ALBUM,
    OperationObject::STORY_COVER,
    OperationObject::STORY_PLAY,
    OperationObject::HIGHLIGHT_DELETE,
    OperationObject::USER_PHOTOGRAPHY,
    OperationObject::ANALYSIS_PROGRESS,
    OperationObject::PAH_BATCH_THUMBNAIL_OPERATE,
    OperationObject::INDEX_CONSTRUCTION_STATUS,
    OperationObject::MEDIA_APP_URI_PERMISSION,
    OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE,
    OperationObject::ANALYSIS_ASSET_SD_MAP,
    OperationObject::ANALYSIS_ALBUM_ASSET_MAP,
    OperationObject::CLOUD_MEDIA_ASSET_OPERATE,
    OperationObject::PAH_BACKUP_POSTPROCESS,
    OperationObject::VISION_ANALYSIS,
};

MediaDataShareExtAbility* MediaDataShareExtAbility::Create(const unique_ptr<Runtime>& runtime)
{
    return new MediaDataShareExtAbility(static_cast<Runtime&>(*runtime));
}

MediaDataShareExtAbility::MediaDataShareExtAbility(Runtime& runtime) : DataShareExtAbility(), runtime_(runtime) {}

MediaDataShareExtAbility::~MediaDataShareExtAbility()
{
}

void MediaDataShareExtAbility::Init(const shared_ptr<AbilityLocalRecord> &record,
    const shared_ptr<OHOSApplication> &application, shared_ptr<AbilityHandler> &handler,
    const sptr<IRemoteObject> &token)
{
    if ((record == nullptr) || (application == nullptr) || (handler == nullptr) || (token == nullptr)) {
        MEDIA_ERR_LOG("MediaDataShareExtAbility::init failed, some object is nullptr");
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    DataShareExtAbility::Init(record, application, handler, token);
}

static bool IsStartBeforeUserUnlock()
{
    int32_t activeUserId = 0;
    ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(activeUserId);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, false,
        "GetForegroundOsAccountLocalId fail, ret code %{public}d, result is not credible", ret);
    MEDIA_INFO_LOG("Current active account is %{public}d ", activeUserId);
    bool isAccountVerified = true; // Assume verified to avoid unknown killing
    ErrCode err = AccountSA::OsAccountManager::IsOsAccountVerified(activeUserId, isAccountVerified);
    CHECK_AND_RETURN_RET_LOG(err == ERR_OK, false,
        "Check activeUserId fail caused by %{public}d, check result is not credible", err);
    MEDIA_INFO_LOG("Current user verification result: %{public}d", isAccountVerified);
    return !isAccountVerified;
}

void MediaDataShareExtAbility::InitPermissionHandler()
{
    MEDIA_DEBUG_LOG("InitPermissionHandler begin");
    // 构造鉴权处理器责任链
    auto mediaToolPermissionHandler = std::make_shared<MediaToolPermissionHandler>();
    auto grantPermissionHandler = std::make_shared<GrantPermissionHandler>();
    grantPermissionHandler->SetNextHandler(mediaToolPermissionHandler);
    auto dbPermissionHandler = std::make_shared<DbPermissionHandler>();
    dbPermissionHandler->SetNextHandler(grantPermissionHandler);
    auto rwPermissionHandler = std::make_shared<ReadWritePermissionHandler>();
    rwPermissionHandler->SetNextHandler(dbPermissionHandler);
    permissionHandler_ = std::make_shared<SystemApiCheckHandler>();
    permissionHandler_->SetNextHandler(rwPermissionHandler);
    MEDIA_DEBUG_LOG("InitPermissionHandler end:permissionHandler_=%{public}d", permissionHandler_ != nullptr);
}

void MediaDataShareExtAbility::OnStartSub(const AAFwk::Want &want)
{
    MediaLibraryTracer tracer;
    tracer.Start("OnStartSub");
#ifdef MEDIALIBRARY_MTP_ENABLE
    MediaMtpManager::GetInstance().Init();
#endif
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    EnhancementManager::GetInstance().InitAsync();
#endif
}

static bool CheckUnlockScene(int64_t startTime)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckUnlockScene");
    if (IsStartBeforeUserUnlock()) {
        DfxReporter::ReportStartResult(DfxType::CHECK_USER_UNLOCK_FAIL, 0, startTime);
        MEDIA_INFO_LOG("%{public}s Killing self caused by booting before unlocking", __func__);
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return false;
    }

    bool isMediaPathExists = MediaFileUtils::IsDirectory(ROOT_MEDIA_DIR);
    if (!isMediaPathExists) {
        DfxReporter::ReportStartResult(DfxType::CHECK_MEDIA_PATH_UNLOCK_FAIL, 0, startTime);
        MEDIA_INFO_LOG("%{public}s Killing self caused by media path unmounted", __func__);
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return false;
    }

    return true;
}

static void RestartCloudMediaAssetDownload()
{
    std::thread([&] {
        MEDIA_INFO_LOG("enter RestartCloudMediaAssetDownload.");
        CHECK_AND_RETURN_INFO_LOG(CloudSyncUtils::IsCloudSyncSwitchOn(), "Cloud sync switch off");
        if (!CloudSyncUtils::IsCloudDataAgingPolicyOn()) {
            CloudMediaAssetManager::GetInstance().CancelDownloadCloudAsset();
            return;
        }
        CloudMediaAssetManager::GetInstance().StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    }).detach();
}

void MediaDataShareExtAbility::OnStart(const AAFwk::Want &want)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    Extension::OnStart(want);
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        DfxReporter::ReportStartResult(DfxType::START_CONTEXT_FAIL, 0, startTime);
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    MEDIA_INFO_LOG("%{public}s runtime language  %{public}d", __func__, runtime_.GetLanguage());
    InitPermissionHandler();
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        DfxReporter::ReportStartResult(DfxType::START_DATAMANAGER_FAIL, 0, startTime);
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }

    CHECK_AND_RETURN(CheckUnlockScene(startTime));
    auto extensionContext = GetContext();
    int32_t sceneCode = DfxType::START_SUCCESS;
    int32_t ret = dataManager->InitMediaLibraryMgr(context, extensionContext, sceneCode, true, true);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to init MediaLibraryMgr");
        CHECK_AND_EXECUTE(sceneCode != DfxType::START_RDB_STORE_FAIL,
            DfxReporter::ReportStartResult(sceneCode, ret, startTime));
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    dataManager->SetOwner(static_pointer_cast<MediaDataShareExtAbility>(shared_from_this()));

    // Start media fuse daemon
    MediaFuseManager::GetInstance().Start();

    DfxManager::GetInstance();
    auto scannerManager = MediaScannerManager::GetInstance();
    if (scannerManager == nullptr) {
        DfxReporter::ReportStartResult(DfxType::START_SCANNER_FAIL, 0, startTime);
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    OnStartSub(want);
    Media::MedialibrarySubscriber::SubscribeAsync();
    Media::HeifTranscodingCheckUtils::InitCheckList();
    dataManager->SetStartupParameter();
    DfxReporter::ReportStartResult(DfxType::START_SUCCESS, 0, startTime);
    CloudMediaAssetManager::GetInstance().RestartForceRetainCloudAssets();
    dataManager->RestoreInvalidHDCCloudDataPos();
    PhotoAlbumUploadStatusOperation::JudgeUploadAlbumEnable();
}

void MediaDataShareExtAbility::OnStop()
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto scannerManager = MediaScannerManager::GetInstance();
    CHECK_AND_EXECUTE(scannerManager == nullptr, scannerManager->Stop());

    Media::HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    MediaFuseManager::GetInstance().Stop();
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MedialibraryAppStateObserverManager::GetInstance().UnSubscribeAppState();
    Media::MedialibrarySubscriber::UnSubscribe();
    MEDIA_INFO_LOG("%{public}s end.", __func__);
}

sptr<IRemoteObject> MediaDataShareExtAbility::OnConnect(const AAFwk::Want &want)
{
    MEDIA_INFO_LOG("%{public}s begin. ", __func__);
    Extension::OnConnect(want);
    sptr<MediaDataShareStubImpl> remoteObject = new (nothrow) MediaDataShareStubImpl(
        static_pointer_cast<MediaDataShareExtAbility>(shared_from_this()),
        nullptr);
    if (remoteObject == nullptr) {
        MEDIA_ERR_LOG("%{public}s No memory allocated for DataShareStubImpl", __func__);
        return nullptr;
    }
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return remoteObject->AsObject();
}

vector<string> MediaDataShareExtAbility::GetFileTypes(const Uri &uri, const string &mimeTypeFilter)
{
    vector<string> ret;
    return ret;
}

static void FillV10Perms(const MediaType mediaType, const bool containsRead, const bool containsWrite,
    vector<string> &perm)
{
    if (containsRead) {
        if (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO ||
            mediaType == Media::MEDIA_TYPE_PHOTO || mediaType == Media::MEDIA_TYPE_ALBUM) {
            perm.push_back(PERM_READ_IMAGEVIDEO);
        } else if (mediaType == MEDIA_TYPE_AUDIO) {
            perm.push_back(PERM_READ_AUDIO);
        } else if (mediaType == MEDIA_TYPE_FILE) {
            perm.push_back(PERM_READ_IMAGEVIDEO);
            perm.push_back(PERM_READ_AUDIO);
            perm.push_back(PERM_READ_DOCUMENT);
        }
    }
    if (containsWrite) {
        if (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO ||
            mediaType == Media::MEDIA_TYPE_PHOTO || mediaType == Media::MEDIA_TYPE_ALBUM) {
            perm.push_back(PERM_WRITE_IMAGEVIDEO);
        } else if (mediaType == MEDIA_TYPE_AUDIO) {
            perm.push_back(PERM_WRITE_AUDIO);
        } else if (mediaType == MEDIA_TYPE_FILE) {
            perm.push_back(PERM_WRITE_IMAGEVIDEO);
            perm.push_back(PERM_WRITE_AUDIO);
            perm.push_back(PERM_WRITE_DOCUMENT);
        }
    }
}

static void FillDeprecatedPerms(const bool containsRead, const bool containsWrite, vector<string> &perm)
{
    CHECK_AND_EXECUTE(!containsRead, perm.push_back(PERMISSION_NAME_READ_MEDIA));
    CHECK_AND_EXECUTE(!containsWrite, perm.push_back(PERMISSION_NAME_WRITE_MEDIA));
}

static inline bool ContainsFlag(const string &mode, const char flag)
{
    return mode.find(flag) != string::npos;
}

static void CollectPermissionInfo(MediaLibraryCommand &cmd, const string &mode,
    const bool permGranted, PermissionUsedType type)
{
    if ((cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL_ASTC)) {
        CHECK_AND_EXECUTE(mode.find("r") == string::npos,
            PermissionUtils::CollectPermissionInfo(PERM_READ_IMAGEVIDEO, permGranted, type));
        CHECK_AND_EXECUTE(mode.find("w") == string::npos,
            PermissionUtils::CollectPermissionInfo(PERM_WRITE_IMAGEVIDEO, permGranted, type));
    }
}

static int32_t CheckOpenFilePermission(MediaLibraryCommand &cmd, string &mode)
{
    MEDIA_DEBUG_LOG("uri: %{private}s mode: %{private}s", cmd.GetUri().ToString().c_str(), mode.c_str());
    MediaType mediaType = MediaFileUri::GetMediaTypeFromUri(cmd.GetUri().ToString());
    const bool containsRead = ContainsFlag(mode, 'r');
    const bool containsWrite = ContainsFlag(mode, 'w');

    CHECK_AND_RETURN_RET(cmd.GetQuerySetParam(IS_TOOL_OPEN) != TOOL_OPEN_TRUE,
        IsDeveloperMediaTool(cmd, mode)? E_SUCCESS : E_PERMISSION_DENIED);
    vector<string> perms;
    FillV10Perms(mediaType, containsRead, containsWrite, perms);
    bool cond = ((cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL_ASTC));
    CHECK_AND_RETURN_RET(!cond, PermissionUtils::CheckPhotoCallerPermission(perms)? E_SUCCESS : E_PERMISSION_DENIED);

    int32_t err = (mediaType == MEDIA_TYPE_FILE) ?
        (PermissionUtils::CheckHasPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED) :
        (PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED);
    CHECK_AND_RETURN_RET(err != E_SUCCESS, E_SUCCESS);
    // Try to check deprecated permissions
    perms.clear();
    FillDeprecatedPerms(containsRead, containsWrite, perms);
    bool ret = PermissionUtils::CheckCallerPermission(perms);
    CHECK_AND_EXECUTE(!ret,
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType())));
    return ret ? E_SUCCESS : E_PERMISSION_DENIED;
}

static inline void AddHiddenAlbumPermission(MediaLibraryCommand &cmd, vector<string> &outPerms)
{
    Media::OperationType type = cmd.GetOprnType();
    if (type == Media::OperationType::QUERY_HIDDEN) {
        outPerms.push_back(PERM_MANAGE_PRIVATE_PHOTOS);
    }
}

static int32_t SystemApiCheck(MediaLibraryCommand &cmd)
{
    static const set<OperationObject> SYSTEM_API_OBJECTS = {
        OperationObject::UFM_PHOTO,
        OperationObject::UFM_AUDIO,
        OperationObject::UFM_ALBUM,
        OperationObject::UFM_MAP,
        OperationObject::SMART_ALBUM,

        OperationObject::ALL_DEVICE,
        OperationObject::ACTIVE_DEVICE,
        OperationObject::PAH_FORM_MAP,
    };

    static const set<string> SYSTEM_API_URIS = {
        // Deleting asset permanently from system is only allowed for system apps.
        URI_DELETE_PHOTOS,
        // Deleting asset to trash album directly without a pop-up box is only allowed for system apps.
        UFM_DELETE_PHOTOS,
        PAH_DELETE_PHOTOS,
    };

    OperationObject obj = cmd.GetOprnObject();
    string uri = cmd.GetUriStringWithoutSegment();
    if (SYSTEM_API_OBJECTS.find(obj) != SYSTEM_API_OBJECTS.end() ||
        (SYSTEM_API_URIS.find(uri) != SYSTEM_API_URIS.end())) {
        CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsSystemApp(), -E_CHECK_SYSTEMAPP_FAIL,
            "Systemapi should only be called by system applications!");
    }
    return E_SUCCESS;
}

static inline int32_t HandleMediaVolumePerm(const MediaLibraryCommand &cmd)
{
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_READ_MEDIA);
    CHECK_AND_EXECUTE(!ret,
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType())));
    return ret ? E_SUCCESS : E_PERMISSION_DENIED;
}

static inline int32_t HandleBundlePermCheck(const MediaLibraryCommand &cmd)
{
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_WRITE_MEDIA);
    if (ret) {
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType()));
        return E_SUCCESS;
    }

    return PermissionUtils::CheckHasPermission(WRITE_PERMS_V10) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t HandleNoPermCheck(MediaLibraryCommand &cmd)
{
    static const set<string> NO_NEED_PERM_CHECK_URI = {
        URI_CLOSE_FILE,
        MEDIALIBRARY_DIRECTORY_URI,
    };

    static const set<OperationObject> NO_NEED_PERM_CHECK_OBJ = {
        OperationObject::ALL_DEVICE,
        OperationObject::ACTIVE_DEVICE,
        OperationObject::MISCELLANEOUS
    };

    string uri = cmd.GetUri().ToString();
    OperationObject obj = cmd.GetOprnObject();
    if (NO_NEED_PERM_CHECK_URI.find(uri) != NO_NEED_PERM_CHECK_URI.end() ||
        NO_NEED_PERM_CHECK_OBJ.find(obj) != NO_NEED_PERM_CHECK_OBJ.end()) {
        return E_SUCCESS;
    }
    return E_NEED_FURTHER_CHECK;
}

static int32_t HandleSecurityComponentPermission(MediaLibraryCommand &cmd)
{
    if (cmd.GetUri().ToString().find(OPRN_CREATE_COMPONENT) != string::npos ||
        cmd.GetUri().ToString().find(OPRN_SAVE_CAMERA_PHOTO_COMPONENT) != string::npos) {
#ifdef MEDIALIBRARY_SECURITY_OPEN
        auto tokenId = PermissionUtils::GetTokenId();
        CHECK_AND_RETURN_RET_LOG(Security::SecurityComponent::SecCompKit::VerifySavePermission(tokenId),
            E_NEED_FURTHER_CHECK, "Failed to verify save permission of security component");
        return E_SUCCESS;
#else
        MEDIA_ERR_LOG("Security component is not existed");
        return E_NEED_FURTHER_CHECK;
#endif
    }
    return E_NEED_FURTHER_CHECK;
}

static int32_t HandleShortPermission(const MediaLibraryCommand &cmd, bool &need)
{
    need = false;
    if (cmd.GetUriStringWithoutSegment() == PAH_CREATE_PHOTO ||
        cmd.GetUriStringWithoutSegment() == PAH_SYS_CREATE_PHOTO) {
        if (PermissionUtils::CheckPhotoCallerPermission(PERM_SHORT_TERM_WRITE_IMAGEVIDEO)) {
            need = true;
            return E_SUCCESS;
        }
    }
    return E_PERMISSION_DENIED;
}

static int32_t HandleRestorePermission(MediaLibraryCommand &cmd)
{
    std::string uri = cmd.GetUriStringWithoutSegment();
    if (uri == PAH_GENERATE_THUMBNAILS_RESTORE ||uri == PAH_RESTORE_INVALID_HDC_CLOUD_DATA_POS) {
        return PermissionUtils::CheckCallerPermission(PERM_READ_IMAGEVIDEO) ? E_SUCCESS : E_PERMISSION_DENIED;
    }
    return E_PERMISSION_DENIED;
}

static int32_t UserFileMgrPermissionCheck(MediaLibraryCommand &cmd, const bool isWrite)
{
    static const set<OperationObject> USER_FILE_MGR_OBJECTS = {
        OperationObject::UFM_PHOTO,
        OperationObject::UFM_AUDIO,
        OperationObject::UFM_ALBUM,
        OperationObject::UFM_MAP,
    };

    OperationObject obj = cmd.GetOprnObject();
    if (USER_FILE_MGR_OBJECTS.find(obj) == USER_FILE_MGR_OBJECTS.end()) {
        return E_NEED_FURTHER_CHECK;
    }

    int32_t err = HandleSecurityComponentPermission(cmd);
    bool cond = (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK));
    CHECK_AND_RETURN_RET(!cond, err);

    vector<string> perms;
    if (obj == OperationObject::UFM_AUDIO) {
        perms.push_back(isWrite ? PERM_WRITE_AUDIO : PERM_READ_AUDIO);
    } else {
        perms.push_back(isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO);
    }
    AddHiddenAlbumPermission(cmd, perms);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t PhotoAccessHelperPermCheck(MediaLibraryCommand &cmd, const bool isWrite)
{
    int32_t err = HandleSecurityComponentPermission(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (PHOTO_ACCESS_HELPER_OBJECTS.find(obj) == PHOTO_ACCESS_HELPER_OBJECTS.end()) {
        return E_NEED_FURTHER_CHECK;
    }
    vector<string> perms;
    AddHiddenAlbumPermission(cmd, perms);
    perms.push_back(isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t HandleSpecialObjectPermission(MediaLibraryCommand &cmd, bool isWrite)
{
    int err = HandleNoPermCheck(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (obj == OperationObject::MEDIA_VOLUME) {
        return HandleMediaVolumePerm(cmd);
    } else if (obj == OperationObject::BUNDLE_PERMISSION) {
        return HandleBundlePermCheck(cmd);
    }

    return E_NEED_FURTHER_CHECK;
}

static void UnifyOprnObject(MediaLibraryCommand &cmd)
{
    static const unordered_map<OperationObject, OperationObject> UNIFY_OP_OBJECT_MAP = {
        { OperationObject::UFM_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::UFM_AUDIO, OperationObject::FILESYSTEM_AUDIO },
        { OperationObject::UFM_ALBUM, OperationObject::PHOTO_ALBUM },
        { OperationObject::UFM_MAP, OperationObject::PHOTO_MAP },
        { OperationObject::PAH_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::PAH_ALBUM, OperationObject::PHOTO_ALBUM },
        { OperationObject::PAH_MAP, OperationObject::PHOTO_MAP },
        { OperationObject::TOOL_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::TOOL_AUDIO, OperationObject::FILESYSTEM_AUDIO },
        { OperationObject::TOOL_ALBUM, OperationObject::PHOTO_ALBUM },
    };

    OperationObject obj = cmd.GetOprnObject();
    if (UNIFY_OP_OBJECT_MAP.find(obj) != UNIFY_OP_OBJECT_MAP.end()) {
        cmd.SetOprnObject(UNIFY_OP_OBJECT_MAP.at(obj));
    }
}

static int32_t MediatoolPermCheck(MediaLibraryCommand &cmd)
{
    if (IsMediatoolOperation(cmd)) {
        CHECK_AND_RETURN_RET(IsDeveloperMediaTool(cmd), E_PERMISSION_DENIED);
        return E_SUCCESS;
    } else {
        return E_NEED_FURTHER_CHECK;
    }
}

static int32_t CheckPermFromUri(MediaLibraryCommand &cmd, bool isWrite)
{
    MEDIA_DEBUG_LOG("uri: %{private}s object: %{public}d, opType: %{public}d isWrite: %{public}d",
        cmd.GetUri().ToString().c_str(), cmd.GetOprnObject(), cmd.GetOprnType(), isWrite);

    int err = SystemApiCheck(cmd);
    CHECK_AND_RETURN_RET(err == E_SUCCESS, err);
    err = MediatoolPermCheck(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = PhotoAccessHelperPermCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = UserFileMgrPermissionCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = HandleSpecialObjectPermission(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }

    // Finally, we should check the permission of medialibrary interfaces.
    string perm = isWrite ? PERMISSION_NAME_WRITE_MEDIA : PERMISSION_NAME_READ_MEDIA;
    err = PermissionUtils::CheckCallerPermission(perm) ? E_SUCCESS : E_PERMISSION_DENIED;
    CHECK_AND_RETURN_RET(err >= 0, err);
    DfxDeprecatedPermUsage::Record(
        static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType()));
    UnifyOprnObject(cmd);
    return E_SUCCESS;
}

static bool AddOwnerCheck(MediaLibraryCommand &cmd, DataSharePredicates &appidPredicates)
{
    bool cond = (cmd.GetTableName() != PhotoColumn::PHOTOS_TABLE &&
        cmd.GetTableName() != AudioColumn::AUDIOS_TABLE && cmd.GetTableName() != MEDIALIBRARY_TABLE);
    CHECK_AND_RETURN_RET(!cond, false);
    string clientAppId = GetClientAppId();
    if (clientAppId.empty()) {
        return false;
    }
    appidPredicates.And()->EqualTo("owner_appid", clientAppId);
    return true;
}

static bool AddOwnerCheck(MediaLibraryCommand &cmd, DataSharePredicates &tokenIdPredicates, vector<string> &columns)
{
    if (cmd.GetTableName() != PhotoColumn::PHOTOS_TABLE && cmd.GetTableName() != AudioColumn::AUDIOS_TABLE &&
        cmd.GetTableName() != MEDIALIBRARY_TABLE) {
        return false;
    }
    uint32_t tokenid = PermissionUtils::GetTokenId();
    string onClause = cmd.GetTableName() + "." + MediaColumn::MEDIA_ID + " = " +
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "." + AppUriPermissionColumn::FILE_ID;
    vector<string> clauses = { onClause };
    tokenIdPredicates.InnerJoin(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE)->On(clauses);
    tokenIdPredicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, to_string(tokenid));
    MediaLibraryRdbUtils::CleanAmbiguousColumn(columns, tokenIdPredicates, PhotoColumn::PHOTOS_TABLE);
    return true;
}

static uint32_t GetFlagFromMode(const string &mode)
{
    if (mode.find("w") != string::npos) {
        return AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION;
    }
    return AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION;
}

int MediaDataShareExtAbility::CheckPermissionForOpenFile(const Uri &uri,
    MediaLibraryCommand &command, string &unifyMode)
{
    auto object = command.GetOprnObject();
    CHECK_AND_RETURN_RET(object != OperationObject::FILESYSTEM_DEBUG_DB, static_cast<int>(E_SUCCESS));
    PermParam permParam = {
        .isWrite = false,
        .isOpenFile = true,
        .openFileNode = unifyMode
    };
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    int err = permissionHandler_->CheckPermission(command, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    CHECK_AND_EXECUTE(err == E_SUCCESS, err = CheckOpenFilePermission(command, unifyMode));

    if (err == E_PERMISSION_DENIED) {
        MEDIA_ERR_LOG("Permission Denied! err = %{public}d", err);
        CollectPermissionInfo(command, unifyMode, false,
            PermissionUsedTypeValue::SECURITY_COMPONENT_TYPE);
    }
    return err;
}

int MediaDataShareExtAbility::OpenFile(const Uri &uri, const string &mode)
{
#ifdef MEDIALIBRARY_COMPATIBILITY
    string realUriStr = MediaFileUtils::GetRealUriFromVirtualUri(uri.ToString());
    Uri realUri(realUriStr);
    MediaLibraryCommand command(realUri, Media::OperationType::OPEN);

#else
    MediaLibraryCommand command(uri, Media::OperationType::OPEN);
#endif

    string unifyMode = mode;
    transform(unifyMode.begin(), unifyMode.end(), unifyMode.begin(), ::tolower);
    int err = CheckPermissionForOpenFile(uri, command, unifyMode);
    CHECK_AND_RETURN_RET_LOG(err >= 0, err, "permission deny: %{public}d", err);
    int32_t object = static_cast<int32_t>(command.GetOprnObject());
    int32_t type = static_cast<int32_t>(command.GetOprnType());
    DfxTimer dfxTimer(type, object, OPEN_FILE_TIME_OUT, true);

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(MEDIA_DATA_DB_THUMBNAIL) == string::npos,
        command.SetOprnObject(OperationObject::THUMBNAIL));

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(MEDIA_DATA_DB_THUMB_ASTC) == string::npos,
        command.SetOprnObject(OperationObject::THUMBNAIL_ASTC));

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(PhotoColumn::PHOTO_CACHE_URI_PREFIX) == string::npos,
        command.SetOprnObject(OperationObject::FILESYSTEM_PHOTO));

    if (command.GetUri().ToString().find(PhotoColumn::HIGHTLIGHT_URI) != string::npos) {
        command.SetOprnObject(OperationObject::HIGHLIGHT_URI);
    } else if (command.GetUri().ToString().find(MEDIA_DATA_DB_HIGHLIGHT) != string::npos) {
        command.SetOprnObject(OperationObject::HIGHLIGHT_COVER);
    }

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(PhotoColumn::PHOTO_REQUEST_PICTURE) == string::npos,
        command.SetOprnObject(OperationObject::REQUEST_PICTURE));

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(PhotoColumn::PHOTO_REQUEST_PICTURE_BUFFER) == string::npos,
        command.SetOprnObject(OperationObject::PHOTO_REQUEST_PICTURE_BUFFER));

    CHECK_AND_EXECUTE(command.GetUri().ToString().find(MEDIA_DATA_DB_KEY_FRAME) == string::npos,
        command.SetOprnObject(OperationObject::KEY_FRAME));
    return MediaLibraryDataManager::GetInstance()->OpenFile(command, unifyMode);
}

int MediaDataShareExtAbility::OpenRawFile(const Uri &uri, const string &mode)
{
    return 0;
}

int MediaDataShareExtAbility::Insert(const Uri &uri, const DataShareValuesBucket &value)
{
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    CHECK_AND_RETURN_RET(err == E_SUCCESS, err);
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, value);
}

int MediaDataShareExtAbility::InsertExt(const Uri &uri, const DataShareValuesBucket &value, string &result)
{
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    bool needToResetTime = false;
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_INFO_LOG("OperationObject=%{public}d, permissionHandler_ err=%{public}d", cmd.GetOprnObject(), err);
    CHECK_AND_RETURN_RET(err != -E_CHECK_SYSTEMAPP_FAIL, err);
    CHECK_AND_EXECUTE(err == E_SUCCESS, err = HandleShortPermission(cmd, needToResetTime));

    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    if (err < 0) {
        MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
        return err;
    }

    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    int32_t ret =  MediaLibraryDataManager::GetInstance()->InsertExt(cmd, value, result);
    if (needToResetTime) {
        AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
        err = Security::AccessToken::AccessTokenKit::GrantPermissionForSpecifiedTime(tokenCaller,
            PERM_SHORT_TERM_WRITE_IMAGEVIDEO, SHORT_TERM_PERMISSION_DURATION_300S);
        CHECK_AND_RETURN_RET_LOG(err >= 0, err, "queryResultSet is nullptr! errCode: %{public}d", err);
    }
    return ret;
}

static bool CheckCloudSyncPermission()
{
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::CheckCallerPermission(PERM_CLOUD_SYNC_MANAGER),
        false, "permission denied");
    return true;
}

int MediaDataShareExtAbility::Update(const Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value)
{
    MediaLibraryCommand cmd(uri);
    if (cmd.GetOprnObject() == OperationObject::CLOUD_MEDIA_ASSET_OPERATE) {
        if (!CheckCloudSyncPermission()) {
            return E_PERMISSION_DENIED;
        }
        return CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmd);
    }
    PermParam permParam = {
        .isWrite = true,
    };
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    cmd.SetDataSharePred(predicates);
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    CHECK_AND_RETURN_RET(err != -E_CHECK_SYSTEMAPP_FAIL, err);
    if (err != E_SUCCESS) {
        err = HandleRestorePermission(cmd);
    }
    bool isMediatoolOperation = IsMediatoolOperation(cmd);
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());

    DataSharePredicates appidPredicates = predicates;
    if (err != E_SUCCESS) {
        if (isMediatoolOperation) {
            MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
            return err;
        }
        if (!AddOwnerCheck(cmd, appidPredicates)) {
            MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
            return err;
        }
    }

    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    auto updateRet = MediaLibraryDataManager::GetInstance()->Update(cmd, value, appidPredicates);
    bool cond = (err < 0 && updateRet <= 0);
    CHECK_AND_RETURN_RET_LOG(!cond, err, "permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
    return updateRet;
}

int MediaDataShareExtAbility::Delete(const Uri &uri, const DataSharePredicates &predicates)
{
    MediaLibraryCommand cmd(uri, Media::OperationType::DELETE);
    PermParam permParam = {
        .isWrite = true,
    };
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    cmd.SetDataSharePred(predicates);
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    if (err != E_SUCCESS) {
        MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
        return err;
    }

    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

shared_ptr<DataShareResultSet> MediaDataShareExtAbility::Query(const Uri &uri,
    const DataSharePredicates &predicates, vector<string> &columns, DatashareBusinessError &businessError)
{
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {.isWrite = false};
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, nullptr, "permissionHandler_ is nullptr");
    cmd.SetDataSharePred(predicates);
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    bool isMediatoolOperation = IsMediatoolOperation(cmd);
    int errCode = businessError.GetCode();
    DataSharePredicates appidPredicates = predicates;
    if (err != E_SUCCESS) {
        if (isMediatoolOperation) {
            MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
            businessError.SetCode(err);
            return nullptr;
        }
        auto& uriPermissionClient = AAFwk::UriPermissionManagerClient::GetInstance();
        if (!AddOwnerCheck(cmd, appidPredicates, columns)) {
            MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
            businessError.SetCode(err);
            return nullptr;
        }
    }
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, appidPredicates, errCode);
    businessError.SetCode(to_string(errCode));
    if (queryResultSet == nullptr) {
        MEDIA_ERR_LOG("queryResultSet is nullptr! errCode: %{public}d", errCode);
        businessError.SetCode(errCode);
        return nullptr;
    }
    auto count = 0;
    queryResultSet->GetRowCount(count);
    if (err < 0 && count == 0) {
        businessError.SetCode(err);
        return nullptr;
    }
    shared_ptr<DataShareResultSet> resultSet = make_shared<DataShareResultSet>(queryResultSet);
    return resultSet;
}

string MediaDataShareExtAbility::GetType(const Uri &uri)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {.isWrite = false};
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, "", "permissionHandler_ is nullptr");
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    if (err != E_SUCCESS) {
        MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
        return "";
    }
    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    string getTypeRet = MediaLibraryDataManager::GetInstance()->GetType(uri);
    return getTypeRet;
}

int MediaDataShareExtAbility::BatchInsert(const Uri &uri, const vector<DataShareValuesBucket> &values)
{
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_PERMISSION_DENIED, "permissionHandler_ is nullptr");
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    MEDIA_DEBUG_LOG("permissionHandler_ err=%{public}d", err);
    int32_t type = static_cast<int32_t>(cmd.GetOprnType());
    int32_t object = static_cast<int32_t>(cmd.GetOprnObject());
    if (err != E_SUCCESS) {
        MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, err);
        return err;
    }
    DfxTimer dfxTimer(type, object, COMMON_TIME_OUT, true);
    return MediaLibraryDataManager::GetInstance()->BatchInsert(cmd, values);
}

bool MediaDataShareExtAbility::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto obsMgrClient = DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient is nullptr", __func__);
        return false;
    }

    ErrCode ret = obsMgrClient->RegisterObserver(uri, dataObserver);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, false,
        "%{public}s obsMgrClient->RegisterObserver error return %{public}d", __func__, ret);
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return true;
}

bool MediaDataShareExtAbility::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto obsMgrClient = DataObsMgrClient::GetInstance();
    CHECK_AND_RETURN_RET_LOG(obsMgrClient != nullptr, false, "%{public}s obsMgrClient is nullptr", __func__);

    ErrCode ret = obsMgrClient->UnregisterObserver(uri, dataObserver);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, false,
        "%{public}s obsMgrClient->UnregisterObserver error return %{public}d", __func__, ret);
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return true;
}

bool MediaDataShareExtAbility::NotifyChange(const Uri &uri)
{
    auto obsMgrClient = DataObsMgrClient::GetInstance();
    CHECK_AND_RETURN_RET_LOG(obsMgrClient != nullptr, false, "%{public}s obsMgrClient is nullptr", __func__);

    ErrCode ret = obsMgrClient->NotifyChange(uri);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, false,
        "%{public}s obsMgrClient->NotifyChange error return %{public}d", __func__, ret);
    return true;
}

Uri MediaDataShareExtAbility::NormalizeUri(const Uri &uri)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto ret = uri;
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return ret;
}

Uri MediaDataShareExtAbility::DenormalizeUri(const Uri &uri)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto ret = uri;
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return ret;
}

int32_t MediaDataShareExtAbility::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(permissionHandler_ != nullptr, E_ERR, "permissionHandler_ is nullptr");
    IPC::MediaReqVo<IPC::MediaEmptyObjVo> reqVo;
    bool retReq = reqVo.Unmarshalling(data);
    CHECK_AND_RETURN_RET_LOG(retReq, E_IPC_SEVICE_UNMARSHALLING_FAIL, "read reqVo from parcel failed");
    uint32_t operationCode = reqVo.GetCode();
    std::string traceId = reqVo.GetTraceId();
    int32_t userId = reqVo.GetUserId();
    std::unordered_map<std::string, std::string> headerMap = reqVo.GetHeader();
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = E_IPC_SEVICE_NOT_FOUND;
    for (auto &controllerService : this->serviceFactory_.GetAllMediaControllerService()) {
        if (!controllerService->Accept(operationCode)) {
            continue;
        }

        std::vector<std::vector<PermissionType>> permissionPolicy;
        bool isDBBypass = false;
        if (controllerService->GetPermissionPolicy(operationCode, permissionPolicy, isDBBypass)) {
            ret = IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
            break;
        }
        PermissionHeaderReq permHeaderReq = PermissionHeaderReq::convertToPermissionHeaderReq(headerMap,
            userId, permissionPolicy, isDBBypass);
        int32_t errCode = PermissionCheck::VerifyPermissions(operationCode, permHeaderReq);
        if (errCode != E_SUCCESS && errCode != E_PERMISSION_DB_BYPASS && errCode != E_DOUBLE_CHECK) {
            ret = IPC::UserDefineIPC().WriteResponseBody(reply, errCode);
            break;
        }
        MEDIA_INFO_LOG("API code %{public}d verify permission success", operationCode);
        IPCContext context(option, errCode);
        context.SetHeader(headerMap);
        ret = controllerService->OnRemoteRequest(operationCode, data, reply, context);
        break;
    }
    if (ret != E_SUCCESS) {
        MEDIA_INFO_LOG("API code %{public}d return fail %{public}d", operationCode, ret);
        DfxManager::GetInstance()->HandleControllerServiceError(operationCode, ret);
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t costTime = endTime - startTime;
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} API excuted, userId: %{public}d, traceId: %{public}s, "
        "code: %{public}d, ret: %{public}d, costTime: %{public}ld",
        MLOG_TAG, __FUNCTION__, __LINE__, userId, traceId.c_str(),
        static_cast<int32_t>(operationCode), ret, static_cast<long>(costTime));
    return ret;
}
} // namespace AbilityRuntime
} // namespace OHOS
