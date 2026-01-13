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

#include "clone_restore_highlight.h"

#include "backup_file_utils.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const std::vector<std::string> HIGHLIGHT_RATIO_WORD_ART = { "1_1", "3_2", "3_4", "microcard", "medium_card",
    "big_card", "screen_0_ver", "screen_0_hor" };
const std::vector<std::string> HIGHLIGHT_COVER_NAME = { "foreground", "background" };
const std::string MUSIC_DIR_DST_PATH = "/storage/media/local/files/highlight/music";
const std::string GARBLE_DST_PATH = "/storage/media/local/files";
const int32_t HIGHLIGHT_STATUS_NOT_PRODUCE = -1;
const int32_t HIGHLIGHT_STATUS_DELETE = -4;
const std::string RESTORE_STATUS_SUCCESS = "1";
const int32_t DEFAULT_IS_VIEWED_VALUE = 1;

const std::unordered_map<std::string, std::unordered_set<std::string>> ALBUM_COLUMNS_MAP = {
    { "AnalysisAlbum",
        {
            "album_id",
            "album_type",
            "album_subtype",
            "album_name",
            "cover_uri",
            "count",
            "date_modified",
            "rank",
            "tag_id",
            "user_operation",
            "group_tag",
            "user_display_level",
            "is_me",
            "is_removed",
            "rename_operation",
            "is_local",
            "is_cover_satisfied",
            "relationship"
        }
    },
    { "AnalysisPhotoMap",
        {
            "map_album",
            "map_asset",
            "order_position"
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
            "highlight_status",
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
            "pin_time",
            "use_subtitle",
            "highlight_location",
            "is_viewed",
            "notification_time"
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
            "cover_key",
            "status"
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

template<typename Key, typename Value>
Value GetValueFromMap(const std::unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it != map.end(), defaultValue);
    return it->second;
}

void CloneRestoreHighlight::Init(const InitInfo &info)
{
    sceneCode_ = info.sceneCode;
    taskId_ = info.taskId;
    mediaLibraryRdb_ = info.mediaLibraryRdb;
    mediaRdb_ = info.mediaRdb;

    std::string highlightSourcePath = info.backupRestoreDir + "/storage/media/local/files/highlight/";
    isHighlightDirExist_ = MediaFileUtils::IsDirectory(highlightSourcePath);
    MEDIA_INFO_LOG("/highlight/ source dir %{public}s.", isHighlightDirExist_ ? "exist" : "don't exist");
    coverPath_ = highlightSourcePath + "cover/";
    musicDir_ = highlightSourcePath + "music";
    garblePath_ = info.backupRestoreDir + GARBLE_DST_PATH;

    photoInfoMap_ = info.photoInfoMap;
}

void CloneRestoreHighlight::Restore()
{
    int64_t startPreprocess = MediaFileUtils::UTCTimeMilliSeconds();
    Preprocess();
    int64_t startRestoreAlbums = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreAlbums();
    int64_t startRestoreMaps = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreMaps();
    int64_t startUpdateAlbums = MediaFileUtils::UTCTimeMilliSeconds();
    UpdateAlbums();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    restoreTimeCost_ += end - startPreprocess;
    MEDIA_INFO_LOG("TimeCost: Preprocess: %{public}" PRId64 ",RestoreAlbums: %{public}" PRId64
        ", RestoreMaps: %{public}" PRId64 ", UpdateAlbums: %{public}" PRId64, startRestoreAlbums - startPreprocess,
        startRestoreMaps - startRestoreAlbums, startUpdateAlbums - startRestoreMaps, end - startUpdateAlbums);
}

void CloneRestoreHighlight::Preprocess()
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    const std::vector<std::string> SQLS = {
        "ALTER TABLE AnalysisAlbum ADD COLUMN need_restore_highlight INTEGER DEFAULT 0 ",
        "UPDATE AnalysisAlbum SET need_restore_highlight = 1 "
            " WHERE album_id IN (SELECT album_id FROM tab_highlight_album WHERE highlight_status > 0 "
            " UNION SELECT ai_album_id FROM tab_highlight_album WHERE highlight_status > 0) ",
        "CREATE INDEX idx_need_restore_highlight ON AnalysisAlbum(need_restore_highlight) "
            " WHERE need_restore_highlight = 1 ",
    };
    for (const auto &sql : SQLS) {
        int32_t errCode = BackupDatabaseUtils::ExecuteSQL(mediaRdb_, sql);
        if (errCode != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute %{public}s failed, errCode: %{public}d", sql.c_str(), errCode);
            ErrorInfo errorInfo(RestoreError::INIT_FAILED, 0, std::to_string(errCode),
                "clone restore highlight preprocess failed");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            return;
        }
    }
    isCloneHighlight_ = true;
}

void CloneRestoreHighlight::RestoreAlbums()
{
    CHECK_AND_RETURN_LOG(isCloneHighlight_, "clone highlight flag is false.");
    MEDIA_INFO_LOG("restore highlight album start.");
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

void CloneRestoreHighlight::RestoreMaps()
{
    CHECK_AND_RETURN_LOG(isCloneHighlight_, "clone highlight flag is false.");
    MEDIA_INFO_LOG("restore highlight map start.");
    int32_t totalNumber = GetTotalNumberOfMaps();
    MEDIA_INFO_LOG("totalNumber: %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += PAGE_SIZE) {
        RestoreMapsBatch();
    }
}

int32_t CloneRestoreHighlight::GetTotalNumberOfMaps()
{
    const std::string QUERY_SQL = "SELECT count(1) AS count FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype IN (4104, 4105) AND a.need_restore_highlight = 1 ";
    return BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_SQL, "count");
}

void CloneRestoreHighlight::RestoreMapsBatch()
{
    std::vector<NativeRdb::ValuesBucket> values;
    int64_t startUpdateTime = MediaFileUtils::UTCTimeMilliSeconds();
    UpdateMapInsertValues(values);
    int64_t startInsertTime = MediaFileUtils::UTCTimeMilliSeconds();
    InsertAnalysisPhotoMap(values);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateMapInsertValues: %{public}" PRId64 ", InsertAnalysisPhotoMap: %{public}" PRId64,
        startInsertTime - startUpdateTime, endTime - startInsertTime);
}

void CloneRestoreHighlight::UpdateAlbums()
{
    CHECK_AND_RETURN_LOG(isCloneHighlight_, "clone highlight flag is false.");
    MEDIA_INFO_LOG("update highlight album start.");
    const std::string UPDATE_ALBUM_SQL = "UPDATE AnalysisAlbum SET "
        "cover_uri = ?, "
        "count = (SELECT count(1) FROM AnalysisPhotoMap AS apm WHERE apm.map_album = AnalysisAlbum.album_id) "
        "WHERE album_id = ?";
    for (const auto &info : analysisInfos_) {
        bool cond = (!info.oldCoverUri.has_value() || !info.albumIdNew.has_value());
        CHECK_AND_CONTINUE(!cond);
        BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_ALBUM_SQL, { info.coverUri,
            info.albumIdNew.value() });
    }

    const std::string UPDATE_COVER_KEY_SQL = "UPDATE tab_highlight_cover_info SET cover_key = ? "
        "WHERE album_id = ? AND ratio = ?";
    for (const auto &coverInfo : coverInfos_) {
        bool cond = (!coverInfo.ratio.has_value() || !coverInfo.highlightIdNew.has_value());
        CHECK_AND_CONTINUE(!cond);
        auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
            [coverInfo](const AnalysisAlbumInfo& info) { return info.highlightIdNew == coverInfo.highlightIdNew; });
        cond = (it == analysisInfos_.end() || !it->albumName.has_value());
        CHECK_AND_CONTINUE(!cond);
        std::string coverUri = it->albumName.value() + "_" + coverInfo.ratio.value() + "_" + it->coverUri;
        BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_COVER_KEY_SQL, { coverUri,
            coverInfo.highlightIdNew.value(), coverInfo.ratio.value()});
    }
}

int32_t CloneRestoreHighlight::GetNewHighlightAlbumId(int32_t oldId)
{
    int32_t newId = -1;
    auto it = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
        [oldId](const HighlightAlbumInfo& info) { return info.highlightIdOld == oldId; });
    bool cond = (it != highlightInfos_.end() && it->highlightIdNew.has_value());
    CHECK_AND_EXECUTE(!cond, newId = it->highlightIdNew.value());
    return newId;
}

int32_t CloneRestoreHighlight::GetNewHighlightPhotoId(int32_t oldId)
{
    auto it = photoInfoMap_.find(oldId);
    if (it == photoInfoMap_.end()) {
        return 0;
    }
    return it->second.fileIdNew;
}

std::string CloneRestoreHighlight::GetNewHighlightPhotoUri(int32_t oldId)
{
    auto it = photoInfoMap_.find(oldId);
    if (it == photoInfoMap_.end()) {
        return "";
    }
    PhotoInfo photoInfo = it->second;
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
        std::to_string(photoInfo.fileIdNew), MediaFileUtils::GetExtraUri(photoInfo.displayName, photoInfo.cloudPath));
}

bool CloneRestoreHighlight::IsCloneHighlight()
{
    return isCloneHighlight_;
}

std::string CloneRestoreHighlight::GetDefaultPlayInfo()
{
    nlohmann::json playInfo;
    playInfo["beatsInfo"] = nlohmann::json::array();
    playInfo["effectline"] = nlohmann::json::object();
    playInfo["effectline"]["effectline"] = nlohmann::json::array();
    playInfo["timeline"] = nlohmann::json::array();
    return playInfo.dump();
}

void CloneRestoreHighlight::UpdateHighlightStatus(const std::vector<int32_t> &highlightIds)
{
    std::unordered_map<int32_t, std::vector<NativeRdb::ValueObject>> highlightStatusMap;
    for (auto highlightId : highlightIds) {
        auto it = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
        [highlightId](const HighlightAlbumInfo &highlightInfo) {
            return highlightInfo.highlightIdNew.has_value() && highlightInfo.highlightIdNew.value() == highlightId &&
                highlightInfo.highlightStatus.has_value() && highlightInfo.highlightStatus.value() > 0;
        });
        CHECK_AND_CONTINUE(it != highlightInfos_.end());
        UpdateHighlightStatusMap(it->highlightStatus.value(), highlightId, highlightStatusMap);
    }
    UpdateHighlightStatusInDatabase(highlightStatusMap);
}

void CloneRestoreHighlight::UpdateHighlightStatusMap(int32_t highlightStatus, int32_t highlightId,
    std::unordered_map<int32_t, std::vector<NativeRdb::ValueObject>> &highlightStatusMap)
{
    std::vector<NativeRdb::ValueObject> &highlightIds = highlightStatusMap[highlightStatus];
    highlightIds.emplace_back(highlightId);
}

void CloneRestoreHighlight::UpdateHighlightStatusInDatabase(
    const std::unordered_map<int32_t, std::vector<NativeRdb::ValueObject>> &highlightStatusMap)
{
    CHECK_AND_RETURN(!highlightStatusMap.empty());
    for (const auto &[highlightStatus, highlightIds] : highlightStatusMap) {
        int32_t changedRows = -1;
        std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
            make_unique<NativeRdb::AbsRdbPredicates>("tab_highlight_album");
        updatePredicates->In("id", highlightIds);
        NativeRdb::ValuesBucket rdbValues;
        rdbValues.PutInt("highlight_status", highlightStatus);
        int32_t errCode = BackupDatabaseUtils::Update(mediaLibraryRdb_, changedRows, rdbValues, updatePredicates);
        std::stringstream updateReport;
        updateReport << "highlightStatus: " << highlightStatus << ", to be pushed: " << highlightIds.size() <<
            ", update: " << changedRows;
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
            .Report("CLONE_RESTORE_HIGHLIGHT_STATUS", std::to_string(errCode), updateReport.str());
    }
}

int32_t CloneRestoreHighlight::GetMaxAlbumId(const std::string &tableName, const std::string &idName)
{
    int32_t maxAlbumId = 1;
    const std::string QUERY_SQL = "SELECT MAX(" + idName + ") " + idName + " FROM " + tableName;
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, QUERY_SQL);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, -1, "query resultSql is null.");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, idName);
        CHECK_AND_EXECUTE(!albumId.has_value(), maxAlbumId = albumId.value() + 1);
    }
    resultSet->Close();
    return maxAlbumId;
}

bool CloneRestoreHighlight::IsMapColumnOrderExist()
{
    bool result = false;
    std::unordered_set<std::string> intersection = GetCommonColumns("AnalysisPhotoMap");
    CHECK_AND_EXECUTE(intersection.count("order_position") <= 0, result = true);
    return result;
}

void CloneRestoreHighlight::GetAnalysisAlbumInfos()
{
    int32_t albumIdNow = GetMaxAlbumId("AnalysisAlbum", "album_id");
    maxIdOfAlbum_ = albumIdNow;
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string QUERY_SQL = "SELECT * FROM AnalysisAlbum "
            " WHERE album_subtype IN (4104, 4105) AND need_restore_highlight = 1 "
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
        CHECK_AND_BREAK_INFO_LOG(resultSet != nullptr, "query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AnalysisAlbumInfo info;
            GetAnalysisRowInfo(info, resultSet);
            info.albumIdNew = std::make_optional<int32_t>(albumIdNow++);
            analysisInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount == PAGE_SIZE);
    MEDIA_INFO_LOG("query AnalysisAlbum nums: %{public}zu", analysisInfos_.size());
}

void CloneRestoreHighlight::GetAnalysisRowInfo(AnalysisAlbumInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (intersectionMap_.find("AnalysisAlbum") == intersectionMap_.end()) {
        intersectionMap_.insert(std::make_pair("AnalysisAlbum", GetCommonColumns("AnalysisAlbum")));
    }
    std::unordered_set<std::string>& intersection = intersectionMap_["AnalysisAlbum"];
    GetIfInIntersection("album_id", info.albumIdOld, intersection, resultSet);
    GetIfInIntersection("album_type", info.albumType, intersection, resultSet);
    GetIfInIntersection("album_subtype", info.albumSubtype, intersection, resultSet);
    GetIfInIntersection("album_name", info.albumName, intersection, resultSet);
    GetIfInIntersection("cover_uri", info.oldCoverUri, intersection, resultSet);
    GetIfInIntersection("date_modified", info.dateModified, intersection, resultSet);
    GetIfInIntersection("rank", info.rank, intersection, resultSet);
    GetIfInIntersection("tag_id", info.tagId, intersection, resultSet);
    GetIfInIntersection("user_operation", info.userOperation, intersection, resultSet);
    GetIfInIntersection("group_tag", info.groupTag, intersection, resultSet);
    GetIfInIntersection("user_display_level", info.userDisplayLevel, intersection, resultSet);
    GetIfInIntersection("is_me", info.isMe, intersection, resultSet);
    GetIfInIntersection("is_removed", info.isRemoved, intersection, resultSet);
    GetIfInIntersection("rename_operation", info.renameOperation, intersection, resultSet);
    GetIfInIntersection("is_local", info.isLocal, intersection, resultSet);
    GetIfInIntersection("is_cover_satisfied", info.isCoverSatisfied, intersection, resultSet);
    GetIfInIntersection("relationship", info.relationship, intersection, resultSet);
    UpdateAlbumCoverUri(info);
}

void CloneRestoreHighlight::UpdateAlbumCoverUri(AnalysisAlbumInfo &info)
{
    std::string fileIdOldStr = MediaFileUtils::GetIdFromUri(info.oldCoverUri.value_or(""));
    CHECK_AND_RETURN(!fileIdOldStr.empty() && MediaLibraryDataManagerUtils::IsNumber(fileIdOldStr));
    int32_t fileIdOld = std::atoi(fileIdOldStr.c_str());
    info.coverUri = GetNewHighlightPhotoUri(fileIdOld);
    MEDIA_INFO_LOG("oldCoverUri: %{public}s, newCoverUri: %{public}s.",
        BackupFileUtils::GarbleFilePath(info.oldCoverUri.value_or(""), DEFAULT_RESTORE_ID).c_str(),
        BackupFileUtils::GarbleFilePath(info.coverUri, DEFAULT_RESTORE_ID).c_str());
}

void CloneRestoreHighlight::InsertIntoAnalysisAlbum()
{
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < analysisInfos_.size(); index++) {
            NativeRdb::ValuesBucket value;
            GetAnalysisInsertValue(value, analysisInfos_[index + offset]);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("AnalysisAlbum", values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into AnalysisAlbum fail, num:" + std::to_string(failNums));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            albumFailedCnt_ += failNums;
        }
        albumSuccessCnt_ += rowNum;
        offset += PAGE_SIZE;
    } while (offset < analysisInfos_.size());
}

void CloneRestoreHighlight::GetAnalysisInsertValue(NativeRdb::ValuesBucket &value, const AnalysisAlbumInfo &info)
{
    std::unordered_set<std::string>& intersection = intersectionMap_["AnalysisAlbum"];
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
    PutIfInIntersection(value, "relationship", info.relationship, intersection);
}

void CloneRestoreHighlight::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values)
{
    const std::string QUERY_SQL = "SELECT map.rowid, map.* FROM AnalysisPhotoMap AS map "
        " INNER JOIN AnalysisAlbum AS a ON map.map_album = a.album_id "
        " WHERE a.album_subtype IN (4104, 4105) AND a.need_restore_highlight = 1 "
        " AND map.rowid > ? ORDER BY map.rowid LIMIT ? ";
    std::vector<NativeRdb::ValueObject> params = { lastIdOfMap_, PAGE_SIZE };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query AnalysisPhotoMap err!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        UpdateMapInsertValuesByAlbumId(values, resultSet);
    }
    resultSet->Close();
}

void CloneRestoreHighlight::UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    lastIdOfMap_ = GetInt32Val("rowid", resultSet);

    std::optional<int32_t> oldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_asset");
    bool exceptCond = oldFileId.has_value() && photoInfoMap_.count(oldFileId.value()) > 0;
    CHECK_AND_RETURN_LOG(exceptCond, "the query oldFileId is invalid!");
    PhotoInfo photoInfo = photoInfoMap_.at(oldFileId.value());

    std::optional<int32_t> oldAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "map_album");
    CHECK_AND_RETURN_LOG(oldAlbumId.has_value(), "the query oldAlbumId is invalid!");
    auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
        [oldAlbumId](const AnalysisAlbumInfo& info) {
            return info.albumIdOld.has_value() && info.albumIdOld.value() == oldAlbumId.value();
        });
    CHECK_AND_RETURN_LOG(it != analysisInfos_.end() && it->albumIdNew.has_value(),
        "not find the needed album info, oldAlbumId: %{public}d", oldAlbumId.value());

    std::optional<int32_t> order = std::nullopt;
    CHECK_AND_EXECUTE(!isMapOrder_,
        order = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "order_position"));
    values.emplace_back(GetMapInsertValue(it->albumIdNew.value(), photoInfo.fileIdNew, order));
    std::lock_guard<mutex> lock(counterMutex_);
    std::string reportAlbumName = std::to_string(it->albumIdNew.value());
    CHECK_AND_EXECUTE(!it->albumName.has_value(), reportAlbumName += "_" + it->albumName.value());
    CHECK_AND_EXECUTE(albumPhotoCounter_.count(reportAlbumName) != 0,
        albumPhotoCounter_[reportAlbumName] = 0);
    albumPhotoCounter_[reportAlbumName]++;
}

void CloneRestoreHighlight::InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
    if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
        int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
            "insert into AnalysisPhotoMap fail, num:" + std::to_string(failNums));
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        mapFailedCnt_ += failNums;
    }
    mapSuccessCnt_ += rowNum;
}

NativeRdb::ValuesBucket CloneRestoreHighlight::GetMapInsertValue(int32_t albumId, int32_t fileId,
    std::optional<int32_t> &order)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    CHECK_AND_EXECUTE(!order.has_value(), value.PutInt("order_position", order.value()));
    return value;
}

int32_t CloneRestoreHighlight::BatchInsertWithRetry(const std::string &tableName,
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

void CloneRestoreHighlight::GetHighlightAlbumInfos()
{
    int32_t idNow = GetMaxAlbumId("tab_highlight_album", "id");
    maxIdOfHighlight_ = idNow;
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string QUERY_SQL = "SELECT * FROM tab_highlight_album WHERE highlight_status > 0 "
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
        CHECK_AND_BREAK_INFO_LOG(resultSet != nullptr, "query resultSql is null.");

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
    } while (rowCount == PAGE_SIZE);
    MEDIA_INFO_LOG("query tab_highlight_album nums: %{public}zu", highlightInfos_.size());
}

void CloneRestoreHighlight::GetHighlightRowInfo(HighlightAlbumInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (intersectionMap_.find("tab_highlight_album") == intersectionMap_.end()) {
        intersectionMap_.insert(std::make_pair("tab_highlight_album", GetCommonColumns("tab_highlight_album")));
    }
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_album"];
    GetIfInIntersection("id", info.highlightIdOld, intersection, resultSet);
    GetIfInIntersection("album_id", info.albumIdOld, intersection, resultSet);
    GetIfInIntersection("ai_album_id", info.aiAlbumIdOld, intersection, resultSet);
    GetIfInIntersection("sub_title", info.subTitle, intersection, resultSet);
    GetIfInIntersection("cluster_type", info.clusterType, intersection, resultSet);
    GetIfInIntersection("cluster_sub_type", info.clusterSubType, intersection, resultSet);
    GetIfInIntersection("cluster_condition", info.clusterCondition, intersection, resultSet);
    GetIfInIntersection("min_date_added", info.minDateAdded, intersection, resultSet);
    GetIfInIntersection("max_date_added", info.maxDateAdded, intersection, resultSet);
    GetIfInIntersection("generate_time", info.generateTime, intersection, resultSet);
    GetIfInIntersection("highlight_version", info.highlightVersion, intersection, resultSet);
    GetIfInIntersection("remarks", info.remarks, intersection, resultSet);
    GetIfInIntersection("highlight_status", info.highlightStatus, intersection, resultSet);
    GetIfInIntersection("insert_pic_count", info.insertPicCount, intersection, resultSet);
    GetIfInIntersection("remove_pic_count", info.removePicCount, intersection, resultSet);
    GetIfInIntersection("share_screenshot_count", info.shareScreenshotCount, intersection, resultSet);
    GetIfInIntersection("share_cover_count", info.shareCoverCount, intersection, resultSet);
    GetIfInIntersection("rename_count", info.renameCount, intersection, resultSet);
    GetIfInIntersection("change_cover_count", info.changeCoverCount, intersection, resultSet);
    GetIfInIntersection("render_viewed_times", info.renderViewedTimes, intersection, resultSet);
    GetIfInIntersection("render_viewed_duration", info.renderViewedDuration, intersection, resultSet);
    GetIfInIntersection("art_layout_viewed_times", info.artLayoutViewedTimes, intersection, resultSet);
    GetIfInIntersection("art_layout_viewed_duration", info.artLayoutViewedDuration, intersection, resultSet);
    GetIfInIntersection("music_edit_count", info.musicEditCount, intersection, resultSet);
    GetIfInIntersection("filter_edit_count", info.filterEditCount, intersection, resultSet);
    GetIfInIntersection("is_muted", info.isMuted, intersection, resultSet);
    GetIfInIntersection("is_favorite", info.isFavorite, intersection, resultSet);
    GetIfInIntersection("theme", info.theme, intersection, resultSet);
    GetIfInIntersection("pin_time", info.pinTime, intersection, resultSet);
    GetIfInIntersection("use_subtitle", info.useSubtitle, intersection, resultSet);
    GetIfInIntersection("highlight_location", info.highlightLocation, intersection, resultSet);
    GetIfInIntersection("notification_time", info.notificationTime, intersection, resultSet);
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
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < highlightInfos_.size(); index++) {
            bool cond = (!highlightInfos_[index + offset].clusterType.has_value() ||
                !highlightInfos_[index + offset].clusterSubType.has_value() ||
                !highlightInfos_[index + offset].clusterCondition.has_value() ||
                !highlightInfos_[index + offset].highlightVersion.has_value());
            CHECK_AND_CONTINUE(!cond);

            NativeRdb::ValuesBucket value;
            GetHighlightInsertValue(value, highlightInfos_[index + offset]);
            PutTempHighlightStatus(value, highlightInfos_[index + offset]);
            values.emplace_back(value);
        }
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_album", values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_album fail, num:" + std::to_string(failNums));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            highlightFailedCnt_ += failNums;
        }
        highlightSuccessCnt_ += rowNum;
        offset += PAGE_SIZE;
    } while (offset < highlightInfos_.size());
}

void CloneRestoreHighlight::GetHighlightInsertValue(NativeRdb::ValuesBucket &value, const HighlightAlbumInfo &info)
{
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_album"];
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
    PutIfInIntersection(value, "use_subtitle", info.useSubtitle, intersection);
    PutIfInIntersection(value, "highlight_location", info.highlightLocation, intersection);
    // 历史时刻克隆到新机 默认已读
    value.PutInt("is_viewed", DEFAULT_IS_VIEWED_VALUE);
    PutIfInIntersection(value, "notification_time", info.notificationTime, intersection);
}

void CloneRestoreHighlight::PutTempHighlightStatus(NativeRdb::ValuesBucket &value, const HighlightAlbumInfo &info)
{
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_album"];
    std::optional<int32_t> status = info.highlightStatus;
    CHECK_AND_EXECUTE(info.highlightStatus.value_or(HIGHLIGHT_STATUS_NOT_PRODUCE) <= 0,
        status = std::make_optional<int32_t>(HIGHLIGHT_STATUS_NOT_PRODUCE));
    PutIfInIntersection(value, "highlight_status", status, intersection);
}

void CloneRestoreHighlight::MoveHighlightCovers()
{
    std::unordered_set<int32_t> hasMovedAlbums;
    for (const auto &info : analysisInfos_) {
        bool cond = (!info.albumIdNew.has_value() || !info.oldCoverUri.has_value() ||
            !info.highlightIdOld.has_value() || !info.highlightIdNew.has_value());
        CHECK_AND_CONTINUE(!cond);
        CHECK_AND_CONTINUE(hasMovedAlbums.count(info.highlightIdOld.value()) <= 0);
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
        CHECK_AND_CONTINUE(MediaFileUtils::IsFileExists(srcPath));
        std::string dstDir = "/storage/media/local/files/highlight/cover/" +
            std::to_string(info.highlightIdNew.value()) + "/" + ratio;
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::CreateDirectory(dstDir), "create %{public}s failed",
            BackupFileUtils::GarbleFilePath(dstDir, sceneCode_, GARBLE_DST_PATH).c_str());

        std::string dstPath = dstDir + "/wordart.png";
        int32_t errCode = BackupFileUtils::MoveFile(srcPath.c_str(), dstPath.c_str(), sceneCode_);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "move file failed, srcPath:%{public}s,"
            " dstPath:%{public}s, errCode:%{public}d",
            BackupFileUtils::GarbleFilePath(srcPath, sceneCode_, garblePath_).c_str(),
            BackupFileUtils::GarbleFilePath(dstPath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
    }
}

void CloneRestoreHighlight::MoveHighlightGround(const AnalysisAlbumInfo &info, const std::string &srcDir)
{
    for (const auto &fileName : HIGHLIGHT_COVER_NAME) {
        std::string groundPath = srcDir + "/full/" + fileName + ".png";
        std::string groundDstDir = "/storage/media/local/files/highlight/cover/" +
            std::to_string(info.highlightIdNew.value()) + "/full";
        CHECK_AND_CONTINUE(MediaFileUtils::IsFileExists(groundPath));
        CHECK_AND_CONTINUE_ERR_LOG(MediaFileUtils::CreateDirectory(groundDstDir), "create %{public}s failed",
            BackupFileUtils::GarbleFilePath(groundDstDir, sceneCode_, GARBLE_DST_PATH).c_str());

        std::string groundDstPath = groundDstDir + "/" + fileName + ".png";
        int32_t errCode = BackupFileUtils::MoveFile(groundPath.c_str(), groundDstPath.c_str(), sceneCode_);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "move file failed, srcPath:%{public}s, dstPath:%{public}s, errCode:%{public}d",
            BackupFileUtils::GarbleFilePath(groundPath, sceneCode_, garblePath_).c_str(),
            BackupFileUtils::GarbleFilePath(groundDstPath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
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
            size_t index = srcFilePath.rfind("/");
            CHECK_AND_CONTINUE(index != std::string::npos);
            std::string subDir = srcFilePath.substr(index);
            MoveHighlightMusic(srcFilePath, dstDir + subDir);
        } else {
            std::string tmpFilePath = srcFilePath;
            std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
            CHECK_AND_CONTINUE_INFO_LOG(!MediaFileUtils::IsFileExists(dstFilePath),
                "dst file already exists, srcPath:%{public}s, dstPath:%{public}s",
                BackupFileUtils::GarbleFilePath(srcFilePath, sceneCode_, garblePath_).c_str(),
                BackupFileUtils::GarbleFilePath(dstFilePath, sceneCode_, GARBLE_DST_PATH).c_str());
            int32_t errCode = BackupFileUtils::MoveFile(srcFilePath.c_str(), dstFilePath.c_str(), sceneCode_);
            CHECK_AND_PRINT_LOG(errCode == E_OK,
                "move file failed, srcPath:%{public}s, dstPath:%{public}s, errCode:%{public}d",
                BackupFileUtils::GarbleFilePath(srcFilePath, sceneCode_, garblePath_).c_str(),
                BackupFileUtils::GarbleFilePath(dstFilePath, sceneCode_, GARBLE_DST_PATH).c_str(), errCode);
        }
    }
    return E_OK;
}

void CloneRestoreHighlight::GetHighlightCoverInfos()
{
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        const std::string QUERY_SQL = "SELECT tab_highlight_cover_info.* FROM tab_highlight_cover_info "
            " INNER JOIN tab_highlight_album AS h ON tab_highlight_cover_info.album_id = h.id "
            " WHERE h.highlight_status > 0 "
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
        CHECK_AND_BREAK_INFO_LOG(resultSet != nullptr, "query resultSql is null.");

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
            CHECK_AND_CONTINUE(albumId.has_value());
            auto itAlbum = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
                [albumId](const HighlightAlbumInfo& hiInfo) {
                    return hiInfo.highlightIdOld.has_value() && hiInfo.highlightIdOld == albumId;
                });
            CHECK_AND_CONTINUE_ERR_LOG(itAlbum != highlightInfos_.end(), "can not find coverinfo in highlight");
            HighlightCoverInfo info;
            info.highlightIdNew = itAlbum->highlightIdNew;
            GetCoverRowInfo(info, resultSet);
            GetCoverGroundSourceInfo(info, resultSet);
            coverInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount == PAGE_SIZE);
    MEDIA_INFO_LOG("query tab_highlight_cover_info nums: %{public}zu", coverInfos_.size());
}

void CloneRestoreHighlight::GetCoverRowInfo(HighlightCoverInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (intersectionMap_.find("tab_highlight_cover_info") == intersectionMap_.end()) {
        intersectionMap_.insert(
            std::make_pair("tab_highlight_cover_info", GetCommonColumns("tab_highlight_cover_info")));
    }
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_cover_info"];
    GetIfInIntersection("ratio", info.ratio, intersection, resultSet);
    GetIfInIntersection("is_covered", info.isCovered, intersection, resultSet);
    GetIfInIntersection("color", info.color, intersection, resultSet);
    GetIfInIntersection("radius", info.radius, intersection, resultSet);
    GetIfInIntersection("saturation", info.saturation, intersection, resultSet);
    GetIfInIntersection("brightness", info.brightness, intersection, resultSet);
    GetIfInIntersection("background_color_type", info.backgroundColorType, intersection, resultSet);
    GetIfInIntersection("shadow_level", info.shadowLevel, intersection, resultSet);
    GetIfInIntersection("title_scale_x", info.scaleX, intersection, resultSet);
    GetIfInIntersection("title_scale_y", info.scaleY, intersection, resultSet);
    GetIfInIntersection("title_rect_width", info.rectWidth, intersection, resultSet);
    GetIfInIntersection("title_rect_height", info.rectHeight, intersection, resultSet);
    GetIfInIntersection("background_scale_x", info.bgrScaleX, intersection, resultSet);
    GetIfInIntersection("background_scale_y", info.bgrScaleY, intersection, resultSet);
    GetIfInIntersection("background_rect_width", info.bgrRectWidth, intersection, resultSet);
    GetIfInIntersection("background_rect_height", info.bgrRectHeight, intersection, resultSet);
    GetIfInIntersection("layout_index", info.layoutIndex, intersection, resultSet);
    GetIfInIntersection("cover_algo_version", info.coverAlgoVer, intersection, resultSet);
    GetIfInIntersection("cover_service_version", info.coverServiceVer, intersection, resultSet);
    GetIfInIntersection("status", info.status, intersection, resultSet);
}

void CloneRestoreHighlight::GetCoverGroundSourceInfo(HighlightCoverInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    bool cond = info.highlightIdNew.has_value() && info.ratio.has_value();
    CHECK_AND_RETURN(cond);
    std::string wordartPath = "/storage/media/local/files/highlight/cover/" +
        std::to_string(info.highlightIdNew.value()) + "/" + info.ratio.value() + "/wordart.png";
    auto wordart = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "wordart");
    bool isWordartNeedUpdate = MediaFileUtils::IsFileExists(wordartPath) &&
        wordart.has_value() && !wordart.value().empty();
    CHECK_AND_EXECUTE(!isWordartNeedUpdate,
        info.wordart = "file://media/highlight/cover/" + std::to_string(info.highlightIdNew.value()) +
        "/" + info.ratio.value() + "/wordart.png?oper=highlight");

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
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < coverInfos_.size(); index++) {
            CHECK_AND_CONTINUE(coverInfos_[index + offset].highlightIdNew.has_value());
            NativeRdb::ValuesBucket value;
            GetCoverInsertValue(value, coverInfos_[index + offset]);
            values.emplace_back(value);
        }

        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_cover_info", values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_cover_info fail, num:" + std::to_string(failNums));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            coverInfoFailedCnt_ += failNums;
        }
        coverInfoSuccessCnt_ += rowNum;
        offset += PAGE_SIZE;
    } while (offset < coverInfos_.size());
}

void CloneRestoreHighlight::GetCoverInsertValue(NativeRdb::ValuesBucket &value, const HighlightCoverInfo &info)
{
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_cover_info"];
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
        const std::string QUERY_SQL = "SELECT tab_highlight_play_info.* FROM tab_highlight_play_info "
            " INNER JOIN tab_highlight_album AS h ON tab_highlight_play_info.album_id = h.id "
            " WHERE h.highlight_status > 0 "
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
        CHECK_AND_BREAK_INFO_LOG(resultSet != nullptr, "query resultSql is null.");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            auto albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
            CHECK_AND_CONTINUE(albumId.has_value());
            auto itAlbum = std::find_if(highlightInfos_.begin(), highlightInfos_.end(),
                [albumId](const HighlightAlbumInfo& hiInfo) {
                    return hiInfo.highlightIdOld.has_value() && hiInfo.highlightIdOld == albumId;
                });
            CHECK_AND_CONTINUE_ERR_LOG(itAlbum != highlightInfos_.end(),
                "can not find playinfo in highlight, albumId: %{public}d", albumId.value());
            HighlightPlayInfo info;
            info.highlightIdNew = itAlbum->highlightIdNew;
            GetPlayRowInfo(info, resultSet);
            playInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount == PAGE_SIZE);
    MEDIA_INFO_LOG("query tab_highlight_play_info nums: %{public}zu", playInfos_.size());
}

void CloneRestoreHighlight::GetPlayRowInfo(HighlightPlayInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (intersectionMap_.find("tab_highlight_play_info") == intersectionMap_.end()) {
        intersectionMap_.insert(
            std::make_pair("tab_highlight_play_info", GetCommonColumns("tab_highlight_play_info")));
    }
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_play_info"];
    GetIfInIntersection("play_info_id", info.playId, intersection, resultSet);
    GetIfInIntersection("music", info.music, intersection, resultSet);
    GetIfInIntersection("filter", info.filter, intersection, resultSet);
    GetIfInIntersection("is_chosen", info.isChosen, intersection, resultSet);
    GetIfInIntersection("play_info_version", info.pInfoVer, intersection, resultSet);
    GetIfInIntersection("highlighting_algo_version", info.hAlgoVer, intersection, resultSet);
    GetIfInIntersection("camera_movement_algo_version", info.cameraAlgoVer, intersection, resultSet);
    GetIfInIntersection("transition_algo_version", info.transAlgoVer, intersection, resultSet);
    GetIfInIntersection("play_service_version", info.playServiceVer, intersection, resultSet);
    GetIfInIntersection("status", info.status, intersection, resultSet);
}

void CloneRestoreHighlight::InsertIntoHighlightPlayInfo()
{
    size_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        for (size_t index = 0; index < PAGE_SIZE && index + offset < playInfos_.size(); index++) {
            CHECK_AND_CONTINUE(playInfos_[index + offset].highlightIdNew.has_value());
            NativeRdb::ValuesBucket value;
            GetPlayInsertValue(value, playInfos_[index + offset]);
            value.PutString("play_info", GetDefaultPlayInfo());
            values.emplace_back(value);
        }

        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_play_info", values, rowNum);
        if (errCode != E_OK || rowNum != static_cast<int64_t>(values.size())) {
            int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                "insert into tab_highlight_play_info fail, num:" + std::to_string(failNums));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
            playInfoFailedCnt_ += failNums;
        }
        playInfoSuccessCnt_ += rowNum;
        offset += PAGE_SIZE;
    } while (offset < playInfos_.size());
}

void CloneRestoreHighlight::GetPlayInsertValue(NativeRdb::ValuesBucket &value, const HighlightPlayInfo &info)
{
    std::unordered_set<std::string>& intersection = intersectionMap_["tab_highlight_play_info"];
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
    std::unordered_set<std::string> result;
    auto comparedColumns = GetValueFromMap(ALBUM_COLUMNS_MAP, tableName);
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        bool cond = (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() &&
            comparedColumns.count(it->first) > 0);
        CHECK_AND_EXECUTE(!cond, result.insert(it->first));
    }
    return result;
}

void CloneRestoreHighlight::ReportCloneRestoreHighlightTask()
{
    ReportRestoreTaskOfTotal();
    ReportRestoreTaskOfAlbumStats();
    ReportRestoreTaskOfAlbumInfo();
}

void CloneRestoreHighlight::ReportRestoreTaskOfTotal()
{
    std::stringstream totalReport;
    totalReport << "isHighlightDirExist: " << isHighlightDirExist_ << ", timeCost: " << restoreTimeCost_ <<
        "; ALBUM: max_id: " << maxIdOfAlbum_ << ", success: " << albumSuccessCnt_ <<
        ", duplicate: " << albumDuplicateCnt_ << ", failed: " << albumFailedCnt_ <<
        "; HIGHLIGHT: max_id: " << maxIdOfHighlight_ << ", success: " << highlightSuccessCnt_ <<
        ", duplicate: " << highlightDuplicateCnt_ << ", failed: " << highlightFailedCnt_ <<
        "; MAP: success: " << mapSuccessCnt_ << ", failed: " << mapFailedCnt_ <<
        "; COVER_INFO: success: " << coverInfoSuccessCnt_ << ", failed: " << coverInfoFailedCnt_ <<
        "; PLAY_INFO: success: " << playInfoSuccessCnt_ << ", failed: " << playInfoFailedCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_RESTORE_HIGHLIGHT_TOTAL", RESTORE_STATUS_SUCCESS, totalReport.str());
}

void CloneRestoreHighlight::ReportRestoreTaskOfAlbumStats()
{
    int32_t maxCnt = 0;
    int32_t totalCnt = 0;
    for (const auto &counter : albumPhotoCounter_) {
        maxCnt = maxCnt > counter.second ? maxCnt : counter.second;
        totalCnt += counter.second;
    }
    double meanCnt = albumPhotoCounter_.size() == 0 ? 0 : (double) totalCnt / albumPhotoCounter_.size();
    std::stringstream albumStatsReport;
    albumStatsReport << "num: " << albumPhotoCounter_.size() << ", max: " << maxCnt << ", mean: " << meanCnt
        << ", total: " << totalCnt;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_RESTORE_HIGHLIGHT_ALBUM_STATS", RESTORE_STATUS_SUCCESS, albumStatsReport.str());
}

void CloneRestoreHighlight::ReportRestoreTaskOfAlbumInfo()
{
    for (const auto &counter : albumPhotoCounter_) {
        std::stringstream albumInfoReport;
        albumInfoReport << "albumName: " << counter.first << ", photo count: " << counter.second;
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
            .ReportInAudit("CLONE_RESTORE_HIGHLIGHT_ALBUM_INFO", RESTORE_STATUS_SUCCESS, albumInfoReport.str());
    }
}

void CloneRestoreHighlight::HighlightDeduplicate(const HighlightAlbumInfo &info)
{
    std::string duplicateAlbumName = "";
    std::unordered_set<int32_t> duplicateAnalysisAlbumIdSet;
    std::vector<NativeRdb::ValueObject> changeIds =
        GetHighlightDuplicateIds(info, duplicateAlbumName, duplicateAnalysisAlbumIdSet);
    UpdateHighlightDuplicateRows(changeIds, duplicateAlbumName);
    DeleteAnalysisDuplicateRows(duplicateAnalysisAlbumIdSet, duplicateAlbumName);
}

std::vector<NativeRdb::ValueObject> CloneRestoreHighlight::GetHighlightDuplicateIds(const HighlightAlbumInfo &info,
    std::string &duplicateAlbumName, std::unordered_set<int32_t> &duplicateAnalysisAlbumIdSet)
{
    std::vector<NativeRdb::ValueObject> changeIds = {};
    CHECK_AND_RETURN_RET((info.clusterType.has_value() && info.clusterSubType.has_value() &&
        info.clusterCondition.has_value() && info.highlightVersion.has_value() && info.albumIdOld.has_value() &&
        info.minDateAdded.has_value() && info.maxDateAdded.has_value()),
        changeIds);
    CHECK_AND_RETURN_RET((!info.highlightStatus.has_value() || info.highlightStatus.value() > 0), changeIds);

    auto it = std::find_if(analysisInfos_.begin(), analysisInfos_.end(),
        [info](const AnalysisAlbumInfo &analysisInfo) {
            return analysisInfo.albumIdOld.has_value() &&
                analysisInfo.albumIdOld.value() == info.albumIdOld.value();
        });
    CHECK_AND_RETURN_RET((it != analysisInfos_.end() && it->albumName.has_value()), changeIds);
    duplicateAlbumName = it->albumName.value();

    const std::string QUERY_SQL = "SELECT t.id, t.album_id, t.ai_album_id "
        "FROM tab_highlight_album AS t INNER JOIN AnalysisAlbum AS a "
        "ON t.album_id = a.album_id WHERE t.cluster_type = ? AND t.cluster_sub_type = ? AND "
        "t.highlight_status <> ? AND (t.cluster_condition = ? OR (t.min_date_added = ? AND t.max_date_added = ?))";
    std::vector<NativeRdb::ValueObject> params = {
        info.clusterType.value(), info.clusterSubType.value(), HIGHLIGHT_STATUS_DELETE, info.clusterCondition.value(),
        info.minDateAdded.value(), info.maxDateAdded.value()
    };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, QUERY_SQL, params);

    CHECK_AND_RETURN_RET((resultSet != nullptr), changeIds);

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return changeIds;
    }

    do {
        std::optional<int32_t> highlightId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "id");
        std::optional<int32_t> albumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "album_id");
        std::optional<int32_t> aiAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "ai_album_id");
        CHECK_AND_CONTINUE(highlightId.has_value());
        changeIds.emplace_back(highlightId.value());
        CHECK_AND_EXECUTE(!albumId.has_value(), duplicateAnalysisAlbumIdSet.insert(albumId.value()));
        CHECK_AND_EXECUTE(!aiAlbumId.has_value(), duplicateAnalysisAlbumIdSet.insert(aiAlbumId.value()));
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return changeIds;
}

void CloneRestoreHighlight::UpdateHighlightDuplicateRows(const std::vector<NativeRdb::ValueObject> &changeIds,
    const std::string &duplicateAlbumName)
{
    CHECK_AND_RETURN(!changeIds.empty());
    int32_t changedRows = 0;
    std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
        make_unique<NativeRdb::AbsRdbPredicates>("tab_highlight_album");
    updatePredicates->In("id", changeIds);
    NativeRdb::ValuesBucket rdbValues;
    rdbValues.PutInt("highlight_status", HIGHLIGHT_STATUS_DELETE);
    BackupDatabaseUtils::Update(mediaLibraryRdb_, changedRows, rdbValues, updatePredicates);
    MEDIA_INFO_LOG("deduplicate highlight album, duplicate album name: %{public}s, duplicate nums: %{public}zu, "
        "update nums: %{public}d", duplicateAlbumName.c_str(), changeIds.size(), changedRows);
    highlightDuplicateCnt_ += changedRows;
}

void CloneRestoreHighlight::DeleteAnalysisDuplicateRows(const std::unordered_set<int32_t> &duplicateAnalysisAlbumIdSet,
    const std::string &duplicateAlbumName)
{
    CHECK_AND_RETURN(!duplicateAnalysisAlbumIdSet.empty());
    int32_t deleteRows = 0;
    std::vector<NativeRdb::ValueObject> duplicateAnalysisAlbumIds(duplicateAnalysisAlbumIdSet.begin(),
        duplicateAnalysisAlbumIdSet.end());
    NativeRdb::AbsRdbPredicates deletePredicates("AnalysisAlbum");
    deletePredicates.In("album_id", duplicateAnalysisAlbumIds);
    BackupDatabaseUtils::Delete(deletePredicates, deleteRows, mediaLibraryRdb_);
    MEDIA_INFO_LOG("delete duplicate analysis album, duplicate album name: %{public}s, duplicate nums: %{public}zu, "
        "delete nums: %{public}d", duplicateAlbumName.c_str(), duplicateAnalysisAlbumIds.size(), deleteRows);
    albumDuplicateCnt_ += deleteRows;
}

void CloneRestoreHighlight::UpdateRestoreTimeCost(int64_t timeCost)
{
    restoreTimeCost_ += timeCost;
}

const std::vector<CloneRestoreHighlight::AnalysisAlbumInfo>& CloneRestoreHighlight::GetAnalysisInfos() const
{
    return analysisInfos_;
}
} // namespace OHOS::Media