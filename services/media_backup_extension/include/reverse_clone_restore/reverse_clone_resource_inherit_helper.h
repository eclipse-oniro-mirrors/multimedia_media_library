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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_HELPER_H
#define OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_HELPER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "backup_const.h"
#include "media_log.h"
#include "rdb_store.h"
#include "reverse_clone_kvstore_executor.h"
#include "reverse_clone_resource_inherit_service.h"
#include "reverse_clone_resource_plan.h"

namespace OHOS::Media {
class TransactionOperations;

struct ReverseStaleTargetResource {
    int32_t fileId {0};
    std::string cloudPath;
    bool origin {false};
    bool movingPhotoVideo {false};
    bool editData {false};
    bool thumbnail {false};
};

struct ReverseClonePhotoBatchContext {
    std::vector<FileInfo> validFileInfos;
    std::unordered_map<int32_t, ReverseCloneResourcePlan> resourcePlans;
    std::vector<ReverseCloneResourcePlan> duplicatePlans;
    std::vector<ReverseCloneKvStoreTask> kvStoreTasks;
    std::vector<NativeRdb::ValuesBucket> values;
    std::vector<ReverseStaleTargetResource> staleTargetResources;
    std::unordered_set<int32_t> pathPrepareFailedFileIds;
    bool cloudRestoreSatisfied {false};
};

class ReverseCloneResourceInheritHelper {
public:
    void Reset();
    void AddFileIdOffsetRule(int64_t oldMaxFileId, int64_t newMaxFileId);
    void SnapshotPureCloudFileIds(const std::shared_ptr<NativeRdb::RdbStore> &rdb);
    std::vector<ReverseCloneKvStoreTask> BuildRetainedOldPhotoKvStoreTasks(
        const std::shared_ptr<NativeRdb::RdbStore> &rdb) const;
    void FinalizeBatch(const ReverseClonePhotoBatchContext &batch,
        std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        ReverseRestoreReportInfo &reportInfo);
    bool CommitPhotosBatch(ReverseClonePhotoBatchContext &batch,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, int64_t &insertedRows);
    void ForceAbsorbSourceResourcesOnCommitFailed(const ReverseClonePhotoBatchContext &batch) const;
    void ReleaseDuplicateDonorReservations(const ReverseClonePhotoBatchContext &batch);
    void AppendKvStoreTasks(const std::vector<ReverseCloneKvStoreTask> &tasks);
    void ExecuteKvStoreTasks(const std::vector<ReverseCloneKvStoreTask> &tasks, const std::string &backupRoot);
    void ExecutePendingKvStoreTasks(const std::string &backupRoot);

    template<typename AlbumAssetAbsorb>
    bool PrepareBatch(std::vector<FileInfo> &fileInfos, int32_t maxDestDbFileId, int32_t minDestDbFileId,
        std::shared_ptr<NativeRdb::RdbStore> &destRdb, AlbumAssetAbsorb &albumAssetAbsorb,
        ReverseClonePhotoBatchContext &batch, typename AlbumAssetAbsorb::DuplicateCount &duplicateCount)
    {
        if (batch.values.empty()) {
            MEDIA_WARN_LOG("RevRes Reverse absorb prepare batch skipped: empty values, queried=%{public}zu",
                fileInfos.size());
            return false;
        }

        batch.validFileInfos = CollectValidFileInfos(fileInfos, batch.resourcePlans);
        if (batch.validFileInfos.empty()) {
            MEDIA_WARN_LOG("RevRes Reverse absorb prepare batch skipped: empty valid file infos, queried=%{public}zu, "
                "values=%{public}zu, resourcePlans=%{public}zu",
                fileInfos.size(), batch.values.size(), batch.resourcePlans.size());
            return false;
        }
        LogDuplicateCheckInputs(batch.validFileInfos, maxDestDbFileId);
        // Resolve duplicate donor plans outside the lock; actual donor deletion happens before batch insert.
        albumAssetAbsorb.CheckAndRemoveDuplicatePhotos(destRdb, batch.validFileInfos, maxDestDbFileId, minDestDbFileId,
            batch.duplicatePlans, originalPureCloudFileIds_, duplicateCount);
        LogDuplicateCheckResults(batch.validFileInfos, batch.duplicatePlans, "after_check");
        ReserveDuplicateDonors(batch);
        LogDuplicateCheckResults(batch.validFileInfos, batch.duplicatePlans, "after_reserve");

        ReverseCloneResourceInheritService resourceInheritService;
        MarkCloudRestoreSatisfied(batch);
        resourceInheritService.MergeDuplicatePlansWithSourceFallback(batch.duplicatePlans, batch.resourcePlans);
        batch.kvStoreTasks = BuildDuplicateKvStoreTasks(batch.duplicatePlans);
        MEDIA_INFO_LOG("RevRes Reverse absorb prepare batch: queried=%{public}zu, valid=%{public}zu, "
            "values=%{public}zu, resourcePlans=%{public}zu, duplicatePlans=%{public}zu, kvTasks=%{public}zu",
            fileInfos.size(), batch.validFileInfos.size(), batch.values.size(), batch.resourcePlans.size(),
            batch.duplicatePlans.size(), batch.kvStoreTasks.size());
        return true;
    }

private:
    struct AssetReferenceUpdate {
        int32_t donorFileId {0};
        int32_t absorbedFileId {0};
        int32_t protectedFileId {0};

        bool NeedsRestore() const;
    };

    struct FileIdOffsetRule {
        int64_t offset {0};
        int64_t sourceMaxExtended {0};

        bool IsValid() const;
        bool Match(int64_t fileId) const;
        int64_t ToOriginalOldDeviceFileId(int64_t fileId) const;
    };

    static int64_t GetExtendedFileIdBoundary(int64_t newMaxFileId);
    static int32_t GetProtectedReferenceFileId(int32_t fileId);
    static ReverseCloneKvStoreTask BuildKvStoreTask(int32_t oldFileId, const std::string &oldDateKey,
        int32_t newFileId, const std::string &newDateKey, ReverseCloneKvStoreWriteMode writeMode);

    int64_t ToOriginalOldDeviceFileId(int64_t oldDeviceFinalFileId) const;
    std::vector<AssetReferenceUpdate> BuildAssetReferenceUpdates(const ReverseClonePhotoBatchContext &batch) const;
    int32_t MoveAssetReferencesBeforePhotoDelete(TransactionOperations &trans,
        const std::vector<AssetReferenceUpdate> &updates) const;
    int32_t RestoreProtectedAssetReferences(TransactionOperations &trans,
        const std::vector<AssetReferenceUpdate> &updates) const;
    int32_t UpdateAssetReferences(TransactionOperations &trans, int32_t fromFileId, int32_t toFileId) const;
    int32_t DeleteProtectedAssetReferences(TransactionOperations &trans, int32_t protectedFileId) const;
    int32_t DeleteDuplicateDonorsInTransaction(TransactionOperations &trans,
        const std::vector<FileInfo> &fileInfos, std::vector<int32_t> &deletedDonorFileIds) const;
    int32_t InsertAbsorbedPhotosInTransaction(TransactionOperations &trans, const ReverseClonePhotoBatchContext &batch,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, int64_t &insertedRows) const;
    bool CommitPhotosInTransaction(ReverseClonePhotoBatchContext &batch,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, int64_t &insertedRows,
        std::vector<int32_t> &deletedDonorFileIds);
    void LogDuplicateCheckInputs(const std::vector<FileInfo> &fileInfos, int32_t maxDestDbFileId) const;
    void LogDuplicateCheckResults(const std::vector<FileInfo> &fileInfos,
        const std::vector<ReverseCloneResourcePlan> &duplicatePlans, const char *stage) const;
    void LogDataConflictsBeforeInsert(const ReverseClonePhotoBatchContext &batch,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb) const;
    void ReserveDuplicateDonors(ReverseClonePhotoBatchContext &batch);
    void ReleaseDuplicateDonors(const std::vector<int32_t> &deletedDonorFileIds);
    void DeletePhotoExtRows(const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        const std::vector<int32_t> &deletedDonorFileIds) const;
    void MarkCloudRestoreSatisfied(ReverseClonePhotoBatchContext &batch) const;
    void ClearDuplicateDonor(std::vector<FileInfo> &fileInfos, int32_t absorbedFileId, int32_t donorFileId) const;
    std::vector<ReverseCloneKvStoreTask> BuildDuplicateKvStoreTasks(
        const std::vector<ReverseCloneResourcePlan> &duplicatePlans) const;
    std::unordered_set<int32_t> ExecuteResourcePlans(
        const std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        ReverseRestoreReportInfo &reportInfo) const;
    std::unordered_set<int32_t> ExecuteStaleTargetFallback(
        const std::vector<ReverseStaleTargetResource> &staleTargetResources) const;
    void UpdateAbsorbedPhotosVisible(std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        const std::vector<FileInfo> &fileInfos, const std::unordered_set<int32_t> &failedResourceFileIds) const;
    std::vector<FileInfo> CollectValidFileInfos(const std::vector<FileInfo> &fileInfos,
        const std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans) const;

    std::vector<FileIdOffsetRule> fileIdOffsetRules_;
    std::unordered_set<int32_t> originalPureCloudFileIds_;
    std::unordered_set<int32_t> reservedDuplicateDonorFileIds_;
    std::vector<ReverseCloneKvStoreTask> pendingKvStoreTasks_;
    std::mutex duplicateMutex_;
    std::mutex kvStoreMutex_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_HELPER_H
