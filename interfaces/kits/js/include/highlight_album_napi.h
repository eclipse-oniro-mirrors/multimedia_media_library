/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_HIGHLIGHT_ALBUM_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_HIGHLIGHT_ALBUM_NAPI_H

#include "photo_album.h"

#include "datashare_values_bucket.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "napi_error.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

struct HighlightAlbumInfo {
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};

class HighlightAlbumNapi {
public:
    EXPORT HighlightAlbumNapi();
    EXPORT ~HighlightAlbumNapi();

    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value AnalysisAlbumInit(napi_env env, napi_value exports);
    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void *nativeObject, void *finalizeHint);

    EXPORT static napi_value JSGetHighlightAlbumInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetHighlightUserActionData(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetHighlightResource(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetOrderPosition(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetHighlightSubtitle(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteHighlightAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAnalysisAlbumGetRelationship(napi_env env, napi_callback_info info);

    napi_env highlightmEnv_;
    std::shared_ptr<PhotoAlbum> highlightAlbumPtr = nullptr;
    static thread_local PhotoAlbum *pAlbumData_;
    static thread_local napi_ref constructor_;
    static thread_local napi_ref analysisAlbumConstructor_;
};

struct HighlightAlbumNapiAsyncContext : public NapiError {
    int32_t changedRows;
    int32_t newCount;
    std::vector<std::string> fetchColumn;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<DataShare::DataShareValuesBucket> valuesBuckets;
    std::string networkId;
    std::string uri;
    std::unique_ptr<FetchResult<FileAsset>> fetchResult;
    ResultNapiType resultNapiType;
    std::string highlightAlbumInfo;
    std::vector<std::string> assetIdArray;
    std::vector<int32_t> orderPositionArray;
    int32_t albumId;
    std::string relationship;
    PhotoAlbumSubType subType;
    shared_ptr<PhotoAlbum> photoAlbumInstance_ = nullptr;

    int32_t highlightAlbumInfoType = HighlightAlbumInfoType::INVALID_INFO;
    int32_t highlightUserActionType = HighlightUserActionType::INVALID_USER_ACTION;

    int32_t actionData = 0;
    std::string resourceUri;
    std::string subtitle;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    napi_value napiArrayBuffer;

    HighlightAlbumNapi *objectInfo;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_HIGHLIGHT_ALBUM_NAPI_H
