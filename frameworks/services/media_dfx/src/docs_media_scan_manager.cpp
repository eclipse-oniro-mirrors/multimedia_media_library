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

#define MLOG_TAG "DocsMediaScan"

#include "docs_media_scan_manager.h"

#include <chrono>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "dfx_const.h"
#include "dfx_database_utils.h"
#include "dfx_reporter.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {

static const std::string KEY_HIVIEW_VERSION_TYPE = "const.logsystem.versiontype";
static const std::string DOCS_MEDIA_SCAN_ROOT_DIR = "/storage/media/local/files/Docs";
static const int32_t DOCS_MEDIA_SCAN_BATCH_SIZE = 20;
static const int32_t DOCS_MEDIA_SCAN_DAILY_LIMIT = 80;
static const int32_t DOCS_MEDIA_SCAN_ATIME_THRESHOLD_SEC = 1800;
static const std::string DOCS_MEDIA_SCAN_DONE = "docs_media_scan_done";
static const std::string DOCS_MEDIA_SCAN_TRAVERSAL_DONE = "docs_media_scan_traversal_done";
static const std::string DOCS_MEDIA_SCAN_REPORTED_FOLDERS = "docs_media_scan_reported_folders";
static const std::string DOCS_MEDIA_SCAN_DAILY_COUNT = "docs_media_scan_daily_count";
static const std::string DOCS_MEDIA_SCAN_DAILY_DATE = "docs_media_scan_daily_date";
static const std::string DEFAULT_VERSION_TYPE = "unknown";
static const std::string BETA_VERSION_KEYWORD = "beta";
static const std::string DATE_FORMAT = "%Y%m%d";
static constexpr int32_t DATE_BUFFER_SIZE = 16;
static constexpr int32_t SIZE_BUCKET_COUNT = 7;
static constexpr off_t SIZE_THRESHOLD_1K = 1024;
static constexpr off_t SIZE_THRESHOLD_10K = 10 * SIZE_THRESHOLD_1K;
static constexpr off_t SIZE_THRESHOLD_50K = 50 * SIZE_THRESHOLD_1K;
static constexpr off_t SIZE_THRESHOLD_100K = 100 * SIZE_THRESHOLD_1K;
static constexpr off_t SIZE_THRESHOLD_500K = 500 * SIZE_THRESHOLD_1K;
static constexpr off_t SIZE_THRESHOLD_1M = 1024 * SIZE_THRESHOLD_1K;
static const std::string JSON_KEY_DIR_PATH = "dp";
static const std::string JSON_KEY_IMAGE_COUNT = "ic";
static const std::string JSON_KEY_VIDEO_COUNT = "vc";
static const std::string JSON_KEY_FORMAT_DISTRIBUTION = "fd";
static const std::string JSON_KEY_SIZE_DISTRIBUTION = "sd";
static const std::string JSON_KEY_ATIME_WITHIN_30MIN = "a30";
static const std::string JSON_KEY_ATIME_DIFF_SEC = "as";
static const std::string CURRENT_DIR = ".";
static const std::string PARENT_DIR = "..";

struct FolderStatsCollector {
    int32_t imageCount = 0;
    int32_t videoCount = 0;
    std::map<std::string, int32_t> formatMap;
    std::vector<int32_t> sizeDistribution;
    time_t minAtime = INT64_MAX;
    time_t maxAtime = 0;
    FolderStatsCollector() : sizeDistribution(SIZE_BUCKET_COUNT, 0) {}
};

struct DirScanResult {
    std::vector<std::string> mediaFiles;
    std::vector<struct stat> mediaStats;
    std::vector<std::pair<std::string, std::string>> subDirs;
};

DocsMediaScanManager& DocsMediaScanManager::GetInstance()
{
    static DocsMediaScanManager instance;
    return instance;
}

DocsMediaScanManager::DocsMediaScanManager()
{
}

bool DocsMediaScanManager::IsBetaVersion()
{
    static const std::string versionType = system::GetParameter(KEY_HIVIEW_VERSION_TYPE, DEFAULT_VERSION_TYPE);
    static bool isBetaVersion = versionType.find(BETA_VERSION_KEYWORD) != std::string::npos;
    return isBetaVersion;
}

shared_ptr<NativePreferences::Preferences> DocsMediaScanManager::GetPrefs()
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
    }
    return prefs;
}

bool DocsMediaScanManager::IsTaskCompleted()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, true, "prefs is nullptr, treat as completed");
    return prefs->GetBool(DOCS_MEDIA_SCAN_DONE, false);
}

bool DocsMediaScanManager::IsTraversalDone()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is nullptr");
    return prefs->GetBool(DOCS_MEDIA_SCAN_TRAVERSAL_DONE, false);
}

void DocsMediaScanManager::MarkTraversalDone()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutBool(DOCS_MEDIA_SCAN_TRAVERSAL_DONE, true);
    prefs->FlushSync();
}

void DocsMediaScanManager::MarkTaskCompleted()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutBool(DOCS_MEDIA_SCAN_DONE, true);
    prefs->FlushSync();
}

int32_t DocsMediaScanManager::ResetDailyCountIfNeeded()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, E_FAIL, "prefs is nullptr");
    auto now = std::chrono::system_clock::now();
    auto nowTimeT = std::chrono::system_clock::to_time_t(now);
    struct tm tmStruct;
    localtime_r(&nowTimeT, &tmStruct);
    char dateStr[DATE_BUFFER_SIZE] = {0};
    strftime(dateStr, sizeof(dateStr), DATE_FORMAT.c_str(), &tmStruct);
    std::string storedDate = prefs->GetString(DOCS_MEDIA_SCAN_DAILY_DATE, "");
    if (storedDate != dateStr) {
        prefs->PutInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
        prefs->PutString(DOCS_MEDIA_SCAN_DAILY_DATE, dateStr);
        prefs->FlushSync();
    }
    return E_OK;
}

bool DocsMediaScanManager::CheckDailyLimit()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, true, "prefs is nullptr, treat as limit reached");
    int32_t dailyCount = prefs->GetInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
    return dailyCount >= DOCS_MEDIA_SCAN_DAILY_LIMIT;
}

void DocsMediaScanManager::IncrementDailyCount()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    int32_t dailyCount = prefs->GetInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
    prefs->PutInt(DOCS_MEDIA_SCAN_DAILY_COUNT, dailyCount + 1);
    prefs->FlushSync();
}

bool DocsMediaScanManager::IsMediaFile(const std::string &filePath)
{
    MediaType mediaType = GetMediaTypeOfFile(filePath);
    return mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO;
}

MediaType DocsMediaScanManager::GetMediaTypeOfFile(const std::string &filePath)
{
    MediaType mediaType = MediaFileUtils::GetMediaType(filePath);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        return mediaType;
    }
    mediaType = MediaFileUtils::GetMediaTypeNotSupported(filePath);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        return mediaType;
    }
    return mediaType;
}

static void UpdateFileStats(const std::string &extension, MediaType mediaType,
    const struct stat &fileStat, FolderStatsCollector &collector)
{
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        collector.imageCount++;
    } else if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        collector.videoCount++;
    } else {
        return;
    }
    if (!extension.empty()) {
        collector.formatMap[extension]++;
    }
    collector.sizeDistribution[GetSizeBucket(fileStat.st_size)]++;
    if (fileStat.st_atime < collector.minAtime) {
        collector.minAtime = fileStat.st_atime;
    }
    if (fileStat.st_atime > collector.maxAtime) {
        collector.maxAtime = fileStat.st_atime;
    }
}

int32_t DocsMediaScanManager::GetSizeBucket(off_t fileSize)
{
    if (fileSize <= SIZE_THRESHOLD_1K) {
        return 0;
    }
    if (fileSize <= SIZE_THRESHOLD_10K) {
        return 1;
    }
    if (fileSize <= SIZE_THRESHOLD_50K) {
        return 2;
    }
    if (fileSize <= SIZE_THRESHOLD_100K) {
        return 3;
    }
    if (fileSize <= SIZE_THRESHOLD_500K) {
        return 4;
    }
    if (fileSize <= SIZE_THRESHOLD_1M) {
        return 5;
    }
return 6;
}

static DirScanResult ReadDirectoryEntries(const std::string &path, const std::string &relativePath)
{
    DirScanResult result;
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        MEDIA_ERR_LOG("Failed to opendir: %{public}s", path.c_str());
        return result;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, CURRENT_DIR.c_str()) == 0 || strcmp(entry->d_name, PARENT_DIR.c_str()) == 0) {
            continue;
        }
        std::string fullPath = path + SPLIT_PATH + entry->d_name;
        struct stat statInfo;
        if (lstat(fullPath.c_str(), &statInfo) != 0) {
            MEDIA_ERR_LOG("Failed to lstat: %{public}s", fullPath.c_str());
            continue;
        }
        if (S_ISLNK(statInfo.st_mode)) {
            continue;
        }
        if (S_ISDIR(statInfo.st_mode)) {
            std::string subRelative = relativePath.empty() ? entry->d_name : relativePath + SPLIT_PATH + entry->d_name;
            result.subDirs.push_back({fullPath, subRelative});
        } else if (S_ISREG(statInfo.st_mode)) {
            if (DocsMediaScanManager::IsMediaFile(fullPath)) {
                result.mediaFiles.push_back(entry->d_name);
                result.mediaStats.push_back(statInfo);
            }
        }
    }
    closedir(dir);
    return result;
}

static DocsScanFolderStats SerializeFolderStats(const std::string &relativePath,
    const FolderStatsCollector &collector)
{
    DocsScanFolderStats stats;
    stats.dirPath = relativePath;
    stats.imageCount = collector.imageCount;
    stats.videoCount = collector.videoCount;

    nlohmann::json formatJson;
    for (const auto &entry : collector.formatMap) {
        formatJson[entry.first] = entry.second;
    }
    stats.formatDistribution = formatJson.dump();

    nlohmann::json sizeJson = collector.sizeDistribution;
    stats.sizeDistribution = sizeJson.dump();

    if (collector.minAtime <= collector.maxAtime) {
        time_t atimeDiff = collector.maxAtime - collector.minAtime;
        stats.atimeWithin30min = (atimeDiff <= DOCS_MEDIA_SCAN_ATIME_THRESHOLD_SEC) ? 1 : 0;
        stats.atimeDiffSec = static_cast<int32_t>(atimeDiff);
    }

    return stats;
}

bool DocsMediaScanManager::CollectFolderStats(const std::string &fullPath,
    const std::string &relativePath, const std::vector<std::string> &mediaFiles,
    const std::vector<struct stat> &mediaStats)
{
    if (DfxDatabaseUtils::IsDirPathInDocsScanTempTable(relativePath)) {
        return true;
    }

    FolderStatsCollector collector;
    for (size_t index = 0; index < mediaFiles.size(); index++) {
        std::string filePath = fullPath + SPLIT_PATH + mediaFiles[index];
        MediaType mediaType = GetMediaTypeOfFile(filePath);
        std::string extension = MediaFileUtils::GetExtensionFromPath(filePath);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        UpdateFileStats(extension, mediaType, mediaStats[index], collector);
    }

    DocsScanFolderStats stats = SerializeFolderStats(relativePath, collector);
    int32_t ret = DfxDatabaseUtils::InsertDocsScanFolderStats(stats);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "InsertDocsScanFolderStats failed: %{public}d", ret);
    return true;
}

bool DocsMediaScanManager::TraverseAndCollect()
{
    DIR *rootDir = opendir(DOCS_MEDIA_SCAN_ROOT_DIR.c_str());
    if (rootDir == nullptr) {
        MEDIA_INFO_LOG("Docs root dir not present, marking task as completed");
        MarkTaskCompleted();
        return true;
    }
    closedir(rootDir);

    int32_t ret = DfxDatabaseUtils::CreateDocsMediaScanTempTable();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "CreateDocsMediaScanTempTable failed: %{public}d", ret);

    std::vector<std::pair<std::string, std::string>> dirStack;
    dirStack.push_back({DOCS_MEDIA_SCAN_ROOT_DIR, ""});

    while (!dirStack.empty()) {
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Background status off, pausing docs media scan");
            return false;
        }

        auto current = dirStack.back();
        dirStack.pop_back();

        DirScanResult result = ReadDirectoryEntries(current.first, current.second);

        if (!result.mediaFiles.empty()) {
            if (!CollectFolderStats(current.first, current.second, result.mediaFiles, result.mediaStats)) {
                return false;
            }
        }

        for (auto &subDir : result.subDirs) {
            dirStack.push_back(subDir);
        }
    }

    MarkTraversalDone();
    return true;
}

static nlohmann::json BuildBatchJson(const std::vector<DocsScanFolderStats> &batch)
{
    nlohmann::json batchArray = nlohmann::json::array();
    for (auto &stats : batch) {
        nlohmann::json folderObj;
        folderObj[JSON_KEY_DIR_PATH] = stats.dirPath;
        folderObj[JSON_KEY_IMAGE_COUNT] = stats.imageCount;
        folderObj[JSON_KEY_VIDEO_COUNT] = stats.videoCount;
        folderObj[JSON_KEY_FORMAT_DISTRIBUTION] = nlohmann::json::parse(stats.formatDistribution, nullptr, false);
        folderObj[JSON_KEY_SIZE_DISTRIBUTION] = nlohmann::json::parse(stats.sizeDistribution, nullptr, false);
        folderObj[JSON_KEY_ATIME_WITHIN_30MIN] = stats.atimeWithin30min == 1;
        folderObj[JSON_KEY_ATIME_DIFF_SEC] = stats.atimeDiffSec;
        batchArray.push_back(folderObj);
    }
    return batchArray;
}

void DocsMediaScanManager::ReportPhase()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");

    ResetDailyCountIfNeeded();

    int32_t totalFolders = 0;
    int32_t ret = DfxDatabaseUtils::QueryDocsScanTotalFolderCount(totalFolders);
    CHECK_AND_RETURN_LOG(ret == E_OK, "QueryDocsScanTotalFolderCount failed");

    int32_t reportedFolders = prefs->GetInt(DOCS_MEDIA_SCAN_REPORTED_FOLDERS, 0);
    if (reportedFolders >= totalFolders) {
        Finalize();
        return;
    }

    while (reportedFolders < totalFolders) {
        if (CheckDailyLimit()) {
            MEDIA_INFO_LOG("Daily limit reached, will continue next trigger");
            return;
        }
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Background status off, pausing report phase");
            return;
        }

        std::vector<DocsScanFolderStats> batch;
        ret = DfxDatabaseUtils::QueryDocsScanFolderStats(reportedFolders, DOCS_MEDIA_SCAN_BATCH_SIZE, batch);
        CHECK_AND_RETURN_LOG(ret == E_OK, "QueryDocsScanFolderStats failed");

        nlohmann::json batchArray = BuildBatchJson(batch);
        int32_t reportRet = DfxReporter::ReportAncoCountFormatInfoForDirScan(batchArray.dump());
        if (reportRet != 0) {
            MEDIA_ERR_LOG("Report failed, will retry next trigger");
            return;
        }

        IncrementDailyCount();
        reportedFolders += static_cast<int32_t>(batch.size());
        prefs->PutInt(DOCS_MEDIA_SCAN_REPORTED_FOLDERS, reportedFolders);
        prefs->FlushSync();
    }

    if (reportedFolders >= totalFolders) {
        Finalize();
    }
}

void DocsMediaScanManager::Finalize()
{
    MarkTaskCompleted();
    DfxDatabaseUtils::DropDocsMediaScanTempTable();
    MEDIA_INFO_LOG("Docs media scan task completed and finalized");
}

void DocsMediaScanManager::Execute()
{
    if (IsTaskCompleted()) {
        return;
    }
    if (!IsBetaVersion()) {
        return;
    }

    if (!IsTraversalDone()) {
        bool traversalOk = TraverseAndCollect();
        if (!traversalOk) {
            return;
        }
    }

    ReportPhase();
}

} // namespace Media
} // namespace OHOS