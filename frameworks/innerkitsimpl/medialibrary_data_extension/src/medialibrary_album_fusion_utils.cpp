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

#include <cerrno>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

#include "dfx_reporter.h"
#include "medialibrary_type_const.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdbstore.h"
#include "metadata.h"
#include "media_file_utils.h"
#include "medialibrary_album_compatibility_fusion_sql.h"
#include "medialibrary_album_refresh.h"
#include "parameters.h"
#include "photo_file_operation.h"
#include "photo_burst_operation.h"
#include "photo_displayname_operation.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "userfile_manager_types.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

constexpr int32_t POSITION_LOCAL_FLAG = 1;
constexpr int32_t POSITION_CLOUD_FLAG = 2;
constexpr int32_t POSITION_BOTH_FLAG = 3;
constexpr int32_t CLOUD_COPY_DIRTY_FLAG = 7;

constexpr int32_t TIME_STAMP_OFFSET = 5;
constexpr int32_t MAX_RETRY_TIMES = 100;
constexpr int32_t TRANSACTION_WAIT_INTERVAL = 50; // in milliseconds.
const std::string ALBUM_FUSION_FLAG = "multimedia.medialibrary.cloneFlag";
const std::string ALBUM_FUSION_UPGRADE_STATUS_FLAG = "persist.multimedia.medialibrary.albumFusion.status";
const int32_t ALBUM_FUSION_UPGRADE_SUCCESS = 1;
const int32_t ALBUM_FUSION_UPGRADE_FAIL = 0;
const int32_t ALBUM_FUSION_BATCH_COUNT = 200;

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
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_FRONT_CAMERA, ResultSetDataType::TYPE_STRING},
    {PhotoColumn::PHOTO_BURST_COVER_LEVEL, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::SUPPORTED_WATERMARK_TYPE, ResultSetDataType::TYPE_INT32},
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
};

std::mutex MediaLibraryAlbumFusionUtils::cloudAlbumAndDataMutex_;

int32_t MediaLibraryAlbumFusionUtils::RemoveMisAddedHiddenData(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_0: Start remove misadded hidden data");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t err = upgradeStore->ExecuteSql(DROP_UNWANTED_ALBUM_RELATIONSHIP_FOR_HIDDEN_ALBUM_ASSET);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to drop unwanted album relationship for .hiddenAlbum! Failed to exec: %{public}s",
            DROP_UNWANTED_ALBUM_RELATIONSHIP_FOR_HIDDEN_ALBUM_ASSET.c_str());
        return err;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_0: End remove misadded hidden data, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static int32_t PrepareTempUpgradeTable(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, int32_t &matchedCount)
{
    int32_t err = upgradeStore->ExecuteSql(DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", DROP_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
        return err;
    }
    MEDIA_INFO_LOG("ALBUM_FUSE begin exec: %{public}s", CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
    err = upgradeStore->ExecuteSql(CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", CREATE_TEMP_UPGRADE_PHOTO_MAP_TABLE.c_str());
        return err;
    }
    auto resultSet = upgradeStore->QuerySql(QUERY_MATCHED_COUNT);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query matched data fails");
        return E_DB_FAIL;
    }
    resultSet->GetInt(0, matchedCount);
    MEDIA_INFO_LOG("ALBUM_FUSE: There are %{public}d matched items", matchedCount);
    err = upgradeStore->ExecuteSql(CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_MAP_ASSET);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_MAP_ASSET.c_str());
        return err;
    }
    err = upgradeStore->ExecuteSql(CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_PHOTO_MAP);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", CREATE_UNIQUE_TEMP_UPGRADE_INDEX_ON_PHOTO_MAP.c_str());
        return err;
    }
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
        if (resultSet->GetInt(colIndex, albumId) != NativeRdb::E_OK) {
            return E_HAS_DB_ERROR;
        }
        resultSet->GetColumnIndex(PhotoMap::ASSET_ID, colIndex);
        if (resultSet->GetInt(colIndex, assetId) != NativeRdb::E_OK) {
            return E_HAS_DB_ERROR;
        }
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

static inline void buildTargetFilePath(const std::string &srcPath, std::string &targetPath)
{
    size_t underlineIndex = srcPath.find_last_of('_');
    size_t dotIndex = srcPath.find_last_of('.');
    if (underlineIndex == std::string::npos || dotIndex == std::string::npos || underlineIndex >= dotIndex) {
        MEDIA_INFO_LOG("Invalid file path format : %{public}s", srcPath.c_str());
        return;
    }
    std::string currentTime = std::to_string(MediaFileUtils::UTCTimeMilliSeconds());
    std::string timeStamp = currentTime.substr(currentTime.length() - TIME_STAMP_OFFSET, TIME_STAMP_OFFSET);
    targetPath = srcPath.substr(0, underlineIndex + 1) + timeStamp + srcPath.substr(dotIndex);
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
        int32_t err = CopyDirectory(originalThumbnailDirPath, targetThumbnailDirPath);
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

struct MediaAssetCopyInfo {
    std::string targetPath;
    bool isCopyThumbnail;
    int32_t ownerAlbumId;
    std::string displayName;
    MediaAssetCopyInfo(const std::string& targetPath, bool isCopyThumbnail, int32_t ownerAlbumId,
        const std::string& displayName = "")
        : targetPath(targetPath), isCopyThumbnail(isCopyThumbnail), ownerAlbumId(ownerAlbumId),
        displayName(displayName) {}
};

static void HandleLowQualityAssetValuesBucket(shared_ptr<NativeRdb::ResultSet>& resultSet,
    NativeRdb::ValuesBucket& values)
{
    int32_t dirty = -1;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_DIRTY, dirty);
    int32_t photoQuality = 0;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_QUALITY, photoQuality);
    if (photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL);
        dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
        values.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
    }
    values.PutInt(PhotoColumn::PHOTO_QUALITY, photoQuality);
    if (dirty == -1 && photoQuality != static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        MEDIA_WARN_LOG("Status error, dirty is -1, cannot upload");
        values.PutInt(PhotoColumn::PHOTO_DIRTY, -1);
    }
}

static int32_t BuildInsertValuesBucket(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    NativeRdb::ValuesBucket &values, shared_ptr<NativeRdb::ResultSet> &resultSet, const MediaAssetCopyInfo &copyInfo)
{
    std::string targetPath = copyInfo.targetPath;
    bool isCopyThumbnail = copyInfo.isCopyThumbnail;
    int32_t ownerAlbumId = copyInfo.ownerAlbumId;
    std::string displayName = copyInfo.displayName;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, targetPath);
    std::string uniqueDisplayName = PhotoDisplayNameOperation().FindDisplayName(rdbStore, resultSet, ownerAlbumId,
        displayName);
    if (!uniqueDisplayName.empty()) {
        values.PutString(MediaColumn::MEDIA_NAME, uniqueDisplayName);
        values.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(uniqueDisplayName));
    } else {
        MEDIA_ERR_LOG("Failed to get unique display name");
    }
    std::string burstKey = PhotoBurstOperation().FindBurstKey(rdbStore, resultSet, ownerAlbumId, uniqueDisplayName);
    if (!burstKey.empty()) {
        values.PutString(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    }
    for (auto it = commonColumnTypeMap.begin(); it != commonColumnTypeMap.end(); ++it) {
        string columnName = it->first;
        ResultSetDataType columnType = it->second;
        ParsingAndFillValue(values, columnName, columnType, resultSet);
    }
    if (isCopyThumbnail) {
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
    HandleLowQualityAssetValuesBucket(resultSet, values);
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

static int32_t UpdateRelationship(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const int32_t &assetId,
    const int32_t &newAssetId, const int32_t &ownerAlbumId, bool isLocalAsset)
{
    const std::string UPDATE_ALBUM_ID_FOR_COPY_ASSET =
        "UPDATE Photos SET owner_album_id = " + to_string(ownerAlbumId) + " WHERE file_id = " + to_string(newAssetId);
    int32_t ret = rdbStore->ExecuteSql(UPDATE_ALBUM_ID_FOR_COPY_ASSET);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UPDATE_ALBUM_ID_FOR_COPY_ASSET failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    const std::string DROP_HANDLED_MAP_RELATIONSHIP =
    "UPDATE PhotoMap SET dirty = '4' WHERE " + PhotoMap::ASSET_ID + " = '" + to_string(assetId) +
        "' AND " + PhotoMap::ALBUM_ID + " = '" + to_string(ownerAlbumId) + "'";
    ret = rdbStore->ExecuteSql(DROP_HANDLED_MAP_RELATIONSHIP);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DROP_HANDLED_MAP_RELATIONSHIP failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    if (!isLocalAsset) {
        const std::string INDICATE_FILE_NEED_CLOUD_COPY =
        "UPDATE Photos SET dirty = '7' WHERE " + PhotoMap::ASSET_ID + " = '" + to_string(newAssetId);
    }
    MEDIA_INFO_LOG("Update handled copy meta success, rowId = %{public}d, ", newAssetId);
    return E_OK;
}

static int32_t GenerateThumbnail(const int32_t &assetId, const std::string &targetPath,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (ThumbnailService::GetInstance() == nullptr) {
        return E_FAIL;
    }
    std::string displayName = "";
    GetStringValueFromResultSet(resultSet, MediaColumn::MEDIA_NAME, displayName);
    int64_t dateTaken = 0;
    GetLongValueFromResultSet(resultSet, MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    std::string uri = PHOTO_URI_PREFIX + to_string(assetId) + MediaFileUtils::GetExtraUri(displayName, targetPath) +
        "?api_version=10&date_taken=" + to_string(dateTaken);
    MEDIA_INFO_LOG("Begin generate thumbnail %{public}s, ", uri.c_str());
    int32_t err = ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, targetPath, false);
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
        "SELECT cover_uri FROM PhotoAlbum WHERE album_id = " + to_string(ownerAlbumId) +
        " AND cover_uri like 'file://media/Photo/" + to_string(oldAssetId) + "%'";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_ALBUM_COVER_INFO);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("No need to update cover_uri");
        return E_OK;
    }
    string newCoverUri = MediaLibraryFormMapOperations::GetUriByFileId(newAssetId, targetPath);
    MEDIA_INFO_LOG("New cover uri is %{public}s", targetPath.c_str());
    const std::string UPDATE_ALBUM_COVER_URI =
        "UPDATE PhotoAlbum SET cover_uri = '" + newCoverUri +"' WHERE album_id = " + to_string(ownerAlbumId);
    int32_t ret = upgradeStore->ExecuteSql(UPDATE_ALBUM_COVER_URI);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("update cover uri failed, ret = %{public}d, target album is %{public}d", ret, ownerAlbumId);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    const int32_t &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId,
    const std::string displayName)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int32_t assetId;
    GetIntValueFromResultSet(resultSet, MediaColumn::MEDIA_ID, assetId);
    MEDIA_INFO_LOG("begin copy local file, fileId is %{public}d, and target album is %{public}d",
        assetId, ownerAlbumId);
    std::string srcPath = "";
    std::string targetPath = "";
    GetSourceFilePath(srcPath, resultSet);
    buildTargetFilePath(srcPath, targetPath);
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
    MediaAssetCopyInfo copyInfo(targetPath, false, ownerAlbumId, displayName);
    NativeRdb::ValuesBucket values;
    err = BuildInsertValuesBucket(upgradeStore, values, resultSet, copyInfo);
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
    err = UpdateRelationship(upgradeStore, assetId, newAssetId, ownerAlbumId, true);
    if (err != E_OK) {
        MEDIA_ERR_LOG("UpdateRelationship fail, "
                      "assetId: %{public}d, newAssetId: %{public}lld, ownerAlbumId: %{public}d, ret = %{public}d",
            assetId, (long long)newAssetId, ownerAlbumId, err);
        return E_OK;
    }
    GenerateThumbnail(newAssetId, targetPath, resultSet);
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
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
    buildTargetFilePath(srcPath, targetPath);
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("Build target path fail, origin file is %{public}s", srcPath.c_str());
        return E_INVALID_PATH;
    }
    MEDIA_INFO_LOG("Begin copy thumbnail original scrPath is %{public}s, and target path is %{public}s",
        srcPath.c_str(), targetPath.c_str());
    int32_t err = CopyOriginThumbnail(srcPath, targetPath);
    if (err != E_OK) {
        return err;
    }
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
    err = UpdateRelationship(upgradeStore, assetId, newAssetId, ownerAlbumId, false);
    if (err != E_OK) {
        return err;
    }
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
}

void SendNewAssetNotify(string newFileAssetUri, const shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("Can not get MediaLibraryNotify, fail to send new asset notify.");
        return;
    }
    watch->Notify(newFileAssetUri, NotifyType::NOTIFY_ADD);

    vector<string> systemAlbumsExcludeSource = {
        to_string(PhotoAlbumSubType::FAVORITE),
        to_string(PhotoAlbumSubType::VIDEO),
        to_string(PhotoAlbumSubType::HIDDEN),
        to_string(PhotoAlbumSubType::TRASH),
        to_string(PhotoAlbumSubType::IMAGE),
        to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
    };
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, systemAlbumsExcludeSource);
    MediaLibraryRdbUtils::UpdateUserAlbumByUri(rdbStore, { newFileAssetUri });
}

void UpdateNewAssetAttr(const shared_ptr<MediaLibraryRdbStore> &rdbStore, RdbPredicates newPredicates)
{
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    values.Put(MediaColumn::MEDIA_DATE_TAKEN, MediaFileUtils::UTCTimeMilliSeconds());
    values.Put(MediaColumn::MEDIA_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    int32_t updateRow = -1;
    rdbStore->Update(updateRow, values, newPredicates);
    if (updateRow < 0) {
        MEDIA_ERR_LOG("Failed to update newAsset data, ret = %{public}d", updateRow);
    }
}

int32_t MediaLibraryAlbumFusionUtils::CloneSingleAsset(const int64_t &assetId, const string title)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
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

    string displayName = title + "." + suffix;
    int32_t ownerAlbumId;
    GetIntValueFromResultSet(resultSet, PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerAlbumId);
    int64_t newAssetId = -1;
    int32_t err = CopyLocalSingleFile(rdbStore, ownerAlbumId, resultSet, newAssetId, displayName);
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
    SendNewAssetNotify(newFileAssetUri, rdbStore);
    UpdateNewAssetAttr(rdbStore->GetRaw(), newPredicates);
    MEDIA_INFO_LOG("End clone asset, newAssetId = %{public}lld", (long long)newAssetId);
    return newAssetId;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNoOwnerData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    MEDIA_INFO_LOG("Begin handle no owner data");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
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
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Query not matched data fails");
        return E_DB_FAIL;
    }
    int32_t err = E_OK;
    if (isLocalAsset(resultSet)) {
        err = CopyLocalSingleFile(upgradeStore, ownerAlbumId, resultSet, newAssetId);
    } else {
        err = CopyCloudSingleFile(upgradeStore, assetId, ownerAlbumId, resultSet, newAssetId);
    }
    if (err != E_OK) {
        MEDIA_ERR_LOG("Copy file fails, is file local : %{public}d, fileId is %{public}d",
            isLocalAsset(resultSet), assetId);
        return err;
    }
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
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fatal error! Failed to query not matched map data");
            break;
        }
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
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, NativeRdb::ValuesBucket &values,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const string &newAlbumName)
{
    MEDIA_INFO_LOG("Begin build inset values Meta Data on set user album name!");
    if (newAlbumName == "") {
        return;
    }
    for (auto it = albumColumnTypeMap.begin(); it != albumColumnTypeMap.end(); ++it) {
        string columnName = it->first;
        ResultSetDataType columnType = it->second;
        ParsingAndFillValue(values, columnName, columnType, resultSet);
    }

    std::string lPath = "/Pictures/Users/" + newAlbumName;
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, newAlbumName);
    int64_t albumDataAdded = 0;
    GetLongValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_DATE_ADDED, albumDataAdded);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, albumDataAdded);
}

static int32_t BuildAlbumInsertValues(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    NativeRdb::ValuesBucket &values, const int32_t &oldAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    MEDIA_ERR_LOG("Begin build inset values Meta Data!");
    for (auto it = albumColumnTypeMap.begin(); it != albumColumnTypeMap.end(); ++it) {
        string columnName = it->first;
        ResultSetDataType columnType = it->second;
        ParsingAndFillValue(values, columnName, columnType, resultSet);
    }

    std::string lPath = "";
    std::string bundle_name = "";
    int32_t album_type = -1;
    std::string album_name = "";
    ValueObject valueObject;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, valueObject)) {
        valueObject.GetString(bundle_name);
        if (bundle_name == "com.huawei.ohos.screenshot") {
            bundle_name = "com.huawei.homs.screenshot";
            values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundle_name);
        }
        if (bundle_name == "com.huawei.ohos.screenrecorder") {
            bundle_name = "com.huawei.homs.screenrecorder";
            values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundle_name);
        }
    }
    if (values.GetObject(PhotoAlbumColumns::ALBUM_TYPE, valueObject)) {
        valueObject.GetInt(album_type);
    }
    if (values.GetObject(PhotoAlbumColumns::ALBUM_NAME, valueObject)) {
        valueObject.GetString(album_name);
    }
    if (album_type == OHOS::Media::PhotoAlbumType::SOURCE) {
        QuerySourceAlbumLPath(upgradeStore, lPath, bundle_name, album_name);
    } else {
        lPath = "/Pictures/Users/" + album_name;
        MEDIA_INFO_LOG("Album type is user type and lPath is %{public}s!!!", lPath.c_str());
    }
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, 1);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, MediaFileUtils::UTCTimeMilliSeconds());
    return E_OK;
}

static int64_t QueryExistsAlbumId(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    NativeRdb::ValuesBucket values, int64_t &newAlbumId)
{
    std::string lPath = "";
    ValueObject valueObject;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_LPATH, valueObject)) {
        valueObject.GetString(lPath);
    }
    std::string queryExistsAlbumId = "SELECT album_id from PhotoAlbum where lpath ='" + lPath + "' and dirty !='4'";
    shared_ptr<NativeRdb::ResultSet> existsAlbumResult = upgradeStore->QuerySql(queryExistsAlbumId);
    if (existsAlbumResult != nullptr && existsAlbumResult->GoToFirstRow() == NativeRdb::E_OK) {
        if (existsAlbumResult->GetLong(0, newAlbumId)!= NativeRdb::E_OK) {
            return E_HAS_DB_ERROR;
        }
        MEDIA_INFO_LOG("There is an album matches new scheme, no need to insert, "
            "albumId : %{public}" PRId64, newAlbumId);
    }
    return E_OK;
}

static int32_t CopyAlbumMetaData(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    std::shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &oldAlbumId, int64_t &newAlbumId)
{
    MEDIA_INFO_LOG("Begin copy album Meta Data!!!");
    if (upgradeStore == nullptr || resultSet == nullptr || oldAlbumId == -1) {
        MEDIA_ERR_LOG("invalid parameter");
        return E_INVALID_ARGUMENTS;
    }
    NativeRdb::ValuesBucket values;
    int32_t err = BuildAlbumInsertValues(upgradeStore, values, oldAlbumId, resultSet);
    int32_t ret = upgradeStore->Insert(newAlbumId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        if (ret == NativeRdb::E_SQLITE_CONSTRAINT) {
            QueryExistsAlbumId(upgradeStore, values, newAlbumId);
        } else {
            MEDIA_ERR_LOG("Insert copyed album failed, ret = %{public}d", ret);
            return E_HAS_DB_ERROR;
        }
    }
    MEDIA_ERR_LOG("Insert copyed album success,oldAlbumId is = %{public}d newAlbumId is %{public}" PRId64,
        oldAlbumId, newAlbumId);
    return ret;
}

int32_t MediaLibraryAlbumFusionUtils::DeleteALbumAndUpdateRelationship(
    const std::shared_ptr<MediaLibraryRdbStore> upgradeStore, const int32_t &oldAlbumId, const int64_t &newAlbumId,
    bool isCloudAblum)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    if (newAlbumId == -1) {
        MEDIA_ERR_LOG("Target album id error, origin albumId is %{public}d", oldAlbumId);
        return E_INVALID_ARGUMENTS;
    }
    std::string DELETE_EXPIRED_ALBUM = "";
    if (isCloudAblum) {
        DELETE_EXPIRED_ALBUM = "UPDATE PhotoAlbum SET dirty = '4' WHERE album_id = " + to_string(oldAlbumId);
    } else {
        DELETE_EXPIRED_ALBUM = "DELETE FROM PhotoAlbum WHERE album_id = " + to_string(oldAlbumId);
    }
    int32_t ret = upgradeStore->ExecuteSql(DELETE_EXPIRED_ALBUM);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("DELETE expired album failed, ret = %{public}d, albumId is %{public}d",
            ret, oldAlbumId);
        return E_HAS_DB_ERROR;
    }
    const std::string UPDATE_NEW_ALBUM_ID_IN_PHOTO_MAP = "UPDATE PhotoMap SET map_album = " +
        to_string(newAlbumId) + " WHERE dirty != '4' AND map_album = " + to_string(oldAlbumId);
    ret = upgradeStore->ExecuteSql(UPDATE_NEW_ALBUM_ID_IN_PHOTO_MAP);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update relationship in photo map fails, ret = %{public}d, albumId is %{public}d",
            ret, oldAlbumId);
        return E_HAS_DB_ERROR;
    }
    const std::string UPDATE_NEW_ALBUM_ID_IN_PHOTOS = "UPDATE Photos SET owner_album_id = " +
     to_string(newAlbumId) + " WHERE dirty != '4' AND owner_album_id = " + to_string(oldAlbumId);
    ret = upgradeStore->ExecuteSql(UPDATE_NEW_ALBUM_ID_IN_PHOTOS);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update relationship in photo map fails, ret = %{public}d, albumId is %{public}d",
            ret, oldAlbumId);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
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
        DeleteALbumAndUpdateRelationship(upgradeStore, oldAlbumId, newAlbumId, IsCloudAlbum(resultSet));
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
    MEDIA_INFO_LOG("End rebuild album table and compensate loss value");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const int32_t &sourceAlbumId, const int64_t &targetAlbumId)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("MergeClashSourceAlbum %{public}d, target album is %{public}" PRId64,
        sourceAlbumId, targetAlbumId);
    DeleteALbumAndUpdateRelationship(upgradeStore, sourceAlbumId, targetAlbumId, IsCloudAlbum(resultSet));
    return E_OK;
}

static int32_t MergeScreenShotAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("Begin handle expired screen shot album data ");
    int32_t oldAlbumId = -1;
    int64_t newAlbumId = -1;
    GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
    const std::string QUERY_NEW_SCREEN_SHOT_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE album_type = 2048 AND bundle_name = 'com.huawei.hmos.screenshot'"
        " AND lpath IS NULL AND dirty != 4";
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
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("Begin merge screenrecord album");
    int32_t oldAlbumId = -1;
    int64_t newAlbumId = -1;
    GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, oldAlbumId);
    const std::string QUERY_NEW_SCREEN_RECORD_ALBUM_INFO =
        "SELECT * FROM PhotoAlbum WHERE album_type = 2048 AND bundle_name = 'com.huawei.hmos.screenrecorder'"
        " AND lpath IS NULL AND dirty != 4";
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
        " AND lpath IS NULL AND dirty != 4";
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
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", UPDATE_COMPENSATE_ALBUM_DATA.c_str());
            return err;
        }
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

int32_t MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("Begin clean duplicated album");
    const std::string QUERY_DUPLICATE_ALBUM =
        "SELECT DISTINCT a1.* FROM PhotoAlbum a1 JOIN PhotoAlbum a2 ON a1.album_name = a2.album_name "
        "AND a1.cloud_id <> a2.cloud_id AND a1.priority = a2.priority AND "
        "(a1.priority is null OR a1.priority ='1') order by "
        "album_name asc, album_subtype desc, cloud_id desc, count desc";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_DUPLICATE_ALBUM);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "Query duplicate album fail");
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    MEDIA_INFO_LOG("Begin clean duplicated album, there are %{public}d to clean", rowCount);
    int32_t indexLeft = 0;
    while (indexLeft < rowCount) {
        resultSet->GoToRow(indexLeft);
        int32_t targetAlbumId = -1;
        std::string targetAlbumName = "";
        GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, targetAlbumId);
        GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_NAME, targetAlbumName);
        MEDIA_INFO_LOG("Clean duplicated album, targetAlbumId is: %{public}d ,target album name is %{public}s",
            targetAlbumId, targetAlbumName.c_str());
        int32_t indexRight = ++indexLeft;
        std::string sourceAlbumName = "";
        while (indexRight < rowCount) {
            resultSet->GoToRow(indexRight);
            int32_t sourceAlbumId = -1;
            GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, sourceAlbumId);
            GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_NAME, sourceAlbumName);
            MEDIA_INFO_LOG("Clean duplicated album, sourceAlbumId is %{public}d ,source album name is %{public}s",
                sourceAlbumId, sourceAlbumName.c_str());
            if (ToLower(targetAlbumName) == ToLower(sourceAlbumName)) {
                DeleteALbumAndUpdateRelationship(upgradeStore, sourceAlbumId, targetAlbumId, IsCloudAlbum(resultSet));
                indexRight++;
            } else {
                indexLeft = indexRight;
                break;
            }
        }
    }
    MEDIA_INFO_LOG("End clean duplicated album, cost: %{public}" PRId64,
        MediaFileUtils::UTCTimeMilliSeconds() - beginTime);
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
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("Query not matched data fails");
            return E_DB_FAIL;
        }
        HandleNewCloudDirtyDataImp(upgradeStore, resultSet, restOwnerAlbumIds, assetId);
    }
    return E_OK;
}

static int32_t TransferMisMatchScreenRecord(const std::shared_ptr<MediaLibraryRdbStore> upgradeStore)
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
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
                CREATE_SCREEN_RECORDS_ALBUM.c_str());
            return err;
        }
    }
    const std::string TRANSFER_MISS_MATCH_ASSET =
        "UPDATE Photos SET owner_album_id = "
        "(SELECT album_id FROM PhotoAlbum WHERE bundle_name ='com.huawei.hmos.screenrecorder' AND dirty <>4) "
        "WHERE owner_album_id = (SELECT album_id FROM PhotoAlbum WHERE "
        "bundle_name ='com.huawei.hmos.screenshot' AND dirty <>'4' limit 1) AND media_type =2";
    int32_t err = upgradeStore->ExecuteSql(TRANSFER_MISS_MATCH_ASSET);
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
                TRANSFER_MISS_MATCH_ASSET.c_str());
            return err;
    }
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

int32_t MediaLibraryAlbumFusionUtils::RefreshAllAlbums()
{
    MEDIA_INFO_LOG("Froce refresh all albums start");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("Can not get MediaLibraryNotify Instance");
        return E_ERR;
    }
    watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("Froce refresh all albums end");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CleanInvalidCloudAlbumAndData()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbstore, try again!");
        rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
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
    int32_t totalNumber = QueryTotalNumberNeedToHandle(rdbStore, QUERY_NEW_NOT_MATCHED_COUNT_IN_PHOTOMAP);
    MEDIA_INFO_LOG("QueryTotalNumberNeedToHandle, totalNumber=%{public}d", totalNumber);
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    for (int32_t offset = 0; offset < totalNumber; offset += ALBUM_FUSION_BATCH_COUNT) {
        MEDIA_INFO_LOG("DATA_CLEAN: handle batch clean, offset: %{public}d", offset);
        notMatchedMap.clear();
        int32_t err = QueryNoMatchedMap(rdbStore, notMatchedMap, false);
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Fatal error! Failed to query not matched map data");
            break;
        }
        if (notMatchedMap.size() != 0) {
            MEDIA_INFO_LOG("There are %{public}d items need to migrate", (int)notMatchedMap.size());
            HandleNewCloudDirtyData(rdbStore, notMatchedMap);
        }
    }
    HandleDuplicateAlbum(rdbStore);
    // Put no relationship asset into other album
    HandleNoOwnerData(rdbStore);
    // Clean duplicative album and rebuild expired album
    RebuildAlbumAndFillCloudValue(rdbStore);
    SetParameterToStartSync();
    RefreshAllAlbums();
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
} // namespace OHOS::Media