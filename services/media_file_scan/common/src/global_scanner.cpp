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

#include "global_scanner.h"

#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <sys/stat.h>

#include "check_dfx_collector.h"
#include "check_scene_helper.h"
#include "i_scan_policy.h"
#include "file_scanner.h"
#include "file_scan_utils.h"
#include "media_time_utils.h"
#include "folder_scanner.h"
#include "folder_scanner_utils.h"
#include "medialibrary_rdb_utils.h"
#include "media_lake_clone_event_manager.h"
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "lake_scan_policy.h"
#include "media_lake_check.h"
#endif
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
#include "file_manager_scan_policy.h"
#endif

namespace OHOS::Media {
namespace fs = std::filesystem;
namespace {
    constexpr int32_t HIGH_TEMP_WAIT_INTERVAL_MS = 30000;
}
// LCOV_EXCL_START
GlobalScanner& GlobalScanner::GetInstance()
{
    static GlobalScanner instance;
    return instance;
}

void GlobalScanner::RunLakeScan(const std::string &path, bool isFirstScanner)
{
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    LakeScanPolicy policy;
    CheckDfxCollector dfxCollector(CheckScene::LAKE);
    Run(path, policy, dfxCollector, isFirstScanner);
#endif
}

void GlobalScanner::RunLakeScan(const std::string &path, CheckDfxCollector &dfxCollector, bool isFirstScanner)
{
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    LakeScanPolicy policy;
    Run(path, policy, dfxCollector, isFirstScanner);
#endif
}

void GlobalScanner::RunFileManagerScan(const std::string &path, CheckDfxCollector &dfxCollector, bool isFirstScanner)
{
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
    FileManagerScanPolicy policy;
    Run(path, policy, dfxCollector, isFirstScanner);
#endif
}

void GlobalScanner::Run(const std::string &path, IScanPolicy &policy, CheckDfxCollector &dfxCollector,
    bool isFirstScanner)
{
    CHECK_AND_RETURN_LOG(!IsForceScanning(), "Global scan is prohibited during the Restore process");
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        if (scannerStatus_ != ScannerStatus::IDLE) {
            MEDIA_WARN_LOG("global scan is running: %{public}d", static_cast<int32_t>(scannerStatus_));
            return;
        }

        scannerStatus_ = isFirstScanner ? ScannerStatus::GLOBAL_SCAN : ScannerStatus::CHECK_SCAN;
        isNotInterruptScanner_ = true;
        InitTemperatureCondition();
    }
    MEDIA_INFO_LOG("global scan start, isFirstScanner: %{public}d, scannerStatus: %{public}d, "
        "isLakeCloneRestoring: %{public}d", isFirstScanner, static_cast<int32_t>(scannerStatus_),
        static_cast<int32_t>(isLakeCloneRestoring_.load()));
    int32_t ret = WalkFileTree(path, policy, dfxCollector);
    CHECK_AND_RETURN_LOG(!IsForceScanning(), "WalkFileTree is prohibited during the Restore process");
    ProcessIncrementScanTask(true, policy);
    SetCloneScanInfo(false, 0);
    CHECK_AND_RETURN_LOG(ret == ERR_SUCCESS, "An exception occurred while walking the file tree");
    policy.OnScanFinished(isFirstScanner);
    MEDIA_INFO_LOG("global scan end, isFirstScanner: %{public}d, deleteCountForCloneRestore_: %{public}d",
        isFirstScanner, deleteCountForCloneRestore_.load());
}

int32_t GlobalScanner::WalkFileTree(const std::string &path, IScanPolicy &policy, CheckDfxCollector &dfxCollector)
{
    std::error_code errorCode;
    bool isDirectory = fs::exists(path, errorCode) && fs::is_directory(path, errorCode);
    CHECK_AND_RETURN_RET_LOG(isDirectory, ERR_INCORRECT_PATH, "invalid directory[%{public}s], error[%{public}s]",
        FileScanUtils::GarbleFilePath(path).c_str(), errorCode.message().c_str());

    const auto &scanRule = CheckSceneHelper::GetScanRuleConfig(policy.GetScene());
    queue<std::string> dirQueue;
    dirQueue.push(path);

    while (!dirQueue.empty() && isNotInterruptScanner_) {
        ProcessIncrementScanTask(false, policy);

        std::string currentDir = dirQueue.front();
        dirQueue.pop();

        // 长度过长
        size_t len = currentDir.length();
        CHECK_AND_CONTINUE_ERR_LOG(len > 0 && len < FILENAME_MAX - 1, "dir[%{public}s] error.",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        bool shouldScan = FolderScannerUtils::ShouldScanDirectory(currentDir, scanRule);
        CHECK_AND_CONTINUE_ERR_LOG(shouldScan, "Not scan dir: %{public}s",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        bool isSkipDirectory = FolderScannerUtils::IsSkipCurrentDirectory(currentDir, scanRule);
        CHECK_AND_CONTINUE_ERR_LOG(!isSkipDirectory, "Skip dir path: %{public}s",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        FolderScanner folderScanner(currentDir, ScanMode::FULL);
        int32_t ret = folderScanner.ScanCurrentDirectory(dirQueue);

        CHECK_AND_RETURN_RET_WARN_LOG(!IsForceScanning(), ERR_SUCCESS,
            "Global scan is due to the Restore process stopping at dir[%{public}s] scanning",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        CHECK_AND_CONTINUE_ERR_LOG(ret == ERR_SUCCESS, "Scan dir[%{public}s] failed",
            FileScanUtils::GarbleFilePath(currentDir).c_str());

        dfxCollector.OnPhotoAdd(folderScanner.GetAddCount());
        dfxCollector.OnPhotoUpdate(folderScanner.GetUpdateCount());
        dfxCollector.OnPhotoDelete(CheckToDeleteAssets(folderScanner));
    }
    return ERR_SUCCESS;
}

int32_t GlobalScanner::ProcessIncrementScanTask(bool isGlobalScanEnd, IScanPolicy &policy)
{
    std::unique_lock<std::mutex> lock(scanMutex_);
    std::queue<std::pair<MediaNotifyInfo, ScanTaskType>> scanTaskQueue;
    scanTaskQueue.swap(scanTaskQueue_);
    if (isGlobalScanEnd) {
        scannerStatus_ = ScannerStatus::IDLE;
    }
    lock.unlock();

    // Temperature control (ScannerStatus::GLOBAL_SCAN)
    CheckScanTemperature();

    while (!scanTaskQueue.empty()) {
        std::pair<MediaNotifyInfo, ScanTaskType> task = scanTaskQueue.front();
        scanTaskQueue.pop();
        MediaNotifyInfo currentPathInfo = task.first;
        if (task.second == ScanTaskType::File) {
            std::unique_ptr<FileScanner> fileScanner = policy.CreateFileScanner(ScanMode::FULL);
            CHECK_AND_PRINT_LOG(fileScanner != nullptr && fileScanner->Run({currentPathInfo}) == ERR_SUCCESS,
                "Scan current file failed, current file path is %{public}s",
                FileScanUtils::GarbleFilePath(currentPathInfo.afterPath).c_str());
            continue;
        }
        FolderScanner folderScanner(currentPathInfo, ScanMode::FULL);
        CHECK_AND_PRINT_LOG(folderScanner.Run() == ERR_SUCCESS,
            "Scan current directory failed, current directory path is %{public}s",
            FileScanUtils::GarbleFilePath(currentPathInfo.afterPath).c_str());
    }
    return ERR_SUCCESS;
}

bool GlobalScanner::IsGlobalScanning(const std::vector<MediaNotifyInfo> &notifyInfos, const ScanTaskType &type)
{
    std::vector<MediaNotifyInfo> filteredInfos;
    for (const auto &notifyInfo : notifyInfos) {
        if (IsCloneRestoringFile(notifyInfo.afterPath)) {
            MEDIA_INFO_LOG("LakeClone: skip clone restoring file: %{public}s",
                FileScanUtils::GarbleFilePath(notifyInfo.afterPath).c_str());
            continue;
        }
        filteredInfos.push_back(notifyInfo);
    }
    std::lock_guard<std::mutex> lock(scanMutex_);
    if (scannerStatus_ == ScannerStatus::IDLE) {
        return false;
    }

    for (const auto &notifyInfo : filteredInfos) {
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

int32_t GlobalScanner::CheckToDeleteAssets(FolderScanner &folderScanner)
{
    if (!folderScanner.IsScanFolderFile() || folderScanner.GetAlbumId() <= 0) {
        MEDIA_INFO_LOG("folder no change");
        return 0;
    }
    int32_t deleteNum = 0;
    std::vector<int32_t> fileIds;
    folderScanner.GetFileIds(fileIds);
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    CheckAndIfNeedDeleteAssets(folderScanner.GetAlbumId(), fileIds, deleteNum);
#endif
    return deleteNum;
}

bool GlobalScanner::IsForceScanning()
{
    bool isRestore = MediaLakeCloneEventManager::GetInstance().IsRestoring();
    if (!isRestore) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        std::queue<std::pair<MediaNotifyInfo, ScanTaskType>> scanTaskQueue;
        scanTaskQueue.swap(scanTaskQueue_);
        // 添加清理状态
        if (scannerStatus_ != ScannerStatus::IDLE) {
            scannerStatus_ = ScannerStatus::IDLE;
            MEDIA_INFO_LOG("LakeClone: set scannerStatus to IDLE");
        }
    }
    MEDIA_WARN_LOG("Global scan is forcibly stopped");
    return true;
}

void GlobalScanner::UpdateTemperatureCondition(bool isHighTemperature)
{
    if (isHighTemperature_.load() != isHighTemperature) {
        isHighTemperature_ = isHighTemperature;
        MEDIA_INFO_LOG("Global scanning temperature status changed, isHighTemperature update: %{public}d",
            isHighTemperature_.load());
    }
}

void GlobalScanner::CheckScanTemperature()
{
    ScannerStatus currentStatus;
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        currentStatus = scannerStatus_;
    }

    CHECK_AND_RETURN(currentStatus == ScannerStatus::GLOBAL_SCAN);
    // 高温循环等待
    while (isHighTemperature_.load()) {
        MEDIA_INFO_LOG("Under global scanning high-temperature control, waiting for cooling down.");
        {
            std::lock_guard<std::mutex> lock(scanMutex_);
            if (scannerStatus_ != ScannerStatus::IDLE) {
                scannerStatus_ = ScannerStatus::IDLE;
                MEDIA_INFO_LOG("Global scan status set to IDLE due to high temperature");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(HIGH_TEMP_WAIT_INTERVAL_MS));
    }

    // 降温恢复全局扫描
    {
        std::lock_guard<std::mutex> lock(scanMutex_);
        if (scannerStatus_ == ScannerStatus::IDLE) {
            scannerStatus_ = ScannerStatus::GLOBAL_SCAN;
            MEDIA_INFO_LOG("Global scan status restored to GLOBAL_SCAN (temperature cooled down)");
        }
    }
}

void GlobalScanner::InitTemperatureCondition()
{
    const int32_t properDeviceTemperatureLevel40 = 2;
    isHighTemperature_ = false;

#ifdef HAS_THERMAL_MANAGER_PART
    auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    int32_t newTemperatureLevel = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
    isHighTemperature_ = newTemperatureLevel > properDeviceTemperatureLevel40 ? true : false;
#endif
    MEDIA_INFO_LOG("Init global scanner temperature condition: %{public}d", isHighTemperature_.load());
}

void GlobalScanner::SetCloneScanInfo(bool isLakeCloneRestoring, int64_t cloneEndTime)
{
    isLakeCloneRestoring_.store(isLakeCloneRestoring);
    cloneEndTime_.store(cloneEndTime);
    deleteCountForCloneRestore_.store(0);
    MEDIA_INFO_LOG("LakeClone: isLakeCloneRestoring: %{public}d, cloneEndTime: %{public}" PRId64
        "deleteCountForCloneRestore: %{public}d", static_cast<int32_t>(isLakeCloneRestoring),
        cloneEndTime, deleteCountForCloneRestore_.load());
}

bool GlobalScanner::IsLakeCloneRestoring()
{
    return isLakeCloneRestoring_.load();
}

bool GlobalScanner::IsCloneRestoringFile(const std::string &filePath)
{
    CHECK_AND_RETURN_RET(IsLakeCloneRestoring(), false);
    int64_t cloneEndTime = cloneEndTime_.load();
    CHECK_AND_RETURN_RET(cloneEndTime != 0, false);
    struct stat fileStat;
    CHECK_AND_RETURN_RET_WARN_LOG(stat(filePath.c_str(), &fileStat) == 0, false, "Failed to stat %{public}s",
        FileScanUtils::GarbleFilePath(filePath).c_str());

    return MediaTimeUtils::Timespec2Millisecond(fileStat.st_atim) < cloneEndTime;
}
// LCOV_EXCL_STOP
}