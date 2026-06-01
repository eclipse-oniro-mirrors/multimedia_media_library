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

#define MLOG_TAG "Media_Background"

#include "media_clone_pending_record_utils.h"

#include <algorithm>
#include <charconv>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"

namespace OHOS::Media::Background {
namespace {
const std::string CLONE_PENDING_EVENT = "/data/storage/el2/base/preferences/clone_pending_events.xml";
const std::string CLONE_PENDING_BUCKET_PREFIX = "clone_pending_bucket_";
constexpr int32_t CLONE_PENDING_MULTI_BUCKET_COUNT = 16;
constexpr int64_t CLONE_PENDING_TOUCH_FLUSH_INTERVAL_MS = 30000;
std::mutex g_clonePendingMutex;
std::unordered_map<int32_t, int64_t> g_lastTouchPersistMs;

std::shared_ptr<NativePreferences::Preferences> GetClonePendingPrefs()
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(CLONE_PENDING_EVENT, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, nullptr,
        "Get clone pending preferences failed, errCode=%{public}d", errCode);
    return prefs;
}

using BucketPendingEntries = std::map<int32_t, int64_t>;

std::string BuildBucketKeyInternal(int32_t bucketIndex)
{
    return CLONE_PENDING_BUCKET_PREFIX + std::to_string(bucketIndex);
}

int32_t GetBucketIndexInternal(int32_t fileId)
{
    if (fileId <= 0) {
        return 0;
    }
    return fileId % CLONE_PENDING_MULTI_BUCKET_COUNT;
}

bool ParseInt32(const std::string &text, int32_t &value)
{
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

bool ParseInt64(const std::string &text, int64_t &value)
{
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

BucketPendingEntries ParsePendingEntriesInternal(const std::string &csv)
{
    BucketPendingEntries entries;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            continue;
        }
        int32_t fileId = -1;
        int64_t touchTs = 0;
        size_t pos = token.find('_');
        if (pos == std::string::npos) {
            if (!ParseInt32(token, fileId) || fileId <= 0) {
                continue;
            }
            entries[fileId] = 0;
            continue;
        }

        std::string fileIdText = token.substr(0, pos);
        std::string touchTsText = token.substr(pos + 1);
        if (!ParseInt32(fileIdText, fileId) || fileId <= 0 || !ParseInt64(touchTsText, touchTs)) {
            continue;
        }
        entries[fileId] = touchTs;
    }
    return entries;
}

std::string SerializePendingEntriesInternal(const BucketPendingEntries &entries)
{
    std::string csv;
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it != entries.begin()) {
            csv += ",";
        }
        csv += std::to_string(it->first) + "_" + std::to_string(it->second);
    }
    return csv;
}

BucketPendingEntries LoadBucketPendingEntries(const std::shared_ptr<NativePreferences::Preferences> &prefs,
    int32_t bucketIndex)
{
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, {}, "prefs is null");
    return ParsePendingEntriesInternal(prefs->GetString(BuildBucketKeyInternal(bucketIndex), ""));
}

void SaveBucketPendingEntries(const std::shared_ptr<NativePreferences::Preferences> &prefs, int32_t bucketIndex,
    const BucketPendingEntries &entries)
{
    CHECK_AND_RETURN(prefs != nullptr);
    prefs->PutString(BuildBucketKeyInternal(bucketIndex), SerializePendingEntriesInternal(entries));
}

// 更新活跃时间戳
bool PersistTouchLocked(const std::shared_ptr<NativePreferences::Preferences> &prefs, int32_t fileId,
    bool forcePersist, bool flush)
{
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is null");
    int32_t bucketIndex = GetBucketIndexInternal(fileId);
    BucketPendingEntries entries = LoadBucketPendingEntries(prefs, bucketIndex);
    auto entryIt = entries.find(fileId);
    if (entryIt == entries.end()) {
        MEDIA_DEBUG_LOG("Skip persist touch, fileId=%{public}d not in pending buckets", fileId);
        return true;
    }

    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t lastPersistTs = entryIt->second;
    auto cacheIt = g_lastTouchPersistMs.find(fileId);
    if (cacheIt != g_lastTouchPersistMs.end()) {
        lastPersistTs = cacheIt->second;
    }
    if (!forcePersist && lastPersistTs > 0 && (now - lastPersistTs) < CLONE_PENDING_TOUCH_FLUSH_INTERVAL_MS) {
        MEDIA_DEBUG_LOG("Skip persist touch by throttle, fileId=%{public}d, elapsed=%{public}" PRId64,
            fileId, now - lastPersistTs);
        return true;
    }
    entryIt->second = now;
    SaveBucketPendingEntries(prefs, bucketIndex, entries);
    if (flush) {
        prefs->FlushSync();
    }
    g_lastTouchPersistMs[fileId] = now;
    MEDIA_DEBUG_LOG("Persist touch success, fileId=%{public}d, ts=%{public}" PRId64, fileId, now);
    return true;
}
} // namespace

bool ClonePendingRecordUtils::AddPendingFileId(int32_t fileId)
{
    if (fileId <= 0) {
        return true;
    }
    std::lock_guard<std::mutex> lock(g_clonePendingMutex);
    auto prefs = GetClonePendingPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is null");

    int32_t bucketIndex = GetBucketIndexInternal(fileId);
    BucketPendingEntries bucketEntries = LoadBucketPendingEntries(prefs, bucketIndex);
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    auto [_, inserted] = bucketEntries.insert({ fileId, now });
    if (!inserted) {
        bucketEntries[fileId] = now;
    }
    SaveBucketPendingEntries(prefs, bucketIndex, bucketEntries);
    if (inserted) {
        MEDIA_INFO_LOG("Add pending fileId=%{public}d to bucket=%{public}d", fileId, bucketIndex);
    } else {
        MEDIA_DEBUG_LOG("Pending fileId already exists, fileId=%{public}d, bucket=%{public}d", fileId, bucketIndex);
    }
    g_lastTouchPersistMs[fileId] = now;
    prefs->FlushSync();
    return true;
}

bool ClonePendingRecordUtils::UpdatePendingFileTouch(int32_t fileId, bool forcePersist)
{
    if (fileId <= 0) {
        return true;
    }
    std::lock_guard<std::mutex> lock(g_clonePendingMutex);
    if (!forcePersist) {
        auto it = g_lastTouchPersistMs.find(fileId);
        if (it != g_lastTouchPersistMs.end()) {
            int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
            if ((now - it->second) < CLONE_PENDING_TOUCH_FLUSH_INTERVAL_MS) {
                MEDIA_DEBUG_LOG("Skip touch update by memory throttle, fileId=%{public}d, elapsed=%{public}" PRId64,
                    fileId, now - it->second);
                return true;
            }
        }
    }
    auto prefs = GetClonePendingPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is null");
    return PersistTouchLocked(prefs, fileId, forcePersist, true);
}

bool ClonePendingRecordUtils::IsPendingFileTouchExpired(int32_t fileId, int64_t timeoutMs)
{
    if (fileId <= 0) {
        return true;
    }
    std::lock_guard<std::mutex> lock(g_clonePendingMutex);
    auto prefs = GetClonePendingPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, true, "prefs is null");
    int32_t bucketIndex = GetBucketIndexInternal(fileId);
    BucketPendingEntries entries = LoadBucketPendingEntries(prefs, bucketIndex);
    auto entryIt = entries.find(fileId);
    int64_t touchTs = (entryIt != entries.end()) ? entryIt->second : 0;
    if (touchTs <= 0) {
        MEDIA_INFO_LOG("Touch timestamp missing, treat as expired, fileId=%{public}d", fileId);
        return true;
    }
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    bool expired = (now - touchTs) >= timeoutMs;
    MEDIA_DEBUG_LOG("Touch expire check, fileId=%{public}d, touchTs=%{public}" PRId64
        ", now=%{public}" PRId64 ", timeout=%{public}" PRId64 ", expired=%{public}d",
        fileId, touchTs, now, timeoutMs, expired);
    return expired;
}

bool ClonePendingRecordUtils::RemovePendingFileId(int32_t fileId)
{
    return RemovePendingFileIds({ fileId });
}

bool ClonePendingRecordUtils::RemovePendingFileIds(const std::vector<int32_t> &pendingFileIds)
{
    if (pendingFileIds.empty()) {
        return true;
    }
    std::lock_guard<std::mutex> lock(g_clonePendingMutex);
    auto prefs = GetClonePendingPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is null");

    std::unordered_map<int32_t, BucketPendingEntries> dirtyBuckets;
    bool bucketChanged = false;
    for (const auto fileId : pendingFileIds) {
        if (fileId <= 0) {
            continue;
        }
        int32_t bucketIndex = GetBucketIndexInternal(fileId);
        auto [it, ignored] = dirtyBuckets.try_emplace(bucketIndex, LoadBucketPendingEntries(prefs, bucketIndex));
        (void)ignored;
        CHECK_AND_CONTINUE(it != dirtyBuckets.end());
        if (it->second.erase(fileId) > 0) {
            bucketChanged = true;
        }
        g_lastTouchPersistMs.erase(fileId);
    }
    if (!bucketChanged) {
        MEDIA_DEBUG_LOG("RemovePendingFileIds no-op, no matched fileIds");
        return true;
    }
    for (auto &[bucketIndex, fileIds] : dirtyBuckets) {
        SaveBucketPendingEntries(prefs, bucketIndex, fileIds);
    }
    prefs->FlushSync();
    MEDIA_INFO_LOG("RemovePendingFileIds success, requested=%{public}zu, bucketTouched=%{public}zu",
        pendingFileIds.size(), dirtyBuckets.size());
    return true;
}

int32_t ClonePendingRecordUtils::GetPendingBucketCount()
{
    return CLONE_PENDING_MULTI_BUCKET_COUNT;
}

std::vector<int32_t> ClonePendingRecordUtils::GetPendingFileIdsByBucket(int32_t bucketIndex)
{
    std::lock_guard<std::mutex> lock(g_clonePendingMutex);
    auto prefs = GetClonePendingPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, {}, "prefs is null");
    if (bucketIndex < 0 || bucketIndex >= CLONE_PENDING_MULTI_BUCKET_COUNT) {
        return {};
    }
    BucketPendingEntries entries = LoadBucketPendingEntries(prefs, bucketIndex);
    std::vector<int32_t> fileIds;
    fileIds.reserve(entries.size());
    for (const auto &[fileId, touchTs] : entries) {
        (void)touchTs;
        fileIds.push_back(fileId);
    }
    return fileIds;
}

QueryClonePendingStatus ClonePendingRecordUtils::QueryClonePendingInfo(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t fileId, ClonePendingInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, QueryClonePendingStatus::QUERY_ERROR, "rdbStore is nullptr");
    const std::string querySql =
        "SELECT file_id, data, date_taken, subtype, time_pending FROM Photos WHERE file_id = " +
        std::to_string(fileId);
    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, QueryClonePendingStatus::QUERY_ERROR,
        "Query pending clone record failed");
    if (resultSet->GoToFirstRow() != E_OK) {
        resultSet->Close();
        return QueryClonePendingStatus::NOT_FOUND;
    }

    info.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    info.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    info.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
    info.subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    info.timePending = GetInt64Val(MediaColumn::MEDIA_TIME_PENDING, resultSet);
    resultSet->Close();
    return QueryClonePendingStatus::FOUND;
}

bool ClonePendingRecordUtils::CleanupPendingAssetByFileId(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    int32_t fileId)
{
    ClonePendingInfo assetInfo;
    auto queryStatus = QueryClonePendingInfo(rdbStore, fileId, assetInfo);
    if (queryStatus == QueryClonePendingStatus::NOT_FOUND) {
        return true;
    }
    CHECK_AND_RETURN_RET_LOG(queryStatus == QueryClonePendingStatus::FOUND, false,
        "Query clone pending asset failed, fileId=%{public}d", fileId);
    if (assetInfo.timePending != -1) {
        return true;
    }

    MEDIA_INFO_LOG("Clone pending cleanup begin, fileId=%{public}d, path=%{public}s",
        fileId, MediaFileUtils::DesensitizePath(assetInfo.filePath).c_str());
    return CleanupPendingAsset(rdbStore, assetInfo);
}

bool ClonePendingRecordUtils::CleanupPendingAsset(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const ClonePendingInfo &info)
{
    std::vector<std::string> ids = { std::to_string(info.fileId) };
    std::vector<std::string> paths = { info.filePath };
    std::vector<std::string> dateTakens = { std::to_string(info.dateTaken) };
    std::vector<int32_t> subTypes = { info.subType };
    MediaLibraryAssetOperations::TaskDataFileProcess(ids, paths, PhotoColumn::PHOTOS_TABLE, dateTakens, subTypes);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(info.fileId));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, std::to_string(-1));
    int32_t deleteRows = 0;
    int32_t ret = rdbStore->Delete(deleteRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "Delete pending clone record failed, fileId=%{public}d, ret=%{public}d", info.fileId, ret);
    return true;
}
} // namespace OHOS::Media::Background