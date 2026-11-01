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
#define MLOG_TAG "MediaLibraryIncrementalRecovery"

#include "medialibrary_meta_recovery.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <set>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>

#include "asset_accurate_refresh.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_scanner_manager.h"
#include "media_string_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_restore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "moving_photo_file_utils.h"
#include "operation/photo_owner_album_id_operation.h"
#include "photo_file_utils.h"
#include "post_event_utils.h"
#include "recovery_bucket_index.h"
#include "recovery_json_utils.h"
#include "recovery_summary.h"
#include "recovery_window.h"
#include "replica_recovery_policy.h"
#include "rdb_errno.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"

#ifdef MEDIALIB_RECOVERY_UNIT_TEST
#include "incremental_recovery_test_hooks.h"
#endif

namespace OHOS::Media {
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
constexpr int64_t MILLISECONDS_PER_SECOND = 1000;
constexpr int64_t NANOSECONDS_PER_MILLISECOND = 1000000;
constexpr int64_t MAX_ENTRY_FAILURE_LOGS = 10;
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
thread_local bool g_testScan = false;
struct RecoveryTestPaths {
    std::string album;
    std::string json;
    std::string photo;
    int32_t *reports = nullptr;
};
thread_local const RecoveryTestPaths *g_testPaths = nullptr;
#endif

struct PhotoRecord {
    bool exists = false;
    int32_t id = 0;
    int32_t position = 0;
    int32_t source = 0;
    int64_t pending = 0;
    int32_t temporary = 0;
    int32_t dirty = 0;
    int32_t flags = -1;
    int32_t subtype = 0;
    int32_t effectMode = 0;
    int32_t owner = 0;
    int32_t exifRotate = 0;
    bool modifiedNull = false;
    bool metaModifiedNull = false;
    bool sizeNull = false;
    bool hasAlbum = false;
    int64_t modified = 0;
    int64_t metaModified = 0;
    int64_t size = -1;
    int64_t deleted = 0;
};

std::string GetLogBucketName(const std::string &directory)
{
    const std::string name = fs::path(directory).filename().string();
    const bool numeric = !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char value) {
        return std::isdigit(value) != 0;
    });
    return numeric ? name : DfxUtils::GetSafePath(directory);
}

struct BucketScanState {
    std::string directory;
    bool jsonPass = false;

    void Start(const std::string &path, bool isJson)
    {
        directory = path;
        jsonPass = isJson;
    }

    void Finish(bool traversalComplete = true)
    {
        if (directory.empty()) {
            return;
        }
        MEDIA_INFO_LOG("Recovery bucket phase=%{public}s bucket=%{public}s completed=%{public}d",
            jsonPass ? "json" : "file", GetLogBucketName(directory).c_str(), traversalComplete);
        directory.clear();
    }
};

void IncrementCount(int64_t &count)
{
    if (count < std::numeric_limits<int64_t>::max()) {
        ++count;
    }
}

void RecordEntryFailure(DatabaseRecoveryReport &report, bool jsonPass, int32_t error, const std::string &path)
{
    IncrementCount(report.failed);
    report.lastScanError = error;
    if (report.failed <= MAX_ENTRY_FAILURE_LOGS) {
        MEDIA_ERR_LOG("Recovery candidate failed, phase=%{public}s error=%{public}d path=%{public}s",
            jsonPass ? "json" : "file", error, DfxUtils::GetSafePath(path).c_str());
    } else if (report.failed == MAX_ENTRY_FAILURE_LOGS + 1) {
        MEDIA_WARN_LOG("Further recovery candidate errors suppressed; see failure count");
    }
}

void FinishScanAttempt(DatabaseRecoveryReport &report, int32_t error, int64_t elapsed)
{
    report.stage = DatabaseRecoveryStage::BACKGROUND_SCAN;
    report.result = GetBackgroundRecoveryResult(error, report.failed);
    if (error != E_OK && error != NativeRdb::E_CANCEL) {
        report.lastScanError = error;
    }
    report.error = report.result == DatabaseRecoveryResult::PARTIAL ?
        (report.lastScanError != 0 ? report.lastScanError : E_ERR) : error;
    elapsed = std::max<int64_t>(elapsed, 0);
    report.scanMs = std::max<int64_t>(report.scanMs, 0);
    report.scanMs = elapsed > std::numeric_limits<int64_t>::max() - report.scanMs ?
        std::numeric_limits<int64_t>::max() : report.scanMs + elapsed;
}

void LogRecoveryCompletion(const RecoveryWindow &window, const DatabaseRecoveryReport &report)
{
    MEDIA_INFO_LOG("Incremental recovery scan completed, windowStart=%{public}lld windowEnd=%{public}lld "
        "result=%{public}d attempts=%{public}d elapsedMs=%{public}lld failed=%{public}lld",
        static_cast<long long>(window.startMs), static_cast<long long>(window.endMs),
        static_cast<int32_t>(report.result), report.scanAttempts, static_cast<long long>(report.scanMs),
        static_cast<long long>(report.failed));
}

void ReportRecoveryWindow(const DatabaseRecoveryReport &report)
{
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    if (g_testPaths != nullptr && g_testPaths->reports != nullptr) {
        ++*g_testPaths->reports;
        return;
    }
#endif
    PostEventUtils::GetInstance().PostDatabaseRecovery(report);
}

bool GetFileMtime(const struct stat &info, int64_t &mtime)
{
    if (info.st_mtim.tv_sec < 0 || info.st_mtim.tv_nsec < 0 ||
        info.st_mtim.tv_nsec >= MILLISECONDS_PER_SECOND * NANOSECONDS_PER_MILLISECOND ||
        info.st_mtim.tv_sec >
        (std::numeric_limits<int64_t>::max() - (MILLISECONDS_PER_SECOND - 1)) / MILLISECONDS_PER_SECOND) {
        return false;
    }
    mtime = info.st_mtim.tv_sec * MILLISECONDS_PER_SECOND +
        info.st_mtim.tv_nsec / NANOSECONDS_PER_MILLISECOND;
    return true;
}

bool SameFile(const struct stat &before, const struct stat &after)
{
    return S_ISREG(after.st_mode) && before.st_dev == after.st_dev && before.st_ino == after.st_ino &&
        before.st_size == after.st_size && before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
        before.st_mtim.tv_nsec == after.st_mtim.tv_nsec;
}

// Keep this order aligned with the SELECT in QueryPhoto.
enum PhotoColumnIndex {
    COL_ID, COL_MODIFIED, COL_SIZE, COL_POSITION, COL_SOURCE, COL_PENDING, COL_TEMPORARY, COL_DELETED,
    COL_DIRTY, COL_FLAGS, COL_OWNER, COL_HAS_ALBUM, COL_EXIF_ROTATE, COL_SUBTYPE, COL_EFFECT_MODE,
    COL_META_MODIFIED, COLUMN_COUNT
};

void AssignPhotoRecord(PhotoRecord &row, const int64_t (&values)[COLUMN_COUNT], const bool (&nulls)[COLUMN_COUNT])
{
    row.exists = true;
    row.id = static_cast<int32_t>(values[COL_ID]);
    row.modified = nulls[COL_MODIFIED] ? 0 : values[COL_MODIFIED];
    row.size = nulls[COL_SIZE] ? -1 : values[COL_SIZE];
    row.modifiedNull = nulls[COL_MODIFIED];
    row.metaModifiedNull = nulls[COL_META_MODIFIED];
    row.metaModified = values[COL_META_MODIFIED];
    row.sizeNull = nulls[COL_SIZE];
    row.position = static_cast<int32_t>(values[COL_POSITION]);
    row.source = static_cast<int32_t>(values[COL_SOURCE]);
    row.pending = values[COL_PENDING];
    row.temporary = static_cast<int32_t>(values[COL_TEMPORARY]);
    row.deleted = values[COL_DELETED];
    row.dirty = static_cast<int32_t>(values[COL_DIRTY]);
    row.flags = nulls[COL_FLAGS] ? -1 : static_cast<int32_t>(values[COL_FLAGS]);
    row.owner = nulls[COL_OWNER] ? 0 : static_cast<int32_t>(values[COL_OWNER]);
    row.hasAlbum = values[COL_HAS_ALBUM] != 0;
    row.exifRotate = nulls[COL_EXIF_ROTATE] ? 0 : static_cast<int32_t>(values[COL_EXIF_ROTATE]);
    row.subtype = nulls[COL_SUBTYPE] ? 0 : static_cast<int32_t>(values[COL_SUBTYPE]);
    row.effectMode = nulls[COL_EFFECT_MODE] ? 0 : static_cast<int32_t>(values[COL_EFFECT_MODE]);
}

int32_t ReadPhotoRow(const std::shared_ptr<NativeRdb::ResultSet> &result, PhotoRecord &row)
{
    int32_t ret = result->GoToFirstRow();
    int64_t values[COLUMN_COUNT] {};
    bool nulls[COLUMN_COUNT] {};
    for (int i = 0; ret == NativeRdb::E_OK && i < COLUMN_COUNT; ++i) {
        ret = result->IsColumnNull(i, nulls[i]);
        if (ret == NativeRdb::E_OK && !nulls[i]) {
            ret = result->GetLong(i, values[i]);
        }
    }
    if (ret == NativeRdb::E_OK &&
        (nulls[COL_ID] || nulls[COL_POSITION] || nulls[COL_SOURCE] || nulls[COL_PENDING] ||
        nulls[COL_TEMPORARY] || nulls[COL_DELETED] || nulls[COL_DIRTY])) {
        ret = E_INVALID_VALUES;
    }
    if (ret == NativeRdb::E_OK) {
        AssignPhotoRecord(row, values, nulls);
    }
    return ret;
}

int32_t QueryPhoto(const std::string &path, PhotoRecord &row)
{
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(store != nullptr, E_HAS_DB_ERROR);
    auto result = store->QuerySql("SELECT file_id,date_modified,size,position,file_source_type,time_pending,"
        "is_temp,date_deleted,dirty,metadata_flags,owner_album_id,"
        "EXISTS(SELECT 1 FROM PhotoAlbum WHERE album_id=Photos.owner_album_id),exif_rotate,subtype,"
        "moving_photo_effect_mode,meta_date_modified FROM Photos WHERE data=?",
        std::vector<std::string> {path});
    CHECK_AND_RETURN_RET(result != nullptr, E_HAS_DB_ERROR);
    int32_t count = 0;
    int32_t ret = result->GetRowCount(count);
    if (ret == NativeRdb::E_OK && count == 1) {
        ret = ReadPhotoRow(result, row);
    } else if (ret == NativeRdb::E_OK && (count < 0 || count > 1)) {
        ret = E_HAS_DB_ERROR;
    }
    result->Close();
    return ret;
}

bool CanProcessPhoto(const PhotoRecord &row)
{
    return row.exists && row.id > 0 && row.source == static_cast<int32_t>(FileSourceTypes::MEDIA) &&
        (row.position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
         row.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) &&
        row.pending == 0 && row.temporary == 0 &&
        row.deleted == 0 && row.dirty != static_cast<int32_t>(DirtyTypes::TYPE_DELETED);
}

bool IsMovingPhoto(const PhotoRecord &row)
{
    return row.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        row.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

int32_t GetMovingPhotoInfo(const std::string &image, const struct stat &imageInfo, int64_t &mtime, int64_t &size)
{
    CHECK_AND_RETURN_RET(GetFileMtime(imageInfo, mtime) && imageInfo.st_size >= 0, E_INVALID_VALUES);
    size = imageInfo.st_size;
    for (const auto &path : {MediaFileUtils::GetMovingPhotoVideoPath(image),
        MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(image)}) {
        CHECK_AND_RETURN_RET(!path.empty(), E_INVALID_PATH);
        struct stat component {};
        if (stat(path.c_str(), &component) != 0) {
            CHECK_AND_RETURN_RET(errno == ENOENT, E_FILE_OPER_FAIL);
            continue;
        }
        CHECK_AND_RETURN_RET(S_ISREG(component.st_mode) && component.st_size >= 0 &&
            size <= std::numeric_limits<int64_t>::max() - component.st_size, E_INVALID_VALUES);
        size += component.st_size;
        if (path == MediaFileUtils::GetMovingPhotoVideoPath(image)) {
            int64_t videoTime = 0;
            CHECK_AND_RETURN_RET(GetFileMtime(component, videoTime), E_INVALID_VALUES);
            mtime = std::max(mtime, videoTime);
        }
    }
    return E_OK;
}

class RecoveryScanCallback final : public IMediaScannerCallback {
public:
    int32_t OnScanFinished(int32_t status, const std::string &, const std::string &) override
    {
        called = true;
        result = status;
        return E_OK;
    }
    bool called = false;
    int32_t result = E_ERR;
};

bool SkipNonJsonEntry(fs::recursive_directory_iterator &entry)
{
    if (entry.depth() == 0) {
        return false;
    }
    if (IsRecoveryBucketMarker(entry->path().filename().string())) {
        entry.disable_recursion_pending();
        return true;
    }
    if (entry->path().extension() == ".json") {
        return false;
    }
    std::error_code error;
    return !entry->is_directory(error) && !error;
}

bool SkipCompletedBucket(fs::recursive_directory_iterator &entry, bool jsonPass, const struct stat &info,
    const DatabaseRecoveryReport *report)
{
    if (entry.depth() != 0 || !S_ISDIR(info.st_mode) || report == nullptr) {
        return false;
    }
    const auto &finished = jsonPass ? report->jsonBucketsDone : report->fileBucketsDone;
    if (finished.count(entry->path().filename().string()) == 0) {
        return false;
    }
    MEDIA_INFO_LOG("Recovery bucket already completed, phase=%{public}s bucket=%{public}s",
        jsonPass ? "json" : "file", GetLogBucketName(entry->path().string()).c_str());
    entry.disable_recursion_pending();
    return true;
}

} // namespace

class IncrementalRecovery {
public:
    explicit IncrementalRecovery(MediaLibraryMetaRecovery &owner) : owner_(owner) {}
    int32_t Run(const RecoveryWindow &window, DatabaseRecoveryReport &report);
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    int32_t RunTestPasses(const RecoveryWindow &window, const std::string &jsonRoot,
        const std::string &photoRoot, DatabaseRecoveryReport &report)
    {
        g_testScan = true;
        report_ = &report;
        ++report.scanAttempts;
        int32_t ret = LoadColumns();
        if (ret == E_OK) {
            ret = RunScanPasses(window, jsonRoot, photoRoot);
        }
        FinishScanAttempt(report, ret, 0);
        g_testScan = false;
        return ret;
    }
    int32_t RunTestAlbums(const std::string &path)
    {
        DatabaseRecoveryReport report;
        report_ = &report;
        return RestoreAlbums(path);
    }
    static int32_t RunTestTask(MediaLibraryMetaRecovery &owner, const std::string &directory,
        const RecoveryTestPaths &paths)
    {
        g_testScan = true;
        g_testPaths = &paths;
        int32_t ret = owner.RunIncrementalRecovery(directory);
        g_testPaths = nullptr;
        g_testScan = false;
        return ret;
    }
#endif

private:
    int32_t LoadColumns();
    int32_t PrepareScan(bool &canScan, const std::string &albumPath);
    int32_t ScanWindow(const RecoveryWindow &window, const std::string &albumPath,
        const std::string &jsonRoot, const std::string &photoRoot);
    int32_t RestoreAlbums(const std::string &path);
    int32_t RunScanPasses(const RecoveryWindow &window, const std::string &jsonRoot, const std::string &photoRoot);
    int32_t RefreshAlbums();
    int32_t ScanDirectory(const std::string &root, bool jsonPass, const RecoveryWindow &window);
    void FinishBucket(BucketScanState &bucket, bool complete = true);
    bool IndexJsonEntry(fs::recursive_directory_iterator &entry, const struct stat &info,
        const RecoveryWindow &window, BucketScanState &bucket);
    int32_t ProcessDirectoryEntry(fs::recursive_directory_iterator &entry, bool jsonPass,
        const RecoveryWindow &window, BucketScanState &bucket);
    int32_t ProcessJson(const std::string &path, const struct stat &info, const RecoveryWindow &window,
        bool allowOutsideForNew = false, bool *damaged = nullptr);
    int32_t ProcessFile(const std::string &path, const struct stat &info, const RecoveryWindow &window);
    int32_t ProcessNewFile(const std::string &path, const std::string &cloud, const struct stat &info,
        const RecoveryWindow &window, MediaType type);
    int32_t ApplyJson(const std::string &path, const std::string &cloudPath, const json &metadata,
        const PhotoRecord &row, const struct stat &jsonInfo, const struct stat &fileInfo);
    int32_t CollectJsonFields(const json &metadata, NativeRdb::ValuesBucket &values) const;
    int32_t AddJsonAlbum(const json &metadata, const PhotoRecord &row, NativeRdb::ValuesBucket &values,
        int32_t &fieldCount);
    void AddNewPhotoDefaults(const json &metadata, const std::string &cloudPath, NativeRdb::ValuesBucket &values) const;
    void SetJsonRecoveryFlags(const json &metadata, const PhotoRecord &row, NativeRdb::ValuesBucket &values) const;
    int32_t WriteJsonValues(const std::string &path, const std::string &cloudPath, const PhotoRecord &row,
        const struct stat &jsonInfo, const struct stat &fileInfo, NativeRdb::ValuesBucket &values);
    int32_t ScanOriginal(const std::string &path, const PhotoRecord &row, const struct stat &info);
    int32_t SetFallbackAlbum(const std::string &path, const PhotoRecord &row, const PhotoRecord &current);
    int32_t GetOtherAlbum(int32_t &id);
    bool ResolveMappedAlbum(const json &metadata, int32_t &id) const;
    int32_t ResolveAlbum(const json &metadata, const PhotoRecord &row, int32_t &id);
    bool Allowed() const { return owner_.IsIncrementalRecoveryAllowed(); }

    MediaLibraryMetaRecovery &owner_;
    std::unordered_map<std::string, std::string> columns_;
    std::set<std::string> ambiguousAlbumPaths_;
    int32_t otherAlbumId_ = 0;
    DatabaseRecoveryReport *report_ = nullptr;
    const RecoveryWindow *window_ = nullptr;
};

int32_t IncrementalRecovery::GetOtherAlbum(int32_t &id)
{
    if (otherAlbumId_ <= 0) {
        PhotoOwnerAlbumIdOperation operation;
        PhotoOwnerAlbumIdOperation::MediaData album {};
        album.lPath = operation.ParseSourcePathToLPath("");
        album.albumName = MediaFileUtils::GetFileName(album.lPath);
        operation.GetAlbumTypeAndSubType(album.lPath, album.albumType, album.albumSubType);
        album.uniqueId = MediaFileUtils::GenerateUUID();
        otherAlbumId_ = operation.CreateAlbumAndGetId(album);
    }
    id = otherAlbumId_;
    return id > 0 ? E_OK : E_HAS_DB_ERROR;
}

int32_t IncrementalRecovery::LoadColumns()
{
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(store != nullptr, E_HAS_DB_ERROR);
    auto result = store->QuerySql("SELECT name,type FROM pragma_table_info('Photos')", std::vector<std::string> {});
    CHECK_AND_RETURN_RET(result != nullptr, E_HAS_DB_ERROR);
    int32_t count = 0;
    int32_t ret = result->GetRowCount(count);
    for (int32_t i = 0; ret == NativeRdb::E_OK && i < count; ++i) {
        ret = i == 0 ? result->GoToFirstRow() : result->GoToNextRow();
        std::string name;
        std::string type;
        if (ret == NativeRdb::E_OK) {
            ret = result->GetString(0, name);
        }
        if (ret == NativeRdb::E_OK) {
            ret = result->GetString(1, type);
        }
        if (ret == NativeRdb::E_OK) {
            std::transform(type.begin(), type.end(), type.begin(),
                [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            columns_[name] = type;
        }
    }
    result->Close();
    if (ret == NativeRdb::E_SQLITE_CORRUPT || ret == NativeRdb::E_CANCEL) {
        return ret;
    }
    return ret == NativeRdb::E_OK && !columns_.empty() ? E_OK : E_HAS_DB_ERROR;
}

bool IncrementalRecovery::ResolveMappedAlbum(const json &metadata, int32_t &id) const
{
    auto owner = metadata.find(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    if (owner == metadata.end() || !owner->is_number_integer() ||
        (owner->is_number_unsigned() && owner->get<uint64_t>() >
        static_cast<uint64_t>(std::numeric_limits<int32_t>::max()))) {
        return false;
    }
    int64_t oldId = owner->get<int64_t>();
    if (oldId < 0 || oldId > std::numeric_limits<int32_t>::max()) {
        return false;
    }
    auto old = owner_.oldAlbumIdToLpath.find(static_cast<int32_t>(oldId));
    if (old == owner_.oldAlbumIdToLpath.end() || old->second.empty() || ambiguousAlbumPaths_.count(old->second) != 0) {
        return false;
    }
    auto current = owner_.lpathToNewAlbumId.find(old->second);
    if (current == owner_.lpathToNewAlbumId.end() || current->second <= 0) {
        return false;
    }
    id = current->second;
    return true;
}

int32_t IncrementalRecovery::ResolveAlbum(const json &metadata, const PhotoRecord &row, int32_t &id)
{
    if (ResolveMappedAlbum(metadata, id)) {
        return E_OK;
    }
    if (row.exists) {
        id = row.owner;
        return E_OK;
    }
    return GetOtherAlbum(id);
}

static bool IsDeletedRecoveryJson(const json &metadata)
{
    auto deleted = metadata.find(MediaColumn::MEDIA_DATE_DELETED);
    auto dirty = metadata.find(PhotoColumn::PHOTO_DIRTY);
    return (deleted != metadata.end() && deleted->is_number_integer() && *deleted > 0) ||
        (dirty != metadata.end() && dirty->is_number_integer() &&
        *dirty == static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
}

int32_t IncrementalRecovery::CollectJsonFields(const json &metadata, NativeRdb::ValuesBucket &values) const
{
    int32_t fieldCount = 0;
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        const std::string &name = it.key();
        if (name == MediaColumn::MEDIA_ID || name == MediaColumn::MEDIA_FILE_PATH ||
            name == MediaColumn::MEDIA_VIRTUAL_PATH || name == PhotoColumn::PHOTO_THUMBNAIL_READY ||
            name == PhotoColumn::PHOTO_METADATA_FLAGS || name == PhotoColumn::PHOTO_OWNER_ALBUM_ID) {
            continue;
        }
        auto column = columns_.find(name);
        if (column != columns_.end() && PutRecoveryJsonValue(values, name, it.value(), column->second)) {
            ++fieldCount;
        }
    }
    return fieldCount;
}

int32_t IncrementalRecovery::AddJsonAlbum(const json &metadata, const PhotoRecord &row,
    NativeRdb::ValuesBucket &values, int32_t &fieldCount)
{
    if (row.exists && !metadata.contains(PhotoColumn::PHOTO_OWNER_ALBUM_ID)) {
        return E_OK;
    }
    int32_t ownerId = 0;
    int32_t ret = ResolveAlbum(metadata, row, ownerId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (ownerId > 0 && (!row.exists || ownerId != row.owner)) {
        values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, ownerId);
        ++fieldCount;
    } else if (!row.exists) {
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void IncrementalRecovery::AddNewPhotoDefaults(const json &metadata, const std::string &cloudPath,
    NativeRdb::ValuesBucket &values) const
{
    values.PutString(MediaColumn::MEDIA_FILE_PATH, cloudPath);
    if (!values.HasColumn(MediaColumn::MEDIA_TYPE)) {
        values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaFileUtils::GetMediaType(cloudPath)));
    }
    auto displayName = metadata.find(MediaColumn::MEDIA_NAME);
    std::string name = displayName != metadata.end() && displayName->is_string() ?
        displayName->get<std::string>() : MediaFileUtils::GetFileName(cloudPath);
    if (!values.HasColumn(MediaColumn::MEDIA_NAME)) {
        values.PutString(MediaColumn::MEDIA_NAME, name);
    }
    if (!values.HasColumn(MediaColumn::MEDIA_TITLE)) {
        values.PutString(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(name));
    }
}

void IncrementalRecovery::SetJsonRecoveryFlags(const json &metadata, const PhotoRecord &row,
    NativeRdb::ValuesBucket &values) const
{
    if (!row.exists || (row.flags != static_cast<int32_t>(MetadataFlags::TYPE_NEW) &&
        row.flags != static_cast<int32_t>(MetadataFlags::TYPE_DIRTY))) {
        values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int32_t>(MetadataFlags::TYPE_DIRTY));
    }
    auto rotate = metadata.find(PhotoColumn::PHOTO_EXIF_ROTATE);
    if (row.exists && rotate != metadata.end() && rotate->is_number_integer() && *rotate != row.exifRotate) {
        values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY,
            static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY));
        values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(LcdReady::GENERATE_LCD_LATER));
    }
}

int32_t IncrementalRecovery::WriteJsonValues(const std::string &path, const std::string &cloudPath,
    const PhotoRecord &row, const struct stat &jsonInfo, const struct stat &fileInfo, NativeRdb::ValuesBucket &values)
{
    struct stat currentJson {};
    struct stat currentFile {};
    const std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    CHECK_AND_RETURN_RET(Allowed(), NativeRdb::E_CANCEL);
    CHECK_AND_RETURN_RET(lstat(path.c_str(), &currentJson) == 0 && SameFile(jsonInfo, currentJson) &&
        lstat(local.c_str(), &currentFile) == 0 && SameFile(fileInfo, currentFile), E_FILE_OPER_FAIL);
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t ret = E_OK;
    if (row.exists) {
        std::string where = "file_id=? AND data=?";
        std::vector<std::string> args {std::to_string(row.id), cloudPath};
        where += row.modifiedNull ? " AND date_modified IS NULL" : " AND date_modified=?";
        if (!row.modifiedNull) {
            args.push_back(std::to_string(row.modified));
        }
        where += row.metaModifiedNull ? " AND meta_date_modified IS NULL" : " AND meta_date_modified=?";
        if (!row.metaModifiedNull) {
            args.push_back(std::to_string(row.metaModified));
        }
        where += row.sizeNull ? " AND size IS NULL" : " AND size=?";
        if (!row.sizeNull) {
            args.push_back(std::to_string(row.size));
        }
        int32_t changed = 0;
        NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.SetWhereClause(where);
        predicates.SetWhereArgs(args);
        ret = refresh.Update(changed, values, predicates);
        CHECK_AND_RETURN_RET(ret == E_OK && changed == 1, ret == E_OK ? E_HAS_DB_ERROR : ret);
        IncrementCount(report_->jsonUpdated);
    } else {
        values.PutString(MediaColumn::MEDIA_FILE_PATH, cloudPath);
        int64_t id = 0;
        ret = refresh.Insert(id, PhotoColumn::PHOTOS_TABLE, values);
        CHECK_AND_RETURN_RET(ret == E_OK && id > 0, ret == E_OK ? E_HAS_DB_ERROR : ret);
        IncrementCount(report_->jsonInserted);
    }
    return refresh.Notify();
}

int32_t IncrementalRecovery::ApplyJson(const std::string &path, const std::string &cloudPath, const json &metadata,
    const PhotoRecord &row, const struct stat &jsonInfo, const struct stat &fileInfo)
{
    if (IsDeletedRecoveryJson(metadata)) {
        return E_OK;
    }
    NativeRdb::ValuesBucket values;
    int32_t fieldCount = CollectJsonFields(metadata, values);
    int32_t ret = AddJsonAlbum(metadata, row, values, fieldCount);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (row.exists && fieldCount == 0) {
        return E_OK;
    }
    if (!row.exists) {
        AddNewPhotoDefaults(metadata, cloudPath, values);
    }
    SetJsonRecoveryFlags(metadata, row, values);
    return WriteJsonValues(path, cloudPath, row, jsonInfo, fileInfo, values);
}

int32_t IncrementalRecovery::ProcessJson(const std::string &path, const struct stat &info,
    const RecoveryWindow &window, bool allowOutsideForNew, bool *damaged)
{
    int64_t mtime = 0;
    CHECK_AND_RETURN_RET(GetFileMtime(info, mtime), E_INVALID_VALUES);
    if (!allowOutsideForNew && !IsInRecoveryWindow(mtime, window)) {
        return E_OK;
    }
    json metadata;
    const auto jsonStatus = ReadRecoveryJsonFile(path, metadata);
    if (damaged != nullptr) {
        *damaged = jsonStatus == RecoveryJsonStatus::INVALID;
    }
    CHECK_AND_RETURN_RET(jsonStatus == RecoveryJsonStatus::OK, E_FILE_OPER_FAIL);
    auto data = metadata.find(MediaColumn::MEDIA_FILE_PATH);
    CHECK_AND_RETURN_RET(data != metadata.end() && data->is_string(), E_INVALID_PATH);
    const std::string cloudPath = data->get<std::string>();
    const std::string photoRoot = ROOT_MEDIA_DIR + "Photo/";
    CHECK_AND_RETURN_RET(MediaStringUtils::StartsWith(cloudPath, photoRoot) &&
        fs::path(cloudPath).lexically_normal().string() == cloudPath, E_INVALID_PATH);
    const MediaType type = MediaFileUtils::GetMediaType(cloudPath);
    CHECK_AND_RETURN_RET(type == MEDIA_TYPE_IMAGE || type == MEDIA_TYPE_VIDEO, E_INVALID_PATH);
    PhotoRecord row;
    int32_t ret = QueryPhoto(cloudPath, row);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (row.exists && (!CanProcessPhoto(row) || allowOutsideForNew ||
        !ShouldApplyRecoveryJson(mtime, row.modified, window))) {
        return E_OK;
    }
    std::string expectedCloud;
    CHECK_AND_RETURN_RET(PhotoFileUtils::GetMetaPathFromOrignalPath(cloudPath, expectedCloud) == E_OK, E_INVALID_PATH);
    const std::string expectedLocal = MediaPathUtils::ConvertCloudPathToLocalPath(expectedCloud);
    CHECK_AND_RETURN_RET(!expectedLocal.empty() &&
        fs::path(expectedLocal).lexically_normal() == fs::path(path).lexically_normal(), E_INVALID_PATH);
    const std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(cloudPath);
    CHECK_AND_RETURN_RET(!local.empty(), E_INVALID_PATH);
    struct stat fileInfo {};
    if (lstat(local.c_str(), &fileInfo) != 0) {
        if (errno == ENOENT) {
            return E_OK;
        }
        return E_FILE_OPER_FAIL;
    }
    if (!S_ISREG(fileInfo.st_mode)) {
        return E_OK;
    }
    return ApplyJson(path, cloudPath, metadata, row, info, fileInfo);
}

int32_t IncrementalRecovery::SetFallbackAlbum(const std::string &path, const PhotoRecord &row,
    const PhotoRecord &current)
{
    if (row.exists || current.hasAlbum) {
        return E_OK;
    }
    int32_t other = 0;
    int32_t ret = GetOtherAlbum(other);
    CHECK_AND_RETURN_RET(ret == E_OK && Allowed(), ret == E_OK ? NativeRdb::E_CANCEL : ret);
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, other);
    values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int32_t>(MetadataFlags::TYPE_NEW));
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t changed = 0;
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.SetWhereClause(
        "file_id=? AND data=? AND NOT EXISTS(SELECT 1 FROM PhotoAlbum WHERE album_id=Photos.owner_album_id)");
    predicates.SetWhereArgs({std::to_string(current.id), path});
    ret = refresh.Update(changed, values, predicates);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(refresh.Notify() == E_OK, E_ERR);
    return E_OK;
}

int32_t IncrementalRecovery::ScanOriginal(const std::string &path, const PhotoRecord &row, const struct stat &info)
{
    CHECK_AND_RETURN_RET(Allowed(), NativeRdb::E_CANCEL);
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    auto callback = std::make_shared<RecoveryScanCallback>();
    int32_t ret = ScanRecoveryFileForTest(path, callback, row.id);
#else
    auto scanner = MediaScannerManager::GetInstance();
    CHECK_AND_RETURN_RET(scanner != nullptr, E_ERR);
    auto callback = std::make_shared<RecoveryScanCallback>();
    int32_t ret = scanner->ScanFileSync(path, callback, MediaLibraryApi::API_10, true, row.id);
#endif
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (callback->called && callback->result != E_OK && callback->result != E_SCANNED) {
        return callback->result;
    }
    CHECK_AND_RETURN_RET(Allowed(), NativeRdb::E_CANCEL);
    PhotoRecord current;
    ret = QueryPhoto(path, current);
    CHECK_AND_RETURN_RET(ret == E_OK && current.exists && (!row.exists || current.id == row.id),
        ret == E_OK ? E_HAS_DB_ERROR : ret);
    struct stat after {};
    const std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(path);
    CHECK_AND_RETURN_RET(Allowed(), NativeRdb::E_CANCEL);
    CHECK_AND_RETURN_RET(!local.empty() && lstat(local.c_str(), &after) == 0 &&
        SameFile(info, after), E_FILE_OPER_FAIL);
    if (!callback->called) {
        int64_t mtime = 0;
        int64_t size = info.st_size;
        if (IsMovingPhoto(current)) {
            CHECK_AND_RETURN_RET(GetMovingPhotoInfo(local, info, mtime, size) == E_OK, E_FILE_OPER_FAIL);
        } else {
            CHECK_AND_RETURN_RET(GetFileMtime(info, mtime), E_INVALID_VALUES);
        }
        CHECK_AND_RETURN_RET(current.size == size &&
            current.modified / MILLISECONDS_PER_SECOND == mtime / MILLISECONDS_PER_SECOND, E_ERR);
    }
    ret = SetFallbackAlbum(path, row, current);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    IncrementCount(report_->fileScanned);
    if (!row.exists) {
        IncrementCount(report_->fileInserted);
    }
    return E_OK;
}

// A moving photo video is not a separate Photos asset. Check its image side before importing a new video.
bool HasPossibleMovingImage(const std::string &localPath)
{
    if (!MediaFileUtils::CheckMovingPhotoVideoExtension(MediaFileUtils::GetExtensionFromPath(localPath))) {
        return false;
    }
    const auto dot = localPath.find_last_of('.');
    if (dot == std::string::npos) {
        return false;
    }
    const std::string stem = localPath.substr(0, dot);
    for (const auto &extension : {".jpg", ".jpeg", ".heic", ".heif", ".JPG", ".JPEG", ".HEIC", ".HEIF"}) {
        const std::string image = stem + extension;
        struct stat info {};
        if (MediaFileUtils::GetMovingPhotoVideoPath(image) == localPath &&
            lstat(image.c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
            return true;
        }
    }
    return false;
}

static bool IsOriginalCandidateInWindow(const std::string &path, MediaType type, int64_t mtime,
    const RecoveryWindow &window)
{
    if (IsInRecoveryWindow(mtime, window)) {
        return true;
    }
    struct stat video {};
    int64_t videoTime = 0;
    const std::string videoPath = type == MEDIA_TYPE_IMAGE ? MediaFileUtils::GetMovingPhotoVideoPath(path) : "";
    return !videoPath.empty() && lstat(videoPath.c_str(), &video) == 0 && S_ISREG(video.st_mode) &&
        GetFileMtime(video, videoTime) && IsInRecoveryWindow(videoTime, window);
}

int32_t IncrementalRecovery::ProcessFile(const std::string &path, const struct stat &info,
    const RecoveryWindow &window)
{
    int64_t mtime = 0;
    CHECK_AND_RETURN_RET(GetFileMtime(info, mtime), E_INVALID_VALUES);
    const std::string localRoot = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR);
    CHECK_AND_RETURN_RET(!localRoot.empty() && MediaStringUtils::StartsWith(path, localRoot), E_INVALID_PATH);
    const std::string cloud = ROOT_MEDIA_DIR + path.substr(localRoot.size());
    const MediaType type = MediaFileUtils::GetMediaType(cloud);
    if (type != MEDIA_TYPE_IMAGE && type != MEDIA_TYPE_VIDEO) {
        return E_OK;
    }
    // A moving photo's video can change after its image. Avoid querying the database for
    // an out-of-window image unless its potential video is also in the window.
    if (!IsOriginalCandidateInWindow(path, type, mtime, window)) {
        return E_OK;
    }
    PhotoRecord row;
    int32_t ret = QueryPhoto(cloud, row);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (row.exists) {
        int64_t size = info.st_size;
        if (IsMovingPhoto(row)) {
            CHECK_AND_RETURN_RET(GetMovingPhotoInfo(path, info, mtime, size) == E_OK, E_FILE_OPER_FAIL);
        }
        if (!CanProcessPhoto(row) || !ShouldScanRecoveryFile(mtime, size, row.modified, row.size, window)) {
            return E_OK;
        }
        return ScanOriginal(cloud, row, info);
    }
    if (!IsInRecoveryWindow(mtime, window)) {
        return E_OK;
    }
    return ProcessNewFile(path, cloud, info, window, type);
}

int32_t IncrementalRecovery::ProcessNewFile(const std::string &path, const std::string &cloud,
    const struct stat &info, const RecoveryWindow &window, MediaType type)
{
    std::string jsonCloud;
    int32_t ret = PhotoFileUtils::GetMetaPathFromOrignalPath(cloud, jsonCloud);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    const std::string jsonPath = MediaPathUtils::ConvertCloudPathToLocalPath(jsonCloud);
    CHECK_AND_RETURN_RET(!jsonPath.empty(), E_INVALID_PATH);
    struct stat jsonInfo {};
    if (lstat(jsonPath.c_str(), &jsonInfo) == 0) {
        CHECK_AND_RETURN_RET(S_ISREG(jsonInfo.st_mode), E_FILE_OPER_FAIL);
        bool damaged = false;
        ret = ProcessJson(jsonPath, jsonInfo, window, true, &damaged);
        if (ret == E_OK || ret == NativeRdb::E_CANCEL || ret == NativeRdb::E_SQLITE_CORRUPT) {
            return ret;
        }
        struct stat currentJson {};
        if (!damaged || lstat(jsonPath.c_str(), &currentJson) != 0 ||
            !SameFile(jsonInfo, currentJson)) {
            return ret;
        }
        RecordEntryFailure(*report_, true, ret, jsonPath);
        if (type == MEDIA_TYPE_VIDEO && HasPossibleMovingImage(path)) {
            MEDIA_DEBUG_LOG("Skip possible moving-photo companion: %{public}s", DfxUtils::GetSafePath(path).c_str());
            return E_OK;
        }
        return ScanOriginal(cloud, PhotoRecord {}, info);
    }
    CHECK_AND_RETURN_RET(errno == ENOENT, E_FILE_OPER_FAIL);
    if (type == MEDIA_TYPE_VIDEO && HasPossibleMovingImage(path)) {
        MEDIA_DEBUG_LOG("Skip possible moving-photo companion: %{public}s", DfxUtils::GetSafePath(path).c_str());
        return E_OK;
    }
    return ScanOriginal(cloud, PhotoRecord {}, info);
}

void IncrementalRecovery::FinishBucket(BucketScanState &bucket, bool complete)
{
    const std::string name = fs::path(bucket.directory).filename().string();
    const bool jsonPass = bucket.jsonPass;
    bucket.Finish(complete);
    if (!complete || name.empty() || report_ == nullptr || window_ == nullptr || !Allowed()) {
        return;
    }
    auto &finished = jsonPass ? report_->jsonBucketsDone : report_->fileBucketsDone;
    finished.insert(name);
    int error = RunForCurrentRecoveryWindow(*window_, [this]() {
        return Allowed() ? SaveRecoverySummary(window_->path, *report_) : ECANCELED;
    });
    CHECK_AND_PRINT_LOG(error == 0, "Cannot save recovery bucket progress, errno=%{public}d", error);
}

bool IncrementalRecovery::IndexJsonEntry(fs::recursive_directory_iterator &entry, const struct stat &info,
    const RecoveryWindow &window, BucketScanState &bucket)
{
    if (entry.depth() == 0) {
        if (!S_ISDIR(info.st_mode)) {
            return false;
        }
        const std::string path = entry->path().string();
        IncrementCount(report_->bucketsChecked);
        if (CanSkipRecoveryBucket(path, window.startMs)) {
            IncrementCount(report_->bucketsSkipped);
            MEDIA_INFO_LOG("Recovery JSON bucket skipped by index, bucket=%{public}s", GetLogBucketName(path).c_str());
            bucket.Start(path, true);
            FinishBucket(bucket);
            entry.disable_recursion_pending();
            return true;
        }
        bucket.Start(path, true);
    }
    return false;
}

int32_t IncrementalRecovery::ProcessDirectoryEntry(fs::recursive_directory_iterator &entry, bool jsonPass,
    const RecoveryWindow &window, BucketScanState &bucket)
{
    const std::string path = entry->path().string();
    if (jsonPass && SkipNonJsonEntry(entry)) {
        return E_OK;
    }
    if (entry.depth() == 0) {
        FinishBucket(bucket);
    }
    struct stat info {};
    if (lstat(path.c_str(), &info) != 0) {
        RecordEntryFailure(*report_, jsonPass, errno, path);
        entry.disable_recursion_pending();
        return E_OK;
    }
    if (SkipCompletedBucket(entry, jsonPass, info, report_)) {
        return E_OK;
    }
    if (jsonPass && IndexJsonEntry(entry, info, window, bucket)) {
        return E_OK;
    }
    if (!jsonPass && entry.depth() == 0 && S_ISDIR(info.st_mode)) {
        bucket.Start(path, false);
    }
    if (!S_ISREG(info.st_mode) || (jsonPass && entry->path().extension() != ".json")) {
        return E_OK;
    }
    int32_t ret = jsonPass ? ProcessJson(path, info, window) : ProcessFile(path, info, window);
    if (ret == NativeRdb::E_CANCEL) {
        return ret;
    }
    if (ret != E_OK) {
        RecordEntryFailure(*report_, jsonPass, ret, path);
        if (ret == NativeRdb::E_SQLITE_CORRUPT) {
            return ret;
        }
    }
    return E_OK;
}

int32_t IncrementalRecovery::ScanDirectory(const std::string &root, bool jsonPass, const RecoveryWindow &window)
{
    struct stat rootInfo {};
    if (lstat(root.c_str(), &rootInfo) != 0) {
        if (!jsonPass || errno != ENOENT) {
            RecordEntryFailure(*report_, jsonPass, errno, root);
            return E_FILE_OPER_FAIL;
        }
        return E_OK;
    }
    if (!S_ISDIR(rootInfo.st_mode)) {
        RecordEntryFailure(*report_, jsonPass, ENOTDIR, root);
        return E_FILE_OPER_FAIL;
    }
    std::error_code error;
    fs::recursive_directory_iterator entry(root, error);
    if (error) {
        RecordEntryFailure(*report_, jsonPass, error.value(), root);
        return E_FILE_OPER_FAIL;
    }
    BucketScanState bucket;
    for (; entry != fs::recursive_directory_iterator(); entry.increment(error)) {
        if (!Allowed()) {
            FinishBucket(bucket, false);
            return NativeRdb::E_CANCEL;
        }
        if (error) {
            FinishBucket(bucket, false);
            RecordEntryFailure(*report_, jsonPass, error.value(), root);
            return E_FILE_OPER_FAIL;
        }
        int32_t ret = ProcessDirectoryEntry(entry, jsonPass, window, bucket);
        if (ret != E_OK) {
            FinishBucket(bucket, false);
            return ret;
        }
    }
    FinishBucket(bucket, !error);
    if (error) {
        RecordEntryFailure(*report_, jsonPass, error.value(), root);
        return E_FILE_OPER_FAIL;
    }
    return E_OK;
}

int32_t IncrementalRecovery::RestoreAlbums(const std::string &albumPath)
{
    if (access(albumPath.c_str(), F_OK) != 0) {
        return E_FILE_OPER_FAIL;
    }
    int32_t ret = owner_.AlbumRecovery(albumPath);
    if (ret == E_OK) {
        ret = owner_.LoadAlbumMaps(albumPath, &ambiguousAlbumPaths_);
    }
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (!ambiguousAlbumPaths_.empty()) {
        IncrementCount(report_->failed);
    }
    for (const auto &[oldId, lpath] : owner_.oldAlbumIdToLpath) {
        if (!lpath.empty() && owner_.lpathToNewAlbumId.count(lpath) == 0) {
            MEDIA_ERR_LOG("Recovered album has no current mapping, oldId=%{public}d", oldId);
            return E_HAS_DB_ERROR;
        }
    }
    return E_OK;
}

int32_t IncrementalRecovery::RunScanPasses(const RecoveryWindow &window,
    const std::string &jsonRoot, const std::string &photoRoot)
{
    MEDIA_INFO_LOG("Incremental recovery JSON scan started");
    int32_t jsonResult = ScanDirectory(jsonRoot, true, window);
    if (jsonResult == NativeRdb::E_SQLITE_CORRUPT || jsonResult == NativeRdb::E_CANCEL || !Allowed()) {
        return jsonResult == E_OK ? NativeRdb::E_CANCEL : jsonResult;
    }
    MEDIA_INFO_LOG("Incremental recovery file scan started, jsonResult=%{public}d", jsonResult);
    int32_t fileResult = ScanDirectory(photoRoot, false, window);
    return fileResult == E_OK ? jsonResult : fileResult;
}

int32_t IncrementalRecovery::RefreshAlbums()
{
    CHECK_AND_RETURN_RET(Allowed(), NativeRdb::E_CANCEL);
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    if (g_testPaths != nullptr) {
        return E_OK;
    }
#endif
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(store != nullptr, E_HAS_DB_ERROR);
    MediaLibraryRdbUtils::UpdateAllAlbums(store);
    MEDIA_INFO_LOG("Album refresh requested; analysis albums may continue asynchronously");
    return E_OK;
}

int32_t IncrementalRecovery::PrepareScan(bool &canScan, const std::string &albumPath)
{
    int32_t ret = LoadColumns();
    canScan = ret == E_OK;
    if (!canScan && ret != NativeRdb::E_SQLITE_CORRUPT && ret != NativeRdb::E_CANCEL) {
        MEDIA_ERR_LOG("Incremental recovery cannot load database columns, error=%{public}d", ret);
        IncrementCount(report_->failed);
        report_->lastScanError = ret;
    }
    if (canScan) {
        int32_t albumError = RestoreAlbums(albumPath);
        if (albumError == NativeRdb::E_SQLITE_CORRUPT || albumError == NativeRdb::E_CANCEL) {
            ret = albumError;
        } else if (albumError != E_OK) {
            MEDIA_ERR_LOG("Incremental recovery cannot restore albums, error=%{public}d", albumError);
            owner_.oldAlbumIdToLpath.clear();
            owner_.lpathToNewAlbumId.clear();
            ambiguousAlbumPaths_.clear();
            IncrementCount(report_->failed);
            report_->lastScanError = albumError;
        }
    }
    return ret;
}

int32_t IncrementalRecovery::ScanWindow(const RecoveryWindow &window, const std::string &albumPath,
    const std::string &jsonRoot, const std::string &photoRoot)
{
    bool canScan = false;
    int32_t ret = PrepareScan(canScan, albumPath);
    if (ret == E_OK && canScan && (jsonRoot.empty() || photoRoot.empty())) {
        MEDIA_ERR_LOG("Incremental recovery cannot resolve photo or JSON root");
        IncrementCount(report_->failed);
        report_->lastScanError = E_FILE_OPER_FAIL;
        ret = E_FILE_OPER_FAIL;
    } else if (ret == E_OK && canScan) {
        ret = RunScanPasses(window, jsonRoot, photoRoot);
    }
    if (canScan && ret != NativeRdb::E_SQLITE_CORRUPT && ret != NativeRdb::E_CANCEL) {
        int32_t albumError = RefreshAlbums();
        if (albumError == NativeRdb::E_SQLITE_CORRUPT || albumError == NativeRdb::E_CANCEL) {
            return albumError;
        }
        if (albumError != E_OK) {
            MEDIA_ERR_LOG("Incremental recovery cannot refresh albums, error=%{public}d", albumError);
            IncrementCount(report_->failed);
            report_->lastScanError = albumError;
        }
    }
    return ret;
}

int32_t IncrementalRecovery::Run(const RecoveryWindow &window, DatabaseRecoveryReport &report)
{
    window_ = &window;
    report_ = &report;
    const auto started = std::chrono::steady_clock::now();
    report.stage = DatabaseRecoveryStage::BACKGROUND_SCAN;
    report.result = DatabaseRecoveryResult::WAITING;
    if (report.scanAttempts < std::numeric_limits<int32_t>::max()) {
        ++report.scanAttempts;
    }
    int saveError = RunForCurrentRecoveryWindow(window, [this, &window, &report]() {
        return Allowed() ? SaveRecoverySummary(window.path, report) : ECANCELED;
    });
    if (saveError == ESTALE || saveError == ENOENT || saveError == ECANCELED) {
        return NativeRdb::E_CANCEL;
    }
    CHECK_AND_PRINT_LOG(saveError == 0, "Could not save background recovery start, errno=%{public}d", saveError);
    std::string albumPath = ROOT_MEDIA_DIR + ".meta/album.json";
    std::string jsonRoot = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR + ".meta/Photo");
    std::string photoRoot = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR + "Photo");
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    if (g_testPaths != nullptr) {
        albumPath = g_testPaths->album;
        jsonRoot = g_testPaths->json;
        photoRoot = g_testPaths->photo;
    }
#endif
    int32_t ret = ScanWindow(window, albumPath, jsonRoot, photoRoot);
    owner_.oldAlbumIdToLpath.clear();
    owner_.lpathToNewAlbumId.clear();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    FinishScanAttempt(report, ret, elapsed);
    saveError = RunForCurrentRecoveryWindow(window, [this, &window, &report]() {
        return !owner_.interruptedByDatabase_.load() && !MediaLibraryRestore::GetInstance().IsRestoring() ?
            SaveRecoverySummary(window.path, report) : ECANCELED;
    });
    if (saveError != 0 && saveError != ESTALE && saveError != ECANCELED) {
        MEDIA_ERR_LOG("Could not save background recovery statistics, errno=%{public}d", saveError);
    }
    return ret;
}

#ifdef MEDIALIB_RECOVERY_UNIT_TEST
int32_t RunIncrementalRecoveryTestPasses(MediaLibraryMetaRecovery &owner, const RecoveryWindow &window,
    const std::string &jsonRoot, const std::string &photoRoot, DatabaseRecoveryReport &report)
{
    IncrementalRecovery recovery(owner);
    return recovery.RunTestPasses(window, jsonRoot, photoRoot, report);
}

int32_t RunIncrementalRecoveryTestAlbums(MediaLibraryMetaRecovery &owner, const std::string &path)
{
    IncrementalRecovery recovery(owner);
    return recovery.RunTestAlbums(path);
}

int32_t RunIncrementalRecoveryTestTask(MediaLibraryMetaRecovery &owner, const std::string &directory,
    const std::string &album, const std::string &jsonRoot, const std::string &photoRoot, int32_t &reports)
{
    return IncrementalRecovery::RunTestTask(owner, directory, {album, jsonRoot, photoRoot, &reports});
}
#endif

bool MediaLibraryMetaRecovery::IsIncrementalRecoveryAllowed() const
{
#ifdef MEDIALIB_RECOVERY_UNIT_TEST
    if (g_testScan) {
        return !cancelIncremental_.load();
    }
#endif
    return recoveryState_.load() == MediaLibraryMetaRecoveryState::STATE_INCREMENTAL_RECOVERING &&
        !cancelIncremental_.load() && !historicalRecoveryPending_.load() &&
        !MediaLibraryRestore::GetInstance().IsRestoring() && MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaLibraryMetaRecovery::StartIncrementalRecovery()
{
    if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
        return;
    }
    int result = RunWhileDatabaseAvailable(MediaLibraryRestore::GetInstance().isRestoring_, [this]() {
        auto expected = MediaLibraryMetaRecoveryState::STATE_NONE;
        if (!recoveryState_.compare_exchange_strong(
            expected, MediaLibraryMetaRecoveryState::STATE_INCREMENTAL_RECOVERING)) {
            return EBUSY;
        }
        interruptedByDatabase_ = false;
        cancelIncremental_ = false;
        return 0;
    });
    if (result != 0) {
        return;
    }
    std::thread([this]() {
        FinishIncrementalRecovery(RunIncrementalRecovery(RECOVERY_WINDOW_DIRECTORY));
    }).detach();
}

void MediaLibraryMetaRecovery::FinishIncrementalRecovery(int32_t ret)
{
    if (ret == NativeRdb::E_SQLITE_CORRUPT) {
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
    }
    if (ret == NativeRdb::E_CANCEL) {
        MEDIA_WARN_LOG("Incremental recovery paused: cancel=%{public}d historical=%{public}d "
            "restoring=%{public}d backgroundReady=%{public}d",
            cancelIncremental_.load(), historicalRecoveryPending_.load(),
            MediaLibraryRestore::GetInstance().IsRestoring(), MedialibrarySubscriber::IsCurrentStatusOn());
    } else if (ret != E_OK) {
        MEDIA_ERR_LOG("Incremental recovery worker failed, error=%{public}d", ret);
    } else {
        MEDIA_INFO_LOG("Incremental recovery worker ended, result=%{public}d", ret);
    }
    auto current = MediaLibraryMetaRecoveryState::STATE_INCREMENTAL_RECOVERING;
    (void)recoveryState_.compare_exchange_strong(current, MediaLibraryMetaRecoveryState::STATE_NONE);
    if (historicalRecoveryPending_.exchange(false)) {
        StartAsyncRecovery();
        return;
    }
    if (!MedialibrarySubscriber::IsCurrentStatusOn() || MediaLibraryRestore::GetInstance().IsRestoring()) {
        return;
    }
    bool noPending = !HasPendingRecoveryWindow(RECOVERY_WINDOW_DIRECTORY);
    bool waiting = ret == NativeRdb::E_CANCEL && !cancelIncremental_.load();
    if (ret == NativeRdb::E_CANCEL && cancelIncremental_.load() && !noPending) {
        CheckRecoveryState();
        return;
    }
    if (ret != E_OK && !waiting && !noPending) {
        return;
    }
    if (ret == E_OK || noPending) {
        CheckRecoveryState();
    }
    MedialibrarySubscriber::ResumeBackgroundOperation();
}

static int32_t CompleteRecoveryWindow(const RecoveryWindow &window, DatabaseRecoveryReport &report,
    const std::function<bool()> &allowed)
{
    int summaryError = 0;
    int error = RunForCurrentRecoveryWindow(window, [&window, &allowed, &report, &summaryError]() {
        if (!allowed()) {
            return ECANCELED;
        }
        summaryError = SaveRecoverySummary(window.path, report);
        return RemoveRecoveryWindowFile(window.path);
    });
    if (error != 0) {
        MEDIA_ERR_LOG("Cannot remove completed recovery window, errno=%{public}d", error);
        if (error == ESTALE || error == ENOENT || error == ECANCELED ||
            RunForCurrentRecoveryWindow(window, [&window, &allowed]() {
                return allowed() ? IgnoreCompletedRecoveryWindow(window) : ECANCELED;
            }) != 0) {
            return NativeRdb::E_CANCEL;
        }
        return E_OK;
    }
    LogRecoveryCompletion(window, report);
    if (summaryError != 0) {
        MEDIA_ERR_LOG("Could not save final recovery result; skip summary report");
    } else {
        ReportRecoveryWindow(report);
    }
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::RunIncrementalRecovery(const std::string &directory)
{
    std::vector<RecoveryWindow> windows;
    int error = ListRecoveryWindows(directory, windows);
    CHECK_AND_RETURN_RET_LOG(error == 0, E_FILE_OPER_FAIL, "Cannot list recovery windows, errno=%{public}d", error);
    for (const auto &window : windows) {
        CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), NativeRdb::E_CANCEL);
        DatabaseRecoveryReport report;
        int summaryError = LoadRecoverySummary(window.path, report);
        if (summaryError != 0) {
            MEDIA_WARN_LOG("Recovery summary unavailable, errno=%{public}d", summaryError);
            report.windowStart = window.startMs;
            report.windowEnd = window.endMs;
            report.decision = static_cast<int32_t>(ReplicaRecoveryDecision::ALLOW);
        }
        // A known failed Force remains pending until a later recovery makes the database usable.
        // An unknown result after interruption follows the normal scan if the database is available.
        if (summaryError == 0 && IsWaitingForDatabaseRecovery(report)) {
            MEDIA_WARN_LOG("Incremental recovery waits for a subsequent successful database recovery");
            return NativeRdb::E_CANCEL;
        }
        MEDIA_INFO_LOG("Incremental recovery started, windowStart=%{public}lld windowEnd=%{public}lld",
            static_cast<long long>(window.startMs), static_cast<long long>(window.endMs));
        IncrementalRecovery recovery(*this);
        int32_t ret = recovery.Run(window, report);
        CHECK_AND_RETURN_RET(ret != NativeRdb::E_CANCEL && ret != NativeRdb::E_SQLITE_CORRUPT &&
            IsIncrementalRecoveryAllowed(), ret == E_OK ? NativeRdb::E_CANCEL : ret);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("Incremental recovery cannot continue; close failed task, error=%{public}d", ret);
        }
        int32_t finish = CompleteRecoveryWindow(window, report, [this]() { return IsIncrementalRecoveryAllowed(); });
        CHECK_AND_RETURN_RET(finish == E_OK, finish);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    return E_OK;
}
} // namespace OHOS::Media
