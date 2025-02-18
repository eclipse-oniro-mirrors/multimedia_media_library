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

#define MLOG_TAG "MediaLibraryCloneRestoreHighlight"

#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "clone_restore_highlight.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const std::vector<std::string> HIGHLIGHT_RATIO_WORD_ART = { "1_1", "3_2", "3_4", "microcard", "medium_card",
    "big_card", "screen_0_ver", "screen_0_hor" };
const std::vector<std::string> HIGHLIGHT_COVER_NAME = { "foreground", "background"};
const std::string MUSIC_DIR_DST_PATH = "/storage/media/local/files/highlight/music";
const std::string GARBLE_DST_PATH = "/storage/media/local/files";

const std::unordered_map<std::string, std::unordered_set<std::string>> NEEDED_COLUMNS_MAP = {
    { "AnalysisAlbum",
        {
            "album_id",
            "album_type",
            "album_subtype",
            "album_name",
            "cover_uri",
            "count",
            "date_modified"
        }
    },
    { "AnalysisPhotoMap",
        {
            "map_album",
            "map_asset"
        }
    },
    { "tab_highlight_album",
        {
            "id",
            "album_id",
            "ai_album_id",
            "sub_title",
            "cluster_type",
            "cluster_sub_type",
            "cluster_condition",
            "min_date_added",
            "max_date_added",
            "generate_time",
            "highlight_version",
            "remarks",
            "highlight_status"
        }
    },
    { "tab_highlight_cover_info",
        {
            "album_id",
            "ratio",
            "background",
            "foreground",
            "wordart",
            "is_covered",
            "color",
            "radius",
            "saturation",
            "brightness",
            "background_color_type",
            "shadow_level",
            "title_scale_x",
            "title_scale_y",
            "title_rect_width",
            "title_rect_height",
            "background_scale_x",
            "background_scale_y",
            "background_rect_width",
            "background_rect_height",
            "is_chosen",
            "layout_index",
            "cover_algo_version",
            "cover_service_version",
            "cover_key"
        }
    },
    { "tab_highlight_play_info",
        {
            "album_id",
            "music",
            "filter",
            "play_info",
            "is_chosen",
            "play_info_version",
            "play_info_id",
            "highlighting_algo_version",
            "camera_movement_algo_version",
            "transition_algo_version",
            "play_service_version",
            "status"
        }
    }
};

const std::unordered_map<std::string, std::unordered_set<std::string>> COMPARED_COLUMNS_MAP = {
    { "AnalysisAlbum",
        {
            "rank",
            "tag_id",
            "user_operation",
            "group_tag",
            "user_display_level",
            "is_me",
            "is_removed",
            "rename_operation",
            "is_local",
            "is_cover_satisfied"
        }
    },
    { "AnalysisPhotoMap",
        {
            "order_position"
        }
    },
    { "tab_highlight_album",
        {
            "insert_pic_count",
            "remove_pic_count",
            "share_screenshot_count",
            "share_cover_count",
            "rename_count",
            "change_cover_count",
            "render_viewed_times",
            "render_viewed_duration",
            "art_layout_viewed_times",
            "art_layout_viewed_duration",
            "music_edit_count",
            "filter_edit_count",
            "is_muted",
            "is_favorite",
            "theme",
            "pin_time"
        }
    },
    { "tab_highlight_cover_info",
        {
            "status"
        }
    },
    { "tab_highlight_play_info",
        {}
    }
};

template<typename Key, typename Value>
Value GetValueFromMap(const std::unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    if (it == map.end()) {
        return defaultValue;
    }
    return it->second;
}

void CloneRestoreHighlight::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, const std::string &backupRestoreDir)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;
    coverPath_ = backupRestoreDir + "/storage/media/local/files/highlight/cover/";
    musicDir_ = backupRestoreDir + "/storage/media/local/files/highlight/music";
    garblePath_ = backupRestoreDir + GARBLE_DST_PATH;
    albumPhotoCounter_.clear();
    failCnt_ = 0;
}

void CloneRestoreHighlight::RestoreAlbums()
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    MEDIA_INFO_LOG("restore highlight album start.");
    isCloneHighlight_ = true;
    isMapOrder_ = IsMapColumnOrderExist();
    GetAnalysisAlbumInfos();
    InsertIntoAnalysisAlbum();
    GetHighlightAlbumInfos();
    InsertIntoHighlightAlbum();
    MoveHighlightCovers();
    MoveHighlightMusic(musicDir_, MUSIC_DIR_DST_PATH);
    GetHighlightCoverInfos();
    InsertIntoHighlightCoverInfo();
    GetHighlightPlayInfos();
    InsertIntoHighlightPlayInfo();
}

void CloneRestoreHighlight::RestoreMaps(std::vector<FileInfo> &fileInfos)
{
    CHECK_AND_RETURN_LOG(isCloneHighlight_, "clone highlight flag is false.");
    MEDIA_INFO_LOG("restore highlight map start.");
    std::vector<NativeRdb::ValuesBucket> values;
    BatchQueryPhoto(fileInfos);
    for (const auto &fileInfo : fileInfos) {
        photoIdMap_.Insert(fileInfo.fileIdOld, fileInfo.fileIdNew);
        std::string fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            std::to_string(fileInfo.fileIdNew), MediaFileUtils::GetExtraUri(fileInfo.displayName, fileInfo.cloudPath));
        photoUriMap_.Insert(fileInfo.fileIdNew, fileUri);
        UpdateMapInsertValues(values, fileInfo);
    }
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    if (errCode != E_OK || rowNum != (long)values.size()) {
        int32_t failNums = values.size() - rowNum;
        MEDIA_ERR_LOG("RestoreMaps fail.");
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
            "RestoreMaps fail. num:" + std::to_string(failNums));
        failCnt_ += failNums;
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    }
}

void CloneRestoreHighlight::UpdateAlbums()
{
    CHECK_AND_RETURN_LOG(isCloneHighlight_, "clone highlight flag is false.");
    MEDIA_INFO_LOG("update highlight album start.");
    const std::string updateAlbumSql = "UPDATE AnalysisAlbum SET "
        "cover_uri = ?, "
        "count = (SELECT count(1) FROM AnalysisPhotoMap AS apm WHERE apm.map_album = AnalysisAlbum.album_id) "
        "WHERE album_id = ?";
    for (const auto &info : analysisInfos_) {
        if (!info.oldCoverUri.has_value() || !info.albumIdNew.has_value()) {
            continue;
        }
        BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateAlbumSql, { info.coverUri,
            info.albumIdNew.value() });
    }

    std::string updateCoverKeySql = "UPDATE tab_highlight_cover_info SET cover_key = ? "
        "WHERE album_id = ? AND ratio = ?";
    for (const auto &coverInfo : coverInfos_) {
        if (!coverInfo.ratio.has_value() || !coverInfo.highlightIdNew.has_value()) {
            continue;
        }
        auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
            [coverInfo](const AnalysisAlbumInfo& info) { return info.highlightIdNew == coverInfo.highlightIdNew; });
        if (it == analysisInfos_.end() || !it->albumName.has_value()) {
            continue;
        }
        std::string coverUri = it->albumName.value() + "_" + coverInfo.ratio.value() + "_" + it->coverUri;
        BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateCoverKeySql, { coverUri,
            coverInfo.highlightIdNew.value(), coverInfo.ratio.value()});
    }
    ReportCloneRestoreHighlightTask();
}

int32_t CloneRestoreHighlight::GetNewHighlightAlbumId(int32_t oldId)
{
    int32_t newId = -1;
    auto it = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
        [oldId](const HighlightAlbumInfo& info) { return info.highlightIdOld == oldId; });
    if (it != highlightInfos_.end() && it->highlightIdNew.has_value()) {
        newId = it->highlightIdNew.value();
    }
    return newId;
}

int32_t CloneRestoreHighlight::GetNewHighlightPhotoId(int32_t oldId)
{
    int32_t value = 0;
    photoIdMap_.Find(oldId, value);
    return value;
}

std::string CloneRestoreHighlight::GetNewHighlightPhotoUri(int32_t newId)
{
    std::string value = "";
    photoUriMap_.Find(newId, value);
    return value;
}

bool CloneRestoreHighlight::IsCloneHighlight()
{
    return isCloneHighlight_;
}

int32_t CloneRestoreHighlight::GetMaxAlbumId(const std::string &tableName, const std::string &idName)
{
    int32_t maxAlbumId = 1;
    const std::string querySql = "SELECT MAX(" + idName + ") " + idName + " FROM " + tableName;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, -1, "query resultSql is null.");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, idName);
        if (albumId.has_value()) {
            maxAlbumId = albumId.value() + 1;
        }
    }
    resultSet->Close();
    return maxAlbumId;
}

bool CloneRestoreHighlight::IsMapColumnOrderExist()
{
    bool result = false;
    std::unordered_set<std::string> intersection = GetCommonColumns("AnalysisPhotoMap");
    if (intersection.count("order_position") > 0) {
        result = true;
    }
    return result;
}

void CloneRestoreHighlight::GetAnalysisAlbumInfos()
{
    int32_t albumIdNow = GetMaxAlbumId("AnalysisAlbum", "album_id");
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string querySql = "SELECT * FROM AnalysisAlbum WHERE album_subtype IN (4104, 4105) "
            "LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_INFO_LOG("query resultSql is null.");
            break;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AnalysisAlbumInfo info;
            info.albumIdNew = std::make_optional<int32_t>(albumIdNow++);
            GetAnalysisRowInfo(info, resultSet);
            if (info.albumIdOld.has_value()) {
                oldAlbumIds_.emplace_back(info.albumIdOld.value());
            }
            analysisInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
    MEDIA_INFO_LOG("query AnalysisAlbum nums: %{public}zu", analysisInfos_.size());
}

void CloneRestoreHighlight::GetAnalysisRowInfo(AnalysisAlbumInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.albumIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
    info.albumType = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_type");
    info.albumSubtype = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_subtype");
    info.albumName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "album_name");
    info.oldCoverUri = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "cover_uri");
    info.dateModified = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "date_modified");
    info.rank = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "rank");
    info.tagId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "tag_id");
    info.userOperation = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "user_operation");
    info.groupTag = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "group_tag");
    info.userDisplayLevel = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "user_display_level");
    info.isMe = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_me");
    info.isRemoved = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_removed");
    info.renameOperation = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "rename_operation");
    info.isLocal = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_local");
    info.isCoverSatisfied = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_cover_satisfied");
}

void CloneRestoreHighlight::InsertIntoAnalysisAlbum()
{
    std::unordered_set<std::string> intersection = GetCommonColumns("AnalysisAlbum");
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < analysisInfos_.size(); index++) {
            NativeRdb::ValuesBucket value;
            GetAnalysisInsertValue(value, analysisInfos_[index + offset], intersection);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("AnalysisAlbum", values, rowNum);
        if (errCode != E_OK || rowNum != (long)values.size()) {
            int32_t failNums = values.size() - rowNum;
            MEDIA_ERR_LOG("insert into AnalysisAlbum fail, num: %{public}d", failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into AnalysisAlbum fail, num:" + std::to_string(failNums));
            failCnt_ += failNums;
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
    } while (offset < analysisInfos_.size());
}

void CloneRestoreHighlight::GetAnalysisInsertValue(NativeRdb::ValuesBucket &value, const AnalysisAlbumInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, "album_id", info.albumIdNew, intersection);
    PutIfInIntersection(value, "album_type", info.albumType, intersection);
    PutIfInIntersection(value, "album_subtype", info.albumSubtype, intersection);
    PutIfInIntersection(value, "album_name", info.albumName, intersection);
    PutIfInIntersection(value, "date_modified", info.dateModified, intersection);
    PutIfInIntersection(value, "rank", info.rank, intersection);
    PutIfInIntersection(value, "tag_id", info.tagId, intersection);
    PutIfInIntersection(value, "user_operation", info.userOperation, intersection);
    PutIfInIntersection(value, "group_tag", info.groupTag, intersection);
    PutIfInIntersection(value, "user_display_level", info.userDisplayLevel, intersection);
    PutIfInIntersection(value, "is_me", info.isMe, intersection);
    PutIfInIntersection(value, "is_removed", info.isRemoved, intersection);
    PutIfInIntersection(value, "rename_operation", info.renameOperation, intersection);
    PutIfInIntersection(value, "is_local", info.isLocal, intersection);
    PutIfInIntersection(value, "is_cover_satisfied", info.isCoverSatisfied, intersection);
}

void CloneRestoreHighlight::BatchQueryPhoto(std::vector<FileInfo> &fileInfos)
{
    std::stringstream querySql;
    querySql << "SELECT file_id, data FROM Photos WHERE data IN (";
    std::vector<NativeRdb::ValueObject> params;
    int32_t count = 0;
    for (const auto &fileInfo : fileInfos) {
        // no need query or already queried
        if (oldAlbumIds_.empty() || fileInfo.fileIdNew > 0) {
            continue;
        }
        querySql << (count++ > 0 ? "," : "");
        querySql << "?";
        params.emplace_back(fileInfo.cloudPath);
    }
    querySql << ")";
    auto resultSet = mediaLibraryRdb_->QuerySql(querySql.str(), params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val("file_id", resultSet);
        std::string data = GetStringVal("data", resultSet);
        auto it = std::find_if(fileInfos.begin(), fileInfos.end(),
            [data](const FileInfo& info) { return info.cloudPath == data; });
        if (it == fileInfos.end()) {
            continue;
        }
        it->fileIdNew = fileId;
        MEDIA_INFO_LOG("update fileId: %{public}s -> %{public}d", it->cloudPath.c_str(), it->fileIdNew);
    }
    resultSet->Close();
}

void CloneRestoreHighlight::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
    const FileInfo &fileInfo)
{
    // no need restore or info missing
    if (oldAlbumIds_.empty() || fileInfo.fileIdNew <= 0) {
        return;
    }
    for (int32_t oldAlbumId : oldAlbumIds_) {
        UpdateMapInsertValuesByAlbumId(values, fileInfo, oldAlbumId);
    }
}

void CloneRestoreHighlight::UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
    const FileInfo &fileInfo, const int32_t &oldAlbumId)
{
    std::string queryParam = "count(1)";
    if (isMapOrder_) {
        queryParam += ", order_position";
    }
    const std::string QUERY_SQL = "SELECT " + queryParam + " FROM AnalysisPhotoMap WHERE map_album = " +
        std::to_string(oldAlbumId) + " AND map_asset = " + std::to_string(fileInfo.fileIdOld);

    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return;
    }

    int32_t totalNum = GetInt32Val("count(1)", resultSet);
    if (totalNum < 1) {
        return;
    }

    auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
        [oldAlbumId](const AnalysisAlbumInfo& info) {
            return info.albumIdOld.has_value() && info.albumIdOld.value() == oldAlbumId;
        });
    if (it == analysisInfos_.end() || !it->albumIdNew.has_value()) {
        MEDIA_ERR_LOG("not find the needed album info, oldAlbumId: %{public}d", oldAlbumId);
        return;
    }

    std::string fileCoverUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
        std::to_string(fileInfo.fileIdOld), MediaFileUtils::GetExtraUri(fileInfo.displayName, fileInfo.cloudPath));
    if (it->oldCoverUri.has_value() && fileCoverUri == it->oldCoverUri.value()) {
        it->coverUri = photoUriMap_.ReadVal(fileInfo.fileIdNew);
    }

    std::optional<int32_t> order = std::nullopt;
    if (isMapOrder_) {
        order = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "order_position");
    }
    values.emplace_back(GetMapInsertValue(it->albumIdNew.value(), fileInfo.fileIdNew, order));
    std::lock_guard<mutex> lock(counterMutex_);
    std::string reportAlbumName = std::to_string(it->albumIdNew.value());
    if (it->albumName.has_value()) {
        reportAlbumName += "_" + it->albumName.value();
    }
    if (albumPhotoCounter_.count(reportAlbumName) == 0) {
        albumPhotoCounter_[reportAlbumName] = 0;
    }
    albumPhotoCounter_[reportAlbumName]++;
}

NativeRdb::ValuesBucket CloneRestoreHighlight::GetMapInsertValue(int32_t albumId, int32_t fileId,
    std::optional<int32_t> &order)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    if (order.has_value()) {
        value.PutInt("order_position", order.value());
    }
    return value;
}

int32_t CloneRestoreHighlight::BatchInsertWithRetry(const std::string &tableName,
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
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        }
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    }
    return errCode;
}

void CloneRestoreHighlight::GetHighlightAlbumInfos()
{
    int32_t idNow = GetMaxAlbumId("tab_highlight_album", "id");
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string querySql = "SELECT * FROM tab_highlight_album LIMIT "
            + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_INFO_LOG("query resultSql is null.");
            break;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            HighlightAlbumInfo info;
            info.highlightIdNew = std::make_optional<int32_t>(idNow++);
            GetHighlightRowInfo(info, resultSet);
            GetHighlightNewAlbumId(info);
            HighlightDeduplicate(info);
            highlightInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
    MEDIA_INFO_LOG("query tab_highlight_album nums: %{public}zu", highlightInfos_.size());
}

void CloneRestoreHighlight::GetHighlightRowInfo(HighlightAlbumInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.highlightIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "id");
    info.albumIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
    info.aiAlbumIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "ai_album_id");
    info.subTitle = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "sub_title");
    info.clusterType = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "cluster_type");
    info.clusterSubType = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "cluster_sub_type");
    info.clusterCondition = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "cluster_condition");
    info.minDateAdded = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "min_date_added");
    info.maxDateAdded = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "max_date_added");
    info.generateTime = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "generate_time");
    info.highlightVersion = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "highlight_version");
    info.remarks = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "remarks");
    info.highlightStatus = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "highlight_status");
    info.insertPicCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "insert_pic_count");
    info.removePicCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "remove_pic_count");
    info.shareScreenshotCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "share_screenshot_count");
    info.shareCoverCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "share_cover_count");
    info.renameCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "rename_count");
    info.changeCoverCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "change_cover_count");
    info.renderViewedTimes = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "render_viewed_times");
    info.renderViewedDuration = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "render_viewed_duration");
    info.artLayoutViewedTimes = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "art_layout_viewed_times");
    info.artLayoutViewedDuration = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet,
        "art_layout_viewed_duration");
    info.musicEditCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "music_edit_count");
    info.filterEditCount = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "filter_edit_count");
    info.isMuted = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_muted");
    info.isFavorite = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_favorite");
    info.theme = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "theme");
    info.pinTime = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, "pin_time");
}

void CloneRestoreHighlight::GetHighlightNewAlbumId(HighlightAlbumInfo &info)
{
    info.albumIdNew = info.albumIdOld;
    info.aiAlbumIdNew = info.aiAlbumIdOld;

    if (info.albumIdOld.has_value()) {
        auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
            [info](const AnalysisAlbumInfo &analysisInfo) {
                return analysisInfo.albumIdOld.has_value() &&
                    analysisInfo.albumIdOld.value() == info.albumIdOld.value();
            });
        if (it != analysisInfos_.end()) {
            info.albumIdNew = it->albumIdNew;
            it->highlightIdOld = info.highlightIdOld;
            it->highlightIdNew = info.highlightIdNew;
        }
    }

    if (info.aiAlbumIdOld.has_value()) {
        auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
            [info](const AnalysisAlbumInfo &analysisInfo) {
                return analysisInfo.albumIdOld.has_value() &&
                    analysisInfo.albumIdOld.value() == info.aiAlbumIdOld.value();
            });
        if (it != analysisInfos_.end()) {
            info.aiAlbumIdNew = it->albumIdNew;
            it->highlightIdOld = info.highlightIdOld;
            it->highlightIdNew = info.highlightIdNew;
        }
    }
}

void CloneRestoreHighlight::InsertIntoHighlightAlbum()
{
    std::unordered_set<std::string> intersection = GetCommonColumns("tab_highlight_album");
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < highlightInfos_.size(); index++) {
            if (!highlightInfos_[index + offset].clusterType.has_value() ||
                !highlightInfos_[index + offset].clusterSubType.has_value() ||
                !highlightInfos_[index + offset].clusterCondition.has_value() ||
                !highlightInfos_[index + offset].highlightVersion.has_value()) {
                continue;
            }

            NativeRdb::ValuesBucket value;
            GetHighlightInsertValue(value, highlightInfos_[index + offset], intersection);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_album", values, rowNum);
        if (errCode != E_OK || rowNum != (long)values.size()) {
            int32_t failNums = values.size() - rowNum;
            MEDIA_ERR_LOG("insert into tab_highlight_album fail, num: %{public}d", failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_album fail, num:" + std::to_string(failNums));
            failCnt_ += failNums;
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
    } while (offset < highlightInfos_.size());
}

void CloneRestoreHighlight::GetHighlightInsertValue(NativeRdb::ValuesBucket &value, const HighlightAlbumInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, "id", info.highlightIdNew, intersection);
    PutIfInIntersection(value, "album_id", info.albumIdNew, intersection);
    PutIfInIntersection(value, "ai_album_id", info.aiAlbumIdNew, intersection);
    PutIfInIntersection(value, "sub_title", info.subTitle, intersection);
    PutIfInIntersection(value, "min_date_added", info.minDateAdded, intersection);
    PutIfInIntersection(value, "max_date_added", info.maxDateAdded, intersection);
    PutIfInIntersection(value, "generate_time", info.generateTime, intersection);
    PutIfInIntersection(value, "cluster_type", info.clusterType, intersection);
    PutIfInIntersection(value, "cluster_sub_type", info.clusterSubType, intersection);
    PutIfInIntersection(value, "cluster_condition", info.clusterCondition, intersection);
    PutIfInIntersection(value, "highlight_status", info.highlightStatus, intersection);
    PutIfInIntersection(value, "remarks", info.remarks, intersection);
    PutIfInIntersection(value, "highlight_version", info.highlightVersion, intersection);
    PutIfInIntersection(value, "insert_pic_count", info.insertPicCount, intersection);
    PutIfInIntersection(value, "remove_pic_count", info.removePicCount, intersection);
    PutIfInIntersection(value, "share_screenshot_count", info.shareScreenshotCount, intersection);
    PutIfInIntersection(value, "share_cover_count", info.shareCoverCount, intersection);
    PutIfInIntersection(value, "rename_count", info.renameCount, intersection);
    PutIfInIntersection(value, "change_cover_count", info.changeCoverCount, intersection);
    PutIfInIntersection(value, "render_viewed_times", info.renderViewedTimes, intersection);
    PutIfInIntersection(value, "render_viewed_duration", info.renderViewedDuration, intersection);
    PutIfInIntersection(value, "art_layout_viewed_times", info.artLayoutViewedTimes, intersection);
    PutIfInIntersection(value, "art_layout_viewed_duration", info.artLayoutViewedDuration, intersection);
    PutIfInIntersection(value, "music_edit_count", info.musicEditCount, intersection);
    PutIfInIntersection(value, "filter_edit_count", info.filterEditCount, intersection);
    PutIfInIntersection(value, "is_muted", info.isMuted, intersection);
    PutIfInIntersection(value, "is_favorite", info.isFavorite, intersection);
    PutIfInIntersection(value, "theme", info.theme, intersection);
    PutIfInIntersection(value, "pin_time", info.pinTime, intersection);
}

void CloneRestoreHighlight::MoveHighlightCovers()
{
    std::unordered_set<int32_t> hasMovedAlbums;
    for (const auto &info : analysisInfos_) {
        if (!info.albumIdNew.has_value() || !info.oldCoverUri.has_value() || !info.highlightIdOld.has_value() ||
            !info.highlightIdNew.has_value()) {
            continue;
        }

        if (hasMovedAlbums.count(info.highlightIdOld.value()) > 0) {
            continue;
        }
        hasMovedAlbums.insert(info.highlightIdOld.value());
        std::string srcDir = coverPath_ + std::to_string(info.highlightIdOld.value()) + "/";
        MoveHighlightWordart(info, srcDir);
        MoveHighlightGround(info, srcDir);
    }
}

void CloneRestoreHighlight::MoveHighlightWordart(const AnalysisAlbumInfo &info, const std::string &srcDir)
{
    for (const auto &ratio : HIGHLIGHT_RATIO_WORD_ART) {
        std::string srcPath = srcDir + ratio + "/wordart.png";
        if (!MediaFileUtils::IsFileExists(srcPath)) {
            continue;
        }
        std::string dstDir = "/storage/media/local/files/highlight/cover/" +
            std::to_string(info.highlightIdNew.value()) + "/" + ratio;
        if (!MediaFileUtils::CreateDirectory(dstDir)) {
            MEDIA_ERR_LOG("create %{public}s failed",
                BackupFileUtils::GarbleFilePath(dstDir, sceneCode_, GARBLE_DST_PATH).c_str());
            continue;
        }
        std::string dstPath = dstDir + "/wordart.png";
        int32_t errCode = BackupFileUtils::MoveFile(srcPath.c_str(), dstPath.c_str(), sceneCode_);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("move file failed, srcPath:%{public}s, dstPath:%{public}s, errCode:%{public}d",
                BackupFileUtils::GarbleFilePath(srcPath, sceneCode_, garblePath_).c_str(),
                BackupFileUtils::GarbleFilePath(dstPath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
        }
    }
}

void CloneRestoreHighlight::MoveHighlightGround(const AnalysisAlbumInfo &info, const std::string &srcDir)
{
    for (const auto &fileName : HIGHLIGHT_COVER_NAME) {
        std::string groundPath = srcDir + "/full/" + fileName + ".png";
        std::string groundDstDir = "/storage/media/local/files/highlight/cover/" +
            std::to_string(info.highlightIdNew.value()) + "/full";
        if (!MediaFileUtils::IsFileExists(groundPath)) {
            continue;
        }

        if (!MediaFileUtils::CreateDirectory(groundDstDir)) {
            MEDIA_ERR_LOG("create %{public}s failed",
                BackupFileUtils::GarbleFilePath(groundDstDir, sceneCode_, GARBLE_DST_PATH).c_str());
            continue;
        }

        std::string groundDstPath = groundDstDir + "/" + fileName + ".png";
        int32_t errCode = BackupFileUtils::MoveFile(groundPath.c_str(), groundDstPath.c_str(), sceneCode_);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("move file failed, srcPath:%{public}s, dstPath:%{public}s, errCode:%{public}d",
                BackupFileUtils::GarbleFilePath(groundPath, sceneCode_, garblePath_).c_str(),
                BackupFileUtils::GarbleFilePath(groundDstPath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
        }
    }
}

int32_t CloneRestoreHighlight::MoveHighlightMusic(const std::string &srcDir, const std::string &dstDir)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(dstDir), E_FAIL, "create dstDir %{public}s failed",
        BackupFileUtils::GarbleFilePath(dstDir, sceneCode_, GARBLE_DST_PATH).c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcDir), E_OK, "%{public}s doesn't exist, skip.",
        BackupFileUtils::GarbleFilePath(srcDir, sceneCode_, garblePath_).c_str());
    for (const auto &dirEntry : std::filesystem::directory_iterator{ srcDir }) {
        std::string srcFilePath = dirEntry.path();
        if (MediaFileUtils::IsDirectory(srcFilePath)) {
            int32_t index = srcFilePath.rfind("/");
            std::string subDir = srcFilePath.substr(index);
            MoveHighlightMusic(srcFilePath, dstDir + subDir);
        } else {
            std::string tmpFilePath = srcFilePath;
            std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
            int32_t errCode = BackupFileUtils::MoveFile(srcFilePath.c_str(), dstFilePath.c_str(), sceneCode_);
            if (errCode != E_OK) {
                MEDIA_ERR_LOG("move file failed, srcPath:%{public}s, dstPath:%{public}s, errCode:%{public}d",
                    BackupFileUtils::GarbleFilePath(srcFilePath, sceneCode_, garblePath_).c_str(),
                    BackupFileUtils::GarbleFilePath(dstFilePath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
            }
        }
    }
    return E_OK;
}

void CloneRestoreHighlight::GetHighlightCoverInfos()
{
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string querySql = "SELECT * FROM tab_highlight_cover_info LIMIT " + std::to_string(offset) + ", " +
            std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_INFO_LOG("query resultSql is null.");
            break;
        }

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
            if (!albumId.has_value()) {
                continue;
            }
            auto itAlbum = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
                [albumId](const HighlightAlbumInfo& hiInfo) {
                    return hiInfo.highlightIdOld.has_value() && hiInfo.highlightIdOld == albumId;
                });
            if (itAlbum == highlightInfos_.end()) {
                MEDIA_ERR_LOG("can not find coverinfo in highlight");
                continue;
            }
            HighlightCoverInfo info;
            info.highlightIdNew = itAlbum->highlightIdNew;
            GetCoverRowInfo(info, resultSet);
            GetCoverGroundSourceInfo(info, resultSet);
            coverInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
    MEDIA_INFO_LOG("query tab_highlight_cover_info nums: %{public}zu", coverInfos_.size());
}

void CloneRestoreHighlight::GetCoverRowInfo(HighlightCoverInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.ratio = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "ratio");
    info.isCovered = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_covered");
    info.color = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "color");
    info.radius = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "radius");
    info.saturation = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "saturation");
    info.brightness = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "brightness");
    info.backgroundColorType = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "background_color_type");
    info.shadowLevel = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "shadow_level");
    info.scaleX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "title_scale_x");
    info.scaleY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "title_scale_y");
    info.rectWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "title_rect_width");
    info.rectHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "title_rect_height");
    info.bgrScaleX = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "background_scale_x");
    info.bgrScaleY = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "background_scale_y");
    info.bgrRectWidth = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "background_rect_width");
    info.bgrRectHeight = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, "background_rect_height");
    info.layoutIndex = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "layout_index");
    info.coverAlgoVer = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "cover_algo_version");
    info.coverServiceVer = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "cover_service_version");
    info.status = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "status");
}

void CloneRestoreHighlight::GetCoverGroundSourceInfo(HighlightCoverInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (!info.highlightIdNew.has_value() || !info.ratio.has_value()) {
        return;
    }
    std::string wordartPath = "/storage/media/local/files/highlight/cover/" +
        std::to_string(info.highlightIdNew.value()) + "/" + info.ratio.value() + "/wordart.png";
    if (MediaFileUtils::IsFileExists(wordartPath)) {
        info.wordart = "file://media/highlight/cover/" + std::to_string(info.highlightIdNew.value()) +
            "/" + info.ratio.value() + "/wordart.png?oper=highlight";
    }

    for (const auto &fileName : HIGHLIGHT_COVER_NAME) {
        std::string groundPath = "/storage/media/local/files/highlight/cover/" +
            std::to_string(info.highlightIdNew.value()) + "/full/" + fileName + ".png";
        if (BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, fileName).has_value()
            && MediaFileUtils::IsFileExists(groundPath)) {
            if (fileName == HIGHLIGHT_COVER_NAME[0]) {
                info.foreground = "file://media/highlight/cover/" +
                    std::to_string(info.highlightIdNew.value()) + "/full/" + fileName + ".png?oper=highlight";
            } else {
                info.background = "file://media/highlight/cover/" +
                    std::to_string(info.highlightIdNew.value()) + "/full/" + fileName + ".png?oper=highlight";
            }
        }
    }
}

void CloneRestoreHighlight::InsertIntoHighlightCoverInfo()
{
    std::unordered_set<std::string> intersection = GetCommonColumns("tab_highlight_cover_info");
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < coverInfos_.size(); index++) {
            if (!coverInfos_[index + offset].highlightIdNew.has_value()) {
                continue;
            }

            NativeRdb::ValuesBucket value;
            GetCoverInsertValue(value, coverInfos_[index + offset], intersection);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_cover_info", values, rowNum);
        if (errCode != E_OK || rowNum != (long)values.size()) {
            int32_t failNums = values.size() - rowNum;
            MEDIA_ERR_LOG("insert into tab_highlight_cover_info fail, num: %{public}d", failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_cover_info fail, num:" + std::to_string(failNums));
            failCnt_ += failNums;
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
    } while (offset < coverInfos_.size());
}

void CloneRestoreHighlight::GetCoverInsertValue(NativeRdb::ValuesBucket &value, const HighlightCoverInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, "album_id", info.highlightIdNew, intersection);
    PutIfInIntersection(value, "ratio", info.ratio, intersection);
    PutIfInIntersection(value, "background", info.background, intersection);
    PutIfInIntersection(value, "foreground", info.foreground, intersection);
    PutIfInIntersection(value, "wordart", info.wordart, intersection);
    PutIfInIntersection(value, "is_covered", info.isCovered, intersection);
    PutIfInIntersection(value, "color", info.color, intersection);
    PutIfInIntersection(value, "radius", info.radius, intersection);
    PutIfInIntersection(value, "saturation", info.saturation, intersection);
    PutIfInIntersection(value, "brightness", info.brightness, intersection);
    PutIfInIntersection(value, "background_color_type", info.backgroundColorType, intersection);
    PutIfInIntersection(value, "shadow_level", info.shadowLevel, intersection);
    PutIfInIntersection(value, "title_scale_x", info.scaleX, intersection);
    PutIfInIntersection(value, "title_scale_y", info.scaleY, intersection);
    PutIfInIntersection(value, "title_rect_width", info.rectWidth, intersection);
    PutIfInIntersection(value, "title_rect_height", info.rectHeight, intersection);
    PutIfInIntersection(value, "background_scale_x", info.bgrScaleX, intersection);
    PutIfInIntersection(value, "background_scale_y", info.bgrScaleY, intersection);
    PutIfInIntersection(value, "background_rect_width", info.bgrRectWidth, intersection);
    PutIfInIntersection(value, "background_rect_height", info.bgrRectHeight, intersection);
    PutIfInIntersection(value, "layout_index", info.layoutIndex, intersection);
    PutIfInIntersection(value, "cover_algo_version", info.coverAlgoVer, intersection);
    PutIfInIntersection(value, "cover_service_version", info.coverServiceVer, intersection);
    PutIfInIntersection(value, "status", info.status, intersection);
}

void CloneRestoreHighlight::GetHighlightPlayInfos()
{
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string querySql = "SELECT * FROM tab_highlight_play_info LIMIT " + std::to_string(offset) + ", " +
            std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_INFO_LOG("query resultSql is null.");
            break;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
            if (!albumId.has_value()) {
                continue;
            }
            auto itAlbum = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
                [albumId](const HighlightAlbumInfo& hiInfo) {
                    return hiInfo.highlightIdOld.has_value() && hiInfo.highlightIdOld == albumId;
                });
            if (itAlbum == highlightInfos_.end()) {
                MEDIA_ERR_LOG("can not find playinfo in highlight, albumId: %{public}d", albumId.value());
                continue;
            }
            HighlightPlayInfo info;
            info.highlightIdNew = itAlbum->highlightIdNew;
            GetPlayRowInfo(info, resultSet);
            playInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
    MEDIA_INFO_LOG("query tab_highlight_play_info nums: %{public}zu", playInfos_.size());
}

void CloneRestoreHighlight::GetPlayRowInfo(HighlightPlayInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    info.playId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "play_info_id");
    info.music = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "music");
    info.filter = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "filter");
    info.isChosen = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "is_chosen");
    info.pInfoVer = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "play_info_version");
    info.hAlgoVer = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "highlighting_algo_version");
    info.cameraAlgoVer = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "camera_movement_algo_version");
    info.transAlgoVer = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "transition_algo_version");
    info.playServiceVer = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "play_service_version");
    info.status = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "status");
}

void CloneRestoreHighlight::InsertIntoHighlightPlayInfo()
{
    std::unordered_set<std::string> intersection = GetCommonColumns("tab_highlight_play_info");
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < playInfos_.size(); index++) {
            if (!playInfos_[index + offset].highlightIdNew.has_value()) {
                continue;
            }

            NativeRdb::ValuesBucket value;
            GetPlayInsertValue(value, playInfos_[index + offset], intersection);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_play_info", values, rowNum);
        if (errCode != E_OK || rowNum != (long)values.size()) {
            int32_t failNums = values.size() - rowNum;
            MEDIA_ERR_LOG("insert into tab_highlight_play_info fail, num: %{public}d", failNums);
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_play_info fail, num:" + std::to_string(failNums));
            failCnt_ += failNums;
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        offset += PAGE_SIZE;
    } while (offset < playInfos_.size());
}

void CloneRestoreHighlight::GetPlayInsertValue(NativeRdb::ValuesBucket &value, const HighlightPlayInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, "album_id", info.highlightIdNew, intersection);
    PutIfInIntersection(value, "play_info_id", info.playId, intersection);
    PutIfInIntersection(value, "music", info.music, intersection);
    PutIfInIntersection(value, "filter", info.filter, intersection);
    PutIfInIntersection(value, "is_chosen", info.isChosen, intersection);
    PutIfInIntersection(value, "play_info_version", info.pInfoVer, intersection);
    PutIfInIntersection(value, "highlighting_algo_version", info.hAlgoVer, intersection);
    PutIfInIntersection(value, "camera_movement_algo_version", info.cameraAlgoVer, intersection);
    PutIfInIntersection(value, "transition_algo_version", info.transAlgoVer, intersection);
    PutIfInIntersection(value, "play_service_version", info.playServiceVer, intersection);
    PutIfInIntersection(value, "status", info.status, intersection);
}

std::unordered_set<std::string> CloneRestoreHighlight::GetCommonColumns(const std::string &tableName)
{
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, tableName);
    std::unordered_map<std::string, std::string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, tableName);
    auto neededColumns = GetValueFromMap(NEEDED_COLUMNS_MAP, tableName);
    std::unordered_set<std::string> result = neededColumns;
    auto comparedColumns = GetValueFromMap(COMPARED_COLUMNS_MAP, tableName);
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() && comparedColumns.count(it->first) > 0) {
            result.insert(it->first);
        }
    }
    return result;
}

void CloneRestoreHighlight::ReportCloneRestoreHighlightTask()
{
    int32_t maxCnt = 0;
    int32_t totalCnt = 0;
    const int32_t ERR_STATUS = 1;
    // albumPhotoCounter_ in single thread does not hold a lock
    for (auto &counter : albumPhotoCounter_) {
        maxCnt = maxCnt > counter.second ? maxCnt : counter.second;
        totalCnt += counter.second;
        MEDIA_INFO_LOG("updateMapInsertValues albumName: %{public}s, photo count: %{public}d",
            counter.first.c_str(), counter.second);
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
            .Report("Highlight Photo Map", std::to_string(ERR_STATUS),
            "albumName: " + counter.first + ", photo count: " + std::to_string(counter.second));
    }
    double meanCnt = albumPhotoCounter_.size() == 0 ? 0 : (double) totalCnt / albumPhotoCounter_.size();
    MEDIA_INFO_LOG("Highlight photos max: %{public}d, mean: %{public}f", maxCnt, meanCnt);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("Highlight Photos", std::to_string(ERR_STATUS),
        "max: " + std::to_string(maxCnt) + ", mean: " + std::to_string(meanCnt));

    MEDIA_INFO_LOG("Highlight restore failCnt_: %{public}d", failCnt_);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("Highlight restore", std::to_string(ERR_STATUS), "failCnt_: " + std::to_string(failCnt_));
}

void CloneRestoreHighlight::HighlightDeduplicate(const HighlightAlbumInfo &info)
{
    if (!info.clusterType.has_value() || !info.clusterSubType.has_value() || !info.clusterCondition.has_value() ||
        !info.highlightVersion.has_value() || !info.albumIdOld.has_value()) {
            return;
    }

    if (info.highlightStatus.has_value() && info.highlightStatus.value() != 1) {
        return;
    }

    auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
        [info](const AnalysisAlbumInfo &analysisInfo) {
            return analysisInfo.albumIdOld.has_value() &&
                analysisInfo.albumIdOld.value() == info.albumIdOld.value();
        });
    if (it == analysisInfos_.end() || !it->albumName.has_value()) {
        return;
    }

    std::string querySql = "SELECT count(1), t.id FROM tab_highlight_album AS t INNER JOIN AnalysisAlbum AS a "
        "ON t.album_id = a.album_id WHERE t.cluster_type = '" + info.clusterType.value() +
        "' AND t.cluster_sub_type = '" + info.clusterSubType.value() +
        "' AND t.cluster_condition = '" + info.clusterCondition.value() +
        "' AND a.album_name = '" + it->albumName.value() + "'";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return;
    }

    int32_t totalNum = GetInt32Val("count(1)", resultSet);
    if (totalNum < 1) {
        return;
    }

    std::optional<int32_t> highlightId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "id");
    if (!highlightId.has_value()) {
        return;
    }

    std::string updateSql = "UPDATE tab_highlight_album SET highlight_status = -2 WHERE id = ?";
    BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updateSql, { highlightId.value() });
    MEDIA_INFO_LOG("deduplicate highlight album, highlight id: %{public}d, album name: %{public}s",
        highlightId.value(), it->albumName.value().c_str());
}
} // namespace OHOS::Media