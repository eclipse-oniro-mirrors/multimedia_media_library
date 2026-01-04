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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_ANI_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ani_error.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "photo_album.h"
#include "media_change_request_ani.h"

namespace OHOS {
namespace Media {

class AlbumHandle {
};

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
    SET_ORDER_POSITION,
    MOVE_ASSETS_WITH_URI,
    RECOVER_ASSETS_WITH_URI,
    DELETE_ASSETS_WITH_URI,
    RESET_COVER_URI,
    SMART_MOVE_ASSET,
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

class MediaAlbumChangeRequestAni : public MediaChangeRequestAni {
public:
    static constexpr int32_t PORTRAIT_REMOVED = -3;
    static const inline std::string TAG_ID = "tag_id";
    MediaAlbumChangeRequestAni() = default;
    ~MediaAlbumChangeRequestAni() = default;
    static MediaAlbumChangeRequestAni* Unwrap(ani_env *env, ani_object mediaAlbumChangeRequestHandle);
    static ani_status Init(ani_env *env);
    static ani_status MediaAnalysisAlbumChangeRequestInit(ani_env *env);
    static ani_status Constructor(ani_env *env, ani_object object, ani_object albumHandle);
    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetTargetPhotoAlbumInstance() const;
    std::shared_ptr<PhotoAlbum> GetReferencePhotoAlbumInstance() const;
    std::vector<std::string> GetAddAssetArray() const;
    std::vector<std::string> GetRemoveAssetArray() const;
    std::vector<std::string> GetRecoverAssetArray() const;
    std::vector<std::string> GetDeleteAssetArray() const;
    std::vector<std::string> GetDismissAssetArray() const;
    std::vector<std::pair<std::string, int32_t>> GetIdOrderPositionPairs() const;
    int32_t GetUserId() const;
    void ClearAddAssetArray();
    void ClearRemoveAssetArray();
    void ClearRecoverAssetArray();
    void ClearDeleteAssetArray();
    void ClearDismissAssetArray();
    void ClearMoveMap();
    static ani_object GetAlbum(ani_env *env, ani_object object);
    static ani_object CreateAlbumRequest(ani_env *env, ani_object object, ani_object aniContext,
        ani_string aniName);
    static ani_status PlaceBefore(ani_env *env, ani_object object, ani_object albumHandle);
    static ani_status DismissAssets(ani_env *env, ani_object object, ani_object arrayPhotoAssetStr);
    static ani_status MergeAlbum(ani_env *env, ani_object object, ani_object albumHandle);
    static ani_status SetAlbumName(ani_env *env, ani_object object, ani_string name);
    static ani_status AddAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset);
    static ani_status RemoveAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset);
    static ani_status RecoverAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset);
    static ani_status MoveAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset, ani_object targetAlbum);
    static ani_status MoveAssetsWithUri(ani_env *env, ani_object object, ani_object arrayUris, ani_object targetAlbum);
    static ani_status SetDisplayLevel(ani_env *env, ani_object object, ani_int displayLevel);
    static ani_status SetCoverUri(ani_env *env, ani_object object, ani_string coverUri);
    static ani_status SetIsMe(ani_env *env, ani_object object);
    static ani_status Dismiss(ani_env *env, ani_object object);
    static ani_status ResetCoverUri(ani_env *env, ani_object object);
    static ani_status DeleteAlbums(ani_env *env, ani_class clazz, ani_object context, ani_object arrayAlbum);
    static ani_status DeleteAlbumsWithUri(ani_env *env, ani_class clazz, ani_object context, ani_object arrayAlbum);
    static ani_status DeleteAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset);
    static ani_status SetOrderPosition(ani_env *env, ani_object object, ani_object assets, ani_object position);
    static ani_status RecoverAssetsWithUri(ani_env *env, ani_object object, ani_object uriArray);
    static ani_status DeleteAssetsWithUri(ani_env *env, ani_object object, ani_object uriArray);
    ani_status ApplyChanges(ani_env *env) override;
    bool CheckChangeOperations(ani_env *env);

    std::map<std::shared_ptr<PhotoAlbum>, std::vector<std::string>, PhotoAlbumPtrCompare> GetMoveMap() const;
    void RecordMoveAssets(std::vector<std::string>& assetArray, std::shared_ptr<PhotoAlbum>& targetAlbum);

private:
    static bool CheckDismissAssetVaild(std::vector<std::string> &dismissAssets,
        std::vector<std::string> &newAssetArray);
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
    std::vector<std::pair<std::string, int32_t>> idOrderPositionPairs_;
    int32_t userId_;
};

struct MediaAlbumChangeRequestContext : public AniError {
    MediaAlbumChangeRequestAni* objectInfo;
    std::vector<AlbumChangeOperation> albumChangeOperations;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> deleteIds;
    std::vector<int32_t> photoAlbumTypes;
    std::vector<int32_t> photoAlbumSubtypes;
};

} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ALBUM_CHANGE_REQUEST_ANI_H