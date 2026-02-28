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

#include "dfx_reporter.h"
#include "file_parser.h"
#include "folder_parser.h"
#include "folder_scanner_utils.h"
#include "folder_scanner.h"
#include "media_lake_notify_info.h"

namespace OHOS::Media {

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
    void Run(const std::string &path, bool isFirstScanner = true);
    bool IsGlobalScanning(const std::vector<MediaLakeNotifyInfo> &notifyInfos, const ScanTaskType &type);
    void InterruptScanner();
    ScannerStatus GetScannerStatus();

private:
    GlobalScanner() = default;
    ~GlobalScanner() = default;
    GlobalScanner(const GlobalScanner&) = delete;
    const GlobalScanner& operator=(const GlobalScanner&) = delete;

    int32_t WalkFileTree(const std::string &path);
    int32_t ProcessIncrementScanTask(bool isGlobalScanEnd = false);
    void CheckToDeleteAssets(FolderScanner &folderScanner);
    void ResetReportData();
    bool IsForceScanning();
    void InitTemperatureCondition();

private:
    std::mutex scanMutex_;
    std::queue<std::pair<MediaLakeNotifyInfo, ScanTaskType>> scanTaskQueue_;
    ScannerStatus scannerStatus_{ScannerStatus::IDLE};
    std::atomic<bool> isNotInterruptScanner_{true};
    AncoCheckInfo reportData_{0, 0, 0, 0, 0};
    std::atomic<bool> isHighTemperature_{false};
};
} // namespace OHOS::Media
#endif // GLOBAL_SCANNER_H