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

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "highlight_restore.h"

#include "backup_database_utils.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const int32_t HIGHLIGHT_STATUS_SUCCESS = 1;
const int32_t HIGHLIGHT_STATUS_FAIL = -2;
const int32_t HIGHLIGHT_STATUS_DUPLICATE = -1;
const std::vector<std::string> HIGHLIGHT_RATIO = {
    "1_1", "3_2", "3_4", "microcard", "medium_card", "big_card", "screen_0_ver", "screen_0_hor"
};
const std::unordered_map<std::string, std::string> CLUSTER_SUB_TYPE_MAP = {
    { "AttractionsAlbum", "Old_Attraction" },
    { "LocationRenamed", "Old_AOI" },
    { "FrequencyLocationGrowingCluster", "Old_FrequencyLocation" },
    { "RegionalFoodGrowingCluster", "Old_RegionalFood" },
    { "CatGrowingCluster", "Old_Cat" },
    { "DogGrowingCluster", "Old_Dog" },
    { "PeopleGrowingCluster", "Old_People" },
    { "LifeStageCluster", "Old_LifeStage" },
    { "夜色", "Old_Heaven" },
    { "恰同学少年", "Old_Graduate" },
    { "我们毕业了", "Old_Graduate" },
    { "DEFAULT_DBSCAN", "Old_DBSCAN" },
    { "", "Old_Null" }
};
const std::unordered_map<std::string, std::string> CLUSTER_TYPE_MAP = { { "DBSCANTIME", "TYPE_DBSCAN" } };

void HighlightRestore::Init(int32_t sceneCode, std::string taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    galleryRdb_ = galleryRdb;
    albumPhotoCounter_.clear();
    successCnt_ = 0;
    duplicateCnt_ = 0;
    failCnt_ = 0;
    tracksParseFailCnt_ = 0;
}

void HighlightRestore::RestoreHighlight(const std::string &albumOdid,
    const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    RestoreAlbums(albumOdid);
    RestoreMaps(photoInfoMap);
    UpdateAlbums();
}

void HighlightRestore::RestoreAlbums(const std::string &albumOdid)
{
    GetAlbumInfos(albumOdid);
    InsertIntoAnalysisAlbum();
    UpdateAlbumIds();
    InsertIntoHighlightTables();
}

void HighlightRestore::GetAlbumInfos(const std::string &albumOdid)
{
    const std::string QUERY_SQL = "SELECT story_id, date, name, min_datetaken, max_datetaken, "
        "cover_id, album_type, generatedtime, cluster_type, cluster_sub_type, cluster_condition, "
        "(SELECT COUNT(1) FROM t_story_album_suggestion "
        "WHERE t_story_album_suggestion.story_id = t_story_album.story_id) AS suggestion "
        "FROM t_story_album WHERE COALESCE(name, '') <> '' AND displayable = 1 LIMIT ?, ?";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            HighlightAlbumInfo info;
            info.albumOdid = albumOdid;
            info.albumIdOld = GetInt32Val("story_id", resultSet);
            info.subTitle = GetStringVal("date", resultSet);
            info.albumName = GetStringVal("name", resultSet);
            info.minDateAdded = GetInt64Val("min_datetaken", resultSet);
            info.maxDateAdded = GetInt64Val("max_datetaken", resultSet);
            info.coverId = GetInt32Val("cover_id", resultSet);
            info.generateTime = GetInt64Val("generatedtime", resultSet);
            info.generateTime = info.generateTime != 0 ? info.generateTime : info.maxDateAdded;

            info.clusterType = GetStringVal("cluster_type", resultSet);
            info.clusterSubType = GetStringVal("cluster_sub_type", resultSet);
            info.clusterCondition = GetStringVal("cluster_condition", resultSet);
            TransferClusterInfo(info);
            int32_t suggestion = GetInt32Val("suggestion", resultSet);
            if (suggestion) {
                info.clusterType = info.clusterType + "_suggestion";
            }
            int32_t albumType = GetInt32Val("album_type", resultSet);
            info.clusterSubType = info.clusterSubType + "_" + std::to_string(albumType);
            info.highlightStatus = !HasSameHighlightAlbum(info) ? HIGHLIGHT_STATUS_SUCCESS : HIGHLIGHT_STATUS_DUPLICATE;
            albumInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        offset += PAGE_SIZE;
    } while (rowCount == PAGE_SIZE);
}

bool HighlightRestore::HasSameHighlightAlbum(HighlightAlbumInfo &info)
{
    const std::string QUERY_SQL = "SELECT highlight.id, highlight.album_id, highlight.ai_album_id FROM "
        "tab_highlight_album highlight "
        "LEFT JOIN AnalysisAlbum album ON highlight.album_id = album.album_id "
        "WHERE highlight.cluster_type = ? AND highlight.cluster_sub_type = ? AND highlight.highlight_status = 1 AND "
        "(highlight.cluster_condition = ? OR (highlight.min_date_added = ? AND highlight.max_date_added = ?))";
    std::vector<NativeRdb::ValueObject> params = {
        info.clusterType, info.clusterSubType, info.clusterCondition, info.minDateAdded, info.maxDateAdded
    };
    std::shared_ptr<NativeRdb::ResultSet> resultSet =
        BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, QUERY_SQL, params);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, false, "query highlight album failed.");

    info.id = GetInt32Val("id", resultSet);
    info.albumIdNew = GetInt32Val("album_id", resultSet);
    info.aiAlbumIdNew = GetInt32Val("ai_album_id", resultSet);
    resultSet->Close();
    return true;
}

void HighlightRestore::TransferClusterInfo(HighlightAlbumInfo &info)
{
    if (info.clusterType.empty()) {
        info.clusterType = "TYPE_NULL";
        info.clusterSubType = "Old_Null";
        nlohmann::json jsonObjects;
        jsonObjects.push_back({
            { "start", std::to_string(info.minDateAdded) }, { "end", std::to_string(info.maxDateAdded) }
        });
        info.clusterCondition = jsonObjects.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        return;
    }

    info.clusterType = CLUSTER_TYPE_MAP.count(info.clusterType) > 0 ?
        CLUSTER_TYPE_MAP.at(info.clusterType) : info.clusterType;
    nlohmann::json jsonObjects;
    if (info.clusterType == "TYPE_DBSCAN") {
        CHECK_AND_EXECUTE(!info.clusterSubType.empty(), info.clusterSubType = info.albumName);
        info.clusterSubType = CLUSTER_SUB_TYPE_MAP.count(info.clusterSubType) > 0 ?
            CLUSTER_SUB_TYPE_MAP.at(info.clusterSubType) : CLUSTER_SUB_TYPE_MAP.at("DEFAULT_DBSCAN");
        jsonObjects.push_back({
            { "start", std::to_string(info.minDateAdded) }, { "end", std::to_string(info.maxDateAdded) }
        });
    } else {
        info.clusterSubType = CLUSTER_SUB_TYPE_MAP.count(info.clusterSubType) > 0 ?
            CLUSTER_SUB_TYPE_MAP.at(info.clusterSubType) : "Old_" + info.clusterSubType;
        nlohmann::json jsonObject = nlohmann::json::parse(info.clusterCondition, nullptr, false);
        if (jsonObject.is_discarded() || info.clusterCondition == "null") {
            MEDIA_ERR_LOG("Parse clusterCondition failed, %{public}s", info.ToString().c_str());
            jsonObjects.push_back({
                { "start", std::to_string(info.minDateAdded) }, { "end", std::to_string(info.maxDateAdded) }
            });
            info.clusterCondition = jsonObjects.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
            return;
        }
        if (jsonObject.contains("startDate")) {
            jsonObject["start"] = jsonObject["startDate"];
            jsonObject.erase("startDate");
        }
        if (jsonObject.contains("endDate")) {
            jsonObject["end"] = jsonObject["endDate"];
            jsonObject.erase("endDate");
        }
        jsonObjects.push_back(jsonObject);
    }
    info.clusterCondition = jsonObjects.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

void HighlightRestore::InsertIntoAnalysisAlbum()
{
    for (HighlightAlbumInfo &info : albumInfos_) {
        CHECK_AND_CONTINUE(info.highlightStatus == HIGHLIGHT_STATUS_SUCCESS);
        vector<NativeRdb::ValuesBucket> values;
        const int64_t ROW_NUM = 2;
        values.emplace_back(GetAnalysisAlbumValuesBucket(info, PhotoAlbumSubType::HIGHLIGHT));
        values.emplace_back(GetAnalysisAlbumValuesBucket(info, PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS));
        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("AnalysisAlbum", values, rowNum);
        if (errCode != E_OK || rowNum != ROW_NUM) {
            info.highlightStatus = HIGHLIGHT_STATUS_FAIL;
            MEDIA_ERR_LOG("InsertIntoAnalysisAlbum fail, %{public}s", info.ToString().c_str());
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                info.albumName + " insert into AnalysisAlbum fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
    }
}

NativeRdb::ValuesBucket HighlightRestore::GetAnalysisAlbumValuesBucket(const HighlightAlbumInfo &info, int32_t subType)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("album_type", PhotoAlbumType::SMART);
    value.PutInt("count", 0);
    value.PutInt("album_subtype", subType);
    value.PutString("album_name", info.albumName);
    value.PutLong("date_modified", info.generateTime);
    return value;
}

void HighlightRestore::UpdateAlbumIds()
{
    CHECK_AND_RETURN(!albumInfos_.empty());
    const std::string QUERY_SQL =
        "SELECT album_id, album_subtype, album_name, date_modified FROM AnalysisAlbum "
        "WHERE album_subtype IN (4104, 4105) LIMIT ?, ?";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t albumId = GetInt32Val("album_id", resultSet);
            int32_t albumSubType = GetInt32Val("album_subtype", resultSet);
            std::string albumName = GetStringVal("album_name", resultSet);
            int64_t dateModified = GetInt64Val("date_modified", resultSet);
            auto it = std::find_if(albumInfos_.begin(), albumInfos_.end(),
                [albumName, dateModified](const HighlightAlbumInfo &info) {
                    return info.albumName == albumName && info.generateTime == dateModified;
                });
            CHECK_AND_CONTINUE(it != albumInfos_.end());
            if (albumSubType == PhotoAlbumSubType::HIGHLIGHT) {
                it->albumIdNew = albumId;
            } else {
                it->aiAlbumIdNew = albumId;
            }
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        offset += PAGE_SIZE;
    } while (rowCount == PAGE_SIZE);
}

void HighlightRestore::InsertIntoHighlightTables()
{
    InsertIntoHighlightAlbum();
    UpdateHighlightIds();
    InsertIntoHighlightCoverAndPlayInfo();
}

void HighlightRestore::InsertIntoHighlightAlbum()
{
    const std::string INSERT_ALBUM_SQL =
        "INSERT INTO tab_highlight_album (album_id, ai_album_id, sub_title, min_date_added, max_date_added, "
        "generate_time, cluster_type, cluster_sub_type, cluster_condition, highlight_status, highlight_version) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 2)";
    for (HighlightAlbumInfo &info : albumInfos_) {
        CHECK_AND_CONTINUE(info.highlightStatus == HIGHLIGHT_STATUS_SUCCESS);
        std::vector<NativeRdb::ValueObject> albumBindArgs = {
            info.albumIdNew, info.aiAlbumIdNew, info.subTitle, info.minDateAdded, info.maxDateAdded,
            info.generateTime, info.clusterType, info.clusterSubType, info.clusterCondition, info.highlightStatus
        };
        int errCode = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, INSERT_ALBUM_SQL, albumBindArgs);
        if (errCode != E_OK) {
            info.highlightStatus = HIGHLIGHT_STATUS_FAIL;
            MEDIA_ERR_LOG("InsertIntoHighlightAlbum fail, %{public}s", info.ToString().c_str());
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                info.albumName + " insert into HighlightAlbum fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
    }
}

void HighlightRestore::InsertIntoHighlightCoverAndPlayInfo()
{
    const std::string INSERT_PLAY_INFO_SQL =
        "INSERT INTO tab_highlight_play_info (album_id, play_service_version, status) "
        "VALUES (?, 0, 1)";
    for (HighlightAlbumInfo &info : albumInfos_) {
        CHECK_AND_CONTINUE(info.highlightStatus == HIGHLIGHT_STATUS_SUCCESS);
        vector<NativeRdb::ValuesBucket> coverValues;
        for (const std::string &ratio : HIGHLIGHT_RATIO) {
            NativeRdb::ValuesBucket value;
            value.PutInt("album_id", info.id);
            value.PutString("ratio", ratio);
            value.PutInt("status", 1);
            coverValues.emplace_back(value);
        }

        int64_t rowNum = 0;
        int32_t errCode = BatchInsertWithRetry("tab_highlight_cover_info", coverValues, rowNum);
        if (errCode != E_OK || rowNum != (int64_t) HIGHLIGHT_RATIO.size()) {
            info.highlightStatus = HIGHLIGHT_STATUS_FAIL;
            MEDIA_ERR_LOG("InsertIntoHighlightCover fail, %{public}s", info.ToString().c_str());
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                info.albumName + " insert into HighlightCover fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }

        CHECK_AND_CONTINUE(info.highlightStatus == HIGHLIGHT_STATUS_SUCCESS);
        std::vector<NativeRdb::ValueObject> playInfoBindArgs = { info.id };
        errCode = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, INSERT_PLAY_INFO_SQL, playInfoBindArgs);
        if (errCode != E_OK) {
            info.highlightStatus = HIGHLIGHT_STATUS_FAIL;
            MEDIA_ERR_LOG("InsertIntoHighlightPlayInfo fail, %{public}s", info.ToString().c_str());
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
                info.albumName + " insert into HighlightPlayInfo fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
    }
}

void HighlightRestore::UpdateHighlightIds()
{
    CHECK_AND_RETURN(!albumInfos_.empty());
    const std::string QUERY_SQL = "SELECT album_id, id FROM tab_highlight_album LIMIT ?, ?";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t albumId = GetInt32Val("album_id", resultSet);
            int32_t id = GetInt32Val("id", resultSet);
            auto it = std::find_if(albumInfos_.begin(), albumInfos_.end(),
                [albumId](const HighlightAlbumInfo &info) {
                    return info.albumIdNew == albumId;
                });
            CHECK_AND_CONTINUE(it != albumInfos_.end());
            it->id = id;
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        offset += PAGE_SIZE;
    } while (rowCount == PAGE_SIZE);
}

void HighlightRestore::RestoreMaps(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    CHECK_AND_RETURN_INFO_LOG(!albumInfos_.empty(), "albumInfos_ is empty, no need to restore maps.");
    std::string querySql = "SELECT _id, story_id, portrait_id, date_modified, hash FROM gallery_media "
        "WHERE ((story_id IS NOT NULL AND story_id != '') OR (portrait_id IS NOT NULL AND portrait_id != '')) "
        "AND story_chosen != 0 AND _id > ? ORDER BY _id ASC LIMIT ?;";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, querySql, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            HighlightPhotoInfo highlightPhoto;
            highlightPhoto.fileIdOld = GetInt32Val("_id", resultSet);
            offset = highlightPhoto.fileIdOld;
            CHECK_AND_CONTINUE(photoInfoMap.find(highlightPhoto.fileIdOld) != photoInfoMap.end());
            highlightPhoto.photoInfo = photoInfoMap.at(highlightPhoto.fileIdOld);
            highlightPhoto.storyIds = GetStringVal("story_id", resultSet);
            highlightPhoto.portraitIds = GetStringVal("portrait_id", resultSet);
            highlightPhoto.hashCode = GetStringVal("hash", resultSet);
            highlightPhoto.dateModified = GetInt64Val("date_modified", resultSet);
            UpdateMapInsertValues(values, highlightPhoto);
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        int64_t rowNum = 0;
        int errCode = BatchInsertWithRetry("AnalysisPhotoMap", values, rowNum);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("RestoreMaps fail.");
            ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode), "RestoreMaps fail.");
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
    } while (rowCount == PAGE_SIZE);
}

void HighlightRestore::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
    const HighlightPhotoInfo &highlightPhoto)
{
    CHECK_AND_RETURN(highlightPhoto.photoInfo.fileIdNew > 0);
    UpdateAlbumInfoCoverUris(highlightPhoto);

    std::stringstream storyIdss(highlightPhoto.storyIds);
    std::string storyId;
    while (std::getline(storyIdss, storyId, ',')) {
        UpdateMapInsertValuesByStoryId(values, highlightPhoto, storyId);
    }

    std::stringstream portraitIdss(highlightPhoto.portraitIds);
    std::string portraitId;
    while (std::getline(portraitIdss, portraitId, ',')) {
        UpdateMapInsertValuesByStoryId(values, highlightPhoto, portraitId);
    }
}

void HighlightRestore::UpdateAlbumInfoCoverUris(const HighlightPhotoInfo &highlightPhoto)
{
    for (auto &info : albumInfos_) {
        if (info.coverId != highlightPhoto.fileIdOld) {
            continue;
        }
        info.coverUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            std::to_string(highlightPhoto.photoInfo.fileIdNew),
            MediaFileUtils::GetExtraUri(highlightPhoto.photoInfo.displayName, highlightPhoto.photoInfo.cloudPath));
        MEDIA_INFO_LOG("album %{public}s get coverUri %{public}s.", info.albumName.c_str(), info.coverUri.c_str());
    }
}

void HighlightRestore::UpdateMapInsertValuesByStoryId(std::vector<NativeRdb::ValuesBucket> &values,
    const HighlightPhotoInfo &highlightPhoto, const std::string &storyId)
{
    CHECK_AND_RETURN(!storyId.empty() && MediaLibraryDataManagerUtils::IsNumber(storyId));
    int32_t albumIdOld = std::atoi(storyId.c_str());
    auto it = std::find_if(albumInfos_.begin(), albumInfos_.end(),
        [albumIdOld](const HighlightAlbumInfo &info) { return info.albumIdOld == albumIdOld; });
    bool cond = (it == albumInfos_.end() || it->albumIdNew <= 0 || it->aiAlbumIdNew <= 0);
    CHECK_AND_RETURN_LOG(!cond, "no such album of albumIdOld: %{public}d", albumIdOld);
    CHECK_AND_EXECUTE(highlightPhoto.photoInfo.fileType != MediaType::MEDIA_TYPE_VIDEO,
        it->effectline.push_back(GetEffectline(highlightPhoto)));

    std::lock_guard<mutex> lock(counterMutex_);
    std::string albumName = std::to_string(it->id) + it->albumName;
    CHECK_AND_EXECUTE(albumPhotoCounter_.count(albumName) != 0, albumPhotoCounter_[albumName] = 0);
    albumPhotoCounter_[albumName]++;
    values.push_back(GetMapInsertValue(it->albumIdNew, highlightPhoto.photoInfo.fileIdNew));
    values.push_back(GetMapInsertValue(it->aiAlbumIdNew, highlightPhoto.photoInfo.fileIdNew));
}

nlohmann::json HighlightRestore::GetEffectline(const HighlightPhotoInfo &highlightPhoto)
{
    nlohmann::json effectline;
    nlohmann::json fileId;
    fileId.emplace_back(highlightPhoto.photoInfo.fileIdNew);
    effectline["fileId"] = fileId;
    nlohmann::json fileUri;
    std::string effectVideoUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
        std::to_string(highlightPhoto.photoInfo.fileIdNew),
        MediaFileUtils::GetExtraUri(highlightPhoto.photoInfo.displayName, highlightPhoto.photoInfo.cloudPath));
    fileUri.emplace_back(effectVideoUri);
    effectline["fileUri"] = fileUri;
    nlohmann::json filedatemodified;
    filedatemodified.emplace_back(highlightPhoto.dateModified);
    effectline["filedatemodified"] = filedatemodified;
    effectline["effectVideoTrack"] = GetEffectVideoTrack(highlightPhoto.hashCode);

    effectline["effect"] = "TYPE_HILIGHT_CLIP";
    effectline["effectVideoUri"] = effectVideoUri;
    effectline["transitionId"] = "";
    effectline["transitionVideoUri"] = "";
    effectline["prefileId"] = nlohmann::json::array();
    effectline["prefileUri"] = nlohmann::json::array();
    effectline["prefiledatemodified"] = nlohmann::json::array();
    return effectline;
}

nlohmann::json HighlightRestore::GetEffectVideoTrack(const std::string &hashCode)
{
    const std::string QUERY_SQL = "SELECT tracks FROM t_video_semantic_analysis "
        "WHERE hash = ? ORDER BY confidence_probability DESC LIMIT 1";
    std::vector<NativeRdb::ValueObject> params = { hashCode };
    auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, QUERY_SQL, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, nlohmann::json::array());

    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return nlohmann::json::array();
    }

    std::string tracks = GetStringVal("tracks", resultSet);
    resultSet->Close();
    nlohmann::json effectVideoTrack = nlohmann::json::parse(tracks, nullptr, false);
    if (effectVideoTrack.is_discarded()) {
        MEDIA_ERR_LOG("EffectVideoTrack parse fail. tracks: %{public}s", tracks.c_str());
        tracksParseFailCnt_++;
        return nlohmann::json::array();
    }
    return effectVideoTrack;
}

NativeRdb::ValuesBucket HighlightRestore::GetMapInsertValue(int32_t albumId, int32_t fileId)
{
    NativeRdb::ValuesBucket value;
    value.PutInt("map_album", albumId);
    value.PutInt("map_asset", fileId);
    return value;
}

int32_t HighlightRestore::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), 0);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]() -> int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.", errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: trans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

void HighlightRestore::UpdateAlbums()
{
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    const std::string UPDATE_ALBUM_SQL = "UPDATE AnalysisAlbum SET "
        "cover_uri = ?, "
        "count = (SELECT count(1) FROM AnalysisPhotoMap AS apm WHERE apm.map_album = AnalysisAlbum.album_id) "
        "WHERE album_id IN (?, ?)";
    const std::string UPDATE_COVER_KEY_SQL = "UPDATE tab_highlight_cover_info SET "
        "cover_key = ?||'_'||ratio||'_'||? "
        "WHERE album_id = ?";
    const std::string UPDATE_PLAY_INFO_SQL = "UPDATE tab_highlight_play_info SET "
        "play_info = ? "
        "WHERE album_id = ?";
    const std::string DELETE_ALBUM_SQL = "DELETE FROM AnalysisAlbum WHERE album_id IN (?, ?)";
    const std::string DELETE_HIGHLIGHT_ALBUM_SQL = "DELETE FROM tab_highlight_album WHERE id = ?";
    const std::string DELETE_HIGHLIGHT_COVER_SQL = "DELETE FROM tab_highlight_cover_info WHERE album_id = ?";
    const std::string DELETE_HIGHLIGHT_PLAY_INFO_SQL = "DELETE FROM tab_highlight_play_info WHERE album_id = ?";

    for (const auto &info : albumInfos_) {
        MEDIA_INFO_LOG("UpdateAlbums %{public}s", info.ToString().c_str());
        if (info.highlightStatus != HIGHLIGHT_STATUS_FAIL) {
            info.highlightStatus == HIGHLIGHT_STATUS_SUCCESS ? successCnt_++ : duplicateCnt_++;
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_ALBUM_SQL,
                { info.coverUri, info.albumIdNew, info.aiAlbumIdNew });
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_COVER_KEY_SQL,
                { info.albumName, info.coverUri, info.id });
            nlohmann::json playInfo;
            nlohmann::json effectline;
            nlohmann::json effectlineArray = nlohmann::json::array();
            for (const auto &e : info.effectline) {
                effectlineArray.emplace_back(e);
            }
            effectline["effectline"] = effectlineArray;
            playInfo["effectline"] = effectline;
            playInfo["beatsInfo"] = nlohmann::json::array();
            playInfo["timeline"] = nlohmann::json::array();
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, UPDATE_PLAY_INFO_SQL,
                { playInfo.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace), info.id });
        } else {
            failCnt_++;
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, DELETE_ALBUM_SQL, { info.albumIdNew, info.aiAlbumIdNew });
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, DELETE_HIGHLIGHT_ALBUM_SQL, { info.id });
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, DELETE_HIGHLIGHT_COVER_SQL, { info.id });
            BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, DELETE_HIGHLIGHT_PLAY_INFO_SQL, { info.id });
        }
    }

    ReportHighlightRestoreTask();
}

void HighlightRestore::ReportHighlightRestoreTask()
{
    int maxCnt = 0;
    int totalCnt = 0;
    for (auto &counter : albumPhotoCounter_) {
        maxCnt = maxCnt > counter.second ? maxCnt : counter.second;
        totalCnt += counter.second;
        MEDIA_INFO_LOG("UpdateMapInsertValues albumName: %{public}s, photo count: %{public}d",
            counter.first.c_str(), counter.second);
        UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
            .Report("Highlight Photo Map", std::to_string(HIGHLIGHT_STATUS_SUCCESS),
            "albumName: " + counter.first + ", photo count: " + std::to_string(counter.second));
    }
    double meanCnt = albumPhotoCounter_.size() == 0 ? 0 : (double) totalCnt / albumPhotoCounter_.size();
    MEDIA_INFO_LOG("Highlight photos max: %{public}d, mean: %{public}f", maxCnt, meanCnt);
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("Highlight Photos", std::to_string(HIGHLIGHT_STATUS_SUCCESS),
        "max: " + std::to_string(maxCnt) + ", mean: " + std::to_string(meanCnt));

    MEDIA_INFO_LOG("Highlight Restore successCnt_: %{public}d, duplicateCnt_: %{public}d, failCnt_: %{public}d",
        successCnt_.load(), duplicateCnt_.load(), failCnt_.load());
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("Highlight Restore", std::to_string(HIGHLIGHT_STATUS_SUCCESS),
        "successCnt_: " + std::to_string(successCnt_) + ", duplicateCnt_: " + std::to_string(duplicateCnt_) +
        ", failCnt_: " + std::to_string(failCnt_));
    MEDIA_ERR_LOG("EffectVideoTrack parse fail. totalCnt: %{public}d", tracksParseFailCnt_.load());
    ErrorInfo errorInfo(RestoreError::PARSE_TRACK_FAILED, 0, "",
        "EffectVideoTrack parse fail. totalCnt: " + std::to_string(tracksParseFailCnt_));
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
}
} // namespace OHOS::Media