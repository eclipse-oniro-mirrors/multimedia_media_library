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

    // 公共变量 - 执行模式
    ScanConfigBuilder& SetExecutionMode(ScanExecutionMode executionMode);

    // 公共变量 - 业务相关
    ScanConfigBuilder& SetForceScan(bool force = true);
    ScanConfigBuilder& SetSkipAlbumUpdate(bool skip = true);

    // 公共变量 - 缩略图相关
    ScanConfigBuilder& SetNeedGenerateThumbnail(bool need = true);
    ScanConfigBuilder& SetCallback(const std::shared_ptr<IMediaScannerCallback>& callback);
    ScanConfigBuilder& SetCreateThumbSync(bool sync = true);
    ScanConfigBuilder& SetInvalidateThumb(bool invalidate = true);
    ScanConfigBuilder& SetOriginalPicture(const std::shared_ptr<Picture>& picture);
    ScanConfigBuilder& SetUpdateDirtyCallback(
        const std::shared_ptr<IMediaScannerCallback>& updateDirtyCallback);

    // 公共变量 - 扫描策略
    ScanConfigBuilder& SetStrategyType(ScanStrategyType type);
    ScanConfigBuilder& SetConflictPolicy(ConflictPolicy policy);
    ScanConfigBuilder& SetQuality(ScanQuality quality);

    // Info 设置（整体传入，不再逐字段配置）
    ScanConfigBuilder& SetDefaultScanInfo(const DefaultScanInfo& info);
    ScanConfigBuilder& SetCustomRestoreInfo(const CustomRestoreInfo& info);

    // 预设方法
    ScanConfigBuilder& UseCustomRestorePreset(const CustomRestoreInfo& info);
    ScanConfigBuilder& UseCameraShotPreset(ScanQuality quality = ScanQuality::DEFAULT);

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