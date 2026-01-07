/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryOthersCloneRestore"

#include "others_clone_restore.h"

#include <securec.h>
#include <dirent.h>
#include <regex>
#include <charconv>

#include "album_plugin_config.h"
#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "datashare_abs_result_set.h"
#include "database_report.h"
#include "directory_ex.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_scanner.h"
#include "medialibrary_rdb_transaction.h"
#include "os_account_manager.h"
#include "upgrade_restore_task_report.h"

namespace OHOS {
namespace Media {
const int BASE32_CHAR_BITS = 5;
const int BASE32_BYTE_BITS = 8;
const int PRE_ALLOC_PHOTOMAP_VECTOR = 100000;
const int PRE_ALLOC_AUDIOMAP_VECTOR = 10000;
const int PRE_ALLOC_PHOTO_VECTOR = 10000;
const int PRE_ALLOC_AUDIO_VECTOR = 1000;
const int BATCHES_CHUNK_NUMBER = 5000;
const int PHONE_FIRST_NUMBER = 105;
const int PHONE_SECOND_NUMBER = 80;
const int PHONE_THIRD_NUMBER = 104;
const int PHONE_FOURTH_NUMBER = 111;
const int PHONE_FIFTH_NUMBER = 110;
const int PHONE_SIXTH_NUMBER = 101;
const int QUERY_NUMBER = 200;
const int STRONG_ASSOCIATION_ENABLE = 1;
constexpr int32_t MAX_CLONE_THREAD_NUM = 2;
const int32_t I_PHONE_DYNAMIC_VIDEO_TYPE = 13;
constexpr int64_t SECONDS_LEVEL_LIMIT = 1e10;
const size_t EXTRADATA_LEN = 60;
const std::string I_PHONE_LPATH = "/Pictures/";
const std::string PHONE_TYPE = "type";
const std::string PHONE_DEVICE_TYPE = "deviceType";
const std::string PHONE_DETAIL = "detail";
const std::string PHOTO_DB_NAME = "photo_MediaInfo.db";
const std::string PHOTO_SD_MEDIA_INFO_DB_NAME = "photo_sd_MediaInfo.db";
const std::string VIDEO_DB_NAME = "video_MediaInfo.db";
const std::string VIDEO_SD_MEDIA_INFO_DB_NAME = "video_sd_MediaInfo.db";
const std::string OTHER_CLONE_FILE_ROOT_PATH = "/storage/media/local/files/.backup/restore";
const std::string LITE_CLONE_SD_FILE_PATH = "/storage/media/local/files/.backup/restore/storage/";
const std::string OTHER_CLONE_DB_PATH = "/storage/media/local/files/.backup/restore/storage/emulated/";
const std::string I_PHONE_IMAGE_FILE_PATH = "/storage/media/local/files/.backup/restore/";
const std::string I_PHONE_DYNAMIC_IMAGE = "_DYNAMIC";
const std::string I_PHONE_DYNAMIC_VIDEO = "_DYNAMIC.MOV";
const std::string I_PHONE_VIDEO_FILE_PATH = "/storage/media/local/files/.backup/restore/storage/emulated/";
const std::string OTHER_CLONE_FILE_PATH = "/storage/media/local/files/.backup/restore/storage/emulated/";
const std::string OTHER_CLONE_DISPLAYNAME = "primaryStr";
const std::string OTHER_CLONE_DATA = "_data";
const std::string OTHER_CLONE_MODIFIED = "date_modified";
const std::string OTHER_CLONE_TAKEN = "datetaken";
const std::string OTHER_CLONE_LATITUDE = "latitude";
const std::string OTHER_CLONE_LONGITUDE = "longitude";
const std::string OTHER_MUSIC_ROOT_PATH = "/storage/emulated/0/";

static constexpr uint32_t CHAR_ARRAY_LENGTH = 5;
static constexpr uint32_t ASCII_CHAR_LENGTH = 8;
static constexpr uint32_t DECODE_NAME_IDX = 4;
static constexpr uint32_t DECODE_SURFIX_IDX = 5;
static constexpr uint32_t DECODE_TIME_IDX = 3;
static constexpr uint32_t IOS_BURST_CODE_LEN = 19;

static std::string GetPhoneName()
{
    int arr[] = { PHONE_FIRST_NUMBER, PHONE_SECOND_NUMBER, PHONE_THIRD_NUMBER, PHONE_FOURTH_NUMBER, PHONE_FIFTH_NUMBER,
        PHONE_SIXTH_NUMBER };
    int len = sizeof(arr) / sizeof(arr[0]);
    std::string phoneName = "";
    for (int i = 0; i < len; i++) {
        phoneName += static_cast<char>(arr[i]);
    }
    return phoneName;
}

OthersCloneRestore::OthersCloneRestore(int32_t sceneCode, const std::string &mediaAppName,
    const std::string &bundleInfo)
{
    sceneCode_ = sceneCode;
    mediaAppName_ = mediaAppName;
    if (sceneCode_ == I_PHONE_CLONE_RESTORE) {
        nlohmann::json jsonObj = nlohmann::json::parse(bundleInfo, nullptr, false);
        if (jsonObj.is_discarded()) {
            MEDIA_ERR_LOG("parse json failed");
            clonePhoneName_ = GetPhoneName();
            return;
        }
        for (auto &obj : jsonObj) {
            if (obj.contains(PHONE_TYPE) && obj.at(PHONE_TYPE) == PHONE_DEVICE_TYPE && obj.contains(PHONE_DETAIL) &&
                obj.at(PHONE_DETAIL).is_string()) {
                clonePhoneName_ = obj.at(PHONE_DETAIL);
            }
        }
        if (clonePhoneName_.empty()) {
            MEDIA_ERR_LOG("read json error");
            clonePhoneName_ = GetPhoneName();
        }
    }
    ffrt_disable_worker_escape();
    MEDIA_INFO_LOG("Set ffrt_disable_worker_escape");
}

void OthersCloneRestore::CloneInfoPushBack(std::vector<CloneDbInfo> &pushInfos, std::vector<CloneDbInfo> &popInfos)
{
    std::lock_guard<std::mutex> guard(cloneMutex_);
    for (auto &info : popInfos) {
        pushInfos.push_back(info);
    }
}

void OthersCloneRestore::GetDbInfo(int32_t sceneCode, std::vector<CloneDbInfo> &mediaDbInfo,
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    std::vector<CloneDbInfo> infos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        CloneDbInfo tmpDbInfo;
        tmpDbInfo.data = GetStringVal(OTHER_CLONE_DATA, resultSet);
        if (sceneCode == I_PHONE_CLONE_RESTORE) {
            tmpDbInfo.displayName = GetStringVal(OTHER_CLONE_DISPLAYNAME, resultSet);
            tmpDbInfo.dateModified = GetDoubleVal(OTHER_CLONE_MODIFIED, resultSet);
            tmpDbInfo.dateTaken = GetDoubleVal(OTHER_CLONE_TAKEN, resultSet);
            tmpDbInfo.latitude = GetDoubleVal(OTHER_CLONE_LATITUDE, resultSet);
            tmpDbInfo.longitude = GetDoubleVal(OTHER_CLONE_LONGITUDE, resultSet);
        } else {
            tmpDbInfo.dateModified = static_cast<double>(GetInt64Val(OTHER_CLONE_MODIFIED, resultSet));
            tmpDbInfo.dateTaken = static_cast<double>(GetInt64Val(OTHER_CLONE_TAKEN, resultSet));
        }
        infos.push_back(tmpDbInfo);
    };
    CloneInfoPushBack(mediaDbInfo, infos);
}

void OthersCloneRestore::HandleSelectBatch(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t offset,
    int32_t sceneCode, std::vector<CloneDbInfo> &mediaDbInfo)
{
    if (mediaRdb == nullptr) {
        MEDIA_ERR_LOG("rdb is nullptr, Maybe init failed.");
        return;
    }

    const int32_t BATCH_SIZE = 1000;
    std::string queryExternalMayClonePhoto;

    if (sceneCode == I_PHONE_CLONE_RESTORE) {
        queryExternalMayClonePhoto =
            "SELECT _data, primaryStr, latitude, longitude, date_modified, datetaken FROM mediainfo LIMIT " +
            std::to_string(offset) + ", " + std::to_string(BATCH_SIZE);
    } else if (sceneCode == LITE_PHONE_CLONE_RESTORE) {
        queryExternalMayClonePhoto = "SELECT _data, date_modified, datetaken FROM mediainfo LIMIT " +
            std::to_string(offset) + ", " + std::to_string(BATCH_SIZE);
    } else {
        queryExternalMayClonePhoto = "SELECT _data, date_modified, datetaken FROM mediainfo LIMIT " +
            std::to_string(offset) + ", " + std::to_string(BATCH_SIZE);
    }

    auto resultSet = mediaRdb->QuerySql(queryExternalMayClonePhoto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return;
    }

    std::vector<CloneDbInfo> batchResults;
    batchResults.reserve(BATCH_SIZE);
    GetDbInfo(sceneCode, batchResults, resultSet);
    CloneInfoPushBack(mediaDbInfo, batchResults);
    resultSet->Close();
}

std::string OthersCloneRestore::BuildDbPath(const std::string &dbName)
{
    if (dbName == PHOTO_SD_MEDIA_INFO_DB_NAME || dbName == VIDEO_SD_MEDIA_INFO_DB_NAME) {
        return OTHER_CLONE_FILE_ROOT_PATH + "/" + dbName;
    }
    return OTHER_CLONE_DB_PATH + std::to_string(userId_) + "/" + dbName;
}

bool OthersCloneRestore::CheckDbExists(const std::string &dbPath, const std::string &dbName)
{
    if (access(dbPath.c_str(), F_OK) != 0) {
        MEDIA_ERR_LOG("Init rdb not exists, dbName = %{public}s", dbName.c_str());
        return false;
    }
    return true;
}

std::shared_ptr<NativeRdb::RdbStore> OthersCloneRestore::InitializeDatabase(
    const std::string &dbName, const std::string &dbPath)
{
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb;
    int32_t initErr = BackupDatabaseUtils::InitDb(mediaRdb, dbName, dbPath, mediaAppName_, false);
    if (mediaRdb == nullptr) {
        MEDIA_ERR_LOG("Init rdb fail, dbName = %{public}s, err = %{public}d", dbName.c_str(), initErr);
        return nullptr;
    }

    return mediaRdb;
}

int32_t OthersCloneRestore::GetTotalRecordCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    const std::string &dbName)
{
    std::string selectTotalCloneMediaNumber = "SELECT count(1) AS count FROM mediainfo";
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(mediaRdb, selectTotalCloneMediaNumber, CUSTOM_COUNT);
    MEDIA_INFO_LOG("dbName = %{public}s, totalNumber = %{public}d", dbName.c_str(), totalNumber);
    return totalNumber;
}

void OthersCloneRestore::ProcessBatch(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t offset,
    std::vector<CloneDbInfo> &mediaDbInfo)
{
    std::vector<CloneDbInfo> batchResults;
    HandleSelectBatch(mediaRdb, offset, sceneCode_, batchResults);
    CloneInfoPushBack(mediaDbInfo, batchResults);
    batchResults.clear();
    batchResults.shrink_to_fit();
}

void OthersCloneRestore::ProcessChunk(std::shared_ptr<NativeRdb::RdbStore> mediaRdb, int32_t chunkStart,
    int32_t chunkEnd, std::vector<CloneDbInfo> &mediaDbInfo)
{
    const int32_t BATCH_SIZE = 1000;
    int32_t batchesInChunk = (chunkEnd - chunkStart + BATCH_SIZE - 1) / BATCH_SIZE;
    for (int32_t batchIdx = 0; batchIdx < batchesInChunk; ++batchIdx) {
        int32_t offset = chunkStart + batchIdx * BATCH_SIZE;
        ProcessBatch(mediaRdb, offset, mediaDbInfo);
    }
}

void OthersCloneRestore::QueryDatabaseRecords(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
    int32_t totalNumber, std::vector<CloneDbInfo> &mediaDbInfo)
{
    const int32_t CHUNK_SIZE = 10000;
    for (int32_t chunkStart = 0; chunkStart < totalNumber; chunkStart += CHUNK_SIZE) {
        int32_t chunkEnd = std::min(chunkStart + CHUNK_SIZE, totalNumber);
        ProcessChunk(mediaRdb, chunkStart, chunkEnd, mediaDbInfo);
        if (chunkStart % BATCHES_CHUNK_NUMBER == 0) {
            MEDIA_INFO_LOG("Processed chunk %{public}d/%{public}d, current restore: %{public}zu records",
                chunkStart / CHUNK_SIZE + 1,
                (totalNumber + CHUNK_SIZE - 1) / CHUNK_SIZE,
                mediaDbInfo.size());
        }
    }
}

bool OthersCloneRestore::IsPhotoVideoDatabase(const std::string &dbName)
{
    return (dbName == PHOTO_DB_NAME || dbName == PHOTO_SD_MEDIA_INFO_DB_NAME ||
            dbName == VIDEO_DB_NAME || dbName == VIDEO_SD_MEDIA_INFO_DB_NAME);
}

void OthersCloneRestore::BuildPhotoVideoDbMap(std::vector<CloneDbInfo> &mediaDbInfo)
{
    photoDbMap_.clear();
    size_t reserveSize = std::min(mediaDbInfo.size(), static_cast<size_t>(PRE_ALLOC_PHOTOMAP_VECTOR));
    photoDbMap_.reserve(reserveSize);

    for (auto& info : mediaDbInfo) {
        photoDbMap_[info.data] = &info;
        if (!info.displayName.empty()) {
            photoDbMap_[info.displayName] = &info;
        }
    }
}

void OthersCloneRestore::BuildAudioDbMap(std::vector<CloneDbInfo> &mediaDbInfo)
{
    audioDbMap_.clear();
    size_t reserveSize = std::min(mediaDbInfo.size(), static_cast<size_t>(PRE_ALLOC_AUDIOMAP_VECTOR));
    audioDbMap_.reserve(reserveSize);

    for (auto& info : mediaDbInfo) {
        audioDbMap_[info.data] = &info;
        if (!info.displayName.empty()) {
            audioDbMap_[info.displayName] = &info;
        }
    }
}

void OthersCloneRestore::BuildDbInfoMap(const std::string &dbName, std::vector<CloneDbInfo> &mediaDbInfo)
{
    if (IsPhotoVideoDatabase(dbName)) {
        BuildPhotoVideoDbMap(mediaDbInfo);
    } else if (dbName == AUDIO_DB_NAME) {
        BuildAudioDbMap(mediaDbInfo);
    }
}

void OthersCloneRestore::GetCloneDbInfos(const std::string &dbName, std::vector<CloneDbInfo> &mediaDbInfo)
{
    std::string dbPath = BuildDbPath(dbName);
    if (!CheckDbExists(dbPath, dbName)) {
        MEDIA_ERR_LOG("Db %{public}s not exist", BackupFileUtils::GarbleFilePath(dbPath, DEFAULT_RESTORE_ID).c_str());
        return;
    }

    auto mediaRdb = InitializeDatabase(dbName, dbPath);
    if (mediaRdb == nullptr) {
        return;
    }

    int32_t totalNumber = GetTotalRecordCount(mediaRdb, dbName);

    QueryDatabaseRecords(mediaRdb, totalNumber, mediaDbInfo);

    BuildDbInfoMap(dbName, mediaDbInfo);
}

void OthersCloneRestore::ReportMissingFilesFromDB(std::vector<CloneDbInfo> &mediaDbInfo, const std::string &dbType)
{
    for (auto &info : mediaDbInfo) {
        if (!info.fileExists) {
            (dbType == STAT_TYPE_PHOTO) ? photoFileNotFoundCount_++ : audioFileNotFoundCount_++;
            MEDIA_ERR_LOG("Missing file from db! Db path: %{public}s", info.data.c_str());
        }
    }
}

int32_t OthersCloneRestore::Init(const std::string &backupRetoreDir, const std::string &upgradeFilePath, bool isUpgrade)
{
    if (BaseRestore::Init() != E_OK) {
        MEDIA_ERR_LOG("GetBackupInfo init failed");
        return E_FAIL;
    }
    if (mediaLibraryRdb_ == nullptr) {
        MEDIA_ERR_LOG("GetBackupInfo Rdbstore is null");
        return E_FAIL;
    }
    userId_ = IsPrivateAccount() ? UserId::PRIVATE : UserId::MAIN;
    int64_t startGetInfo = MediaFileUtils::UTCTimeMilliSeconds();
    GetCloneDbInfos(AUDIO_DB_NAME, audioDbInfo_);
    GetCloneDbInfos(PHOTO_DB_NAME, photoDbInfo_);
    GetCloneDbInfos(PHOTO_SD_MEDIA_INFO_DB_NAME, photoDbInfo_);
    GetCloneDbInfos(VIDEO_DB_NAME, photoDbInfo_);
    GetCloneDbInfos(VIDEO_SD_MEDIA_INFO_DB_NAME, photoDbInfo_);
    int64_t startCurrent = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t err = GetAllfilesInCurrentDir(backupRestoreDir_);
    if (err != E_OK) {
        MEDIA_ERR_LOG("get all files err %{public}d", err);
        return err;
    }
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    ReportMissingFilesFromDB(audioDbInfo_, STAT_TYPE_AUDIO);
    ReportMissingFilesFromDB(photoDbInfo_, STAT_TYPE_PHOTO);

    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("INIT_OTHERS_CLONE", "",
            "userId: " + std::to_string(userId_) +
            ", get clone db cost: " + std::to_string(startCurrent - startGetInfo) +
            ", recursively getting all files cost: " + std::to_string(end - startCurrent) +
            "; photo file size: " + std::to_string(photoInfos_.size()) +
            ", audio file size: " + std::to_string(audioInfos_.size()) +
            "; photo db size: " + std::to_string(photoDbInfo_.size()) +
            ", audio db size: " + std::to_string(audioDbInfo_.size()) +
            "; db photo file not found: " + std::to_string(photoFileNotFoundCount_) +
            ", db audio file not found: " + std::to_string(audioFileNotFoundCount_));
    this->photoAlbumDao_.SetMediaLibraryRdb(this->mediaLibraryRdb_);
    this->photosRestore_.OnStart(this->mediaLibraryRdb_, nullptr);
    MEDIA_INFO_LOG("Init end");
    return E_OK;
}

NativeRdb::ValuesBucket OthersCloneRestore::GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
    int32_t sourceType)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
    values.PutString(MediaColumn::MEDIA_TITLE, fileInfo.title);
    values.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(fileInfo.displayName));
    // use owner_album_id to mark the album id which the photo is in.
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, fileInfo.ownerAlbumId);
    // only SOURCE album has package_name and owner_package.
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, fileInfo.packageName);
    values.PutString(MediaColumn::MEDIA_OWNER_PACKAGE, fileInfo.bundleName);
    values.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION, fileInfo.strongAssociation);
    if (fileInfo.dateTaken != 0) {
        values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, fileInfo.dateTaken);
        values.PutLong(MediaColumn::MEDIA_DATE_ADDED, fileInfo.dateTaken);
    }
    if (fileInfo.dateModified != 0) {
        values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, fileInfo.dateModified);
    }
    values.PutInt(MediaColumn::MEDIA_HIDDEN, fileInfo.hidden);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, fileInfo.dateTrashed);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    if (fileInfo.burstKey.size() > 0) {
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
        values.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, this->photosRestore_.FindBurstCoverLevel(fileInfo));
        values.PutString(PhotoColumn::PHOTO_BURST_KEY, this->photosRestore_.FindBurstKey(fileInfo));
    }
    values.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, fileInfo.compositeDisplayStatus);
    return values;
}

static std::string ParseSourcePathToPath(const std::string &sourcePath, const std::string &prefix)
{
    size_t startPos = sourcePath.find(prefix);
    std::string result = sourcePath;
    if (startPos != std::string::npos) {
        startPos += prefix.length();
        result = sourcePath.substr(startPos, sourcePath.size() - startPos);
    }
    return result;
}

static std::string Base32Decode(const std::string &input)
{
    std::string result;
    result.reserve(input.length() * BASE32_CHAR_BITS / BASE32_BYTE_BITS + 1);
    uint32_t val = 0;
    uint32_t valbits = 0;
    const char* data = input.data();
    size_t len = input.length();

    for (size_t idx = 0; idx < len; ++idx) {
        char c = data[idx];
        if (c >= 'A' && c <= 'Z') {
            val = (val << CHAR_ARRAY_LENGTH) + (c - 'A');
            valbits += CHAR_ARRAY_LENGTH;
        } else if (c >= '2' && c <= '7') {
            val = (val << CHAR_ARRAY_LENGTH) + (c - '2' + 26); // 26 : A-Z
            valbits += CHAR_ARRAY_LENGTH;
        }
        if (valbits >= ASCII_CHAR_LENGTH) {
            valbits -= ASCII_CHAR_LENGTH;
            result += static_cast<char>(val >> valbits);
            val &= (1 << valbits) - 1;
        }
    }
    return result;
}

static std::vector<std::string> GetSubStrings(const std::string &originalString, char delimiter)
{
    std::vector<std::string> substrings;
    if (originalString.empty()) {
        return substrings;
    }

    substrings.reserve(BASE32_BYTE_BITS); // Expected number of segments
    const char* start = originalString.data();
    const char* end = start + originalString.length();
    const char* current = start;

    for (const char* p = start; p != end; ++p) {
        if (*p == delimiter) {
            substrings.emplace_back(current, p - current);
            current = p + 1;
        }
    }

    if (current != end) {
        substrings.emplace_back(current, end - current);
    }

    return substrings;
}

static bool RecoverHiddenOrRecycleFile(std::string &currentPath, FileInfo &tmpInfo, std::string &decodeFileName)
{
    size_t hiddenAlbumPos = currentPath.find("hiddenAlbum/bins/");
    size_t recyclePos = currentPath.find("recycle/bins/");
    bool recycleFlag = false;
    if (hiddenAlbumPos != std::string::npos) {
        tmpInfo.hidden = 1;
    } else if (recyclePos != std::string::npos) {
        recycleFlag = true;
    } else {
        return false;
    }

    size_t lastSlashPos = currentPath.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        MEDIA_INFO_LOG("currentPath %{public}s is abnormal", currentPath.c_str());
        return false;
    }
    std::string target = currentPath.substr(lastSlashPos + 1);
    if (target.find(".") != std::string::npos) {
        MEDIA_INFO_LOG("currentPath %{public}s is already decoded", currentPath.c_str());
        return false;
    }
    std::vector<std::string> substrings = GetSubStrings(Base32Decode(target), '|');
    if (substrings.size() != 8) { // 8 : info num after decode
        MEDIA_ERR_LOG("currentPath %{public}s decode fail", currentPath.c_str());
        return false;
    }
    decodeFileName = substrings[DECODE_NAME_IDX] + substrings[DECODE_SURFIX_IDX];
    if (recycleFlag) {
        auto res = std::from_chars(substrings[DECODE_TIME_IDX].data(),
            substrings[DECODE_TIME_IDX].data() + substrings[DECODE_TIME_IDX].size(),
            tmpInfo.dateTrashed, 10); //10 : decimal
        if (res.ec != std::errc()) {
            MEDIA_ERR_LOG("get dateTrashed %{public}s fail", substrings[DECODE_TIME_IDX].c_str());
            return false;
        }
    }
    return true;
}

void OthersCloneRestore::AddAudioFile(FileInfo &tmpInfo)
{
    UpDateFileModifiedTime(tmpInfo);
    size_t relativePathPos = 0;
    size_t startPos = tmpInfo.filePath.find(INTERNAL_PREFIX);
    std::string startPath = tmpInfo.filePath;
    if (startPos != std::string::npos) {
        startPath = tmpInfo.filePath.substr(startPos);
        BackupFileUtils::GetPathPosByPrefixLevel(sceneCode_, startPath, INTERNAL_PREFIX_LEVEL, relativePathPos);
    }
    tmpInfo.relativePath = startPath.substr(relativePathPos);
    if (tmpInfo.relativePath == tmpInfo.filePath) {
        tmpInfo.relativePath = ParseSourcePathToPath(tmpInfo.filePath, OTHER_CLONE_FILE_ROOT_PATH);
    }
    audioInfos_.emplace_back(tmpInfo);
}

int32_t CheckBurstAndGetKey(const std::string &displayName, std::string &burstKey)
{
    size_t burstPos = displayName.find("_BURST");
    if (burstPos == std::string::npos) {
        return 0;
    }
    size_t burstStartPos = (burstPos >= IOS_BURST_CODE_LEN) ? burstPos - IOS_BURST_CODE_LEN : 0;
    burstKey = displayName.substr(burstStartPos, burstPos - burstStartPos);
    if (displayName.find("COVER") != std::string::npos) {
        return static_cast<int32_t>(BurstCoverLevelType::COVER);
    }
    return static_cast<int32_t>(BurstCoverLevelType::MEMBER);
}

void CloneIosBurstPhoto(FileInfo &fileInfo)
{
    std::string burstKey = fileInfo.burstKey;
    fileInfo.isBurst = CheckBurstAndGetKey(fileInfo.displayName, burstKey);
    fileInfo.burstKey = burstKey;
}

void OthersCloneRestore::SetFileInfosInCurrentDir(const std::string &file, struct stat &statInfo)
{
    FileInfo tmpInfo;
    std::string tmpFile = file;
    std::string decodeFileName = "";
    if (RecoverHiddenOrRecycleFile(tmpFile, tmpInfo, decodeFileName)) {
        tmpInfo.displayName = decodeFileName;
    } else {
        tmpInfo.displayName = ExtractFileName(tmpFile);
    }
    tmpInfo.filePath = tmpFile;
    tmpInfo.oldPath = tmpFile; // for failed files statistics
    tmpInfo.title = BackupFileUtils::GetFileTitle(tmpInfo.displayName);
    tmpInfo.fileType = MediaFileUtils::GetMediaType(tmpInfo.displayName);
    tmpInfo.fileSize = statInfo.st_size;
    tmpInfo.dateModified = MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim);
    if (sceneCode_ == I_PHONE_CLONE_RESTORE) {
        CloneIosBurstPhoto(tmpInfo);
    }
    if (tmpInfo.fileType == MediaType::MEDIA_TYPE_IMAGE) {
        size_t enhancedPos = file.rfind("_enhanced");
        if (enhancedPos != std::string::npos) {
            size_t dotPos = file.find_last_of('.');
            if (dotPos != std::string::npos && dotPos > enhancedPos) {
                MEDIA_INFO_LOG("%{private}s is an enhanced image!", file.c_str());
                tmpInfo.strongAssociation = STRONG_ASSOCIATION_ENABLE;
            }
        }
    }
    if (tmpInfo.fileType  == MediaType::MEDIA_TYPE_IMAGE || tmpInfo.fileType  == MediaType::MEDIA_TYPE_VIDEO) {
        UpDateFileModifiedTime(tmpInfo);
        UpdateFileGPS(tmpInfo);
        photoInfos_.emplace_back(tmpInfo);
    } else if (tmpInfo.fileType  == MediaType::MEDIA_TYPE_AUDIO) {
        AddAudioFile(tmpInfo);
    } else {
        tmpInfo.fileType = MediaFileUtils::GetMediaTypeNotSupported(tmpInfo.displayName);
        if (tmpInfo.fileType  == MediaType::MEDIA_TYPE_IMAGE || tmpInfo.fileType  == MediaType::MEDIA_TYPE_VIDEO) {
            UpDateFileModifiedTime(tmpInfo);
            photoInfos_.emplace_back(tmpInfo);
            MEDIA_WARN_LOG("Not supported media %{public}s",
                BackupFileUtils::GarbleFilePath(tmpFile, sceneCode_).c_str());
        } else if (tmpInfo.fileType  == MediaType::MEDIA_TYPE_AUDIO) {
            AddAudioFile(tmpInfo);
            MEDIA_WARN_LOG("Not supported audio %{public}s",
                BackupFileUtils::GarbleFilePath(tmpFile, sceneCode_).c_str());
        } else {
            MEDIA_WARN_LOG("Not supported file %{public}s",
                BackupFileUtils::GarbleFilePath(tmpFile, sceneCode_).c_str());
        }
    }
}

bool OthersCloneRestore::ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix,
    std::string &newPath, std::string &relativePath)
{
    size_t pos = 0;
    CHECK_AND_RETURN_RET(BackupFileUtils::GetPathPosByPrefixLevel(sceneCode_, srcPath,
        INTERNAL_PREFIX_LEVEL, pos), false);
    newPath = prefix + srcPath;
    relativePath = srcPath.substr(pos);
    return true;
}

bool OthersCloneRestore::ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix,
    std::string &newPath, std::string &relativePath, FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(srcPath != "", false);
    if (MediaFileUtils::StartsWith(srcPath, INTERNAL_PREFIX)) {
        return ConvertPathToRealPath(srcPath, prefix, newPath, relativePath);
    }
    size_t pos = 0;
    if (!BackupFileUtils::GetPathPosByPrefixLevel(sceneCode_, srcPath, SD_PREFIX_LEVEL, pos)) {
        return false;
    }
    relativePath = srcPath.substr(pos);
    if (fileInfo.fileSize < TAR_FILE_LIMIT || fileInfo.hidden > 0 ||
        fileInfo.dateTrashed > 0) {
        newPath = prefix + srcPath; // packed as tar, hidden or trashed, use path in DB
    } else {
        newPath = prefix + relativePath; // others, remove sd prefix, use relative path
    }
    fileInfo.isInternal = false;
    return true;
}

bool OthersCloneRestore::CheckSamePathForSD(const std::string &dataPath, FileInfo &fileInfo,
    const std::string &filePath)
{
    std::string newPath = "";
    std::string newRelativePath = "";
    ConvertPathToRealPath(dataPath, OTHER_CLONE_FILE_ROOT_PATH, newPath, newRelativePath, fileInfo);
    return newPath == filePath;
}

void OthersCloneRestore::UpdateFileGPS(FileInfo &fileInfo)
{
    CloneDbInfo* infoPtr = nullptr;

    if (fileInfo.fileType == MediaType::MEDIA_TYPE_AUDIO) {
        auto it = audioDbMap_.find(fileInfo.displayName);
        if (it != audioDbMap_.end()) {
            infoPtr = it->second;
        } else {
            return;
        }
    } else if (fileInfo.fileType  == MediaType::MEDIA_TYPE_IMAGE || fileInfo.fileType  == MediaType::MEDIA_TYPE_VIDEO) {
        auto it = photoDbMap_.find(fileInfo.displayName);
        if (it != photoDbMap_.end()) {
            infoPtr = it->second;
        } else {
            it = audioDbMap_.find(fileInfo.displayName);
            if (it != audioDbMap_.end()) {
                MEDIA_WARN_LOG("find video in audio info map %{public}s", fileInfo.displayName.c_str());
                infoPtr = it->second;
            } else {
                return;
            }
        }
    } else {
        MEDIA_WARN_LOG("Not supported file %{public}s", fileInfo.displayName.c_str());
        return;
    }

    fileInfo.latitude = infoPtr->latitude;
    fileInfo.longitude = infoPtr->longitude;
}

std::string OthersCloneRestore::GenerateSearchKey(const FileInfo &fileInfo)
{
    if (sceneCode_ == I_PHONE_CLONE_RESTORE) {
        return fileInfo.displayName;
    } else if (sceneCode_ == LITE_PHONE_CLONE_RESTORE) {
        return ParseSourcePathToPath(fileInfo.filePath, OTHER_CLONE_FILE_ROOT_PATH);
    } else {
        return ParseSourcePathToPath(fileInfo.filePath, OTHER_CLONE_FILE_ROOT_PATH);
    }
}

CloneDbInfo* OthersCloneRestore::FindAudioDbInfo(const std::string &key, const std::string &displayName)
{
    auto it = audioDbMap_.find(key);
    if (it != audioDbMap_.end()) {
        it->second->fileExists = true;
        return it->second;
    }

    it = audioDbMap_.find(displayName);
    if (it != audioDbMap_.end()) {
        it->second->fileExists = true;
        return it->second;
    }

    return nullptr;
}

CloneDbInfo* OthersCloneRestore::FindPhotoDbInfo(const std::string &key, const std::string &displayName)
{
    auto it = photoDbMap_.find(key);
    if (it != photoDbMap_.end()) {
        it->second->fileExists = true;
        return it->second;
    }

    it = photoDbMap_.find(displayName);
    if (it != photoDbMap_.end()) {
        it->second->fileExists = true;
        return it->second;
    }

    it = audioDbMap_.find(key);
    if (it == audioDbMap_.end()) {
        it = audioDbMap_.find(displayName);
    }

    if (it != audioDbMap_.end()) {
        MEDIA_WARN_LOG("find video in audio info map %{public}s", displayName.c_str());
        it->second->fileExists = true;
        return it->second;
    }

    return nullptr;
}

CloneDbInfo* OthersCloneRestore::FindDbInfoByFileType(const FileInfo &fileInfo, const std::string &key)
{
    if (fileInfo.fileType == MediaType::MEDIA_TYPE_AUDIO) {
        return FindAudioDbInfo(key, fileInfo.displayName);
    } else if (fileInfo.fileType == MediaType::MEDIA_TYPE_IMAGE ||
               fileInfo.fileType == MediaType::MEDIA_TYPE_VIDEO) {
        return FindPhotoDbInfo(key, fileInfo.displayName);
    } else {
        MEDIA_WARN_LOG("Not supported file %{public}s", fileInfo.displayName.c_str());
        return nullptr;
    }
}

double OthersCloneRestore::ConvertTimeToSeconds(double timeValue)
{
    if (timeValue < SECONDS_LEVEL_LIMIT) {
        return timeValue * static_cast<double>(MSEC_TO_SEC);
    }
    return timeValue;
}

void OthersCloneRestore::UpdateFileTimeInfo(FileInfo &fileInfo, CloneDbInfo *dbInfo)
{
    double dateModified = ConvertTimeToSeconds(dbInfo->dateModified);
    double dateTaken = ConvertTimeToSeconds(dbInfo->dateTaken);

    fileInfo.dateModified = static_cast<int64_t>(dateModified);
    fileInfo.dateTaken = static_cast<int64_t>(dateTaken);

    MediaFileUtils::UpdateModifyTimeInMsec(fileInfo.filePath, fileInfo.dateModified);
}

void OthersCloneRestore::UpDateFileModifiedTime(FileInfo &fileInfo)
{
    std::string key = GenerateSearchKey(fileInfo);
    CloneDbInfo* infoPtr = FindDbInfoByFileType(fileInfo, key);

    if (infoPtr == nullptr) {
        return;
    }

    UpdateFileTimeInfo(fileInfo, infoPtr);
}

int32_t OthersCloneRestore::GetAllfilesInCurrentDir(const std::string &path)
{
    int err = E_OK;
    DIR *dirPath = nullptr;
    struct dirent *currentFile = nullptr;
    size_t len = path.length();
    struct stat statInfo;

    if (len >= FILENAME_MAX - 1) {
        return ERR_INCORRECT_PATH;
    }

    std::unique_ptr<char[]> fName = std::make_unique<char[]>(FILENAME_MAX);
    if (strcpy_s(fName.get(), FILENAME_MAX, path.c_str()) != ERR_SUCCESS) {
        return ERR_MEM_ALLOC_FAIL;
    }
    fName[len++] = '/';
    if ((dirPath = opendir(path.c_str())) == nullptr) {
        MEDIA_ERR_LOG("Failed to opendir %{private}s, errno %{private}d", path.c_str(), errno);
        return ERR_NOT_ACCESSIBLE;
    }

    while ((currentFile = readdir(dirPath)) != nullptr) {
        if (!strcmp(currentFile->d_name, ".") || !strcmp(currentFile->d_name, "..")) {
            continue;
        }
        if (strncpy_s(fName.get() + len, FILENAME_MAX - len, currentFile->d_name, FILENAME_MAX - len)) {
            MEDIA_ERR_LOG("Failed to copy file name %{private}s ", fName.get());
            continue;
        }
        if (lstat(fName.get(), &statInfo) == -1) {
            MEDIA_ERR_LOG("Failed to get info of directory %{private}s ", fName.get());
            continue;
        }
        std::string currentPath = fName.get();
        if (S_ISDIR(statInfo.st_mode)) {
            (void)GetAllfilesInCurrentDir(currentPath);
        } else if (S_ISREG(statInfo.st_mode)) {
            SetFileInfosInCurrentDir(fName.get(), statInfo);
        } else {
            MEDIA_INFO_LOG("Not directory or regular file, name is %{private}s", fName.get());
        }
    }
    closedir(dirPath);
    dirPath = nullptr;
    return err;
}

void OthersCloneRestore::HandleInsertBatch(int32_t offset)
{
    int32_t totalNumber = std::min(static_cast<int32_t>(photoInfos_.size()),
        static_cast<int32_t>(offset + QUERY_NUMBER));
    vector<FileInfo> insertInfos;
    for (offset; offset < totalNumber; offset++) {
        FileInfo info = photoInfos_[offset];
        if (info.fileType != MediaType::MEDIA_TYPE_IMAGE && info.fileType != MediaType::MEDIA_TYPE_VIDEO) {
            MEDIA_WARN_LOG("photo info error : %{public}s", info.displayName.c_str());
            continue;
        }
        if (sceneCode_ == I_PHONE_CLONE_RESTORE && info.otherSubtype == I_PHONE_DYNAMIC_VIDEO_TYPE) {
            continue;
        }
        UpdateAlbumInfo(info);
        insertInfos.push_back(info);
    }
    InsertPhoto(insertInfos);
}

void OthersCloneRestore::ReportCloneBefore()
{
    MEDIA_INFO_LOG("Start ReportCloneBefore.");
    DatabaseReport()
        .SetSceneCode(sceneCode_)
        .SetTaskId(taskId_)
        .ReportMedia(mediaLibraryRdb_, DatabaseReport::PERIOD_BEFORE);

    UpgradeRestoreTaskReport()
        .SetSceneCode(sceneCode_)
        .SetTaskId(taskId_)
        .Report("others Clone Restore", "",
            "sceneCode_: " + std::to_string(sceneCode_) +
            ", isAccountValid_: " + std::to_string(isAccountValid_) +
            ", isSyncSwitchOpen: " + std::to_string(isSyncSwitchOn_));
    MEDIA_INFO_LOG("End ReportCloneBefore.");
}

void OthersCloneRestore::RestorePhoto()
{
    if (!photoInfos_.size()) {
        MEDIA_INFO_LOG("photo infos size zero");
        return;
    }
    ReportCloneBefore();
    std::vector<FileInfo> fileInfos;
    totalNumber_ += photoInfos_.size();
    RestoreAlbum(photoInfos_);
    unsigned long pageSize = 200;
    vector<FileInfo> insertInfos;
    int32_t totalNumber = static_cast<int32_t>(photoInfos_.size());
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_CLONE_THREAD_NUM);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_NUMBER) {
        ffrt::submit([this, offset]() {
            HandleInsertBatch(offset);
            }, { &offset }, {}, ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    ProcessBurstPhotos();
}

void OthersCloneRestore::InsertPhoto(std::vector<FileInfo> &fileInfos)
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = BaseRestore::GetInsertValues(sceneCode_, fileInfos, SourceType::PHOTOS);
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoColumn::PHOTOS_TABLE, values, rowNum);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("BatchInsert fail err %{public}d", errCode);
        UpdateFailedFiles(fileInfos, RestoreError::INSERT_FAILED);
        return;
    }

    int64_t startMove = MediaFileUtils::UTCTimeMilliSeconds();
    migrateDatabaseNumber_ += rowNum;
    int32_t fileMoveCount = 0;
    int32_t videoFileMoveCount = 0;
    MoveMigrateFile(fileInfos, fileMoveCount, videoFileMoveCount, sceneCode_);
    int64_t startUpdate = MediaFileUtils::UTCTimeMilliSeconds();
    UpdatePhotosByFileInfoMap(mediaLibraryRdb_, fileInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("generate values cost %{public}ld, insert %{public}ld assets cost %{public}ld"
        ", and move %{public}ld files (%{public}ld + %{public}ld) cost %{public}ld. update cost %{public}ld",
        (long)(startInsert - start), (long)rowNum, (long)(startMove - startInsert),
        (long)fileMoveCount, (long)(fileMoveCount - videoFileMoveCount),
        (long)videoFileMoveCount, (long)(startUpdate - startMove), (long)(end - startUpdate));
}

bool OthersCloneRestore::NeedBatchQueryPhotoForPortrait(const std::vector<FileInfo> &fileInfos,
    NeedQueryMap &needQueryMap)
{
    return true;
}

void OthersCloneRestore::RestoreAudio()
{
    MEDIA_INFO_LOG("restore audio");
    if (sceneCode_ == I_PHONE_CLONE_RESTORE) {
        MEDIA_INFO_LOG("No need move audio");
        return;
    }
    if (!audioInfos_.size()) {
        MEDIA_INFO_LOG("audio infos size zero");
        return;
    }
    audioTotalNumber_ += audioInfos_.size();
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    InsertAudio(sceneCode_, audioInfos_);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("generate values cost %{public}ld, insert audio %{public}ld",
        (long)(end - start), (long)audioTotalNumber_);
}

void OthersCloneRestore::HandleRestData()
{
    MEDIA_INFO_LOG("Start to handle rest data in native.");
    RestoreThumbnail();
}

std::string OthersCloneRestore::ParseSourcePathToLPath(int32_t sceneCode, const std::string &filePath,
    int32_t fileType)
{
    std::string lPath = filePath;
    std::string source = GetFileHeadPath(sceneCode, fileType);
    auto findPos = lPath.find(source);
    if (findPos != std::string::npos) {
        lPath.replace(lPath.find(source), source.length(), "");
    } else {
        MEDIA_WARN_LOG("find other clone path error path: %{public}s",
            BackupFileUtils::GarbleFilePath(filePath, sceneCode).c_str());
        source = LITE_CLONE_SD_FILE_PATH;
        findPos = lPath.find(source);
        if (findPos != std::string::npos) {
            lPath.replace(lPath.find(source), source.length(), "");
            std::size_t startPos = lPath.find_first_of(FILE_SEPARATOR);
            if (startPos != std::string::npos) {
                lPath = lPath.substr(startPos);
            }
        } else {
            source = OTHER_CLONE_FILE_ROOT_PATH;
            findPos = lPath.find(source);
            if (findPos != std::string::npos) {
                lPath.replace(lPath.find(source), source.length(), "");
            }
        }
    }
    std::size_t startPos = lPath.find_first_of(FILE_SEPARATOR);
    if (startPos != std::string::npos) {
        lPath = lPath.substr(startPos);
    }
    std::size_t pos = lPath.find_last_of(FILE_SEPARATOR);
    if (pos != std::string::npos) {
        lPath = lPath.substr(0, pos);
    } else {
        MEDIA_WARN_LOG("find error path is: %{public}s",
            BackupFileUtils::GarbleFilePath(filePath, sceneCode).c_str());
        lPath = FILE_SEPARATOR;
    }
    if (lPath.empty()) {
        MEDIA_WARN_LOG("find path is empty: %{public}s",
            BackupFileUtils::GarbleFilePath(filePath, sceneCode).c_str());
        lPath = FILE_SEPARATOR;
    }
    lPath = lPath == AlbumPlugin::LPATH_HIDDEN_ALBUM ? AlbumPlugin::LPATH_RECOVER : lPath;
    return lPath;
}

std::string OthersCloneRestore::GetFileHeadPath(int32_t sceneCode, int32_t fileType)
{
    if (sceneCode == I_PHONE_CLONE_RESTORE) {
        if (fileType == MediaType::MEDIA_TYPE_IMAGE) {
            return I_PHONE_IMAGE_FILE_PATH;
        }
        if (fileType == MediaType::MEDIA_TYPE_VIDEO) {
            return I_PHONE_VIDEO_FILE_PATH;
        }
        MEDIA_WARN_LOG("GetFileHeadPath other fileType.");
        return I_PHONE_IMAGE_FILE_PATH;
    }
    return OTHER_CLONE_FILE_PATH;
}

void OthersCloneRestore::AddGalleryAlbum(std::vector<PhotoAlbumRestore::GalleryAlbumRowData> &galleryAlbumInfos,
    const std::string &lPath)
{
    auto pathMatch = [outerLPath {lPath}](const auto &galleryAlbumInfo) {
        return galleryAlbumInfo.lPath == outerLPath;
    };
    auto it = std::find_if(galleryAlbumInfos.begin(), galleryAlbumInfos.end(), pathMatch);
    if (it != galleryAlbumInfos.end()) {
        return;
    }

    PhotoAlbumRestore::GalleryAlbumRowData galleryAlbum;
    std::size_t pos = lPath.find_last_of(FILE_SEPARATOR);
    if (pos != std::string::npos) {
        galleryAlbum.albumName = lPath.substr(pos + 1);
    }
    if (galleryAlbum.albumName.empty()) {
        galleryAlbum.albumName = lPath;
    }
    galleryAlbum.lPath = lPath;
    galleryAlbum.priority = 1;
    galleryAlbumInfos.emplace_back(galleryAlbum);
}

bool OthersCloneRestore::ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
    std::string dbName)
{
    return true;
}

bool OthersCloneRestore::ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info)
{
    return true;
}

void OthersCloneRestore::RestoreAlbum(std::vector<FileInfo> &fileInfos)
{
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> albumInfos = this->photoAlbumDao_.GetPhotoAlbums();
    std::vector<PhotoAlbumRestore::GalleryAlbumRowData> galleryAlbumInfos;
    for (auto &info : fileInfos) {
        if (IsIosMovingPhotoVideo(info, sceneCode_)) {
            continue;
        }
        info.lPath = ParseSourcePathToLPath(sceneCode_, info.filePath, info.fileType);
        AddGalleryAlbum(galleryAlbumInfos, info.lPath);
    }
    std::vector<PhotoAlbumDao::PhotoAlbumRowData> albumInfosToRestore =
        photoAlbumRestore_.GetAlbumsToRestore(albumInfos, galleryAlbumInfos);
    auto ret =  photoAlbumDao_.RestoreAlbums(albumInfosToRestore);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to RestoreAlbums : %{public}d", ret);
    }
}

bool OthersCloneRestore::HasSameFileForDualClone(FileInfo &fileInfo)
{
    PhotosDao::PhotosRowData rowData = this->photosRestore_.FindSameFile(fileInfo);
    if (!rowData.IsValid()) {
        return false;
    }
    // Meed extra check to determine whether or not to drop the duplicate file.
    return ExtraCheckForCloneSameFile(fileInfo, rowData);
}

static std::string ToLower(const std::string &str)
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

PhotoAlbumDao::PhotoAlbumRowData OthersCloneRestore::FindAlbumInfo(FileInfo &fileInfo)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo;
    if (fileInfo.lPath.empty()) {
        MEDIA_ERR_LOG("others clone lPath is empty, path: %{public}s",
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode_).c_str());
        return albumInfo;
    }
    if (ToLower(fileInfo.lPath) == ToLower(AlbumPlugin::LPATH_SCREEN_SHOTS) &&
        fileInfo.fileType == MediaType::MEDIA_TYPE_VIDEO) {
        albumInfo = this->photoAlbumDao_.BuildAlbumInfoOfRecorders();
        albumInfo = this->photoAlbumDao_.GetOrCreatePhotoAlbum(albumInfo);
        MEDIA_INFO_LOG(
            "others clone: screenshots redirect to screenrecords, path: %{public}s",
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode_).c_str());
        fileInfo.lPath = AlbumPlugin::LPATH_SCREEN_RECORDS;
        return albumInfo;
    }
    albumInfo = this->photoAlbumDao_.GetPhotoAlbum(fileInfo.lPath);
    if (albumInfo.lPath.empty()) {
        MEDIA_ERR_LOG("others clone: albumInfo is empty, path: %{public}s",
            BackupFileUtils::GarbleFilePath(fileInfo.filePath, sceneCode_).c_str());
    }
    return albumInfo;
}

void OthersCloneRestore::UpdateAlbumInfo(FileInfo &info)
{
    PhotoAlbumDao::PhotoAlbumRowData albumInfo = FindAlbumInfo(info);
    info.mediaAlbumId = albumInfo.albumId;
    info.ownerAlbumId = albumInfo.albumId;
}

void OthersCloneRestore::AnalyzeSource()
{
    MEDIA_INFO_LOG("analyze source later");
}

size_t GetIosMovingPhotoSize(const std::string iosMovingPhotoImagePath)
{
    size_t destPos = iosMovingPhotoImagePath.find_last_of(".");
    CHECK_AND_RETURN_RET_LOG(destPos != std::string::npos, 0, "iosMovingPhotoImagePath not contain '.'");
    std::string iosMovingPhotoVideoPath = iosMovingPhotoImagePath.substr(0, destPos) + ".MOV";
    size_t imageSize = 0;
    size_t videoSize = 0;
    (void)MediaFileUtils::GetFileSize(iosMovingPhotoImagePath, imageSize);
    (void)MediaFileUtils::GetFileSize(iosMovingPhotoVideoPath, videoSize);
    return (videoSize == 0) ? 0 : imageSize + videoSize + EXTRADATA_LEN;
}

bool OthersCloneRestore::IsIosMovingPhotoVideo(FileInfo &fileInfo, int32_t sceneCode)
{
    if (sceneCode != I_PHONE_CLONE_RESTORE || fileInfo.filePath.find(I_PHONE_DYNAMIC_IMAGE) == string::npos) {
        return false;
    }
    auto idx = fileInfo.filePath.find(I_PHONE_DYNAMIC_VIDEO);
    if (idx != string::npos) {
        fileInfo.otherSubtype = I_PHONE_DYNAMIC_VIDEO_TYPE;
        return true;
    } else {
        fileInfo.subtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
        fileInfo.fileSize = static_cast<int64_t>(GetIosMovingPhotoSize(fileInfo.filePath));
    }
    return false;
}

bool OthersCloneRestore::IsPrivateAccount()
{
    AccountSA::OsAccountType type;
    ErrCode ret = AccountSA::OsAccountManager::GetOsAccountTypeFromProcess(type);
    CHECK_AND_RETURN_RET_LOG(ret == ERR_OK, false, "Get OsAccountType failed, ret: %{public}d", ret);
    MEDIA_INFO_LOG("Current accountType: %{public}d", static_cast<int32_t>(type));
    return type == AccountSA::OsAccountType::PRIVATE;
}
} // namespace Media
} // namespace OHOS
