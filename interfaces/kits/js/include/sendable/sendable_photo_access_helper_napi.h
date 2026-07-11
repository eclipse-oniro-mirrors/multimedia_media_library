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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_ACCESS_HELPER_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_ACCESS_HELPER_NAPI_H_

#include <mutex>
#include <vector>
#include "abs_shared_result_set.h"
#include "album_napi.h"
#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "data_ability_predicates.h"
#include "sendable_fetch_file_result_napi.h"
#include "sendable_file_asset_napi.h"
#include "napi_base_context.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "photo_album.h"
#include "smart_album_asset.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "uv.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string SENDABLE_PHOTOACCESSHELPER_NAPI_CLASS_NAME = "SendablePhotoAccessHelper";

class SendablePhotoAccessHelper {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);

    static void OnThumbnailGenerated(napi_env env, napi_value cb, void *context, void *data);
    int32_t GetUserId();
    void SetUserId(const int32_t &userId);

    EXPORT SendablePhotoAccessHelper();
    EXPORT ~SendablePhotoAccessHelper();

    static std::mutex sUserFileClientMutex_;

private:
    EXPORT static void MediaLibraryNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value MediaLibraryNapiConstructor(napi_env env, napi_callback_info info);
   
    EXPORT static napi_value JSRelease(napi_env env, napi_callback_info info);

    EXPORT static napi_value CreateMediaTypeUserFileEnum(napi_env env);
    EXPORT static napi_value CreateKeyFrameThumbnailTypeEnum(napi_env env);
    EXPORT static napi_value CreatePhotoSubTypeEnum(napi_env env);
    EXPORT static napi_value CreateAlbumTypeEnum(napi_env env);
    EXPORT static napi_value CreateAlbumSubTypeEnum(napi_env env);
    EXPORT static napi_value CreatePositionTypeEnum(napi_env env);
    EXPORT static napi_value CreateMovingPhotoEffectModeEnum(napi_env env);
    EXPORT static napi_value CreateDynamicRangeTypeEnum(napi_env env);

    EXPORT static napi_value GetPhotoAccessHelper(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCreatePhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetBurstAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetSharedPhotoAssets(napi_env env, napi_callback_info info);

    EXPORT static napi_value PahGetHiddenAlbums(napi_env env, napi_callback_info info);

    napi_env env_;
    int32_t userId_ = -1;

    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local napi_ref sMediaTypeEnumRef_;
    static thread_local napi_ref sKeyFrameThumbnailTypeRef_;
    static thread_local napi_ref sPhotoKeysEnumRef_;
    static thread_local napi_ref sAlbumKeyEnumRef_;
    static thread_local napi_ref sAlbumType_;
    static thread_local napi_ref sAlbumSubType_;
    static thread_local napi_ref sDeliveryModeEnumRef_;
    static thread_local napi_ref sPositionTypeEnumRef_;
    static thread_local napi_ref sHiddenPhotosDisplayModeEnumRef_;
    static thread_local napi_ref sPhotoSubType_;
    static thread_local napi_ref sNotifyType_;
    static thread_local napi_ref sDefaultChangeUriRef_;
    static thread_local napi_ref sAnalysisType_;
    static thread_local napi_ref sRequestPhotoTypeEnumRef_;
    static thread_local napi_ref sResourceTypeEnumRef_;
    static thread_local napi_ref sSourceModeEnumRef_;
    static thread_local napi_ref sHighlightAlbumInfoType_;
    static thread_local napi_ref sHighlightUserActionType_;
    static thread_local napi_ref sHighlightAlbumChangeAttributeEnumRef_;
    static thread_local napi_ref sMovingPhotoEffectModeEnumRef_;
    static thread_local napi_ref sDynamicRangeTypeEnumRef_;

    static std::mutex sOnOffMutex_;
};

constexpr int32_t SENDABLE_DEFAULT_PRIVATEALBUMTYPE = 3;
struct SendablePhotoAccessHelperAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    bool isDelete;
    bool isCreateByComponent;
    bool needSystemApp = false;
    NapiAssetType assetType;
    AlbumType albumType;
    SendablePhotoAccessHelper *objectInfo;
    std::string selection;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::string uri;
    std::string networkId;
    std::string extendArgs;
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    std::unique_ptr<FetchResult<AlbumAsset>> fetchAlbumResult;
    std::unique_ptr<FetchResult<PhotoAlbum>> fetchPhotoAlbumResult;
    std::unique_ptr<FetchResult<SmartAlbumAsset>> fetchSmartAlbumResult;
    std::unique_ptr<FileAsset> fileAsset;
    std::unique_ptr<PhotoAlbum> photoAlbumData;
    std::unique_ptr<SmartAlbumAsset> smartAlbumData;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    unsigned int dirType = 0;
    int32_t privateAlbumType = SENDABLE_DEFAULT_PRIVATEALBUMTYPE;
    int32_t retVal;
    std::string directoryRelativePath;
    std::vector<std::unique_ptr<AlbumAsset>> albumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> smartAlbumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> privateSmartAlbumNativeArray;
    Ability *ability_;
    std::string storeMediaSrc;
    int32_t imagePreviewIndex;
    int32_t parentSmartAlbumId = 0;
    int32_t smartAlbumId = -1;
    int32_t isLocationAlbum = 0;
    int32_t isHighlightAlbum = 0;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::string tableName;
    std::vector<uint32_t> mediaTypes;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
    std::vector<std::string> uris;
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    std::string formId;
    std::string indexProgress;
};

struct SendablePhotoAccessHelperInitContext : public NapiError  {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_ref resultRef_;
    sptr<IRemoteObject> token_;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_PHOTO_ACCESS_HELPER_NAPI_H_
