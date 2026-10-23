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

#include "reverse_clone_restore/reverse_clone_kvstore_absorber.h"

#include <array>

#include "backup_const.h"
#include "backup_database_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore.h"
#include "medialibrary_kvstore_manager.h"
#include "result_set_utils.h"
#include "reverse_clone_restore/reverse_clone_resource_plan.h"

namespace OHOS {
namespace Media {

ReverseCloneKvStoreAbsorber::~ReverseCloneKvStoreAbsorber()
{
    Close();
}

ReverseCloneKvStoreAbsorbResult ReverseCloneKvStoreAbsorber::Init()
{
    masterBaseDir_ = MEDIA_KVDB_DIR;
    tempBaseDir_ = REVERSE_KVDB_TEMP_DIR;
    monthStores_.name = "month";
    yearStores_.name = "year";

    monthStores_.master = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(
        KvStoreRoleType::OWNER, MEDIA_KVSTORE_MONTH_STOREID, masterBaseDir_);
    if (monthStores_.master == nullptr) {
        MEDIA_ERR_LOG("Absorber: month master kvstore init failed");
    }
    monthStores_.temp = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(
        KvStoreRoleType::OWNER, CLONE_KVSTORE_MONTH_STOREID, tempBaseDir_);
    if (monthStores_.temp == nullptr) {
        MEDIA_ERR_LOG("Absorber: month temp kvstore init failed");
    }
    yearStores_.master = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(
        KvStoreRoleType::OWNER, MEDIA_KVSTORE_YEAR_STOREID, masterBaseDir_);
    if (yearStores_.master == nullptr) {
        MEDIA_ERR_LOG("Absorber: year master kvstore init failed");
    }
    yearStores_.temp = MediaLibraryKvStoreManager::GetInstance().GetSingleKvStore(
        KvStoreRoleType::OWNER, CLONE_KVSTORE_YEAR_STOREID, tempBaseDir_);
    if (yearStores_.temp == nullptr) {
        MEDIA_ERR_LOG("Absorber: year temp kvstore init failed");
    }

    bool masterCorrupted = monthStores_.master == nullptr || yearStores_.master == nullptr;
    bool tempCorrupted = monthStores_.temp == nullptr || yearStores_.temp == nullptr;

    if (!masterCorrupted && !tempCorrupted) {
        MEDIA_INFO_LOG("Absorber: init success, tempBaseDir=%{public}s",
            MediaFileUtils::DesensitizePath(tempBaseDir_).c_str());
        return ReverseCloneKvStoreAbsorbResult::SUCCESS;
    }
    if (masterCorrupted) {
        DeleteCorruptedStore(MEDIA_KVSTORE_MONTH_STOREID, masterBaseDir_);
        DeleteCorruptedStore(MEDIA_KVSTORE_YEAR_STOREID, masterBaseDir_);
    }
    if (tempCorrupted) {
        DeleteCorruptedStore(CLONE_KVSTORE_MONTH_STOREID, tempBaseDir_);
        DeleteCorruptedStore(CLONE_KVSTORE_YEAR_STOREID, tempBaseDir_);
    }
    if (masterCorrupted && tempCorrupted) {
        return ReverseCloneKvStoreAbsorbResult::BOTH_CORRUPTED;
    }
    return masterCorrupted ? ReverseCloneKvStoreAbsorbResult::MASTER_CORRUPTED
                           : ReverseCloneKvStoreAbsorbResult::TEMP_CORRUPTED;
}

void ReverseCloneKvStoreAbsorber::DeleteCorruptedStore(const std::string &storeId, const std::string &baseDir)
{
    int32_t ret = MediaLibraryKvStore().DeleteKvStoreByStoreId(storeId, baseDir);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: delete store failed, storeId=%{public}s, ret=%{public}d",
            storeId.c_str(), ret);
    } else {
        MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: deleted store, storeId=%{public}s", storeId.c_str());
    }
}

void ReverseCloneKvStoreAbsorber::Close()
{
    auto close = [](std::shared_ptr<MediaLibraryKvStore> &ptr) {
        if (ptr != nullptr) {
            ptr->Close();
            ptr.reset();
        }
    };
    close(monthStores_.master);
    close(monthStores_.temp);
    close(yearStores_.master);
    close(yearStores_.temp);
}

namespace {
enum class AbsorbCopyStatus {
    COPIED = 0,
    KEPT,
    MISSING,
    FAILED,
    COUNT,
};

int32_t &GetStoreCounter(const char *name, AbsorbCopyStatus status, ReverseCloneKvStoreAbsorbStats &stats)
{
    bool isMonth = std::string(name) == "month";
    using Counter = int32_t ReverseCloneKvStoreAbsorbStats::*;
    static const std::array<std::array<Counter, 2>, static_cast<size_t>(AbsorbCopyStatus::COUNT)> counters = {{
        { &ReverseCloneKvStoreAbsorbStats::copiedMonth, &ReverseCloneKvStoreAbsorbStats::copiedYear },
        { &ReverseCloneKvStoreAbsorbStats::keptMonth, &ReverseCloneKvStoreAbsorbStats::keptYear },
        { &ReverseCloneKvStoreAbsorbStats::missingMonth, &ReverseCloneKvStoreAbsorbStats::missingYear },
        { &ReverseCloneKvStoreAbsorbStats::failedMonth, &ReverseCloneKvStoreAbsorbStats::failedYear }
    }};
    return stats.*(counters[static_cast<size_t>(status)][isMonth ? 0 : 1]);
}

void LogAbsorbStats(const ReverseCloneKvStoreAbsorbStats &stats)
{
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: copied month=%{public}d year=%{public}d, "
        "kept month=%{public}d year=%{public}d, missing month=%{public}d year=%{public}d, "
        "failed month=%{public}d year=%{public}d, deleted month=%{public}d year=%{public}d, "
        "skipped=%{public}d, copyStoreSuccess=%{public}d, copyStoreFailed=%{public}d",
        stats.copiedMonth, stats.copiedYear, stats.keptMonth, stats.keptYear,
        stats.missingMonth, stats.missingYear, stats.failedMonth, stats.failedYear,
        stats.deletedMonth, stats.deletedYear, stats.skipped,
        stats.copyStoreSuccess, stats.copyStoreFailed);
}
} // namespace


void ReverseCloneKvStoreAbsorber::AbsorbRetainedPhoto(int32_t currentFileId, int32_t originalFileId,
    int64_t dateTaken, ReverseCloneKvStoreAbsorbStats &stats)
{
    std::string dateKey = std::to_string(dateTaken);
    if (originalFileId == currentFileId) {
        stats.skipped++;
        return;
    }
    std::string oldKey;
    std::string newKey;
    if (!MediaFileUtils::GenerateKvStoreKey(std::to_string(originalFileId), dateKey, oldKey) ||
        !MediaFileUtils::GenerateKvStoreKey(std::to_string(currentFileId), dateKey, newKey)) {
        MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: GenerateKvStoreKey failed, fileId=%{public}d", currentFileId);
        stats.skipped++;
        return;
    }
    StorePair pairs[] = { monthStores_, yearStores_ };
    for (auto &pair : pairs) {
        if (pair.master == nullptr) {
            continue;
        }
        std::vector<uint8_t> value;
        if (pair.master->Query(oldKey, value) != E_OK) {
            GetStoreCounter(pair.name, AbsorbCopyStatus::MISSING, stats)++;
            continue;
        }
        std::vector<uint8_t> targetValue;
        if (pair.master->Query(newKey, targetValue) == E_OK) {
            GetStoreCounter(pair.name, AbsorbCopyStatus::KEPT, stats)++;
            continue;
        }
        if (pair.master->Insert(newKey, value) != E_OK) {
            MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: Insert failed, skip delete, fileId=%{public}d",
                currentFileId);
            GetStoreCounter(pair.name, AbsorbCopyStatus::FAILED, stats)++;
            continue;
        }
        if (pair.master->Delete(oldKey) != E_OK) {
            MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: Delete oldKey failed, store=%{public}s, key=%{public}s",
                pair.name, oldKey.c_str());
            GetStoreCounter(pair.name, AbsorbCopyStatus::FAILED, stats)++;
            continue;
        }
        GetStoreCounter(pair.name, AbsorbCopyStatus::COPIED, stats)++;
    }
}

// 旧机偏移数据
int32_t ReverseCloneKvStoreAbsorber::AbsorbRetainedPhotos(
    const std::shared_ptr<NativeRdb::RdbStore> &destRdb, ReverseCloneKvStoreAbsorbStats &stats)
{
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: AbsorbRetainedPhotos start");
    if (destRdb == nullptr) {
        MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: destRdb is null");
        return E_FAIL;
    }
    std::string querySql = "SELECT P.file_id, P.date_taken, "
        "COALESCE(C.old_file_id, P.file_id) AS original_file_id FROM " + PhotoColumn::PHOTOS_TABLE + " AS P "
        "LEFT JOIN tab_cloned_old_photos AS C ON C.file_id = P.file_id "
        "WHERE P." + MediaColumn::MEDIA_TIME_PENDING + " = 0 AND P." +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND P." + MediaColumn::MEDIA_DATE_TRASHED + " = 0";

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, querySql, {});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL,
        "ReverseCloneKvStoreAbsorber: AbsorbRetainedPhotos query failed");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AbsorbRetainedPhoto(GetInt32Val(MediaColumn::MEDIA_ID, resultSet),
            GetInt32Val("original_file_id", resultSet),
            GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet), stats);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: AbsorbRetainedPhotos end, copied month=%{public}d year=%{public}d, "
        "kept month=%{public}d year=%{public}d, missing month=%{public}d year=%{public}d, "
        "failed month=%{public}d year=%{public}d, skipped=%{public}d",
        stats.copiedMonth, stats.copiedYear, stats.keptMonth, stats.keptYear,
        stats.missingMonth, stats.missingYear, stats.failedMonth, stats.failedYear, stats.skipped);
    return E_OK;
}

int32_t ReverseCloneKvStoreAbsorber::AbsorbDuplicatePhotos(const std::vector<ReverseCloneResourcePlan> &plans,
    ReverseCloneKvStoreAbsorbStats &stats)
{
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: AbsorbDuplicatePhotos start, plans size=%{public}zu", plans.size());
    for (const auto &plan : plans) {
        if (plan.matchType == ReverseCloneMatchType::SAME_CLOUD_CONFLICT) {
            stats.skipped++;
            continue;
        }
        if (plan.donor.fileId <= 0 || plan.donor.dateTaken <= 0) {
            stats.skipped++;
            continue;
        }
        int32_t originalOldFileId = plan.donor.originalFileId > 0
            ? plan.donor.originalFileId : plan.donor.fileId;
        std::string deleteKey;
        if (!MediaFileUtils::GenerateKvStoreKey(std::to_string(originalOldFileId),
            std::to_string(plan.donor.dateTaken), deleteKey)) {
            MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: generate deleteKey failed, donorId=%{public}d",
                originalOldFileId);
            stats.skipped++;
            continue;
        }
        DeleteDonorKeyFromStores(deleteKey, stats);
    }
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: AbsorbDuplicatePhotos end, deleted month=%{public}d year=%{public}d, "
        "skipped=%{public}d", stats.deletedMonth, stats.deletedYear, stats.skipped);
    return E_OK;
}

void ReverseCloneKvStoreAbsorber::DeleteDonorKeyFromStores(const std::string &deleteKey,
    ReverseCloneKvStoreAbsorbStats &stats)
{
    StorePair pairs[] = { monthStores_, yearStores_ };
    for (auto &pair : pairs) {
        if (pair.master == nullptr) {
            continue;
        }
        int32_t delRet = pair.master->Delete(deleteKey);
        if (delRet != E_OK) {
            MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: delete donor key failed, store=%{public}s, "
                "key=%{public}s, ret=%{public}d", pair.name, deleteKey.c_str(), delRet);
            stats.skipped++;
            continue;
        }
        if (strcmp(pair.name, "month") == 0) {
            stats.deletedMonth++;
        } else {
            stats.deletedYear++;
        }
    }
}

int32_t ReverseCloneKvStoreAbsorber::CopyAllFromTempToMaster(ReverseCloneKvStoreAbsorbStats &stats)
{
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: CopyAllFromTempToMaster start");
    StorePair pairs[] = { monthStores_, yearStores_ };
    for (auto &pair : pairs) {
        if (pair.master == nullptr) {
            stats.copyStoreFailed++;
            continue;
        }
        if (pair.temp == nullptr) {
            MEDIA_WARN_LOG("ReverseCloneKvStoreAbsorber: CopyAllFromTempToMaster temp %{public}s is null, skip",
                pair.name);
            stats.copyStoreFailed++;
            continue;
        }
        int32_t ret = pair.temp->PutAllValueToNewKvStore(pair.master);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: CopyAllFromTempToMaster %{public}s failed, ret=%{public}d",
                pair.name, ret);
            stats.copyStoreFailed++;
        } else {
            stats.copyStoreSuccess++;
        }
    }
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: CopyAllFromTempToMaster completed, "
        "success=%{public}d, failed=%{public}d", stats.copyStoreSuccess, stats.copyStoreFailed);
    return stats.copyStoreFailed == 0 ? E_OK : E_FAIL;
}


bool ReverseCloneKvStoreAbsorber::Absorb(
    const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const std::vector<ReverseCloneResourcePlan> &duplicatePlans)
{
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: start");
    ReverseCloneKvStoreAbsorbStats stats;
    int32_t ret = AbsorbRetainedPhotos(destRdb, stats);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: AbsorbRetainedPhotos failed, ret=%{public}d", ret);
        Close();
        return false;
    }
    ret = AbsorbDuplicatePhotos(duplicatePlans, stats);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: Absorb failed at AbsorbDuplicatePhotos, ret=%{public}d", ret);
        Close();
        return false;
    }
    ret = CopyAllFromTempToMaster(stats);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneKvStoreAbsorber: Absorb failed at CopyAllFromTempToMaster, ret=%{public}d", ret);
        Close();
        return false;
    }
    Close();
    LogAbsorbStats(stats);
    MEDIA_INFO_LOG("ReverseCloneKvStoreAbsorber: completed");
    return true;
}

} // namespace Media
} // namespace OHOS
