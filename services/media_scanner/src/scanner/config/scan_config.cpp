/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "ScanConfig"

#include "scan_config.h"

#include <sstream>

#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scanner_utils.h"
// LCOV_EXCL_START
namespace OHOS {
namespace Media {

bool ScanConfig::Validate(std::string& realPath) const
{
    if (!HasSingleScanInfo() || GetFilePath().empty()) {
        MEDIA_ERR_LOG("ScanConfig::Validate: filePath is empty");
        return false;
    }

    if (!PathToRealPath(GetFilePath(), realPath)) {
        MEDIA_ERR_LOG("ScanConfig::Validate: failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(GetFilePath()).c_str(), errno);
        return false;
    }

    if (!ScannerUtils::IsRegularFile(realPath)) {
        MEDIA_ERR_LOG("ScanConfig::Validate: path %{public}s is not a regular file",
            MediaFileUtils::DesensitizePath(realPath).c_str());
        return false;
    }

    return true;
}

ScanConfig ScanConfig::Merge(const ScanConfig& other, ScanExecutionMode executionMode) const
{
    if (GetFileId() <= 0 || other.GetFileId() <= 0 || GetFileId() != other.GetFileId()) {
        MEDIA_WARN_LOG("Merge: fileId invalid or mismatch (this=%{public}d, other=%{public}d)",
            GetFileId(), other.GetFileId());
    }

    ScanConfig merged;

    merged.SetFileId(GetFileId());
    merged.SetFilePath(!other.GetFilePath().empty() ? other.GetFilePath() : GetFilePath());

    merged.SetIsMovingPhoto(GetIsMovingPhoto() || other.GetIsMovingPhoto());
    merged.SetForceScan(true);
    merged.SetSkipAlbumUpdate(false);
    
    if (GetStrategyType() == other.GetStrategyType()) {
        merged.SetStrategyType(GetStrategyType());
    } else {
        merged.SetStrategyType(ScanStrategyType::DEFAULT_SCAN);
    }
    
    if (GetConflictPolicy() == other.GetConflictPolicy()) {
        merged.SetConflictPolicy(GetConflictPolicy());
    } else {
        merged.SetConflictPolicy(ConflictPolicy::DEFAULT);
    }
    
    // callback 以同步的为准
    if (GetExecutionMode() == ScanExecutionMode::SYNC) {
        merged.SetCallback(callback_);
    } else if (other.GetExecutionMode() == ScanExecutionMode::SYNC) {
        merged.SetCallback(other.GetCallback());
    } else {
        merged.SetCallback(callback_ ? callback_ : other.GetCallback());
    }
    
    // 合并后置空 originalPhotoPicture，避免 picture 数据不准确导致缩略图生成异常
    auto mergedCallback = merged.GetCallback();
    if (mergedCallback) {
        mergedCallback->SetOriginalPhotoPicture(nullptr);
    }

    merged.SetExecutionMode(executionMode);

    return merged;
}

std::string ScanConfig::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"strategyType\": " << static_cast<int>(strategyType_) << ", "
       << "\"conflictPolicy\": " << static_cast<int>(conflictPolicy_) << ", "
       << "\"executionMode\": " << static_cast<int>(executionMode_) << ", "
       << "\"fileId\": " << GetFileId() << ", "
       << "\"isMovingPhoto\": " << (GetIsMovingPhoto() ? "true" : "false") << ", "
       << "\"isSkipAlbumUpdate\": " << (isSkipAlbumUpdate_ ? "true" : "false") << ", "
       << "\"needGenerateThumbnail\": " << (needGenerateThumbnail_ ? "true" : "false") << ", "
       << "\"hasSingleScanInfo\": " << (HasSingleScanInfo() ? "true" : "false") << ", "
       << "\"hasBatchScanInfo\": " << (HasBatchScanInfo() ? "true" : "false")
       << "}";
    return ss.str();
}

ScanExecutionMode ScanConfig::GetExecutionMode() const
{
    return executionMode_;
}

void ScanConfig::SetExecutionMode(ScanExecutionMode executionMode)
{
    executionMode_ = executionMode;
}

bool ScanConfig::HasSingleScanInfo() const
{
    return singleScanInfo_ != nullptr;
}

const std::string& ScanConfig::GetFilePath() const
{
    static const std::string emptyPath;
    return singleScanInfo_ ? singleScanInfo_->filePath_ : emptyPath;
}

void ScanConfig::SetFilePath(const std::string& path)
{
    if (!singleScanInfo_) {
        singleScanInfo_ = std::make_shared<SingleScanInfo>();
    }
    singleScanInfo_->filePath_ = path;
}

int32_t ScanConfig::GetFileId() const
{
    return singleScanInfo_ ? singleScanInfo_->fileId_ : 0;
}

void ScanConfig::SetFileId(int32_t id)
{
    if (!singleScanInfo_) {
        singleScanInfo_ = std::make_shared<SingleScanInfo>();
    }
    singleScanInfo_->fileId_ = id;
}

bool ScanConfig::GetIsMovingPhoto() const
{
    return singleScanInfo_ ? singleScanInfo_->isMovingPhoto_ : false;
}

void ScanConfig::SetIsMovingPhoto(bool isMoving)
{
    if (!singleScanInfo_) {
        singleScanInfo_ = std::make_shared<SingleScanInfo>();
    }
    singleScanInfo_->isMovingPhoto_ = isMoving;
}

bool ScanConfig::GetForceScan() const
{
    return isForceScan_;
}

void ScanConfig::SetForceScan(bool force)
{
    isForceScan_ = force;
}

bool ScanConfig::GetSkipAlbumUpdate() const
{
    return isSkipAlbumUpdate_;
}

void ScanConfig::SetSkipAlbumUpdate(bool skip)
{
    isSkipAlbumUpdate_ = skip;
}

bool ScanConfig::GetNeedGenerateThumbnail() const
{
    return needGenerateThumbnail_;
}

void ScanConfig::SetNeedGenerateThumbnail(bool need)
{
    needGenerateThumbnail_ = need;
}

const std::shared_ptr<IMediaScannerCallback>& ScanConfig::GetCallback() const
{
    return callback_;
}

void ScanConfig::SetCallback(const std::shared_ptr<IMediaScannerCallback>& cb)
{
    callback_ = cb;
}

bool ScanConfig::GetCreateThumbSync() const
{
    return isCreateThumbSync_;
}

void ScanConfig::SetCreateThumbSync(bool sync)
{
    isCreateThumbSync_ = sync;
}

bool ScanConfig::GetInvalidateThumb() const
{
    return isInvalidateThumb_;
}

void ScanConfig::SetInvalidateThumb(bool invalidate)
{
    isInvalidateThumb_ = invalidate;
}

const std::shared_ptr<Picture>& ScanConfig::GetOriginalPicture() const
{
    return originalPicture_;
}

void ScanConfig::SetOriginalPicture(const std::shared_ptr<Picture>& picture)
{
    originalPicture_ = picture;
}

const std::shared_ptr<IMediaScannerCallback>& ScanConfig::GetUpdateDirtyCallback() const
{
    return updateDirtyCallback_;
}

void ScanConfig::SetUpdateDirtyCallback(const std::shared_ptr<IMediaScannerCallback>& cb)
{
    updateDirtyCallback_ = cb;
}

ScanStrategyType ScanConfig::GetStrategyType() const
{
    return strategyType_;
}

void ScanConfig::SetStrategyType(ScanStrategyType type)
{
    strategyType_ = type;
}

bool ScanConfig::HasBatchScanInfo() const
{
    return batchScanInfo_ != nullptr;
}

const std::shared_ptr<BatchScanInfo>& ScanConfig::GetBatchScanInfo() const
{
    return batchScanInfo_;
}

void ScanConfig::SetBatchScanInfo(const std::shared_ptr<BatchScanInfo>& info)
{
    batchScanInfo_ = info;
}

// 多文件扫描信息 - 输入字段

const std::vector<std::string>& ScanConfig::GetFilePaths() const
{
    static const std::vector<std::string> empty;
    return batchScanInfo_ ? batchScanInfo_->filePaths : empty;
}

void ScanConfig::SetFilePaths(const std::vector<std::string>& paths)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->filePaths = paths;
}

const std::vector<RestoreFileInfo>& ScanConfig::GetFileInfos() const
{
    static const std::vector<RestoreFileInfo> empty;
    return batchScanInfo_ ? batchScanInfo_->fileInfos : empty;
}

void ScanConfig::SetFileInfos(const std::vector<RestoreFileInfo>& fileInfos)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->fileInfos = fileInfos;
}

const std::unordered_map<std::string, TimeInfo>& ScanConfig::GetTimeInfoMap() const
{
    static const std::unordered_map<std::string, TimeInfo> empty;
    return batchScanInfo_ ? batchScanInfo_->timeInfoMap : empty;
}

void ScanConfig::SetTimeInfoMap(const std::unordered_map<std::string, TimeInfo>& timeInfoMap)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->timeInfoMap = timeInfoMap;
}

int32_t ScanConfig::GetAlbumId() const
{
    return batchScanInfo_ ? batchScanInfo_->albumId : 0;
}

void ScanConfig::SetAlbumId(int32_t albumId)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->albumId = albumId;
}

bool ScanConfig::GetIsDeduplication() const
{
    return batchScanInfo_ ? batchScanInfo_->isDeduplication : false;
}

void ScanConfig::SetIsDeduplication(bool isDeduplication)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->isDeduplication = isDeduplication;
}

bool ScanConfig::GetHasPhotoCache() const
{
    return batchScanInfo_ ? batchScanInfo_->hasPhotoCache : false;
}

void ScanConfig::SetHasPhotoCache(bool hasPhotoCache)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->hasPhotoCache = hasPhotoCache;
}

const std::unordered_set<std::string>& ScanConfig::GetPhotoCache() const
{
    static const std::unordered_set<std::string> empty;
    return batchScanInfo_ ? batchScanInfo_->photoCache : empty;
}

void ScanConfig::SetPhotoCache(const std::unordered_set<std::string>& photoCache)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->photoCache = photoCache;
}

const std::string& ScanConfig::GetPackageName() const
{
    static const std::string empty;
    return batchScanInfo_ ? batchScanInfo_->packageName : empty;
}

void ScanConfig::SetPackageName(const std::string& packageName)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->packageName = packageName;
}

const std::string& ScanConfig::GetBundleName() const
{
    static const std::string empty;
    return batchScanInfo_ ? batchScanInfo_->bundleName : empty;
}

void ScanConfig::SetBundleName(const std::string& bundleName)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->bundleName = bundleName;
}

const std::string& ScanConfig::GetAppId() const
{
    static const std::string empty;
    return batchScanInfo_ ? batchScanInfo_->appId : empty;
}

void ScanConfig::SetAppId(const std::string& appId)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->appId = appId;
}

bool ScanConfig::GetIsFirstBatch() const
{
    return batchScanInfo_ ? batchScanInfo_->isFirstBatch : true;
}

void ScanConfig::SetIsFirstBatch(bool isFirstBatch)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->isFirstBatch = isFirstBatch;
}

// 多文件扫描信息 - 输出字段

const std::vector<RestoreFileInfo>& ScanConfig::GetOutFileInfos() const
{
    static const std::vector<RestoreFileInfo> empty;
    return batchScanInfo_ ? batchScanInfo_->outFileInfos : empty;
}

void ScanConfig::SetOutFileInfos(const std::vector<RestoreFileInfo>& outFileInfos)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->outFileInfos = outFileInfos;
}

int32_t ScanConfig::GetOutSameFileNum() const
{
    return batchScanInfo_ ? batchScanInfo_->outSameFileNum : 0;
}

void ScanConfig::SetOutSameFileNum(int32_t outSameFileNum)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->outSameFileNum = outSameFileNum;
}

int32_t ScanConfig::GetOutSuccessFileNum() const
{
    return batchScanInfo_ ? batchScanInfo_->outSuccessFileNum : 0;
}

void ScanConfig::SetOutSuccessFileNum(int32_t outSuccessFileNum)
{
    if (!batchScanInfo_) {
        batchScanInfo_ = std::make_shared<BatchScanInfo>();
    }
    batchScanInfo_->outSuccessFileNum = outSuccessFileNum;
}

ConflictPolicy ScanConfig::GetConflictPolicy() const
{
    return conflictPolicy_;
}

void ScanConfig::SetConflictPolicy(ConflictPolicy policy)
{
    conflictPolicy_ = policy;
}

ScanQuality ScanConfig::GetQuality() const
{
    return quality_;
}

void ScanConfig::SetQuality(ScanQuality q)
{
    quality_ = q;
}

MediaLibraryApi ScanConfig::GetApiVersion() const
{
    return MediaLibraryApi::API_10;
}

} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP