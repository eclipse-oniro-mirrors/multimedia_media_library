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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_ANI_UTILS_H_
#define FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_ANI_UTILS_H_

#include <map>
#include <string>
#include <vector>
#include "ani_error.h"
#include "album_change_info.h"
#include "photo_asset_change_info.h"
#include "ani.h"
#include "media_change_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class RegisterNotifyType {
public:
    static const std::string PHOTO_CHANGE EXPORT;
    static const std::string HIDDEN_PHOTO_CHANGE EXPORT;
    static const std::string TRASH_PHOTO_CHANGE EXPORT;
    static const std::string PHOTO_ALBUM_CHANGE EXPORT;
    static const std::string HIDDEN_ALBUM_CHANGE EXPORT;
    static const std::string TRASHED_ALBUM_CHANGE EXPORT;
    static const std::string BATCH_DOWNLOAD_PROGRESS_CHANGE EXPORT;
    static const std::string SINGLE_PHOTO_CHANGE EXPORT;
    static const std::string SINGLE_PHOTO_ALBUM_CHANGE EXPORT;
    static const std::string USER_CLIENT_CHANGE EXPORT;
};

class MediaLibraryNotifyAniUtils {
public:
    struct AniArrayOperator {
        ani_class cls {};
        ani_method ctorMethod {};
        ani_method setMethod {};
    };
    static const std::map<std::string, Notification::NotifyUriType> REGISTER_ASSET_MANAGER_NOTIFY_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_ASSET_MANAGER_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_ASSET_MANAGER_URI_MAP;

    static const std::map<std::string, Notification::NotifyUriType> REGISTER_NOTIFY_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_URI_MAP;
    static const std::map<Notification::AccurateNotifyType, NotifyChangeType> NOTIFY_CHANGE_TYPE_MAP;

    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_USER_DEFINE_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_USER_DEFINE_URI_MAP;

    static int32_t GetAssetManagerNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, std::string &uri);
    static int32_t GetUserDefineNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, string &uri);
    static int32_t GetRegisterNotifyType(const std::string &type, Notification::NotifyUriType &uriType);
    static int32_t GetNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, std::string &uri);
    static NotifyChangeType GetNotifyChangeType(const Notification::AccurateNotifyType &notifyType);

    static ani_status CreateAniObject(ani_env* env, const std::string className, ani_object& result);
    static ani_status SetValueInt32(ani_env* env, const char* name, const int32_t intValue, ani_object& result);
    static ani_status SetValueInt64(ani_env* env, const char* name, const int64_t intValue, ani_object& result);
    static ani_status SetValueString(ani_env* env, const char* name, const std::string& stringValue,
        ani_object& result);
    static ani_status SetValueBool(ani_env* env, const char* name, const bool boolValue, ani_object& result);
    static ani_status SetValueEnum(ani_env* env, const char* name, const int32_t intValue, ani_object& result);
    static ani_status SetValueNull(ani_env* env, const char* name, ani_object& result);
    static ani_status InitAniArrayOperator(ani_env *env, AniArrayOperator &arrayOperator);
    static ani_status ToPhotoChangeInfoAniArray(ani_env *env,
        const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
        &changeInfos, ani_object &aniArray);
    static ani_object BuildPhotoAssetChangeInfo(ani_env* env,
        const AccurateRefresh::PhotoAssetChangeInfo &photoAssetChangeInfo);
    static ani_object BuildPhotoAssetChangeData(ani_env* env,
        const AccurateRefresh::PhotoAssetChangeData &photoAssetChangeData);
    static ani_object BuildPhotoAssetChangeInfos(ani_env* env,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);

    static ani_object BuildAlbumChangeInfo(ani_env* env, const AccurateRefresh::AlbumChangeInfo &albumChangeInfo);
    static ani_object BuildAlbumChangeData(ani_env* env, const AccurateRefresh::AlbumChangeData &albumChangeData);
    static ani_status ToAlbumChangeDataAniArray(ani_env *env,
        const vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
        &changeInfos, ani_object &aniArray);
    static ani_object BuildAlbumChangeInfos(ani_env* env,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    static ani_object BuildPhotoAssetRecheckChangeInfos(ani_env *env);
    static ani_object BuildAlbumRecheckChangeInfos(ani_env *env);
    static int32_t ConvertToJsError(int32_t innerErr);
};
}
}
#endif // FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_UTILS_H_