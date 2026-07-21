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
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "batch_restore_types.h"
#include "imedia_scanner_callback.h"
#include "picture.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ScanConfigBuilder;

struct SingleScanInfo {
    std::string filePath_;
    int32_t fileId_ = 0;
    bool isMovingPhoto_ = false;
};

struct BatchScanInfo {
    // Input: file paths to process
    std::vector<std::string> filePaths;
 
    std::vector<RestoreFileInfo> fileInfos;
    // Input: external time information (keyed by fileName)
    std::unordered_map<std::string, TimeInfo> timeInfoMap;
    // Input: target album id
    int32_t albumId = 0;
    // Input: whether to deduplicate
    bool isDeduplication = false;
    // Input: whether photo cache is available
    bool hasPhotoCache = false;
    // Input: dedup cache set (keyed by "fileName_size_mediaType_orientation")
    std::unordered_set<std::string> photoCache;
    // Input: source app identifiers
    std::string packageName;
    std::string bundleName;
    std::string appId;
    // Input: whether this is the first batch (affects retry behavior)
    bool isFirstBatch = true;
 
    std::vector<RestoreFileInfo> outFileInfos;
    // Output: number of duplicate files skipped
    int32_t outSameFileNum = 0;
    // Output: number of files successfully moved
    int32_t outSuccessFileNum = 0;
};

enum class ScanStrategyType {
    DEFAULT_SCAN = 0,
    BATCH_SCAN = 1
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
    const std::shared_ptr<Picture>& GetOriginalPicture() const;
    const std::shared_ptr<IMediaScannerCallback>& GetUpdateDirtyCallback() const;
 
    // 公共变量 - 扫描策略
    ScanStrategyType GetStrategyType() const;
 
    ConflictPolicy GetConflictPolicy() const;
    ScanQuality GetQuality() const;
 
    // 单文件扫描信息
    bool HasSingleScanInfo() const;
    const std::string& GetFilePath() const;
    int32_t GetFileId() const;
    bool GetIsMovingPhoto() const;
 
    // 多文件扫描信息
    bool HasBatchScanInfo() const;
    const std::shared_ptr<BatchScanInfo>& GetBatchScanInfo() const;
 
    // 多文件扫描信息 - 输入字段
    const std::vector<std::string>& GetFilePaths() const;
    const std::vector<RestoreFileInfo>& GetFileInfos() const;
    const std::unordered_map<std::string, TimeInfo>& GetTimeInfoMap() const;
    int32_t GetAlbumId() const;
    bool GetIsDeduplication() const;
    bool GetHasPhotoCache() const;
    const std::unordered_set<std::string>& GetPhotoCache() const;
    const std::string& GetPackageName() const;
    const std::string& GetBundleName() const;
    const std::string& GetAppId() const;
    bool GetIsFirstBatch() const;
 
    // 多文件扫描信息 - 输出字段
    const std::vector<RestoreFileInfo>& GetOutFileInfos() const;
    int32_t GetOutSameFileNum() const;
    int32_t GetOutSuccessFileNum() const;
 
private:
    ScanConfig() = default;
 
    // 公共变量 - 执行模式
    void SetExecutionMode(ScanExecutionMode executionMode);
 
    // 公共变量 - 业务相关
 
    void SetForceScan(bool force);
    void SetSkipAlbumUpdate(bool skip);
 
    // 公共变量 - 缩略图相关
    void SetNeedGenerateThumbnail(bool need);
    void SetCallback(const std::shared_ptr<IMediaScannerCallback>& cb);
    void SetCreateThumbSync(bool sync);
    void SetInvalidateThumb(bool invalidate);
    void SetOriginalPicture(const std::shared_ptr<Picture>& picture);
    void SetUpdateDirtyCallback(const std::shared_ptr<IMediaScannerCallback>& cb);
 
    // 公共变量 - 扫描策略
    void SetStrategyType(ScanStrategyType type);
 
    void SetConflictPolicy(ConflictPolicy policy);
    void SetQuality(ScanQuality q);
 
    // 单文件扫描信息
    void SetFilePath(const std::string& path);
    void SetFileId(int32_t id);
    void SetIsMovingPhoto(bool isMoving);
 
    // 多文件扫描信息
    void SetBatchScanInfo(const std::shared_ptr<BatchScanInfo>& info);
    void SetFilePaths(const std::vector<std::string>& paths);
    void SetFileInfos(const std::vector<RestoreFileInfo>& fileInfos);
    void SetTimeInfoMap(const std::unordered_map<std::string, TimeInfo>& timeInfoMap);
    void SetAlbumId(int32_t albumId);
    void SetIsDeduplication(bool isDeduplication);
    void SetHasPhotoCache(bool hasPhotoCache);
    void SetPhotoCache(const std::unordered_set<std::string>& photoCache);
    void SetPackageName(const std::string& packageName);
    void SetBundleName(const std::string& bundleName);
    void SetAppId(const std::string& appId);
    void SetIsFirstBatch(bool isFirstBatch);
 
    void SetOutFileInfos(const std::vector<RestoreFileInfo>& outFileInfos);
    void SetOutSameFileNum(int32_t outSameFileNum);
    void SetOutSuccessFileNum(int32_t outSuccessFileNum);
private:
    // 公共变量 - 执行模式
    ScanExecutionMode executionMode_ = ScanExecutionMode::ASYNC;
 
    // 公共变量 - 业务相关
    bool isForceScan_ = true;               // 默认: 强制扫描
    bool isSkipAlbumUpdate_ = false;        // 默认: 需要刷新相册
 
    // 公共变量 - 缩略图相关
    bool needGenerateThumbnail_ = true;     // 默认: 需要生成缩略图
    std::shared_ptr<IMediaScannerCallback> callback_ = nullptr;
    bool isCreateThumbSync_ = false;        // 默认: 异步生成缩略图
    bool isInvalidateThumb_ = true;         // 默认: 需要删除旧缩略图
    std::shared_ptr<Picture> originalPicture_ = nullptr;
    std::shared_ptr<IMediaScannerCallback> updateDirtyCallback_ = nullptr;
 
    // 公共变量 - 扫描策略
    ScanStrategyType strategyType_ = ScanStrategyType::DEFAULT_SCAN;
    ConflictPolicy conflictPolicy_ = ConflictPolicy::DEFAULT;
    ScanQuality quality_ = ScanQuality::DEFAULT;
 
    // 单文件扫描信息
    std::shared_ptr<SingleScanInfo> singleScanInfo_ = nullptr;
 
    // 多文件扫描信息
    std::shared_ptr<BatchScanInfo> batchScanInfo_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // SCAN_CONFIG_H