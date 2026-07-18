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
#define MLOG_TAG "Media_Reverse_Restore"

#include "reverse_clone_restore.h"

#include "backup_database_utils.h"
#include "medialibrary_db_const.h"
#include "backup_file_utils.h"
#include "media_file_utils.h"
#include "rdb_helper.h"
#include "backup_dfx_utils.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "application_context.h"
#include "cloud_data_cleaner.h"
#include "file_id_migrator.h"
#include "media_library_upgrade_manager.h"
#include "vision_column.h"
#include "clone_reverse_restore_classify.h"
#include "clone_reverse_restore_geo.h"
#include "clone_restore_highlight.h"
#include "clone_restore_cv_analysis.h"
#include "cloud_media_asset_manager.h"
#include "media_library_db_upgrade.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "rdb_predicates.h"
#include "reverse_clone_resource_inherit_helper.h"
#include "anco_reverse_clone_adapter.h"
#include "photo_file_utils.h"
#include "medialibrary_type_const.h"
#include "reverse_clone_resource_inherit_service.h"
#include "database_report.h"
#include "clone_restore_portrait_album.h"
#include "clone_restore_group_photo.h"
#include "search_index_clone.h"
#include "portrait_nickname_clone.h"
#include "ai_retouch_clone.h"
#include "settings_data_manager.h"
#include "clone_restore_dup_sim.h"
#include "clone_restore_selection.h"
#include "lcd_aging_service.h"
#include "reverse_clone_restore_marker.h"
#include "restore_map_code_utils.h"
#include "photo_map_code_operation.h"
#include "medialibrary_tab_asset_and_album_operations.h"
#include "photo_album_upload_status_operation.h"
#include "reverse_clone_reliability_marker.h"
#include "photo_count_strategy.h"
#include "photo_count_context.h"
#include "shooting_mode_album_clone.h"
#include "media_time_utils.h"
#include "userfile_manager_types.h"
#include "parameters.h"

#include <cstring>
#include <dirent.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <future>
#include <chrono>
#include <thread>

namespace OHOS {
namespace Media {

// --- rename failure diagnostic helpers ---
struct DirCloser {
    void operator()(DIR *d) const
    {
        if (d) {
            closedir(d);
        }
    }
};
using UniqueDir = std::unique_ptr<DIR, DirCloser>;

struct PathStat {
    struct stat st{};
    bool ok;
    int err;
    explicit PathStat(const std::string &path) : ok(stat(path.c_str(), &st) == 0), err(errno) {}
    bool IsDir() const { return ok && S_ISDIR(st.st_mode); }
    bool IsReg() const { return ok && S_ISREG(st.st_mode); }
    const char *Type() const
    {
        if (!ok) {
            return "N/A";
        }

        if (S_ISDIR(st.st_mode)) {
            return "DIR";
        }
            
        if (S_ISREG(st.st_mode)) {
            return "REG";
        }
        return "OTHER";
    }
};

static void LogPathStat(const char *label, const PathStat &ps)
{
    if (!ps.ok) {
        MEDIA_ERR_LOG("  %{public}s: stat failed errno=%{public}d (%{public}s)",
            label, ps.err, strerror(ps.err));
        return;
    }
    MEDIA_ERR_LOG("  %{public}s: type=%{public}s dev=%{public}llu "
        "mode=0%{public}o uid=%{public}u gid=%{public}u",
        label, ps.Type(), static_cast<unsigned long long>(ps.st.st_dev),
        ps.st.st_mode & 07777,
        static_cast<unsigned>(ps.st.st_uid % 1000U), static_cast<unsigned>(ps.st.st_gid % 1000U));
}

static void CheckTypeConflict(const PathStat &srcPs, const PathStat &dstPs)
{
    if (!srcPs.ok || !dstPs.ok) {
        return;
    }
    bool dirVsFile = srcPs.IsDir() && dstPs.IsReg();
    bool fileVsDir = srcPs.IsReg() && dstPs.IsDir();
    if (dirVsFile || fileVsDir) {
        MEDIA_ERR_LOG("  TYPE CONFLICT: src=%{public}s dst=%{public}s -> %{public}s",
            srcPs.Type(), dstPs.Type(), dirVsFile ? "ENOTDIR" : "EISDIR");
    }
}

static void CheckNonEmptyDir(const PathStat &dstPs, const std::string &dst)
{
    if (!dstPs.IsDir()) {
        return;
    }
    UniqueDir d(opendir(dst.c_str()));
    if (!d) {
        return;
    }
    int count = 0;
    while (struct dirent *e = readdir(d.get())) {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0) {
            ++count;
        }
    }
    if (count > 0) {
        MEDIA_ERR_LOG("  dst dir NOT EMPTY (%{public}d entries) -> ENOTEMPTY", count);
    }
}

static void CheckParentWritable(const PathStat &parentPs, const std::string &parent)
{
    if (!parentPs.ok) {
        MEDIA_ERR_LOG("  dst parent stat failed: %{public}s errno=%{public}d",
            parent.c_str(), parentPs.err);
        return;
    }
    if (access(parent.c_str(), W_OK) != 0) {
        MEDIA_ERR_LOG("  dst parent NOT writable: errno=%{public}d (%{public}s)",
            errno, strerror(errno));
    }
}

static void CheckCrossDevice(const PathStat &srcParentPs, const PathStat &dstParentPs, int savedErrno)
{
    if (savedErrno == EXDEV) {
        MEDIA_ERR_LOG("  EXDEV: src/dst on different filesystems");
    }
    if (srcParentPs.ok && dstParentPs.ok &&
        srcParentPs.st.st_dev != dstParentPs.st.st_dev) {
        MEDIA_ERR_LOG("  cross-device: src dev=%{public}llu != dst dev=%{public}llu",
            static_cast<unsigned long long>(srcParentPs.st.st_dev),
            static_cast<unsigned long long>(dstParentPs.st.st_dev));
    }
}

static void CheckMountPoint(const PathStat &ps, const PathStat &parentPs, const char *label)
{
    if (!ps.ok || !parentPs.ok) {
        return;
    }
    if (ps.st.st_dev != parentPs.st.st_dev) {
        MEDIA_ERR_LOG("  %{public}s is a MOUNT POINT (dev=%{public}llu != parent=%{public}llu) -> EBUSY",
            label, static_cast<unsigned long long>(ps.st.st_dev), static_cast<unsigned long long>(parentPs.st.st_dev));
    }
}

static void DiagnoseRenameFailure(const std::string &src, const std::string &dst, int savedErrno)
{
    MEDIA_ERR_LOG("DiagnoseRename: rename(%{public}s,%{public}s) errno=%{public}d (%{public}s)",
        src.c_str(), dst.c_str(), savedErrno, strerror(savedErrno));
    PathStat srcPs(src);
    PathStat dstPs(dst);
    LogPathStat("src", srcPs);
    LogPathStat("dst", dstPs);
    CheckTypeConflict(srcPs, dstPs);
    CheckNonEmptyDir(dstPs, dst);
    auto dstParent = dst.substr(0, dst.find_last_of('/'));
    auto srcParent = src.substr(0, src.find_last_of('/'));
    PathStat dstParentPs(dstParent);
    PathStat srcParentPs(srcParent);
    LogPathStat("dstParent", dstParentPs);
    CheckParentWritable(dstParentPs, dstParent);
    CheckCrossDevice(srcParentPs, dstParentPs, savedErrno);
    CheckMountPoint(srcPs, srcParentPs, "src");
    CheckMountPoint(dstPs, dstParentPs, "dst");
}

const int32_t CLONE_QUERY_COUNT = 200;
const int32_t RELEATED_TO_PHOTO_MAP = 1;

static const std::vector<std::string> CLONE_ALBUMS = { PhotoAlbumColumns::TABLE };

static const std::string MEDIA_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
static const std::string MEDIA_RDB_DIR = "/data/storage/el2/database/rdb";
static const std::vector<std::string> DB_FILE_SUFFIXES = {"", "-compare", "-dwr", "-shm", "-wal", "_binlog"};
static constexpr int64_t DB_SPACE_RESERVE_BYTES = 128LL * 1024 * 1024;
static constexpr int64_t DB_SPACE_FACTOR_NUMERATOR = 3;
static constexpr int64_t DB_SPACE_FACTOR_DENOMINATOR = 2;
static constexpr int32_t BACKUP_OPER_TYPE = 3;
static constexpr const int64_t ERR_SQLITE_CORRUPT = 27394104;
static constexpr const char *STAGE_PREPARE_OLD_DB = "PrepareOldDb";
static constexpr const char *STAGE_UPGRADE_OLD_DB = "DoDataBaseUpgrade";
static constexpr const char *STAGE_INITIAL_MIGRATION = "PerformInitialMigration";
static constexpr const char *STAGE_BACKUP_NEW_DB = "BackupAndRenameNewDb";

static const std::string SQL_QUERY_CLASSIFY_ALBUM_EXIST_REVERSE = " \
    SELECT \
        count(1) AS count \
    FROM AnalysisAlbum \
    WHERE \
        album_type = ? \
    AND \
        album_subtype = ?;";

// 反向克隆：查询所有照片的 SQL 语句
const std::string ReverseCloneRestore::SQL_PHOTOS_TABLE_QUERY_ALL = "\
    SELECT \
        PhotoAlbum.lpath, \
        Photos.* \
    FROM Photos \
        LEFT JOIN PhotoAlbum \
        ON Photos.owner_album_id=PhotoAlbum.album_id \
    WHERE position IN (1, 3) AND \
        COALESCE(Photos.clean_flag, 0) = 0 AND \
        COALESCE(Photos.sync_status, 0) = 0 \
    ORDER BY Photos.file_id \
    LIMIT ?, ? ;";

const std::string ReverseCloneRestore::SQL_CLOUD_PHOTOS_TABLE_QUERY_ALL = "\
    SELECT \
        PhotoAlbum.lpath, \
        Photos.* \
    FROM Photos \
        LEFT JOIN PhotoAlbum \
        ON Photos.owner_album_id=PhotoAlbum.album_id \
    WHERE position = 2 AND \
        COALESCE(Photos.clean_flag, 0) = 0 AND \
        COALESCE(Photos.sync_status, 0) = 0 \
    ORDER BY Photos.file_id \
    LIMIT ?, ? ;";

const std::string ReverseCloneRestore::SQL_PHOTOS_TABLE_COUNT_ALL = "\
    SELECT COUNT(1) AS count \
    FROM Photos \
        LEFT JOIN PhotoAlbum \
        ON Photos.owner_album_id = PhotoAlbum.album_id \
    WHERE position IN (1, 3) AND \
        COALESCE(Photos.clean_flag, 0) = 0 AND \
        COALESCE(Photos.sync_status, 0) = 0;";

const std::string ReverseCloneRestore::SQL_CLOUD_PHOTOS_TABLE_COUNT_ALL = "\
    SELECT COUNT(1) AS count \
    FROM Photos \
        LEFT JOIN PhotoAlbum \
        ON Photos.owner_album_id = PhotoAlbum.album_id \
    WHERE position = 2 AND \
        COALESCE(Photos.clean_flag, 0) = 0 AND \
        COALESCE(Photos.sync_status, 0) = 0;";

namespace {
const std::string REVERSE_RESTORE_RESOURCE_ROOT = "/storage/media/local/files/reverse_restore";
const std::string LAKE_STORAGE_ROOT = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
constexpr int32_t DATA_CONFLICT_UNIQUE_ID_COUNT = 3;
const std::string ACTIVE_DELETE_DISPLAY_NAME = "cloud_media_asset_deleted";

enum class ReverseDataConflictStatus {
    SKIPPED_DUPLICATE,
    NO_CONFLICT,
    MOVED_OLD_ASSET,
    DELETED_OLD_ASSET,
    STALE_RESOURCE_LEFT,
    FAILED,
};

struct ReverseDataConflictResult {
    ReverseDataConflictStatus status {ReverseDataConflictStatus::NO_CONFLICT};
    int32_t fileId {0};
    std::string data;
};

struct DestDataConflictInfo {
    bool found {false};
    bool queryFailed {false};
    int32_t fileId {0};
    int32_t mediaType {0};
    int32_t subtype {0};
    int32_t effectMode {0};
    int32_t fileSourceType {0};
    int32_t position {0};
    std::string displayName;
    std::string storagePath;
    std::string data;
};

struct ReverseDataConflictStats {
    int32_t skippedDuplicateCount {0};
    int32_t noConflictCount {0};
    int32_t movedOldAssetCount {0};
    int32_t deletedOldAssetCount {0};
    int32_t staleResourceCount {0};
    int32_t failedCount {0};
};

using UniqueIdGetter = std::function<int32_t(int32_t)>;

// 资产移动状态XML文件
static constexpr const char* ASSET_MOVES_XML = "/data/storage/el2/base/preferences/asset_moves.xml";

// 键名前缀
static constexpr const char* ASSET_MOVE_KEY_PREFIX = "asset_move_";
static constexpr const char* ASSET_MOVE_COUNT_KEY = "asset_move_count";

bool IsPathUnderDirectory(const std::string &path, const std::string &directory)
{
    if (directory.empty() || path.length() <= directory.length() ||
        path.compare(0, directory.length(), directory) != 0) {
        return false;
    }
    return directory.back() == '/' || path[directory.length()] == '/';
}

bool HasLakeStoragePath(const FileInfo &fileInfo)
{
    return IsPathUnderDirectory(fileInfo.storagePath, LAKE_STORAGE_ROOT);
}

bool HasExistingLakeStoragePath(const FileInfo &fileInfo)
{
    return HasLakeStoragePath(fileInfo) && MediaFileUtils::IsFileExists(fileInfo.storagePath);
}

bool ShouldKeepMissingReverseSourceForLake(const FileInfo &fileInfo, int32_t errCode)
{
    return errCode == E_NO_SUCH_FILE && HasExistingLakeStoragePath(fileInfo);
}

// LCOV_EXCL_START
std::string GetLocalPathByCloudPath(const std::string &cloudPath)
{
    return BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, cloudPath);
}

std::string GetLocalEditDataPathByCloudPath(const std::string &cloudPath)
{
    return BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL_EDIT_DATA, cloudPath);
}

std::string GetLocalThumbPathByCloudPath(const std::string &cloudPath)
{
    if (!IsPathUnderDirectory(cloudPath, RESTORE_FILES_CLOUD_DIR)) {
        return "";
    }
    return RESTORE_FILES_LOCAL_DIR + ".thumbs/" + cloudPath.substr(RESTORE_FILES_CLOUD_DIR.length());
}

bool PrepareAbsorbSourceFileInfo(FileInfo &fileInfo)
{
    fileInfo.cloudPath = BackupFileUtils::GetFullPathByPrefixType(PrefixType::CLOUD, fileInfo.relativePath);
    CHECK_AND_RETURN_RET_LOG(!fileInfo.cloudPath.empty(), false, "Reverse absorb cloudPath is empty");

    int32_t errCode = BackupFileUtils::PreparePath(GetLocalPathByCloudPath(fileInfo.cloudPath));
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "Reverse absorb prepare local path failed, errCode=%{public}d", errCode);
    return true;
}

std::string GetSourceFilePathForReverseAbsorb(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET_LOG(!fileInfo.relativePath.empty(), "", "Reverse absorb relativePath is empty");
    return REVERSE_RESTORE_RESOURCE_ROOT + fileInfo.relativePath;
}

DestDataConflictInfo QueryDestDataConflict(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const std::string &cloudPath)
{
    DestDataConflictInfo info;
    info.data = cloudPath;
    if (destRdb == nullptr) {
        info.queryFailed = true;
        MEDIA_ERR_LOG("QueryDestDataConflict: destRdb is null");
        return info;
    }

    const std::string querySql = "SELECT file_id, media_type, display_name, subtype, moving_photo_effect_mode, "
        "file_source_type, storage_path, position FROM Photos WHERE data = ? LIMIT 1";
    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, querySql, {cloudPath});
    if (resultSet == nullptr) {
        info.queryFailed = true;
        MEDIA_ERR_LOG("QueryDestDataConflict: query failed, data=%{public}s",
            MediaFileUtils::DesensitizePath(cloudPath).c_str());
        return info;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return info;
    }

    info.found = true;
    info.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    info.mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    info.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    info.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    info.effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    info.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    info.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    info.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    resultSet->Close();
    return info;
}

bool IsLakeConflictAsset(const DestDataConflictInfo &conflict)
{
    return conflict.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE) &&
        IsPathUnderDirectory(conflict.storagePath, LAKE_STORAGE_ROOT);
}

bool HasLocalConflictOrigin(const DestDataConflictInfo &conflict)
{
    return conflict.position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
        conflict.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
}

bool HasMovingPhotoVideo(const DestDataConflictInfo &conflict)
{
    return conflict.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        conflict.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

std::string GetMovingPhotoVideoPathByCloudPath(const std::string &cloudPath)
{
    return MediaFileUtils::GetMovingPhotoVideoPath(GetLocalPathByCloudPath(cloudPath));
}

void MarkExistingResource(const std::string &path, bool &resourceFlag)
{
    resourceFlag = !path.empty() && MediaFileUtils::IsFileExists(path);
}

ReverseStaleTargetResource BuildTargetResourceState(int32_t fileId, const std::string &cloudPath,
    bool includeMovingPhotoVideo)
{
    ReverseStaleTargetResource resource;
    resource.fileId = fileId;
    resource.cloudPath = cloudPath;
    MarkExistingResource(GetLocalPathByCloudPath(cloudPath), resource.origin);
    if (includeMovingPhotoVideo) {
        MarkExistingResource(GetMovingPhotoVideoPathByCloudPath(cloudPath), resource.movingPhotoVideo);
    }
    MarkExistingResource(GetLocalEditDataPathByCloudPath(cloudPath), resource.editData);
    MarkExistingResource(GetLocalThumbPathByCloudPath(cloudPath), resource.thumbnail);
    return resource;
}

bool HasMarkedResource(const ReverseStaleTargetResource &resource)
{
    return resource.origin || resource.movingPhotoVideo || resource.editData || resource.thumbnail;
}

bool DeletePathIfExists(const std::string &path)
{
    if (path.empty() || !MediaFileUtils::IsFileExists(path)) {
        return true;
    }
    bool isFile = !MediaFileUtils::IsDirectory(path);
    bool deleted = MediaFileUtils::DeleteFileOrFolder(path, isFile);
    CHECK_AND_PRINT_LOG(deleted, "Reverse data conflict delete path failed, path=%{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    return deleted;
}

bool CleanMarkedTargetResources(const ReverseStaleTargetResource &resource)
{
    bool success = true;
    if (resource.origin) {
        success = DeletePathIfExists(GetLocalPathByCloudPath(resource.cloudPath)) && success;
    }
    if (resource.movingPhotoVideo) {
        success = DeletePathIfExists(GetMovingPhotoVideoPathByCloudPath(resource.cloudPath)) && success;
    }
    if (resource.editData) {
        success = DeletePathIfExists(GetLocalEditDataPathByCloudPath(resource.cloudPath)) && success;
    }
    if (resource.thumbnail) {
        success = DeletePathIfExists(GetLocalThumbPathByCloudPath(resource.cloudPath)) && success;
    }
    return success;
}

bool MoveExistingResource(const std::string &src, const std::string &dst, bool isDirectory)
{
    CHECK_AND_RETURN_RET_LOG(!src.empty() && MediaFileUtils::IsFileExists(src), false,
        "Reverse data conflict source missing, src=%{public}s, dst=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str());
    CHECK_AND_RETURN_RET_LOG(!dst.empty() && !MediaFileUtils::IsFileExists(dst), false,
        "Reverse data conflict target invalid, src=%{public}s, dst=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str());
    int32_t errCode = BackupFileUtils::PreparePath(dst);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, false,
        "Reverse data conflict prepare target failed, dst=%{public}s, errCode=%{public}d",
        MediaFileUtils::DesensitizePath(dst).c_str(), errCode);
    bool moved = isDirectory ? MediaFileUtils::RenameDir(src, dst) : MediaFileUtils::MoveFile(src, dst, true);
    CHECK_AND_RETURN_RET_LOG(moved, false,
        "Reverse data conflict move failed, src=%{public}s, dst=%{public}s",
        MediaFileUtils::DesensitizePath(src).c_str(), MediaFileUtils::DesensitizePath(dst).c_str());
    return true;
}

bool MoveOptionalResource(const std::string &src, const std::string &dst, bool isDirectory)
{
    if (src.empty() || !MediaFileUtils::IsFileExists(src)) {
        return true;
    }
    return MoveExistingResource(src, dst, isDirectory);
}

bool MoveConflictResources(const DestDataConflictInfo &conflict, const std::string &oldCloudPath,
    const std::string &newCloudPath)
{
    std::string oldLocalPath = GetLocalPathByCloudPath(oldCloudPath);
    std::string newLocalPath = GetLocalPathByCloudPath(newCloudPath);
    if (!IsLakeConflictAsset(conflict)) {
        if (HasLocalConflictOrigin(conflict)) {
            CHECK_AND_RETURN_RET(MoveExistingResource(oldLocalPath, newLocalPath, false), false);
            if (HasMovingPhotoVideo(conflict)) {
                CHECK_AND_RETURN_RET(MoveOptionalResource(GetMovingPhotoVideoPathByCloudPath(oldCloudPath),
                    GetMovingPhotoVideoPathByCloudPath(newCloudPath), false), false);
            }
        } else {
            CHECK_AND_RETURN_RET(MoveOptionalResource(oldLocalPath, newLocalPath, false), false);
            if (HasMovingPhotoVideo(conflict)) {
                CHECK_AND_RETURN_RET(MoveOptionalResource(GetMovingPhotoVideoPathByCloudPath(oldCloudPath),
                    GetMovingPhotoVideoPathByCloudPath(newCloudPath), false), false);
            }
        }
    }
    CHECK_AND_RETURN_RET(MoveOptionalResource(GetLocalEditDataPathByCloudPath(oldCloudPath),
        GetLocalEditDataPathByCloudPath(newCloudPath), true), false);
    CHECK_AND_RETURN_RET(MoveOptionalResource(GetLocalThumbPathByCloudPath(oldCloudPath),
        GetLocalThumbPathByCloudPath(newCloudPath), true), false);
    return true;
}

void CleanMovedConflictResources(const DestDataConflictInfo &conflict, const std::string &newCloudPath)
{
    ReverseStaleTargetResource movedResource =
        BuildTargetResourceState(conflict.fileId, newCloudPath, HasMovingPhotoVideo(conflict));
    if (!HasMarkedResource(movedResource)) {
        return;
    }
    CHECK_AND_PRINT_LOG(CleanMarkedTargetResources(movedResource),
        "Reverse data conflict clean moved resources failed, fileId=%{public}d, newData=%{public}s",
        conflict.fileId, MediaFileUtils::DesensitizePath(newCloudPath).c_str());
}

bool DeleteConflictResources(const DestDataConflictInfo &conflict, const std::string &cloudPath,
    ReverseStaleTargetResource &staleResource)
{
    ReverseStaleTargetResource beforeDelete =
        BuildTargetResourceState(conflict.fileId, cloudPath, HasMovingPhotoVideo(conflict));
    bool success = CleanMarkedTargetResources(beforeDelete);
    staleResource = success ? ReverseStaleTargetResource {} :
        BuildTargetResourceState(conflict.fileId, cloudPath, HasMovingPhotoVideo(conflict));
    if (success) {
        staleResource = ReverseStaleTargetResource {};
    }
    return success;
}

ReverseDataConflictResult MakeDataConflictResult(ReverseDataConflictStatus status, int32_t fileId,
    const std::string &data)
{
    ReverseDataConflictResult result;
    result.status = status;
    result.fileId = fileId;
    result.data = data;
    return result;
}

int32_t DeletePhotoExtRow(TransactionOperations &trans, int32_t fileId)
{
    const std::string deletePhotoExtSql = "DELETE FROM tab_photos_ext WHERE photo_id = ?";
    return trans.ExecuteSql(deletePhotoExtSql, {std::to_string(fileId)});
}

int32_t DeletePhotoRow(TransactionOperations &trans, int32_t fileId)
{
    const std::string deletePhotoSql = "DELETE FROM Photos WHERE file_id = ?";
    return trans.ExecuteSql(deletePhotoSql, {std::to_string(fileId)});
}

int32_t DeleteLakeConflictRowPreserveOrigin(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const DestDataConflictInfo &conflict)
{
    CHECK_AND_RETURN_RET_LOG(destRdb != nullptr, E_FAIL, "Reverse data conflict target rdb is null");
    TransactionOperations trans { __func__ };
    trans.SetBackupRdbStore(destRdb);
    std::function<int()> deleteRows = [&]() -> int {
        int32_t errCode = DeletePhotoExtRow(trans, conflict.fileId);
        CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
            "Reverse data conflict delete lake photo ext failed, fileId=%{public}d, errCode=%{public}d",
            conflict.fileId, errCode);
        errCode = DeletePhotoRow(trans, conflict.fileId);
        CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
            "Reverse data conflict delete lake photo row failed, fileId=%{public}d, errCode=%{public}d",
            conflict.fileId, errCode);
        return NativeRdb::E_OK;
    };
    int32_t errCode = trans.RetryTrans(deleteRows, true);
    CHECK_AND_PRINT_LOG(errCode == NativeRdb::E_OK,
        "Reverse data conflict delete lake rows transaction failed, fileId=%{public}d, errCode=%{public}d",
        conflict.fileId, errCode);
    return errCode;
}

ReverseDataConflictResult DeleteConflictAssetForAbsorb(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const DestDataConflictInfo &conflict, ReverseStaleTargetResource &staleResource)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(conflict.fileId));
    int32_t errCode = IsLakeConflictAsset(conflict) ?
        DeleteLakeConflictRowPreserveOrigin(destRdb, conflict) :
        MediaLibraryAssetOperations::DeletePermanently(predicates, true);

    DestDataConflictInfo conflictAfterDelete = QueryDestDataConflict(destRdb, conflict.data);
    if (conflictAfterDelete.queryFailed || conflictAfterDelete.found) {
        MEDIA_ERR_LOG("Reverse data conflict delete old asset failed, fileId=%{public}d, errCode=%{public}d, "
            "stillFound=%{public}d, data=%{public}s",
            conflict.fileId, errCode, conflictAfterDelete.found,
            MediaFileUtils::DesensitizePath(conflict.data).c_str());
        return MakeDataConflictResult(ReverseDataConflictStatus::FAILED, conflict.fileId, conflict.data);
    }

    bool filesDeleted = DeleteConflictResources(conflict, conflict.data, staleResource);
    MEDIA_INFO_LOG("Reverse data conflict deleted old asset, fileId=%{public}d, errCode=%{public}d, "
        "filesDeleted=%{public}d, data=%{public}s",
        conflict.fileId, errCode, filesDeleted, MediaFileUtils::DesensitizePath(conflict.data).c_str());
    return MakeDataConflictResult(filesDeleted ? ReverseDataConflictStatus::DELETED_OLD_ASSET :
        ReverseDataConflictStatus::STALE_RESOURCE_LEFT, conflict.fileId, conflict.data);
}

bool CreateConflictCloudPath(const DestDataConflictInfo &conflict, int32_t uniqueId, std::string &newCloudPath)
{
    int32_t bucket = IsLakeConflictAsset(conflict) ? 0 : -1;
    std::string extension = MediaFileUtils::GetExtensionFromPath(conflict.displayName);
    int32_t errCode = BackupFileUtils::CreateAssetPathById(uniqueId, conflict.mediaType, extension, newCloudPath,
        bucket);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK && !newCloudPath.empty(), false,
        "Reverse data conflict create new data failed, fileId=%{public}d, errCode=%{public}d",
        conflict.fileId, errCode);
    return true;
}

bool IsCandidateDataFree(const std::shared_ptr<NativeRdb::RdbStore> &destRdb, const std::string &newCloudPath,
    const DestDataConflictInfo &conflict)
{
    DestDataConflictInfo dbConflict = QueryDestDataConflict(destRdb, newCloudPath);
    if (dbConflict.queryFailed || dbConflict.found) {
        return false;
    }
    ReverseStaleTargetResource candidateResource =
        BuildTargetResourceState(conflict.fileId, newCloudPath, HasMovingPhotoVideo(conflict));
    return !HasMarkedResource(candidateResource);
}

bool UpdateConflictData(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const DestDataConflictInfo &conflict, const std::string &newCloudPath)
{
    const std::string updateSql = "UPDATE Photos SET data = ? WHERE file_id = ?";
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(destRdb, updateSql,
        {newCloudPath, std::to_string(conflict.fileId)});
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, false,
        "Reverse data conflict update old row failed, fileId=%{public}d, errCode=%{public}d",
        conflict.fileId, errCode);
    return true;
}

ReverseDataConflictResult TryMoveConflictAssetToNewData(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const DestDataConflictInfo &conflict, const std::vector<int32_t> &candidateUniqueIds, std::string &newCloudPath)
{
    for (size_t retry = 0; retry < candidateUniqueIds.size(); retry++) {
        if (!CreateConflictCloudPath(conflict, candidateUniqueIds[retry], newCloudPath)) {
            break;
        }
        if (!IsCandidateDataFree(destRdb, newCloudPath, conflict)) {
            MEDIA_WARN_LOG("Reverse data conflict candidate occupied, fileId=%{public}d, retry=%{public}d, "
                "candidate=%{public}s",
                conflict.fileId, static_cast<int32_t>(retry), MediaFileUtils::DesensitizePath(newCloudPath).c_str());
            continue;
        }
        if (MoveConflictResources(conflict, conflict.data, newCloudPath) &&
            UpdateConflictData(destRdb, conflict, newCloudPath)) {
            MEDIA_INFO_LOG("Reverse data conflict moved old asset, fileId=%{public}d, oldData=%{public}s, "
                "newData=%{public}s, lake=%{public}d",
                conflict.fileId, MediaFileUtils::DesensitizePath(conflict.data).c_str(),
                MediaFileUtils::DesensitizePath(newCloudPath).c_str(), IsLakeConflictAsset(conflict));
            return MakeDataConflictResult(ReverseDataConflictStatus::MOVED_OLD_ASSET, conflict.fileId,
                conflict.data);
        }

        CleanMovedConflictResources(conflict, newCloudPath);
        MEDIA_ERR_LOG("Reverse data conflict move old asset failed, fileId=%{public}d, retry=%{public}d, "
            "newData=%{public}s",
            conflict.fileId, static_cast<int32_t>(retry), MediaFileUtils::DesensitizePath(newCloudPath).c_str());
        break;
    }
    return MakeDataConflictResult(ReverseDataConflictStatus::FAILED, conflict.fileId, conflict.data);
}

ReverseDataConflictResult ReleaseDestDataConflict(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const DestDataConflictInfo &conflict, const std::vector<int32_t> &candidateUniqueIds,
    ReverseStaleTargetResource &staleResource)
{
    std::string newCloudPath;
    ReverseDataConflictResult moveResult =
        TryMoveConflictAssetToNewData(destRdb, conflict, candidateUniqueIds, newCloudPath);
    if (moveResult.status == ReverseDataConflictStatus::MOVED_OLD_ASSET) {
        return moveResult;
    }
    return DeleteConflictAssetForAbsorb(destRdb, conflict, staleResource);
}

ReverseDataConflictResult CleanLocalTargetResidue(int32_t fileId, const std::string &cloudPath,
    ReverseStaleTargetResource &staleResource)
{
    staleResource = BuildTargetResourceState(fileId, cloudPath, true);
    if (!HasMarkedResource(staleResource)) {
        return MakeDataConflictResult(ReverseDataConflictStatus::NO_CONFLICT, fileId, cloudPath);
    }
    if (CleanMarkedTargetResources(staleResource)) {
        staleResource = ReverseStaleTargetResource {};
        return MakeDataConflictResult(ReverseDataConflictStatus::DELETED_OLD_ASSET, fileId, cloudPath);
    }
    return MakeDataConflictResult(ReverseDataConflictStatus::STALE_RESOURCE_LEFT, fileId, cloudPath);
}

void PutBackStaleResource(ReverseClonePhotoBatchContext &batch, const ReverseStaleTargetResource &staleResource)
{
    if (!HasMarkedResource(staleResource)) {
        return;
    }
    batch.staleTargetResources.emplace_back(staleResource);
}

std::vector<int32_t> BuildDataConflictCandidateUniqueIds(const DestDataConflictInfo &conflict,
    const UniqueIdGetter &getUniqueId)
{
    std::vector<int32_t> candidateUniqueIds;
    candidateUniqueIds.reserve(DATA_CONFLICT_UNIQUE_ID_COUNT);
    for (int32_t i = 0; i < DATA_CONFLICT_UNIQUE_ID_COUNT; i++) {
        int32_t uniqueId = getUniqueId(conflict.mediaType);
        if (uniqueId < 0) {
            MEDIA_ERR_LOG("Reverse data conflict get unique id failed, fileId=%{public}d, mediaType=%{public}d",
                conflict.fileId, conflict.mediaType);
            break;
        }
        candidateUniqueIds.emplace_back(uniqueId);
    }
    return candidateUniqueIds;
}

ReverseDataConflictResult ResolveDataConflictForFile(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
    const FileInfo &fileInfo, const UniqueIdGetter &getUniqueId, ReverseStaleTargetResource &staleResource)
{
    if (fileInfo.deletedSrcdbFileId > 0) {
        return MakeDataConflictResult(ReverseDataConflictStatus::SKIPPED_DUPLICATE, fileInfo.fileIdOld,
            fileInfo.cloudPath);
    }

    DestDataConflictInfo conflict = QueryDestDataConflict(destRdb, fileInfo.cloudPath);
    if (conflict.queryFailed) {
        return MakeDataConflictResult(ReverseDataConflictStatus::FAILED, fileInfo.fileIdOld, fileInfo.cloudPath);
    }
    if (!conflict.found) {
        return CleanLocalTargetResidue(fileInfo.fileIdOld, fileInfo.cloudPath, staleResource);
    }

    std::vector<int32_t> candidateUniqueIds = BuildDataConflictCandidateUniqueIds(conflict, getUniqueId);
    return ReleaseDestDataConflict(destRdb, conflict, candidateUniqueIds, staleResource);
}

void RecordDataConflictResult(ReverseClonePhotoBatchContext &batch, const ReverseDataConflictResult &result,
    const ReverseStaleTargetResource &staleResource, ReverseDataConflictStats &stats)
{
    switch (result.status) {
        case ReverseDataConflictStatus::SKIPPED_DUPLICATE:
            stats.skippedDuplicateCount++;
            break;
        case ReverseDataConflictStatus::NO_CONFLICT:
            stats.noConflictCount++;
            break;
        case ReverseDataConflictStatus::MOVED_OLD_ASSET:
            stats.movedOldAssetCount++;
            break;
        case ReverseDataConflictStatus::DELETED_OLD_ASSET:
            stats.deletedOldAssetCount++;
            break;
        case ReverseDataConflictStatus::STALE_RESOURCE_LEFT:
            stats.staleResourceCount++;
            PutBackStaleResource(batch, staleResource);
            break;
        case ReverseDataConflictStatus::FAILED:
            stats.failedCount++;
            break;
    }
}

struct FailedOriginEnsureStats {
    size_t noPlanCount {0};
    size_t invalidDstCount {0};
    size_t dstSameSizeCount {0};
    size_t dstSizeMismatchCount {0};
    size_t dstDeleteFailedCount {0};
    size_t noSourceResourceCount {0};
    size_t sourceMissingCount {0};
    size_t needEnsureCount {0};
};

std::string NormalizeResourceRoot(const std::string &root)
{
    if (root.empty()) {
        return RESTORE_FILES_LOCAL_DIR;
    }
    if (root.back() == '/') {
        return root.substr(0, root.length() - 1);
    }
    return root;
}

std::string GetResourcePathByCloudPathForRoot(const std::string &cloudPath, const std::string &root)
{
    if (!IsPathUnderDirectory(cloudPath, RESTORE_FILES_CLOUD_DIR)) {
        return "";
    }
    return NormalizeResourceRoot(root) + "/" + cloudPath.substr(RESTORE_FILES_CLOUD_DIR.length());
}

void AddUniqueSourcePath(std::vector<std::string> &sourcePaths, const std::string &sourcePath)
{
    if (sourcePath.empty()) {
        return;
    }
    for (const auto &path : sourcePaths) {
        if (path == sourcePath) {
            return;
        }
    }
    sourcePaths.emplace_back(sourcePath);
}

const ReverseCloneAssetResource *GetFailedAssetSourceResource(const ReverseCloneResourcePlan &plan)
{
    if (plan.hasFallbackSource && plan.fallbackSource.HasResourcePath()) {
        return &plan.fallbackSource;
    }
    if (plan.matchType == ReverseCloneMatchType::SOURCE_ASSET && plan.donor.HasResourcePath()) {
        return &plan.donor;
    }
    return nullptr;
}

bool FindExistingSourceOrigin(const ReverseCloneAssetResource &source, std::string &sourceOrigin)
{
    std::vector<std::string> sourcePaths;
    AddUniqueSourcePath(sourcePaths, source.storagePath);
    AddUniqueSourcePath(sourcePaths, source.originPath);
    AddUniqueSourcePath(sourcePaths, GetResourcePathByCloudPathForRoot(source.cloudPath, source.localRoot));
    for (const auto &path : sourcePaths) {
        if (MediaFileUtils::IsFileExists(path)) {
            sourceOrigin = path;
            return true;
        }
    }
    return false;
}

ReverseClonePhotoBatchContext BuildFailedResourceBatch(const ReverseClonePhotoBatchContext &batch,
    const std::vector<int32_t> &failedFileIds, FailedOriginEnsureStats &stats)
{
    ReverseClonePhotoBatchContext failedBatch;
    failedBatch.cloudRestoreSatisfied = batch.cloudRestoreSatisfied;
    if (failedFileIds.empty()) {
        return failedBatch;
    }

    for (int32_t fileId : failedFileIds) {
        auto plan = batch.resourcePlans.find(fileId);
        if (plan == batch.resourcePlans.end()) {
            stats.noPlanCount++;
            MEDIA_WARN_LOG("Reverse absorb committed failed asset has no resource plan, fileId=%{public}d", fileId);
            continue;
        }

        std::string dstOrigin = GetLocalPathByCloudPath(plan->second.absorbed.cloudPath);
        const ReverseCloneAssetResource *source = GetFailedAssetSourceResource(plan->second);
        if (source == nullptr) {
            stats.noSourceResourceCount++;
            MEDIA_WARN_LOG("Reverse absorb committed failed asset has no source resource, fileId=%{public}d",
                fileId);
            continue;
        }

        std::string sourceOrigin;
        if (!FindExistingSourceOrigin(*source, sourceOrigin)) {
            stats.sourceMissingCount++;
            MEDIA_WARN_LOG("Reverse absorb committed failed asset source origin missing, fileId=%{public}d",
                fileId);
            continue;
        }

        if (dstOrigin.empty()) {
            stats.invalidDstCount++;
            MEDIA_WARN_LOG("Reverse absorb committed failed asset target origin path invalid, fileId=%{public}d",
                fileId);
            continue;
        }
        if (MediaFileUtils::IsFileExists(dstOrigin)) {
            stats.dstSizeMismatchCount++;
            if (!MediaFileUtils::DeleteFile(dstOrigin)) {
                stats.dstDeleteFailedCount++;
                MEDIA_WARN_LOG("Reverse absorb committed failed asset delete mismatched target origin failed, "
                    "fileId=%{public}d, src=%{public}s, dst=%{public}s", fileId,
                    MediaFileUtils::DesensitizePath(sourceOrigin).c_str(),
                    MediaFileUtils::DesensitizePath(dstOrigin).c_str());
                continue;
            }
            MEDIA_WARN_LOG("Reverse absorb committed failed asset deleted mismatched target origin, "
                "fileId=%{public}d, src=%{public}s, dst=%{public}s", fileId,
                MediaFileUtils::DesensitizePath(sourceOrigin).c_str(),
                MediaFileUtils::DesensitizePath(dstOrigin).c_str());
        }

        stats.needEnsureCount++;
        MEDIA_INFO_LOG("Reverse absorb committed failed asset needs origin ensure, fileId=%{public}d, "
            "src=%{public}s, dst=%{public}s", fileId, MediaFileUtils::DesensitizePath(sourceOrigin).c_str(),
            MediaFileUtils::DesensitizePath(dstOrigin).c_str());
        failedBatch.resourcePlans.emplace(plan->first, plan->second);
    }

    failedBatch.validFileInfos.reserve(failedBatch.resourcePlans.size());
    failedBatch.values.reserve(failedBatch.resourcePlans.size());
    for (size_t index = 0; index < batch.validFileInfos.size(); ++index) {
        const FileInfo &fileInfo = batch.validFileInfos[index];
        if (failedBatch.resourcePlans.find(fileInfo.fileIdOld) == failedBatch.resourcePlans.end()) {
            continue;
        }
        failedBatch.validFileInfos.emplace_back(fileInfo);
        if (index < batch.values.size()) {
            failedBatch.values.emplace_back(batch.values[index]);
        }
    }
    return failedBatch;
}

} // namespace

ReverseCloneRestore::ReverseCloneRestore()
{
    sceneCode_ = CLONE_RESTORE_ID;
    minDestDbFileId_ = 0;
    MEDIA_INFO_LOG("ReverseCloneRestore constructed");
}

int32_t ReverseCloneRestore::Init(const string &backupRestoreDir, const string &upgradePath, bool isUpgrade)
{
    MEDIA_INFO_LOG("ReverseCloneRestore INIT");
    CloneRestore::Init(backupRestoreDir, upgradePath, isUpgrade);
    return E_OK;
}

void InsertForAncoProcess()
{
    NativeRdb::ValuesBucket values;
    values.PutInt("type", BACKUP_OPER_TYPE);
    values.PutInt("opt_type", 0);
    values.PutInt("is_sent", 0);
    int32_t ret = MediaLibraryTableAssetAlbumOperations::Insert(values);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "BackUpOperation::Insert failed: %{public}d", ret);
}

bool ReverseCloneRestore::ShouldUseReverseCloneRestore()
{
    // 旧机不存在数据库升级记录xml，则不支持反向克隆
    string xmlPath = backupRestoreDir_ + CLONE_RESTORE_PREFERENCE_PATH + MEDIA_DB_UPGRADE_EVENTS_XML;
    if (!MediaFileUtils::IsFileExists(xmlPath)) {
        MEDIA_INFO_LOG("xml file NOT EXIST: %{public}s", xmlPath.c_str());
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "The XML file of the old service is not transferred.";
        return false;
    }

    PhotoCountContext context(mediaRdb_, mediaLibraryRdb_, IsCloudRestoreSatisfied(),
        ShouldAbsorbCloudFromSourceRdb(), sceneCode_, taskId_);
    return context.NeedReverseRestore(reverseRestoreReportInfo_);
}

static bool IsPathExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

static bool AddFileSize(int64_t &totalSize, int64_t fileSize)
{
    if (fileSize < 0 || totalSize > std::numeric_limits<int64_t>::max() - fileSize) {
        return false;
    }
    totalSize += fileSize;
    return true;
}

static bool GetPathSize(const std::string &path, int64_t &totalSize)
{
    struct stat statInfo;
    if (lstat(path.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("GetPathSize: stat failed, path=%{public}s, errno=%{public}d", path.c_str(), errno);
        return false;
    }

    if (!S_ISDIR(statInfo.st_mode)) {
        return AddFileSize(totalSize, statInfo.st_size);
    }

    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        MEDIA_ERR_LOG("GetPathSize: open dir failed, path=%{public}s, errno=%{public}d", path.c_str(), errno);
        return false;
    }

    struct dirent *entry = nullptr;
    bool success = true;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (!GetPathSize(path + "/" + entry->d_name, totalSize)) {
            success = false;
            break;
        }
    }
    closedir(dir);
    return success;
}

static bool EnsureDbDirSpace(const char *stage)
{
    int64_t dbDirSize = 0;
    if (!GetPathSize(MEDIA_RDB_DIR, dbDirSize)) {
        MEDIA_ERR_LOG("EnsureDbDirSpace: get db dir size failed, stage=%{public}s", stage);
        return false;
    }

    struct statvfs statInfo;
    if (statvfs(MEDIA_RDB_DIR.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("EnsureDbDirSpace: statvfs failed, stage=%{public}s, dir=%{public}s, errno=%{public}d",
            stage, MEDIA_RDB_DIR.c_str(), errno);
        return false;
    }

    int64_t availableBytes = static_cast<int64_t>(statInfo.f_bavail) * static_cast<int64_t>(statInfo.f_frsize);
    int64_t requiredBytes = dbDirSize * DB_SPACE_FACTOR_NUMERATOR / DB_SPACE_FACTOR_DENOMINATOR +
        DB_SPACE_RESERVE_BYTES;
    if (availableBytes < requiredBytes) {
        MEDIA_ERR_LOG("EnsureDbDirSpace: insufficient space, stage=%{public}s, dirSize=%{public}lld, "
            "required=%{public}lld, available=%{public}lld",
            stage, static_cast<long long>(dbDirSize), static_cast<long long>(requiredBytes),
            static_cast<long long>(availableBytes));
        return false;
    }
    MEDIA_INFO_LOG("EnsureDbDirSpace: success, stage=%{public}s, dirSize=%{public}lld, required=%{public}lld, "
        "available=%{public}lld",
        stage, static_cast<long long>(dbDirSize), static_cast<long long>(requiredBytes),
        static_cast<long long>(availableBytes));
    return true;
}

static void DeleteDbFileGroup(const std::string& basePath) {
    std::vector<std::string> suffixes = {"", "-compare", "-dwr", "-shm", "-wal", "_binlog"};
    for (const auto& suffix : suffixes) {
        std::string file = basePath + suffix;
        bool result = MediaFileUtils::DeleteFileOrFolder(file, suffix != "_binlog");
        if (!result) {
            MEDIA_ERR_LOG("ReverseCloneRestore: DeleteDbFileGroup failed to delete %{public}s", file.c_str());
        }
    }
}

static void RollbackMovedDbFiles(const std::vector<std::pair<std::string, std::string>>& movedFiles)
{
    for (auto iter = movedFiles.rbegin(); iter != movedFiles.rend(); ++iter) {
        if (rename(iter->second.c_str(), iter->first.c_str()) != 0) {
            int savedErrno = errno;
            MEDIA_ERR_LOG("MoveDbFileGroup: rollback %{public}s -> %{public}s failed, errno=%{public}d",
                iter->second.c_str(), iter->first.c_str(), savedErrno);
            DiagnoseRenameFailure(iter->second, iter->first, savedErrno);
        }
    }
}

static bool MoveDbFileGroup(const std::string& srcBase, const std::string& dstBase, bool deleteSrc = false) {
    std::vector<std::string> fileSuffixes = {"", "-compare", "-dwr", "-shm", "-wal"};
    std::vector<std::string> dirSuffixes  = {"_binlog"};
    std::vector<std::pair<std::string, std::string>> movedFiles;
    // 移动普通文件
    for (const auto& suffix : fileSuffixes) {
        std::string src = srcBase + suffix;
        std::string dst = dstBase + suffix;
        if (!IsPathExists(src)) continue;
        if (rename(src.c_str(), dst.c_str()) != 0) {
            int savedErrno = errno;
            MEDIA_ERR_LOG("MoveDbFileGroup: rename %{public}s -> %{public}s failed, errno=%{public}d",
                src.c_str(), dst.c_str(), savedErrno);
            DiagnoseRenameFailure(src, dst, savedErrno);
            RollbackMovedDbFiles(movedFiles);
            return false;
        }
        movedFiles.emplace_back(src, dst);
    }
    // 复制目录（rename 对目录可能失败，改用 CopyDirectory）
    for (const auto& suffix : dirSuffixes) {
        std::string src = srcBase + suffix;
        std::string dst = dstBase + suffix;
        if (!IsPathExists(src)) continue;
        if (MediaFileUtils::CopyDirectory(src, dst) != E_OK) {
            MEDIA_ERR_LOG("MoveDbFileGroup: copy directory %{public}s -> %{public}s failed", src.c_str(), dst.c_str());
            RollbackMovedDbFiles(movedFiles);
            return false;
        }
        // 复制成功后删除源目录
        MediaFileUtils::DeleteFileOrFolder(src, false);
    }
    if (deleteSrc) {
        DeleteDbFileGroup(srcBase);
    }
    return true;
}

static bool CopyDbFileGroup(const std::string& srcBase, const std::string& dstBase,
    bool deleteSrc = false, bool verifySize = false)
{
    const std::vector<std::string> fileSuffixes = {"", "-compare", "-dwr", "-shm", "-wal"};
    std::unordered_map<std::string, int64_t> srcFileSizes;

    for (const auto& suffix : fileSuffixes) {
        const std::string src = srcBase + suffix;
        const std::string dst = dstBase + suffix;
        CHECK_AND_CONTINUE(IsPathExists(src));

        if (verifySize) {
            struct stat srcStat;
            if (stat(src.c_str(), &srcStat) != 0) {
                MEDIA_ERR_LOG("CopyDbFileGroup: failed to stat %{public}s", src.c_str());
                return false;
            }
            srcFileSizes[src] = srcStat.st_size;
        }

        if (!MediaFileUtils::CopyFileUtil(src, dst, true)) {
            MEDIA_ERR_LOG("CopyDbFileGroup: copy %{public}s -> %{public}s failed",
                          src.c_str(), dst.c_str());
            return false;
        }
    }
    if (deleteSrc) {
        DeleteDbFileGroup(srcBase);
    }

    CHECK_AND_RETURN_RET(verifySize, true);

    for (const auto& [srcPath, srcSize] : srcFileSizes) {
        struct stat dstStat;
        std::string dstPath = srcPath;
        dstPath.replace(0, srcBase.length(), dstBase);

        if (stat(dstPath.c_str(), &dstStat) != 0) {
            MEDIA_ERR_LOG("CopyDbFileGroup: failed to stat dst %{public}s", dstPath.c_str());
            return false;
        }

        if (dstStat.st_size != srcSize) {
            MEDIA_ERR_LOG("CopyDbFileGroup: size mismatch %{public}s: src=%{public}lld, dst=%{public}lld",
                srcPath.c_str(), static_cast<long long>(srcSize), static_cast<long long>(dstStat.st_size));
            return false;
        }
    }
    return true;
}

bool ReverseCloneRestore::PrepareOldDb(const std::string &backupRestorePath,
    std::shared_ptr<NativeRdb::RdbStore> &oldDbStore)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    const std::string backupOldDbPath = backupRestorePath + MEDIA_DB_PATH;
    const std::string backupOldDbTemp = "/data/storage/el2/database/rdb/old_media_library_temp.db";

    MEDIA_INFO_LOG("PrepareOldDb: close all db handles");
    mediaRdb_.reset();
    mediaRdb_ = nullptr;

    // 1. 创建旧 db 临时副本（用于升级和后续处理）
    if (!EnsureDbDirSpace(STAGE_PREPARE_OLD_DB)) {
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "insufficient storage space" + string(STAGE_PREPARE_OLD_DB);
        return false;
    }
    MEDIA_INFO_LOG("PrepareOldDb: create temp copy of old db");
    if (!CopyDbFileGroup(backupOldDbPath, backupOldDbTemp, false, true)) {
        MEDIA_ERR_LOG("PrepareOldDb: create old db temp copy failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "old database copy failed";
        return false;
    }

    // 2. 启动异步完整性检查任务（在 CopyDbFileGroup 成功后立即启动）
    if (!StartIntegrityCheckAsync(backupRestorePath, "")) {
        MEDIA_WARN_LOG("PrepareOldDb: start integrity check async failed, continue");
    }

    // 3. 打开临时副本的 RdbStore，返回给调用方
    MEDIA_INFO_LOG("PrepareOldDb: open old db temp store");
    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "PrepareOldDb: get context failed");

    int32_t err = BackupDatabaseUtils::InitDb(oldDbStore, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                              backupOldDbTemp, CONST_BUNDLE_NAME, true, context->GetArea(), false);
    if (oldDbStore == nullptr) {
        MEDIA_ERR_LOG("PrepareOldDb: init old db temp store failed, err=%{public}d", err);
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "temp old database init failed";
        return false;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" PrepareOldDb: ")
        .append(std::to_string(endTime - startTime) + ";");
    MEDIA_INFO_LOG("PrepareOldDb: success");
    return true;
}

bool ReverseCloneRestore::WaitForMainCloseDataBase()
{
    int32_t REVERSE_CLOSE_DATABASE = 1;
    NotifyDbStatusForClone(REVERSE_CLOSE_DATABASE);
    // 等待媒体库主进程关库完成（SettingsData key=media_reverse_backup_close_database value=1）
    constexpr int32_t POLL_INTERVAL_MS = 100;
    constexpr int32_t POLL_TIMEOUT_MS = 150000; // 2.5 minutes
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(POLL_TIMEOUT_MS);
    bool dbReOpen = false;
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
        std::string value;
        int32_t ret = SettingsDataManager::GetCloseDatabaseStatus(value);
        const std::string CLOSE_READY = "0";
        if (ret == E_OK && value == CLOSE_READY) {
            dbReOpen = true;
            MEDIA_INFO_LOG("FinalizeDatabaseSwap: database close confirmed");
            break;
        }
        MEDIA_INFO_LOG("FinalizeDatabaseSwap: database close not confirmed");
    }
    if (!dbReOpen) {
        MEDIA_ERR_LOG("FinalizeDatabaseSwap: wait for database close timeout");
        return false;
    }
    return true;
}

bool ReverseCloneRestore::BackupAndRenameNewDb()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    const std::string newDbPath = "/data/storage/el2/database/rdb/media_library.db";
    const std::string newDbSourcePath = "/data/storage/el2/database/rdb/media_library_source.db";

    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "BackupAndRenameNewDb: get context failed");

    // 1. 打开新 db（当前正使用的主库）
    std::shared_ptr<NativeRdb::RdbStore> newDbStore;
    int32_t err = BackupDatabaseUtils::InitDb(newDbStore, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                              newDbPath, CONST_BUNDLE_NAME, true, context->GetArea(), false);
    if (newDbStore == nullptr) {
        MEDIA_ERR_LOG("BackupAndRenameNewDb: open new db failed, err=%{public}d", err);
        return false;
    }

    // 2. 执行 Backup（不指定特殊名称，使用默认方式生成 slave 双写文件）
    if (!EnsureDbDirSpace(STAGE_BACKUP_NEW_DB)) {
        return false;
    }
    MEDIA_INFO_LOG("BackupAndRenameNewDb: start backup new db");
    err = newDbStore->Backup("", {}, false);
    newDbStore.reset(); // 关闭句柄，后续通过文件操作完成
    newDbStore = nullptr;
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("BackupAndRenameNewDb: backup failed, err=%{public}d", err);
        return false;
    }

    CHECK_AND_RETURN_RET_LOG(WaitForMainCloseDataBase(), false,
        "BackupAndRenameNewDb main process close database failed");

    // 3. 重命名新 db 主文件到 source
    if (!MoveDbFileGroup(newDbPath, newDbSourcePath, false)) {
        MEDIA_ERR_LOG("BackupAndRenameNewDb: rename new db to source failed");
        return false;
    }
    // 3.1 同时重命名 slave 文件到 source_slave
    const std::string slavePath = "/data/storage/el2/database/rdb/media_library_slave.db";
    const std::string slaveSourcePath = "/data/storage/el2/database/rdb/media_library_source_slave.db";
    if (IsPathExists(slavePath)) {
        if (!MoveDbFileGroup(slavePath, slaveSourcePath, false)) {
            MEDIA_ERR_LOG("BackupAndRenameNewDb: rename slave to source_slave failed");
            return false;
        }
    }

    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" BackupRdbstore: ")
        .append(std::to_string(endTime - startTime) + ";");
    MEDIA_INFO_LOG("BackupAndRenameNewDb: success");
    return true;
}

void SetNeedCreateDentryFlag(int32_t value)
{
    const std::string CLOUD_DENTRY_PREFS = "/data/storage/el2/base/preferences/cloud_dentry_config.xml";
    const std::string NEED_CREATE_DENTRY_AFTER_CLONE = "need_create_dentry_after_clone";
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLOUD_DENTRY_PREFS, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);

    prefs->PutInt(NEED_CREATE_DENTRY_AFTER_CLONE, value);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Set need_insert_dentry_after_clone to %{public}d", value);
}

static void SafeDeleteFile(const std::string& path)
{
    if (IsPathExists(path)) {
        if (unlink(path.c_str()) != 0) {
            MEDIA_ERR_LOG("SafeDeleteFile: failed to delete %{public}s, errno=%{public}d", path.c_str(), errno);
        } else {
            MEDIA_INFO_LOG("SafeDeleteFile: deleted %{public}s", path.c_str());
        }
    } else {
        MEDIA_DEBUG_LOG("SafeDeleteFile: path not exist, skip %{public}s", path.c_str());
    }
}

static void SafeDeleteDir(const std::string& path)
{
    if (IsPathExists(path)) {
        if (!MediaFileUtils::DeleteFileOrFolder(path, false)) {
            MEDIA_ERR_LOG("SafeDeleteDir: failed to delete directory %{public}s", path.c_str());
        } else {
            MEDIA_INFO_LOG("SafeDeleteDir: deleted directory %{public}s", path.c_str());
        }
    } else {
        MEDIA_DEBUG_LOG("SafeDeleteDir: path not exist, skip %{public}s", path.c_str());
    }
}

static void CleanDbBinlogResiduals()
{
    MEDIA_INFO_LOG("CleanDbBinlogResiduals: start cleaning binlog residuals");
    for (const auto &dir : {"/data/storage/el2/database/rdb/media_library.db_binlog",
                            "/data/storage/el2/database/rdb/media_library_source.db_binlog"}) {
        SafeDeleteDir(dir);
    }
    MEDIA_INFO_LOG("CleanDbBinlogResiduals: completed");
}

void ReverseCloneRestore::CleanSlaveAndBinlog()
{
    MEDIA_INFO_LOG("CleanSlaveAndBinlog: start cleaning slave and binlog residuals");
    const std::string slaveBase = "/data/storage/el2/database/rdb/media_library_source_slave.db";
    for (const auto &suffix : {"", "-dwr", "-shm", "-wal", "-compare"}) {
        std::string file = slaveBase + suffix;
        SafeDeleteFile(file);
    }

    for (const auto &dir : {"/data/storage/el2/database/rdb/media_library.db_binlog",
                            "/data/storage/el2/database/rdb/media_library_source.db_binlog",
                            "/data/storage/el2/database/rdb/media_library_source_slave.db_binlog"}) {
        SafeDeleteDir(dir);
    }
    MEDIA_INFO_LOG("CleanSlaveAndBinlog: completed");
}

void ReverseCloneRestore::CleanReverseRestoreTempFiles()
{
    MEDIA_INFO_LOG("CleanReverseRestoreTempFiles: start");

    // 删除旧 db 临时副本
    if (IsPathExists("/data/storage/el2/database/rdb/old_media_library_temp.db")) {
        MEDIA_INFO_LOG("CleanReverseRestoreTempFiles: deleting old db temp copy");
        DeleteDbFileGroup("/data/storage/el2/database/rdb/old_media_library_temp.db");
    }

    // 删除 slave 和 binlog
    CleanSlaveAndBinlog();

    // 删除 source db 文件
    if (IsPathExists("/data/storage/el2/database/rdb/media_library_source.db")) {
        MEDIA_INFO_LOG("CleanReverseRestoreTempFiles: deleting source db");
        DeleteDbFileGroup("/data/storage/el2/database/rdb/media_library_source.db");
    }

    if (IsPathExists(REVERSE_RESTORE_RESOURCE_ROOT)) {
        MEDIA_INFO_LOG("CleanReverseRestoreTempFiles: deleting reverse restore resource dir");
        SafeDeleteDir(REVERSE_RESTORE_RESOURCE_ROOT);
    }

    // 删除资产移动状态XML文件
    int32_t errCode = 0;
    NativePreferences::PreferencesHelper::DeletePreferences(ASSET_MOVES_XML);

    MEDIA_INFO_LOG("CleanReverseRestoreTempFiles: completed");
}

/**
 * @brief 回退反向克隆操作，恢复正向克隆所需的环境。
 *
 * 为什么需要 isEarlyStage 参数：
 *   反向克隆过程中，不同阶段对主库和文件目录的操作不同。前期失败时
 *   （PrepareOldDb、升级、纯云清理、ID偏移等），主库尚未被替换或重命名，
 *   文件目录也未移动，因此只需要清理反向克隆创建的临时文件即可恢复正向环境。
 *   而 BackupAndRenameNewDb 之后，主库已被重命名为 source，文件目录也可能
 *   被移动，必须执行完整的回退，将 source 移回主库、恢复文件目录，并清理
 *   所有中间产物，才能保证正向克隆能正确执行。
 *
 * @param isEarlyStage true: 早期失败，仅清理临时文件和 slave/binlog。
 *                     false: 非早期失败，需完整恢复主库、文件目录，并清理所有
 *                            临时文件。
 *
 * 各场景说明：
 *   isEarlyStage == true（早期失败）：
 *     - PrepareOldDb 失败
 *     - DoDataBaseUpgrade 失败
 *     - ClearRedundantData 失败
 *     - CloudDataCleaner 失败
 *     - PerformInitialMigration 失败
 *     回退动作：删除 old_media_library_temp.db，清理 slave 和 binlog，主库与
 *              文件目录保持不动。
 *
 *   isEarlyStage == false（非早期失败）：
 *     - BackupAndRenameNewDb 失败
 *     - MoveAssets 失败
 *     回退动作：将 source db 移回主库（覆盖已被替换的旧 db），恢复文件目录，
 *              删除临时副本，清理 slave/binlog。
 *
 *   特殊情况：
 *     - FinalizeDatabaseSwap 失败：调用本函数完整回退。若回退成功，则恢复到
 *       正向克隆可继续使用的 DB 和资源目录状态。
 */
bool ReverseCloneRestore::RollbackReverseRestore(bool isEarlyStage)
{
    MEDIA_INFO_LOG("RollbackReverseRestore start, isEarlyStage=%{public}d", isEarlyStage);
    // Stop the background thread
    StopReverseRestoreStatusUpdateThread();
    // 关闭可能已打开的句柄
    destRdb_.reset();
    sourceRdb_.reset();
    mediaLibraryRdb_.reset();
    mediaRdb_.reset();

    const std::string newDbPath = "/data/storage/el2/database/rdb/media_library.db";
    const std::string newDbSourcePath = "/data/storage/el2/database/rdb/media_library_source.db";
    const std::string backupOldDbTemp = "/data/storage/el2/database/rdb/old_media_library_temp.db";

    // 早期阶段：主库与文件目录均未变动，仅删除临时副本和 slave
    if (isEarlyStage) {
        if (IsPathExists(backupOldDbTemp)) {
            MEDIA_INFO_LOG("RollbackReverseRestore (early): deleting old db temp copy");
            DeleteDbFileGroup(backupOldDbTemp);
        }
        CleanSlaveAndBinlog();
        MEDIA_INFO_LOG("RollbackReverseRestore (early) completed");
        return true;
    }

    // 非早期阶段：主库可能已被替换或 rename，需要恢复
    // 1. 如果存在 source 文件（即主库已被 rename），将其移回主库位置
    if (IsPathExists(newDbSourcePath)) {
        // 如果 newDbPath 已经存在（例如处理后的旧 db 已被移入），先删除
        int32_t REVERSE_ROLLBACK_CLOSE_DATABASE = 3;
        NotifyDbStatusForClone(REVERSE_ROLLBACK_CLOSE_DATABASE);
        if (IsPathExists(newDbPath)) {
            MEDIA_INFO_LOG("RollbackReverseRestore: removing current main db before restoring source");
            DeleteDbFileGroup(newDbPath);
        }
        MEDIA_INFO_LOG("RollbackReverseRestore: moving source db back to main db");
        if (!MoveDbFileGroup(newDbSourcePath, newDbPath, false)) {
            MEDIA_WARN_LOG("RollbackReverseRestore: move failed, trying copy");
            if (!CopyDbFileGroup(newDbSourcePath, newDbPath, true)) {
                MEDIA_ERR_LOG("RollbackReverseRestore: copy also failed, manual recovery needed");
                SetErrorCode(RestoreError::INIT_FAILED);
                return false;
            }
            MEDIA_INFO_LOG("RollbackReverseRestore: copy succeeded");
        } else {
            MEDIA_INFO_LOG("RollbackReverseRestore: move succeeded");
        }
        int32_t REVERSE_ROLLBACK_OPEN_DATABASE = 4;
        NotifyDbStatusForClone(REVERSE_ROLLBACK_OPEN_DATABASE);
    }

    // 2. 删除旧 db 临时副本
    if (IsPathExists(backupOldDbTemp)) {
        MEDIA_INFO_LOG("RollbackReverseRestore: deleting old db temp copy");
        DeleteDbFileGroup(backupOldDbTemp);
    }

    // 3. 恢复文件目录
    if (!RestoreDirectoriesFromBackup()) {
        MEDIA_ERR_LOG("RollbackReverseRestore: restore asset directories failed");
        SetErrorCode(RestoreError::INIT_FAILED);
        return false;
    }

    // 4. 清理 slave 和 binlog
    CleanSlaveAndBinlog();
    MEDIA_INFO_LOG("RollbackReverseRestore completed");
    InsertForAncoProcess();
    return true;
}

bool ReverseCloneRestore::RecoverSourceDbFromSlave()
{
    const std::string slaveBase = "/data/storage/el2/database/rdb/media_library_source_slave.db";
    const std::string sourcePath = "/data/storage/el2/database/rdb/media_library_source.db";

    if (!IsPathExists(slaveBase)) {
        MEDIA_ERR_LOG("RecoverSourceDbFromSlave: slave db does not exist");
        return false;
    }

    // 复制 slave 文件覆盖 source
    for (const auto& suffix : {"", "-dwr", "-shm", "-wal", "-compare"}) {
        std::string src = slaveBase + suffix;
        std::string dst = sourcePath + suffix;
        if (!IsPathExists(src)) continue;
        if (!MediaFileUtils::CopyFileUtil(src, dst, true)) {
            MEDIA_ERR_LOG("RecoverSourceDbFromSlave: copy %{public}s -> %{public}s failed", src.c_str(), dst.c_str());
            return false;
        }
    }

    // 复制 binlog 目录
    std::string srcBinlog = slaveBase + "_binlog";
    std::string dstBinlog = sourcePath + "_binlog";
    if (IsPathExists(srcBinlog) &&
        MediaFileUtils::CopyDirectory(srcBinlog, dstBinlog) != E_OK) {
        MEDIA_ERR_LOG("RecoverSourceDbFromSlave: copy binlog dir failed");
        return false;
    }
    MEDIA_INFO_LOG("RecoverSourceDbFromSlave: slave copied to source");
    return true;
}

bool ReverseCloneRestore::CheckSourceRdbIntegrityAndFallback(const std::string &backupRestorePath,
                                                             const std::string &upgradePath)
{
    MEDIA_INFO_LOG("CheckSourceRdbIntegrityAndFallback: start checking sourceRdb integrity");

    // 检查 sourceRdb 完整性
    std::string integrityResult =
        BackupDatabaseUtils::CheckDbIntegrity(sourceRdb_, sceneCode_, "sourceRdb", &reverseRestoreReportInfo_);
    if (integrityResult == "ok") {
        MEDIA_INFO_LOG("CheckSourceRdbIntegrityAndFallback: sourceRdb integrity check passed");
        return true;
    }

    // sourceRdb 损坏，尝试使用备份数据库恢复
    MEDIA_ERR_LOG("CheckSourceRdbIntegrityAndFallback: sourceRdb integrity check failed, result=%{public}s",
                  integrityResult.c_str());
    if (TryRecoverFromBackupDb()) {
        MEDIA_INFO_LOG("CheckSourceRdbIntegrityAndFallback: recovered from backup database");
        return true;
    }

    // 备份数据库也损坏或无法打开，回滚所有更改到原目录
    MEDIA_INFO_LOG("CheckSourceRdbIntegrityAndFallback: rolling back all moved files and databases");
    if (!RollbackReverseRestore(false)) {
        MEDIA_ERR_LOG("CheckSourceRdbIntegrityAndFallback: rollback failed");
        SetErrorCode(RestoreError::INIT_FAILED);
        return false;
    }
    MEDIA_INFO_LOG("CheckSourceRdbIntegrityAndFallback: rollback completed");
    return false;
}

bool ReverseCloneRestore::TryRecoverFromBackupDb()
{
    MEDIA_INFO_LOG("TryRecoverFromBackupDb: checking backup database integrity");

    std::shared_ptr<NativeRdb::RdbStore> slaveRdb;
    const std::string slaveDbPath = "/data/storage/el2/database/rdb/media_library_source_slave.db";
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        MEDIA_ERR_LOG("TryRecoverFromBackupDb: get context failed");
        return false;
    }

    int32_t err = BackupDatabaseUtils::InitDb(slaveRdb, CONST_MEDIA_DATA_ABILITY_DB_NAME, slaveDbPath,
                                              CONST_BUNDLE_NAME, true, context->GetArea());
    if (slaveRdb == nullptr) {
        MEDIA_ERR_LOG("TryRecoverFromBackupDb: failed to open slave db, err=%{public}d", err);
        return false;
    }

    MEDIA_INFO_LOG("TryRecoverFromBackupDb: opened slave db for integrity check");
    std::string slaveIntegrityResult =
        BackupDatabaseUtils::CheckDbIntegrity(slaveRdb, sceneCode_, "source_slave", &reverseRestoreReportInfo_);
    slaveRdb.reset();

    if (slaveIntegrityResult != "ok") {
        MEDIA_ERR_LOG("TryRecoverFromBackupDb: backup database also corrupted, result=%{public}s",
                      slaveIntegrityResult.c_str());
        return false;
    }

    MEDIA_INFO_LOG("TryRecoverFromBackupDb: backup database is intact, recovering");
    if (!RecoverSourceDbFromSlave()) {
        MEDIA_ERR_LOG("TryRecoverFromBackupDb: RecoverSourceDbFromSlave failed");
        SetErrorCode(RestoreError::INIT_FAILED);
        return false;
    }

    return true;
}

bool ReverseCloneRestore::FinalizeDatabaseSwap(const std::string &backupRestorePath)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    const std::string newDbPath = "/data/storage/el2/database/rdb/media_library.db";
    const std::string newDbSourcePath = "/data/storage/el2/database/rdb/media_library_source.db";
    const std::string backupOldDbTemp = "/data/storage/el2/database/rdb/old_media_library_temp.db";

    // 不删除备份目录中的原始旧 db，以便失败重入
    mediaLibraryRdb_.reset();
    mediaLibraryRdb_ = nullptr;

    // 1. 将处理后的旧 db 临时副本 rename 到主库位置（转正）
    MEDIA_INFO_LOG("FinalizeDatabaseSwap: move upgraded old db to new db path");
    if (!MoveDbFileGroup(backupOldDbTemp, newDbPath, true)) {
        MEDIA_ERR_LOG("FinalizeDatabaseSwap: move old db temp to new db path failed");
        return false;
    }

    // 2. 清理主库和 source 库 binlog 残留，保留 source_slave 以便 source 开库失败时恢复。
    MEDIA_INFO_LOG("FinalizeDatabaseSwap: cleaning up db binlog residuals");
    CleanDbBinlogResiduals();
    // 媒体库主进程重新开库
    int32_t REVERSE_OPEN_DATABASE = 2;
    NotifyDbStatusForClone(REVERSE_OPEN_DATABASE);
    CHECK_AND_PRINT_LOG(ReverseCloneRestoreMarker::Recreate(), "recreate reverse clone restore marker failed");
    MediaLibraryDataManager::GetInstance()->InitDatabaseACLPermission();
    InsertForAncoProcess();

    // 3. 初始化 destRdb_ 和 sourceRdb_
    MEDIA_INFO_LOG("FinalizeDatabaseSwap: init dest and source rdb stores");
    mediaRdb_.reset();
    mediaLibraryRdb_.reset();

    if (!InitSourceAndDestRdb(newDbPath, newDbSourcePath, "FinalizeDatabaseSwap")) {
        MEDIA_ERR_LOG("FinalizeDatabaseSwap: init source and dest rdb failed");
        return false;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" DatabaseSwap: ")
        .append(std::to_string(endTime - startTime) + ";");
    MEDIA_INFO_LOG("FinalizeDatabaseSwap: completed successfully");
    return true;
}

bool ReverseCloneRestore::InitDatabasesForResume()
{
    const std::string newDbPath = "/data/storage/el2/database/rdb/media_library.db";
    const std::string newDbSourcePath = "/data/storage/el2/database/rdb/media_library_source.db";

    MEDIA_INFO_LOG("InitDatabasesForResume: init dest and source rdb stores");

    if (!InitSourceAndDestRdb(newDbPath, newDbSourcePath, "InitDatabasesForResume")) {
        MEDIA_ERR_LOG("InitDatabasesForResume: init source and dest rdb failed");
        return false;
    }

    MEDIA_INFO_LOG("InitDatabasesForResume: completed successfully");
    return true;
}

bool ReverseCloneRestore::InitSourceAndDestRdb(const std::string &newDbPath, const std::string &newDbSourcePath,
                                               const std::string &logTag)
{
    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "%{public}s: get context failed", logTag.c_str());

    int32_t err = BackupDatabaseUtils::InitDb(destRdb_, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                              newDbPath, CONST_BUNDLE_NAME, true, context->GetArea(), false);
    CHECK_AND_RETURN_RET_LOG(destRdb_ != nullptr, false,
        "%{public}s: init destRdb_ failed, err=%{public}d", logTag.c_str(), err);

    // 使用同目录下的 source 文件初始化 sourceRdb_
    err = BackupDatabaseUtils::InitDb(sourceRdb_, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                      newDbSourcePath, CONST_BUNDLE_NAME, true, context->GetArea(), false);
    if (sourceRdb_ == nullptr) {
        MEDIA_ERR_LOG("%{public}s: init sourceRdb_ failed, err=%{public}d, try recover", logTag.c_str(), err);
        if (!RecoverSourceDbFromSlave()) {
            MEDIA_ERR_LOG("%{public}s: recover source db from slave failed", logTag.c_str());
            return false;
        }
        err = BackupDatabaseUtils::InitDb(sourceRdb_, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                          newDbSourcePath, CONST_BUNDLE_NAME, true, context->GetArea(), false);
        CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, false,
            "%{public}s: init recovered sourceRdb_ failed, err=%{public}d", logTag.c_str(), err);
    }

    // 打印两个数据库的照片数量
    int32_t destCount = BackupDatabaseUtils::GetDbPhotoCount(destRdb_, false);
    int32_t sourceCount = BackupDatabaseUtils::GetDbPhotoCount(sourceRdb_, false);
    MEDIA_INFO_LOG("%{public}s: destRdb_ photo count=%{public}d, sourceRdb_ photo count=%{public}d",
        logTag.c_str(), destCount, sourceCount);

    return true;
}

bool ReverseCloneRestore::PrepareForResume()
{
    MEDIA_INFO_LOG("PrepareForResume: start preparing for resume");

    // 1. 初始化数据库
    if (!InitDatabasesForResume()) {
        MEDIA_ERR_LOG("PrepareForResume: init databases failed");
        return false;
    }

    // 2. 初始化 isAccountValid_ 和 isSrcDstSwitchStatusMatch_
    GetAccountValid();
    GetSyncSwitchOn();

    // 3. 初始化配置信息
    srcCloneRestoreConfigInfo_ = GetCloneConfigInfoFromOriginDB();
    dstCloneRestoreConfigInfo_ = GetCurrentDeviceCloneConfigInfo();
    CheckSrcDstSwitchStatusMatch();

    MEDIA_INFO_LOG("PrepareForResume: isAccountValid_=%{public}d, isSrcDstSwitchStatusMatch_=%{public}d",
        isAccountValid_, isSrcDstSwitchStatusMatch_);

    // 4. 初始化 resourceInheritHelper_ 中的 fileIdOffsetRules_
    // todo 持久化
    int64_t oldMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(sourceRdb_);
    int64_t newMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(destRdb_);
    if (oldMaxFileId > 0 && newMaxFileId > 0) {
        resourceInheritHelper_.AddFileIdOffsetRule(oldMaxFileId, newMaxFileId);
        MEDIA_INFO_LOG("PrepareForResume: added FileIdOffsetRule, oldMaxFileId=%{public}lld, newMaxFileId=%{public}lld",
            static_cast<long long>(oldMaxFileId), static_cast<long long>(newMaxFileId));
    }

    // 5. 初始化 resourceInheritHelper_ 中的 originalPureCloudFileIds_
    resourceInheritHelper_.SnapshotPureCloudFileIds(destRdb_);

    // 6. 初始化 tableColumnStatusMap_ （新增）
    CheckTableColumnStatus(destRdb_, CLONE_TABLE_LISTS_PHOTO);

    MEDIA_INFO_LOG("PrepareForResume: completed successfully");
    return true;
}

bool ReverseCloneRestore::ReplaceDirWithBackup(ReverseCloneRestore::AssetMoveState &move)
{
    move.hadSrc = IsPathExists(move.src);
    move.hadDst = IsPathExists(move.dst);
    if (!move.hadSrc && !move.hadDst) {
        MEDIA_INFO_LOG("ReplaceDirWithBackup: both not exist, skip");
        return true;
    }

    size_t lastSlashPos = move.backup.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        MEDIA_ERR_LOG("ReplaceDirWithBackup: invalid backup path, no slash found: %{public}s", move.backup.c_str());
        return false;
    }
    std::string parentDir = move.backup.substr(0, lastSlashPos);
    if (!IsPathExists(parentDir) && mkdir(parentDir.c_str(), 0755) != 0 && errno != EEXIST) {
        MEDIA_ERR_LOG("ReplaceDirWithBackup: create backup parent failed, dir=%{public}s, errno=%{public}d",
            parentDir.c_str(), errno);
        return false;
    }
    if (IsPathExists(move.backup)) {
        MEDIA_WARN_LOG("ReplaceDirWithBackup: backup dir already exists, deleting it: %{public}s",
            move.backup.c_str());
        if (!MediaFileUtils::DeleteFileOrFolder(move.backup, false)) {
            MEDIA_ERR_LOG("ReplaceDirWithBackup: delete stale backup dir failed, dir=%{public}s",
                move.backup.c_str());
            return false;
        }
    }

    if (move.hadDst) {
        if (rename(move.dst.c_str(), move.backup.c_str()) != 0) {
            int savedErrno = errno;
            MEDIA_WARN_LOG("ReplaceDirWithBackup: rename dst %{public}s to backup %{public}s failed, errno=%{public}d",
                move.dst.c_str(), move.backup.c_str(), savedErrno);
            DiagnoseRenameFailure(move.dst, move.backup, savedErrno);
            return false;
        }
        MEDIA_INFO_LOG("ReplaceDirWithBackup: backed up dst %{public}s -> %{public}s", move.dst.c_str(),
            move.backup.c_str());
        move.backedUpDst = true;
    }

    if (move.hadSrc) {
        if (rename(move.src.c_str(), move.dst.c_str()) != 0) {
            int savedErrno = errno;
            MEDIA_WARN_LOG("ReplaceDirWithBackup: rename src %{public}s to dst %{public}s failed, errno=%{public}d",
                move.src.c_str(), move.dst.c_str(), savedErrno);
            DiagnoseRenameFailure(move.src, move.dst, savedErrno);
            if (move.hadDst && !RollbackAssetMove(move)) {
                MEDIA_ERR_LOG("ReplaceDirWithBackup: rollback current asset dir failed, src=%{public}s",
                    move.src.c_str());
                return false;
            }
            return false;
        }
        MEDIA_INFO_LOG("ReplaceDirWithBackup: moved src %{public}s -> %{public}s", move.src.c_str(),
            move.dst.c_str());
        move.movedSrc = true;
    }
    return true;
}

bool ReverseCloneRestore::RollbackAssetMove(const AssetMoveState &move)
{
    bool ret = true;
    if (move.movedSrc) {
        if (IsPathExists(move.src)) {
            MEDIA_INFO_LOG("RollbackAssetMove: source already restored, src=%{public}s", move.src.c_str());
        } else if (IsPathExists(move.dst)) {
            if (rename(move.dst.c_str(), move.src.c_str()) != 0) {
                int savedErrno = errno;
                MEDIA_ERR_LOG("RollbackAssetMove: restore source %{public}s from dst %{public}s failed, "
                    "errno=%{public}d", move.src.c_str(), move.dst.c_str(), savedErrno);
                DiagnoseRenameFailure(move.dst, move.src, savedErrno);
                ret = false;
            } else {
                MEDIA_INFO_LOG("RollbackAssetMove: restored source %{public}s from dst %{public}s",
                    move.src.c_str(), move.dst.c_str());
            }
        } else {
            MEDIA_ERR_LOG("RollbackAssetMove: source and dst are both missing, src=%{public}s, dst=%{public}s",
                move.src.c_str(), move.dst.c_str());
            ret = false;
        }
    } else if (move.hadSrc && !IsPathExists(move.src)) {
        MEDIA_ERR_LOG("RollbackAssetMove: source missing after failed move, src=%{public}s", move.src.c_str());
        ret = false;
    }

    CHECK_AND_RETURN_RET(move.backedUpDst, ret);

    if (!IsPathExists(move.backup)) {
        CHECK_AND_RETURN_RET_LOG(!IsPathExists(move.dst), ret,
            "RollbackAssetMove: backup already restored, dst=%{public}s", move.dst.c_str());
        MEDIA_ERR_LOG("RollbackAssetMove: backup and dst are both missing, backup=%{public}s, dst=%{public}s",
            move.backup.c_str(), move.dst.c_str());
        return false;
    }
    if (IsPathExists(move.dst)) {
        MEDIA_ERR_LOG("RollbackAssetMove: dst still exists before restoring backup, dst=%{public}s",
            move.dst.c_str());
        return false;
    }
    if (rename(move.backup.c_str(), move.dst.c_str()) != 0) {
        int savedErrno = errno;
        MEDIA_ERR_LOG("RollbackAssetMove: restore backup %{public}s to dst %{public}s failed, errno=%{public}d",
            move.backup.c_str(), move.dst.c_str(), savedErrno);
        DiagnoseRenameFailure(move.backup, move.dst, savedErrno);
        return false;
    }
    MEDIA_INFO_LOG("RollbackAssetMove: restored backup %{public}s to dst %{public}s", move.backup.c_str(),
        move.dst.c_str());
    return ret;
}

bool ReverseCloneRestore::RollbackCompletedAssetMoves()
{
    bool ret = true;
    for (auto iter = completedAssetMoves_.rbegin(); iter != completedAssetMoves_.rend(); ++iter) {
        ret = RollbackAssetMove(*iter) && ret;
    }
    if (ret) {
        completedAssetMoves_.clear();
    }
    return ret;
}

bool ReverseCloneRestore::SetCompletedAssetMovesAndRollback(const std::vector<AssetMoveState> &moves)
{
    MEDIA_INFO_LOG("SetCompletedAssetMovesAndRollback: setting %{public}zu moves", moves.size());
    completedAssetMoves_ = moves;
    return RollbackCompletedAssetMoves();
}

bool ReverseCloneRestore::SaveAssetMoveToXml(const AssetMoveState &move, int index)
{
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(ASSET_MOVES_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("SaveAssetMoveToXml: get preferences failed, errCode=%{public}d", errCode);
        return false;
    }

    // 使用序列化的字符串存储
    std::string key = ASSET_MOVE_KEY_PREFIX + std::to_string(index);
    std::string value = move.src + "|" + move.dst + "|" + move.backup + "|" +
                       std::to_string(move.hadSrc) + "|" + std::to_string(move.hadDst) + "|" +
                       std::to_string(move.movedSrc) + "|" + std::to_string(move.backedUpDst);

    prefs->PutString(key, value);
    prefs->FlushSync();
    return true;
}

std::vector<ReverseCloneRestore::AssetMoveState> ReverseCloneRestore::GenerateAssetMoveStates(
    const std::string& backupRoot, const std::string& reverseRestoreBase)
{
    std::vector<AssetMoveState> moves = {
        { backupRoot + "/storage/media/local/files/Photo",     "/storage/media/local/files/Photo",
          reverseRestoreBase + "/Photo" },
        { backupRoot + "/storage/media/local/files/Camera",    "/storage/media/local/files/Camera",
          reverseRestoreBase + "/Camera" },
        { backupRoot + "/storage/media/local/files/Pictures",  "/storage/media/local/files/Pictures",
          reverseRestoreBase + "/Pictures" },
        { backupRoot + "/storage/media/local/files/Videos",    "/storage/media/local/files/Videos",
          reverseRestoreBase + "/Videos" },
        { backupRoot + "/storage/media/local/files/.editData", "/storage/media/local/files/.editData",
          reverseRestoreBase + "/.editData" },
        { backupRoot + "/storage/media/local/files/Audio",     "/storage/media/local/files/Audio",
          reverseRestoreBase + "/Audio" },
        { backupRoot + "/storage/media/local/files/Audios",    "/storage/media/local/files/Audios",
          reverseRestoreBase + "/Audios" },
        { backupRoot + "/storage/media/local/files/.thumbs",   "/storage/media/local/files/.thumbs",
          reverseRestoreBase + "/.thumbs" },
        { backupRoot + "/storage/media/local/files/highlight", "/storage/media/local/files/highlight",
          reverseRestoreBase + "/highlight" },
    };
    return moves;
}

bool ReverseCloneRestore::MoveAssets(const std::string& backupRoot)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    const std::string reverseRestoreBase = REVERSE_RESTORE_RESOURCE_ROOT;
    if (!IsPathExists(reverseRestoreBase)) {
        if (mkdir(reverseRestoreBase.c_str(), 0755) != 0 && errno != EEXIST) {
            MEDIA_ERR_LOG("MoveAssets: create reverse_restore dir failed");
            return false;
        }
    }
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(ASSET_MOVES_XML, errCode);
    if (prefs != nullptr) {
        prefs->PutInt(ASSET_MOVE_COUNT_KEY, 0);
        prefs->FlushSync();
    }
    completedAssetMoves_.clear();
    std::vector<AssetMoveState> moves = GenerateAssetMoveStates(backupRoot, reverseRestoreBase);
    for (const auto& m : moves) {
        MEDIA_INFO_LOG("MoveAssets: src=%{public}s, dst=%{public}s, backup=%{public}s", m.src.c_str(), m.dst.c_str(),
            m.backup.c_str());
        AssetMoveState move = m;
        if (!ReplaceDirWithBackup(move)) {
            MEDIA_ERR_LOG("MoveAssets: failed for src=%{public}s", m.src.c_str());
            if (move.movedSrc || move.backedUpDst) {
                completedAssetMoves_.push_back(move);
                SaveAssetMoveToXml(move, completedAssetMoves_.size() - 1);
            }
            if (!RollbackCompletedAssetMoves()) {
                MEDIA_ERR_LOG("MoveAssets: rollback completed asset moves failed");
            }
            return false;
        }
        if (move.movedSrc || move.backedUpDst) {
            completedAssetMoves_.push_back(move);
            SaveAssetMoveToXml(move, completedAssetMoves_.size() - 1);
        }
    }
    if (prefs != nullptr) {
        prefs->PutInt(ASSET_MOVE_COUNT_KEY, completedAssetMoves_.size());
        prefs->FlushSync();
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" MoveAssets: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

bool ReverseCloneRestore::RestoreDirectoriesFromBackup()
{
    if (completedAssetMoves_.empty()) {
        MEDIA_INFO_LOG("RestoreDirectoriesFromBackup: no completed asset moves, skip");
        return true;
    }
    return RollbackCompletedAssetMoves();
}

bool ReverseCloneRestore::DoDataBaseUpgrade(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    string eventPath = backupRestoreDir_ + CLONE_RESTORE_PREFERENCE_PATH + MEDIA_DB_UPGRADE_EVENTS_XML;
    string configPath = backupRestoreDir_ + CLONE_RESTORE_PREFERENCE_PATH + MEDIA_DB_UPGRADE_VERSION_XML;
    if (oldDbVersion_ > MEDIA_RDB_VERSION) {
        MEDIA_WARN_LOG("old database version %{public}d bigger than current version %{public}d", oldDbVersion_,
            MEDIA_RDB_VERSION);
        return false;
    }
    reverseRestoreReportInfo_.restoreDatabaseVersion = std::to_string(oldDbVersion_) +
        " | " + std::to_string(MEDIA_RDB_VERSION);
    UpgradeManagerConfig config(true, eventPath, configPath, oldDbVersion_, MEDIA_RDB_VERSION);
    int32_t ret = UpgradeManager::GetInstance().Initialize(config);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpgradeManager Initialize failed: %{public}d", ret);
        reverseRestoreReportInfo_.databaseUpgradeResultInfo = "Initialize failed";
        return false;
    }
    ret = UpgradeManager::GetInstance().UpgradeSync(*oldDbTempStore);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpgradeManager UpgradeSync failed: %{public}d", ret);
        reverseRestoreReportInfo_.databaseUpgradeResultInfo = "UpgradeSync failed";
        return false;
    }
    ret = UpgradeManager::GetInstance().UpgradeAsync(*oldDbTempStore);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpgradeManager UpgradeAsync failed: %{public}d", ret);
        reverseRestoreReportInfo_.databaseUpgradeResultInfo = "UpgradeAsync failed";
        return false;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    bool isSubset = UpgradeManager::GetInstance().IsSchemaSubsetByAttach(*oldDbTempStore, *mediaLibraryRdb_);
    reverseRestoreReportInfo_.restoreDatabaseContains = static_cast<int32_t>(isSubset);
    reverseRestoreReportInfo_.databaseUpgradeResult = 1;
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" UpgradeCost: ")
        .append(std::to_string(endTime - startTime) + ";");
    CHECK_AND_RETURN_RET_LOG(isSubset, false, "Upgraded new db schema is not subset of old db");
    return true;
}

void ReverseCloneRestore::ReverseRestoreClassifyData()
{
    CloneReverseRestoreClassify reverseRestoreClassify;
    ClassifyCloneRestoreConfig config;
    config.sceneCode = sceneCode_;
    config.taskId = taskId_;
    // destRdb_ 最终db
    config.mediaLibraryRdb = sourceRdb_;
    config.mediaRdb = destRdb_;
    config.scoreMaskMap = &scoreMaskMap_;
    config.duplicateMap = &duplicateAssetMap_;
    reverseRestoreClassify.Init(config);
    reverseRestoreClassify.Restore();
}

void ReverseCloneRestore::RestoreReverseAnalysisGeo()
{
    CloneReverseRestoreGeo reverseRestoreGeo;
    reverseRestoreGeo.Init(sceneCode_, taskId_, sourceRdb_, destRdb_);
    reverseRestoreGeo.Restore();
}

void ReverseCloneRestore::RestoreReverseAnalysisPortrait()
{
    CloneRestorePortrait portraitAlbumClone;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    // sourceRdb_ 新机db
    // destRdb_ 转正后的旧db
    portraitAlbumClone.Init(
        sceneCode_, taskId_, sourceRdb_, destRdb_, reversePhotoInfoMap_,
        isCloudRestoreSatisfied, &scoreMaskMap_);
    if (!duplicateAssetMap_.empty()) {
        BackupDatabaseUtils::UpdateDuplicateCoverUris(destRdb_, duplicateAssetMap_,
            reversePhotoInfoMap_, {PORTRAIT, GROUP_PHOTO});
    }
    // 判断旧机是否有数据，没有数据则需从新机吸收
    if (!portraitAlbumClone.RefreshTotalAlbumNumber()) {
        portraitAlbumClone.Init(
            sceneCode_, taskId_, destRdb_, sourceRdb_, reversePhotoInfoMap_,
            isCloudRestoreSatisfied, &scoreMaskMap_);
        portraitAlbumClone.RefreshTotalAlbumNumber();
        portraitAlbumClone.Restore(true);
        // 合影
        RestoreReverseGroupPhoto();
    } else {
        // 旧机有人像数据，则处理新机tab_analysis_profile表
        portraitAlbumClone.ClearProfileFaceScore(sourceRdb_);
        portraitAlbumClone.ClearTotalScoreBit2(sourceRdb_);
        portraitAlbumClone.DeleteExistingGdbPortraitData();
    }
}

void ReverseCloneRestore::RestoreReverseGroupPhoto()
{
    MEDIA_INFO_LOG("start RestoreReverseGroupPhoto");
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    // 旧机有数据，则不吸收新机数据；旧机无数据，就插入新机数据，跳过删除流程
    CloneRestoreGroupPhoto cloneRestoreGroupPhoto;
    cloneRestoreGroupPhoto.Init(
        sceneCode_, taskId_, restoreInfo_, destRdb_, sourceRdb_, isCloudRestoreSatisfied);
    cloneRestoreGroupPhoto.Restore(reversePhotoInfoMap_, true);
    MEDIA_INFO_LOG("end RestoreReverseGroupPhoto");
}

void ReverseCloneRestore::ReverseRestoreSearchIndexData()
{
    // 正向：重复以新机为准，非重复删除新机并插入旧机（以旧机为准）
    // 反向：直接插入旧机
    maxSearchId_ = BackupDatabaseUtils::QueryMaxId(sourceRdb_,
        ANALYSIS_SEARCH_INDEX_TABLE, SEARCH_IDX_COL_ID);
    SearchIndexClone searchIndexClone(sourceRdb_, destRdb_, reversePhotoInfoMap_, maxSearchId_);
    searchIndexClone.ReverseClone();
}

void ReverseCloneRestore::ReverseRestoreBeautyScoreData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: beauty score data");
    maxBeautyFileId_ = BackupDatabaseUtils::QueryMaxId(sourceRdb_,
        ANALYSIS_BEAUTY_SCORE_TABLE, BEAUTY_SCORE_COL_FILE_ID);
    BeautyScoreClone beautyScoreClone(sourceRdb_, destRdb_, reversePhotoInfoMap_,
        maxBeautyFileId_, &scoreMaskMap_);
    beautyScoreClone.ReverseCloneBeautyScoreInfo();
}

void ReverseCloneRestore::ReverseRestoreVideoFaceData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: video face data");
    VideoFaceClone videoFaceClone(sourceRdb_, destRdb_, reversePhotoInfoMap_, true);
    videoFaceClone.CloneVideoFaceInfo();
}

void ReverseCloneRestore::ReverseRestoreAiRetouchData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: ai retouch data");
    maxTotalFileId_ = BackupDatabaseUtils::QueryMaxId(mediaRdb_,
        VISION_TOTAL_TABLE, TOTAL_COL_FILE_ID);
    AiRetouchClone aiRetouchClone(sourceRdb_, destRdb_, reversePhotoInfoMap_,
        maxTotalFileId_, taskId_, true);
    aiRetouchClone.ReverseCloneAiRetouchInfo();
}

void ReverseCloneRestore::ReverseRestoreDupSimData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: dup sim data");
    CloneRestoreDupSim dupSimRestore;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    dupSimRestore.Init(
        sceneCode_, taskId_, destRdb_, sourceRdb_, reversePhotoInfoMap_,
        isCloudRestoreSatisfied, &scoreMaskMap_);
    dupSimRestore.Restore();
}

void ReverseCloneRestore::ReverseRestoreWatermarkData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: watermark data");
    WaterMarkClone waterMarkClone(sourceRdb_, destRdb_, reversePhotoInfoMap_);
    waterMarkClone.ReverseClone();
}

void ReverseCloneRestore::ReverseRestoreSelectionData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: selection data");
    CloneRestoreSelection selectionRestore;
    bool isCloudRestoreSatisfied = IsCloudRestoreSatisfied();
    selectionRestore.Init(sceneCode_, taskId_, sourceRdb_, destRdb_,
        reversePhotoInfoMap_, isCloudRestoreSatisfied);
    // 正向：旧机有数据则删除新机
    // 反向：旧机有数据则不吸收新机
    if (selectionRestore.RefreshTotalNumber()) {
        selectionRestore.Init(sceneCode_, taskId_, destRdb_, sourceRdb_,
            reversePhotoInfoMap_, isCloudRestoreSatisfied);
        selectionRestore.RefreshTotalNumber();
        selectionRestore.Restore();
    }
}

void ReverseCloneRestore::ReverseRestoreAnalysisTablesData()
{
    MEDIA_INFO_LOG("Start reverse clone restore: analysis tables data");

    cloneRestoreAnalysisData_.Init(this->sceneCode_, this->taskId_, sourceRdb_, destRdb_);
    std::unordered_set<std::string> excludedColumns = {"id", "file_id"};
    vector<std::string> analysisTables = {
        "tab_analysis_head",
        "tab_analysis_pose",
        "tab_analysis_composition",
        "tab_analysis_ocr",
        "tab_analysis_segmentation",
        "tab_analysis_object",
        "tab_analysis_saliency_detect",
        "tab_analysis_recommendation",
        "tab_analysis_crop",
        "tab_analysis_caption"
    };

    vector<std::string> totalTypes = {
        "head",
        "pose",
        "composition",
        "ocr",
        "segmentation",
        "object",
        "saliency",
        "recommendation",
        "aesthetics_crop",
        "caption"
    };

    for (size_t index = 0; index < analysisTables.size(); index++) {
        std::string table = analysisTables[index];
        std::string type = totalTypes[index];
        cloneRestoreAnalysisData_.CloneAnalysisData(table, type, reversePhotoInfoMap_, excludedColumns);
    }

    MEDIA_INFO_LOG("Reverse clone analysis tables completed");
}

void ReverseCloneRestore::ReverseRestorePortraitNickNameData()
{
    // 反向克隆策略，如果旧机有，就无需克；如果旧机没有，就需要插入
    auto albumIdMapIt = tableAlbumIdMap_.find(ANALYSIS_ALBUM_TABLE);
    CHECK_AND_RETURN_LOG(albumIdMapIt != tableAlbumIdMap_.end(),
        "analysis album map not found, skip portrait nickname clone");
    PortraitNickNameClone portraitNickNameClone(sourceRdb_, destRdb_, albumIdMapIt->second,
        IsCloudRestoreSatisfied());
    portraitNickNameClone.Clone();
}

void ReverseCloneRestore::RestoreReverseHighlightAlbums()
{
    // 正向：不满足云图迁移且旧机有时刻数据，则不迁移
    // 反向：不满足云图迁移且旧机有时刻数据，删除旧机，插入新机；其他场景，插入新机
    mediaRdb_ = destRdb_;
    int32_t highlightCloudMediaCnt = GetHighlightCloudMediaCnt();
    CloneRestoreHighlight cloneRestoreHighlight;
    string reverseHighlightPath = "/storage/media/local/files/reverse_restore/highlight/";
    CloneRestoreHighlight::InitInfo initInfo = { sceneCode_, taskId_, destRdb_, sourceRdb_, reverseHighlightPath,
        reversePhotoInfoMap_, duplicateAssetMap_ };
    UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_)
        .Report("CLONE_RESTORE_HIGHLIGHT_CHECK", "",
        "highlightCloudMediaCnt: " + std::to_string(highlightCloudMediaCnt) +
        ", isCloudRestoreSatisfied: " + std::to_string(IsCloudRestoreSatisfied()));
    if (highlightCloudMediaCnt != 0 && !IsCloudRestoreSatisfied()) {
        MEDIA_INFO_LOG("old highlightAlbum condition not met, clear old data");
        cloneRestoreHighlight.ClearHighlightAlbums();
    }
    cloneRestoreHighlight.ReverseInit(initInfo);
    cloneRestoreHighlight.ReverseRestore();

    CloneRestoreCVAnalysis cloneRestoreCVAnalysis;
    cloneRestoreCVAnalysis.Init(sceneCode_, taskId_, destRdb_, sourceRdb_, reverseHighlightPath);
    cloneRestoreCVAnalysis.RestoreAlbums(cloneRestoreHighlight);
    // 反向存储智慧相册映射
    StoreHighlightAlbumMappings(cloneRestoreHighlight);

    cloneRestoreHighlight.ReportCloneRestoreHighlightTask();
    // 刷新时刻相册is_viewed
    cloneRestoreHighlight.UpdateHighlightAlbumsViewed();
    mediaRdb_ = sourceRdb_;
}

void ReverseCloneRestore::ReverseRestoreTabOldAlbumsData(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore)
{
    // oldDbTempStore 旧机DB
    // mediaLibraryRdb_ 新机DB
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    TabOldAlbumsClone tabOldAlbumsClone(nullptr, mediaLibraryRdb_, tableAlbumIdMap_, true);
    // 查询新机 clone_sequence
    tabOldAlbumsClone.GetNextCloneSequence();
    // 从oldDbTempStore插入oldDbTempStore
    tabOldAlbumsClone.InitDatabase(oldDbTempStore, oldDbTempStore);
    // 根据tab_old_album表映射更新智慧相册映射
    // 插入映射关系
    tabOldAlbumsClone.CloneAlbumsFromPhotoAlbum();
    tabOldAlbumsClone.CloneAlbumsFromAnalysisAlbum();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" Tab_old_album Restore: ")
        .append(std::to_string(endTime - startTime) + ";");
}

void ReverseCloneRestore::PopulateAnalysisAlbumIdMap(const std::vector<int32_t>& subtypes)
{
    MEDIA_INFO_LOG("Mapping portrait, group photo, and shooting mode albums");
    CHECK_AND_RETURN_LOG(!subtypes.empty(), "empty subtype");

    std::string srcSql = BuildSourceQuerySql(subtypes);
    auto srcResultSet = mediaRdb_->QuerySql(srcSql);
    CHECK_AND_RETURN_LOG(srcResultSet != nullptr, "Failed to query special album types from source");

    auto& albumIdMap = tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE];
    int32_t mappedCount = 0;

    while (srcResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldAlbumId = GetInt32Val("album_id", srcResultSet);
        int32_t albumType = GetInt32Val("album_type", srcResultSet);
        int32_t albumSubtype = GetInt32Val("album_subtype", srcResultSet);
        std::string albumName = GetStringVal("album_name", srcResultSet);
        std::string tagId = GetStringVal("tag_id", srcResultSet);
        if (albumIdMap.find(oldAlbumId) != albumIdMap.end()) {
            continue;
        }

        if (auto newAlbumId = FindMatchingAlbum(albumType, albumSubtype, albumName, tagId)) {
            if (!newAlbumId.has_value()) {
                continue;
            }
            if (newAlbumId.value() == oldAlbumId) {
                MEDIA_INFO_LOG("old database has portrait data, skip nickname");
                continue;
            }
            tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE][oldAlbumId] = newAlbumId.value();
            mappedCount++;
            MEDIA_DEBUG_LOG("Mapped album (subtype=%{public}d): old=%{public}d -> new=%{public}d",
                albumSubtype, oldAlbumId, *newAlbumId);
        }
    }
    srcResultSet->Close();

    MEDIA_INFO_LOG("Album mapping completed. Mapped %{public}d albums", mappedCount);
}

void ReverseCloneRestore::ReverseRestoreAnalysisData()
{
    // sourceRdb_ 新机db
    // destRdb_ 转正后的旧db
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    mediaRdb_ = sourceRdb_;
    mediaLibraryRdb_ = destRdb_;
    // 拍摄模式
    MergeShootingModeAlbums();
    // 地理
    cloneRestoreGeoDictionary_.Init(sceneCode_, taskId_, destRdb_, sourceRdb_);
    cloneRestoreGeoDictionary_.ReverseRestoreAlbums();
    // 分类
    ReverseRestoreClassifyData();
    // 城市
    RestoreReverseAnalysisGeo();
    // 人像 & 合影
    RestoreReverseAnalysisPortrait();
    // 地理任务上报
    cloneRestoreGeoDictionary_.ReportGeoRestoreTask();
    // 搜索数据
    ReverseRestoreSearchIndexData();
    // 美学评分
    ReverseRestoreBeautyScoreData();
    // 视频人脸
    ReverseRestoreVideoFaceData();
    // 多项智慧表
    ReverseRestoreAnalysisTablesData();
    // 时刻相册
    RestoreReverseHighlightAlbums();
    // AI构图
    ReverseRestoreAiRetouchData();
    // 时刻水印
    ReverseRestoreWatermarkData();
    // 重复相似
    ReverseRestoreDupSimData();
    // 精选视图
    ReverseRestoreSelectionData();
    // 智慧相册id映射，复用正向，放到最后
    PopulateAnalysisAlbumIdMap({
        PhotoAlbumSubType::PORTRAIT
    });
    // 人像昵称依赖智慧相册映射，放到最后
    ReverseRestorePortraitNickNameData();
    // 重复资产特殊处理
    FileIdMigrator migrator;
    migrator.UpdateFaceTableFileIds(destRdb_, duplicateAssetMap_);
    migrator.UpdateAnalysisTotalFields(destRdb_, duplicateAssetMap_);
    migrator.MigrateAnalysisTotalScore(destRdb_, sourceRdb_, duplicateAssetMap_);
    // 更新AnalysisPhotoMap的map_asset_date_taken列，智慧相册更新完后刷新
    CloneRestore::UpdatePhotoMapAssetDateTaken();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" AbsorbSmart: ")
        .append(std::to_string(endTime - startTime) + ";");
}

void ReverseCloneRestore::DealDuplicatePhotoAlbum()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    TabOldAlbumsClone tabOldAlbumsClone(destRdb_, destRdb_, tableAlbumIdMap_);
    tabOldAlbumsClone.UpdateAlbumIdsByMappingWithSubtype();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" Tab_old_album Process: ")
        .append(std::to_string(endTime - startTime) + ";");
}

bool ReverseCloneRestore::ReInitForForwardRestore(const std::string &backupRestorePath,
                                                  const std::string &upgradePath)
{
    MEDIA_INFO_LOG("ReInitForForwardRestore: re-initializing for forward clone");

    // 关闭可能残留的句柄
    mediaRdb_.reset();
    mediaLibraryRdb_.reset();

    // 重新设置系统参数（可重复执行）
    SetParameterForClone();
    SetParameterForRestore();

    // 重新设置备份目录和垃圾路径
    backupRestoreDir_ = backupRestorePath;
    if (backupRestoreDir_.empty()) {
        MEDIA_ERR_LOG("backupRestoreDir_ is empty.");
        return false;
    }
    garbagePath_ = backupRestoreDir_ + "/storage/media/local/files";

    // 重新初始化数据库句柄（打开新机主库和备份旧库）
    int32_t err = Init(backupRestoreDir_, upgradePath, true);
    if (err != E_OK) {
        MEDIA_ERR_LOG("ReInitForForwardRestore: Init failed, err=%{public}d", err);
        SetErrorCode(RestoreError::INIT_FAILED);
        return false;
    }

    MEDIA_INFO_LOG("ReInitForForwardRestore: success");
    return true;
}

void ReverseCloneRestore::FallbackToForwardRestore(const std::string &backupRestorePath,
                                                   const std::string &upgradePath, bool isEarlyStage)
{
    MEDIA_INFO_LOG("FallbackToForwardRestore begin, isEarlyStage=%{public}d", isEarlyStage);
    if (!RollbackReverseRestore(isEarlyStage)) {
        MEDIA_ERR_LOG("FallbackToForwardRestore: rollback reverse restore failed");
        SetErrorCode(RestoreError::INIT_FAILED);
        return;
    }
    if (ReInitForForwardRestore(backupRestorePath, upgradePath)) {
        CloneRestore::StartRestore(backupRestorePath, upgradePath);
    } else {
        MEDIA_ERR_LOG("FallbackToForwardRestore: re-init failed, cannot fall back to forward");
        SetErrorCode(RestoreError::INIT_FAILED);
    }
}

void ReverseCloneRestore::HandleRestData()
{
    MEDIA_INFO_LOG("ReverseCloneRestore::HandleRestData start");
    this->restorePhotosAlbumHidden_.UpdateEmptyAlbumHidden(destRdb_);
    MEDIA_INFO_LOG("ReverseCloneRestore::HandleRestData end");
}

void ReverseCloneRestore::ResetReverseRestoreState()
{
    resourceInheritHelper_.Reset();
    completedAssetMoves_.clear();
}

void ReverseCloneRestore::ActiveFullDonation()
{
    MEDIA_INFO_LOG("ActiveFullDonation: start full donation for reverse clone");
    bool enableOldPath =
        system::GetBoolParameter("persist.multimedia.media_analysis_service.search_oldpath_enable", true);
    CHECK_AND_RETURN_INFO_LOG(!enableOldPath, "search_oldpath_enable is true, skip");
    std::vector<std::string> tables;
    tables.push_back("Photos");
    destRdb_->SetDistributedTables(tables, DistributedRdb::DISTRIBUTED_SEARCH, {.isRebuild = true});
}

void ReverseCloneRestore::AppendErrorInfo(const std::string &errorInfo)
{
    if (!reverseRestoreReportInfo_.absorbNewBasicDataErrorInfo.empty()) {
        reverseRestoreReportInfo_.absorbNewBasicDataErrorInfo += "; ";
    }
    reverseRestoreReportInfo_.absorbNewBasicDataErrorInfo += errorInfo;
}

void ReverseCloneRestore::MergeShootingModeAlbums()
{
    MEDIA_INFO_LOG("MergeShootingModeAlbums start");
    ShootingModeAlbumClone clone(sourceRdb_, destRdb_);
    clone.Execute();
    MEDIA_INFO_LOG("MergeShootingModeAlbums completed");
}

void ReverseCloneRestore::UpdateReverseRestoreStatusTimestampThread(std::atomic<bool>& shouldUpdate)
{
    MEDIA_INFO_LOG("UpdateReverseRestoreStatusTimestampThread started");
    constexpr int64_t TWO_MINUTES_MS = 2 * 60 * 1000;

    while (shouldUpdate.load()) {
        std::string reverseRestoreStatus;
        int32_t ret = SettingsDataManager::GetReverseRestoreStatus(reverseRestoreStatus);
        if (ret == E_OK && !reverseRestoreStatus.empty()) {
            MEDIA_DEBUG_LOG("UpdateReverseRestoreStatusTimestampThread: updating timestamp");
            ret = SettingsDataManager::UpdateReverseRestoreStatusTimestamp(reverseRestoreStatus);
            if (ret != E_OK) {
                MEDIA_WARN_LOG(
                    "UpdateReverseRestoreStatusTimestampThread: failed to update timestamp, ret=%{public}d", ret);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(TWO_MINUTES_MS));
    }

    MEDIA_INFO_LOG("UpdateReverseRestoreStatusTimestampThread stopped");
}

void ReverseCloneRestore::StopReverseRestoreStatusUpdateThread()
{
    MEDIA_INFO_LOG("Stopping reverse restore status update thread");
    shouldUpdateReverseRestoreStatus_.store(false);
    if (reverseRestoreStatusUpdateThread_.joinable()) {
        reverseRestoreStatusUpdateThread_.detach();
        MEDIA_INFO_LOG("Reverse restore status update thread detached");
    }
}

int32_t ReverseCloneRestore::HandleReverseRestoreStatus()
{
    // Check reverse restore status before clone
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::string reverseRestoreStatus;
    int32_t ret = SettingsDataManager::GetReverseRestoreStatus(reverseRestoreStatus);
    if (ret == E_OK && !reverseRestoreStatus.empty()) {
        MEDIA_WARN_LOG("Reverse restore status exists: %{public}s, another restore in progress, waiting...",
            reverseRestoreStatus.c_str());

        // Poll every 1 minute until status is empty or 4 hours timeout
        constexpr int64_t ONE_MINUTE_MS = 60 * 1000;
        constexpr int64_t FOUR_HOURS_MS = 4 * 60 * 60 * 1000;
        int32_t maxPolls = static_cast<int32_t>(FOUR_HOURS_MS / ONE_MINUTE_MS);

        for (int32_t i = 0; i < maxPolls; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ONE_MINUTE_MS));

            ret = SettingsDataManager::GetReverseRestoreStatus(reverseRestoreStatus);
            if (ret != E_OK) {
                MEDIA_WARN_LOG("Failed to query reverse restore status, ret: %{public}d", ret);
                continue;
            }

            if (reverseRestoreStatus.empty()) {
                MEDIA_INFO_LOG("Reverse restore status is now empty, proceeding");
                break;
            }

            MEDIA_DEBUG_LOG("Reverse restore status still exists: %{public}s, continuing to wait",
                reverseRestoreStatus.c_str());
        }

        // Check if status is still not empty after polling
        if (!reverseRestoreStatus.empty()) {
            MEDIA_ERR_LOG("Wait for reverse restore status timeout after 4 hours");
            return E_ERR;
        }
    }

    // Set reverse restore status to uncancelable
    int64_t currentTime = MediaTimeUtils::UTCTimeMilliSeconds();
    std::string newStatus = std::to_string(currentTime) + "|uncancelable";
    ret = SettingsDataManager::SetReverseRestoreStatus(newStatus);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Failed to set reverse restore status, ret: %{public}d", ret);
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" RestoreStatus: ")
        .append(std::to_string(endTime - startTime) + ";");
    return E_OK;
}

bool ReverseCloneRestore::PreprocessReverseRestore(const string &backupRestorePath, const string &upgradePath)
{
    if (preprocessErrorCode_ != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneRestore: PreprocessBeforeClone failed");
        CloneRestore::StartRestore(backupRestorePath, upgradePath);
        return false;
    }

    // wait for reverse status
    int32_t ret = HandleReverseRestoreStatus();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("wait reverse clone timeout.");
        SetErrorCode(RestoreError::RETAIN_FORCE_TIMEOUT);
        ErrorInfo errorInfo(RestoreError::RETAIN_FORCE_TIMEOUT, 0, "",
            "wait reverse clone timeout.");
        UpgradeRestoreTaskReport(sceneCode_, taskId_).ReportError(errorInfo);
        return false;
    }

    if (!ShouldUseReverseCloneRestore()) {
        MEDIA_INFO_LOG("ReverseCloneRestore: conditions not met, fallback to normal CloneRestore");
        CloneRestore::StartRestore(backupRestorePath, upgradePath);
        return false;
    }

    // Start background thread to update reverse restore status timestamp
    shouldUpdateReverseRestoreStatus_.store(true);
    reverseRestoreStatusUpdateThread_ = std::thread(UpdateReverseRestoreStatusTimestampThread,
        std::ref(shouldUpdateReverseRestoreStatus_));
    totalNumber_ = reverseRestoreReportInfo_.failedCount;
    MEDIA_INFO_LOG("Started reverse restore status update thread");

    return true;
}

bool ReverseCloneRestore::PrepareReverseOldDb(const string &backupRestorePath, const string &upgradePath,
    std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore)
{
    if (!PrepareOldDb(backupRestorePath, oldDbTempStore)) {
        MEDIA_ERR_LOG("ReverseCloneRestore: PrepareOldDb failed");
        FallbackToForwardRestore(backupRestorePath, upgradePath, true);
        return false;
    }

    if (!EnsureDbDirSpace(STAGE_UPGRADE_OLD_DB)) {
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "insufficient storage space, stage:" +
            string(STAGE_UPGRADE_OLD_DB);
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    if (!DoDataBaseUpgrade(oldDbTempStore)) {
        MEDIA_WARN_LOG("new database not subset of upgraded old database");
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "upgrade database failed";
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    if (!CleanOldDbData(oldDbTempStore)) {
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "clean old database data failed";
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    if (!CleanCloudDataIfNeeded(oldDbTempStore)) {
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "clean old database cloud data failed";
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    ReverseRestoreTabOldAlbumsData(oldDbTempStore);

    if (!EnsureDbDirSpace(STAGE_INITIAL_MIGRATION)) {
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "insufficient storage space, stage:" +
            string(STAGE_INITIAL_MIGRATION);
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    if (!PerformInitialMigration(oldDbTempStore)) {
        MEDIA_ERR_LOG("PerformInitialMigration failed");
        WaitForIntegrityCheck();
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "performInitialMigration failed";
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    // 等待异步完整性检查完成（在所有操作成功后）
    if (!WaitForIntegrityCheck()) {
        MEDIA_ERR_LOG("ReverseCloneRestore: integrity check failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "integrity check failed";
        HandlePrepareFailure(backupRestorePath, upgradePath, oldDbTempStore);
        return false;
    }

    return true;
}

void ReverseCloneRestore::HandlePrepareFailure(const string &backupRestorePath, const string &upgradePath,
    std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore)
{
    oldDbTempStore.reset();
    FallbackToForwardRestore(backupRestorePath, upgradePath, true);
}

bool ReverseCloneRestore::StartIntegrityCheckAsync(const std::string& backupRestorePath,
    const std::string& upgradePath)
{
    integrityChecker_ = std::make_unique<DbIntegrityChecker>(backupRestorePath, upgradePath);
    if (integrityChecker_ == nullptr) {
        MEDIA_ERR_LOG("StartIntegrityCheckAsync: create checker failed");
        return false;
    }

    integrityChecker_->StartAsyncCheck();
    MEDIA_INFO_LOG("StartIntegrityCheckAsync: integrity check started");
    return true;
}

bool ReverseCloneRestore::WaitForIntegrityCheck()
{
    if (integrityChecker_ == nullptr) {
        MEDIA_WARN_LOG("WaitForIntegrityCheck: checker is null, skip");
        return true;
    }

    bool result = integrityChecker_->WaitForResult();
    if (result) {
        integrityChecker_.reset();
        reverseRestoreReportInfo_.quickCheckResult = "CHECK PASS";
        MEDIA_INFO_LOG("WaitForIntegrityCheck: integrity check passed");
    } else {
        reverseRestoreReportInfo_.quickCheckResult = "CHECK FAILED";
        MEDIA_ERR_LOG("WaitForIntegrityCheck: integrity check failed");
    }

    return result;
}

bool ReverseCloneRestore::CleanOldDbData(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = tableDataAdapter_.ClearRedundantData(oldDbTempStore, mediaLibraryRdb_);
    std::string failedTable = tableDataAdapter_.GetFailedTable();

    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneRestore: ClearRedundantData failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo =
            "ClearRedundantData failed, table: " + failedTable;
        return false;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" CleanRedundantData: ")
        .append(std::to_string(endTime - startTime) + ";");

    startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (tableDataAdapter_.CleanInvalidPhotosFromOldDb(oldDbTempStore) != E_OK) {
        MEDIA_ERR_LOG("ReverseCloneRestore: CleanInvalidPhotosFromOldDb failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "CleanInvalidPhotosFromOldDb failed";
        return false;
    }
    endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" CleanInvalidData: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

bool ReverseCloneRestore::CleanCloudDataIfNeeded(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore)
{
    if (IsCloudRestoreSatisfied()) {
        return true;
    }

    MEDIA_INFO_LOG("ReverseCloneRestore: Account invalid, starting cloud data cleanup");
    CloudDataCleaner cleaner(oldDbTempStore);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (!cleaner.CleanCloudData()) {
        MEDIA_ERR_LOG("ReverseCloneRestore: CloudDataCleaner failed");
        return false;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" CleanClouddata: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

bool ReverseCloneRestore::SwitchReverseDbAndAssets(const string &backupRestorePath, const string &upgradePath)
{
    if (!BackupAndRenameNewDb()) {
        MEDIA_ERR_LOG("ReverseCloneRestore: BackupAndRenameNewDb failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "BackupAndRenameNewDb failed";
        FallbackToForwardRestore(backupRestorePath, upgradePath, false);   // 非早期失败, 已经修改了主库
        return false;
    }

    if (!MoveAssets(backupRestorePath)) {
        MEDIA_ERR_LOG("ReverseCloneRestore: SwapDirectories failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "SwapDirectories failed";
        FallbackToForwardRestore(backupRestorePath, upgradePath, false);   // 主库与文件均已动
        return false;
    }

    if (!FinalizeDatabaseSwap(backupRestorePath)) {
        MEDIA_ERR_LOG("ReverseCloneRestore: FinalizeDatabaseSwap failed");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "FinalizeDatabaseSwap failed";
        FallbackToForwardRestore(backupRestorePath, upgradePath, false);   // 完整回退后回到正向克隆
        return false;
    }

    if (!PerformSecondaryMigration()) {
        MEDIA_ERR_LOG("PerformSecondaryMigration failed, abort restore");
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "PerformSecondaryMigration failed";
        FallbackToForwardRestore(backupRestorePath, upgradePath, false);   // 完整回退后回到正向克隆
        return false;
    }

    int32_t ancoRet = RepairFinalAncoAssetsAfterSecondaryMigration();
    if (ancoRet != E_OK) {
        reverseRestoreReportInfo_.reverseChangeErrorInfo = "RepairFinalAncoAssetsAfterSecondaryMigration failed";
        FallbackToForwardRestore(backupRestorePath, upgradePath, false);   // 完整回退后回到正向克隆
        return false;
    }
    return true;
}

int32_t ReverseCloneRestore::RepairFinalAncoAssetsAfterSecondaryMigration()
{
    int32_t initReverseRet = MediaLibraryDataManager::GetInstance()->InitReverseMediaLibraryRdbStore();
    CHECK_AND_RETURN_RET_LOG(initReverseRet == E_OK, initReverseRet,
        "ReverseCloneRestore: InitReverseMediaLibraryRdbStore before anco repair failed, ret=%{public}d",
        initReverseRet);

    AncoReverseCloneContext ancoContext;
    ancoContext.dstConfig = dstDevFileTransferConfig_;

    AncoReverseCloneAdapter ancoAdapter;
    int32_t ancoRet = ancoAdapter.RepairFinalDb(destRdb_, ancoContext);
    CHECK_AND_RETURN_RET_LOG(ancoRet == E_OK, ancoRet,
        "ReverseCloneRestore: Repair final anco assets failed, ret=%{public}d", ancoRet);
    MEDIA_INFO_LOG("ReverseCloneRestore: Repair final anco assets success");
    return E_OK;
}

void ReverseCloneRestore::ProcessPostSecondarySpecialTables()
{
    MEDIA_INFO_LOG("Processing tab_asset_and_album_operation after secondary migration");
    int32_t ret = tableDataAdapter_.ProcessSingleTableByRecreate("tab_asset_and_album_operation",
                                                                   destRdb_, sourceRdb_);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to process tab_asset_and_album_operation after secondary migration");
    }
}

bool ReverseCloneRestore::PostProcessFinalReverseDb(vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();

    ProcessPostSecondarySpecialTables();

    MEDIA_INFO_LOG("Updating special fields in Photos table after db conversion");
    resourceInheritHelper_.SnapshotPureCloudFileIds(destRdb_);
    UpdatePhotosSpecialFields();
    UpdateChangeTime();

    CheckTableColumnStatus(destRdb_, CLONE_TABLE_LISTS_PHOTO);
    retainedOldPhotoKvStoreTasks = resourceInheritHelper_.BuildRetainedOldPhotoKvStoreTasks(destRdb_);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" Absorb Anticipation: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

void ReverseCloneRestore::AbsorbNewDeviceData(const string &backupRestorePath,
    const vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks)
{
    mediaRdb_ = sourceRdb_;
    mediaLibraryRdb_ = destRdb_;
    DatabaseReport()
        .SetSceneCode(this->sceneCode_)
        .SetTaskId(this->taskId_)
        .ReportMedia(this->mediaLibraryRdb_, DatabaseReport::PERIOD_OLD)
        .ReportMedia(this->mediaRdb_, DatabaseReport::PERIOD_BEFORE);
    UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_)
        .Report("RESTORE_CLOUD_STATUS", "",
            "isAccountValid_: " + std::to_string(isAccountValid_) +
            "isSrcDstSwitchStatusMatch_: " + std::to_string(isSrcDstSwitchStatusMatch_));
    AbsorbNewAlbums();
    AbsorbNewPhotos();
    // 吸收云图数据 - 通过查询sourceRdb的configinfo判断是否需要吸收
    if (ShouldAbsorbCloudFromSourceRdb()) {
        AbsorbNewPhotosForCloud();
    }
    mediaRdb_ = nullptr;
    mediaLibraryRdb_ = nullptr;

    reverseDupMap_.clear();
    for (const auto &entry : duplicateAssetMap_) {
        reverseDupMap_[entry.second] = entry.first;
    }
    MEDIA_INFO_LOG("StartRestore: reverseDupMap_ size=%{public}zu (includes local+cloud photos)",
        reverseDupMap_.size());
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    albumAssetAbsorb_.UpdateTabClonedOldPhotos(destRdb_, reverseDupMap_);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" UpdateTabClonedOldPhotos: ")
        .append(std::to_string(endTime - startTime) + ";");
    startTime = MediaFileUtils::UTCTimeMilliSeconds();
    resourceInheritHelper_.AppendKvStoreTasks(retainedOldPhotoKvStoreTasks);
    resourceInheritHelper_.ExecutePendingKvStoreTasks(backupRestorePath);
    endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" AppendKvStore: ")
        .append(std::to_string(endTime - startTime) + ";");
    ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::ANALYSIS_RESTORE);
    ReverseRestoreAnalysisData();
}

void ReverseCloneRestore::CalculateMigrateNumbers(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore)
{
    int32_t migratedCount = 0;
    int32_t migratedHoLakeVideoCount = 0;
    int32_t migratedHoLakeImageCount = 0;
    int32_t migratedNonHoLakeVideoCount = 0;
    if (oldDbTempStore != nullptr) {
        std::string countSql = std::string("SELECT COUNT(1) AS count, ") +
            "SUM(CASE WHEN file_source_type = " + std::to_string(FileSourceType::MEDIA_HO_LAKE) +
            " AND media_type = " + std::to_string(MediaType::MEDIA_TYPE_VIDEO) +
            " THEN 1 ELSE 0 END) AS ho_lake_video_count, " +
            "SUM(CASE WHEN file_source_type = " + std::to_string(FileSourceType::MEDIA_HO_LAKE) +
            " AND media_type = " + std::to_string(MediaType::MEDIA_TYPE_IMAGE) +
            " THEN 1 ELSE 0 END) AS ho_lake_image_count, " +
            "SUM(CASE WHEN media_type = " + std::to_string(MediaType::MEDIA_TYPE_VIDEO) +
            " AND file_source_type != " + std::to_string(FileSourceType::MEDIA_HO_LAKE) +
            " THEN 1 ELSE 0 END) AS non_ho_lake_video_count FROM Photos" +
            " WHERE sync_status = 0 AND clean_flag = 0 AND time_pending = 0 AND is_temp = 0";
        auto resultSet = BackupDatabaseUtils::QuerySql(oldDbTempStore, countSql, {});
        if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
            migratedCount = GetInt32Val("count", resultSet);
            migratedHoLakeVideoCount = GetInt32Val("ho_lake_video_count", resultSet);
            migratedHoLakeImageCount = GetInt32Val("ho_lake_image_count", resultSet);
            migratedNonHoLakeVideoCount = GetInt32Val("non_ho_lake_video_count", resultSet);
            MEDIA_INFO_LOG("migrated count: total=%{public}d, ho_lake_video=%{public}d, "
                "ho_lake_image=%{public}d, non_ho_lake_video=%{public}d",
                migratedCount, migratedHoLakeVideoCount, migratedHoLakeImageCount, migratedNonHoLakeVideoCount);
            resultSet->Close();
        }
    }
    reverseRestoreReportInfo_.failedCount -= migratedCount;
    reverseRestoreReportInfo_.migrateCount = migratedCount;
    reverseRestoreReportInfo_.migrateLakeImageCount = migratedHoLakeImageCount;
    reverseRestoreReportInfo_.migrateLakeVideoCount = migratedHoLakeVideoCount;
    reverseRestoreReportInfo_.migratedNonHoLakeVideoCount = migratedNonHoLakeVideoCount;
    reverseRestoreReportInfo_.successCount = migratedCount;
}

int32_t ReverseCloneRestore::QueryActiveDeleteDataFromSourceRdb(int32_t lastFileId,
    CloudMediaAssetDeleteData &data, int32_t &nextFileId)
{
    data.Clear();
    data.Reserve(CLOUD_MEDIA_DELETE_BATCH_LIMIT);
    nextFileId = lastFileId;
    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, E_ERR,
        "QueryActiveDeleteDataFromSourceRdb failed, sourceRdb_ is null");

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastFileId);
    predicates.And()->BeginWrap()
        ->EqualTo(MediaColumn::MEDIA_NAME, ACTIVE_DELETE_DISPLAY_NAME)
        ->Or()
        ->BeginWrap()
            ->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN))
            ->And()
            ->EqualTo(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID)
            ->And()
            ->EqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)))
        ->EndWrap()
    ->EndWrap();
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(CLOUD_MEDIA_DELETE_BATCH_LIMIT);

    std::vector<std::string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_DATE_TAKEN, PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE};
    auto resultSet = sourceRdb_->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR,
        "QueryActiveDeleteDataFromSourceRdb failed, resultSet is null");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        nextFileId = std::max(nextFileId, fileId);

        CloudMediaAssetDeleteItem item;
        item.path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        if (item.path.empty()) {
            MEDIA_WARN_LOG("QueryActiveDeleteDataFromSourceRdb skip empty path, fileId=%{public}d", fileId);
            continue;
        }
        item.fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        item.dateTaken = GetStringVal(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        item.lcdVisitTime = GetInt64Val(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, resultSet);
        item.subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        item.southDevice = GetInt32Val(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, resultSet);
        item.effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        data.Add(item);
    }
    resultSet->Close();
    return E_OK;
}

int32_t ReverseCloneRestore::ApplyActiveDeleteDbActionToSourceRdb(const CloudMediaAssetDeleteDbAction &action)
{
    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, E_ERR,
        "ApplyActiveDeleteDbActionToSourceRdb failed, sourceRdb_ is null");
    if (action.deleteFileIds.empty() && action.updateFileIds.empty()) {
        return E_OK;
    }

    auto [errCode, transaction] = sourceRdb_->CreateTransaction(OHOS::NativeRdb::Transaction::DEFERRED);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && transaction != nullptr, E_ERR,
        "ApplyActiveDeleteDbActionToSourceRdb create transaction failed, err=%{public}d", errCode);
    if (!action.deleteFileIds.empty()) {
        NativeRdb::AbsRdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
        deletePredicates.In(MediaColumn::MEDIA_ID, action.deleteFileIds);
        auto res = transaction->Delete(deletePredicates);
        if (res.first != NativeRdb::E_OK || res.second <= 0) {
            MEDIA_ERR_LOG("ApplyActiveDeleteDbActionToSourceRdb delete failed, ret=%{public}d, rows=%{public}d",
                res.first, static_cast<int32_t>(res.second));
            transaction->Rollback();
            return E_ERR;
        }
    }

    if (!action.updateFileIds.empty()) {
        NativeRdb::AbsRdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
        updatePredicates.In(MediaColumn::MEDIA_ID, action.updateFileIds);
        NativeRdb::ValuesBucket values;
        values.PutLong(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_DELETED);
        auto res = transaction->Update(values, updatePredicates);
        if (res.first != NativeRdb::E_OK || res.second <= 0) {
            MEDIA_ERR_LOG("ApplyActiveDeleteDbActionToSourceRdb update failed, ret=%{public}d, rows=%{public}d",
                res.first, static_cast<int32_t>(res.second));
            transaction->Rollback();
            return E_ERR;
        }
    }

    int32_t ret = transaction->Commit();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ApplyActiveDeleteDbActionToSourceRdb commit failed, ret=%{public}d", ret);
        transaction->Rollback();
        return E_ERR;
    }
    return E_OK;
}

int32_t ReverseCloneRestore::CompensateActiveDeleteCloudMediaAssetsLocked()
{
    MEDIA_INFO_LOG("CompensateActiveDeleteCloudMediaAssetsLocked start");
    int32_t lastFileId = 0;
    bool completed = false;
    while (!completed) {
        CloudMediaAssetDeleteData data;
        int32_t nextFileId = lastFileId;
        int32_t ret = QueryActiveDeleteDataFromSourceRdb(lastFileId, data, nextFileId);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "CompensateActiveDeleteCloudMediaAssetsLocked query failed, ret=%{public}d", ret);
        if (data.Empty()) {
            if (nextFileId > lastFileId) {
                lastFileId = nextFileId;
                continue;
            }
            completed = true;
            continue;
        }
        CHECK_AND_RETURN_RET_LOG(nextFileId > lastFileId, E_ERR,
            "CompensateActiveDeleteCloudMediaAssetsLocked no progress, lastFileId=%{public}d", lastFileId);

        CloudMediaAssetDeleteDbAction action;
        CloudMediaAssetManager::BuildDbActionAndCleanFiles(data, action);
        ret = ApplyActiveDeleteDbActionToSourceRdb(action);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
            "CompensateActiveDeleteCloudMediaAssetsLocked apply db action failed, ret=%{public}d", ret);
        CloudMediaAssetManager::FinishDeleteBatch(data);
        MEDIA_INFO_LOG("CompensateActiveDeleteCloudMediaAssetsLocked processed batch, "
            "count=%{public}zu, lastFileId=%{public}d", data.Size(), nextFileId);
        lastFileId = nextFileId;
    }
    MEDIA_INFO_LOG("CompensateActiveDeleteCloudMediaAssetsLocked completed");
    return E_OK;
}

void ReverseCloneRestore::FinishReverseRestore()
{
    StopReverseRestoreStatusUpdateThread();
    // 13 tab_old_album 基础相册处理
    DealDuplicatePhotoAlbum();
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    UpdateDatabase();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" UpdateDatabase: ")
        .append(std::to_string(endTime - startTime) + ";");
    startTime = MediaFileUtils::UTCTimeMilliSeconds();
    HandleRestData();
    FileIdMigrator::UpdateSqliteSequenceForPhotos(destRdb_);
    SetMediaAnalysisClearDirtyDataParameter();
    CloneActiveLcdAgingFromOldDevice();
    CloseAllKvStore();
    SetNeedCreateDentryFlag(1);
    {
        auto deleteLock = CloudMediaAssetManager::AcquireDeleteCloudMediaAssetsLock();
        CHECK_AND_PRINT_LOG(CompensateActiveDeleteCloudMediaAssetsLocked() == E_OK,
            "ReverseCloneRestore: compensate cloud media delete failed");
    }
    CleanReverseRestoreTempFiles();
    LcdAgingService::GetInstance().MarkRecentLcdPhotos(destRdb_);
    endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" PostProcess: ")
        .append(std::to_string(endTime - startTime) + ";");
    ActiveFullDonation();
    StopParameterForRestore();
    StopParameterForClone();
    CHECK_AND_PRINT_LOG(ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::COMPLETED),
        "set reverse clone reliability marker completed failed");
    CHECK_AND_PRINT_LOG(ReverseCloneReliabilityMarker::Delete(), "delete reverse clone reliability marker failed");
    MEDIA_INFO_LOG("ReverseCloneRestore: End clone restore (reverse path)");
}

void ReverseCloneRestore::SetCloneParameterAndStopSyncForResume()
{
    SetCloneParameterAndStopSync();
}

bool ReverseCloneRestore::PrepareReverseDbBeforeAbsorb(const std::string &backupRestorePath,
    const std::string &upgradePath, std::vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks)
{
    std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore;
    if (!PrepareReverseOldDb(backupRestorePath, upgradePath, oldDbTempStore)) {
        ReverseCloneReliabilityMarker::Delete();
        return false;
    }
    CalculateMigrateNumbers(oldDbTempStore);
    oldDbTempStore.reset();
    oldDbTempStore = nullptr;

    // 更新标记位为数据库已切换
    ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::DB_SWITCHED);

    if (!SwitchReverseDbAndAssets(backupRestorePath, upgradePath)) {
        ReverseCloneReliabilityMarker::Delete();
        return false;
    }

    if (!PostProcessFinalReverseDb(retainedOldPhotoKvStoreTasks)) {
        ReverseCloneReliabilityMarker::Delete();
        return false;
    }

    // 在吸收新机数据之前检查 sourceRdb 完整性
    if (!CheckSourceRdbIntegrityAndFallback(backupRestorePath, upgradePath)) {
        MEDIA_ERR_LOG("StartRestore: CheckSourceRdbIntegrityAndFallback failed");
        return false;
    }
    return true;
}
void ReverseCloneRestore::SetMigrateNumbers()
{
    migrateLakePhotoDuplicateNumber_ = reverseRestoreReportInfo_.duplicateLakeImageCount;
    migrateLakeVideoDuplicateNumber_ = reverseRestoreReportInfo_.duplicateLakeVideoCount;
    migrateLakePhotoNumber_ = reverseRestoreReportInfo_.migrateLakeImageCount - migrateLakePhotoDuplicateNumber_;
    migrateLakeVideoNumber_ = reverseRestoreReportInfo_.migrateLakeVideoCount - migrateLakeVideoDuplicateNumber_;
    migratePhotoDuplicateNumber_ = reverseRestoreReportInfo_.duplicateImageCount;
    migrateVideoDuplicateNumber_ = reverseRestoreReportInfo_.duplicateVideoCount;
    migrateFileNumber_ = reverseRestoreReportInfo_.migrateCount - reverseRestoreReportInfo_.duplicateCount -
        migrateLakePhotoNumber_ - migrateLakeVideoNumber_;
    migrateVideoFileNumber_ = reverseRestoreReportInfo_.migratedNonHoLakeVideoCount - migrateVideoDuplicateNumber_;

    MEDIA_INFO_LOG("SetMigrateNumbers: migrateLakePhotoDuplicateNumber_=%{public}lld, "
                   "migrateLakeVideoDuplicateNumber_=%{public}lld, "
                   "migrateLakePhotoNumber_=%{public}lld, "
                   "migrateLakeVideoNumber_=%{public}lld, "
                   "migratePhotoDuplicateNumber_=%{public}lld, "
                   "migrateVideoDuplicateNumber_=%{public}lld, "
                   "migrateFileNumber_=%{public}lld, "
                   "migrateVideoFileNumber_=%{public}lld, "
                   "totalNumber_=%{public}lld",
                   (long long)migrateLakePhotoDuplicateNumber_,
                   (long long)migrateLakeVideoDuplicateNumber_,
                   (long long)migrateLakePhotoNumber_,
                   (long long)migrateLakeVideoNumber_,
                   (long long)migratePhotoDuplicateNumber_,
                   (long long)migrateVideoDuplicateNumber_,
                   (long long)migrateFileNumber_,
                   (long long)migrateVideoFileNumber_,
                   (long long)totalNumber_);
}

void ReverseCloneRestore::StartRestore(const string &backupRestorePath, const string &upgradePath)
{
    int64_t startTime = MediaFileUtils::UTCTimeSeconds();
    MEDIA_INFO_LOG("ReverseCloneRestore: Start clone restore(reverse)");
    preprocessErrorCode_ = CloneRestore::PreprocessBeforeClone(backupRestorePath, upgradePath);
#ifndef REVERSE_RESTORE_SUPPORT
    MEDIA_INFO_LOG(
        "ReverseCloneRestore: not supported, back to normal CloneRestore");
    CloneRestore::StartRestore(backupRestorePath, upgradePath);
    return;
#endif

    ResetReverseRestoreState();
    if (!PreprocessReverseRestore(backupRestorePath, upgradePath)) {
        return;
    }

    // 创建标记位
    if (!ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::EARLY_STAGE)) {
        MEDIA_ERR_LOG("StartRestore: set reverse clone reliability marker early stage failed");
    }

    // 跨进程等待：云删除完成或等待超时。
    CloudMediaAssetManager::WaitDeleteCloudMediaAssetsIdleOrTimeout();

    // 同进程栅栏：等待已经运行的云删除释放 deleteMutex_。
    CloudMediaAssetManager::WaitDeleteCloudMediaAssetsOperationFinish();

    vector<ReverseCloneKvStoreTask> retainedOldPhotoKvStoreTasks;
    if (!PrepareReverseDbBeforeAbsorb(backupRestorePath, upgradePath, retainedOldPhotoKvStoreTasks)) {
        return;
    }

    // 更新标记位为正在吸收数据
    ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::ABSORBING_DATA);

    reverseRestoreReportInfo_.restoreDirection = "REVERSE";
    AbsorbNewDeviceData(backupRestorePath, retainedOldPhotoKvStoreTasks);

    // 更新标记位为智慧数据恢复
    ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::FINISHING);
    SetMigrateNumbers();
    FinishReverseRestore();
    int64_t endTime = MediaFileUtils::UTCTimeSeconds();
    reverseRestoreReportInfo_.dataReplaceResult = 1;
    reverseRestoreReportInfo_.perfectRestoreTime = endTime - startTime;
}

bool ReverseCloneRestore::PerformInitialMigration(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    auto context = AbilityRuntime::Context::GetApplicationContext();
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "PerformInitialMigration: get context failed");

    // 打开当前新设备数据库，用于查询最大 ID
    const std::string newDbPath = "/data/storage/el2/database/rdb/media_library.db";
    std::shared_ptr<NativeRdb::RdbStore> newDbStore;
    int32_t err = BackupDatabaseUtils::InitDb(newDbStore, CONST_MEDIA_DATA_ABILITY_DB_NAME,
                                              newDbPath, CONST_BUNDLE_NAME, true, context->GetArea(), false);
    if (newDbStore == nullptr) {
        MEDIA_ERR_LOG("PerformInitialMigration: open new db failed, err=%{public}d", err);
        return false;
    }

    // 执行第一次完整迁移（file_id + album_id）
    int64_t oldMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(oldDbTempStore);
    int64_t newMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(newDbStore);
    if (oldMaxFileId > 0 && newMaxFileId > 0) {
        resourceInheritHelper_.AddFileIdOffsetRule(oldMaxFileId, newMaxFileId);
    }
    FileIdMigrator migrator;
    if (!migrator.Migrate(oldDbTempStore, newDbStore)) {
        MEDIA_ERR_LOG("PerformInitialMigration: initial migration failed");
        newDbStore.reset();
        return false;
    }

    // 记录第一次迁移后新 db 的最大 ID，供后续二次迁移比较
    initialNewMaxFileId_ = FileIdMigrator::GetMaxFileIdFromAllTables(newDbStore);
    initialNewMaxAlbumId_ = FileIdMigrator::GetMaxAlbumIdFromAllTables(newDbStore);
    newMaxExtended_ = migrator.GetNewMaxExtended();

    MEDIA_INFO_LOG("PerformInitialMigration: initialNewMaxFileId=%{public}lld, "
                   "initialNewMaxAlbumId=%{public}lld, newMaxExtended=%{public}lld",
                   static_cast<long long>(initialNewMaxFileId_),
                   static_cast<long long>(initialNewMaxAlbumId_),
                   static_cast<long long>(newMaxExtended_));

    newDbStore.reset();
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" FirstMigration: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

bool ReverseCloneRestore::PerformSecondaryMigration()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (sourceRdb_ == nullptr || destRdb_ == nullptr) {
        MEDIA_ERR_LOG("PerformSecondaryMigration: sourceRdb_ or destRdb_ is null");
        return false;
    }

    // 检查转正后备份的新 db (sourceRdb_) 是否产生了更大的 ID
    int64_t currentNewMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(sourceRdb_);
    int64_t currentNewMaxAlbumId = FileIdMigrator::GetMaxAlbumIdFromAllTables(sourceRdb_);

    bool needReMigrate = false;
    if (currentNewMaxFileId > initialNewMaxFileId_) {
        MEDIA_INFO_LOG("PerformSecondaryMigration: new max file_id increased from %{public}lld to %{public}lld",
                       static_cast<long long>(initialNewMaxFileId_),
                       static_cast<long long>(currentNewMaxFileId));
        needReMigrate = true;
    }
    if (currentNewMaxAlbumId > initialNewMaxAlbumId_) {
        MEDIA_INFO_LOG("PerformSecondaryMigration: new max album_id increased from %{public}lld to %{public}lld",
                       static_cast<long long>(initialNewMaxAlbumId_),
                       static_cast<long long>(currentNewMaxAlbumId));
        needReMigrate = true;
    }

    if (!needReMigrate) {
        MEDIA_INFO_LOG("PerformSecondaryMigration: no change in max IDs, skip");
        return true;
    }

    // 有新增数据，再次执行完整迁移 (此时 oldDb = 已转正的旧 db, newDb = 备份的新 db)
    int64_t oldMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(destRdb_);
    int64_t newMaxFileId = FileIdMigrator::GetMaxFileIdFromAllTables(sourceRdb_);
    if (oldMaxFileId > 0 && newMaxFileId > 0) {
        resourceInheritHelper_.AddFileIdOffsetRule(oldMaxFileId, newMaxFileId);
    }
    FileIdMigrator migrator;
    if (!migrator.Migrate(destRdb_, sourceRdb_)) {
        MEDIA_ERR_LOG("PerformSecondaryMigration: second migration failed");
        return false;
    }

    newMaxExtended_ = migrator.GetNewMaxExtended();
    MEDIA_INFO_LOG("PerformSecondaryMigration: second migration success, newMaxExtended=%{public}lld",
                   static_cast<long long>(newMaxExtended_));
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.beforeTransformTimeCost.append(" SecondMigration: ")
        .append(std::to_string(endTime - startTime) + ";");
    return true;
}

int32_t ReverseCloneRestore::ClearRedundantData()
{
    MEDIA_INFO_LOG("ReverseCloneRestore: ClearRedundantData called");
    return tableDataAdapter_.ClearRedundantData(destRdb_, sourceRdb_);
}

void ReverseCloneRestore::UpdatePhotosSpecialFields()
{
    MEDIA_INFO_LOG("ReverseCloneRestore: UpdatePhotosSpecialFields called");
    CHECK_AND_RETURN_LOG(destRdb_ != nullptr, "destRdb_ is null");

    // 1. 更新 PHOTO_QUALITY：判断是否为 1，是就不变，否则改为 0
    std::string updatePhotoQualitySql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_QUALITY +
                                        " = CASE WHEN " + PhotoColumn::PHOTO_QUALITY + " = 1 THEN 0 ELSE " +
                                        PhotoColumn::PHOTO_QUALITY + " END";
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updatePhotoQualitySql, {});

    // 2. 更新 STAGE_VIDEO_TASK_STATUS
    std::string updateStageVideoTaskStatusSql =
        "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::STAGE_VIDEO_TASK_STATUS + " = " +
        std::to_string(static_cast<int32_t>(StageVideoTaskStatus::NO_NEED_TO_STAGE));
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updateStageVideoTaskStatusSql, {});

    // 3. 更新 PHOTO_METADATA_FLAGS：统一设置为默认值 0
    std::string updateMetadataFlagsSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
                                        PhotoColumn::PHOTO_METADATA_FLAGS + " = 0";
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updateMetadataFlagsSql, {});

    // 4. 更新 PHOTO_CE_AVAILABLE：统一设置为默认值 0
    std::string updateCeAvailableSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
                                      PhotoColumn::PHOTO_CE_AVAILABLE + " = 0";
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updateCeAvailableSql, {});

    // 5. 更新所有 Photos 表中的 change_time 为当前时间
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::string updateChangeTimeSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
                                      " SET " + PhotoColumn::PHOTO_CHANGE_TIME + " = " +
                                      std::to_string(currentTime);
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updateChangeTimeSql);

    MEDIA_INFO_LOG("UpdatePhotosSpecialFields: completed");
}

void ReverseCloneRestore::UpdateChangeTime()
{
    MEDIA_INFO_LOG("ReverseCloneRestore: UpdateChangeTime called");
    CHECK_AND_RETURN_LOG(destRdb_ != nullptr, "destRdb_ is null");

    // 更新所有 PhotoAlbum 表中的 change_time 为当前时间
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::string updateChangeTimeSql = "UPDATE " + PhotoAlbumColumns::TABLE +
                                      " SET " + PhotoAlbumColumns::CHANGE_TIME + " = " +
                                      std::to_string(currentTime) +
                                      " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " <> 1024";
    BackupDatabaseUtils::ExecuteSQL(destRdb_, updateChangeTimeSql);

    MEDIA_INFO_LOG("UpdateChangeTime: completed");
}

void ReverseCloneRestore::SetAggregateBitThird()
{
    CHECK_AND_RETURN_LOG(sourceRdb_ != nullptr, "SetAggregateBitThird failed, sourceRdb_ is nullptr");
    DataTransfer::MediaLibraryDbUpgrade medialibraryDbUpgrade;
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumType::SMART)));
    params.push_back(NativeRdb::ValueObject(std::to_string(PhotoAlbumSubType::CLASSIFY)));
    auto resultSet = sourceRdb_->QuerySql(SQL_QUERY_CLASSIFY_ALBUM_EXIST_REVERSE, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK && GetInt32Val("count", resultSet) > 0) {
        resultSet->Close();
        MEDIA_INFO_LOG("classify album already exist, no need to SetAggregateBitThird");
        return;
    }
    resultSet->Close();
    int32_t bitPosition = 2;
    medialibraryDbUpgrade.SetAggregateBit(bitPosition);
}

void ReverseCloneRestore::AbsorbNewAlbums()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ReverseCloneRestore: AbsorbNewAlbums called");
    SetAggregateBitThird();

    // 初始化反向克隆的相册ID映射
    InitializeTableAlbumIdMapForReverse();

    // 相册吸收
    for (const auto &tableName : CLONE_ALBUMS) {
        MEDIA_INFO_LOG("AbsorbNewAlbums: Processing table %{public}s", tableName.c_str());
        if (!IsReadyForRestore(tableName)) {
            MEDIA_ERR_LOG("Column status of %{public}s is not ready for absorb album, quit",
                          BackupDatabaseUtils::GarbleInfoName(tableName).c_str());
            continue;
        }
        MEDIA_INFO_LOG("AbsorbNewAlbums: Table %{public}s is ready for restore", tableName.c_str());

        unordered_map<string, string> srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(sourceRdb_, tableName);
        MEDIA_INFO_LOG("AbsorbNewAlbums: Got %{public}zu columns from sourceRdb for %{public}s",
                       srcColumnInfoMap.size(), tableName.c_str());
        unordered_map<string, string> dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(destRdb_, tableName);
        MEDIA_INFO_LOG("AbsorbNewAlbums: Got %{public}zu columns from destRdb for %{public}s", dstColumnInfoMap.size(),
                       tableName.c_str());

        if (!PrepareCommonColumnInfoMapForAbsorb(tableName, srcColumnInfoMap, dstColumnInfoMap)) {
            MEDIA_ERR_LOG("Prepare common column info failed");
            continue;
        }
        GetAlbumExtraQueryWhereClause(tableName);

        // 1. 更新系统相册（type=1024）的 album_id
        UpdateSystemAlbumFields();

        // 2. 处理 SOURCE (type=2048) 和 USER (type=0) 相册的判重插入
        InsertSourceAndUserAlbumsWithDuplicateCheck();

        MEDIA_INFO_LOG("AbsorbNewAlbums: Finished processing table %{public}s", tableName.c_str());
    }

    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" AbsorbAlbum: ")
        .append(std::to_string(endTime - startTime) + ";");
    MEDIA_INFO_LOG("AbsorbNewAlbums: All steps completed, total albums migrated=%{public}ld",
                   (long)migrateDatabaseAlbumNumber_);
}

void ReverseCloneRestore::AbsorbNewPhotos()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ReverseCloneRestore: AbsorbNewPhotos called");
    CHECK_AND_RETURN_LOG(
        IsReadyForRestore(PhotoColumn::PHOTOS_TABLE), "Column status is not ready for absorb photo, quit");
    unordered_map<string, string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(sourceRdb_, PhotoColumn::PHOTOS_TABLE);
    unordered_map<string, string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(destRdb_, PhotoColumn::PHOTOS_TABLE);
    if (!PrepareCommonColumnInfoMapForAbsorb(PhotoColumn::PHOTOS_TABLE, srcColumnInfoMap, dstColumnInfoMap)) {
        MEDIA_ERR_LOG("Prepare common column info failed");
        return;
    }

    int32_t maxSourceDbFileId = 0;
    int32_t maxDestDbFileId = 0;
    if (!PrepareAbsorbPhotosCommonInfo(maxSourceDbFileId, maxDestDbFileId)) {
        return;
    }

    InitializeDuplicateAssetMapForPhotos();

    int totalNumber = GetAllPhotosRowCount();
    MEDIA_INFO_LOG("AbsorbNewPhotos: totalNumber = %{public}d", totalNumber);
    PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_NOT_READY);
    SubmitAbsorbNewPhotosTasks(totalNumber, maxSourceDbFileId, maxDestDbFileId, false);

    ProcessNewPhotosFailedOffsets(0, maxSourceDbFileId, maxDestDbFileId);

    if (RestoreMapCodeUtils::GetNotReadyPhotoCount(destRdb_) == 0) {
        PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_READY);
    }
    MEDIA_INFO_LOG("ReverseCloneRestore: AbsorbNewPhotos completed");
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" AbsorbAssets: ")
        .append(std::to_string(endTime - startTime) + ";");
}

bool ReverseCloneRestore::ShouldAbsorbCloudFromSourceRdb()
{
    // 查询新机云开关
    SwitchStatus currentSwitchStatus = SettingsDataManager::GetPhotosSyncSwitchStatus();
    bool shouldAbsorbCloud = (currentSwitchStatus == SwitchStatus::CLOUD ||
                              currentSwitchStatus == SwitchStatus::HDC);
    MEDIA_INFO_LOG(
        "ReverseCloneRestore: current device photo_sync_status=%{public}d, shouldAbsorbCloud=%{public}d",
        static_cast<int>(currentSwitchStatus), shouldAbsorbCloud);
    return shouldAbsorbCloud;
}

void ReverseCloneRestore::AbsorbNewPhotosForCloud()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("ReverseCloneRestore: AbsorbNewPhotosForCloud called");

    CHECK_AND_RETURN_LOG(
        IsReadyForRestore(PhotoColumn::PHOTOS_TABLE), "Column status is not ready for absorb cloud photos, quit");
    unordered_map<string, string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(sourceRdb_, PhotoColumn::PHOTOS_TABLE);
    unordered_map<string, string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(destRdb_, PhotoColumn::PHOTOS_TABLE);
    CHECK_AND_RETURN_LOG(
        PrepareCommonColumnInfoMapForAbsorb(PhotoColumn::PHOTOS_TABLE, srcColumnInfoMap, dstColumnInfoMap),
        "Prepare common column info failed");

    int32_t maxSourceDbFileId = 0;
    int32_t maxDestDbFileId = 0;
    if (!PrepareAbsorbPhotosCommonInfo(maxSourceDbFileId, maxDestDbFileId)) {
        return;
    }

    int totalNumber = GetAllCloudPhotosRowCount();
    MEDIA_INFO_LOG("AbsorbNewPhotosForCloud: totalNumber = %{public}d", totalNumber);
    MEDIA_INFO_LOG("AbsorbNewPhotosForCloud: Update totalNumber_: %{public}lld", (long long)totalNumber_);
    PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_NOT_READY);
    SubmitAbsorbNewPhotosForCloudTasks(totalNumber, maxSourceDbFileId, maxDestDbFileId);

    ProcessNewPhotosForCloudFailedOffsets(0, maxSourceDbFileId, maxDestDbFileId);

    if (RestoreMapCodeUtils::GetNotReadyPhotoCount(destRdb_) == 0) {
        PhotoMapCodeOperation::SetMapCodeReadyStatus(MAP_CODE_IS_READY);
    }
    MEDIA_INFO_LOG("ReverseCloneRestore: AbsorbNewPhotosForCloud completed");
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    reverseRestoreReportInfo_.afterTransformTimeCost.append(" AbsorbCloud: ")
        .append(std::to_string(endTime - startTime) + ";");
}

bool ReverseCloneRestore::PrepareCommonColumnInfoMapForAbsorb(const string &tableName,
    const unordered_map<string, string> &srcColumnInfoMap, const unordered_map<string, string> &dstColumnInfoMap)
{
    auto &commonColumnInfoMap = tableCommonColumnInfoMap_[tableName];
    commonColumnInfoMap.clear();

    // 遍历目标数据库的所有列
    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        const string &columnName = it->first;
        const string &columnType = it->second;

        // 只检查源数据库是否有相同名称和类型的列，不排除任何字段
        if (!HasSameColumn(srcColumnInfoMap, columnName, columnType)) {
            continue;
        }

        // 将所有共有的字段都加入 commonColumnInfoMap（无脑复制）
        commonColumnInfoMap[columnName] = columnType;
    }

    MEDIA_INFO_LOG("Table %{public}s has %{public}zu common columns (absorb mode)",
        BackupDatabaseUtils::GarbleInfoName(tableName).c_str(),
        commonColumnInfoMap.size());
    return true;
}

vector<NativeRdb::ValuesBucket> ReverseCloneRestore::GetInsertValuesForAbsorb(vector<FileInfo> &fileInfos,
    unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans)
{
    vector<NativeRdb::ValuesBucket> values;
    ReverseCloneResourceInheritService resourceInheritService;
    int32_t sourceCheckFailedCount = 0;
    int32_t invalidLocalCount = 0;
    int32_t lakeMissingSourceKeptCount = 0;
    int32_t pathPrepareFailedCount = 0;
    for (size_t i = 0; i < fileInfos.size(); i++) {
        // 检查文件是否有效（复用）
        std::string sourceFilePath = GetSourceFilePathForReverseAbsorb(fileInfos[i]);
        int32_t errCode = BackupFileUtils::IsFileValid(sourceFilePath, CLONE_RESTORE_ID, "", false);
        sourceCheckFailedCount += errCode == E_OK ? 0 : 1;
        bool keepMissingLakeSource = ShouldKeepMissingReverseSourceForLake(fileInfos[i], errCode);
        if (IsInvalidLocalFile(errCode, fileInfos[i]) && !keepMissingLakeSource) {
            invalidLocalCount++;
        }
        if (keepMissingLakeSource) {
            lakeMissingSourceKeptCount++;
            fileInfos[i].filePath = fileInfos[i].storagePath;
            MEDIA_INFO_LOG("Reverse absorb keep missing media source for lake, fileId=%{public}d, "
                "errCode=%{public}d, fileSourceType=%{public}d, storagePath=%{public}s, sourceFilePath=%{public}s",
                fileInfos[i].fileIdOld, errCode, fileInfos[i].fileSourceType,
                MediaFileUtils::DesensitizePath(fileInfos[i].storagePath).c_str(),
                MediaFileUtils::DesensitizePath(sourceFilePath).c_str());
        } else {
            fileInfos[i].filePath = sourceFilePath;
        }

        // Reverse absorb deletes duplicate donors later; do not run normal duplicate skip here.
        if (!PrepareAbsorbSourceFileInfo(fileInfos[i])) {
            pathPrepareFailedCount++;
            continue;
        }

        // 直接从 valMap 构造 ValuesBucket，不调用 GetInsertValue
        NativeRdb::ValuesBucket value;

        // 设置 photo_id (MEDIA_ID)
        value.PutInt(MediaColumn::MEDIA_ID, fileInfos[i].fileIdOld);

        // 设置其他字段（从 valMap 获取）
        for (auto it = fileInfos[i].valMap.begin(); it != fileInfos[i].valMap.end(); ++it) {
            const string &columnName = it->first;

            const auto &columnValue = it->second;

            // 跳过 MEDIA_ID，因为已经设置过了
            if (columnName == MediaColumn::MEDIA_ID) {
                continue;
            }

            // 根据值类型放入 ValuesBucket
            if (holds_alternative<int32_t>(columnValue)) {
                value.PutInt(columnName, get<int32_t>(columnValue));
            } else if (holds_alternative<int64_t>(columnValue)) {
                value.PutLong(columnName, get<int64_t>(columnValue));
            } else if (holds_alternative<double>(columnValue)) {
                value.PutDouble(columnName, get<double>(columnValue));
            } else if (holds_alternative<string>(columnValue)) {
                value.PutString(columnName, get<string>(columnValue));
            }
        }

        value.PutString(MediaColumn::MEDIA_FILE_PATH, fileInfos[i].cloudPath);
        values.emplace_back(value);
        CHECK_AND_CONTINUE(fileInfos[i].fileIdOld > 0);
        resourcePlans[fileInfos[i].fileIdOld] = resourceInheritService.BuildSourcePlanAfterPrepare(fileInfos[i]);
    }

    MEDIA_INFO_LOG("Reverse absorb prepare insert values: queried=%{public}zu, values=%{public}zu, "
        "resourcePlans=%{public}zu, sourceCheckFailed=%{public}d, invalidLocal=%{public}d, "
        "lakeMissingSourceKept=%{public}d, pathPrepareFailed=%{public}d",
        fileInfos.size(), values.size(), resourcePlans.size(), sourceCheckFailedCount, invalidLocalCount,
        lakeMissingSourceKeptCount, pathPrepareFailedCount);
    return values;
}

bool ReverseCloneRestore::PrepareAbsorbPhotosCommonInfo(int32_t &maxSourceDbFileId, int32_t &maxDestDbFileId)
{
    maxSourceDbFileId = AlbumAssetAbsorb::QueryMaxFileId(sourceRdb_);
    maxDestDbFileId = AlbumAssetAbsorb::QueryMaxFileId(destRdb_);
    MEDIA_INFO_LOG("AbsorbNewPhotos: maxSourceDbFileId=%{public}d, maxDestDbFileId=%{public}d",
        maxSourceDbFileId,
        maxDestDbFileId);
    return true;
}

void ReverseCloneRestore::InitializeDuplicateAssetMapForPhotos()
{
    duplicateAssetMap_.clear();
    minDestDbFileId_ = INT32_MAX;
    std::string queryAllFileIdsSql = "SELECT file_id FROM " + std::string(PhotoColumn::PHOTOS_TABLE);
    auto fileIdResultSet = BackupDatabaseUtils::QuerySql(destRdb_, queryAllFileIdsSql, {});

    if (fileIdResultSet == nullptr) {
        MEDIA_WARN_LOG("InitializeDuplicateAssetMapForPhotos: query failed, resultSet is null");
        minDestDbFileId_ = 0;
        MEDIA_INFO_LOG(
            "AbsorbNewPhotos: initialized duplicateAssetMap_ with %{public}zu entries, minDestDbFileId=%{public}d",
            duplicateAssetMap_.size(), minDestDbFileId_);
        return;
    }

    while (fileIdResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = 0;
        fileIdResultSet->GetInt(0, fileId);
        if (fileId <= 0) {
            continue;
        }
        duplicateAssetMap_[fileId] = fileId;
        if (fileId < minDestDbFileId_) {
            minDestDbFileId_ = fileId;
        }
    }
    fileIdResultSet->Close();

    if (minDestDbFileId_ == INT32_MAX) {
        minDestDbFileId_ = 0;
    }
    MEDIA_INFO_LOG(
        "AbsorbNewPhotos: initialized duplicateAssetMap_ with %{public}zu entries, minDestDbFileId=%{public}d",
        duplicateAssetMap_.size(), minDestDbFileId_);
}

void ReverseCloneRestore::SubmitAbsorbNewPhotosTasks(int32_t totalNumber, int32_t maxSourceDbFileId,
    int32_t maxDestDbFileId, bool isCloud)
{
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_THREAD_NUM);
    needReportFailed_ = false;
    int32_t batchCount = 0;
    for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
        batchCount++;
        MEDIA_INFO_LOG("AbsorbNewPhotos: Submitting batch %{public}d, offset=%{public}d", batchCount, offset);
        ffrt::submit(
            [this, offset, maxSourceDbFileId, maxDestDbFileId]() {
                AbsorbNewPhotosBatch(offset, 0, maxSourceDbFileId, maxDestDbFileId, minDestDbFileId_);
            },
            {&offset},
            {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
}

void ReverseCloneRestore::SubmitAbsorbNewPhotosForCloudTasks(int32_t totalNumber, int32_t maxSourceDbFileId,
    int32_t maxDestDbFileId)
{
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_THREAD_NUM);
    needReportFailed_ = false;
    int32_t batchCount = 0;
    for (int32_t offset = 0; offset < totalNumber; offset += CLONE_QUERY_COUNT) {
        batchCount++;
        MEDIA_INFO_LOG("AbsorbNewPhotosForCloud: Submitting batch %{public}d, offset=%{public}d", batchCount, offset);
        ffrt::submit(
            [this, offset, maxSourceDbFileId, maxDestDbFileId]() {
                AbsorbNewPhotosForCloudBatch(offset, 0, maxSourceDbFileId, maxDestDbFileId, minDestDbFileId_);
            },
            {&offset},
            {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
}

void ReverseCloneRestore::InitializeTableAlbumIdMapForReverse()
{
    MEDIA_INFO_LOG("ReverseCloneRestore: InitializeTableAlbumIdMapForReverse start");

    CHECK_AND_RETURN_LOG(this->destRdb_ != nullptr, "destination rdbStore is null");

    // 从 destRdb 读取所有 PhotoAlbum 的相册
    std::string sql = "SELECT album_id FROM " + PhotoAlbumColumns::TABLE + " ORDER BY album_id";
    auto resultSet = destRdb_->QuerySql(sql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query PhotoAlbum from destRdb");

    // 建立 identity mapping: albumId -> albumId
    auto& albumIdMap = tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    int32_t mappedCount = 0;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = GetInt32Val("album_id", resultSet);
        albumIdMap[albumId] = albumId;
        mappedCount++;
    }
    resultSet->Close();

    MEDIA_INFO_LOG(
        "ReverseCloneRestore: InitializeTableAlbumIdMapForReverse completed, mapped %{public}d albums",
        mappedCount);
}

void ReverseCloneRestore::BuildReversePhotoInfoMap(const vector<FileInfo>& fileInfos)
{
    std::lock_guard<ffrt::mutex> lock(photosInfoMutex_);
    for (const auto& fileInfo : fileInfos) {
        if (fileInfo.fileIdOld <= 0) {
            continue;
        }
        PhotoInfo photoInfo;
        photoInfo.fileIdNew = fileInfo.fileIdOld;
        photoInfo.fileType = fileInfo.fileType;
        photoInfo.displayName = fileInfo.displayName;
        photoInfo.cloudPath = fileInfo.cloudPath;
        reversePhotoInfoMap_.insert(std::make_pair(fileInfo.fileIdOld, photoInfo));
    }
}

static void InsertMapCodes(
    int64_t photoRowNum, vector<FileInfo> fileInfos, std::shared_ptr<NativeRdb::RdbStore> &destRdb)
{
    int64_t startMapCode = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t mapErrCode = RestoreMapCodeUtils::ReverseFileInfosToMapCode(destRdb, fileInfos);
    int64_t endMapCode = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("InsertMapCodes mapErrCode %{public}d photoRowNum %{public}" PRId64
        ", mapCode restore cost %{public}" PRId64 ".", mapErrCode, photoRowNum, endMapCode - startMapCode);
}

bool ReverseCloneRestore::ResolveDataConflictsAfterDuplicate(ReverseClonePhotoBatchContext &batch,
    std::vector<int32_t> &failedFileIds)
{
    ReverseDataConflictStats stats;
    failedFileIds.clear();
    const UniqueIdGetter getUniqueId = [this](int32_t mediaType) {
        return GetUniqueId(mediaType);
    };

    for (auto &fileInfo : batch.validFileInfos) {
        ReverseStaleTargetResource staleResource;
        ReverseDataConflictResult result =
            ResolveDataConflictForFile(destRdb_, fileInfo, getUniqueId, staleResource);
        if (result.status == ReverseDataConflictStatus::FAILED && result.fileId > 0) {
            failedFileIds.emplace_back(result.fileId);
        }
        RecordDataConflictResult(batch, result, staleResource, stats);
    }

    MEDIA_INFO_LOG("Reverse data conflict after duplicate: valid=%{public}zu, values=%{public}zu, "
        "skippedDuplicate=%{public}d, noConflict=%{public}d, movedOld=%{public}d, deletedOld=%{public}d, "
        "stale=%{public}d, failed=%{public}d, staleResources=%{public}zu",
        batch.validFileInfos.size(), batch.values.size(), stats.skippedDuplicateCount, stats.noConflictCount,
        stats.movedOldAssetCount, stats.deletedOldAssetCount, stats.staleResourceCount, stats.failedCount,
        batch.staleTargetResources.size());
    return !batch.values.empty() && !batch.validFileInfos.empty();
}

void ReverseCloneRestore::UpdateSyncStatusForInsertedPhotos(
    const ReverseClonePhotoBatchContext& batch,
    int64_t photoRowNum)
{
    if (photoRowNum <= 0) {
        return;
    }

    string updateSql = "UPDATE " + string(PhotoColumn::PHOTOS_TABLE) + " SET " +
        string(PhotoColumn::PHOTO_SYNC_STATUS) + " = -1 WHERE " +
        string(MediaColumn::MEDIA_ID) + " IN (";

    // 构造 IN 子句
    bool first = true;
    for (size_t i = 0; i < batch.validFileInfos.size(); i++) {
        if (batch.validFileInfos[i].fileIdOld <= 0) {
            continue;
        }
        if (!first) {
            updateSql += ",";
        }
        updateSql += to_string(batch.validFileInfos[i].fileIdOld);
        first = false;
    }
    updateSql += ")";

    if (!first) {
        BackupDatabaseUtils::ExecuteSQL(destRdb_, updateSql, {});
        MEDIA_INFO_LOG("UpdateSyncStatusForInsertedPhotos: updated sync_status to -1 for inserted photos");
    }
}

void ReverseCloneRestore::EnsureCommittedFailedAssetsOrigin(const ReverseClonePhotoBatchContext &batch,
    const std::vector<int32_t> &failedFileIds, const std::string &stage)
{
    if (failedFileIds.empty()) {
        return;
    }

    FailedOriginEnsureStats stats;
    ReverseClonePhotoBatchContext failedBatch = BuildFailedResourceBatch(batch, failedFileIds, stats);
    MEDIA_INFO_LOG("Reverse absorb committed failed assets checked, stage=%{public}s, failedFileIds=%{public}zu, "
        "needEnsure=%{public}zu, dstSameSize=%{public}zu, dstSizeMismatch=%{public}zu, "
        "dstDeleteFailed=%{public}zu, sourceMissing=%{public}zu, noSource=%{public}zu, invalidDst=%{public}zu, "
        "noPlan=%{public}zu", stage.c_str(), failedFileIds.size(), stats.needEnsureCount, stats.dstSameSizeCount,
        stats.dstSizeMismatchCount, stats.dstDeleteFailedCount, stats.sourceMissingCount,
        stats.noSourceResourceCount, stats.invalidDstCount, stats.noPlanCount);
    if (failedBatch.resourcePlans.empty()) {
        MEDIA_WARN_LOG("Reverse absorb committed failed assets need no origin ensure, stage=%{public}s, "
            "failedFileIds=%{public}zu", stage.c_str(), failedFileIds.size());
        return;
    }

    MEDIA_WARN_LOG("Reverse absorb committed failed assets ensure origin, stage=%{public}s, "
        "failedFileIds=%{public}zu, resourcePlans=%{public}zu", stage.c_str(), failedFileIds.size(),
        failedBatch.resourcePlans.size());
    resourceInheritHelper_.FinalizeBatch(failedBatch, destRdb_, reverseRestoreReportInfo_);
}

void ReverseCloneRestore::HandleAbsorbPhotosFinalFailure(const std::string &stage, int32_t offset,
    const ReverseClonePhotoBatchContext *batch)
{
    if (!needReportFailed_) {
        AddToPhotosFailedOffsets(offset);
        return;
    }

    if (batch == nullptr) {
        MEDIA_WARN_LOG("Reverse absorb photos final failed before batch context ready, stage=%{public}s, "
            "offset=%{public}d", stage.c_str(), offset);
        AddToPhotosFailedOffsets(offset);
        return;
    }

    if (batch->resourcePlans.empty()) {
        MEDIA_WARN_LOG("Reverse absorb photos final failed without source resource plans, stage=%{public}s, "
            "offset=%{public}d", stage.c_str(), offset);
        AddToPhotosFailedOffsets(offset);
        return;
    }

    MEDIA_WARN_LOG("Reverse absorb photos final failed, force absorb source resources, stage=%{public}s, "
        "offset=%{public}d, resourcePlans=%{public}zu", stage.c_str(), offset, batch->resourcePlans.size());
    resourceInheritHelper_.ForceAbsorbSourceResourcesOnCommitFailed(*batch);
    AddToPhotosFailedOffsets(offset);
}

void ReverseCloneRestore::AbsorbNewPhotosBatch(int32_t offset, int32_t isRelatedToPhotoMap,
    int32_t maxSourceDbFileId, int32_t maxDestDbFileId, int32_t minDestDbFileId)
{
    MEDIA_INFO_LOG(
        "AbsorbNewPhotosBatch: start, offset=%{public}d, isRelatedToPhotoMap=%{public}d, minDestDbFileId=%{public}d",
        offset, isRelatedToPhotoMap, minDestDbFileId);

    vector<FileInfo> fileInfos = QueryFileInfos(offset, isRelatedToPhotoMap);
    if (fileInfos.empty()) {
        HandleAbsorbPhotosFinalFailure("QueryLocalFileInfos", offset, nullptr);
        return;
    }

    ReverseClonePhotoBatchContext batch;
    batch.cloudRestoreSatisfied = IsCloudRestoreSatisfied();
    batch.values = GetInsertValuesForAbsorb(fileInfos, batch.resourcePlans);
    std::vector<int32_t> dataConflictFailedFileIds;
    AlbumAssetAbsorb::DuplicateCount duplicateCount;
    if (!resourceInheritHelper_.PrepareBatch(fileInfos, maxDestDbFileId, minDestDbFileId, destRdb_, albumAssetAbsorb_,
        batch, duplicateCount)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosBatch: PrepareBatch failed");
        AppendErrorInfo("PrepareLocalBatch failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("PrepareLocalBatch", offset, &batch);
        return;
    }
    if (!ResolveDataConflictsAfterDuplicate(batch, dataConflictFailedFileIds)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosBatch: ResolveDataConflictsAfterDuplicate failed");
        AppendErrorInfo("ResolveLocalDataConflict failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("ResolveLocalDataConflict", offset, &batch);
        return;
    }

    int64_t photoRowNum = 0;
    if (!resourceInheritHelper_.CommitPhotosBatch(batch, destRdb_, photoRowNum)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosBatch: CommitPhotosBatch failed");
        AppendErrorInfo("CommitLocalBatch failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("CommitLocalBatch", offset, &batch);
        return;
    }

    BuildReversePhotoInfoMap(batch.validFileInfos);

    migrateDatabaseNumber_ += photoRowNum;
    reverseRestoreReportInfo_.duplicateCount += duplicateCount.total;
    reverseRestoreReportInfo_.duplicateLakeVideoCount += duplicateCount.hoLakeVideo;
    reverseRestoreReportInfo_.duplicateLakeImageCount += duplicateCount.hoLakeImage;
    reverseRestoreReportInfo_.duplicateVideoCount += duplicateCount.nonHoLakeVideo;
    reverseRestoreReportInfo_.duplicateImageCount += duplicateCount.nonHoLakeImage;
    MEDIA_INFO_LOG("AbsorbNewPhotosBatch: committed %{public}ld photos", photoRowNum);

    UpdateSyncStatusForInsertedPhotos(batch, photoRowNum);
    resourceInheritHelper_.FinalizeBatch(batch, destRdb_, reverseRestoreReportInfo_);
    EnsureCommittedFailedAssetsOrigin(batch, dataConflictFailedFileIds, "ResolveLocalDataConflict");

    // 更新duplicateAssetMap_：记录判重删掉的file_id到新机数据库中的file_id的映射（加锁保护）
    albumAssetAbsorb_.UpdateDuplicateAssetMapForDuplicates(batch.validFileInfos, duplicateAssetMap_,
        &duplicateAssetMapMutex_);

    InsertMapCodes(photoRowNum, batch.validFileInfos, destRdb_);
    MEDIA_INFO_LOG("AbsorbNewPhotosBatch: end, offset=%{public}d", offset);
}

void ReverseCloneRestore::ProcessNewPhotosFailedOffsets(int32_t isRelatedToPhotoMap,
    int32_t maxSourceDbFileId, int32_t maxDestDbFileId)
{
    MEDIA_INFO_LOG("ProcessNewPhotosFailedOffsets: start, isRelatedToPhotoMap=%{public}d", isRelatedToPhotoMap);
    size_t vectorLen = photosFailedOffsets_.size();
    needReportFailed_ = true;
    for (size_t offset = 0; offset < vectorLen; offset++) {
        AbsorbNewPhotosBatch(photosFailedOffsets_[offset], isRelatedToPhotoMap,
            maxSourceDbFileId, maxDestDbFileId, minDestDbFileId_);
    }
    photosFailedOffsets_.clear();
    MEDIA_INFO_LOG("ProcessNewPhotosFailedOffsets: end, processed %{public}zu failed batches", vectorLen);
}

// 反向克隆相册吸收相关方法

vector<NativeRdb::ValuesBucket> ReverseCloneRestore::GetInsertValuesForAbsorbAlbum(vector<AlbumInfo> &albumInfos,
                                                                                   vector<string> &albumIds,
                                                                                   const string &tableName)
{
    vector<NativeRdb::ValuesBucket> values;
    MEDIA_INFO_LOG("GetInsertValuesForAbsorbAlbum: Processing %{public}zu albums for table %{public}s",
                   albumInfos.size(), tableName.c_str());

    for (size_t i = 0; i < albumInfos.size(); i++) {
        // 直接从 sourceRdb 查询所有字段并构造 ValuesBucket
        NativeRdb::ValuesBucket value;

        // 设置 album_id
        value.PutInt(PhotoAlbumColumns::ALBUM_ID, albumInfos[i].albumIdOld);
        MEDIA_INFO_LOG("GetInsertValuesForAbsorbAlbum: album[%{public}zu] albumId=%{public}d, albumName=%{public}s", i,
                       albumInfos[i].albumIdOld, albumInfos[i].albumName.c_str());

        // 设置其他字段（从 valMap 获取）
        for (auto it = albumInfos[i].valMap.begin(); it != albumInfos[i].valMap.end(); ++it) {
            const string &columnName = it->first;
            const auto &columnValue = it->second;

            // 根据值类型放入 ValuesBucket
            if (holds_alternative<int32_t>(columnValue)) {
                value.PutInt(columnName, get<int32_t>(columnValue));
            } else if (holds_alternative<int64_t>(columnValue)) {
                value.PutLong(columnName, get<int64_t>(columnValue));
            } else if (holds_alternative<double>(columnValue)) {
                value.PutDouble(columnName, get<double>(columnValue));
            } else if (holds_alternative<string>(columnValue)) {
                value.PutString(columnName, get<string>(columnValue));
            }
        }

        values.emplace_back(value);
    }

    MEDIA_INFO_LOG("GetInsertValuesForAbsorbAlbum: Generated %{public}zu ValuesBucket", values.size());
    return values;
}

void ReverseCloneRestore::UpdateSystemAlbumFields()
{
    MEDIA_INFO_LOG("UpdateSystemAlbumFields: Starting to update system album IDs");

    // 从 sourceRdb 查询所有 type=1024 的系统相册
    string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
                      PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SYSTEM);
    auto sourceResultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_LOG(sourceResultSet != nullptr, "Failed to query system albums from sourceRdb");

    int32_t updatedCount = 0;
    int32_t insertedCount = 0;

    while (sourceResultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumInfo albumInfo;
        if (!ParseAlbumResultSet(PhotoAlbumColumns::TABLE, sourceResultSet, albumInfo)) {
            continue;
        }

        int32_t sourceAlbumId = albumInfo.albumIdOld;
        int32_t albumSubtype = static_cast<int32_t>(albumInfo.albumSubType);

        // 在 destRdb 中查找相同 subtype 的系统相册
        string destQuerySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE +
                              " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SYSTEM) +
                              " AND " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(albumSubtype);
        auto destResultSet = BackupDatabaseUtils::GetQueryResultSet(destRdb_, destQuerySql);
        CHECK_AND_CONTINUE(destResultSet != nullptr);

        if (destResultSet->GoToFirstRow() == NativeRdb::E_OK) {
            int32_t destAlbumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, destResultSet);
            destResultSet->Close();

            if (destAlbumId != sourceAlbumId) {
                MEDIA_INFO_LOG("UpdateSystemAlbumFields: Updating system album, destAlbumId=%{public}d, "
                               "sourceAlbumId=%{public}d, subtype=%{public}d",
                               destAlbumId, sourceAlbumId, albumSubtype);
                UpdateSystemAlbumOwnerAlbumId(sourceAlbumId, destAlbumId);
                UpdateSystemAlbumField(sourceAlbumId, destAlbumId);
                updatedCount++;
            }
        } else {
            destResultSet->Close();
            // 不存在重复，插入新相册
            if (!InsertNewAlbum("UpdateSystemAlbumFields", sourceAlbumId, albumInfo)) {
                AppendErrorInfo("SystemAlbum failed: " +
                                to_string(sourceAlbumId));
            } else {
                insertedCount++;
            }
        }
    }
    sourceResultSet->Close();

    migrateDatabaseAlbumNumber_ += insertedCount;
    MEDIA_INFO_LOG("UpdateSystemAlbumFields: Completed, inserted=%{public}d, updated=%{public}d system albums",
                   insertedCount, updatedCount);
}

void ReverseCloneRestore::UpdateSystemAlbumOwnerAlbumId(int32_t sourceAlbumId, int32_t destAlbumId)
{
    string updatePhotosSql = "UPDATE Photos SET owner_album_id = ? WHERE owner_album_id = ?";
    int32_t ret = destRdb_->ExecuteSql(updatePhotosSql, vector<NativeRdb::ValueObject>{sourceAlbumId, destAlbumId});
    MEDIA_INFO_LOG("UpdateSystemAlbumFields: updated owner_album_id from %{public}d to %{public}d, ret=%{public}d",
                   destAlbumId, sourceAlbumId, ret);
}

void ReverseCloneRestore::UpdateSystemAlbumField(int32_t sourceAlbumId, int32_t destAlbumId)
{
    // 查询 sourceRdb 中的封面字段
    string querySql = "SELECT " + PhotoAlbumColumns::ALBUM_COVER_URI + ", " +
                      PhotoAlbumColumns::COVER_URI_SOURCE + ", " +
                      PhotoAlbumColumns::COVER_CLOUD_ID +
                      " FROM " + PhotoAlbumColumns::TABLE +
                      " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = " + to_string(sourceAlbumId);
    auto sourceResultSet = BackupDatabaseUtils::QuerySql(sourceRdb_, querySql, {});
    CHECK_AND_RETURN_LOG(sourceResultSet != nullptr, "Failed to query cover fields from sourceRdb");
    
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_ID, sourceAlbumId);
    
    if (sourceResultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string coverUri = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, sourceResultSet);
        int32_t coverUriSource = GetInt32Val(PhotoAlbumColumns::COVER_URI_SOURCE, sourceResultSet);
        string coverCloudId = GetStringVal(PhotoAlbumColumns::COVER_CLOUD_ID, sourceResultSet);
        
        values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
        values.PutInt(PhotoAlbumColumns::COVER_URI_SOURCE, coverUriSource);
        values.PutString(PhotoAlbumColumns::COVER_CLOUD_ID, coverCloudId);
    }
    sourceResultSet->Close();
    
    unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoAlbumColumns::TABLE);
    predicates->EqualTo(PhotoAlbumColumns::ALBUM_ID, destAlbumId);
    int32_t updatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(destRdb_, updatedRows, values, predicates);
    MEDIA_INFO_LOG("UpdateSystemAlbumFields: updated album_id from %{public}d to %{public}d, ret=%{public}d, "
                   "rows=%{public}d",
                   destAlbumId, sourceAlbumId, ret, updatedRows);

    // 更新 tableAlbumIdMap_
    auto& albumIdMap = tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    albumIdMap[destAlbumId] = sourceAlbumId;

    MEDIA_INFO_LOG("UpdateSystemAlbumField: tableAlbumIdMap_[%{public}d] = %{public}d", destAlbumId, sourceAlbumId);
}

NativeRdb::ValuesBucket ReverseCloneRestore::BuildAlbumValuesBucket(const AlbumInfo& albumInfo,
    const std::unordered_set<std::string>& excludeColumns, int32_t destAlbumId)
{
    NativeRdb::ValuesBucket values;

    // 基本字段
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(albumInfo.albumType));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(albumInfo.albumSubType));
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumInfo.albumName);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, albumInfo.lPath);
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, albumInfo.albumBundleName);

    // dateModified
    if (destAlbumId != -1) {
        // 更新场景（destAlbumId != -1）：使用当前时间戳
        values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    } else {
        // 插入新相册场景（destAlbumId == -1）：直接使用原始 dateModified
        values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, albumInfo.dateModified);
    }

    // upload_status（如果不在排除列表中）
    if (excludeColumns.find(PhotoAlbumColumns::UPLOAD_STATUS) == excludeColumns.end()) {
        values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, albumInfo.uploadStatus);
    }

    // 遍历 valMap 添加所有其他字段
    for (auto it = albumInfo.valMap.begin(); it != albumInfo.valMap.end(); ++it) {
        const string &columnName = it->first;
        const auto &columnValue = it->second;

        // 跳过排除的列
        if (excludeColumns.find(columnName) != excludeColumns.end()) {
            continue;
        }

        if (holds_alternative<int32_t>(columnValue)) {
            values.PutInt(columnName, get<int32_t>(columnValue));
        } else if (holds_alternative<int64_t>(columnValue)) {
            values.PutLong(columnName, get<int64_t>(columnValue));
        } else if (holds_alternative<double>(columnValue)) {
            values.PutDouble(columnName, get<double>(columnValue));
        } else if (holds_alternative<string>(columnValue)) {
            values.PutString(columnName, get<string>(columnValue));
        }
    }

    return values;
}

bool ReverseCloneRestore::InsertNewAlbum(const std::string& logTag, int32_t sourceAlbumId, const AlbumInfo &albumInfo)
{
    MEDIA_INFO_LOG("%{public}s: Inserting new album, sourceAlbumId=%{public}d, type=%{public}d", logTag.c_str(),
                   sourceAlbumId, static_cast<int32_t>(albumInfo.albumType));

    NativeRdb::ValuesBucket values = BuildAlbumValuesBucket(albumInfo, {}, -1);

    int64_t insertRowId = -1;
    int32_t ret = destRdb_->Insert(insertRowId, PhotoAlbumColumns::TABLE, values);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("%{public}s: Failed to insert album, sourceAlbumId=%{public}d, ret=%{public}d",
                      logTag.c_str(), sourceAlbumId, ret);
        return false;
    }
    MEDIA_INFO_LOG("%{public}s: inserted albumId=%{public}d, ret=%{public}d, rowId=%{public}lld",
                   logTag.c_str(), sourceAlbumId, ret, (long long)insertRowId);
    return true;
}

std::string ReverseCloneRestore::EnsureAlbumLPath(const std::string& lPath, const std::string& sourcePath)
{
    // 如果 lPath 非空，直接返回
    if (!lPath.empty()) {
        return lPath;
    }

    // lPath 为空，从 sourcePath 解析
    std::string result = "/Pictures/其它";

    // 查找根目录位置
    size_t startPos = std::string::npos;
    std::string nestedRootPattern = "/storage/emulated/\\d+/";
    std::string nonNestedRootPattern = "^/storage/\\d+/";
    std::smatch matchResult;
    std::regex nestedRegex(nestedRootPattern);
    std::regex nonNestedRegex(nonNestedRootPattern);

    if (std::regex_search(sourcePath, matchResult, nestedRegex)) {
        startPos = static_cast<size_t>(matchResult.position()) + static_cast<size_t>(matchResult.length());
    } else if (std::regex_search(sourcePath, matchResult, nonNestedRegex)) {
        startPos = static_cast<size_t>(matchResult.position()) + static_cast<size_t>(matchResult.length());
    }

    if (startPos == std::string::npos) {
        MEDIA_WARN_LOG("EnsureAlbumLPath: Failed to parse sourcePath, using default lPath");
        return result;
    }

    size_t endPos = sourcePath.find_last_of("/");
    if (endPos == std::string::npos || endPos <= startPos) {
        MEDIA_WARN_LOG("EnsureAlbumLPath: Invalid sourcePath format, using default lPath");
        return result;
    }

    result = sourcePath.substr(startPos, endPos - startPos);

    // 特殊处理：隐藏相册路径
    if (result == "/Pictures/HiddenAlbum") {
        result = "/Pictures/Recover";
    }

    MEDIA_INFO_LOG("EnsureAlbumLPath: lPath empty, parsed from sourcePath: %{public}s",
                   MediaFileUtils::DesensitizePath(result).c_str());
    return result;
}

void ReverseCloneRestore::UpdateDuplicateSourceOrUserAlbum(int32_t sourceAlbumId, int32_t destAlbumId,
                                                           const AlbumInfo &albumInfo)
{
    MEDIA_INFO_LOG("InsertSourceAndUserAlbumsWithDuplicateCheck: Found duplicate album by lPath, "
                   "destAlbumId=%{public}d, sourceAlbumId=%{public}d, name=%{public}s, lPath=%{public}s",
                   destAlbumId, sourceAlbumId, albumInfo.albumName.c_str(), albumInfo.lPath.c_str());

    // 更新照片的 owner_album_id
    string updatePhotosSql = "UPDATE Photos SET owner_album_id = ? WHERE owner_album_id = ?";
    int32_t ret = destRdb_->ExecuteSql(updatePhotosSql, vector<NativeRdb::ValueObject>{sourceAlbumId, destAlbumId});
    MEDIA_INFO_LOG("InsertSourceAndUserAlbumsWithDuplicateCheck: updated owner_album_id from %{public}d to "
                   "%{public}d, ret=%{public}d",
                   destAlbumId, sourceAlbumId, ret);

    // 构建排除列集合
    std::unordered_set<std::string> excludeColumns = BuildExcludeColumnsForDuplicateAlbum(destAlbumId);

    NativeRdb::ValuesBucket values = BuildAlbumValuesBucket(albumInfo, excludeColumns, destAlbumId);

    unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoAlbumColumns::TABLE);
    predicates->EqualTo(PhotoAlbumColumns::ALBUM_ID, destAlbumId);
    int32_t updatedRows = 0;
    ret = BackupDatabaseUtils::Update(destRdb_, updatedRows, values, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateDuplicateSourceOrUserAlbum: failed to update album, ret=%{public}d", ret);
        AppendErrorInfo("dup Souce/UserAlbum failed: " +
                        to_string(sourceAlbumId));
    }
    MEDIA_INFO_LOG("InsertSourceAndUserAlbumsWithDuplicateCheck: updated album_id from %{public}d to %{public}d, "
                   "all fields (except upload_status), ret=%{public}d, rows=%{public}d",
                   destAlbumId, sourceAlbumId, ret, updatedRows);

    // 更新 tableAlbumIdMap_
    auto &albumIdMap = tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    albumIdMap[destAlbumId] = sourceAlbumId;

    MEDIA_INFO_LOG("UpdateDuplicateSourceOrUserAlbum: tableAlbumIdMap_[%{public}d] = %{public}d", destAlbumId,
                   sourceAlbumId);
}

std::unordered_set<std::string> ReverseCloneRestore::BuildExcludeColumnsForDuplicateAlbum(int32_t destAlbumId)
{
    // 查询 destRdb 中的 cover_uri_source，判断是否有自定义封面
    string queryCoverSql = "SELECT " + PhotoAlbumColumns::COVER_URI_SOURCE +
                           " FROM " + PhotoAlbumColumns::TABLE +
                           " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = ?";
    auto coverResultSet = BackupDatabaseUtils::QuerySql(destRdb_, queryCoverSql, {destAlbumId});
    int32_t destCoverUriSource = 0;
    if (coverResultSet != nullptr && coverResultSet->GoToNextRow() == NativeRdb::E_OK) {
        destCoverUriSource = GetInt32Val(PhotoAlbumColumns::COVER_URI_SOURCE, coverResultSet);
    }
    if (coverResultSet != nullptr) {
        coverResultSet->Close();
    }

    // 构建基础 excludeColumns
    std::unordered_set<std::string> excludeColumns = {
        PhotoAlbumColumns::UPLOAD_STATUS,
        PhotoAlbumColumns::ALBUMS_ORDER,
        PhotoAlbumColumns::ORDER_SECTION,
        PhotoAlbumColumns::ORDER_TYPE,
        PhotoAlbumColumns::ORDER_STATUS,
        PhotoAlbumColumns::STYLE2_ALBUMS_ORDER,
        PhotoAlbumColumns::STYLE2_ORDER_SECTION,
        PhotoAlbumColumns::STYLE2_ORDER_TYPE,
        PhotoAlbumColumns::STYLE2_ORDER_STATUS
    };

    // 如果 destRdb 有自定义封面（cover_uri_source > 0），则保留 destRdb 的封面字段
    if (destCoverUriSource > 0) {
        excludeColumns.insert(PhotoAlbumColumns::ALBUM_COVER_URI);
        excludeColumns.insert(PhotoAlbumColumns::COVER_URI_SOURCE);
        excludeColumns.insert(PhotoAlbumColumns::COVER_CLOUD_ID);
        MEDIA_INFO_LOG("BuildExcludeColumnsForDuplicateAlbum: Preserving destRdb custom cover for albumId=%{public}d",
                       destAlbumId);
    }

    return excludeColumns;
}

int32_t ReverseCloneRestore::CheckDuplicateAlbumInDest(const std::string &lPath)
{
    string destQuerySql = "SELECT " + PhotoAlbumColumns::ALBUM_ID + " FROM " + PhotoAlbumColumns::TABLE + " WHERE (" +
                          PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::SOURCE) + " OR " +
                          PhotoAlbumColumns::ALBUM_TYPE + " = " + to_string(PhotoAlbumType::USER) + ") AND " +
                          "LOWER(" + PhotoAlbumColumns::ALBUM_LPATH + ") = LOWER(?)";
    auto destResultSet = BackupDatabaseUtils::QuerySql(destRdb_, destQuerySql, {lPath});
    if (destResultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query destRdb for duplicate album");
        return -1;
    }

    int32_t destAlbumId = -1;
    if (destResultSet->GoToFirstRow() == NativeRdb::E_OK) {
        destAlbumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, destResultSet);
    }
    destResultSet->Close();
    return destAlbumId;
}

bool ReverseCloneRestore::ProcessSourceAndUserAlbum(AlbumInfo &albumInfo, int32_t &insertedCount, int32_t &updatedCount)
{
    int32_t sourceAlbumId = albumInfo.albumIdOld;
    string lPath = albumInfo.lPath;

    // 如果 lPath 为空，从 sourcePath 解析
    if (lPath.empty()) {
        string sourcePath;
        auto it = albumInfo.valMap.find("source_path");
        if (it != albumInfo.valMap.end() && holds_alternative<string>(it->second)) {
            sourcePath = get<string>(it->second);
        }
        lPath = EnsureAlbumLPath(lPath, sourcePath);
        albumInfo.lPath = lPath;
    }

    // 在 destRdb 中查找相同 lPath 的 SOURCE 或 USER 相册（不区分大小写）
    int32_t destAlbumId = CheckDuplicateAlbumInDest(lPath);
    if (destAlbumId != -1) {
        UpdateSourceOrUserAlbumUploadStatus(destAlbumId, albumInfo);
        if (destAlbumId != sourceAlbumId) {
            // lPath 一致：更新 destRdb 中对应相册的 album_id 和 album_name
            UpdateDuplicateSourceOrUserAlbum(sourceAlbumId, destAlbumId, albumInfo);
            updatedCount++;
        }
    } else if (destAlbumId == -1) {
        // lPath 不一致：直接从 sourceRdb 插入新相册，完整保留整行信息
        if (!InsertNewAlbum("InsertSourceAndUserAlbumsWithDuplicateCheck", sourceAlbumId, albumInfo)) {
            AppendErrorInfo("Souce/UserAlbum failed: " +
                            to_string(sourceAlbumId));
        } else {
            insertedCount++;
        }
    }
    return true;
}

void ReverseCloneRestore::InsertSourceAndUserAlbumsWithDuplicateCheck()
{
    MEDIA_INFO_LOG("InsertSourceAndUserAlbumsWithDuplicateCheck: Starting to process SOURCE and USER albums");

    // 从 sourceRdb 查询所有 type=2048 (SOURCE) 和 type=0 (USER) 的相册
    string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + " = " +
                      to_string(PhotoAlbumType::SOURCE) + " OR " + PhotoAlbumColumns::ALBUM_TYPE + " = " +
                      to_string(PhotoAlbumType::USER);
    auto sourceResultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    if (sourceResultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query SOURCE and USER albums from sourceRdb");
        AppendErrorInfo("no SOURCE/USER album");
        return;
    }

    int32_t insertedCount = 0;
    int32_t updatedCount = 0;

    while (sourceResultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumInfo albumInfo;
        if (!ParseAlbumResultSet(PhotoAlbumColumns::TABLE, sourceResultSet, albumInfo)) {
            continue;
        }
        ProcessSourceAndUserAlbum(albumInfo, insertedCount, updatedCount);
    }
    sourceResultSet->Close();

    // 处理仅旧机独有的相册upload_status
    UpdateDestOnlyAlbumsUploadStatus();

    migrateDatabaseAlbumNumber_ += insertedCount;
    MEDIA_INFO_LOG("InsertSourceAndUserAlbumsWithDuplicateCheck: Completed, inserted=%{public}d, "
                   "updated=%{public}d SOURCE and USER albums",
                   insertedCount, updatedCount);
}

void ReverseCloneRestore::UpdateDatabase()
{
    int64_t startUpdateDatabase = MediaFileUtils::UTCTimeMilliSeconds();
    updateProcessStatus_ = ProcessStatus::START;
    GetUpdateTotalCount();
    MEDIA_INFO_LOG("Start update all albums");
    int32_t errCode = MediaLibraryDataManager::GetInstance()->InitReverseMediaLibraryRdbStore();
    ExecuteAnalyzeInDatabase(mediaLibraryRdb_);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);
    MEDIA_INFO_LOG("Start update unique number");
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, imageNumber_, CONST_IMAGE_ASSET_TYPE);
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, videoNumber_, CONST_VIDEO_ASSET_TYPE);
    BackupDatabaseUtils::UpdateUniqueNumber(mediaLibraryRdb_, audioNumber_, CONST_AUDIO_ASSET_TYPE);
    MEDIA_INFO_LOG("Start notify");
    NotifyAlbum();
    updateProcessStatus_ = ProcessStatus::STOP;
    int64_t endUpdateDatabase = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("TimeCost: UpdateDatabase cost: %{public}" PRId64, endUpdateDatabase - startUpdateDatabase);
}

void ReverseCloneRestore::AbsorbNewPhotosForCloudBatch(int32_t offset, int32_t isRelatedToPhotoMap,
    int32_t maxSourceDbFileId, int32_t maxDestDbFileId, int32_t minDestDbFileId)
{
    MEDIA_INFO_LOG("AbsorbNewPhotosForCloudBatch: start, offset=%{public}d, isRelatedToPhotoMap=%{public}d, "
                   "minDestDbFileId=%{public}d", offset, isRelatedToPhotoMap, minDestDbFileId);

    vector<FileInfo> fileInfos = QueryCloudFileInfos(offset, isRelatedToPhotoMap);
    if (fileInfos.empty()) {
        HandleAbsorbPhotosFinalFailure("QueryCloudFileInfos", offset, nullptr);
        return;
    }

    ReverseClonePhotoBatchContext batch;
    batch.cloudRestoreSatisfied = IsCloudRestoreSatisfied();
    batch.values = GetInsertValuesForAbsorb(fileInfos, batch.resourcePlans);
    std::vector<int32_t> dataConflictFailedFileIds;
    AlbumAssetAbsorb::DuplicateCount duplicateCount;
    if (!resourceInheritHelper_.PrepareBatch(fileInfos, maxDestDbFileId, minDestDbFileId, destRdb_, albumAssetAbsorb_,
        batch, duplicateCount)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosForCloudBatch: PrepareBatch failed");
        AppendErrorInfo("PrepareCloudBatch failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("PrepareCloudBatch", offset, &batch);
        return;
    }
    if (!ResolveDataConflictsAfterDuplicate(batch, dataConflictFailedFileIds)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosForCloudBatch: ResolveDataConflictsAfterDuplicate failed");
        AppendErrorInfo("ResolveCloudDataConflict failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("ResolveCloudDataConflict", offset, &batch);
        return;
    }

    int64_t photoRowNum = 0;
    if (!resourceInheritHelper_.CommitPhotosBatch(batch, destRdb_, photoRowNum)) {
        MEDIA_ERR_LOG("AbsorbNewPhotosForCloudBatch: CommitPhotosBatch failed");
        AppendErrorInfo("CommitCloudBatch failed: " + to_string(offset));
        HandleAbsorbPhotosFinalFailure("CommitCloudBatch", offset, &batch);
        return;
    }

    BuildReversePhotoInfoMap(batch.validFileInfos);

    migrateDatabaseNumber_ += photoRowNum;
    reverseRestoreReportInfo_.duplicateCount += duplicateCount.total;
    reverseRestoreReportInfo_.duplicateLakeVideoCount += duplicateCount.hoLakeVideo;
    reverseRestoreReportInfo_.duplicateLakeImageCount += duplicateCount.hoLakeImage;
    reverseRestoreReportInfo_.duplicateVideoCount += duplicateCount.nonHoLakeVideo;
    reverseRestoreReportInfo_.duplicateImageCount += duplicateCount.nonHoLakeImage;
    MEDIA_INFO_LOG("AbsorbNewPhotosForCloudBatch: committed %{public}ld cloud photos", photoRowNum);

    UpdateSyncStatusForInsertedPhotos(batch, photoRowNum);
    resourceInheritHelper_.FinalizeBatch(batch, destRdb_, reverseRestoreReportInfo_);
    EnsureCommittedFailedAssetsOrigin(batch, dataConflictFailedFileIds, "ResolveCloudDataConflict");

    // 更新duplicateAssetMap_：记录判重删掉的file_id到新机数据库中的file_id的映射（加锁保护）
    albumAssetAbsorb_.UpdateDuplicateAssetMapForDuplicates(batch.validFileInfos, duplicateAssetMap_,
        &duplicateAssetMapMutex_);

    InsertMapCodes(photoRowNum, batch.validFileInfos, destRdb_);
    MEDIA_INFO_LOG("AbsorbNewPhotosForCloudBatch: end, offset=%{public}d", offset);
}

void ReverseCloneRestore::ProcessNewPhotosForCloudFailedOffsets(int32_t isRelatedToPhotoMap,
    int32_t maxSourceDbFileId, int32_t maxDestDbFileId)
{
    MEDIA_INFO_LOG("ProcessNewPhotosForCloudFailedOffsets: start, isRelatedToPhotoMap=%{public}d", isRelatedToPhotoMap);
    size_t vectorLen = photosFailedOffsets_.size();
    needReportFailed_ = true;
    for (size_t offset = 0; offset < vectorLen; offset++) {
        AbsorbNewPhotosForCloudBatch(photosFailedOffsets_[offset], isRelatedToPhotoMap,
            maxSourceDbFileId, maxDestDbFileId, minDestDbFileId_);
    }
    photosFailedOffsets_.clear();
    MEDIA_INFO_LOG("ProcessNewPhotosForCloudFailedOffsets: end, processed %{public}zu failed batches", vectorLen);
}

void ReverseCloneRestore::UpdateSourceOrUserAlbumUploadStatus(const int32_t destAlbumId, const AlbumInfo &albumInfo)
{
    if (IsCloudRestoreSatisfied()) {
        return;
    }
    // 更新相册的 upload_status（使用 sourceRdb 的值）
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, albumInfo.uploadStatus);
    unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        make_unique<NativeRdb::AbsRdbPredicates>(PhotoAlbumColumns::TABLE);
    predicates->EqualTo(PhotoAlbumColumns::ALBUM_ID, destAlbumId);
    int32_t updatedRows = 0;
    int32_t ret = BackupDatabaseUtils::Update(destRdb_, updatedRows, values, predicates);
    MEDIA_INFO_LOG("Update uploadStatus: album_id %{public}d, upload_status: %{public}d, "
        "ret=%{public}d, rows=%{public}d", destAlbumId, albumInfo.uploadStatus, ret, updatedRows);
}

void ReverseCloneRestore::UpdateDestOnlyAlbumsUploadStatus()
{
    if (IsCloudRestoreSatisfied()) {
        return;
    }
    auto& albumIdMap = tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    // 收集 dest 独有的相册id（key == value）
    std::vector<std::string> destOnlyAlbumIds;
    for (const auto& entry : albumIdMap) {
        if (entry.first == entry.second) {
            destOnlyAlbumIds.push_back(std::to_string(entry.first));
        }
    }
    CHECK_AND_RETURN_LOG(!destOnlyAlbumIds.empty(), "No dest-only albums found");
    MEDIA_INFO_LOG("Found %{public}zu dest-only albums", destOnlyAlbumIds.size());

    std::string inClause = "(";
    for (size_t i = 0; i < destOnlyAlbumIds.size(); i++) {
        if (i > 0) {
            inClause += ", ";
        }
        inClause += destOnlyAlbumIds[i];
    }
    inClause += ")";

    std::string updateSql = "UPDATE PhotoAlbum SET upload_status = ( \
        CASE WHEN LOWER(lpath) IN ( \
            LOWER('/DCIM/Camera'), \
            LOWER('/Pictures/Screenrecords'), \
            LOWER('/Pictures/Screenshots') \
        ) \
        THEN 1 ELSE ? END ) \
        WHERE album_type IN (0, 2048) AND album_id IN " + inClause + ";";
    int32_t uploadStatus = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus();
    int32_t ret = destRdb_->ExecuteSql(updateSql, vector<NativeRdb::ValueObject>{ uploadStatus });
    MEDIA_INFO_LOG("update uploadStatus: %{public}d, ret: %{public}d", uploadStatus, ret);
}

vector<FileInfo> ReverseCloneRestore::QueryFileInfos(int32_t offset, int32_t isRelatedToPhotoMap)
{
    vector<FileInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, CLONE_QUERY_COUNT};

    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, result, "sourceRdb_ is null.");
    auto resultSet = sourceRdb_->QuerySql(SQL_PHOTOS_TABLE_QUERY_ALL, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo fileInfo;
        fileInfo.isRelatedToPhotoMap = isRelatedToPhotoMap;
        CHECK_AND_EXECUTE(!ParseReverseResultSet(resultSet, fileInfo), result.emplace_back(fileInfo));
    }
    resultSet->Close();

    this->photosClone_.SetFilePathForReverseClone(result, dstDevFileTransferConfig_.ancoFileTransfer);
    return result;
}

vector<FileInfo> ReverseCloneRestore::QueryCloudFileInfos(int32_t offset, int32_t isRelatedToPhotoMap)
{
    MEDIA_INFO_LOG("ReverseCloneRestore::QueryCloudFileInfos");
    vector<FileInfo> result;
    result.reserve(CLONE_QUERY_COUNT);
    std::vector<NativeRdb::ValueObject> bindArgs = {offset, CLONE_QUERY_COUNT};

    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, result, "sourceRdb_ is null.");
    auto resultSet = sourceRdb_->QuerySql(SQL_CLOUD_PHOTOS_TABLE_QUERY_ALL, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "QueryCloudFileInfos resultSql is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo fileInfo;
        fileInfo.isRelatedToPhotoMap = isRelatedToPhotoMap;
        if (ParseReverseResultSet(resultSet, fileInfo)) {
            result.emplace_back(fileInfo);
            RemoveInvalidLocalFiles(fileInfo);
        }
    }
    resultSet->Close();
    return result;
}

int32_t ReverseCloneRestore::GetAllPhotosRowCount()
{
    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, 0, "sourceRdb_ is null.");
    auto resultSet = sourceRdb_->QuerySql(SQL_PHOTOS_TABLE_COUNT_ALL);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

int32_t ReverseCloneRestore::GetAllCloudPhotosRowCount()
{
    CHECK_AND_RETURN_RET_LOG(sourceRdb_ != nullptr, 0, "sourceRdb_ is null.");
    auto resultSet = sourceRdb_->QuerySql(SQL_CLOUD_PHOTOS_TABLE_COUNT_ALL);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

ReverseCloneRestore::~ReverseCloneRestore()
{
    StopReverseRestoreStatusUpdateThread();
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS