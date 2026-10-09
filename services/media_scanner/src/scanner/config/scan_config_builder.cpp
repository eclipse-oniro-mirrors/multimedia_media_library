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

// 公共变量 - 执行模式

ScanConfigBuilder& ScanConfigBuilder::SetExecutionMode(ScanExecutionMode executionMode)
{
    config_.SetExecutionMode(executionMode);
    return *this;
}

// 公共变量 - 业务相关

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

// 公共变量 - 缩略图相关

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

ScanConfigBuilder& ScanConfigBuilder::SetUpdateDirtyCallback(
    const std::shared_ptr<IMediaScannerCallback>& updateDirtyCallback)
{
    config_.SetUpdateDirtyCallback(updateDirtyCallback);
    return *this;
}

// 公共变量 - 扫描策略

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

// Info 设置

ScanConfigBuilder& ScanConfigBuilder::SetDefaultScanInfo(const DefaultScanInfo& info)
{
    config_.GetDefaultScanInfo() = info;
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::SetCustomRestoreInfo(const CustomRestoreInfo& info)
{
    config_.GetCustomRestoreInfo() = info;
    return *this;
}

// 预设方法

ScanConfigBuilder& ScanConfigBuilder::UseCustomRestorePreset(const CustomRestoreInfo& info)
{
    config_.GetCustomRestoreInfo() = info;
    config_.SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN);
    return *this;
}

ScanConfigBuilder& ScanConfigBuilder::UseCameraShotPreset(ScanQuality quality)
{
    config_.SetStrategyType(ScanStrategyType::DEFAULT_SCAN);
    config_.SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY);
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
