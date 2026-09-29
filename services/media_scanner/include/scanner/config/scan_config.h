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

#include "custom_restore_info.h"
#include "default_scan_info.h"
#include "imedia_scanner_callback.h"
#include "picture.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ScanConfigBuilder;

enum class ScanStrategyType {
    DEFAULT_SCAN = 0,
    CUSTOM_RESTORE_SCAN = 1
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

    // 公共变量 - 执行模式
    MediaLibraryApi GetApiVersion() const;
    ScanExecutionMode GetExecutionMode() const;

    // 公共变量 - 业务相关
    bool GetForceScan() const;
    bool GetSkipAlbumUpdate() const;

    // 公共变量 - 缩略图相关
    bool GetNeedGenerateThumbnail() const;
    const std::shared_ptr<IMediaScannerCallback>& GetCallback() const;
    bool GetCreateThumbSync() const;
    bool GetInvalidateThumb() const;
    const std::shared_ptr<Picture>& GetOriginalPicture() const;
    const std::shared_ptr<IMediaScannerCallback>& GetUpdateDirtyCallback() const;

    // 公共变量 - 扫描策略
    ScanStrategyType GetStrategyType() const;
    ConflictPolicy GetConflictPolicy() const;
    ScanQuality GetQuality() const;

    // Info 访问器
    DefaultScanInfo& GetDefaultScanInfo();
    const DefaultScanInfo& GetDefaultScanInfo() const;
    CustomRestoreInfo& GetCustomRestoreInfo();
    const CustomRestoreInfo& GetCustomRestoreInfo() const;

private:
    ScanConfig() = default;

    // 公共 setter（friend builder 可访问）
    void SetExecutionMode(ScanExecutionMode executionMode);
    void SetForceScan(bool force);
    void SetSkipAlbumUpdate(bool skip);
    void SetNeedGenerateThumbnail(bool need);
    void SetCallback(const std::shared_ptr<IMediaScannerCallback>& cb);
    void SetCreateThumbSync(bool sync);
    void SetInvalidateThumb(bool invalidate);
    void SetOriginalPicture(const std::shared_ptr<Picture>& picture);
    void SetUpdateDirtyCallback(const std::shared_ptr<IMediaScannerCallback>& cb);
    void SetStrategyType(ScanStrategyType type);
    void SetConflictPolicy(ConflictPolicy policy);
    void SetQuality(ScanQuality q);

    // 公共变量 - 执行模式
    ScanExecutionMode executionMode_ = ScanExecutionMode::ASYNC;

    // 公共变量 - 业务相关
    bool isForceScan_ = true;
    bool isSkipAlbumUpdate_ = false;

    // 公共变量 - 缩略图相关
    bool needGenerateThumbnail_ = true;
    std::shared_ptr<IMediaScannerCallback> callback_ = nullptr;
    bool isCreateThumbSync_ = false;
    bool isInvalidateThumb_ = true;
    std::shared_ptr<Picture> originalPicture_ = nullptr;
    std::shared_ptr<IMediaScannerCallback> updateDirtyCallback_ = nullptr;

    // 公共变量 - 扫描策略
    ScanStrategyType strategyType_ = ScanStrategyType::DEFAULT_SCAN;
    ConflictPolicy conflictPolicy_ = ConflictPolicy::DEFAULT;
    ScanQuality quality_ = ScanQuality::DEFAULT;

    // Info 成员（值类型，通过 strategyType_ 区分哪个生效）
    DefaultScanInfo defaultScanInfo_;
    CustomRestoreInfo customRestoreInfo_;
};

} // namespace Media
} // namespace OHOS

#endif // SCAN_CONFIG_H
