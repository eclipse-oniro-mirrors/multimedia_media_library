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
 
#define MLOG_TAG "BatchScannerObj"
 
#include "batch_scanner_obj.h"
 
#include "batch_restore_utils.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "scan_config.h"
 
// LCOV_EXCL_START
namespace OHOS::Media {
 
// Fields that can be directly written from Metadata via valueFuncMap_
static const std::vector<std::string> directFields = {
    PhotoColumn::PHOTO_ORIENTATION,
    PhotoColumn::PHOTO_EXIF_ROTATE,
    MediaColumn::MEDIA_MIME_TYPE,
    PhotoColumn::PHOTO_MEDIA_SUFFIX,
    MediaColumn::MEDIA_SIZE,
    PhotoColumn::LOCAL_ASSET_SIZE,
    MediaColumn::MEDIA_DURATION,
    PhotoColumn::PHOTO_HEIGHT,
    PhotoColumn::PHOTO_WIDTH,
    PhotoColumn::PHOTO_ASPECT_RATIO,
    PhotoColumn::PHOTO_LONGITUDE,
    PhotoColumn::PHOTO_LATITUDE,
    PhotoColumn::PHOTO_ALL_EXIF,
    PhotoColumn::PHOTO_SHOOTING_MODE,
    PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
    PhotoColumn::PHOTO_LAST_VISIT_TIME,
    PhotoColumn::PHOTO_FRONT_CAMERA,
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
    PhotoColumn::PHOTO_HDR_MODE,
    PhotoColumn::PHOTO_USER_COMMENT,
    PhotoColumn::PHOTO_VIDEO_MODE,
};
 
BatchScannerObj::BatchScannerObj(std::shared_ptr<BatchScanInfo> batchScanInfo)
    : batchScanInfo_(std::move(batchScanInfo))
{
}
 
int32_t BatchScannerObj::Execute()
{
    MEDIA_INFO_LOG("BatchScannerObj::Execute begin, file count: %{public}d",
        static_cast<int32_t>(batchScanInfo_->fileInfos.size()));
 
    // Step 1: Resolve metadata for all files
    int32_t err = ResolveMetadata();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "ResolveMetadata failed");
 
    // Step 2: Deduplicate with accurate orientation from EXIF
    err = Deduplicate();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Deduplicate failed");
 
    // Step 3: Convert metadata to ValuesBucket (skip duplicates)
    err = ConvertToValues();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "ConvertToValues failed");
 
    // Step 4: Batch insert to database
    err = Insert();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Insert failed");
 
    // Step 5: Post-process results
    PostProcess();
 
    return E_OK;
}
 
// Step 1: Data parsing - extract metadata from files
int32_t BatchScannerObj::ResolveMetadata()
{
    const auto &fileInfos = batchScanInfo_->fileInfos;
    const auto &timeInfoMap = batchScanInfo_->timeInfoMap;
    int32_t resolveCount = 0;
 
    items_.reserve(fileInfos.size());
    for (const auto &fileInfo : fileInfos) {
        BatchScanItem item;
        item.fileInfo = fileInfo;
        item.metadata = std::make_unique<Metadata>();
 
        int32_t err = BatchRestoreUtils::FillMetadata(timeInfoMap, fileInfo, item.metadata);
        if (err != E_OK) {
            MEDIA_ERR_LOG("ResolveMetadata: FillMetadata failed for %{public}s",
                fileInfo.fileName.c_str());
            item.metadata = nullptr;
            item.isDuplicate = true;
        } else {
            resolveCount++;
        }
        items_.push_back(std::move(item));
    }
    MEDIA_INFO_LOG("ResolveMetadata done, total: %{public}d, resolved: %{public}d",
        static_cast<int32_t>(items_.size()), resolveCount);
    return E_OK;
}
 
// Step 2: Deduplicate with accurate size and orientation from metadata
int32_t BatchScannerObj::Deduplicate()
{
    int32_t dupCount = 0;
 
    for (auto &item : items_) {
        if (item.isDuplicate) {
            dupCount++;
            continue;
        }
 
        // Back-fill fileInfo from metadata for accurate dedup
        if (item.metadata != nullptr) {
            item.fileInfo.size = item.metadata->GetFileSize();
            item.fileInfo.orientation = item.metadata->GetOrientation();
        }
 
        if (BatchRestoreUtils::IsDuplication(*batchScanInfo_, batchScanInfo_->photoCache, item.fileInfo)) {
            item.isDuplicate = true;
            dupCount++;
            // Update unique_id for duplicate
            std::string querySql =
                "UPDATE Photos SET unique_id = ? WHERE data = ? AND "
                "(unique_id IS NULL OR unique_id = '' OR unique_id = '-1') AND (media_type = ? OR media_type = ?)";
            std::vector<NativeRdb::ValueObject> params = {MediaFileUtils::GenerateUUID(),
                item.fileInfo.filePath, MediaType::MEDIA_TYPE_IMAGE, MediaType::MEDIA_TYPE_VIDEO};
            auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
            if (rdbStore != nullptr && rdbStore->ExecuteSql(querySql, params) != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("Update failed for path:%{public}s.", item.fileInfo.filePath.c_str());
            }
        }
    }
 
    MEDIA_INFO_LOG("Deduplicate done, total: %{public}d, duplicate: %{public}d",
        static_cast<int32_t>(items_.size()), dupCount);
    return E_OK;
}
 
// Step 3: Data conversion - build ValuesBucket from metadata
int32_t BatchScannerObj::ConvertToValues()
{
    for (auto &item : items_) {
        if (item.isDuplicate) {
            continue;
        }
        if (item.metadata == nullptr) {
            item.isDuplicate = true;
            continue;
        }
 
        // Compute subtype for ValuesBucket
        int32_t subtype = item.fileInfo.isLivePhoto
            ? static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)
            : static_cast<int32_t>(PhotoSubType::DEFAULT);
        if (item.metadata->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS)) {
            subtype = static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS);
        }
 
        // Fields from fileInfo
        item.values.Put(MediaColumn::MEDIA_FILE_PATH, item.fileInfo.filePath);
        item.values.Put(MediaColumn::MEDIA_TITLE, item.fileInfo.title);
        item.values.Put(MediaColumn::MEDIA_NAME, item.fileInfo.displayName);
        item.values.Put(MediaColumn::MEDIA_PACKAGE_NAME, batchScanInfo_->packageName);
        item.values.Put(MediaColumn::MEDIA_OWNER_PACKAGE, batchScanInfo_->bundleName);
        item.values.Put(MediaColumn::MEDIA_OWNER_APPID, batchScanInfo_->appId);
 
        // Fields with special logic
        if (item.fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE ||
            item.fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            item.values.Put(PhotoColumn::UNIQUE_ID, MediaFileUtils::GenerateUUID());
        }
        BatchRestoreUtils::SetTimeInfo(item.metadata, item.fileInfo, item.values);
        item.values.Put(PhotoColumn::PHOTO_SUBTYPE, subtype);
        item.values.Put(MediaColumn::MEDIA_TYPE, item.fileInfo.mediaType);
        item.values.Put(MediaColumn::MEDIA_TIME_PENDING, -1);
        item.values.Put(PhotoColumn::PHOTO_QUALITY, 0);
 
        // Direct fields from Metadata via valueFuncMap_
        for (const auto &field : directFields) {
            item.values.Put(field, item.metadata->GetValue(field));
        }
    }
    MEDIA_INFO_LOG("ConvertToValues done");
    return E_OK;
}
 
// Step 4: Batch insert to database
int32_t BatchScannerObj::Insert()
{
    std::vector<NativeRdb::ValuesBucket> insertValues;
    std::vector<size_t> insertIndices;
 
    for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].isDuplicate) {
            continue;
        }
        insertValues.push_back(items_[i].values);
        insertIndices.push_back(i);
    }
 
    if (insertValues.empty()) {
        MEDIA_INFO_LOG("Insert: no values to insert");
        return E_OK;
    }
 
    int64_t rowNum = 0;
    int32_t errCode = E_ERR;
    TransactionOperations trans{__func__};
    std::function<int(void)> func = [&]() -> int {
        errCode = trans.BatchInsert(rowNum, PhotoColumn::PHOTOS_TABLE, insertValues);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "BatchInsert failed, errCode: %{public}d, rowNum: %{public}" PRId64, errCode, rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, !batchScanInfo_->isFirstBatch);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Insert: RetryTrans fail, ret:%{public}d", errCode);
        // Mark all insert items as failed
        for (auto idx : insertIndices) {
            items_[idx].isDuplicate = true;
        }
        return errCode;
    }
 
    MEDIA_INFO_LOG("Insert success rowNum: %{public}" PRId64, rowNum);
    return E_OK;
}
 
// Step 5: Post-process - back-fill fileInfo and write results back to BatchScanInfo
void BatchScannerObj::PostProcess()
{
    std::vector<RestoreFileInfo> outFileInfos;
    int32_t sameFileNum = 0;
    int32_t successFileNum = 0;
 
    for (auto &item : items_) {
        if (item.isDuplicate) {
            sameFileNum++;
            continue;
        }
        // Back-fill fileInfo from metadata for caller
        if (item.metadata != nullptr) {
            item.fileInfo.mimeType = item.metadata->GetFileMimeType();
            item.fileInfo.shootingMode = item.metadata->GetShootingMode();
            item.fileInfo.frontCamera = item.metadata->GetFrontCamera();
            item.fileInfo.movingPhotoEffectMode = 0;
            item.fileInfo.subtype = item.fileInfo.isLivePhoto
                ? static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)
                : static_cast<int32_t>(PhotoSubType::DEFAULT);
            if (item.metadata->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS)) {
                item.fileInfo.subtype = static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS);
            }
        }
        outFileInfos.push_back(item.fileInfo);
        successFileNum++;
    }
 
    batchScanInfo_->outFileInfos = std::move(outFileInfos);
    batchScanInfo_->outSameFileNum = sameFileNum;
    batchScanInfo_->outSuccessFileNum = successFileNum;
 
    MEDIA_INFO_LOG("PostProcess done, success: %{public}d, duplicate: %{public}d",
        successFileNum, sameFileNum);
}
 
} // namespace OHOS::Media
// LCOV_EXCL_STOP