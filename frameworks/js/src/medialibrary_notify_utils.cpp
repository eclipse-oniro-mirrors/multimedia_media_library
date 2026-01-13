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

#define MLOG_TAG "AccurateRefresh::MediaLibraryNotifyUtils"
#include "medialibrary_notify_utils.h"

#include "media_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"

using namespace std;

namespace OHOS {
namespace Media {
const std::string RegisterNotifyType::PHOTO_CHANGE = "photoChange";
const std::string RegisterNotifyType::HIDDEN_PHOTO_CHANGE = "hiddenPhotoChange";
const std::string RegisterNotifyType::TRASH_PHOTO_CHANGE = "trashedPhotoChange";
const std::string RegisterNotifyType::PHOTO_ALBUM_CHANGE = "photoAlbumChange";
const std::string RegisterNotifyType::HIDDEN_ALBUM_CHANGE = "hiddenAlbumChange";
const std::string RegisterNotifyType::TRASHED_ALBUM_CHANGE = "trashedAlbumChange";
const std::string RegisterNotifyType::BATCH_DOWNLOAD_PROGRESS_CHANGE = "downloadProgressChange";
const std::string RegisterNotifyType::SINGLE_PHOTO_CHANGE = "singlePhotoChange";
const std::string RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE = "singlePhotoAlbumChange";
const std::string RegisterNotifyType::USER_CLIENT_CHANGE = "userDefineChange";

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyUtils::REGISTER_ASSET_MANAGER_TYPE_MAP = {
    { Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI,
        Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI },
};

const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyUtils::REGISTER_ASSET_MANAGER_URI_MAP = {
    { Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI, RegisterNotifyType::BATCH_DOWNLOAD_PROGRESS_CHANGE },
};

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyUtils::REGISTER_USER_DEFINE_TYPE_MAP = {
    { Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI, Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI },
};
 
const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyUtils::REGISTER_USER_DEFINE_URI_MAP = {
    { Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI, RegisterNotifyType::USER_CLIENT_CHANGE },
};

const std::map<std::string, Notification::NotifyUriType> MediaLibraryNotifyUtils::REGISTER_SINGLE_NOTIFY_TYPE_MAP = {
    { RegisterNotifyType::SINGLE_PHOTO_CHANGE, Notification::NotifyUriType::SINGLE_PHOTO_URI },
    { RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE, Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyUtils::REGISTER_SINGLE_TYPE_MAP = {
    { Notification::NotifyUriType::SINGLE_PHOTO_URI, Notification::NotifyUriType::SINGLE_PHOTO_URI },
    { Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI, Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyUtils::REGISTER_SINGLE_URI_MAP = {
    { Notification::NotifyUriType::SINGLE_PHOTO_URI, RegisterNotifyType::SINGLE_PHOTO_CHANGE },
    { Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI, RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE },
};

const std::map<std::string, Notification::NotifyUriType> MediaLibraryNotifyUtils::REGISTER_NOTIFY_TYPE_MAP = {
    { RegisterNotifyType::PHOTO_CHANGE, Notification::NotifyUriType::PHOTO_URI },
    { RegisterNotifyType::HIDDEN_PHOTO_CHANGE, Notification::NotifyUriType::HIDDEN_PHOTO_URI },
    { RegisterNotifyType::TRASH_PHOTO_CHANGE, Notification::NotifyUriType::TRASH_PHOTO_URI },
    { RegisterNotifyType::PHOTO_ALBUM_CHANGE, Notification::NotifyUriType::PHOTO_ALBUM_URI },
    { RegisterNotifyType::HIDDEN_ALBUM_CHANGE, Notification::NotifyUriType::HIDDEN_ALBUM_URI },
    { RegisterNotifyType::TRASHED_ALBUM_CHANGE, Notification::NotifyUriType::TRASH_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, Notification::NotifyUriType> MediaLibraryNotifyUtils::REGISTER_TYPE_MAP = {
    { Notification::NotifyUriType::PHOTO_URI, Notification::NotifyUriType::PHOTO_URI },
    { Notification::NotifyUriType::HIDDEN_PHOTO_URI, Notification::NotifyUriType::HIDDEN_PHOTO_URI },
    { Notification::NotifyUriType::TRASH_PHOTO_URI, Notification::NotifyUriType::TRASH_PHOTO_URI },
    { Notification::NotifyUriType::PHOTO_ALBUM_URI, Notification::NotifyUriType::PHOTO_ALBUM_URI },
    { Notification::NotifyUriType::HIDDEN_ALBUM_URI, Notification::NotifyUriType::HIDDEN_ALBUM_URI },
    { Notification::NotifyUriType::TRASH_ALBUM_URI, Notification::NotifyUriType::TRASH_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyUtils::REGISTER_URI_MAP = {
    { Notification::NotifyUriType::PHOTO_URI, RegisterNotifyType::PHOTO_CHANGE },
    { Notification::NotifyUriType::HIDDEN_PHOTO_URI, RegisterNotifyType::HIDDEN_PHOTO_CHANGE },
    { Notification::NotifyUriType::TRASH_PHOTO_URI, RegisterNotifyType::TRASH_PHOTO_CHANGE },
    { Notification::NotifyUriType::PHOTO_ALBUM_URI, RegisterNotifyType::PHOTO_ALBUM_CHANGE },
    { Notification::NotifyUriType::HIDDEN_ALBUM_URI, RegisterNotifyType::HIDDEN_ALBUM_CHANGE },
    { Notification::NotifyUriType::TRASH_ALBUM_URI, RegisterNotifyType::TRASHED_ALBUM_CHANGE },
};

const std::map<Notification::AccurateNotifyType, NotifyChangeType> MediaLibraryNotifyUtils::NOTIFY_CHANGE_TYPE_MAP = {
    { Notification::AccurateNotifyType::NOTIFY_ASSET_ADD, NotifyChangeType::NOTIFY_CHANGE_ADD },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE, NotifyChangeType::NOTIFY_CHANGE_UPDATE },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE, NotifyChangeType::NOTIFY_CHANGE_REMOVE },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD, NotifyChangeType::NOTIFY_CHANGE_ADD },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE, NotifyChangeType::NOTIFY_CHANGE_UPDATE },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE, NotifyChangeType::NOTIFY_CHANGE_REMOVE },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY, NotifyChangeType::NOTIFY_CHANGE_YUV_READY },
};

const std::unordered_map<int32_t, int32_t> ERROR_MAP = {
    { E_PERMISSION_DENIED,     OHOS_PERMISSION_DENIED_CODE },
    { -E_CHECK_SYSTEMAPP_FAIL, E_CHECK_SYSTEMAPP_FAIL },
    { JS_E_PARAM_INVALID,      JS_E_PARAM_INVALID },
    { OHOS_INVALID_PARAM_CODE, OHOS_INVALID_PARAM_CODE },
};

int32_t MediaLibraryNotifyUtils::GetUserDefineNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_USER_DEFINE_TYPE_MAP.find(type) == REGISTER_USER_DEFINE_TYPE_MAP.end()) {
        NAPI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_USER_DEFINE_TYPE_MAP.at(type);
    if (REGISTER_USER_DEFINE_URI_MAP.find(uriType) == REGISTER_USER_DEFINE_URI_MAP.end()) {
        NAPI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_USER_DEFINE_URI_MAP.at(uriType);
    return E_OK;
}

int32_t MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_ASSET_MANAGER_TYPE_MAP.find(type) == REGISTER_ASSET_MANAGER_TYPE_MAP.end()) {
        NAPI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_ASSET_MANAGER_TYPE_MAP.at(type);
    if (REGISTER_ASSET_MANAGER_URI_MAP.find(uriType) == REGISTER_ASSET_MANAGER_URI_MAP.end()) {
        NAPI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_ASSET_MANAGER_URI_MAP.at(uriType);
    return E_OK;
}

int32_t MediaLibraryNotifyUtils::GetRegisterNotifyType(const string &type, Notification::NotifyUriType &uriType)
{
    if (REGISTER_NOTIFY_TYPE_MAP.find(type) == REGISTER_NOTIFY_TYPE_MAP.end()) {
        NAPI_ERR_LOG("registerNotifyType is invalid");
        return E_ERR;
    }
    uriType = REGISTER_NOTIFY_TYPE_MAP.at(type);
    return E_OK;
}

int32_t MediaLibraryNotifyUtils::GetNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_TYPE_MAP.find(type) == REGISTER_TYPE_MAP.end()) {
        NAPI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_TYPE_MAP.at(type);
    if (REGISTER_URI_MAP.find(uriType) == REGISTER_URI_MAP.end()) {
        NAPI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_URI_MAP.at(uriType);
    return E_OK;
}

int32_t MediaLibraryNotifyUtils::GetNotifyChangeType(const Notification::AccurateNotifyType &notifyType)
{
    if (NOTIFY_CHANGE_TYPE_MAP.find(notifyType) == NOTIFY_CHANGE_TYPE_MAP.end()) {
        NAPI_ERR_LOG("notifyType is invalid");
        return E_ERR;
    }
    return static_cast<int32_t>(NOTIFY_CHANGE_TYPE_MAP.at(notifyType));
}

int32_t MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(const string &type, Notification::NotifyUriType &uriType)
{
    if (REGISTER_SINGLE_NOTIFY_TYPE_MAP.find(type) == REGISTER_SINGLE_NOTIFY_TYPE_MAP.end()) {
        NAPI_ERR_LOG("registerNotifyType is invalid");
        return E_ERR;
    }
    uriType = REGISTER_SINGLE_NOTIFY_TYPE_MAP.at(type);
    return E_OK;
}

int32_t MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_SINGLE_TYPE_MAP.find(type) == REGISTER_SINGLE_TYPE_MAP.end()) {
        NAPI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_SINGLE_TYPE_MAP.at(type);
    if (REGISTER_SINGLE_URI_MAP.find(uriType) == REGISTER_SINGLE_URI_MAP.end()) {
        NAPI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_SINGLE_URI_MAP.at(uriType);
    return E_OK;
}

napi_status MediaLibraryNotifyUtils::SetValueInt32(const napi_env& env, const char* name, const int32_t intValue,
    napi_value& result)
{
    if (result == nullptr) {
        NAPI_ERR_LOG("result is nullptr");
        return napi_invalid_arg;
    }

    napi_value value;
    napi_status status = napi_create_int32(env, intValue, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create int32 error! name: %{public}s, status: %{public}d, intValue: %{public}d",
            name, status, intValue);
        return status;
    }
    status = napi_set_named_property(env, result, name, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set int32 named property error! name: %{public}s, status: %{public}d, intValue: %{public}d",
            name, status, intValue);
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::SetValueString(const napi_env& env, const char* name, const string& stringValue,
    napi_value& result)
{
    if (result == nullptr) {
        NAPI_ERR_LOG("result is nullptr");
        return napi_invalid_arg;
    }

    napi_value value;
    napi_status status = napi_create_string_utf8(env, stringValue.c_str(), NAPI_AUTO_LENGTH, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create string error! name: %{public}s, status: %{public}d, stringValue: %{public}s",
            name, status, stringValue.c_str());
        return status;
    }
    status = napi_set_named_property(env, result, name, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set string named property error! name: %{public}s, status: %{public}d, stringValue: %{public}s",
            name, status, stringValue.c_str());
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::SetValueBool(const napi_env& env, const char* name, const bool boolValue,
    napi_value& result)
{
    if (result == nullptr) {
        NAPI_ERR_LOG("result is nullptr");
        return napi_invalid_arg;
    }

    napi_value value;
    napi_status status = napi_get_boolean(env, boolValue, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create string error! name: %{public}s, status: %{public}d, boolValue: %{public}d",
            name, status, boolValue);
        return status;
    }
    status = napi_set_named_property(env, result, name, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set string named property error! name: %{public}s, status: %{public}d, boolValue: %{public}d",
            name, status, boolValue);
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::SetValueInt64(const napi_env& env, const char* name, const int64_t intValue,
    napi_value& result)
{
    if (result == nullptr) {
        NAPI_ERR_LOG("result is nullptr");
        return napi_invalid_arg;
    }

    napi_value value;
    napi_status status = napi_create_int64(env, intValue, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create int64 error! name: %{public}s, status: %{public}d, intValue: %{public}" PRId64,
            name, status, intValue);
        return status;
    }
    status = napi_set_named_property(env, result, name, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set int64 named property error! name: %{public}s, status: %{public}d, intValue: %{public}" PRId64,
            name, status, intValue);
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::SetValueNull(const napi_env& env, const char* name, napi_value& result)
{
    if (result == nullptr) {
        NAPI_ERR_LOG("result is nullptr");
        return napi_invalid_arg;
    }

    napi_value nullValue = nullptr;
    napi_status status = napi_get_null(env, &nullValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi get null error! name: %{public}s, status: %{public}d", name, status);
        return status;
    }

    status = napi_set_named_property(env, result, name, nullValue);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set null named property error! name: %{public}s, status: %{public}d", name, status);
    }
    return status;
}

napi_value MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(napi_env env,
    const AccurateRefresh::PhotoAssetChangeInfo &photoAssetChangeInfo)
{
    if (photoAssetChangeInfo.fileId_ == AccurateRefresh::INVALID_INT32_VALUE) {
        return nullptr;
    }

    napi_value result = nullptr;
    napi_create_object(env, &result);

    SetValueString(env, "uri", photoAssetChangeInfo.uri_.c_str(), result);
    SetValueInt32(env, "mediaType", photoAssetChangeInfo.mediaType_, result);
    SetValueString(env, "albumUri", photoAssetChangeInfo.ownerAlbumUri_.c_str(), result);
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        return result;
    }
    SetValueInt32(env, "fileId", photoAssetChangeInfo.fileId_, result);
    SetValueString(env, "dateDay", photoAssetChangeInfo.dateDay_.c_str(), result);
    SetValueBool(env, "isFavorite", photoAssetChangeInfo.isFavorite_, result);
    SetValueBool(env, "isHidden", photoAssetChangeInfo.isHidden_, result);
    SetValueInt32(env, "strongAssociation", photoAssetChangeInfo.strongAssociation_, result);
    SetValueInt32(env, "thumbnailVisible", photoAssetChangeInfo.thumbnailVisible_, result);
    SetValueInt64(env, "dateTrashedMs", photoAssetChangeInfo.dateTrashedMs_, result);
    SetValueInt64(env, "dateAddedMs", photoAssetChangeInfo.dateAddedMs_, result);
    SetValueInt64(env, "dateTakenMs", photoAssetChangeInfo.dateTakenMs_, result);
    SetValueInt32(env, "position", photoAssetChangeInfo.position_, result);
    SetValueString(env, "displayName", photoAssetChangeInfo.displayName_, result);
    SetValueInt64(env, "size", photoAssetChangeInfo.size_, result);

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildPhotoAssetChangeData(napi_env env,
    const AccurateRefresh::PhotoAssetChangeData &photoAssetChangeData)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;

    napi_value assetBeforeChangeValue = BuildPhotoAssetChangeInfo(env, photoAssetChangeData.infoBeforeChange_);
    if (assetBeforeChangeValue == nullptr) {
        SetValueNull(env, "assetBeforeChange", result);
    } else {
        status = napi_set_named_property(env, result, "assetBeforeChange", assetBeforeChangeValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: assetBeforeChange");
        }
    }

    napi_value assetAfterChangeValue = BuildPhotoAssetChangeInfo(env, photoAssetChangeData.infoAfterChange_);
    if (assetAfterChangeValue == nullptr) {
        SetValueNull(env, "assetAfterChange", result);
    } else {
        status = napi_set_named_property(env, result, "assetAfterChange", assetAfterChangeValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: assetAfterChange");
        }
    }

    status = SetValueBool(env, "isContentChanged", photoAssetChangeData.isContentChanged_, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isContentChanged");
    }

    status = SetValueBool(env, "isDeleted", photoAssetChangeData.isDelete_, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isDeleted");
    }
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        return result;
    }

    status = SetValueInt32(env, "thumbnailChangeStatus", photoAssetChangeData.thumbnailChangeStatus_, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: thumbnailChangeStatus");
    }

    status = SetValueInt64(env, "version", photoAssetChangeData.version_, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: version");
    }

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildPhotoNapiArray(napi_env env,
    const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> &changeInfos)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildPhotoNapiArray");
    napi_value result = nullptr;
    napi_status status = napi_create_array_with_length(env, changeInfos.size(), &result);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    napi_value tmpValue = nullptr;
    status = napi_create_array_with_length(env, 0, &tmpValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");

    size_t resultIndex = 0;
    for (const auto &changeInfo : changeInfos) {
        if (const auto changeInfoPtr = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&changeInfo)) {
            napi_value assetValue = BuildPhotoAssetChangeData(env, *changeInfoPtr);
            if ((assetValue == nullptr) || (napi_set_element(env, result, resultIndex++, assetValue) != napi_ok)) {
                NAPI_ERR_LOG("failed to add element");
                return tmpValue;
            }
        } else {
            NAPI_ERR_LOG("failed to get changeInfoPtr");
            return nullptr;
        }
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfos(napi_env env,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildPhotoAssetChangeInfos");
    if (changeInfo == nullptr) {
        NAPI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;

    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type", GetNotifyChangeType(changeInfo->notifyType), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    napi_value assetResults = BuildPhotoNapiArray(env, changeInfo->changeInfos);
    if (assetResults == nullptr) {
        NAPI_ERR_LOG("Failed to build assetResults");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "assetChangeDatas", assetResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: assetChangeDatas");
        return nullptr;
    }

    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", changeInfo->isForRecheck, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildAlbumChangeInfo(napi_env env,
    const AccurateRefresh::AlbumChangeInfo &albumChangeInfo)
{
    if (albumChangeInfo.albumId_ == AccurateRefresh::INVALID_INT32_VALUE) {
        return nullptr;
    }

    napi_value result = nullptr;
    napi_create_object(env, &result);

    SetValueInt32(env, "albumType", albumChangeInfo.albumType_, result);
    SetValueInt32(env, "albumSubtype", albumChangeInfo.albumSubType_, result);
    SetValueString(env, "albumName", albumChangeInfo.albumName_, result);
    SetValueString(env, "albumUri", albumChangeInfo.albumUri_, result);
    SetValueInt64(env, "imageCount", albumChangeInfo.imageCount_, result);
    SetValueInt64(env, "videoCount", albumChangeInfo.videoCount_, result);
    SetValueInt64(env, "count", albumChangeInfo.count_, result);
    SetValueString(env, "coverUri", albumChangeInfo.coverUri_, result);
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        return result;
    }
    SetValueInt64(env, "hiddenCount", albumChangeInfo.hiddenCount_, result);
    SetValueString(env, "hiddenCoverUri", albumChangeInfo.hiddenCoverUri_, result);
    SetValueBool(env, "isCoverChanged", albumChangeInfo.isCoverChange_, result);
    SetValueBool(env, "isHiddenCoverChanged", albumChangeInfo.isHiddenCoverChange_, result);
    SetValueInt32(env, "orderSection", albumChangeInfo.orderSection_, result);
    SetValueInt32(env, "albumOrder", albumChangeInfo.albumsOrder_, result);
    if (!(albumChangeInfo.albumSubType_ >= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_START) &&
        albumChangeInfo.albumSubType_ <= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_END))) {
        SetValueBool(env, "hidden", static_cast<bool>(albumChangeInfo.hidden_), result);
    }

    napi_status status = napi_ok;
    napi_value coverInfoValue = BuildPhotoAssetChangeInfo(env, albumChangeInfo.coverInfo_);
    if (coverInfoValue != nullptr) {
        status = napi_set_named_property(env, result, "coverInfo", coverInfoValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: coverInfo");
        }
    }

    napi_value hiddenCoverInfoValue = BuildPhotoAssetChangeInfo(env, albumChangeInfo.hiddenCoverInfo_);
    if (hiddenCoverInfoValue != nullptr) {
        status = napi_set_named_property(env, result, "hiddenCoverInfo", hiddenCoverInfoValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: hiddenCoverInfo");
        }
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildAlbumChangeData(napi_env env,
    const AccurateRefresh::AlbumChangeData &albumChangeData)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;

    napi_value albumBeforeChangeValue = BuildAlbumChangeInfo(env, albumChangeData.infoBeforeChange_);
    if (albumBeforeChangeValue == nullptr) {
        SetValueNull(env, "albumBeforeChange", result);
    } else {
        status = napi_set_named_property(env, result, "albumBeforeChange", albumBeforeChangeValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: albumBeforeChange");
        }
    }

    napi_value albumAfterChangeValue = BuildAlbumChangeInfo(env, albumChangeData.infoAfterChange_);
    if (albumAfterChangeValue == nullptr) {
        SetValueNull(env, "albumAfterChange", result);
    } else {
        status = napi_set_named_property(env, result, "albumAfterChange", albumAfterChangeValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("set array named property error: albumAfterChange");
        }
    }

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        return result;
    }

    status = SetValueInt64(env, "version", albumChangeData.version_, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: version");
    }

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildAlbumNapiArray(napi_env env,
    const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> &changeInfos)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildAlbumNapiArray");
    napi_value result = nullptr;
    napi_status status = napi_create_array_with_length(env, changeInfos.size(), &result);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    napi_value tmpValue = nullptr;
    status = napi_create_array_with_length(env, 0, &tmpValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");

    size_t resultIndex = 0;
    for (const auto &changeInfo : changeInfos) {
        if (const auto changeInfoPtr = std::get_if<AccurateRefresh::AlbumChangeData>(&changeInfo)) {
            napi_value assetValue = BuildAlbumChangeData(env, *changeInfoPtr);
            if ((assetValue == nullptr) || (napi_set_element(env, result, resultIndex++, assetValue) != napi_ok)) {
                NAPI_ERR_LOG("failed to add element");
                return tmpValue;
            }
        } else {
            NAPI_ERR_LOG("failed to get changeInfoPtr");
            return nullptr;
        }
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildAlbumChangeInfos(napi_env env,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildAlbumChangeInfos");
    if (changeInfo == nullptr) {
        NAPI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;

    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type", GetNotifyChangeType(changeInfo->notifyType), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    napi_value albumResults = BuildAlbumNapiArray(env, changeInfo->changeInfos);
    if (albumResults == nullptr) {
        NAPI_ERR_LOG("Failed to build albumResults");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "albumChangeDatas", albumResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: albumChangeDatas");
        return nullptr;
    }

    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", changeInfo->isForRecheck, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildPhotoAssetRecheckChangeInfos(napi_env env)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildPhotoAssetRecheckChangeInfos");

    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    // 发送全查的通知，默认通知类型为UPDATE
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type",
        static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    status = SetValueNull(env, "assetChangeDatas", result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: assetChangeDatas");
        return nullptr;
    }

    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", true, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return result;
}

napi_value MediaLibraryNotifyUtils::BuildAlbumRecheckChangeInfos(napi_env env)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildAlbumRecheckChangeInfos");

    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    // 发送全查的通知，默认通知类型为UPDATE
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type",
        static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    status = SetValueNull(env, "albumChangeDatas", result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: albumChangeDatas");
        return nullptr;
    }

    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", true, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return result;
}

int32_t MediaLibraryNotifyUtils::ConvertToJsError(int32_t innerErr)
{
    int32_t err = JS_E_INNER_FAIL;
    if (ERROR_MAP.find(innerErr) != ERROR_MAP.end()) {
        err = ERROR_MAP.at(innerErr);
    }
    return err;
}

napi_status MediaLibraryNotifyUtils::BuildFileIdPercentSubInfos(napi_env env,
    const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result)
{
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "fileId", changeInfo->fileId, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: fileId");
        return status;
    }
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "percent", changeInfo->percent, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: percent");
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::BuildFileIdSubInfos(napi_env env,
    const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result)
{
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "fileId", changeInfo->fileId, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: fileId");
    }
    return status;
}

napi_status MediaLibraryNotifyUtils::BuildPauseReasonSubInfos(napi_env env,
    const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result)
{
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "autoPauseReason", changeInfo->autoPauseReason, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: fileId");
    }
    return status;
}

napi_value MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(napi_env env,
    const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildBatchDownloadProgressInfos");
    if (changeInfo == nullptr) {
        NAPI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "downloadEventType",
        static_cast<int32_t>(changeInfo->downloadAssetNotifyType), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: downloadEventType");
        return nullptr;
    }

    switch (changeInfo->downloadAssetNotifyType) {
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS:
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_FINISH:
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_FAILED:
            status = BuildFileIdPercentSubInfos(env, changeInfo, result);
            break;
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE:
            status = BuildFileIdSubInfos(env, changeInfo, result);
            break;
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE:
            status = BuildPauseReasonSubInfos(env, changeInfo, result);
            break;
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_RESUME:
        case Notification::DownloadAssetsNotifyType::DOWNLOAD_REFRESH:
            NAPI_INFO_LOG("set downloadProgressInfo AUTO Action");
            break;
        default:
            NAPI_ERR_LOG("Invalid registerUriType");
    }
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSingleAssetNapi(napi_env env,
    const std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> &changeInfo)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSingleAssetNapi");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSingleAssetNapi");
    napi_value result = nullptr;
    napi_status status = napi_create_array_with_length(env, ARGS_ONE, &result);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create error!");
    napi_value tmpValue = nullptr;
    status = napi_create_array_with_length(env, 0, &tmpValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create error!");
    size_t resultIndex = 0;
    napi_value assetValue = BuildPhotoAssetChangeData(env, *changeInfo);
    if ((assetValue == nullptr) || (napi_set_element(env, result, resultIndex++, assetValue) != napi_ok)) {
        NAPI_ERR_LOG("failed to add element");
        return tmpValue;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSinglePhotoAssetChangeInfos(napi_env env,
    const std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> &changeInfo,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfos)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSinglePhotoAssetChangeInfos");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSinglePhotoAssetChangeInfos");
    if (changeInfo == nullptr) {
        NAPI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type", GetNotifyChangeType(changeInfos->notifyType), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }
    MEDIA_INFO_LOG("photoAssetData %{public}s", changeInfo->ToString().c_str());
    napi_value assetResults = BuildSingleAssetNapi(env, changeInfo);
    if (assetResults == nullptr) {
        NAPI_ERR_LOG("Failed to build assetResults");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "assetChangeDatas", assetResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: assetChangeDatas");
        return nullptr;
    }
    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", changeInfos->isForRecheck, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSingleAlbumNapi(napi_env env,
    const std::shared_ptr<AccurateRefresh::AlbumChangeData> &changeInfo)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSingleAlbumNapi");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSingleAlbumNapi");
    napi_value result = nullptr;
    napi_status status = napi_create_array_with_length(env, ARGS_ONE, &result);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    napi_value tmpValue = nullptr;
    status = napi_create_array_with_length(env, 0, &tmpValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    size_t resultIndex = 0;
    napi_value assetValue = BuildAlbumChangeData(env, *changeInfo);
    if ((assetValue == nullptr) || (napi_set_element(env, result, resultIndex++, assetValue) != napi_ok)) {
        NAPI_ERR_LOG("failed to add element");
        return tmpValue;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSingleAlbumChangeInfos(napi_env env,
    const std::shared_ptr<AccurateRefresh::AlbumChangeData> &changeInfo,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfos)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSingleAlbumChangeInfos");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSingleAlbumChangeInfos");
    if (changeInfo == nullptr) {
        NAPI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type", GetNotifyChangeType(changeInfos->notifyType), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }
    MEDIA_INFO_LOG("photoAlbumData %{public}s", changeInfo->ToString().c_str());
    napi_value albumResults = BuildSingleAlbumNapi(env, changeInfo);
    if (albumResults == nullptr) {
        NAPI_ERR_LOG("Failed to build albumResults");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "albumChangeDatas", albumResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: albumChangeDatas");
        return nullptr;
    }
    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", changeInfos->isForRecheck, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSinglePhotoAssetRecheckChangeInfos(napi_env env)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSinglePhotoAssetRecheckChangeInfos");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSinglePhotoAssetRecheckChangeInfos");
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type",
        static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }
    status = SetValueNull(env, "assetChangeDatas", result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: assetChangeDatas");
        return nullptr;
    }
    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", true, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }
    return result;
}

napi_value MediaLibraryNotifyUtils::BuildSingleAlbumRecheckChangeInfos(napi_env env)
{
    NAPI_INFO_LOG("MediaLibraryNotifyUtils::BuildSingleAlbumRecheckChangeInfos");
    MediaLibraryTracer tracer;
    tracer.Start("BuildSingleAlbumRecheckChangeInfos");
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_status status = napi_ok;
    status = MediaLibraryNotifyUtils::SetValueInt32(env, "type",
        static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE), result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: type");
        return nullptr;
    }
    status = SetValueNull(env, "albumChangeDatas", result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: albumChangeDatas");
        return nullptr;
    }
    status = MediaLibraryNotifyUtils::SetValueBool(env, "isForRecheck", true, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }
    return result;
}
}  // namespace Media
}  // namespace OHOS
