/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_download_dao.h"

#include <string>
#include <utime.h>
#include <vector>

#include "medialibrary_unistore_manager.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "cloud_media_operation_code.h"
#include "cloud_media_dao_utils.h"
#include "cloud_media_sync_utils.h"
#include "moving_photo_file_utils.h"
#include "accurate_common_data.h"
#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"
#include "metadata_extractor.h"
#include "exif_rotate_utils.h"
#include "media_string_utils.h"
#include "thumbnail_const.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"
#include "medialibrary_tracer.h"

namespace OHOS::Media::CloudSync {
NativeRdb::AbsRdbPredicates CloudMediaDownloadDao::GetDownloadThmsConditions(const int32_t type)
{
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL))
        ->And()
        ->BeginWrap();
    ThmLcdState state;
    if (type != static_cast<int32_t>(ThmLcdState::THM) && type != static_cast<int32_t>(ThmLcdState::THMLCD) &&
        type != static_cast<int32_t>(ThmLcdState::LCD)) {
        state = ThmLcdState::THMLCD;
    } else {
        state = static_cast<ThmLcdState>(type);
    }
    predicates.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::TO_DOWNLOAD));
    predicates.Or()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::THM_TO_DOWNLOAD));
    if (state == ThmLcdState::LCD || state == ThmLcdState::THMLCD) {
        predicates.Or()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::LCD_TO_DOWNLOAD));
    }
    predicates.EndWrap();
    return predicates;
}

int32_t CloudMediaDownloadDao::GetDownloadThmNum(const int32_t type, int32_t &totalNum)
{
    MEDIA_INFO_LOG("GetDownloadThmNum begin %{public}d", type);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Query Thumbs to Download Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = this->GetDownloadThmsConditions(type);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, {"COUNT(1) AS count"});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "resultSet is null");
    bool isValid = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_EXECUTE(isValid, resultSet->Close());
    CHECK_AND_RETURN_RET_LOG(isValid, E_RDB, "failed to get row");
    totalNum = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("QueryThumbsToDownload end %{public}d", totalNum);
    return E_OK;
}

int32_t CloudMediaDownloadDao::GetDownloadThms(const DownloadThumbnailQueryDto &queryDto, std::vector<PhotosPo> &photos)
{
    MEDIA_INFO_LOG("QueryThumbsToDownload begin %{public}s", queryDto.ToString().c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Query Thumbs to Download Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = this->GetDownloadThmsConditions(queryDto.type);
    if (queryDto.isDownloadDisplayFirst) {
        predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, 0);       // NOT_IN_TRASH
        predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);       // NOT_IN_PENDING
        predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, 0);             // NOT_HIDDEN
        predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);            // NOT_TEMP_FILE
        predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, 1);  // IS_BURST_COVER
    }
    predicates.Limit(queryDto.offset, queryDto.size);
    predicates.OrderByDesc(PhotoColumn::MEDIA_DATE_TAKEN);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, this->DOWNLOAD_THUMBNAIL_COLUMNS);
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Query Thumbs to Download Failed to read records, ret: %{public}d", ret);
    MEDIA_INFO_LOG("GetDownloadThms, photos size: %{public}zu", photos.size());
    return E_OK;
}

int32_t CloudMediaDownloadDao::GetDownloadAsset(const std::vector<int32_t> &fileIds, std::vector<PhotosPo> &photos)
{
    MEDIA_INFO_LOG("enter GetDownloadAsset");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "GetDownloadAsset Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(PhotoColumn::MEDIA_ID, CloudMediaDaoUtils::GetStringVector(fileIds))
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE))
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    auto resultSet = rdbStore->Query(queryPredicates, this->COLUMNS_DOWNLOAD_ASSET_QUERY_BY_FILE_ID);
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetDownloadAsset Failed to query, ret: %{public}d", ret);
    return ret;
}

int32_t CloudMediaDownloadDao::UpdateDownloadThm(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("enter UpdateDownloadThm");
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdateDownloadThm cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    CHECK_AND_PRINT_LOG(cloudIds.size() <= BATCH_LIMIT_SIZE,
        "UpdateDownloadThm cloudIds size: %{public}zu, limit: %{public}d",
        cloudIds.size(),
        BATCH_LIMIT_SIZE);
    std::string sql = "\
        UPDATE Photos \
            SET sync_status = 0, \
                thumb_status = thumb_status & ? \
        WHERE cloud_id IN ({0});";
    std::vector<std::string> params = {CloudMediaDaoUtils::ToStringWithCommaAndQuote(cloudIds)};
    std::string execSql = MediaStringUtils::FillParams(sql, params);
    std::vector<NativeRdb::ValueObject> bindArgs = {static_cast<int32_t>(~THM_TO_DOWNLOAD_MASK)};
    int32_t ret = rdbStore->ExecuteSql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to UpdateDownloadThm.");
    return ret;
}

int32_t CloudMediaDownloadDao::UpdateDownloadLcd(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("enter UpdateDownloadLcd");
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdateDownloadLcd cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    CHECK_AND_PRINT_LOG(cloudIds.size() <= BATCH_LIMIT_SIZE,
        "UpdateDownloadLcd cloudIds size: %{public}zu, limit: %{public}d",
        cloudIds.size(),
        BATCH_LIMIT_SIZE);
    std::string sql = "\
        UPDATE Photos \
            SET thumb_status = thumb_status & ? \
        WHERE cloud_id IN ({0});";
    std::vector<std::string> params = {CloudMediaDaoUtils::ToStringWithCommaAndQuote(cloudIds)};
    std::string execSql = MediaStringUtils::FillParams(sql, params);
    std::vector<NativeRdb::ValueObject> bindArgs = {static_cast<int32_t>(~LCD_TO_DOWNLOAD_MASK)};
    int32_t ret = rdbStore->ExecuteSql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to UpdateDownloadLcd.");
    return ret;
}

int32_t CloudMediaDownloadDao::UpdateDownloadThmAndLcd(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("enter UpdateDownloadThmAndLcd");
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "UpdateDownloadThmAndLcd cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::DOWNLOADED));
    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to UpdateDownloadThmAndLcd.");
    CHECK_AND_PRINT_LOG(changedRows > 0, "Check updateRows: %{public}d.", changedRows);
    MEDIA_INFO_LOG("UpdateDownloadThmAndLcd, changedRows: %{public}d.", changedRows);
    return ret;
}

int32_t CloudMediaDownloadDao::GetFileIdFromCloudId(
    const std::vector<std::string> &cloudIds, std::vector<std::string> &fileIds)
{
    MEDIA_INFO_LOG("enter GetFileIdFromCloudId, cloudIds size: %{public}zu", cloudIds.size());
    CHECK_AND_RETURN_RET_LOG(cloudIds.size() > 0, E_OK, "GetFileIdFromCloudId cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    std::vector<std::string> columns = {MediaColumn::MEDIA_ID};
    auto resultSet = rdbStore->Query(predicates, columns);
    std::vector<PhotosPo> photos;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetFileIdFromCloudId Failed to query, ret: %{public}d", ret);
    for (const auto &photo : photos) {
        CHECK_AND_CONTINUE(photo.fileId.has_value());
        fileIds.emplace_back(std::to_string(photo.fileId.value_or(0)));
    }
    return ret;
}

int32_t CloudMediaDownloadDao::QueryDownloadAssetByCloudIds(
    const std::vector<std::string> &cloudIds, std::vector<PhotosPo> &result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    predicates.NotEqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    auto resultSet = rdbStore->Query(predicates, {});
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(result);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to query, ret: %{public}d", ret);
    MEDIA_INFO_LOG("QueryDownloadAssetByCloudId, rowCount: %{public}d", static_cast<int32_t>(result.size()));
    return E_OK;
}

int32_t CloudMediaDownloadDao::UpdateDownloadAssetExifRotateFix(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh,
    const int32_t fileId, const int32_t exifRotate, const DirtyTypes dirtyType, bool needRegenerateThumbnail)
{
    CHECK_AND_RETURN_RET_LOG(dirtyType == DirtyTypes::TYPE_MDIRTY || dirtyType == DirtyTypes::TYPE_FDIRTY,
        E_ERR, "Not support update this dirtype:%{public}d", dirtyType);
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL,
        "UpdateDownloadAssetExifRotateFix Failed to get rdbStore.");

    std::vector<NativeRdb::ValueObject> bindArgs = {
        exifRotate, std::to_string(static_cast<int32_t>(dirtyType)), fileId,
    };
    int32_t ret = 0;
    if (needRegenerateThumbnail) {
        ret = photoRefresh->ExecuteSql(this->SQL_FIX_EXIF_ROTATE_WITH_REGENERATE_THUMBNAIL,
            bindArgs, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    } else {
        ret = photoRefresh->ExecuteSql(this->SQL_FIX_EXIF_ROTATE_WITHOUT_REGENERATE_THUMBNAIL,
            bindArgs, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    }

    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
        E_ERR,
        "UpdateDownloadAssetExifRotateFix Failed to Update, ret: %{public}d.",
        ret);
    return E_OK;
}

int32_t CloudMediaDownloadDao::UpdateTransCodeInfo(const std::string &path)
{
    MEDIA_INFO_LOG("enter UpdateTransCodeInfo");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, path);
    NativeRdb::ValuesBucket values;

    values.PutLong(PhotoColumn::PHOTO_TRANSCODE_TIME, 0);
    values.PutLong(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, 0);
    values.PutLong(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, 0);
    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to UpdateTransCodeInfo.");
    CHECK_AND_PRINT_LOG(changedRows > 0, "Check updateRows: %{public}d.", changedRows);
    MEDIA_INFO_LOG("UpdateTransCodeInfo success, changedRows: %{public}d.", changedRows);
    return ret;
}

void CloudMediaDownloadDao::FillHdrModeInfo(NativeRdb::ValuesBucket &values,
    const CloudMediaScanService::ScanResult &scanResult, const std::optional<PhotosPo> &localPhotosPoOp)
{
    bool isValid = scanResult.scanSuccess;
    CHECK_AND_RETURN(isValid);
    isValid = localPhotosPoOp.has_value();
    CHECK_AND_RETURN(isValid);
    bool needUpdate = localPhotosPoOp.value().hdrMode.value_or(0) != scanResult.hdrMode;
    CHECK_AND_PRINT_LOG(needUpdate, "no need update hdrMode");
    CHECK_AND_RETURN(needUpdate);
    values.PutInt(PhotoColumn::PHOTO_HDR_MODE, scanResult.hdrMode);
    values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
}

void CloudMediaDownloadDao::FillScanedSubtypeInfo(NativeRdb::ValuesBucket &values,
    const CloudMediaScanService::ScanResult &scanResult, bool isNeedUpdate)
{
    bool isValid = scanResult.scanSuccess;
    CHECK_AND_RETURN(isValid);
    CHECK_AND_RETURN(scanResult.subType == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS) ||
        scanResult.subType == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO));
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, scanResult.subType);
    CHECK_AND_RETURN(isNeedUpdate);
    values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
}

int32_t CloudMediaDownloadDao::UpdateDownloadAsset(const OnDownloadAssetData &assetData,
    const CloudMediaScanService::ScanResult& scanResult)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, assetData.path);
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    values.Put(PhotoColumn::LOCAL_ASSET_SIZE, scanResult.localAssetSize);
    if (assetData.fixFileType) {
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    }
    if (scanResult.scanSuccess) {
        values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, scanResult.shootingMode);
        values.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, scanResult.shootingModeTag);
        values.PutString(PhotoColumn::PHOTO_FRONT_CAMERA, scanResult.frontCamera);
    }
    
    this->FillScanedHeightWidth(values, scanResult);
    this->HandleAdditionalFileInfo(assetData, values);
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
        E_ERR,
        "UpdateDownloadAsset Failed to Update, ret: %{public}d.",
        ret);
    MEDIA_INFO_LOG("UpdateDownloadAsset, ret: %{public}d, changedRows: %{public}d, cloudId: %{public}s",
        ret, changedRows, assetData.cloudId.c_str());
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

void CloudMediaDownloadDao::FillScanedHeightWidth(
    NativeRdb::ValuesBucket &values, const CloudMediaScanService::ScanResult &scanResult)
{
    CHECK_AND_RETURN(scanResult.scanSuccess);
    bool isValid = scanResult.height != -1 && scanResult.width != -1 && scanResult.height != 0 && scanResult.width != 0;
    CHECK_AND_RETURN(isValid);
    values.Put(PhotoColumn::PHOTO_HEIGHT, scanResult.height);
    values.Put(PhotoColumn::PHOTO_WIDTH, scanResult.width);
    values.Put(PhotoColumn::PHOTO_ORIENTATION, scanResult.orientation);
    double aspectRatio = MediaFileUtils::CalculateAspectRatio(scanResult.height, scanResult.width);
    values.Put(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
}

int32_t CloudMediaDownloadDao::HandleAdditionalFileInfo(
    const OnDownloadAssetData &assetData, NativeRdb::ValuesBucket &values)
{
    if (assetData.lakeInfo.fileSourceType != 0) {
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, assetData.lakeInfo.fileSourceType);
    }
    if (!assetData.lakeInfo.storagePath.empty()) {
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, assetData.lakeInfo.storagePath);
    }
    if (!assetData.lakeInfo.title.empty()) {
        values.PutString(MediaColumn::MEDIA_TITLE, assetData.lakeInfo.title);
    }
    if (!assetData.lakeInfo.displayName.empty()) {
        values.PutString(MediaColumn::MEDIA_NAME, assetData.lakeInfo.displayName);
    }
    return E_OK;
}

int32_t CloudMediaDownloadDao::UpdateLcdFileSize(const std::vector<std::string> &cloudIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLcdFileSize");
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_ERR, "UpdateLcdFileSize cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    std::vector<LcdFileInfoDto> lcdFileDataList;
    int32_t ret = QueryLcdFileInfoByCloudIds(cloudIds, lcdFileDataList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "QueryLcdFileInfoByCloudIds failed, ret: %{public}d", ret);

    std::vector<std::pair<int32_t, int64_t>> fileIdAndSizeList;
    for (const auto &data : lcdFileDataList) {
        size_t lcdFileSize = ThumbnailUtils::CalcLcdFileSize(data.lcdExPath, data.lcdPath);
        if (lcdFileSize > 0) {
            fileIdAndSizeList.emplace_back(std::make_pair(data.fileId, static_cast<int64_t>(lcdFileSize)));
        }
    }

    CHECK_AND_RETURN_RET(!fileIdAndSizeList.empty(), E_OK);

    std::string updateSql = "UPDATE Photos SET lcd_file_size = CASE file_id ";
    std::string fileIdList = "";
    for (const auto &item : fileIdAndSizeList) {
        updateSql += " WHEN " + std::to_string(item.first) + " THEN " + std::to_string(item.second);
        fileIdList += std::to_string(item.first) + ",";
    }
    fileIdList.pop_back();
    updateSql += " ELSE lcd_file_size END WHERE file_id IN (" + fileIdList + ");";

    ret = rdbStore->ExecuteSql(updateSql);
    MEDIA_INFO_LOG("execute completed, ret: %{public}d, cloudId size: %{public}zu, fileIdAndSizeList size: %{public}zu",
        ret, cloudIds.size(), fileIdAndSizeList.size());
    return ret;
}

int32_t CloudMediaDownloadDao::QueryLcdFileInfoByCloudIds(
    const std::vector<std::string> &cloudIds, std::vector<LcdFileInfoDto> &result)
{
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_ERR, "cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    std::vector<std::string> queryColumns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};
    auto resultSet = rdbStore->Query(queryPredicates, queryColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "resultSet is null");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        LcdFileInfoDto data;
        data.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        data.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        data.lcdExPath = GetThumbnailPath(data.filePath, THUMBNAIL_LCD_EX_SUFFIX);
        data.lcdPath = GetThumbnailPath(data.filePath, THUMBNAIL_LCD_SUFFIX);
        result.emplace_back(data);
    }
    resultSet->Close();
    return E_OK;
}

int32_t CloudMediaDownloadDao::UpdateLcdFileSizeAndLcdSize(const std::vector<std::string> &cloudIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLcdFileSizeAndLcdSize");
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_ERR, "cloudIds is empty.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    std::vector<LcdFileInfoDto> lcdFileDataList;
    int32_t ret = QueryLcdFileInfoByCloudIds(cloudIds, lcdFileDataList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "QueryLcdFileInfoByCloudIds failed, ret: %{public}d", ret);

    for (const auto &data : lcdFileDataList) {
        NativeRdb::ValuesBucket values;
        size_t lcdFileSize = ThumbnailUtils::CalcLcdFileSize(data.lcdExPath, data.lcdPath);
        if (lcdFileSize > 0) {
            values.PutLong(PhotoColumn::PHOTO_LCD_FILE_SIZE, static_cast<int64_t>(lcdFileSize));
        }
        std::string lcdSizeStr;
        if (ThumbnailUtils::CalcLcdSize(data.lcdPath, lcdSizeStr)) {
            values.PutString(PhotoColumn::PHOTO_LCD_SIZE, lcdSizeStr);
        }
        if (values.IsEmpty()) {
            MEDIA_WARN_LOG("no data to update for fileId: %{public}d", data.fileId);
            continue;
        }
        NativeRdb::AbsRdbPredicates updatePredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        updatePredicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(data.fileId));
        int32_t changedRows = DEFAULT_VALUE;
        ret = rdbStore->Update(changedRows, values, updatePredicates);
        CHECK_AND_PRINT_LOG(ret == E_OK, "failed to update fileId: %{public}d, ret: %{public}d", data.fileId, ret);
    }
    return E_OK;
}
}  // namespace OHOS::Media::CloudSync