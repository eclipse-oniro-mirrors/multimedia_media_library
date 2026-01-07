/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "AnalysisAlbumOperation"

#include "medialibrary_analysis_album_operations.h"

#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_data_manager.h"
#include "media_file_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"
#include "story_album_column.h"
#include "vision_album_column.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"
#include "vision_photo_map_column.h"
#include "medialibrary_rdb_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media {
constexpr int32_t E_INDEX = -1;
constexpr int32_t ALBUM_IS_ME = 1;
constexpr int32_t ALBUM_IS_NOT_ME = 0;
constexpr int32_t ALBUM_IS_REMOVED = 1;
constexpr int32_t SINGLE_FACE = 1;
constexpr int32_t QUERY_GROUP_PHOTO_ALBUM_RELATED_TO_ME = 1;
constexpr int32_t QUERY_GROUP_PHOTO_ALBUM_REMOVED = 1;
constexpr int32_t GROUP_ALBUM_RENAMED = 2;
constexpr int32_t ALBUM_TO_RENAME_FOR_ANALYSIS = 3;
const string GROUP_PHOTO_TAG = "group_photo_tag";
const string GROUP_PHOTO_IS_ME = "group_photo_is_me";
const string GROUP_PHOTO_ALBUM_NAME = "album_name";
static std::mutex updateGroupPhotoAlbumMutex;
const string GROUP_ALBUM_FAVORITE_ORDER_CLAUSE = " CASE WHEN user_display_level = 3 THEN 1 ELSE 2 END ";
const string GROUP_ALBUM_USER_NAME_ORDER_CLAUSE = " CASE WHEN rename_operation = 2 THEN 1 ELSE 2 END ";
const string GROUP_ALBUM_SYSTEM_NAME_ORDER_CLAUSE =
    " CASE WHEN album_name IS NULL OR album_name = '' THEN 2 ELSE 1 END ";
const string TRUE_VALUE = "1";
const string FALSE_VALUE = "0";

static int32_t ExecSqls(const vector<string> &sqls, const shared_ptr<MediaLibraryUnistore> &store)
{
    int32_t err = NativeRdb::E_OK;
    for (const auto &sql : sqls) {
        err = store->ExecuteSql(sql);
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to exec: %{private}s", sql.c_str());
            break;
        }
    }
    return err;
}

static int32_t GetArgsValueByName(const string &valueName, const string &whereClause, const vector<string> &whereArgs)
{
    size_t pos = whereClause.find(valueName);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause is invalid");
        return E_INDEX;
    }
    size_t argsIndex = 0;
    for (size_t i = 0; i < pos; i++) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return E_INDEX;
    }
    return atoi(whereArgs[argsIndex].c_str());
}

static int32_t GetAlbumId(const string &whereClause, const vector<string> &whereArgs)
{
    size_t pos = whereClause.find(PhotoAlbumColumns::ALBUM_ID);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause is invalid");
        return E_INDEX;
    }
    size_t argsIndex = 0;
    for (size_t i = 0; i < pos; i++) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return E_INDEX;
    }
    auto albumId = whereArgs[argsIndex];
    if (MediaLibraryDataManagerUtils::IsNumber(albumId)) {
        return atoi(albumId.c_str());
    }
    return E_INDEX;
}

static int32_t GetIntValueFromResultSet(shared_ptr<NativeRdb::ResultSet> resultSet, const string &column, int &value)
{
    int index = E_INDEX;
    resultSet->GetColumnIndex(column, index);
    CHECK_AND_RETURN_RET(index != E_INDEX, E_HAS_DB_ERROR);
    CHECK_AND_RETURN_RET(resultSet->GetInt(index, value) == NativeRdb::E_OK, E_HAS_DB_ERROR);
    return E_OK;
}

static int32_t GetStringValueFromResultSet(shared_ptr<NativeRdb::ResultSet> resultSet, const string &column,
    string &value)
{
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_HAS_DB_ERROR);
    int index = E_INDEX;
    resultSet->GetColumnIndex(column, index);
    CHECK_AND_RETURN_RET(index != E_INDEX, E_HAS_DB_ERROR);
    CHECK_AND_RETURN_RET(resultSet->GetString(index, value) == NativeRdb::E_OK, E_HAS_DB_ERROR);
    return E_OK;
}

static string GetCoverUri(int32_t fileId, const string &path, const string &displayName)
{
    string fileName;
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != string::npos && path.size() > (lastSlash + 1)) {
        fileName = path.substr(lastSlash + 1);
    }
    string fileTitle = fileName;
    size_t lastDot = fileName.find_last_of('.');
    if (lastDot != string::npos) {
        fileTitle = fileName.substr(0, lastDot);
    }
    return PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId) + "/" + fileTitle + "/" + displayName;
}

inline int32_t GetStringObject(const ValuesBucket &values, const string &key, string &value)
{
    value = "";
    ValueObject valueObject;
    if (values.GetObject(key, valueObject)) {
        valueObject.GetString(value);
    } else {
        return -EINVAL;
    }
    return E_OK;
}

static void NotifyGroupAlbum(const vector<int32_t> &changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (int32_t albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
    }
}

static string BuildUpdateGroupPhotoAlbumSql(const GroupPhotoAlbumInfo &albumInfo)
{
    string withSql;
    string coverUriValueSql;
    string isCoverSatisfiedValueSql;
    if (albumInfo.isCoverSatisfied == static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING)) {
        coverUriValueSql = "'" + albumInfo.candidateUri + "'";
        isCoverSatisfiedValueSql = to_string(static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING));
    } else {
        string oldCoverId = MediaFileUri::GetPhotoId(albumInfo.coverUri);
        if (!oldCoverId.empty() && MediaLibraryDataManagerUtils::IsNumber(oldCoverId)) {
            withSql = "WITH is_cover_exist AS (SELECT 1 FROM " + ANALYSIS_ALBUM_TABLE +
                " INNER JOIN " + ANALYSIS_PHOTO_MAP_TABLE + " ON " +
                MAP_ALBUM + " = " + ALBUM_ID + " AND INSTR('" + albumInfo.tagId + "', " + GROUP_TAG + ") > 0" +
                " INNER JOIN " + PhotoColumn::PHOTOS_TABLE + " ON " +
                MAP_ASSET + " = " + MediaColumn::MEDIA_ID + " AND " +
                MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
                MediaColumn::MEDIA_HIDDEN + " = 0 AND " +
                MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
                MediaColumn::MEDIA_ID + " = " + oldCoverId + ")";
            coverUriValueSql = "CASE (SELECT 1 FROM is_cover_exist) WHEN 1 THEN '" + albumInfo.coverUri +
                "' ELSE '" + albumInfo.candidateUri + "' END";
            isCoverSatisfiedValueSql = "CASE (SELECT 1 FROM is_cover_exist) WHEN 1 THEN " +
                to_string(albumInfo.isCoverSatisfied) + " ELSE " +
                to_string(static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING)) + " END";
        } else {
            coverUriValueSql = "'" + albumInfo.candidateUri + "'";
            isCoverSatisfiedValueSql = to_string(static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING));
        }
    }
    string albumNameArg = "";
    if (!albumInfo.albumName.empty()) {
        albumNameArg = ", " + GROUP_PHOTO_ALBUM_NAME + " = '" + albumInfo.albumName + "'";
    }
    string updateSql = withSql + " UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " +
        PhotoAlbumColumns::ALBUM_COUNT + " = " + to_string(albumInfo.count) + ", " +
        IS_ME + " = " + to_string(albumInfo.isMe) + ", " +
        PhotoAlbumColumns::ALBUM_COVER_URI + " = " + coverUriValueSql + ", " +
        IS_REMOVED + " = " + to_string(albumInfo.isRemoved) + ", " +
        RENAME_OPERATION + " = " + to_string(albumInfo.renameOperation) + ", " +
        IS_COVER_SATISFIED + " = " + isCoverSatisfiedValueSql + albumNameArg +
        " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumInfo.albumId) + ";";
    return updateSql;
}

static void UpdateGroupPhotoAlbumInfo(const vector<GroupPhotoAlbumInfo> &updateAlbums)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Update group photo album info failed. rdbStore is null");

    for (auto album : updateAlbums) {
        string sql = BuildUpdateGroupPhotoAlbumSql(album);
        auto ret = rdbStore->ExecuteSql(sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Update group photo album failed! Error: %{public}d", ret);
    }
}

static string GetGroupPhotoAlbumSql()
{
    string innerJoinAnalysisAlbum = "INNER JOIN " + ANALYSIS_ALBUM_TABLE + " AA ON F." +
        TAG_ID + " = AA." + TAG_ID + " AND " + TOTAL_FACES + " > " + to_string(SINGLE_FACE);
    string innerJoinAnalysisPhotoMap = "INNER JOIN " + ANALYSIS_PHOTO_MAP_TABLE + " ON " +
        MAP_ALBUM + " = AA." + PhotoAlbumColumns::ALBUM_ID + " AND " +
        MAP_ASSET + " = F." + MediaColumn::MEDIA_ID;
    string innerJoinPhotos = "INNER JOIN " + PhotoColumn::PHOTOS_TABLE + " P ON F." +
        MediaColumn::MEDIA_ID + " = P." + MediaColumn::MEDIA_ID + " AND " +
        MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
        MediaColumn::MEDIA_HIDDEN + " = 0 AND " +
        MediaColumn::MEDIA_TIME_PENDING + " = 0";
    string innerSql = "SELECT F." + MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", " +
        TOTAL_FACES + ", " + GROUP_TAG + ", " + IS_ME + " FROM " + VISION_IMAGE_FACE_TABLE + " F " +
        innerJoinAnalysisAlbum + " " + innerJoinAnalysisPhotoMap + " " + innerJoinPhotos +
        " ORDER BY F." + MediaColumn::MEDIA_ID + ", " + GROUP_TAG;
    string groupPhotoTagSql = "SELECT " + MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ", " +
        MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", " + IS_ME +
        ", GROUP_CONCAT(DISTINCT " + GROUP_TAG + ") AS " + GROUP_PHOTO_TAG +
        " FROM (" + innerSql + ") GROUP BY " + MediaColumn::MEDIA_ID +
        " HAVING COUNT(" + GROUP_TAG +") = " + TOTAL_FACES + " AND COUNT(DISTINCT " + GROUP_TAG + ") > " +
        to_string(SINGLE_FACE);
    string leftJoinAnalysisAlbum = "LEFT JOIN " + ANALYSIS_ALBUM_TABLE + " AA ON " +
        GROUP_PHOTO_TAG + " = AA." + TAG_ID + " AND AA." + ALBUM_SUBTYPE + " = " +
        to_string(PhotoAlbumSubType::GROUP_PHOTO);
    string queryGroupPhotoIsMe = "CASE WHEN MAX(GP." + IS_ME + ") = " + to_string(ALBUM_IS_ME) + " THEN " +
        to_string(ALBUM_IS_ME) + " ELSE " + to_string(ALBUM_IS_NOT_ME) + " END AS " + GROUP_PHOTO_IS_ME;
    string fullSql = "SELECT " + GROUP_PHOTO_TAG + ", " + PhotoAlbumColumns::ALBUM_ID + ", " +
        PhotoAlbumColumns::ALBUM_COVER_URI + ", " + IS_COVER_SATISFIED + ", " +
        MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_NAME + ", " +
        GROUP_PHOTO_ALBUM_NAME + ", " + IS_REMOVED + ", " + RENAME_OPERATION + ", " +
        "COUNT (DISTINCT " + MediaColumn::MEDIA_ID + ") AS " + PhotoAlbumColumns::ALBUM_COUNT + ", " +
        queryGroupPhotoIsMe + ", " + "MAX(" + MediaColumn::MEDIA_DATE_ADDED + ") FROM (" + groupPhotoTagSql +
        ") AS GP " + leftJoinAnalysisAlbum + " GROUP BY " + GROUP_PHOTO_TAG + ";";
    return fullSql;
}

static GroupPhotoAlbumInfo AssemblyInfo(shared_ptr<NativeRdb::ResultSet> resultSet)
{
    string tagId = GetStringVal(GROUP_PHOTO_TAG, resultSet);
    int32_t isCoverSatisfied = GetInt32Val(IS_COVER_SATISFIED, resultSet);
    int32_t count = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    int32_t isMe = GetInt32Val(GROUP_PHOTO_IS_ME, resultSet);
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    string coverUri = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet);
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    string candidateUri = GetCoverUri(fileId, path, displayName);
    string albumName = GetStringVal(GROUP_PHOTO_ALBUM_NAME, resultSet);
    int32_t isRemoved = GetInt32Val(IS_REMOVED, resultSet);
    int32_t renameOperation = GetInt32Val(RENAME_OPERATION, resultSet);
    GroupPhotoAlbumInfo info {albumId, tagId, coverUri, isCoverSatisfied, count, fileId, candidateUri, isMe,
        albumName, isRemoved, renameOperation};
    return info;
}

std::string GetUserDisplayLevelClause(const std::string &clause)
{
    size_t pos = clause.find("user_display_level");
    if (pos == string::npos) {
        return "";
    }

    std::string subClause = clause.substr(pos);
    size_t argsIndex = subClause.find('?');
    if (argsIndex == string::npos) {
        return "";
    }

    return subClause.substr(0, argsIndex);
}

int32_t CheckHighlightAttributeValue(const int32_t &key, const std::string &value)
{
    if (key == static_cast<int32_t>(HighlightAlbumChangeAttribute::IS_VIEWED) ||
        key == static_cast<int32_t>(HighlightAlbumChangeAttribute::IS_FAVORITE)) {
        CHECK_AND_RETURN_RET(value == TRUE_VALUE || value == FALSE_VALUE, E_PARAM_CONVERT_FORMAT);
    } else if (key == static_cast<int32_t>(HighlightAlbumChangeAttribute::NOTIFICATION_TIME)) {
        CHECK_AND_RETURN_RET(MediaLibraryDataManagerUtils::IsNumber(value), E_PARAM_CONVERT_FORMAT);
    }
    return E_OK;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(
    MediaLibraryCommand &cmd, const std::vector<std::string> &columns)
{
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    RdbPredicates rdbPredicates(ANALYSIS_ALBUM_TABLE);

    string clause = PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SMART) + " AND " +
            PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(PhotoAlbumSubType::GROUP_PHOTO);
    auto albumId = GetAlbumId(whereClause, whereArgs);
    if (albumId != E_INDEX) {
        clause += " AND " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId);
    }
    if (whereClause.find(IS_ME) != string::npos) {
        int32_t value = GetArgsValueByName(IS_ME, whereClause, whereArgs);
        if (value == QUERY_GROUP_PHOTO_ALBUM_RELATED_TO_ME) {
            clause += " AND " + IS_ME + " = " + to_string(ALBUM_IS_ME);
        }
    }
    if (whereClause.find(IS_REMOVED_EQ) != string::npos) {
        int32_t value = GetArgsValueByName(IS_REMOVED_EQ, whereClause, whereArgs);
        if (value == QUERY_GROUP_PHOTO_ALBUM_REMOVED) {
            clause += " AND " + IS_REMOVED + " = " + to_string(ALBUM_IS_REMOVED);
        }
    } else {
        clause += " AND " + IS_REMOVED + " <> " + to_string(ALBUM_IS_REMOVED) + " OR " + IS_REMOVED + " IS NULL)";
    }
    if (whereClause.find(USER_DISPLAY_LEVEL) != string::npos) {
        std::string userDisplayLevelClause = GetUserDisplayLevelClause(whereClause);
        if (userDisplayLevelClause != "") {
            auto userDisplayLevelVal = GetArgsValueByName(USER_DISPLAY_LEVEL, whereClause, whereArgs);
            clause += " AND " + userDisplayLevelClause + to_string(userDisplayLevelVal);
        }
    }
    int limitVal = cmd.GetAbsRdbPredicates()->GetLimit();
    int offsetVal = cmd.GetAbsRdbPredicates()->GetOffset();
    CHECK_AND_EXECUTE(limitVal < 0, rdbPredicates.Limit(limitVal));
    CHECK_AND_EXECUTE(limitVal < 0 || offsetVal < 0, rdbPredicates.Offset(offsetVal));
    rdbPredicates.SetWhereClause(clause);
    rdbPredicates.OrderByAsc(GROUP_ALBUM_FAVORITE_ORDER_CLAUSE);
    rdbPredicates.OrderByAsc(GROUP_ALBUM_USER_NAME_ORDER_CLAUSE);
    rdbPredicates.OrderByAsc(GROUP_ALBUM_SYSTEM_NAME_ORDER_CLAUSE);
    rdbPredicates.OrderByDesc(COUNT);
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    return resultSet;
}

static int32_t GetMergeAlbumCoverUri(MergeAlbumInfo &updateAlbumInfo, const MergeAlbumInfo &currentAlbum,
    const MergeAlbumInfo &targetAlbum)
{
    if (targetAlbum.isCoverSatisfied == static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING)) {
        return E_ERR;
    }
    if (currentAlbum.isCoverSatisfied == static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING) ||
        currentAlbum.isCoverSatisfied == static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING_EDITE)) {
        updateAlbumInfo.isCoverSatisfied = currentAlbum.isCoverSatisfied;
        updateAlbumInfo.coverUri = currentAlbum.coverUri;
        return E_OK;
    }

    string currentFileId = MediaFileUri::GetPhotoId(currentAlbum.coverUri);
    string targetFileId = MediaFileUri::GetPhotoId(targetAlbum.coverUri);
    bool cond = (currentFileId.empty() || targetFileId.empty());
    CHECK_AND_RETURN_RET(!cond, E_DB_FAIL);

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed query get merge album cover uri");

    string candidateIds;
    if (currentAlbum.isCoverSatisfied == targetAlbum.isCoverSatisfied) {
        candidateIds = currentFileId + ", " + targetFileId;
    } else {
        candidateIds = currentAlbum.isCoverSatisfied != static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING) ?
            currentFileId :
            targetFileId;
    }
    const std::string queryAlbumInfo = "SELECT " + MediaColumn::MEDIA_ID + "," + MediaColumn::MEDIA_TITLE + "," +
        MediaColumn::MEDIA_NAME + ", MAX(" + MediaColumn::MEDIA_DATE_ADDED + ") FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_ID + " IN (" + candidateIds + " )";

    auto resultSet = uniStore->QuerySql(queryAlbumInfo);
    cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "Failed to query merge album cover uri");

    int mergeFileId;
    string mergeTitle;
    string mergeDisplayName;
    CHECK_AND_RETURN_RET_LOG(GetIntValueFromResultSet(resultSet,
        MediaColumn::MEDIA_ID, mergeFileId) == NativeRdb::E_OK,
        E_HAS_DB_ERROR, "Failed to get file id of merge album cover uri.");
   
    CHECK_AND_RETURN_RET_LOG(GetStringValueFromResultSet(resultSet,
        MediaColumn::MEDIA_TITLE, mergeTitle) == NativeRdb::E_OK,
        E_HAS_DB_ERROR, "Failed to get title of merge album cover uri.");
    
    CHECK_AND_RETURN_RET_LOG(GetStringValueFromResultSet(resultSet,
        MediaColumn::MEDIA_NAME, mergeDisplayName) == NativeRdb::E_OK,
        E_HAS_DB_ERROR, "Failed to get display name of merge album cover uri.");
    updateAlbumInfo.coverUri = "file://media/Photo/" + to_string(mergeFileId) + "/" + mergeTitle + "/" +
        mergeDisplayName;
    return E_OK;
}

static int32_t DeleteRepeatRecordInMap(const vector<string> &deleteAlbumIds)
{
    RdbPredicates rdbPredicates(ANALYSIS_PHOTO_MAP_TABLE);
    rdbPredicates.In(MAP_ALBUM, deleteAlbumIds);
    int32_t deleteRow = MediaLibraryRdbStore::Delete(rdbPredicates);
    MEDIA_INFO_LOG("deleted row = %{public}d", deleteRow);
    return deleteRow < 0 ? E_ERR : E_OK;
}

static string GetSqlsForInsertFileIdInAnalysisAlbumMap(const MergeAlbumInfo &updateMap)
{
    string defaultOrderPosition = "-1";
    string sql = "SELECT Distinct map_asset, " + std::to_string(updateMap.albumId) + ", " +
        defaultOrderPosition + " FROM AnalysisPhotoMap WHERE map_album IN (";
    vector<string> deleteAlbumIds = updateMap.repeatedAlbumIds;
    string strDeleteAlbumIds;
    if (deleteAlbumIds.size() == 0) {
        MEDIA_WARN_LOG("There are no duplicate albums that need to be deleted.");
        return "";
    }
    for (size_t i = 0; i < deleteAlbumIds.size(); i++) {
        strDeleteAlbumIds += deleteAlbumIds[i];
        if (i != deleteAlbumIds.size() - 1) {
            strDeleteAlbumIds += ", ";
        }
    }
    sql = sql + strDeleteAlbumIds + ")";
    return sql;
}

static int32_t InsertNewRecordInMap(const shared_ptr<MediaLibraryRdbStore> store, const MergeAlbumInfo &updateMap)
{
    vector<string> insertSqls;
    string insertSql = "INSERT OR REPLACE INTO AnalysisPhotoMap(map_asset, map_album, order_position) ";
    string queryClause = GetSqlsForInsertFileIdInAnalysisAlbumMap(updateMap);
    CHECK_AND_RETURN_RET_LOG(queryClause != "", E_ERR, "fail to construct querySql.");
    insertSql += queryClause;
    insertSqls.push_back(insertSql);
    int ret = ExecSqls(insertSqls, store);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "fail to insert new record in analysisPhotoMap.");
    return E_OK;
}

static int32_t UpdateAnalysisPhotoMapForMergeGroupPhoto(const shared_ptr<MediaLibraryRdbStore> store,
    const std::unordered_map<string, MergeAlbumInfo> updateMaps)
{
    for (const auto& it : updateMaps) {
        int32_t ret = InsertNewRecordInMap(store, it.second);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("failed to insert newRecord");
            continue;
        }
        ret = DeleteRepeatRecordInMap(it.second.repeatedAlbumIds);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("failed to delete repeat record");
            continue;
        }
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(store, {std::to_string(it.second.albumId)});
    }
    return E_OK;
}

static int32_t UpdateForMergeGroupAlbums(const shared_ptr<MediaLibraryRdbStore> store, const vector<string> &deleteId,
    const std::unordered_map<string, MergeAlbumInfo> updateMaps)
{
    for (auto it : deleteId) {
        RdbPredicates rdbPredicates(ANALYSIS_ALBUM_TABLE);
        rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, it);
        MediaLibraryRdbStore::Delete(rdbPredicates);
    }
    vector<string> updateSqls;
    for (auto it : updateMaps) {
        int32_t renameOperation =
            it.second.renameOperation != GROUP_ALBUM_RENAMED ? ALBUM_TO_RENAME_FOR_ANALYSIS : GROUP_ALBUM_RENAMED;
        int32_t isCoverSatisfied =
            (it.second.isCoverSatisfied != static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING) &&
            it.second.isCoverSatisfied != static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING_EDITE)) ?
            static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING) : it.second.isCoverSatisfied;
        string sql = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + TAG_ID + " = '" + it.first + "', " +
            GROUP_TAG + " = '" + it.first + "', " + COVER_URI + " = '" + it.second.coverUri + "', " +
            IS_REMOVED + " = 0, " + IS_COVER_SATISFIED  + " = " + to_string(isCoverSatisfied) + ", " +
            RENAME_OPERATION + " = " + to_string(renameOperation) + ", " + ALBUM_NAME + " = '" +
            it.second.albumName + "' WHERE " + ALBUM_ID + " = " + to_string(it.second.albumId);
        updateSqls.push_back(sql);
    }
    int ret = ExecSqls(updateSqls, store);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "fail to update analysisAlbum");
    ret = UpdateAnalysisPhotoMapForMergeGroupPhoto(store, updateMaps);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "fail to update analysisPhotoMap.");
    return E_OK;
}

static string ReorderTagId(string target, const vector<MergeAlbumInfo> &mergeAlbumInfo)
{
    string reordererTagId;
    vector<string> splitResult;
    CHECK_AND_RETURN_RET(!target.empty(), reordererTagId);
    string pattern = ",";
    string strs = target;
    if (static_cast<int32_t>(target.find_last_of(",")) != static_cast<int32_t>(target.size()) - 1) {
        strs += pattern;
    }
    size_t pos = strs.find(pattern);
    while (pos != strs.npos) {
        string groupTag = strs.substr(0, pos);
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(pattern);
        if (groupTag.compare(mergeAlbumInfo[0].groupTag) != 0 && groupTag.compare(mergeAlbumInfo[1].groupTag) != 0) {
            splitResult.push_back(groupTag);
        }
    }

    string newTagId = mergeAlbumInfo[0].groupTag + "|" + mergeAlbumInfo[1].groupTag;
    splitResult.push_back(newTagId);
    std::sort(splitResult.begin(), splitResult.end());
    for (auto tagId : splitResult) {
        reordererTagId += (tagId + ",");
    }
    if (static_cast<int32_t>(reordererTagId.find(",")) != static_cast<int32_t>(reordererTagId.size()) - 1) {
        reordererTagId = reordererTagId.substr(0, reordererTagId.size() - 1);
    }
    return reordererTagId;
}

int32_t GetMergeAlbumInfo(shared_ptr<NativeRdb::ResultSet> resultSet, MergeAlbumInfo &info)
{
    int isCoverSatisfied = 0;
    bool cond = (GetIntValueFromResultSet(resultSet, ALBUM_ID, info.albumId) != E_OK ||
        GetStringValueFromResultSet(resultSet, TAG_ID, info.tagId) != E_OK ||
        GetStringValueFromResultSet(resultSet, COVER_URI, info.coverUri) != E_OK ||
        GetIntValueFromResultSet(resultSet, IS_COVER_SATISFIED, isCoverSatisfied) != E_OK ||
        GetIntValueFromResultSet(resultSet, RENAME_OPERATION, info.renameOperation) != E_OK ||
        GetStringValueFromResultSet(resultSet, ALBUM_NAME, info.albumName) != E_OK);
    CHECK_AND_RETURN_RET(!cond, E_HAS_DB_ERROR);

    info.isCoverSatisfied = static_cast<uint8_t>(isCoverSatisfied);
    return E_OK;
}

std::string GetQueryTagIdSql(std::string groupTag1, std::string groupTag2)
{
    std::string groupMergeSqlPre = "SELECT " + ALBUM_ID + ", " + COVER_URI + ", " + IS_COVER_SATISFIED + ", "
        + TAG_ID + ", " + RENAME_OPERATION + ", " + ALBUM_NAME + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ALBUM_SUBTYPE + " = " + to_string(GROUP_PHOTO) + " AND (INSTR(" + TAG_ID + ", '";
    std::string queryTagId = groupMergeSqlPre + groupTag1 + "') > 0 OR INSTR(" + TAG_ID + ", '" +
        groupTag2 + "') > 0)";
    std::string albumMergeUserDisplayLevelOrderClause =
        " ORDER BY CASE WHEN user_display_level = -1 THEN 0 WHEN user_display_level = 3 THEN 3 ELSE 1 END DESC";
    queryTagId += albumMergeUserDisplayLevelOrderClause;
    return queryTagId;
}

int32_t MediaLibraryAnalysisAlbumOperations::UpdateMergeGroupAlbumsInfo(const vector<MergeAlbumInfo> &mergeAlbumInfo)
{
    CHECK_AND_RETURN_RET_LOG(mergeAlbumInfo.size() > 1, E_INVALID_VALUES, "mergeAlbumInfo size is not enough.");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL, "UniStore is nullptr! Query album order failed.");
    std::string queryTagId = GetQueryTagIdSql(mergeAlbumInfo[0].groupTag, mergeAlbumInfo[1].groupTag);
    auto resultSet = uniStore->QuerySql(queryTagId);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_HAS_DB_ERROR);

    std::vector<string> deleteId;
    std::unordered_map<string, MergeAlbumInfo> updateMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        MergeAlbumInfo info;
        CHECK_AND_RETURN_RET(GetMergeAlbumInfo(resultSet, info) == E_OK, E_HAS_DB_ERROR);

        string reorderedTagId = ReorderTagId(info.tagId, mergeAlbumInfo);
        auto it = updateMap.find(reorderedTagId);
        if (reorderedTagId.empty()) {
            continue;
        } else if (it == updateMap.end()) {
            updateMap.insert(std::make_pair(reorderedTagId, info));
        } else {
            MergeAlbumInfo newInfo;
            if (it->second.coverUri.empty()) {
                updateMap[reorderedTagId].coverUri = info.coverUri;
                updateMap[reorderedTagId].isCoverSatisfied = info.isCoverSatisfied;
                updateMap[reorderedTagId].repeatedAlbumIds.push_back(std::to_string(info.albumId));
                deleteId.push_back(std::to_string(info.albumId));
                continue;
            } else if (info.coverUri.empty()) {
                deleteId.push_back(std::to_string(info.albumId));
                updateMap[reorderedTagId].repeatedAlbumIds.push_back(std::to_string(info.albumId));
                continue;
            } else if (GetMergeAlbumCoverUri(newInfo, info, it->second) == E_OK) {
                updateMap[reorderedTagId].coverUri = newInfo.coverUri;
                updateMap[reorderedTagId].isCoverSatisfied = newInfo.isCoverSatisfied;
            }
            if (info.renameOperation == GROUP_ALBUM_RENAMED) {
                updateMap[reorderedTagId].albumName = info.albumName;
                updateMap[reorderedTagId].renameOperation = info.renameOperation;
            }
            updateMap[reorderedTagId].repeatedAlbumIds.push_back(std::to_string(info.albumId));
            deleteId.push_back(std::to_string(info.albumId));
        }
    }
    return UpdateForMergeGroupAlbums(uniStore, deleteId, updateMap);
}

int32_t MediaLibraryAnalysisAlbumOperations::SetGroupAlbumName(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL, "uniStore is nullptr! failed update for set album name");

    string albumName;
    int err = GetStringObject(values, ALBUM_NAME, albumName);
    if (err < 0 || albumName.empty()) {
        MEDIA_ERR_LOG("invalid album name");
        return E_INVALID_VALUES;
    }
    std::string updateForSetAlbumName = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + ALBUM_NAME + " = '" + albumName +
        "' , " + RENAME_OPERATION + " = 2 WHERE " + ALBUM_ID + " = " + targetAlbumId;
    vector<string> updateSqls = { updateForSetAlbumName };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyGroupAlbum(changeAlbumIds);
    }
    return err;
}

int32_t MediaLibraryAnalysisAlbumOperations::SetGroupCoverUri(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed update for set album cover uri");

    string coverUri;
    int err = GetStringObject(values, COVER_URI, coverUri);
    if (err < 0 || coverUri.empty()) {
        MEDIA_ERR_LOG("invalid album cover uri");
        return E_INVALID_VALUES;
    }
    std::string updateForSetCoverUri = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + COVER_URI + " = '" + coverUri +
        "', " + IS_COVER_SATISFIED + " = " + to_string(static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING)) +
        " WHERE " + ALBUM_ID + " = " + targetAlbumId;
    vector<string> updateSqls = { updateForSetCoverUri };
    err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyGroupAlbum(changeAlbumIds);
    }
    return err;
}

int32_t MediaLibraryAnalysisAlbumOperations::DismissGroupPhotoAlbum(const ValuesBucket &values,
    const DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, ANALYSIS_ALBUM_TABLE);
    auto whereArgs = rdbPredicates.GetWhereArgs();
    if (whereArgs.size() == 0) {
        MEDIA_ERR_LOG("no target album id");
        return E_INVALID_VALUES;
    }
    string targetAlbumId = whereArgs[0];
    CHECK_AND_RETURN_RET_LOG(!targetAlbumId.empty() && MediaLibraryDataManagerUtils::IsNumber(targetAlbumId),
        E_INVALID_VALUES, "target album id not exists");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_DB_FAIL,
        "uniStore is nullptr! failed update for set album cover uri");

    std::string updateForDeleteGroupAlbum = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + IS_REMOVED + " = 1 WHERE " +
        ALBUM_ID + " = " + targetAlbumId;
    vector<string> updateSqls = { updateForDeleteGroupAlbum };
    int err = ExecSqls(updateSqls, uniStore);
    if (err == E_OK) {
        vector<int32_t> changeAlbumIds = { atoi(targetAlbumId.c_str()) };
        NotifyGroupAlbum(changeAlbumIds);
    }
    return err;
}

int32_t MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(const OperationType &opType,
    const NativeRdb::ValuesBucket &values, const DataShare::DataSharePredicates &predicates)
{
    switch (opType) {
        case OperationType::DISMISS:
            return DismissGroupPhotoAlbum(values, predicates);
        case OperationType::GROUP_ALBUM_NAME:
            return SetGroupAlbumName(values, predicates);
        case OperationType::GROUP_COVER_URI:
            return SetGroupCoverUri(values, predicates);
        default:
            MEDIA_ERR_LOG("Unknown operation type: %{public}d", opType);
            return E_ERR;
    }
}

void MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(int32_t albumId)
{
    const string &querySql = GetGroupPhotoAlbumSql();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr,
        "Update group photo album by id: %{public}d failed, rdbStore is null.", albumId);
    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr,
        "Update group photo album by id: %{public}d failed, query resultSet is null.", albumId);

    vector<GroupPhotoAlbumInfo> updateAlbums;
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t id = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        if (id == albumId) {
            auto info = AssemblyInfo(resultSet);
            updateAlbums.push_back(info);
            break;
        }
    }
    if (updateAlbums.empty() && albumId > 0) {
        GroupPhotoAlbumInfo info;
        info.albumId = albumId;
        info.count = 0;
        updateAlbums.push_back(info);
    }
    UpdateGroupPhotoAlbumInfo(updateAlbums);
}

void MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr,
        "UpdatePortraitAlbumCoverSatisfied failed, fileId: %{public}d, rdbStore is null.", fileId);

    const string coverUriPrefix = "'" + PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId) + "/%'";

    const string updateSql = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + IS_COVER_SATISFIED + " = " +
        IS_COVER_SATISFIED + " | " + to_string(static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING)) + " WHERE " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT)) +
        " AND " + PhotoAlbumColumns::ALBUM_COVER_URI + " LIKE " + coverUriPrefix;

    int32_t ret = rdbStore->ExecuteSql(updateSql);
    CHECK_AND_RETURN_LOG(ret == E_OK, "ExecuteSql error, fileId: %{public}d, ret: %{public}d.", fileId, ret);
}

int32_t MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumPortraitsOrder(MediaLibraryCommand &cmd)
{
    // Build update column and values
    const string orderColumn = RANK;

    auto valueBucket = cmd.GetValueBucket();
    ValueObject orderValue;
    valueBucket.GetObject(orderColumn, orderValue);
    string orderValueString;
    orderValue.GetString(orderValueString);

    // Build update sql
    stringstream updateSql;
    updateSql << "UPDATE " << cmd.GetTableName() << " SET " << orderColumn << " = " << orderValueString
        << " WHERE " << cmd.GetAbsRdbPredicates()->GetWhereClause();

    // Start update
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Get rdbStore fail");
        return E_HAS_DB_ERROR;
    }
    std::string sqlStr = updateSql.str();
    auto args = cmd.GetAbsRdbPredicates()->GetBindArgs();
    int ret = rdbStore->ExecuteSql(sqlStr, args);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Update portraits order failed, error id: %{public}d", ret);
    return ret;
}

int32_t MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdbStore fail");

    stringstream updatePositionSql;
    auto valueBucket = cmd.GetValueBucket();
    const string orderPositionColumn = ORDER_POSITION;
    ValueObject orderPositionValue;
    valueBucket.GetObject(orderPositionColumn, orderPositionValue);
    string orderPositionStr;
    orderPositionValue.GetString(orderPositionStr);

    updatePositionSql << "UPDATE " << cmd.GetTableName() << " SET " << orderPositionColumn << " = " << orderPositionStr
                      << " WHERE " << cmd.GetAbsRdbPredicates()->GetWhereClause();

    std::string sqlStr = updatePositionSql.str();
    auto args = cmd.GetAbsRdbPredicates()->GetBindArgs();

    int ret = rdbStore->ExecuteSql(sqlStr, args);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Update orderPositions failed, error id: %{public}d", ret);

    ValueObject albumIdValue;
    int32_t albumId = -1;
    if (valueBucket.GetObject(ALBUM_ID, albumIdValue)) {
        albumIdValue.GetInt(albumId);
    }
    CHECK_AND_RETURN_RET_LOG(albumId > 0, ret, "Get album_id failed, album_id: %{public}d", albumId);
    std::stringstream updateUserOperationSql;
    const int userOperationSetted = 1;
    updateUserOperationSql << "UPDATE AnalysisAlbum SET user_operation = " << userOperationSetted
                           << " WHERE album_id = " << albumId;
    int updateUserOperationRet = rdbStore->ExecuteSql(updateUserOperationSql.str());
    CHECK_AND_RETURN_RET_LOG(updateUserOperationRet == NativeRdb::E_OK,
        updateUserOperationRet,
        "Update user_operation failed, ret: %{public}d",
        updateUserOperationRet);

    return updateUserOperationRet;
}

int32_t MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(const int32_t &albumId,
    const int32_t &highlightAlbumChangeAttribute, const std::string &value)
{
    MEDIA_INFO_LOG("albumId: %{public}d, highlightAlbumChangeAttribute: %{public}d, value: %{public}s",
        albumId, highlightAlbumChangeAttribute, value.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "get rdbStore failed");
    int32_t result = CheckHighlightAttributeValue(highlightAlbumChangeAttribute, value);
    CHECK_AND_RETURN_RET_LOG(result == E_OK, result, "check value invalid");
    std::string updateColumn;
    std::vector<NativeRdb::ValueObject> params = {};
    switch (static_cast<HighlightAlbumChangeAttribute>(highlightAlbumChangeAttribute)) {
        case HighlightAlbumChangeAttribute::IS_VIEWED:
            updateColumn = HIGHLIGHT_IS_VIEWED;
            break;
        case HighlightAlbumChangeAttribute::NOTIFICATION_TIME:
            updateColumn = HIGHLIGHT_NOTIFICATION_TIME;
            break;
        case HighlightAlbumChangeAttribute::IS_FAVORITE:
            updateColumn = HIGHLIGHT_IS_FAVORITE;
            break;
        default:
            return E_PARAM_CONVERT_FORMAT;
    }
    std::string updateSql = "UPDATE tab_highlight_album SET " + updateColumn + " = ? WHERE album_id = ?;";
    params.push_back(NativeRdb::ValueObject(value));
    params.push_back(NativeRdb::ValueObject(std::to_string(albumId)));
    int32_t ret = rdbStore->ExecuteSql(updateSql, params);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_INNER_FAIL,
        "update highlight album attribute failed");
    return ret;
}
} // namespace OHOS::Media
