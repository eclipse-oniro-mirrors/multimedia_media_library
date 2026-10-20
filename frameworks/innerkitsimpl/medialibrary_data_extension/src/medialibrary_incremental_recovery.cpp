/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
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
#include <dirent.h>
#include <limits>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"
#if defined(MEDIALIBRARY_LAKE_SUPPORT) || defined(MEDIALIBRARY_FILE_MGR_SUPPORT)
#include "consistency_check_manager.h"
#endif
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_scanner_manager.h"
#include "media_string_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify_new.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_restore.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "moving_photo_file_utils.h"
#include "photo_file_utils.h"
#include "post_event_utils.h"
#include "preferences_helper.h"
#include "replica_recovery_policy.h"

namespace OHOS {
namespace Media {
using json = nlohmann::json;

int32_t MediaLibraryMetaRecovery::CheckDatabaseUsability(NativeRdb::RdbStore &store,
    int64_t expectedVersion, bool checkIntegrity)
{
    if (expectedVersion <= 0 || store.IsReadOnly()) {
        return NativeRdb::E_INVALID_ARGS;
    }
    int version = 0;
    int32_t ret = store.GetVersion(version);
    if (ret != NativeRdb::E_OK || version != expectedVersion) {
        return ret != NativeRdb::E_OK ? ret : NativeRdb::E_ERROR;
    }
    if (checkIntegrity) {
        auto [error, value] = store.Execute("PRAGMA quick_check");
        if (error != NativeRdb::E_OK) {
            return error;
        }
        std::string integrity;
        if (value.GetString(integrity) != NativeRdb::E_OK || integrity != "ok") {
            return NativeRdb::E_SQLITE_CORRUPT;
        }
    }
    for (const auto &table : {PhotoColumn::PHOTOS_TABLE, PhotoAlbumColumns::TABLE}) {
        auto result = store.QueryByStep("SELECT COUNT(*) FROM (SELECT 1 FROM " + table + " LIMIT 1)");
        if (result == nullptr) {
            return NativeRdb::E_ERROR;
        }
        int64_t count = -1;
        ret = result->GoToFirstRow();
        if (ret == NativeRdb::E_OK) {
            ret = result->GetLong(0, count);
        }
        result->Close();
        if (ret != NativeRdb::E_OK || count < 0) {
            return ret != NativeRdb::E_OK ? ret : NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

void MediaLibraryMetaRecovery::RecoverPendingRecords()
{
    CHECK_AND_RETURN(!cancelCurrentWork_.load() && !MediaLibraryRestore::GetInstance().IsRestoring());
#if defined(MEDIALIBRARY_LAKE_SUPPORT) || defined(MEDIALIBRARY_FILE_MGR_SUPPORT)
    auto executionLock = ConsistencyCheckManager::GetInstance().TryAcquireRecoveryLock();
    CHECK_AND_RETURN(executionLock.owns_lock());
#endif
    std::unique_lock<std::mutex> lock(metadataMutex_, std::try_to_lock);
    CHECK_AND_RETURN(lock.owns_lock() && !MediaLibraryRestore::GetInstance().IsRestoring());
    int32_t error = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, error);
    if (recoveryRequest_.path.empty()) {
        CHECK_AND_RETURN(prefs != nullptr && prefs->GetBool(REPLICA_REPAIR_PENDING, false));
        recoveryRequest_ = {prefs->GetString(REPLICA_REPAIR_DB_PATH, ""),
            prefs->GetLong(REPLICA_REPAIR_VERSION, 0), prefs->GetString(REPLICA_RECOVERY_DATE, "")};
    }
    auto store = MediaLibraryRdbStore::GetRaw();
    CHECK_AND_RETURN(store != nullptr && store->GetPath() == recoveryRequest_.path &&
        recoveryRequest_.version == MEDIA_RDB_VERSION && !recoveryRequest_.date.empty());
    CHECK_AND_RETURN(prefs == nullptr || !prefs->GetBool(REPLICA_REPAIR_PENDING, false) ||
        HasReplicaRepairTarget(*prefs, recoveryRequest_.path, recoveryRequest_.version));
    auto expected = MediaLibraryMetaRecoveryState::STATE_NONE;
    CHECK_AND_RETURN(recoveryState_.compare_exchange_strong(expected, MediaLibraryMetaRecoveryState::STATE_REPAIRING));
    incrementalStore_ = store;
    recoveryReport_ = {};
    recoveryReport_.date = recoveryRequest_.date;
    recoveryReport_.stage = DatabaseRecoveryStage::BACKGROUND_SCAN;
    int64_t begin = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t ret = ProcessPendingIncrementalRecovery(prefs);
    MediaLibraryRestore::GetInstance().CheckRestore(ret);
    RecordRecoveryScanResult(ret, begin);
    expected = MediaLibraryMetaRecoveryState::STATE_REPAIRING;
    (void)recoveryState_.compare_exchange_strong(expected, MediaLibraryMetaRecoveryState::STATE_NONE);
}

int32_t MediaLibraryMetaRecovery::ProcessPendingIncrementalRecovery(
    const std::shared_ptr<NativePreferences::Preferences> &prefs)
{
    CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
    if (prefs != nullptr && (!HasReplicaRepairTarget(*prefs, recoveryRequest_.path, recoveryRequest_.version) ||
        prefs->GetString(REPLICA_RECOVERY_DATE, "") != recoveryRequest_.date)) {
        int32_t saved = PrepareRecoveryRequest(*prefs, recoveryRequest_);
        CHECK_AND_PRINT_LOG(saved == E_OK, "Recovery pending persistence failed, code=%{public}d", saved);
    }
    int32_t ret = CheckDatabaseUsability(*incrementalStore_, recoveryRequest_.version);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    Notification::MediaLibraryNotifyNew::AddDbAvailabilityItem("available", "");
    ret = RecoverMissingRecordsAfterRestore();
    CHECK_AND_RETURN_RET(ret == E_OK && IsIncrementalRecoveryAllowed(), ret == E_OK ? E_ERR : ret);
    ret = CheckDatabaseUsability(*incrementalStore_, recoveryRequest_.version, false);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(prefs != nullptr, E_ERR);
    CHECK_AND_RETURN_RET(!prefs->GetBool(REPLICA_REPAIR_PENDING, false) ||
        HasReplicaRepairTarget(*prefs, recoveryRequest_.path, recoveryRequest_.version), E_ERR);
    ret = SetRecoveryFlag(*prefs, REPLICA_REPAIR_PENDING, false);
    if (ret == E_OK) {
        recoveryRequest_ = {};
    }
    return ret;
}

int32_t MediaLibraryMetaRecovery::RecordRecoveryScanResult(int32_t error, int64_t begin)
{
    recoveryReport_.error = error;
    recoveryReport_.durationMs = std::max<int64_t>(0, MediaFileUtils::UTCTimeMilliSeconds() - begin);
    recoveryReport_.result = error == E_OK ? DatabaseRecoveryResult::SUCCESS : DatabaseRecoveryResult::FAILED;
    if (IsReplicaRecoveryBusy(error)) {
        recoveryReport_.result = DatabaseRecoveryResult::WAITING;
    } else if (!ShouldPauseRecoveryScan(error) && error != E_HAS_DB_ERROR) {
        if (!IsIncrementalRecoveryAllowed()) {
            recoveryReport_.result = DatabaseRecoveryResult::PAUSED;
        } else if (recoveryReport_.failed > 0 || recoveryReport_.deferred > 0) {
            recoveryReport_.result = DatabaseRecoveryResult::PARTIAL;
        }
    }
    PostEventUtils::GetInstance().PostDatabaseRecovery(recoveryReport_);
    return error;
}

void MediaLibraryMetaRecovery::PrepareIncrementalRecovery(const std::shared_ptr<NativeRdb::RdbStore> &store,
    const std::string &date)
{
    CHECK_AND_RETURN_LOG(store != nullptr && !store->GetPath().empty() && !date.empty(), "Invalid recovery target");
    std::lock_guard<std::mutex> lock(metadataMutex_);
    recoveryRequest_ = {store->GetPath(), MEDIA_RDB_VERSION, date};
    int32_t error = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, error);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Recovery pending preferences unavailable, code=%{public}d", error);
    error = PrepareRecoveryRequest(*prefs, recoveryRequest_);
    CHECK_AND_PRINT_LOG(error == E_OK, "Recovery pending persistence failed, code=%{public}d", error);
}

int32_t MediaLibraryMetaRecovery::QueryPhotoRecoveryState(const std::string &path, int32_t &fileId, bool &hasAlbum)
{
    constexpr int32_t deletedColumnIndex = 2;
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(store != nullptr, E_HAS_DB_ERROR);
    const std::string sql = "SELECT p.file_id, EXISTS(SELECT 1 FROM PhotoAlbum a "
        "WHERE a.album_id=p.owner_album_id), (p.date_deleted>0 OR p.dirty=?) FROM Photos p WHERE p.data=?";
    auto result = store->QuerySql(sql,
        std::vector<std::string>{std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)), path});
    CHECK_AND_RETURN_RET(result != nullptr, E_HAS_DB_ERROR);
    int32_t count = 0;
    int32_t album = 0;
    int32_t deleted = 0;
    int32_t error = result->GetRowCount(count);
    fileId = 0;
    hasAlbum = false;
    if (error == E_OK && count == 1) {
        error = result->GoToFirstRow();
        if (error == E_OK) {
            error = result->GetInt(0, fileId);
        }
        if (error == E_OK) {
            error = result->GetInt(1, album);
            hasAlbum = album != 0;
        }
        if (error == E_OK) {
            error = result->GetInt(deletedColumnIndex, deleted);
        }
    }
    result->Close();
    CHECK_AND_RETURN_RET(error != E_OK || deleted == 0, E_DELETE_DENIED);
    return error != E_OK ? error : (count < 0 || count > 1 ? E_HAS_DB_ERROR : E_OK);
}

class RecoveryScanCallback final : public IMediaScannerCallback {
public:
    int32_t OnScanFinished(const int32_t status, const std::string &, const std::string &) override
    {
        result = status;
        return E_OK;
    }
    // E_NO_THUMB suppresses the callback; the caller also verifies the committed row.
    int32_t result = E_OK;
};

int32_t MediaLibraryMetaRecovery::ScanAssetForRecoverySync(const std::string &path, int32_t fileId)
{
    auto callback = std::make_shared<RecoveryScanCallback>();
    int32_t ret = MediaScannerManager::GetInstance()->ScanFileSync(path, callback,
        MediaLibraryApi::API_10, true, fileId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(callback->result == E_OK || callback->result == E_SCANNED, callback->result);
    int32_t actualId = 0;
    bool hasAlbum = false;
    ret = QueryPhotoRecoveryState(path, actualId, hasAlbum);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    return actualId > 0 && hasAlbum && (fileId == 0 || actualId == fileId) ? E_OK : E_ERR;
}

static std::string ConvertRecoveryLocalPathToCloudPath(const std::string &path)
{
    const std::string localRoot = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR);
    CHECK_AND_RETURN_RET(!localRoot.empty() && MediaStringUtils::StartsWith(path, localRoot), std::string());
    return MediaFileUtils::GetReplacedPathByPrefix(localRoot, ROOT_MEDIA_DIR, path);
}

static int32_t QueryAlbumIdByLpath(const std::string &lpath, int32_t &albumId)
{
    auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(store != nullptr, E_HAS_DB_ERROR);
    auto result = store->QuerySql("SELECT album_id FROM PhotoAlbum WHERE lpath=? "
        "AND album_type IN (?,?) AND share_type=?", std::vector<std::string>{lpath,
            std::to_string(PhotoAlbumType::USER), std::to_string(PhotoAlbumType::SOURCE),
            std::to_string(SHARE_TYPE_NONEALBUM)});
    CHECK_AND_RETURN_RET(result != nullptr, E_HAS_DB_ERROR);
    int32_t count = 0;
    int32_t ret = result->GetRowCount(count);
    albumId = 0;
    if (ret == E_OK && count == 1) {
        ret = result->GoToFirstRow();
        if (ret == E_OK) {
            ret = result->GetInt(0, albumId);
        }
    }
    result->Close();
    return ret != E_OK ? ret : (count < 0 || count > 1 ? E_HAS_DB_ERROR : E_OK);
}

int32_t MediaLibraryMetaRecovery::EnsureAlbumAndMapId(const std::shared_ptr<PhotoAlbum> &album)
{
    CHECK_AND_RETURN_RET(album != nullptr, E_ERR);
    const std::string &lpath = album->GetLPath();
    if (lpath.empty() || album->GetAlbumId() <= 0) {
        return E_OK;
    }
    CHECK_AND_RETURN_RET((album->GetPhotoAlbumType() == PhotoAlbumType::USER ||
        album->GetPhotoAlbumType() == PhotoAlbumType::SOURCE) && album->GetShareType() == SHARE_TYPE_NONEALBUM,
        E_OPERATION_NOT_SUPPORT);
    auto old = oldAlbumIdToLpath.find(album->GetAlbumId());
    CHECK_AND_RETURN_RET(old == oldAlbumIdToLpath.end() || old->second == lpath, E_ERR);
    int32_t albumId = 0;
    int32_t ret = QueryAlbumIdByLpath(lpath, albumId);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (albumId == 0) {
        ret = InsertMetadataInDb(std::vector<std::shared_ptr<PhotoAlbum>>{album});
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        ret = QueryAlbumIdByLpath(lpath, albumId);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        CHECK_AND_RETURN_RET(albumId > 0, E_HAS_DB_ERROR);
        ++recoveryReport_.albumsCreated;
    }
    oldAlbumIdToLpath[album->GetAlbumId()] = lpath;
    lpathToNewAlbumId[lpath] = albumId;
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::PrepareAlbumMappingsForRecovery()
{
    oldAlbumIdToLpath.clear();
    lpathToNewAlbumId.clear();
    std::vector<std::shared_ptr<PhotoAlbum>> albums;
    deferredAlbumIds_.clear();
    if (access(META_RECOVERY_ALBUM_PATH.c_str(), R_OK) != 0) {
        CHECK_AND_RETURN_RET(errno == ENOENT, E_FILE_OPER_FAIL);
        ++recoveryReport_.deferred;
        return E_OK; // Photo candidates are independent; missing album evidence is not full success.
    }
    int32_t ret = ReadPhotoAlbumFromFile(META_RECOVERY_ALBUM_PATH, albums);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Album evidence is incomplete; recover independent photo candidates");
        ++recoveryReport_.deferred;
    }
    for (const auto &album : albums) {
        CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
        int32_t ret = EnsureAlbumAndMapId(album);
        if (ret == E_OPERATION_NOT_SUPPORT) {
            ++recoveryReport_.deferred;
            deferredAlbumIds_.insert(album->GetAlbumId());
            continue;
        }
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    return E_OK;
}

static const std::vector<std::string> &RecoveryPhotoColumns()
{
    static const std::vector<std::string> names = {MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_SIZE,
        MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_TYPE, MediaColumn::MEDIA_MIME_TYPE,
        MediaColumn::MEDIA_DATE_ADDED, MediaColumn::MEDIA_DATE_MODIFIED, MediaColumn::MEDIA_DATE_TAKEN,
        MediaColumn::MEDIA_DURATION, MediaColumn::MEDIA_IS_FAV, MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_DATE_TRASHED, PhotoColumn::PHOTO_WIDTH, PhotoColumn::PHOTO_HEIGHT,
        PhotoColumn::PHOTO_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE, PhotoColumn::PHOTO_USER_COMMENT,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_COVER_POSITION,
        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, PhotoColumn::PHOTO_HDR_MODE, PhotoColumn::PHOTO_DETAIL_TIME,
        PhotoColumn::PHOTO_DATE_YEAR, PhotoColumn::PHOTO_DATE_MONTH, PhotoColumn::PHOTO_DATE_DAY,
        PhotoColumn::PHOTO_LATITUDE, PhotoColumn::PHOTO_LONGITUDE};
    return names;
}

NativeRdb::ValuesBucket MediaLibraryMetaRecovery::BuildRecoveryPhotoValues(FileAsset &asset)
{
    std::unordered_map<std::string, ResultSetDataType> columns;
    for (const auto &name : RecoveryPhotoColumns()) {
        if ((name == PhotoColumn::PHOTO_LATITUDE || name == PhotoColumn::PHOTO_LONGITUDE) &&
            asset.GetMemberMap().count(name) == 0) {
            continue;
        }
        columns[name] = GetDataType(name);
    }
    NativeRdb::ValuesBucket values;
    SetValuesFromFileAsset(asset, values, columns);
    int64_t dateAdded = asset.GetDateAdded() > 0 ? asset.GetDateAdded() :
        (asset.GetDateTaken() > 0 ? asset.GetDateTaken() : asset.GetDateModified());
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    const auto [yearAdded, monthAdded, dayAdded] = PhotoFileUtils::ConstructDateAddedDateParts(dateAdded);
    values.PutString(PhotoColumn::PHOTO_DATE_ADDED_YEAR, yearAdded);
    values.PutString(PhotoColumn::PHOTO_DATE_ADDED_MONTH, monthAdded);
    values.PutString(PhotoColumn::PHOTO_DATE_ADDED_DAY, dayAdded);
    std::string detail = asset.GetStrMember(PhotoColumn::PHOTO_DETAIL_TIME);
    if (detail.empty()) {
        detail = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT,
            (asset.GetDateTaken() > 0 ? asset.GetDateTaken() : dateAdded) / MSEC_TO_SEC);
    }
    const auto [year, month, day] = PhotoFileUtils::ExtractYearMonthDay(detail);
    values.PutString(PhotoColumn::PHOTO_DETAIL_TIME, detail);
    values.PutString(PhotoColumn::PHOTO_DATE_YEAR, year);
    values.PutString(PhotoColumn::PHOTO_DATE_MONTH, month);
    values.PutString(PhotoColumn::PHOTO_DATE_DAY, day);
    values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_DIRTY));
    return values;
}

int32_t MediaLibraryMetaRecovery::WriteRecoveredPhotoRecord(FileAsset &asset, int32_t fileId)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t changed = 0;
    int64_t rowId = fileId;
    int32_t ret = E_OK;
    if (fileId > 0) {
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, asset.GetOwnerAlbumId());
        values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_DIRTY));
        ret = refresh.Update(changed, PhotoColumn::PHOTOS_TABLE, values,
            "file_id=? AND data=? AND date_deleted=0 AND dirty<>? AND "
            "NOT EXISTS(SELECT 1 FROM PhotoAlbum WHERE album_id=Photos.owner_album_id)",
            {std::to_string(fileId), asset.GetFilePath(),
                std::to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED))});
    } else {
        auto values = BuildRecoveryPhotoValues(asset);
        ret = refresh.Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
        changed = ret == E_OK && rowId > 0 ? 1 : 0;
    }
    CHECK_AND_RETURN_RET(ret == E_OK || (fileId == 0 && !MediaLibraryRestore::GetInstance().IsRestoring()), ret);
    int32_t actualId = 0;
    bool hasAlbum = false;
    int32_t query = QueryPhotoRecoveryState(asset.GetFilePath(), actualId, hasAlbum);
    if (ret != E_OK) {
        // A concurrent writer may have committed this path; never replace its state.
        return query == E_OK && actualId > 0 && hasAlbum ? E_OK : ret;
    }
    CHECK_AND_RETURN_RET(query == E_OK, query);
    CHECK_AND_RETURN_RET(actualId > 0 && actualId == rowId && hasAlbum, E_ERR);
    if (changed > 0) {
        ++recoveryReport_.repaired;
        ret = refresh.RefreshAlbumNoDateModified();
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        return refresh.Notify();
    }
    return E_OK;
}

bool MediaLibraryMetaRecovery::IsMetadataMatchingAsset(const std::string &path, const FileAsset &asset)
{
    MediaType type = MediaFileUtils::GetMediaType(path);
    return !path.empty() && asset.GetFilePath() == path &&
        (type == MEDIA_TYPE_IMAGE || type == MEDIA_TYPE_VIDEO) && asset.GetMediaType() == type;
}

bool MediaLibraryMetaRecovery::DecodeIncrementalMetadata(const json &metadata, FileAsset &asset)
{
    const std::vector<std::string> required = {MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED, MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_DATE_TRASHED, MediaColumn::MEDIA_DATE_DELETED, MediaColumn::MEDIA_TIME_PENDING,
        PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_CLOUD_ID, PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        PhotoColumn::PHOTO_IS_TEMP};
    for (const auto &name : required) {
        CHECK_AND_RETURN_RET(metadata.contains(name), false);
    }
    auto names = RecoveryPhotoColumns();
    names.insert(names.end(), required.begin(), required.end());
    names.insert(names.end(), {PhotoColumn::PHOTO_IS_SHARED, PhotoColumn::PHOTO_IS_CRITICAL,
        PhotoColumn::PHOTO_RISK_STATUS, PhotoColumn::PHOTO_DIRTY});
    for (const auto &name : names) {
        if (!metadata.contains(name)) {
            continue; // Optional legacy fields must not require every current schema column.
        }
        bool riskField = name == PhotoColumn::PHOTO_IS_CRITICAL || name == PhotoColumn::PHOTO_RISK_STATUS;
        ResultSetDataType type = riskField ? TYPE_INT32 : GetDataType(name);
        bool decoded = DecodeMetadataMember(name, metadata.at(name), type, asset);
        bool coreField = riskField || name == PhotoColumn::PHOTO_IS_SHARED ||
            name == PhotoColumn::PHOTO_DIRTY ||
            std::find(required.begin(), required.end(), name) != required.end();
        CHECK_AND_RETURN_RET(decoded || !coreField, false);
    }
    return true;
}

bool MediaLibraryMetaRecovery::IsOrdinaryLocalMetadata(const FileAsset &asset)
{
    int32_t hidden = asset.GetInt32Member(MediaColumn::MEDIA_HIDDEN);
    int32_t risk = asset.GetInt32Member(PhotoColumn::PHOTO_RISK_STATUS);
    return asset.GetFileSourceType() == static_cast<int32_t>(FileSourceTypes::MEDIA) &&
        asset.GetInt32Member(PhotoColumn::PHOTO_POSITION) == 1 &&
        asset.GetStrMember(PhotoColumn::PHOTO_CLOUD_ID).empty() &&
        asset.GetInt64Member(MediaColumn::MEDIA_TIME_PENDING) == 0 &&
        asset.GetInt32Member(PhotoColumn::PHOTO_DIRTY) != static_cast<int32_t>(DirtyTypes::TYPE_DELETED) &&
        asset.GetInt64Member(MediaColumn::MEDIA_DATE_DELETED) == 0 && asset.GetDateTrashed() >= 0 &&
        (hidden == 0 || hidden == 1) && asset.GetInt32Member(PhotoColumn::PHOTO_IS_TEMP) == 0 &&
        asset.GetInt32Member(PhotoColumn::PHOTO_IS_SHARED) == 0 &&
        asset.GetInt32Member(PhotoColumn::PHOTO_IS_CRITICAL) == 0 &&
        risk >= 0 && risk <= 1 &&
        !MovingPhotoFileUtils::IsMovingPhoto(asset.GetPhotoSubType(), asset.GetMovingPhotoEffectMode(),
            asset.GetOriginalSubType());
}

int32_t MediaLibraryMetaRecovery::ValidateRecoveryMedia(FileAsset &asset)
{
    const std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(asset.GetFilePath());
    struct stat before {};
    CHECK_AND_RETURN_RET(!local.empty() && lstat(local.c_str(), &before) == 0 && S_ISREG(before.st_mode),
        E_FILE_OPER_FAIL);
    auto data = std::make_unique<Metadata>();
    data->SetFilePath(local);
    data->SetFileExtension(MediaFileUtils::GetExtensionFromPath(local));
    data->SetFileMediaType(asset.GetMediaType());
    data->SetFileSize(asset.GetSize());
    data->SetFileDateModified(asset.GetDateModified());
    data->SetDateTaken(asset.GetDateTaken());
    data->SetForAdd(false);
    int32_t ret = asset.GetMediaType() == MEDIA_TYPE_IMAGE ? MetadataExtractor::ExtractImageMetadata(data) :
        MetadataExtractor::ExtractAVMetadata(data);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(data->GetFileWidth() > 0 && data->GetFileHeight() > 0 && !data->GetFileMimeType().empty(),
        E_FILE_OPER_FAIL);
    struct stat after {};
    CHECK_AND_RETURN_RET(lstat(local.c_str(), &after) == 0 && after.st_dev == before.st_dev &&
        after.st_ino == before.st_ino && IsRecoveryFileIdentityValid(asset.GetSize(), asset.GetDateModified(),
        after.st_size, after.st_mtime), E_FILE_OPER_FAIL);
    asset.SetWidth(data->GetFileWidth());
    asset.SetHeight(data->GetFileHeight());
    asset.SetMimeType(data->GetFileMimeType());
    asset.SetExifRotate(data->GetExifRotate());
    asset.SetOrientation(data->GetOrientation());
    asset.SetDuration(data->GetFileDuration());
    asset.SetMemberValue(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, data->GetDynamicRangeType());
    asset.SetMemberValue(PhotoColumn::PHOTO_HDR_MODE, data->GetHdrMode());
    if (asset.GetDateTaken() <= 0) {
        asset.SetDateTaken(data->GetDateTaken());
    }
    if (data->HasLatitude()) {
        asset.SetLatitude(data->GetLatitude());
    }
    if (data->HasLongitude()) {
        asset.SetLongitude(data->GetLongitude());
    }
    if (asset.GetDisplayName().empty()) {
        asset.SetDisplayName(MediaFileUtils::GetFileName(asset.GetFilePath()));
    }
    if (asset.GetTitle().empty()) {
        asset.SetTitle(MediaFileUtils::GetTitleFromDisplayName(asset.GetDisplayName()));
    }
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::ReadIncrementalMetadata(const std::string &path, FileAsset &asset)
{
    json metadata;
    std::string metaPath;
    int32_t ret = PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    if (access(metaPath.c_str(), R_OK) != 0) {
        return errno == ENOENT ? E_OPERATION_NOT_SUPPORT : E_FILE_OPER_FAIL;
    }
    CHECK_AND_RETURN_RET(ReadJsonFile(metaPath, metadata) && metadata.is_object(), E_FILE_OPER_FAIL);
    CHECK_AND_RETURN_RET(!IsMetadataMarkedDeleted(metadata), E_DELETE_DENIED);
    CHECK_AND_RETURN_RET(DecodeIncrementalMetadata(metadata, asset), E_OPERATION_NOT_SUPPORT);
    CHECK_AND_RETURN_RET(IsMetadataMatchingAsset(path, asset), E_ERR);
    bool hasVersion = metadata.contains("_media_db_version");
    CHECK_AND_RETURN_RET(!hasVersion || metadata.at("_media_db_version").is_number_integer(), E_ERR);
    CHECK_AND_RETURN_RET(!hasVersion || !metadata.at("_media_db_version").is_number_unsigned() ||
        metadata.at("_media_db_version").get<uint64_t>() <=
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), E_ERR);
    int64_t version = hasVersion ? metadata.at("_media_db_version").get<int64_t>() : 0;
    CHECK_AND_RETURN_RET(!hasVersion || version == MEDIA_RDB_VERSION, E_ERR);
    CHECK_AND_RETURN_RET(IsOrdinaryLocalMetadata(asset), E_OPERATION_NOT_SUPPORT);
    struct stat info {};
    std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(path);
    CHECK_AND_RETURN_RET(!local.empty() && lstat(local.c_str(), &info) == 0 && S_ISREG(info.st_mode), E_FILE_OPER_FAIL);
    CHECK_AND_RETURN_RET(IsRecoveryFileIdentityValid(asset.GetSize(), asset.GetDateModified(),
        info.st_size, info.st_mtime), E_ERR);
    return ValidateRecoveryMedia(asset);
}

int32_t MediaLibraryMetaRecovery::RepairPhotoRecord(const std::string &path)
{
    int32_t fileId = 0;
    bool hasAlbum = false;
    int32_t ret = QueryPhotoRecoveryState(path, fileId, hasAlbum);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(!hasAlbum, E_OK);
    // A companion video without its own ordinary-asset metadata is deferred, never guessed by basename.
    FileAsset asset;
    ret = ReadIncrementalMetadata(path, asset);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
    CHECK_AND_RETURN_RET(deferredAlbumIds_.count(asset.GetOwnerAlbumId()) == 0, E_OPERATION_NOT_SUPPORT);
    auto oldAlbum = oldAlbumIdToLpath.find(asset.GetOwnerAlbumId());
    if (oldAlbum != oldAlbumIdToLpath.end() && !oldAlbum->second.empty()) {
        auto newAlbum = lpathToNewAlbumId.find(oldAlbum->second);
        if (newAlbum != lpathToNewAlbumId.end() && newAlbum->second > 0) {
            asset.SetOwnerAlbumId(newAlbum->second);
            return WriteRecoveredPhotoRecord(asset, fileId);
        }
    }
    // A scanner must not publish a hidden/trashed asset or overwrite an existing row.
    CHECK_AND_RETURN_RET(fileId == 0 && asset.GetInt32Member(MediaColumn::MEDIA_HIDDEN) == 0 &&
        asset.GetDateTrashed() == 0, E_OPERATION_NOT_SUPPORT);
    ret = ScanAssetForRecoverySync(path, fileId);
    if (ret == E_OK) {
        ++recoveryReport_.repaired;
    }
    return ret;
}

int32_t MediaLibraryMetaRecovery::RepairRecoveryEntry(const std::string &path, const struct stat &info)
{
    if (S_ISDIR(info.st_mode)) {
        return RepairPhotoRecordsInDirectory(path);
    }
    MediaType type = MediaFileUtils::GetMediaType(path);
    if (!S_ISREG(info.st_mode) || (type != MEDIA_TYPE_IMAGE && type != MEDIA_TYPE_VIDEO)) {
        return E_OK;
    }
    ++recoveryReport_.visited;
    std::string cloud = ConvertRecoveryLocalPathToCloudPath(path);
    int32_t ret = cloud.empty() ? E_INVALID_PATH : RepairPhotoRecord(cloud);
    if (ret == E_DELETE_DENIED) {
        return E_OK; // Explicit deletion is a terminal skip, not a failed or deferred import.
    }
    if (ret == E_OPERATION_NOT_SUPPORT) {
        ++recoveryReport_.deferred;
    } else if (ret != E_OK) {
        ++recoveryReport_.failed;
    }
    return ret;
}

int32_t MediaLibraryMetaRecovery::RepairPhotoRecordsInDirectory(const std::string &path)
{
    CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
    struct stat directory {};
    CHECK_AND_RETURN_RET(lstat(path.c_str(), &directory) == 0 && S_ISDIR(directory.st_mode), E_FILE_OPER_FAIL);
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(path.c_str()), closedir);
    CHECK_AND_RETURN_RET(dir != nullptr, E_FILE_OPER_FAIL);
    int32_t ret = E_OK;
    // Durable DB rows are the progress authority. Re-enumeration never trusts a separate completion file.
    for (errno = 0; auto entry = readdir(dir.get()); errno = 0) {
        CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
        std::string name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }
        std::string child = path + "/" + name;
        struct stat info {};
        if (lstat(child.c_str(), &info) != 0) {
            ret = E_FILE_OPER_FAIL;
            continue;
        }
        int32_t item = RepairRecoveryEntry(child, info);
        CHECK_AND_RETURN_RET(!ShouldPauseRecoveryScan(item) && item != E_HAS_DB_ERROR, item);
        // Unsupported candidates are counted, but must not hide an earlier retryable failure.
        if (item != E_OK && item != E_OPERATION_NOT_SUPPORT) {
            ret = item;
        }
    }
    CHECK_AND_RETURN_RET(errno == 0 && IsIncrementalRecoveryAllowed(), E_FILE_OPER_FAIL);
    return ret;
}

int32_t MediaLibraryMetaRecovery::RecoverMissingRecordsAfterRestore()
{
    CHECK_AND_RETURN_RET(IsIncrementalRecoveryAllowed(), E_ERR);
    int32_t ret = PrepareAlbumMappingsForRecovery();
    // Enumerate physical photo assets, not the media_fuse view. No mtime filter:
    // an old photo may have lost an album change made during the replica gap.
    if (ret == E_OK) {
        std::string localRoot = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR + "Photo");
        ret = localRoot.empty() ? E_INVALID_PATH : RepairPhotoRecordsInDirectory(localRoot);
    }
    oldAlbumIdToLpath.clear();
    lpathToNewAlbumId.clear();
    deferredAlbumIds_.clear();
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    ret = RefreshAlbumCount();
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    // Also covers rows committed by an earlier interrupted pass whose per-item notification failed.
    ret = AccurateRefresh::AlbumAccurateRefresh::NotifyForReCheck();
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    return AccurateRefresh::AssetAccurateRefresh::NotifyForReCheck();
}

} // namespace Media
} // namespace OHOS
