/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaPermissionHelper"

#include "media_permission_helper.h"

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "os_account_manager.h"
#include "system_ability_definition.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "permission_utils.h"
#include "result_set_utils.h"
#include "media_uri_utils.h"
#include "user_inner_ipc_client.h"
#include "medialibrary_business_code.h"
#include "cancel_photo_uri_permission_inner_vo.h"
#include "grant_photo_uri_permission_inner_vo.h"
#include "check_photo_uri_permission_inner_vo.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace Media {
static constexpr int32_t URI_MAX_SIZE = 1000;
static constexpr uint32_t URI_PERMISSION_FLAG_READ = 1;
static constexpr uint32_t URI_PERMISSION_FLAG_WRITE = 2;
static constexpr uint32_t URI_PERMISSION_FLAG_READWRITE = 3;
static constexpr int32_t DEFUALT_USER_ID = 100;

static const std::string MEDIALIBRARY_TYPE_IMAGE_URI = "/image";
static const std::string MEDIALIBRARY_TYPE_VIDEO_URI = "/video";
static const std::string MEDIALIBRARY_TYPE_AUDIO_URI = "/audio";
static const std::string MEDIALIBRARY_DATA_URI = "datashare:///media";
static const std::string CHECK_URI_PERMISSION = "phaccess_checkuripermission";
static const std::string MEDIALIBRARY_CHECK_URIPERM_URI = MEDIALIBRARY_DATA_URI + "/" + CHECK_URI_PERMISSION;
static const std::string PHOTO_TYPE_URI = "/Photo";
static const std::string AUDIO_TYPE_URI = "/Audio";

static map<string, TableType> tableMap = {
    { MEDIALIBRARY_TYPE_IMAGE_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_VIDEO_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_AUDIO_URI, TableType::TYPE_AUDIOS },
    { PHOTO_TYPE_URI, TableType::TYPE_PHOTOS },
    { AUDIO_TYPE_URI, TableType::TYPE_AUDIOS }
};
// LCOV_EXCL_START
MediaPermissionHelper *MediaPermissionHelper::GetMediaPermissionHelper()
{
    static MediaPermissionHelper mediaPermHelper;
    return &mediaPermHelper;
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

static sptr<IRemoteObject> InitToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, nullptr, "get system ability mgr failed.");

    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, nullptr, "GetSystemAbility Service failed.");
    return remoteObj;
}

void MediaPermissionHelper::InitMediaPermissionHelper()
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

bool MediaPermissionHelper::ForceReconnect()
{
    dataShareHelper_ = nullptr;
    InitMediaPermissionHelper();
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, false, "init manager fail");
    return true;
}

static int32_t CheckInputParameters(const vector<string> &urisSource, vector<uint32_t> flags)
{
    CHECK_AND_RETURN_RET_LOG(!urisSource.empty(), E_ERR, "Media Uri list is empty");
    CHECK_AND_RETURN_RET_LOG(urisSource.size() <= URI_MAX_SIZE, E_ERR,
        "Uri list is exceed one Thousand, current list size: %{public}d", (int)urisSource.size());
    CHECK_AND_RETURN_RET_LOG(flags.size() == urisSource.size(), E_ERR,
        "uri size not equal flag size: %{public}d", (int)flags.size());
    for (uint32_t flag : flags) {
        bool cond = (flag == 0 || flag > URI_PERMISSION_FLAG_READWRITE);
        CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Flag is invalid, current flag is: %{public}d", flag);
    }
    return E_SUCCESS;
}

static int32_t ClassifyUri(const vector<string> &urisSource, vector<string> &photoIds, vector<string> &audioIds)
{
    for (string uri : urisSource) {
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }

        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaUriUtils::GetFileIdStr(uri);
        if (tableType == static_cast<int32_t>(TableType::TYPE_PHOTOS)) {
            photoIds.push_back(fileId);
        } else if (tableType == static_cast<int32_t>(TableType::TYPE_AUDIOS)) {
            audioIds.push_back(fileId);
        } else {
            MEDIA_ERR_LOG("Uri invalid error, uri:%{private}s", uri.c_str());
            return E_ERR;
        }
    }
    return E_SUCCESS;
}

static void CheckPhotoAccessTokenPermission(uint32_t tokenId, const vector<string> &photoIds,
    map<string, pair<bool, bool>> &photoPermissionMap, bool readWriteIsolation)
{
    if (photoIds.size() > 0) {
        bool haveReadPermission = AccessTokenKit::VerifyAccessToken(tokenId, PERM_READ_IMAGEVIDEO) == 0;
        bool haveWritePermission = AccessTokenKit::VerifyAccessToken(tokenId, PERM_WRITE_IMAGEVIDEO) == 0;
        for (string fileId : photoIds) {
            if (haveReadPermission) {
                photoPermissionMap[fileId].first = true;
            }
            if (haveWritePermission && !readWriteIsolation) {
                photoPermissionMap[fileId].first = true;
                photoPermissionMap[fileId].second = true;
            }
            if (haveWritePermission && readWriteIsolation) {
                photoPermissionMap[fileId].second = true;
            }
        }
    }
}

static void CheckAudioAccessTokenPermission(uint32_t tokenId, const vector<string> &audioIds,
    map<string, pair<bool, bool>> &audioPermissionMap, bool readWriteIsolation)
{
    if (audioIds.size() > 0) {
        bool haveReadPermission = AccessTokenKit::VerifyAccessToken(tokenId, PERM_READ_AUDIO) == 0;
        bool haveWritePermission = AccessTokenKit::VerifyAccessToken(tokenId, PERM_WRITE_AUDIO) == 0;
        for (string fileId : audioIds) {
            if (haveReadPermission) {
                audioPermissionMap[fileId].first = true;
            }
            if (haveWritePermission && !readWriteIsolation) {
                audioPermissionMap[fileId].first = true;
                audioPermissionMap[fileId].second = true;
            }
            if (haveWritePermission && readWriteIsolation) {
                audioPermissionMap[fileId].second = true;
            }
        }
    }
}

static void SaveCheckPermission(const string &fileId, map<std::string, pair<bool, bool>> &permissionMap,
    PhotoPermissionType currentType)
{
    switch (currentType) {
        case PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            break;
        case PhotoPermissionType::PERSIST_READ_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            break;
        case PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            permissionMap[fileId].second = true;
            break;
        case PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO:
            permissionMap[fileId].second = true;
            break;
        case PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            permissionMap[fileId].second = true;
            break;
        case PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            permissionMap[fileId].second = true;
            break;
        default:
            MEDIA_WARN_LOG("invalid permission, break");
    }
}

int32_t MediaPermissionHelper::QueryGrantedIndex(uint32_t targetTokenId,
    const std::string &uriType, const std::vector<string> &fileIds,
    std::map<string, pair<bool, bool>> &permissionMap, uint32_t businessCode)
{
    CheckUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = static_cast<int64_t>(targetTokenId);
    reqBody.uriType = uriType;
    reqBody.fileIds = fileIds;
    reqBody.columns.emplace_back(AppUriPermissionColumn::FILE_ID);
    reqBody.columns.emplace_back(AppUriPermissionColumn::PERMISSION_TYPE);
    CheckUriPermissionInnerRespBody respBody;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR,
        "Failed to checkPhotoUriPermission, datashareHelper is nullptr");
    MEDIA_INFO_LOG("before IPC::UserDefineIPCClient().Call, INNER_CHECK_URI_PERMISSION");
    int32_t result = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode,
        reqBody, respBody);
    if (result != E_SUCCESS && ForceReconnect()) {
        MEDIA_WARN_LOG("QueryGrantedIndex Failed, reconnect and retry");
        result = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody, respBody);
    }
    CHECK_AND_RETURN_RET_LOG(result == E_SUCCESS, E_ERR, "QueryGrantedIndex Failed");
    auto fileIds_size = respBody.fileIds.size();
    auto permissionTypes_size = respBody.permissionTypes.size();
    bool isValid = (fileIds_size == permissionTypes_size);
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "Failed cause fileIds_size:%{public}zu"
        " not same to permissionTypes_size:%{public}zu", fileIds_size, permissionTypes_size);
    for (size_t i = 0; i < respBody.fileIds.size(); i++) {
        string fileId = respBody.fileIds[i];
        int32_t permissionType = respBody.permissionTypes[i];
        SaveCheckPermission(fileId, permissionMap, static_cast<PhotoPermissionType>(permissionType));
    }
    return E_SUCCESS;
}

static bool CheckPermissionByMap(const string &fileId, uint32_t flag,
    const map<string, pair<bool, bool>> &permissionMap)
{
    auto it = permissionMap.find(fileId);
    if (it == permissionMap.end()) {
        return false;
    }
    switch (flag) {
        case URI_PERMISSION_FLAG_READ:
            return it->second.first;
        case URI_PERMISSION_FLAG_WRITE:
            return it->second.second;
        case URI_PERMISSION_FLAG_READWRITE:
            return it->second.first && it->second.second;
        default:
            MEDIA_WARN_LOG("invalid check flag!");
            return false;
    }
}

int32_t MediaPermissionHelper::CheckPhotoUriPermission(uint32_t tokenId,
    const vector<string> &urisSource, vector<bool> &results, const vector<uint32_t> &flags,
    bool readWriteIsolation)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPermissionHelper::CheckPhotoUriPermission");
    auto ret = CheckInputParameters(urisSource, flags);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);
    map<string, pair<bool, bool>> photoPermissionMap;
    map<string, pair<bool, bool>> audioPermissionMap;
    vector<string> photoIds;
    vector<string> audioIds;
    ret = ClassifyUri(urisSource, photoIds, audioIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_ERR, "invalid uri");
    if (photoIds.size() > 0) {
        CheckPhotoAccessTokenPermission(tokenId, photoIds, photoPermissionMap, readWriteIsolation);
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION);
        ret = QueryGrantedIndex(tokenId, to_string(static_cast<int32_t>(TableType::TYPE_PHOTOS)),
            photoIds, photoPermissionMap, businessCode);
    }
    if (audioIds.size() > 0) {
        CheckAudioAccessTokenPermission(tokenId, audioIds, audioPermissionMap, readWriteIsolation);
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_AUDIO_URI_PERMISSION);
        ret = QueryGrantedIndex(tokenId, to_string(static_cast<int32_t>(TableType::TYPE_AUDIOS)),
            audioIds, audioPermissionMap, businessCode);
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_ERR, "query db fail!");

    results.resize(urisSource.size(), false);
    for (size_t i = 0; i < urisSource.size(); i++) {
        string uri = urisSource.at(i);
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }
        string fileId = MediaUriUtils::GetFileIdStr(uri);
        if (tableType == static_cast<int32_t>(TableType::TYPE_AUDIOS)) {
            results[i] = CheckPermissionByMap(fileId, flags.at(i), audioPermissionMap);
        } else {
            results[i] = CheckPermissionByMap(fileId, flags.at(i), photoPermissionMap);
        }
    }
    return E_SUCCESS;
}

int32_t MediaPermissionHelper::CheckCloudDownloadPermission(uint32_t tokenId,
    const std::vector<std::string> &uris, std::vector<bool> &result, const std::vector<uint32_t> &flags)
{
    uint64_t tokenIdEx = IPCSkeleton::GetCallingFullTokenID();
    CHECK_AND_RETURN_RET_LOG(TokenIdKit::IsSystemAppByFullTokenID(tokenIdEx),
        E_ERR, "only invoke by systemapp");
    return CheckPhotoUriPermission(tokenId, uris, result, flags, true);
}

static int32_t CheckGrantPermission(const vector<string> &urisSource, vector<PhotoPermissionType> photoPermissionTypes,
    HideSensitiveType hideSensitiveTpye)
{
    bool cond = ((urisSource.empty()) || (urisSource.size() > URI_MAX_SIZE));
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Media Uri list error, please check!");
    CHECK_AND_RETURN_RET_LOG(urisSource.size() == photoPermissionTypes.size(), E_ERR,
        "uris size not equal PermissionTypes size!");
    for (PhotoPermissionType photoPermissionType : photoPermissionTypes) {
        cond = (photoPermissionType < PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO ||
            photoPermissionType > PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO ||
            photoPermissionType == PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO);
        CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "photoPermissionType error, please check param!");
    }

    cond = (hideSensitiveTpye < HideSensitiveType::ALL_DESENSITIZE ||
        hideSensitiveTpye > HideSensitiveType::DEFAULT);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "HideSensitiveType error, please check param!");
    return E_SUCCESS;
}

int32_t MediaPermissionHelper::GrantPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId,
    const std::vector<string> &uris, const vector<PhotoPermissionType> &photoPermissionTypes,
    HideSensitiveType hideSensitiveTpye)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPermissionHelper::GrantPhotoUriPermission");
    auto ret = CheckGrantPermission(uris, photoPermissionTypes, hideSensitiveTpye);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);
    MEDIA_INFO_LOG("MediaPermissionHelper::GrantPhotoUriPermission Start");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is nullptr");
    GrantUrisPermissionInnerReqBody reqBody;
    reqBody.tokenId = (int64_t)targetTokenId;
    reqBody.srcTokenId = (int64_t)srcTokenId;
    reqBody.hideSensitiveType = static_cast<int32_t>(hideSensitiveTpye);
    for (size_t i = 0; i < uris.size(); i++) {
        auto uri = uris.at(i);
        auto photoPermissionType = photoPermissionTypes.at(i);
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }
        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaUriUtils::GetFileIdStr(uri);
        reqBody.fileIds.emplace_back(fileId);
        reqBody.uriTypes.emplace_back(tableType);
        if (photoPermissionType == PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO) {
            reqBody.permissionTypes.emplace_back(static_cast<int32_t>(PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO));
            reqBody.fileIds.emplace_back(fileId);
            reqBody.uriTypes.emplace_back(tableType);
            reqBody.permissionTypes.emplace_back(static_cast<int32_t>(PhotoPermissionType::PERSIST_READ_IMAGEVIDEO));
            continue;
        }
        reqBody.permissionTypes.emplace_back(static_cast<int32_t>(photoPermissionType));
    }
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION);
    MEDIA_INFO_LOG("before IPC::UserDefineIPCClient().Call, INNER_GRANT_PHOTO_URI_PERMISSION");
    int32_t result = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
    if (result == E_ERR && ForceReconnect()) {
        MEDIA_WARN_LOG("Failed to Call INNER_GRANT_PHOTO_URI_PERMISSION and retry");
        result = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
    }
    return result;
}

static vector<string> BuildPermissionType(const bool persistFlag, const OperationMode mode)
{
    vector<string> permissionTypes;
    if (persistFlag) {
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OperationMode::READ_MODE)) {
            permissionTypes.push_back(
                to_string(static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
            permissionTypes.push_back(
                to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READ_IMAGEVIDEO)));
        }
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OperationMode::WRITE_MODE)) {
            permissionTypes.push_back(
                to_string(static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
            permissionTypes.push_back(
                to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO)));
        }
        if (static_cast<uint32_t>(mode) & static_cast<uint32_t>(OperationMode::READ_MODE) &&
            static_cast<uint32_t>(mode) & static_cast<uint32_t>(OperationMode::WRITE_MODE)) {
            permissionTypes.push_back(to_string(
                static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
            permissionTypes.push_back(
                to_string(static_cast<uint32_t>(PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO)));
        }
    } else {
        permissionTypes.push_back(
            to_string(static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
        permissionTypes.push_back(
            to_string(static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
        permissionTypes.push_back(
            to_string(static_cast<uint32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
    }
    return permissionTypes;
}

int32_t MediaPermissionHelper::CancelPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId,
    const std::vector<string> &uris, const bool persistFlag, const vector<OperationMode> &operationModes)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPermissionHelper::CancelPhotoUriPermission");
    MEDIA_DEBUG_LOG("CancelPermission begin, srcToken:%{private}d, targetToken:%{private}d", srcTokenId, targetTokenId);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is nullptr");
    vector<OperationMode> operModesCopy;
    if (persistFlag) {
        operModesCopy.insert(operModesCopy.end(), operationModes.begin(), operationModes.end());
    } else {
        operModesCopy.resize(uris.size(), OperationMode::READ_WRITE_MODE);
    }
    vector<DataShareValuesBucket> valueSet;
    bool cond = ((uris.empty()) || (uris.size() > URI_MAX_SIZE) || (uris.size() != operModesCopy.size()));
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Media Uri list error, please check!");
    CancelUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = (int64_t)targetTokenId;
    reqBody.srcTokenId = (int64_t)srcTokenId;
    for (size_t i = 0; i < uris.size(); i++) {
        string uri = uris[i];
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }
        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaUriUtils::GetFileIdStr(uri);
        MEDIA_DEBUG_LOG("CancelPermission fileId:%{private}s, tableType:%{private}d", fileId.c_str(), tableType);
        reqBody.fileIds.emplace_back(fileId);
        reqBody.uriTypes.emplace_back(tableType);
        vector<string> permissionTypes = BuildPermissionType(persistFlag, operModesCopy[i]);
        reqBody.permissionTypes.emplace_back(permissionTypes);
    }
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CANCEL_PHOTO_URI_PERMISSION);
    MEDIA_INFO_LOG("before IPC::UserDefineIPCClient().Call, INNER_CANCEL_PHOTO_URI_PERMISSION");
    int32_t result = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper_).Call(businessCode, reqBody);
    return result;
}

static bool CheckPermissionType(const string &fileId, map<string, pair<bool, bool>> &permissionMap,
    PhotoPermissionType checkType)
{
    auto it = permissionMap.find(fileId);
    if (it == permissionMap.end()) {
        return false;
    }
    switch (checkType) {
        case PhotoPermissionType::PERSIST_READ_IMAGEVIDEO:
            return it->second.first;
        case PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO:
            return it->second.second;
        case PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO:
            return it->second.first && it->second.second;
        default:
            MEDIA_WARN_LOG("invalid permissiontype");
            return false;
    }
}

static void SavePermission(const string &fileId, map<string, pair<bool, bool>> &permissionMap,
    PhotoPermissionType currentType)
{
    switch (currentType) {
        case PhotoPermissionType::PERSIST_READ_IMAGEVIDEO:
            permissionMap[fileId].first = true;
            break;
        case PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO:
            permissionMap[fileId].second = true;
            break;
        default:
            MEDIA_WARN_LOG("invalid permission, break");
            return;
    }
}

int32_t MediaPermissionHelper::GetPhotoUrisPermission(uint32_t targetTokenId, const std::vector<string> &uris,
    PhotoPermissionType photoPermissionType, std::vector<bool> &result)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "dataShareHelper is nullptr");
    MEDIA_DEBUG_LOG("GetPhotoUrisPermission begin, targetTokenId:%{private}d", targetTokenId);

    vector<string> columns = {
        AppUriPermissionColumn::FILE_ID,
        AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::TARGET_TOKENID
    };

    DataSharePredicates predicates;
    predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(targetTokenId));

    result.resize(uris.size(), false);

    std::vector<std::string> fileIds;
    for (const auto &uri : uris) {
        fileIds.push_back(MediaUriUtils::GetFileIdStr(uri));
    }
    predicates.In(AppUriPermissionColumn::FILE_ID, fileIds);

    Uri queryUri(MEDIALIBRARY_CHECK_URIPERM_URI);
    auto queryResultSet = dataShareHelper_->Query(queryUri, predicates, columns);
    if (queryResultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query Uris");
        return E_ERR;
    }

    map<string, pair<bool, bool>> permissionMap;
    while (queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        string fileId = GetStringVal(AppUriPermissionColumn::FILE_ID, queryResultSet);
        int32_t permissionType = GetInt32Val(AppUriPermissionColumn::PERMISSION_TYPE, queryResultSet);
        SavePermission(fileId, permissionMap, static_cast<PhotoPermissionType>(permissionType));
    }
    for (size_t i = 0; i < uris.size(); ++i) {
        string fileId = MediaUriUtils::GetFileIdStr(uris[i]);
        result[i] = CheckPermissionType(fileId, permissionMap, photoPermissionType);
    }
    return E_SUCCESS;
}

int32_t MediaPermissionHelper::GetPhotoUrisPermission(uint32_t targetTokenId, const std::vector<string> &uris,
    const vector<PhotoPermissionType> &photoPermissionTypes, std::vector<bool> &result)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPermissionHelper::GetPhotoUrisPermission");
    CHECK_AND_RETURN_RET_LOG(!uris.empty() && uris.size() <= URI_MAX_SIZE && uris.size() == photoPermissionTypes.size(),
        E_ERR, "Uri or photoPermissionType list error, please check!");

    std::set<PhotoPermissionType> queryTypes;
    for (auto &photoPermissionType : photoPermissionTypes) {
        if (photoPermissionType != PhotoPermissionType::PERSIST_READ_IMAGEVIDEO &&
            photoPermissionType != PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO &&
            photoPermissionType != PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO) {
            MEDIA_ERR_LOG("photoPermissionType error, please check param!");
            return E_ERR;
        }
        queryTypes.insert(photoPermissionType);
    }
    std::vector<bool> persistReadResult(uris.size(), false);
    std::vector<bool> persistReadWriteResult(uris.size(), false);
    std::vector<bool> persistWriteResult(uris.size(), false);
    map<PhotoPermissionType, std::vector<bool>> typeMap = {
        { PhotoPermissionType::PERSIST_READ_IMAGEVIDEO, persistReadResult },
        { PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO, persistWriteResult },
        { PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO, persistReadWriteResult },
    };
    for (auto& type : queryTypes) {
        int32_t ret = GetPhotoUrisPermission(targetTokenId, uris, type, typeMap[type]);
        if (ret != E_SUCCESS) {
            MEDIA_ERR_LOG("Failed to get permission: %d", type);
            return E_ERR;
        }
    }

    result.resize(uris.size(), false);
    for (size_t i = 0; i < uris.size(); ++i) {
        result[i] = typeMap[photoPermissionTypes[i]][i];
    }
    return E_SUCCESS;
}

int32_t MediaPermissionHelper::GetUrisFromFusePaths(const std::vector<std::string> paths,
    std::vector<std::string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPermissionHelper::GetUrisFromFusePaths");

    if ((paths.empty()) || (paths.size() > URI_MAX_SIZE)) {
        MEDIA_ERR_LOG("Path list error, please check!");
        return E_ERR;
    }
    const std::string FUSE_PATH_PREFIX_1 = "/data/storage/el2/media/";
    const std::string FUSE_PATH_PREFIX_2 = "/mnt/data/100/media_fuse/";
    const std::string URI_PREFIX = "file://media/";

    for (const auto &path : paths) {
        if (path.compare(0, FUSE_PATH_PREFIX_1.length(), FUSE_PATH_PREFIX_1) == 0) {
            std::string uri = URI_PREFIX + path.substr(FUSE_PATH_PREFIX_1.length());
            uris.push_back(uri);
        } else if (path.compare(0, FUSE_PATH_PREFIX_2.length(), FUSE_PATH_PREFIX_2) == 0) {
            std::string uri = URI_PREFIX + path.substr(FUSE_PATH_PREFIX_2.length());
            uris.push_back(uri);
        } else {
            MEDIA_ERR_LOG("Invalid path: %{private}s", path.c_str());
            return E_ERR;
        }
    }
    return E_SUCCESS;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS