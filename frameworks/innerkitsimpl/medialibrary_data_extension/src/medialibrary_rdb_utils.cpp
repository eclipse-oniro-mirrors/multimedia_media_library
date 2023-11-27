/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_rdb_utils.h"

#include <iomanip>
#include <sstream>
#include <string>

#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "result_set.h"
#include "userfile_manager_types.h"
#include "vision_column.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

constexpr int32_t E_HAS_DB_ERROR = -222;
constexpr int32_t E_SUCCESS = 0;

static inline string GetStringValFromColumn(const shared_ptr<ResultSet> &resultSet, const int index)
{
    string value;
    if (resultSet->GetString(index, value)) {
        return "";
    }
    return value;
}

static inline int32_t GetIntValFromColumn(const shared_ptr<ResultSet> &resultSet, const int index)
{
    int32_t value = 0;
    if (resultSet->GetInt(index, value)) {
        return 0;
    }
    return value;
}

static inline string GetStringValFromColumn(const shared_ptr<ResultSet> &resultSet, const string &columnName)
{
    int32_t index = 0;
    if (resultSet->GetColumnIndex(columnName, index)) {
        return "";
    }

    return GetStringValFromColumn(resultSet, index);
}

static inline int32_t GetIntValFromColumn(const shared_ptr<ResultSet> &resultSet, const string &columnName)
{
    int32_t index = 0;
    if (resultSet->GetColumnIndex(columnName, index)) {
        return 0;
    }

    return GetIntValFromColumn(resultSet, index);
}

static inline shared_ptr<ResultSet> GetUserAlbum(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const vector<string> &userAlbumIds, const vector<string> &columns)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (userAlbumIds.empty()) {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    } else {
        predicates.In(PhotoAlbumColumns::ALBUM_ID, userAlbumIds);
    }
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(predicates, columns);
}

static inline shared_ptr<ResultSet> GetAnalysisAlbum(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const vector<string> &analysisAlbumIds, const vector<string> &columns)
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    if (!analysisAlbumIds.empty()) {
        predicates.In(PhotoAlbumColumns::ALBUM_ID, analysisAlbumIds);
    }
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(predicates, columns);
}

static string GetQueryFilter(const string &tableName)
{
    if (tableName == MEDIALIBRARY_TABLE) {
        return MEDIALIBRARY_TABLE + "." + MEDIA_DATA_DB_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    }
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        return PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_SYNC_STATUS + " = " +
            to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
            PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
            to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    }
    if (tableName == PhotoAlbumColumns::TABLE) {
        return PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_DIRTY + " != " +
            to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
    }
    if (tableName == PhotoMap::TABLE) {
        return PhotoMap::TABLE + "." + PhotoMap::DIRTY + " != " + to_string(static_cast<int32_t>(
            DirtyTypes::TYPE_DELETED));
    }
    return "";
}

void MediaLibraryRdbUtils::AddQueryFilter(AbsRdbPredicates &predicates)
{
    /* build all-table vector */
    string tableName = predicates.GetTableName();
    vector<string> joinTables = predicates.GetJoinTableNames();
    joinTables.push_back(tableName);
    /* add filters */
    string filters;
    for (auto &t : joinTables) {
        string filter = GetQueryFilter(t);
        if (filter.empty()) {
            continue;
        }
        if (filters.empty()) {
            filters += filter;
        } else {
            filters += " AND " + filter;
        }
    }
    if (filters.empty()) {
        return;
    }

    /* rebuild */
    string queryCondition = predicates.GetWhereClause();
    queryCondition = queryCondition.empty() ? filters : filters + " AND " + queryCondition;
    predicates.SetWhereClause(queryCondition);
}

static shared_ptr<AbsSharedResultSet> Query(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const AbsRdbPredicates &predicates, const vector<string> &columns)
{
    MediaLibraryRdbUtils::AddQueryFilter(const_cast<AbsRdbPredicates &>(predicates));
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(predicates, columns);
}

static shared_ptr<ResultSet> QueryGoToFirst(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const RdbPredicates &predicates, const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryGoToFirst");
    auto resultSet = Query(rdbStore, predicates, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }

    MediaLibraryTracer goToFirst;
    goToFirst.Start("GoToFirstRow");
    resultSet->GoToFirstRow();
    return resultSet;
}

static int32_t ForEachRow(const shared_ptr<RdbStore> &rdbStore, const shared_ptr<ResultSet> &resultSet,
    const bool hiddenState, const function<int32_t(
        const shared_ptr<RdbStore> &rdbStore, const shared_ptr<ResultSet> &albumResult, const bool hiddenState)> &func)
{
    TransactionOperations transactionOprn(rdbStore);
    int32_t err = transactionOprn.Start();
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to begin transaction, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        // Ignore failure here, try to iterate rows as much as possible.
        func(rdbStore, resultSet, hiddenState);
    }
    transactionOprn.Finish();
    return E_SUCCESS;
}

static inline int32_t GetFileCount(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, MEDIA_COLUMN_COUNT_1);
}

static inline int32_t GetAlbumCount(const shared_ptr<ResultSet> &resultSet, const string &column)
{
    return GetIntValFromColumn(resultSet, column);
}

static inline string GetAlbumCover(const shared_ptr<ResultSet> &resultSet, const string &column)
{
    return GetStringValFromColumn(resultSet, column);
}

static inline int32_t GetAlbumId(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, PhotoAlbumColumns::ALBUM_ID);
}

static inline int32_t GetAlbumSubType(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, PhotoAlbumColumns::ALBUM_SUBTYPE);
}

static string GetFileName(const string &filePath)
{
    string fileName;

    size_t lastSlash = filePath.rfind('/');
    if (lastSlash == string::npos) {
        return fileName;
    }
    if (filePath.size() > (lastSlash + 1)) {
        fileName = filePath.substr(lastSlash + 1);
    }
    return fileName;
}

static string GetTitleFromDisplayName(const string &displayName)
{
    auto pos = displayName.find_last_of('.');
    if (pos == string::npos) {
        return "";
    }
    return displayName.substr(0, pos);
}

static string Encode(const string &uri)
{
    const unordered_set<char> uriCompentsSet = {
        ';', ',', '/', '?', ':', '@', '&',
        '=', '+', '$', '-', '_', '.', '!',
        '~', '*', '(', ')', '#', '\''
    };
    constexpr int32_t encodeLen = 2;
    ostringstream outPutStream;
    outPutStream.fill('0');
    outPutStream << std::hex;

    for (unsigned char tmpChar : uri) {
        if (std::isalnum(tmpChar) || uriCompentsSet.find(tmpChar) != uriCompentsSet.end()) {
            outPutStream << tmpChar;
        } else {
            outPutStream << std::uppercase;
            outPutStream << '%' << std::setw(encodeLen) << static_cast<unsigned int>(tmpChar);
            outPutStream << std::nouppercase;
        }
    }

    return outPutStream.str();
}

static string GetExtraUri(const string &displayName, const string &path)
{
    string extraUri = "/" + GetTitleFromDisplayName(GetFileName(path)) + "/" + displayName;
    return Encode(extraUri);
}

static string GetUriByExtrConditions(const string &prefix, const string &fileId, const string &suffix)
{
    return prefix + fileId + suffix;
}

static inline string GetCover(const shared_ptr<ResultSet> &resultSet)
{
    string coverUri;
    int32_t fileId = GetIntValFromColumn(resultSet, PhotoColumn::MEDIA_ID);
    if (fileId <= 0) {
        return coverUri;
    }

    string extrUri = GetExtraUri(GetStringValFromColumn(resultSet, PhotoColumn::MEDIA_NAME),
        GetStringValFromColumn(resultSet, PhotoColumn::MEDIA_FILE_PATH));
    return GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId), extrUri);
}

static int32_t SetCount(const shared_ptr<ResultSet> &fileResult, const shared_ptr<ResultSet> &albumResult,
    ValuesBucket &values, const bool hiddenState)
{
    const string &targetColumn = hiddenState ? PhotoAlbumColumns::HIDDEN_COUNT : PhotoAlbumColumns::ALBUM_COUNT;
    int32_t oldCount = GetAlbumCount(albumResult, targetColumn);
    int32_t newCount = GetFileCount(fileResult);
    if (oldCount != newCount) {
        MEDIA_INFO_LOG("Update album %{public}s, oldCount: %{public}d, newCount: %{public}d",
            targetColumn.c_str(), oldCount, newCount);
        values.PutInt(targetColumn, newCount);
        if (hiddenState) {
            MEDIA_INFO_LOG("Update album contains hidden: %{public}d", newCount != 0);
            values.PutInt(PhotoAlbumColumns::CONTAINS_HIDDEN, newCount != 0);
        }
    }
    return newCount;
}

static void SetCover(const shared_ptr<ResultSet> &fileResult, const shared_ptr<ResultSet> &albumResult,
    ValuesBucket &values, const bool hiddenState)
{
    string newCover;
    int32_t newCount = GetFileCount(fileResult);
    if (newCount != 0) {
        newCover = GetCover(fileResult);
    }
    const string &targetColumn = hiddenState ? PhotoAlbumColumns::HIDDEN_COVER : PhotoAlbumColumns::ALBUM_COVER_URI;
    string oldCover = GetAlbumCover(albumResult, targetColumn);
    if (oldCover != newCover) {
        MEDIA_INFO_LOG("Update album %{public}s. oldCover: %{private}s, newCover: %{private}s",
            targetColumn.c_str(), oldCover.c_str(), newCover.c_str());
        values.PutString(targetColumn, newCover);
    }
}

static void GetAlbumPredicates(PhotoAlbumSubType subtype, const shared_ptr<ResultSet> &albumResult,
    NativeRdb::RdbPredicates &predicates, const bool hiddenState)
{
    if (!subtype) {
        PhotoAlbumColumns::GetUserAlbumPredicates(GetAlbumId(albumResult), predicates, hiddenState);
    } else if (subtype >= PhotoAlbumSubType::ANALYSIS_START && subtype <= PhotoAlbumSubType::ANALYSIS_END) {
        PhotoAlbumColumns::GetAnalysisAlbumPredicates(GetAlbumId(albumResult), predicates, hiddenState);
    } else {
        PhotoAlbumColumns::GetSystemAlbumPredicates(subtype, predicates, hiddenState);
    }
}

static void SetImageVideoCount(int32_t newTotalCount,
    const shared_ptr<ResultSet> &fileResultVideo, const shared_ptr<ResultSet> &albumResult,
    ValuesBucket &values)
{
    int32_t oldVideoCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    int32_t newVideoCount = GetFileCount(fileResultVideo);
    if (oldVideoCount != newVideoCount) {
        MEDIA_INFO_LOG("Update album %{public}s, oldCount: %{public}d, newCount: %{public}d",
            PhotoAlbumColumns::ALBUM_VIDEO_COUNT.c_str(), oldVideoCount, newVideoCount);
        values.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, newVideoCount);
    }
    int32_t oldImageCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
    int32_t newImageCount = newTotalCount - newVideoCount;
    if (oldImageCount != newImageCount) {
        MEDIA_INFO_LOG("Update album %{public}s, oldCount: %{public}d, newCount: %{public}d",
            PhotoAlbumColumns::ALBUM_IMAGE_COUNT.c_str(), oldImageCount, newImageCount);
        values.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, newImageCount);
    }
}

static int32_t SetUpdateValues(const shared_ptr<NativeRdb::RdbStore> &rdbStore,
    const shared_ptr<ResultSet> &albumResult, ValuesBucket &values, PhotoAlbumSubType subtype, const bool hiddenState)
{
    const vector<string> columns = {
        MEDIA_COLUMN_COUNT_1,
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME
    };

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    GetAlbumPredicates(subtype, albumResult, predicates, hiddenState);
    if (subtype == PhotoAlbumSubType::HIDDEN || hiddenState) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    } else {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_ADDED_INDEX);
    }
    auto fileResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fileResult == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t newCount = SetCount(fileResult, albumResult, values, hiddenState);
    SetCover(fileResult, albumResult, values, hiddenState);
    if (hiddenState == 0) {
        predicates.Clear();
        GetAlbumPredicates(subtype, albumResult, predicates, hiddenState);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
        predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO));
        auto fileResultVideo = QueryGoToFirst(rdbStore, predicates, columns);
        if (fileResultVideo == nullptr) {
            return E_HAS_DB_ERROR;
        }
        SetImageVideoCount(newCount, fileResultVideo, albumResult, values);
    }
    return E_SUCCESS;
}

static int32_t UpdateUserAlbumIfNeeded(const shared_ptr<RdbStore> &rdbStore, const shared_ptr<ResultSet> &albumResult,
    const bool hiddenState)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumIfNeeded");
    ValuesBucket values;
    int err = SetUpdateValues(rdbStore, albumResult, values, static_cast<PhotoAlbumSubType>(0), hiddenState);
    if (err < 0) {
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(GetAlbumId(albumResult)));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    int32_t changedRows = 0;
    err = rdbStore->Update(changedRows, values, predicates);
    if (err < 0) {
        MEDIA_WARN_LOG("Failed to update album count and cover! err: %{public}d", err);
    }
    return E_SUCCESS;
}

static int32_t UpdateAnalysisAlbumIfNeeded(const shared_ptr<RdbStore> &rdbStore,
    const shared_ptr<ResultSet> &albumResult, const bool hiddenState)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumIfNeeded");
    ValuesBucket values;
    auto subtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
    int err = SetUpdateValues(rdbStore, albumResult, values, subtype, hiddenState);
    if (err < 0) {
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(GetAlbumId(albumResult)));
    int32_t changedRows = 0;
    err = rdbStore->Update(changedRows, values, predicates);
    if (err < 0) {
        MEDIA_WARN_LOG("Failed to update album count and cover! err: %{public}d", err);
        return err;
    }
    return E_SUCCESS;
}

static int32_t UpdateSysAlbumIfNeeded(const shared_ptr<RdbStore> &rdbStore,
    const shared_ptr<ResultSet> &albumResult, const bool hiddenState)
{
    ValuesBucket values;
    auto subtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSysAlbum: " + to_string(subtype));
    int err = SetUpdateValues(rdbStore, albumResult, values, subtype, hiddenState);
    if (err < 0) {
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subtype));
    int32_t changedRows = 0;
    err = rdbStore->Update(changedRows, values, predicates);
    if (err < 0) {
        MEDIA_WARN_LOG("Failed to update album count and cover! err: %{public}d", err);
    }
    return E_SUCCESS;
}

void MediaLibraryRdbUtils::UpdateUserAlbumInternal(const shared_ptr<RdbStore> &rdbStore,
    const vector<string> &userAlbumIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumInternal");

    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
    };
    auto albumResult = GetUserAlbum(rdbStore, userAlbumIds, columns);
    if (albumResult == nullptr) {
        return;
    }
    ForEachRow(rdbStore, albumResult, false, UpdateUserAlbumIfNeeded);
}

void MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(const shared_ptr<RdbStore> &rdbStore,
    const vector<string> &anaAlbumAlbumIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumInternal");
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
    };
    auto albumResult = GetAnalysisAlbum(rdbStore, anaAlbumAlbumIds, columns);
    if (albumResult == nullptr) {
        return;
    }
    ForEachRow(rdbStore, albumResult, false, UpdateAnalysisAlbumIfNeeded);
}

static inline shared_ptr<ResultSet> GetSystemAlbum(const shared_ptr<RdbStore> &rdbStore,
    const vector<string> &subtypes, const vector<string> &columns)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (subtypes.empty()) {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, ALL_SYS_PHOTO_ALBUM);
    } else {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, subtypes);
    }
    return Query(rdbStore, predicates, columns);
}

void MediaLibraryRdbUtils::UpdateSystemAlbumInternal(const shared_ptr<RdbStore> &rdbStore,
    const vector<string> &subtypes)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSystemAlbumInternal");

    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
    };
    auto albumResult = GetSystemAlbum(rdbStore, subtypes, columns);
    if (albumResult == nullptr) {
        return;
    }

    ForEachRow(rdbStore, albumResult, false, UpdateSysAlbumIfNeeded);
}

static void UpdateUserAlbumHiddenState(const shared_ptr<RdbStore> &rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumHiddenState");
    vector<string> userAlbumIds;

    auto albumResult = GetUserAlbum(rdbStore, userAlbumIds, {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::CONTAINS_HIDDEN,
        PhotoAlbumColumns::HIDDEN_COUNT,
        PhotoAlbumColumns::HIDDEN_COVER,
    });
    if (albumResult == nullptr) {
        return;
    }
    ForEachRow(rdbStore, albumResult, true, UpdateUserAlbumIfNeeded);
}

static void UpdateSysAlbumHiddenState(const shared_ptr<RdbStore> &rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSysAlbumHiddenState");

    auto albumResult = GetSystemAlbum(rdbStore, {
        to_string(PhotoAlbumSubType::IMAGES),
        to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::FAVORITE),
        to_string(PhotoAlbumSubType::SCREENSHOT),
        to_string(PhotoAlbumSubType::CAMERA),
    }, {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::CONTAINS_HIDDEN,
        PhotoAlbumColumns::HIDDEN_COUNT,
        PhotoAlbumColumns::HIDDEN_COVER,
    });
    if (albumResult == nullptr) {
        return;
    }
    ForEachRow(rdbStore, albumResult, true, UpdateSysAlbumIfNeeded);
}

void MediaLibraryRdbUtils::UpdateHiddenAlbumInternal(const shared_ptr<RdbStore> &rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateHiddenAlbumInternal");

    UpdateUserAlbumHiddenState(rdbStore);
    UpdateSysAlbumHiddenState(rdbStore);
}
} // namespace OHOS::Media
