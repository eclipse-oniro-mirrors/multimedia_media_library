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
 
    // 公共变量 - 扫描策略
    ScanConfigBuilder& SetStrategyType(ScanStrategyType type);
    ScanConfigBuilder& SetConflictPolicy(ConflictPolicy policy);
    ScanConfigBuilder& SetQuality(ScanQuality quality);
 
    // 单文件扫描信息
    ScanConfigBuilder& SetFilePath(const std::string& path);
    ScanConfigBuilder& SetFileId(int32_t fileId);
    ScanConfigBuilder& SetIsMovingPhoto(bool isMoving = true);
 
    // 多文件扫描信息
    ScanConfigBuilder& SetBatchScanInfo(const std::shared_ptr<BatchScanInfo>& multiScanInfo);
    ScanConfigBuilder& SetFilePaths(const std::vector<std::string>& paths);
    ScanConfigBuilder& SetFileInfos(const std::vector<RestoreFileInfo>& fileInfos);
    ScanConfigBuilder& SetTimeInfoMap(const std::unordered_map<std::string, TimeInfo>& timeInfoMap);
    ScanConfigBuilder& SetAlbumId(int32_t albumId);
    ScanConfigBuilder& SetIsDeduplication(bool isDeduplication);
    ScanConfigBuilder& SetHasPhotoCache(bool hasPhotoCache);
    ScanConfigBuilder& SetPhotoCache(const std::unordered_set<std::string>& photoCache);
    ScanConfigBuilder& SetPackageName(const std::string& packageName);
    ScanConfigBuilder& SetBundleName(const std::string& bundleName);
    ScanConfigBuilder& SetAppId(const std::string& appId);
    ScanConfigBuilder& SetIsFirstBatch(bool isFirstBatch);
 
    // 多文件扫描信息 - 全参数构建
    ScanConfigBuilder& UseCustomRestorePreset(
        const std::vector<std::string>& filePaths,
        const std::vector<RestoreFileInfo>& fileInfos,
        const std::unordered_map<std::string, TimeInfo>& timeInfoMap,
        int32_t albumId = 0,
        bool isDeduplication = false,
        bool hasPhotoCache = false,
        const std::unordered_set<std::string>& photoCache = {},
        const std::string& packageName = "",
        const std::string& bundleName = "",
        const std::string& appId = "",
        bool isFirstBatch = true);
 
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