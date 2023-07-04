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

#include <cstdlib>

#include "ability_info.h"
#include "app_mgr_client.h"
#include "dataobs_mgr_client.h"
#include "datashare_ext_ability_context.h"
#include "hilog_wrapper.h"
#include "ipc_skeleton.h"
#include "media_datashare_stub_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_scanner_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_uripermission_operations.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "permission_utils.h"
#include "photo_album_column.h"
#include "runtime.h"
#include "singleton.h"
#include "system_ability_definition.h"
#ifdef MEDIALIBRARY_SECURITY_OPEN
#include "sec_comp_kit.h"
#endif

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::NativeRdb;
using namespace OHOS::DistributedKv;
using namespace OHOS::Media;
using namespace OHOS::DataShare;

namespace OHOS {
namespace AbilityRuntime {
using namespace OHOS::AppExecFwk;
using DataObsMgrClient = OHOS::AAFwk::DataObsMgrClient;

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

void MediaDataShareExtAbility::OnStart(const AAFwk::Want &want)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    Extension::OnStart(want);
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        MEDIA_ERR_LOG("Failed to get context");
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    MEDIA_INFO_LOG("%{public}s runtime language  %{public}d", __func__, runtime_.GetLanguage());

    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        MEDIA_ERR_LOG("Failed to get dataManager");
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    auto extensionContext = GetContext();
    int32_t ret = dataManager->InitMediaLibraryMgr(context, extensionContext);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to init MediaLibraryMgr");
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }
    dataManager->SetOwner(static_pointer_cast<MediaDataShareExtAbility>(shared_from_this()));

    auto scannerManager = MediaScannerManager::GetInstance();
    if (scannerManager != nullptr) {
        scannerManager->Start();
    } else {
        MEDIA_ERR_LOG("Failed to get scanner manager");
        DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
        return;
    }

    Media::MedialibrarySubscriber::Subscribe();
    MEDIA_INFO_LOG("%{public}s end.", __func__);
}

void MediaDataShareExtAbility::OnStop()
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto scannerManager = MediaScannerManager::GetInstance();
    if (scannerManager != nullptr) {
        scannerManager->Stop();
    }
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
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
    if (containsRead) {
        perm.push_back(PERMISSION_NAME_READ_MEDIA);
    }
    if (containsWrite) {
        perm.push_back(PERMISSION_NAME_WRITE_MEDIA);
    }
}

static inline bool ContainsFlag(const string &mode, const char flag)
{
    return mode.find(flag) != string::npos;
}

static int32_t CheckOpenFilePermission(MediaLibraryCommand &cmd, string &mode)
{
    MEDIA_DEBUG_LOG("uri: %{public}s mode: %{public}s", cmd.GetUri().ToString().c_str(), mode.c_str());
    MediaType mediaType = MediaFileUri::GetMediaTypeFromUri(cmd.GetUri().ToString());
    const bool containsRead = ContainsFlag(mode, 'r');
    const bool containsWrite = ContainsFlag(mode, 'w');

    vector<string> perms;
    FillV10Perms(mediaType, containsRead, containsWrite, perms);
    int32_t err = (mediaType == MEDIA_TYPE_FILE) ?
        (PermissionUtils::CheckHasPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED) :
        (PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED);
    if (err == E_SUCCESS) {
        return E_SUCCESS;
    }
    // Try to check deprecated permissions
    perms.clear();
    FillDeprecatedPerms(containsRead, containsWrite, perms);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t SystemApiCheck(MediaLibraryCommand &cmd)
{
    OperationObject obj = cmd.GetOprnObject();
    static const set<OperationObject> SYSTEM_API_OBJECTS = {
        OperationObject::UFM_PHOTO,
        OperationObject::UFM_AUDIO,
        OperationObject::UFM_ALBUM,
        OperationObject::UFM_MAP,
        OperationObject::SMART_ALBUM,
        OperationObject::SMART_ALBUM_MAP,

        OperationObject::ALL_DEVICE,
        OperationObject::ACTIVE_DEVICE,
    };

    if (SYSTEM_API_OBJECTS.find(obj) != SYSTEM_API_OBJECTS.end() ||
        // Delete asset permanently from system is only allowed for system apps.
        ((obj == OperationObject::FILESYSTEM_ASSET) && (cmd.GetOprnType() == Media::OperationType::DELETE))) {
#ifdef MEDIALIBRARY_SECURITY_OPEN
        if (cmd.GetUri().ToString().find(OPRN_CREATE_COMPONENT) != string::npos) {
            return E_SUCCESS;
        }
#endif
        if (!PermissionUtils::IsSystemApp()) {
            MEDIA_ERR_LOG("Systemapi should only be called by system applications!");
            return E_CHECK_SYSTEMAPP_FAIL;
        }
    }
    return E_SUCCESS;
}

static inline int32_t HandleMediaVolumePerm()
{
    return PermissionUtils::CheckCallerPermission(PERMISSION_NAME_READ_MEDIA) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static inline int32_t HandleBundlePermCheck()
{
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_WRITE_MEDIA);
    if (ret) {
        return E_SUCCESS;
    }

    return PermissionUtils::CheckHasPermission(WRITE_PERMS_V10) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t HandleSecurityComponentPermission(MediaLibraryCommand &cmd)
{
    if (cmd.GetUri().ToString().find(OPRN_CREATE_COMPONENT) != string::npos) {
#ifdef MEDIALIBRARY_SECURITY_OPEN
        auto tokenId = PermissionUtils::GetTokenId();
        if (!Security::SecurityComponent::SecCompKit::ReduceAfterVerifySavePermission(tokenId)) {
            return E_NEED_FURTHER_CHECK;
        }
        return E_SUCCESS;
#else
        MEDIA_ERR_LOG("Security component is not existed");
        return E_NEED_FURTHER_CHECK;
#endif
    }
    return E_NEED_FURTHER_CHECK;
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
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    string perm;
    if (obj == OperationObject::UFM_AUDIO) {
        perm = isWrite ? PERM_WRITE_AUDIO : PERM_READ_AUDIO;
    } else {
        perm = isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO;
    }
    return PermissionUtils::CheckCallerPermission(perm) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t PhotoAccessHelperPermCheck(MediaLibraryCommand &cmd, const bool isWrite)
{
    static const set<OperationObject> PHOTO_ACCESS_HELPER_OBJECTS = {
        OperationObject::PAH_PHOTO,
        OperationObject::PAH_ALBUM,
        OperationObject::PAH_MAP,
    };

    int32_t err = HandleSecurityComponentPermission(cmd);
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (PHOTO_ACCESS_HELPER_OBJECTS.find(obj) == PHOTO_ACCESS_HELPER_OBJECTS.end()) {
        return E_NEED_FURTHER_CHECK;
    }
    return PermissionUtils::CheckCallerPermission(
        isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO) ? E_SUCCESS : E_PERMISSION_DENIED;
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
    };

    string uri = cmd.GetUri().ToString();
    OperationObject obj = cmd.GetOprnObject();
    if (NO_NEED_PERM_CHECK_URI.find(uri) != NO_NEED_PERM_CHECK_URI.end() ||
        NO_NEED_PERM_CHECK_OBJ.find(obj) != NO_NEED_PERM_CHECK_OBJ.end()) {
        return E_SUCCESS;
    }
    return E_NEED_FURTHER_CHECK;
}

static int32_t HandleSpecialObjectPermission(MediaLibraryCommand &cmd, bool isWrite)
{
    int err = HandleNoPermCheck(cmd);
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (obj == OperationObject::MEDIA_VOLUME) {
        return HandleMediaVolumePerm();
    } else if (obj == OperationObject::BUNDLE_PERMISSION) {
        return HandleBundlePermCheck();
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
    };

    OperationObject obj = cmd.GetOprnObject();
    if (UNIFY_OP_OBJECT_MAP.find(obj) != UNIFY_OP_OBJECT_MAP.end()) {
        cmd.SetOprnObject(UNIFY_OP_OBJECT_MAP.at(obj));
    }
}

static int32_t CheckPermFromUri(MediaLibraryCommand &cmd, bool isWrite)
{
    MEDIA_DEBUG_LOG("uri: %{public}s object: %{public}d, opType: %{public}d isWrite: %{public}d",
        cmd.GetUri().ToString().c_str(), cmd.GetOprnObject(), cmd.GetOprnType(), isWrite);

    int err = SystemApiCheck(cmd);
    if (err != E_SUCCESS) {
        return err;
    }

    err = PhotoAccessHelperPermCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = UserFileMgrPermissionCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = HandleSpecialObjectPermission(cmd, isWrite);
    if (err == E_SUCCESS || (err != SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }

    // Finally, we should check the permission of medialibrary interfaces.
    string perm = isWrite ? PERMISSION_NAME_WRITE_MEDIA : PERMISSION_NAME_READ_MEDIA;
    err = PermissionUtils::CheckCallerPermission(perm) ? E_SUCCESS : E_PERMISSION_DENIED;
    if (err < 0) {
        return err;
    }
    UnifyOprnObject(cmd);
    return E_SUCCESS;
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

    int err = CheckOpenFilePermission(command, unifyMode);
    if (err == E_PERMISSION_DENIED) {
        err = UriPermissionOperations::CheckUriPermission(command.GetUriStringWithoutSegment(), unifyMode);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Permission Denied! err = %{public}d", err);
            return err;
        }
    } else if (err < 0) {
        return err;
    }
    if (command.GetUri().ToString().find(MEDIA_DATA_DB_THUMBNAIL) != string::npos) {
        command.SetOprnObject(OperationObject::THUMBNAIL);
    }
    return MediaLibraryDataManager::GetInstance()->OpenFile(command, unifyMode);
}

int MediaDataShareExtAbility::OpenRawFile(const Uri &uri, const string &mode)
{
    return 0;
}

int MediaDataShareExtAbility::Insert(const Uri &uri, const DataShareValuesBucket &value)
{
    MediaLibraryCommand cmd(uri);
    int32_t err = CheckPermFromUri(cmd, true);
    if (err < 0) {
        return err;
    }

    return MediaLibraryDataManager::GetInstance()->Insert(cmd, value);
}

int MediaDataShareExtAbility::Update(const Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value)
{
    MediaLibraryCommand cmd(uri);
    int32_t err = CheckPermFromUri(cmd, true);
    if (err < 0) {
        return err;
    }

    return MediaLibraryDataManager::GetInstance()->Update(cmd, value, predicates);
}

int MediaDataShareExtAbility::Delete(const Uri &uri, const DataSharePredicates &predicates)
{
    MediaLibraryCommand cmd(uri, Media::OperationType::DELETE);
    int err = CheckPermFromUri(cmd, true);
    if (err < 0) {
        return err;
    }

    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

shared_ptr<DataShareResultSet> MediaDataShareExtAbility::Query(const Uri &uri,
    const DataSharePredicates &predicates, vector<string> &columns, DatashareBusinessError &businessError)
{
    MediaLibraryCommand cmd(uri, Media::OperationType::QUERY);
    int32_t err = CheckPermFromUri(cmd, false);
    if (err < 0) {
        businessError.SetCode(err);
        return nullptr;
    }

    int errCode = businessError.GetCode();
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    businessError.SetCode(to_string(errCode));
    if (queryResultSet == nullptr) {
        MEDIA_ERR_LOG("queryResultSet is nullptr! errCode: %{public}d", errCode);
        return nullptr;
    }
    shared_ptr<DataShareResultSet> resultSet = make_shared<DataShareResultSet>(queryResultSet);
    return resultSet;
}

string MediaDataShareExtAbility::GetType(const Uri &uri)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto ret = MediaLibraryDataManager::GetInstance()->GetType(uri);
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return ret;
}

int MediaDataShareExtAbility::BatchInsert(const Uri &uri, const vector<DataShareValuesBucket> &values)
{
    MediaLibraryCommand cmd(uri);
    int32_t err = CheckPermFromUri(cmd, true);
    if (err < 0) {
        return err;
    }
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
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient->RegisterObserver error return %{public}d", __func__, ret);
        return false;
    }
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return true;
}

bool MediaDataShareExtAbility::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_INFO_LOG("%{public}s begin.", __func__);
    auto obsMgrClient = DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient is nullptr", __func__);
        return false;
    }

    ErrCode ret = obsMgrClient->UnregisterObserver(uri, dataObserver);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient->UnregisterObserver error return %{public}d", __func__, ret);
        return false;
    }
    MEDIA_INFO_LOG("%{public}s end.", __func__);
    return true;
}

bool MediaDataShareExtAbility::NotifyChange(const Uri &uri)
{
    auto obsMgrClient = DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient is nullptr", __func__);
        return false;
    }

    ErrCode ret = obsMgrClient->NotifyChange(uri);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("%{public}s obsMgrClient->NotifyChange error return %{public}d", __func__, ret);
        return false;
    }
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
} // namespace AbilityRuntime
} // namespace OHOS
