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

namespace OHOS {
namespace Media {

bool ScanConfig::Validate(std::string& realPath) const
{
    if (filePath_.empty()) {
        MEDIA_ERR_LOG("ScanConfig::Validate: filePath is empty");
        return false;
    }

    if (!PathToRealPath(filePath_, realPath)) {
        MEDIA_ERR_LOG("ScanConfig::Validate: failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(filePath_).c_str(), errno);
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
    if (fileId_ <= 0 || other.GetFileId() <= 0 || fileId_ != other.GetFileId()) {
        MEDIA_WARN_LOG("Merge: fileId invalid or mismatch (this=%{public}d, other=%{public}d)",
            fileId_, other.GetFileId());
    }

    ScanConfig merged;

    merged.SetFileId(fileId_);
    merged.SetFilePath(!other.GetFilePath().empty() ? other.GetFilePath() : filePath_);

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
       << "\"fileId\": " << fileId_ << ", "
       << "\"isMovingPhoto\": " << (isMovingPhoto_ ? "true" : "false") << ", "
       << "\"isSkipAlbumUpdate\": " << (isSkipAlbumUpdate_ ? "true" : "false") << ", "
       << "\"needGenerateThumbnail\": " << (needGenerateThumbnail_ ? "true" : "false")
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

const std::string& ScanConfig::GetFilePath() const
{
    return filePath_;
}

void ScanConfig::SetFilePath(const std::string& path)
{
    filePath_ = path;
}

int32_t ScanConfig::GetFileId() const
{
    return fileId_;
}

void ScanConfig::SetFileId(int32_t id)
{
    fileId_ = id;
}

bool ScanConfig::GetIsMovingPhoto() const
{
    return isMovingPhoto_;
}

void ScanConfig::SetIsMovingPhoto(bool isMoving)
{
    isMovingPhoto_ = isMoving;
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