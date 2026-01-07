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
    DISMISS,
    SET_RELATIONSHIP,
    SET_ORDER_POSITION,
    MOVE_ASSETS_WITH_URI,
    RECOVER_ASSETS_WITH_URI,
    DELETE_ASSETS_WITH_URI,
    RESET_COVER_URI,
    SET_HIGHLIGHT_ATTRIBUTE,
    SET_UPLOAD_STATUS,
    SMART_MOVE_ASSETS,
    CREATE_ANALYSIS_ALBUM,
};

enum class ParameterType {
    PHOTO_ASSET,
    ASSET_URI,
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
    static constexpr int32_t PORTRAIT_REMOVED = -3;
    static const inline std::string TAG_ID = "tag_id";
    EXPORT MediaAlbumChangeRequestNapi() = default;
    EXPORT ~MediaAlbumChangeRequestNapi() override = default;

    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value MediaAnalysisAlbumChangeRequestInit(napi_env env, napi_value exports);
    EXPORT static napi_value MediaHighlightAlbumChangeRequestInit(napi_env env, napi_value exports);

    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetReferencePhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetTargetPhotoAlbumInstance() const;
    std::vector<std::string> GetAddAssetArray() const;
    std::vector<std::string> GetRemoveAssetArray() const;
    std::vector<std::string> GetRecoverAssetArray() const;
    std::vector<std::string> GetDeleteAssetArray() const;
    std::vector<std::string> GetDismissAssetArray() const;
    std::vector<std::pair<std::string, int32_t>> GetIdOrderPositionPairs() const;
    std::map<std::shared_ptr<PhotoAlbum>, std::vector<std::string>, PhotoAlbumPtrCompare> GetMoveMap() const;
    int32_t GetUserId() const;
    std::pair<int32_t, std::string> GetHighlightAlbumChangeAttributePair() const;
    void RecordMoveAssets(std::vector<std::string>& assetArray, std::shared_ptr<PhotoAlbum>& targetAlbum);
    void ClearAddAssetArray();
    void ClearRemoveAssetArray();
    void ClearRecoverAssetArray();
    void ClearDeleteAssetArray();
    void ClearDismissAssetArray();
    std::string GetRelationship() const;
    int32_t GetIsMe() const;
    void ClearMoveMap();
    napi_value ApplyChanges(napi_env env, napi_callback_info info) override;

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSGetAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateAlbumRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAlbumsWithUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAddAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRemoveAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSMoveAssetsImplement(napi_env env, napi_callback_info info,
        ParameterType parameterType);
    EXPORT static napi_value JSMoveAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSMoveAssetsWithUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRecoverAssetsImplement(napi_env env, napi_callback_info info,
        ParameterType parameterType);
    EXPORT static napi_value JSRecoverAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRecoverAssetsWithUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAssetsImplement(napi_env env, napi_callback_info info,
        ParameterType parameterType);
    EXPORT static napi_value JSDeleteAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAssetsWithUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetCoverUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSResetCoverUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPlaceBefore(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetDisplayLevel(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSMergeAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDismissAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetRelationship(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetIsMe(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDismiss(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetOrderPosition(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetUploadStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetHighlightAttribute(napi_env env, napi_callback_info info);
    EXPORT static bool CheckDismissAssetVaild(std::vector<std::string> &dismissAssets,
        std::vector<std::string> &newAssetArray);
    EXPORT static napi_value JSCreateAnalysisAlbumRequest(napi_env env, napi_callback_info info);

    bool CheckPortraitMergeAlbum();
    bool CheckChangeOperations(napi_env env);

    static thread_local napi_ref constructor_;
    static thread_local napi_ref mediaAnalysisAlbumChangeRequestConstructor_;
    static thread_local napi_ref mediaHighlightAlbumChangeRequestConstructor_;
    std::shared_ptr<PhotoAlbum> photoAlbum_ = nullptr;
    std::shared_ptr<PhotoAlbum> referencePhotoAlbum_ = nullptr;
    std::shared_ptr<PhotoAlbum> targetAlbum_ = nullptr;
    std::vector<std::string> assetsToAdd_;
    std::vector<std::string> assetsToRemove_;
    std::vector<std::string> assetsToRecover_;
    std::vector<std::string> assetsToDelete_;
    std::vector<std::string> dismissAssets_;
    int32_t userId_ = -1;
    std::map<std::shared_ptr<PhotoAlbum>, std::vector<std::string>, PhotoAlbumPtrCompare> moveMap_;
    std::vector<AlbumChangeOperation> albumChangeOperations_;
    std::vector<std::pair<std::string, int32_t>> idOrderPositionPairs_;
    std::string relationship_;
    int32_t isMe_ = 0;
    std::pair<int32_t, std::string> highlightAlbumChangeAttributePair_;
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
    std::vector<std::string> deleteIds;
    std::vector<int32_t> photoAlbumTypes;
    std::vector<int32_t> photoAlbumSubtypes;
    int32_t photoAlbumSubType = -1;
    bool allowUpload = false;
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_NAPI_H