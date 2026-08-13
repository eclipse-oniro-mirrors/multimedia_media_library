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

#ifndef OHOS_MEDIA_LIBRARY_MONITOR_H
#define OHOS_MEDIA_LIBRARY_MONITOR_H

#include <cstdint>
#include <string>

namespace OHOS::Media::Monitor {

#define EXPORT __attribute__ ((visibility ("default")))

struct ProcessMemoryInfo {
    uint64_t rssKb = 0;
    uint64_t pssKb = 0;
    uint64_t sharedCleanKb = 0;
    uint64_t sharedDirtyKb = 0;
    uint64_t privateCleanKb = 0;
    uint64_t privateDirtyKb = 0;
    uint64_t swapPssKb = 0;
};

class EXPORT MediaLibraryMonitor {
public:
    static MediaLibraryMonitor& GetInstance();
    void Tick();

private:
    MediaLibraryMonitor();
    ~MediaLibraryMonitor();
    MediaLibraryMonitor(const MediaLibraryMonitor&) = delete;
    MediaLibraryMonitor& operator=(const MediaLibraryMonitor&) = delete;

    bool ReadSmapsRollup(ProcessMemoryInfo& info);
    void LogMemoryInfo(const ProcessMemoryInfo& info);

    static constexpr const char* SMAPS_ROLLUP_PATH = "/proc/self/smaps_rollup";
};
}  // namespace OHOS::Media::Monitor
#endif  // OHOS_MEDIA_LIBRARY_MONITOR_H
