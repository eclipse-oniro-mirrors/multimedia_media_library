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

#define MLOG_TAG "FileManagerOfflineCleanup"

#include "media_file_manager_offline_cleanup_task.h"

#include <algorithm>
#include <cinttypes>
#include <cctype>
#include <limits>
#include <sstream>
#include <vector>

#include "file_scan_utils.h"
#include "hi_audit.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_string_utils.h"
#include "photo_owner_album_id_operation.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "userfile_manager_types.h"

namespace OHOS::Media::Background {
namespace {
constexpr char OFFLINE_CLEANUP_PREFERENCES[] = "/data/storage/el2/base/preferences/task_progress.xml";
constexpr int32_t OFFLINE_CLEANUP_VERSION = 1;
constexpr int32_t BATCH_SIZE = 100;
constexpr int32_t SUBTYPE_BURST = static_cast<int32_t>(PhotoSubType::BURST);
constexpr int32_t BURST_COVER = static_cast<int32_t>(BurstCoverLevelType::COVER);
constexpr int32_t ALBUM_DIRTY_DELETED = static_cast<int32_t>(DirtyType::TYPE_DELETED);
constexpr int32_t MAX_ALBUM_NAME_SEQUENCE = 1000;

const std::string KEY_TASK_VERSION = "offline_cleanup_version";
const std::string KEY_LOCAL_DELETE_CURSOR = "fm_local_delete_cursor";
const std::string KEY_FILE_DELETE_CURSOR = "fm_file_delete_cursor";
const std::string KEY_BURST_CONVERT_CURSOR = "fm_burst_convert_cursor";
const std::string KEY_LOCAL_CLOUD_CURSOR = "fm_local_cloud_cursor";
const std::string KEY_CLOUD_ONLY_CURSOR = "fm_cloud_only_cursor";
const std::string KEY_ALBUM_MIGRATE_CURSOR = "fm_album_migrate_cursor";

const std::string LEGACY_ALBUM_PREFIX = "/FromDocs/";
const std::string TARGET_SOURCE_PREFIX = "/storage/emulated/0";
const std::string DENTRY_INFO_ORIGIN = "CONTENT";

std::shared_ptr<NativePreferences::Preferences> GetCleanupPrefs()
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(OFFLINE_CLEANUP_PREFERENCES, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, nullptr, "Get preferences failed, errCode: %{public}d", errCode);
    return prefs;
}

std::string FindLPath(const std::string &lpath, const std::string &sourcePath)
{
    CHECK_AND_RETURN_RET(lpath.empty(), lpath);
    PhotoOwnerAlbumIdOperation albumOperation;
    return albumOperation.ParseSourcePathToLPath(sourcePath);
}

std::string FindAlbumName(const std::string &albumName, const std::string &lpath)
{
    CHECK_AND_RETURN_RET(albumName.empty(), albumName);
    size_t index = lpath.find_last_of("/");
    return index != std::string::npos ? lpath.substr(index + 1) : "";
}

std::string ConvertLegacyAlbumLpath(const std::string &legacyLpath)
{
    if (!MediaStringUtils::StartsWithIgnoreCase(legacyLpath, LEGACY_ALBUM_PREFIX)) {
        return legacyLpath;
    }
    if (MediaStringUtils::EqualToIgnoreCase(legacyLpath, LEGACY_ALBUM_PREFIX)) {
        return "/";
    }
    return legacyLpath.substr(LEGACY_ALBUM_PREFIX.size() - 1);
}

std::string BuildTargetSourcePath(const OfflineCleanupPhotoRecord &photo, const std::string &targetLpath)
{
    const std::string fileName = !photo.displayName.empty() ? photo.displayName :
        MediaFileUtils::GetFileName(photo.sourcePath);
    CHECK_AND_RETURN_RET(!fileName.empty(), photo.sourcePath);
    const std::string normalizedLpath = ConvertLegacyAlbumLpath(targetLpath);
    return normalizedLpath == "/" ? TARGET_SOURCE_PREFIX + "/" + fileName :
        TARGET_SOURCE_PREFIX + normalizedLpath + "/" + fileName;
}

bool SaveIntPref(const std::string &key, int32_t value)
{
    auto prefs = GetCleanupPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, false, "prefs is nullptr, key=%{public}s, value=%{public}d",
        key.c_str(), value);
    prefs->PutInt(key, value);
    prefs->FlushSync();
    return true;
}
}  // namespace

bool MediaFileManagerOfflineCleanupTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaFileManagerOfflineCleanupTask::Execute()
{
    ResetRunState();
    PrepareProgress();

    ProcessLocalPhotosToDelete();
    CHECK_AND_RETURN(Accept());

    ConvertBurstCoverPhotos();
    CHECK_AND_RETURN(Accept());

    ConvertLocalCloudPhotos();
    CHECK_AND_RETURN(Accept());

    ConvertCloudOnlyPhotos();
    CHECK_AND_RETURN(Accept());

    MigratePhotoAlbumRelations();
    CHECK_AND_RETURN(Accept());

    CleanupLegacyAlbums();
    CHECK_AND_RETURN(Accept());

    ReportCleanupResult();
}

void MediaFileManagerOfflineCleanupTask::ResetRunState()
{
    statistics_ = {};
    targetAlbumIdCache_.clear();
    assetRefresh_ = AccurateRefresh::AssetAccurateRefresh();
    albumRefresh_ = AccurateRefresh::AlbumAccurateRefresh();
    MEDIA_INFO_LOG("Offline cleanup run state reset");
}

void MediaFileManagerOfflineCleanupTask::PrepareProgress()
{
    CHECK_AND_RETURN(GetSavedTaskVersion() != OFFLINE_CLEANUP_VERSION);
    ResetAllCursors();
    SaveTaskVersion(OFFLINE_CLEANUP_VERSION);
}

void MediaFileManagerOfflineCleanupTask::ProcessLocalPhotosToDelete()
{
    int32_t lastFileId = LoadCursor(KEY_LOCAL_DELETE_CURSOR);
    auto &result = statistics_.markedForDeletion;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryLocalDeleteCandidates(lastFileId, BATCH_SIZE);
        CHECK_AND_BREAK(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        std::vector<int32_t> fileIdsToMark;
        for (const auto &photo : photos) {
            CHECK_AND_CONTINUE(ShouldMarkForDeletion(photo));
            fileIdsToMark.emplace_back(photo.fileId);
        }
        int32_t processedCount = 0;
        CHECK_AND_RETURN_LOG(cleanupDao_.MarkPhotosForOfflineCleanup(fileIdsToMark, assetRefresh_, processedCount),
            "Mark photos for offline cleanup failed");
        result.count += processedCount;
        result.endId = batchLastId;
        RefreshAssets();
        LogBatchResult("mark_local_delete", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_LOCAL_DELETE_CURSOR, lastFileId);
    }
    CleanupPendingDeletedPhotos();
}

void MediaFileManagerOfflineCleanupTask::CleanupPendingDeletedPhotos()
{
    int32_t lastFileId = LoadCursor(KEY_FILE_DELETE_CURSOR);
    auto &result = statistics_.deletedPhotos;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryPendingDeletedPhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_RETURN(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        std::vector<std::string> ids;
        std::vector<std::string> paths;
        std::vector<std::string> dateTakens;
        std::vector<int32_t> subtypes;
        std::vector<int32_t> effectModes;
        for (const auto &photo : photos) {
            ids.emplace_back(std::to_string(photo.fileId));
            paths.emplace_back(photo.data);
            dateTakens.emplace_back(std::to_string(photo.dateTaken));
            subtypes.emplace_back(photo.subtype);
            effectModes.emplace_back(photo.effectMode);
        }
        MediaLibraryAssetOperations::CleanupMediaAssets(ids, paths, dateTakens, subtypes, effectModes);
        int32_t processedCount = 0;
        CHECK_AND_RETURN_LOG(cleanupDao_.DeleteOfflineCleanupPhotos(ids, assetRefresh_, processedCount),
            "Delete offline cleanup photos failed");

        for (const auto &photo : photos) {
            WriteDeleteAuditLog(photo, processedCount);
        }
        result.count += processedCount;
        result.endId = batchLastId;
        LogBatchResult("delete_local_photos", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_FILE_DELETE_CURSOR, lastFileId);
    }
    RefreshAssets();
}

void MediaFileManagerOfflineCleanupTask::ConvertBurstCoverPhotos()
{
    int32_t lastFileId = LoadCursor(KEY_BURST_CONVERT_CURSOR);
    auto &result = statistics_.burstConverted;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryBurstCoverPhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_RETURN(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        int64_t processedCount = 0;
        for (const auto &photo : photos) {
            CHECK_AND_CONTINUE(ShouldConvertToMediaBurstCover(photo));
            processedCount += ConvertBurstCoverPhoto(photo) ? 1 : 0;
        }
        result.count += processedCount;
        result.endId = batchLastId;
        RefreshAssets();
        LogBatchResult("convert_burst_cover", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_BURST_CONVERT_CURSOR, lastFileId);
    }
}

void MediaFileManagerOfflineCleanupTask::ConvertLocalCloudPhotos()
{
    int32_t lastFileId = LoadCursor(KEY_LOCAL_CLOUD_CURSOR);
    auto &result = statistics_.localCloudConverted;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryLocalCloudPhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_RETURN(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        int64_t processedCount = 0;
        for (const auto &photo : photos) {
            processedCount += ConvertLocalCloudPhoto(photo) ? 1 : 0;
        }
        result.count += processedCount;
        result.endId = batchLastId;
        RefreshAssets();
        LogBatchResult("convert_local_cloud", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_LOCAL_CLOUD_CURSOR, lastFileId);
    }
}

void MediaFileManagerOfflineCleanupTask::ConvertCloudOnlyPhotos()
{
    int32_t lastFileId = LoadCursor(KEY_CLOUD_ONLY_CURSOR);
    auto &result = statistics_.cloudOnlyConverted;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryCloudOnlyPhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_RETURN(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        std::vector<int32_t> fileIds;
        for (const auto &photo : photos) {
            fileIds.emplace_back(photo.fileId);
        }
        CHECK_AND_RETURN_LOG(cleanupDao_.UpdateCloudOnlyPhotos(fileIds, assetRefresh_),
            "Update cloud-only photos failed");
        const int64_t processedCount = static_cast<int64_t>(fileIds.size());
        result.count += processedCount;
        result.endId = batchLastId;
        RefreshAssets();
        LogBatchResult("convert_cloud_only", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_CLOUD_ONLY_CURSOR, lastFileId);
    }
}

void MediaFileManagerOfflineCleanupTask::MigratePhotoAlbumRelations()
{
    int32_t lastFileId = LoadCursor(KEY_ALBUM_MIGRATE_CURSOR);
    auto &result = statistics_.albumRelationsMigrated;
    result.startId = lastFileId;
    result.endId = lastFileId;
    while (Accept()) {
        auto photos = cleanupDao_.QueryLegacyAlbumPhotos(lastFileId, BATCH_SIZE);
        CHECK_AND_RETURN(!photos.empty());

        const int32_t batchLastId = photos.back().fileId;
        int64_t processedCount = 0;
        for (const auto &photo : photos) {
            OfflineCleanupAlbumRecord sourceAlbum;
            sourceAlbum.albumId = photo.ownerAlbumId;
            sourceAlbum.albumSubtype = photo.albumSubtype;
            sourceAlbum.lpath = FindLPath(photo.albumLpath, photo.sourcePath);
            sourceAlbum.albumName = FindAlbumName(photo.albumName, sourceAlbum.lpath);
            const std::string targetLpath = ConvertLegacyAlbumLpath(sourceAlbum.lpath);
            const int32_t targetAlbumId = EnsureTargetAlbum(sourceAlbum);
            CHECK_AND_CONTINUE_ERR_LOG(targetAlbumId > 0, "EnsureTargetAlbum failed, %{public}s",
                sourceAlbum.ToString().c_str());
            processedCount += cleanupDao_.UpdatePhotoAlbumRelation(photo.fileId, photo.ownerAlbumId, targetAlbumId,
                BuildTargetSourcePath(photo, targetLpath), assetRefresh_) ? 1 : 0;
        }
        result.count += processedCount;
        result.endId = batchLastId;
        RefreshAssets();
        LogBatchResult("migrate_album_relation", lastFileId, batchLastId, photos.size(), processedCount);

        lastFileId = batchLastId;
        SaveCursor(KEY_ALBUM_MIGRATE_CURSOR, lastFileId);
    }
}

void MediaFileManagerOfflineCleanupTask::CleanupLegacyAlbums()
{
    int32_t lastAlbumId = 0;
    auto &result = statistics_.legacyAlbumsDeleted;
    result.startId = lastAlbumId;
    result.endId = lastAlbumId;
    while (Accept()) {
        auto albums = cleanupDao_.QueryEmptyLegacyAlbums(lastAlbumId, BATCH_SIZE);
        CHECK_AND_RETURN(!albums.empty());
        const int32_t batchLastId = albums.back().albumId;
        std::vector<int32_t> albumIds;
        for (const auto &album : albums) {
            albumIds.emplace_back(album.albumId);
        }
        int32_t deletedCount = 0;
        CHECK_AND_RETURN_LOG(cleanupDao_.LogicalDeleteEmptyLegacyAlbums(albumIds, albumRefresh_, deletedCount),
            "Cleanup legacy albums failed");
        const int64_t processedCount = deletedCount;
        result.count += processedCount;
        result.endId = batchLastId;
        albumRefresh_.Notify();
        for (const auto &album : albums) {
            WriteAlbumDeleteAuditLog(album, deletedCount);
        }
        LogBatchResult("delete_legacy_albums", lastAlbumId, batchLastId, albums.size(), processedCount);

        lastAlbumId = batchLastId;
    }
}

void MediaFileManagerOfflineCleanupTask::ReportCleanupResult()
{
    const int64_t remainLegacyPhotos = cleanupDao_.CountLegacyPhotos();
    const int64_t remainTombstone = cleanupDao_.CountPendingDeletedPhotos();
    const int64_t remainLegacyAlbums = cleanupDao_.CountLegacyAlbums();
    MEDIA_INFO_LOG("markedForDeletion=%{public}" PRId64
        ", deletedPhotos=%{public}" PRId64 ", burstConverted=%{public}" PRId64
        ", localCloudConverted=%{public}" PRId64 ", cloudOnlyConverted=%{public}" PRId64
        ", albumRelationsMigrated=%{public}" PRId64 ", legacyAlbumsDeleted=%{public}" PRId64
        ", remainLegacyPhotos=%{public}" PRId64 ", remainTombstone=%{public}" PRId64
        ", remainLegacyAlbums=%{public}" PRId64,
        statistics_.markedForDeletion, statistics_.deletedPhotos, statistics_.burstConverted,
        statistics_.localCloudConverted, statistics_.cloudOnlyConverted, statistics_.albumRelationsMigrated,
        statistics_.legacyAlbumsDeleted, remainLegacyPhotos, remainTombstone, remainLegacyAlbums);

    const int64_t totalCount = statistics_.markedForDeletion.count + statistics_.deletedPhotos.count +
        statistics_.burstConverted.count + statistics_.localCloudConverted.count +
        statistics_.cloudOnlyConverted.count + statistics_.albumRelationsMigrated.count +
        statistics_.legacyAlbumsDeleted.count;
    CHECK_AND_RETURN(totalCount > 0);
    const std::vector<std::string> results = {
        statistics_.markedForDeletion.ToString(),
        statistics_.deletedPhotos.ToString(),
        statistics_.burstConverted.ToString(),
        statistics_.localCloudConverted.ToString(),
        statistics_.cloudOnlyConverted.ToString(),
        statistics_.albumRelationsMigrated.ToString(),
        statistics_.legacyAlbumsDeleted.ToString(),
    };
    std::ostringstream ss;
    ss << "stat:";
    for (size_t i = 0; i < results.size(); ++i) {
        ss << (i > 0 ? "|" : "") << results[i];
    }
    ss << ";remain:photo[" << remainLegacyPhotos << "]|tombstone[" << remainTombstone << "]|album["
        << remainLegacyAlbums << "];";

    AuditLog auditLog = {false, "FM_OFFLINE", "RESULT", "SUMMARY", 1, "success", ss.str()};
    HiAudit::GetInstance().Write(auditLog, false);
}

void MediaFileManagerOfflineCleanupTask::WriteDeleteAuditLog(const OfflineCleanupPhotoRecord &photo, int32_t totalCount)
{
    CHECK_AND_RETURN(totalCount > 0);
    AuditLog auditLog = {false, "FM_OFFLINE", "DELETE", "PHOTO", 1, "success", "ok"};
    auditLog.id = std::to_string(photo.fileId);
    auditLog.type = photo.mediaType;
    auditLog.size = totalCount;
    auditLog.path = FileScanUtils::GarbleFilePath(photo.storagePath);
    HiAudit::GetInstance().Write(auditLog, false);
}

void MediaFileManagerOfflineCleanupTask::WriteAlbumDeleteAuditLog(const OfflineCleanupAlbumRecord &album,
    int32_t totalCount)
{
    CHECK_AND_RETURN(totalCount > 0);
    AuditLog auditLog = {false, "FM_OFFLINE", "DELETE", "ALBUM", 1, "success", "ok"};
    auditLog.id = std::to_string(album.albumId);
    auditLog.type = album.albumSubtype;
    auditLog.size = totalCount;
    auditLog.path = FileScanUtils::GarbleFilePath(album.lpath);
    HiAudit::GetInstance().Write(auditLog, false);
}

void MediaFileManagerOfflineCleanupTask::LogBatchResult(const char *stage, int32_t startCursor, int32_t endCursor,
    size_t scannedCount, int64_t processedCount) const
{
    MEDIA_INFO_LOG("stage=%{public}s, cursor=%{public}d -> %{public}d, scanned=%{public}zu, processed=%{public}" PRId64,
        stage, startCursor, endCursor, scannedCount, processedCount);
}

bool MediaFileManagerOfflineCleanupTask::ShouldMarkForDeletion(const OfflineCleanupPhotoRecord &photo)
{
    CHECK_AND_RETURN_RET_LOG(!photo.storagePath.empty(), false,
        "File storagePath empty, %{public}s", photo.ToString().c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(photo.storagePath), false,
        "File not exist, path: %{public}s", photo.ToString().c_str());
    CHECK_AND_RETURN_RET_INFO_LOG(!cleanupDao_.ExistMediaBurstMember(photo), false,
        "Skip burst cover, path: %{public}s", photo.ToString().c_str());
    return true;
}

bool MediaFileManagerOfflineCleanupTask::ShouldConvertToMediaBurstCover(const OfflineCleanupPhotoRecord &photo)
{
    CHECK_AND_RETURN_RET_LOG(!photo.storagePath.empty(), false,
        "File storagePath empty, %{public}s", photo.ToString().c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(photo.storagePath), false,
        "File not exist, path: %{public}s", photo.ToString().c_str());
    CHECK_AND_RETURN_RET(cleanupDao_.ExistMediaBurstMember(photo), false);
    return true;
}

bool MediaFileManagerOfflineCleanupTask::ConvertBurstCoverPhoto(const OfflineCleanupPhotoRecord &photo)
{
    std::string targetPath = MediaPathUtils::ConvertCloudPathToLocalPath(photo.data);
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), false, "Get targetPath empty, %{public}s", photo.ToString().c_str());
    std::string targetParentPath = MediaFileUtils::GetParentPath(targetPath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(targetParentPath), false,
        "Create dir %{public}s failed, %{public}s",
        MediaFileUtils::DesensitizePath(targetParentPath).c_str(), photo.ToString().c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileUtil(photo.storagePath, targetPath), false,
        "Copy failed from %{public}s to %{public}s", MediaFileUtils::DesensitizePath(photo.storagePath).c_str(),
        MediaFileUtils::DesensitizePath(targetPath).c_str());
    MediaFileUtils::UpdateModifyTimeInMsec(targetPath, photo.dateModified);
    CHECK_AND_RETURN_RET_LOG(cleanupDao_.UpdateBurstCoverPhoto(photo, assetRefresh_), false,
        "Update burst cover photo failed, %{public}s", photo.ToString().c_str());
    return true;
}

bool MediaFileManagerOfflineCleanupTask::ConvertLocalCloudPhoto(const OfflineCleanupPhotoRecord &photo)
{
    CHECK_AND_RETURN_RET_LOG(CreateOriginDentry(photo), false, "Create origin dentry failed, %{public}s",
        photo.ToString().c_str());
    CHECK_AND_RETURN_RET_LOG(cleanupDao_.UpdateLocalCloudPhoto(photo, assetRefresh_), false,
        "Update local+cloud photo failed, %{public}s", photo.ToString().c_str());
    return true;
}

bool MediaFileManagerOfflineCleanupTask::CreateOriginDentry(const OfflineCleanupPhotoRecord &photo)
{
    CHECK_AND_RETURN_RET_LOG(!photo.cloudId.empty(), false, "cloudId is empty");
    FileManagement::CloudSync::DentryFileInfo dentryInfo;
    dentryInfo.cloudId = photo.cloudId;
    dentryInfo.modifiedTime = photo.dateModified;
    dentryInfo.fileType = DENTRY_INFO_ORIGIN;
    dentryInfo.size = photo.size;
    dentryInfo.path = photo.data;
    dentryInfo.fileName = photo.displayName;
    std::vector<std::string> failCloudIds;
    const int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().BatchDentryFileInsert(
        {dentryInfo}, failCloudIds);
    return ret == E_OK && failCloudIds.empty();
}

int32_t MediaFileManagerOfflineCleanupTask::EnsureTargetAlbum(const OfflineCleanupAlbumRecord &sourceAlbum)
{
    const std::string targetLpath = ConvertLegacyAlbumLpath(sourceAlbum.lpath);
    const std::string targetLpathLower = MediaStringUtils::ToLower(targetLpath);
    auto it = targetAlbumIdCache_.find(targetLpathLower);
    CHECK_AND_RETURN_RET(it == targetAlbumIdCache_.end(), it->second);

    OfflineCleanupAlbumRecord targetAlbum;
    if (cleanupDao_.QueryAlbumByLpath(targetLpath, targetAlbum)) {
        CHECK_AND_RETURN_RET_LOG(targetAlbum.dirty != ALBUM_DIRTY_DELETED ||
            cleanupDao_.RenewDeletedAlbum(targetAlbum.albumId, albumRefresh_), 0,
            "RenewDeletedAlbum failed, %{public}s", sourceAlbum.ToString().c_str());
        targetAlbumIdCache_[targetLpathLower] = targetAlbum.albumId;
        return targetAlbum.albumId;
    }

    PhotoOwnerAlbumIdOperation albumOperation;
    PhotoOwnerAlbumIdOperation::MediaData albumInfo;
    albumOperation.GetAlbumTypeAndSubType(targetLpath, albumInfo.albumType, albumInfo.albumSubType);
    albumInfo.albumName = ResolveTargetAlbumName(sourceAlbum);
    albumInfo.lPath = targetLpath;
    const int32_t albumId = albumOperation.CreateAlbumAndGetId(albumInfo);
    CHECK_AND_RETURN_RET(albumId > 0, 0);
    targetAlbumIdCache_[targetLpathLower] = albumId;
    MEDIA_INFO_LOG("Album[%{public}d, %{public}s] created", albumId,
        FileScanUtils::GarbleFilePath(targetLpath).c_str());
    return albumId;
}

std::string MediaFileManagerOfflineCleanupTask::ResolveTargetAlbumName(const OfflineCleanupAlbumRecord &sourceAlbum)
{
    std::string albumName = sourceAlbum.albumName;
    CHECK_AND_RETURN_RET(!albumName.empty() && cleanupDao_.IsAlbumNameOccupied(albumName), albumName);

    for (int32_t sequence = 1; sequence < MAX_ALBUM_NAME_SEQUENCE; ++sequence) {
        std::string candidate = sourceAlbum.albumName + " " + std::to_string(sequence);
        if (!cleanupDao_.IsAlbumNameOccupied(candidate)) {
            MEDIA_INFO_LOG("albumName %{public}s -> %{public}s", albumName.c_str(), candidate.c_str());
            return candidate;
        }
    }
    return albumName;
}

int32_t MediaFileManagerOfflineCleanupTask::GetSavedTaskVersion()
{
    auto prefs = GetCleanupPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, 0, "prefs is nullptr");
    return prefs->GetInt(KEY_TASK_VERSION, 0);
}

void MediaFileManagerOfflineCleanupTask::SaveTaskVersion(int32_t version)
{
    SaveIntPref(KEY_TASK_VERSION, version);
}

int32_t MediaFileManagerOfflineCleanupTask::LoadCursor(const std::string &key)
{
    auto prefs = GetCleanupPrefs();
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, 0, "prefs is nullptr");
    return prefs->GetInt(key, 0);
}

void MediaFileManagerOfflineCleanupTask::SaveCursor(const std::string &key, int32_t value)
{
    SaveIntPref(key, value);
}

void MediaFileManagerOfflineCleanupTask::ResetAllCursors()
{
    SaveCursor(KEY_LOCAL_DELETE_CURSOR, 0);
    SaveCursor(KEY_FILE_DELETE_CURSOR, 0);
    SaveCursor(KEY_BURST_CONVERT_CURSOR, 0);
    SaveCursor(KEY_LOCAL_CLOUD_CURSOR, 0);
    SaveCursor(KEY_CLOUD_ONLY_CURSOR, 0);
    SaveCursor(KEY_ALBUM_MIGRATE_CURSOR, 0);
}

void MediaFileManagerOfflineCleanupTask::RefreshAssets()
{
    assetRefresh_.RefreshAlbum();
    assetRefresh_.Notify();
}
}  // namespace OHOS::Media::Background
