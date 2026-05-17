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

#define MLOG_TAG "ScanConfigBuilder"

#include "scan_config_builder.h"

#include "media_log.h"

namespace OHOS {
namespace Media {

ScanConfigBuilder::ScanConfigBuilder(const ScanConfig& config)
{
    config_ = config;
}

ScanConfigBuilder& ScanConfigBuilder::SetExecutionMode(ScanExecutionMode executionMode)
{
    config_.SetExecutionMode(executionMode);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetFilePath(const std::string& path)
{
    config_.SetFilePath(path);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetFileId(int32_t fileId)
{
    config_.SetFileId(fileId);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetIsMovingPhoto(bool isMoving)
{
    config_.SetIsMovingPhoto(isMoving);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetForceScan(bool force)
{
    config_.SetForceScan(force);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetSkipAlbumUpdate(bool skip)
{
    config_.SetSkipAlbumUpdate(skip);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetNeedGenerateThumbnail(bool need)
{
    config_.SetNeedGenerateThumbnail(need);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetCallback(
    const std::shared_ptr<IMediaScannerCallback>& callback)
{
    config_.SetCallback(callback);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetCreateThumbSync(bool sync)
{
    config_.SetCreateThumbSync(sync);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetInvalidateThumb(bool invalidate)
{
    config_.SetInvalidateThumb(invalidate);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetOriginalPicture(const std::shared_ptr<Picture>& picture)
{
    config_.SetOriginalPicture(picture);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetStrategyType(ScanStrategyType type)
{
    config_.SetStrategyType(type);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetConflictPolicy(ConflictPolicy policy)
{
    config_.SetConflictPolicy(policy);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetQuality(ScanQuality quality)
{
    config_.SetQuality(quality);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::UseCameraShotPreset(bool isMovingPhoto, ScanQuality quality)
{
    config_.SetStrategyType(ScanStrategyType::DEFAULT_SCAN);
    config_.SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY);
    config_.SetIsMovingPhoto(isMovingPhoto);
    config_.SetQuality(quality);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::UseThumbnailCallbackPreset(bool isCreateThumbSync, bool isInvalidateThumb,
    std::shared_ptr<Media::Picture> picture, std::shared_ptr<IMediaScannerCallback> updateDirtyCallback)
{
    config_.SetCreateThumbSync(isCreateThumbSync);
    config_.SetInvalidateThumb(isInvalidateThumb);
    config_.SetOriginalPicture(picture);
    config_.SetUpdateDirtyCallback(updateDirtyCallback);
    return *this;
}

ScanConfig ScanConfigBuilder::Build()
{
    MEDIA_DEBUG_LOG("ScanConfig built: %{public}s", config_.ToString().c_str());
    return config_;
}

} // namespace Media
} // namespace OHOS