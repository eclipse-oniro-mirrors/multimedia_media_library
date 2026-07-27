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

// ==================== ScanConfig ====================

bool ScanConfig::Validate(std::string& realPath) const
{
    const auto& info = GetDefaultScanInfo();
    if (info.GetFilePath().empty()) {
        MEDIA_ERR_LOG("ScanConfig::Validate: filePath is empty");
        return false;
    }

    if (!PathToRealPath(info.GetFilePath(), realPath)) {
        MEDIA_ERR_LOG("ScanConfig::Validate: failed to get real path %{public}s, errno %{public}d",
            MediaFileUtils::DesensitizePath(info.GetFilePath()).c_str(), errno);
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
    const auto& thisInfo = GetDefaultScanInfo();
    const auto& otherInfo = other.GetDefaultScanInfo();

    if (thisInfo.GetFileId() <= 0 || otherInfo.GetFileId() <= 0 ||
        thisInfo.GetFileId() != otherInfo.GetFileId()) {
        MEDIA_WARN_LOG("Merge: fileId invalid or mismatch (this=%{public}d, other=%{public}d)",
            thisInfo.GetFileId(), otherInfo.GetFileId());
    }

    ScanConfig merged;

    merged.defaultScanInfo_.SetFileId(thisInfo.GetFileId());
    merged.defaultScanInfo_.SetFilePath(
        !otherInfo.GetFilePath().empty() ? otherInfo.GetFilePath() : thisInfo.GetFilePath());
    merged.defaultScanInfo_.SetIsMovingPhoto(
        thisInfo.GetIsMovingPhoto() || otherInfo.GetIsMovingPhoto());

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
    const auto& info = GetDefaultScanInfo();
    std::stringstream ss;
    ss << "{"
       << "\"strategyType\": " << static_cast<int>(strategyType_) << ", "
       << "\"conflictPolicy\": " << static_cast<int>(conflictPolicy_) << ", "
       << "\"executionMode\": " << static_cast<int>(executionMode_) << ", "
       << "\"fileId\": " << info.GetFileId() << ", "
       << "\"isMovingPhoto\": " << (info.GetIsMovingPhoto() ? "true" : "false") << ", "
       << "\"isSkipAlbumUpdate\": " << (isSkipAlbumUpdate_ ? "true" : "false") << ", "
       << "\"needGenerateThumbnail\": " << (needGenerateThumbnail_ ? "true" : "false")
       << "}";
    return ss.str();
}

// 公共变量 - 执行模式

ScanExecutionMode ScanConfig::GetExecutionMode() const
{
    return executionMode_;
}

void ScanConfig::SetExecutionMode(ScanExecutionMode executionMode)
{
    executionMode_ = executionMode;
}

// 公共变量 - 业务相关

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

// 公共变量 - 缩略图相关

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

// 公共变量 - 扫描策略

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

// Info 访问器

DefaultScanInfo& ScanConfig::GetDefaultScanInfo()
{
    return defaultScanInfo_;
}

const DefaultScanInfo& ScanConfig::GetDefaultScanInfo() const
{
    return defaultScanInfo_;
}

CustomRestoreInfo& ScanConfig::GetCustomRestoreInfo()
{
    return customRestoreInfo_;
}

const CustomRestoreInfo& ScanConfig::GetCustomRestoreInfo() const
{
    return customRestoreInfo_;
}

} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
