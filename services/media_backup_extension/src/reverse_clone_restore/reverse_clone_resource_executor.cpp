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

#include "reverse_clone_resource_executor.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <sys/stat.h>
#include <vector>

#include "backup_file_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_db_const.h"
#include "moving_photo_file_utils.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
namespace {
enum class ThumbnailKind {
    LCD,
    THUMBNAIL,
};

struct ThumbnailSpec {
    const char *relativePath;
    ThumbnailKind kind;
};

enum class OriginResourceState {
    NONE,
    TARGET_EXISTING,
    TARGET_EXISTING_WITH_SOURCE,
    TRANSFERRED,
};

struct OriginResourceResult {
    OriginResourceState state {OriginResourceState::NONE};
    ReverseCloneAssetResource source;

    bool Exists() const
    {
        return state != OriginResourceState::NONE;
    }

    bool HasSource() const
    {
        return state == OriginResourceState::TARGET_EXISTING_WITH_SOURCE ||
            state == OriginResourceState::TRANSFERRED;
    }

    bool WasTransferred() const
    {
        return state == OriginResourceState::TRANSFERRED;
    }

    void MarkExisting()
    {
        state = OriginResourceState::TARGET_EXISTING;
    }

    void MarkExisting(const ReverseCloneAssetResource &resource, const std::string &sourcePath)
    {
        state = OriginResourceState::TARGET_EXISTING_WITH_SOURCE;
        source = resource;
        source.originPath = sourcePath;
    }

    void MarkTransferred(const ReverseCloneAssetResource &resource, const std::string &sourcePath)
    {
        state = OriginResourceState::TRANSFERRED;
        source = resource;
        source.originPath = sourcePath;
    }
};

enum class ThumbnailSourceState {
    NONE,
    TARGET_EXISTING,
    SOURCE_CHANGED,
};

struct ThumbnailSourceResult {
    ThumbnailSourceState state {ThumbnailSourceState::NONE};
    ReverseCloneAssetResource source;

    bool Exists() const
    {
        return state != ThumbnailSourceState::NONE;
    }

    bool SourceChanged() const
    {
        return state == ThumbnailSourceState::SOURCE_CHANGED;
    }

    void Remember(const ReverseCloneAssetResource &resource, bool sourceChanged)
    {
        if (sourceChanged || state == ThumbnailSourceState::NONE) {
            source = resource;
        }
        if (sourceChanged) {
            state = ThumbnailSourceState::SOURCE_CHANGED;
            return;
        }
        if (state == ThumbnailSourceState::NONE) {
            state = ThumbnailSourceState::TARGET_EXISTING;
        }
    }
};

struct ThumbnailTransferStats {
    int32_t copiedCount {0};
    int32_t movedCount {0};
    int32_t existingCount {0};
    int32_t missingCount {0};
    int32_t failedCount {0};
};

struct ThumbnailTransferResult {
    ThumbnailSourceResult lcd;
    ThumbnailSourceResult thumb;
    ThumbnailTransferStats stats;

    bool HasLcd() const
    {
        return lcd.Exists();
    }

    bool HasThumbnail() const
    {
        return thumb.Exists();
    }

    bool SourceChanged() const
    {
        return lcd.SourceChanged() || thumb.SourceChanged();
    }

    ThumbnailSourceResult &Get(ThumbnailKind kind)
    {
        return kind == ThumbnailKind::LCD ? lcd : thumb;
    }
};

struct ResourceExecuteResult {
    OriginResourceResult origin;
    ThumbnailTransferResult thumbnail;
    int32_t editDataWithoutEditTimeCount {0};
    std::vector<std::string> deferredDeletePaths;
};

struct FileTransferResult {
    int32_t errCode {E_OK};
    bool moved {false};
    bool copied {false};
};

enum class ThumbnailFileTransferState {
    MISSING,
    TARGET_EXISTING,
    TRANSFERRED,
};

struct ThumbnailFileTransferResult {
    int32_t errCode {E_OK};
    ThumbnailFileTransferState state {ThumbnailFileTransferState::MISSING};
};

struct TransferCleanupResult {
    int32_t deletedCount {0};
    int32_t skippedCount {0};
    int32_t failedCount {0};
};

const std::array<ThumbnailSpec, 5> THUMBNAIL_SPECS = {{
    {"LCD.jpg", ThumbnailKind::LCD},
    {"THM_EX/LCD.jpg", ThumbnailKind::LCD},
    {"THM.jpg", ThumbnailKind::THUMBNAIL},
    {"THM_EX/THM.jpg", ThumbnailKind::THUMBNAIL},
    {"THM_ASTC.astc", ThumbnailKind::THUMBNAIL},
}};
const std::string LAKE_STORAGE_ROOT = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const std::string CLOUD_DENTRY_ROOT = "/storage/media/cloud/files/";
constexpr int32_t MAX_RENAME_NUMBER = 1000;

// LCOV_EXCL_START
bool IsPathUnderDirectory(const std::string &path, const std::string &directory)
{
    if (directory.empty() || path.length() <= directory.length() ||
        path.compare(0, directory.length(), directory) != 0) {
        return false;
    }
    return directory.back() == '/' || path[directory.length()] == '/';
}

bool HasRealLakeStoragePath(const ReverseCloneAssetResource &resource)
{
    return IsPathUnderDirectory(resource.storagePath, LAKE_STORAGE_ROOT);
}

bool IsLakeStorageAsset(const ReverseCloneAssetResource &resource)
{
    return resource.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE) &&
        HasRealLakeStoragePath(resource);
}

bool IsInactiveAsset(const ReverseCloneAssetResource &resource)
{
    return resource.dateTrashed > 0 || resource.hidden > 0;
}

bool HasLocalOriginPosition(const ReverseCloneAssetResource &resource)
{
    return resource.position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
        resource.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
}

bool CanUseLakeStorageAsSource(const ReverseCloneAssetResource &resource)
{
    return HasLocalOriginPosition(resource) && IsLakeStorageAsset(resource) &&
        MediaFileUtils::IsFileExists(resource.storagePath);
}

bool ShouldUseLakeStorageAsTarget(const ReverseCloneAssetResource &resource)
{
    return HasRealLakeStoragePath(resource) && !IsInactiveAsset(resource);
}

std::string GetNumberedStoragePath(const std::string &storagePath, int32_t number)
{
    size_t slashPos = storagePath.find_last_of('/');
    size_t dotPos = storagePath.find_last_of('.');
    if (dotPos == std::string::npos || (slashPos != std::string::npos && dotPos < slashPos)) {
        return storagePath + "(" + std::to_string(number) + ")";
    }
    return storagePath.substr(0, dotPos) + "(" + std::to_string(number) + ")" + storagePath.substr(dotPos);
}

std::string ResolveLakeTargetStoragePath(const std::string &storagePath)
{
    if (!MediaFileUtils::IsFileExists(storagePath)) {
        return storagePath;
    }
    for (int32_t number = 1; number <= MAX_RENAME_NUMBER; ++number) {
        std::string numberedPath = GetNumberedStoragePath(storagePath, number);
        if (!MediaFileUtils::IsFileExists(numberedPath)) {
            return numberedPath;
        }
    }
    return "";
}

bool IsLakeTargetAlreadyOwnedByDonor(const ReverseCloneResourcePlan &plan)
{
    return CanUseLakeStorageAsSource(plan.donor) && plan.donor.storagePath == plan.absorbed.storagePath;
}

int32_t PrepareLakeStorageTarget(ReverseCloneResourcePlan &plan)
{
    if (!ShouldUseLakeStorageAsTarget(plan.absorbed)) {
        return E_OK;
    }
    if (IsLakeTargetAlreadyOwnedByDonor(plan)) {
        return E_OK;
    }
    std::string resolvedPath = ResolveLakeTargetStoragePath(plan.absorbed.storagePath);
    CHECK_AND_RETURN_RET_LOG(!resolvedPath.empty(), E_FAIL,
        "RevRes resolve lake target storage path failed, targetFileId=%{public}d, path=%{public}s",
        plan.absorbed.fileId, MediaFileUtils::DesensitizePath(plan.absorbed.storagePath).c_str());
    if (resolvedPath == plan.absorbed.storagePath) {
        return E_OK;
    }
    MEDIA_INFO_LOG("RevRes lake target storage path renamed, fileId=%{public}d, old=%{public}s, new=%{public}s",
        plan.absorbed.fileId, MediaFileUtils::DesensitizePath(plan.absorbed.storagePath).c_str(),
        MediaFileUtils::DesensitizePath(resolvedPath).c_str());
    plan.absorbed.storagePath = resolvedPath;
    plan.lakeTargetRenamed = true;
    return E_OK;
}

std::string NormalizeRoot(const std::string &root)
{
    if (root.empty()) {
        return RESTORE_FILES_LOCAL_DIR;
    }
    if (root.back() == '/') {
        return root.substr(0, root.length() - 1);
    }
    return root;
}

std::string GetResourcePathByCloudPath(const std::string &cloudPath, const std::string &root)
{
    if (!IsPathUnderDirectory(cloudPath, RESTORE_FILES_CLOUD_DIR)) {
        return "";
    }
    return NormalizeRoot(root) + "/" + cloudPath.substr(RESTORE_FILES_CLOUD_DIR.length());
}

std::string GetDentryPathByCloudPath(const std::string &cloudPath)
{
    return GetResourcePathByCloudPath(cloudPath, CLOUD_DENTRY_ROOT);
}

std::string GetThumbnailDentryDirByCloudPath(const std::string &cloudPath)
{
    return GetResourcePathByCloudPath(cloudPath, NormalizeRoot(CLOUD_DENTRY_ROOT) + "/.thumbs");
}

std::string JoinLocalHiddenDir(const std::string &dirName)
{
    return NormalizeRoot(RESTORE_FILES_LOCAL_DIR) + "/" + dirName;
}

class ReverseCloneResourceLocator {
public:
    explicit ReverseCloneResourceLocator(const ReverseCloneResourcePlan &plan) : plan_(plan) {}

    std::string GetSourceOriginPath(const ReverseCloneAssetResource &resource) const
    {
        std::vector<std::string> paths = GetSourceOriginPaths(resource);
        if (paths.empty()) {
            return "";
        }
        return paths.front();
    }

    std::vector<std::string> GetSourceOriginPaths(const ReverseCloneAssetResource &resource) const
    {
        std::vector<std::string> sourcePaths;
        if (HasRealLakeSource(resource)) {
            sourcePaths.emplace_back(resource.storagePath);
        }
        if (!resource.originPath.empty() && resource.originPath != resource.storagePath) {
            sourcePaths.emplace_back(resource.originPath);
        }
        std::string sourcePath = GetResourcePathByCloudPath(resource.cloudPath, resource.localRoot);
        if (!sourcePath.empty() && sourcePath != resource.storagePath && sourcePath != resource.originPath) {
            sourcePaths.emplace_back(sourcePath);
        }
        return sourcePaths;
    }

    std::string GetTargetOriginPath() const
    {
        if (IsLakeStorageTarget(plan_.absorbed)) {
            return plan_.absorbed.storagePath;
        }
        return GetResourcePathByCloudPath(plan_.absorbed.cloudPath, RESTORE_FILES_LOCAL_DIR);
    }

    bool HasRealLakeSource(const ReverseCloneAssetResource &resource) const
    {
        return CanUseLakeStorageAsSource(resource);
    }

    bool IsLakeStorageTarget(const ReverseCloneAssetResource &resource) const
    {
        return ShouldUseLakeStorageAsTarget(resource);
    }

    std::string GetSourceThumbDir(const ReverseCloneAssetResource &resource) const
    {
        if (!resource.localRoot.empty() && !resource.relativePath.empty()) {
            return NormalizeRoot(resource.localRoot) + "/.thumbs" + resource.relativePath;
        }
        return GetResourcePathByCloudPath(resource.cloudPath, NormalizeRoot(resource.localRoot) + "/.thumbs");
    }

    std::string GetTargetThumbDir() const
    {
        return GetResourcePathByCloudPath(plan_.absorbed.cloudPath, JoinLocalHiddenDir(".thumbs"));
    }

    std::string GetSourceEditDataPath(const ReverseCloneAssetResource &resource) const
    {
        if (!resource.localRoot.empty() && !resource.relativePath.empty()) {
            return NormalizeRoot(resource.localRoot) + "/.editData" + resource.relativePath;
        }
        return GetResourcePathByCloudPath(resource.cloudPath, NormalizeRoot(resource.localRoot) + "/.editData");
    }

    std::string GetTargetEditDataPath() const
    {
        return GetResourcePathByCloudPath(plan_.absorbed.cloudPath, JoinLocalHiddenDir(".editData"));
    }

private:
    const ReverseCloneResourcePlan &plan_;
};

class ReverseCloneResourceTransferSession {
public:
    ResourceExecuteResult &Result()
    {
        return result_;
    }

    const ResourceExecuteResult &Result() const
    {
        return result_;
    }

    FileTransferResult TransferFile(const std::string &srcPath, const std::string &dstPath)
    {
        // Reverse restore prefers rename for performance. If a later DB update fails, the absorbed row is still not
        // marked visible in this flow, and leftover files are handled by retry or cleanup instead of rolling back here.
        FileTransferResult result;
        if (MediaFileUtils::MoveFile(srcPath, dstPath)) {
            result.moved = true;
            return result;
        }
        MEDIA_WARN_LOG("RevRes Reverse clone rename failed, fallback copy, src:%{public}s, dst:%{public}s, "
            "errno:%{public}d",
            MediaFileUtils::DesensitizePath(srcPath).c_str(), MediaFileUtils::DesensitizePath(dstPath).c_str(), errno);
        result.errCode = CopyFile(srcPath, dstPath);
        if (result.errCode == E_OK) {
            result.copied = true;
            result_.deferredDeletePaths.emplace_back(srcPath);
        }
        return result;
    }

    void DeferSourceCleanup(const std::string &srcPath, const std::string &dstPath)
    {
        if (srcPath.empty() || srcPath == dstPath || !MediaFileUtils::IsFileExists(srcPath)) {
            return;
        }
        if (std::find(result_.deferredDeletePaths.begin(), result_.deferredDeletePaths.end(), srcPath) !=
            result_.deferredDeletePaths.end()) {
            return;
        }
        result_.deferredDeletePaths.emplace_back(srcPath);
        MEDIA_INFO_LOG("RevRes deferred source cleanup, src:%{public}s, dst:%{public}s",
            MediaFileUtils::DesensitizePath(srcPath).c_str(), MediaFileUtils::DesensitizePath(dstPath).c_str());
    }

    FileTransferResult TransferDirectory(const std::string &srcPath, const std::string &dstPath)
    {
        FileTransferResult result;
        if (MediaFileUtils::RenameDir(srcPath, dstPath)) {
            result.moved = true;
            return result;
        }
        MEDIA_WARN_LOG("RevRes Reverse clone rename dir failed, fallback copy, src:%{public}s, dst:%{public}s, "
            "errno:%{public}d",
            MediaFileUtils::DesensitizePath(srcPath).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str(), errno);
        result.errCode = MediaFileUtils::CopyDirectory(srcPath, dstPath);
        if (result.errCode == E_OK) {
            result.copied = true;
            result_.deferredDeletePaths.emplace_back(srcPath);
        }
        return result;
    }

    void CleanTransferredSourcePaths() const
    {
        TransferCleanupResult cleanupResult = CleanTransferredSourcePaths(result_.deferredDeletePaths);
        MEDIA_INFO_LOG("RevRes clean transferred source paths completed, count=%{public}zu, deleted=%{public}d, "
            "skipped=%{public}d, failed=%{public}d, editDataWithoutEditTime=%{public}d",
            result_.deferredDeletePaths.size(), cleanupResult.deletedCount, cleanupResult.skippedCount,
            cleanupResult.failedCount, result_.editDataWithoutEditTimeCount);
    }

private:
    static TransferCleanupResult CleanTransferredSourcePaths(const std::vector<std::string> &paths)
    {
        TransferCleanupResult result;
        for (const auto &path : paths) {
            CleanTransferredSourcePath(path, result);
        }
        return result;
    }

    static int32_t CopyFile(const std::string &srcPath, const std::string &dstPath)
    {
        if (!MediaFileUtils::CopyFileUtil(srcPath, dstPath)) {
            MEDIA_ERR_LOG("RevRes Reverse clone copy file failed, src:%{public}s, dst:%{public}s, errno:%{public}d",
                MediaFileUtils::DesensitizePath(srcPath).c_str(), MediaFileUtils::DesensitizePath(dstPath).c_str(),
                errno);
            return E_FAIL;
        }
        return E_OK;
    }

    static void CleanTransferredSourcePath(const std::string &path, TransferCleanupResult &result)
    {
        if (path.empty() || !MediaFileUtils::IsFileExists(path)) {
            result.skippedCount++;
            return;
        }
        bool deleted = MediaFileUtils::IsDirectory(path) ? MediaFileUtils::DeleteDir(path) :
            MediaFileUtils::DeleteFile(path);
        if (deleted) {
            result.deletedCount++;
            return;
        }
        result.failedCount++;
        MEDIA_WARN_LOG("RevRes clean transferred source failed, path=%{public}s",
            MediaFileUtils::DesensitizePath(path).c_str());
    }

    ResourceExecuteResult result_;
};

bool HasMovingPhotoVideo(const ReverseCloneAssetResource &resource)
{
    return resource.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        resource.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

std::vector<const ReverseCloneAssetResource *> GetResourceCandidates(const ReverseCloneResourcePlan &plan)
{
    std::vector<const ReverseCloneAssetResource *> candidates;
    if (plan.donor.HasResourcePath()) {
        candidates.emplace_back(&plan.donor);
    }
    if (plan.hasFallbackSource && plan.fallbackSource.HasResourcePath()) {
        candidates.emplace_back(&plan.fallbackSource);
    }
    return candidates;
}

bool HasResourceCandidate(const ReverseCloneResourcePlan &plan)
{
    return plan.donor.HasResourcePath() || (plan.hasFallbackSource && plan.fallbackSource.HasResourcePath());
}

bool IsSameAssetSource(const ReverseCloneAssetResource &left, const ReverseCloneAssetResource &right)
{
    if (left.fileId > 0 && right.fileId > 0) {
        return left.fileId == right.fileId;
    }
    return left.cloudPath == right.cloudPath && left.localRoot == right.localRoot &&
        left.originPath == right.originPath && left.storagePath == right.storagePath;
}

bool IsExistingLakeOriginCandidate(const ReverseCloneResourcePlan &plan, const ReverseCloneAssetResource &lakeSource)
{
    if (CanUseLakeStorageAsSource(lakeSource)) {
        return true;
    }
    MEDIA_INFO_LOG("RevRes lake origin storage path is residual, fallback to normal origin candidates, "
        "sourceFileId=%{public}d, targetFileId=%{public}d, position=%{public}d, src=%{public}s",
        lakeSource.fileId, plan.absorbed.fileId, lakeSource.position,
        MediaFileUtils::DesensitizePath(lakeSource.storagePath).c_str());
    return false;
}

const ReverseCloneAssetResource *FindExistingLakeOriginCandidate(const ReverseCloneResourcePlan &plan)
{
    if (HasRealLakeStoragePath(plan.donor) && IsExistingLakeOriginCandidate(plan, plan.donor)) {
        return &plan.donor;
    }
    if (plan.hasFallbackSource && HasRealLakeStoragePath(plan.fallbackSource) &&
        IsExistingLakeOriginCandidate(plan, plan.fallbackSource)) {
        return &plan.fallbackSource;
    }
    return nullptr;
}

bool ShouldMoveLakeOriginToMedia(const ReverseCloneResourcePlan &plan)
{
    return !ShouldUseLakeStorageAsTarget(plan.absorbed) && FindExistingLakeOriginCandidate(plan) != nullptr;
}

std::string FindExistingOriginSourcePath(const ReverseCloneResourceLocator &paths,
    const ReverseCloneAssetResource &candidate, const std::string &dstPath)
{
    std::vector<std::string> sourcePaths = paths.GetSourceOriginPaths(candidate);
    for (size_t index = 0; index < sourcePaths.size(); ++index) {
        bool srcExists = !sourcePaths[index].empty() && MediaFileUtils::IsFileExists(sourcePaths[index]);
        MEDIA_INFO_LOG("RevRes Reverse clone origin candidate, fileId=%{public}d, src=%{public}s, "
            "dst=%{public}s, srcExists=%{public}d, sourceIndex=%{public}zu, sourceCount=%{public}zu",
            candidate.fileId, MediaFileUtils::DesensitizePath(sourcePaths[index]).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str(), srcExists, index, sourcePaths.size());
        if (srcExists) {
            return sourcePaths[index];
        }
    }
    return "";
}

void DeferLakeStorageCleanup(const ReverseCloneResourcePlan &plan, const ReverseCloneAssetResource &source,
    const std::string &dstPath, ReverseCloneResourceTransferSession &session)
{
    if (!CanUseLakeStorageAsSource(source) || ShouldUseLakeStorageAsTarget(plan.absorbed)) {
        return;
    }
    session.DeferSourceCleanup(source.storagePath, dstPath);
}

int32_t TransferLakeOriginToMedia(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    ReverseCloneResourceTransferSession &session)
{
    ResourceExecuteResult &result = session.Result();
    const ReverseCloneAssetResource *lakeSource = FindExistingLakeOriginCandidate(plan);
    CHECK_AND_RETURN_RET_LOG(lakeSource != nullptr, E_FAIL,
        "RevRes lake origin source is invalid, targetFileId=%{public}d", plan.absorbed.fileId);

    std::string srcPath = lakeSource->storagePath;
    std::string dstPath = paths.GetTargetOriginPath();
    CHECK_AND_RETURN_RET_LOG(!srcPath.empty() && !dstPath.empty(), E_FAIL,
        "RevRes lake origin path invalid, sourceFileId=%{public}d, targetFileId=%{public}d, "
        "src=%{public}s, dst=%{public}s",
        lakeSource->fileId, plan.absorbed.fileId, MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str());

    bool srcExists = MediaFileUtils::IsFileExists(srcPath);
    bool dstExists = MediaFileUtils::IsFileExists(dstPath);
    MEDIA_INFO_LOG("RevRes lake origin migrate, sourceFileId=%{public}d, targetFileId=%{public}d, "
        "src=%{public}s, dst=%{public}s, srcExists=%{public}d, dstExists=%{public}d",
        lakeSource->fileId, plan.absorbed.fileId, MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str(), srcExists, dstExists);

    if (dstExists) {
        if (srcExists) {
            result.origin.MarkExisting(*lakeSource, srcPath);
        } else {
            result.origin.MarkExisting();
        }
        if (!IsSameAssetSource(*lakeSource, plan.absorbed)) {
            MediaFileUtils::UpdateModifyTimeInMsec(dstPath, lakeSource->dateModified);
        }
        session.DeferSourceCleanup(srcPath, dstPath);
        return E_OK;
    }

    CHECK_AND_RETURN_RET_LOG(srcExists, E_FAIL,
        "RevRes lake origin source missing, sourceFileId=%{public}d, targetFileId=%{public}d, "
        "src=%{public}s, dst=%{public}s",
        lakeSource->fileId, plan.absorbed.fileId, MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str());
    CHECK_AND_RETURN_RET(BackupFileUtils::PreparePath(dstPath) == E_OK, E_FAIL);
    FileTransferResult transferResult = session.TransferFile(srcPath, dstPath);
    CHECK_AND_RETURN_RET(transferResult.errCode == E_OK, E_FAIL);
    MediaFileUtils::UpdateModifyTimeInMsec(dstPath, lakeSource->dateModified);

    result.origin.MarkTransferred(*lakeSource, srcPath);
    MEDIA_INFO_LOG("RevRes lake origin migrated, sourceFileId=%{public}d, targetFileId=%{public}d, "
        "moved=%{public}d, copied=%{public}d",
        lakeSource->fileId, plan.absorbed.fileId, transferResult.moved, transferResult.copied);
    return E_OK;
}

int32_t TransferOrigin(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    ReverseCloneResourceTransferSession &session)
{
    if (ShouldMoveLakeOriginToMedia(plan)) {
        return TransferLakeOriginToMedia(plan, paths, session);
    }
    ResourceExecuteResult &result = session.Result();
    std::string dstPath = paths.GetTargetOriginPath();
    CHECK_AND_RETURN_RET_LOG(!dstPath.empty(), E_FAIL, "RevRes Reverse clone origin path is invalid");
    std::vector<const ReverseCloneAssetResource *> candidates = GetResourceCandidates(plan);
    if (MediaFileUtils::IsFileExists(dstPath)) {
        result.origin.MarkExisting();
        MEDIA_INFO_LOG("RevRes Reverse clone origin target already exists, skip copy: %{public}s",
            MediaFileUtils::DesensitizePath(dstPath).c_str());
        for (const auto *candidate : candidates) {
            CHECK_AND_CONTINUE(candidate != nullptr);
            const auto &candidateResource = *candidate;
            std::string srcPath = FindExistingOriginSourcePath(paths, candidateResource, dstPath);
            CHECK_AND_CONTINUE(!srcPath.empty());
            result.origin.MarkExisting(candidateResource, srcPath);
            if (!IsSameAssetSource(candidateResource, plan.absorbed)) {
                MediaFileUtils::UpdateModifyTimeInMsec(dstPath, candidateResource.dateModified);
            }
            session.DeferSourceCleanup(srcPath, dstPath);
            DeferLakeStorageCleanup(plan, candidateResource, dstPath, session);
            break;
        }
        return E_OK;
    }
    for (const auto *candidate : candidates) {
        CHECK_AND_CONTINUE(candidate != nullptr);
        const auto &candidateResource = *candidate;
        std::string srcPath = FindExistingOriginSourcePath(paths, candidateResource, dstPath);
        CHECK_AND_CONTINUE(!srcPath.empty());
        CHECK_AND_RETURN_RET(BackupFileUtils::PreparePath(dstPath) == E_OK, E_FAIL);
        FileTransferResult transferResult = session.TransferFile(srcPath, dstPath);
        CHECK_AND_RETURN_RET(transferResult.errCode == E_OK, E_FAIL);
        MediaFileUtils::UpdateModifyTimeInMsec(dstPath, candidateResource.dateModified);
        result.origin.MarkTransferred(candidateResource, srcPath);
        DeferLakeStorageCleanup(plan, candidateResource, dstPath, session);
        MEDIA_INFO_LOG("RevRes Reverse clone origin transferred, sourceFileId=%{public}d, targetFileId=%{public}d, "
            "moved=%{public}d, copied=%{public}d",
            candidateResource.fileId, plan.absorbed.fileId, transferResult.moved, transferResult.copied);
        return E_OK;
    }
    MEDIA_INFO_LOG("RevRes Reverse clone origin donor does not exist");
    return E_OK;
}

int32_t TransferMovingPhotoVideo(const ReverseCloneResourceLocator &paths,
    const ReverseCloneAssetResource &originSource, ReverseCloneResourceTransferSession &session)
{
    std::string srcVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(paths.GetSourceOriginPath(originSource));
    std::string dstVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(paths.GetTargetOriginPath());
    if (!MediaFileUtils::IsFileExists(srcVideoPath)) {
        return E_OK;
    }
    if (MediaFileUtils::IsFileExists(dstVideoPath)) {
        session.DeferSourceCleanup(srcVideoPath, dstVideoPath);
        return E_OK;
    }
    CHECK_AND_RETURN_RET(BackupFileUtils::PreparePath(dstVideoPath) == E_OK, E_FAIL);
    FileTransferResult transferResult = session.TransferFile(srcVideoPath, dstVideoPath);
    CHECK_AND_RETURN_RET(transferResult.errCode == E_OK, E_FAIL);
    MediaFileUtils::UpdateModifyTimeInMsec(dstVideoPath, originSource.dateModified);
    return E_OK;
}

int32_t TransferEditData(const ReverseCloneResourceLocator &paths, const ReverseCloneAssetResource &originSource,
    ReverseCloneResourceTransferSession &session)
{
    ResourceExecuteResult &result = session.Result();
    std::string srcEditDataPath = paths.GetSourceEditDataPath(originSource);
    std::string dstEditDataPath = paths.GetTargetEditDataPath();
    if (!MediaFileUtils::IsFileExists(srcEditDataPath)) {
        return E_OK;
    }
    // A real editData directory is resource evidence even when edit_time is missing in historical DB rows.
    if (MediaFileUtils::IsFileExists(dstEditDataPath)) {
        session.DeferSourceCleanup(srcEditDataPath, dstEditDataPath);
        return E_OK;
    }
    CHECK_AND_RETURN_RET(BackupFileUtils::PreparePath(dstEditDataPath) == E_OK, E_FAIL);
    FileTransferResult transferResult = session.TransferDirectory(srcEditDataPath, dstEditDataPath);
    CHECK_AND_RETURN_RET(transferResult.errCode == E_OK, E_FAIL);
    bool editDataWithoutEditTime = originSource.editTime <= 0;
    result.editDataWithoutEditTimeCount += editDataWithoutEditTime ? 1 : 0;
    MEDIA_INFO_LOG("RevRes edit data transferred, fileId=%{public}d, editTime=%{public}lld, "
        "withoutEditTime=%{public}d, src=%{public}s, dst=%{public}s, moved=%{public}d, copied=%{public}d",
        originSource.fileId, static_cast<long long>(originSource.editTime), editDataWithoutEditTime,
        MediaFileUtils::DesensitizePath(srcEditDataPath).c_str(),
        MediaFileUtils::DesensitizePath(dstEditDataPath).c_str(), transferResult.moved, transferResult.copied);
    return E_OK;
}

bool NeedCopyThumbnail(const ReverseCloneResourcePlan &plan, ThumbnailKind kind)
{
    return (kind == ThumbnailKind::LCD && plan.inheritLcdThumbnail) ||
        (kind == ThumbnailKind::THUMBNAIL && plan.inheritThumbnail);
}

void RememberThumbnailSource(ThumbnailKind kind, const ReverseCloneAssetResource &source,
    const ReverseCloneResourcePlan &plan, ResourceExecuteResult &result)
{
    result.thumbnail.Get(kind).Remember(source, !IsSameAssetSource(source, plan.absorbed));
}

ThumbnailFileTransferResult TransferThumbnailFile(const std::string &srcThumbDir, const std::string &dstThumbDir,
    const ThumbnailSpec &spec, ReverseCloneResourceTransferSession &session)
{
    ResourceExecuteResult &result = session.Result();
    std::string srcPath = srcThumbDir + "/" + spec.relativePath;
    std::string dstPath = dstThumbDir + "/" + spec.relativePath;
    if (MediaFileUtils::IsFileExists(dstPath)) {
        result.thumbnail.stats.existingCount++;
        session.DeferSourceCleanup(srcPath, dstPath);
        MEDIA_INFO_LOG("RevRes thumbnail target already exists, name=%{public}s, dst=%{public}s",
            spec.relativePath, MediaFileUtils::DesensitizePath(dstPath).c_str());
        return {E_OK, ThumbnailFileTransferState::TARGET_EXISTING};
    }
    if (!MediaFileUtils::IsFileExists(srcPath)) {
        result.thumbnail.stats.missingCount++;
        MEDIA_WARN_LOG("RevRes thumbnail source missing, name=%{public}s, src=%{public}s, dst=%{public}s",
            spec.relativePath, MediaFileUtils::DesensitizePath(srcPath).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str());
        return {E_OK, ThumbnailFileTransferState::MISSING};
    }
    if (BackupFileUtils::PreparePath(dstPath) != E_OK) {
        result.thumbnail.stats.failedCount++;
        MEDIA_ERR_LOG("RevRes thumbnail prepare target failed, name=%{public}s, src=%{public}s, dst=%{public}s",
            spec.relativePath, MediaFileUtils::DesensitizePath(srcPath).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str());
        return {E_FAIL, ThumbnailFileTransferState::MISSING};
    }
    FileTransferResult transferResult = session.TransferFile(srcPath, dstPath);
    if (transferResult.errCode != E_OK) {
        result.thumbnail.stats.failedCount++;
        MEDIA_ERR_LOG("RevRes thumbnail transfer failed, name=%{public}s, src=%{public}s, dst=%{public}s",
            spec.relativePath, MediaFileUtils::DesensitizePath(srcPath).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str());
        return {E_FAIL, ThumbnailFileTransferState::MISSING};
    }
    result.thumbnail.stats.copiedCount += transferResult.copied ? 1 : 0;
    result.thumbnail.stats.movedCount += transferResult.moved ? 1 : 0;
    MEDIA_INFO_LOG("RevRes thumbnail transferred, name=%{public}s, src=%{public}s, dst=%{public}s, "
        "moved=%{public}d, copied=%{public}d",
        spec.relativePath, MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str(), transferResult.moved, transferResult.copied);
    return {E_OK, ThumbnailFileTransferState::TRANSFERRED};
}

int32_t TransferThumbnailFiles(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    ReverseCloneResourceTransferSession &session)
{
    ResourceExecuteResult &result = session.Result();
    std::string dstThumbDir = paths.GetTargetThumbDir();
    CHECK_AND_RETURN_RET_LOG(!dstThumbDir.empty(), E_FAIL, "RevRes Reverse clone thumbnail path is invalid");
    std::vector<const ReverseCloneAssetResource *> candidates = GetResourceCandidates(plan);
    MEDIA_INFO_LOG("RevRes Reverse clone thumbnail transfer start, targetFileId=%{public}d, dstThumbDir=%{public}s, "
        "candidateCount=%{public}zu, inheritLcd=%{public}d, inheritThumb=%{public}d",
        plan.absorbed.fileId, MediaFileUtils::DesensitizePath(dstThumbDir).c_str(), candidates.size(),
        plan.inheritLcdThumbnail, plan.inheritThumbnail);
    for (const auto &spec : THUMBNAIL_SPECS) {
        CHECK_AND_CONTINUE(NeedCopyThumbnail(plan, spec.kind));
        for (const auto *candidate : candidates) {
            CHECK_AND_CONTINUE(candidate != nullptr);
            const auto &candidateResource = *candidate;
            std::string srcThumbDir = paths.GetSourceThumbDir(candidateResource);
            CHECK_AND_CONTINUE(!srcThumbDir.empty());
            ThumbnailFileTransferResult fileResult = TransferThumbnailFile(srcThumbDir, dstThumbDir, spec, session);
            CHECK_AND_RETURN_RET(fileResult.errCode == E_OK, E_FAIL);
            if (fileResult.state == ThumbnailFileTransferState::TARGET_EXISTING) {
                RememberThumbnailSource(spec.kind, candidateResource, plan, result);
                break;
            }
            if (fileResult.state == ThumbnailFileTransferState::TRANSFERRED) {
                RememberThumbnailSource(spec.kind, candidateResource, plan, result);
                break;
            }
        }
    }
    MEDIA_INFO_LOG("RevRes Reverse clone thumbnail transfer finish, targetFileId=%{public}d, moved=%{public}d, "
        "copied=%{public}d, existing=%{public}d, missing=%{public}d, failed=%{public}d, hasLcd=%{public}d, "
        "hasThumb=%{public}d",
        plan.absorbed.fileId, result.thumbnail.stats.movedCount, result.thumbnail.stats.copiedCount,
        result.thumbnail.stats.existingCount, result.thumbnail.stats.missingCount,
        result.thumbnail.stats.failedCount, result.thumbnail.HasLcd(), result.thumbnail.HasThumbnail());
    return E_OK;
}

int64_t GetLocalAssetSize(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    const ResourceExecuteResult &result)
{
    int64_t localAssetSize = 0;
    const ReverseCloneAssetResource &originSource = result.origin.HasSource() ? result.origin.source : plan.donor;
    MovingPhotoFileUtils::GetLocalAssetSize(originSource.effectMode, paths.GetTargetOriginPath(),
        originSource.fingerprint.fileSize, localAssetSize);
    return localAssetSize;
}

PhotoPositionType ResolveAbsorbedPosition(const ReverseCloneResourcePlan &plan)
{
    if (plan.absorbed.position == static_cast<int32_t>(PhotoPositionType::CLOUD) ||
        plan.absorbed.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        return PhotoPositionType::LOCAL_AND_CLOUD;
    }
    return PhotoPositionType::LOCAL;
}

int64_t GetDirectorySize(const std::string &dirPath)
{
    if (dirPath.empty() || !MediaFileUtils::IsDirectory(dirPath)) {
        return 0;
    }
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(dirPath, totalSize);
    return static_cast<int64_t>(totalSize);
}

struct ThumbnailExistence {
    bool hasLcd {false};
    bool hasThumbnail {false};
    bool hasDayAstc {false};

    bool IsReadyForDisplay() const
    {
        return hasThumbnail && hasDayAstc;
    }
};

bool HasAnyFile(const std::string &dirPath, const std::vector<std::string> &relativePaths)
{
    for (const auto &relativePath : relativePaths) {
        if (MediaFileUtils::IsFileExists(dirPath + "/" + relativePath)) {
            return true;
        }
    }
    return false;
}

ThumbnailExistence ResolveThumbnailExistence(const std::string &thumbDir)
{
    ThumbnailExistence existence;
    if (thumbDir.empty() || !MediaFileUtils::IsFileExists(thumbDir)) {
        return existence;
    }
    existence.hasLcd = HasAnyFile(thumbDir, {"LCD.jpg", "THM_EX/LCD.jpg"});
    existence.hasThumbnail = HasAnyFile(thumbDir, {"THM.jpg", "THM_EX/THM.jpg"});
    existence.hasDayAstc = HasAnyFile(thumbDir, {"THM_ASTC.astc"});
    return existence;
}

int32_t ResolveThumbStatus(const ThumbnailExistence &existence)
{
    if (existence.hasLcd && existence.hasThumbnail) {
        return RESTORE_THUMBNAIL_STATUS_ALL;
    }
    if (!existence.hasLcd && existence.hasThumbnail) {
        return RESTORE_THUMBNAIL_STATUS_NOT_LCD;
    }
    if (existence.hasLcd && !existence.hasThumbnail) {
        return RESTORE_THUMBNAIL_STATUS_NOT_THUMB;
    }
    return RESTORE_THUMBNAIL_STATUS_NOT_ALL;
}

bool ShouldRefreshThumbnailFields(const ResourceExecuteResult &result, const ThumbnailExistence &existence,
    bool hasThumbnailPlan)
{
    return result.thumbnail.SourceChanged() || (hasThumbnailPlan && !existence.IsReadyForDisplay());
}

bool ShouldRefreshThumbnailReady(const ResourceExecuteResult &result, const ThumbnailExistence &existence)
{
    return !existence.IsReadyForDisplay() || result.thumbnail.thumb.SourceChanged();
}

int64_t ResolveThumbnailReady(const ResourceExecuteResult &result, const ThumbnailExistence &existence)
{
    return existence.IsReadyForDisplay() ? result.thumbnail.thumb.source.thumbnailReady :
        static_cast<int64_t>(RESTORE_THUMBNAIL_READY_NO_THUMBNAIL);
}

struct DentryCleanupResult {
    int32_t deletedCount {0};
    int32_t missingCount {0};
    int32_t failedCount {0};
};

void DeleteDentryFileIfExists(const std::string &path, DentryCleanupResult &result)
{
    if (path.empty() || !MediaFileUtils::IsFileExists(path)) {
        result.missingCount++;
        return;
    }
    if (MediaFileUtils::DeleteFile(path)) {
        result.deletedCount++;
        return;
    }
    result.failedCount++;
    MEDIA_WARN_LOG("RevRes delete cloud dentry failed, path=%{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
}

void DeleteThumbnailDentryIfTargetExists(const std::string &targetThumbDir, const std::string &thumbDentryDir,
    const std::string &relativePath, DentryCleanupResult &result)
{
    if (!MediaFileUtils::IsFileExists(targetThumbDir + "/" + relativePath)) {
        return;
    }
    DeleteDentryFileIfExists(thumbDentryDir + "/" + relativePath, result);
}

void DeleteAbsorbedCloudDentry(const ReverseCloneResourcePlan &plan, const ResourceExecuteResult &result)
{
    const ReverseCloneAssetResource &target = plan.absorbed;
    if (target.cloudPath.empty()) {
        return;
    }

    DentryCleanupResult cleanupResult;
    if (result.origin.Exists()) {
        DeleteDentryFileIfExists(GetDentryPathByCloudPath(target.cloudPath), cleanupResult);
    }
    ReverseCloneResourceLocator paths(plan);
    std::string targetThumbDir = paths.GetTargetThumbDir();
    std::string thumbDentryDir = GetThumbnailDentryDirByCloudPath(target.cloudPath);
    DeleteThumbnailDentryIfTargetExists(targetThumbDir, thumbDentryDir, "LCD.jpg", cleanupResult);
    DeleteThumbnailDentryIfTargetExists(targetThumbDir, thumbDentryDir, "THM_EX/LCD.jpg", cleanupResult);
    DeleteThumbnailDentryIfTargetExists(targetThumbDir, thumbDentryDir, "THM.jpg", cleanupResult);
    DeleteThumbnailDentryIfTargetExists(targetThumbDir, thumbDentryDir, "THM_EX/THM.jpg", cleanupResult);

    MEDIA_INFO_LOG("RevRes absorbed cloud dentry cleanup finished, targetFileId=%{public}d, "
        "deleted=%{public}d, missing=%{public}d, failed=%{public}d, hasOrigin=%{public}d, hasLcd=%{public}d, "
        "hasThumb=%{public}d",
        target.fileId, cleanupResult.deletedCount, cleanupResult.missingCount, cleanupResult.failedCount,
        result.origin.Exists(), result.thumbnail.HasLcd(), result.thumbnail.HasThumbnail());
}

void AppendSetValue(const std::string &column, const NativeRdb::ValueObject &value,
    std::vector<std::string> &setClauses, std::vector<NativeRdb::ValueObject> &args)
{
    setClauses.emplace_back(column + " = ?");
    args.emplace_back(value);
}

std::string BuildRenamedSourcePath(const ReverseCloneAssetResource &target, const std::string &displayName)
{
    if (target.sourcePath.empty()) {
        return "";
    }
    std::string parentPath = MediaFileUtils::GetParentPath(target.sourcePath);
    return parentPath.empty() ? "" : parentPath + "/" + displayName;
}

void AppendRenamedLakeFieldsIfNeeded(const ReverseCloneAssetResource &target,
    std::vector<std::string> &setClauses, std::vector<NativeRdb::ValueObject> &args)
{
    std::string displayName = MediaFileUtils::GetFileName(target.storagePath);
    if (displayName.empty()) {
        return;
    }
    AppendSetValue(MediaColumn::MEDIA_NAME, NativeRdb::ValueObject(displayName), setClauses, args);
    AppendSetValue(MediaColumn::MEDIA_TITLE,
        NativeRdb::ValueObject(MediaFileUtils::GetTitleFromDisplayName(displayName)), setClauses, args);
    std::string sourcePath = BuildRenamedSourcePath(target, displayName);
    if (!sourcePath.empty()) {
        AppendSetValue(PhotoColumn::PHOTO_SOURCE_PATH, NativeRdb::ValueObject(sourcePath), setClauses, args);
    }
}

void AppendLcdMetadataIfNeeded(const ResourceExecuteResult &result, std::vector<std::string> &setClauses,
    std::vector<NativeRdb::ValueObject> &args)
{
    const ThumbnailSourceResult &lcd = result.thumbnail.lcd;
    if (!lcd.SourceChanged()) {
        return;
    }
    AppendSetValue(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, NativeRdb::ValueObject(lcd.source.realLcdVisitTime),
        setClauses, args);
    AppendSetValue(PhotoColumn::PHOTO_LCD_VISIT_COUNT, NativeRdb::ValueObject(lcd.source.lcdVisitCount),
        setClauses, args);
    if (!lcd.source.lcdSize.empty()) {
        AppendSetValue(PhotoColumn::PHOTO_LCD_SIZE, NativeRdb::ValueObject(lcd.source.lcdSize), setClauses,
            args);
    }
    AppendSetValue(PhotoColumn::PHOTO_LCD_FILE_SIZE, NativeRdb::ValueObject(lcd.source.lcdFileSize),
        setClauses, args);
}

void AppendThumbnailMetadataIfNeeded(const ResourceExecuteResult &result, std::vector<std::string> &setClauses,
    std::vector<NativeRdb::ValueObject> &args)
{
    const ThumbnailSourceResult &thumbnail = result.thumbnail.thumb;
    if (!thumbnail.SourceChanged() || thumbnail.source.thumbSize.empty()) {
        return;
    }
    AppendSetValue(PhotoColumn::PHOTO_THUMB_SIZE, NativeRdb::ValueObject(thumbnail.source.thumbSize),
        setClauses, args);
}

std::string GetFileInode(const std::string &path)
{
    struct stat statInfo {};
    if (path.empty() || stat(path.c_str(), &statInfo) != 0) {
        return "";
    }
    return std::to_string(statInfo.st_ino);
}

void AppendLakeOriginFields(const ReverseCloneResourcePlan &plan, std::vector<std::string> &setClauses,
    std::vector<NativeRdb::ValueObject> &args)
{
    setClauses.emplace_back(PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = ?");
    args.emplace_back(static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    setClauses.emplace_back(PhotoColumn::PHOTO_STORAGE_PATH + " = ?");
    args.emplace_back(plan.absorbed.storagePath);
    std::string inode = GetFileInode(plan.absorbed.storagePath);
    if (!inode.empty()) {
        setClauses.emplace_back(PhotoColumn::PHOTO_FILE_INODE + " = ?");
        args.emplace_back(inode);
    }
    if (plan.lakeTargetRenamed) {
        AppendRenamedLakeFieldsIfNeeded(plan.absorbed, setClauses, args);
    }
    MEDIA_INFO_LOG("RevRes Reverse clone target uses lake origin, fileId=%{public}d, storagePath=%{public}s, "
        "inode=%{public}s",
        plan.absorbed.fileId, MediaFileUtils::DesensitizePath(plan.absorbed.storagePath).c_str(), inode.c_str());
}

void AppendOriginLocationFields(const ReverseCloneResourcePlan &plan, std::vector<std::string> &setClauses,
    std::vector<NativeRdb::ValueObject> &args)
{
    if (ShouldUseLakeStorageAsTarget(plan.absorbed)) {
        AppendLakeOriginFields(plan, setClauses, args);
        return;
    }
    MEDIA_INFO_LOG("RevRes Reverse clone target uses media origin, keep storage fields, fileId=%{public}d, "
        "fileSourceType=%{public}d, inactive=%{public}d",
        plan.absorbed.fileId, plan.absorbed.fileSourceType, IsInactiveAsset(plan.absorbed));
}

void UpdatePhotoExtSizes(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb)
{
    CHECK_AND_RETURN_LOG(targetRdb != nullptr, "RevRes update photo ext sizes skipped, targetRdb is null");
    CHECK_AND_RETURN_LOG(plan.absorbed.fileId > 0,
        "RevRes Reverse clone update photo ext sizes skipped, invalid absorbed fileId");

    int64_t thumbnailSize = GetDirectorySize(paths.GetTargetThumbDir());
    int64_t editDataSize = GetDirectorySize(paths.GetTargetEditDataPath());
    const std::string sql =
        "INSERT INTO tab_photos_ext (photo_id, thumbnail_size, editdata_size) VALUES (?, ?, ?) "
        "ON CONFLICT(photo_id) DO UPDATE SET thumbnail_size = excluded.thumbnail_size, "
        "editdata_size = excluded.editdata_size";
    std::vector<NativeRdb::ValueObject> args = {
        NativeRdb::ValueObject(plan.absorbed.fileId),
        NativeRdb::ValueObject(thumbnailSize),
        NativeRdb::ValueObject(editDataSize),
    };

    int32_t ret = targetRdb->ExecuteSql(sql, args);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "RevRes Reverse clone update photo ext sizes failed, fileId=%{public}d, ret=%{public}d",
        plan.absorbed.fileId, ret);
    MEDIA_INFO_LOG("RevRes Reverse clone photo ext sizes updated, fileId=%{public}d, "
        "thumbnailSize=%{public}lld, editDataSize=%{public}lld",
        plan.absorbed.fileId, static_cast<long long>(thumbnailSize), static_cast<long long>(editDataSize));
}

void AppendOriginFieldsIfNeeded(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    const ResourceExecuteResult &result, std::vector<std::string> &setClauses,
    std::vector<NativeRdb::ValueObject> &args)
{
    if (!result.origin.Exists()) {
        return;
    }
    AppendSetValue(PhotoColumn::PHOTO_POSITION, NativeRdb::ValueObject(static_cast<int32_t>(
        ResolveAbsorbedPosition(plan))), setClauses, args);
    setClauses.emplace_back(PhotoColumn::PHOTO_CLEAN_FLAG + " = 0");
    AppendSetValue(PhotoColumn::LOCAL_ASSET_SIZE, NativeRdb::ValueObject(GetLocalAssetSize(plan, paths, result)),
        setClauses, args);
    AppendOriginLocationFields(plan, setClauses, args);
}

void AppendThumbnailFieldsIfNeeded(const ResourceExecuteResult &result, const ThumbnailExistence &existence,
    bool hasThumbnailPlan, std::vector<std::string> &setClauses, std::vector<NativeRdb::ValueObject> &args)
{
    if (!ShouldRefreshThumbnailFields(result, existence, hasThumbnailPlan)) {
        return;
    }
    AppendSetValue(PhotoColumn::PHOTO_THUMB_STATUS, NativeRdb::ValueObject(ResolveThumbStatus(existence)),
        setClauses, args);
    AppendSetValue(PhotoColumn::PHOTO_LCD_VISIT_TIME,
        NativeRdb::ValueObject(existence.hasLcd ? RESTORE_LCD_VISIT_TIME_SUCCESS : RESTORE_LCD_VISIT_TIME_NO_LCD),
        setClauses, args);
    AppendSetValue(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE,
        NativeRdb::ValueObject(existence.IsReadyForDisplay() ? RESTORE_THUMBNAIL_VISIBLE_TRUE :
            RESTORE_THUMBNAIL_VISIBLE_FALSE),
        setClauses, args);
    if (ShouldRefreshThumbnailReady(result, existence)) {
        AppendSetValue(PhotoColumn::PHOTO_THUMBNAIL_READY,
            NativeRdb::ValueObject(ResolveThumbnailReady(result, existence)), setClauses, args);
    }
}

std::string BuildUpdatePhotosSql(const std::vector<std::string> &setClauses)
{
    std::string sql = "UPDATE Photos SET ";
    for (size_t i = 0; i < setClauses.size(); i++) {
        if (i != 0) {
            sql += ", ";
        }
        sql += setClauses[i];
    }
    return sql + " WHERE " + MediaColumn::MEDIA_ID + " = ?";
}

int32_t UpdateTargetRow(const ReverseCloneResourcePlan &plan, const ReverseCloneResourceLocator &paths,
    const ResourceExecuteResult &result, const std::shared_ptr<NativeRdb::RdbStore> &targetRdb)
{
    CHECK_AND_RETURN_RET(targetRdb != nullptr && plan.absorbed.fileId > 0, E_OK);
    std::vector<std::string> setClauses;
    std::vector<NativeRdb::ValueObject> args;
    AppendOriginFieldsIfNeeded(plan, paths, result, setClauses, args);
    ThumbnailExistence existence = ResolveThumbnailExistence(paths.GetTargetThumbDir());
    AppendThumbnailFieldsIfNeeded(result, existence, plan.inheritLcdThumbnail || plan.inheritThumbnail, setClauses,
        args);
    AppendLcdMetadataIfNeeded(result, setClauses, args);
    AppendThumbnailMetadataIfNeeded(result, setClauses, args);
    if (setClauses.empty()) {
        MEDIA_INFO_LOG("RevRes Reverse clone target row skip update, fileId=%{public}d, hasOrigin=%{public}d, "
            "hasLcd=%{public}d, hasThumb=%{public}d",
            plan.absorbed.fileId, result.origin.Exists(), result.thumbnail.HasLcd(), result.thumbnail.HasThumbnail());
        return E_OK;
    }
    args.emplace_back(plan.absorbed.fileId);
    CHECK_AND_RETURN_RET(targetRdb->ExecuteSql(BuildUpdatePhotosSql(setClauses), args) == NativeRdb::E_OK, E_FAIL);
    MEDIA_INFO_LOG("RevRes Reverse clone target row updated, fileId=%{public}d, hasOrigin=%{public}d, "
        "hasLcd=%{public}d, hasThumb=%{public}d, lcdChanged=%{public}d, thumbChanged=%{public}d, "
        "readyForDisplay=%{public}d, cloudRestoreSatisfied=%{public}d",
        plan.absorbed.fileId, result.origin.Exists(), result.thumbnail.HasLcd(), result.thumbnail.HasThumbnail(),
        result.thumbnail.lcd.SourceChanged(), result.thumbnail.thumb.SourceChanged(), existence.IsReadyForDisplay(),
        plan.cloudRestoreSatisfied);
    return E_OK;
}

} // namespace

int32_t ReverseCloneResourceExecutor::Execute(const ReverseCloneResourcePlan &plan,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
    ReverseRestoreReportInfo &reportInfo) const
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (plan.decision != ReverseCloneResourceDecision::INHERIT || !plan.HasResourceAction()) {
        return E_OK;
    }
    ReverseCloneResourcePlan executePlan = plan;
    CHECK_AND_RETURN_RET_LOG(!executePlan.absorbed.cloudPath.empty(), E_FAIL,
        "RevRes Reverse clone absorbed path is invalid");
    CHECK_AND_RETURN_RET_LOG(HasResourceCandidate(executePlan), E_FAIL,
        "RevRes Reverse clone has no valid resource candidate");
    ReverseCloneResourceTransferSession session;
    if (executePlan.inheritOrigin) {
        CHECK_AND_RETURN_RET(PrepareLakeStorageTarget(executePlan) == E_OK, E_FAIL);
    }
    ReverseCloneResourceLocator paths(executePlan);
    if (executePlan.inheritOrigin) {
        CHECK_AND_RETURN_RET(TransferOrigin(executePlan, paths, session) == E_OK, E_FAIL);
    }
    const ResourceExecuteResult &result = session.Result();
    if (result.origin.HasSource() && HasMovingPhotoVideo(result.origin.source)) {
        CHECK_AND_RETURN_RET(TransferMovingPhotoVideo(paths, result.origin.source, session) == E_OK, E_FAIL);
    }
    if (result.origin.HasSource()) {
        CHECK_AND_RETURN_RET(TransferEditData(paths, result.origin.source, session) == E_OK, E_FAIL);
    }
    if (executePlan.inheritLcdThumbnail || executePlan.inheritThumbnail) {
        CHECK_AND_RETURN_RET(TransferThumbnailFiles(executePlan, paths, session) == E_OK, E_FAIL);
    }
    CHECK_AND_RETURN_RET(UpdateTargetRow(executePlan, paths, session.Result(), targetRdb) == E_OK, E_FAIL);
    UpdatePhotoExtSizes(executePlan, paths, targetRdb);
    DeleteAbsorbedCloudDentry(executePlan, session.Result());
    session.CleanTransferredSourcePaths();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reportInfo.afterTransformTimeCost.append(" absorb origin&thumbnail: ")
        .append(std::to_string(endTime - startTime) + ";");
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
