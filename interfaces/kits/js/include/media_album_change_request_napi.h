/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_NAPI_H

#include <map>

#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "media_change_request_napi.h"
#include "photo_album.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class AlbumChangeOperation {
    CREATE_ALBUM,
    ADD_ASSETS,
    REMOVE_ASSETS,
    MOVE_ASSETS,
    RECOVER_ASSETS,
    DELETE_ASSETS,
    SET_ALBUM_NAME,
    SET_COVER_URI,
    ORDER_ALBUM,
    SET_DISPLAY_LEVEL,
    MERGE_ALBUM,
    DISMISS_ASSET,
    SET_IS_ME,
};

struct PhotoAlbumPtrCompare {
    bool operator()(const std::shared_ptr<PhotoAlbum>& album, const std::shared_ptr<PhotoAlbum>& albumCmp) const
    {
        if (album == nullptr || albumCmp == nullptr) {
            return album < albumCmp;
        }
        return album->GetAlbumId() < albumCmp->GetAlbumId();
    }
};

class MediaAlbumChangeRequestNapi : public MediaChangeRequestNapi {
public:
    EXPORT MediaAlbumChangeRequestNapi() = default;
    EXPORT ~MediaAlbumChangeRequestNapi() override = default;

    EXPORT static napi_value Init(napi_env env, napi_value exports);

    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetReferencePhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetTargetPhotoAlbumInstance() const;
    std::vector<std::string> GetAddAssetArray() const;
    std::vector<std::string> GetRemoveAssetArray() const;
    std::vector<std::string> GetRecoverAssetArray() const;
    std::vector<std::string> GetDeleteAssetArray() const;
    std::vector<std::string> GetDismissAssetArray() const;
    std::map<std::shared_ptr<PhotoAlbum>, std::vector<std::string>, PhotoAlbumPtrCompare> GetMoveMap() const;
    void RecordMoveAssets(std::vector<std::string>& assetArray, std::shared_ptr<PhotoAlbum>& targetAlbum);
    void ClearAddAssetArray();
    void ClearRemoveAssetArray();
    void ClearRecoverAssetArray();
    void ClearDeleteAssetArray();
    void ClearDismissAssetArray();
    void ClearMoveMap();
    napi_value ApplyChanges(napi_env env, napi_callback_info info) override;

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSGetAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateAlbumRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAddAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRemoveAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSMoveAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRecoverAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetCoverUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPlaceBefore(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetDisplayLevel(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSMergeAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDismissAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetIsMe(napi_env env, napi_callback_info info);
    EXPORT static bool CheckDismissAssetVaild(std::vector<std::string> &dismissAssets,
        std::vector<std::string> &newAssetArray);

    bool CheckPortraitMergeAlbum();
    bool CheckChangeOperations(napi_env env);

    static thread_local napi_ref constructor_;
    std::shared_ptr<PhotoAlbum> photoAlbum_ = nullptr;
    std::shared_ptr<PhotoAlbum> referencePhotoAlbum_ = nullptr;
    std::shared_ptr<PhotoAlbum> targetAlbum_ = nullptr;
    std::vector<std::string> assetsToAdd_;
    std::vector<std::string> assetsToRemove_;
    std::vector<std::string> assetsToRecover_;
    std::vector<std::string> assetsToDelete_;
    std::vector<std::string> dismissAssets_;
    std::map<std::shared_ptr<PhotoAlbum>, std::vector<std::string>, PhotoAlbumPtrCompare> moveMap_;
    std::vector<AlbumChangeOperation> albumChangeOperations_;
};

struct MediaAlbumChangeRequestAsyncContext : public NapiError {
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    MediaAlbumChangeRequestNapi* objectInfo;
    std::vector<AlbumChangeOperation> albumChangeOperations;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_NAPI_H