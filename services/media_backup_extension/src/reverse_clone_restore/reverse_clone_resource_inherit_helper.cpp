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

#include "reverse_clone_resource_inherit_helper.h"

#include <cerrno>
#include <cstdio>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_type_const.h"
#include "moving_photo_file_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"

namespace OHOS::Media {
namespace {
const std::string LAKE_STORAGE_ROOT = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const std::string REVERSE_RESTORE_RESOURCE_ROOT = "/storage/media/local/files/reverse_restore";
const std::string FORCE_ABSORB_BACKUP_ROOT = REVERSE_RESTORE_RESOURCE_ROOT + "/.force_absorb_bak";

bool HasMovingPhotoVideo(const ReverseCloneAssetResource &resource)
{
    return resource.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        resource.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

bool HasLocalOriginPosition(const ReverseCloneAssetResource &resource)
{
    return resource.position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
        resource.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
}

bool IsPathUnderDirectory(const std::string &path, const std::string &directory)
{
    if (directory.empty() || path.length() <= directory.length() ||
        path.compare(0, directory.length(), directory) != 0) {
        return false;
    }
    return directory.back() == '/' || path[directory.length()] == '/';
}

bool HasExistingLakeOrigin(const ReverseCloneAssetResource &resource)
{
    return resource.IsLakeAsset() && IsPathUnderDirectory(resource.storagePath, LAKE_STORAGE_ROOT) &&
        MediaFileUtils::IsFileExists(resource.storagePath);
}

bool IsCloudResourcePath(const std::string &path)
{
    return IsPathUnderDirectory(path, RESTORE_FILES_CLOUD_DIR);
}

std::string GetLocalPathByCloudPath(const std::string &cloudPath)
{
    return IsCloudResourcePath(cloudPath) ?
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, cloudPath) : "";
}

std::string GetEditDataPathByCloudPath(const std::string &cloudPath)
{
    return IsCloudResourcePath(cloudPath) ?
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL_EDIT_DATA, cloudPath) : "";
}

std::string GetSourceOriginPath(const ReverseCloneAssetResource &source)
{
    if (!source.originPath.empty()) {
        return source.originPath;
    }
    return GetLocalPathByCloudPath(source.cloudPath);
}

std::string GetSourceEditDataPath(const ReverseCloneAssetResource &source)
{
    if (!source.localRoot.empty() && !source.relativePath.empty()) {
        std::string root = source.localRoot.back() == '/' ? source.localRoot.substr(0, source.localRoot.length() - 1) :
            source.localRoot;
        return root + "/.editData" + source.relativePath;
    }
    return GetEditDataPathByCloudPath(source.cloudPath);
}

bool StrictRenameExistingResource(const std::string &src, const std::string &dst)
{
    CHECK_AND_RETURN_RET_LOG(!dst.empty(), false,
        "Reverse force absorb source resource skipped, src=%{public}s, dst is empty",
        MediaFileUtils::DesensitizePath(src).c_str());
    int32_t errCode = BackupFileUtils::PreparePath(dst);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "Reverse force absorb prepare path failed, src=%{public}s, dst=%{public}s, errCode=%{public}d",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str(), errCode);
    errno = 0;
    CHECK_AND_RETURN_RET_LOG(std::rename(src.c_str(), dst.c_str()) == 0, false,
        "Reverse force absorb strict rename failed, src=%{public}s, dst=%{public}s, errno=%{public}d",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str(), errno);
    return true;
}

bool IsValidRequiredRename(const std::string &src, const std::string &dst)
{
    CHECK_AND_RETURN_RET_LOG(!src.empty() && MediaFileUtils::IsFileExists(src), false,
        "Reverse force absorb required source missing, src=%{public}s, dst=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str());
    CHECK_AND_RETURN_RET_LOG(!dst.empty(), false, "Reverse force absorb required target invalid, src=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str());
    return true;
}

std::string GetForceAbsorbBackupPath(const std::string &path)
{
    if (!IsPathUnderDirectory(path, RESTORE_FILES_LOCAL_DIR)) {
        return "";
    }
    return FORCE_ABSORB_BACKUP_ROOT + "/" + path.substr(RESTORE_FILES_LOCAL_DIR.length());
}

bool StrictRenameDirectoryWithBackup(const std::string &src, const std::string &dst)
{
    CHECK_AND_RETURN_RET_LOG(!dst.empty(), false, "Reverse force absorb editData target is empty, src=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str());
    std::string backup = GetForceAbsorbBackupPath(dst);
    CHECK_AND_RETURN_RET_LOG(!backup.empty(), false,
        "Reverse force absorb editData backup path invalid, dst=%{public}s",
        MediaFileUtils::DesensitizePath(dst).c_str());
    CHECK_AND_RETURN_RET_LOG(!MediaFileUtils::IsFileExists(backup) || MediaFileUtils::DeleteDir(backup), false,
        "Reverse force absorb delete stale editData backup failed, backup=%{public}s",
        MediaFileUtils::DesensitizePath(backup).c_str());
    if (!MediaFileUtils::IsFileExists(dst)) {
        return StrictRenameExistingResource(src, dst);
    }
    CHECK_AND_RETURN_RET(StrictRenameExistingResource(dst, backup), false);
    if (StrictRenameExistingResource(src, dst)) {
        CHECK_AND_WARN_LOG(MediaFileUtils::DeleteDir(backup),
            "Reverse force absorb editData succeeded but backup cleanup failed, backup=%{public}s",
            MediaFileUtils::DesensitizePath(backup).c_str());
        return true;
    }
    bool restored = StrictRenameExistingResource(backup, dst);
    CHECK_AND_PRINT_LOG(restored, "Reverse force absorb restore editData backup failed, backup=%{public}s, "
        "dst=%{public}s", MediaFileUtils::DesensitizePath(backup).c_str(),
        MediaFileUtils::DesensitizePath(dst).c_str());
    return false;
}

bool ForceAbsorbEditDataResource(const std::string &srcEditData, const std::string &dstEditData)
{
    if (srcEditData.empty() || !MediaFileUtils::IsFileExists(srcEditData)) {
        return true;
    }
    return MediaFileUtils::IsDirectory(srcEditData) ?
        StrictRenameDirectoryWithBackup(srcEditData, dstEditData) :
        StrictRenameExistingResource(srcEditData, dstEditData);
}

bool ForceAbsorbOriginResources(const ReverseCloneAssetResource &source, const std::string &srcOrigin,
    const std::string &dstOrigin)
{
    if (HasExistingLakeOrigin(source)) {
        return true;
    }
    bool hasVideo = HasMovingPhotoVideo(source);
    std::string srcVideo = hasVideo ? MediaFileUtils::GetMovingPhotoVideoPath(srcOrigin) : "";
    std::string dstVideo = hasVideo ? MediaFileUtils::GetMovingPhotoVideoPath(dstOrigin) : "";
    if (!IsValidRequiredRename(srcOrigin, dstOrigin) ||
        (hasVideo && !IsValidRequiredRename(srcVideo, dstVideo))) {
        return false;
    }
    if (!StrictRenameExistingResource(srcOrigin, dstOrigin)) {
        return false;
    }
    if (!hasVideo) {
        return true;
    }
    if (StrictRenameExistingResource(srcVideo, dstVideo)) {
        return true;
    }
    MEDIA_ERR_LOG("Reverse force absorb moving photo video failed after origin replaced, srcOrigin=%{public}s, "
        "dstOrigin=%{public}s",
        MediaFileUtils::DesensitizePath(srcOrigin).c_str(), MediaFileUtils::DesensitizePath(dstOrigin).c_str());
    return false;
}

const ReverseCloneAssetResource *GetSourceResourceForCommitFailure(const ReverseCloneResourcePlan &plan)
{
    if (plan.hasFallbackSource && plan.fallbackSource.HasResourcePath()) {
        return &plan.fallbackSource;
    }
    if (plan.matchType == ReverseCloneMatchType::SOURCE_ASSET && plan.donor.HasResourcePath()) {
        return &plan.donor;
    }
    return nullptr;
}

bool ForceAbsorbSourceResourceOnCommitFailed(const ReverseCloneResourcePlan &plan, int32_t &successCount,
    int32_t &failedCount)
{
    const ReverseCloneAssetResource *source = GetSourceResourceForCommitFailure(plan);
    if (source == nullptr || !HasLocalOriginPosition(*source)) {
        return false;
    }
    const ReverseCloneAssetResource &sourceResource = *source;

    std::string srcOrigin = GetSourceOriginPath(sourceResource);
    std::string dstOrigin = GetLocalPathByCloudPath(plan.absorbed.cloudPath);
    bool originSatisfied = ForceAbsorbOriginResources(sourceResource, srcOrigin, dstOrigin);

    std::string srcEditData = GetSourceEditDataPath(sourceResource);
    std::string dstEditData = GetEditDataPathByCloudPath(plan.absorbed.cloudPath);
    bool editDataSatisfied = ForceAbsorbEditDataResource(srcEditData, dstEditData);

    bool success = originSatisfied && editDataSatisfied;
    successCount += success ? 1 : 0;
    failedCount += success ? 0 : 1;
    CHECK_AND_EXECUTE(success,
        MEDIA_ERR_LOG("Reverse force absorb source resources after db commit failed, fileId=%{public}d, "
        "matchType=%{public}d, "
        "keepLakeOrigin=%{public}d, src=%{public}s, dst=%{public}s, editSrc=%{public}s, editDst=%{public}s, "
        "originSatisfied=%{public}d, editDataSatisfied=%{public}d",
        plan.absorbed.fileId, static_cast<int32_t>(plan.matchType), HasExistingLakeOrigin(sourceResource),
        MediaFileUtils::DesensitizePath(srcOrigin).c_str(), MediaFileUtils::DesensitizePath(dstOrigin).c_str(),
        MediaFileUtils::DesensitizePath(srcEditData).c_str(), MediaFileUtils::DesensitizePath(dstEditData).c_str(),
        originSatisfied, editDataSatisfied));
    return true;
}

std::string GetLocalThumbPathByCloudPath(const std::string &cloudPath)
{
    if (cloudPath.compare(0, RESTORE_FILES_CLOUD_DIR.length(), RESTORE_FILES_CLOUD_DIR) != 0) {
        return "";
    }
    return RESTORE_FILES_LOCAL_DIR + ".thumbs/" + cloudPath.substr(RESTORE_FILES_CLOUD_DIR.length());
}

std::string GetMovingPhotoVideoPathByCloudPath(const std::string &cloudPath)
{
    return MediaFileUtils::GetMovingPhotoVideoPath(GetLocalPathByCloudPath(cloudPath));
}

bool DeletePathIfExists(const std::string &path)
{
    if (path.empty() || !MediaFileUtils::IsFileExists(path)) {
        return true;
    }
    bool isFile = !MediaFileUtils::IsDirectory(path);
    bool deleted = MediaFileUtils::DeleteFileOrFolder(path, isFile);
    CHECK_AND_PRINT_LOG(deleted, "RevRes stale target fallback delete failed, path=%{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    return deleted;
}
} // namespace

// LCOV_EXCL_START
bool ReverseCloneResourceInheritHelper::AssetReferenceUpdate::NeedsRestore() const
{
    return protectedFileId != absorbedFileId;
}

bool ReverseCloneResourceInheritHelper::FileIdOffsetRule::IsValid() const
{
    return offset > 0 && sourceMaxExtended > 0;
}

bool ReverseCloneResourceInheritHelper::FileIdOffsetRule::Match(int64_t fileId) const
{
    return IsValid() && fileId > offset && fileId <= sourceMaxExtended + offset;
}

int64_t ReverseCloneResourceInheritHelper::FileIdOffsetRule::ToOriginalOldDeviceFileId(int64_t fileId) const
{
    return Match(fileId) ? fileId - offset : fileId;
}

int64_t ReverseCloneResourceInheritHelper::GetExtendedFileIdBoundary(int64_t newMaxFileId)
{
    int64_t offset = newMaxFileId * 10 / 100;
    if (offset < 1000) {
        offset = 1000;
    }
    if (offset > 10000) {
        offset = 10000;
    }
    return newMaxFileId + offset;
}

int32_t ReverseCloneResourceInheritHelper::GetProtectedReferenceFileId(int32_t fileId)
{
    return -fileId;
}

void ReverseCloneResourceInheritHelper::Reset()
{
    fileIdOffsetRules_.clear();
    originalPureCloudFileIds_.clear();
    reservedDuplicateDonorFileIds_.clear();
    {
        std::lock_guard<std::mutex> lock(kvStoreMutex_);
        pendingKvStoreTasks_.clear();
    }
}

void ReverseCloneResourceInheritHelper::AddFileIdOffsetRule(int64_t oldMaxFileId, int64_t newMaxFileId)
{
    FileIdOffsetRule rule {
        .offset = oldMaxFileId,
        .sourceMaxExtended = GetExtendedFileIdBoundary(newMaxFileId),
    };
    if (!rule.IsValid()) {
        return;
    }
    fileIdOffsetRules_.emplace_back(rule);
    MEDIA_INFO_LOG("Reverse clone add file id offset rule, offset=%{public}lld, sourceMaxExtended=%{public}lld",
        static_cast<long long>(rule.offset), static_cast<long long>(rule.sourceMaxExtended));
}

int64_t ReverseCloneResourceInheritHelper::ToOriginalOldDeviceFileId(int64_t oldDeviceFinalFileId) const
{
    int64_t fileId = oldDeviceFinalFileId;
    for (auto it = fileIdOffsetRules_.rbegin(); it != fileIdOffsetRules_.rend(); ++it) {
        fileId = it->ToOriginalOldDeviceFileId(fileId);
    }
    return fileId;
}

void ReverseCloneResourceInheritHelper::LogDuplicateCheckInputs(
    const std::vector<FileInfo> &fileInfos, int32_t maxDestDbFileId) const
{
    MEDIA_INFO_LOG("Reverse duplicate check input, count=%{public}zu, maxDestDbFileId=%{public}d",
        fileInfos.size(), maxDestDbFileId);
}

void ReverseCloneResourceInheritHelper::LogDuplicateCheckResults(
    const std::vector<FileInfo> &fileInfos, const std::vector<ReverseCloneResourcePlan> &duplicatePlans,
    const char *stage) const
{
    int32_t duplicateCount = 0;
    for (const auto &fileInfo : fileInfos) {
        if (fileInfo.deletedSrcdbFileId <= 0) {
            continue;
        }
        duplicateCount++;
    }
    MEDIA_INFO_LOG("Reverse duplicate check result finish, stage=%{public}s, duplicateCount=%{public}d, "
        "duplicatePlans=%{public}zu", stage, duplicateCount, duplicatePlans.size());
}

void ReverseCloneResourceInheritHelper::LogDataConflictsBeforeInsert(
    const ReverseClonePhotoBatchContext &batch, const std::shared_ptr<NativeRdb::RdbStore> &targetRdb) const
{
    CHECK_AND_RETURN(targetRdb != nullptr);
    for (const auto &fileInfo : batch.validFileInfos) {
        if (fileInfo.cloudPath.empty()) {
            continue;
        }
        const std::string sql = "SELECT file_id, display_name, size, storage_path, file_source_type "
            "FROM Photos WHERE data = ?";
        auto resultSet = BackupDatabaseUtils::QuerySql(targetRdb, sql, {fileInfo.cloudPath});
        CHECK_AND_CONTINUE(resultSet != nullptr);
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t conflictFileId = 0;
            int64_t conflictSize = 0;
            int32_t conflictFileSourceType = 0;
            std::string conflictDisplayName;
            std::string conflictStoragePath;
            resultSet->GetInt(0, conflictFileId);
            resultSet->GetString(1, conflictDisplayName);
            resultSet->GetLong(2, conflictSize);
            resultSet->GetString(3, conflictStoragePath);
            resultSet->GetInt(4, conflictFileSourceType);
            MEDIA_INFO_LOG("Reverse absorb data conflict before insert, absorbedFileId=%{public}d, "
                "conflictFileId=%{public}d, originalConflictFileId=%{public}lld, data=%{public}s, "
                "absorbedName=%{public}s, conflictName=%{public}s, absorbedSize=%{public}lld, "
                "conflictSize=%{public}lld, conflictStoragePath=%{public}s, conflictFileSourceType=%{public}d",
                fileInfo.fileIdOld, conflictFileId,
                static_cast<long long>(ToOriginalOldDeviceFileId(conflictFileId)),
                MediaFileUtils::DesensitizePath(fileInfo.cloudPath).c_str(),
                fileInfo.displayName.c_str(), conflictDisplayName.c_str(), static_cast<long long>(fileInfo.fileSize),
                static_cast<long long>(conflictSize),
                MediaFileUtils::DesensitizePath(conflictStoragePath).c_str(), conflictFileSourceType);
        }
        resultSet->Close();
    }
}

void ReverseCloneResourceInheritHelper::ClearDuplicateDonor(std::vector<FileInfo> &fileInfos,
    int32_t absorbedFileId, int32_t donorFileId) const
{
    for (auto &fileInfo : fileInfos) {
        if (fileInfo.fileIdOld == absorbedFileId && fileInfo.deletedSrcdbFileId == donorFileId) {
            fileInfo.deletedSrcdbFileId = 0;
            return;
        }
    }
}

void ReverseCloneResourceInheritHelper::ReserveDuplicateDonors(ReverseClonePhotoBatchContext &batch)
{
    std::lock_guard<std::mutex> lock(duplicateMutex_);
    std::vector<ReverseCloneResourcePlan> reservedPlans;
    int32_t reservedCount = 0;
    int32_t skippedCount = 0;
    for (const auto &plan : batch.duplicatePlans) {
        int32_t donorFileId = plan.donor.fileId;
        int32_t absorbedFileId = plan.absorbed.fileId;
        if (donorFileId <= 0 || absorbedFileId <= 0) {
            skippedCount++;
            continue;
        }
        if (reservedDuplicateDonorFileIds_.find(donorFileId) != reservedDuplicateDonorFileIds_.end()) {
            ClearDuplicateDonor(batch.validFileInfos, absorbedFileId, donorFileId);
            skippedCount++;
            MEDIA_WARN_LOG("ReserveDuplicateDonors: donor already reserved, donorFileId=%{public}d, "
                "absorbedFileId=%{public}d", donorFileId, absorbedFileId);
            continue;
        }
        reservedDuplicateDonorFileIds_.emplace(donorFileId);
        reservedPlans.emplace_back(plan);
        reservedCount++;
    }
    batch.duplicatePlans.swap(reservedPlans);
    MEDIA_INFO_LOG("ReserveDuplicateDonors: reserved=%{public}d, skipped=%{public}d, totalReserved=%{public}zu",
        reservedCount, skippedCount, reservedDuplicateDonorFileIds_.size());
}

void ReverseCloneResourceInheritHelper::MarkCloudRestoreSatisfied(ReverseClonePhotoBatchContext &batch) const
{
    for (auto &entry : batch.resourcePlans) {
        entry.second.cloudRestoreSatisfied = batch.cloudRestoreSatisfied;
    }
    for (auto &plan : batch.duplicatePlans) {
        plan.cloudRestoreSatisfied = batch.cloudRestoreSatisfied;
    }
}

void ReverseCloneResourceInheritHelper::ReleaseDuplicateDonorReservations(
    const ReverseClonePhotoBatchContext &batch)
{
    std::lock_guard<std::mutex> lock(duplicateMutex_);
    int32_t releasedCount = 0;
    for (const auto &plan : batch.duplicatePlans) {
        if (plan.donor.fileId <= 0) {
            continue;
        }
        releasedCount += reservedDuplicateDonorFileIds_.erase(plan.donor.fileId) > 0 ? 1 : 0;
    }
    MEDIA_INFO_LOG("ReleaseDuplicateDonorReservations: released=%{public}d, remaining=%{public}zu",
        releasedCount, reservedDuplicateDonorFileIds_.size());
}

ReverseCloneKvStoreTask ReverseCloneResourceInheritHelper::BuildKvStoreTask(int32_t oldFileId,
    const std::string &oldDateKey, int32_t newFileId, const std::string &newDateKey,
    ReverseCloneKvStoreWriteMode writeMode)
{
    ReverseCloneKvStoreTask task;
    task.oldFileId = oldFileId;
    task.oldDateKey = oldDateKey;
    task.newFileId = newFileId;
    task.newDateKey = newDateKey;
    task.writeMode = writeMode;
    return task;
}

std::vector<ReverseCloneKvStoreTask> ReverseCloneResourceInheritHelper::BuildDuplicateKvStoreTasks(
    const std::vector<ReverseCloneResourcePlan> &duplicatePlans) const
{
    std::vector<ReverseCloneKvStoreTask> tasks;
    int32_t skippedConflict = 0;
    int32_t skippedInvalid = 0;
    ReverseCloneKvStoreTask sampleTask;
    for (const auto &plan : duplicatePlans) {
        if (plan.matchType == ReverseCloneMatchType::SAME_CLOUD_CONFLICT) {
            skippedConflict++;
            continue;
        }
        if (plan.donor.fileId <= 0 || plan.absorbed.fileId <= 0 ||
            plan.donor.dateTaken <= 0 || plan.absorbed.dateTaken <= 0) {
            skippedInvalid++;
            continue;
        }

        int32_t originalOldFileId = plan.donor.originalFileId > 0 ? plan.donor.originalFileId : plan.donor.fileId;
        tasks.emplace_back(BuildKvStoreTask(originalOldFileId, std::to_string(plan.donor.dateTaken),
            plan.absorbed.fileId, std::to_string(plan.absorbed.dateTaken),
            ReverseCloneKvStoreWriteMode::FILL_IF_MISSING));
        if (!sampleTask.IsValid()) {
            sampleTask = tasks.back();
        }
    }
    MEDIA_INFO_LOG("RevRes BuildDuplicateKvStoreTasks: duplicatePlans=%{public}zu, tasks=%{public}zu, "
        "skippedConflict=%{public}d, skippedInvalid=%{public}d, sample=%{public}d:%{public}s->%{public}d:%{public}s",
        duplicatePlans.size(), tasks.size(), skippedConflict, skippedInvalid,
        sampleTask.oldFileId, sampleTask.oldDateKey.c_str(), sampleTask.newFileId, sampleTask.newDateKey.c_str());
    return tasks;
}

std::vector<ReverseCloneKvStoreTask> ReverseCloneResourceInheritHelper::BuildRetainedOldPhotoKvStoreTasks(
    const std::shared_ptr<NativeRdb::RdbStore> &rdb) const
{
    std::vector<ReverseCloneKvStoreTask> tasks;
    CHECK_AND_RETURN_RET_LOG(rdb != nullptr, tasks, "RevRes BuildRetainedOldPhotoKvStoreTasks: rdb is null");

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::string querySql = "SELECT P.file_id, P.date_taken, "
        "COALESCE(C.old_file_id, P.file_id) AS original_file_id FROM " + PhotoColumn::PHOTOS_TABLE + " AS P "
        "LEFT JOIN tab_cloned_old_photos AS C ON C.file_id = P.file_id "
        "WHERE P." + MediaColumn::MEDIA_TIME_PENDING + " = 0 AND P." +
        PhotoColumn::PHOTO_IS_TEMP + " = 0 AND P." + MediaColumn::MEDIA_DATE_TRASHED + " = 0";
    auto resultSet = BackupDatabaseUtils::QuerySql(rdb, querySql, {});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, tasks, "RevRes BuildRetainedOldPhotoKvStoreTasks: query failed");
    int32_t scannedRows = 0;
    ReverseCloneKvStoreTask sampleTask;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        scannedRows++;
        int32_t newFileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        int32_t oldFileId = GetInt32Val("original_file_id", resultSet);
        int64_t dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        std::string dateKey = std::to_string(dateTaken);
        tasks.emplace_back(BuildKvStoreTask(oldFileId, dateKey, newFileId, dateKey,
            ReverseCloneKvStoreWriteMode::OVERWRITE));
        if (!sampleTask.IsValid()) {
            sampleTask = tasks.back();
        }
    }
    resultSet->Close();
    int64_t cost = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("RevRes BuildRetainedOldPhotoKvStoreTasks: scannedRows=%{public}d, tasks=%{public}zu, "
        "sample=%{public}d:%{public}s->%{public}d:%{public}s, cost=%{public}lld",
        scannedRows, tasks.size(), sampleTask.oldFileId, sampleTask.oldDateKey.c_str(),
        sampleTask.newFileId, sampleTask.newDateKey.c_str(), static_cast<long long>(cost));
    return tasks;
}

void ReverseCloneResourceInheritHelper::ExecuteKvStoreTasks(const std::vector<ReverseCloneKvStoreTask> &tasks,
    const std::string &backupRoot)
{
    if (tasks.empty()) {
        MEDIA_INFO_LOG("RevRes ExecuteKvStoreTasks: no kvstore tasks");
        return;
    }
    std::lock_guard<std::mutex> lock(kvStoreMutex_);

    ReverseCloneKvStoreExecutor executor;
    CHECK_AND_RETURN_LOG(executor.Init(backupRoot), "RevRes ExecuteKvStoreTasks: init kvstore failed");
    MEDIA_INFO_LOG("RevRes ExecuteKvStoreTasks: start, tasks=%{public}zu", tasks.size());
    int32_t ret = executor.Execute(tasks);
    CHECK_AND_RETURN_LOG(ret == E_OK, "RevRes ExecuteKvStoreTasks: execute failed, ret=%{public}d", ret);
    MEDIA_INFO_LOG("RevRes ExecuteKvStoreTasks: completed, tasks=%{public}zu", tasks.size());
}

void ReverseCloneResourceInheritHelper::AppendKvStoreTasks(const std::vector<ReverseCloneKvStoreTask> &tasks)
{
    if (tasks.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(kvStoreMutex_);
    pendingKvStoreTasks_.insert(pendingKvStoreTasks_.end(), tasks.begin(), tasks.end());
    MEDIA_INFO_LOG("RevRes AppendKvStoreTasks: appended=%{public}zu, pending=%{public}zu",
        tasks.size(), pendingKvStoreTasks_.size());
}

void ReverseCloneResourceInheritHelper::ExecutePendingKvStoreTasks(const std::string &backupRoot)
{
    std::vector<ReverseCloneKvStoreTask> tasks;
    {
        std::lock_guard<std::mutex> lock(kvStoreMutex_);
        tasks.swap(pendingKvStoreTasks_);
    }
    MEDIA_INFO_LOG("RevRes ExecutePendingKvStoreTasks: totalTasks=%{public}zu", tasks.size());
    ExecuteKvStoreTasks(tasks, backupRoot);
}

std::unordered_set<int32_t> ReverseCloneResourceInheritHelper::ExecuteResourcePlans(
    const std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
    ReverseRestoreReportInfo &reportInfo) const
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::unordered_set<int32_t> failedResourceFileIds;
    ReverseCloneResourceInheritService resourceInheritService;
    int32_t actionableCount = 0;
    int32_t successCount = 0;
    int32_t failedCount = 0;
    for (const auto &[fileId, plan] : resourcePlans) {
        bool isActionable = plan.decision == ReverseCloneResourceDecision::INHERIT && plan.HasResourceAction();
        actionableCount += isActionable ? 1 : 0;
        int32_t ret = resourceInheritService.ExecuteAfterInsert(plan, targetRdb, reportInfo);
        if (ret != E_OK) {
            failedCount++;
            if (fileId > 0) {
                failedResourceFileIds.emplace(fileId);
            }
            MEDIA_ERR_LOG(
                "RevRes ExecuteResourcePlans failed, fileId=%{public}d, donorFileId=%{public}d, ret=%{public}d",
                fileId, plan.donor.fileId, ret);
            continue;
        }
        successCount += isActionable ? 1 : 0;
    }
    int64_t cost = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("RevRes ExecuteResourcePlans: totalPlans=%{public}zu, actionable=%{public}d, "
        "success=%{public}d, failed=%{public}d, failedResourceIds=%{public}zu, cost=%{public}lld",
        resourcePlans.size(), actionableCount, successCount, failedCount, failedResourceFileIds.size(),
        static_cast<long long>(cost));
    return failedResourceFileIds;
}

std::unordered_set<int32_t> ReverseCloneResourceInheritHelper::ExecuteStaleTargetFallback(
    const std::vector<ReverseStaleTargetResource> &staleTargetResources) const
{
    std::unordered_set<int32_t> failedResourceFileIds;
    int32_t successCount = 0;
    int32_t failedCount = 0;
    for (const auto &resource : staleTargetResources) {
        bool success = true;
        if (resource.origin) {
            success = DeletePathIfExists(GetLocalPathByCloudPath(resource.cloudPath)) && success;
        }
        if (resource.movingPhotoVideo) {
            success = DeletePathIfExists(GetMovingPhotoVideoPathByCloudPath(resource.cloudPath)) && success;
        }
        if (resource.editData) {
            success = DeletePathIfExists(GetEditDataPathByCloudPath(resource.cloudPath)) && success;
        }
        if (resource.thumbnail) {
            success = DeletePathIfExists(GetLocalThumbPathByCloudPath(resource.cloudPath)) && success;
        }
        if (!success && resource.fileId > 0) {
            failedResourceFileIds.emplace(resource.fileId);
        }
        successCount += success ? 1 : 0;
        failedCount += success ? 0 : 1;
    }
    MEDIA_INFO_LOG("RevRes ExecuteStaleTargetFallback: total=%{public}zu, success=%{public}d, "
        "failed=%{public}d, failedResourceIds=%{public}zu",
        staleTargetResources.size(), successCount, failedCount, failedResourceFileIds.size());
    return failedResourceFileIds;
}

void ReverseCloneResourceInheritHelper::UpdateAbsorbedPhotosVisible(
    std::shared_ptr<NativeRdb::RdbStore> &targetRdb, const std::vector<FileInfo> &fileInfos,
    const std::unordered_set<int32_t> &failedResourceFileIds) const
{
    CHECK_AND_RETURN_LOG(targetRdb != nullptr, "UpdateAbsorbedPhotosVisible: targetRdb is null");

    std::vector<std::string> visibleIds;
    for (const auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(fileInfo.fileIdOld > 0);
        CHECK_AND_CONTINUE(failedResourceFileIds.find(fileInfo.fileIdOld) == failedResourceFileIds.end());
        visibleIds.emplace_back(std::to_string(fileInfo.fileIdOld));
    }
    CHECK_AND_RETURN_LOG(!visibleIds.empty(), "UpdateAbsorbedPhotosVisible: visibleIds is empty");

    NativeRdb::ValuesBucket updateBucket;
    updateBucket.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));

    auto predicates = std::make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->In(MediaColumn::MEDIA_ID, visibleIds);

    int32_t changeRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(targetRdb, changeRows, updateBucket, predicates);
    CHECK_AND_RETURN_LOG(ret == E_OK, "UpdateAbsorbedPhotosVisible: update failed, ret=%{public}d", ret);
    MEDIA_INFO_LOG("UpdateAbsorbedPhotosVisible: valid=%{public}zu, skippedByResourceFailure=%{public}zu, "
        "visibleIds=%{public}zu, changeRows=%{public}d",
        fileInfos.size(), failedResourceFileIds.size(), visibleIds.size(), changeRows);
}

std::vector<FileInfo> ReverseCloneResourceInheritHelper::CollectValidFileInfos(
    const std::vector<FileInfo> &fileInfos,
    const std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans) const
{
    std::vector<FileInfo> validFileInfos;
    for (const auto &fileInfo : fileInfos) {
        CHECK_AND_CONTINUE(fileInfo.fileIdOld > 0);
        CHECK_AND_CONTINUE(resourcePlans.find(fileInfo.fileIdOld) != resourcePlans.end());
        validFileInfos.emplace_back(fileInfo);
    }
    return validFileInfos;
}

std::vector<ReverseCloneResourceInheritHelper::AssetReferenceUpdate>
ReverseCloneResourceInheritHelper::BuildAssetReferenceUpdates(const ReverseClonePhotoBatchContext &batch) const
{
    std::vector<AssetReferenceUpdate> updates;
    for (const auto &fileInfo : batch.validFileInfos) {
        int32_t donorFileId = fileInfo.deletedSrcdbFileId;
        int32_t absorbedFileId = fileInfo.fileIdOld;
        CHECK_AND_CONTINUE(donorFileId > 0 && absorbedFileId > 0);

        // Self replace must move references temporarily, otherwise Photos delete trigger removes them.
        updates.emplace_back(AssetReferenceUpdate {
            .donorFileId = donorFileId,
            .absorbedFileId = absorbedFileId,
            .protectedFileId = donorFileId == absorbedFileId ? GetProtectedReferenceFileId(donorFileId) :
                absorbedFileId,
        });
    }
    return updates;
}

int32_t ReverseCloneResourceInheritHelper::UpdateAssetReferences(
    TransactionOperations &trans, int32_t fromFileId, int32_t toFileId) const
{
    const std::string updateAnalysisPhotoMapSql = "UPDATE OR IGNORE AnalysisPhotoMap SET map_asset = ? "
        "WHERE map_asset = ?";
    return trans.ExecuteSql(updateAnalysisPhotoMapSql, {std::to_string(toFileId), std::to_string(fromFileId)});
}

int32_t ReverseCloneResourceInheritHelper::DeleteProtectedAssetReferences(
    TransactionOperations &trans, int32_t protectedFileId) const
{
    const std::string deleteAnalysisPhotoMapSql = "DELETE FROM AnalysisPhotoMap WHERE map_asset = ?";
    return trans.ExecuteSql(deleteAnalysisPhotoMapSql, {std::to_string(protectedFileId)});
}

int32_t ReverseCloneResourceInheritHelper::MoveAssetReferencesBeforePhotoDelete(
    TransactionOperations &trans, const std::vector<AssetReferenceUpdate> &updates) const
{
    for (const auto &update : updates) {
        int32_t ret = UpdateAssetReferences(trans, update.donorFileId, update.protectedFileId);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "CommitPhotosBatch: move asset references failed, donorFileId=%{public}d, targetFileId=%{public}d, "
            "ret=%{public}d", update.donorFileId, update.protectedFileId, ret);
    }
    return NativeRdb::E_OK;
}

int32_t ReverseCloneResourceInheritHelper::RestoreProtectedAssetReferences(
    TransactionOperations &trans, const std::vector<AssetReferenceUpdate> &updates) const
{
    for (const auto &update : updates) {
        CHECK_AND_CONTINUE(update.NeedsRestore());
        int32_t ret = UpdateAssetReferences(trans, update.protectedFileId, update.absorbedFileId);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "CommitPhotosBatch: restore asset references failed, protectedFileId=%{public}d, "
            "absorbedFileId=%{public}d, ret=%{public}d", update.protectedFileId, update.absorbedFileId, ret);
        ret = DeleteProtectedAssetReferences(trans, update.protectedFileId);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "CommitPhotosBatch: clean protected asset references failed, protectedFileId=%{public}d, ret=%{public}d",
            update.protectedFileId, ret);
    }
    return NativeRdb::E_OK;
}

void ReverseCloneResourceInheritHelper::FinalizeBatch(const ReverseClonePhotoBatchContext &batch,
    std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
    ReverseRestoreReportInfo &reportInfo)
{
    MEDIA_INFO_LOG("RevRes Reverse absorb finalize batch: valid=%{public}zu, resourcePlans=%{public}zu, "
        "duplicatePlans=%{public}zu, kvTasks=%{public}zu, staleTargets=%{public}zu",
        batch.validFileInfos.size(), batch.resourcePlans.size(), batch.duplicatePlans.size(),
        batch.kvStoreTasks.size(), batch.staleTargetResources.size());
    std::unordered_set<int32_t> failedResourceFileIds = ExecuteStaleTargetFallback(batch.staleTargetResources);
    std::unordered_set<int32_t> planFailedResourceFileIds = ExecuteResourcePlans(batch.resourcePlans,
        targetRdb, reportInfo);
    failedResourceFileIds.insert(planFailedResourceFileIds.begin(), planFailedResourceFileIds.end());
    AppendKvStoreTasks(batch.kvStoreTasks);
    UpdateAbsorbedPhotosVisible(targetRdb, batch.validFileInfos, failedResourceFileIds);
}

int32_t ReverseCloneResourceInheritHelper::DeleteDuplicateDonorsInTransaction(TransactionOperations &trans,
    const std::vector<FileInfo> &fileInfos, std::vector<int32_t> &deletedDonorFileIds) const
{
    const std::string deletePhotoSql = "DELETE FROM Photos WHERE file_id = ?";
    for (auto &fileInfo : fileInfos) {
        int32_t donorFileId = fileInfo.deletedSrcdbFileId;
        if (donorFileId <= 0) {
            continue;
        }
        MEDIA_INFO_LOG("Reverse duplicate donor delete start, donorFileId=%{public}d, "
            "originalDonorFileId=%{public}lld, absorbedFileId=%{public}d, displayName=%{public}s, "
            "lPath=%{public}s, size=%{public}lld, orientation=%{public}d",
            donorFileId, static_cast<long long>(ToOriginalOldDeviceFileId(donorFileId)),
            fileInfo.fileIdOld, fileInfo.displayName.c_str(),
            MediaFileUtils::DesensitizePath(fileInfo.lPath).c_str(), static_cast<long long>(fileInfo.fileSize),
            fileInfo.orientation);
        int32_t ret = trans.ExecuteSql(deletePhotoSql, {std::to_string(donorFileId)});
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
            "CommitPhotosBatch: delete donor failed, donorFileId=%{public}d, absorbedFileId=%{public}d, ret=%{public}d",
            donorFileId, fileInfo.fileIdOld, ret);
        MEDIA_INFO_LOG("Reverse duplicate donor delete success, donorFileId=%{public}d, absorbedFileId=%{public}d",
            donorFileId, fileInfo.fileIdOld);
        deletedDonorFileIds.emplace_back(donorFileId);
    }
    return NativeRdb::E_OK;
}

int32_t ReverseCloneResourceInheritHelper::InsertAbsorbedPhotosInTransaction(TransactionOperations &trans,
    const ReverseClonePhotoBatchContext &batch, const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
    int64_t &insertedRows) const
{
    int32_t ret = trans.BatchInsert(insertedRows, PhotoColumn::PHOTOS_TABLE, batch.values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("CommitPhotosBatch: insert absorbed photos failed, ret=%{public}d", ret);
        LogDataConflictsBeforeInsert(batch, targetRdb);
    }
    return ret;
}

bool ReverseCloneResourceInheritHelper::CommitPhotosInTransaction(ReverseClonePhotoBatchContext &batch,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, int64_t &insertedRows,
    std::vector<int32_t> &deletedDonorFileIds)
{
    insertedRows = 0;
    std::vector<AssetReferenceUpdate> referenceUpdates = BuildAssetReferenceUpdates(batch);
    TransactionOperations trans { __func__ };
    trans.SetBackupRdbStore(targetRdb);
    std::function<int()> commitPhotos = [&]() -> int {
        deletedDonorFileIds.clear();
        insertedRows = 0;
        int32_t ret = MoveAssetReferencesBeforePhotoDelete(trans, referenceUpdates);
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        ret = DeleteDuplicateDonorsInTransaction(trans, batch.validFileInfos, deletedDonorFileIds);
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        ret = InsertAbsorbedPhotosInTransaction(trans, batch, targetRdb, insertedRows);
        CHECK_AND_RETURN_RET(ret == NativeRdb::E_OK, ret);
        return RestoreProtectedAssetReferences(trans, referenceUpdates);
    };
    int32_t transRet = trans.RetryTrans(commitPhotos, true);
    if (transRet != NativeRdb::E_OK) {
        ReleaseDuplicateDonorReservations(batch);
        insertedRows = 0;
        MEDIA_ERR_LOG("CommitPhotosBatch: transaction failed, ret=%{public}d", transRet);
        return false;
    }
    return true;
}

void ReverseCloneResourceInheritHelper::ReleaseDuplicateDonors(const std::vector<int32_t> &deletedDonorFileIds)
{
    std::lock_guard<std::mutex> lock(duplicateMutex_);
    for (int32_t donorFileId : deletedDonorFileIds) {
        reservedDuplicateDonorFileIds_.erase(donorFileId);
    }
}

void ReverseCloneResourceInheritHelper::DeletePhotoExtRows(const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
    const std::vector<int32_t> &deletedDonorFileIds) const
{
    CHECK_AND_RETURN(targetRdb != nullptr);
    const std::string deletePhotoExtSql = "DELETE FROM tab_photos_ext WHERE photo_id = ?";
    for (int32_t donorFileId : deletedDonorFileIds) {
        // tab_photos_ext is derived data and does not follow Photos row deletion automatically.
        int32_t ret = BackupDatabaseUtils::ExecuteSQL(targetRdb, deletePhotoExtSql, {std::to_string(donorFileId)});
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("CommitPhotosBatch: delete photo ext failed, donorFileId=%{public}d, ret=%{public}d",
                donorFileId, ret);
        }
    }
}

bool ReverseCloneResourceInheritHelper::CommitPhotosBatch(ReverseClonePhotoBatchContext &batch,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, int64_t &insertedRows)
{
    CHECK_AND_RETURN_RET_LOG(targetRdb != nullptr, false, "CommitPhotosBatch: targetRdb is null");
    std::vector<int32_t> deletedDonorFileIds;
    if (!CommitPhotosInTransaction(batch, targetRdb, insertedRows, deletedDonorFileIds)) {
        return false;
    }

    ReleaseDuplicateDonors(deletedDonorFileIds);
    DeletePhotoExtRows(targetRdb, deletedDonorFileIds);
    MEDIA_INFO_LOG("CommitPhotosBatch: deletedDonors=%{public}zu, insertedRows=%{public}lld, "
        "totalReserved=%{public}zu", deletedDonorFileIds.size(),
        static_cast<long long>(insertedRows), reservedDuplicateDonorFileIds_.size());
    return true;
}

void ReverseCloneResourceInheritHelper::ForceAbsorbSourceResourcesOnCommitFailed(
    const ReverseClonePhotoBatchContext &batch) const
{
    int32_t successCount = 0;
    int32_t failedCount = 0;
    int32_t attemptedCount = 0;
    for (const auto &entry : batch.resourcePlans) {
        attemptedCount += ForceAbsorbSourceResourceOnCommitFailed(entry.second, successCount, failedCount) ? 1 : 0;
    }
    int32_t skippedCount = static_cast<int32_t>(batch.resourcePlans.size()) - attemptedCount;
    MEDIA_ERR_LOG("Reverse force absorb source resources after db commit failed finished, total=%{public}zu, "
        "attempted=%{public}d, skipped=%{public}d, success=%{public}d, failed=%{public}d",
        batch.resourcePlans.size(), attemptedCount, skippedCount, successCount, failedCount);
}

void ReverseCloneResourceInheritHelper::SnapshotPureCloudFileIds(
    const std::shared_ptr<NativeRdb::RdbStore> &rdb)
{
    originalPureCloudFileIds_.clear();
    CHECK_AND_RETURN_LOG(rdb != nullptr, "SnapshotPureCloudFileIds: rdb is null");
    std::string querySql = "SELECT " + MediaColumn::MEDIA_ID + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::PHOTO_CLEAN_FLAG + " = 1 AND " + PhotoColumn::PHOTO_POSITION + " = " +
        std::to_string(static_cast<int32_t>(PhotoPositionType::CLOUD));
    auto resultSet = rdb->QuerySql(querySql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "SnapshotPureCloudFileIds: query pure cloud rows failed");

    std::unordered_set<int32_t> fileIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileIds.emplace(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
    }
    resultSet->Close();
    originalPureCloudFileIds_ = std::move(fileIds);
    MEDIA_INFO_LOG("SnapshotPureCloudFileIds: count=%{public}zu", originalPureCloudFileIds_.size());
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
