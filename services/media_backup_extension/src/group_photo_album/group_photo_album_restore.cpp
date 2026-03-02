/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "CloneGroupPhotoAlbum"

#include "group_photo_album_restore.h"

#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "backup_log_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "photo_album_restore.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "upgrade_restore_task_report.h"
#include "vision_album_column.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"

namespace OHOS {
namespace Media {
const int32_t BATCH_SIZE = 100;
const int32_t PAGE_SIZE = 200;
const int64_t THRESHOLD_DATA_SIZE = 30000;
const int64_t THRESHOLD_DATA_TIME = 600000;
const int64_t DEFAULT_FAULT_TIME = 0;

const int64_t BASIC_NUMBER = 10000;
const int64_t SUPPORT_NUMBER = 9999;
const int64_t SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;

const int32_t NORMAL_EXIT_CODE = 0;
const int32_t MIDDLE_EXIT_CODE = 1;
const int32_t BEGIN_EXIT_CODE = 2;

CloneGroupPhotoAlbum::CloneGroupPhotoAlbum(int32_t sceneCode, const std::string& taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    galleryRdb_ = galleryRdb;
}

static std::string JoinGroupTag(const std::unordered_map<std::string, std::vector<std::string>>& groupTagMap)
{
    string groupTagId = "";
    CHECK_AND_RETURN_RET(groupTagMap.size() != 0, groupTagId);
    std::vector<std::string> tagVector;
    for (const auto& tagIt : groupTagMap) {
        CHECK_AND_CONTINUE(tagIt.second.size() != 0);
        if (tagIt.second.size() > 1) {
            tagVector.emplace_back(BackupDatabaseUtils::JoinValues(tagIt.second, "|"));
        } else {
            tagVector.emplace_back(tagIt.second[0]);
        }
    }
    groupTagId = BackupDatabaseUtils::JoinValues(tagVector, ",");
    return groupTagId;
}

bool CloneGroupPhotoAlbum::GetFileIdsByGroupTag(CloneGroupPhotoAlbum::GroupAlbumInfo &info)
{
    const std::unordered_map<std::string, std::vector<std::string>>& tagMap = info.groupTagMap;
    CHECK_AND_RETURN_RET_LOG(!tagMap.empty(), false, "tagMap is empty.");
    std::string querySql = "SELECT DISTINCT file_id FROM tab_analysis_image_face WHERE 1=1 ";
    std::string groupSql = " ORDER BY file_id DESC";
    std::vector<std::string> tagIdVector;
    for (const auto& tagIt : tagMap) {
        std::string firstSql = "AND file_id IN (SELECT file_id FROM tab_analysis_image_face "
            "face JOIN tab_analysis_face_tag tag ON face.tag_id = tag.tag_id WHERE tag.tag_id IN (";
        for (int32_t i = 0; i < static_cast<int32_t>(tagIt.second.size()); i++) {
            firstSql.append("?");
            if (i != static_cast<int32_t>(tagIt.second.size() - 1)) {
                firstSql.append(",");
            }
        }
        firstSql += ") AND (face.total_faces = 0 OR face.total_faces = " + std::to_string(tagMap.size()) + "))";
        tagIdVector.insert(tagIdVector.end(), tagIt.second.begin(), tagIt.second.end());
        querySql += firstSql;
    }
    querySql += groupSql;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql, tagIdVector);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        info.fileIdVec.emplace_back(std::to_string(GetInt32Val(MediaColumn::MEDIA_ID, resultSet)));
    }
    info.fileIdCount = info.fileIdVec.size();
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(info.fileIdCount > 0, false, "mediaLibraryRdb do not find any fileId.");
    return true;
}

void CloneGroupPhotoAlbum::QueryTagIdFromMergeTag(CloneGroupPhotoAlbum::GroupAlbumInfo &info)
{
    std::string querySql = "SELECT " + GALLERY_MERGE_TAG_TAG_ID + ", " + GALLERY_GROUP_TAG + " FROM merge_tag WHERE " +
        GALLERY_GROUP_TAG + " IN (";
    const std::vector<std::string>& tagVector = info.groupTagVec;
    CHECK_AND_RETURN_LOG(!tagVector.empty(), "tagVector is empty.");
    std::string tagStr = BackupDatabaseUtils::JoinSQLValues(tagVector, ",");
    querySql += tagStr + ")";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(galleryRdb_, querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string tagId = GetStringVal(GALLERY_MERGE_TAG_TAG_ID, resultSet);
        std::string groupTag = GetStringVal(GALLERY_GROUP_TAG, resultSet);
        CHECK_AND_RETURN_LOG(!tagId.empty() && !groupTag.empty(), "tagId or groupTag is empty.");
        info.groupTagMap[groupTag].emplace_back(std::move(tagId));
    }
    resultSet->Close();
    return;
}

std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> CloneGroupPhotoAlbum::GetGroupPhotoAlbumInfo(int32_t offset)
{
    MEDIA_INFO_LOG("Start GetGroupPhotoAlbumInfo.");
    std::string querySql = "SELECT " + GALLERY_MERGE_TAG_TAG_ID + ", " + GALLERY_GROUP_TAG + ", " +
        GALLERY_TAG_NAME + ", " + GALLERY_USER_OPERATION + ", " + GALLERY_RENAME_OPERATION +
        " FROM merge_tag WHERE group_tag LIKE '%|%' ORDER BY _id LIMIT " + std::to_string(PAGE_SIZE) +
        " OFFSET " + std::to_string(offset);
    std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> result;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(galleryRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        CloneGroupPhotoAlbum::GroupAlbumInfo info;
        std::vector<std::string> tagVector;
        string groupTag = GetStringVal(GALLERY_GROUP_TAG, resultSet);
        CHECK_AND_CONTINUE(!groupTag.empty());
        tagVector = BackupDatabaseUtils::SplitString(groupTag, '|');
        info.groupTagVec = tagVector;
        info.tagName = GetStringVal(GALLERY_TAG_NAME, resultSet);
        info.userOperation = GetInt32Val(GALLERY_USER_OPERATION, resultSet);
        info.renameOperation = (!info.tagName.empty() ? RENAME_OPERATION_RENAMED : 0);
        result.emplace_back(info);
    }
    resultSet->Close();
    for (auto it = result.begin(); it != result.end();) {
        auto& info = *it;
        QueryTagIdFromMergeTag(info);
        if (!GetFileIdsByGroupTag(info)) {
            it = result.erase(it);
        } else {
            info.groupTag = JoinGroupTag(info.groupTagMap);
            MEDIA_INFO_LOG("info.groupTag = %{public}s", info.groupTag.c_str());
            info.tagId = info.groupTag;
            ++it;
        }
    }
    MEDIA_INFO_LOG("GetGroupPhotoAlbumInfo end.");
    return result;
}

void CloneGroupPhotoAlbum::QueryGroupPhotoAlbum(const std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo>&
    groupPhotoAlbumInfos, std::map<int32_t, std::vector<string>> &groupPhotoMap)
{
    std::string groupSql = "SELECT " + ALBUM_ID + "," + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ALBUM_SUBTYPE + " = " + std::to_string(PhotoAlbumSubType::GROUP_PHOTO);
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, groupSql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Query resultSql is null.");
    std::unordered_map<std::string, int32_t> albumIdMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = GetInt32Val(ALBUM_ID, resultSet);
        std::string groupTag = GetStringVal(GROUP_TAG, resultSet);
        albumIdMap[groupTag] = albumId;
    }
    resultSet->Close();

    for (const auto& info : groupPhotoAlbumInfos) {
        CHECK_AND_CONTINUE(albumIdMap.find(info.groupTag) != albumIdMap.end());
        groupPhotoMap[albumIdMap[info.groupTag]] = info.fileIdVec;
    }
}

void CloneGroupPhotoAlbum::ModifyGroupVersion(const std::map<int32_t, std::vector<string>> &groupPhotoMap)
{
    std::string baseUpdateSql = "UPDATE tab_analysis_image_face SET group_version = '1' WHERE file_id IN (";
    int32_t currentBatchSize = 0;
    string updateSql = baseUpdateSql;
    bool isFirst = true;
    for (const auto& album : groupPhotoMap) {
        string fileIds = BackupDatabaseUtils::JoinValues(album.second, ",");
        currentBatchSize += album.second.size();
        if (!isFirst) {
            updateSql += ",";
        }
        updateSql += fileIds;
        isFirst = false;
        if (currentBatchSize >= BATCH_SIZE) {
            int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql + ")");
            CHECK_AND_RETURN_LOG(ret >= 0, "execute update image_face failed, ret=%{public}d", ret);
            updateSql = baseUpdateSql;
            currentBatchSize = 0;
            isFirst = true;
        }
    }
    if (currentBatchSize > 0) {
        int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql + ")");
        CHECK_AND_RETURN_LOG(ret >= 0, "execute update image_face failed, ret=%{public}d", ret);
    }
}

void CloneGroupPhotoAlbum::InsertAnalysisPhotoMap(const std::map<int32_t, std::vector<string>> &groupPhotoMap)
{
    std::string baseSql = "INSERT OR REPLACE INTO AnalysisPhotoMap (map_album, map_asset) VALUES ";

    int32_t currentBatchSize = 0;
    string sql = baseSql;
    bool isFirst = true;
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t count = 0;
    for (const auto& albumIdIt : groupPhotoMap) {
        string albumIdStr = "(" + std::to_string(albumIdIt.first) + ", ";
        std::vector<string> valuesVec = albumIdIt.second;
        valuesVec = BackupDatabaseUtils::LeftJoinValues(valuesVec, albumIdStr);
        std::string valueStr = BackupDatabaseUtils::JoinValues(valuesVec, "),");
        if (!isFirst) {
            sql += ",";
        }
        sql += valueStr + ")";
        isFirst = false;
        currentBatchSize += albumIdIt.second.size();
        if (currentBatchSize >= BATCH_SIZE) {
            int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, sql);
            CHECK_AND_RETURN_LOG(ret >= 0, "execute insert AnalysisPhotoMap failed, ret=%{public}d", ret);
            sql = baseSql;
            isFirst = true;
            count += currentBatchSize;
            currentBatchSize = 0;
        }
    }
    if (currentBatchSize > 0) {
        int32_t ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, sql);
        CHECK_AND_RETURN_LOG(ret >= 0, "execute insert AnalysisPhotoMap failed, ret=%{public}d", ret);
        count += currentBatchSize;
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t time = end - start;
    std::stringstream albumReport;
    albumReport << "InsertAnalysisPhotoMap rowCount: " << count << ", InsertAnalysisPhotoMap time: " << time;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_GROUP_PHOTO_ALBUM_STATS", "", albumReport.str());
    MEDIA_INFO_LOG("InsertAnalysisPhotoMap end, count = %{public}d, time = %{public}" PRId64, count, time);
}

int32_t CloneGroupPhotoAlbum::BatchInsertWithRetry(const std::string &tableName,
    const std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    if (values.empty()) {
        return 0;
    }

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

int64_t CloneGroupPhotoAlbum::GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_RET_LOG(!taskId_.empty() && MediaLibraryDataManagerUtils::IsNumber(taskId_),
        DEFAULT_FAULT_TIME, "taskId: %{public}s invalid", taskId_.c_str());
    int64_t backupStartTime = std::stoll(taskId_) * 1000;
    int64_t dataSize = static_cast<int64_t>(photoInfoMap.size());
    MEDIA_INFO_LOG("dataSize: %{public}" PRId64 ", backupStartTime: %{public}" PRId64,
        dataSize, backupStartTime);
    CHECK_AND_RETURN_RET(dataSize > THRESHOLD_DATA_SIZE, backupStartTime + THRESHOLD_DATA_TIME);
    return backupStartTime + (dataSize + SUPPORT_NUMBER) / BASIC_NUMBER * SINGLE_OVER_THRESHOLD_DATA_TIME;
}

void CloneGroupPhotoAlbum::InsertAnalysisAlbumTable(const std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> &result,
    const std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t analysisAlbumStart = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(ANALYSIS_ALBUM_TABLE, values, rowNum);
    int64_t analysisAlbumEnd = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t analysisAlbumTime = analysisAlbumEnd - analysisAlbumStart;
    MEDIA_INFO_LOG("End InsertAnalysisAlbum. AnalysisAlbumRowNum = %{public}" PRId64
        ", AnalysisAlbumTime = %{public}" PRId64, rowNum, analysisAlbumTime);
    CHECK_AND_RETURN(errCode == E_OK);

    std::map<int32_t, std::vector<std::string>> groupPhotoMap;
    QueryGroupPhotoAlbum(result, groupPhotoMap);
    CHECK_AND_RETURN(groupPhotoMap.size() != 0);
    InsertAnalysisPhotoMap(groupPhotoMap);
    ModifyGroupVersion(groupPhotoMap);
}

void CloneGroupPhotoAlbum::RestoreGroupPhotoAlbum(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    MEDIA_INFO_LOG("Start to update group photo album.");
    std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> result;
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t shouldEndTime = GetShouldEndTime(photoInfoMap);
    int32_t offset = 0;
    int32_t count = 0;
    bool firstBatch = true;
    do {
        int64_t partOne = MediaFileUtils::UTCTimeMilliSeconds();
        CHECK_AND_EXECUTE(partOne <= shouldEndTime || !firstBatch, exitCode_ = BEGIN_EXIT_CODE);
        CHECK_AND_EXECUTE(partOne <= shouldEndTime || firstBatch, exitCode_ = MIDDLE_EXIT_CODE);
        CHECK_AND_BREAK_INFO_LOG(partOne <= shouldEndTime,
            "current time: %{public}" PRId64 ", over shouldEndTime: %{public}" PRId64
            ", RestoreGroupPhotoAlbum cost: %{public}" PRId64,
            partOne, shouldEndTime, (partOne - start));
        result = GetGroupPhotoAlbumInfo(offset);
        CHECK_AND_BREAK_ERR_LOG(result.size() != 0, "Query resultSql is null.");
        std::vector<NativeRdb::ValuesBucket> values;
        for (const auto& info : result) {
            NativeRdb::ValuesBucket valuesBucket;
            valuesBucket.PutInt(ALBUM_TYPE, PhotoAlbumType::SMART);
            valuesBucket.PutString(ALBUM_NAME, info.tagName);
            valuesBucket.PutInt(ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
            valuesBucket.PutString(GROUP_TAG, info.groupTag);
            valuesBucket.PutString(TAG_ID, info.tagId);
            valuesBucket.PutInt(USER_OPERATION, info.userOperation);
            valuesBucket.PutInt(IS_LOCAL, IS_LOCAL_TRUE);
            valuesBucket.PutInt(COUNT, info.fileIdCount);
            valuesBucket.PutInt(USER_DISPLAY_LEVEL, info.userDisplayLevel);
            valuesBucket.PutInt(RENAME_OPERATION, info.renameOperation);
            values.emplace_back(valuesBucket);
        }
        InsertAnalysisAlbumTable(result, values);
        offset += PAGE_SIZE;
        count += result.size();
        firstBatch = false;
    } while (result.size() == PAGE_SIZE);
    exitCode_ = NORMAL_EXIT_CODE;
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t sumTime = end - start;
    std::stringstream updateGroupReport;
    updateGroupReport <<"GroupPhotoRestoreSumTime: " << sumTime <<", GroupPhotoRestoreExitCode: " << exitCode_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_GROUP_PHOTO_ALBUM_STATS", "", updateGroupReport.str());
    MEDIA_INFO_LOG("End RestoreGroupPhotoAlbum. GroupPhotoRestoreSumTime = %{public}" PRId64
        ", Count = %{public}d, Offset = %{public}d, GroupPhotoRestoreExitCode = %{public}d",
        sumTime, count, offset, exitCode_.load());
}
}
}  // namespace OHOS