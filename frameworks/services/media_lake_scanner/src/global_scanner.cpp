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
#define MLOG_TAG "GlobalScanner"

#include <filesystem>
#include <iostream>

#include "dfx_anco_manager.h"
#include "file_scanner.h"
#include "global_scanner.h"
#include "lake_file_utils.h"
#include "media_file_utils.h"
#include "media_lake_check.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS::Media {
namespace fs = std::filesystem;

GlobalScanner& GlobalScanner::GetInstance()
{
    static GlobalScanner instance;
    return instance;
}

void GlobalScanner::Run(const std::string &path, bool isFirstScanner)
{
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        if (scannerStatus_ != ScannerStatus::IDLE) {
            MEDIA_WARN_LOG("global scan is running: %{public}d", static_cast<int32_t>(scannerStatus_));
            return;
        }

        if (isFirstScanner) {
            scannerStatus_ = ScannerStatus::GLOBAL_SCAN;
        } else {
            scannerStatus_ = ScannerStatus::CHECK_SCAN;
            if (reportData_.checkStartTime == 0) {
                reportData_.checkStartTime = static_cast<uint64_t>(MediaFileUtils::UTCTimeMilliSeconds());
            }
        }
        isNotInterruptScanner_ = true;
    }
    MEDIA_INFO_LOG("global scan start, isFirstScanner: %{public}d, scannerStatus: %{public}d", isFirstScanner,
        static_cast<int32_t>(scannerStatus_));
    int32_t ret = WalkFileTree(path);
    ProcessIncrementScanTask(true);
    CHECK_AND_RETURN_LOG(ret == ERR_SUCCESS, "An exception occurred while walking the file tree");
    if (isFirstScanner) {
        MediaInLakeSetCheckFinish();
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Get rdb store failed. rdbStorePtr is null");

    MEDIA_INFO_LOG("global scan end, isFirstScanner: %{public}d", isFirstScanner);
}

void GlobalScanner::ResetReportData()
{
    reportData_ = {0, 0, 0, 0, 0};
}

int32_t GlobalScanner::WalkFileTree(const std::string &path)
{
    std::error_code errorCode;
    bool isDirectory = fs::exists(path, errorCode) && fs::is_directory(path, errorCode);
    CHECK_AND_RETURN_RET_LOG(isDirectory, ERR_INCORRECT_PATH, "invalid directory[%{public}s], error[%{public}s]",
        LakeFileUtils::GarbleFilePath(path).c_str(), errorCode.message().c_str());

    queue<std::string> dirQueue;
    dirQueue.push(path);

    while (!dirQueue.empty() && isNotInterruptScanner_) {
        ProcessIncrementScanTask();

        std::string currentDir = dirQueue.front();
        dirQueue.pop();

        // 长度过长
        size_t len = currentDir.length();
        CHECK_AND_CONTINUE_ERR_LOG(len > 0 && len < FILENAME_MAX - 1, "dir[%{public}s] error.",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());

        bool shouldScan = FolderScannerUtils::shouldScanDirectory(currentDir);
        CHECK_AND_CONTINUE_ERR_LOG(shouldScan, "Not scan dir: %{public}s",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());
        bool isSkipDirectory = FolderScannerUtils::IsSkipCurrentDirectory(currentDir);
        MEDIA_INFO_LOG("call IsSkipCurrentDirectory");
        CHECK_AND_CONTINUE_ERR_LOG(!isSkipDirectory, "Skip dir path: %{public}s",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());

        FolderScanner folderScanner(currentDir, LakeScanMode::FULL);
        int32_t ret = folderScanner.ScanCurrentDirectory(dirQueue);
        CHECK_AND_CONTINUE_ERR_LOG(ret == ERR_SUCCESS, "Scan dir[%{public}s] failed",
            LakeFileUtils::GarbleFilePath(currentDir).c_str());

        reportData_.checkAdd += folderScanner.GetAddCount();
        reportData_.checkUpdate += folderScanner.GetUpdateCount();
        CheckToDeleteAssets(folderScanner);
    }

    if (isNotInterruptScanner_ || dirQueue.empty()) {
        int32_t deleteNum = 0;
        if (CheckAndIfNeedDeletePhotoAlbum(deleteNum,
            [this]()->bool { return scannerStatus_ == ScannerStatus::CHECK_SCAN && !isNotInterruptScanner_;})) {
            reportData_.checkDelete += deleteNum;
            MediaInLakeSetCheckFinish();
            if (scannerStatus_ == ScannerStatus::CHECK_SCAN) {
                reportData_.checkEndTime = static_cast<uint64_t>(MediaFileUtils::UTCTimeMilliSeconds());
                AncoDfxManager::GetInstance().ReportAncoCheckInfo(reportData_);
                ResetReportData();
            }
        } else {
            reportData_.checkDelete += deleteNum;
        }
    }
    return ERR_SUCCESS;
}

int32_t GlobalScanner::ProcessIncrementScanTask(bool isGlobalScanEnd)
{
    std::unique_lock<std::mutex> lock(scanMutex_);
    std::queue<std::pair<MediaLakeNotifyInfo, ScanTaskType>> scanTaskQueue;
    scanTaskQueue.swap(scanTaskQueue_);
    if (isGlobalScanEnd) {
        scannerStatus_ = ScannerStatus::IDLE;
    }
    lock.unlock();

    while (!scanTaskQueue.empty()) {
        std::pair<MediaLakeNotifyInfo, ScanTaskType> task = scanTaskQueue.front();
        scanTaskQueue.pop();
        MediaLakeNotifyInfo currentPathInfo = task.first;
        if (task.second == ScanTaskType::File) {
            FileScanner fileScanner(LakeScanMode::FULL);
            CHECK_AND_PRINT_LOG(fileScanner.Run({currentPathInfo}) == ERR_SUCCESS,
                "Scan current file failed, current file path is %{public}s",
                LakeFileUtils::GarbleFilePath(currentPathInfo.afterPath).c_str());
            continue;
        }
        FolderScanner folderScanner(currentPathInfo, LakeScanMode::FULL);
        CHECK_AND_PRINT_LOG(folderScanner.Run() == ERR_SUCCESS,
            "Scan current directory failed, current directory path is %{public}s",
            LakeFileUtils::GarbleFilePath(currentPathInfo.afterPath).c_str());
    }
    return ERR_SUCCESS;
}

bool GlobalScanner::IsGlobalScanning(const std::vector<MediaLakeNotifyInfo> &notifyInfos, const ScanTaskType &type)
{
    std::lock_guard<std::mutex> lock(scanMutex_);
    if (scannerStatus_ == ScannerStatus::IDLE) {
        return false;
    }

    for (const auto &notifyInfo : notifyInfos) {
        scanTaskQueue_.push({notifyInfo, type});
    }
    return true;
}

ScannerStatus GlobalScanner::GetScannerStatus()
{
    std::lock_guard<std::mutex> lock(scanMutex_);
    return scannerStatus_;
}

void GlobalScanner::InterruptScanner()
{
    std::lock_guard<std::mutex> lock(scanMutex_);
    MEDIA_INFO_LOG("Interrupt scanner, scannerStatus: %{public}d, isNotInterruptScanner: %{public}d",
        static_cast<int32_t>(scannerStatus_), isNotInterruptScanner_.load());
    if (scannerStatus_ != ScannerStatus::CHECK_SCAN) {
        return;
    }
    isNotInterruptScanner_ = false;
}

void GlobalScanner::CheckToDeleteAssets(FolderScanner &folderScanner)
{
    if (!folderScanner.IsScanFolderFile() || folderScanner.GetAlbumId() <= 0) {
        MEDIA_INFO_LOG("folder no change");
        return;
    }
    int32_t deleteNum = 0;
    std::vector<int32_t> fileIds;
    folderScanner.GetFileIds(fileIds);
    CheckAndIfNeedDeleteAssets(folderScanner.GetAlbumId(), fileIds, deleteNum);
    reportData_.checkDelete += deleteNum;
}

}