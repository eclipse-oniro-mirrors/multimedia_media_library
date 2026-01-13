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

#define MLOG_TAG "PortraitAlbumUtils"

#include "portrait_album_utils.h"
#include "backup_database_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

int32_t PortraitAlbumUtils::DeleteExistingAlbumData(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    AlbumDeleteType deleteType)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");

    switch (deleteType) {
        case AlbumDeleteType::PORTRAIT:
            return DeletePortraitAlbumData(rdbStore);
        case AlbumDeleteType::GROUP_PHOTO:
            return DeleteGroupPhotoAlbumData(rdbStore);
        case AlbumDeleteType::ALL:
            return DeletePortraitAlbumData(rdbStore) || DeleteGroupPhotoAlbumData(rdbStore);
        default:
            MEDIA_ERR_LOG("Invalid delete type: %{public}d", static_cast<int32_t>(deleteType));
            return E_ERR;
    }
}

int32_t PortraitAlbumUtils::DeletePortraitAlbumData(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("DeletePortraitAlbumData");

    // Delete maps
    int32_t ret = DeleteAnalysisPhotoMapData(rdbStore, SMART, PORTRAIT);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete portrait album maps");
    // Delete portrait albums
    ret = DeleteAnalysisAlbums(rdbStore, SMART, PORTRAIT);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete portrait albums");

    // Delete face tag data
    ret = DeleteFaceTagData(rdbStore);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete face tag data");

    // Update analysis total face status
    ret = UpdateAnalysisTotalFaceStatus(rdbStore);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to update analysis total face status");

    // Update analysis search index cv status
    ret = UpdateAnalysisSearchIndexCvStatus(rdbStore);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to update analysis search index cv status");

    // Delete image face data
    ret = DeleteImageFaceData(rdbStore);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete image face data");
    return E_OK;
}

int32_t PortraitAlbumUtils::DeleteGroupPhotoAlbumData(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("DeleteGroupPhotoAlbumData");

    // Get group photo album IDs
    std::vector<std::string> albumIds;
    int32_t ret = GetAlbumIdsByType(rdbStore, SMART, GROUP_PHOTO, albumIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to get group photo album IDs");

    if (albumIds.empty()) {
        MEDIA_INFO_LOG("No group photo albums to delete");
        return E_OK;
    }

    // Delete group photo album maps
    ret = DeleteAnalysisPhotoMapData(rdbStore, SMART, GROUP_PHOTO);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete group photo album maps");

    // Delete group photo albums
    ret = DeleteAnalysisAlbums(rdbStore, SMART, GROUP_PHOTO);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete group photo albums");

    return E_OK;
}

int32_t PortraitAlbumUtils::DeleteAnalysisAlbums(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    int32_t albumType, int32_t albumSubtype)
{
    MEDIA_INFO_LOG("DeleteAnalysisAlbums: type=%{public}d, subtype=%{public}d", albumType, albumSubtype);

    std::string deleteSql = "DELETE FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ANALYSIS_COL_ALBUM_TYPE + " = " + std::to_string(albumType) +
        " AND " + ANALYSIS_COL_ALBUM_SUBTYPE + " = " + std::to_string(albumSubtype);

    return ExecuteSQLSafe(rdbStore, deleteSql);
}

int32_t PortraitAlbumUtils::DeleteFaceTagData(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("DeleteFaceTagData");

    std::string deleteSql = "DELETE FROM " + VISION_FACE_TAG_TABLE;
    return ExecuteSQLSafe(rdbStore, deleteSql);
}

int32_t PortraitAlbumUtils::DeleteImageFaceData(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("DeleteImageFaceData");

    // Delete from VISION_IMAGE_FACE_TABLE
    std::string deleteImageFaceSql = "DELETE FROM " + VISION_IMAGE_FACE_TABLE;
    int32_t ret = ExecuteSQLSafe(rdbStore, deleteImageFaceSql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete from VISION_IMAGE_FACE_TABLE");

    // Delete from VISION_VIDEO_FACE_TABLE
    std::string deleteVideoFaceSql = "DELETE FROM " + VISION_VIDEO_FACE_TABLE;
    ret = ExecuteSQLSafe(rdbStore, deleteVideoFaceSql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to delete from VISION_VIDEO_FACE_TABLE");

    return E_OK;
}

int32_t PortraitAlbumUtils::DeleteAnalysisPhotoMapData(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    int32_t albumType, int32_t albumSubtype)
{
    MEDIA_INFO_LOG("DeleteAnalysisPhotoMapData: type=%{public}d, subtype=%{public}d", albumType, albumSubtype);

    std::string deleteSql = "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE + " WHERE " +
        "map_album IN (SELECT album_id FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ANALYSIS_COL_ALBUM_TYPE + " = " + std::to_string(albumType) + " AND " +
        ANALYSIS_COL_ALBUM_SUBTYPE + " = " + std::to_string(albumSubtype) + ")";

    return ExecuteSQLSafe(rdbStore, deleteSql);
}

int32_t PortraitAlbumUtils::UpdateAnalysisTotalFaceStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    MEDIA_INFO_LOG("UpdateAnalysisTotalFaceStatus");

    std::string fileIdFilterClause = std::string("(") + "SELECT " + IMAGE_FACE_COL_FILE_ID + " FROM " +
        VISION_IMAGE_FACE_TABLE + " UNION" + " SELECT " + VIDEO_FACE_COL_FILE_ID + " FROM " +
        VISION_VIDEO_FACE_TABLE + ")";
    std::string fileIdCondition = IMAGE_FACE_COL_FILE_ID + " IN " + fileIdFilterClause + " AND status >= 0";

    std::unique_ptr<NativeRdb::AbsRdbPredicates> totalTablePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(VISION_TOTAL_TABLE);
    totalTablePredicates->SetWhereClause(fileIdCondition);

    NativeRdb::ValuesBucket totalValues;
    totalValues.PutInt("face", 0);
    totalValues.PutInt("status", 0);

    int32_t totalUpdatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(rdbStore, totalUpdatedRows, totalValues, totalTablePredicates);

    MEDIA_INFO_LOG("Update VISION_TOTAL_TABLE, updatedRows %{public}d", totalUpdatedRows);
    bool totalUpdateFailed = (totalUpdatedRows < 0 || ret < 0);
    CHECK_AND_RETURN_RET_LOG(!totalUpdateFailed, E_FAIL, "Failed to update VISION_TOTAL_TABLE face status");

    return E_OK;
}

int32_t PortraitAlbumUtils::UpdateAnalysisSearchIndexCvStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    int32_t ret;
    MEDIA_INFO_LOG("UpdateAnalysisSearchIndexCvStatus");

    std::string fileIdFilterClause = std::string("(") + "SELECT " + IMAGE_FACE_COL_FILE_ID + " FROM " +
        VISION_IMAGE_FACE_TABLE + " UNION" + " SELECT " + VIDEO_FACE_COL_FILE_ID + " FROM " +
        VISION_VIDEO_FACE_TABLE + ")";
    std::string fileIdCondition = IMAGE_FACE_COL_FILE_ID + " IN " + fileIdFilterClause + " AND status >= 0";

    std::unique_ptr<NativeRdb::AbsRdbPredicates> searchIndexTablePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(ANALYSIS_SEARCH_INDEX_TABLE);
    std::string statusCondition = " cv_status = 1";
    searchIndexTablePredicates->SetWhereClause(fileIdCondition + " AND" + statusCondition);

    NativeRdb::ValuesBucket searchIndexValues;
    searchIndexValues.PutInt("cv_status", 0);

    int32_t totalUpdatedRows = 0;
    ret = BackupDatabaseUtils::Update(rdbStore, totalUpdatedRows, searchIndexValues, searchIndexTablePredicates);

    MEDIA_INFO_LOG("Update ANALYSIS_SEARCH_INDEX_TABLE, updatedRows %{public}d", totalUpdatedRows);
    bool totalUpdateFailed = (totalUpdatedRows < 0 || ret < 0);
    CHECK_AND_RETURN_RET_LOG(!totalUpdateFailed, E_FAIL, "Failed to update ANALYSIS_SEARCH_INDEX_TABLE cv status");

    return E_OK;
}

int32_t PortraitAlbumUtils::GetAlbumIdsByType(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
    int32_t albumType, int32_t albumSubtype, std::vector<std::string>& albumIds)
{
    albumIds.clear();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");
    std::string querySql = "SELECT album_id FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ANALYSIS_COL_ALBUM_TYPE + " = " + std::to_string(albumType) + " AND " +
        ANALYSIS_COL_ALBUM_SUBTYPE + " = " + std::to_string(albumSubtype);

    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Query album IDs failed");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = 0;
        int32_t columnIndex = 0;
        if (resultSet->GetColumnIndex("album_id", columnIndex) == NativeRdb::E_OK) {
            resultSet->GetInt(columnIndex, albumId);
            albumIds.push_back(std::to_string(albumId));
        } else {
            MEDIA_ERR_LOG("Get column index for album_id failed");
            resultSet->Close();
            return E_ERR;
        }
    }

    resultSet->Close();
    return E_OK;
}

int32_t PortraitAlbumUtils::ExecuteSQLSafe(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string& sql)
{
    int32_t ret = BackupDatabaseUtils::ExecuteSQL(rdbStore, sql);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Execute SQL failed: %{public}s", sql.c_str());
    return E_OK;
}

} // namespace Media
} // namespace OHOS