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

#include "medialibrary_type_const.h"
#include "medialibrary_formmap_operations.h"
#include "metadata.h"
#include "media_file_utils.h"
#include "medialibrary_album_compatibility_fusion_sql.h"
#include "parameters.h"
#include "thumbnail_service.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

constexpr int32_t POSITION_LOCAL_FLAG = 1;
constexpr int32_t POSITION_CLOUD_FLAG = 2;
constexpr int32_t POSITION_BOTH_FLAG = 3;
constexpr int32_t CLOUD_COPY_DIRTY_FLAG = 7;

constexpr int32_t TIME_STAMP_OFFSET = 5;
const std::string ALBUM_FUSION_FLAG = "multimedia.medialibrary.cloneFlag";

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
    {PhotoColumn::PHOTO_QUALITY, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_FIRST_VISIT_TIME, ResultSetDataType::TYPE_INT64},
    {PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, ResultSetDataType::TYPE_INT32},
    {PhotoColumn::PHOTO_FRONT_CAMERA, ResultSetDataType::TYPE_STRING}
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

int32_t MediaLibraryAlbumFusionUtils::HandleMatchedDataFusion(NativeRdb::RdbStore *upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_1: Start handle matched relationship");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t err = upgradeStore->ExecuteSql(UPDATE_ALBUM_ASSET_MAPPING_CONSISTENCY_DATA_SQL);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            UPDATE_ALBUM_ASSET_MAPPING_CONSISTENCY_DATA_SQL.c_str());
        return err;
    }
    err = upgradeStore->ExecuteSql(DELETE_MATCHED_RELATIONSHIP_IN_PHOTOMAP_SQL);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            DELETE_MATCHED_RELATIONSHIP_IN_PHOTOMAP_SQL.c_str());
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

int32_t MediaLibraryAlbumFusionUtils::QueryNoMatchedMap(NativeRdb::RdbStore *upgradeStore,
    std::multimap<int32_t, vector<int32_t>> &notMathedMap, bool isUpgrade)
{
    if (upgradeStore == nullptr) {
        MEDIA_ERR_LOG("invalid rdbstore or nullptr map");
        return E_INVALID_ARGUMENTS;
    }
    std::string queryNotMatchedDataSql = "";
    if (isUpgrade) {
        queryNotMatchedDataSql = QUERY_NOT_MATCHED_DATA_IN_PHOTOMAP;
    } else {
        queryNotMatchedDataSql = QUERY_NEW_NOT_MATCHED_DATA_IN_PHOTOMAP;
    }
    auto resultSet = upgradeStore->QuerySql(queryNotMatchedDataSql);
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
    MEDIA_INFO_LOG("There are %{public}d assets need to handle", notMatchedCount);
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

int32_t MediaLibraryAlbumFusionUtils::HandleFirstData(NativeRdb::RdbStore *upgradeStore,
    const int32_t &assetId, const int32_t &ownerAlbumId)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    MEDIA_INFO_LOG("begin handle first data, assetId is %{public}d target album id is %{public}d",
        assetId, ownerAlbumId);
    const std::string UPDATE_FIRST_NOT_MATCHED_ALBUM_ID_SQL =
    "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = '" +
        to_string(ownerAlbumId) + "' WHERE " + PhotoColumn::MEDIA_ID + " = '" + to_string(assetId) + "'";
    int32_t err = upgradeStore->ExecuteSql(UPDATE_FIRST_NOT_MATCHED_ALBUM_ID_SQL);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s",
            UPDATE_ALBUM_ASSET_MAPPING_CONSISTENCY_DATA_SQL.c_str());
        return err;
    }
    const std::string DROP_HANDLED_MAP_RELATIONSHIP =
    "UPDATE PhotoMap SET dirty = '4' WHERE " + PhotoMap::ASSET_ID + " = '" + to_string(assetId) +
        "' AND " + PhotoMap::ALBUM_ID + " = '" + to_string(ownerAlbumId) + "'";
    err = upgradeStore->ExecuteSql(DROP_HANDLED_MAP_RELATIONSHIP);
    return err;
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

static int32_t CopyOriginPhoto(const std::string &srcPath, std::string &targetPath)
{
    if (srcPath.empty() || !MediaFileUtils::IsFileExists((srcPath)) || !MediaFileUtils::IsFileValid(srcPath)) {
        MEDIA_ERR_LOG("source file invalid!");
        return E_INVALID_PATH;
    }
    if (!MediaFileUtils::CopyFileUtil(srcPath, targetPath) || !MediaFileUtils::IsFileExists((targetPath))) {
        MEDIA_ERR_LOG("CopyFile failed, filePath: %{public}s, errmsg: %{public}s", srcPath.c_str(),
            strerror(errno));
        return E_RENAME_FILE_FAIL;
    }
    return E_OK;
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
            double doubleComlunValue;
            GetDoubleValueFromResultSet(resultSet, columnName, doubleComlunValue);
            values.PutDouble(columnName, doubleComlunValue);
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

static int32_t BuildInsertValuesBucket(NativeRdb::ValuesBucket &values, const std::string &targetPath,
    shared_ptr<NativeRdb::ResultSet> &resultSet, bool isCopyThumbnail)
{
    values.PutString(MediaColumn::MEDIA_FILE_PATH, targetPath);
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
    return E_OK;
}

static int32_t copyMetaData(NativeRdb::RdbStore *upgradeStore, int64_t &newAssetId,
    NativeRdb::ValuesBucket &values)
{
    int32_t ret = upgradeStore->Insert(newAssetId, PhotoColumn::PHOTOS_TABLE, values);
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

static int32_t UpdateRelationship(NativeRdb::RdbStore *upgradeStore, const int32_t &assetId,
    const int32_t &newAssetId, const int32_t &ownerAlbumId, bool isLocalAsset)
{
    const std::string UPDATE_ALBUM_ID_FOR_COPY_ASSET =
        "UPDATE Photos SET owner_album_id = " + to_string(ownerAlbumId) + " WHERE file_id = " + to_string(newAssetId);
    int32_t ret = upgradeStore->ExecuteSql(UPDATE_ALBUM_ID_FOR_COPY_ASSET);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UPDATE_ALBUM_ID_FOR_COPY_ASSET failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    const std::string DROP_HANDLED_MAP_RELATIONSHIP =
    "UPDATE PhotoMap SET dirty = '4' WHERE " + PhotoMap::ASSET_ID + " = '" + to_string(assetId) +
        "' AND " + PhotoMap::ALBUM_ID + " = '" + to_string(ownerAlbumId) + "'";
    ret = upgradeStore->ExecuteSql(DROP_HANDLED_MAP_RELATIONSHIP);
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

static int32_t GenerateThumbnail(const int32_t &assetId, const std::string &targetPath, const std::string &displayName)
{
    if (ThumbnailService::GetInstance() == nullptr) {
        return E_FAIL;
    }
    std::string uri = "file://media/photo/" + to_string(assetId) + MediaFileUtils::GetExtraUri(displayName, targetPath);
    MEDIA_INFO_LOG("Begin generate thumbnail %{public}s, ", uri.c_str());
    int32_t err = ThumbnailService::GetInstance()->CreateThumbnailFileScaned(uri, targetPath, true);
    if (err != E_SUCCESS) {
        MEDIA_ERR_LOG("ThumbnailService CreateThumbnailFileScaned failed : %{public}d", err);
    }
    MEDIA_INFO_LOG("Generate thumbnail %{public}s, success ", uri.c_str());
    return err;
}

static int32_t UpdateCoverInfoForAlbum(NativeRdb::RdbStore *upgradeStore, const int32_t &oldAssetId,
    const int32_t &ownerAlbumId, int64_t &newAssetId, const std::string &targetPath)
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

int32_t MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(NativeRdb::RdbStore *upgradeStore, const int32_t &assetId,
    const int32_t &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
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
    int32_t err = CopyOriginPhoto(srcPath, targetPath);
    if (err != E_OK) {
        return err;
    }
    NativeRdb::ValuesBucket values;
    err = BuildInsertValuesBucket(values, targetPath, resultSet, false);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Insert meta data fail and delete migrated file %{public}s ", targetPath.c_str());
        DeleteFile(targetPath);
        return err;
    }
    err = copyMetaData(upgradeStore, newAssetId, values);
    if (err != E_OK) {
        // If insert fails, delete the moved file to avoid wasted space
        DeleteFile(targetPath);
        return err;
    }
    err = UpdateRelationship(upgradeStore, assetId, newAssetId, ownerAlbumId, true);
    if (err != E_OK) {
        return E_OK;
    }
    std::string displayName = "";
    ValueObject valueObject;
    values.GetObject(MediaColumn::MEDIA_NAME, valueObject);
    valueObject.GetString(displayName);
    GenerateThumbnail(newAssetId, targetPath, displayName);
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::CopyCloudSingleFile(NativeRdb::RdbStore *upgradeStore, const int32_t &assetId,
    const int32_t &ownerAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet, int64_t &newAssetId)
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
    NativeRdb::ValuesBucket values;
    err = BuildInsertValuesBucket(values, targetPath, resultSet, true);
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
    err = UpdateRelationship(upgradeStore, assetId, newAssetId, ownerAlbumId, false);
    if (err != E_OK) {
        return err;
    }
    UpdateCoverInfoForAlbum(upgradeStore, assetId, ownerAlbumId, newAssetId, targetPath);
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNoOwnerData(NativeRdb::RdbStore *upgradeStore)
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

int32_t MediaLibraryAlbumFusionUtils::HandleRestData(NativeRdb::RdbStore *upgradeStore,
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
        for (size_t i = 1; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = CopyLocalSingleFile(upgradeStore, assetId, restOwnerAlbumIds[i], resultSet, newAssetId);
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
        for (size_t i = 1; i < restOwnerAlbumIds.size(); i++) {
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

int32_t MediaLibraryAlbumFusionUtils::HandleNotMatchedDataMigration(NativeRdb::RdbStore *upgradeStore,
    std::multimap<int32_t, vector<int32_t>> &notMathedMap)
{
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    static int handledCount = 0;
    for (auto it = notMathedMap.begin(); it != notMathedMap.end(); ++it) {
        HandleFirstData(upgradeStore, it->first, it->second[0]);
        handledCount++;
        HandleRestData(upgradeStore, it->first, it->second, handledCount);
    }
    MEDIA_INFO_LOG("handled %{public}d items", handledCount);
    // Put no relationship asset into other album
    HandleNoOwnerData(upgradeStore);
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleSingleFileCopy(NativeRdb::RdbStore *upgradeStore,
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
        err = CopyLocalSingleFile(upgradeStore, assetId, ownerAlbumId, resultSet, newAssetId);
    } else {
        err = CopyCloudSingleFile(upgradeStore, assetId, ownerAlbumId, resultSet, newAssetId);
    }
    if (err != E_OK) {
        MEDIA_ERR_LOG("Copy file fails, is file local : %{public}d, fileId is %{public}d",
            isLocalAsset(resultSet), assetId);
        return err;
    }
    MEDIA_INFO_LOG("Copy file success, fileId %{public}d, albumId %{public}d, and copyed file id %{public}" PRId64,
        assetId, ownerAlbumId, newAssetId);
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleNotMatchedDataFusion(NativeRdb::RdbStore *upgradeStore)
{
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_2: Start handle not matched relationship");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    int32_t err = QueryNoMatchedMap(upgradeStore, notMatchedMap, true);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to query not matched map data");
        return err;
    }
    if (notMatchedMap.size() != 0) {
        HandleNotMatchedDataMigration(upgradeStore, notMatchedMap);
    }
    MEDIA_INFO_LOG("ALBUM_FUSE: STEP_2: End handle not matched relationship, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return E_OK;
}

static void QuerySourceAlbumLPath(NativeRdb::RdbStore *upgradeStore,
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

static int32_t BuildAlbumInsertValues(NativeRdb::RdbStore *upgradeStore, NativeRdb::ValuesBucket &values,
    const int32_t &oldAlbumId, shared_ptr<NativeRdb::ResultSet> &resultSet)
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

static int32_t CopyAlbumMetaData(NativeRdb::RdbStore *upgradeStore,
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
        MEDIA_ERR_LOG("Insert copyed album failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    MEDIA_ERR_LOG("Insert copyed album success,oldAlbumId is = %{public}d newAlbumId is %{public}" PRId64,
        oldAlbumId, newAlbumId);
    return ret;
}

static int32_t DeleteALbumAndUpdateRelationship(NativeRdb::RdbStore *upgradeStore,
    const int32_t &oldAlbumId, const int64_t &newAlbumId, bool isCloudAblum)
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

static inline bool IsCloudAlbum(shared_ptr<NativeRdb::ResultSet> resultSet)
{
    string cloudId = "";
    GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_CLOUD_ID, cloudId);
    return !cloudId.empty();
}

int32_t MediaLibraryAlbumFusionUtils::HandleExpiredAlbumData(NativeRdb::RdbStore *upgradeStore)
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

static int32_t KeepHiddenAlbumAssetSynced(NativeRdb::RdbStore *upgradeStore)
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

static int32_t RemediateErrorSourceAlbumSubType(NativeRdb::RdbStore *upgradeStore)
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

int32_t MediaLibraryAlbumFusionUtils::RebuildAlbumAndFillCloudValue(NativeRdb::RdbStore *upgradeStore)
{
    MEDIA_INFO_LOG("Start rebuild album table and compensate loss value");
    if (upgradeStore == nullptr) {
        MEDIA_INFO_LOG("fail to get rdbstore");
        return E_DB_FAIL;
    }
    int32_t err = HandleChangeNameAlbum(upgradeStore);
    HandleExpiredAlbumData(upgradeStore);
    // Keep dual hidden assets dirty state synced, let cloudsync handle compensating for hidden flags
    KeepHiddenAlbumAssetSynced(upgradeStore);
    RemediateErrorSourceAlbumSubType(upgradeStore);
    MEDIA_INFO_LOG("End rebuild album table and compensate loss value");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(NativeRdb::RdbStore *upgradeStore,
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

static int32_t MergeScreenShotAlbum(NativeRdb::RdbStore *upgradeStore, shared_ptr<NativeRdb::ResultSet> &resultSet)
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
    shared_ptr<NativeRdb::ResultSet> newAlbunResultSet = upgradeStore->QuerySql(QUERY_NEW_SCREEN_SHOT_ALBUM_INFO);
    MEDIA_INFO_LOG("Begin merge screenshot album, old album is %{public}d", oldAlbumId);
    if (newAlbunResultSet == nullptr || newAlbunResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        // Create a new bundle name screenshot album
        CopyAlbumMetaData(upgradeStore, resultSet, oldAlbumId, newAlbumId);
        MEDIA_INFO_LOG("Create new screenshot album, album id is %{public}" PRId64, newAlbumId);
    } else {
        GetLongValueFromResultSet(newAlbunResultSet, PhotoAlbumColumns::ALBUM_ID, newAlbumId);
    }
    MEDIA_INFO_LOG("Begin merge screenshot album, new album is %{public}" PRId64, newAlbumId);
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, oldAlbumId, newAlbumId);
    MEDIA_INFO_LOG("End handle expired screen shot album data ");
    return E_OK;
}

static int32_t MergeScreenRecordAlbum(NativeRdb::RdbStore *upgradeStore, shared_ptr<NativeRdb::ResultSet> &resultSet)
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
    shared_ptr<NativeRdb::ResultSet> newAlbunResultSet = upgradeStore->QuerySql(QUERY_NEW_SCREEN_RECORD_ALBUM_INFO);
    if (newAlbunResultSet == nullptr || newAlbunResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        // Create a new bundle name screenshot album
        CopyAlbumMetaData(upgradeStore, resultSet, oldAlbumId, newAlbumId);
        MEDIA_INFO_LOG("Create new screenrecord album, album id is %{public}" PRId64, newAlbumId);
    } else {
        GetLongValueFromResultSet(newAlbunResultSet, PhotoAlbumColumns::ALBUM_ID, newAlbumId);
    }
    MediaLibraryAlbumFusionUtils::MergeClashSourceAlbum(upgradeStore, resultSet, oldAlbumId, newAlbumId);
    MEDIA_INFO_LOG("End merge screenrecord album");
    return E_OK;
}

int32_t MediaLibraryAlbumFusionUtils::HandleChangeNameAlbum(NativeRdb::RdbStore *upgradeStore)
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

int32_t MediaLibraryAlbumFusionUtils::HandleDuplicateAlbum(NativeRdb::RdbStore *upgradeStore)
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
        "(a1.priority is null OR a1.priority ='1') AND a1.album_type = '2048' "
        "order by album_name asc,  album_subtype desc, cloud_id desc, count desc";
    shared_ptr<NativeRdb::ResultSet> resultSet = upgradeStore->QuerySql(QUERY_DUPLICATE_ALBUM);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query duplicate album fail");
        return E_DB_FAIL;
    }
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
        int32_t indexRight = ++indexLeft;
        std::string sourceAlbumName = "";
        while (indexRight < rowCount) {
            resultSet->GoToRow(indexRight);
            int32_t sourceAlbumId = -1;
            GetIntValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_ID, sourceAlbumId);
            GetStringValueFromResultSet(resultSet, PhotoAlbumColumns::ALBUM_NAME, sourceAlbumName);
            if (targetAlbumName == sourceAlbumName) {
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

static void HandleNewCloudDirtyDataImp(NativeRdb::RdbStore *upgradeStore,
    shared_ptr<NativeRdb::ResultSet> &resultSet, std::vector<int32_t> &restOwnerAlbumIds, int32_t &assetId)
{
    int64_t newAssetId = -1;
    if (isLocalAsset(resultSet)) {
        MEDIA_INFO_LOG("File is local asset %{public}d", assetId);
        // skip first one, already handled
        for (size_t i = 0; i < restOwnerAlbumIds.size(); i++) {
            int32_t err = MediaLibraryAlbumFusionUtils::CopyLocalSingleFile(upgradeStore,
                assetId, restOwnerAlbumIds[i], resultSet, newAssetId);
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

int32_t MediaLibraryAlbumFusionUtils::HandleNewCloudDirtyData(NativeRdb::RdbStore *upgradeStore,
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
    int64_t beginTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("DATA_CLEAN:Clean invalid cloud album and dirty data start!");
    SetParameterToStopSync();
    auto upgradeStorePtr = rdbStore->GetRaw();
    NativeRdb::RdbStore *upgradeStore = upgradeStorePtr.get();
    std::multimap<int32_t, vector<int32_t>> notMatchedMap;
    int32_t err = QueryNoMatchedMap(upgradeStore, notMatchedMap, false);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to query not matched map data");
        return err;
    }
    if (notMatchedMap.size() != 0) {
        HandleNewCloudDirtyData(upgradeStore, notMatchedMap);
    }
    HandleDuplicateAlbum(upgradeStore);
    // Put no relationship asset into other album
    HandleNoOwnerData(upgradeStore);
    // Clean duplicative album and rebuild expired album
    RebuildAlbumAndFillCloudValue(upgradeStore);
    SetParameterToStartSync();
    MEDIA_INFO_LOG("DATA_CLEAN:Clean invalid cloud album and dirty data, cost %{public}ld",
        (long)(MediaFileUtils::UTCTimeMilliSeconds() - beginTime));
    return err;
}
} // namespace OHOS::Media