/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef GLOBAL_SCANNER_H
#define GLOBAL_SCANNER_H

#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "media_file_notify_info.h"

namespace OHOS::Media {
class IScanPolicy;
class CheckDfxCollector;
class FolderScanner;

enum ScanTaskType: int32_t {
    File = 0,
    Folder,
};

enum class ScannerStatus {
    IDLE,
    GLOBAL_SCAN,
    CHECK_SCAN
};

class GlobalScanner {
public:
    static GlobalScanner& GetInstance();
    void RunLakeScan(const std::string &path, bool isFirstScanner = true);
    void RunLakeScan(const std::string &path, CheckDfxCollector &dfxCollector, bool isFirstScanner = true);
    void RunFileManagerScan(const std::string &path, CheckDfxCollector &dfxCollector, bool isFirstScanner = true);
    bool IsGlobalScanning(const std::vector<MediaNotifyInfo> &notifyInfos, const ScanTaskType &type);
    void InterruptScanner();
    ScannerStatus GetScannerStatus();
    void UpdateTemperatureCondition(bool isHighTemperature);
    void SetCloneScanInfo(bool isLakeCloneRestoring, int64_t cloneEndTime);
    bool IsLakeCloneRestoring();

private:
    GlobalScanner() = default;
    ~GlobalScanner() = default;
    GlobalScanner(const GlobalScanner&) = delete;
    const GlobalScanner& operator=(const GlobalScanner&) = delete;

    void Run(const std::string &path, IScanPolicy &policy, CheckDfxCollector &dfxCollector, bool isFirstScanner = true);
    int32_t WalkFileTree(const std::string &path, IScanPolicy &policy, CheckDfxCollector &dfxCollector);
    int32_t ProcessIncrementScanTask(bool isGlobalScanEnd, IScanPolicy &policy);
    int32_t CheckToDeleteAssets(FolderScanner &folderScanner);
    bool IsForceScanning();
    void CheckScanTemperature();
    void InitTemperatureCondition();
    bool IsCloneRestoringFile(const std::string &filePath);

private:
    std::mutex scanMutex_;
    std::queue<std::pair<MediaNotifyInfo, ScanTaskType>> scanTaskQueue_;
    ScannerStatus scannerStatus_{ScannerStatus::IDLE};
    std::atomic<bool> isNotInterruptScanner_{true};
    std::atomic<bool> isHighTemperature_{false};
    std::atomic<int32_t> deleteCountForCloneRestore_{0};
    std::atomic<bool> isLakeCloneRestoring_{false};
    std::atomic<int64_t> cloneEndTime_{0};
};
} // namespace OHOS::Media
#endif // GLOBAL_SCANNER_H