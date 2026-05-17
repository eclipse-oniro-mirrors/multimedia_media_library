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

#ifndef SCAN_CONFIG_H
#define SCAN_CONFIG_H

#include <memory>
#include <string>

#include "imedia_scanner_callback.h"
#include "picture.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ScanConfigBuilder;

enum class ScanStrategyType {
    DEFAULT_SCAN = 0
};

enum class ScanQuality {
    DEFAULT = -1,
    LOW = 0,
    FULL = 1
};

enum class ConflictPolicy {
    DEFAULT = 0,
    QUALITY_PRIORITY = 1
};

enum class ScanExecutionMode {
    ASYNC = 0,
    SYNC = 1
};

class EXPORT ScanConfig {
public:
    friend class ScanConfigBuilder;

    ScanConfig(const ScanConfig&) = default;
    ScanConfig& operator=(const ScanConfig&) = default;
    ~ScanConfig() = default;

    bool Validate(std::string& realPath) const;
    ScanConfig Merge(const ScanConfig& other, ScanExecutionMode executionMode) const;
    std::string ToString() const;

    MediaLibraryApi GetApiVersion() const;
    ScanExecutionMode GetExecutionMode() const;
    const std::string& GetFilePath() const;
    int32_t GetFileId() const;

    // 业务相关
    bool GetIsMovingPhoto() const;
    bool GetForceScan() const;
    bool GetSkipAlbumUpdate() const;

    // 缩略图相关
    bool GetNeedGenerateThumbnail() const;
    const std::shared_ptr<IMediaScannerCallback>& GetCallback() const;
    bool GetCreateThumbSync() const;
    bool GetInvalidateThumb() const;
    const std::shared_ptr<Picture>& GetOriginalPicture() const;
    const std::shared_ptr<IMediaScannerCallback>& GetUpdateDirtyCallback() const;

    // 扫描策略
    ScanStrategyType GetStrategyType() const;

    // 并发解决策略
    ConflictPolicy GetConflictPolicy() const;
    ScanQuality GetQuality() const;

private:
    ScanConfig() = default;

    void SetExecutionMode(ScanExecutionMode executionMode);
    void SetFilePath(const std::string& path);
    void SetFileId(int32_t id);

    // 业务相关
    void SetIsMovingPhoto(bool isMoving);
    void SetForceScan(bool force);
    void SetSkipAlbumUpdate(bool skip);

    // 缩略图相关
    void SetNeedGenerateThumbnail(bool need);
    void SetCallback(const std::shared_ptr<IMediaScannerCallback>& cb);
    void SetCreateThumbSync(bool sync);
    void SetInvalidateThumb(bool invalidate);
    void SetOriginalPicture(const std::shared_ptr<Picture>& picture);
    void SetUpdateDirtyCallback(const std::shared_ptr<IMediaScannerCallback>& cb);

    // 扫描策略
    void SetStrategyType(ScanStrategyType type);

    // 并发解决策略
    void SetConflictPolicy(ConflictPolicy policy);
    void SetQuality(ScanQuality q);

private:
    ScanExecutionMode executionMode_ = ScanExecutionMode::ASYNC;
    std::string filePath_;
    int32_t fileId_ = 0;

    // 业务相关
    bool isMovingPhoto_ = false;            // 默认: 非动态照片
    bool isForceScan_ = true;               // 默认: 强制扫描
    bool isSkipAlbumUpdate_ = false;        // 默认: 需要刷新相册

    // 缩略图相关
    bool needGenerateThumbnail_ = true;     // 默认: 需要生成缩略图
    std::shared_ptr<IMediaScannerCallback> callback_ = nullptr;
    bool isCreateThumbSync_ = false;        // 默认: 异步生成缩略图
    bool isInvalidateThumb_ = true;         // 默认: 需要删除旧缩略图
    std::shared_ptr<Picture> originalPicture_ = nullptr;
    std::shared_ptr<IMediaScannerCallback> updateDirtyCallback_ = nullptr;        // 待日落

    // 扫描策略
    ScanStrategyType strategyType_ = ScanStrategyType::DEFAULT_SCAN;
    
    // 并发解决策略
    ConflictPolicy conflictPolicy_ = ConflictPolicy::DEFAULT;
    ScanQuality quality_ = ScanQuality::DEFAULT;
};

} // namespace Media
} // namespace OHOS

#endif // SCAN_CONFIG_H