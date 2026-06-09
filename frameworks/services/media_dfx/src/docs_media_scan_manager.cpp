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

#define MLOG_TAG "DocsMediaScan"

#include "docs_media_scan_manager.h"

#include <algorithm>
#include <dirent.h>
#include <nlohmann/json.hpp>
#include <regex>

#include "dfx_reporter.h"
#include "file_manager_scan_rule_config.h"
#include "folder_scanner_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "media_time_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "parameters.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {

static const std::string KEY_HIVIEW_VERSION_TYPE = "const.logsystem.versiontype";
static const int32_t DOCS_MEDIA_SCAN_BATCH_SIZE = 10;
static const int32_t DOCS_MEDIA_SCAN_DAILY_LIMIT = 60;
static const int32_t DOCS_MEDIA_SCAN_ATIME_THRESHOLD_SEC = 1800; // 30 * 60 seconds
static const std::string DOCS_MEDIA_SCAN_DONE = "docs_media_scan_done";
static const std::string DOCS_MEDIA_SCAN_TRAVERSAL_DONE = "docs_media_scan_traversal_done";
static const std::string DOCS_MEDIA_SCAN_LAST_REPORTED_ID = "docs_media_scan_last_reported_id";
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
static constexpr int32_t SIZE_BUCKET_1K = 0;
static constexpr int32_t SIZE_BUCKET_10K = 1;
static constexpr int32_t SIZE_BUCKET_50K = 2;
static constexpr int32_t SIZE_BUCKET_100K = 3;
static constexpr int32_t SIZE_BUCKET_500K = 4;
static constexpr int32_t SIZE_BUCKET_1M = 5;
static constexpr int32_t SIZE_BUCKET_1M_PLUS = 6;
static const std::string JSON_KEY_DIR_PATH = "dp";
static const std::string JSON_KEY_IMAGE_COUNT = "ic";
static const std::string JSON_KEY_VIDEO_COUNT = "vc";
static const std::string JSON_KEY_FORMAT_DISTRIBUTION = "fd";
static const std::string JSON_KEY_SIZE_DISTRIBUTION = "sd";
static const std::string JSON_KEY_ATIME_WITHIN_30MIN = "a30";
static const std::string JSON_KEY_ATIME_DIFF_SEC = "as";
static const std::string CURRENT_DIR = ".";
static const std::string PARENT_DIR = "..";
static const std::string ANONYMIZED_IP_FORMAT = "$1.[IP]";
static const std::string ANONYMIZED_ID_CARD_FORMAT = "$1[IDCARD]$2";
static const std::string ANONYMIZED_ADDRESS_FORMAT = "[ADDR:$2]";

DocsMediaScanManager::FolderStatsCollector::FolderStatsCollector() : sizeDistribution(SIZE_BUCKET_COUNT, 0) {}

DocsMediaScanManager &DocsMediaScanManager::GetInstance()
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

std::shared_ptr<NativePreferences::Preferences> DocsMediaScanManager::GetPrefs()
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefs, "Get preferences error: %{public}d", errCode);
    return prefs;
}

bool DocsMediaScanManager::IsTaskCompleted()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, true, "prefs is nullptr, treat as completed");
    bool isTaskCompleted = prefs->GetBool(DOCS_MEDIA_SCAN_DONE, false);
    MEDIA_INFO_LOG("Get %{public}s: %{public}d", DOCS_MEDIA_SCAN_DONE.c_str(), isTaskCompleted);
    return isTaskCompleted;
}

bool DocsMediaScanManager::IsTraversalDone()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is nullptr");
    bool isTraversalDone = prefs->GetBool(DOCS_MEDIA_SCAN_TRAVERSAL_DONE, false);
    MEDIA_INFO_LOG("Get %{public}s: %{public}d", DOCS_MEDIA_SCAN_TRAVERSAL_DONE.c_str(), isTraversalDone);
    return isTraversalDone;
}

void DocsMediaScanManager::MarkTraversalDone()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutBool(DOCS_MEDIA_SCAN_TRAVERSAL_DONE, true);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Set %{public}s: %{public}d", DOCS_MEDIA_SCAN_TRAVERSAL_DONE.c_str(), true);
}

void DocsMediaScanManager::MarkTaskCompleted()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutBool(DOCS_MEDIA_SCAN_DONE, true);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Set %{public}s: %{public}d", DOCS_MEDIA_SCAN_DONE.c_str(), true);
}

int32_t DocsMediaScanManager::ResetDailyCountIfNeeded()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, E_FAIL, "prefs is nullptr");
    std::string currentDate = MediaTimeUtils::StrCreateTime(DATE_FORMAT, MediaTimeUtils::UTCTimeSeconds());
    std::string storedDate = prefs->GetString(DOCS_MEDIA_SCAN_DAILY_DATE, "");
    CHECK_AND_RETURN_RET(storedDate != currentDate, E_OK);
    MEDIA_INFO_LOG("Reset daily count to 0 for %{public}s", currentDate.c_str());
    prefs->PutInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
    prefs->PutString(DOCS_MEDIA_SCAN_DAILY_DATE, currentDate);
    prefs->FlushSync();
    return E_OK;
}

bool DocsMediaScanManager::CheckDailyLimit()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, true, "prefs is nullptr, treat as limit reached");
    int32_t dailyCount = prefs->GetInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
    return dailyCount >= DOCS_MEDIA_SCAN_DAILY_LIMIT;
}

void DocsMediaScanManager::IncrementDailyCountAndUpdateLastReportedId(int32_t lastReportedId)
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    int32_t dailyCount = prefs->GetInt(DOCS_MEDIA_SCAN_DAILY_COUNT, 0);
    prefs->PutInt(DOCS_MEDIA_SCAN_DAILY_COUNT, dailyCount + 1);
    prefs->PutInt(DOCS_MEDIA_SCAN_LAST_REPORTED_ID, lastReportedId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Set %{public}s: %{public}d, %{public}s: %{public}d", DOCS_MEDIA_SCAN_DAILY_COUNT.c_str(),
        dailyCount + 1, DOCS_MEDIA_SCAN_LAST_REPORTED_ID.c_str(), lastReportedId);
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
    return mediaType;
}

void DocsMediaScanManager::UpdateFileStats(const std::string &extension, MediaType mediaType,
    const struct stat &fileStat, DocsMediaScanManager::FolderStatsCollector &collector)
{
    CHECK_AND_RETURN(mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO);

    collector.imageCount += mediaType == MediaType::MEDIA_TYPE_IMAGE;
    collector.videoCount += mediaType == MediaType::MEDIA_TYPE_VIDEO;

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
        return SIZE_BUCKET_1K;
    }
    if (fileSize <= SIZE_THRESHOLD_10K) {
        return SIZE_BUCKET_10K;
    }
    if (fileSize <= SIZE_THRESHOLD_50K) {
        return SIZE_BUCKET_50K;
    }
    if (fileSize <= SIZE_THRESHOLD_100K) {
        return SIZE_BUCKET_100K;
    }
    if (fileSize <= SIZE_THRESHOLD_500K) {
        return SIZE_BUCKET_500K;
    }
    if (fileSize <= SIZE_THRESHOLD_1M) {
        return SIZE_BUCKET_1M;
    }
    return SIZE_BUCKET_1M_PLUS;
}

bool DocsMediaScanManager::ShouldScanDirectory(const std::string &path)
{
    const auto &ruleConfig = GetFileManagerScanRuleConfig();
    return FolderScannerUtils::ShouldScanDirectory(path, ruleConfig) &&
        !FolderScannerUtils::IsSkipCurrentDirectory(path, ruleConfig);
}

bool DocsMediaScanManager::ShouldScanFile(const std::string &path)
{
    const auto &ruleConfig = GetFileManagerScanRuleConfig();
    return IsMediaFile(path) && !FolderScannerUtils::IsSkipCurrentFile(path, ruleConfig);
}

DocsMediaScanManager::DirScanResult DocsMediaScanManager::ReadDirectoryEntries(const std::string &path,
    const std::string &relativePath)
{
    DocsMediaScanManager::DirScanResult result;
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
        std::string fullPath = path + "/" + entry->d_name;
        struct stat statInfo;
        if (lstat(fullPath.c_str(), &statInfo) != 0) {
            MEDIA_ERR_LOG("Failed to lstat: %{public}s", MediaFileUtils::DesensitizePath(fullPath).c_str());
            continue;
        }
        if (S_ISDIR(statInfo.st_mode) && ShouldScanDirectory(fullPath)) {
            std::string subRelative = relativePath.empty() ? entry->d_name : relativePath + "/" + entry->d_name;
            result.subDirs.push_back({fullPath, subRelative});
        } else if (S_ISREG(statInfo.st_mode) && ShouldScanFile(fullPath)) {
            result.mediaFiles.push_back(entry->d_name);
            result.mediaStats.push_back(statInfo);
        }
    }
    closedir(dir);
    return result;
}

static DocsScanFolderStats SerializeFolderStats(const std::string &relativePath,
    const DocsMediaScanManager::FolderStatsCollector &collector)
{
    DocsScanFolderStats stats;
    stats.dirPath = relativePath.empty() ? "/" : relativePath;
    stats.imageCount = collector.imageCount;
    stats.videoCount = collector.videoCount;

    nlohmann::json formatJson;
    for (const auto &entry : collector.formatMap) {
        formatJson[entry.first] = entry.second;
    }
    stats.formatDistribution = formatJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    nlohmann::json sizeJson = collector.sizeDistribution;
    stats.sizeDistribution = sizeJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    if (collector.minAtime <= collector.maxAtime) {
        time_t atimeDiff = collector.maxAtime - collector.minAtime;
        stats.atimeWithin30min = (atimeDiff <= DOCS_MEDIA_SCAN_ATIME_THRESHOLD_SEC) ? 1 : 0;
        stats.atimeDiffSec = static_cast<int64_t>(atimeDiff);
    }

    return stats;
}

bool DocsMediaScanManager::CollectFolderStats(const std::string &fullPath,
    const std::string &relativePath, const std::vector<std::string> &mediaFiles,
    const std::vector<struct stat> &mediaStats)
{
    CHECK_AND_RETURN_RET(!mediaFiles.empty(), true);
    CHECK_AND_RETURN_RET(!DfxDatabaseUtils::IsDirPathInDocsScanTempTable(relativePath), true);

    DocsMediaScanManager::FolderStatsCollector collector;
    for (size_t index = 0; index < mediaFiles.size(); index++) {
        std::string filePath = fullPath + "/" + mediaFiles[index];
        MediaType mediaType = GetMediaTypeOfFile(filePath);
        std::string extension = MediaStringUtils::ToLower(MediaFileUtils::GetExtensionFromPath(filePath));
        UpdateFileStats(extension, mediaType, mediaStats[index], collector);
    }

    DocsScanFolderStats stats = SerializeFolderStats(relativePath, collector);
    int32_t ret = DfxDatabaseUtils::InsertDocsScanFolderStats(stats);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "InsertDocsScanFolderStats failed: %{public}d", ret);
    return true;
}

bool DocsMediaScanManager::TraverseAndCollect()
{
    MEDIA_INFO_LOG("Start TraverseAndCollect");
    if (!MediaFileUtils::IsFileExists(std::string(FILE_MANAGER_ROOT_PATH))) {
        MEDIA_INFO_LOG("Docs root dir not exist, marking task as completed");
        MarkTaskCompleted();
        return true;
    }

    int32_t ret = DfxDatabaseUtils::CreateDocsMediaScanTempTable();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "CreateDocsMediaScanTempTable failed: %{public}d", ret);

    std::vector<std::pair<std::string, std::string>> dirStack;
    dirStack.push_back({std::string(FILE_MANAGER_ROOT_PATH), ""});

    while (!dirStack.empty()) {
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Background status off, pausing docs media scan");
            return false;
        }

        auto current = dirStack.back();
        dirStack.pop_back();

        DocsMediaScanManager::DirScanResult result = ReadDirectoryEntries(current.first, current.second);
        CHECK_AND_PRINT_LOG(CollectFolderStats(current.first, current.second, result.mediaFiles, result.mediaStats),
            "CollectFolderStats failed, path: %{public}s", MediaFileUtils::DesensitizePath(current.first).c_str());

        for (const auto &subDir : result.subDirs) {
            dirStack.push_back(subDir);
        }
    }

    MarkTraversalDone();
    return true;
}

static std::string AnonymizePath(const std::string &path)
{
    static const std::regex ipRegex(R"((\d{1,3})\.\d{1,3}\.\d{1,3}\.\d{1,3})");
    static const std::regex idCardRegex(
        R"((\d{6})(?:19|20)\d{2}(?:0[1-9]|1[0-2])(?:0[1-9]|[12]\d|3[01])(\d{3}[\dXx]))");
    static const std::regex addressRegex(
        R"([^/]*(小区|花园|广场|中心|大厦|单元|省|市|区|县|镇|乡|村|路|街|道|巷|号|栋|楼|室|层)[^/]*)");
    std::string result = path;
    result = std::regex_replace(result, ipRegex, ANONYMIZED_IP_FORMAT);
    result = std::regex_replace(result, idCardRegex, ANONYMIZED_ID_CARD_FORMAT);
    result = std::regex_replace(result, addressRegex, ANONYMIZED_ADDRESS_FORMAT);
    return result;
}

static nlohmann::json BuildBatchJson(const std::vector<DocsScanFolderStats> &batch)
{
    nlohmann::json batchArray = nlohmann::json::array();
    for (auto &stats : batch) {
        nlohmann::json folderObj;
        folderObj[JSON_KEY_DIR_PATH] = AnonymizePath(stats.dirPath);
        folderObj[JSON_KEY_IMAGE_COUNT] = stats.imageCount;
        folderObj[JSON_KEY_VIDEO_COUNT] = stats.videoCount;
        folderObj[JSON_KEY_ATIME_WITHIN_30MIN] = stats.atimeWithin30min == 1;
        folderObj[JSON_KEY_ATIME_DIFF_SEC] = stats.atimeDiffSec;
        nlohmann::json formatJson = nlohmann::json::parse(stats.formatDistribution, nullptr, false);
        CHECK_AND_EXECUTE(formatJson.is_discarded(), folderObj[JSON_KEY_FORMAT_DISTRIBUTION] = formatJson);
        nlohmann::json sizeJson = nlohmann::json::parse(stats.sizeDistribution, nullptr, false);
        CHECK_AND_EXECUTE(sizeJson.is_discarded(), folderObj[JSON_KEY_SIZE_DISTRIBUTION] = sizeJson);
        batchArray.push_back(folderObj);
    }
    return batchArray;
}

void DocsMediaScanManager::ReportPhase()
{
    auto prefs = GetPrefs();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");

    ResetDailyCountIfNeeded();

    MEDIA_INFO_LOG("Start ReportPhase");
    int32_t maxId = 0;
    int32_t ret = DfxDatabaseUtils::QueryDocsScanMaxId(maxId);
    CHECK_AND_RETURN_LOG(ret == E_OK, "QueryDocsScanMaxId failed");

    int32_t lastReportedId = prefs->GetInt(DOCS_MEDIA_SCAN_LAST_REPORTED_ID, 0);
    MEDIA_INFO_LOG("Get lastReportedId / maxId: %{public}d / %{public}d", lastReportedId, maxId);
    if (lastReportedId >= maxId) {
        Finalize();
        return;
    }

    while (lastReportedId < maxId) {
        if (CheckDailyLimit()) {
            MEDIA_INFO_LOG("Daily limit reached, will continue next trigger");
            return;
        }
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Background status off, pausing report phase");
            return;
        }

        std::vector<DocsScanFolderStats> batch = DfxDatabaseUtils::QueryDocsScanFolderStats(lastReportedId,
            DOCS_MEDIA_SCAN_BATCH_SIZE);
        CHECK_AND_RETURN_LOG(!batch.empty(), "QueryDocsScanFolderStats failed");

        nlohmann::json batchArray = BuildBatchJson(batch);
        int32_t reportRet = DfxReporter::ReportAncoCountFormatInfoForDirScan(batchArray.dump());
        if (reportRet != 0) {
            MEDIA_ERR_LOG("Report failed, will retry next trigger");
            return;
        }

        lastReportedId = batch.back().id;
        IncrementDailyCountAndUpdateLastReportedId(lastReportedId);
    }

    if (lastReportedId >= maxId) {
        Finalize();
    }
}

void DocsMediaScanManager::Finalize()
{
    CHECK_AND_RETURN(DfxDatabaseUtils::DropDocsMediaScanTempTable() == E_OK);
    MarkTaskCompleted();
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
    MEDIA_INFO_LOG("Start Execute");
    CHECK_AND_RETURN(IsTraversalDone() || TraverseAndCollect());
    ReportPhase();
}

} // namespace Media
} // namespace OHOS