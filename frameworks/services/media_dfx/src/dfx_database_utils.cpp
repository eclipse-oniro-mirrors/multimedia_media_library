/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <sys/stat.h>
#include "dfx_database_utils.h"

#include "dfx_utils.h"
#include "lake_const.h"
#include "dfx_reporter.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "media_log.h"
#include "media_column.h"
#include "mimetype_utils.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "moving_photo_file_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "power_efficiency_manager.h"
#include "settings_data_manager.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
const std::string RECORD_COUNT = "recordCount";
const std::string ABNORMAL_VALUE = "-1";
const std::string DFX_OPT_TYPE = "opt_type";
const std::string OPT_ADD_VALUE = "1";
const std::string OPT_DEL_VALUE = "2";
const std::string OPT_UPDATE_VALUE = "3";
const int32_t BATCH_QUERY_PHOTO_NUMBER = 2000;
const int32_t FILE_HEIGHT_AND_WIDTH_240 = 240;
const int32_t FILE_HEIGHT_AND_WIDTH_360 = 360;
const int32_t FILE_HEIGHT_AND_WIDTH_480 = 480;
const int32_t FILE_HEIGHT_AND_WIDTH_720 = 720;
const int32_t FILE_HEIGHT_AND_WIDTH_1080 = 1080;
const int32_t FILE_HEIGHT_AND_WIDTH_1440 = 1440;
const int32_t FILE_HEIGHT_AND_WIDTH_2000 = 2000;
const int32_t FILE_HEIGHT_AND_WIDTH_4000 = 4000;
const int64_t PHOTO_FILE_SIZE = 1024;

static const std::vector<std::pair<int64_t, std::string>> sizeRanges = {
    {100, "size100K"},
    {250, "size250K"},
    {500, "size500K"},
    {1 * 1024, "size1M"},
    {2 * 1024, "size2M"},
    {3 * 1024, "size3M"},
    {4 * 1024, "size4M"},
    {5 * 1024, "size5M"},
    {7 * 1024, "size7M"},
    {10 * 1024, "size10M"},
    {15 * 1024, "size15M"},
    {20 * 1024, "size20M"},
    {30 * 1024, "size30M"},
    {50 * 1024, "size50M"},
    {100 * 1024, "size100M"},
    {500 * 1024, "size500M"},
    {1 * 1024 * 1024, "size1G"},
    {2 * 1024 * 1024, "size2G"},
    {5 * 1024 * 1024, "size5G"}
};

static const std::unordered_map<std::string, int32_t> DEFAULT_PHOTO_SIZE_MAP = {
    {"size100K", 0},
    {"size250K", 0},
    {"size500K", 0},
    {"size1M", 0},
    {"size2M", 0},
    {"size3M", 0},
    {"size4M", 0},
    {"size5M", 0},
    {"size7M", 0},
    {"size10M", 0},
    {"size15M", 0},
    {"size20M", 0},
    {"size30M", 0},
    {"size50M", 0},
    {"size100M", 0},
    {"size500M", 0},
    {"size1G", 0},
    {"size2G", 0},
    {"size5G", 0},
    {"sizeOther", 0}
};

static const std::unordered_map<std::string, int32_t> DEFAULT_PHOTO_RESOLUTION_MAP = {
    {"resolutionLess240P", 0},
    {"resolution240P", 0},
    {"resolution360P", 0},
    {"resolution480P", 0},
    {"resolution720P", 0},
    {"resolution1080P", 0},
    {"resolution2K", 0},
    {"resolution4K", 0}
};

static const std::unordered_map<std::string, int32_t> DEFAULT_VIDEO_RESOLUTION_MAP = {
    {"resolutionLess720P", 0},
    {"resolution720P", 0},
    {"resolution1080P", 0},
    {"resolution2K", 0},
    {"resolution4K", 0}
};

struct SizeAndResolutionInfoMap {
    unordered_map<std::string, int32_t> localImageSizeMap = DEFAULT_PHOTO_SIZE_MAP;
    unordered_map<std::string, int32_t> localVideoSizeMap = DEFAULT_PHOTO_SIZE_MAP;
    unordered_map<std::string, int32_t> cloudImageSizeMap = DEFAULT_PHOTO_SIZE_MAP;
    unordered_map<std::string, int32_t> cloudVideoSizeMap = DEFAULT_PHOTO_SIZE_MAP;
    unordered_map<std::string, int32_t> localImageResolutionMap = DEFAULT_PHOTO_RESOLUTION_MAP;
    unordered_map<std::string, int32_t> localVideoResolutionMap = DEFAULT_VIDEO_RESOLUTION_MAP;
    unordered_map<std::string, int32_t> cloudImageResolutionMap = DEFAULT_PHOTO_RESOLUTION_MAP;
    unordered_map<std::string, int32_t> cloudVideoResolutionMap = DEFAULT_VIDEO_RESOLUTION_MAP;
};

int32_t DfxDatabaseUtils::QueryFromPhotos(int32_t mediaType, int32_t position)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, mediaType);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, position);

    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";

    int32_t count;
    int32_t errCode = QueryInt(predicates, columns, queryColumn, count);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query photos fail: %{public}d mediaType: %{public}d position: %{public}d",
            errCode, mediaType, position);
    }

    return count;
}

AlbumInfo DfxDatabaseUtils::QueryAlbumInfoBySubtype(int32_t albumSubtype)
{
    AlbumInfo albumInfo;
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubtype);
    std::vector<std::string> columns = { PhotoAlbumColumns::ALBUM_COUNT, PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT, PhotoAlbumColumns::ALBUM_CLOUD_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query album fail");
        return albumInfo;
    }
    albumInfo.count = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    albumInfo.imageCount = GetInt32Val(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet);
    albumInfo.videoCount = GetInt32Val(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet);
    albumInfo.isLocal = GetStringVal(PhotoAlbumColumns::ALBUM_CLOUD_ID, resultSet) == "" ? true : false;
    return albumInfo;
}

std::vector<PhotoInfo> DfxDatabaseUtils::QueryDirtyCloudPhoto()
{
    vector<PhotoInfo> photoInfoList;
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, 1);
    predicates.NotEqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t> (DirtyType::TYPE_SYNCED));
    predicates.Limit(DIRTY_PHOTO_COUNT);
    std::vector<std::string> columns = { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_DIRTY,
        PhotoColumn::PHOTO_CLOUD_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return photoInfoList;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotoInfo photoInfo;
        photoInfo.data = DfxUtils::GetSafePath(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet));
        photoInfo.dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        photoInfo.cloudVersion = GetInt32Val(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        photoInfoList.push_back(photoInfo);
    }
    return photoInfoList;
}

static bool ParseResultSet(const string &querySql, int32_t mediaTypePara, int32_t &photoInfoCount)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr!");
        return false;
    }
    auto resultSet = rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (mediaTypePara > 0) {
            int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
            if (mediaType == mediaTypePara) {
                photoInfoCount = GetInt32Val(RECORD_COUNT, resultSet);
            }
        } else {
            photoInfoCount = GetInt32Val(RECORD_COUNT, resultSet);
        }
    }
    return true;
}

static bool QueryOperationResultSet(const string &querySql, int32_t &operationQueryInfoCount)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return E_HAS_DB_ERROR;
    }
    int32_t rowCount = 0;
    auto resultSet = rdbStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query not match data failed");
        return E_DB_FAIL;
    }
    if (resultSet->GetInt(0, rowCount) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdb failed");
        return E_DB_FAIL;
    }
    operationQueryInfoCount = rowCount;
    return true;
}

static string GetDuplicateLpathCountQuerrySql()
{
    return "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE COALESCE(lpath, '') IN (SELECT lpath FROM " +
        PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " IN (0, 2048) AND " +
        "COALESCE(lpath, '') <> '' AND " + PhotoAlbumColumns::ALBUM_DIRTY +
        " <> 4 GROUP BY lpath HAVING COUNT(1) > 1) AND COALESCE(PhotoAlbum.dirty, 1) <> 4";
}

static string GetAbnormalLpathCountQuerySql()
{
    return "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoAlbumColumns::TABLE + " WHERE COALESCE(lpath, '') = '' AND " +
        PhotoAlbumColumns::ALBUM_TYPE + " != " + std::to_string(PhotoAlbumType::SYSTEM) + " AND " +
        PhotoAlbumColumns::ALBUM_DIRTY + " != 4";
}

static void BuildDbInfo(PhotoRecordInfo &photoRecordInfo)
{
    string databaseDir = MEDIA_DB_DIR + "/rdb";
    if (access(databaseDir.c_str(), E_OK) != 0) {
        MEDIA_WARN_LOG("can not get rdb through sandbox");
        return;
    }
    string dbPath = databaseDir.append("/").append(MEDIA_DATA_ABILITY_DB_NAME);

    struct stat statInfo {};
    if (stat(dbPath.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err");
        return;
    }
    photoRecordInfo.dbFileSize = statInfo.st_size;

    struct stat slaveStatInfo {};
    if (stat(MEDIA_DB_FILE_SLAVE.c_str(), &slaveStatInfo) == 0) {
        photoRecordInfo.slaveDbFileSize = slaveStatInfo.st_size;
    }
}

static void FillWaitUploadCount(PhotoRecordInfo &photoRecordInfo, bool &ret)
{
    const string filterCondition = MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
        PhotoColumn::PHOTO_SYNC_STATUS + " = " +
        to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
        to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    auto switchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    if (switchStatus == SwitchStatus::CLOUD) {
        const string photoWaitUploadCloudCountQuerySql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM "
            + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_POSITION + " = "
            + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " AND " + filterCondition;
        ret = ParseResultSet(photoWaitUploadCloudCountQuerySql, 0, photoRecordInfo.photoWaitUploadCloudCount) && ret;
        photoRecordInfo.photoWaitUploadHdcCount = -1;
    } else if (switchStatus == SwitchStatus::HDC) {
        const string photoWaitUploadHdcCountQuerySql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM "
            + PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_POSITION + " = "
            + to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) + " AND " + filterCondition;
        ret = ParseResultSet(photoWaitUploadHdcCountQuerySql, 0, photoRecordInfo.photoWaitUploadHdcCount) && ret;
        photoRecordInfo.photoWaitUploadCloudCount = -1;
    } else if (switchStatus == SwitchStatus::CLOSE) {
        photoRecordInfo.photoWaitUploadCloudCount = 0;
        photoRecordInfo.photoWaitUploadHdcCount = 0;
    } else {
        photoRecordInfo.photoWaitUploadCloudCount = -1;
        photoRecordInfo.photoWaitUploadHdcCount = -1;
    }
}

int32_t DfxDatabaseUtils::QueryPhotoRecordInfo(PhotoRecordInfo &photoRecordInfo)
{
    const string filterCondition = MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
        PhotoColumn::PHOTO_SYNC_STATUS + " = " +
        to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
        to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));

    const string imageAndVideoCountQuerySql = "SELECT " + MediaColumn::MEDIA_TYPE + ", COUNT(*) AS " + RECORD_COUNT +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " + filterCondition + " GROUP BY " + MediaColumn::MEDIA_TYPE;

    const string abnormalSizeCountQuerySql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_SIZE + " = " + ABNORMAL_VALUE +
        " AND " + filterCondition;

    const string abnormalWidthHeightQuerySql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE (" + PhotoColumn::PHOTO_WIDTH + " = " + ABNORMAL_VALUE +
        " OR " + PhotoColumn::PHOTO_HEIGHT + " = " + ABNORMAL_VALUE + ") AND " + filterCondition;

    const string abnormalVideoDurationQuerySql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + MediaColumn::MEDIA_DURATION + " = " + ABNORMAL_VALUE +
        " AND " + MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_VIDEO) + " AND " + filterCondition;

    const string totalAbnormalRecordSql = "SELECT COUNT(*) AS " + RECORD_COUNT + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE (" + MediaColumn::MEDIA_SIZE + " = 0 OR " +
        MediaColumn::MEDIA_SIZE + " IS NULL OR " + MediaColumn::MEDIA_MIME_TYPE + " IS NULL OR " +
        MediaColumn::MEDIA_MIME_TYPE + " = '' OR " + PhotoColumn::PHOTO_HEIGHT + " = 0 OR " +
        PhotoColumn::PHOTO_HEIGHT + " IS NULL OR " + PhotoColumn::PHOTO_WIDTH + " = 0 OR " +
        PhotoColumn::PHOTO_WIDTH + " IS NULL OR ((" + MediaColumn::MEDIA_DURATION + " IS NULL OR " +
        MediaColumn::MEDIA_DURATION + " = 0 ) AND " + MediaColumn::MEDIA_TYPE + " = " +
        std::to_string(MEDIA_TYPE_VIDEO) + " )) AND " + filterCondition;
    
    const string duplicateLpathCountQuerySql = GetDuplicateLpathCountQuerrySql();
    const string abnormalLpathCountQuerySql = GetAbnormalLpathCountQuerySql();

    bool ret = ParseResultSet(imageAndVideoCountQuerySql, MEDIA_TYPE_VIDEO, photoRecordInfo.videoCount);
    ret = ParseResultSet(imageAndVideoCountQuerySql, MEDIA_TYPE_IMAGE, photoRecordInfo.imageCount) && ret;
    ret = ParseResultSet(abnormalSizeCountQuerySql, 0, photoRecordInfo.abnormalSizeCount) && ret;
    ret = ParseResultSet(abnormalWidthHeightQuerySql, 0, photoRecordInfo.abnormalWidthOrHeightCount) && ret;
    ret = ParseResultSet(abnormalVideoDurationQuerySql, 0, photoRecordInfo.abnormalVideoDurationCount) && ret;
    ret = ParseResultSet(totalAbnormalRecordSql, 0, photoRecordInfo.toBeUpdatedRecordCount) && ret;
    ret = ParseResultSet(duplicateLpathCountQuerySql, 0, photoRecordInfo.duplicateLpathCount) && ret;
    ret = ParseResultSet(abnormalLpathCountQuerySql, 0, photoRecordInfo.abnormalLpathCount) && ret;
    FillWaitUploadCount(photoRecordInfo, ret);
    BuildDbInfo(photoRecordInfo);
    return ret;
}

int32_t DfxDatabaseUtils::QueryOperationRecordInfo(OperationRecordInfo &operationRecordInfo)
{
    const string addTotalCountQuerySql = "SELECT COUNT(*) FROM " + PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE +
        " WHERE " + DFX_OPT_TYPE + " = " + OPT_ADD_VALUE;
    const string delTotalCountQuerySql = "SELECT COUNT(*) FROM " + PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE +
        " WHERE " + DFX_OPT_TYPE + " = " + OPT_DEL_VALUE;
    const string updateTotalCountQuerySql = "SELECT COUNT(*) FROM " + PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE +
        " WHERE " + DFX_OPT_TYPE + " = " + OPT_UPDATE_VALUE;

    bool ret = QueryOperationResultSet(addTotalCountQuerySql, operationRecordInfo.addTotalCount);
    ret = QueryOperationResultSet(delTotalCountQuerySql, operationRecordInfo.delTotalCount) && ret;
    ret = QueryOperationResultSet(updateTotalCountQuerySql, operationRecordInfo.updateTotalCount) && ret;
    operationRecordInfo.totalCount = operationRecordInfo.addTotalCount + operationRecordInfo.delTotalCount +
        operationRecordInfo.updateTotalCount;
    return ret ? E_OK : E_FAIL;
}

int32_t DfxDatabaseUtils::QueryAnalysisVersion(const std::string &table, const std::string &column)
{
    NativeRdb::RdbPredicates predicates(table);
    string whereClause = "max(" + column + ") AS version";
    std::vector<std::string> columns = { whereClause };
    string version = "version";
    double count;
    int32_t errCode = QueryDouble(predicates, columns, version, count);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query analysis version fail: %{public}d", errCode);
    }
    return static_cast<int32_t> (count);
}

int32_t DfxDatabaseUtils::QueryDbVersion()
{
    int64_t dbVersion = 0;
    MediaLibraryRdbStore::QueryPragma("user_version", dbVersion);
    return static_cast<int32_t> (dbVersion);
}

int32_t DfxDatabaseUtils::QueryInt(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, const std::string &queryColumn, int32_t &value)
{
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_DB_FAIL;
    }
    value = GetInt32Val(queryColumn, resultSet);
    return E_OK;
}

int32_t DfxDatabaseUtils::QueryDouble(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, const std::string &queryColumn, double &value)
{
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_DB_FAIL;
    }
    value = GetDoubleVal(queryColumn, resultSet);
    return E_OK;
}

int32_t DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(int32_t& downloadedThumb, int32_t& generatedThumb)
{
    // cloud image that are all generated
    NativeRdb::RdbPredicates generatePredicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    int32_t cloudImage = 2;
    int32_t thumbGeneratedFinished = 2;
    generatePredicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_POSITION, cloudImage)->And()
        ->GreaterThanOrEqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, thumbGeneratedFinished);
    int32_t errCode = QueryInt(generatePredicates, columns, queryColumn, generatedThumb);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query generated image fail: %{public}d", errCode);
        return errCode;
    }

    // cloud image that are downloaded
    NativeRdb::RdbPredicates downloadPredicates(PhotoColumn::PHOTOS_TABLE);
    downloadPredicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_POSITION, cloudImage)->And()
        ->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, 0);
    errCode = QueryInt(downloadPredicates, columns, queryColumn, downloadedThumb);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query downloaded image fail: %{public}d", errCode);
        return errCode;
    }
    return E_OK;
}

int32_t DfxDatabaseUtils::QueryASTCThumb(bool isLocal)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    int32_t localImage = 1;
    int32_t cloudImage = 2;
    int32_t localAndCloudImage = 3;
    int32_t thumbnail_ready = 3;
    if (isLocal) {
        predicates.BeginWrap();
        predicates.EqualTo(PhotoColumn::PHOTO_POSITION, localImage);
        predicates.Or()->EqualTo(PhotoColumn::PHOTO_POSITION, localAndCloudImage);
        predicates.EndWrap();
    } else {
        predicates.EqualTo(PhotoColumn::PHOTO_POSITION, cloudImage);
    }

    predicates.And()
        ->GreaterThanOrEqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, thumbnail_ready);

    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    int32_t count = 0;
    int32_t errCode = QueryInt(predicates, columns, queryColumn, count);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query astc thumb fail: %{public}d", errCode);
    }

    return count;
}

int32_t DfxDatabaseUtils::QueryLCDThumb(bool isLocal)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    int32_t lcd_visit_time = 2;
    int32_t localImage = 1;
    int32_t cloudImage = 2;
    int32_t localAndCloudImage = 3;
    int32_t localThumb = 0;
    int32_t cloudThumb = 2;
    if (isLocal) {
        predicates.BeginWrap();
        predicates.EqualTo(PhotoColumn::PHOTO_POSITION, localImage);
        predicates.And()->EqualTo(PhotoColumn::PHOTO_LCD_VISIT_TIME, lcd_visit_time);
        predicates.EndWrap();
        predicates.Or()->BeginWrap();
        predicates.BeginWrap();
        predicates.EqualTo(PhotoColumn::PHOTO_LCD_VISIT_TIME, lcd_visit_time);
        predicates.Or()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, localThumb);
        predicates.Or()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, cloudThumb);
        predicates.EndWrap();
        predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, localAndCloudImage);
        predicates.EndWrap();
    } else {
        predicates.BeginWrap();
        predicates.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, localThumb);
        predicates.Or()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, cloudThumb);
        predicates.EndWrap();
        predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, cloudImage);
    }

    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";

    int32_t count = 0;
    int32_t errCode = QueryInt(predicates, columns, queryColumn, count);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query lcd thumb fail: %{public}d", errCode);
    }

    return count;
}

static shared_ptr<NativeRdb::ResultSet> QueryPhotoFilePath(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, int offset)
{
    string querySql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::PHOTO_POSITION + " = 1" +
        " LIMIT " + std::to_string(offset) + ", " + std::to_string(BATCH_QUERY_PHOTO_NUMBER);
    return rdbStore->QuerySql(querySql);
}

bool DfxDatabaseUtils::CheckChargingAndScreenOff(bool isReported)
{
    bool bFlag = PowerEfficiencyManager::IsChargingAndScreenOff();
    if (bFlag) {
        return bFlag;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return bFlag;
    }
    if (isReported) {
        int64_t time = MediaFileUtils::UTCTimeSeconds() - TWO_DAY;
        MEDIA_INFO_LOG("change to last two day time");
        prefs->PutLong(LAST_TWO_DAY_REPORT_TIME, time);
    } else {
        int64_t time = MediaFileUtils::UTCTimeSeconds() - ONE_WEEK;
        MEDIA_INFO_LOG("change to last week day time");
        prefs->PutLong(LAST_WEEK_REPORT_TIME, time);
    }
    prefs->FlushSync();
    return bFlag;
}

int32_t DfxDatabaseUtils::QueryPhotoErrorCount()
{
    MEDIA_DEBUG_LOG("QueryPhotoErrorCount start");
    MediaLibraryTracer tracer;
    tracer.Start("QueryPhotoErrorCount");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_OK, "rdbStore is nullptr");
    std::string querySql = "SELECT count(1) as count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_POSITION + " = 1";
    shared_ptr<NativeRdb::ResultSet> resultCount = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultCount != nullptr, E_OK, "Failed to query resultCount");
    int32_t count = 0;
    if (resultCount->GoToNextRow() == NativeRdb::E_OK) {
        count = GetInt32Val("count", resultCount);
        MEDIA_INFO_LOG("position == one ,count: %{public}d", count);
    }
    resultCount->Close();
    CHECK_AND_RETURN_RET_LOG(count > 0, E_OK, "Failed to get count");
    int32_t photoCount = 0;
    for (int32_t offset = 0; offset < count; offset += BATCH_QUERY_PHOTO_NUMBER) {
        if (!CheckChargingAndScreenOff(true)) {
            MEDIA_ERR_LOG("Charging and screen off");
            return E_OK;
        }
        auto resultSet = QueryPhotoFilePath(rdbStore, offset);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_OK, "Failed to query resultSet");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            if (!PhotoFileUtils::IsThumbnailExists(filePath)) {
                MEDIA_ERR_LOG("Invali date thumbnail, file path: %{private}s", filePath.c_str());
                continue;
            }
            if (!MediaFileUtils::IsFileExists(filePath)) {
                MEDIA_ERR_LOG("File not exists, file path: %{private}s", filePath.c_str());
                photoCount++;
            }
        }
        resultSet->Close();
    }
    return photoCount;
}

int32_t DfxDatabaseUtils::QueryTotalCloudThumb(int32_t& totalDownload)
{
    int32_t cloudImage = 2;
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_POSITION, cloudImage);
    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    int32_t errCode = QueryInt(predicates, columns, queryColumn, totalDownload);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query total download image fail: %{public}d", errCode);
        return errCode;
    }
    return E_OK;
}

static void UpdatePhotoSizeMap(const int64_t size, std::unordered_map<std::string, int32_t> &photoSizeMap)
{
    if (size <= 0) {
        MEDIA_ERR_LOG("is wrong size");
        return;
    }
    int64_t fileSize = size / PHOTO_FILE_SIZE;
    for (const auto &range : sizeRanges) {
        if (fileSize <= range.first) {
            photoSizeMap[range.second]++;
            return;
        }
    }
    photoSizeMap["sizeOther"]++;
}

static void UpdatePhotoResolutionMap(const int32_t height, const int32_t width,
    std::unordered_map<std::string, int32_t> &resolutionMap)
{
    int32_t min = std::min(height, width);
    int32_t max = std::max(height, width);
    if (max < FILE_HEIGHT_AND_WIDTH_2000) {
        if (min < FILE_HEIGHT_AND_WIDTH_240) {
            resolutionMap["resolutionLess240P"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_240 && min < FILE_HEIGHT_AND_WIDTH_360) {
            resolutionMap["resolution240P"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_360 && min < FILE_HEIGHT_AND_WIDTH_480) {
            resolutionMap["resolution360P"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_480 && min < FILE_HEIGHT_AND_WIDTH_720) {
            resolutionMap["resolution480P"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_720 && min < FILE_HEIGHT_AND_WIDTH_1080) {
            resolutionMap["resolution720P"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_1080 && min < FILE_HEIGHT_AND_WIDTH_1440) {
            resolutionMap["resolution1080P"]++;
        } else {
            MEDIA_ERR_LOG("is wrong resolution");
        }
    } else {
        if (min >= FILE_HEIGHT_AND_WIDTH_1440 && max < FILE_HEIGHT_AND_WIDTH_4000) {
            resolutionMap["resolution2K"]++;
        } else if (min >= FILE_HEIGHT_AND_WIDTH_1440 && max >= FILE_HEIGHT_AND_WIDTH_4000) {
            resolutionMap["resolution4K"]++;
        } else {
            MEDIA_ERR_LOG("is wrong resolution");
        }
    }
}

static void UpdateVideoResolutionMap(const int32_t height, const int32_t width,
    std::unordered_map<std::string, int32_t> &resolutionMap)
{
    int32_t min = std::min(height, width);
    int32_t max = std::max(height, width);
    if (min < FILE_HEIGHT_AND_WIDTH_720 && max < FILE_HEIGHT_AND_WIDTH_2000) {
        resolutionMap["resolutionLess720P"]++;
    } else if (min >= FILE_HEIGHT_AND_WIDTH_720 && min < FILE_HEIGHT_AND_WIDTH_1080 &&
        max < FILE_HEIGHT_AND_WIDTH_2000) {
        resolutionMap["resolution720P"]++;
    } else if (min >= FILE_HEIGHT_AND_WIDTH_1080 && min < FILE_HEIGHT_AND_WIDTH_1440 &&
        max < FILE_HEIGHT_AND_WIDTH_2000) {
        resolutionMap["resolution1080P"]++;
    } else if (min >= FILE_HEIGHT_AND_WIDTH_1440 && max < FILE_HEIGHT_AND_WIDTH_4000 &&
        max >= FILE_HEIGHT_AND_WIDTH_2000) {
        resolutionMap["resolution2K"]++;
    } else if (min >= FILE_HEIGHT_AND_WIDTH_1440 && max >= FILE_HEIGHT_AND_WIDTH_4000) {
        resolutionMap["resolution4K"]++;
    } else {
        MEDIA_ERR_LOG("is wrong resolution");
    }
}

void DfxDatabaseUtils::GetPhotoMimeType(std::string &photoMimeType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    std::string querySql = "SELECT " + MediaColumn::MEDIA_MIME_TYPE + ", COUNT(*) AS count FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::MEDIA_DATE_TRASHED + " = 0 AND " + PhotoColumn::PHOTO_BURST_COVER_LEVEL +
        " = 1 GROUP BY " + MediaColumn::MEDIA_MIME_TYPE;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query resultCount");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string mediaType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t count = GetInt32Val("count", resultSet);
        photoMimeType += mediaType + ":" + std::to_string(count) + ", ";
    }
    resultSet->Close();
}

static void StatisticsSizeAndResolution(shared_ptr<NativeRdb::ResultSet> &resultSet,
    SizeAndResolutionInfoMap &infoMap)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to statistics");
    int32_t position = GetInt64Val(PhotoColumn::PHOTO_POSITION, resultSet);
    int32_t mediaType = GetInt64Val(MediaColumn::MEDIA_TYPE, resultSet);
    int32_t height = GetInt64Val(PhotoColumn::PHOTO_HEIGHT, resultSet);
    int32_t width = GetInt64Val(PhotoColumn::PHOTO_WIDTH, resultSet);
    int64_t size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    if (mediaType == MEDIA_TYPE_IMAGE) {
        if (position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
            UpdatePhotoSizeMap(size, infoMap.localImageSizeMap);
            UpdatePhotoResolutionMap(height, width, infoMap.localImageResolutionMap);
        } else  {
            UpdatePhotoSizeMap(size, infoMap.cloudImageSizeMap);
            UpdatePhotoResolutionMap(height, width, infoMap.cloudImageResolutionMap);
        }
    } else if (mediaType == MEDIA_TYPE_VIDEO) {
        if (position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
            UpdatePhotoSizeMap(size, infoMap.localVideoSizeMap);
            UpdateVideoResolutionMap(height, width, infoMap.localVideoResolutionMap);
        } else {
            UpdatePhotoSizeMap(size, infoMap.cloudVideoSizeMap);
            UpdateVideoResolutionMap(height, width, infoMap.cloudVideoResolutionMap);
        }
    } else {
        MEDIA_ERR_LOG("is wrong mediaType");
    }
}

static string GetInfoToString(const unordered_map<string, int32_t>& defaultMap)
{
    std::string result;
    for (const auto& pair : defaultMap) {
        if (pair.second > 0) {
            result += pair.first + ":" + std::to_string(pair.second) + "; ";
        }
    }
    return result;
}

bool DfxDatabaseUtils::GetSizeAndResolutionInfo(QuerySizeAndResolution &queryInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    std::string querySql = "SELECT count(1) as count FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " + PhotoColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
        PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 ";
    shared_ptr<NativeRdb::ResultSet> resultCount = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultCount != nullptr, false, "Failed to query resultCount");
    int32_t count = 0;
    if (resultCount->GoToNextRow() == NativeRdb::E_OK) {
        count = GetInt32Val("count", resultCount);
    }
    resultCount->Close();
    CHECK_AND_RETURN_RET_LOG(count > 0, false, "Failed to get count");
    SizeAndResolutionInfoMap infoMap;
    for (int32_t offset = 0; offset < count; offset += BATCH_QUERY_PHOTO_NUMBER) {
        if (!CheckChargingAndScreenOff(false)) {
            MEDIA_ERR_LOG("Charging and screen off");
            return false;
        }
        std::string querySql = "SELECT " + PhotoColumn::PHOTO_POSITION + ", " + MediaColumn::MEDIA_TYPE +
            " , " + MediaColumn::MEDIA_SIZE + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH +
            " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
            PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " + PhotoColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
            PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 " +
            " LIMIT " + std::to_string(offset) + ", " + std::to_string(BATCH_QUERY_PHOTO_NUMBER);
 
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Failed to query resultCount");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            StatisticsSizeAndResolution(resultSet, infoMap);
        }
        resultSet->Close();
    }
    queryInfo.localImageSize = GetInfoToString(infoMap.localImageSizeMap);
    queryInfo.localVideoSize = GetInfoToString(infoMap.localVideoSizeMap);
    queryInfo.cloudImageSize = GetInfoToString(infoMap.cloudImageSizeMap);
    queryInfo.cloudVideoSize = GetInfoToString(infoMap.cloudVideoSizeMap);
    queryInfo.localImageResolution = GetInfoToString(infoMap.localImageResolutionMap);
    queryInfo.localVideoResolution = GetInfoToString(infoMap.localVideoResolutionMap);
    queryInfo.cloudImageResolution = GetInfoToString(infoMap.cloudImageResolutionMap);
    queryInfo.cloudVideoResolution = GetInfoToString(infoMap.cloudVideoResolutionMap);
    return true;
}
 
int32_t DfxDatabaseUtils::QueryAncoPhotosFormatAndCount(AncoCountFormatInfo &reportData)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    predicates.And()->EqualTo(PhotoColumn::MEDIA_DATE_TRASHED, "0");
    auto resultSet = MediaLibraryRdbStore::Query(predicates,
        { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_MIME_TYPE,
            PhotoColumn::PHOTO_MEDIA_SUFFIX, PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return E_DB_FAIL;
    }
    set<int32_t> albumSet;
    map<std::string, int32_t> formatCountMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string mtStr = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        std::string mediaSuffix = GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet);
        if (mtStr.empty() || mediaSuffix.empty()) {
            continue;
        }
        MediaType mt = MimeTypeUtils::GetMediaTypeFromMimeType(mtStr);
        if (mt == MediaType::MEDIA_TYPE_IMAGE) {
            reportData.imageCount += 1;
        } else if (mt == MediaType::MEDIA_TYPE_VIDEO) {
            reportData.videoCount += 1;
        } else {
            continue;
        }
        if (formatCountMap.find(mediaSuffix) == formatCountMap.end()) {
            formatCountMap[mediaSuffix] = 1;
        } else {
            formatCountMap[mediaSuffix] = formatCountMap[mediaSuffix] + 1;
        }
        int32_t album_id = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
        albumSet.insert(album_id);
    }
    resultSet->Close();
    reportData.albumCount = static_cast<int32_t>(albumSet.size());
    // 格式数量
    nlohmann::json staticsJson(formatCountMap);
    reportData.assetFormatDistribution = staticsJson.dump();
    return E_OK;
}

std::vector<std::string> DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(const int32_t uploadStatus)
{
    std::vector<std::string> albumNames;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, albumNames, "rdbStore is nullptr");
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::USER));
    predicates.Or();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(PhotoAlbumType::SOURCE));
    predicates.EndWrap();
    predicates.EqualTo(PhotoAlbumColumns::UPLOAD_STATUS, uploadStatus);
    std::vector<std::string> columns = { PhotoAlbumColumns::ALBUM_NAME };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumNames, "query album failed. resultSet is null");

    int32_t count = 0;
    constexpr int32_t maxCount = 50;
    std::string albumNameStr = "";
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
        CHECK_AND_CONTINUE_ERR_LOG(!albumName.empty(), "albumName is empty");
        albumNameStr += DfxUtils::GetSafeAlbumNameWhenChinese(albumName) + ";";
        count++;
        if (count == maxCount) {
            albumNames.emplace_back(std::move(albumNameStr));
            albumNameStr.clear();
            count = 0;
        }
        MEDIA_DEBUG_LOG("albumName: %{public}s, uploadStatus: %{public}d", albumName.c_str(), uploadStatus);
    }
    resultSet->Close();

    if (!albumNameStr.empty()) {
        albumNames.emplace_back(std::move(albumNameStr));
    }
    return albumNames;
}
} // namespace Media
} // namespace OHOS