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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_common_dao.h"

#include "abs_rdb_predicates.h"
#include "photo_map_column.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "result_set_reader.h"

namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
int32_t CloudMediaCommonDao::QueryLocalByCloudId(
    const std::vector<std::string> &cloudIds, const std::vector<std::string> &columns, std::vector<PhotosPo> &result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryLocalByCloudId Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryLocalByCloudId Failed to query.");
    return ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(result);
}

int32_t CloudMediaCommonDao::QueryLocalMap(const int32_t &fileId, std::map<int32_t, int32_t> &localMapIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoMap::TABLE);
    predicates.EqualTo(PhotoMap::ASSET_ID, std::to_string(fileId));
    auto resultSet = rdbStore->Query(predicates, {PhotoMap::ALBUM_ID, PhotoMap::DIRTY});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "QueryLocalMap Failed to query.");
    int32_t albumId;
    int32_t dirty;
    while (resultSet->GoToNextRow() == E_OK) {
        albumId = GetInt32Val(PhotoMap::ALBUM_ID, resultSet);
        dirty = GetInt32Val(PhotoMap::DIRTY, resultSet);
        localMapIds[albumId] = dirty;
    }
    return E_OK;
}

int32_t CloudMediaCommonDao::QueryPhotoByCloudId(
    const std::string &cloudId, std::optional<PhotosPo> &photoInfoOp) const
{
    CHECK_AND_RETURN_RET_LOG(!cloudId.empty(), E_ERR, "cloudId is empty");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    auto resultSet = rdbStore->Query(predicates, {});  // 查询所有字段
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    std::vector<PhotosPo> photosPoList;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPoList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "Failed to read records. cloudId: %{public}s, ret: %{public}d", cloudId.c_str(), ret);
    if (!photosPoList.empty()) {
        photoInfoOp = photosPoList[0];
    }
    return E_OK;
}

int32_t CloudMediaCommonDao::QueryPhotoByFilePath(const std::string &filePath,
                                                  std::optional<PhotosPo> &photoInfoOp) const
{
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_ERR, "filePath is empty");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, filePath);

    auto resultSet = rdbStore->Query(predicates, {});  // 查询所有字段
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    std::vector<PhotosPo> photosPoList;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPoList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "Failed to read records. filePath: %{public}s, ret: %{public}d", filePath.c_str(), ret);
    if (!photosPoList.empty()) {
        photoInfoOp = photosPoList[0];
    }
    return E_OK;
}

int32_t CloudMediaCommonDao::QueryPhotoAlbumByAlbumId(const int32_t albumId,
                                                      std::optional<PhotoAlbumPo> &photoAlbumInfoOp) const
{
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_ERR, "albumId is invalid");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);

    auto resultSet = rdbStore->Query(predicates, {});  // 查询所有字段
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    std::vector<PhotoAlbumPo> photoAlbumPoList;
    int32_t ret = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(resultSet).ReadRecords(photoAlbumPoList);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "Failed to read records. albumId: %{public}d, ret: %{public}d", albumId, ret);
    if (!photoAlbumPoList.empty()) {
        photoAlbumInfoOp = photoAlbumPoList[0];
    }
    return E_OK;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync