/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "PhotosDataHandler"

#include "photos_data_handler.h"

#include <sstream>

#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"

namespace OHOS::Media {
void PhotosDataHandler::OnStart(int32_t sceneCode, const std::string &taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    SetSceneCode(sceneCode).SetTaskId(taskId).SetMediaLibraryRdb(mediaLibraryRdb);
}

PhotosDataHandler &PhotosDataHandler::SetSceneCode(int32_t sceneCode)
{
    sceneCode_ = sceneCode;
    return *this;
}

PhotosDataHandler &PhotosDataHandler::SetTaskId(const std::string &taskId)
{
    taskId_ = taskId;
    return *this;
}

PhotosDataHandler &PhotosDataHandler::SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    mediaLibraryRdb_ = mediaLibraryRdb;
    photosDao_.SetMediaLibraryRdb(mediaLibraryRdb);
    return *this;
}

void PhotosDataHandler::HandleDirtyFiles(bool isRestore)
{
    if (!isRestore) {
        MEDIA_WARN_LOG("Only handle dirty files in restore scene");
        return;
    }
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t totalNumber = photosDao_.GetDirtyFilesCount();
    MEDIA_INFO_LOG("totalNumber = %{public}d", totalNumber);
    CHECK_AND_RETURN(totalNumber > 0);
    ffrt_set_cpu_worker_max_num(ffrt::qos_utility, MAX_THREAD_NUM);
    int64_t startClean = MediaFileUtils::UTCTimeMilliSeconds();
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        ffrt::submit([this, offset]() { HandleDirtyFilesBatch(offset); }, { &offset }, {},
            ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_utility)));
    }
    ffrt::wait();
    int64_t startSetVisible = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t setCount = PhotosDataHandler::SetVisibleFilesInDb();
    int64_t startDelete = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t deleteCount = PhotosDataHandler::DeleteDirtyFilesInDb();
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("HANDLE_DIRTY_FILES", "",
            "total dirty count: " + std::to_string(totalNumber) +
            ", query total cost: " + std::to_string(startClean - startQuery) +
            "; clean files count: " + std::to_string(dirtyFileCleanNumber_) +
            ", clean files cost: " + std::to_string(startDelete - startClean) +
            ", failed clean files count: " + std::to_string(failedDirtyFileCleanNumber_) +
            "; set visible count: " + std::to_string(setCount) +
            ", update in db cost: " + std::to_string(end - startSetVisible) +
            "; clean db count: " + std::to_string(deleteCount) +
            ", delete in db cost: " + std::to_string(end - startDelete));
}

void PhotosDataHandler::HandleDirtyFilesBatch(int32_t offset)
{
    MEDIA_INFO_LOG("start clean dirty files offset: %{public}d", offset);
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<PhotosDao::PhotosRowData> dirtyFiles = photosDao_.GetDirtyFiles(offset);
    int64_t startClean = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileCleanNumber_ += CleanDirtyFiles(dirtyFiles);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("query %{public}zu dirty files cost %{public}" PRId64", clean cost %{public}" PRId64,
        dirtyFiles.size(), startClean - startQuery, end - startClean);
}

int32_t PhotosDataHandler::CleanDirtyFiles(const std::vector<PhotosDao::PhotosRowData> &dirtyFiles)
{
    int32_t count = 0;
    CHECK_AND_RETURN_RET(!dirtyFiles.empty(), count);
    for (const auto &dirtyFile : dirtyFiles) {
        if (ShouldSetVisible(dirtyFile)) {
            IsFileExist(dirtyFile) ? AddToSetVisibleFiles(dirtyFile) : AddToCleanFailedFiles(dirtyFile);
            continue;
        }
        if (!DeleteDirtyFile(dirtyFile)) {
            AddToCleanFailedFiles(dirtyFile);
            continue;
        }
        count++;
    }
    return count;
}

bool PhotosDataHandler::DeleteDirtyFile(const PhotosDao::PhotosRowData &dirtyFile)
{
    // clean cloud path
    bool deleteFileRet = MediaFileUtils::DeleteFileOrFolder(dirtyFile.data, true);
    // clean thumbs folder
    std::string thumbsFolder =
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::CLOUD_THUMB, dirtyFile.data);
    bool deleteThumbsRet = MediaFileUtils::DeleteFileOrFolder(thumbsFolder, false);
    // clean editData folder
    std::string editDataFolder =
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::CLOUD_EDIT_DATA, dirtyFile.data);
    bool deleteEditDataRet = MediaFileUtils::DeleteFileOrFolder(editDataFolder, false);
    CHECK_AND_RETURN_RET_LOG(deleteFileRet && deleteThumbsRet && deleteEditDataRet, false,
        "Clean file failed, path: %{public}s, deleteFileRet: %{public}d, deleteThumbsRet: %{public}d, "
        "deleteEditDataRet: %{public}d, errno: %{public}d",
        BackupFileUtils::GarbleFilePath(dirtyFile.data, DEFAULT_RESTORE_ID).c_str(),
        deleteFileRet, deleteThumbsRet, deleteEditDataRet, errno);
    return true;
}

int32_t PhotosDataHandler::DeleteDirtyFilesInDb()
{
    // clean database
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    predicates.NotIn(MediaColumn::MEDIA_ID, cleanFailedFiles_);
    int32_t changedRows = 0;
    int32_t deleteDbRet = BackupDatabaseUtils::Delete(predicates, changedRows, mediaLibraryRdb_);
    MEDIA_INFO_LOG("changedRows: %{public}d, deleteRet: %{public}d", changedRows, deleteDbRet);
    return changedRows;
}

int32_t PhotosDataHandler::SetVisibleFilesInDb()
{
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(PhotoColumn::PHOTOS_TABLE);
    predicates->EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_BACKUP));
    predicates->In(MediaColumn::MEDIA_ID, setVisibleFiles_);

    int32_t changedRows = 0;
    int32_t updateRet = BackupDatabaseUtils::Update(mediaLibraryRdb_, changedRows, valuesBucket, predicates);
    MEDIA_INFO_LOG("changedRows: %{public}d, updateRet: %{public}d", changedRows, updateRet);
    return changedRows;
}

bool PhotosDataHandler::ShouldSetVisible(const PhotosDao::PhotosRowData &dirtyFile)
{
    return sceneCode_ == UPGRADE_RESTORE_ID && dirtyFile.position == static_cast<int32_t>(PhotoPositionType::LOCAL) &&
        IsLocalFileExist(dirtyFile);
}

void PhotosDataHandler::AddToCleanFailedFiles(const PhotosDao::PhotosRowData &dirtyFile)
{
    std::lock_guard<mutex> lock(cleanFailedFilesMutex_);
    cleanFailedFiles_.emplace_back(std::to_string(dirtyFile.fileId));
    failedDirtyFileCleanNumber_++;
}

void PhotosDataHandler::AddToSetVisibleFiles(const PhotosDao::PhotosRowData &dirtyFile)
{
    std::lock_guard<mutex> lock(setVisibleFilesMutex_);
    setVisibleFiles_.emplace_back(std::to_string(dirtyFile.fileId));
}

bool PhotosDataHandler::IsLocalFileExist(const PhotosDao::PhotosRowData &dirtyFile)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::StartsWith(dirtyFile.data, RESTORE_FILES_CLOUD_DIR), false,
        "Invalid prefix, path: %{public}s",
        BackupFileUtils::GarbleFilePath(dirtyFile.data, DEFAULT_RESTORE_ID).c_str());
    std::string localPath =
        BackupFileUtils::GetReplacedPathByPrefixType(PrefixType::CLOUD, PrefixType::LOCAL, dirtyFile.data);
    return MediaFileUtils::IsFileExists(localPath);
}

bool PhotosDataHandler::IsFileExist(const PhotosDao::PhotosRowData &dirtyFile)
{
    bool ret = dirtyFile.subtype != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ?
        BackupFileUtils::IsValidFile(dirtyFile.data) :
        BackupFileUtils::IsValidFile(dirtyFile.data) && BackupFileUtils::IsMovingPhotoExist(dirtyFile.data);
    CHECK_AND_EXECUTE(ret, ReportDirtyFile("FILE_NOT_EXIST", dirtyFile));
    return ret;
}

void PhotosDataHandler::ReportDirtyFile(const std::string &errorCode, const PhotosDao::PhotosRowData &dirtyFile)
{
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .ReportInAudit("HANDLE_DIRTY_FILE", errorCode,
            "Dirty file id: " + std::to_string(dirtyFile.fileId) +
            ", subtype: " + std::to_string(dirtyFile.subtype) +
            ", data: " + BackupFileUtils::GarbleFilePath(dirtyFile.data, DEFAULT_RESTORE_ID));
}
}  // namespace OHOS::Media
