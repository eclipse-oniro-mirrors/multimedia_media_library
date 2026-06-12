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
#define MLOG_TAG "MediaLibraryFileInterwork"

#include "media_fileinterwork_scanner.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <securec.h>
#include <regex>

#include "dfx_utils.h"
#include "media_fileinterwork_column.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "power_efficiency_manager.h"
#include "media_container_types.h"
#include "mimetype_utils.h"
#include "media_file_utils.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdb_utils.h"
#include "values_bucket.h"
#include "rdb_predicates.h"
#include "rdb_sql_utils.h"
#include "medialibrary_asset_operations.h"
#include "refresh_business_name.h"
#include "metadata_extractor.h"
#include "scanner_utils.h"
#include "shooting_mode_column.h"
#include "media_file_uri.h"
#include "moving_photo_file_utils.h"
#include "media_fileinterwork_util.h"
#include "consistency_check_manager.h"

using namespace std;
namespace OHOS::Media {

constexpr int32_t TASK_STATUS_IDLE = 0;
constexpr int32_t TASK_STATUS_PHASE_ONE = 1;
constexpr int32_t TASK_STATUS_PHASE_TWO = 2;
constexpr int32_t TASK_STATUS_COMPLETED = 3;

constexpr int32_t MIN_FILE_SIZE = 1 * 1024;
constexpr int32_t BATCH_INSERT_SIZE = 100;
constexpr int32_t MAX_TEMPERATURE = 37;
constexpr int32_t OPT_INIT_STATUS = 0;
constexpr int32_t OPT_FINISH_STATUS = 1;

// 定义支持的图片和视频文件扩展名
const std::vector<std::string> SUPPORTED_MEDIA_EXTENSIONS  = {
    // photo
    ".arw", ".avif", ".bm", ".bmp", ".cr2", ".cur", ".dng", ".gif", ".heic", ".heif", ".heics",
    ".hif", ".ico", ".jpe", ".jpeg", ".jpg", ".mpo", ".nrw", ".pef", ".png", ".psd", ".raw", ".rw2",
    ".srw", ".svg", ".tif", ".webp", ".raf", ".wbmp", ".m4u", ".m4v", ".mov", ".m", ".raf", ".wbmp",
    // video
    ".m4u", ".m4v", ".mov", ".mp4", ".mpe", ".mpeg", ".mpg4", ".mkv", ".webm", ".3gpp", ".asf",
    ".asx", ".avi", ".flv", ".3g2", ".wmv", ".rm", ".3gp", ".mpg", ".rv", ".ts", ".divx", ".f4v",
    ".3gpp2", "rmvb"
};

// 跳过的文件目录
const std::vector<std::string> SKIP_SCAN_DIR = {
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::HO_DATA_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::THUMBS_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::RECENT_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::BACKUP_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::TRASH_DIR_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::VM_DOCS_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::OHPM_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::PCE_ENGINE_DIR,
    MediaFileInterworkColumn::FILE_ROOT_DIR + MediaFileInterworkColumn::APPDATA_DIR,
};

MediaFileInterworkScanner* MediaFileInterworkScanner::GetInstance()
{
    static MediaFileInterworkScanner instance;
    return &instance;
}

bool MediaFileInterworkScanner::CheckSystemConditions()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

int32_t MediaFileInterworkScanner::ScanDirectory(const std::string &path, std::vector<std::string> &files)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        MEDIA_ERR_LOG("Failed to open directory: %{public}s", path.c_str());
        return E_OK;
    }
    if (!CheckSystemConditions()) {
        MEDIA_INFO_LOG("System conditions not met, pausing task");
        closedir(dir);
        return E_ERR;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string fullPath = path + "/" + entry->d_name;
        struct stat statInfo;
        if (lstat(fullPath.c_str(), &statInfo) != 0) {
            MEDIA_ERR_LOG("Failed to stat file: %{public}s", DfxUtils::GetSafePath(fullPath).c_str());
            continue;
        }
        if (S_ISDIR(statInfo.st_mode)) {
            if (ShouldSkipDirectory(fullPath)) {
                MEDIA_INFO_LOG("invalid dir %{public}s", DfxUtils::GetSafePath(fullPath).c_str());
                continue;
            }
            int32_t status = ScanDirectory(fullPath, files);
            if (status != E_OK) {
                MEDIA_INFO_LOG("System conditions not met, pausing task");
                return E_ERR;
            }
        } else if (S_ISREG(statInfo.st_mode)) {
            if (IsImageOrVideoFile(fullPath) && statInfo.st_size >= MIN_FILE_SIZE &&
                IsValidFileName(entry->d_name)) {
                files.push_back(fullPath);
            } else {
                MEDIA_INFO_LOG("invalid file %{public}s", DfxUtils::GetSafePath(fullPath).c_str());
            }
        }
    }
    closedir(dir);
    return E_OK;
}

int32_t MediaFileInterworkScanner::ExecutePhaseOne()
{
    MEDIA_INFO_LOG("ExecutePhaseOne started");
    std::vector<std::string> allFiles;
    int32_t ret = ScanDirectory(MediaFileInterworkColumn::FILE_ROOT_DIR, allFiles);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "ScanDirectory failed, ret = %{public}d", ret);
    MEDIA_INFO_LOG("Found %{public}zu files to process", allFiles.size());
    std::vector<std::string> batch;
    for (const auto& filePath : allFiles) {
        batch.push_back(filePath);
        if (batch.size() >= static_cast<size_t>(BATCH_INSERT_SIZE)) {
            ret = InsertFileBatch(batch);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "InsertFileBatch failed, ret = %{public}d", ret);
            batch.clear();
            CHECK_AND_RETURN_RET_LOG(CheckSystemConditions(), E_ERR, "System conditions not met, pausing task");
            batch.push_back(filePath);
        }
    }

    if (!batch.empty()) {
        ret = InsertFileBatch(batch);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "InsertFileBatch failed, ret = %{public}d", ret);
        batch.clear();
    }
    ret = MediaFileInterworkUtil::SetScannerTaskStatus(TASK_STATUS_PHASE_TWO);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "SetScannerTaskStatus failed, ret = %{public}d", ret);

    MEDIA_INFO_LOG("ExecutePhaseOne completed successfully");
    return E_OK;
}

int32_t MediaFileInterworkScanner::ExecutePhaseTwo()
{
    MEDIA_INFO_LOG("ExecutePhaseTwo started");
    int32_t ret = ProcessPhaseTwoRecords();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "ProcessPhaseTwoRecords failed, ret = %{public}d", ret);

    ret = MediaFileInterworkUtil::SetScannerTaskStatus(TASK_STATUS_COMPLETED);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "SetScannerTaskStatus failed, ret = %{public}d", ret);
    MediaFileInterworkUtil::ReportFileManagerFirstLoad();
    MEDIA_INFO_LOG("ExecutePhaseTwo completed successfully");
    return E_OK;
}

bool MediaFileInterworkScanner::IsImageOrVideoFile(const std::string &filePath)
{
    size_t pos = filePath.find_last_of('.');
    CHECK_AND_RETURN_RET_LOG(pos != std::string::npos, false, "Invalid file path: %{public}s", filePath.c_str());
    std::string extension = filePath.substr(pos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return std::find(SUPPORTED_MEDIA_EXTENSIONS.begin(), SUPPORTED_MEDIA_EXTENSIONS.end(),
        extension) != SUPPORTED_MEDIA_EXTENSIONS.end();
}

bool MediaFileInterworkScanner::IsValidFileName(const std::string& fileName)
{
    // 定义正则表达式，匹配包含特殊字符的文件名
    std::regex pattern(R"(\.\.|\[|\])");
    return !std::regex_search(fileName, pattern);
}

bool MediaFileInterworkScanner::ShouldSkipDirectory(const std::string &dirPath)
{
    size_t pos = dirPath.find_last_of('/');
    if (pos == std::string::npos) {
        return true;
    }
    return std::find(SKIP_SCAN_DIR.begin(), SKIP_SCAN_DIR.end(),
        dirPath) != SKIP_SCAN_DIR.end();
}

int32_t MediaFileInterworkScanner::InsertFileBatch(const std::vector<std::string> &files)
{
    CHECK_AND_RETURN_RET_LOG(!files.empty(), E_OK, "files is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");
    int32_t successCount = 0;
    int32_t skipCount = 0;
    int32_t failCount = 0;
    for (const auto& filePath : files) {
        NativeRdb::RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
        predicates.EqualTo(MediaFileInterworkColumn::AFTER_PATH_COLUMN, filePath);
        predicates.EqualTo(MediaFileInterworkColumn::OPT_STATUS_COLUMN, 0);
        auto resultSet = rdbStore->Query(predicates, {});
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("Query check failed for: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
            skipCount++;
            continue;
        }
        if (resultSet->GoToNextRow() == E_OK) {
            MEDIA_WARN_LOG("already exist %{public}s", DfxUtils::GetSafePath(filePath).c_str());
            skipCount++;
            resultSet->Close();
            continue;
        }
        resultSet->Close();
        string insertSql = "INSERT INTO " + std::string(MediaFileInterworkColumn::OPT_TABLE_NAME) +
            " (" + std::string(MediaFileInterworkColumn::AFTER_PATH_COLUMN) + ", " +
            std::string(MediaFileInterworkColumn::OPT_COLUMN) + ", " +
            std::string(MediaFileInterworkColumn::OPT_STATUS_COLUMN) + ") VALUES (?, 0, 0)";
        int64_t rowId = 0;
        int32_t ret = rdbStore->ExecuteSql(insertSql, {filePath});
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Insert failed for: %{public}s, ret = %{public}d",
                DfxUtils::GetSafePath(filePath).c_str(), ret);
            failCount++;
            continue;
        }
        successCount++;
    }

    MEDIA_INFO_LOG("Inserted %{public}d, skip %{public}d, failed %{public}d, total %{public}zu",
        successCount, skipCount, failCount, files.size());
    return E_OK;
}

int32_t MediaFileInterworkScanner::CleanTabFileOptTable()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");

    std::string deleteSql = "DELETE FROM " + std::string(MediaFileInterworkColumn::OPT_TABLE_NAME);
    int32_t ret = rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("CleanTabFileOptTable failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }

    MEDIA_INFO_LOG("Cleaned tab_file_opt table successfully");
    return E_OK;
}

int32_t MediaFileInterworkScanner::GetFileMetadata(std::unique_ptr<Metadata> &data)
{
    struct stat statInfo {};
    if (stat(data->GetFilePath().c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        return E_FAIL;
    }
    data->SetFileSize(statInfo.st_size);
    auto dateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(statInfo.st_mtim));
    if (dateModified == 0) {
        dateModified = MediaFileUtils::UTCTimeMilliSeconds();
        MEDIA_WARN_LOG("Invalid dateModified from st_mtim, use current time instead: %{public}lld",
            static_cast<long long>(dateModified));
    }
    if (dateModified != 0 && data->GetFileDateModified() == 0) {
        data->SetFileDateModified(dateModified);
    }
    string extension = ScannerUtils::GetFileExtension(data->GetFileName());
    data->SetFileExtension(extension);
    return E_OK;
}

int32_t MediaFileInterworkScanner::FillMetadata(const FileInfo &fileInfo, std::unique_ptr<Metadata> &data)
{
    data->SetFilePath(fileInfo.originFilePath);
    data->SetFileName(fileInfo.fileName);
    data->SetFileMediaType(fileInfo.mediaType);
    int32_t err = GetFileMetadata(data);
    if (err != E_OK) {
        MEDIA_ERR_LOG("failed to get file metadata");
        return err;
    }
    if (data->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(data);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(data, Scene::AV_META_SCENE_CLONE);
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "failed to extension data");
    return E_OK;
}

void MediaFileInterworkScanner::SetTimeInfo(
    const std::unique_ptr<Metadata> &data, FileInfo &info, NativeRdb::ValuesBucket &value)
{
    int64_t dateAdded =
        PhotoFileUtils::NormalizeTimestamp(data->GetFileDateAdded(), MediaFileUtils::UTCTimeMilliSeconds());
    int64_t dateModified = PhotoFileUtils::NormalizeTimestamp(data->GetFileDateModified(), dateAdded);
    int64_t dateTaken = PhotoFileUtils::NormalizeTimestamp(data->GetDateTaken(), min(dateAdded, dateModified));

    value.Put(MediaColumn::MEDIA_DATE_ADDED, dateAdded);
    value.Put(MediaColumn::MEDIA_DATE_MODIFIED, dateModified);
    value.Put(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);

    std::string detailTime = data->GetDetailTime();
    const auto [normalizeDateTaken, normalizeDetailTime] =
        PhotoFileUtils::ExtractTimeInfo(detailTime, PhotoColumn::PHOTO_DETAIL_TIME_FORMAT);
    if (normalizeDateTaken < MIN_MILSEC_TIMESTAMP || normalizeDateTaken > MAX_MILSEC_TIMESTAMP ||
        abs(dateTaken - normalizeDateTaken) > MAX_TIMESTAMP_DIFF) {
        MEDIA_ERR_LOG("invalid detailTime: %{public}s, dateTaken: %{public}lld",
            detailTime.c_str(),
            static_cast<long long>(dateTaken));
        detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    } else {
        detailTime = normalizeDetailTime;
    }

    value.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);

    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    value.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    value.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    value.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
}

static void FillFileInfo(FileInfo& fileInfo, const std::unique_ptr<Metadata>& data)
{
    fileInfo.size = data->GetFileSize();
    fileInfo.orientation = data->GetOrientation();
    fileInfo.mimeType = data->GetFileMimeType();
    fileInfo.shootingMode = data->GetShootingMode();
    fileInfo.frontCamera = data->GetFrontCamera();
    fileInfo.movingPhotoEffectMode = 0;
    if (data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS) ||
        data->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO)) {
        fileInfo.subtype = data->GetPhotoSubType();
    }
}

NativeRdb::ValuesBucket MediaFileInterworkScanner::GetInsertValue(FileInfo &fileInfo)
{
    NativeRdb::ValuesBucket value;
    value.PutString(PhotoColumn::PHOTO_STORAGE_PATH, fileInfo.originFilePath);
    value.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, fileInfo.albumId);
    value.PutString(MediaColumn::MEDIA_FILE_PATH, fileInfo.filePath);
    value.PutString(MediaColumn::MEDIA_TITLE, fileInfo.title);
    value.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    fileInfo.subtype = fileInfo.isLivePhoto ? static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)
                                           : static_cast<int32_t>(PhotoSubType::DEFAULT);
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    int32_t errCode = FillMetadata(fileInfo, data);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, value, "failed to fill meta data, ret:%{public}d", errCode);
    // [time info]date_taken, date_modified, date_added, detail_time, date_year, date_month, date_day
    SetTimeInfo(data, fileInfo, value);
    FillFileInfo(fileInfo, data);
    value.PutInt(PhotoColumn::PHOTO_SUBTYPE, fileInfo.subtype);
    value.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    value.PutInt(PhotoColumn::PHOTO_ORIENTATION, data->GetOrientation());
    value.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, data->GetExifRotate());
    value.PutString(MediaColumn::MEDIA_MIME_TYPE, data->GetFileMimeType());
    value.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, data->GetFileExtension());
    value.PutInt(MediaColumn::MEDIA_TYPE, fileInfo.mediaType);
    value.PutLong(MediaColumn::MEDIA_SIZE, data->GetFileSize());
    value.PutInt(MediaColumn::MEDIA_DURATION, data->GetFileDuration());
    value.PutLong(MediaColumn::MEDIA_TIME_PENDING, -1);
    value.PutInt(PhotoColumn::PHOTO_HEIGHT, data->GetFileHeight());
    value.PutInt(PhotoColumn::PHOTO_WIDTH, data->GetFileWidth());
    value.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, data->GetFileAspectRatio());
    value.PutDouble(PhotoColumn::PHOTO_LONGITUDE, data->GetLongitude());
    value.PutDouble(PhotoColumn::PHOTO_LATITUDE, data->GetLatitude());
    value.PutString(PhotoColumn::PHOTO_ALL_EXIF, data->GetAllExif());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, data->GetShootingMode());
    value.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, data->GetShootingModeTag());
    value.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, data->GetLastVisitTime());
    value.PutString(PhotoColumn::PHOTO_FRONT_CAMERA, data->GetFrontCamera());
    value.PutInt(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, data->GetDynamicRangeType());
    value.PutInt(PhotoColumn::PHOTO_HDR_MODE, data->GetHdrMode());
    value.PutString(PhotoColumn::PHOTO_USER_COMMENT, data->GetUserComment());
    value.PutInt(PhotoColumn::PHOTO_QUALITY, 0);
    value.PutInt(PhotoColumn::PHOTO_VIDEO_MODE, data->GetVideoMode());
    return value;
}

int32_t MediaFileInterworkScanner::UpdateUniqueNumber(UniqueNumber &uniqueNumber)
{
    int32_t errCode = MediaLibraryAssetOperations::CreateAssetUniqueIds(
        MediaType::MEDIA_TYPE_IMAGE, uniqueNumber.imageTotalNumber, uniqueNumber.imageCurrentNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateAssetUniqueNumber IMAGE fail!, ret:%{public}d", errCode);

    MEDIA_DEBUG_LOG("imageTotalNumber: %{public}d imageCurrentNumber: %{public}d",
        uniqueNumber.imageTotalNumber, uniqueNumber.imageCurrentNumber);
    errCode = MediaLibraryAssetOperations::CreateAssetUniqueIds(
        MediaType::MEDIA_TYPE_VIDEO, uniqueNumber.videoTotalNumber, uniqueNumber.videoCurrentNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "CreateAssetUniqueNumber VIDEO fail!, ret:%{public}d", errCode);
    MEDIA_DEBUG_LOG("videoTotalNumber: %{public}d videoCurrentNumber: %{public}d",
        uniqueNumber.videoTotalNumber, uniqueNumber.videoCurrentNumber);
    return E_OK;
}

vector<FileInfo> MediaFileInterworkScanner::SetDestinationPath(
    vector<FileInfo> &restoreFiles, UniqueNumber &uniqueNumber)
{
    vector<FileInfo> newRestoreFiles;
    for (auto &fileInfo : restoreFiles) {
        string mediaDirPath = "Photo/";
        int32_t mediaType = fileInfo.mediaType;
        if (mediaDirPath.empty()) {
            MEDIA_ERR_LOG("get asset root dir failed. mediaType: %{public}d", mediaType);
            continue;
        }
        int32_t fileId = 0;
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            fileId = uniqueNumber.imageCurrentNumber++;
        } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            fileId = uniqueNumber.videoCurrentNumber++;
        }

        int32_t bucketNum = 0;
        int32_t retCode = MediaFileUri::CreateAssetBucket(fileId, bucketNum);
        if (retCode != E_OK) {
            MEDIA_ERR_LOG("CreateAssetBucket failed. bucketNum: %{public}d", bucketNum);
            continue;
        }

        string realName;
        retCode = MediaFileUtils::CreateAssetRealName(fileId, fileInfo.mediaType, fileInfo.extension, realName);
        if (retCode != E_OK) {
            MEDIA_ERR_LOG("CreateAssetRealName failed. retCode: %{public}d", retCode);
            continue;
        }
        string dirPath = ROOT_MEDIA_DIR + mediaDirPath + to_string(bucketNum);
        if (!MediaFileUtils::IsFileExists(dirPath)) {
            if (!MediaFileUtils::CreateDirectory(dirPath)) {
                MEDIA_ERR_LOG("CreateDirectory failed. retCode: %{public}s", dirPath.c_str());
                continue;
            }
        }
        fileInfo.filePath = dirPath + "/" + realName;
        newRestoreFiles.push_back(fileInfo);
    }
    return newRestoreFiles;
}

vector<FileInfo> MediaFileInterworkScanner::GetFileInfos(
    const vector<string> &filePathVector, UniqueNumber &uniqueNumber)
{
    vector<FileInfo> restoreFiles;
    for (const auto &filePath : filePathVector) {
        if (!MediaFileUtils::IsFileExists(filePath)) {
            MEDIA_ERR_LOG("File [%{public}s] not exist, skip restore.",
                MediaFileUtils::DesensitizePath(filePath).c_str());
            continue;
        }
        FileInfo fileInfo;
        fileInfo.fileName = MediaFileUtils::GetFileName(filePath);
        fileInfo.displayName = fileInfo.fileName;
        fileInfo.originFilePath = filePath;
        fileInfo.extension = ScannerUtils::GetFileExtension(fileInfo.fileName);
        fileInfo.title = ScannerUtils::GetFileTitle(fileInfo.fileName);
        fileInfo.mediaType = MediaFileUtils::GetMediaType(fileInfo.fileName);
        fileInfo.isLivePhoto = MovingPhotoFileUtils::IsLivePhoto(filePath);
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_FILE) {
            fileInfo.mediaType = MediaFileUtils::GetMediaTypeNotSupported(fileInfo.fileName);
            if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE ||
                fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
                MEDIA_WARN_LOG("media type not support, fileName is %{private}s", fileInfo.fileName.c_str());
            }
        }
        if (fileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
            uniqueNumber.imageTotalNumber++;
        } else if (fileInfo.mediaType == MediaType::MEDIA_TYPE_VIDEO) {
            uniqueNumber.videoTotalNumber++;
        } else {
            MEDIA_ERR_LOG(
                "This media type[%{public}s] is not image or video, skip restore.", fileInfo.extension.c_str());
            continue;
        }
        restoreFiles.push_back(fileInfo);
    }
    return restoreFiles;
}

std::vector<string> MediaFileInterworkScanner::GetPhotosNotExists(
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore, const std::vector<string> &files)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_STORAGE_PATH, files);
    std::vector<std::string> columns = {
        PhotoColumn::PHOTO_STORAGE_PATH
    };
    
    std::vector<string> filesPath = files;
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, filesPath, "query failed");
    while (resultSet->GoToNextRow() == E_OK) {
        string storagePath;
        int32_t result = resultSet->GetString(0, storagePath);
        if (result == E_OK) {
            filesPath.erase(std::remove(filesPath.begin(), filesPath.end(), storagePath), filesPath.end());
        } else {
            MEDIA_WARN_LOG("Get storagePath fail: %{public}d", result);
        }
    }
    resultSet->Close();
    return filesPath;
}

int32_t MediaFileInterworkScanner::SetRestoreFileAlbumId(std::vector<FileInfo> &destRestoreFiles)
{
    for (FileInfo &fileInfo : destRestoreFiles) {
        string albumPath = MediaFileUtils::GetParentPath(fileInfo.originFilePath);
        string lPath;
        int32_t ret = MediaFileInterworkUtil::GetFileAlbumLPath(albumPath, lPath);
        if (albumCache_.find(lPath) == albumCache_.end()) {
            int32_t albumId = 0;
            CHECK_AND_RETURN_RET_LOG(MediaFileInterworkUtil::InsertOrUpdateAlbum(albumPath, albumId) == E_OK,
                E_ERR, "insertorupdate album failed");
            albumCache_[lPath] = albumId;
            fileInfo.albumId = albumId;
            MEDIA_INFO_LOG("SetRestoreFileAlbumId albumId:%{public}d.", fileInfo.albumId);
        } else {
            fileInfo.albumId = albumCache_.at(lPath);
        }
    }
    return E_OK;
}

static void NotifyAnalysisAlbum(const vector<string>& changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const string& albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

static void UpdateAndNotifyShootingModeAlbumIfNeeded(const vector<FileInfo>& fileInfos)
{
    set<ShootingModeAlbumType> albumTypesSet;
    for (const auto &fileInfo : fileInfos) {
        vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
            fileInfo.subtype, fileInfo.mimeType, fileInfo.movingPhotoEffectMode,
            fileInfo.frontCamera, fileInfo.shootingMode);
        albumTypesSet.insert(albumTypes.begin(), albumTypes.end());
    }

    vector<string> albumIdsToUpdate;
    for (const auto& type : albumTypesSet) {
        int32_t albumId;
        if (MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId)) {
            albumIdsToUpdate.push_back(to_string(albumId));
        }
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbstore is nullptr");

    if (albumIdsToUpdate.size() > 0) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsToUpdate);
        NotifyAnalysisAlbum(albumIdsToUpdate);
    }
}

int32_t MediaFileInterworkScanner::BatchUpdateTimePending(const vector<FileInfo> &restoreFiles,
    AccurateRefresh::AssetAccurateRefresh &assetRefresh)
{
    // 构建包含所有文件路径的参数列表
    std::vector<std::string> filePahts;
    for (const auto& file : restoreFiles) {
        filePahts.push_back(file.filePath);
    }

    NativeRdb::ValuesBucket values;
    values.Put(MediaColumn::MEDIA_TIME_PENDING, 0);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_FILE_PATH, filePahts);
    int32_t changeRows = -1;

    int32_t errCode = assetRefresh.Update(changeRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((errCode == E_OK && changeRows > 0), E_HAS_DB_ERROR,
        "BatchUpdateTimePending: update time_pending failed. errCode: %{public}d, updateRows: %{public}d", errCode,
        changeRows);
    return E_OK;
}

int32_t MediaFileInterworkScanner::HandlePhotosRestore(const std::vector<string> &files)
{
    if (files.empty()) {
        MEDIA_WARN_LOG("HandlePhotosRestore success.");
        return E_OK;
    }
    MEDIA_INFO_LOG("HandlePhotosRestore filessize: %{public}zu", files.size());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");
    std::vector<string> filesToInsert = GetPhotosNotExists(rdbStore, files);
    UniqueNumber uniqueNumber;
    vector<FileInfo> restoreFiles = GetFileInfos(filesToInsert, uniqueNumber);
    CHECK_AND_RETURN_RET_INFO_LOG(!restoreFiles.empty(), E_OK, "no need to restore");
    int32_t errCode = UpdateUniqueNumber(uniqueNumber);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "UpdateUniqueNumber failed. errCode: %{public}d", errCode);

    vector<FileInfo> destRestoreFiles = SetDestinationPath(restoreFiles, uniqueNumber);
    CHECK_AND_RETURN_RET_LOG(destRestoreFiles.size() > 0, E_ERR, "restore file number is zero.");
    CHECK_AND_RETURN_RET_LOG(SetRestoreFileAlbumId(destRestoreFiles) == E_OK, E_ERR, "get album failed");
    int32_t result = BatchInsert(destRestoreFiles);
    CHECK_AND_RETURN_RET_LOG(result == E_OK, result, "BatchInsert photos failed");
    MediaFileInterworkUtil::AddImageAndVideoCount(uniqueNumber.imageTotalNumber, uniqueNumber.videoTotalNumber);
    MediaFileInterworkUtil::AddAlbumCount(static_cast<int32_t>(albumCache_.size()));
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SCAN_FILE_BUSSINESS_NAME);
    errCode = BatchUpdateTimePending(destRestoreFiles, assetRefresh);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "BatchUpdateTimePending failed. errCode: %{public}d", errCode);
    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    UpdateAndNotifyShootingModeAlbumIfNeeded(destRestoreFiles);
    MEDIA_INFO_LOG("HandlePhotosRestore success.");
    return E_OK;
}

int32_t MediaFileInterworkScanner::BatchInsert(std::vector<FileInfo> &files)
{
    std::vector<NativeRdb::ValuesBucket> values;
    for (auto& fileInfo : files) {
        NativeRdb::ValuesBucket value = GetInsertValue(fileInfo);
        values.push_back(value);
    }
    int64_t rowNum = 0;
    int32_t errCode = E_ERR;
    TransactionOperations trans{__func__};
    std::function<int(void)> func = [&]() -> int {
        errCode = trans.BatchInsert(rowNum, PhotoColumn::PHOTOS_TABLE, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "BatchInsert failed, errCode: %{public}d,"
            " rowNum: %{public}" PRId64,
            errCode,
            rowNum);
        return errCode;
    };
    
    errCode = trans.RetryTrans(func, true);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("BatchInsertFilesToPhotos failed, ret = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

std::shared_ptr<NativeRdb::ResultSet> MediaFileInterworkScanner::GetOptFile(
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    NativeRdb::RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    predicates.EqualTo(MediaFileInterworkColumn::OPT_STATUS_COLUMN, OPT_INIT_STATUS);
    predicates.Limit(BATCH_INSERT_SIZE);
    std::vector<std::string> columns = {
        MediaFileInterworkColumn::AFTER_PATH_COLUMN,
    };
    return rdbStore->Query(predicates, columns);
}

int32_t MediaFileInterworkScanner::UpdateOptStatus(
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore, std::vector<string> fileBatch)
{
    NativeRdb::RdbPredicates predicates(MediaFileInterworkColumn::OPT_TABLE_NAME);
    predicates.In(MediaFileInterworkColumn::AFTER_PATH_COLUMN, fileBatch);
    NativeRdb::ValuesBucket values;
    values.PutInt(MediaFileInterworkColumn::OPT_STATUS_COLUMN, OPT_FINISH_STATUS);
    int32_t changeRow = 0;
    return rdbStore->Update(changeRow, values, predicates);
}

int32_t MediaFileInterworkScanner::ProcessPhaseTwoRecords()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");
    int32_t processedCount = 0;
    int32_t offset = 0;
    while (true) {
        if (!CheckSystemConditions()) {
            MEDIA_INFO_LOG("System conditions not met, pausing task");
            return E_ERR;
        }
        std::vector<string> fileBatch;
        auto resultSet = GetOptFile(rdbStore);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetOptFile failed");
        int32_t batchProcessedCount = 0;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string path;
            int pathIndex = 0;
            if (resultSet->GetString(pathIndex, path) != NativeRdb::E_OK) {
                MEDIA_ERR_LOG("GetString failed for after_path");
                continue;
            }
            fileBatch.push_back(path);
            batchProcessedCount++;
            processedCount++;
        }
        resultSet->Close();
        if (!fileBatch.empty()) {
            int32_t ret = HandlePhotosRestore(fileBatch);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "BatchInsertFilesToPhotos failed, ret = %{public}d", ret);
        }
        CHECK_AND_RETURN_RET_LOG(UpdateOptStatus(rdbStore, fileBatch) == E_OK, E_ERR,
            "BatchUpdateFilesToPhotos failed");
        if (batchProcessedCount < BATCH_INSERT_SIZE) {
            MEDIA_INFO_LOG("Reached end of data, batchProcessedCount: %{public}d", batchProcessedCount);
            break;
        }
        MEDIA_INFO_LOG("Processed batch, next offset: %{public}d", offset);
    }

    int32_t ret = CleanTabFileOptTable();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CleanTabFileOptTable failed, ret = %{public}d", ret);
    MEDIA_INFO_LOG("Phase Two processed %{public}d files total", processedCount);
    return E_OK;
}

void MediaFileInterworkScanner::ScanFileManager()
{
    MEDIA_INFO_LOG("ScanFileManager");
    int32_t status = MediaFileInterworkUtil::GetScannerTaskStatus();
    CHECK_AND_RETURN_LOG(status != TASK_STATUS_COMPLETED, "already storage in");
    CHECK_AND_RETURN_LOG(!isAsyncTaskRunning_.exchange(true), "Scanning, wait");
    if (status == TASK_STATUS_IDLE) {
        status = TASK_STATUS_PHASE_ONE;
        MediaFileInterworkUtil::SetScannerTaskStatus(status);
        MediaFileInterworkUtil::SetLoadFirstTime();
    }
    taskThread_ = std::thread([this, status]() {
        std::lock_guard<std::mutex> lock(asyncTaskMutex_);
        ConsistencyCheckManager::GetInstance().DisableCheck();
        bool goAhead = false;
        if (status == TASK_STATUS_PHASE_ONE) {
            MEDIA_INFO_LOG("Starting Phase One");
            int32_t ret = ExecutePhaseOne();
            CHECK_AND_PRINT_LOG(ret == E_OK, "Phase One failed, ret = %{public}d", ret);
            goAhead = (ret == E_OK);
        }
        if (status == TASK_STATUS_PHASE_TWO || goAhead) {
            MEDIA_INFO_LOG("Starting Phase Two");
            int32_t ret = ExecutePhaseTwo();
            CHECK_AND_PRINT_LOG(ret == E_OK, "Phase One failed, ret = %{public}d", ret);
        }
        if (status == TASK_STATUS_COMPLETED) {
            MEDIA_INFO_LOG("Task already completed or in invalid state: %{public}d", status);
        }
        ConsistencyCheckManager::GetInstance().EnableCheck();
        albumCache_.clear();
        isAsyncTaskRunning_.store(false);
    });
    taskThread_.detach();
}
} // namespace OHOS::Media