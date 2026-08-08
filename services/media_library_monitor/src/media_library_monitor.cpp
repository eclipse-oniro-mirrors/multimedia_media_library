/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryMonitor"

#include "media_library_monitor.h"

#include <cinttypes>
#include <chrono>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <string>

#include "media_log.h"

using namespace std;

namespace OHOS::Media::Monitor {

MediaLibraryMonitor& MediaLibraryMonitor::GetInstance()
{
    static MediaLibraryMonitor instance;
    return instance;
}

MediaLibraryMonitor::MediaLibraryMonitor() {}

MediaLibraryMonitor::~MediaLibraryMonitor()
{
    Stop();
}

void MediaLibraryMonitor::Start()
{
    {
        lock_guard<mutex> lockGuard(mutex_);
        if (isRunning_) {
            return;
        }
        isRunning_ = true;
    }
    monitorThread_ = thread(&MediaLibraryMonitor::Run, this);
    MEDIA_INFO_LOG("MediaLibraryMonitor started");
}

void MediaLibraryMonitor::Stop()
{
    {
        lock_guard<mutex> lockGuard(mutex_);
        if (!isRunning_) {
            return;
        }
        isRunning_ = false;
    }
    cv_.notify_all();
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    MEDIA_INFO_LOG("MediaLibraryMonitor stopped");
}

void MediaLibraryMonitor::Run()
{
    pthread_setname_np(pthread_self(), "MedLibMonitor");
    MEDIA_INFO_LOG("MediaLibraryMonitor thread enter");
    unique_lock<mutex> lock(mutex_);
    while (isRunning_) {
        ProcessMemoryInfo info;
        if (ReadSmapsRollup(info)) {
            LogMemoryInfo(info);
        }
        cv_.wait_for(lock, chrono::milliseconds(MONITOR_INTERVAL_MS),
            [this]() { return !isRunning_; });
    }
    MEDIA_INFO_LOG("MediaLibraryMonitor thread exit");
}

bool MediaLibraryMonitor::ReadSmapsRollup(ProcessMemoryInfo& info)
{
    ifstream file(SMAPS_ROLLUP_PATH);
    if (!file.is_open()) {
        MEDIA_ERR_LOG("Failed to open %{public}s", SMAPS_ROLLUP_PATH);
        return false;
    }
    string line;
    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) {
            continue;
        }
        string key = line.substr(0, colon);
        uint64_t val = 0;
        istringstream iss(line.substr(colon + 1));
        iss >> val;
        if (key == "Rss") {
            info.rssKb = val;
        } else if (key == "Pss") {
            info.pssKb = val;
        } else if (key == "Shared_Clean") {
            info.sharedCleanKb = val;
        } else if (key == "Shared_Dirty") {
            info.sharedDirtyKb = val;
        } else if (key == "Private_Clean") {
            info.privateCleanKb = val;
        } else if (key == "Private_Dirty") {
            info.privateDirtyKb = val;
        } else if (key == "SwapPss") {
            info.swapPssKb = val;
        }
    }
    return true;
}

void MediaLibraryMonitor::LogMemoryInfo(const ProcessMemoryInfo& info)
{
    if (hasLastPrinted_) {
        uint64_t delta = info.pssKb > lastPrintedPssKb_ ? info.pssKb - lastPrintedPssKb_
                                                        : lastPrintedPssKb_ - info.pssKb;
        if (delta < PRINT_THRESHOLD_KB) {
            return;
        }
    }
    MEDIA_INFO_LOG("smaps_rollup Rss:%{public}" PRIu64 " kB Pss:%{public}" PRIu64 " kB "
        "Shared_Clean:%{public}" PRIu64 " kB Shared_Dirty:%{public}" PRIu64 " kB "
        "Private_Clean:%{public}" PRIu64 " kB Private_Dirty:%{public}" PRIu64 " kB "
        "Swap_Pss:%{public}" PRIu64 " kB",
        info.rssKb, info.pssKb, info.sharedCleanKb, info.sharedDirtyKb,
        info.privateCleanKb, info.privateDirtyKb, info.swapPssKb);
    lastPrintedPssKb_ = info.pssKb;
    hasLastPrinted_ = true;
}
}  // namespace OHOS::Media::Monitor
