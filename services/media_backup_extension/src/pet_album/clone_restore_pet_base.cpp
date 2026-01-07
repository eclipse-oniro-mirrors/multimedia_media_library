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
#define MLOG_TAG "CloneRestorePetBase"

#include "clone_restore_pet_base.h"

#include <vector>
#include <algorithm>
#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "media_library_db_upgrade.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "result_set_utils.h"
#include "upgrade_restore_task_report.h"
#include "userfile_manager_types.h"
#include "vision_column.h"


namespace OHOS::Media {
void CloneRestorePetBase::GetMaxAlbumId()
{
    maxAnalysisAlbumId_ = BackupDatabaseUtils::QueryMaxId(mediaLibraryRdb_, ANALYSIS_ALBUM_TABLE, "album_id");
    MEDIA_INFO_LOG("GetMaxAlbumId, maxAnalysisAlbumId_ = %{public}d", maxAnalysisAlbumId_);
}

void CloneRestorePetBase::GetAnalysisAlbumInsertValue(NativeRdb::ValuesBucket &value, const AnalysisAlbumTbl &info)
{
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_ALBUM_TYPE, info.albumType);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_ALBUM_SUBTYPE, info.albumSubtype);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_ALBUM_NAME, info.albumName);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_RANK, info.rank);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_TAG_ID, info.tagId);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_USER_OPERATION, info.userOperation);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_GROUP_TAG, info.groupTag);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_USER_DISPLAY_LEVEL, info.userDisplayLevel);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_IS_ME, info.isMe);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_IS_REMOVED, info.isRemoved);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_RENAME_OPERATION, info.renameOperation);
    BackupDatabaseUtils::PutIfPresent(value, ANALYSIS_COL_IS_LOCAL, info.isLocal);
    BackupDatabaseUtils::PutIfPresent<std::string>(value, "relationship", info.relationship);
}

void CloneRestorePetBase::ParseAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    AnalysisAlbumTbl &analysisAlbumTbl)
{
    analysisAlbumTbl.albumIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_ALBUM_ID);
    analysisAlbumTbl.albumType = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_ALBUM_TYPE);
    analysisAlbumTbl.albumSubtype = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        ANALYSIS_COL_ALBUM_SUBTYPE);
    analysisAlbumTbl.albumName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_ALBUM_NAME);
    analysisAlbumTbl.coverUri = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_COVER_URI);
    analysisAlbumTbl.rank = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_RANK);
    analysisAlbumTbl.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_TAG_ID);
    analysisAlbumTbl.userOperation = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        ANALYSIS_COL_USER_OPERATION);
    analysisAlbumTbl.groupTag = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ANALYSIS_COL_GROUP_TAG);
    analysisAlbumTbl.userDisplayLevel = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        ANALYSIS_COL_USER_DISPLAY_LEVEL);
    analysisAlbumTbl.isMe = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_IS_ME);
    analysisAlbumTbl.isRemoved = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_IS_REMOVED);
    analysisAlbumTbl.renameOperation = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        ANALYSIS_COL_RENAME_OPERATION);
    analysisAlbumTbl.isLocal = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ANALYSIS_COL_IS_LOCAL);
    analysisAlbumTbl.isCoverSatisfied = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet,
        ANALYSIS_COL_IS_COVER_SATISFIED);
}

int32_t CloneRestorePetBase::BatchInsertWithRetry(const std::string &tableName,
    const std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), 0);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}" PRId64, errCode, rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void CloneRestorePetBase::AppendExtraWhereClause(std::string& whereClause)
{
    std::string photoQueryWhereClause;
    if (isCloudRestoreSatisfied_) {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 2, 3) AND ";
    } else {
        photoQueryWhereClause = PhotoColumn::PHOTO_POSITION + " IN (1, 3) AND ";
    }
    photoQueryWhereClause += PhotoColumn::PHOTO_SYNC_STATUS + " = " +
        std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) +
        " AND " + PhotoColumn::PHOTO_CLEAN_FLAG + " = " + to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " AND " + MediaColumn::MEDIA_TIME_PENDING + " = 0" +
        " AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0";
    std::string albumQueryWhereClause = "EXISTS (SELECT " + PhotoMap::ASSET_ID + " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
        " WHERE " + PhotoMap::ALBUM_ID + " = " + PhotoAlbumColumns::ALBUM_ID + " AND EXISTS (SELECT " +
        MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_ID + " = " +
        PhotoMap::ASSET_ID;
    albumQueryWhereClause += " AND " + photoQueryWhereClause + "))";

    whereClause += whereClause.empty() ? "" : " AND ";
    whereClause += albumQueryWhereClause;
}

void CloneRestorePetBase::GenNewCoverUris(const std::vector<CoverUriInfo>& coverUriInfo)
{
    MEDIA_INFO_LOG("GenNewCoverUris");
    bool cond = (coverUriInfo.empty() && photoInfoMap_.empty());
    CHECK_AND_RETURN_LOG(!cond, "Empty coverUriInfo or fileIdPairs, skipping.");

    std::unordered_map<std::string, std::pair<std::string, int32_t>> tagIdToCoverInfo;
    for (const auto& [tagId, coverInfo] : coverUriInfo) {
        tagIdToCoverInfo[tagId] = coverInfo;
    }
    auto fileIdPairs = CollectFileIdPairs(photoInfoMap_);
    std::unordered_map<std::string, int32_t> oldToNewFileId;
    for (const auto& [oldId, newId] : fileIdPairs) {
        oldToNewFileId[std::to_string(oldId)] = newId;
    }

    std::vector<std::string> tagIds;
    std::string updateSql = GenCoverUriUpdateSql(tagIdToCoverInfo, oldToNewFileId, photoInfoMap_, tagIds);
    if (updateSql.empty()) {
        MEDIA_INFO_LOG("updateSql is empty");
        return;
    }

    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql);
}

std::string CloneRestorePetBase::GenCoverUriUpdateSql(const std::unordered_map<std::string,
    std::pair<std::string, int32_t>>& tagIdToCoverInfo, const std::unordered_map<std::string, int32_t>& oldToNewFileId,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, std::vector<std::string>& tagIds)
{
    MEDIA_INFO_LOG("GenCoverUriUpdateSql");
    std::unordered_map<std::string, std::string> coverUriUpdates;
    std::unordered_map<std::string, int32_t> isCoverSatisfiedUpdates;

    for (const auto& [tagId, coverInfo] : tagIdToCoverInfo) {
        const auto& [oldCoverUri, isCoverSatisfied] = coverInfo;
        std::string newUri = ProcessUriAndGenNew(tagId, oldCoverUri, oldToNewFileId, photoInfoMap);
        if (!newUri.empty()) {
            coverUriUpdates[tagId] = newUri;
            isCoverSatisfiedUpdates[tagId] = isCoverSatisfied;
            tagIds.push_back(tagId);
        }
    }

    bool cond = (coverUriUpdates.empty() || isCoverSatisfiedUpdates.empty());
    CHECK_AND_RETURN_RET(!cond, "");

    std::string updateSql = "UPDATE AnalysisAlbum SET ";

    updateSql += "cover_uri = CASE ";
    for (const auto& [tagId, newUri] : coverUriUpdates) {
        updateSql += "WHEN tag_id = '" + tagId + "' THEN '" + newUri + "' ";
    }
    updateSql += "ELSE cover_uri END";

    bool hasValidIsCoverSatisfied = false;
    std::string isCoverSatisfiedSql = ", is_cover_satisfied = CASE ";
    for (const auto& [tagId, isCoverSatisfied] : isCoverSatisfiedUpdates) {
        if (isCoverSatisfied != PET_INVALID_COVER_SATISFIED_STATUS) {
            hasValidIsCoverSatisfied = true;
            isCoverSatisfiedSql += "WHEN tag_id = '" + tagId + "' THEN " + std::to_string(isCoverSatisfied) + " ";
        }
    }

    isCoverSatisfiedSql += "ELSE is_cover_satisfied END ";
    CHECK_AND_EXECUTE(!hasValidIsCoverSatisfied, updateSql += isCoverSatisfiedSql);

    updateSql += "WHERE tag_id IN ('" +
        BackupDatabaseUtils::JoinValues(tagIds, "','") + "')";

    return updateSql;
}

std::string CloneRestorePetBase::ProcessUriAndGenNew(const std::string& tagId, const std::string& oldCoverUri,
    const std::unordered_map<std::string, int32_t>& oldToNewFileId,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    auto uriParts = BackupDatabaseUtils::SplitString(oldCoverUri, '/');
    if (uriParts.size() >= COVER_URI_NUM) {
        std::string fileIdOld = uriParts[uriParts.size() - 3];
        auto it = oldToNewFileId.find(fileIdOld);
        if (it != oldToNewFileId.end()) {
            int32_t fileIdNew = it->second;
            PhotoInfo photoInfo {};
            if (GetFileInfoByFileId(fileIdNew, photoInfoMap, photoInfo)) {
                std::string extraUri = MediaFileUtils::GetExtraUri(photoInfo.displayName, photoInfo.cloudPath);
                return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
                    std::to_string(fileIdNew), extraUri);
            }
        }
    }
    return "";
}

bool CloneRestorePetBase::GetFileInfoByFileId(int32_t fileId,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, PhotoInfo& outPhotoInfo)
{
    auto it = std::find_if(photoInfoMap.begin(), photoInfoMap.end(),
        [fileId](const auto& entry) {
            return entry.second.fileIdNew == fileId;
        });
    if (it != photoInfoMap.end()) {
        outPhotoInfo = it->second;
        return true;
    }
    
    return false;
}

std::vector<FileIdPair> CloneRestorePetBase::CollectFileIdPairs(
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    std::set<FileIdPair> uniquePairs;

    for (const auto& [fileIdOld, photoInfo] : photoInfoMap) {
        uniquePairs.emplace(fileIdOld, photoInfo.fileIdNew);
    }

    return std::vector<FileIdPair>(uniquePairs.begin(), uniquePairs.end());
}

bool CloneRestorePetBase::IsMapColumnOrderExist()
{
    std::vector<std::string> intersection = BackupDatabaseUtils::GetCommonColumnInfos(mediaLibraryRdb_,
        mediaRdb_, "AnalysisPhotoMap");
    auto it = std::find(intersection.begin(), intersection.end(), "order_position");
    return it != intersection.end();
}
}