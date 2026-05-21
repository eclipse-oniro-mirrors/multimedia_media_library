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
#define MLOG_TAG "FileManagerCheckScenario"

#include "file_manager_check_scenario.h"

#include <algorithm>

#include "album_scan_info_column.h"
#include "check_status_helper.h"
#include "file_manager_scanner.h"
#include "file_manager_scan_rule_config.h"
#include "file_scan_utils.h"
#include "global_scanner.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdb_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_file_monitor_rdb_utils.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "photo_album_column.h"

namespace OHOS::Media {
const std::string FILE_MANAGER_ALBUM_PREFIX = "/FromDocs";
const std::string FILE_MANAGER_PATH_PREFIX = "/storage/media/local/files/Docs";

bool FileManagerCheckScenario::IsConditionSatisfied(const ConsistencyCheck::DeviceStatus &deviceStatus)
{
    const int32_t PROPER_DEVICE_BATTERY_CAPACITY = 50;
    const int64_t PROPER_PERIOD_IN_MS = 7 * 24 * 60 * 60 * 1000;

    return deviceStatus.isScreenOff && deviceStatus.isCharging && deviceStatus.isBackgroundTaskAllowed &&
        deviceStatus.batteryCapacity >= PROPER_DEVICE_BATTERY_CAPACITY &&
        IsTemperatureSatisfied(deviceStatus.temperature) && IsCheckPeriodSatisfied(PROPER_PERIOD_IN_MS);
}

bool FileManagerCheckScenario::IsTemperatureSatisfied(int32_t temperature)
{
    const int32_t TIME_START_RELEASE_TEMPERATURE_LIMIT = 1;
    const int32_t TIME_STOP_RELEASE_TEMPERATURE_LIMIT = 6;
    const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_37 = 1;
    const int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_40 = 2;

    std::time_t nowTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm nowLocalTime;
    CHECK_AND_RETURN_RET(localtime_r(&nowTime, &nowLocalTime) != nullptr, false);

    return (nowLocalTime.tm_hour >= TIME_START_RELEASE_TEMPERATURE_LIMIT &&
        nowLocalTime.tm_hour < TIME_STOP_RELEASE_TEMPERATURE_LIMIT) ?
        temperature <= PROPER_DEVICE_TEMPERATURE_LEVEL_40 : temperature <= PROPER_DEVICE_TEMPERATURE_LEVEL_37;
}

bool FileManagerCheckScenario::IsCheckPeriodSatisfied(int64_t requiredPeriodInMs)
{
    CheckStatusHelper checkStatusHelper(CheckScene::FILE_MANAGER);
    int64_t lastCheckTimeInMs = checkStatusHelper.GetLastCheckTimeInMs(0);
    int64_t currentTimeInMs = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t checkPeriodInMs = currentTimeInMs - lastCheckTimeInMs;
    MEDIA_DEBUG_LOG("lastCheckTimeInMs: %{public}" PRId64 ", currentTimeInMs: %{public}" PRId64 ", checkPeriodInMs: "
        "%{public}" PRId64 ", requiredPeriod: %{public}" PRId64,
        lastCheckTimeInMs, currentTimeInMs, checkPeriodInMs, requiredPeriodInMs);
    return checkPeriodInMs >= requiredPeriodInMs;
}

void FileManagerCheckScenario::Execute(std::atomic<bool> &isInterrupted)
{
    MEDIA_INFO_LOG("Start Execute");
    CheckDfxCollector dfxCollector(CheckScene::FILE_MANAGER);
    dfxCollector.OnCheckStart();
    ConsistencyCheck::ScenarioProgress progress = LoadProgress();
    ScenarioContext context = {isInterrupted, dfxCollector, progress};

    int32_t runningStatus = RunForward(context);
    if (runningStatus != RunningStatus::FINISHED) {
        MEDIA_ERR_LOG("RunForward not finished, end executing. RunningStatus: %{public}d", runningStatus);
        return;
    }

    runningStatus = RunBackwardPhoto(context);
    if (runningStatus == RunningStatus::INTERRUPTED) {
        MEDIA_WARN_LOG("RunBackwardPhoto interrupted");
        SaveCurrentProgress(progress);
        return;
    }

    runningStatus = RunBackwardAlbum(context);
    if (runningStatus == RunningStatus::INTERRUPTED) {
        MEDIA_INFO_LOG("RunBackwardAlbum interrupted");
        SaveCurrentProgress(progress);
        return;
    }

    SaveFinishedProgress();
    dfxCollector.OnCheckEnd();
    dfxCollector.Report();
}

int32_t FileManagerCheckScenario::RunForward(ScenarioContext &context)
{
    MEDIA_INFO_LOG("Start RunForward");
    auto &scanner = GlobalScanner::GetInstance();
    CHECK_AND_RETURN_RET(scanner.GetScannerStatus() == ScannerStatus::IDLE, RunningStatus::NOT_STARTED);

    scanner.RunFileManagerScan(std::string(FILE_MANAGER_ROOT_PATH), context.dfxCollector, false);

    return context.isInterrupted.load() ? RunningStatus::INTERRUPTED : RunningStatus::FINISHED;
}

int32_t FileManagerCheckScenario::RunBackwardPhoto(ScenarioContext &context)
{
    MEDIA_INFO_LOG("Start RunBackwardPhoto");
    std::vector<ConsistencyCheck::PhotoRecord> photoRecords;

    do {
        photoRecords = GetPhotoRecords(context);
        FileManagerCheckScenario::PhotoCandidates candidates = SelectPhotoCandidates(context, photoRecords);
        ProcessPhotoCandidates(context, candidates);
        ApplyPhotoChanges(candidates);
        SaveCurrentProgress(context.progress);
    } while (!context.isInterrupted.load() && photoRecords.size() == BATCH_SIZE);

    return context.isInterrupted.load() ? RunningStatus::INTERRUPTED : RunningStatus::FINISHED;
}

std::vector<ConsistencyCheck::PhotoRecord> FileManagerCheckScenario::GetPhotoRecords(ScenarioContext &context)
{
    std::vector<ConsistencyCheck::PhotoRecord> photoRecords;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoRecords, "GetPhotoRecords failed. rdbStore is nullptr");

    std::string querySql = "SELECT Photos.file_id, Photos.storage_path, Photos.data, Photos.position, "
                           "Photos.display_name, Photos.file_source_type, Photos.cloud_id, Photos.date_modified, "
                           "Photos.date_taken, Photos.subtype,"
                           "PhotoAlbum.album_id, PhotoAlbum.lpath, PhotoAlbum.album_subtype FROM Photos LEFT JOIN "
                           "PhotoAlbum ON Photos.owner_album_id = PhotoAlbum.album_id WHERE "
                           "Photos.file_id > ? AND Photos.sync_status = 0 AND Photos.clean_flag = 0 AND "
                           "Photos.time_pending = 0 AND Photos.is_temp = 0 AND "
                           "Photos.file_source_type = ? AND Photos.position IN (?, ?) ORDER BY Photos.file_id LIMIT ?";
    std::vector<NativeRdb::ValueObject> args = {context.progress.lastFileId, FileSourceType::FILE_MANAGER,
        static_cast<int32_t>(PhotoPositionType::LOCAL), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD),
        BATCH_SIZE};
    auto resultSet = rdbStore->QueryByStep(querySql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, photoRecords, "GetPhotoRecords failed. resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ConsistencyCheck::PhotoRecord photoRecord;
        photoRecord.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        photoRecord.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        photoRecord.data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        photoRecord.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        photoRecord.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        photoRecord.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
        photoRecord.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        photoRecord.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        photoRecord.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
        photoRecord.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        photoRecord.albumRecord.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        photoRecord.albumRecord.lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        photoRecord.albumRecord.albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        photoRecords.emplace_back(photoRecord);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("photoRecords size: %{public}zu", photoRecords.size());
    return photoRecords;
}

FileManagerCheckScenario::PhotoCandidates FileManagerCheckScenario::SelectPhotoCandidates(ScenarioContext &context,
    const std::vector<ConsistencyCheck::PhotoRecord> &photoRecords)
{
    FileManagerCheckScenario::PhotoCandidates candidates;
    for (const auto &photoRecord : photoRecords) {
        CHECK_AND_RETURN_RET(!context.isInterrupted.load(), candidates);

        context.progress.lastFileId = photoRecord.fileId;
        CHECK_AND_CONTINUE_ERR_LOG(!photoRecord.storagePath.empty(),
            "Get empty storage_path, %{public}s", photoRecord.ToString().c_str());
        if (MediaFileUtils::IsFileExists(photoRecord.storagePath)) {
            HandleExistingPhoto(photoRecord, candidates);
        } else {
            HandleNonExistingPhoto(photoRecord, candidates);
        }
    }
    return candidates;
}

void FileManagerCheckScenario::HandleExistingPhoto(const ConsistencyCheck::PhotoRecord &photoRecord,
    PhotoCandidates &candidates)
{
    int64_t fileDateModified = 0;
    if (!MediaFileUtils::GetDateModified(photoRecord.storagePath, fileDateModified)) {
        MEDIA_ERR_LOG("Get fileDateModified failed, %{public}s", photoRecord.ToString().c_str());
        return;
    }
    if (photoRecord.dateModified == fileDateModified) {
        return;
    }
    candidates.photosToScan.emplace_back(photoRecord);
    MEDIA_INFO_LOG("Scan %{public}s, reason: %{public}" PRId64 " != %{public}" PRId64,
        photoRecord.ToString().c_str(), photoRecord.dateModified, fileDateModified);
}

void FileManagerCheckScenario::HandleNonExistingPhoto(const ConsistencyCheck::PhotoRecord &photoRecord,
    PhotoCandidates &candidates)
{
    bool isCloudIdEmpty = photoRecord.cloudId.empty();
    bool isPostionNotSatisfied = photoRecord.position != static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    if (isCloudIdEmpty || isPostionNotSatisfied) {
        MEDIA_INFO_LOG("Delete %{public}s, reason: file not exist, cloudId: %{public}s, position: %{public}d",
            photoRecord.ToString().c_str(), photoRecord.cloudId.c_str(), photoRecord.position);
        candidates.photosToDelete.emplace_back(photoRecord);
        return;
    }

    candidates.photosToUpdatePosition.emplace_back(photoRecord);
    MEDIA_INFO_LOG("Update %{public}s, reason: file not exist, cloudId: %{public}s, position: %{public}d",
        photoRecord.ToString().c_str(), photoRecord.cloudId.c_str(), photoRecord.position);
}

void FileManagerCheckScenario::ProcessPhotoCandidates(ScenarioContext &context, PhotoCandidates &candidates)
{
    DeletePhotos(context, candidates);
    UpdatePhotosPosition(context, candidates.photosToUpdatePosition);
    ScanPhotos(context, candidates.photosToScan);
}

void FileManagerCheckScenario::DeletePhotos(ScenarioContext &context, PhotoCandidates &candidates)
{
    CHECK_AND_RETURN(!candidates.photosToDelete.empty());

    MEDIA_INFO_LOG("photosToDelete size: %{public}zu", candidates.photosToDelete.size());
    QueryAffectedAnalysisAlbumIds(candidates);
    DeletePhotoRecords(context, candidates.photosToDelete);
    DeletePhotoFiles(candidates.photosToDelete);
}

void FileManagerCheckScenario::QueryAffectedAnalysisAlbumIds(PhotoCandidates &candidates)
{
    std::vector<std::string> fileIdsArgs;
    std::transform(candidates.photosToDelete.begin(),
        candidates.photosToDelete.end(),
        std::back_inserter(fileIdsArgs),
        [](const ConsistencyCheck::PhotoRecord &record) { return std::to_string(record.fileId); });
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(fileIdsArgs, candidates.affectedAnalysisAlbumIds);
}

void FileManagerCheckScenario::DeletePhotoRecords(ScenarioContext &context,
    const std::vector<ConsistencyCheck::PhotoRecord> &photos)
{
    NativeRdb::AbsRdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> fileIdsArgs;
    std::transform(photos.begin(),
        photos.end(),
        std::back_inserter(fileIdsArgs),
        [](const ConsistencyCheck::PhotoRecord &record) { return std::to_string(record.fileId); });
    deletePredicates.In(MediaColumn::MEDIA_ID, fileIdsArgs);
    int32_t deletedRows = 0;
    int32_t ret = assetRefresh_.Delete(deletedRows, deletePredicates);
    if (ret != E_OK || deletedRows <= 0) {
        MEDIA_ERR_LOG("DeletePhotoRecords failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
        return;
    }
    context.dfxCollector.OnPhotoDelete(deletedRows);
    MEDIA_INFO_LOG("DeletePhotoRecords succeeded, deletedRows: %{public}d, dfxCollector: %{public}s", deletedRows,
        context.dfxCollector.ToString().c_str());
}

void FileManagerCheckScenario::DeletePhotoFiles(const std::vector<ConsistencyCheck::PhotoRecord> &photos)
{
    std::vector<std::string> ids;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int32_t> subtypes;
    for (const auto &photoRecord : photos) {
        ids.emplace_back(std::to_string(photoRecord.fileId));
        paths.emplace_back(photoRecord.data);
        dateTakens.emplace_back(std::to_string(photoRecord.dateTaken));
        subtypes.emplace_back(photoRecord.subtype);
    }
    MediaLibraryAssetOperations::TaskDataFileProcess(ids, paths, PhotoColumn::PHOTOS_TABLE, dateTakens, subtypes);
    MEDIA_INFO_LOG("DeletePhotoFiles finished");
}

void FileManagerCheckScenario::UpdatePhotosPosition(ScenarioContext &context,
    const std::vector<ConsistencyCheck::PhotoRecord> &photos)
{
    CHECK_AND_RETURN(!photos.empty());

    MEDIA_INFO_LOG("photosToUpdatePosition size: %{public}zu", photos.size());
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    NativeRdb::AbsRdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> fileIdsArgs;
    std::transform(photos.begin(),
        photos.end(),
        std::back_inserter(fileIdsArgs),
        [](const ConsistencyCheck::PhotoRecord &record) { return std::to_string(record.fileId); });
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIdsArgs);

    int32_t updatedRows = 0;
    int32_t ret = assetRefresh_.Update(updatedRows, values, updatePredicates);
    if (ret != NativeRdb::E_OK || updatedRows <= 0) {
        MEDIA_ERR_LOG("UpdatePhotosPosition failed, ret: %{public}d, updatedRows: %{public}d", ret, updatedRows);
        return;
    }
    context.dfxCollector.OnPhotoUpdate(updatedRows);
    MEDIA_INFO_LOG("UpdatePhotosPosition succeeded, updatedRows: %{public}d, dfxCollector: %{public}s", updatedRows,
        context.dfxCollector.ToString().c_str());
}

void FileManagerCheckScenario::ScanPhotos(ScenarioContext &context,
    const std::vector<ConsistencyCheck::PhotoRecord> &photos)
{
    CHECK_AND_RETURN(!photos.empty());

    MEDIA_INFO_LOG("photosToScan size: %{public}zu", photos.size());
    std::vector<MediaNotifyInfo> notifyInfos;
    for (const auto &record : photos) {
        MediaNotifyInfo notifyInfo;
        notifyInfo.afterPath = record.storagePath;
        notifyInfo.objType = FileNotifyObjectType::FILE;
        notifyInfo.optType = FileNotifyOperationType::MOD;
        notifyInfos.emplace_back(notifyInfo);
    }
    FileManagerScanner scanner;
    scanner.Run(notifyInfos);
    context.dfxCollector.OnPhotoUpdate(static_cast<int32_t>(photos.size()));
    MEDIA_INFO_LOG("ScanPhotos finished, dfxCollector: %{public}s", context.dfxCollector.ToString().c_str());
}

void FileManagerCheckScenario::ApplyPhotoChanges(const PhotoCandidates &candidates)
{
    assetRefresh_.RefreshAlbum();
    assetRefresh_.Notify();
    UpdateAnalysisAlbumsAndNotify(candidates);
}

void FileManagerCheckScenario::UpdateAnalysisAlbumsAndNotify(const PhotoCandidates &candidates)
{
    CHECK_AND_RETURN(!candidates.affectedAnalysisAlbumIds.empty());

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateAnalysisAlbumsAndNotify failed. rdbStore is nullptr");
    std::vector<std::string> albumIds(
        candidates.affectedAnalysisAlbumIds.begin(), candidates.affectedAnalysisAlbumIds.end());
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIds);
    MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
}

int32_t FileManagerCheckScenario::RunBackwardAlbum(ScenarioContext &context)
{
    MEDIA_INFO_LOG("Start RunBackwardAlbum");
    std::vector<ConsistencyCheck::AlbumRecord> albumRecords;

    do {
        albumRecords = GetAlbumRecords(context);
        FileManagerCheckScenario::AlbumCandidates albumCandidates = SelectAlbumCandidates(context, albumRecords);
        ProcessAlbumCandidates(context, albumCandidates);
        ClearAlbumScanInfo(albumRecords);
        ApplyAlbumChanges();
        SaveCurrentProgress(context.progress);
    } while (!context.isInterrupted.load() && albumRecords.size() == BATCH_SIZE);

    return context.isInterrupted.load() ? RunningStatus::INTERRUPTED : RunningStatus::FINISHED;
}

std::vector<ConsistencyCheck::AlbumRecord> FileManagerCheckScenario::GetAlbumRecords(ScenarioContext &context)
{
    std::vector<ConsistencyCheck::AlbumRecord> albumRecords;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, albumRecords, "GetAlbumRecords failed. rdbStore is nullptr");

    std::string querySql = "SELECT album_id, lpath, album_subtype FROM PhotoAlbum WHERE "
                           "album_id > ? AND album_subtype = ? AND NOT EXISTS (SELECT 1 FROM Photos WHERE "
                           "owner_album_id = album_id) ORDER BY album_id LIMIT ?";
    std::vector<NativeRdb::ValueObject> args = {context.progress.lastAlbumId,
        PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER, BATCH_SIZE};
    auto resultSet = rdbStore->QueryByStep(querySql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumRecords, "GetAlbumRecords failed. resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        ConsistencyCheck::AlbumRecord albumRecord;
        albumRecord.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        albumRecord.lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        albumRecord.albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        albumRecords.emplace_back(albumRecord);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("albumRecords size: %{public}zu", albumRecords.size());
    return albumRecords;
}

FileManagerCheckScenario::AlbumCandidates FileManagerCheckScenario::SelectAlbumCandidates(ScenarioContext &context,
    const std::vector<ConsistencyCheck::AlbumRecord> &albumRecords)
{
    FileManagerCheckScenario::AlbumCandidates candidates;
    for (const auto &albumRecord : albumRecords) {
        CHECK_AND_RETURN_RET(!context.isInterrupted.load(), candidates);

        context.progress.lastAlbumId = albumRecord.albumId;
        CHECK_AND_CONTINUE_ERR_LOG(!albumRecord.lpath.empty(), "Get empty lpath, %{public}s",
            albumRecord.ToString().c_str());
        std::string realPath = ConvertLpathToRealPath(albumRecord.lpath);
        if (realPath.empty()) {
            MEDIA_ERR_LOG("Get empty realPath, lpath: %{public}s",
                FileScanUtils::GarbleFilePath(albumRecord.lpath).c_str());
            continue;
        }
        if (MediaFileUtils::IsFileExists(realPath)) {
            continue;
        }
        candidates.albumsToDelete.emplace_back(albumRecord);
        MEDIA_INFO_LOG("Delete %{public}s, reason: folder not exist", albumRecord.ToString().c_str());
    }
    return candidates;
}

void FileManagerCheckScenario::ProcessAlbumCandidates(ScenarioContext &context,
    const FileManagerCheckScenario::AlbumCandidates &candidates)
{
    DeleteAlbums(context, candidates.albumsToDelete);
}

void FileManagerCheckScenario::DeleteAlbums(ScenarioContext &context,
    const std::vector<ConsistencyCheck::AlbumRecord> &albums)
{
    CHECK_AND_RETURN(!albums.empty());

    MEDIA_INFO_LOG("albumsToDelete size: %{public}zu", albums.size());
    int32_t photoAlbumDeletedRows = DeleteInPhotoAlbum(albums);
    if (photoAlbumDeletedRows <= 0) {
        return;
    }
    context.dfxCollector.OnAlbumDelete(photoAlbumDeletedRows);
    MEDIA_INFO_LOG("Delete succeeded, photoAlbumDeletedRows: %{public}d, dfxCollector: %{public}s",
        photoAlbumDeletedRows, context.dfxCollector.ToString().c_str());
}

int32_t FileManagerCheckScenario::DeleteInPhotoAlbum(const std::vector<ConsistencyCheck::AlbumRecord> &albums)
{
    NativeRdb::AbsRdbPredicates photoAlbumDeletePredicates(PhotoAlbumColumns::TABLE);
    std::vector<std::string> albumIdsArgs;
    std::transform(albums.begin(),
        albums.end(),
        std::back_inserter(albumIdsArgs),
        [](const ConsistencyCheck::AlbumRecord &record) { return std::to_string(record.albumId); });
    photoAlbumDeletePredicates.In(PhotoAlbumColumns::ALBUM_ID, albumIdsArgs);
    int32_t photoAlbumDeletedRows = 0;
    int32_t ret = albumRefresh_.Delete(photoAlbumDeletedRows, photoAlbumDeletePredicates);
    if (ret != E_OK || photoAlbumDeletedRows <= 0) {
        MEDIA_ERR_LOG("Delete failed, ret: %{public}d, photoAlbumDeletedRows: %{public}d", ret, photoAlbumDeletedRows);
    }
    return photoAlbumDeletedRows;
}

void FileManagerCheckScenario::ClearAlbumScanInfo(const std::vector<ConsistencyCheck::AlbumRecord> &albums)
{
    CHECK_AND_RETURN_LOG(!albums.empty(), "albumsToDelete is empty");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "ClearAlbumScanInfo failed. rdbStore is nullptr");

    auto iter = std::max_element(albums.begin(),
        albums.end(),
        [](const ConsistencyCheck::AlbumRecord &a, const ConsistencyCheck::AlbumRecord &b) {
            return a.albumId < b.albumId;
        });
    int32_t maxAlbumId = iter->albumId;

    NativeRdb::AbsRdbPredicates predicates(AlbumScanInfoColumn::TABLE);
    std::string whereClause =
        "album_id <= ? AND NOT EXISTS (SELECT 1 FROM PhotoAlbum WHERE PhotoAlbum.album_id = AlbumScanInfo.album_id "
        "AND LOWER(PhotoAlbum.lpath) = LOWER(REPLACE(AlbumScanInfo.storage_path, '/storage/media/local/files/Docs', "
        "'/FromDocs')))";
    std::vector<std::string> whereArgs = { std::to_string(maxAlbumId) };
    predicates.SetWhereClause(whereClause);
    predicates.SetWhereArgs(whereArgs);
    int32_t deletedRows = 0;
    int32_t ret = rdbStore->Delete(deletedRows, predicates);
    if (ret != E_OK || deletedRows < 0) {
        MEDIA_ERR_LOG("ClearAlbumScanInfo failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
        return;
    }
    MEDIA_INFO_LOG("ClearAlbumScanInfo succeeded, deletedRows: %{public}d", deletedRows);
}

void FileManagerCheckScenario::ApplyAlbumChanges()
{
    albumRefresh_.Notify();
}

std::string FileManagerCheckScenario::ConvertLpathToRealPath(const std::string &lpath)
{
    if (!MediaStringUtils::StartsWith(lpath, FILE_MANAGER_ALBUM_PREFIX)) {
        MEDIA_ERR_LOG("Convert failed, not start with /FromDocs, lpath: %{public}s",
            FileScanUtils::GarbleFilePath(lpath).c_str());
        return "";
    }
    std::string realPath = lpath;
    realPath.replace(0, FILE_MANAGER_ALBUM_PREFIX.length(), FILE_MANAGER_PATH_PREFIX);
    return realPath;
}

ConsistencyCheck::ScenarioProgress FileManagerCheckScenario::LoadProgress()
{
    MEDIA_INFO_LOG("Start LoadProgress");
    CheckStatusHelper checkStatusHelper(CheckScene::FILE_MANAGER);
    return checkStatusHelper.GetScenarioProgress();
}

void FileManagerCheckScenario::SaveCurrentProgress(const ConsistencyCheck::ScenarioProgress &progress)
{
    MEDIA_INFO_LOG("Start SaveCurrentProgress");
    CheckStatusHelper checkStatusHelper(CheckScene::FILE_MANAGER);
    checkStatusHelper.SetValuesByCurrentProgress(progress);
}

void FileManagerCheckScenario::SaveFinishedProgress()
{
    MEDIA_INFO_LOG("Start SaveFinishedProgress");
    ConsistencyCheck::ScenarioProgress progress;
    progress.lastCheckTimeInMs = MediaFileUtils::UTCTimeMilliSeconds();
    CheckStatusHelper checkStatusHelper(CheckScene::FILE_MANAGER);
    checkStatusHelper.SetValuesByFinishedProgress(progress);
}
} // namespace OHOS::Media
