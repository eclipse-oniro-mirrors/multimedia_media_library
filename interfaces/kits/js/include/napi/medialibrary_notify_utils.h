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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_UTILS_H_
#define FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_UTILS_H_

#include <map>
#include <string>
#include <vector>
#include "album_change_info.h"
#include "photo_asset_change_info.h"
#include "napi/native_api.h"
#include "media_change_info.h"
#include "userfile_manager_types.h"

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

class MediaLibraryNotifyUtils {
public:
    static const std::map<std::string, Notification::NotifyUriType> REGISTER_ASSET_MANAGER_NOTIFY_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_ASSET_MANAGER_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_ASSET_MANAGER_URI_MAP;

    static const std::map<std::string, Notification::NotifyUriType> REGISTER_NOTIFY_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_URI_MAP;
    static const std::map<Notification::AccurateNotifyType, NotifyChangeType> NOTIFY_CHANGE_TYPE_MAP;

    static const std::map<std::string, Notification::NotifyUriType> REGISTER_SINGLE_NOTIFY_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_SINGLE_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_SINGLE_URI_MAP;

    static const std::map<Notification::NotifyUriType, Notification::NotifyUriType> REGISTER_USER_DEFINE_TYPE_MAP;
    static const std::map<Notification::NotifyUriType, std::string> REGISTER_USER_DEFINE_URI_MAP;

    static int32_t GetAssetManagerNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, std::string &uri);
    static int32_t GetUserDefineNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, string &uri);
    static int32_t GetRegisterNotifyType(const std::string &type, Notification::NotifyUriType &uriType);
    static int32_t GetNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, std::string &uri);
    static int32_t GetNotifyChangeType(const Notification::AccurateNotifyType &notifyType);
    static int32_t GetSingleRegisterNotifyType(const string &type, Notification::NotifyUriType &uriType);
    static int32_t GetSingleNotifyTypeAndUri(const Notification::NotifyUriType type,
        Notification::NotifyUriType &uriType, string &uri);

    static napi_status SetValueInt32(const napi_env& env, const char* name, const int32_t intValue, napi_value& result);
    static napi_status SetValueInt64(const napi_env& env, const char* name, const int64_t intValue, napi_value& result);
    static napi_status SetValueString(const napi_env& env, const char* name, const std::string& stringValue,
        napi_value& result);
    static napi_status SetValueBool(const napi_env& env, const char* name, const bool boolValue, napi_value& result);
    static napi_status SetValueNull(const napi_env& env, const char* name, napi_value& result);

    static napi_value BuildPhotoAssetChangeInfo(napi_env env,
        const AccurateRefresh::PhotoAssetChangeInfo &photoAssetChangeInfo);
    static napi_value BuildPhotoAssetChangeData(napi_env env,
        const AccurateRefresh::PhotoAssetChangeData &photoAssetChangeData);
    static napi_value BuildPhotoNapiArray(napi_env env,
        const std::vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
        &changeInfos);
    static napi_value BuildPhotoAssetChangeInfos(napi_env env,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);

    static napi_value BuildAlbumChangeInfo(napi_env env, const AccurateRefresh::AlbumChangeInfo &albumChangeInfo);
    static napi_value BuildAlbumChangeData(napi_env env, const AccurateRefresh::AlbumChangeData &albumChangeData);
    static napi_value BuildAlbumNapiArray(napi_env env,
        const std::vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>>
        &changeInfos);
    static napi_value BuildAlbumChangeInfos(napi_env env,
        const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo);
    static napi_value BuildPhotoAssetRecheckChangeInfos(napi_env env);
    static napi_value BuildAlbumRecheckChangeInfos(napi_env env);
    static napi_value BuildBatchDownloadProgressInfos(napi_env env,
        const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo);
    static int32_t ConvertToJsError(int32_t innerErr);
    static napi_status BuildFileIdPercentSubInfos(napi_env env,
        const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result);
    static napi_status BuildFileIdSubInfos(napi_env env,
        const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result);
    static napi_status BuildPauseReasonSubInfos(napi_env env,
        const shared_ptr<Notification::AssetManagerNotifyInfo> &changeInfo, napi_value &result);
    static napi_value BuildSingleAssetNapi(napi_env env,
        const std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> &changeInfo);
    static napi_value BuildSinglePhotoAssetChangeInfos(napi_env env,
        const std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> &changeInfo,
        const shared_ptr<Notification::MediaChangeInfo> &changeInfos);
    static napi_value BuildSingleAlbumNapi(napi_env env,
        const std::shared_ptr<AccurateRefresh::AlbumChangeData> &changeInfo);
    static napi_value BuildSingleAlbumChangeInfos(napi_env env,
        const std::shared_ptr<AccurateRefresh::AlbumChangeData> &changeInfo,
        const shared_ptr<Notification::MediaChangeInfo> &changeInfos);
    static napi_value BuildSinglePhotoAssetRecheckChangeInfos(napi_env env);
    static napi_value BuildSingleAlbumRecheckChangeInfos(napi_env env);
};
}
}
#endif // FRAMEWORKS_INNERKITSIMPL_MEDIALIBRARY_NOTIFY_UTILS_H_