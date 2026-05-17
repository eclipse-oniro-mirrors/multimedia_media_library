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

#ifndef SCAN_CONFIG_BUILDER_H
#define SCAN_CONFIG_BUILDER_H

#include <memory>

#include "scan_config.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ScanConfigBuilder {
public:
    ScanConfigBuilder() = default;
    explicit ScanConfigBuilder(const ScanConfig& config);
    ~ScanConfigBuilder() = default;

    ScanConfigBuilder& SetExecutionMode(ScanExecutionMode executionMode);
    ScanConfigBuilder& SetFilePath(const std::string& path);
    ScanConfigBuilder& SetFileId(int32_t fileId);

    // 业务相关
    ScanConfigBuilder& SetIsMovingPhoto(bool isMoving = true);
    ScanConfigBuilder& SetForceScan(bool force = true);
    ScanConfigBuilder& SetSkipAlbumUpdate(bool skip = true);

    // 缩略图相关
    ScanConfigBuilder& SetNeedGenerateThumbnail(bool need = true);
    ScanConfigBuilder& SetCallback(const std::shared_ptr<IMediaScannerCallback>& callback);
    ScanConfigBuilder& SetCreateThumbSync(bool sync = true);
    ScanConfigBuilder& SetInvalidateThumb(bool invalidate = true);
    ScanConfigBuilder& SetOriginalPicture(const std::shared_ptr<Picture>& picture);

    // 扫描策略
    ScanConfigBuilder& SetStrategyType(ScanStrategyType type);

    // 并发解决策略
    ScanConfigBuilder& SetConflictPolicy(ConflictPolicy policy);
    ScanConfigBuilder& SetQuality(ScanQuality quality);

    // 通用业务的批量配置
    ScanConfigBuilder& UseCameraShotPreset(bool isMovingPhoto, ScanQuality quality = ScanQuality::DEFAULT);
    ScanConfigBuilder& UseThumbnailCallbackPreset(bool isCreateThumbSync, bool isInvalidateThumb,
        std::shared_ptr<Media::Picture> picture = nullptr,
        std::shared_ptr<IMediaScannerCallback> updateDirtyCallback = nullptr);

    ScanConfig Build();

private:
    ScanConfig config_;
};

} // namespace Media
} // namespace OHOS

#endif // SCAN_CONFIG_BUILDER_H