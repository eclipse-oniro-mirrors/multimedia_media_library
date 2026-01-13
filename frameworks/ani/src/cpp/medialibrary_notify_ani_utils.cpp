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

#define MLOG_TAG "AccurateRefresh::MediaLibraryNotifyAniUtils"
#include "medialibrary_notify_ani_utils.h"

#include "media_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"
#include "media_library_enum_ani.h"
#include "ani_class_name.h"

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
const std::string RegisterNotifyType::USER_CLIENT_CHANGE = "userDefineChange";

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyAniUtils::REGISTER_ASSET_MANAGER_TYPE_MAP = {
    { Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI,
        Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI },
};

const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyAniUtils::REGISTER_ASSET_MANAGER_URI_MAP = {
    { Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI, RegisterNotifyType::BATCH_DOWNLOAD_PROGRESS_CHANGE },
};

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyAniUtils::REGISTER_USER_DEFINE_TYPE_MAP = {
    { Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI, Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI },
};
 
const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyAniUtils::REGISTER_USER_DEFINE_URI_MAP = {
    { Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI, RegisterNotifyType::USER_CLIENT_CHANGE },
};

const std::map<std::string, Notification::NotifyUriType> MediaLibraryNotifyAniUtils::REGISTER_NOTIFY_TYPE_MAP = {
    { RegisterNotifyType::PHOTO_CHANGE, Notification::NotifyUriType::PHOTO_URI },
    { RegisterNotifyType::HIDDEN_PHOTO_CHANGE, Notification::NotifyUriType::HIDDEN_PHOTO_URI },
    { RegisterNotifyType::TRASH_PHOTO_CHANGE, Notification::NotifyUriType::TRASH_PHOTO_URI },
    { RegisterNotifyType::PHOTO_ALBUM_CHANGE, Notification::NotifyUriType::PHOTO_ALBUM_URI },
    { RegisterNotifyType::HIDDEN_ALBUM_CHANGE, Notification::NotifyUriType::HIDDEN_ALBUM_URI },
    { RegisterNotifyType::TRASHED_ALBUM_CHANGE, Notification::NotifyUriType::TRASH_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, Notification::NotifyUriType>
    MediaLibraryNotifyAniUtils::REGISTER_TYPE_MAP = {
    { Notification::NotifyUriType::PHOTO_URI, Notification::NotifyUriType::PHOTO_URI },
    { Notification::NotifyUriType::HIDDEN_PHOTO_URI, Notification::NotifyUriType::HIDDEN_PHOTO_URI },
    { Notification::NotifyUriType::TRASH_PHOTO_URI, Notification::NotifyUriType::TRASH_PHOTO_URI },
    { Notification::NotifyUriType::PHOTO_ALBUM_URI, Notification::NotifyUriType::PHOTO_ALBUM_URI },
    { Notification::NotifyUriType::HIDDEN_ALBUM_URI, Notification::NotifyUriType::HIDDEN_ALBUM_URI },
    { Notification::NotifyUriType::TRASH_ALBUM_URI, Notification::NotifyUriType::TRASH_ALBUM_URI },
};

const std::map<Notification::NotifyUriType, std::string> MediaLibraryNotifyAniUtils::REGISTER_URI_MAP = {
    { Notification::NotifyUriType::PHOTO_URI, RegisterNotifyType::PHOTO_CHANGE },
    { Notification::NotifyUriType::HIDDEN_PHOTO_URI, RegisterNotifyType::HIDDEN_PHOTO_CHANGE },
    { Notification::NotifyUriType::TRASH_PHOTO_URI, RegisterNotifyType::TRASH_PHOTO_CHANGE },
    { Notification::NotifyUriType::PHOTO_ALBUM_URI, RegisterNotifyType::PHOTO_ALBUM_CHANGE },
    { Notification::NotifyUriType::HIDDEN_ALBUM_URI, RegisterNotifyType::HIDDEN_ALBUM_CHANGE },
    { Notification::NotifyUriType::TRASH_ALBUM_URI, RegisterNotifyType::TRASHED_ALBUM_CHANGE },
};

const std::map<Notification::AccurateNotifyType, NotifyChangeType>
    MediaLibraryNotifyAniUtils::NOTIFY_CHANGE_TYPE_MAP = {
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

int32_t MediaLibraryNotifyAniUtils::GetUserDefineNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_USER_DEFINE_TYPE_MAP.find(type) == REGISTER_USER_DEFINE_TYPE_MAP.end()) {
        ANI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_USER_DEFINE_TYPE_MAP.at(type);
    if (REGISTER_USER_DEFINE_URI_MAP.find(uriType) == REGISTER_USER_DEFINE_URI_MAP.end()) {
        ANI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_USER_DEFINE_URI_MAP.at(uriType);
    return E_OK;
}

int32_t MediaLibraryNotifyAniUtils::GetAssetManagerNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_ASSET_MANAGER_TYPE_MAP.find(type) == REGISTER_ASSET_MANAGER_TYPE_MAP.end()) {
        ANI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_ASSET_MANAGER_TYPE_MAP.at(type);
    if (REGISTER_ASSET_MANAGER_URI_MAP.find(uriType) == REGISTER_ASSET_MANAGER_URI_MAP.end()) {
        ANI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_ASSET_MANAGER_URI_MAP.at(uriType);
    return E_OK;
}

int32_t MediaLibraryNotifyAniUtils::GetRegisterNotifyType(const string &type, Notification::NotifyUriType &uriType)
{
    if (REGISTER_NOTIFY_TYPE_MAP.find(type) == REGISTER_NOTIFY_TYPE_MAP.end()) {
        ANI_ERR_LOG("registerNotifyType is invalid");
        return E_ERR;
    }
    uriType = REGISTER_NOTIFY_TYPE_MAP.at(type);
    return E_OK;
}

int32_t MediaLibraryNotifyAniUtils::GetNotifyTypeAndUri(const Notification::NotifyUriType type,
    Notification::NotifyUriType &uriType, string &uri)
{
    if (REGISTER_TYPE_MAP.find(type) == REGISTER_TYPE_MAP.end()) {
        ANI_ERR_LOG("type is invalid");
        return E_ERR;
    }
    uriType = REGISTER_TYPE_MAP.at(type);
    if (REGISTER_URI_MAP.find(uriType) == REGISTER_URI_MAP.end()) {
        ANI_ERR_LOG("uriType is invalid");
        return E_ERR;
    }
    uri = REGISTER_URI_MAP.at(uriType);
    return E_OK;
}

NotifyChangeType MediaLibraryNotifyAniUtils::GetNotifyChangeType(const Notification::AccurateNotifyType &notifyType)
{
    if (NOTIFY_CHANGE_TYPE_MAP.find(notifyType) == NOTIFY_CHANGE_TYPE_MAP.end()) {
        ANI_ERR_LOG("notifyType is invalid");
        return NotifyChangeType::NOTIFY_CHANGE_INVALID;
    }
    return NOTIFY_CHANGE_TYPE_MAP.at(notifyType);
}

ani_status MediaLibraryNotifyAniUtils::CreateAniObject(ani_env* env, const std::string className,
    ani_object& result)
{
    ani_class cls {};
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls),
        "Can't find class %{public}s", className.c_str());
    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", nullptr, &method),
        "Can't find method <ctor> in %{public}s", className.c_str());
    CHECK_STATUS_RET(env->Object_New(cls, method, &result),
        "Call method <ctor> fail");
    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueInt32(ani_env* env, const char* name, const int32_t intValue,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");
    ani_int value = {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniInt(env, intValue, value),
        "ToAniInt failed! intValue: %{public}s", name);
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Int(result, name, value),
        "Set int32 named property error! field: %{public}s", name);
    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueString(ani_env* env, const char* name, const string& stringValue,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");
    ani_string strVal = {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, stringValue, strVal),
        "Call ToAniString %{public}s fail", name);
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, name, static_cast<ani_ref>(strVal)),
        "Call Object_SetPropertyByName_Ref fail: %{public}s", name);

    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueBool(ani_env* env, const char* name, const bool boolValue,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");
    ani_boolean boolVal = boolValue ? ANI_TRUE : ANI_FALSE;
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Boolean(result, name, boolVal),
        "Call Object_SetPropertyByName_Ref fail: %{public}s", name);
    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueInt64(ani_env* env, const char* name, const int64_t intValue,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");

    ani_long longVal = {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniLong(env, intValue, longVal),
        "ToAniLong failed! longVal: %{public}s", name);
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Long(result, name, longVal),
        "Set int64 named property error! field: %{public}s", name);
    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueEnum(ani_env* env, const char* name, const int32_t intValue,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");

    ani_enum_item enumVal = {};
    if (strcmp(name, "mediaType") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<PhotoType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "strongAssociation") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<StrongAssociationType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "thumbnailVisible") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<ThumbnailVisibility>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "position") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<PhotoPositionType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "type") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<NotifyChangeType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "albumType") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<PhotoAlbumType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else if (strcmp(name, "albumSubtype") == 0) {
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, static_cast<PhotoAlbumSubType>(intValue), enumVal),
            "ToAniEnum failed! enumVal: %{public}s", name);
    } else {
        ANI_ERR_LOG("SetValueEnum no such name: %{public}s", name);
        return ANI_ERROR;
    }

    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, name, static_cast<ani_ref>(enumVal)),
        "Call Object_SetPropertyByName_Ref fail %{public}s", name);

    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::SetValueNull(ani_env* env, const char* name,
    ani_object& result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(result != nullptr, ANI_ERROR, "result is nullptr");

    ani_object nullVal = nullptr;
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, name, static_cast<ani_ref>(nullVal)),
        "Set nullVal named property error! field: %{public}s", name);
    return ANI_OK;
}

ani_status MediaLibraryNotifyAniUtils::InitAniArrayOperator(ani_env *env, AniArrayOperator &arrayOperator)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const std::string className = "std.core.Array";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &(arrayOperator.cls)), "Can't find std.core.Array.");

    CHECK_STATUS_RET(env->Class_FindMethod(arrayOperator.cls, "<ctor>", "i:", &(arrayOperator.ctorMethod)),
        "Can't find method <ctor> in std.core.Array.");

    CHECK_STATUS_RET(env->Class_FindMethod(arrayOperator.cls, "$_set", "iY:",
        &(arrayOperator.setMethod)), "Can't find method $_set in std.core.Array.");
    return ANI_OK;
}


ani_status MediaLibraryNotifyAniUtils::ToPhotoChangeInfoAniArray(ani_env *env,
    const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
    &changeInfos,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    size_t resultIndex = 0;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");
    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, changeInfos.size()),
        "Call method <ctor> failed.");
    for (const auto &changeInfo : changeInfos) {
        if (const auto changeInfoPtr = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&changeInfo)) {
            ani_object assetValue = MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeData(env, *changeInfoPtr);
            CHECK_COND_RET(assetValue != nullptr, ANI_ERROR, "CreatePhotoAsset failed");
            CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod,
                (ani_int)resultIndex++, assetValue),
                "Call method $_set failed.");
        } else {
            ANI_ERR_LOG("failed to get changeInfoPtr");
            return ANI_ERROR;
        }
    }
    return ANI_OK;
}

ani_object MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeInfo(ani_env *env,
    const AccurateRefresh::PhotoAssetChangeInfo &photoAssetChangeInfo)
{
    if (photoAssetChangeInfo.fileId_ == AccurateRefresh::INVALID_INT32_VALUE) {
        return nullptr;
    }

    ani_object retObj = nullptr;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFO_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFO_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    SetValueString(env, "uri", photoAssetChangeInfo.uri_.c_str(), retObj);
    SetValueString(env, "albumUri", photoAssetChangeInfo.ownerAlbumUri_.c_str(), retObj);
    SetValueEnum(env, "mediaType", photoAssetChangeInfo.mediaType_, retObj);
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        return retObj;
    }

    SetValueInt32(env, "fileId", photoAssetChangeInfo.fileId_, retObj);
    SetValueString(env, "dateDay", photoAssetChangeInfo.dateDay_.c_str(), retObj);
    SetValueBool(env, "isFavorite", photoAssetChangeInfo.isFavorite_, retObj);
    SetValueBool(env, "isHidden", photoAssetChangeInfo.isHidden_, retObj);
    SetValueEnum(env, "strongAssociation", photoAssetChangeInfo.strongAssociation_, retObj);
    SetValueEnum(env, "thumbnailVisible", photoAssetChangeInfo.thumbnailVisible_, retObj);
    SetValueInt64(env, "dateTrashedMs", photoAssetChangeInfo.dateTrashedMs_, retObj);
    SetValueInt64(env, "dateAddedMs", photoAssetChangeInfo.dateAddedMs_, retObj);
    SetValueInt64(env, "dateTakenMs", photoAssetChangeInfo.dateTakenMs_, retObj);

    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeData(ani_env* env,
    const AccurateRefresh::PhotoAssetChangeData &photoAssetChangeData)
{
    ani_object retObj = nullptr;
    ani_status status = ANI_OK;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_DATA_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_DATA_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    ani_object assetBeforeChangeValue = BuildPhotoAssetChangeInfo(env, photoAssetChangeData.infoBeforeChange_);
    if (assetBeforeChangeValue == nullptr) {
        SetValueNull(env, "assetBeforeChange", retObj);
    } else {
        status = env->Object_SetPropertyByName_Ref(retObj, "assetBeforeChange", assetBeforeChangeValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: assetBeforeChange");
        }
    }

    ani_object assetAfterChangeValue = BuildPhotoAssetChangeInfo(env, photoAssetChangeData.infoAfterChange_);
    if (assetAfterChangeValue == nullptr) {
        SetValueNull(env, "assetAfterChange", retObj);
    } else {
        status = env->Object_SetPropertyByName_Ref(retObj, "assetAfterChange", assetAfterChangeValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: assetAfterChange");
        }
    }

    status = SetValueBool(env, "isContentChanged", photoAssetChangeData.isContentChanged_, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: isContentChanged");
    }

    status = SetValueBool(env, "isDeleted", photoAssetChangeData.isDelete_, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: isDeleted");
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        return retObj;
    }

    status = SetValueInt32(env, "thumbnailChangeStatus", photoAssetChangeData.thumbnailChangeStatus_, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: thumbnailChangeStatus");
    }

    status = SetValueInt64(env, "version", photoAssetChangeData.version_, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: version");
    }

    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeInfos(ani_env *env,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildPhotoAssetChangeInfos");
    CHECK_COND_RET(changeInfo != nullptr, nullptr, "retObj is nullptr");

    ani_object retObj = nullptr;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFOS_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFOS_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    ani_enum_item type;
    NotifyChangeType ChangeType = GetNotifyChangeType(changeInfo->notifyType);
    if (ChangeType == NotifyChangeType::NOTIFY_CHANGE_INVALID) {
        return nullptr;
    }
    CHECK_COND_RET(MediaLibraryEnumAni::ToAniEnum(env, ChangeType, type) == ANI_OK,
        nullptr, "Call ToAniEnum NotifyChangeType fail");
    CHECK_COND_RET(env->Object_SetPropertyByName_Ref(retObj, "type", static_cast<ani_ref>(type)) == ANI_OK, nullptr,
        "Call Object_SetPropertyByName_Ref fail");

    ani_object assetResults = nullptr;
    CHECK_COND_RET(ToPhotoChangeInfoAniArray(env, changeInfo->changeInfos, assetResults)
        == ANI_OK, nullptr, "Call ToPhotoChangeInfoAniArray NotifyChangeType fail");

    CHECK_COND_RET(env->Object_SetPropertyByName_Ref(retObj, "assetChangeDatas",
        static_cast<ani_ref>(assetResults)) == ANI_OK, nullptr,
        "Set object named property error! field: assetChangeDatas");

    ani_boolean isForRecheck = changeInfo->isForRecheck? ANI_TRUE : ANI_FALSE;
    CHECK_COND_RET(env->Object_SetPropertyByName_Boolean(retObj, "isForRecheck", isForRecheck) == ANI_OK, nullptr,
        "Call Object_SetPropertyByName_Ref fail");

    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildAlbumChangeInfo(ani_env* env,
    const AccurateRefresh::AlbumChangeInfo &albumChangeInfo)
{
    if (albumChangeInfo.albumId_ == AccurateRefresh::INVALID_INT32_VALUE) {
        return nullptr;
    }

    ani_object retObj = nullptr;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_ALBUM_CHANGE_INFO_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_ALBUM_CHANGE_INFO_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    SetValueEnum(env, "albumType", albumChangeInfo.albumType_, retObj);
    SetValueEnum(env, "albumSubtype", albumChangeInfo.albumSubType_, retObj);
    SetValueString(env, "albumName", albumChangeInfo.albumName_, retObj);
    SetValueString(env, "albumUri", albumChangeInfo.albumUri_, retObj);
    SetValueInt64(env, "imageCount", albumChangeInfo.imageCount_, retObj);
    SetValueInt64(env, "videoCount", albumChangeInfo.videoCount_, retObj);
    SetValueInt64(env, "count", albumChangeInfo.count_, retObj);
    SetValueString(env, "coverUri", albumChangeInfo.coverUri_, retObj);
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        return retObj;
    }
    SetValueInt64(env, "hiddenCount", albumChangeInfo.hiddenCount_, retObj);
    SetValueString(env, "hiddenCoverUri", albumChangeInfo.hiddenCoverUri_, retObj);
    SetValueBool(env, "isCoverChanged", albumChangeInfo.isCoverChange_, retObj);
    SetValueBool(env, "isHiddenCoverChanged", albumChangeInfo.isHiddenCoverChange_, retObj);

    ani_status status = ANI_OK;
    ani_object coverInfoValue = BuildPhotoAssetChangeInfo(env, albumChangeInfo.coverInfo_);
    if (coverInfoValue != nullptr) {
        status = env->Object_SetPropertyByName_Ref(retObj, "coverInfo", coverInfoValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: coverInfoValue");
        }
    }

    ani_object hiddenCoverInfoValue = BuildPhotoAssetChangeInfo(env, albumChangeInfo.hiddenCoverInfo_);
    if (hiddenCoverInfoValue != nullptr) {
        status = env->Object_SetPropertyByName_Ref(retObj, "hiddenCoverInfo", hiddenCoverInfoValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: hiddenCoverInfoValue");
        }
    }
    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildAlbumChangeData(ani_env *env,
    const AccurateRefresh::AlbumChangeData &albumChangeData)
{
    ani_status status = ANI_OK;
    ani_object retObj = nullptr;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_ALBUM_CHANGE_DATA_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_ALBUM_CHANGE_DATA_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    ani_object albumBeforeChangeValue = BuildAlbumChangeInfo(env, albumChangeData.infoBeforeChange_);
    if (albumBeforeChangeValue == nullptr) {
        SetValueNull(env, "albumBeforeChange", retObj);
    } else {
        status = env->Object_SetPropertyByName_Ref(retObj, "albumBeforeChange",
            static_cast<ani_ref>(albumBeforeChangeValue));
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: albumBeforeChange");
        }
    }

    ani_object albumAfterChangeValue = BuildAlbumChangeInfo(env, albumChangeData.infoAfterChange_);
    if (albumAfterChangeValue == nullptr) {
        SetValueNull(env, "albumAfterChange", retObj);
    } else {
        status = env->Object_SetPropertyByName_Ref(retObj, "albumAfterChange",
            static_cast<ani_ref>(albumAfterChangeValue));
        if (status != ANI_OK) {
            ANI_ERR_LOG("set array named property error: albumAfterChange");
        }
    }

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        return retObj;
    }

    status = SetValueInt64(env, "version", albumChangeData.version_, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: version");
    }

    return retObj;
}

ani_status MediaLibraryNotifyAniUtils::ToAlbumChangeDataAniArray(ani_env *env,
    const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
    &changeInfos,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    size_t resultIndex = 0;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");
    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, changeInfos.size()),
        "Call method <ctor> failed.");
    for (const auto &changeInfo : changeInfos) {
        if (const auto changeInfoPtr = std::get_if<AccurateRefresh::AlbumChangeData>(&changeInfo)) {
            ani_object assetValue = MediaLibraryNotifyAniUtils::BuildAlbumChangeData(env, *changeInfoPtr);
            CHECK_COND_RET(assetValue != nullptr, ANI_ERROR, "CreatePhotoAsset failed");
            CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod,
                (ani_int)resultIndex++, assetValue),
                "Call method $_set failed.");
        } else {
            ANI_ERR_LOG("failed to get changeInfoPtr");
            return ANI_ERROR;
        }
    }
    return ANI_OK;
}

ani_object MediaLibraryNotifyAniUtils::BuildAlbumChangeInfos(ani_env* env,
    const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildAlbumChangeInfos");
    if (changeInfo == nullptr) {
        ANI_ERR_LOG("Invalid changeInfo");
        return nullptr;
    }
    ani_object retObj = nullptr;
    ani_status status = ANI_OK;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_ALBUM_CHANGE_INFOS_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_ALBUM_CHANGE_INFOS_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    NotifyChangeType ChangeType = GetNotifyChangeType(changeInfo->notifyType);
    if (ChangeType == NotifyChangeType::NOTIFY_CHANGE_INVALID) {
        return nullptr;
    }
    status = MediaLibraryNotifyAniUtils::SetValueEnum(env, "type", GetNotifyChangeType(changeInfo->notifyType), retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    ani_object albumResults = nullptr;
    CHECK_COND_RET(ToAlbumChangeDataAniArray(env, changeInfo->changeInfos, albumResults)
        == ANI_OK, nullptr, "Call ToAlbumChangeDataAniArray fail");
    CHECK_COND_RET(env->Object_SetPropertyByName_Ref(retObj, "albumChangeDatas", albumResults) == ANI_OK,
        nullptr, "Call Object_SetPropertyByName_Ref albumChangeDatas fail");

    status = MediaLibraryNotifyAniUtils::SetValueBool(env, "isForRecheck", changeInfo->isForRecheck, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildPhotoAssetRecheckChangeInfos(ani_env *env)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildPhotoAssetRecheckChangeInfos");

    ani_object retObj = nullptr;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFOS_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFOS_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");

    ani_enum_item type;
    CHECK_COND_RET(MediaLibraryEnumAni::ToAniEnum(env, NotifyChangeType::NOTIFY_CHANGE_UPDATE, type) == ANI_OK,
        nullptr, "Call ToAniEnum NotifyChangeType fail");
    CHECK_COND_RET(env->Object_SetPropertyByName_Ref(retObj, "type", static_cast<ani_ref>(type)) == ANI_OK, nullptr,
        "Call Object_SetPropertyByName_Ref fail");

    ani_object assetChangeDatas = nullptr;
    CHECK_COND_RET(env->Object_SetPropertyByName_Ref(retObj, "assetChangeDatas",
        static_cast<ani_ref>(assetChangeDatas)) == ANI_OK,
        nullptr, "Call Object_SetPropertyByName_Ref fail");

    ani_boolean isForRecheck = ANI_TRUE;
    CHECK_COND_RET(env->Object_SetPropertyByName_Boolean(retObj, "isForRecheck", isForRecheck) == ANI_OK, nullptr,
        "Call Object_SetPropertyByName_Ref fail");

    return retObj;
}

ani_object MediaLibraryNotifyAniUtils::BuildAlbumRecheckChangeInfos(ani_env *env)
{
    MediaLibraryTracer tracer;
    tracer.Start("BuildAlbumRecheckChangeInfos");

    ani_object retObj = nullptr;
    ani_status status = ANI_OK;
    CHECK_COND_RET(CreateAniObject(env, PAH_ANI_CLASS_ALBUM_CHANGE_INFOS_HANDLE, retObj) == ANI_OK, nullptr,
        "CreateAniObject fail: %{public}s", PAH_ANI_CLASS_ALBUM_CHANGE_INFOS_HANDLE.c_str());
    CHECK_COND_RET(retObj != nullptr, nullptr, "retObj is nullptr");
    // 发送全查的通知，默认通知类型为UPDATE
    status = SetValueEnum(env, "type", static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE), retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: type");
        return nullptr;
    }

    status = SetValueNull(env, "albumChangeDatas", retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: albumChangeDatas");
        return nullptr;
    }

    status = SetValueBool(env, "isForRecheck", true, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: isForRecheck");
        return nullptr;
    }

    return retObj;
}

int32_t MediaLibraryNotifyAniUtils::ConvertToJsError(int32_t innerErr)
{
    int32_t err = JS_E_INNER_FAIL;
    if (ERROR_MAP.find(innerErr) != ERROR_MAP.end()) {
        err = ERROR_MAP.at(innerErr);
    }
    return err;
}
}  // namespace Media
}  // namespace OHOS
