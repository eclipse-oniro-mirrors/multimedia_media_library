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

#include "reverse_clone_kvstore_executor.h"

#include <array>
#include <map>

#include "backup_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore.h"
#include "medialibrary_kvstore_manager.h"

namespace OHOS::Media {
struct ReverseCloneKvStoreExecuteStats {
    int32_t totalTasks {0};
    int32_t validTasks {0};
    int32_t invalidTasks {0};
    int32_t successTasks {0};
    int32_t failedTasks {0};
    int32_t copiedMonth {0};
    int32_t copiedYear {0};
    int32_t overwrittenMonth {0};
    int32_t overwrittenYear {0};
    int32_t keptMonth {0};
    int32_t keptYear {0};
    int32_t missingMonth {0};
    int32_t missingYear {0};
    int32_t failedMonth {0};
    int32_t failedYear {0};
    std::map<std::string, std::string> samples;
};

namespace {
struct AstcKeys {
    std::string oldKey;
    std::string newKey;
};

struct KvStorePair {
    std::shared_ptr<MediaLibraryKvStore> oldStore;
    std::shared_ptr<MediaLibraryKvStore> newStore;
    const char *name;
};

enum class AstcCopyStatus {
    COPIED = 0,
    OVERWRITTEN,
    KEPT,
    MISSING,
    FAILED,
    COUNT,
};

// LCOV_EXCL_START
const char *ToString(ReverseCloneKvStoreWriteMode writeMode)
{
    return writeMode == ReverseCloneKvStoreWriteMode::FILL_IF_MISSING ? "fill_if_missing" : "overwrite";
}

int32_t &GetStoreCounter(const KvStorePair &stores, AstcCopyStatus status, ReverseCloneKvStoreExecuteStats &stats)
{
    bool isMonth = std::string(stores.name) == "month";
    using Counter = int32_t ReverseCloneKvStoreExecuteStats::*;
    static const std::array<std::array<Counter, 2>, static_cast<size_t>(AstcCopyStatus::COUNT)> counters = {{
        {&ReverseCloneKvStoreExecuteStats::copiedMonth, &ReverseCloneKvStoreExecuteStats::copiedYear},
        {&ReverseCloneKvStoreExecuteStats::overwrittenMonth, &ReverseCloneKvStoreExecuteStats::overwrittenYear},
        {&ReverseCloneKvStoreExecuteStats::keptMonth, &ReverseCloneKvStoreExecuteStats::keptYear},
        {&ReverseCloneKvStoreExecuteStats::missingMonth, &ReverseCloneKvStoreExecuteStats::missingYear},
        {&ReverseCloneKvStoreExecuteStats::failedMonth, &ReverseCloneKvStoreExecuteStats::failedYear},
    }};
    return stats.*(counters[static_cast<size_t>(status)][isMonth ? 0 : 1]);
}

std::string BuildSampleName(const KvStorePair &stores, AstcCopyStatus status)
{
    static const std::array<const char *, static_cast<size_t>(AstcCopyStatus::COUNT)> statusNames = {
        "copied", "overwritten", "kept", "missing", "failed"};
    return std::string(stores.name) + "_" + statusNames[static_cast<size_t>(status)];
}

bool BuildAstcKeys(const ReverseCloneKvStoreTask &task, AstcKeys &keys)
{
    return MediaFileUtils::GenerateKvStoreKey(std::to_string(task.oldFileId), task.oldDateKey, keys.oldKey) &&
        MediaFileUtils::GenerateKvStoreKey(std::to_string(task.newFileId), task.newDateKey, keys.newKey);
}

void UpdateStoreStats(const KvStorePair &stores, AstcCopyStatus status, const AstcKeys &keys,
    ReverseCloneKvStoreExecuteStats &stats)
{
    GetStoreCounter(stores, status, stats)++;
    std::string sampleName = BuildSampleName(stores, status);
    if (!sampleName.empty() && stats.samples.find(sampleName) == stats.samples.end()) {
        stats.samples[sampleName] = status == AstcCopyStatus::MISSING ? keys.oldKey : keys.newKey;
    }
}

AstcCopyStatus CopyAstcValue(const KvStorePair &stores, const AstcKeys &keys,
    ReverseCloneKvStoreWriteMode writeMode)
{
    std::vector<uint8_t> value;
    int32_t ret = stores.oldStore->Query(keys.oldKey, value);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("RevRes source missing, store=%{public}s, oldKey=%{public}s, ret=%{public}d",
            stores.name, keys.oldKey.c_str(), ret);
        return AstcCopyStatus::MISSING;
    }

    std::vector<uint8_t> ignoredTargetValue;
    bool targetExists = stores.newStore->Query(keys.newKey, ignoredTargetValue) == E_OK;
    if (targetExists && writeMode == ReverseCloneKvStoreWriteMode::FILL_IF_MISSING) {
        MEDIA_INFO_LOG("RevRes Reverse clone kvstore target kept, store=%{public}s, oldKey=%{public}s, "
            "newKey=%{public}s", stores.name, keys.oldKey.c_str(), keys.newKey.c_str());
        return AstcCopyStatus::KEPT;
    }

    ret = stores.newStore->Insert(keys.newKey, value);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Reverse clone insert %{public}s astc failed, newKey:%{public}s, ret:%{public}d",
            stores.name, keys.newKey.c_str(), ret);
        return AstcCopyStatus::FAILED;
    }
    MEDIA_INFO_LOG("RevRes Reverse clone kvstore target %{public}s, mode=%{public}s, store=%{public}s, "
        "oldKey=%{public}s, newKey=%{public}s, bytes=%{public}zu", targetExists ? "overwritten" : "copied",
        ToString(writeMode), stores.name, keys.oldKey.c_str(), keys.newKey.c_str(), value.size());
    return targetExists ? AstcCopyStatus::OVERWRITTEN : AstcCopyStatus::COPIED;
}

void CloseKvStore(std::shared_ptr<MediaLibraryKvStore> &kvStore)
{
    if (kvStore == nullptr) {
        return;
    }
    kvStore->Close();
    kvStore.reset();
}

std::string GetSample(const ReverseCloneKvStoreExecuteStats &stats, const std::string &sampleName)
{
    auto iter = stats.samples.find(sampleName);
    return iter == stats.samples.end() ? "" : iter->second;
}
} // namespace

bool ReverseCloneKvStoreTask::IsValid() const
{
    return oldFileId > 0 && newFileId > 0 && !oldDateKey.empty() && !newDateKey.empty();
}

ReverseCloneKvStoreExecutor::~ReverseCloneKvStoreExecutor()
{
    Close();
}

bool ReverseCloneKvStoreExecutor::Init(const std::string &backupRoot)
{
    CHECK_AND_RETURN_RET_LOG(!backupRoot.empty(), false, "Reverse clone kvstore backupRoot is empty");
    std::string oldBaseDir = backupRoot + CLONE_KVDB_BACKUP_DIR;
    std::string newBaseDir = MEDIA_KVDB_DIR;
    MEDIA_INFO_LOG("RevRes Reverse clone kvstore init, oldBaseDir=%{public}s, newBaseDir=%{public}s",
        oldBaseDir.c_str(), newBaseDir.c_str());

    oldMonthKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(KvStoreRoleType::OWNER,
        CLONE_KVSTORE_MONTH_STOREID, oldBaseDir);
    oldYearKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(KvStoreRoleType::OWNER,
        CLONE_KVSTORE_YEAR_STOREID, oldBaseDir);
    newMonthKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(KvStoreRoleType::OWNER,
        MEDIA_KVSTORE_MONTH_STOREID, newBaseDir);
    newYearKvStorePtr_ = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(KvStoreRoleType::OWNER,
        MEDIA_KVSTORE_YEAR_STOREID, newBaseDir);

    bool isReady = IsReady();
    if (!isReady) {
        Close();
        MEDIA_ERR_LOG("Reverse clone init kvstore failed");
        return false;
    }
    return true;
}

void ReverseCloneKvStoreExecutor::Close()
{
    CloseKvStore(oldMonthKvStorePtr_);
    CloseKvStore(oldYearKvStorePtr_);
    CloseKvStore(newMonthKvStorePtr_);
    CloseKvStore(newYearKvStorePtr_);
}

int32_t ReverseCloneKvStoreExecutor::Execute(const std::vector<ReverseCloneKvStoreTask> &tasks)
{
    if (tasks.empty()) {
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(IsReady(), E_FAIL, "Reverse clone kvstore is not ready");

    ReverseCloneKvStoreExecuteStats stats;
    stats.totalTasks = static_cast<int32_t>(tasks.size());
    for (const auto &task : tasks) {
        if (!task.IsValid()) {
            MEDIA_WARN_LOG("Reverse clone skip invalid kvstore task, oldFileId:%{public}d, newFileId:%{public}d",
                task.oldFileId, task.newFileId);
            stats.invalidTasks++;
            stats.failedTasks++;
            continue;
        }

        stats.validTasks++;
        int32_t ret = ExecuteOne(task, stats);
        if (ret != E_OK) {
            stats.failedTasks++;
            continue;
        }
        stats.successTasks++;
    }

    MEDIA_INFO_LOG("RevRes Reverse clone kvstore execute finished, totalTasks=%{public}d, validTasks=%{public}d, "
        "successTasks=%{public}d, failedTasks=%{public}d, invalidTasks=%{public}d, "
        "copied(month/year)=%{public}d/%{public}d, overwritten(month/year)=%{public}d/%{public}d, "
        "kept(month/year)=%{public}d/%{public}d, "
        "missing(month/year)=%{public}d/%{public}d, failedStore(month/year)=%{public}d/%{public}d, "
        "sampleCopied(month/year)=%{public}s/%{public}s, sampleOverwritten(month/year)=%{public}s/%{public}s, "
        "sampleKept(month/year)=%{public}s/%{public}s, sampleMissing(month/year)=%{public}s/%{public}s, "
        "sampleFailed(month/year)=%{public}s/%{public}s",
        stats.totalTasks, stats.validTasks, stats.successTasks, stats.failedTasks, stats.invalidTasks,
        stats.copiedMonth, stats.copiedYear, stats.overwrittenMonth, stats.overwrittenYear,
        stats.keptMonth, stats.keptYear, stats.missingMonth, stats.missingYear, stats.failedMonth,
        stats.failedYear,
        GetSample(stats, "month_copied").c_str(), GetSample(stats, "year_copied").c_str(),
        GetSample(stats, "month_overwritten").c_str(), GetSample(stats, "year_overwritten").c_str(),
        GetSample(stats, "month_kept").c_str(), GetSample(stats, "year_kept").c_str(),
        GetSample(stats, "month_missing").c_str(), GetSample(stats, "year_missing").c_str(),
        GetSample(stats, "month_failed").c_str(), GetSample(stats, "year_failed").c_str());
    return stats.failedTasks == 0 ? E_OK : E_FAIL;
}

int32_t ReverseCloneKvStoreExecutor::ExecuteOne(const ReverseCloneKvStoreTask &task,
    ReverseCloneKvStoreExecuteStats &stats) const
{
    AstcKeys keys;
    CHECK_AND_RETURN_RET_LOG(BuildAstcKeys(task, keys), E_FAIL,
        "Reverse clone build kvstore key failed, oldFileId:%{public}d, newFileId:%{public}d",
        task.oldFileId, task.newFileId);

    const std::array<KvStorePair, 2> stores = {{
        {oldMonthKvStorePtr_, newMonthKvStorePtr_, "month"},
        {oldYearKvStorePtr_, newYearKvStorePtr_, "year"},
    }};
    for (const auto &store : stores) {
        AstcCopyStatus status = CopyAstcValue(store, keys, task.writeMode);
        UpdateStoreStats(store, status, keys, stats);
        CHECK_AND_RETURN_RET(status != AstcCopyStatus::FAILED, E_FAIL);
    }
    return E_OK;
}

bool ReverseCloneKvStoreExecutor::IsReady() const
{
    return oldMonthKvStorePtr_ != nullptr && oldYearKvStorePtr_ != nullptr &&
        newMonthKvStorePtr_ != nullptr && newYearKvStorePtr_ != nullptr;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
