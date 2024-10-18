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

#define MLOG_TAG "MovingPhotoProcessor"

#include "moving_photo_processor.h"

#include <fcntl.h>

#include "abs_rdb_predicates.h"
#include "cloud_sync_helper.h"
#include "directory_ex.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "mimetype_utils.h"
#include "moving_photo_file_utils.h"
#include "parameters.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static constexpr int32_t MOVING_PHOTO_PROCESS_NUM = 100;
static constexpr int32_t DIRTY_NOT_UPLOADING = -1;
static constexpr int32_t DEFAULT_EXTRA_DATA_SIZE = MIN_STANDARD_SIZE;
static constexpr int32_t LIVE_PHOTO_QUERY_NUM = 3000;
static constexpr int32_t LIVE_PHOTO_PROCESS_NUM = 200;

static const string MOVING_PHOTO_PROCESS_FLAG = "multimedia.medialibrary.cloneFlag";
static const string LIVE_PHOTO_COMPAT_DONE = "0";

bool MovingPhotoProcessor::isProcessing_ = false;

static void StopCloudSync()
{
    string currentTime = to_string(MediaFileUtils::UTCTimeSeconds());
    MEDIA_DEBUG_LOG("Stop cloud sync for processing moving photo: %{public}s", currentTime.c_str());
    bool retFlag = system::SetParameter(MOVING_PHOTO_PROCESS_FLAG, currentTime);
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to set parameter, retFlag: %{public}d", retFlag);
    }
}

static void StartCloudSync()
{
    MEDIA_DEBUG_LOG("Reset parameter for cloud sync");
    bool retFlag = system::SetParameter(MOVING_PHOTO_PROCESS_FLAG, "0");
    if (!retFlag) {
        MEDIA_ERR_LOG("Failed to set parameter for cloud sync, retFlag: %{public}d", retFlag);
    }
}

static bool IsCloudLivePhotoRefreshed()
{
    string refreshStatus = system::GetParameter(REFRESH_CLOUD_LIVE_PHOTO_FLAG, CLOUD_LIVE_PHOTO_REFRESHED);
    return refreshStatus.compare(CLOUD_LIVE_PHOTO_REFRESHED) == 0;
}

void MovingPhotoProcessor::StartProcessMovingPhoto()
{
    auto resultSet = QueryMovingPhoto();
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query moving photo");
        return;
    }

    MovingPhotoDataList dataList;
    ParseMovingPhotoData(resultSet, dataList);
    if (dataList.movingPhotos.empty()) {
        MEDIA_DEBUG_LOG("No moving photo need to be processed");
        return;
    }

    isProcessing_ = true;
    StopCloudSync();
    CompatMovingPhoto(dataList);
    StartCloudSync();
}

static string GetLivePhotoCompatId()
{
    return system::GetParameter(COMPAT_LIVE_PHOTO_FILE_ID, LIVE_PHOTO_COMPAT_DONE);
}

static bool IsLivePhotoCompatDone()
{
    string currentFileId = GetLivePhotoCompatId();
    return currentFileId.compare(LIVE_PHOTO_COMPAT_DONE) == 0;
}

static void SetLivePhotoCompatId(string fileId)
{
    bool ret = system::SetParameter(COMPAT_LIVE_PHOTO_FILE_ID, fileId);
    if (!ret) {
        MEDIA_ERR_LOG("Failed to set parameter for compating local live photo: %{public}s", fileId.c_str());
    }
}

void MovingPhotoProcessor::StartProcessLivePhoto()
{
    if (IsLivePhotoCompatDone()) {
        MEDIA_DEBUG_LOG("Live photo compat done or no need to compat");
        return;
    }

    auto resultSet = QueryCandidateLivePhoto();
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query candidate live photo");
        return;
    }

    LivePhotoDataList dataList;
    ParseLivePhotoData(resultSet, dataList);
    if (dataList.livePhotos.empty()) {
        SetLivePhotoCompatId(LIVE_PHOTO_COMPAT_DONE);
        MEDIA_INFO_LOG("No live photo need to compat");
        return;
    }

    isProcessing_ = true;
    CompatLivePhoto(dataList);
}

void MovingPhotoProcessor::StartProcess()
{
    MEDIA_DEBUG_LOG("Start processing moving photo task");

    // 1. compat old moving photo
    StartProcessMovingPhoto();

    // 2. compat local live photo
    StartProcessLivePhoto();

    // 3. refresh cloud live photo if needed
    if (!IsCloudLivePhotoRefreshed()) {
        MEDIA_INFO_LOG("Strat reset cloud cursor for cloud live photo");
        FileManagement::CloudSync::CloudSyncManager::GetInstance().ResetCursor();
        MEDIA_INFO_LOG("End reset cloud cursor for cloud live photo");
        bool ret = system::SetParameter(REFRESH_CLOUD_LIVE_PHOTO_FLAG, CLOUD_LIVE_PHOTO_REFRESHED);
        MEDIA_INFO_LOG("Set parameter of isRefreshed to 1, ret: %{public}d", ret);
    }

    isProcessing_ = false;
    MEDIA_DEBUG_LOG("Finsh processing moving photo task");
}

void MovingPhotoProcessor::StopProcess()
{
    if (isProcessing_) {
        isProcessing_ = false;
        StartCloudSync();
    }
}

shared_ptr<NativeRdb::ResultSet> MovingPhotoProcessor::QueryMovingPhoto()
{
    const vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_FILE_PATH,
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, DIRTY_NOT_UPLOADING)
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO))
        ->Or()
        ->EqualTo(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY))
        ->EndWrap()
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0)
        ->And()
        ->EqualTo(PhotoColumn::MEDIA_TIME_PENDING, 0)
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL))
        ->Or()
        ->IsNull(PhotoColumn::PHOTO_QUALITY)
        ->EndWrap()
        ->Limit(MOVING_PHOTO_PROCESS_NUM);
    return MediaLibraryRdbStore::Query(predicates, columns);
}

void MovingPhotoProcessor::ParseMovingPhotoData(shared_ptr<NativeRdb::ResultSet>& resultSet,
    MovingPhotoDataList& dataList)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        int32_t subtype = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        int32_t effectMode = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet, TYPE_INT32));
        int64_t size = get<int64_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_SIZE, resultSet, TYPE_INT64));
        std::string path = get<std::string>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));

        MovingPhotoData movingPhotoData;
        movingPhotoData.fileId = fileId;
        movingPhotoData.subtype = subtype;
        movingPhotoData.effectMode = effectMode;
        movingPhotoData.size = size;
        movingPhotoData.path = path;
        dataList.movingPhotos.push_back(movingPhotoData);
    }
}

void MovingPhotoProcessor::UpdateMovingPhotoData(const MovingPhotoData& movingPhotoData)
{
    ValuesBucket values;
    string whereClause = PhotoColumn::MEDIA_ID + " = ? AND " + PhotoColumn::PHOTO_DIRTY + " = ?";
    vector<string> whereArgs = { to_string(movingPhotoData.fileId), to_string(DIRTY_NOT_UPLOADING) };
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, movingPhotoData.subtype);
    values.PutLong(PhotoColumn::MEDIA_SIZE, movingPhotoData.size);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null");
        return;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("rdbStorePtr is null");
        return;
    }
    if (!isProcessing_) {
        MEDIA_INFO_LOG("stop updateing moving photo data");
        return;
    }
    int32_t updateCount = 0;
    int32_t result = rdbStorePtr->Update(updateCount, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    if (result != NativeRdb::E_OK || updateCount <= 0) {
        MEDIA_ERR_LOG("Update failed. result: %{public}d, updateCount: %{public}d", result, updateCount);
        return;
    }
}

static string GetDefaultExtraData()
{
    static string defaultExtraData = "v3_f0               0:0                 LIVE_10000000       ";
    return defaultExtraData;
}

int32_t MovingPhotoProcessor::GetUpdatedMovingPhotoData(const MovingPhotoData& currentData,
    MovingPhotoData& newData)
{
    newData = currentData;
    string imagePath = currentData.path;
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(imagePath);
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(imagePath);
    size_t imageSize = 0;
    size_t videoSize = 0;
    size_t extraSize = 0;
    if (!MediaFileUtils::GetFileSize(imagePath, imageSize) || imageSize == 0) {
        MEDIA_WARN_LOG("Failed to get image of moving photo, id: %{public}d", currentData.fileId);
        newData.size = -1; // set abnormal size to -1 if original size is 0
        newData.subtype = static_cast<int32_t>(PhotoSubType::DEFAULT);
        return E_OK;
    }

    if (!MediaFileUtils::GetFileSize(videoPath, videoSize) || videoSize == 0) {
        MEDIA_WARN_LOG("Failed to get video of moving photo, id: %{public}d", currentData.fileId);
        newData.size = static_cast<int64_t>(imageSize);
        newData.subtype = static_cast<int32_t>(PhotoSubType::DEFAULT);
        return E_OK;
    }

    if (MediaFileUtils::GetFileSize(extraDataPath, extraSize) && extraSize > 0) {
        newData.size = static_cast<int64_t>(imageSize + videoSize + extraSize);
        return E_OK;
    }
    string extraDataDir = MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(imagePath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(extraDataDir), E_HAS_FS_ERROR,
        "Cannot create dir %{private}s, errno:%{public}d", extraDataDir.c_str(), errno);
    if (!MediaFileUtils::IsFileExists(extraDataPath) && MediaFileUtils::CreateAsset(extraDataPath) != E_OK) {
        MEDIA_ERR_LOG("Failed to create extraData:%{private}s, errno:%{public}d", extraDataPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    if (!MediaFileUtils::WriteStrToFile(extraDataPath, GetDefaultExtraData())) {
        MEDIA_ERR_LOG("Failed to write extraData, errno:%{public}d", errno);
        return E_HAS_FS_ERROR;
    }
    newData.size = static_cast<int64_t>(imageSize + videoSize + DEFAULT_EXTRA_DATA_SIZE);
    return E_OK;
}

void MovingPhotoProcessor::CompatMovingPhoto(const MovingPhotoDataList& dataList)
{
    MEDIA_INFO_LOG("Start processing %{public}zu moving photos", dataList.movingPhotos.size());
    int32_t count = 0;
    for (const auto& movingPhoto : dataList.movingPhotos) {
        if (!isProcessing_) {
            MEDIA_INFO_LOG("stop compating moving photo");
            return;
        }
        MovingPhotoData newData;
        if (GetUpdatedMovingPhotoData(movingPhoto, newData) != E_OK) {
            MEDIA_INFO_LOG("Failed to get updated data of moving photo, id: %{public}d", movingPhoto.fileId);
            continue;
        }
        UpdateMovingPhotoData(newData);
        count += 1;
    }
    MEDIA_INFO_LOG("Finish processing %{public}d moving photos", count);
}

shared_ptr<NativeRdb::ResultSet> MovingPhotoProcessor::QueryCandidateLivePhoto()
{
    const vector<string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::PHOTO_POSITION,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::MEDIA_FILE_PATH,
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    string currentFileIdStr = GetLivePhotoCompatId();
    int32_t currentFileId = std::atoi(currentFileIdStr.c_str());
    MEDIA_INFO_LOG("Start query candidate live photo from file_id: %{public}d", currentFileId);
    predicates.GreaterThanOrEqualTo(PhotoColumn::MEDIA_ID, currentFileId)
        ->And()
        ->EqualTo(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_IMAGE))
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0)
        ->And()
        ->EqualTo(PhotoColumn::MEDIA_TIME_PENDING, 0)
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CAMERA))
        ->EndWrap()
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))
        ->EndWrap()
        ->OrderByAsc(PhotoColumn::MEDIA_ID)
        ->Limit(LIVE_PHOTO_QUERY_NUM);
    return MediaLibraryRdbStore::Query(predicates, columns);
}

void MovingPhotoProcessor::ParseLivePhotoData(shared_ptr<NativeRdb::ResultSet>& resultSet,
    LivePhotoDataList& dataList)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        int32_t mediaType = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_TYPE, resultSet, TYPE_INT32));
        int32_t subtype = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        int32_t position = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_POSITION, resultSet, TYPE_INT32));
        int64_t editTime = get<int64_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_EDIT_TIME, resultSet, TYPE_INT64));
        std::string path = get<std::string>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));

        LivePhotoData livePhotoData;
        livePhotoData.isLivePhoto = false;
        livePhotoData.fileId = fileId;
        livePhotoData.mediaType = mediaType;
        livePhotoData.subtype = subtype;
        livePhotoData.position = position;
        livePhotoData.editTime = editTime;
        livePhotoData.path = path;
        dataList.livePhotos.push_back(livePhotoData);
    }
}

void MovingPhotoProcessor::CompatLivePhoto(const LivePhotoDataList& dataList)
{
    MEDIA_INFO_LOG("Start processing %{public}zu candidate live photos", dataList.livePhotos.size());
    int32_t count = 0;
    int32_t livePhotoCount = 0;
    int32_t processedFileId = 0;
    for (const auto& livePhoto : dataList.livePhotos) {
        if (!isProcessing_) {
            SetLivePhotoCompatId(std::to_string(livePhoto.fileId));
            MEDIA_INFO_LOG("Stop compating live photo, file_id: %{public}d", livePhoto.fileId);
            return;
        }
        processedFileId = livePhoto.fileId;
        LivePhotoData newData;
        if (GetUpdatedLivePhotoData(livePhoto, newData) != E_OK) {
            MEDIA_INFO_LOG("Failed to get updated data of candidate live photo, id: %{public}d", livePhoto.fileId);
            continue;
        }
        if (newData.isLivePhoto) {
            UpdateLivePhotoData(newData);
            livePhotoCount += 1;
        }
        count += 1;

        if (livePhotoCount >= LIVE_PHOTO_PROCESS_NUM) {
            SetLivePhotoCompatId(std::to_string(livePhoto.fileId + 1));
            MEDIA_INFO_LOG("Stop compating live photo, %{public}d processed", livePhotoCount);
            return;
        }
    }
    SetLivePhotoCompatId(std::to_string(processedFileId + 1));
    MEDIA_INFO_LOG("Finish processing %{public}d candidates, contains %{public}d live photos, file_id: %{public}d",
        count, livePhotoCount, processedFileId);
}

static void addCompatPathSuffix(const string &oldPath, const string &suffix, string &newPath)
{
    if (oldPath.empty() || suffix.empty()) {
        MEDIA_WARN_LOG("oldPath or suffix is empty");
        return;
    }

    newPath = oldPath + ".compat" + suffix;
    while (MediaFileUtils::IsFileExists(newPath)) {
        newPath += ".dup" + suffix;
    }
}

static int32_t MoveMovingPhoto(const string &path,
    const string &compatImagePath, const string &compatVideoPath, const string &compatExtraDataPath)
{
    string movingPhotoVideoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
    string movingPhotoExtraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path);
    CHECK_AND_RETURN_RET_LOG(!movingPhotoVideoPath.empty(), E_INVALID_VALUES, "Failed to get video path");
    CHECK_AND_RETURN_RET_LOG(!movingPhotoExtraDataPath.empty(), E_INVALID_VALUES, "Failed to get extraData path");
    CHECK_AND_RETURN_RET_LOG(
        !MediaFileUtils::IsFileExists(movingPhotoVideoPath), E_INVALID_VALUES, "Video path exists!");
    CHECK_AND_RETURN_RET_LOG(
        !MediaFileUtils::IsFileExists(movingPhotoExtraDataPath), E_INVALID_VALUES, "extraData path exists");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(MovingPhotoFileUtils::GetMovingPhotoExtraDataDir(path)),
        E_HAS_FS_ERROR, "Failed to create extraData dir of %{private}s", path.c_str());

    int32_t ret = rename(compatExtraDataPath.c_str(), movingPhotoExtraDataPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename extraData, src: %{public}s, dest: %{public}s, errno: %{public}d",
            compatExtraDataPath.c_str(), movingPhotoExtraDataPath.c_str(), errno);
    }
    ret = rename(compatVideoPath.c_str(), movingPhotoVideoPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename video, src: %{public}s, dest: %{public}s, errno: %{public}d",
            compatVideoPath.c_str(), movingPhotoVideoPath.c_str(), errno);
    }
    ret = rename(compatImagePath.c_str(), path.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename image, src: %{public}s, dest: %{public}s, errno: %{public}d",
            compatImagePath.c_str(), path.c_str(), errno);
    }
    return ret;
}

int32_t MovingPhotoProcessor::ProcessLocalLivePhoto(LivePhotoData& data)
{
    data.isLivePhoto = false;
    bool isLivePhoto = MovingPhotoFileUtils::IsLivePhoto(data.path);
    if (!isLivePhoto) {
        return E_OK;
    }

    string livePhotoPath = data.path;
    string compatImagePath;
    string compatVideoPath;
    string compatExtraDataPath;
    addCompatPathSuffix(livePhotoPath, ".jpg", compatImagePath);
    addCompatPathSuffix(livePhotoPath, ".mp4", compatVideoPath);
    addCompatPathSuffix(livePhotoPath, ".extra", compatExtraDataPath);
    int32_t ret = MovingPhotoFileUtils::ConvertToMovingPhoto(
        livePhotoPath, compatImagePath, compatVideoPath, compatExtraDataPath);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to convert live photo, ret:%{public}d, file_id:%{public}d", ret, data.fileId);
        (void)MediaFileUtils::DeleteFile(compatImagePath);
        (void)MediaFileUtils::DeleteFile(compatVideoPath);
        (void)MediaFileUtils::DeleteFile(compatExtraDataPath);
        return ret;
    }

    uint64_t coverPosition = 0;
    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraphInfo = false;
    string absExtraDataPath;
    if (!PathToRealPath(compatExtraDataPath, absExtraDataPath)) {
        MEDIA_ERR_LOG("extraData is not real path: %{private}s, errno: %{public}d", compatExtraDataPath.c_str(), errno);
        return E_HAS_FS_ERROR;
    }
    UniqueFd extraDataFd(open(absExtraDataPath.c_str(), O_RDONLY));
    (void)MovingPhotoFileUtils::GetVersionAndFrameNum(extraDataFd.Get(), version, frameIndex, hasCinemagraphInfo);
    (void)MovingPhotoFileUtils::GetCoverPosition(compatVideoPath, frameIndex, coverPosition);
    data.coverPosition = static_cast<int64_t>(coverPosition);

    ret = MoveMovingPhoto(livePhotoPath, compatImagePath, compatVideoPath, compatExtraDataPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to move moving photo, file_id:%{public}d", data.fileId);
    data.subtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    data.isLivePhoto = true;
    return E_OK;
}

int32_t MovingPhotoProcessor::ProcessLocalCloudLivePhoto(LivePhotoData& data)
{
    if (data.editTime == 0) {
        return ProcessLocalLivePhoto(data);
    }

    data.isLivePhoto = false;
    string sourcePath = PhotoFileUtils::GetEditDataSourcePath(data.path);
    bool isLivePhotoEdited = MovingPhotoFileUtils::IsLivePhoto(sourcePath);
    if (!isLivePhotoEdited) {
        return E_OK;
    }
    data.metaDateModified = MediaFileUtils::UTCTimeMilliSeconds();
    data.isLivePhoto = true;
    return E_OK;
}

int32_t MovingPhotoProcessor::GetUpdatedLivePhotoData(const LivePhotoData& currentData, LivePhotoData& newData)
{
    newData = currentData;
    string path = currentData.path;
    string extension = ScannerUtils::GetFileExtension(path);
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    if (mimeType.compare("image/jpeg") != 0) {
        newData.isLivePhoto = false;
        return E_OK;
    }

    if (currentData.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        return ProcessLocalLivePhoto(newData);
    } else if (currentData.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        return ProcessLocalCloudLivePhoto(newData);
    } else {
        MEDIA_ERR_LOG("Invalid position to process: %{public}d", currentData.position);
        return E_INVALID_VALUES;
    }
}

void MovingPhotoProcessor::UpdateLivePhotoData(const LivePhotoData& livePhotoData)
{
    if (!livePhotoData.isLivePhoto) {
        return;
    }

    ValuesBucket values;
    string whereClause = PhotoColumn::MEDIA_ID + " = ?";
    vector<string> whereArgs = { to_string(livePhotoData.fileId) };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null");
        return;
    }
    auto rdbStorePtr = rdbStore->GetRaw();
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("rdbStorePtr is null");
        return;
    }

    if (livePhotoData.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, livePhotoData.subtype);
        values.PutLong(PhotoColumn::PHOTO_COVER_POSITION, livePhotoData.coverPosition);
    } else if (livePhotoData.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, livePhotoData.metaDateModified);
    } else {
        MEDIA_ERR_LOG("Invalid position: %{public}d", livePhotoData.position);
        return;
    }

    int32_t updateCount = 0;
    int32_t result = rdbStorePtr->Update(updateCount, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    if (result != NativeRdb::E_OK || updateCount <= 0) {
        MEDIA_ERR_LOG("Update failed. result: %{public}d, updateCount: %{public}d", result, updateCount);
        return;
    }
}
} // namespace Media
} // namespace OHOS
