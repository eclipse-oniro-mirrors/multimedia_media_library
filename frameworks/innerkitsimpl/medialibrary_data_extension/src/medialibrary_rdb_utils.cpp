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

#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include "datashare_values_bucket.h"
#include "media_analysis_helper.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_refresh_album_column.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_business_record_column.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "result_set.h"
#include "userfile_manager_types.h"
#include "vision_album_column.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"
#include "vision_photo_map_column.h"
#include "vision_total_column.h"
#include "location_column.h"
#include "search_column.h"
#include "story_cover_info_column.h"
#include "power_efficiency_manager.h"
#include "rdb_sql_utils.h"
#include "medialibrary_restore.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

constexpr int32_t E_EMPTY_ALBUM_ID = 1;
constexpr size_t ALBUM_UPDATE_THRESHOLD = 1000;
constexpr int32_t SINGLE_FACE = 1;
constexpr double LOCATION_DB_ZERO = 0;
constexpr double LOCATION_LATITUDE_MAX = 90.0;
constexpr double LOCATION_LATITUDE_MIN = -90.0;
constexpr double LOCATION_LONGITUDE_MAX = 180.0;
constexpr double LOCATION_LONGITUDE_MIN = -180.0;
constexpr int32_t SEARCH_UPDATE_STATUS = 2;
constexpr int32_t FACE_RECOGNITION = 1;
constexpr int32_t FACE_FEATURE = 2;
constexpr int32_t FACE_CLUSTERED = 3;
constexpr int32_t CLOUD_POSITION_STATUS = 2;
constexpr int32_t UPDATE_ALBUM_TIME_OUT = 1000;
mutex MediaLibraryRdbUtils::sRefreshAlbumMutex_;

// 注意，端云同步代码仓也有相同常量，添加新相册时，请通知端云同步进行相应修改
const std::vector<std::string> ALL_SYS_PHOTO_ALBUM = {
    std::to_string(PhotoAlbumSubType::FAVORITE),
    std::to_string(PhotoAlbumSubType::VIDEO),
    std::to_string(PhotoAlbumSubType::HIDDEN),
    std::to_string(PhotoAlbumSubType::TRASH),
    std::to_string(PhotoAlbumSubType::SCREENSHOT),
    std::to_string(PhotoAlbumSubType::CAMERA),
    std::to_string(PhotoAlbumSubType::IMAGE),
    std::to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
    std::to_string(PhotoAlbumSubType::SOURCE_GENERIC),
};

// 注意，端云同步代码仓也有相同常量，添加新相册时，请通知端云同步进行相应修改
const std::vector<std::string> ALL_ANALYSIS_ALBUM = {
    std::to_string(PhotoAlbumSubType::CLASSIFY),
    std::to_string(PhotoAlbumSubType::GEOGRAPHY_LOCATION),
    std::to_string(PhotoAlbumSubType::GEOGRAPHY_CITY),
    std::to_string(PhotoAlbumSubType::SHOOTING_MODE),
    std::to_string(PhotoAlbumSubType::PORTRAIT),
};

struct BussinessRecordValue {
    string bussinessType;
    string key;
    string value;
};

struct UpdateAlbumData {
    int32_t albumId;
    int32_t albumSubtype;
    int32_t hiddenCount;
    int32_t albumCount;
    int32_t albumImageCount;
    int32_t albumVideoCount;
    string hiddenCover;
    string  albumCoverUri;
    uint8_t isCoverSatisfied;
};

struct RefreshAlbumData {
    int32_t albumId;
    int32_t albumSubtype;
};

enum UpdateAlbumType {
    UPDATE_SYSTEM_ALBUM = 400,
    UPDATE_HIDDEN_ALBUM,
    UPDATE_USER_ALBUM,
    UPDATE_SOURCE_ALBUM,
    UPDATE_ANALYSIS_ALBUM,
};

using UpdateHandler = std::function<int32_t(
    const shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const UpdateAlbumData &data,
    const bool hiddenState,
    std::shared_ptr<TransactionOperations> trans)>;

atomic<bool> MediaLibraryRdbUtils::isNeedRefreshAlbum = false;
atomic<bool> MediaLibraryRdbUtils::isInRefreshTask = false;

const string ANALYSIS_REFRESH_BUSINESS_TYPE = "ANALYSIS_ALBUM_REFRESH";

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

static inline shared_ptr<ResultSet> GetUserAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore,
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

static inline shared_ptr<ResultSet> GetAnalysisAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore,
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

static inline shared_ptr<ResultSet> GetSourceAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &sourceAlbumIds, const vector<string> &columns)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (!sourceAlbumIds.empty()) {
        predicates.In(PhotoAlbumColumns::ALBUM_ID, sourceAlbumIds);
    } else {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC));
    }
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(predicates, columns);
}

static inline shared_ptr<ResultSet> GetAnalysisAlbumBySubtype(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &subtypes, const vector<string> &columns)
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    if (!subtypes.empty()) {
        predicates.In(ALBUM_SUBTYPE, subtypes);
    } else {
        predicates.In(ALBUM_SUBTYPE, ALL_ANALYSIS_ALBUM);
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

static shared_ptr<ResultSet> QueryGoToFirst(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const RdbPredicates &predicates, const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryGoToFirst");
    auto resultSet = rdbStore->StepQueryWithoutCheck(predicates, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }

    MediaLibraryTracer goToFirst;
    goToFirst.Start("GoToFirstRow");
    int32_t err = resultSet->GoToFirstRow();
    MediaLibraryRestore::GetInstance().CheckRestore(err);
    return resultSet;
}

static int32_t ForEachRow(const shared_ptr<MediaLibraryRdbStore> rdbStore, const std::vector<UpdateAlbumData> &datas,
    const bool hiddenState, const UpdateHandler &func)
{
    int32_t err = NativeRdb::E_OK;
    for (auto data : datas) {
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        std::function<int(void)> transFunc = [&]()->int {
            // Ignore failure here, try to iterate rows as much as possible.
            func(rdbStore, data, hiddenState, trans);
            return err;
        };
        err = trans->RetryTrans(transFunc);
        if (err != E_OK) {
            MEDIA_ERR_LOG("ForEachRow: trans retry fail!, ret:%{public}d", err);
        }
    }
    return E_SUCCESS;
}

static inline int32_t GetFileCount(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, MEDIA_COLUMN_COUNT_1);
}

static inline int32_t GetPortraitFileCount(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, MEDIA_COLUMN_COUNT_DISTINCT_FILE_ID);
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

static inline uint8_t GetIsCoverSatisfied(const shared_ptr<ResultSet> &resultSet)
{
    return GetIntValFromColumn(resultSet, IS_COVER_SATISFIED);
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

static int32_t SetCount(const shared_ptr<ResultSet> &fileResult, const UpdateAlbumData &data,
    ValuesBucket &values, const bool hiddenState, PhotoAlbumSubType subtype)
{
    const string &targetColumn = hiddenState ? PhotoAlbumColumns::HIDDEN_COUNT : PhotoAlbumColumns::ALBUM_COUNT;
    int32_t oldCount = hiddenState ? data.hiddenCount : data.albumCount;
    int32_t newCount;
    if (subtype == PORTRAIT) {
        newCount = GetPortraitFileCount(fileResult);
    } else {
        newCount = GetFileCount(fileResult);
    }
    int32_t id = data.albumId;
    if (oldCount != newCount) {
        MEDIA_INFO_LOG("Album %{public}d Update %{public}s, oldCount: %{public}d, newCount: %{public}d", id,
            targetColumn.c_str(), oldCount, newCount);
        values.PutInt(targetColumn, newCount);
        if (hiddenState) {
            MEDIA_INFO_LOG("Update album contains hidden: %{public}d", newCount != 0);
            values.PutInt(PhotoAlbumColumns::CONTAINS_HIDDEN, newCount != 0);
        }
    }
    return newCount;
}

static void SetPortraitCover(const shared_ptr<ResultSet> &fileResult, const UpdateAlbumData &data,
    ValuesBucket &values, int newCount)
{
    string newCover;
    if (newCount != 0) {
        newCover = GetCover(fileResult);
    }
    string oldCover = data.albumCoverUri;
    if (oldCover != newCover) {
        values.PutInt(IS_COVER_SATISFIED, static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING));
        values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, newCover);
        int32_t albumId = data.albumId;
        MEDIA_INFO_LOG("Update portrait album %{public}d. oldCover: %{public}s, newCover: %{public}s",
            albumId, MediaFileUtils::GetUriWithoutDisplayname(oldCover).c_str(),
            MediaFileUtils::GetUriWithoutDisplayname(newCover).c_str());
    }
}

static void SetCover(const shared_ptr<ResultSet> &fileResult, const UpdateAlbumData &data,
    ValuesBucket &values, const bool hiddenState)
{
    string newCover;
    int32_t newCount = GetFileCount(fileResult);
    if (newCount != 0) {
        newCover = GetCover(fileResult);
    }
    const string &targetColumn = hiddenState ? PhotoAlbumColumns::HIDDEN_COVER : PhotoAlbumColumns::ALBUM_COVER_URI;
    string oldCover = hiddenState ? data.hiddenCover : data.albumCoverUri;
    if (oldCover != newCover) {
        int32_t id = data.albumId;
        MEDIA_INFO_LOG("Update album %{public}d %{public}s. oldCover: %{public}s, newCover: %{public}s",
            id, targetColumn.c_str(), MediaFileUtils::GetUriWithoutDisplayname(oldCover).c_str(),
            MediaFileUtils::GetUriWithoutDisplayname(newCover).c_str());
        values.PutString(targetColumn, newCover);
    }
}

static void GetAlbumPredicates(PhotoAlbumSubType subtype, const int32_t albumId,
    NativeRdb::RdbPredicates &predicates, const bool hiddenState, const bool isUpdateAlbum = false)
{
    static const string QUERY_ASSETS_FROM_ANALYSIS_ALBUM =
        PhotoColumn::PHOTO_SYNC_STATUS + " = " + to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) +
        " AND " + PhotoColumn::PHOTO_CLEAN_FLAG + " = " + to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " AND " + MediaColumn::MEDIA_ID + " IN (SELECT " + PhotoMap::ASSET_ID + " FROM " + ANALYSIS_PHOTO_MAP_TABLE +
        " WHERE " + PhotoMap::ALBUM_ID + " = ?) AND " + MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
        MediaColumn::MEDIA_HIDDEN + " = ? AND " + MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = " +
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER));

    bool isUserAlbum = !subtype;
    bool isSourceAlbum = subtype == PhotoAlbumSubType::SOURCE_GENERIC;
    bool isAnalysisAlbum = subtype >= PhotoAlbumSubType::ANALYSIS_START && subtype <= PhotoAlbumSubType::ANALYSIS_END;
    if (isUpdateAlbum && isAnalysisAlbum) {
        predicates.SetWhereClause(QUERY_ASSETS_FROM_ANALYSIS_ALBUM);
        predicates.SetWhereArgs({ to_string(albumId), to_string(hiddenState) });
        return;
    }

    if (isUserAlbum) {
        PhotoAlbumColumns::GetUserAlbumPredicates(albumId, predicates, hiddenState);
    } else if (subtype == PhotoAlbumSubType::PORTRAIT) {
        PhotoAlbumColumns::GetPortraitAlbumPredicates(albumId, predicates, hiddenState);
    } else if (isAnalysisAlbum) {
        PhotoAlbumColumns::GetAnalysisAlbumPredicates(albumId, predicates, hiddenState);
    } else if (isSourceAlbum) {
        PhotoAlbumColumns::GetSourceAlbumPredicates(albumId, predicates, hiddenState);
    } else {
        PhotoAlbumColumns::GetSystemAlbumPredicates(subtype, predicates, hiddenState);
    }
}

static void SetImageVideoCount(int32_t newTotalCount, const shared_ptr<ResultSet> &fileResultVideo,
    const UpdateAlbumData &data, ValuesBucket &values)
{
    int32_t oldVideoCount = data.albumVideoCount;
    int32_t newVideoCount = GetFileCount(fileResultVideo);
    if (oldVideoCount != newVideoCount) {
        MEDIA_DEBUG_LOG("Update album %{public}s, oldCount: %{public}d, newCount: %{public}d",
            PhotoAlbumColumns::ALBUM_VIDEO_COUNT.c_str(), oldVideoCount, newVideoCount);
        values.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, newVideoCount);
    }
    int32_t oldImageCount = data.albumImageCount;
    int32_t newImageCount = newTotalCount - newVideoCount;
    if (oldImageCount != newImageCount) {
        MEDIA_DEBUG_LOG("Update album %{public}s, oldCount: %{public}d, newCount: %{public}d",
            PhotoAlbumColumns::ALBUM_IMAGE_COUNT.c_str(), oldImageCount, newImageCount);
        values.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, newImageCount);
    }
}

static int32_t QueryAlbumCount(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype)
{
    const vector<string> columns = { MEDIA_COLUMN_COUNT_1 };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (subtype == PhotoAlbumSubType::USER_GENERIC) {
        GetAlbumPredicates(static_cast<PhotoAlbumSubType>(0), albumId, predicates, false);
    } else {
        GetAlbumPredicates(subtype, albumId, predicates, false);
    }
    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        return E_HAS_DB_ERROR;
    }
    return GetFileCount(fetchResult);
}

static int32_t QueryAlbumVideoCount(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype)
{
    const vector<string> columns = { MEDIA_COLUMN_COUNT_1 };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (subtype == PhotoAlbumSubType::USER_GENERIC) {
        GetAlbumPredicates(static_cast<PhotoAlbumSubType>(0), albumId, predicates, false);
    } else {
        GetAlbumPredicates(subtype, albumId, predicates, false);
    }
    predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO));
    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        return E_HAS_DB_ERROR;
    }
    return GetFileCount(fetchResult);
}

static int32_t QueryAlbumHiddenCount(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype)
{
    const vector<string> columns = { MEDIA_COLUMN_COUNT_1 };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (subtype == PhotoAlbumSubType::USER_GENERIC) {
        GetAlbumPredicates(static_cast<PhotoAlbumSubType>(0), albumId, predicates, true);
    } else {
        GetAlbumPredicates(subtype, albumId, predicates, true);
    }
    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        return E_HAS_DB_ERROR;
    }
    return GetFileCount(fetchResult);
}

static int32_t SetAlbumCounts(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype, AlbumCounts &albumCounts)
{
    int ret = QueryAlbumCount(rdbStore, albumId, subtype);
    if (ret < E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to QueryAlbumCount, ret:%{public}d", ret);
        return ret;
    }
    albumCounts.count = ret;

    ret = QueryAlbumVideoCount(rdbStore, albumId, subtype);
    if (ret < E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to QueryAlbumVideoCount, ret:%{public}d", ret);
        return ret;
    }
    albumCounts.videoCount = ret;
    albumCounts.imageCount = albumCounts.count - albumCounts.videoCount;

    ret = QueryAlbumHiddenCount(rdbStore, albumId, subtype);
    if (ret < E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to QueryAlbumCount, ret:%{public}d", ret);
        return ret;
    }
    albumCounts.hiddenCount = ret;
    return E_SUCCESS;
}

static int32_t SetAlbumCoverUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype, string &uri)
{
    const vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (subtype == PhotoAlbumSubType::HIDDEN) {
        GetAlbumPredicates(subtype, albumId, predicates, true);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    } else if (subtype == PhotoAlbumSubType::USER_GENERIC) {
        GetAlbumPredicates(static_cast<PhotoAlbumSubType>(0), albumId, predicates, false);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_ADDED_INDEX);
    } else {
        GetAlbumPredicates(subtype, albumId, predicates, false);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_ADDED_INDEX);
    }
    predicates.Limit(1);

    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        MEDIA_ERR_LOG("QueryGoToFirst failed");
        return E_HAS_DB_ERROR;
    }
    uri = GetCover(fetchResult);
    return E_SUCCESS;
}

static int32_t SetAlbumCoverHiddenUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype, string &uri)
{
    const vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    if (subtype == PhotoAlbumSubType::USER_GENERIC) {
        GetAlbumPredicates(static_cast<PhotoAlbumSubType>(0), albumId, predicates, true);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    } else {
        GetAlbumPredicates(subtype, albumId, predicates, true);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    }
    predicates.Limit(1);
    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        MEDIA_ERR_LOG("QueryGoToFirst failed");
        return E_HAS_DB_ERROR;
    }
    uri = GetCover(fetchResult);
    return E_SUCCESS;
}

static int32_t FillOneAlbumCountAndCoverUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    int32_t albumId, PhotoAlbumSubType subtype, string &sql)
{
    AlbumCounts albumCounts = { 0, 0, 0, 0 };
    int32_t ret = SetAlbumCounts(rdbStore, albumId, subtype, albumCounts);
    if (ret != E_SUCCESS) {
        return ret;
    }
    string coverUri;
    ret = SetAlbumCoverUri(rdbStore, albumId, subtype, coverUri);
    if (ret != E_SUCCESS) {
        return ret;
    }
    string coverHiddenUri;
    if (albumCounts.hiddenCount != 0) {
        ret = SetAlbumCoverHiddenUri(rdbStore, albumId, subtype, coverHiddenUri);
        if (ret != E_SUCCESS) {
            return ret;
        }
    }

    if (albumId < 0) {
        MEDIA_ERR_LOG("Can not get correct albumId, error albumId is %{public}d", albumId);
        return E_HAS_DB_ERROR;
    }
    string coverUriSql = PhotoAlbumColumns::ALBUM_COVER_URI;
    if (coverUri.empty()) {
        coverUriSql += " = NULL";
    } else {
        coverUriSql += " = '" + coverUri + "'";
    }
    string coverHiddenUriSql = PhotoAlbumColumns::HIDDEN_COVER;
    if (coverHiddenUri.empty()) {
        coverHiddenUriSql += " = NULL";
    } else {
        coverHiddenUriSql += " = '" + coverHiddenUri + "'";
    }

    sql = "UPDATE " + PhotoAlbumColumns::TABLE + " SET " +
        PhotoAlbumColumns::ALBUM_COUNT + " = " + to_string(albumCounts.count) + ", " +
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT + " = " +  to_string(albumCounts.imageCount) + ", " +
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT + " = " + to_string(albumCounts.videoCount) + ", " +
        PhotoAlbumColumns::HIDDEN_COUNT + " = " + to_string(albumCounts.hiddenCount) + ", " +
        PhotoAlbumColumns::CONTAINS_HIDDEN + " = " + to_string((albumCounts.hiddenCount == 0) ? 0 : 1) + ", " +
        coverUriSql + ", " + coverHiddenUriSql + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " = " + to_string(albumId) + ";";
    return E_SUCCESS;
}

static std::string GetPhotoId(const std::string &uri)
{
    if (uri.compare(0, PhotoColumn::PHOTO_URI_PREFIX.size(),
        PhotoColumn::PHOTO_URI_PREFIX) != 0) {
        return "";
    }
    std::string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());
    return tmp.substr(0, tmp.find_first_of('/'));
}

static int32_t RefreshAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<RefreshAlbumData> &datas,
    function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler)
{
    for (auto data : datas) {
        auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
        int32_t albumId = data.albumId;
        string sql;
        int32_t ret = FillOneAlbumCountAndCoverUri(rdbStore, albumId, subtype, sql);
        if (ret != E_SUCCESS) {
            return ret;
        }

        ret = rdbStore->ExecuteSql(sql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to execute sql:%{private}s", sql.c_str());
            return E_HAS_DB_ERROR;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", sql.c_str());
        refreshProcessHandler(PhotoAlbumType::SYSTEM, subtype, albumId);
    }
    return E_SUCCESS;
}

static void DeleteAllAlbumId(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    string updateRefreshTableSql = "DELETE FROM " + ALBUM_REFRESH_TABLE;
    int32_t ret = rdbStore->ExecuteSql(updateRefreshTableSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute sql:%{private}s", updateRefreshTableSql.c_str());
        return;
    }
    MEDIA_INFO_LOG("Delete AlbumRefreshTable success");
}

static int32_t GetAllRefreshAlbumIds(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<RefreshAlbumData> &datas)
{
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_SUBTYPE };
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.SetWhereClause(PhotoAlbumColumns::ALBUM_ID + " IN (SELECT " + REFRESHED_ALBUM_ID + " FROM " +
        ALBUM_REFRESH_TABLE + ")");
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not query ALBUM_REFRESH_TABLE");
        return E_HAS_DB_ERROR;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        RefreshAlbumData data;
        data.albumId = GetAlbumId(resultSet);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(resultSet));
        datas.push_back(data);
    }
    resultSet->Close();
    DeleteAllAlbumId(rdbStore);
    return E_SUCCESS;
}

shared_ptr<ResultSet> QueryAlbumById(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &albumIds)
{
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE
    };
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not Query from rdb");
        return nullptr;
    }
    return resultSet;
}

int32_t MediaLibraryRdbUtils::IsNeedRefreshByCheckTable(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    bool &signal)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdb is nullptr");
        return E_HAS_DB_ERROR;
    }

    RdbPredicates predicates(ALBUM_REFRESH_TABLE);
    vector<string> columns = { REFRESHED_ALBUM_ID };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not query ALBUM_REFRESH_TABLE");
        return E_HAS_DB_ERROR;
    }

    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetRowCount failed ret:%{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    if (count == 0) {
        MEDIA_DEBUG_LOG("count is zero, should not refresh");
        signal = false;
    } else {
        MEDIA_DEBUG_LOG("count is %{public}d, should refresh", count);
        signal = true;
    }
    return E_SUCCESS;
}

bool MediaLibraryRdbUtils::IsNeedRefreshAlbum()
{
    return isNeedRefreshAlbum.load();
}

void MediaLibraryRdbUtils::SetNeedRefreshAlbum(bool isNeedRefresh)
{
    isNeedRefreshAlbum = isNeedRefresh;
}

bool MediaLibraryRdbUtils::IsInRefreshTask()
{
    return isInRefreshTask.load();
}

static void GetPortraitAlbumCountPredicates(const string &albumId, RdbPredicates &predicates)
{
    string anaAlbumGroupTag = ANALYSIS_ALBUM_TABLE + "." + GROUP_TAG;
    string anaAlbumId = ANALYSIS_ALBUM_TABLE + "." + ALBUM_ID;
    string anaPhotoMapAlbum = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM;
    string anaPhotoMapAsset = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ASSET;
    string photosDateTrashed = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_DATE_TRASHED;
    string photosFileId = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID;
    string photosHidden = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_HIDDEN;
    string photosTimePending = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_TIME_PENDING;
    string photosIsTemp = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_IS_TEMP;
    string photoIsCover = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_BURST_COVER_LEVEL;

    string clause = anaPhotoMapAsset + " = " + photosFileId;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ clause });
    clause = anaAlbumId + " = " + anaPhotoMapAlbum;
    predicates.InnerJoin(ANALYSIS_ALBUM_TABLE)->On({ clause });

    clause = "( " + anaAlbumGroupTag + " IN ( SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = " + albumId + " ))";
    predicates.SetWhereClause(clause + " AND ");
    predicates.BeginWrap();
    predicates.EqualTo(photosDateTrashed, to_string(0));
    predicates.EqualTo(photosHidden, to_string(0));
    predicates.EqualTo(photosTimePending, to_string(0));
    predicates.EqualTo(photosIsTemp, to_string(0));
    predicates.EqualTo(photoIsCover, to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    predicates.EndWrap();
    predicates.Distinct();
}

static bool IsCoverValid(const shared_ptr<MediaLibraryRdbStore> rdbStore, const string &albumId, const string &fileId)
{
    if (fileId.empty()) {
        MEDIA_WARN_LOG("Invalid cover: empty file_id");
        return false;
    }
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    string anaPhotoMapAsset = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ASSET;
    string photosFileId = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID;
    string clause = anaPhotoMapAsset + " = " + photosFileId;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ clause });

    string anaAlbumId = ANALYSIS_ALBUM_TABLE + "." + ALBUM_ID;
    string anaPhotoMapAlbum = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM;
    clause = anaAlbumId + " = " + anaPhotoMapAlbum;
    predicates.InnerJoin(ANALYSIS_ALBUM_TABLE)->On({ clause });

    string photoSyncStatus = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_SYNC_STATUS;
    string photoCleanFlag = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_CLEAN_FLAG;
    string photosDateTrashed = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_DATE_TRASHED;
    string photosHidden = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_HIDDEN;
    string photosTimePending = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_TIME_PENDING;
    string photosIsTemp = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_IS_TEMP;
    string photoIsCover = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_BURST_COVER_LEVEL;

    string whereClause = anaAlbumId + " = " + albumId + " AND " + photosFileId + " = " + fileId + " AND " +
        photoSyncStatus + " = " + to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
        photoCleanFlag + " = " + to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) + " AND " +
        photosDateTrashed + " = " + to_string(0) + " AND " + photosHidden + " = " + to_string(0) + " AND " +
        photosTimePending + " = " + to_string(0) + " AND " + photosIsTemp + " = " + to_string(0) + " AND " +
        photoIsCover + " = " + to_string(static_cast<int32_t>(BurstCoverLevelType::COVER));

    predicates.SetWhereClause(whereClause);
    predicates.Limit(1);
    vector<string> columns;
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not query Photos, albumId: %{public}s, fileId: %{public}s", albumId.c_str(), fileId.c_str());
        return false;
    }
    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetRowCount failed, albumId: %{public}s, fileId: %{public}s, ret:%{public}d", albumId.c_str(),
            fileId.c_str(), ret);
        return false;
    }
    if (count == 0) {
        MEDIA_WARN_LOG("Invalid cover: albumId: %{public}s, fileId: %{public}s not exist", albumId.c_str(),
            fileId.c_str());
        return false;
    }
    return true;
}

static inline bool ShouldUpdatePortraitAlbumCover(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const string &albumId, const string &fileId, const uint8_t isCoverSatisfied)
{
    return isCoverSatisfied == static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING) ||
        !IsCoverValid(rdbStore, albumId, fileId);
}

static shared_ptr<ResultSet> QueryPortraitAlbumCover(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const string &albumId)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryPortraitCover");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    // INNER JOIN AnalysisPhotoMap ON AnalysisPhotoMap.map_asset = Photos.file_id
    string anaPhotoMapAsset = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ASSET;
    string photosFileId = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID;
    string clause = anaPhotoMapAsset + " = " + photosFileId;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ clause });

    // INNER JOIN AnalysisAlbum ON AnalysisAlbum.album_id = AnalysisPhotoMap.map_album
    string anaAlbumId = ANALYSIS_ALBUM_TABLE + "." + ALBUM_ID;
    string anaPhotoMapAlbum = ANALYSIS_PHOTO_MAP_TABLE + "." + MAP_ALBUM;
    clause = anaAlbumId + " = " + anaPhotoMapAlbum;
    predicates.InnerJoin(ANALYSIS_ALBUM_TABLE)->On({ clause });

    // INNER JOIN tab_analysis_image_face ON tab_analysis_image_face.file_id = Photos.file_id
    string anaImageFaceFileId = VISION_IMAGE_FACE_TABLE + "." + MediaColumn::MEDIA_ID;
    clause = anaImageFaceFileId + "=" + photosFileId;
    predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ clause });

    clause = "Photos.sync_status = 0 "
        "AND Photos.clean_flag = 0 "
        "AND Photos.date_trashed = 0 "
        "AND Photos.hidden = 0 "
        "AND Photos.time_pending = 0 "
        "AND Photos.is_temp = 0 "
        "AND Photos.burst_cover_level = 1 "
        "AND AnalysisAlbum.group_tag = (SELECT group_tag FROM AnalysisAlbum WHERE album_id = " +
        albumId +
        " LIMIT 1) "
        "AND AnalysisAlbum.group_tag LIKE '%' || tab_analysis_image_face.tag_id || '%'";
    predicates.SetWhereClause(clause);

    predicates.OrderByDesc(VISION_IMAGE_FACE_TABLE + "." + IS_EXCLUDED);
    predicates.OrderByDesc(VISION_IMAGE_FACE_TABLE + "." + FACE_AESTHETICS_SCORE);
    predicates.OrderByAsc("CASE WHEN tab_analysis_image_face.total_faces = 1 THEN 0 ELSE 1 END");
    predicates.OrderByDesc(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_DATE_ADDED);
    predicates.Limit(1);
    const string columnFileId = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID;
    const string columnDisplayName = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_NAME;
    const string columnData = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_FILE_PATH;
    const vector<string> columns = { columnFileId, columnDisplayName, columnData };
    auto resultSet = rdbStore->StepQueryWithoutCheck(predicates, columns);
    string sql = RdbSqlUtils::BuildQueryString(predicates, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    int32_t err = resultSet->GoToFirstRow();
    MediaLibraryRestore::GetInstance().CheckRestore(err);
    return resultSet;
}

static int32_t SetPortraitUpdateValues(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const UpdateAlbumData &data, const vector<string> &fileIds, ValuesBucket &values)
{
    const vector<string> countColumns = {
        MEDIA_COLUMN_COUNT_DISTINCT_FILE_ID
    };

    string coverUri = data.albumCoverUri;
    string coverId = GetPhotoId(coverUri);
    uint8_t isCoverSatisfied = data.isCoverSatisfied;

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    string albumId = to_string(data.albumId);
    GetPortraitAlbumCountPredicates(albumId, predicates);
    shared_ptr<ResultSet> countResult = QueryGoToFirst(rdbStore, predicates, countColumns);
    if (countResult == nullptr) {
        MEDIA_ERR_LOG("Failed to query Portrait Album Count");
        return E_HAS_DB_ERROR;
    }
    int32_t newCount = SetCount(countResult, data, values, false, PhotoAlbumSubType::PORTRAIT);
    if (!ShouldUpdatePortraitAlbumCover(rdbStore, albumId, coverId, isCoverSatisfied)) {
        return E_SUCCESS;
    }
    shared_ptr<ResultSet> coverResult = QueryPortraitAlbumCover(rdbStore, albumId);
    if (coverResult == nullptr) {
        MEDIA_ERR_LOG("Failed to query Portrait Album Cover");
        return E_HAS_DB_ERROR;
    }
    SetPortraitCover(coverResult, data, values, newCount);
    return E_SUCCESS;
}

static void RefreshHighlightAlbum(int32_t albumId)
{
    vector<string> albumIds;
    albumIds.push_back(to_string(albumId));
    MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(Media::MediaAnalysisProxy::ActivateServiceType::HIGHLIGHT_COVER_GENERATE), albumIds);
}

static int32_t SetUpdateValues(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const UpdateAlbumData &data, ValuesBucket &values, PhotoAlbumSubType subtype, const bool hiddenState)
{
    const vector<string> columns = {
        MEDIA_COLUMN_COUNT_1,
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    GetAlbumPredicates(subtype, data.albumId, predicates, hiddenState, true);
    if (subtype == PhotoAlbumSubType::HIDDEN || hiddenState) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    } else if (subtype == PhotoAlbumSubType::VIDEO || subtype == PhotoAlbumSubType::IMAGE) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
    } else if (subtype == PhotoAlbumSubType::FAVORITE) {
        predicates.IndexedBy(PhotoColumn::PHOTO_FAVORITE_INDEX);
    } else {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_ADDED_INDEX);
    }
    auto fileResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fileResult == nullptr) {
        MEDIA_ERR_LOG("Failed to query fileResult");
        return E_HAS_DB_ERROR;
    }
    int32_t newCount = SetCount(fileResult, data, values, hiddenState, subtype);
    if (subtype != PhotoAlbumSubType::HIGHLIGHT && subtype != PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        SetCover(fileResult, data, values, hiddenState);
    } else {
        RefreshHighlightAlbum(data.albumId);
    }
    if (hiddenState == 0 && (subtype < PhotoAlbumSubType::ANALYSIS_START ||
        subtype > PhotoAlbumSubType::ANALYSIS_END)) {
        predicates.Clear();
        GetAlbumPredicates(subtype, data.albumId, predicates, hiddenState, true);
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
        string queryCondition = predicates.GetWhereClause();
        if (queryCondition.empty()) {
            predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO));
        } else {
            predicates.SetWhereClause(
                "(" + queryCondition + ") AND " + MediaColumn::MEDIA_TYPE + " = " + to_string(MEDIA_TYPE_VIDEO));
        }
        auto fileResultVideo = QueryGoToFirst(rdbStore, predicates, columns);
        if (fileResultVideo == nullptr) {
            MEDIA_ERR_LOG("Failed to query fileResultVideo");
            return E_HAS_DB_ERROR;
        }
        SetImageVideoCount(newCount, fileResultVideo, data, values);
    }
    return E_SUCCESS;
}

static void QueryAlbumId(const shared_ptr<MediaLibraryRdbStore> rdbStore, const RdbPredicates predicates,
    vector<string> &albumId)
{
    const vector<string> columns = {
        "Distinct " + PhotoColumn::PHOTO_OWNER_ALBUM_ID
    };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_WARN_LOG("Failed to Query");
        return;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        albumId.push_back(to_string(GetIntValFromColumn(resultSet, 0)));
    }
}

static int32_t UpdateUserAlbumIfNeeded(const shared_ptr<MediaLibraryRdbStore> rdbStore, const UpdateAlbumData &data,
    const bool hiddenState, std::shared_ptr<TransactionOperations> trans)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumIfNeeded");
    if (trans == nullptr) {
        MEDIA_ERR_LOG("transactionOprn is null");
        return E_HAS_DB_ERROR;
    }
    ValuesBucket values;
    int err = SetUpdateValues(rdbStore, data, values, static_cast<PhotoAlbumSubType>(0), hiddenState);
    if (err < 0) {
        MEDIA_ERR_LOG(
            "Failed to set update values when updating albums, album id: %{public}d, hidden state: %{public}d",
            data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(data.albumId));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    int32_t changedRows = 0;
    err = trans->Update(changedRows, values, predicates);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update album count and cover! album id: %{public}d, hidden state: %{public}d",
            data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    return E_SUCCESS;
}

static int32_t UpdatePortraitAlbumIfNeeded(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const UpdateAlbumData &data, const vector<string> &fileIds, std::shared_ptr<TransactionOperations> trans)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePortraitAlbumIfNeeded");
    if (trans == nullptr) {
        MEDIA_ERR_LOG("transactionOprn is null");
        return E_HAS_DB_ERROR;
    }
    auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
    if (subtype != PhotoAlbumSubType::PORTRAIT) {
        return E_SUCCESS;
    }

    ValuesBucket values;
    int32_t albumId = data.albumId;
    int setRet = SetPortraitUpdateValues(rdbStore, data, fileIds, values);
    if (setRet != E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to set portrait album update values! album id: %{public}d, err: %{public}d", albumId,
            setRet);
        return setRet;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    int32_t changedRows = 0;
    int updateRet = trans->Update(changedRows, values, predicates);
    if (updateRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update album count and cover! album id: %{public}d, err: %{public}d", albumId,
            updateRet);
        return updateRet;
    }
    return E_SUCCESS;
}

static int32_t UpdateAnalysisAlbumIfNeeded(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const UpdateAlbumData &data, const bool hiddenState, std::shared_ptr<TransactionOperations> trans = nullptr)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumIfNeeded");
    ValuesBucket values;
    auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
    int err = SetUpdateValues(rdbStore, data, values, subtype, hiddenState);
    if (err < 0) {
        MEDIA_ERR_LOG(
            "Failed to set update values when updating albums, album id: %{public}d, hidden state: %{public}d",
                data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(data.albumId));
    int32_t changedRows = 0;
    if (trans == nullptr) {
        err = rdbStore->Update(changedRows, values, predicates);
    } else {
        err = trans->Update(changedRows, values, predicates);
    }

    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update album count and cover! album id: %{public}d, hidden state: %{public}d",
            data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    return E_SUCCESS;
}

static int32_t UpdateSourceAlbumIfNeeded(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const UpdateAlbumData &data, const bool hiddenState, std::shared_ptr<TransactionOperations> trans)
{
    if (trans == nullptr) {
        MEDIA_ERR_LOG("transactionOprn is null");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSourceAlbumIfNeeded");
    ValuesBucket values;
    auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
    int err = SetUpdateValues(rdbStore, data, values, subtype, hiddenState);
    if (err < 0) {
        MEDIA_ERR_LOG(
            "Failed to set update values when updating albums, album id: %{public}d, hidden state: %{public}d",
                data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(data.albumId));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SOURCE_GENERIC));
    int32_t changedRows = 0;
    err = trans->Update(changedRows, values, predicates);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update album count and cover! album id: %{public}d, hidden state: %{public}d",
            data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    return E_SUCCESS;
}

static int32_t UpdateSysAlbumIfNeeded(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const UpdateAlbumData &data,
    const bool hiddenState, std::shared_ptr<TransactionOperations> trans)
{
    if (trans == nullptr) {
        MEDIA_ERR_LOG("transactionOprn is null");
        return E_HAS_DB_ERROR;
    }
    auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSysAlbum: " + to_string(subtype));
    ValuesBucket values;
    int err = SetUpdateValues(rdbStore, data, values, subtype, hiddenState);
    if (err < 0) {
        MEDIA_ERR_LOG(
            "Failed to set update values when updating albums, album id: %{public}d, hidden state: %{public}d",
                data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    if (values.IsEmpty()) {
        return E_SUCCESS;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subtype));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(data.albumId));
    int32_t changedRows = 0;
    err = trans->Update(changedRows, values, predicates);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update album count and cover! album id: %{public}d, hidden state: %{public}d",
            data.albumId, hiddenState ? 1 : 0);
        return err;
    }
    return E_SUCCESS;
}

static void UpdateUserAlbumHiddenState(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &userAlbumIds = {})
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumHiddenState");
    auto albumResult = GetUserAlbum(rdbStore, userAlbumIds, {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::CONTAINS_HIDDEN,
        PhotoAlbumColumns::HIDDEN_COUNT,
        PhotoAlbumColumns::HIDDEN_COVER,
    });
    if (albumResult == nullptr) {
        return;
    }
    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(0);
        data.hiddenCount = GetAlbumCount(albumResult, PhotoAlbumColumns::HIDDEN_COUNT);
        data.hiddenCover = GetAlbumCover(albumResult, PhotoAlbumColumns::HIDDEN_COVER);
        datas.push_back(data);
    }
    albumResult->Close();

    ForEachRow(rdbStore, datas, true, UpdateUserAlbumIfNeeded);
}

static bool CopyAssetIfNeed(int32_t fileId, int32_t albumId,
    const shared_ptr<MediaLibraryRdbStore> rdbStore, vector<int32_t> &updateIds)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    vector<string> columns;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        return false;
    }
    bool needCopy = true;
    int64_t newAssetId = -1;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        auto albumIdQuery = GetIntValFromColumn(resultSet, PhotoColumn::PHOTO_OWNER_ALBUM_ID);
        if (albumIdQuery == albumId) {
            needCopy = false;
            updateIds.push_back(fileId);
        } else {
            needCopy = true;
            MEDIA_DEBUG_LOG("add assets: need copy assets id is: %{public}s", to_string(fileId).c_str());
            MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(rdbStore, fileId, albumId, newAssetId);
            updateIds.push_back(newAssetId);
        }
    }
    return needCopy;
}

void MediaLibraryRdbUtils::UpdateUserAlbumByUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumByUri");

    if (uris.size() == 0) {
        UpdateUserAlbumInternal(rdbStore);
        UpdateUserAlbumHiddenState(rdbStore);
    }
    vector<string> albumIds;
    string idArgs;
    for (size_t i = 0; i < uris.size(); i++) {
        string fileId = GetPhotoId(uris[i]);
        if (fileId.size() > 0) {
            idArgs.append("'").append(fileId).append("'").append(",");
        }
        if ((i == 0 || i % ALBUM_UPDATE_THRESHOLD != 0) && i < uris.size() - 1) {
            continue;
        }
        if (idArgs.size() == 0) {
            continue;
        }
        idArgs = idArgs.substr(0, idArgs.size() - 1);
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.SetWhereClause(PhotoColumn::MEDIA_ID + " in(" + idArgs + ") and " +
            PhotoColumn::PHOTO_OWNER_ALBUM_ID + " in(select " + PhotoAlbumColumns::ALBUM_ID + " from " +
            PhotoAlbumColumns::TABLE + " where " + PhotoAlbumColumns::ALBUM_TYPE + " = " +
            to_string(PhotoAlbumType::USER) + ")");
        QueryAlbumId(rdbStore, predicates, albumIds);
        idArgs.clear();
    }
    if (albumIds.size() > 0) {
        UpdateUserAlbumInternal(rdbStore, albumIds);
        UpdateUserAlbumHiddenState(rdbStore, albumIds);
    }
}

void MediaLibraryRdbUtils::UpdateUserAlbumInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
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

    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(0);
        data.albumCoverUri = GetAlbumCover(albumResult, PhotoAlbumColumns::ALBUM_COVER_URI);
        data.albumCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_COUNT);
        data.albumImageCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        data.albumVideoCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        datas.push_back(data);
    }
    albumResult->Close();
    ForEachRow(rdbStore, datas, false, UpdateUserAlbumIfNeeded);
}

static int32_t GetIntFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, int &value)
{
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int index = -1;
    resultSet->GetColumnIndex(column, index);
    if (index == -1) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetInt(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t GetStringFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, string &value)
{
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int index = -1;
    resultSet->GetColumnIndex(column, index);
    if (index == -1) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetString(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryRdbUtils::UpdateTrashedAssetOnAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    RdbPredicates &predicates)
{
    vector<string> newWhereIdArgs;
    for (auto albumId: predicates.GetWhereArgs()) {
        const std::string QUERY_FILE_ASSET_INFO = "SELECT * FROM Photos WHERE owner_album_id = " + albumId +
            " AND clean_flag =0 AND hidden =0";
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_FILE_ASSET_INFO);
        vector<string> fileAssetsIds, fileAssetsUri;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int isHiddenAsset = 0;
            GetIntFromResultSet(resultSet, MediaColumn::MEDIA_HIDDEN, isHiddenAsset);
            if (isHiddenAsset == 1) {
                continue;
            }
            int32_t fileId = -1;
            string assetData;
            GetIntFromResultSet(resultSet, MediaColumn::MEDIA_ID, fileId);
            GetStringFromResultSet(resultSet, MediaColumn::MEDIA_FILE_PATH, assetData);
            fileAssetsIds.push_back(to_string(fileId));
            string uri = MediaLibraryFormMapOperations::GetUriByFileId(fileId, assetData);
            fileAssetsUri.push_back(uri);
        }
        MediaLibraryPhotoOperations::UpdateSourcePath(fileAssetsIds);
        RdbPredicates predicatesPhotos(PhotoColumn::PHOTOS_TABLE);
        predicatesPhotos.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
        predicatesPhotos.And()->In(MediaColumn::MEDIA_ID, fileAssetsIds);
        ValuesBucket values;
        values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
        int32_t updateRow = -1;
        rdbStore->Update(updateRow, values, predicatesPhotos);
        if (updateRow < 0) {
            continue;
        }
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, {
            to_string(PhotoAlbumSubType::IMAGE), to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::FAVORITE), to_string(PhotoAlbumSubType::TRASH),
            to_string(PhotoAlbumSubType::HIDDEN)
        });
        MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, fileAssetsUri);
        newWhereIdArgs.push_back(albumId);
        MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
            static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), fileAssetsUri);
        MediaLibraryPhotoOperations::TrashPhotosSendNotify(fileAssetsUri);
    }
    predicates.SetWhereArgs(newWhereIdArgs);
    return newWhereIdArgs.size();
}

int32_t MediaLibraryRdbUtils::UpdateRemovedAssetToTrash(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &whereIdArgs)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    int32_t updateRows = 0;
    RdbPredicates predicatesPhotos(PhotoColumn::PHOTOS_TABLE);
    predicatesPhotos.In(MediaColumn::MEDIA_ID, whereIdArgs);
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    rdbStore->Update(updateRows, values, predicatesPhotos);
    if (updateRows <= 0) {
        MEDIA_ERR_LOG("Failed to remove assets, updateRows: %{public}d", updateRows);
        return E_HAS_DB_ERROR;
    }
    return updateRows;
}

int32_t MediaLibraryRdbUtils::UpdateOwnerAlbumId(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<DataShare::DataShareValuesBucket> &values, vector<int32_t> &updateIds)
{
    vector<string> whereIdArgs;
    int32_t updateRows = 0;
    bool isValid = false;
    int32_t albumId = values[0].Get(PhotoColumn::PHOTO_OWNER_ALBUM_ID, isValid);
    for (const auto &value : values) {
        bool isValidNew = false;
        std::string assetUri = value.Get(MediaColumn::MEDIA_ID, isValidNew);
        if (!MediaFileUtils::StartsWith(assetUri, PhotoColumn::PHOTO_URI_PREFIX)) {
            continue;
        }
        auto photoId = std::stoi(MediaFileUri::GetPhotoId(assetUri));
        if (CopyAssetIfNeed(photoId, albumId, rdbStore, updateIds)) {
            updateRows++;
            continue;
        }
        whereIdArgs.push_back(MediaFileUri::GetPhotoId(assetUri));
    }
    if (whereIdArgs.empty()) {
        MEDIA_INFO_LOG("add assets: no need copy assets is 0 for update owner album id");
        return updateRows;
    }
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, whereIdArgs);
    ValuesBucket updateValues;
    updateValues.PutString(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    int32_t changedRowsNoNeedCopy = 0;
    int err = rdbStore->Update(changedRowsNoNeedCopy, updateValues, updatePredicates);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to update owner album id");
    }
    return updateRows + changedRowsNoNeedCopy;
}

static void QueryAnalysisAlbumId(const shared_ptr<MediaLibraryRdbStore> rdbStore, const RdbPredicates predicates,
    vector<string> &albumId)
{
    const vector<string> columns = {
        "Distinct " + PhotoMap::ALBUM_ID
    };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_WARN_LOG("Failed to Query Analysis Album ID");
        return;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        albumId.push_back(to_string(GetIntValFromColumn(resultSet, 0)));
    }
}

void MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumByUri");

    if (uris.size() == 0) {
        UpdateAnalysisAlbumInternal(rdbStore);
    }
    vector<string> albumIds;
    vector<string> idArgs;
    vector<string> fileIds;
    for (size_t i = 0; i < uris.size(); i++) {
        string fileId = GetPhotoId(uris[i]);
        if (fileId.size() > 0) {
            idArgs.push_back(fileId);
            fileIds.push_back(fileId);
        }
        if (idArgs.size() == ALBUM_UPDATE_THRESHOLD || i == uris.size() - 1) {
            RdbPredicates predicates(ANALYSIS_PHOTO_MAP_TABLE);
            predicates.In(PhotoMap::ASSET_ID, idArgs);
            QueryAnalysisAlbumId(rdbStore, predicates, albumIds);
            idArgs.clear();
        }
    }
    if (albumIds.size() > 0) {
        UpdateAnalysisAlbumInternal(rdbStore, albumIds, fileIds);
    }
}

int32_t MediaLibraryRdbUtils::GetAlbumIdsForPortrait(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    vector<string> &portraitAlbumIds)
{
    std::stringstream labelIds;
    unordered_set<string> resultAlbumIds;
    for (size_t i = 0; i < portraitAlbumIds.size(); i++) {
        labelIds << portraitAlbumIds[i];
        if (i != portraitAlbumIds.size() - 1) {
            labelIds << ",";
        }
        resultAlbumIds.insert(portraitAlbumIds[i]);
    }

    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.SetWhereClause(GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " IN (" + labelIds.str() + ") AND " + ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) +")");
    vector<string> columns = {
        ALBUM_ID,
    };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string albumId = to_string(GetIntValFromColumn(resultSet, ALBUM_ID));
        if (resultAlbumIds.find(albumId) == resultAlbumIds.end()) {
            resultAlbumIds.insert(albumId);
            portraitAlbumIds.push_back(albumId);
        }
    }
    return E_OK;
}

int32_t MediaLibraryRdbUtils::GetAlbumSubtypeArgument(const RdbPredicates &predicates)
{
    string whereClause = predicates.GetWhereClause();
    vector<string> whereArgs = predicates.GetWhereArgs();
    size_t subtypePos = whereClause.find(PhotoAlbumColumns::ALBUM_SUBTYPE + " = ?");
    if (subtypePos == string::npos) {
        return E_ERR;
    }
    size_t argsIndex = 0;
    for (size_t i = 0; i < subtypePos; i++) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        return E_ERR;
    }
    const string &subtype = whereArgs[argsIndex];
    if (subtype.empty() || !MediaLibraryDataManagerUtils::IsNumber(subtype)) {
        return E_ERR;
    }
    return std::stoi(subtype);
}

static void GetUpdateAlbumDataInfo(shared_ptr<ResultSet> albumResult, std::vector<UpdateAlbumData> &datas)
{
    if (albumResult == nullptr) {
        MEDIA_ERR_LOG("albumResult is nullptr");
        return;
    }
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
        data.albumCoverUri = GetAlbumCover(albumResult, PhotoAlbumColumns::ALBUM_COVER_URI);
        data.albumCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_COUNT);
        data.isCoverSatisfied = GetIsCoverSatisfied(albumResult);
        datas.push_back(data);
    }
}

void MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &anaAlbumAlbumIds, const vector<string> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumInternal");
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_COVER_URI, PhotoAlbumColumns::ALBUM_COUNT,
        IS_COVER_SATISFIED };
    vector<string> tempAlbumId = anaAlbumAlbumIds;
    if (tempAlbumId.size() > 0) {
        GetAlbumIdsForPortrait(rdbStore, tempAlbumId);
    }
    auto albumResult = GetAnalysisAlbum(rdbStore, tempAlbumId, columns);
    if (albumResult == nullptr) {
        MEDIA_ERR_LOG("Failed to get Analysis Album");
        return;
    }
    std::vector<UpdateAlbumData> datas;
    GetUpdateAlbumDataInfo(albumResult, datas);
    albumResult->Close();

    // For each row
    int32_t err = NativeRdb::E_OK;
    for (auto data : datas) {
        int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        std::function<int(void)> func = [&]()->int {
            auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
            if (subtype == PhotoAlbumSubType::PORTRAIT) {
                UpdatePortraitAlbumIfNeeded(rdbStore, data, fileIds, trans);
            } else if (subtype == PhotoAlbumSubType::GROUP_PHOTO) {
                MEDIA_INFO_LOG("No need to update group photo");
            } else {
                UpdateAnalysisAlbumIfNeeded(rdbStore, data, false, trans);
            }
            return err;
        };
        err = trans->RetryTrans(func);
        if (err != E_OK) {
            MEDIA_ERR_LOG("UpdateAnalysisAlbumInternal: tans finish fail!, ret:%{public}d", err);
        }
        int32_t costTime = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - start);
        if (costTime > UPDATE_ALBUM_TIME_OUT) {
            MEDIA_INFO_LOG("udpate analysis album: %{public}d cost %{public}d", data.albumId, costTime);
        }
    }
}

void MediaLibraryRdbUtils::UpdateAnalysisAlbumByFile(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &fileIds, const vector<int> &albumTypes)
{
    if (fileIds.empty()) {
        MEDIA_ERR_LOG("Failed to UpdateAnalysisAlbumByFile cause fileIds empty");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumByFile");
    vector<string> columns = {
        PhotoMap::ALBUM_ID,
        PhotoMap::ASSET_ID,
    };
    RdbPredicates predicates(ANALYSIS_PHOTO_MAP_TABLE);
    if (!albumTypes.empty()) {
        std::string files;
        for (std::string fileId : fileIds) {
            files.append("'").append(fileId).append("'").append(",");
        }
        files = files.substr(0, files.length() - 1);
        std::string subTypes;
        for (int subtype : albumTypes) {
            subTypes.append(to_string(subtype)).append(",");
        }
        subTypes = subTypes.substr(0, subTypes.length() - 1);
        predicates.SetWhereClause(PhotoMap::ASSET_ID + " in(" + files + ") and " + PhotoMap::ALBUM_ID +
            " in(select album_id from AnalysisAlbum where album_subtype in(" + subTypes + "))");
    } else {
        predicates.In(PhotoMap::ASSET_ID, fileIds);
    }
    shared_ptr<ResultSet> mapResult = rdbStore->Query(predicates, columns);
    if (mapResult == nullptr) {
        MEDIA_ERR_LOG("Failed query AnalysisAlbum");
        return;
    }
    vector<string> albumIds;
    while (mapResult->GoToNextRow() == E_OK) {
        albumIds.push_back(to_string(GetIntValFromColumn(mapResult, PhotoMap::ALBUM_ID)));
    }
    int err = E_HAS_DB_ERROR;
    int32_t deletedRows = 0;
    err = rdbStore->Delete(deletedRows, predicates);
    if (err != E_OK || deletedRows <= 0) {
        MEDIA_ERR_LOG("Failed Delete AnalysisPhotoMap");
        return;
    }
    UpdateAnalysisAlbumInternal(rdbStore, albumIds, fileIds);
}

static void UpdateSourceAlbumHiddenState(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &sourceAlbumIds = {})
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSourceAlbumHiddenState");

    auto albumResult = GetSourceAlbum(rdbStore, sourceAlbumIds, {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::CONTAINS_HIDDEN,
        PhotoAlbumColumns::HIDDEN_COUNT,
        PhotoAlbumColumns::HIDDEN_COVER,
    });
    if (albumResult == nullptr) {
        return;
    }
    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
        data.hiddenCount = GetAlbumCount(albumResult, PhotoAlbumColumns::HIDDEN_COUNT);
        data.hiddenCover = GetAlbumCover(albumResult, PhotoAlbumColumns::HIDDEN_COVER);
        datas.push_back(data);
    }
    albumResult->Close();
    ForEachRow(rdbStore, datas, true, UpdateSourceAlbumIfNeeded);
}

void MediaLibraryRdbUtils::UpdateSourceAlbumByUri(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSourceAlbumByUri");

    if (uris.size() == 0) {
        UpdateSourceAlbumInternal(rdbStore);
        UpdateSourceAlbumHiddenState(rdbStore);
    }
    vector<string> albumIds;
    string idArgs;
    for (size_t i = 0; i < uris.size(); i++) {
        string fileId = GetPhotoId(uris[i]);
        if (fileId.size() > 0) {
            idArgs.append("'").append(fileId).append("'").append(",");
        }
        if ((i == 0 || i % ALBUM_UPDATE_THRESHOLD != 0) && i < uris.size() - 1) {
            continue;
        }
        if (idArgs.size() == 0) {
            continue;
        }
        idArgs = idArgs.substr(0, idArgs.size() - 1);
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.SetWhereClause(PhotoColumn::MEDIA_ID + " in(" + idArgs + ") and " +
            PhotoColumn::PHOTO_OWNER_ALBUM_ID + " in(select " + PhotoAlbumColumns::ALBUM_ID + " from " +
            PhotoAlbumColumns::TABLE + " where " + PhotoAlbumColumns::ALBUM_TYPE + " = " +
            to_string(PhotoAlbumType::SOURCE) + ")");
        QueryAlbumId(rdbStore, predicates, albumIds);
        idArgs.clear();
    }
    if (albumIds.size() > 0) {
        UpdateSourceAlbumInternal(rdbStore, albumIds);
        UpdateSourceAlbumHiddenState(rdbStore, albumIds);
    }
}

void MediaLibraryRdbUtils::UpdateSourceAlbumInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &sourceAlbumIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSourceAlbumInternal");

    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
    };
    auto albumResult = GetSourceAlbum(rdbStore, sourceAlbumIds, columns);
    if (albumResult == nullptr) {
        return;
    }
    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
        data.albumCoverUri = GetAlbumCover(albumResult, PhotoAlbumColumns::ALBUM_COVER_URI);
        data.albumCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_COUNT);
        data.albumImageCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        data.albumVideoCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        datas.push_back(data);
    }
    albumResult->Close();

    ForEachRow(rdbStore, datas, false, UpdateSourceAlbumIfNeeded);
}

static inline shared_ptr<ResultSet> GetSystemAlbum(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &subtypes, const vector<string> &columns)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (subtypes.empty()) {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, ALL_SYS_PHOTO_ALBUM);
    } else {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, subtypes);
    }
    return rdbStore->QueryWithFilter(predicates, columns);
}

void MediaLibraryRdbUtils::UpdateSystemAlbumInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
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
    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
        data.albumCoverUri = GetAlbumCover(albumResult, PhotoAlbumColumns::ALBUM_COVER_URI);
        data.albumCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_COUNT);
        data.albumImageCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        data.albumVideoCount = GetAlbumCount(albumResult, PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        datas.push_back(data);
    }
    albumResult->Close();
    ForEachRow(rdbStore, datas, false, UpdateSysAlbumIfNeeded);
}

void MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &subtypes)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSysAlbumHiddenState");

    shared_ptr<ResultSet> albumResult = nullptr;

    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::CONTAINS_HIDDEN,
        PhotoAlbumColumns::HIDDEN_COUNT,
        PhotoAlbumColumns::HIDDEN_COVER,
    };

    if (subtypes.empty()) {
        albumResult = GetSystemAlbum(rdbStore, {
            to_string(PhotoAlbumSubType::IMAGE),
            to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::FAVORITE),
            to_string(PhotoAlbumSubType::SCREENSHOT),
            to_string(PhotoAlbumSubType::CAMERA),
            to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
        }, columns);
    } else {
        albumResult = GetSystemAlbum(rdbStore, subtypes, columns);
    }

    if (albumResult == nullptr) {
        return;
    }
    std::vector<UpdateAlbumData> datas;
    while (albumResult->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(albumResult);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(albumResult));
        data.hiddenCount = GetAlbumCount(albumResult, PhotoAlbumColumns::HIDDEN_COUNT);
        data.hiddenCover = GetAlbumCover(albumResult, PhotoAlbumColumns::HIDDEN_COVER);
        datas.push_back(data);
    }
    albumResult->Close();

    ForEachRow(rdbStore, datas, true, UpdateSysAlbumIfNeeded);
}

void MediaLibraryRdbUtils::UpdateAllAlbums(shared_ptr<MediaLibraryRdbStore> rdbStore, const vector<string> &uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAllAlbums");
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore, try again!");
        rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        if (rdbStore == nullptr) {
            MEDIA_ERR_LOG("Fatal error! Failed to get rdbstore, new cloud data is not processed!!");
            return;
        }
    }

    vector<string> systemAlbumsExcludeSource = {
        to_string(PhotoAlbumSubType::FAVORITE),
        to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::HIDDEN),
        to_string(PhotoAlbumSubType::TRASH),
        to_string(PhotoAlbumSubType::IMAGE),
        to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
    };
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, systemAlbumsExcludeSource);
    MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(rdbStore);
    MediaLibraryRdbUtils::UpdateUserAlbumByUri(rdbStore, uris);
    MediaLibraryRdbUtils::UpdateSourceAlbumByUri(rdbStore, uris);
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, uris);
}

static int32_t UpdateAlbumReplacedSignal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &albumIdVector)
{
    if (albumIdVector.empty()) {
        return E_SUCCESS;
    }

    ValuesBucket refreshValues;
    string insertRefreshTableSql = "INSERT OR IGNORE INTO " + ALBUM_REFRESH_TABLE + " VALUES ";
    for (size_t i = 0; i < albumIdVector.size(); ++i) {
        if (i != albumIdVector.size() - 1) {
            insertRefreshTableSql += "(" + albumIdVector[i] + "), ";
        } else {
            insertRefreshTableSql += "(" + albumIdVector[i] + ");";
        }
    }
    MEDIA_DEBUG_LOG("output insertRefreshTableSql:%{public}s", insertRefreshTableSql.c_str());

    int32_t ret = rdbStore->ExecuteSql(insertRefreshTableSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not insert refreshed table, ret:%{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    return E_SUCCESS;
}

static int32_t UpdateBussinessRecord(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<BussinessRecordValue> &updateValue)
{
    if (updateValue.empty()) {
        return E_SUCCESS;
    }

    ValuesBucket refreshValues;
    string insertTableSql = "INSERT OR IGNORE INTO " + MedialibraryBusinessRecordColumn::TABLE + "(" +
        MedialibraryBusinessRecordColumn::BUSINESS_TYPE + "," + MedialibraryBusinessRecordColumn::KEY + "," +
        MedialibraryBusinessRecordColumn::VALUE + ") VALUES ";
    for (size_t i = 0; i < updateValue.size(); ++i) {
        if (i != updateValue.size() - 1) {
            insertTableSql += "('" + updateValue[i].bussinessType + "', '" + updateValue[i].key + "', '" +
                updateValue[i].value + "'), ";
        } else {
            insertTableSql += "('" + updateValue[i].bussinessType + "', '" + updateValue[i].key + "', '" +
                updateValue[i].value + "');";
        }
    }
    MEDIA_DEBUG_LOG("output insertTableSql:%{public}s", insertTableSql.c_str());

    int32_t ret = rdbStore->ExecuteSql(insertTableSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not insert bussinessRecord table, ret:%{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    return E_SUCCESS;
}

void MediaLibraryRdbUtils::UpdateSystemAlbumCountInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &subtypes)
{
    // Only use in dfs
    MediaLibraryTracer tracer;
    tracer.Start("UpdateSystemAlbumCountInternal");

    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    auto albumResult = GetSystemAlbum(rdbStore, subtypes, columns);
    if (albumResult == nullptr) {
        return;
    }

    vector<string> replaceSignalAlbumVector;
    while (albumResult->GoToNextRow() == NativeRdb::E_OK) {
        int32_t ret = GetIntValFromColumn(albumResult, PhotoAlbumColumns::ALBUM_ID);
        if (ret <= 0) {
            MEDIA_WARN_LOG("Can not get ret:%{public}d", ret);
        } else {
            replaceSignalAlbumVector.push_back(to_string(ret));
        }
    }
    if (!replaceSignalAlbumVector.empty()) {
        int32_t ret = UpdateAlbumReplacedSignal(rdbStore, replaceSignalAlbumVector);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Update sysalbum replaced signal failed ret:%{public}d", ret);
        }
    }
    // Do not call SetNeedRefreshAlbum in this function
    // This is set by the notification from dfs
    // and is set by the media library observer after receiving the notification
}

void MediaLibraryRdbUtils::UpdateUserAlbumCountInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &userAlbumIds)
{
    // only use in dfs
    MediaLibraryTracer tracer;
    tracer.Start("UpdateUserAlbumCountInternal");

    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    auto albumResult = GetUserAlbum(rdbStore, userAlbumIds, columns);
    if (albumResult == nullptr) {
        return;
    }

    vector<string> replaceSignalAlbumVector;
    while (albumResult->GoToNextRow() == NativeRdb::E_OK) {
        int32_t ret = GetIntValFromColumn(albumResult, PhotoAlbumColumns::ALBUM_ID);
        if (ret <= 0) {
            MEDIA_WARN_LOG("Can not get ret:%{public}d", ret);
        } else {
            replaceSignalAlbumVector.push_back(to_string(ret));
        }
    }
    if (!replaceSignalAlbumVector.empty()) {
        int32_t ret = UpdateAlbumReplacedSignal(rdbStore, replaceSignalAlbumVector);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Update user album replaced signal failed ret:%{public}d", ret);
            return;
        }
    }
    // Do not call SetNeedRefreshAlbum in this function
    // This is set by the notification from dfs
    // and is set by the media library observer after receiving the notification
}

void MediaLibraryRdbUtils::UpdateAnalysisAlbumCountInternal(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const vector<string> &subtypes)
{
    // only use in dfs
    MediaLibraryTracer tracer;
    tracer.Start("UpdateAnalysisAlbumCountInternal");

    vector<string> columns = { ALBUM_ID, ALBUM_SUBTYPE };
    auto albumResult = GetAnalysisAlbumBySubtype(rdbStore, subtypes, columns);
    if (albumResult == nullptr) {
        return;
    }

    vector<BussinessRecordValue> updateAlbumIdList;
    while (albumResult->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = GetIntValFromColumn(albumResult, ALBUM_ID);
        int32_t subtype = GetIntValFromColumn(albumResult, ALBUM_SUBTYPE);
        if (albumId <= 0) {
            MEDIA_WARN_LOG("Can not get ret:%{public}d", albumId);
        } else {
            updateAlbumIdList.push_back({ ANALYSIS_REFRESH_BUSINESS_TYPE, to_string(subtype), to_string(albumId) });
        }
    }
    if (!updateAlbumIdList.empty()) {
        int32_t ret = UpdateBussinessRecord(rdbStore, updateAlbumIdList);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Update sysalbum replaced signal failed ret:%{public}d", ret);
        }
    }
    // Do not call SetNeedRefreshAlbum in this function
    // This is set by the notification from dfs
    // and is set by the media library observer after receiving the notification
}

int RefreshPhotoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler)
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<RefreshAlbumData> datas;
    int ret = GetAllRefreshAlbumIds(rdbStore, datas);
    if (ret != E_SUCCESS) {
        MEDIA_ERR_LOG("Failed to get refresh album ids");
        return ret;
    }

    ret = RefreshAlbums(rdbStore, datas, refreshProcessHandler);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("%{public}d photo albums update cost %{public}ld", (int)datas.size(), (long)(end - start));
    return ret;
}

static int32_t RefreshAnalysisAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<UpdateAlbumData> &datas,
    function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler,
    const vector<string> &subtypes)
{
    for (auto data : datas) {
        int ret = UpdateAnalysisAlbumIfNeeded(rdbStore, data, false);
        if (ret != E_SUCCESS) {
            MEDIA_ERR_LOG("UpdateAnalysisAlbumIfNeeded fail");
            return E_HAS_DB_ERROR;
        }
        auto subtype = static_cast<PhotoAlbumSubType>(data.albumSubtype);
        int32_t albumId = data.albumId;
        refreshProcessHandler(PhotoAlbumType::SMART, subtype, albumId);
    }

    string deleteRefreshTableSql = "DELETE FROM " + MedialibraryBusinessRecordColumn::TABLE + " WHERE " +
        MedialibraryBusinessRecordColumn::BUSINESS_TYPE + " = '" + ANALYSIS_REFRESH_BUSINESS_TYPE + "'";
    int32_t ret = rdbStore->ExecuteSql(deleteRefreshTableSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to execute sql:%{private}s", deleteRefreshTableSql.c_str());
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("Delete RefreshAnalysisAlbums success");
    return E_SUCCESS;
}

static int32_t GetRefreshAnalysisAlbumIds(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    vector<string> &albumIds, const vector<string> &subtypes)
{
    RdbPredicates predicates(MedialibraryBusinessRecordColumn::TABLE);
    if (!subtypes.empty()) {
        predicates.In(MedialibraryBusinessRecordColumn::KEY, subtypes);
    } else {
        predicates.In(MedialibraryBusinessRecordColumn::KEY, ALL_ANALYSIS_ALBUM);
    }
    predicates.EqualTo(MedialibraryBusinessRecordColumn::BUSINESS_TYPE, ANALYSIS_REFRESH_BUSINESS_TYPE);

    vector<string> columns = { MedialibraryBusinessRecordColumn::VALUE };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not query ALBUM_REFRESH_TABLE");
        return E_HAS_DB_ERROR;
    }

    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetRowCount failed ret:%{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    if (count == 0) {
        MEDIA_DEBUG_LOG("count is zero, break");
        return E_SUCCESS;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t columnIndex = 0;
        ret = resultSet->GetColumnIndex(MedialibraryBusinessRecordColumn::VALUE, columnIndex);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("GetColumnIndex failed ret:%{public}d", ret);
            return E_HAS_DB_ERROR;
        }
        string refreshAlbumId;
        ret = resultSet->GetString(columnIndex, refreshAlbumId);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("GetString failed ret:%{public}d", ret);
            return E_HAS_DB_ERROR;
        }
        albumIds.push_back(refreshAlbumId);
    }
    return E_SUCCESS;
}

int RefreshAnalysisPhotoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler, const vector<string> &subtypes)
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    vector<string> albumIds;
    int ret = GetRefreshAnalysisAlbumIds(rdbStore, albumIds, subtypes);
    if (ret != E_SUCCESS) {
        return ret;
    }
    if (albumIds.empty()) {
        MEDIA_DEBUG_LOG("albumIds is empty");
        return E_EMPTY_ALBUM_ID;
    }
    vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
    };
    auto resultSet = GetAnalysisAlbum(rdbStore, albumIds, columns);
    if (resultSet == nullptr) {
        ret = E_HAS_DB_ERROR;
        return E_HAS_DB_ERROR;
    }
    std::vector<UpdateAlbumData> datas;
    while (resultSet->GoToNextRow() == E_OK) {
        UpdateAlbumData data;
        data.albumId = GetAlbumId(resultSet);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetAlbumSubType(resultSet));
        data.albumCoverUri = GetAlbumCover(resultSet, PhotoAlbumColumns::ALBUM_COVER_URI);
        data.albumCount = GetAlbumCount(resultSet, PhotoAlbumColumns::ALBUM_COUNT);
        datas.push_back(data);
    }
    resultSet->Close();

    ret = RefreshAnalysisAlbums(rdbStore, datas, refreshProcessHandler, subtypes);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("%{public}d analysis albums update cost %{public}ld", (int)albumIds.size(), (long)(end - start));
    return ret;
}

static bool IsRefreshAlbumEmpty(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(ALBUM_REFRESH_TABLE);
    vector<string> columns = { MEDIA_COLUMN_COUNT_1 };
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not query ALBUM_REFRESH_TABLE");
        return true;
    }
    int32_t count = GetFileCount(resultSet);
    MEDIA_DEBUG_LOG("RefreshAllAlbuming remain count:%{public}d", count);
    return count <= 0;
}

int32_t MediaLibraryRdbUtils::RefreshAllAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    function<void(PhotoAlbumType, PhotoAlbumSubType, int)> refreshProcessHandler, function<void()> refreshCallback)
{
    unique_lock<mutex> lock(sRefreshAlbumMutex_);
    if (IsInRefreshTask()) {
        lock.unlock();
        MEDIA_DEBUG_LOG("RefreshAllAlbuming, quit");
        return E_OK;
    }
    isInRefreshTask = true;
    lock.unlock();

    MediaLibraryTracer tracer;
    tracer.Start("RefreshAllAlbums");

    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdb");
        return E_HAS_DB_ERROR;
    }

    int ret = E_SUCCESS;
    bool isRefresh = false;
    while (IsNeedRefreshAlbum() || !IsRefreshAlbumEmpty(rdbStore)) {
        SetNeedRefreshAlbum(false);
        ret = RefreshPhotoAlbums(rdbStore, refreshProcessHandler);
        if (ret == E_EMPTY_ALBUM_ID) {
            ret = E_SUCCESS;
            continue;
        }
        if (ret != E_SUCCESS) {
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(PowerEfficiencyManager::GetAlbumUpdateInterval()));
        isRefresh = true;
    }

    if (ret != E_SUCCESS) {
        // refresh failed and set flag, try to refresh next time
        SetNeedRefreshAlbum(true);
    } else {
        // refresh task is successful
        SetNeedRefreshAlbum(false);
    }
    isInRefreshTask = false;
    if (isRefresh) {
        refreshCallback();
    }

    return ret;
}

void MediaLibraryRdbUtils::UpdateAllAlbumsForCloud(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    // 注意，端云同步代码仓也有相同函数，添加新相册时，请通知端云同步进行相应修改
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore);
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore);
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore);
}

void MediaLibraryRdbUtils::UpdateAllAlbumsCountForCloud(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    // 注意，端云同步代码仓也有相同函数，添加新相册时，请通知端云同步进行相应修改
    MediaLibraryRdbUtils::UpdateSystemAlbumCountInternal(rdbStore);
    MediaLibraryRdbUtils::UpdateUserAlbumCountInternal(rdbStore);
    vector<string> subtype = { "4101" };
    MediaLibraryRdbUtils::UpdateAnalysisAlbumCountInternal(rdbStore, subtype);
}

void MediaLibraryRdbUtils::AddQueryIndex(AbsPredicates& predicates, const vector<string>& columns)
{
    auto it = find(columns.begin(), columns.end(), MEDIA_COLUMN_COUNT);
    if (it == columns.end()) {
        return;
    }
    const string &group = predicates.GetGroup();
    if (group.empty()) {
        predicates.GroupBy({ PhotoColumn::PHOTO_DATE_DAY });
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_DAY_INDEX);
        return;
    }
    if (group == PhotoColumn::MEDIA_TYPE) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_MEDIA_TYPE_INDEX);
        return;
    }
    if (group == PhotoColumn::PHOTO_DATE_DAY) {
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_DAY_INDEX);
        return;
    }
}

void MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(vector<string>& columns)
{
    vector<string> dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
            MEDIA_DATA_DB_DATE_TAKEN };
    vector<string> dateTypeSeconds = { MEDIA_DATA_DB_DATE_ADDED_TO_SECOND,
            MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND, MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND,
            MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND };
    for (size_t i = 0; i < dateTypes.size(); i++) {
        auto it = find(columns.begin(), columns.end(), dateTypes[i]);
        if (it != columns.end()) {
            columns.push_back(dateTypeSeconds[i]);
        }
    }
}

vector<string> GetPhotoAndKnowledgeConnection()
{
    vector<string> clauses;
    clauses.push_back(
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE + " = " + GEO_KNOWLEDGE_TABLE + "." + LATITUDE);
    clauses.push_back(
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE + " = " + GEO_KNOWLEDGE_TABLE + "." + LONGITUDE);
    return clauses;
}

int QueryCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const RdbPredicates &predicates)
{
    const vector<string> columns = { MEDIA_COLUMN_COUNT_1 };
    auto fetchResult = QueryGoToFirst(rdbStore, predicates, columns);
    if (fetchResult == nullptr) {
        return 0;
    }
    return GetFileCount(fetchResult);
}

int GetNewKnowledgeDataCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.BeginWrap()->BeginWrap()
        ->LessThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE, LOCATION_LATITUDE_MAX)
        ->And()->GreaterThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE, LOCATION_DB_ZERO)
        ->EndWrap()->Or()->BeginWrap()
        ->LessThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE, LOCATION_DB_ZERO)
        ->And()->GreaterThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE, LOCATION_LATITUDE_MIN)
        ->EndWrap()->EndWrap()->And()->BeginWrap()->BeginWrap()
        ->LessThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE, LOCATION_LONGITUDE_MAX)->And()
        ->GreaterThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE, LOCATION_DB_ZERO)->EndWrap()
        ->Or()->BeginWrap()->LessThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE, LOCATION_DB_ZERO)
        ->And()->GreaterThan(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE, LOCATION_LONGITUDE_MIN)
        ->EndWrap()->EndWrap();
    auto clauses = GetPhotoAndKnowledgeConnection();
    predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(clauses);
    predicates.And()->BeginWrap()
        ->IsNull(GEO_KNOWLEDGE_TABLE + "." + LATITUDE)
        ->Or()->IsNull(GEO_KNOWLEDGE_TABLE + "." + LONGITUDE)
        ->Or()->IsNull(GEO_KNOWLEDGE_TABLE + "." + LANGUAGE)
        ->EndWrap();

    return QueryCount(rdbStore, predicates);
}

int GetUpdateKnowledgeDataCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(GEO_KNOWLEDGE_TABLE);
    predicates.LessThan(GEO_KNOWLEDGE_TABLE + "." + LOCATION_KEY, 0);
    return QueryCount(rdbStore, predicates);
}

int GetNewDictionaryDataCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(GEO_KNOWLEDGE_TABLE);
    vector<string> clauses;
    clauses.push_back(GEO_KNOWLEDGE_TABLE + "." + CITY_ID + " = " + GEO_DICTIONARY_TABLE + "." + CITY_ID);
    clauses.push_back(GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = " + GEO_DICTIONARY_TABLE + "." + LANGUAGE);
    predicates.LeftOuterJoin(GEO_DICTIONARY_TABLE)->On(clauses);
    predicates.BeginWrap()->IsNull(GEO_DICTIONARY_TABLE + "." + CITY_ID)
        ->And()->IsNotNull(GEO_KNOWLEDGE_TABLE + "." + COUNTRY)->EndWrap();
    vector<string> columns;
    auto resultSet = QueryGoToFirst(rdbStore, predicates, columns);
    if (resultSet == nullptr) {
        return 0;
    }
    set<string> citySet;
    do {
        string cityId = GetStringValFromColumn(resultSet, CITY_ID);
        string cityName = GetStringValFromColumn(resultSet, CITY_NAME);
        if (cityId == "" || cityName == "") {
            continue;
        }
        citySet.insert(cityId);
    } while (!resultSet->GoToNextRow());
    return citySet.size();
}

bool HasLocationData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    int newDataCount = GetNewKnowledgeDataCount(rdbStore);
    int updateDataCount = GetUpdateKnowledgeDataCount(rdbStore);
    MEDIA_INFO_LOG("loc newDataCount:%{public}d, updateDataCount:%{public}d", newDataCount, updateDataCount);

    int newDictionaryCount = GetNewDictionaryDataCount(rdbStore);
    MEDIA_INFO_LOG("newDictionaryCount:%{public}d", newDictionaryCount);
    return (newDataCount + updateDataCount + newDictionaryCount) > 0;
}

int GetCvDataCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> clauses;
    clauses.push_back(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID  + " = " +
        VISION_TOTAL_TABLE + "." + PhotoColumn::MEDIA_ID);
    predicates.InnerJoin(VISION_TOTAL_TABLE)->On(clauses);
    predicates.BeginWrap()->EqualTo(VISION_TOTAL_TABLE + "." + STATUS, 0)->And()
        ->BeginWrap()->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_TIME_PENDING, 0)->And()
        ->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_DATE_TRASHED, 0)->And()
        ->BeginWrap()->NotEqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_POSITION, CLOUD_POSITION_STATUS)
        ->Or()->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_POSITION, CLOUD_POSITION_STATUS)->And()
        ->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_THUMB_STATUS, 0)
        ->EndWrap()->EndWrap()->EndWrap()->EndWrap();
    return QueryCount(rdbStore, predicates);
}

bool HasCvData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    int count = GetCvDataCount(rdbStore);
    MEDIA_INFO_LOG("cv count:%{public}d", count);
    return count > 0;
}

int GetSearchBaseCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(SEARCH_TOTAL_TABLE);
    vector<string> clasues;
    clasues.push_back(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID + " = " +
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID);
    predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On(clasues);
    predicates.EqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_PHOTO_STATUS, 0)
        ->And()
        ->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_TIME_PENDING, 0)
        ->And()
        ->GreaterThanOrEqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID, 0);
    return QueryCount(rdbStore, predicates);
}

int GetSearchUpdateCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(SEARCH_TOTAL_TABLE);
    vector<string> clauses;
    clauses.push_back(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID + " = " +
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID);
    vector<string> clausesTotal;
    clausesTotal.push_back(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID + " = " +
        VISION_TOTAL_TABLE + "." + PhotoColumn::MEDIA_ID);
    vector<string> clausesGeo;
    clausesGeo.push_back(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LATITUDE + " = " + GEO_KNOWLEDGE_TABLE + "." + LATITUDE);
    clausesGeo.push_back(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LONGITUDE +
        " = " + GEO_KNOWLEDGE_TABLE + "." + LONGITUDE);
    predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On(clauses);
    predicates.InnerJoin(VISION_TOTAL_TABLE)->On(clausesTotal);
    predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(clausesGeo);
    predicates.GreaterThanOrEqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID, 0)->And()
        ->EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_TIME_PENDING, 0)->And()
        ->GreaterThan(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_PHOTO_STATUS, 0)->And()
        ->BeginWrap()->EqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_PHOTO_STATUS, SEARCH_UPDATE_STATUS)->Or()
        ->BeginWrap()->EqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_CV_STATUS, SEARCH_UPDATE_STATUS)->And()
        ->EqualTo(VISION_TOTAL_TABLE + "." + FACE, FACE_CLUSTERED)->EndWrap()->Or()
        ->BeginWrap()->EqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_CV_STATUS, 0)->And()
        ->BeginWrap()->NotEqualTo(VISION_TOTAL_TABLE + "." + OCR, 0)->Or()
        ->NotEqualTo(VISION_TOTAL_TABLE + "." + LABEL, 0)->Or()
        ->BeginWrap()->NotEqualTo(VISION_TOTAL_TABLE + "." + FACE, 0)->And()
        ->NotEqualTo(VISION_TOTAL_TABLE + "." + FACE, FACE_RECOGNITION)->And()
        ->NotEqualTo(VISION_TOTAL_TABLE + "." + FACE, FACE_FEATURE)->EndWrap()->EndWrap()->EndWrap()->Or()
        ->BeginWrap()->EqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_GEO_STATUS, 0)->And()
        ->BeginWrap()->NotEqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LATITUDE, 0)->Or()
        ->NotEqualTo(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LONGITUDE, 0)->EndWrap()->And()
        ->IsNotNull(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LATITUDE)->And()
        ->IsNotNull(SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_LONGITUDE)->And()
        ->BeginWrap()->IsNotNull(GEO_KNOWLEDGE_TABLE + "." + LATITUDE)->And()
        ->IsNotNull(GEO_KNOWLEDGE_TABLE + "." + LONGITUDE)->EndWrap()->EndWrap()->EndWrap();
    return QueryCount(rdbStore, predicates);
}

bool HasSearchData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    int baseCount = GetSearchBaseCount(rdbStore);
    int upateCount = GetSearchUpdateCount(rdbStore);
    MEDIA_INFO_LOG("baseCount:%{public}d, upateCount:%{public}d", baseCount, upateCount);
    return (baseCount + upateCount) > 0;
}

bool HasHighLightData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    vector<string> clauses;
    clauses.push_back(ANALYSIS_ALBUM_TABLE + "." + ALBUM_ID + " = " +
        HIGHLIGHT_COVER_INFO_TABLE + "." + ALBUM_ID);
    predicates.InnerJoin(HIGHLIGHT_COVER_INFO_TABLE)->On(clauses);
    predicates.EqualTo(ANALYSIS_ALBUM_TABLE + "." + ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIGHLIGHT))->And()
        ->NotEqualTo(ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_COVER_URI,
        HIGHLIGHT_COVER_INFO_TABLE + "." + COVER_KEY);
    int count = QueryCount(rdbStore, predicates);
    MEDIA_INFO_LOG("highligh count:%{public}d", count);
    return (count > 0);
}

bool MediaLibraryRdbUtils::HasDataToAnalysis(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("HasDataToAnalysis rdbstore is null");
        return false;
    }
    bool loc = HasLocationData(rdbStore);
    bool cv = HasCvData(rdbStore);
    bool search = HasSearchData(rdbStore);
    bool highlight = HasHighLightData(rdbStore);
    return (loc || cv || search || highlight);
}

int32_t MediaLibraryRdbUtils::UpdateThumbnailRelatedDataToDefault(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const int64_t fileId)
{
    int32_t err = -1;
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("RdbStore is null!");
        return err;
    }

    ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, 0);
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, 0);

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = 0;
    err = rdbStore->Update(changedRows, values, predicates);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("RdbStore Update failed! err: %{public}d", err);
    }
    return err;
}
} // namespace OHOS::Media
