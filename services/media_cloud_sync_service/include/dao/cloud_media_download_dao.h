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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_DAO_H

#include <string>
#include <vector>

#include "on_download_asset_data_dto.h"
#include "asset_accurate_refresh.h"
#include "cloud_media_scan_service.h"
#include "rdb_store.h"
#include "photos_po.h"
#include "photos_dto.h"
#include "media_column.h"
#include "download_thumbnail_query_dto.h"
#include "cloud_lake_info.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class CloudMediaDownloadDao {
public:
    int32_t GetDownloadThmNum(const int32_t type, int32_t &totalNum);
    int32_t GetDownloadThms(const DownloadThumbnailQueryDto &queryDto, std::vector<PhotosPo> &photos);
    int32_t GetDownloadAsset(const std::vector<int32_t> &fileIds, std::vector<PhotosPo> &photos);
    int32_t UpdateDownloadThm(const std::vector<std::string> &cloudIds);
    int32_t UpdateDownloadLcd(const std::vector<std::string> &cloudIds);
    int32_t UpdateDownloadThmAndLcd(const std::vector<std::string> &cloudIds);
    int32_t GetFileIdFromCloudId(const std::vector<std::string> &cloudIds, std::vector<std::string> &fileIds);
    int32_t QueryDownloadAssetByCloudIds(const std::vector<std::string> &cloudIds, std::vector<PhotosPo> &result);
    int32_t UpdateDownloadAsset(const OnDownloadAssetData &assetData,
        const CloudMediaScanService::ScanResult& scanResult);
    int32_t UpdateDownloadAssetExifRotateFix(
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh,
        const int32_t fileId, const int32_t exifRotate, const DirtyTypes dirtyType, bool needRegenerateThumbnail);
    int32_t UpdateTransCodeInfo(const std::string &path);
    int32_t UpdateDownloadLakeAsset(const OnDownloadAssetData &assetData,
        const CloudMediaScanService::ScanResult& scanResult);
    int32_t QueryDownloadLakeAssetByCloudIds(
        const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos, std::vector<PhotosPo> &result);

private:
    NativeRdb::AbsRdbPredicates GetDownloadThmsConditions(const int32_t type);
    int32_t GetFileIdFromUri(const std::string &uri, int32_t &fileUniqueId);
    void FillHdrModeInfo(NativeRdb::ValuesBucket &values, const CloudMediaScanService::ScanResult &scanResult,
        bool isNeedUpdate);
    void FillScanedSubtypeInfo(NativeRdb::ValuesBucket &values, const CloudMediaScanService::ScanResult &scanResult,
        bool isNeedUpdate);

private:
    const std::vector<std::string> DOWNLOAD_THUMBNAIL_COLUMNS = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_SIZE,
        MediaColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_CLOUD_ID,
        PhotoColumn::PHOTO_THUMB_STATUS,
        PhotoColumn::PHOTO_ORIENTATION,
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        PhotoColumn::PHOTO_STORAGE_PATH,
        MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_DATE_TRASHED,
    };
    const std::vector<std::string> COLUMNS_DOWNLOAD_ASSET_QUERY_BY_FILE_ID = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::PHOTO_CLOUD_ID,
        PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIENTATION,
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_SUBTYPE,
        MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_DATE_TRASHED,
    };
    const uint32_t THM_TO_DOWNLOAD_MASK = 0x2;
    const uint32_t LCD_TO_DOWNLOAD_MASK = 0x1;

    const std::string SQL_FIX_EXIF_ROTATE_WITH_REGENERATE_THUMBNAIL = "\
        UPDATE Photos \
        SET \
            exif_rotate = ?, \
            thumbnail_ready = 2, \
            lcd_visit_time = 0, \
            meta_date_modified = strftime('%s000', 'now'), \
            dirty = CASE \
                WHEN dirty IN (0, 2, 6, 8) THEN ? \
                ELSE dirty \
            END \
        WHERE file_id = ? \
        ;";
    const std::string SQL_FIX_EXIF_ROTATE_WITHOUT_REGENERATE_THUMBNAIL = "\
        UPDATE Photos \
        SET \
            exif_rotate = ?, \
            meta_date_modified = strftime('%s000', 'now'), \
            dirty = CASE \
                WHEN dirty IN (0, 2, 6, 8) THEN ? \
                ELSE dirty \
            END \
        WHERE file_id = ? \
        ;";
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DOWNLOAD_DAO_H