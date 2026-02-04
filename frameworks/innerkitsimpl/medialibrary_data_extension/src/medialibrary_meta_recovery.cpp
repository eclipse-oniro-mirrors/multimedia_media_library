/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryMetaRecovery"

#include "medialibrary_meta_recovery.h"

#include "acl.h"
#ifdef HAS_BATTERY_MANAGER_PART
#include "battery_srv_client.h"
#endif
#include "cloud_sync_helper.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "hisysevent.h"
#include "media_scanner_manager.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "photo_file_utils.h"
#include "post_event_utils.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#ifdef HAS_THERMAL_MANAGER_PART
#include "thermal_mgr_client.h"
#endif
#include "parameters.h"
#include "media_string_utils.h"

namespace OHOS {
namespace Media {
using namespace std;
using json = nlohmann::json;

namespace {
    const string META_RECOVERY_ROOT_DIR = ROOT_MEDIA_DIR + ".meta/";
    const string META_RECOVERY_META_PATH = ROOT_MEDIA_DIR + ".meta/Photo";
    const string META_RECOVERY_ALBUM_PATH = META_RECOVERY_ROOT_DIR + "album.json";
    const string META_STATUS_PATH = META_RECOVERY_ROOT_DIR + "status.json";
    constexpr int32_t QUERY_BATCH_SIZE = 500;
    constexpr int32_t META_RETRY_MAX_COUNTS = 10;
    constexpr int32_t META_RETRY_INTERVAL = 100;
    const std::string RDB_CONFIG = "/data/storage/el2/base/preferences/recovery_config.xml";
    const std::string BACKUP_PHOTO_COUNT = "BACKUP_PHOTO_COUNT";
    const std::string BACKUP_COST_TIME = "BACKUP_COST_TIME";
    const std::string REBUILT_COUNT = "REBUILT_COUNT";
    const std::string RECOVERY_BACKUP_TOTAL_COUNT = "RECOVERY_BACKUP_TOTAL_COUNT";
    const std::string RECOVERY_SUCC_PHOTO_COUNT = "RECOVERY_SUCC_PHOTO_COUNT";
    const std::string RECOVERY_COST_TIME = "RECOVERY_COST_TIME";
    static const std::unordered_set<std::string> EXCLUDED_COLUMNS = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_VIRTUAL_PATH,
        PhotoColumn::PHOTO_THUMBNAIL_READY,
        PhotoColumn::PHOTO_METADATA_FLAGS,
    };
}  // namespace

static void SetStartupParam()
{
    static constexpr uint32_t BASE_USER_RANGE = 200000; // for get uid
    uid_t uid = getuid() / BASE_USER_RANGE;
    const string key = "multimedia.medialibrary.startup." + to_string(uid);
    string value = "true";
    int ret = system::SetParameter(key.c_str(), value.c_str());
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to set startup, result: %{public}d", ret);
    } else {
        MEDIA_INFO_LOG("Set startup success: %{public}s", to_string(uid).c_str());
    }
}

static int32_t RefreshThumbnail()
{
    MediaLibraryKvStoreManager::GetInstance().RebuildInvalidKvStore(KvStoreValueType::MONTH_ASTC);
    MediaLibraryKvStoreManager::GetInstance().RebuildInvalidKvStore(KvStoreValueType::YEAR_ASTC);
    Acl::AclSetDatabase();
    return E_OK;
}

static int32_t RefreshAlbumCount()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RefreshAlbumCount: failed to get rdb store handler");

    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");

    watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_ADD);
    watch->Notify(PhotoAlbumColumns::ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

static std::optional<std::string> GetStringFromJson(const nlohmann::json &j, const std::string &key)
{
    if (j.contains(key) && j.at(key).is_string()) {
        std::string value = j.at(key);
        MEDIA_DEBUG_LOG("get string json ok, %{private}s: %{private}s", key.c_str(), value.c_str());
        return std::optional<std::string>(value);
    } else {
        MEDIA_ERR_LOG("get key: %{private}s failed", key.c_str());
        return std::nullopt;
    }
}

static std::optional<int64_t> GetNumberFromJson(const nlohmann::json &j, const std::string &key)
{
    if (j.contains(key) && j.at(key).is_number_integer()) {
        int64_t value = j.at(key);
        return std::optional<int64_t>(value);
    } else {
        MEDIA_ERR_LOG("get key: %{private}s failed", key.c_str());
        return std::nullopt;
    }
}

static std::optional<double> GetDoubleFromJson(const nlohmann::json &j, const std::string &key)
{
    if (j.contains(key) && j.at(key).is_number_float()) {
        double value = j.at(key);
        MEDIA_DEBUG_LOG("get double json ok, %{private}s: %{private}f", key.c_str(), value);
        return std::optional<double>(value);
    } else {
        MEDIA_ERR_LOG("get key: %{private}s failed", key.c_str());
        return std::nullopt;
    }
}

static void SetValuesFromFileAsset(const FileAsset &fileAsset, NativeRdb::ValuesBucket &values,
    const std::unordered_map<std::string, ResultSetDataType> &columnInfoMap)
{
    for (const auto &[name, type] : columnInfoMap) {
        if (type == TYPE_STRING) {
            values.PutString(name, fileAsset.GetStrMember(name));
        } else if (type == TYPE_INT32) {
            values.PutInt(name, fileAsset.GetInt32Member(name));
        } else if (type == TYPE_INT64) {
            values.PutLong(name, fileAsset.GetInt64Member(name));
        } else if (type == TYPE_DOUBLE) {
            values.PutDouble(name, fileAsset.GetDoubleMember(name));
        } else {
            MEDIA_DEBUG_LOG("Invalid fileasset value type, name = %{private}s, type = %{public}d", name.c_str(), type);
        }
    }
}

static void SetValuesFromPhotoAlbum(shared_ptr<PhotoAlbum> &photoAlbumPtr, NativeRdb::ValuesBucket &values)
{
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, photoAlbumPtr->GetPhotoAlbumType());
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, photoAlbumPtr->GetPhotoAlbumSubType());
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, photoAlbumPtr->GetAlbumName());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, photoAlbumPtr->GetDateModified());
    values.PutInt(PhotoAlbumColumns::CONTAINS_HIDDEN, photoAlbumPtr->GetContainsHidden());
    values.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, photoAlbumPtr->GetBundleName());
    values.PutString(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, photoAlbumPtr->GetLocalLanguage());
    values.PutInt(PhotoAlbumColumns::ALBUM_IS_LOCAL, photoAlbumPtr->GetIsLocal());
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_ADDED, photoAlbumPtr->GetDateAdded());
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, photoAlbumPtr->GetLPath());
    values.PutInt(PhotoAlbumColumns::ALBUM_PRIORITY, photoAlbumPtr->GetPriority());
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, photoAlbumPtr->GetUploadStatus());
}

static bool GetPhotoAlbumFromJsonPart1(const nlohmann::json &j, PhotoAlbum &photoAlbum)
{
    bool ret = true;

    optional<string> bundleName = GetStringFromJson(j, PhotoAlbumColumns::ALBUM_BUNDLE_NAME);
    if (bundleName.has_value()) {
        photoAlbum.SetBundleName(bundleName.value());
    } else {
        ret = false;
    }

    optional<string> localLanguage = GetStringFromJson(j, PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE);
    if (localLanguage.has_value()) {
        photoAlbum.SetLocalLanguage(localLanguage.value());
    } else {
        ret = false;
    }

    optional<int64_t> dateAdded = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_DATE_ADDED);
    if (dateAdded.has_value()) {
        photoAlbum.SetDateAdded(dateAdded.value());
    } else {
        ret = false;
    }

    optional<int64_t> isLocal = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_IS_LOCAL);
    if (isLocal.has_value()) {
        photoAlbum.SetIsLocal((int32_t)isLocal.value());
    } else {
        ret = false;
    }

    optional<string> lPath = GetStringFromJson(j, PhotoAlbumColumns::ALBUM_LPATH);
    if (lPath.has_value()) {
        photoAlbum.SetLPath(lPath.value());
    } else {
        ret = false;
    }

    optional<int64_t> priority = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_PRIORITY);
    if (priority.has_value()) {
        photoAlbum.SetPriority((int32_t)priority.value());
    } else {
        ret = false;
    }

    optional<int64_t> uploadStatus = GetNumberFromJson(j, PhotoAlbumColumns::UPLOAD_STATUS);
    if (uploadStatus.has_value()) {
        photoAlbum.SetUploadStatus((int32_t)uploadStatus.value());
    } else {
        ret = false;
    }

    return ret;
}

MediaLibraryMetaRecovery &MediaLibraryMetaRecovery::GetInstance()
{
    static MediaLibraryMetaRecovery instance;
    return instance;
}

void MediaLibraryMetaRecovery::CheckRecoveryState()
{
    MediaLibraryMetaRecoveryState expect = MediaLibraryMetaRecoveryState::STATE_NONE;
    if (recoveryState_.compare_exchange_strong(expect, MediaLibraryMetaRecoveryState::STATE_BACKING_UP)) {
        std::thread([this]() {
            int64_t backupStartTime = MediaFileUtils::UTCTimeMilliSeconds();
            this->DoBackupMetadata();
            int64_t backupTotalTime = MediaFileUtils::UTCTimeMilliSeconds() - backupStartTime;
            backupCostTime_ += backupTotalTime;
            MediaLibraryMetaRecoveryState expect = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
            if (recoveryState_.compare_exchange_strong(expect, MediaLibraryMetaRecoveryState::STATE_NONE)) {
                MEDIA_INFO_LOG("End backing up normaly");
            } else {
                MEDIA_INFO_LOG("End backing up interrupted");
            }
        }).detach();
    } else {
        MEDIA_INFO_LOG("Ignore backing up, current status = %{public}d", expect);
    }
}

void MediaLibraryMetaRecovery::InterruptRecovery()
{
    switch (recoveryState_.load()) {
        case MediaLibraryMetaRecoveryState::STATE_BACKING_UP: {
            MediaLibraryMetaRecoveryState expect = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
            if (recoveryState_.compare_exchange_strong(expect, MediaLibraryMetaRecoveryState::STATE_NONE)) {
                MEDIA_INFO_LOG("InterruptRecovery: success send interrupt request");
            } else {
                MEDIA_INFO_LOG("InterruptRecovery: backup process is finished, no need to interrupt");
            }
            break;
        }
        case MediaLibraryMetaRecoveryState::STATE_RECOVERING: {
            MEDIA_INFO_LOG("InterruptRecovery: need to interrupt recovery process");
            break;
        }
        default: {
            MEDIA_INFO_LOG("InterruptRecovery: nother recovery or backup is processing, ignore");
            break;
        }
    }
}

void MediaLibraryMetaRecovery::LoadAlbumMaps(const string &path)
{
    // 1. album.json to oldAlbumIdToLpath
    int32_t ret = E_OK;
    std::vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    ret = ReadPhotoAlbumFromFile(path, vecPhotoAlbum);
    CHECK_AND_RETURN_LOG(ret == E_OK, "read album file failed, path=%{public}s", DfxUtils::GetSafePath(path).c_str());

    for (auto it : vecPhotoAlbum) {
        oldAlbumIdToLpath[it->GetAlbumId()] = it->GetLPath();
        MEDIA_INFO_LOG("oldAlbumIdToLpath, json id %{public}d, path=%{public}s", it->GetAlbumId(),
            DfxUtils::GetSafePath(it->GetLPath()).c_str());
    }
    // 2. db PhotoAlbum to lpathToNewAlbumId
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_LPATH};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet == nullptr)");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        string lPath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        lpathToNewAlbumId[lPath] = albumId;
        MEDIA_INFO_LOG("lpathToNewAlbumId, path=%{public}s db id %{public}d, ",
            DfxUtils::GetSafePath(lPath).c_str(), albumId);
    }
    return;
}

void MediaLibraryMetaRecovery::DoDataBaseRecovery()
{
    SetStartupParam();
    StopCloudSync();
    RefreshThumbnail();
    MEDIA_INFO_LOG("Album recovery start");
    if (AlbumRecovery(ROOT_MEDIA_DIR + ".meta/album.json") != E_OK) {
        MEDIA_ERR_LOG("Recovery Album failed");
    }
    MEDIA_INFO_LOG("Album recovery end");

    LoadAlbumMaps(ROOT_MEDIA_DIR+".meta/album.json");

    MEDIA_INFO_LOG("Photo recovery start");
    if (PhotoRecovery(META_RECOVERY_META_PATH) != E_OK) {
        MEDIA_ERR_LOG("Recover Photo failed");
    }
    MEDIA_INFO_LOG("Photo recovery end");

    RefreshAlbumCount();
    RestartCloudSync();
    oldAlbumIdToLpath.clear();
    lpathToNewAlbumId.clear();
}

int32_t MediaLibraryMetaRecovery::AlbumRecovery(const string &path)
{
    int32_t ret = E_OK;
    std::vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;

    do {
        ret = access(path.c_str(), F_OK | R_OK);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK,
            "file is not exist or no read access, path=%{public}s", DfxUtils::GetSafePath(path).c_str());

        ret = ReadPhotoAlbumFromFile(path, vecPhotoAlbum);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "read album file failed, errCode = %{public}d", ret);

        ret = InsertMetadataInDb(vecPhotoAlbum);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "AlbumRecovery: insert album failed, errCode = %{public}d", ret);
        MEDIA_INFO_LOG("AlbumRecovery: photo album is recovered successful");
    } while (false);

    return ret;
}

static int32_t GetTotalBackupFileCount()
{
    int count = 0;
    if (access(META_RECOVERY_META_PATH.c_str(), F_OK) != E_OK) {
        return count;
    }

    filesystem::path dir(META_RECOVERY_META_PATH);
    std::error_code ec;
    auto iter = filesystem::recursive_directory_iterator(dir, ec);
    CHECK_AND_RETURN_RET_LOG(ec, count, "Iterator creation failed: %{public}s", ec.message().c_str());
    for (; iter != filesystem::recursive_directory_iterator(); iter.increment(ec)) {
        if (ec) {
            MEDIA_ERR_LOG("Iteration error: %{public}s", ec.message().c_str());
            ec.clear(); // 清除错误，尝试继续处理其他文件
            continue;
        }
        if (iter->is_regular_file() && iter->path().extension() == ".json") {
            ++count;
        }
    }

    return count;
}

int32_t MediaLibraryMetaRecovery::PhotoRecovery(const string &path)
{
    string realPath;
    int32_t bucket_id = -1;

    if (!PathToRealPath(path, realPath)) {
        if (errno == ENOENT) {
            // Delte Metastatus Json;
            remove(META_STATUS_PATH.c_str());
            // Delete status
            metaStatus.clear();
            MEDIA_ERR_LOG("no meta file no need to recovery");
            return E_OK;
        }
        MEDIA_ERR_LOG("Failed to get real path %{private}s, errno %{public}d", path.c_str(), errno);
        return E_INVALID_PATH;
    }

    recoveryTotalBackupCnt_ = GetTotalBackupFileCount();
    MEDIA_INFO_LOG("recovery success total backup");

    if (!ScannerUtils::IsDirectory(realPath)) {
        MEDIA_ERR_LOG("The path %{private}s is not a directory", realPath.c_str());
        return E_INVALID_PATH;
    }

    int err = ScanMetaDir(path, bucket_id);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to ScanMetaDir, errCode=%{public}d", err);
    }

    recoverySuccCnt_ += ReadMetaRecoveryCountFromFile();

    // Delte Metastatus Json;
    err = remove(META_STATUS_PATH.c_str());
    if (err != E_OK) {
        MEDIA_WARN_LOG("Remove META_STATUS_PATH failed, errCode=%{public}d", err);
    }
    // Delete status
    metaStatus.clear();

    return err;
}

int32_t MediaLibraryMetaRecovery::WriteSingleMetaDataById(int32_t rowId)
{
    int ret = E_OK;

    MEDIA_DEBUG_LOG("WriteSingleMetaDataById : rowId %{public}d", rowId);
    auto asset = MediaLibraryAssetOperations::QuerySinglePhoto(rowId);
    CHECK_AND_RETURN_RET_LOG(asset != nullptr, E_HAS_DB_ERROR, "QuerySinglePhoto : rowId %{public}d failed", rowId);

    ret = WriteSingleMetaData(*asset);
    if (ret == E_OK) {
        backupSuccCnt_++;
    }
    return ret;
}

int32_t MediaLibraryMetaRecovery::WriteSingleMetaData(const FileAsset &asset)
{
    string metaFilePath;
    int32_t ret = E_OK;

    ret = PhotoFileUtils::GetMetaPathFromOrignalPath(asset.GetPath(), metaFilePath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("invalid photo path, path = %{public}s", DfxUtils::GetSafePath(asset.GetPath()).c_str());
        return ret;
    }

    // Create direcotry
    const string metaParentPath = MediaFileUtils::GetParentPath(metaFilePath);
    if (!MediaFileUtils::CreateDirectory(metaParentPath)) {
        MEDIA_ERR_LOG("photo: CreateDirectory failed, filePath = %{public}s",
            DfxUtils::GetSafePath(metaParentPath).c_str());
        return E_HAS_FS_ERROR;
    }

    // Create metadata file
    ret = WriteMetadataToFile(metaFilePath, asset);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("photo: WriteMetadataToFile failed, filePath = %{public}s",
            DfxUtils::GetSafePath(metaFilePath).c_str());
        return ret;
    }

    // Up to date
    ret = UpdateMetadataFlagInDb(asset.GetId(), MetadataFlags::TYPE_UPTODATE);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("photo: Up to date failed, filePath = %{public}s", DfxUtils::GetSafePath(metaFilePath).c_str());
        return ret;
    }

    return ret;
}

void MediaLibraryMetaRecovery::DoBackupMetadata()
{
    int32_t temp = 0;
    int32_t tempLevel = 0;
    int32_t batteryCapacity = 0;

#ifdef HAS_THERMAL_MANAGER_PART
    auto& thermalMgrClient = PowerMgr::ThermalMgrClient::GetInstance();
    temp = static_cast<int32_t>(thermalMgrClient.GetThermalSensorTemp(PowerMgr::SensorType::SHELL));
    tempLevel = static_cast<int32_t>(thermalMgrClient.GetThermalLevel());
#endif
#ifdef HAS_BATTERY_MANAGER_PART
    batteryCapacity = PowerMgr::BatterySrvClient::GetInstance().GetCapacity();
#endif
    MEDIA_INFO_LOG("Start backing up, batteryCap = %{public}d, temp = %{public}d(%{public}d)",
        batteryCapacity, temp, tempLevel);

    // Backing up photo albums
    AlbumBackup();

    // Backing up photos
    PhotoBackupBatch();
}

void MediaLibraryMetaRecovery::AlbumBackup()
{
    vector<shared_ptr<PhotoAlbum>> photoAlbumVector;
    MediaLibraryAssetOperations::QueryTotalAlbum(photoAlbumVector);
    int photoAlbumCount = photoAlbumVector.size();
    if (photoAlbumCount <= 0) {
        MEDIA_INFO_LOG("AlbumBackup: no photo albums need to backup");
        return;
    }

    MEDIA_INFO_LOG("AlbumBackup: album count = %{public}d", photoAlbumCount);
    if (E_OK != WritePhotoAlbumToFile(META_RECOVERY_ALBUM_PATH, photoAlbumVector)) {
        MEDIA_ERR_LOG("AlbumBackup: WritePhotoAlbumToFile failed");
    }
}

void MediaLibraryMetaRecovery::PhotoBackupBatch()
{
    int32_t photoTotalCount = 0;
    int32_t photoProcessedCount = 0;
    int32_t photoSuccessedCount = 0;
    vector<shared_ptr<FileAsset>> photoVector;
    do {
        if (recoveryState_.load() != MediaLibraryMetaRecoveryState::STATE_BACKING_UP) {
            MEDIA_INFO_LOG("Photo backing up process is interrupted");
            break;
        }

        photoVector.clear();
        MediaLibraryAssetOperations::QueryTotalPhoto(photoVector, QUERY_BATCH_SIZE);
        if (photoVector.size() > 0) {
            photoTotalCount += photoVector.size();
            PhotoBackup(photoVector, photoProcessedCount, photoSuccessedCount);
        }
    } while (photoVector.size() == QUERY_BATCH_SIZE);
    MEDIA_INFO_LOG("Photo backup end, result = %{public}d/%{public}d/%{public}d",
        photoSuccessedCount, photoProcessedCount, photoTotalCount);
    backupSuccCnt_ += photoSuccessedCount;
}

void MediaLibraryMetaRecovery::PhotoBackup(const vector<shared_ptr<FileAsset>> &photoVector,
                                           int32_t &processCount,
                                           int32_t &successCount)
{
    for (auto &asset : photoVector) {
        // Check interrupt request
        if (recoveryState_.load() != MediaLibraryMetaRecoveryState::STATE_BACKING_UP) {
            MEDIA_INFO_LOG("Photo backing up process is interrupted");
            break;
        }

        processCount++;

        if (!asset) {
            MEDIA_ERR_LOG("Photo asset pointer is null");
            continue;
        }

        if (E_OK != WriteSingleMetaData(*asset)) {
            MEDIA_ERR_LOG("WriteSingleMetaData failed");
            continue;
        }

        successCount++;
    }
}

int32_t MediaLibraryMetaRecovery::ScanMetaDir(const string &path, int32_t bucket_id)
{
    int err = E_OK;
    DIR *dirPath = nullptr;
    struct dirent *ent = nullptr;
    size_t len = path.length();
    struct stat statInfo;

    if (len >= FILENAME_MAX - 1) {
        return ERR_INCORRECT_PATH;
    }

    auto fName = (char *)calloc(FILENAME_MAX, sizeof(char));
    if (fName == nullptr) {
        return ERR_MEM_ALLOC_FAIL;
    }

    if (strcpy_s(fName, FILENAME_MAX, path.c_str()) != ERR_SUCCESS) {
        FREE_MEMORY_AND_SET_NULL(fName);
        return ERR_MEM_ALLOC_FAIL;
    }
    fName[len++] = '/';
    if ((dirPath = opendir(path.c_str())) == nullptr) {
        MEDIA_ERR_LOG("Failed to opendir %{private}s, errno %{private}d", path.c_str(), errno);
        FREE_MEMORY_AND_SET_NULL(fName);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, -errno},
            {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::SCAN}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return ERR_NOT_ACCESSIBLE;
    }

    int32_t recoverySuccessCnt = 0;
    while ((ent = readdir(dirPath)) != nullptr) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
            continue;
        }

        if (strncpy_s(fName + len, FILENAME_MAX - len, ent->d_name, FILENAME_MAX - len)) {
            continue;
        }

        if (lstat(fName, &statInfo) == -1) {
            continue;
        }

        string currentPath = fName;
        if (S_ISDIR(statInfo.st_mode)) {
            int32_t cur_bucket = atoi(ent->d_name);
            MEDIA_INFO_LOG("currentPath=%{public}s, path=%{public}s, cur_bucket %{public}d recovery start",
                           DfxUtils::GetSafePath(currentPath).c_str(), DfxUtils::GetSafePath(path).c_str(), cur_bucket);

            // Recovery after interrupt, skip bucket which scanned.
            if (metaStatus.find(cur_bucket) != metaStatus.end()) {
                MEDIA_INFO_LOG("skip bucket id=%{public}d", cur_bucket);
                continue;
            }
            (void)ScanMetaDir(currentPath, cur_bucket);
            RefreshAlbumCount();
            continue;
        }

        MEDIA_DEBUG_LOG("currentPath=%{public}s, path=%{public}s",
            DfxUtils::GetSafePath(currentPath).c_str(), DfxUtils::GetSafePath(path).c_str());

        FileAsset fileAsset;
        if (ReadMetadataFromFile(currentPath, fileAsset) != E_OK) {
            MEDIA_ERR_LOG("ScanMetaDir: ReadMetadataFrom file failed");
            continue;
        }

        // Insert fileAsset to DB
        if (InsertMetadataInDbRetry(fileAsset) == E_OK) {
            recoverySuccessCnt++;
        }
    }

    closedir(dirPath);
    FREE_MEMORY_AND_SET_NULL(fName);

    if (bucket_id != -1) {
        err = WriteMetaStatusToFile(to_string(bucket_id), recoverySuccessCnt);
        if (err != E_OK) {
            MEDIA_ERR_LOG("write meta status failed");
        }
        MEDIA_INFO_LOG("cur_bucket %{public}d recovery end", bucket_id);
    }

    return err;
}

bool MediaLibraryMetaRecovery::WriteJsonFile(const std::string &filePath, const nlohmann::json &j)
{
    const string parentDir = MediaFileUtils::GetParentPath(filePath);
    if (!MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("CreateDirectory failed, dir = %{public}s", DfxUtils::GetSafePath(parentDir).c_str());
        return false;
    }

    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open()) {
        MEDIA_ERR_LOG("open filePath: %{private}s failed", filePath.c_str());
        return false;
    }
    std::string jsonString = j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    outFile << jsonString << std::endl;
    outFile.close();
    return true;
}

bool MediaLibraryMetaRecovery::ReadJsonFile(const std::string &filePath, nlohmann::json &j)
{
    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        MEDIA_ERR_LOG("open filePath: %{private}s failed", filePath.c_str());
        return false;
    }
    std::string buffer = std::string((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    j = json::parse(buffer, nullptr, false);
    inFile.close();
    return !j.is_discarded();
}

int32_t MediaLibraryMetaRecovery::WriteMetadataToFile(const string &filePath, const FileAsset &fileAsset)
{
    json jsonMetadata;
    AddMetadataToJson(jsonMetadata, fileAsset);
    if (!WriteJsonFile(filePath, jsonMetadata)) {
        MEDIA_ERR_LOG("WriteJsonFile failed");
        return E_FILE_OPER_FAIL;
    }
    return E_OK;
}

void MediaLibraryMetaRecovery::AddMetadataToJson(nlohmann::json &j, const FileAsset &fileAsset)
{
    std::unordered_map<std::string, ResultSetDataType> columnInfoMap = QueryRecoveryPhotosTableColumnInfo();
    if (columnInfoMap.empty()) {
        MEDIA_ERR_LOG("QueryRecoveryPhotosTableColumnInfo failed");
        return;
    }

    for (const auto &[key, type] : columnInfoMap) {
        if (type == TYPE_STRING) {
            string value = fileAsset.GetStrMember(key);
            MEDIA_DEBUG_LOG("Writejson string: %{private}s: %{private}s", key.c_str(), value.c_str());
            j[key] = json::string_t(value);
        } else if (type == TYPE_INT32) {
            int32_t value = fileAsset.GetInt32Member(key);
            MEDIA_DEBUG_LOG("Writejson int32_t: %{private}s: %{public}d", key.c_str(), value);
            j[key] = json::number_integer_t(value);
        } else if (type == TYPE_INT64) {
            int64_t value = fileAsset.GetInt64Member(key);
            j[key] = json::number_integer_t(value);
        } else if (type == TYPE_DOUBLE) {
            double value = fileAsset.GetDoubleMember(key);
            MEDIA_DEBUG_LOG("Writejson double: %{private}s: %{public}f", key.c_str(), value);
            j[key] = json::number_float_t(value);
        } else {
            MEDIA_ERR_LOG("WriteFile: error type: %{public}d", type);
        }
    }
}

bool MediaLibraryMetaRecovery::GetMetadataFromJson(const nlohmann::json &j, FileAsset &fileAsset)
{
    std::unordered_map<std::string, ResultSetDataType> columnInfoMap = QueryRecoveryPhotosTableColumnInfo();
    if (columnInfoMap.empty()) {
        MEDIA_ERR_LOG("QueryRecoveryPhotosTableColumnInfo failed");
        return false;
    }

    bool ret = true;
    for (const auto &[name, type] : columnInfoMap) {
        if (type == TYPE_STRING) {
            optional<string> value = GetStringFromJson(j, name);
            if (value.has_value()) {
                fileAsset.SetMemberValue(name, value.value());
            } else {
                ret = false;
            }
        } else if (type == TYPE_INT32) {
            optional<int64_t> value = GetNumberFromJson(j, name);
            if (value.has_value()) {
                fileAsset.SetMemberValue(name, (int32_t)value.value());
            } else {
                ret = false;
            }
        } else if (type == TYPE_INT64) {
            optional<int64_t> value = GetNumberFromJson(j, name);
            if (value.has_value()) {
                fileAsset.SetMemberValue(name, (int64_t)value.value());
            } else {
                ret = false;
            }
        } else if (type == TYPE_DOUBLE) {
            optional<double> value = GetDoubleFromJson(j, name);
            if (value.has_value()) {
                fileAsset.SetMemberValue(name, (double)value.value());
            } else {
                ret = false;
            }
        } else {
            MEDIA_ERR_LOG("ReadFile: error %{public}d", type);
        }
    }

    return ret;
}

int32_t MediaLibraryMetaRecovery::ReadMetadataFromFile(const string &filePath, FileAsset &fileAsset)
{
    int ret = E_OK;
    json jsonMetadata;
    if (!ReadJsonFile(filePath, jsonMetadata)) {
        MEDIA_ERR_LOG("ReadJsonFile failed");
        return E_FILE_OPER_FAIL;
    }

    if (!GetMetadataFromJson(jsonMetadata, fileAsset)) {
        MEDIA_ERR_LOG("GetMetadataFromJson not all right");
    }

    // Meida file path
    string mediaFilePath = filePath;
    size_t pos = mediaFilePath.find(META_RECOVERY_META_RELATIVE_PATH);
    if (pos != string::npos) {
        mediaFilePath.replace(pos, META_RECOVERY_META_RELATIVE_PATH.length(), META_RECOVERY_PHOTO_RELATIVE_PATH);
    }
    if (MediaStringUtils::EndsWith(mediaFilePath, META_RECOVERY_META_FILE_SUFFIX)) {
        mediaFilePath.erase(mediaFilePath.length() - META_RECOVERY_META_FILE_SUFFIX.length());
    }
    fileAsset.SetFilePath(mediaFilePath);

    struct stat statInfo = { 0 };
    if (stat(mediaFilePath.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("ReadMetadataFromFile: stat syscall err %{public}d", errno);
        ret = E_SYSCALL;
        if (errno == ENOENT) {
            remove(filePath.c_str());
        }
    }

    return ret;
}

void MediaLibraryMetaRecovery::AddPhotoAlbumToJson(nlohmann::json &j, const PhotoAlbum &photoAlbum)
{
    j = json {
        {PhotoAlbumColumns::ALBUM_ID, json::number_integer_t(photoAlbum.GetAlbumId())},
        {PhotoAlbumColumns::ALBUM_TYPE, json::number_integer_t(photoAlbum.GetPhotoAlbumType())},
        {PhotoAlbumColumns::ALBUM_SUBTYPE, json::number_integer_t(photoAlbum.GetPhotoAlbumSubType())},
        {PhotoAlbumColumns::ALBUM_NAME, json::string_t(photoAlbum.GetAlbumName())},
        {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, json::number_integer_t(photoAlbum.GetDateModified())},
        {PhotoAlbumColumns::CONTAINS_HIDDEN, json::number_integer_t(photoAlbum.GetContainsHidden())},
        {PhotoAlbumColumns::ALBUM_ORDER, json::number_integer_t(photoAlbum.GetOrder())},
        {PhotoAlbumColumns::ALBUM_BUNDLE_NAME, json::string_t(photoAlbum.GetBundleName())},
        {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, json::string_t(photoAlbum.GetLocalLanguage())},
        {PhotoAlbumColumns::ALBUM_DATE_ADDED, json::number_integer_t(photoAlbum.GetDateAdded())},
        {PhotoAlbumColumns::ALBUM_IS_LOCAL, json::number_integer_t(photoAlbum.GetIsLocal())},
        {PhotoAlbumColumns::ALBUM_LPATH, json::string_t(photoAlbum.GetLPath())},
        {PhotoAlbumColumns::ALBUM_PRIORITY, json::number_integer_t(photoAlbum.GetPriority())},
        {PhotoAlbumColumns::UPLOAD_STATUS, json::number_integer_t(photoAlbum.GetUploadStatus())}
    };
}

bool MediaLibraryMetaRecovery::GetPhotoAlbumFromJson(const nlohmann::json &j, PhotoAlbum &photoAlbum)
{
    bool ret = true;
    optional<int64_t> albumId = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_ID);
    if (albumId.has_value()) {
        photoAlbum.SetAlbumId((int32_t)albumId.value());
    } else {
        ret = false;
    }

    optional<int64_t> albumType = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_TYPE);
    if (albumType.has_value()) {
        int32_t type = (int32_t)albumType.value();
        photoAlbum.SetPhotoAlbumType(static_cast<PhotoAlbumType>(type));
    } else {
        ret = false;
    }

    optional<int64_t> albumSubType = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_SUBTYPE);
    if (albumSubType.has_value()) {
        int32_t type = (int32_t)albumSubType.value();
        photoAlbum.SetPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(type));
    } else {
        ret = false;
    }

    optional<string> albumName = GetStringFromJson(j, PhotoAlbumColumns::ALBUM_NAME);
    if (albumName.has_value()) {
        photoAlbum.SetAlbumName(albumName.value());
    } else {
        ret = false;
    }

    optional<int64_t> dateModified = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
    if (dateModified.has_value()) {
        photoAlbum.SetDateModified(dateModified.value());
    } else {
        ret = false;
    }

    optional<int64_t> containsHidden = GetNumberFromJson(j, PhotoAlbumColumns::CONTAINS_HIDDEN);
    if (containsHidden.has_value()) {
        photoAlbum.SetContainsHidden((int32_t)containsHidden.value());
    } else {
        ret = false;
    }

    optional<int64_t> order = GetNumberFromJson(j, PhotoAlbumColumns::ALBUM_ORDER);
    if (order.has_value()) {
        photoAlbum.SetOrder((int32_t)order.value());
    } else {
        ret = false;
    }

    return (ret && GetPhotoAlbumFromJsonPart1(j, photoAlbum));
    // !! Do not add upgrade code here !!
}

int32_t MediaLibraryMetaRecovery::WritePhotoAlbumToFile(const string &filePath,
                                                        const vector<shared_ptr<PhotoAlbum>> &vecPhotoAlbum)
{
    MEDIA_DEBUG_LOG("WritePhotoAlbumToFile start\n");

    const string parentDir = MediaFileUtils::GetParentPath(filePath);
    if (!MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("CreateDirectory failed, dir = %{public}s", DfxUtils::GetSafePath(parentDir).c_str());
        return false;
    }

    json jsonArray = json::array();
    for (auto &album : vecPhotoAlbum) {
        if (album == nullptr) {
            MEDIA_ERR_LOG("album == nullptr");
            continue;
        }

        json jsonPhotoAlbumItem;
        OHOS::Media::PhotoAlbum &photoAlbumRef = *album.get();
        AddPhotoAlbumToJson(jsonPhotoAlbumItem, photoAlbumRef);

        jsonArray.push_back(jsonPhotoAlbumItem);
    }

    if (!WriteJsonFile(filePath, jsonArray)) {
        MEDIA_ERR_LOG("WriteJsonFile failed");
        return E_FILE_OPER_FAIL;
    }

    MEDIA_DEBUG_LOG("WritePhotoAlbumToFile end\n");
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::ReadPhotoAlbumFromFile(const string &filePath,
                                                         vector<shared_ptr<PhotoAlbum>> &photoAlbumVector)
{
    json jsonArray;
    if (!ReadJsonFile(filePath, jsonArray)) {
        MEDIA_ERR_LOG("ReadJsonFile failed");
        return E_FILE_OPER_FAIL;
    }
    if (!jsonArray.is_array()) {
        MEDIA_ERR_LOG("json not is array");
        return E_ERR;
    }

    int ret = E_OK;
    for (const json &j : jsonArray) {
        PhotoAlbum photoAlbum;
        if (!GetPhotoAlbumFromJson(j, photoAlbum)) {
            MEDIA_WARN_LOG("GetPhotoAlbumFromJson failed");
            ret = E_ERR;
        }
        if (photoAlbum.GetPhotoAlbumSubType() != 1 && photoAlbum.GetLPath() == "") {
            continue;
        }
        photoAlbumVector.emplace_back(make_shared<PhotoAlbum>(photoAlbum));
    }

    return ret;
}

int32_t MediaLibraryMetaRecovery::WriteMetaStatusToFile(const string &keyPath, const int32_t status)
{
    json j;
    if (!ReadJsonFile(META_STATUS_PATH, j)) {
        MEDIA_WARN_LOG("ReadFile META_STATUS_PATH failed, will write new META_STATUS_PATH file");
        j = json::object();
    }

    j[keyPath] = json::number_integer_t(status);

    if (!WriteJsonFile(META_STATUS_PATH, j)) {
        MEDIA_ERR_LOG("WriteJsonFile failed");
        return E_FILE_OPER_FAIL;
    }

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::ReadMetaStatusFromFile(set<int32_t> &status)
{
    json j;
    if (!ReadJsonFile(META_STATUS_PATH, j)) {
        MEDIA_ERR_LOG("ReadFile META_STATUS_PATH failed");
        return E_FILE_OPER_FAIL;
    }

    for (const auto& [key, value] : j.items()) {
        if (!value.is_number_integer()) {
            MEDIA_ERR_LOG("key: %{public}s not is number", key.c_str());
            continue;
        }
        // Read bucket_id which finish recovery
        int32_t val = atoi(key.c_str());
        MEDIA_INFO_LOG("finish recovery bucket_id: %{public}s", key.c_str());
        status.insert(val);
    }

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::ReadMetaRecoveryCountFromFile()
{
    json j;
    int32_t recoverySuccessCnt = 0;

    if (!ReadJsonFile(META_STATUS_PATH, j)) {
        MEDIA_ERR_LOG("ReadFile META_STATUS_PATH failed");
        return E_FILE_OPER_FAIL;
    }

    for (const auto& [key, value] : j.items()) {
        if (!value.is_number_integer()) {
            MEDIA_ERR_LOG("key: %{public}s not is number", key.c_str());
            continue;
        }
        int32_t val = value.get<int32_t>();
        recoverySuccessCnt += val;
        MEDIA_INFO_LOG("finish recovery bucket_id: %{public}s, recovery success count=%{public}d", key.c_str(), val);
    }

    return recoverySuccessCnt;
}

bool MediaLibraryMetaRecovery::UpdatePhotoOwnerAlbumId(NativeRdb::ValuesBucket &values)
{
    NativeRdb::ValueObject valueObject;
    if (!values.GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, valueObject)) {
        return false;
    }

    int32_t oldOwnerAlbumId = 0;
    valueObject.GetInt(oldOwnerAlbumId);
    if (oldAlbumIdToLpath.find(oldOwnerAlbumId) == oldAlbumIdToLpath.end()) {
        return false;
    }

    const std::string &lpath = oldAlbumIdToLpath[oldOwnerAlbumId];
    if (lpathToNewAlbumId.end() == lpathToNewAlbumId.find(lpath)) {
        return false;
    }

    int32_t newOwnerAlbumId = lpathToNewAlbumId[lpath];
    if (newOwnerAlbumId == oldOwnerAlbumId) {
        return false;
    }

    MEDIA_DEBUG_LOG("convert album %{public}d to %{public}d", oldOwnerAlbumId, newOwnerAlbumId);
    values.Delete(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, newOwnerAlbumId);

    return true;
}

int32_t MediaLibraryMetaRecovery::InsertMetadataInDbRetry(const FileAsset &fileAsset)
{
    int32_t retry_cnt = 0;
    do {
        if (InsertMetadataInDb(fileAsset) == E_OK) {
            break;
        }

        retry_cnt++;
        MEDIA_ERR_LOG("InsertMetadataInDb failed, retry_cnt = %{public}d", retry_cnt);
        this_thread::sleep_for(chrono::milliseconds(META_RETRY_INTERVAL));
    } while (retry_cnt < META_RETRY_MAX_COUNTS);

    if (retry_cnt >= META_RETRY_MAX_COUNTS) {
        MEDIA_ERR_LOG("InsertMetadataInDb finally failed, retry_cnt = %{public}d", retry_cnt);
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, E_DB_FAIL},
            {KEY_OPT_TYPE, OptType::CREATE}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::RECOVERY_ERR, map);
        return ERR_FAIL;
    }

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::InsertMetadataInDb(const FileAsset &fileAsset)
{
    std::string filePath = fileAsset.GetFilePath();
    MEDIA_DEBUG_LOG("InsertMetadataInDb: photo filepath = %{public}s", DfxUtils::GetSafePath(filePath).c_str());
    if (MediaLibraryAssetOperations::CheckExist(filePath) == E_OK) {
        MEDIA_DEBUG_LOG("InsertMetadataInDb: insert: photo is exist in db, ignore");
        return E_OK;
    }

    std::unordered_map<std::string, ResultSetDataType> columnInfoMap = QueryRecoveryPhotosTableColumnInfo();
    if (columnInfoMap.empty()) {
        MEDIA_ERR_LOG("QueryRecoveryPhotosTableColumnInfo failed");
        return E_ERR;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("GetRdbStore failed, return nullptr");
        return E_HAS_DB_ERROR;
    }

    NativeRdb::ValuesBucket valuesBucket;
    SetValuesFromFileAsset(fileAsset, valuesBucket, columnInfoMap);

    // set meta flags uptodate, to avoid backup db into meta again
    if (UpdatePhotoOwnerAlbumId(valuesBucket)) {
        valuesBucket.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_DIRTY));
    } else {
        valuesBucket.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_UPTODATE));
    }

    int64_t outRowId = -1;
    int32_t errCode = rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert photo failed, errCode = %{public}d", errCode);
        return errCode;
    }

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::InsertMetadataInDb(const std::vector<shared_ptr<PhotoAlbum>> &vecPhotoAlbum)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "GetRdbStore failed, return nullptr)");

    for (auto iter : vecPhotoAlbum) {
        MEDIA_INFO_LOG("InsertMetadataInDb: album name = %{private}s", iter->GetAlbumName().c_str());
        NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        vector<string> columns = {PhotoAlbumColumns::ALBUM_ID,
            PhotoAlbumColumns::ALBUM_LPATH};
        predicates.IsNotNull(PhotoAlbumColumns::ALBUM_LPATH)->And()->EqualTo(PhotoAlbumColumns::ALBUM_LPATH,
                             iter->GetLPath());
        auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("resultSet == nullptr)");
            continue;
        }
        if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            MEDIA_ERR_LOG("skip duplicate lpath %{public}s", iter->GetLPath().c_str());
            continue;
        }
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        int32_t errCode = NativeRdb::E_OK;
        std::function<int(void)> func = [&]()->int {
            // Insert album item
            NativeRdb::ValuesBucket valuesBucket;
            SetValuesFromPhotoAlbum(iter, valuesBucket);
            int64_t outRowId = -1;
            errCode = trans->Insert(outRowId, PhotoAlbumColumns::TABLE, valuesBucket);
            if (errCode != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("InsertMetadataInDb: insert album failed, errCode = %{public}d", errCode);
                return errCode;
            }

            // Update album order inserted just now
            int32_t changedRows = -1;
            NativeRdb::RdbPredicates predicatesOrder(PhotoAlbumColumns::TABLE);
            predicatesOrder.And()->EqualTo(PhotoAlbumColumns::ALBUM_ID, outRowId)
                                 ->And()
                                 ->NotEqualTo(PhotoAlbumColumns::ALBUM_ORDER, iter->GetOrder());
            valuesBucket.Clear();
            valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_ORDER, iter->GetOrder());
            errCode = trans->Update(changedRows, valuesBucket, predicatesOrder);
            if (errCode != E_OK) {
                MEDIA_ERR_LOG("Update album order failed, err = %{public}d", errCode);
                return E_HAS_DB_ERROR;
            }
            if (changedRows > 0) {
                MEDIA_INFO_LOG("Update album order");
            }
            return errCode;
        };
        errCode = trans->RetryTrans(func);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("InsertMetadataInDb: trans retry fail!, ret:%{public}d", errCode);
            return errCode;
        }
    }

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::UpdateMetadataFlagInDb(const int32_t fieldId, const MetadataFlags &flag)
{
    int32_t errCode = E_OK;

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (!rdbStore) {
        MEDIA_ERR_LOG("GetRdbStore failed");
        return E_HAS_DB_ERROR;
    }

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fieldId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(flag));

    int32_t changedRows = -1;
    errCode = rdbStore->Update(changedRows, values, predicates);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Database update failed, err = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::SetRdbRebuiltStatus(bool status)
{
    rdbRebuilt_ = status;
    reBuiltCount_++;
    return E_OK;
}

int32_t MediaLibraryMetaRecovery::StartAsyncRecovery()
{
    StatisticRestore();

    MediaLibraryMetaRecoveryState oldState = recoveryState_.exchange(MediaLibraryMetaRecoveryState::STATE_RECOVERING);
    if (oldState == MediaLibraryMetaRecoveryState::STATE_RECOVERING) {
        MEDIA_INFO_LOG("recovery process is already running");
        return E_OK;
    }

    if (oldState == MediaLibraryMetaRecoveryState::STATE_NONE) {
        bool hasStatusFile = bool(access(META_STATUS_PATH.c_str(), F_OK) == E_OK);
        MEDIA_INFO_LOG("rebuild status %{public}d, has status file %{public}d", rdbRebuilt_, hasStatusFile);
        if (!hasStatusFile && !rdbRebuilt_) {
            MEDIA_INFO_LOG("StartAsyncRecovery: no need to recovery");
            recoveryState_.exchange(MediaLibraryMetaRecoveryState::STATE_NONE);
            return E_OK;
        }
        oldState = hasStatusFile ? MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT :
                                    MediaLibraryMetaRecoveryState::STATE_NONE;
    }

    if (oldState == MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT) {
        ReadMetaStatusFromFile(metaStatus);
    } else {
        // Create status.json if not exist
        const string parentDir = MediaFileUtils::GetParentPath(META_STATUS_PATH);
        if (MediaFileUtils::CreateDirectory(parentDir)) {
            MediaFileUtils::CreateFile(META_STATUS_PATH);
        } else {
            MEDIA_ERR_LOG("CreateDirectory failed, dir = %{public}s", DfxUtils::GetSafePath(parentDir).c_str());
        }
    }

    std::thread([this]() {
        MEDIA_INFO_LOG("Start recovery");
        int64_t recoveryStartTime = MediaFileUtils::UTCTimeMilliSeconds();
        this->DoDataBaseRecovery();
        int64_t recoveryTotalTime = MediaFileUtils::UTCTimeMilliSeconds() - recoveryStartTime;
        bool isStatusFileExist = bool(access(META_STATUS_PATH.c_str(), F_OK) == E_OK);
        if (isStatusFileExist) {
            recoveryState_.exchange(MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT);
        } else {
            recoveryState_.exchange(MediaLibraryMetaRecoveryState::STATE_NONE);
        }
        recoveryCostTime_ += recoveryTotalTime;
        RecoveryStatistic();
    }).detach();

    return E_OK;
}

int32_t MediaLibraryMetaRecovery::DeleteMetaDataByPath(const string &filePath)
{
    string metaFilePath;
    if (PhotoFileUtils::GetMetaPathFromOrignalPath(filePath, metaFilePath) != E_OK) {
        MEDIA_ERR_LOG("DeleteMetaDataByPath: invalid photo filePath, %{public}s",
            DfxUtils::GetSafePath(filePath).c_str());
        return E_INVALID_PATH;
    }

    if (remove(metaFilePath.c_str()) != 0 && errno != ENOENT) {
        MEDIA_ERR_LOG("remove metafile failed %{public}d, path %s", errno, DfxUtils::GetSafePath(metaFilePath).c_str());
    }

    MEDIA_INFO_LOG("DeleteMetaDataByPath: metafile removed successful, %{public}s",
        DfxUtils::GetSafePath(metaFilePath).c_str());
    return E_OK;
}

void MediaLibraryMetaRecovery::StopCloudSync()
{
    MEDIA_INFO_LOG("Begin StopCloudSync");
    FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync(CONST_BUNDLE_NAME, true);
}

void MediaLibraryMetaRecovery::RestartCloudSync()
{
    MEDIA_INFO_LOG("Begin reset cloud cursor");
    static uint32_t baseUserRange = 200000; // uid base offset
    uid_t uid = getuid() / baseUserRange;
    FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor();

    int32_t ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().StartSync(CONST_BUNDLE_NAME);
    if (ret != 0) {
        MEDIA_ERR_LOG("StartCloudSync fail, errcode=%{public}d", ret);
    }
    MEDIA_INFO_LOG("End StartCloudSync");
}

ResultSetDataType MediaLibraryMetaRecovery::GetDataType(const std::string &name)
{
    auto it = GetFileAssetMemberMap().find(name);
    if (it == GetFileAssetMemberMap().end()) {
        MEDIA_ERR_LOG("FILEASSET_MEMBER_MAP not find name: %{public}s", name.c_str());
        return TYPE_NULL;
    }

    switch (it->second) {
        case MEMBER_TYPE_INT32: {
            return TYPE_INT32;
            break;
        }
        case MEMBER_TYPE_INT64: {
            return TYPE_INT64;
            break;
        }
        case MEMBER_TYPE_STRING: {
            return TYPE_STRING;
            break;
        }
        case MEMBER_TYPE_DOUBLE: {
            return TYPE_DOUBLE;
            break;
        }
        default: {
            return TYPE_NULL;
            break;
        }
    }
}

std::unordered_map<std::string, ResultSetDataType> MediaLibraryMetaRecovery::QueryRecoveryPhotosTableColumnInfo()
{
    MEDIA_DEBUG_LOG("QueryRecoveryPhotosTableColumnInfo");
    std::unordered_map<std::string, ResultSetDataType> columnInfoMap;
    std::vector<std::string> columnInfo = MediaLibraryAssetOperations::QueryPhotosTableColumnInfo();
    if (columnInfo.empty()) {
        MEDIA_ERR_LOG("GetPhotosTableColumnInfo failed");
        return columnInfoMap;
    }

    for (const std::string &name : columnInfo) {
        if (EXCLUDED_COLUMNS.count(name) > 0) {
            continue;
        }
        ResultSetDataType type = GetDataType(name);
        columnInfoMap.emplace(name, type);
        MEDIA_DEBUG_LOG("photos table name: %{public}s, type: %{public}d", name.c_str(), type);
    }

    return columnInfoMap;
}

int32_t MediaLibraryMetaRecovery::ResetAllMetaDirty()
{
    const std::string RESET_ALL_META_DIRTY_SQL =
        " UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_METADATA_FLAGS +
        " = 0 " + " WHERE " + PhotoColumn::PHOTO_METADATA_FLAGS + " == 2; END;";

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t err = rdbStore->ExecuteSql(RESET_ALL_META_DIRTY_SQL);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", RESET_ALL_META_DIRTY_SQL.c_str());
    }
    return err;
}

static int32_t QueryInt(const NativeRdb::AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, const std::string &queryColumn, int32_t &value)
{
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return E_DB_FAIL;
    }
    value = GetInt32Val(queryColumn, resultSet);
    return E_OK;
}

static int32_t QueryAllPhoto(bool backup)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.And()->BeginWrap()->EqualTo(PhotoColumn::PHOTO_POSITION, "1")->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, "3")->EndWrap();

    if (backup) {
        predicates.BeginWrap()
                  ->EqualTo(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_NEW))
                  ->Or()
                  ->EqualTo(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int>(MetadataFlags::TYPE_DIRTY))
                  ->Or()
                  ->IsNull(PhotoColumn::PHOTO_METADATA_FLAGS)
                  ->EndWrap();
    }

    std::vector<std::string> columns = { "count(1) AS count" };
    std::string queryColumn = "count";
    int32_t count;
    int32_t errCode = QueryInt(predicates, columns, queryColumn, count);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("query local image fail: %{public}d", errCode);
    }
    return count;
}

void MediaLibraryMetaRecovery::StatisticSave()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(BACKUP_PHOTO_COUNT, backupSuccCnt_);
    prefs->PutLong(BACKUP_COST_TIME, backupCostTime_);
    prefs->PutLong(REBUILT_COUNT, reBuiltCount_);
    prefs->PutLong(RECOVERY_BACKUP_TOTAL_COUNT, recoveryTotalBackupCnt_);
    prefs->PutLong(RECOVERY_SUCC_PHOTO_COUNT, recoverySuccCnt_);
    prefs->PutLong(RECOVERY_COST_TIME, recoveryCostTime_);
    prefs->FlushSync();
}

void MediaLibraryMetaRecovery::StatisticRestore()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(RDB_CONFIG, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    backupSuccCnt_ = prefs->GetLong(BACKUP_PHOTO_COUNT, 0);
    backupCostTime_ = prefs->GetLong(BACKUP_COST_TIME, 0);
    reBuiltCount_ += prefs->GetLong(REBUILT_COUNT, 0);  // reBuiltCount_ will be set before Restore
    recoveryTotalBackupCnt_ = prefs->GetLong(RECOVERY_BACKUP_TOTAL_COUNT, 0);
    recoverySuccCnt_ = prefs->GetLong(RECOVERY_SUCC_PHOTO_COUNT, 0);
    recoveryCostTime_ = prefs->GetLong(RECOVERY_COST_TIME, 0);
}

void MediaLibraryMetaRecovery::StatisticReset()
{
    backupSuccCnt_ = 0;
    reBuiltCount_ = 0;
    backupCostTime_ = 0;
    recoverySuccCnt_ = 0;
    recoveryCostTime_ = 0;
    recoveryTotalBackupCnt_ = 0;
    StatisticSave();
}

void MediaLibraryMetaRecovery::RecoveryStatistic()
{
    static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
    int64_t totalPhotoCount = QueryAllPhoto(false);
    int64_t totalbackupCount = GetTotalBackupFileCount();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_META_RECOVERY_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "TOTAL_PHOTO_COUNT", totalPhotoCount,
        "TOTAL_BACKUP_COUNT", totalbackupCount,
        "BACKUP_PHOTO_COUNT", backupSuccCnt_,
        "BACKUP_COST_TIME", backupCostTime_,
        "REBUILT_COUNT", reBuiltCount_,
        "RECOVERY_BACKUP_TOTAL_COUNT", recoveryTotalBackupCnt_,
        "RECOVERY_SUCC_PHOTO_COUNT", recoverySuccCnt_,
        "RECOVERY_COST_TIME", recoveryCostTime_);
    if (ret != 0) {
        MEDIA_ERR_LOG("RecoveryStatistic error:%{public}d", ret);
    }
    StatisticReset();
}
} // namespace Media
} // namespace OHOS
