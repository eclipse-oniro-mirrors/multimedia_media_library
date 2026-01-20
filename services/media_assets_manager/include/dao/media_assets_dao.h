/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_MEDIA_ASSETS_DAO_H
#define OHOS_MEDIA_MEDIA_ASSETS_DAO_H

#include <vector>

#include "medialibrary_errno.h"
#include "photos_po.h"
#include "photo_album_po.h"
#include "medialibrary_rdbstore.h"
#include "asset_accurate_refresh.h"

namespace OHOS::Media::Common {
using namespace OHOS::Media::ORM;
class MediaAssetsDao {
public:
    /**
     * @brief Query the photo information by albumId or sourcePath.
     *
     * @param albumId The albumId of the photo album.
     * @param sourcePath The sourcePath of the photo.
     * @return albumInfo The photo album information.
     * @return int32_t The return code. E_OK means found album. Otherwise, return error code.
     */
    int32_t QueryAlbum(const int32_t albumId, const std::string &sourcePath, std::optional<PhotoAlbumPo> &albumInfo);
    int32_t QueryAssets(const std::vector<std::string> &fileIds, std::vector<PhotosPo> &queryResult);
    int32_t CreateNewAsset(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, int64_t &newAssetId,
        NativeRdb::ValuesBucket &values);
    int32_t ClearCloudInfo(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, const int32_t fileId);
    int32_t ResetPositionToCloudOnly(
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, int32_t fileId);
    int32_t MergeCloudInfoIntoTargetPhoto(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t DeletePhotoInfo(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, const int32_t fileId);
    int32_t FindSamePhoto(const PhotosPo &photoInfo, std::optional<PhotosPo> &samePhotoInfoOp);
    int32_t MoveOutTrash(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t LogicalDeleteCloudTrashedPhoto(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    bool IsSameAssetIgnoreAlbum(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo);
    int32_t UpdatePositionToBoth(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t UpdatePositionToBothAndFileSourceTypeToLake(
        const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t FindAssetsByBurstKey(const std::string &burstKey, std::vector<PhotosPo> &photoInfoList);

private:
    int32_t FindSamePhotoInTargetAlbum(
        const PhotosPo &photoInfo, const PhotoAlbumPo &albumInfo, std::optional<PhotosPo> &samePhotoInfoOp);
    /**
     * @brief Query the photo information by albumId.
     *
     * @param albumId The albumId of the photo album.
     * @return albumInfo The photo album information.
     * @return int32_t The return code. E_OK means found album. Otherwise, return error code.
     */
    int32_t QueryAlbumByAlbumId(const int32_t albumId, std::optional<PhotoAlbumPo> &albumInfo);
    /**
     * @brief Query the photo information by lPath.
     *
     * @param lPath The lPath of the photo album.
     * @return albumInfo The photo album information.
     * @return int32_t The return code. E_OK means found album. Otherwise, return error code.
     */
    int32_t QueryAlbumBylPath(const std::string &lPath, std::optional<PhotoAlbumPo> &albumInfo);
    std::string GetLpathFromSourcePath(const std::string &sourcePath);
    int32_t FindSamePhotoInHiddenAlbum(const PhotosPo &photoInfo, std::optional<PhotosPo> &samePhotoInfoOp);

private:
    const std::string SOURCE_PATH_PERFIX = "/storage/emulated/0";
    const std::string SQL_PHOTO_ALBUM_QUERY_BY_LPATH =
        "SELECT * FROM PhotoAlbum WHERE LOWER(lpath) = LOWER(?) LIMIT 1;";
    const std::string SQL_PHOTOS_QUERY_FOR_SAME = "\
        SELECT * \
        FROM Photos \
        WHERE date_trashed = 0 AND \
            dirty NOT IN (4, 7) AND \
            owner_album_id = ? AND \
            display_name = ? AND \
            size = ? AND \
            orientation = CASE WHEN media_type = 1 THEN ? ELSE orientation END \
        ORDER BY file_id ASC \
        LIMIT 1;";
    const std::string SQL_PHOTOS_QUERY_FOR_SAME_IN_HIDDEN_ALBUM = "\
        SELECT * \
        FROM Photos \
        WHERE date_trashed = 0 AND \
            dirty NOT IN (4, 7) AND \
            hidden = 1 AND \
            COALESCE(source_path, '') = ? AND \
            display_name = ? AND \
            size = ? AND \
            orientation = CASE WHEN media_type = 1 THEN ? ELSE orientation END \
        ORDER BY file_id ASC \
        LIMIT 1;";
};
}  // namespace OHOS::Media::Common
#endif  // OHOS_MEDIA_MEDIA_ASSETS_DAO_H