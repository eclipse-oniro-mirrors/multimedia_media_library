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

#ifndef OHOS_MEDIA_DOCS_MEDIA_SCAN_MANAGER_H
#define OHOS_MEDIA_DOCS_MEDIA_SCAN_MANAGER_H

#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "dfx_const.h"
#include "dfx_database_utils.h"
#include "preferences.h"

namespace OHOS {
namespace Media {

class DocsMediaScanManager {
public:
    struct FolderStatsCollector {
        int32_t imageCount = 0;
        int32_t videoCount = 0;
        std::unordered_map<std::string, int32_t> formatMap;
        std::vector<int32_t> sizeDistribution;
        time_t minAtime = INT64_MAX;
        time_t maxAtime = 0;
        FolderStatsCollector();
    };

    struct DirScanResult {
        std::vector<std::string> mediaFiles;
        std::vector<struct stat> mediaStats;
        std::vector<std::pair<std::string, std::string>> subDirs;
    };

    static DocsMediaScanManager& GetInstance();
    void Execute();

private:
    DocsMediaScanManager();
    ~DocsMediaScanManager() = default;
    DocsMediaScanManager(const DocsMediaScanManager&) = delete;
    DocsMediaScanManager& operator=(const DocsMediaScanManager&) = delete;
    bool IsBetaVersion();
    bool IsTaskCompleted();
    bool IsTraversalDone();
    std::shared_ptr<NativePreferences::Preferences> GetPrefs();
    void MarkTraversalDone();
    void MarkTaskCompleted();
    int32_t ResetDailyCountIfNeeded();
    bool CheckDailyLimit();
    void IncrementDailyCount();

    bool TraverseAndCollect();
    bool CollectFolderStats(const std::string &fullPath, const std::string &relativePath,
        const std::vector<std::string> &mediaFiles, const std::vector<struct stat> &mediaStats);
    MediaType GetMediaTypeOfFile(const std::string &filePath);
    bool IsMediaFile(const std::string &filePath);
    void UpdateFileStats(const std::string &extension, MediaType mediaType, const struct stat &fileStat,
        DocsMediaScanManager::FolderStatsCollector &collector);
    int32_t GetSizeBucket(off_t fileSize);
    DocsMediaScanManager::DirScanResult ReadDirectoryEntries(const std::string &path, const std::string &relativePath);
    void ReportPhase();
    void Finalize();
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_DOCS_MEDIA_SCAN_MANAGER_H