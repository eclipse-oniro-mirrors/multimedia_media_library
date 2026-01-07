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

#define MLOG_TAG "MediaLibraryCloneRestoreCVAnalysis"

#include "clone_restore_cv_analysis.h"

#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const std::vector<std::string> EFFECTLINE_ID = { "fileId", "prefileId" };
const std::vector<std::string> EFFECTLINE_URI = { "fileUri", "prefileUri" };
const std::string EFFECTLINE_TYPE_HITCHCOCK = "TYPE_HITCHCOCK";
const std::string EFFECTLINE_TYPE_UAV = "TYPE_UAV";
const std::string EFFECTLINE_TYPE_HILIGHT_SLOW = "TYPE_HILIGHT_SLOW";
const std::string EFFECTLINE_TYPE_HILIGHT_CLIP = "TYPE_HILIGHT_CLIP";
const std::string EFFECTLINE_TYPE_MASK1 = "TYPE_MASK1";
const std::string EFFECTLINE_TYPE_MASK2 = "TYPE_MASK2";
const std::string HIGHLIGHT_ASSET_URI_PREFIX = "file://media/highlight/video/";
const std::string HIGHLIGHT_ASSET_URI_SUFFIX = "?oper=highlight";
const std::string PHOTO_URI_PREFIX = "file://media/Photo/";
const std::string GARBLE_DST_PATH = "/storage/media/local/files";
const std::string RESTORE_STATUS_SUCCESS = "1";

template<typename Key, typename Value>
Value GetValueFromMap(const std::unordered_map<Key, Value> &map, const Key &key, const Value &defaultValue = Value())
{
    auto it = map.find(key);
    CHECK_AND_RETURN_RET(it != map.end(), defaultValue);
    return it->second;
}

void CloneRestoreCVAnalysis::Init(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, const std::string &backupRestoreDir)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    mediaRdb_ = mediaRdb;

    assetPath_ = backupRestoreDir + "/storage/media/local/files/highlight/video/";
    isHighlightVideoDirExist_ = MediaFileUtils::IsDirectory(assetPath_);
    MEDIA_INFO_LOG("/highlight/video/ source dir %{public}s.", isHighlightVideoDirExist_ ? "exist" : "don't exist");
    garblePath_ = backupRestoreDir + GARBLE_DST_PATH;
}

void CloneRestoreCVAnalysis::RestoreAlbums(CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN_LOG(mediaRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr.");
    CHECK_AND_RETURN_LOG(cloneHighlight.IsCloneHighlight(), "clone highlight flag is false.");

    MEDIA_INFO_LOG("restore highlight cv analysis album start.");
    int64_t startRestoreAssetSdTime = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreAssetSdMap(cloneHighlight);
    int64_t startRestoreAlbumAssetTime = MediaFileUtils::UTCTimeMilliSeconds();
    RestoreAlbumAssetMap(cloneHighlight);
    int64_t startUpdatePlayInfoTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> updateHighlightIds;
    UpdateHighlightPlayInfos(cloneHighlight, updateHighlightIds);
    cloneHighlight.UpdateHighlightStatus(updateHighlightIds);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: RestoreAlbums: %{public}" PRId64 ", RestoreAssetSdMap: %{public}" PRId64
        ", RestoreAlbumAssetMap: %{public}" PRId64 ", UpdateHighlightPlayInfos: %{public}" PRId64,
        endTime - startRestoreAssetSdTime, startRestoreAlbumAssetTime - startRestoreAssetSdTime,
        startUpdatePlayInfoTime - startRestoreAlbumAssetTime, endTime - startUpdatePlayInfoTime);
    restoreTimeCost_ += endTime - startRestoreAssetSdTime;
    cloneHighlight.UpdateRestoreTimeCost(restoreTimeCost_);
    ReportCloneRestoreCVAnalysisTask();
}

void CloneRestoreCVAnalysis::RestoreAssetSdMap(CloneRestoreHighlight &cloneHighlight)
{
    const std::string QUERY_SQL = "SELECT * FROM tab_analysis_asset_sd_map LIMIT ?, ?";
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t oldFileId = GetInt32Val("map_asset_source", resultSet);
            int32_t oldAssetId = GetInt32Val("map_asset_destination", resultSet);
            NativeRdb::ValuesBucket value;
            value.PutInt("map_asset_source", cloneHighlight.GetNewHighlightPhotoId(oldFileId));
            value.PutInt("map_asset_destination", cloneHighlight.GetNewHighlightPhotoId(oldAssetId));
            values.emplace_back(value);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
        InsertIntoAssetSdMap(values);
    } while (rowCount == PAGE_SIZE);
}

void CloneRestoreCVAnalysis::RestoreAlbumAssetMap(CloneRestoreHighlight &cloneHighlight)
{
    const std::string QUERY_SQL = "SELECT tab_analysis_album_asset_map.* FROM tab_analysis_album_asset_map "
        " INNER JOIN tab_highlight_album AS h ON tab_analysis_album_asset_map.map_album = h.id "
        " WHERE h.highlight_status > 0 LIMIT ?, ?";
    int32_t rowCount = 0;
    int32_t offset = 0;
    do {
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t oldAlbumId = GetInt32Val("map_album", resultSet);
            int32_t oldAssetId = GetInt32Val("map_asset", resultSet);
            NativeRdb::ValuesBucket value;
            value.PutInt("map_album", cloneHighlight.GetNewHighlightAlbumId(oldAlbumId));
            value.PutInt("map_asset", cloneHighlight.GetNewHighlightPhotoId(oldAssetId));
            values.emplace_back(value);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
        InsertIntoAlbumAssetMap(values);
    } while (rowCount == PAGE_SIZE);
}

void CloneRestoreCVAnalysis::InsertIntoAssetSdMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("tab_analysis_asset_sd_map", values, rowNum);
    assetSdSuccessCnt_ += rowNum;
    CHECK_AND_RETURN(errCode != E_OK || rowNum != static_cast<int64_t>(values.size()));
    int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
    ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
        "insert into sdMap fail, num:" + std::to_string(failNums));
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    assetSdFailedCnt_ += failNums;
}

void CloneRestoreCVAnalysis::InsertIntoAlbumAssetMap(std::vector<NativeRdb::ValuesBucket> &values)
{
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry("tab_analysis_album_asset_map", values, rowNum);
    albumAssetSuccessCnt_ += rowNum;
    CHECK_AND_RETURN(errCode != E_OK || rowNum != static_cast<int64_t>(values.size()));
    int64_t failNums = static_cast<int64_t>(values.size()) - rowNum;
    ErrorInfo errorInfo(RestoreError::INSERT_FAILED, 0, std::to_string(errCode),
        "insert into AssetMap fail, num:" + std::to_string(failNums));
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
    albumAssetFailedCnt_ += failNums;
}

int32_t CloneRestoreCVAnalysis::BatchInsertWithRetry(const std::string &tableName,
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

std::string CloneRestoreCVAnalysis::ParsePlayInfo(const std::string &oldPlayInfo, CloneRestoreHighlight &cloneHighlight)
{
    nlohmann::json newPlayInfo = nlohmann::json::parse(oldPlayInfo, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!newPlayInfo.is_discarded(), cloneHighlight.GetDefaultPlayInfo(),
        "parse json string failed.");
    ParseEffectline(newPlayInfo, cloneHighlight);
    ParseTimeline(newPlayInfo, cloneHighlight);
    return newPlayInfo.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

void CloneRestoreCVAnalysis::ParseEffectline(nlohmann::json &newPlayInfo, CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(newPlayInfo.contains("effectline") && newPlayInfo["effectline"].is_object() &&
        newPlayInfo["effectline"].contains("effectline") && newPlayInfo["effectline"]["effectline"].is_array());
    nlohmann::json &effectlineInfoArray = newPlayInfo["effectline"]["effectline"];
    for (size_t effectlineIndex = 0; effectlineIndex < effectlineInfoArray.size(); effectlineIndex++) {
        ProcessEffectVideoUri(effectlineInfoArray, effectlineIndex, cloneHighlight);
        ProcessTransitionVideoUri(effectlineInfoArray, effectlineIndex, cloneHighlight);
        ParseEffectlineFileData(effectlineInfoArray, effectlineIndex, cloneHighlight);
    }
}

void CloneRestoreCVAnalysis::ProcessEffectVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(effectlineInfoArray.is_array() && effectlineIndex < effectlineInfoArray.size());

    nlohmann::json &effectlineInfo = effectlineInfoArray[effectlineIndex];
    CHECK_AND_RETURN(effectlineInfo.is_object());
    CHECK_AND_RETURN(effectlineInfo.contains("effectVideoUri") && effectlineInfo["effectVideoUri"].is_string());
    std::string oldEffectVideoUri = effectlineInfo["effectVideoUri"];
    if (MediaFileUtils::StartsWith(oldEffectVideoUri, PHOTO_URI_PREFIX)) {
        effectlineInfo["effectVideoUri"] = GetNewPhotoUriByUri(oldEffectVideoUri, cloneHighlight);
    } else if (MediaFileUtils::StartsWith(oldEffectVideoUri, HIGHLIGHT_ASSET_URI_PREFIX)) {
        effectlineInfo["effectVideoUri"] = GetNewEffectVideoUri(oldEffectVideoUri, cloneHighlight);
    }
}

void CloneRestoreCVAnalysis::ProcessTransitionVideoUri(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(effectlineInfoArray.is_array() && effectlineIndex < effectlineInfoArray.size());

    nlohmann::json &effectlineInfo = effectlineInfoArray[effectlineIndex];
    CHECK_AND_RETURN(effectlineInfo.is_object());
    CHECK_AND_RETURN(effectlineInfo.contains("effect") && effectlineInfo["effect"].is_string() &&
        effectlineInfo["effect"] == EFFECTLINE_TYPE_MASK2 &&
        effectlineInfo.contains("transitionVideoUri") && effectlineInfo["transitionVideoUri"].is_string());
    std::string transVideoUri = GetNewTransitionVideoUri(effectlineInfo["transitionVideoUri"], cloneHighlight);
    effectlineInfo["transitionVideoUri"] = transVideoUri;

    CHECK_AND_RETURN(effectlineIndex > 0);
    nlohmann::json &prevEffectlineInfo = effectlineInfoArray[effectlineIndex - 1];
    CHECK_AND_RETURN(prevEffectlineInfo.is_object());
    CHECK_AND_RETURN(prevEffectlineInfo.contains("effect") && prevEffectlineInfo["effect"].is_string() &&
        prevEffectlineInfo["effect"] == EFFECTLINE_TYPE_MASK1 &&
        prevEffectlineInfo.contains("transitionVideoUri") && prevEffectlineInfo["transitionVideoUri"].is_string());
    prevEffectlineInfo["transitionVideoUri"] = transVideoUri;
}

void CloneRestoreCVAnalysis::ParseEffectlineFileData(nlohmann::json &effectlineInfoArray, size_t effectlineIndex,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(effectlineInfoArray.is_array() && effectlineIndex < effectlineInfoArray.size());

    nlohmann::json &effectlineInfo = effectlineInfoArray[effectlineIndex];
    ProcessIds(effectlineInfo, cloneHighlight);
    ProcessUris(effectlineInfo, cloneHighlight);
}

void CloneRestoreCVAnalysis::ProcessIds(nlohmann::json &effectlineInfo, CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(effectlineInfo.is_object());
    for (const auto &effectlineId : EFFECTLINE_ID) {
        CHECK_AND_CONTINUE(effectlineInfo.contains(effectlineId) && effectlineInfo[effectlineId].is_array());
        for (auto &fileId : effectlineInfo[effectlineId]) {
            CHECK_AND_CONTINUE(fileId.is_number());
            fileId = cloneHighlight.GetNewHighlightPhotoId(fileId.get<int32_t>());
        }
    }
}

void CloneRestoreCVAnalysis::ProcessUris(nlohmann::json &effectlineInfo, CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(effectlineInfo.is_object());
    for (const auto &effectlineUri : EFFECTLINE_URI) {
        CHECK_AND_CONTINUE(effectlineInfo.contains(effectlineUri) && effectlineInfo[effectlineUri].is_array());
        for (auto &fileUri : effectlineInfo[effectlineUri]) {
            CHECK_AND_CONTINUE(fileUri.is_string());
            fileUri = GetNewPhotoUriByUri(fileUri.get<std::string>(), cloneHighlight);
        }
    }
}

void CloneRestoreCVAnalysis::ParseTimeline(nlohmann::json &newPlayInfo, CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(newPlayInfo.contains("timeline") && newPlayInfo["timeline"].is_array());
    for (nlohmann::json &timelineInfo : newPlayInfo["timeline"]) {
        ProcessTimelineInfo(timelineInfo, cloneHighlight);
    }
}

void CloneRestoreCVAnalysis::ProcessTimelineInfo(nlohmann::json &timelineInfo, CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN(timelineInfo.is_object());
    if (timelineInfo.contains("effectVideoUri") && timelineInfo["effectVideoUri"].is_string()) {
        std::string oldEffectVideoUri = timelineInfo["effectVideoUri"];
        timelineInfo["effectVideoUri"] = GetValueFromMap(assetUriMap_, oldEffectVideoUri);
    }

    if (timelineInfo.contains("transitionVideoUri") && timelineInfo["transitionVideoUri"].is_string()) {
        std::string oldTransVideoUri = timelineInfo["transitionVideoUri"];
        timelineInfo["transitionVideoUri"] = GetValueFromMap(assetUriMap_, oldTransVideoUri);
    }

    if (timelineInfo.contains("fileId") && timelineInfo["fileId"].is_array()) {
        for (auto &fileId : timelineInfo["fileId"]) {
            CHECK_AND_CONTINUE(fileId.is_number());
            fileId = cloneHighlight.GetNewHighlightPhotoId(fileId.get<int32_t>());
        }
    }

    if (timelineInfo.contains("fileUri") && timelineInfo["fileUri"].is_array()) {
        for (auto &fileUri : timelineInfo["fileUri"]) {
            CHECK_AND_CONTINUE(fileUri.is_string());
            fileUri = GetNewPhotoUriByUri(fileUri.get<std::string>(), cloneHighlight);
        }
    }
}

std::string CloneRestoreCVAnalysis::GetNewEffectVideoUri(const std::string &oldVideoUri,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN_RET(assetUriMap_.find(oldVideoUri) == assetUriMap_.end(), assetUriMap_.at(oldVideoUri));
    CHECK_AND_RETURN_RET(oldVideoUri != "", "");
    size_t rightIndex = oldVideoUri.rfind("/");
    CHECK_AND_RETURN_RET(rightIndex != std::string::npos && rightIndex > 0, "");
    size_t leftIndex = oldVideoUri.rfind("/", rightIndex - 1);
    CHECK_AND_RETURN_RET(leftIndex != std::string::npos && rightIndex > leftIndex + 1, "");
    int32_t oldAssetId = std::atoi((oldVideoUri.substr(leftIndex + 1, rightIndex - leftIndex - 1)).c_str());
    int32_t newAssetId = cloneHighlight.GetNewHighlightPhotoId(oldAssetId);

    size_t suffixLeftIndex = oldVideoUri.find("_", rightIndex);
    CHECK_AND_RETURN_RET(suffixLeftIndex != std::string::npos, "");
    size_t suffixRightIndex = oldVideoUri.find("?", suffixLeftIndex);
    CHECK_AND_RETURN_RET(suffixRightIndex != std::string::npos && suffixRightIndex > suffixLeftIndex, "");
    std::string suffix = oldVideoUri.substr(suffixLeftIndex, suffixRightIndex - suffixLeftIndex);
    std::string newVideoUri = HIGHLIGHT_ASSET_URI_PREFIX + std::to_string(newAssetId)
        + "/" + std::to_string(newAssetId) + suffix + HIGHLIGHT_ASSET_URI_SUFFIX;

    assetUriMap_.insert(std::make_pair(oldVideoUri, newVideoUri));
    std::string dstPath = "/storage/media/local/files/highlight/video/" + std::to_string(newAssetId) + "/" +
        std::to_string(newAssetId) + suffix;
    std::string srcPath = assetPath_ + std::to_string(oldAssetId) + "/" + std::to_string(oldAssetId) + suffix;
    MoveAnalysisAssets(srcPath, dstPath);
    return newVideoUri;
}

std::string CloneRestoreCVAnalysis::GetNewTransitionVideoUri(const std::string &oldVideoUri,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN_RET(assetUriMap_.find(oldVideoUri) == assetUriMap_.end(), assetUriMap_.at(oldVideoUri));
    CHECK_AND_RETURN_RET(oldVideoUri != "", "");
    size_t rightIndex = oldVideoUri.rfind("/");
    CHECK_AND_RETURN_RET(rightIndex != std::string::npos && rightIndex > 0, "");
    size_t leftIndex = oldVideoUri.rfind("/", rightIndex - 1);
    CHECK_AND_RETURN_RET(leftIndex != std::string::npos && rightIndex > leftIndex + 1, "");
    int32_t oldAssetId = std::atoi((oldVideoUri.substr(leftIndex + 1, rightIndex - leftIndex - 1)).c_str());
    int32_t newAssetId = cloneHighlight.GetNewHighlightPhotoId(oldAssetId);

    size_t secondLeftIndex = oldVideoUri.find("_", rightIndex);
    CHECK_AND_RETURN_RET(secondLeftIndex != std::string::npos, "");
    size_t secondRightIndex = oldVideoUri.find("_", secondLeftIndex + 1);
    CHECK_AND_RETURN_RET(secondRightIndex != std::string::npos && secondRightIndex > secondLeftIndex + 1, "");
    int32_t oldNextAssetId = std::atoi((oldVideoUri.substr(secondLeftIndex + 1,
        secondRightIndex - secondLeftIndex - 1)).c_str());
    int32_t newNextPhotoId = cloneHighlight.GetNewHighlightPhotoId(oldNextAssetId);

    size_t suffixRightIndex = oldVideoUri.find("?", secondRightIndex);
    CHECK_AND_RETURN_RET(suffixRightIndex != std::string::npos && suffixRightIndex > secondRightIndex, "");
    std::string suffix = oldVideoUri.substr(secondRightIndex, suffixRightIndex - secondRightIndex);

    std::string newVideoUri = HIGHLIGHT_ASSET_URI_PREFIX + std::to_string(newAssetId) + "/" +
        std::to_string(newAssetId) + "_" + std::to_string(newNextPhotoId) + suffix + HIGHLIGHT_ASSET_URI_SUFFIX;
    assetUriMap_.insert(std::make_pair(oldVideoUri, newVideoUri));

    std::string dstPath = "/storage/media/local/files/highlight/video/" + std::to_string(newAssetId) + "/" +
        std::to_string(newAssetId) + "_" + std::to_string(newNextPhotoId) + suffix;
    std::string srcPath = assetPath_ + std::to_string(oldAssetId) + "/" +
        std::to_string(oldAssetId) + "_" + std::to_string(oldNextAssetId) + suffix;
    MoveAnalysisAssets(srcPath, dstPath);
    return newVideoUri;
}

std::string CloneRestoreCVAnalysis::GetNewPhotoUriByUri(const std::string &oldUri,
    CloneRestoreHighlight &cloneHighlight)
{
    CHECK_AND_RETURN_RET(assetUriMap_.find(oldUri) == assetUriMap_.end(), assetUriMap_.at(oldUri));
    CHECK_AND_RETURN_RET(oldUri != "", "");
    size_t rightIndex = oldUri.rfind("/");
    CHECK_AND_RETURN_RET(rightIndex != std::string::npos && rightIndex > 0, "");
    size_t suffixIndex = oldUri.find("?", rightIndex);
    rightIndex = oldUri.rfind("/", rightIndex - 1);
    CHECK_AND_RETURN_RET(rightIndex != std::string::npos && rightIndex > 0, "");
    size_t leftIndex = oldUri.rfind("/", rightIndex - 1);
    CHECK_AND_RETURN_RET(leftIndex != std::string::npos && rightIndex > leftIndex + 1, "");

    int32_t oldPhotoId = std::atoi((oldUri.substr(leftIndex + 1, rightIndex - leftIndex - 1)).c_str());
    std::string newUri = cloneHighlight.GetNewHighlightPhotoUri(oldPhotoId);

    CHECK_AND_EXECUTE(suffixIndex == std::string::npos, newUri += oldUri.substr(suffixIndex));
    assetUriMap_.insert(std::make_pair(oldUri, newUri));
    return newUri;
}

void CloneRestoreCVAnalysis::MoveAnalysisAssets(const std::string &srcPath, const std::string &dstPath)
{
    std::string srcGarble = srcPath;
    srcGarble.replace(0, assetPath_.length(), "*/");
    std::string dstGarble = MediaFileUtils::DesensitizePath(dstPath);
    CHECK_AND_RETURN_LOG(MediaFileUtils::IsFileExists(srcPath), "src file doesn't exist, srcPath:%{public}s."
        "dstPath:%{public}s", srcGarble.c_str(), dstGarble.c_str());
    CHECK_AND_RETURN_INFO_LOG(!MediaFileUtils::IsFileExists(dstPath),
        "dst file already exists, srcPath:%{public}s, dstPath:%{public}s.", srcGarble.c_str(), dstGarble.c_str());
    int32_t errCode = BackupFileUtils::PreparePath(dstPath);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "PreparePath failed, can't move file, srcPath:%{public}s, "
        "dstPath:%{public}s, errcode:%{public}d", srcGarble.c_str(), dstGarble.c_str(), errCode);
    errCode = BackupFileUtils::MoveFile(srcPath, dstPath, sceneCode_);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "move file failed, srcPath:%{public}s, dstPath:%{public}s, "
        "errcode:%{public}d", srcGarble.c_str(), dstGarble.c_str(), errCode);
    MEDIA_INFO_LOG("move file successed, srcPath:%{public}s, dstPath:%{public}s", srcGarble.c_str(), dstGarble.c_str());
}

void CloneRestoreCVAnalysis::UpdateHighlightPlayInfos(CloneRestoreHighlight &cloneHighlight,
    std::vector<int32_t> &updateHighlightIds)
{
    int32_t rowCount = 0;
    int32_t offset = 0;
    std::string defaultPlayInfo = cloneHighlight.GetDefaultPlayInfo();
    do {
        const std::string QUERY_SQL = "SELECT p.album_id, p.play_info_id, p.play_info FROM tab_highlight_play_info AS p"
            " INNER JOIN tab_highlight_album AS h ON p.album_id = h.id WHERE h.highlight_status > 0 "
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(PAGE_SIZE);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaRdb_, QUERY_SQL);
        CHECK_AND_RETURN_INFO_LOG(resultSet != nullptr, "query resultSql is null.");

        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::optional<int32_t> oldAlbumId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "p.album_id");
            CHECK_AND_CONTINUE(oldAlbumId.has_value());
            std::optional<int32_t> playId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, "p.play_info_id");
            std::optional<std::string> oldPlayInfo =
                BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, "p.play_info");
            std::string newPlayInfo = defaultPlayInfo;
            CHECK_AND_EXECUTE(!oldPlayInfo.has_value(),
                newPlayInfo = ParsePlayInfo(oldPlayInfo.value(), cloneHighlight));

            int32_t albumId = cloneHighlight.GetNewHighlightAlbumId(oldAlbumId.value());
            std::string updatePlayInfoSql = "UPDATE tab_highlight_play_info SET play_info = ? WHERE album_id = ? ";
            int32_t ret = E_ERR;
            if (playId.has_value()) {
                std::string playInfoId = std::to_string(playId.value());
                updatePlayInfoSql += "AND play_info_id = ?";
                ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updatePlayInfoSql,
                    { newPlayInfo, albumId, playInfoId });
            } else {
                updatePlayInfoSql += "AND play_info_id ISNULL";
                ret = BackupDatabaseUtils::ExecuteSQL(mediaLibraryRdb_, updatePlayInfoSql,
                    { newPlayInfo, albumId });
            }
            bool successfulParseCond = (newPlayInfo != defaultPlayInfo && ret == E_OK) ||
                (newPlayInfo == defaultPlayInfo && oldPlayInfo == defaultPlayInfo);
            if (successfulParseCond) {
                updateHighlightIds.emplace_back(albumId);
                continue;
            }
            CHECK_AND_PRINT_LOG(ret == E_OK, "Update play_info Sql err, highlight id: %{public}d, errCode: %{public}d",
                albumId, ret);
            ErrorInfo errorInfo(RestoreError::UPDATE_FAILED, 0, std::to_string(ret),
                "Update play_info failed. highlight id:" + std::to_string(albumId));
            UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_).ReportError(errorInfo);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount == PAGE_SIZE);
}

void CloneRestoreCVAnalysis::ReportCloneRestoreCVAnalysisTask()
{
    std::stringstream totalReport;
    totalReport << "isHighlightVideoDirExist: " << isHighlightVideoDirExist_ << ", timeCost: " << restoreTimeCost_ <<
        "; ASSET_SD: success: " << assetSdSuccessCnt_ << ", failed: " << assetSdFailedCnt_ <<
        "; ALBUM_ASSET: success: " << albumAssetSuccessCnt_ << ", failed: " << albumAssetFailedCnt_;
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("CLONE_RESTORE_CV_ANALYSIS_TOTAL", RESTORE_STATUS_SUCCESS, totalReport.str());
}
} // namespace OHOS::Media