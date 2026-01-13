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

#include "medialibrary_album_fusion_utils.h"

#include "dfx_manager.h"
#include "dfx_reporter.h"
#include "map_operation_flag.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_notify.h"
#include "medialibrary_tracer.h"
#include "parameters.h"
#include "photo_file_operation.h"
#include "photo_asset_copy_operation.h"
#include "thumbnail_service.h"
#include "photo_source_path_operation.h"
#include "photo_album_lpath_operation.h"
#include "photo_album_update_date_modified_operation.h"
#include "photo_album_copy_meta_data_operation.h"
#include "medialibrary_bundle_manager.h"
#include "permission_utils.h"
#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"
#include "metadata_extractor.h"
#include "moving_photo_file_utils.h"
#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"
#include "refresh_business_name.h"
#include "medialibrary_subscriber.h"
#include "media_scanner_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

constexpr int32_t POSITION_LOCAL_FLAG = 1;
constexpr int32_t POSITION_CLOUD_FLAG = 2;
constexpr int32_t POSITION_BOTH_FLAG = 3;
constexpr int32_t CLOUD_COPY_DIRTY_FLAG = 7;

constexpr int32_t TIME_STAMP_OFFSET = 5;
const std::string ALBUM_FUSION_FLAG = "multimedia.medialibrary.cloneFlag";
const std::string ALBUM_FUSION_UPGRADE_STATUS_FLAG = "persist.multimedia.medialibrary.albumFusion.status";
const int32_t ALBUM_FUSION_UPGRADE_SUCCESS = 1;
const int32_t ALBUM_FUSION_UPGRADE_FAIL = 0;
const int32_t ALBUM_FUSION_BATCH_COUNT = 200;
const string SQL_GET_DUPLICATE_PHOTO = "SELECT p.file_id FROM Photos p "
            "LEFT JOIN PhotoAlbum a ON p.owner_album_id = a.album_id "
            "WHERE p.dirty = 7 AND a.album_id IS NULL LIMIT 500";

static unordered_map<string, ResultSetDataType> convertFormatCommonColumn = {
    {MediaColumn::MEDIA_DATE_TAKEN, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_SUBTYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_USER_COMMENT, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_YEAR, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_MONTH, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_DAY, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_THUMB_STATUS, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_COVER_POSITION, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_THUMBNAIL_READY, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_LCD_SIZE, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_THUMB_SIZE, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DETAIL_TIME, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_OWNER_ALBUM_ID, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_IS_FAV, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::SUPPORTED_WATERMARK_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_IS_RECENT_SHOW, ResultSetDataType::TYPE_INT32},
};

static unordered_map<string, ResultSetDataType> commonColumnTypeMap = {
    {MediaColumn::MEDIA_SIZE, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_TITLE, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_NAME, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_TYPE, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_MIME_TYPE, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_OWNER_PACKAGE, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_OWNER_APPID, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_PACKAGE_NAME, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_DEVICE_NAME, ResultSetDataType::TYPE_STRING},
    {MediaColumn::MEDIA_DATE_MODIFIED, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_DATE_ADDED, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_DATE_TAKEN, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_DURATION, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_IS_FAV, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_DATE_TRASHED, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_DATE_DELETED, ResultSetDataType::TYPE_INT64},
    {MediaColumn::MEDIA_HIDDEN, ResultSetDataType::TYPE_INT32},
    {MediaColumn::MEDIA_PARENT_ID, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_META_DATE_MODIFIED, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_ORIENTATION, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_EXIF_ROTATE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_LATITUDE, ResultSetDataType::TYPE_DOUBLE},
    {PhotoColumn::PHOTO_LONGITUDE, ResultSetDataType::TYPE_DOUBLE},
    {PhotoColumn::PHOTO_HEIGHT, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_WIDTH, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_EDIT_TIME, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_SUBTYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::CAMERA_SHOT_KEY, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_USER_COMMENT, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_SHOOTING_MODE, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_SHOOTING_MODE_TAG, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_ALL_EXIF, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_YEAR, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_MONTH, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_DATE_DAY, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_HIDDEN_TIME, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_FIRST_VISIT_TIME, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_HDR_MODE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_FRONT_CAMERA, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_BURST_COVER_LEVEL, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::SUPPORTED_WATERMARK_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_MEDIA_SUFFIX, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_IS_RECENT_SHOW, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_VIDEO_MODE, ResultSetDataType::TYPE_INT32},
};

static unordered_map<string, ResultSetDataType> thumbnailColumnTypeMap = {
    {PhotoColumn::PHOTO_LCD_VISIT_TIME, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_THUMBNAIL_READY, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_LCD_SIZE, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_THUMB_SIZE, ResultSetDataType::TYPE_STRING},
};

static unordered_map<string, ResultSetDataType> albumColumnTypeMap = {
    {PhotoAlbumColumns::ALBUM_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_SUBTYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_NAME, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_COVER_URI, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, ResultSetDataType::TYPE_INT64},
    {PhotoAlbumColumns::CONTAINS_HIDDEN, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::HIDDEN_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::HIDDEN_COVER, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_ORDER, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_IMAGE_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_VIDEO_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_BUNDLE_NAME, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_IS_LOCAL, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUMS_ORDER, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ORDER_SECTION, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ORDER_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ORDER_STATUS, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::STYLE2_ALBUMS_ORDER, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::STYLE2_ORDER_SECTION, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::STYLE2_ORDER_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::STYLE2_ORDER_STATUS, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::COVER_URI_SOURCE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::COVER_CLOUD_ID, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::UPLOAD_STATUS, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_HIDDEN, ResultSetDataType::TYPE_INT32},
};

std::mutex MediaLibraryAlbumFusionUtils::cloudAlbumAndDataMutex_;
std::atomic<bool> MediaLibraryAlbumFusionUtils::isNeedRefreshAlbum = false;
// LCOV_EXCL_START
int32_t MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_0: Start remove misadded hidden data");
    CHECK_AND_RETURN_RET_LOG(upgradeStore != nullptr, E_DB_FAIL, "fail to get rdbstore");
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t err = upgradeStore->ExecuteSql(DROP_UNWANTED_ALBUM_RELATIONSHIP_FOR_HIDDEN_ALBUM_ASSET);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Failed to drop unwanted album relationship for .hiddenAlbum! Failed to exec: %{public}s",
        DROP_UNWANTED_ALBUM_RELATIONSHIP_FOR_HIDDEN_ALBUM_ASSET.c_str());
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_0: End remove misadded hidden data, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static int32_t PrepareTempUpgradeTable(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, int32_t &matchedCount)
{
    int32_t err = upgradeStore->ExecuteSql(DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Fatal error! Failed to exec: %{public}s", DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
    MEDIA_INFO_LOG("ALBUM_FUSE begin exec: %{public}s", CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
    err = upgradeStore->ExecuteSql(CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE);

    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Fatal error! Failed to exec: %{public}s", CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
    auto resultSet = upgradeStore->QuerySql(QUERY_MATCHED_COUNT);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query matched data fails");
        return E_DB_FAIL;
    }
    resultSet->GetInt(0, matchedCount);
    MEDIA_INFO_LOG("ALBUM_FUSE: There are %{public}d matched items", matchedCount);
    err = upgradeStore->ExecuteSql(CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_MAP_ASSET);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Fatal error! Failed to exec: %{public}s", CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_MAP_ASSET.c_str());
    err = upgradeStore->ExecuteSql(CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_PHOTO_MAP);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Fatal error! Failed to exec: %{public}s", CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_PHOTO_MAP.c_str());
    return E_OK;
}

static int32_t IfHandledDataCountMatched(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    int32_t &exceptCount)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_1: Check if compensate matched data owner_album_id success");
    int32_t updatedSuccessCount = 0;
    auto resultSet = upgradeStore->QuerySql(QUERY_SUCCESS_MATCHED_COUNT);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query matched data fails");
        return E_DB_FAIL;
    }
    resultSet->GetInt(0, updatedSuccessCount);
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_1: There are %{public}d items update success", updatedSuccessCount);
    if (updatedSuccessCount >= exceptCount) {
        MEDIA_INFO_LOG("Handled count matches!");
        return E_OK;
    }
    return E_DB_FAIL;
}

int32_t MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_1: Start handle matched relationship");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t matchedCount = 0;
    int32_t err = PrepareTempUpgradeTable(upgradeStore, matchedCount);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Prepare temp upgrade table fails");
        return err;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: execute update!");
    err = upgradeStore->ExecuteSql(UPDATE_ALBUM_ASSET_MAPPING_CONSISTENCY_DATA_SQL);
    MEDIA_INFO_LOG("ALBUM_FUSE: execute finish!");
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            UPDATE_ALBUM_ASSET_MAPPING_CONSISTENCY_DATA_SQL.c_str());
        return err;
    }

    if (IfHandledDataCountMatched(upgradeStore, matchedCount) != E_OK) {
        MEDIA_ERR_LOG("Handled count not match, may has other transaction!");
        return E_HAS_DB_ERROR;
    }
    err = upgradeStore->ExecuteSql(DELETE_MATCHED_RELATIONSHIP_IN_PHOTOMAP_SQL);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", DELETE_MATCHED_RELATIONSHIP_IN_PHOTOMAP_SQL.c_str());
        return err;
    }
    err = upgradeStore->ExecuteSql(DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
        return err;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_1: End handle matched relationship, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static inline void AddToMap(std::multimap<int32_t, vector<int32_t>> &targetMap, int key, int value)
{
    auto it = targetMap.find(key);
    if (it == targetMap.end()) {
        std::vector<int32_t> valueVector = {value};
        targetMap.insert(std::make_pair(key, valueVector));
    } else {
        it->second.push_back(value);
    }
}

int32_t MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    std::multimap<int32_t, vector<int32_t>> &notMathedMap, bool isUpgrade)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    std::string queryNotMatchedDataSql = "";
    if (isUpgrade) {
        queryNotMatchedDataSql = QUERY_NOT_MATCHED_DATA_IN_PHOTOMAP_BY_PAGE;
    } else {
        queryNotMatchedDataSql = QUERY_NEW_NOT_MATCHED_DATA_IN_PHOTOMAP_BY_PAGE;
    }
    auto resultSet = upgradeStore->QuerySql(queryNotMatchedDataSql);
    MEDIA_INFO_LOG("query sql is %{public}s", queryNotMatchedDataSql.c_str());
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }
    int32_t notMatchedCount = 0;
    resultSet->GetRowCount(notMatchedCount);
    if (notMatchedCount == 0) {
        MEDIA_INFO_LOG("Already matched, no need to handle");
        return E_OK;
    }
    MEDIA_INFO_LOG("There are %{public}d assets need to copy", notMatchedCount);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int colIndex = -1;
        int32_t assetId = 0;
        int32_t albumId = 0;
        resultSet->GetColumnIndex(PhotoMap::ALBUM_ID, colIndex);
        CHECK_AND_RETURN_RET(resultSet->GetInt(colIndex, albumId) == NativeRdb::E_OK, E_HAS_DB_ERROR);

        resultSet->GetColumnIndex(PhotoMap::ASSET_ID, colIndex);
        CHECK_AND_RETURN_RET(resultSet->GetInt(colIndex, assetId) == NativeRdb::E_OK, E_HAS_DB_ERROR);
        AddToMap(notMathedMap, assetId, albumId);
    }
    return E_OK;
}

static bool isLocalAsset(shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }
    int colIndex = -1;
    int32_t position = POSITION_CLOUD_FLAG;
    resultSet->GetColumnIndex("position", colIndex);
    if (resultSet->GetInt(colIndex, position) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return position != POSITION_CLOUD_FLAG;
}

void MediaLibraryAlbumFusionUtils::BuildTargetFilePath(
    std::string &targetPath, std::string displayName, int32_t mediaType)
{
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::function<int(void)> tryReuseDeleted = [&]()->int {
        int32_t uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(mediaType, trans);
        return MediaLibraryAssetOperations::CreateAssetPathById(uniqueId, mediaType,
            MediaFileUtils::GetExtensionFromPath(displayName), targetPath);
    };
    int ret = trans->RetryTrans(tryReuseDeleted);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Create targetPath failed, ret=%{public}d", ret);
    }
}

static std::string getThumbnailPathFromOrignalPath(std::string srcPath)
{
    if (srcPath.empty()) {
        MEDIA_ERR_LOG("source file invalid!");
        return "";
    }
    std::string photoRelativePath = "/Photo/";
    std::string thumbRelativePath = "/.thumbs/Photo/";
    size_t pos = srcPath.find(photoRelativePath);
    std::string thumbnailPath = "";
    if (pos != string::npos) {
        thumbnailPath = srcPath.replace(pos, photoRelativePath.length(), thumbRelativePath);
    }
    return thumbnailPath;
}

int32_t CopyDirectory(const std::string &srcDir, const std::string &dstDir)
{
    if (!MediaFileUtils::CreateDirectory(dstDir)) {
        MEDIA_ERR_LOG("Create dstDir %{public}s failed", dstDir.c_str());
        return E_FAIL;
    }
    if (!MediaFileUtils::IsFileExists(srcDir)) {
        MEDIA_WARN_LOG("%{public}s doesn't exist, skip.", srcDir.c_str());
        return E_OK;
    }
    for (const auto &dirEntry : std::filesystem::directory_iterator{ srcDir }) {
        std::string srcFilePath = dirEntry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        if (!MediaFileUtils::IsFileExists(srcFilePath) || !MediaFileUtils::IsFileValid(srcFilePath)) {
            MEDIA_ERR_LOG("Copy file from %{public}s failed , because of thumbnail is invalid", srcFilePath.c_str());
        }
        if (!MediaFileUtils::CopyFileUtil(srcFilePath, dstFilePath)) {
            MEDIA_ERR_LOG("Copy file from %{public}s to %{public}s failed",
                srcFilePath.c_str(), dstFilePath.c_str());
            return E_FAIL;
        }
    }
    return E_OK;
}

static int32_t CopyOriginThumbnail(const std::string &srcPath, std::string &targetPath)
{
    if (srcPath.empty() || targetPath.empty()) {
        MEDIA_ERR_LOG("source file or targetPath empty");
        return E_INVALID_PATH;
    }
    std::string originalThumbnailDirPath = getThumbnailPathFromOrignalPath(srcPath);
    std::string targetThumbnailDirPath = getThumbnailPathFromOrignalPath(targetPath);
    if (!targetThumbnailDirPath.empty()) {
        int32_t err = MediaFileUtils::CopyDirectory(originalThumbnailDirPath, targetThumbnailDirPath);
        if (err != E_OK) {
            MEDIA_ERR_LOG("copy thumbnail dir fail because of %{public}d, dir:%{public}s",
                err, originalThumbnailDirPath.c_str());
        }
    }
    return E_OK;
}

static int32_t DeleteFile(const std::string &targetPath)
{
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("targetPath empty");
        return E_INVALID_PATH;
    }
    MediaFileUtils::DeleteFile(targetPath);
    return E_OK;
}

static int32_t DeleteThumbnail(const std::string &targetPath)
{
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("targetPath empty");
        return E_INVALID_PATH;
    }
    std::string targetThumbnailDirPath = getThumbnailPathFromOrignalPath(targetPath);
    MediaFileUtils::DeleteDir(targetThumbnailDirPath);
    return E_OK;
}

static int32_t GetIntValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, int &value)
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

static int32_t GetDoubleValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, double &value)
{
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int index = -1;
    resultSet->GetColumnIndex(column, index);
    if (index == -1) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetDouble(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int64_t GetLongValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, int64_t &value)
{
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int index = -1;
    resultSet->GetColumnIndex(column, index);
    if (index == -1) {
        return E_HAS_DB_ERROR;
    }
    if (resultSet->GetLong(index, value) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

static int32_t GetStringValueFromResultSet(shared_ptr<ResultSet> resultSet, const string &column, string &value)
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

static void ParsingAndFillValue(NativeRdb::ValuesBucket &values, const string &columnName,
    ResultSetDataType columnType, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    switch (columnType) {
        case ResultSetDataType::TYPE_INT32: {
            int32_t intColumnValue;
            GetIntValueFromResultSet(resultSet, columnName, intColumnValue);
            values.PutInt(columnName, intColumnValue);
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            int64_t longColumnValue;
            GetLongValueFromResultSet(resultSet, columnName, longColumnValue);
            values.PutLong(columnName, longColumnValue);
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            double doubleColumnValue;
            GetDoubleValueFromResultSet(resultSet, columnName, doubleColumnValue);
            values.PutDouble(columnName, doubleColumnValue);
            break;
        }
        case ResultSetDataType::TYPE_STRING: {
            std::string stringValue = "";
            GetStringValueFromResultSet(resultSet, columnName, stringValue);
            values.PutString(columnName, stringValue);
            break;
        }
        default:
            MEDIA_ERR_LOG("No such column type");
    }
}

static void ParsingAndFillValueForAllAlbumColumns(NativeRdb::ValuesBucket &values, const string &columnName,
    const ColumnSchema &columnSchema, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    bool isNull = false;
    int columnIndex = -1;
    resultSet->GetColumnIndex(columnName, columnIndex);
    resultSet->IsColumnNull(columnIndex, isNull);
    if (isNull) {
        values.PutNull(columnName);
    } else if (columnSchema.columnType == "INTEGER" || columnSchema.columnType == "INT") {
        int32_t intColumnValue;
        GetIntValueFromResultSet(resultSet, columnName, intColumnValue);
        values.PutInt(columnName, intColumnValue);
    } else if (columnSchema.columnType == "BIGINT") {
        int64_t longColumnValue;
        GetLongValueFromResultSet(resultSet, columnName, longColumnValue);
        values.PutLong(columnName, longColumnValue);
    } else if (columnSchema.columnType == "DOUBLE") {
        double doubleColumnValue;
        GetDoubleValueFromResultSet(resultSet, columnName, doubleColumnValue);
        values.PutDouble(columnName, doubleColumnValue);
    } else {
        std::string stringValue = "";
        GetStringValueFromResultSet(resultSet, columnName, stringValue);
        values.PutString(columnName, stringValue);
    }
}

struct MediaAssetCopyInfo {
    std::string targetPath;
    bool isCopyThumbnail;
    int32_t ownerAlbumId;
    std::string displayName;
    bool isCopyDateAdded;
    bool isCopyCeAvailable;
    bool isCopyPackageName;
    bool isCopyOwnerPackage;
    MediaAssetCopyInfo(const std::string& targetPath, bool isCopyThumbnail, int32_t ownerAlbumId,
        const std::string& displayName = "", bool isCopyDateAdded = true, bool isCopyCeAvailable = false,
        bool isCopyPackageName = true, bool isCopyOwnerPackage = true) : targetPath(targetPath),
        isCopyThumbnail(isCopyThumbnail), ownerAlbumId(ownerAlbumId), displayName(displayName),
        isCopyDateAdded(isCopyDateAdded), isCopyCeAvailable(isCopyCeAvailable), isCopyPackageName(isCopyPackageName),
        isCopyOwnerPackage(isCopyOwnerPackage) {}
};

static void HandleLowQualityAssetValuesBucket(shared_ptr<NativeRdb::ResultSet>& resultSet,
    NativeRdb::ValuesBucket& values)
{
    int32_t dirty = -1;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_DIRTY, dirty);
    int32_t photoQuality = 0;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_QUALITY, photoQuality);
    if (photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        photoQuality = static_cast<int32_t>(MultiStagesPhotoQuality::FULL);
        dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
        values.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
    }
    values.PutInt(PhotoColumn::PHOTO_QUALITY, photoQuality);
    if (dirty == -1 && photoQuality != static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        MEDIA_WARN_LOG("Status error, dirty is -1, cannot upload");
        values.PutInt(PhotoColumn::PHOTO_DIRTY, -1);
    }
}

static void HandleTempFileAssetValuesBucket(shared_ptr<NativeRdb::ResultSet>& resultSet,
    NativeRdb::ValuesBucket& values)
{
    int32_t type = 0;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_FILE_SOURCE_TYPE, type);
    if (type == static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    }
}

static void HandleCeAvailableValuesBucket(const MediaAssetCopyInfo &copyInfo,
    shared_ptr<NativeRdb::ResultSet>& resultSet, NativeRdb::ValuesBucket& values)
{
    CHECK_AND_RETURN(copyInfo.isCopyCeAvailable);
    int32_t ceAvailable = -1;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    if (ceAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::FINISH)) {
        values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    }
}

static string GetPackageName()
{
    string clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (clientBundle.empty()) {
        MEDIA_ERR_LOG("GetClientBundleName failed");
        return "";
    }
    return PermissionUtils::GetPackageNameByBundleName(clientBundle);
}

static string GetOwnerPackage()
{
    string clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (clientBundle.empty()) {
        MEDIA_ERR_LOG("GetClientBundleName failed");
        return "";
    }
    return clientBundle;
}

static int32_t BuildInsertValuesBucket(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    NativeRdb::ValuesBucket &values, shared_ptr<NativeRdb::ResultSet> &resultSet, const MediaAssetCopyInfo &copyInfo)
{
    values.PutString(MediaColumn::MEDIA_FILE_PATH, copyInfo.targetPath);
    PhotoAssetCopyOperation()
        .SetTargetPhotoInfo(resultSet)
        .SetTargetAlbumId(copyInfo.ownerAlbumId)
        .SetDisplayName(copyInfo.displayName)
        .CopyPhotoAsset(rdbStore, values);
    for (auto it = commonColumnTypeMap.begin(); it != commonColumnTypeMap.end(); ++it) {
        string columnName = it->first;
        ResultSetDataType columnType = it->second;
        ParsingAndFillValue(values, columnName, columnType, resultSet);
        if (columnName == PhotoColumn::PHOTO_FILE_SOURCE_TYPE) {
            values.Put(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceTypes::MEDIA));
        }
    }
    if (copyInfo.isCopyThumbnail) {
        for (auto it = thumbnailColumnTypeMap.begin(); it != thumbnailColumnTypeMap.end(); ++it) {
            string columnName = it->first;
            ResultSetDataType columnType = it->second;
            ParsingAndFillValue(values, columnName, columnType, resultSet);
        }
        // Indicate original file cloud_id for cloud copy
        std::string cloudId = "";
        GetStringValueFromResultSet(resultSet, PhotoColumn::PHOTO_CLOUD_ID, cloudId);
        if (cloudId.empty()) {
            // copy from copyed asset, may not synced, need copy from original asset
            GetStringValueFromResultSet(resultSet, PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID, cloudId);
        }
        values.PutString(PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID, cloudId);
        values.PutInt(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
        values.PutInt(PhotoColumn::PHOTO_DIRTY, CLOUD_COPY_DIRTY_FLAG);
    }
    if (!copyInfo.isCopyDateAdded) {
        values.Delete(MediaColumn::MEDIA_DATE_ADDED);
        values.PutLong(MediaColumn::MEDIA_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    }
    if (!copyInfo.isCopyPackageName) {
        values.Delete(MediaColumn::MEDIA_PACKAGE_NAME);
        values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, GetPackageName());
    }
    if (!copyInfo.isCopyOwnerPackage) {
        values.Put(MediaColumn::MEDIA_OWNER_PACKAGE, GetOwnerPackage());
    }
    HandleLowQualityAssetValuesBucket(resultSet, values);
    HandleTempFileAssetValuesBucket(resultSet, values);
    HandleCeAvailableValuesBucket(copyInfo, resultSet, values);
    return E_OK;
}

static int32_t copyMetaData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, int64_t &newAssetId,
    NativeRdb::ValuesBucket &values)
{
    int32_t ret = rdbStore->Insert(newAssetId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("upgradeStore->Insert failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_DEBUG_LOG("Insert copy meta data success, rowId=%{public}" PRId64", ret=%{public}d", newAssetId, ret);
    return ret;
}

static int32_t GetSourceFilePath(std::string &srcPath, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int colIndex = -1;
    resultSet->GetColumnIndex(MediaColumn::MEDIA_FILE_PATH, colIndex);
    if (resultSet->GetString(colIndex, srcPath) != NativeRdb::E_OK) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

struct MediaAssetInfo {
    int32_t assetId = -1;
    int32_t newAssetId = -1;
    int32_t ownerAlbumId = -1;
};

static int32_t UpdateRelationship(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const MediaAssetInfo &assetInfo,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr)
{
    const std::string UPDATE_ALBUM_ID_FOR_COPY_ASSET = "UPDATE Photos SET owner_album_id = " +
        to_string(assetInfo.ownerAlbumId) + " WHERE file_id = " + to_string(assetInfo.newAssetId);
    int32_t ret = -1;
    if (assetRefresh) {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(assetInfo.newAssetId));
        assetRefresh->Init(predicates);
        ret = assetRefresh->ExecuteSql(UPDATE_ALBUM_ID_FOR_COPY_ASSET, AccurateRefresh::RDB_OPERATION_UPDATE);
    } else {
        ret = rdbStore->ExecuteSql(UPDATE_ALBUM_ID_FOR_COPY_ASSET);
    }
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DROP_HANDLED_MAP_RELATIONSHIP failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    const std::string DROP_HANDLED_MAP_RELATIONSHIP =
    "UPDATE PhotoMap SET dirty = '4' WHERE " + PhotoMap::ASSET_ID + " = '" + to_string(assetInfo.assetId) +
        "' AND " + PhotoMap::ALBUM_ID + " = '" + to_string(assetInfo.ownerAlbumId) + "'";
    ret = rdbStore->ExecuteSql(DROP_HANDLED_MAP_RELATIONSHIP);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DROP_HANDLED_MAP_RELATIONSHIP failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_INFO_LOG("Update handled copy meta success, rowId = %{public}d, ", assetInfo.newAssetId);
    return E_OK;
}

static int32_t GenerateThumbnail(const int32_t &assetId, const std::string &targetPath,
    shared_ptr<NativeRdb::ResultSet> &resultSet, bool isSyncGenerateThumbnail)
{
    if (ThumbnailService::GetInstance() == nullptr) {
        return E_FAIL;
    }
    std::string displayName = "";
    GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_NAME, displayName);
    int64_t dateTaken = 0;
    GetLongValueFromResultSet(resultSet, MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    int64_t dateModified = 0;
    GetLongValueFromResultSet(resultSet, MediaColumn::MEDIA_DATE_MODIFIED, dateModified);
    std::string uri = PHOTO_URI_PREFIX + to_string(assetId) + MediaFileUtils::GetExtraUri(displayName, targetPath) +
        "?api_version=10&date_modified=" + to_string(dateModified) + "&date_taken=" + to_string(dateTaken);
    MEDIA_INFO_LOG("Begin generate thumbnail %{public}s, ", uri.c_str());
    int32_t err = ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, targetPath, isSyncGenerateThumbnail);
    if (err != E_SUCCESS) {
        MEDIA_ERR_LOG("ThumbnailService CreateThumbnailFileScaned failed : %{public}d", err);
    }
    MEDIA_INFO_LOG("Generate thumbnail %{public}s, success ", uri.c_str());
    return err;
}

static int32_t UpdateCoverInfoForAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &oldAssetId, const int32_t &ownerAlbumId, int64_t &newAssetId, const std::string &targetPath)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    const std::string QUERY_ALBUM_COVER_INFO =
        "SELECT cover_uri, cover_uri_source FROM PhotoAlbum WHERE album_id = " + to_string(ownerAlbumId) +
        " AND cover_uri like 'file://media/Photo/" + to_string(oldAssetId) + "%'";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_ALBUM_COVER_INFO);
    if (resultSet == nullptr) {
        MEDIA_INFO_LOG("No need to update cover_uri");
        return E_OK;
    }
    auto ret = resultSet->GoToNextRow();
    if (ret == NativeRdb::E_OK) {
        int32_t coverUriSource = 0;
        GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::COVER_URI_SOURCE, coverUriSource);
        if (coverUriSource >= CoverUriSource::MANUAL_CLOUD_COVER) {
            MEDIA_INFO_LOG("Manual cover do not update cover_uri");
            return E_OK;
        }
    } else {
        MEDIA_INFO_LOG("ResultSet GoToNextRow error, ret:%{public}d, ownerAlbumId:%{public}d", ret, ownerAlbumId);
        return E_OK;
    }
    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("map operation flag is true");
        string newCoverUri = MediaLibraryFormMapOperations::GetUriByFileId(newAssetId, targetPath);
        MEDIA_INFO_LOG("New cover uri is %{public}s", targetPath.c_str());
        const std::string updateAlbumCoverUri =
            "UPDATE PhotoAlbum SET cover_uri = '" + newCoverUri +"' WHERE album_id = " + to_string(ownerAlbumId);
        ret = upgradeStore->ExecuteSql(updateAlbumCoverUri);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
            "update cover uri failed, ret = %{public}d, target album is %{public}d", ret, ownerAlbumId);
    }
    return E_OK;
}

static int32_t CopyLocalFile(shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &ownerAlbumId,
    const std::string displayName, std::string &targetPath, const int32_t &assetId)
{
    MEDIA_INFO_LOG("begin copy local file, fileId:%{public}d, and target album:%{public}d", assetId, ownerAlbumId);
    std::string srcPath = "";
    GetSourceFilePath(srcPath, resultSet);

    int32_t mediaType;
    GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_TYPE, mediaType);
    MediaLibraryAlbumFusionUtils::BuildTargetFilePath(targetPath, displayName, mediaType);
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("Build target path fail, origin file is %{public}s", srcPath.c_str());
        return E_INVALID_PATH;
    }
    MEDIA_INFO_LOG("begin copy local file, scrPath is %{public}s, and target path is %{public}s",
        srcPath.c_str(), targetPath.c_str());
    // Copy photo files, supporting copy moving photo's video and extraData folder.
    int32_t err = PhotoFileOperation().CopyPhoto(resultSet, targetPath);
    if (err != E_OK) {
        MEDIA_ERR_LOG("CopyPhoto failed, srcPath = %{public}s, targetPath = %{public}s, ret = %{public}d",
            srcPath.c_str(), targetPath.c_str(), err);
        return err;
    }
    return E_OK;
}

static int32_t CopyMateData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, shared_ptr<NativeRdb::ResultSet>
    &resultSet, int64_t &newAssetId, std::string &targetPath, const MediaAssetCopyInfo &copyInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyMateData");
    NativeRdb::ValuesBucket values;
    int32_t err = BuildInsertValuesBucket(upgradeStore, values, resultSet, copyInfo);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Insert meta data fail and delete migrated file %{public}s ", targetPath.c_str());
        DeleteFile(targetPath);
        return err;
    }
    err = copyMetaData(upgradeStore, newAssetId, values);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Insert meta data fail and delete migrated file err %{public}d ", err);
        // If insert fails, delete the moved file to avoid wasted space
        DeleteFile(targetPath);
        return err;
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId,
    std::string displayName)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyLocalSingleFile");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }

    int32_t assetId;
    GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_ID, assetId);
    GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_NAME, displayName);
    std::string targetPath = "";
    int32_t err = CopyLocalFile(resultSet, ownerAlbumId, displayName, targetPath, assetId);
    CHECK_AND_RETURN_RET_INFO_LOG(err == E_OK, E_ERR, "Failed to copy local file.");

    MediaAssetCopyInfo copyInfo(targetPath, false, ownerAlbumId, displayName);
    err = CopyMateData(upgradeStore, resultSet, newAssetId, targetPath, copyInfo);
    if (err != E_OK) {
        MEDIA_INFO_LOG("Failed to copy local file.");
        return E_ERR;
    }

    err = UpdateRelationship(upgradeStore, {assetId, newAssetId, ownerAlbumId});
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_OK, "UpdateRelationship fail, assetId: %{public}d,"
        " newAssetId: %{public}" PRId64 "ownerAlbumId: %{public}d, ret = %{public}d",
        assetId, newAssetId, ownerAlbumId, err);

    err = PhotoFileOperation().CopyThumbnail(resultSet, targetPath, newAssetId);
    if (err != E_OK && GenerateThumbnail(newAssetId, targetPath, resultSet, false) != E_SUCCESS) {
        MediaLibraryRdbUtils::UpdateThumbnailRelatedDataToDefault(upgradeStore, newAssetId);
        MEDIA_ERR_LOG("Copy thumbnail failed, targetPath = %{public}s, ret = %{public}d, newAssetId = %{public}" PRId64,
            targetPath.c_str(), err, newAssetId);
        return err;
    }
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
}

static int32_t CopyLocalSingleFileSync(shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh, const int32_t
    &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId, const std::string displayName)
{
    MediaLibraryTracer tracer;
    tracer.Start("CopyLocalSingleFileSync");
    auto upgradeStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }

    int32_t assetId;
    GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_ID, assetId);
    std::string targetPath = "";
    int32_t err = CopyLocalFile(resultSet, ownerAlbumId, displayName, targetPath, assetId);
    if (err != E_OK) {
        MEDIA_INFO_LOG("Failed to copy local file.");
        return E_ERR;
    }

    MediaAssetCopyInfo copyInfo(targetPath, false, ownerAlbumId, displayName, false, true, false, false);
    tracer.Start("CopyMateData");
    NativeRdb::ValuesBucket values;
    err = BuildInsertValuesBucket(upgradeStore, values, resultSet, copyInfo);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Insert meta data fail and delete migrated file %{public}s ", targetPath.c_str());
        DeleteFile(targetPath);
        return err;
    }
    err = assetRefresh->Insert(newAssetId, PhotoColumn::PHOTOS_TABLE, values);
    if (err != E_OK) {
        MEDIA_INFO_LOG("Failed to copy local file.");
        DeleteFile(targetPath);
        return E_ERR;
    }
    MEDIA_DEBUG_LOG("Insert copy meta data success, ret = %{public}d, rowId = %{public}" PRId64, err, newAssetId);
    tracer.Finish();

    err = UpdateRelationship(upgradeStore, {assetId, newAssetId, ownerAlbumId}, assetRefresh);
    if (err != E_OK) {
        MEDIA_ERR_LOG("UpdateRelationship fail, assetId: %{public}d, newAssetId: %{public}lld,"
            "ownerAlbumId: %{public}d, ret = %{public}d", assetId, (long long)newAssetId, ownerAlbumId, err);
        return E_OK;
    }

    err = PhotoFileOperation().CopyThumbnail(resultSet, targetPath, newAssetId);
    if (err != E_OK && GenerateThumbnail(newAssetId, targetPath, resultSet, true) != E_SUCCESS) {
        MediaLibraryRdbUtils::UpdateThumbnailRelatedDataToDefault(upgradeStore, newAssetId);
        MEDIA_ERR_LOG("Copy thumbnail failed, targetPath = %{public}s, ret = %{public}d, newAssetId = %{public}" PRId64,
            targetPath.c_str(), err, newAssetId);
        return err;
    }
    return E_OK;
}

void MediaLibraryAlbumFusionUtils::SetRefreshAlbum(bool needRefresh)
{
    isNeedRefreshAlbum = needRefresh;
}

int32_t MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &assetId, const int32_t &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet,
    int64_t &newAssetId)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("Begin copy cloud file, fileId is %{public}d, and target album is %{public}d",
        assetId, ownerAlbumId);
    std::string srcPath = "";
    std::string targetPath = "";
    GetSourceFilePath(srcPath, resultSet);

    std::string displayName;
    int32_t mediaType;
    GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_NAME, displayName);
    GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_TYPE, mediaType);
    MediaLibraryAlbumFusionUtils::BuildTargetFilePath(targetPath, displayName, mediaType);
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("Build target path fail, origin file is %{public}s", srcPath.c_str());
        return E_INVALID_PATH;
    }
    MEDIA_INFO_LOG("Begin copy thumbnail original scrPath is %{public}s, and target path is %{public}s",
        srcPath.c_str(), targetPath.c_str());
    int32_t err = CopyOriginThumbnail(srcPath, targetPath);
    CHECK_AND_RETURN_RET(err == E_OK, err);

    MediaAssetCopyInfo copyInfo(targetPath, true, ownerAlbumId);
    NativeRdb::ValuesBucket values;
    err = BuildInsertValuesBucket(upgradeStore, values, resultSet, copyInfo);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Build meta data fail and delete migrated file %{public}s ", targetPath.c_str());
        DeleteThumbnail(targetPath);
        return err;
    }
    err = copyMetaData(upgradeStore, newAssetId, values);
    if (err != E_OK) {
        // If insert fails, delete the moved file to avoid wasted space
        MEDIA_ERR_LOG("Build meta data fail and delete migrated file %{public}s ", targetPath.c_str());
        DeleteThumbnail(targetPath);
        return err;
    }
    ThumbnailService::GetInstance()->CreateAstcCloudDownload(to_string(newAssetId), true);
    err = UpdateRelationship(upgradeStore, {assetId, newAssetId, ownerAlbumId});
    CHECK_AND_RETURN_RET(err == E_OK, err);
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
}

void SendNewAssetNotify(string newFileAssetUri, const shared_ptr<MediaLibraryRdbStore> rdbStore,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    assetRefresh->RefreshAlbum(NotifyAlbumType::SYS_ALBUM);
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("Can not get MediaLibraryNotify, fail to send new asset notify.");
        return;
    }
    watch->Notify(newFileAssetUri, NotifyType::NOTIFY_ADD);
    watch->Notify(newFileAssetUri, NotifyType::NOTIFY_ALBUM_ADD_ASSET);
    assetRefresh->Notify();
}

int32_t MediaLibraryAlbumFusionUtils::CloneSingleAsset(const int64_t &assetId, const string title)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore.");
        return E_DB_FAIL;
    }

    const std::string querySql = "SELECT * FROM Photos WHERE file_id = ?";
    std::vector<NativeRdb::ValueObject> params = { assetId };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql, params);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }

    string oldDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    string suffix = MediaFileUtils::SplitByChar(oldDisplayName, '.');
    if (suffix.empty()) {
        MEDIA_ERR_LOG("Failed to get file suffix.");
        return E_FAIL;
    }

    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::CLONE_SINGLE_ASSET_BUSSINESS_NAME);
    string displayName = title + "." + suffix;
    int32_t ownerAlbumId;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
    int64_t newAssetId = -1;
    int32_t err = CopyLocalSingleFileSync(assetRefresh, ownerAlbumId, resultSet, newAssetId, displayName);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Clone local asset failed, ret = %{public}d, assetId = %{public}lld", err, (long long)assetId);
        return err;
    }

    RdbPredicates newPredicates(PhotoColumn::PHOTOS_TABLE);
    newPredicates.EqualTo(PhotoColumn::MEDIA_ID, newAssetId);
    vector<string> columns = {
        PhotoColumn::MEDIA_FILE_PATH
    };
    shared_ptr<NativeRdb::ResultSet> newResultSet = rdbStore->Query(newPredicates, columns);
    if (newResultSet == nullptr || newResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }

    string newFileAssetUri = MediaFileUtils::GetFileAssetUri(GetStringVal(MediaColumn::MEDIA_FILE_PATH, newResultSet),
        displayName, newAssetId);
    SendNewAssetNotify(newFileAssetUri, rdbStore, assetRefresh);
    MEDIA_INFO_LOG("End clone asset, newAssetId = %{public}lld", (long long)newAssetId);
    return newAssetId;
}

static void SavePackageMetaDate(NativeRdb::ValuesBucket &values)
{
    std::string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    std::string packageName = PermissionUtils::GetPackageNameByBundleName(bundleName);
    std::string appId = PermissionUtils::GetAppIdByBundleName(bundleName);
    values.Put(MediaColumn::MEDIA_OWNER_PACKAGE, bundleName);
    values.Put(MediaColumn::MEDIA_PACKAGE_NAME, packageName);
    values.Put(MediaColumn::MEDIA_OWNER_APPID, appId);
}

static void SaveDefaultMetaData(NativeRdb::ValuesBucket &values, shared_ptr<NativeRdb::ResultSet> resultSet,
    const std::string &path, bool isBurst, const std::string &displayName)
{
    std::string sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    int64_t editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
    int32_t supportedWaterMark = GetInt32Val(PhotoColumn::SUPPORTED_WATERMARK_TYPE, resultSet);

    values.Put(MediaColumn::MEDIA_FILE_PATH, path);
    values.Put(MediaColumn::MEDIA_NAME, displayName);
    values.Put(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(displayName));
    values.Put(PhotoColumn::PHOTO_MEDIA_SUFFIX, MediaFileUtils::GetExtensionFromPath(displayName));

    // data
    values.Put(MediaColumn::MEDIA_TIME_PENDING, 0);
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.Put(MediaColumn::MEDIA_DATE_DELETED, 0);
    values.Put(MediaColumn::MEDIA_HIDDEN, 0);
    values.Put(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    values.Put(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    values.Put(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(LcdReady::GENERATE_LCD_COMPLETED));
    values.Put(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    values.Put(PhotoColumn::PHOTO_HIDDEN_TIME, 0);
    values.Put(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    values.Put(PhotoColumn::PHOTO_IS_TEMP, 0);
    values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    values.Put(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::COVER));
    values.Put(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int32_t>(MetadataFlags::TYPE_NEW));
    values.Put(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    values.Put(PhotoColumn::MEDIA_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, supportedWaterMark);
    values.Put(PhotoColumn::PHOTO_EDIT_TIME, editTime > 0 ? MediaFileUtils::UTCTimeSeconds() : 0);
    if (isBurst) {
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
        values.Put(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    }
    if (!sourcePath.empty()) {
        std::string newSourcePath = MediaFileUtils::GetParentPath(sourcePath) + "/" + displayName;
        values.Put(PhotoColumn::PHOTO_SOURCE_PATH, newSourcePath);
    }
}

static bool SaveConvertFormatMetaData(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh,
    std::shared_ptr<NativeRdb::ResultSet> resultSet, const std::string &path, const std::string &displayName,
    int64_t &newAssetId)
{
    MEDIA_INFO_LOG("SaveConvertFormatMetaData start");
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != E_OK) {
        MEDIA_ERR_LOG("stat path: %{public}s failed, errno: %{public}d", path.c_str(), errno);
        return false;
    }

    NativeRdb::ValuesBucket values;
    for (const auto &[columnName, columnType]: convertFormatCommonColumn) {
        ParsingAndFillValue(values, columnName, columnType, resultSet);
    }

    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    bool isBurst = (subtype == static_cast<int32_t>(PhotoSubType::BURST) ||
        originalSubtype == static_cast<int32_t>(PhotoSubType::BURST));

    SaveDefaultMetaData(values, resultSet, path, isBurst, displayName);
    SavePackageMetaDate(values);

    int32_t ret = assetRefresh->Insert(newAssetId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Insert failed, ret: %{public}d", ret);
        return false;
    }

    MEDIA_INFO_LOG("SaveConvertFormatMetaData success, newAssetId: %{public}" PRId64, newAssetId);
    return true;
}

static int32_t ConvertFormatFileSync(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh, std::shared_ptr<NativeRdb::ResultSet> resultSet,
    const std::string &displayName, int64_t &newAssetId, std::string &targetPath)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertFormatFileSync");
    if (upgradeStore == nullptr || assetRefresh == nullptr || resultSet == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }

    int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    MediaLibraryAlbumFusionUtils::BuildTargetFilePath(targetPath, displayName, mediaType);
    std::string extension = MediaFileUtils::GetExtensionFromPath(displayName);
    MEDIA_INFO_LOG("ConvertFormatPhoto failed, displayName: %{public}s, targetPath: %{public}s",
        displayName.c_str(), targetPath.c_str());
    int32_t err = PhotoFileOperation().ConvertFormatPhoto(resultSet, targetPath, extension);
    if (err != E_OK) {
        MEDIA_ERR_LOG("ConvertFormatPhoto failed, err: %{public}d", err);
        return E_ERR;
    }

    if (!SaveConvertFormatMetaData(assetRefresh, resultSet, targetPath, displayName, newAssetId)) {
        MEDIA_ERR_LOG("SaveConvertFormatMetaData failed");
        DeleteFile(targetPath);
        return E_ERR;
    }
    err = PhotoFileOperation().CopyThumbnail(resultSet, targetPath, newAssetId);
    if (err != E_OK && GenerateThumbnail(newAssetId, targetPath, resultSet, true) != E_SUCCESS) {
        MediaLibraryRdbUtils::UpdateThumbnailRelatedDataToDefault(upgradeStore, newAssetId);
        MEDIA_ERR_LOG("Copy thumbnail failed, targetPath = %{public}s, ret = %{public}d, newAssetId = %{public}" PRId64,
            targetPath.c_str(), err, newAssetId);
        return err;
    }
    return E_OK;
}

static bool CheckConvertFormatAsset(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::shared_ptr<NativeRdb::ResultSet> resultSet, const std::string &newTitle)
{
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    if (position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        MEDIA_ERR_LOG("pure cloud asset is invalid");
        return false;
    }
    int32_t isTemp = GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet);
    if (isTemp != 0) {
        MEDIA_ERR_LOG("isTemp: %{public}d is invalid", isTemp);
        return false;
    }
    int64_t timePending = GetInt64Val(MediaColumn::MEDIA_TIME_PENDING, resultSet);
    if (timePending != 0) {
        MEDIA_ERR_LOG("timePending: %{public}" PRId64 " is invalid", timePending);
        return false;
    }
    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    if (hidden != 0) {
        MEDIA_ERR_LOG("hidden: %{public}d is invalid", hidden);
        return false;
    }
    int64_t dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    int64_t dateDeleted = GetInt64Val(MediaColumn::MEDIA_DATE_DELETED, resultSet);
    if (dateTrashed != 0 || dateDeleted != 0) {
        MEDIA_ERR_LOG("dateTrashed: %{public}" PRId64 ", dateDeleted: %{public}" PRId64 " is invalid",
            dateTrashed, dateDeleted);
        return false;
    }
    int32_t ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    RdbPredicates newPredicates(PhotoColumn::PHOTOS_TABLE);
    newPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
    newPredicates.And()->EqualTo(MediaColumn::MEDIA_TITLE, newTitle);
    shared_ptr<NativeRdb::ResultSet> titleResultSet = rdbStore->Query(newPredicates, {MediaColumn::MEDIA_TITLE});
    CHECK_AND_RETURN_RET_LOG(titleResultSet != nullptr, false, "query albumId: %{public}d title failed", ownerAlbumId);
    CHECK_AND_RETURN_RET_LOG(titleResultSet->GoToFirstRow() != NativeRdb::E_OK, false,
        "newTitle is same in album: %{public}d", ownerAlbumId);
    return true;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryAlbumFusionUtils::ConvertFormatAsset(const int64_t &assetId,
    const std::string &title, const std::string &extension)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore.");
        return nullptr;
    }

    const std::string querySql = "SELECT * FROM Photos WHERE file_id = ?";
    std::vector<NativeRdb::ValueObject> params = { assetId };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql, params);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return nullptr;
    }
    if (!CheckConvertFormatAsset(rdbStore, resultSet, title)) {
        MEDIA_ERR_LOG("CheckConvertFormatAsset failed");
        return nullptr;
    }
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::CONVERT_FORMAT_ASSET_BUSSINESS_NAME);
    string displayName = title + "." + extension;
    int64_t newAssetId = -1;
    std::string targetPath;
    int32_t err = ConvertFormatFileSync(rdbStore, assetRefresh, resultSet, displayName, newAssetId, targetPath);
    if (err != E_OK) {
        MEDIA_ERR_LOG("ConvertFormatFileSync failed, ret = %{public}d, assetId = %{public}" PRId64, err, assetId);
        return nullptr;
    }
    MediaScannerManager::GetInstance()->ScanFileSync(targetPath, nullptr, MediaLibraryApi::API_10, true, newAssetId);

    RdbPredicates newPredicates(PhotoColumn::PHOTOS_TABLE);
    newPredicates.EqualTo(PhotoColumn::MEDIA_ID, newAssetId);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_TYPE, PhotoColumn::PHOTO_SUBTYPE };
    shared_ptr<NativeRdb::ResultSet> newResultSet = rdbStore->Query(newPredicates, columns);
    if (newResultSet == nullptr) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return nullptr;
    }

    string newFileAssetUri = MediaFileUtils::GetFileAssetUri(targetPath, displayName, newAssetId);
    SendNewAssetNotify(newFileAssetUri, rdbStore, assetRefresh);
    MEDIA_INFO_LOG("ConvertFormatAsset success, newAssetId = %{public}" PRId64, newAssetId);
    return newResultSet;
}

static int32_t GetNoOwnerDataCnt(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, 0);
    vector<string> columns;
    int rowCount = 0;
    shared_ptr<NativeRdb::ResultSet> resultSet = store->Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GetRowCount(rowCount) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query not matched data fails");
    }
    MEDIA_INFO_LOG("Begin handle no owner data: count %{public}d", rowCount);
    return rowCount;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNoOwnerData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    auto rowCount = GetNoOwnerDataCnt(upgradeStore);
    SetRefreshAlbum(rowCount > 0);
    if (rowCount <= 0) {
        return E_OK;
    }
    const std::string UPDATE_NO_OWNER_ASSET_INTO_OTHER_ALBUM = "UPDATE PHOTOS SET owner_album_id = "
        "(SELECT album_id FROM PhotoAlbum where album_name = '其它') WHERE owner_album_id = 0";
    int32_t ret = upgradeStore->ExecuteSql(UPDATE_NO_OWNER_ASSET_INTO_OTHER_ALBUM);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UPDATE_NO_OWNER_ASSET_INTO_OTHER_ALBUM failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleRestData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &assetId, const std::vector<int32_t> &restOwnerAlbumIds, int32_t &handledCount)
{
    MEDIA_INFO_LOG("Begin handle rest data assetId is %{public}d", assetId);
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    const std::string QUERY_FILE_META_INFO =
        "SELECT * FROM Photos WHERE file_id = " + to_string(assetId);
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_FILE_META_INFO);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }
    int64_t newAssetId = -1;
    if (isLocalAsset(resultSet)) {
        MEDIA_INFO_LOG("file is local asset %{public}d", assetId);
        // skip first one, already handled
        for (size_t i = 0; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = CopyLocalSingleFile(upgradeStore, restOwnerAlbumIds[i], resultSet, newAssetId);
            if (err != E_OK) {
                MEDIA_WARN_LOG("Copy file fails, fileId is %{public}d", assetId);
                continue;
            }
            MEDIA_INFO_LOG("Copy file success, fileId is %{public}d, albumId is %{public}d",
                assetId, restOwnerAlbumIds[i]);
            handledCount++;
        }
    } else {
        MEDIA_INFO_LOG("file is cloud asset %{public}d", assetId);
        // skip first one, already handled
        for (size_t i = 0; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = CopyCloudSingleFile(upgradeStore, assetId, restOwnerAlbumIds[i], resultSet, newAssetId);
            if (err != E_OK) {
                MEDIA_WARN_LOG("Copy cloud file fails, fileId is %{public}d", assetId);
                continue;
            }
            MEDIA_INFO_LOG("Copy cloud file success, fileId is %{public}d, albumId is %{public}d",
                assetId, restOwnerAlbumIds[i]);
            handledCount++;
        }
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, std::multimap<int32_t, vector<int32_t>> &notMathedMap)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    static int handledCount = 0;
    for (auto it = notMathedMap.begin(); it != notMathedMap.end(); ++it) {
        HandleRestData(upgradeStore, it->first, it->second, handledCount);
    }
    MEDIA_INFO_LOG("handled %{public}d not matched items", handledCount);
    // Put no relationship asset into other album
    HandleNoOwnerData(upgradeStore);
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(const shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &assetId, const int32_t &ownerAlbumId, int64_t &newAssetId)
{
    MEDIA_INFO_LOG("Begin copy single file assetId is %{public}d", assetId);
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    const std::string QUERY_FILE_META_INFO =
        "SELECT * FROM Photos WHERE file_id = " + to_string(assetId);
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_FILE_META_INFO);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_INFO_LOG(!cond, E_DB_FAIL, "Query not matched data fails");

    int32_t err = E_OK;
    if (isLocalAsset(resultSet)) {
        err = CopyLocalSingleFile(upgradeStore, ownerAlbumId, resultSet, newAssetId);
    } else {
        err = CopyCloudSingleFile(upgradeStore, assetId, ownerAlbumId, resultSet, newAssetId);
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Copy file fails, is file local : %{public}d,"
        " fileId is %{public}d", isLocalAsset(resultSet), assetId);
    MEDIA_INFO_LOG("Copy file success, fileId is %{public}d, albumId is %{public}d,"
        "and copyed file id is %{public}" PRId64, assetId, ownerAlbumId, newAssetId);
    return E_OK;
}

static int32_t QueryTotalNumberNeedToHandle(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const std::string &querySql)
{
    int32_t rowCount = 0;
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return rowCount;
    }
    if (querySql.empty()) {
        return rowCount;
    }
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("No need to update cover_uri");
        return rowCount;
    }
    if (resultSet->GetInt(0, rowCount) != NativeRdb::E_OK) {
        return rowCount;
    }
    return rowCount;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_2: Start handle not matched relationship");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t totalNumber = QueryTotalNumberNeedToHandle(upgradeStore, QUERY_NOT_MATCHED_COUNT_IN_PHOTOMAP);
    MEDIA_INFO_LOG("QueryTotalNumberNeedToHandle, totalNumber=%{public}d", totalNumber);
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    for (int32_t offset = 0; offset < totalNumber; offset += ALBUM_FUSION_BATCH_COUNT) {
        MEDIA_INFO_LOG("ALBUM_FUSE: handle batch clean, offset: %{public}d", offset);
        notMatchedMap.clear();
        int32_t err = QueryNoMatchedMap(upgradeStore, notMatchedMap, true);
        CHECK_AND_BREAK_ERR_LOG(err == NativeRdb::E_OK, "Fatal error! Failed to query not matched map data");

        if (notMatchedMap.size() != 0) {
            MEDIA_INFO_LOG("There are %{public}d items need to migrate", (int)notMatchedMap.size());
            HandleNotMatchedDataMigration(upgradeStore, notMatchedMap);
        }
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_2: end handle not matched relationship, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static void QuerySourceAlbumLPath(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    std::string &lPath, const std::string bundle_name, const std::string album_name)
{
    std::string queryExpiredAlbumInfo = "";
    if (bundle_name.empty()) {
        queryExpiredAlbumInfo = "SELECT lPath FROM album_plugin WHERE "
            "album_name = '" + album_name + "' AND priority = '1'";
    } else {
        queryExpiredAlbumInfo = "SELECT lPath FROM album_plugin WHERE bundle_name = '" + bundle_name +
            "' OR album_name = '" + album_name + "' AND priority = '1'";
    }
    shared_ptr<NativeRdb::ResultSet> albumPluginResultSet = upgradeStore->QuerySql(queryExpiredAlbumInfo);
    if (albumPluginResultSet == nullptr || albumPluginResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query lpath data fails, bundleName is %{public}s and albumName is %{public}s",
            bundle_name.c_str(), album_name.c_str());
        lPath = "/Pictures/" + album_name;
        return;
    }
    GetStringValueFromResultSet(albumPluginResultSet, PhotoAlbumColumns::ALBUM_LPATH, lPath);
    if (lPath.empty()) {
        lPath = "/Pictures/" + album_name;
    }
    MEDIA_ERR_LOG("Album lPath is %{public}s", lPath.c_str());
}

void MediaLibraryAlbumFusionUtils::BuildAlbumInsertValuesSetName(
    const std::shared_ptr<MediaLibraryRdbStore>& upgradeStore, NativeRdb::ValuesBucket &values,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const string &newAlbumName)
{
    const unordered_map<string, ColumnSchema>& photoAlbumSchema =
        MediaLibraryAlbumOperations::GetPhotoAlbumTableSchema();
    for (auto it = photoAlbumSchema.begin(); it != photoAlbumSchema.end(); ++it) {
        ParsingAndFillValueForAllAlbumColumns(values, it->first, it->second, resultSet);
    }

    std::string lPath = "";
    int32_t albumType = -1;
    GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_LPATH, lPath);
    GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_TYPE, albumType);
    if (albumType == PhotoAlbumType::SOURCE) {
        size_t lastSlashIndex = lPath.find_last_of("/\\");
        if (lastSlashIndex != std::string::npos) {
            lPath = lPath.substr(0, lastSlashIndex + 1) + newAlbumName;
        }
        values.Delete(PhotoAlbumColumns::ALBUM_BUNDLE_NAME);
    } else if (albumType == PhotoAlbumType::USER) {
        lPath = "/Pictures/Users/" + newAlbumName;
    }

    values.Delete(PhotoAlbumColumns::ALBUM_PRIORITY);
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.Delete(PhotoAlbumColumns::ALBUM_LPATH);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    values.Delete(PhotoAlbumColumns::ALBUM_NAME);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, newAlbumName);
    values.Delete(PhotoAlbumColumns::ALBUM_CLOUD_ID);
    values.Delete(PhotoAlbumColumns::ALBUM_DIRTY);
}

static int32_t CopyAlbumMetaData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    std::shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &oldAlbumId, int64_t &newAlbumId)
{
    MEDIA_INFO_LOG("Begin copy album Meta Data!!!");
    bool cond = (upgradeStore == nullptr || resultSet == nullptr || oldAlbumId == -1);
    CHECK_AND_RETURN_RET_LOG(!cond, E_INVALID_ARGUMENTS, "invalid parameter");
    NativeRdb::ValuesBucket values;
    for (auto it = albumColumnTypeMap.begin(); it != albumColumnTypeMap.end(); ++it) {
        std::string columnName = it->first;
        ResultSetDataType columnType = it->second;
        ParsingAndFillValue(values, columnName, columnType, resultSet);
    }

    newAlbumId =
        PhotoAlbumCopyMetaDataOperation()
            .SetRdbStore(upgradeStore)
            .CopyAlbumMetaData(values);
    CHECK_AND_RETURN_RET(newAlbumId > 0, E_HAS_DB_ERROR);
    MEDIA_ERR_LOG("Insert copyed album success,oldAlbumId is = %{public}d newAlbumId is %{public}" PRId64,
        oldAlbumId, newAlbumId);
    return E_OK;
}

static int32_t DeleteOldAlbum(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject,
    int32_t oldAlbumId, bool isCloudAblum)
{
    std::string DELETE_EXPIRED_ALBUM = "";
    if (isCloudAblum) {
        DELETE_EXPIRED_ALBUM = "UPDATE PhotoAlbum SET dirty = '4' WHERE album_id = " + to_string(oldAlbumId);
    } else {
        DELETE_EXPIRED_ALBUM = "DELETE FROM PhotoAlbum WHERE album_id = " + to_string(oldAlbumId);
    }
    int32_t ret = E_HAS_DB_ERROR;
    if (executeObject.albumRefresh != nullptr) {
        RdbPredicates rdbPredicatesAlbum(PhotoAlbumColumns::TABLE);
        rdbPredicatesAlbum.EqualTo(PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
        executeObject.albumRefresh->Init(rdbPredicatesAlbum);
        ret = executeObject.albumRefresh->ExecuteSql(
            DELETE_EXPIRED_ALBUM, isCloudAblum ? AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE :
            AccurateRefresh::RdbOperation::RDB_OPERATION_REMOVE);
    } else {
        ret = executeObject.trans->ExecuteSql(DELETE_EXPIRED_ALBUM);
    }
    return ret;
}

static int32_t UpdateAlbumPhotoOwnerAlbumId(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject,
    int32_t oldAlbumId, int64_t newAlbumId, const vector<string>* fileIdsInAlbum)
{
    string UpdatePhotoOwnerAlbumSql;
    if (fileIdsInAlbum) {
        UpdatePhotoOwnerAlbumSql = "UPDATE Photos SET owner_album_id = " +
            to_string(newAlbumId) + ", meta_date_modified = strftime('%s000', 'now') WHERE file_id IN (";
        bool isFirst = true;
        for (const auto& fileId : *fileIdsInAlbum) {
            if (!isFirst) {
                UpdatePhotoOwnerAlbumSql += ", ";
            }
            UpdatePhotoOwnerAlbumSql += fileId;
            isFirst = false;
        }
        UpdatePhotoOwnerAlbumSql += ")";
    } else {
        UpdatePhotoOwnerAlbumSql = "UPDATE Photos SET owner_album_id = " + to_string(newAlbumId) +
            ", meta_date_modified = strftime('%s000', 'now') WHERE owner_album_id = " + to_string(oldAlbumId);
    }
    if (executeObject.assetRefresh != nullptr) {
        RdbPredicates rdbPredicatesPhoto(PhotoColumn::PHOTOS_TABLE);
        if (fileIdsInAlbum) {
            rdbPredicatesPhoto.And()->In(MediaColumn::MEDIA_ID, *fileIdsInAlbum);
        } else {
            rdbPredicatesPhoto.And()->EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, oldAlbumId);
        }
        executeObject.assetRefresh->Init(rdbPredicatesPhoto);
        return executeObject.assetRefresh->ExecuteSql(
            UpdatePhotoOwnerAlbumSql, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    } else {
        return executeObject.trans->ExecuteSql(UpdatePhotoOwnerAlbumSql);
    }
}

static bool IsDeleteOtherAlbum(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject,
    int32_t oldAlbumId)
{
    CHECK_AND_RETURN_RET_LOG(executeObject.rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");
    const std::string querySql = "SELECT lpath from PhotoAlbum WHERE album_id = " + std::to_string(oldAlbumId);
    auto resultSet = executeObject.rdbStore->QueryByStep(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Is delete other album, find album resultSet null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("IsDeleteOtherAlbum first row empty");
        resultSet->Close();
        return false;
    }
    std::string deletedAlbumLPath = MediaLibraryRdbStore::GetString(resultSet, PhotoAlbumColumns::ALBUM_LPATH);
    resultSet->Close();
    if (deletedAlbumLPath == "/Pictures/其它") {
        return true;
    }
    return false;
}

static int32_t RecreateOtherAlbum(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject)
{
    CHECK_AND_RETURN_RET_LOG(executeObject.albumRefresh != nullptr, E_HAS_DB_ERROR, "albumRefresh is null");
    int32_t ret = executeObject.albumRefresh->ExecuteSql(CREATE_DEFALUT_ALBUM_FOR_NO_RELATIONSHIP_ASSET,
        AccurateRefresh::RDB_OPERATION_ADD);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_HAS_DB_ERROR,
        "Cannot insert new othersAlbum into PhotoAlbum table, ret: %{public}d", ret);
    return E_OK;
}

static int32_t BatchDeleteAlbumAndUpdateRelation(MediaLibraryAlbumFusionUtils::ExecuteObject& executeObject,
    int32_t oldAlbumId, int64_t newAlbumId, bool isCloudAblum, const vector<string>* fileIdsInAlbum = nullptr)
{
    CHECK_AND_RETURN_RET_LOG(executeObject.trans != nullptr, E_HAS_DB_ERROR, "transactionOprn is null");

    bool isOthers = IsDeleteOtherAlbum(executeObject, oldAlbumId);
    int32_t ret = DeleteOldAlbum(executeObject, oldAlbumId, isCloudAblum);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "DELETE expired album failed, ret = %{public}d, albumId is %{public}d",
        ret, oldAlbumId);

    const std::string UPDATE_NEW_ALBUM_ID_IN_PHOTO_MAP = "UPDATE PhotoMap SET map_album = " +
        to_string(newAlbumId) + " WHERE dirty != '4' AND map_album = " + to_string(oldAlbumId);
    ret = executeObject.trans->ExecuteSql(UPDATE_NEW_ALBUM_ID_IN_PHOTO_MAP);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Update relationship in photo map fails, ret = %{public}d, albumId is %{public}d",
        ret, oldAlbumId);

    ret = UpdateAlbumPhotoOwnerAlbumId(executeObject, oldAlbumId, newAlbumId, fileIdsInAlbum);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Update relationship in photo map fails, ret = %{public}d, albumId is %{public}d",
        ret, oldAlbumId);
    if (isOthers) {
        ret = RecreateOtherAlbum(executeObject);
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::DeleteAlbumAndUpdateRelationship(
    ExecuteObject& executeObject, const int32_t &oldAlbumId, const int64_t &newAlbumId,
    bool isCloudAblum, const vector<string>* fileIdsInAlbum)
{
    if (executeObject.rdbStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    if (newAlbumId == -1) {
        MEDIA_ERR_LOG("Target album id error, origin albumId is %{public}d", oldAlbumId);
        return E_INVALID_ARGUMENTS;
    }

    int32_t errCode = E_OK;
    if (executeObject.albumRefresh != nullptr && executeObject.assetRefresh != nullptr) {
        errCode = BatchDeleteAlbumAndUpdateRelation(
            executeObject, oldAlbumId, newAlbumId, isCloudAblum, fileIdsInAlbum);
    } else {
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        ExecuteObject object;
        object.trans = trans;
        std::function<int(void)> func = [&]()->int {
            return BatchDeleteAlbumAndUpdateRelation(object, oldAlbumId, newAlbumId, isCloudAblum, fileIdsInAlbum);
        };
        errCode = trans->RetryTrans(func);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("DeleteAlbumAndUpdateRelationship trans retry fail!, ret = %{public}d", errCode);
        }
    }
    return errCode;
}

bool MediaLibraryAlbumFusionUtils::IsCloudAlbum(shared_ptr<NativeRdb::ResultSet> resultSet)
{
    string cloudId = "";
    GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_CLOUD_ID, cloudId);
    return !cloudId.empty();
}

int32_t MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    const std::string QUERY_EXPIRED_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE (album_type = 2048 OR album_type = 0) "
        "AND cloud_id not like '%default-album%' AND (lpath IS NULL OR lpath = '') AND dirty != 4";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_EXPIRED_ALBUM_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query not matched data fails");
        return E_HAS_DB_ERROR;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldAlbumId = -1;
        int64_t newAlbumId = -1;
        GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
        CopyAlbumMetaData(upgradeStore, resultSet, oldAlbumId, newAlbumId);
        ExecuteObject executeObject;
        executeObject.rdbStore = upgradeStore;
        DeleteAlbumAndUpdateRelationship(executeObject, oldAlbumId, newAlbumId, IsCloudAlbum(resultSet));
        MEDIA_ERR_LOG("Finish handle old album %{public}d, new inserted album id is %{public}" PRId64,
            oldAlbumId, newAlbumId);
    }
    return E_OK;
}

static int32_t KeepHiddenAlbumAssetSynced(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    const std::string UPDATE_DIRTY_FLAG_FOR_DUAL_HIDDEN_ASSET =
    "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET dirty = 0 WHERE owner_album_id ="
        "(SELECT album_id FROM PhotoALbum where (cloud_id = "
        "'default-album-4' OR album_name = '.hiddenAlbum') and dirty != 4)";
    int32_t err = upgradeStore->ExecuteSql(UPDATE_DIRTY_FLAG_FOR_DUAL_HIDDEN_ASSET);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            UPDATE_DIRTY_FLAG_FOR_DUAL_HIDDEN_ASSET.c_str());
        return err;
    }
    return E_OK;
}

static int32_t RemediateErrorSourceAlbumSubType(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    const std::string REMEDIATE_ERROR_SOURCE_ALBUM_SUBTYPE =
        "UPDATE " + PhotoAlbumColumns::TABLE + " SET album_subtype = 2049 "
        "WHERE album_type = 2048 and album_subtype <> 2049";
    int32_t err = upgradeStore->ExecuteSql(REMEDIATE_ERROR_SOURCE_ALBUM_SUBTYPE);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            REMEDIATE_ERROR_SOURCE_ALBUM_SUBTYPE.c_str());
        return err;
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Start rebuild album table and compensate loss value");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int32_t err = HandleChangeNameAlbum(upgradeStore);
    CompensateLpathForLocalAlbum(upgradeStore);
    HandleExpiredAlbumData(upgradeStore);
    // Keep dual hidden assets dirty state synced, let cloudsync handle compensating for hidden flags
    KeepHiddenAlbumAssetSynced(upgradeStore);
    RemediateErrorSourceAlbumSubType(upgradeStore);
    HandleMisMatchScreenRecord(upgradeStore);
    int32_t albumAffectedCount = PhotoAlbumLPathOperation::GetInstance()
                                     .SetRdbStore(upgradeStore)
                                     .Start()
                                     .CleanInvalidPhotoAlbums()
                                     .CleanDuplicatePhotoAlbums()
                                     .CleanEmptylPathPhotoAlbums()
                                     .GetAlbumAffectedCount();
    MediaLibraryAlbumFusionUtils::SetRefreshAlbum(albumAffectedCount > 0);
    MEDIA_INFO_LOG("End rebuild album table and compensate loss value");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &sourceAlbumId, const int64_t &targetAlbumId)
{
    CHECK_AND_RETURN_RET_LOG(upgradeStore != nullptr, E_DB_FAIL, "fail to get rdbstore");
    MEDIA_INFO_LOG("MergeClashSourceAlbum %{public}d, target album is %{public}" PRId64,
        sourceAlbumId, targetAlbumId);
    if (sourceAlbumId == targetAlbumId) {
        return E_OK;
    }
    ExecuteObject executeObject;
    executeObject.rdbStore = upgradeStore;
    DeleteAlbumAndUpdateRelationship(executeObject, sourceAlbumId, targetAlbumId, IsCloudAlbum(resultSet));
    return E_OK;
}

static int32_t MergeScreenShotAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    CHECK_AND_RETURN_RET_LOG(upgradeStore != nullptr, E_DB_FAIL, "fail to get rdbstore");
    MEDIA_INFO_LOG("Begin handle expired screen shot album data ");
    int32_t oldAlbumId = -1;
    int64_t newAlbumId = -1;
    GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
    const std::string QUERY_NEW_SCREEN_SHOT_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE album_type = 2048 AND bundle_name = 'com.huawei.hmos.screenshot'"
        " AND dirty != 4";
    shared_ptr<NativeRdb::ResultSet> newAlbumResultSet = upgradeStore->QuerySql(QUERY_NEW_SCREEN_SHOT_ALBUM_INFO);
    MEDIA_INFO_LOG("Begin merge screenshot album, old album is %{public}d", oldAlbumId);
    if (newAlbumResultSet == nullptr || newAlbumResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        // Create a new bundle name screenshot album
        CopyAlbumMetaData(upgradeStore, resultSet, oldAlbumId, newAlbumId);
        MEDIA_INFO_LOG("Create new screenshot album, album id is %{public}" PRId64, newAlbumId);
    } else {
        GetLongValueFromResultSet(newAlbumResultSet, PhotoAlbumColumns::ALBUM_ID, newAlbumId);
    }
    MEDIA_INFO_LOG("Begin merge screenshot album, new album is %{public}" PRId64, newAlbumId);
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, oldAlbumId, newAlbumId);
    MEDIA_INFO_LOG("End handle expired screen shot album data ");
    return E_OK;
}

static int32_t MergeScreenRecordAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    CHECK_AND_RETURN_RET_INFO_LOG(upgradeStore != nullptr, E_DB_FAIL, "fail to get rdbstore");
    MEDIA_INFO_LOG("Begin merge screenrecord album");
    int32_t oldAlbumId = -1;
    int64_t newAlbumId = -1;
    GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
    const std::string QUERY_NEW_SCREEN_RECORD_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE album_type = 2048 AND bundle_name = 'com.huawei.hmos.screenrecorder'"
        " AND dirty != 4";
    shared_ptr<NativeRdb::ResultSet> newAlbumResultSet = upgradeStore->QuerySql(QUERY_NEW_SCREEN_RECORD_ALBUM_INFO);
    if (newAlbumResultSet == nullptr || newAlbumResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        // Create a new bundle name screenshot album
        CopyAlbumMetaData(upgradeStore, resultSet, oldAlbumId, newAlbumId);
        MEDIA_INFO_LOG("Create new screenrecord album, album id is %{public}" PRId64, newAlbumId);
    } else {
        GetLongValueFromResultSet(newAlbumResultSet, PhotoAlbumColumns::ALBUM_ID, newAlbumId);
    }
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, oldAlbumId, newAlbumId);
    MEDIA_INFO_LOG("End merge screenrecord album");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Begin handle change name album data");
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    const std::string QUERY_CHANGE_NAME_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE album_type = 2048"
        " AND (bundle_name = 'com.huawei.ohos.screenshot' OR bundle_name = 'com.huawei.ohos.screenrecorder')"
        " AND dirty != 4";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_CHANGE_NAME_ALBUM_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query expired bundle_name fails");
        return E_HAS_DB_ERROR;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string bundle_name = "";
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundle_name);
        if (bundle_name == "com.huawei.ohos.screenshot") {
            MergeScreenShotAlbum(upgradeStore, resultSet);
        } else {
            MergeScreenRecordAlbum(upgradeStore, resultSet);
        }
    }
    MEDIA_INFO_LOG("End handle change name album data");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CompensateLpathForLocalAlbum(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Begin compensate Lpath for local album");
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    const std::string QUERY_COMPENSATE_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE cloud_id IS NULL"
        " AND (priority IS NULL OR lpath IS NULL) AND dirty != 4 AND album_type IN (0, 2048)";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_COMPENSATE_ALBUM_INFO);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query album info fails");
        return E_HAS_DB_ERROR;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int album_id = -1;
        int32_t album_type = -1;
        std::string album_name = "";
        std::string bundle_name = "";
        std::string lpath = "";

        GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, album_id);
        GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_TYPE, album_type);
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_NAME, album_name);
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundle_name);
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_LPATH, lpath);

        if (lpath.empty()) {
            if (album_type == OHOS::Media::PhotoAlbumType::SOURCE) {
                QuerySourceAlbumLPath(upgradeStore, lpath, bundle_name, album_name);
            } else {
                lpath = "/Pictures/Users/" + album_name;
                MEDIA_INFO_LOG("Album type is user type and lPath is %{public}s!!!", lpath.c_str());
            }
        }

        const std::string UPDATE_COMPENSATE_ALBUM_DATA =
            "UPDATE PhotoAlbum SET lpath = '" + lpath + "', "
            "priority = COALESCE ((SELECT priority FROM album_plugin WHERE lpath = '" + lpath + "'), 1) "
            "WHERE album_id = " + to_string(album_id);
        int32_t err = upgradeStore->ExecuteSql(UPDATE_COMPENSATE_ALBUM_DATA);
        CHECK_AND_CONTINUE_ERR_LOG(err == NativeRdb::E_OK,
            "Fatal error! Failed to exec: %{public}s", UPDATE_COMPENSATE_ALBUM_DATA.c_str());
    }
    MEDIA_INFO_LOG("End compensate Lpath for local album");
    return E_OK;
}

void MediaLibraryAlbumFusionUtils::SetParameterToStopSync()
{
    auto currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_INFO_LOG("Set parameter for album fusion currentTime:%{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(ALBUM_FUSION_FLAG, currentTime);
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to set parameter cloneFlag, retFlag:%{public}d", retFlag);
    }
}

void MediaLibraryAlbumFusionUtils::SetParameterToStartSync()
{
    MEDIA_INFO_LOG("Reset parameter for album fusion");
    bool retFlag = system::SetParameter(ALBUM_FUSION_FLAG, "0");
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to Set parameter for album fusion, retFlag:%{public}d", retFlag);
    }
}

int32_t MediaLibraryAlbumFusionUtils::GetAlbumFuseUpgradeStatus()
{
    std::string albumFuseUpgradeStatus = system::GetParameter(ALBUM_FUSION_UPGRADE_STATUS_FLAG, "1");
    MEDIA_ERR_LOG("Current album upgrade status :%{public}s", albumFuseUpgradeStatus.c_str());
    if (albumFuseUpgradeStatus == "1") {
        return ALBUM_FUSION_UPGRADE_SUCCESS;
    } else {
        return ALBUM_FUSION_UPGRADE_FAIL;
    }
}

int32_t MediaLibraryAlbumFusionUtils::SetAlbumFuseUpgradeStatus(int32_t upgradeStatus)
{
    if (upgradeStatus != ALBUM_FUSION_UPGRADE_SUCCESS && upgradeStatus != ALBUM_FUSION_UPGRADE_FAIL) {
        MEDIA_ERR_LOG("Invalid parameter for album fusion upgrade status :%{public}d", upgradeStatus);
        return E_INVALID_ARGUMENTS;
    }
    MEDIA_INFO_LOG("Set parameter for album fusion upgrade status :%{public}d", upgradeStatus);
    bool retFlag = system::SetParameter(ALBUM_FUSION_UPGRADE_STATUS_FLAG, to_string(upgradeStatus));
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to set parameter, retFlag:%{public}d", retFlag);
        return E_INVALID_MODE;
    }
    return E_OK;
}

static std::string ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

static int32_t DeleteDuplicatePhoto(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    const string sql = "UPDATE Photos SET dirty = 8 WHERE file_id IN ( " +
        SQL_GET_DUPLICATE_PHOTO + " )";

    int32_t err = store->ExecuteSql(sql);
    CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "DeleteDuplicatePhoto fail %{public}d", err);
    return err;
}

void DuplicateDebugPrint(const vector<int32_t> &idArr)
{
    constexpr int32_t maxPrintWidth = 50;
    string assetStr;
    for (auto assetId: idArr) {
        assetStr += to_string(assetId) + ",";
        if (assetStr.size() > maxPrintWidth) {
            MEDIA_DEBUG_LOG("delete dup photo %{public}s", assetStr.c_str());
            assetStr = "";
        }
    }
    if (assetStr.size() != 0) {
        MEDIA_DEBUG_LOG("delete dup photo %{public}s", assetStr.c_str());
    }
}

void DuplicateDebug(std::shared_ptr<NativeRdb::ResultSet> resultSet, vector<int32_t> &idArr)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t colIndex = -1;
        int32_t assetId = 0;
        resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, colIndex);
        CHECK_AND_BREAK_ERR_LOG(resultSet->GetInt(colIndex, assetId) == NativeRdb::E_OK, "db error");
        idArr.push_back(assetId);
    }
}

static int32_t HandleDuplicatePhoto(const std::shared_ptr<MediaLibraryRdbStore> store)
{
    int32_t row = 0;
    int32_t count = 0;
    // set max loop count to avoid trapped in loop if delete fail
    constexpr int32_t maxLoopCount = 1000;
    do {
        count++;
        shared_ptr<NativeRdb::ResultSet> resultSet = store->QuerySql(SQL_GET_DUPLICATE_PHOTO);
        if (resultSet == nullptr || resultSet->GetRowCount(row) != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("rdb fail");
            return E_DB_FAIL;
        }
        MEDIA_INFO_LOG("duplicate photo %{public}d, need to delete", row);
        if (row == 0) {
            return E_OK;
        }
        vector<int32_t> idArr;
        DuplicateDebug(resultSet, idArr);
        auto err = DeleteDuplicatePhoto(store);
        if (err == NativeRdb::E_OK) {
            DuplicateDebugPrint(idArr);
        } else {
            MEDIA_ERR_LOG("duplicate photo %{public}d, delete fail %{public}d", row, err);
        }
    } while (row > 0 && count < maxLoopCount);

    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Media_Operation: Skip HandleDuplicateAlbum.");
    return E_OK;
}

static void HandleNewCloudDirtyDataImp(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet, std::vector<int32_t> &restOwnerAlbumIds, int32_t &assetId)
{
    int64_t newAssetId = -1;
    if (isLocalAsset(resultSet)) {
        MEDIA_INFO_LOG("File is local asset %{public}d", assetId);
        // skip first one, already handled
        for (size_t i = 0; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(upgradeStore,
                restOwnerAlbumIds[i], resultSet, newAssetId);
            if (err != E_OK) {
                MEDIA_WARN_LOG("Copy file fails, fileId is %{public}d", assetId);
                continue;
            }
            MEDIA_INFO_LOG("Copy file success, fileId is %{public}d, albumId is %{public}d",
                assetId, restOwnerAlbumIds[i]);
        }
    } else {
        MEDIA_INFO_LOG("File is cloud asset %{public}d", assetId);
        // skip first one, already handled
        for (size_t i = 0; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(upgradeStore,
                assetId, restOwnerAlbumIds[i], resultSet, newAssetId);
            if (err != E_OK) {
                MEDIA_WARN_LOG("Copy cloud file fails, fileId is %{public}d", assetId);
                continue;
            }
            MEDIA_INFO_LOG("Copy cloud file success, fileId is %{public}d, albumId is %{public}d",
                assetId, restOwnerAlbumIds[i]);
        }
    }
}

int32_t MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    std::multimap<int32_t, vector<int32_t>> &notMathedMap)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    for (auto it = notMathedMap.begin(); it != notMathedMap.end(); ++it) {
        int32_t assetId = it->first;
        std::vector<int32_t> &restOwnerAlbumIds = it->second;
        const std::string QUERY_FILE_META_INFO =
            "SELECT * FROM Photos WHERE file_id = " + to_string(assetId);
        shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_FILE_META_INFO);
        bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
        CHECK_AND_RETURN_RET_INFO_LOG(!cond, E_DB_FAIL, "Query not matched data fails");
        HandleNewCloudDirtyDataImp(upgradeStore, resultSet, restOwnerAlbumIds, assetId);
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::TransferMisMatchScreenRecord(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Transfer miss matched screeRecord begin");
    const std::string QUERY_SCREEN_RECORD_ALBUM =
        "SELECT album_id FROM PhotoAlbum WHERE bundle_name ='com.huawei.hmos.screenrecorder' AND dirty <>4";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_SCREEN_RECORD_ALBUM);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("No screen record album");
        const std::string CREATE_SCREEN_RECORDS_ALBUM =
            "INSERT INTO " + PhotoAlbumColumns::TABLE +
            "(album_type, album_subtype, album_name,bundle_name, dirty, is_local, date_added, lpath, priority)"
            " Values ('2048', '2049', '屏幕录制', 'com.huawei.hmos.screenrecorder', '1', '1',"
            " strftime('%s000', 'now'), '/Pictures/Screenrecords', '1')";
        int32_t err = upgradeStore->ExecuteSql(CREATE_SCREEN_RECORDS_ALBUM);
        CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
            "Fatal error! Failed to exec: %{public}s", CREATE_SCREEN_RECORDS_ALBUM.c_str());
    }
    const std::string TRANSFER_MISS_MATCH_ASSET =
        "UPDATE Photos SET owner_album_id = "
        "(SELECT album_id FROM PhotoAlbum WHERE bundle_name ='com.huawei.hmos.screenrecorder' AND dirty <>4) "
        "WHERE owner_album_id = (SELECT album_id FROM PhotoAlbum WHERE "
        "bundle_name ='com.huawei.hmos.screenshot' AND dirty <>'4' limit 1) AND media_type =2";
    int32_t err = upgradeStore->ExecuteSql(TRANSFER_MISS_MATCH_ASSET);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, err,
        "Fatal error! Failed to exec: %{public}s", TRANSFER_MISS_MATCH_ASSET.c_str());
    SetRefreshAlbum(true);
    MEDIA_INFO_LOG("Transfer miss matched screenRecord end");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleMisMatchScreenRecord(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore");
        return E_INVALID_ARGUMENTS;
    }
    const std::string QUERY_MISS_MATCHED_RECORDS =
        "SELECT file_id FROM Photos WHERE owner_album_id = "
        "(SELECT album_id FROM PhotoAlbum WHERE bundle_name ='com.huawei.hmos.screenshot' AND dirty <>4) "
        " AND media_type =2";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_MISS_MATCHED_RECORDS);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("No miss matched screen record");
        return E_OK;
    }
    return TransferMisMatchScreenRecord(upgradeStore);
}

static int32_t UpdateTranscodeTime(int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "Failed to get rdbStore.");

    ValuesBucket value;
    value.PutLong(PhotoColumn::PHOTO_TRANSCODE_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    int32_t changeRows = 0;
    int32_t err = rdbStore->Update(changeRows, value, rdbPredicate);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, E_INNER_FAIL, "Failed to update[%{public}d].", err);
    MEDIA_INFO_LOG("UpdateTranscodeTime success");
    return E_OK;
}

static int32_t CheckTmpCompatibleDup(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int32_t fileId, int32_t &dupExist)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INNER_FAIL,
        "no matched data.");
    dupExist = GetInt32Val(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, resultSet);
    if (dupExist > 0) {
        MEDIA_INFO_LOG("compatible duplicate file is exists");
        return UpdateTranscodeTime(fileId);
    }
    std::string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
    CHECK_AND_RETURN_RET_LOG(mimeType == "image/heic" || mimeType == "image/heif", E_PARAM_CONVERT_FORMAT,
        "mimeType is invalid, mimeType: %{public}s", mimeType.c_str());
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    CHECK_AND_RETURN_RET_LOG(position != static_cast<int32_t>(PhotoPositionType::CLOUD), E_PARAM_CONVERT_FORMAT,
        "pure cloud asset is invalid, position: %{public}d", position);

    int32_t isTemp = GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet);
    CHECK_AND_RETURN_RET_LOG(isTemp == 0, E_PARAM_CONVERT_FORMAT, "photo is temp");

    int64_t timePending = GetInt64Val(MediaColumn::MEDIA_TIME_PENDING, resultSet);
    CHECK_AND_RETURN_RET_LOG(timePending == 0, E_PARAM_CONVERT_FORMAT, "photo is timePending");

    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    CHECK_AND_RETURN_RET_LOG(hidden == 0, E_PARAM_CONVERT_FORMAT, "photo is hidden");

    int64_t dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    int64_t dateDeleted = GetInt64Val(MediaColumn::MEDIA_DATE_DELETED, resultSet);
    CHECK_AND_RETURN_RET_LOG(dateTrashed == 0 && dateDeleted == 0, E_PARAM_CONVERT_FORMAT,
        "photo is trashed or deleted");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CreateTmpCompatibleDup(int32_t fileId, const std::string &path, size_t &size,
    int32_t &dupExist)
{
    auto dfxManager = DfxManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dfxManager != nullptr, E_INVALID_VALUES, "DfxManager::GetInstance() returned nullptr");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore.");
        dfxManager->HandleTranscodeFailed(INNER_FAILED);
        return E_INNER_FAIL;
    }

    const std::string querySql = R"(SELECT exist_compatible_duplicate, position, is_temp, time_pending, hidden,
        date_trashed, date_deleted, mime_type FROM Photos WHERE file_id = ?)";
    std::vector<NativeRdb::ValueObject> params = { fileId };
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql, params);
    dupExist = 0;
    auto err = CheckTmpCompatibleDup(resultSet, fileId, dupExist);
    CHECK_AND_EXECUTE(resultSet == nullptr, resultSet->Close());
    if (dupExist > 0) {
        return err;
    }
    if (err == E_OK) {
        return PhotoFileOperation().CreateTmpCompatibleDup(path, size);
    }
    MEDIA_ERR_LOG("CheckTmpCompatibleDup fail %{public}d", err);
    dfxManager->HandleTranscodeFailed(INNER_FAILED);
    return err;
}

int32_t MediaLibraryAlbumFusionUtils::RefreshAllAlbums()
{
    MEDIA_INFO_LOG("Froce refresh all albums start");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("Froce refresh all albums end");
    return E_OK;
}

bool MediaLibraryAlbumFusionUtils::ScreenOnInterrupt()
{
    if (MedialibrarySubscriber::IsCurrentStatusOn()) {
        return false;
    }
    SetParameterToStartSync();
    if (isNeedRefreshAlbum.load() == true) {
        RefreshAllAlbums();
        isNeedRefreshAlbum = false;
    }
    return true;
}

void MediaLibraryAlbumFusionUtils::MigratePhotoMapData(const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbstore is nullptr");
        return;
    }
    int32_t totalNumber = QueryTotalNumberNeedToHandle(rdbStore, QUERY_NEW_NOT_MATCHED_COUNT_IN_PHOTOMAP);
    MEDIA_INFO_LOG("QueryTotalNumberNeedToHandle, totalNumber=%{public}d", totalNumber);
    SetRefreshAlbum(totalNumber > 0);
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    for (int32_t offset = 0; offset < totalNumber; offset += ALBUM_FUSION_BATCH_COUNT) {
        MEDIA_INFO_LOG("DATA_CLEAN: handle batch clean, offset: %{public}d", offset);
        notMatchedMap.clear();
        int32_t err = QueryNoMatchedMap(rdbStore, notMatchedMap, false);
        CHECK_AND_BREAK_ERR_LOG(err == NativeRdb::E_OK, "Fatal error! Failed to query not matched map data");

        if (notMatchedMap.size() != 0) {
            MEDIA_INFO_LOG("There are %{public}d items need to migrate", (int)notMatchedMap.size());
            HandleNewCloudDirtyData(rdbStore, notMatchedMap);
        }
    }
}

const std::vector<std::function<void(const std::shared_ptr<MediaLibraryRdbStore>& store)>>
    MediaLibraryAlbumFusionUtils::ALBUM_FUSION_CLEAN_TASKS = {
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { MigratePhotoMapData(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) {
        HandleDuplicateAlbum(store);
    },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { HandleDuplicatePhoto(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) {
        HandleNoOwnerData(store);
    },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { HandleChangeNameAlbum(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { CompensateLpathForLocalAlbum(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { HandleExpiredAlbumData(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { KeepHiddenAlbumAssetSynced(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { RemediateErrorSourceAlbumSubType(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) { HandleMisMatchScreenRecord(store); },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) {
        int32_t albumAffectedCount = PhotoAlbumLPathOperation::GetInstance()
                                     .SetRdbStore(store)
                                     .Start()
                                     .CleanInvalidPhotoAlbums()
                                     .CleanDuplicatePhotoAlbums()
                                     .CleanEmptylPathPhotoAlbums()
                                     .GetAlbumAffectedCount();
        SetRefreshAlbum(albumAffectedCount > 0);
    },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) {
        PhotoAlbumUpdateDateModifiedOperation photoAlbumOperation;
        if (photoAlbumOperation.CheckAlbumDateNeedFix(store)) {
            photoAlbumOperation.UpdateAlbumDateNeedFix(store);
            SetRefreshAlbum(true);
        }
    },
    [](const std::shared_ptr<MediaLibraryRdbStore>& store) {
        PhotoSourcePathOperation().ResetPhotoSourcePath(store);
    }
};

int32_t MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData(bool isBackgroundExecute)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore, try again!");
        rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        if (rdbStore == nullptr) {
            MEDIA_ERR_LOG("Fatal error! Failed to get rdbstore, new cloud data is not processed!!");
            return E_DB_FAIL;
        }
    }
    if (GetAlbumFuseUpgradeStatus() != ALBUM_FUSION_UPGRADE_SUCCESS) {
        MEDIA_ERR_LOG("ALBUM_FUSE: First upgrade fails, perform upgrade again.");
        MediaLibraryRdbStore::ReconstructMediaLibraryStorageFormat(rdbStore);
        return E_OK;
    }
    std::unique_lock<std::mutex> cloudAlbumAndDataUniqueLock(
        MediaLibraryAlbumFusionUtils::cloudAlbumAndDataMutex_, std::defer_lock);
    if (!cloudAlbumAndDataUniqueLock.try_lock()) {
        MEDIA_WARN_LOG("ALBUM_FUSE: Failed to acquire lock, skipping task Clean.");
        return E_OK;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DATA_CLEAN:Clean invalid cloud album and dirty data start!");
    SetParameterToStopSync();
    for (auto& task : ALBUM_FUSION_CLEAN_TASKS) {
        if (isBackgroundExecute && ScreenOnInterrupt()) {
            MEDIA_INFO_LOG("DATA_CLEAN:Screen on, interrupt album fusion background task");
            return E_OK;
        }
        task(rdbStore);
    }

    SetParameterToStartSync();
    if (isNeedRefreshAlbum.load() == true) {
        RefreshAllAlbums();
        isNeedRefreshAlbum = false;
    }

    MEDIA_INFO_LOG("DATA_CLEAN:Clean invalid cloud album and dirty data, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static int QueryCount(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const string& sql, const string& column)
{
    if (rdbStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return -1;
    }
    auto resultSet = rdbStore->QueryByStep(sql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, failed when executing sql: %{public}s", sql.c_str());
        return -1;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query result go to first row failed, sql: %{public}s", sql.c_str());
        return -1;
    }
    return GetInt32Val(column, resultSet);
}

void MediaLibraryAlbumFusionUtils::ReportAlbumFusionData(int64_t albumFusionTag, AlbumFusionState albumFusionState,
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    AlbumFusionDfxDataPoint dataPoint;
    dataPoint.albumFusionTag = albumFusionTag;
    dataPoint.reportTimeStamp = MediaFileUtils::UTCTimeMilliSeconds();
    dataPoint.albumFusionState = static_cast<int32_t>(albumFusionState);
    MEDIA_INFO_LOG("ALBUM_FUSE: Report album fusion data start, tag is %{public}" PRId64 ", fusion state is %{public}d",
        albumFusionTag, static_cast<int32_t>(albumFusionState));

    dataPoint.imageAssetCount = QueryCount(rdbStore,
        "SELECT count FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::IMAGE),
        "count");
    dataPoint.videoAssetCount = QueryCount(rdbStore,
        "SELECT count FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::VIDEO),
        "count");
    dataPoint.numberOfSourceAlbum = QueryCount(rdbStore,
        "SELECT count(*) FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::SOURCE_GENERIC),
        "count(*)");
    dataPoint.numberOfUserAlbum = QueryCount(rdbStore,
        "SELECT count(*) FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::USER_GENERIC),
        "count(*)");
    dataPoint.totalAssetsInSourceAlbums = QueryCount(rdbStore,
        "SELECT sum(count) FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::SOURCE_GENERIC),
        "sum(count)");
    dataPoint.totalAssetsInUserAlbums = QueryCount(rdbStore,
        "SELECT sum(count) FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::USER_GENERIC),
        "sum(count)");
    dataPoint.albumDetails = "";
    int32_t hiddenAssetCount = QueryCount(rdbStore,
        "SELECT count FROM PhotoAlbum WHERE album_subtype = " + to_string(PhotoAlbumSubType::HIDDEN),
        "count");
    int32_t dotHiddenAlbumAssetCount = QueryCount(rdbStore,
        "SELECT count FROM PhotoAlbum WHERE album_name = '.hiddenAlbum' AND dirty <> 4 AND album_subtype = " +
            to_string(PhotoAlbumSubType::SOURCE_GENERIC),
        "count");
    dataPoint.hiddenAssetInfo = "{hidden assets: " + to_string(hiddenAssetCount) + ", .hiddenAlbum assets: " +
        to_string(dotHiddenAlbumAssetCount) + "}";

    DfxReporter::ReportAlbumFusion(dataPoint);
    MEDIA_INFO_LOG("ALBUM_FUSE: Report album fusion data end, tag is %{public}" PRId64 ", fusion state is %{public}d",
        albumFusionTag, albumFusionState);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media