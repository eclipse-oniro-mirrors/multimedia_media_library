/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryExtendManager"

#include "media_library_extend_manager.h"

#include <cinttypes>

#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"

#include "base_data_uri.h"
#include "media_file_utils.h"
#include "media_uri_utils.h"
#include "media_string_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "user_inner_ipc_client.h"
#include "medialibrary_business_code.h"
#include "get_result_set_from_db_vo.h"
#include "get_result_set_from_photos_extend_vo.h"
#include "start_asset_change_scan_vo.h"
#include "get_asset_compress_version_vo.h"
#include "notify_asset_sended_vo.h"
#include "open_asset_compress_vo.h"
#include "get_compress_asset_size_vo.h"

using namespace std;

namespace OHOS {
namespace Media {
static constexpr int32_t DEFUALT_USER_ID = 100;
static constexpr int32_t DATASHARE_ERR = -1;
static constexpr int64_t SHARE_UID = 5520;
static constexpr int32_t COMPRESS_URI_MAX_SIZE = 500;

static const std::string OPEN_PRIVATE_LIVE_PHOTO = "open_private_live_photo";
static const std::string MEDIA_DATA_DB_URI = "uri";
static const std::string MEDIA_MOVING_PHOTO_OPRN_KEYWORD = "moving_photo_operation";

MediaLibraryExtendManager *MediaLibraryExtendManager::GetMediaLibraryExtendManager()
{
    static MediaLibraryExtendManager mediaLibMgr;
    return &mediaLibMgr;
}

static sptr<IRemoteObject> InitToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, nullptr, "get system ability mgr failed.");

    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, nullptr, "GetSystemAbility Service failed.");
    return remoteObj;
}

static int32_t GetCurrentAccountId()
{
    int32_t activeUserId = DEFUALT_USER_ID;
    ErrCode ret = OHOS::AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(activeUserId);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("fail to get activeUser:%{public}d", ret);
    }
    return activeUserId;
}

void MediaLibraryExtendManager::InitMediaLibraryExtendManager()
{
    int32_t activeUser =  GetCurrentAccountId();
    if (dataShareHelper_ == nullptr || activeUser != userId_) {
        auto token = InitToken();
        if (token == nullptr) {
            MEDIA_ERR_LOG("fail to get token.");
            return;
        }
        dataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        userId_ = activeUser;
    }
}

bool MediaLibraryExtendManager::ForceReconnect()
{
    dataShareHelper_ = nullptr;
    InitMediaLibraryExtendManager();
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, false, "init manager fail");
    return true;
}

static bool CheckUri(string &uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string uriprex = "file://media";
    return uri.substr(0, uriprex.size()) == uriprex;
}

int32_t MediaLibraryExtendManager::OpenAsset(string &uri, const string openMode, HideSensitiveType type)
{
    CHECK_AND_RETURN_RET(!openMode.empty(), E_ERR);
    CHECK_AND_RETURN_RET_LOG(CheckUri(uri), E_ERR, "invalid uri");

    string originOpenMode = openMode;
    std::transform(originOpenMode.begin(), originOpenMode.end(),
        originOpenMode.begin(), [](unsigned char c) {return std::tolower(c);});
    CHECK_AND_RETURN_RET(MEDIA_OPEN_MODES.count(originOpenMode), E_ERR);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "Failed to open Asset, datashareHelper is nullptr");

    string assetUri = uri;
    MediaUriUtils::AppendKeyValue(assetUri, "type", to_string(static_cast<int32_t>(type)));
    MEDIA_DEBUG_LOG("merged uri = %{public}s", assetUri.c_str());
    Uri openUri(assetUri);
    int ret = dataShareHelper_->OpenFile(openUri, openMode);
    if (ret == DATASHARE_ERR && ForceReconnect()) {
        MEDIA_WARN_LOG("Failed to OpenFile and retry");
        ret = dataShareHelper_->OpenFile(openUri, openMode);
    }
    return ret;
}

int32_t MediaLibraryExtendManager::ReadPrivateMovingPhoto(string &uri, const HideSensitiveType type)
{
    CHECK_AND_RETURN_RET_LOG(CheckUri(uri), E_ERR, "invalid uri: %{public}s", uri.c_str());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR,
        "Failed to read video of moving photo, datashareHelper is nullptr");

    string movingPhotoUri = uri;
    MediaUriUtils::AppendKeyValue(movingPhotoUri, "type", to_string(static_cast<int32_t>(type)));
    MediaUriUtils::AppendKeyValue(movingPhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OPEN_PRIVATE_LIVE_PHOTO);
    Uri openMovingPhotoUri(movingPhotoUri);
    int ret = dataShareHelper_->OpenFile(openMovingPhotoUri, MEDIA_FILEMODE_READONLY);
    if (ret == DATASHARE_ERR && ForceReconnect()) {
        MEDIA_WARN_LOG("Failed to OpenFile and retry");
        ret = dataShareHelper_->OpenFile(openMovingPhotoUri, MEDIA_FILEMODE_READONLY);
    }
    return ret;
}

static bool CheckPhotoUri(const string &uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string photoUriPrefix = "file://media/Photo/";
    return MediaStringUtils::StartsWith(uri, photoUriPrefix);
}

std::shared_ptr<DataShareResultSet> MediaLibraryExtendManager::GetResultSetFromPhotos(const string &value,
    vector<string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, nullptr, "datashareHelper is nullptr");
    if (!CheckPhotoUri(value)) {
        MEDIA_ERR_LOG("Failed to check invalid uri: %{public}s", value.c_str());
        return nullptr;
    }

    GetResultSetFromPhotosExtendReqBody reqBody;
    reqBody.value = value;
    reqBody.columns = columns;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS_EXTEND);
    GetResultSetFromDbRespBody respBody;
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
    if (errCode != E_OK) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
        }
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("errCode: %{public}d", errCode);
            return nullptr;
        }
    }
    return respBody.resultSet;
}
                                   
int32_t MediaLibraryExtendManager::SendBrokerChangeOperation(string operation)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "datashareHelper is nullptr");
    StartAssetChangeScanReqBody reqBody;
    reqBody.operation = operation;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHANGE_SCAN_ASSET);
    MEDIA_INFO_LOG("before IPC::UserDefineIPCClient().Call, INNER_CHANGE_SCAN_ASSET");
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
    return errCode;
}

std::shared_ptr<DataShareResultSet> MediaLibraryExtendManager::GetResultSetFromDb(string columnName,
    const string &value, vector<string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, nullptr, "dataShareHelper is null");
    if (columnName == MEDIA_DATA_DB_URI) {
        return GetResultSetFromPhotos(value, columns);
    }

    GetResultSetFromDbReqBody reqBody;
    reqBody.columnName = columnName;
    reqBody.value = value;
    reqBody.columns = columns;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB_EXTEND);
    GetResultSetFromDbRespBody respBody;
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
    if (errCode != E_OK) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
        }
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("errCode: %{public}d", errCode);
            return nullptr;
        }
    }
    return respBody.resultSet;
}

int32_t MediaLibraryExtendManager::OpenAssetCompress(const string &uri, HideSensitiveType type, int32_t version)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryExtendManager::OpenAssetCompress");
    MEDIA_INFO_LOG("OpenAssetCompress begin");

    CHECK_AND_RETURN_RET_LOG(IPCSkeleton::GetCallingUid() == SHARE_UID, E_ERR, "only support share");
    CHECK_AND_RETURN_RET_LOG(CheckPhotoUri(uri), E_ERR, "Invalid uri");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "Failed to open Asset, datashareHelper is nullptr");

    OpenAssetCompressReqBody reqBody;
    reqBody.uri = uri;
    reqBody.version = version;
    reqBody.type = static_cast<int32_t>(type);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_OPEN_ASSET_COMPRESS);
    OpenAssetCompressRespBody respBody;
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
    if (errCode != E_SUCCESS) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
        }
        CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, E_ERR, "OpenAssetCompress failed, errCode: %{public}d", errCode);
    }
    return respBody.fileDescriptor;
}

int32_t MediaLibraryExtendManager::GetAssetCompressVersion()
{
    MEDIA_INFO_LOG("GetAssetCompressVersion begin");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is null");

    GetAssetCompressVersionRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSET_COMPRESS_VERSION);
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Get(businessCode, respBody);
    if (errCode != E_SUCCESS) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Get(businessCode, respBody);
        }
        CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, E_ERR,
            "GetAssetCompressVersion failed, errCode: %{public}d", errCode);
    }
    MEDIA_INFO_LOG("GetAssetCompressVersion end, version=%{public}d", respBody.version);
    return respBody.version;
}

int32_t MediaLibraryExtendManager::NotifyAssetSended(const string &uri)
{
    MEDIA_INFO_LOG("NotifyAssetSended begin, uri:%{private}s", uri.c_str());
    CHECK_AND_RETURN_RET_LOG(CheckPhotoUri(uri), E_ERR, "invalid uri");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is null");

    NotifyAssetSendedReqBody reqBody;
    reqBody.uri = uri;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_NOTIFY_ASSET_SENDED);
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
    if (errCode != E_SUCCESS) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
        }
        CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, E_ERR, "NotifyAssetSended failed, errCode: %{public}d", errCode);
    }
    return E_SUCCESS;
}

int64_t MediaLibraryExtendManager::GetCompressAssetSize(const std::vector<std::string> &uris)
{
    MEDIA_INFO_LOG("GetCompressAssetSize begin, count: %{public}zu", uris.size());
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is null");
    CHECK_AND_RETURN_RET_LOG(uris.size() > 0 && uris.size() <= COMPRESS_URI_MAX_SIZE, E_ERR, "invalid uris size");
    GetCompressAssetSizeReqBody reqBody;
    reqBody.uris = uris;
    GetCompressAssetSizeRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_COMPRESS_ASSET_SIZE);
    int32_t errCode =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
    if (errCode != E_SUCCESS) {
        MEDIA_WARN_LOG("errCode: %{public}d, reconnect and retry", errCode);
        if (ForceReconnect()) {
            errCode =
                IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
        }
    }
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, E_ERR, "GetCompressAssetSize failed, errCode: %{public}d", errCode);
    MEDIA_INFO_LOG("GetCompressAssetSize success, total bytes: %{public}" PRId64, respBody.totalSize);
    return respBody.totalSize;
}
} // namespace Media
} // namespace OHOS