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

#define MLOG_TAG "EnhancementServiceCallback"

#include "enhancement_service_callback.h"

#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>

#include "enhancement_database_operations.h"
#include "enhancement_manager.h"
#include "enhancement_task_manager.h"
#include "enhancement_service_adapter.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "result_set_utils.h"
#include "file_utils.h"
#include "medialibrary_object_utils.h"
#include "media_file_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_notify.h"
#include "photo_file_utils.h"
#include "medialibrary_photo_operations.h"
#include "mimetype_utils.h"
#include "securec.h"
#include "moving_photo_file_utils.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"
#include "multistages_video_capture_manager.h"
#include "medialibrary_tracer.h"
// LCOV_EXCL_START
using namespace std;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif
namespace OHOS {
namespace Media {
static constexpr int32_t BOTH = 2;
static vector<string> needUpdateUris;

EnhancementServiceCallback::EnhancementServiceCallback()
{}

EnhancementServiceCallback::~EnhancementServiceCallback()
{}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
bool checkStatusCode(int32_t statusCode)
{
    return (statusCode >= static_cast<int32_t>(MediaEnhance_Status_Code::LIMIT_USAGE)
        && statusCode <= static_cast<int32_t>(MediaEnhance_Status_Code::TASK_CANNOT_EXECUTE))
        || statusCode == static_cast<int32_t>(MediaEnhance_Status_Code::NON_RECOVERABLE);
}

static int32_t CheckDisplayNameWithType(const string &displayName, int32_t mediaType)
{
    int32_t ret = MediaFileUtils::CheckDisplayName(displayName);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_INVALID_DISPLAY_NAME, "Check DisplayName failed, "
        "displayName=%{private}s", displayName.c_str());

    string ext = MediaFileUtils::GetExtensionFromPath(displayName);
    CHECK_AND_RETURN_RET_LOG(!ext.empty(), E_INVALID_DISPLAY_NAME, "invalid extension, displayName=%{private}s",
        displayName.c_str());

    auto typeFromExt = MediaFileUtils::GetMediaType(displayName);
    CHECK_AND_RETURN_RET_LOG(typeFromExt == mediaType, E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL,
        "cannot match, mediaType=%{public}d, ext=%{private}s, type from ext=%{public}d",
        mediaType, ext.c_str(), typeFromExt);
    return E_OK;
}

static int32_t SetAssetPathInCreate(FileAsset &fileAsset, std::shared_ptr<TransactionOperations> trans)
{
    if (!fileAsset.GetPath().empty()) {
        return E_OK;
    }
    string extension = MediaFileUtils::GetExtensionFromPath(fileAsset.GetDisplayName());
    string filePath;
    int32_t uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(fileAsset.GetMediaType(), trans);
    int32_t errCode = MediaLibraryAssetOperations::CreateAssetPathById(uniqueId, fileAsset.GetMediaType(),
        extension, filePath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Create Asset Path failed, errCode=%{public}d", errCode);
        return errCode;
    }

    // filePath can not be empty
    fileAsset.SetPath(filePath);
    return E_OK;
}

static int32_t CheckAddrAndBytes(CloudEnhancementThreadTask& task)
{
    if (task.addr == nullptr || task.bytes == 0) {
        MEDIA_ERR_LOG("task.addr is nullptr or task.bytes(%{public}u) is 0", task.bytes);
        delete[] task.addr;
        task.addr = nullptr;
        return E_ERR;
    }
    return E_OK;
}

static int32_t CheckVideoAddrAndBytes(CloudEnhancementThreadTask& task)
{
    if (task.videoAddr == nullptr || task.videoBytes == 0) {
        MEDIA_ERR_LOG("video buffer invalid: videoAddr is nullptr or videoBytes(%{public}u) is 0",
            task.videoBytes);
        delete[] task.videoAddr;
        task.videoAddr = nullptr;
        return E_ERR;
    }
    return E_OK;
}

static void RemoveVideo(int32_t stageVideoTaskStatus, const string &photoId)
{
    bool isNeedRemove = stageVideoTaskStatus == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED);
    CHECK_AND_RETURN_LOG(isNeedRemove, "should not remove video");
    MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(photoId, false);
}

int32_t EnhancementServiceCallback::SaveCloudEnhancementPhoto(shared_ptr<CloudEnhancementFileInfo> info,
    CloudEnhancementThreadTask& task, shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    CHECK_AND_RETURN_RET(CheckAddrAndBytes(task) == E_OK, E_ERR);
    CHECK_AND_RETURN_RET_LOG(info, E_FAIL, "cloud enhancement file info is empty");
    std::unique_ptr<uint8_t[]> buffer(task.addr);
    task.addr = nullptr;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(info->displayName) == E_OK,
        E_ERR, "display name not valid");

    string editDataDirPath = PhotoFileUtils::GetEditDataDir(info->filePath);
    string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(info->filePath);
    string editDataSourcePath = PhotoFileUtils::GetEditDataSourcePath(info->filePath);
    string editDataSourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(info->filePath);

    if (!MediaFileUtils::IsDirExists(editDataDirPath)) {
        if (!MediaFileUtils::CreateDirectory(editDataDirPath)) {
            MEDIA_ERR_LOG("Create directory %{public}s failed", editDataDirPath.c_str());
            return E_FAIL;
        }
    }

    string primarySourcePath = MediaFileUtils::IsFileExists(editDataCameraPath) ? editDataSourcePath : info->filePath;
    MEDIA_INFO_LOG("Save cloud enhancement image, path: %{public}s", primarySourcePath.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(primarySourcePath, editDataSourceBackPath), E_ERR,
        "Fail to move %{public}s to %{public}s", primarySourcePath.c_str(), editDataSourceBackPath.c_str());
    int32_t ret = FileUtils::SaveImage(primarySourcePath, (void*)(buffer.get()), static_cast<size_t>(task.bytes));
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "save cloud enhancement photo failed. ret=%{public}d, errno=%{public}d",
        ret, errno);
        
    // 为 primarySourcePath 加exif
    uint32_t errorCode = 0;
    SourceOptions opts;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(primarySourcePath, opts, errorCode);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, E_ERR, "imageSource is nullptr err: %{public}d", errorCode);

    // 修改 exif 字段
    ret = imageSource->ModifyImageProperty(0, PHOTO_DATA_CLOUD_ENHANCE_MODE, to_string(1), primarySourcePath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "modify image property longitude fail %{public}d", ret);

    if (MediaFileUtils::IsFileExists(editDataCameraPath)) {
        string extension = MediaFileUtils::GetExtensionFromPath(info->filePath);
        string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
        ret = MediaLibraryPhotoOperations::AddFiltersForCloudEnhancementPhoto(info->fileId,
            info->filePath, editDataCameraPath, mimeType);
        MEDIA_INFO_LOG("save cloud enhancement photo with editDataCamera, ret: %{public}d", ret);
        CHECK_AND_EXECUTE(ret == E_OK, CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(editDataSourcePath,
            info->filePath), E_ERR, "Fail to copy editdata_source to file_path"));
    }

    int err = UpdateCloudEnhancementPhotoInfo(info->fileId, assetRefresh);
    CHECK_AND_PRINT_LOG(err == E_OK, "fail to update composite enhancement photo info");

    MediaLibraryObjectUtils::ScanFileSyncWithoutAlbumUpdate(
        info->filePath, to_string(info->fileId), MediaLibraryApi::API_10);
    return info->fileId;
}

static int32_t SaveVideo(const string &filePath, void *output, size_t writeSize)
{
    const mode_t fileMode = 0644;
    MediaLibraryTracer tracer;
    tracer.Start("FileUtils::SaveVideo");
    string filePathTemp = filePath + ".high";
    UniqueFd fd(open(filePathTemp.c_str(), O_CREAT|O_WRONLY|O_TRUNC, fileMode));
    if (!FileUtils::IsFileExist(filePathTemp)) {
        MEDIA_ERR_LOG("file not exist: %{private}s", filePathTemp.c_str());
        return E_ERR;
    }
    if (fd.Get() < 0) {
        int err = errno;
        MEDIA_ERR_LOG("Open temp file fail, path: %{private}s, fd=%d, errno=%d",
            filePathTemp.c_str(), fd.Get(), err);
        return E_ERR;
    }
    MEDIA_DEBUG_LOG("SaveVideo temp file open success: %{private}s, fd: %{private}d", filePath.c_str(), fd.Get());

    int ret = write(fd.Get(), output, writeSize);
    if (ret < 0) {
        MEDIA_ERR_LOG("write fail, ret: %{public}d, errno: %{public}d", ret, errno);
        MediaFileUtils::DeleteFile(filePathTemp);
        return E_ERR;
    }

    if (static_cast<size_t>(ret) != writeSize) {
        MEDIA_ERR_LOG("write incomplete: wrote %{public}d bytes, expected %{public}zu bytes", ret, writeSize);
        MediaFileUtils::DeleteFile(filePathTemp);
        return E_ERR;
    }

    int32_t errCode = fsync(fd.Get());
    if (errCode < 0) {
        MEDIA_ERR_LOG("fsync failed errno %{public}d", errno);
        MediaFileUtils::DeleteFile(filePathTemp);
        return E_ERR;
    }

    ret = rename(filePathTemp.c_str(), filePath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("rename fail, ret: %{public}d, errno: %{public}d", ret, errno);
        MediaFileUtils::DeleteFile(filePathTemp);
        return E_ERR;
    }

    return E_OK;
}

int32_t EnhancementServiceCallback::SaveCloudEnhancementMovingPhotoVideo(shared_ptr<CloudEnhancementFileInfo> info,
    CloudEnhancementThreadTask& task, shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    CHECK_AND_RETURN_RET(CheckVideoAddrAndBytes(task) == E_OK, E_ERR);
    CHECK_AND_RETURN_RET_LOG(info, E_FAIL, "cloud enhancement file info is empty");
    std::unique_ptr<uint8_t[]> buffer(task.videoAddr);
    task.videoAddr = nullptr;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(info->displayName) == E_OK,
        E_ERR, "display name not valid");

    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(info->filePath);
    CHECK_AND_RETURN_RET_LOG(!videoPath.empty(), E_ERR, "Can not get video path, fileid: %{public}d", info->fileId);

    string editDataDirPath = PhotoFileUtils::GetEditDataDir(info->filePath);
    string editDataSourcePath = PhotoFileUtils::GetEditDataSourcePath(info->filePath);
    string editVideoDataSourcePath = MediaFileUtils::GetMovingPhotoVideoPath(editDataSourcePath);
    string editDataSourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(info->filePath);
    string editVideoDataSourceBackPath = MediaFileUtils::GetMovingPhotoVideoPath(editDataSourceBackPath);
    string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(info->filePath);
    if (!MediaFileUtils::IsDirExists(editDataDirPath)) {
        if (!MediaFileUtils::CreateDirectory(editDataDirPath)) {
            MEDIA_ERR_LOG("Failed to create editData directory, path: %{private}s", editDataDirPath.c_str());
            return E_ERR;
        }
    }

    string primarySourcePath = MediaFileUtils::IsFileExists(editDataCameraPath) ? editVideoDataSourcePath : videoPath;
    MEDIA_INFO_LOG("Save cloud enhancement video, path: %{private}s", primarySourcePath.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(primarySourcePath, editVideoDataSourceBackPath), E_ERR,
        "Fail to move %{private}s to %{private}s", primarySourcePath.c_str(), editVideoDataSourceBackPath.c_str());
    int32_t ret = SaveVideo(primarySourcePath, (void*)(buffer.get()), static_cast<size_t>(task.videoBytes));
    if (ret != E_OK) {
        MEDIA_INFO_LOG("save cloud enhancement video failed. ret=%{public}d, errno=%{public}d", ret, errno);
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(editVideoDataSourceBackPath, primarySourcePath), E_ERR,
            "Fail to move %{private}s to %{private}s", editVideoDataSourceBackPath.c_str(), primarySourcePath.c_str());
    }
    ret = MediaFileUtils::DeleteFile(editVideoDataSourceBackPath);

    if (MediaFileUtils::IsFileExists(editDataCameraPath)) {
        ret = MediaLibraryPhotoOperations::AddFiltersToVideoExecute(info->filePath, false, false);
        MEDIA_INFO_LOG("MediaLibraryPhotoOperations AddFiltersToVideoExecute ret: %{public}d", ret);
        CHECK_AND_EXECUTE(ret == E_OK, CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(editVideoDataSourcePath,
            info->filePath), E_ERR, "Fail to copy editdata_source to file_path"));
    }

    int err = UpdateCloudEnhancementMovingPhotoInfo(info->fileId, assetRefresh);
    CHECK_AND_PRINT_LOG(err == E_OK, "fail to update composite enhancement photo info");

    MediaLibraryObjectUtils::ScanFileSyncWithoutAlbumUpdate(
        info->filePath, to_string(info->fileId), MediaLibraryApi::API_10);

    MEDIA_INFO_LOG("Save cloud enhancement moving photo video success, file_id: %{public}d", info->fileId);
    return info->fileId;
}

int32_t EnhancementServiceCallback::UpdateCloudEnhancementMovingPhotoInfo(int32_t fileId,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    NativeRdb::ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
    rdbValues.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT));
    rdbValues.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
        static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, predicates, assetRefresh);
    CHECK_AND_PRINT_LOG(ret == E_OK, "update source photo info failed. ret: %{public}d, fileId: %{public}d",
        ret, fileId);

    return E_OK;
}

int32_t EnhancementServiceCallback::UpdateCloudEnhancementPhotoInfo(int32_t fileId,
    shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    int32_t fileType = EnhancementManager::GetInstance().QueryFileTypeByFileId(fileId);
    if (fileType == BOTH) {
        MEDIA_INFO_LOG("File is moving photo with BOTH enhancement, skip photo-only DB update, fileId: %{public}d",
            fileId);
        return E_OK;
    }

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    NativeRdb::ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
    rdbValues.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT));
    rdbValues.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
        static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, predicates, assetRefresh);
    CHECK_AND_PRINT_LOG(ret == E_OK, "update source photo info failed. ret: %{public}d, fileId: %{public}d",
        ret, fileId);

    return E_OK;
}

void EnhancementServiceCallback::OnSuccess(const char* photoId, MediaEnhanceBundleHandle* bundle)
{
    string taskId = string(photoId);
    MEDIA_INFO_LOG("callback OnSuccess start, photo_id: %{public}s", taskId.c_str());
    CHECK_AND_RETURN_LOG(!taskId.empty(), "enhancement callback error: taskId is empty");
    CHECK_AND_RETURN_LOG(bundle != nullptr, "enhancement callback error: bundle is nullptr");
    EnhancementTaskManager::SetTaskRequestCount(taskId, 1);
    CloudEnhancementThreadTask task(taskId, 0, nullptr, 0, true, nullptr, 0);
    int32_t ret = EnhancementManager::GetInstance().enhancementService_->FillTaskWithResultBuffer(bundle, task);
    CHECK_AND_RETURN_LOG(ret == E_OK, "enhancement callback error: FillTaskWithResultBuffer failed");
    EnhancementManager::GetInstance().threadManager_->OnProducerCallback(task);
    MEDIA_INFO_LOG("callback OnSuccess: add %{public}s to queue", photoId);
}

void EnhancementServiceCallback::OnFailed(const char* photoId, MediaEnhanceBundleHandle* bundle)
{
    string taskId = string(photoId);
    CHECK_AND_RETURN_LOG(!taskId.empty(), "enhancement callback error: taskId is empty");
    CHECK_AND_RETURN_LOG(bundle != nullptr, "enhancement callback error: bundle is nullptr");
    int32_t statusCode = EnhancementManager::GetInstance().enhancementService_->GetInt(bundle,
        MediaEnhance_Bundle_Key::ERROR_CODE);
    MEDIA_INFO_LOG("callback start, photo_id: %{public}s enter, status code: %{public}d", taskId.c_str(), statusCode);
    CHECK_AND_RETURN_LOG(checkStatusCode(statusCode),
        "status code is invalid, task id:%{public}s, statusCode: %{public}d", taskId.c_str(), statusCode);
    CloudEnhancementThreadTask task(taskId, statusCode, nullptr, 0, false, nullptr, 0);
    EnhancementManager::GetInstance().threadManager_->OnProducerCallback(task);
    MEDIA_INFO_LOG("callback OnFailed: add %{public}s to queue", photoId);
}

void EnhancementServiceCallback::OnServiceReconnected()
{
    MEDIA_INFO_LOG("Cloud enhancement service is reconnected, try to submit processing tasks");
    EnhancementManager::GetInstance().Init();
}

void EnhancementServiceCallback::DealWithSuccessedTask(CloudEnhancementThreadTask& task)
{
    string taskId = task.taskId;
    MEDIA_INFO_LOG("DealWithSuccessedTask start, photo_id: %{public}s", taskId.c_str());
    // query 100 per
    string where = PhotoColumn::PHOTO_ID + " = ? ";
    vector<string> whereArgs { taskId };
    NativeRdb::RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.SetWhereClause(where);
    servicePredicates.SetWhereArgs(whereArgs);
    vector<string> columns;
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == E_OK,
        "enhancement callback error: query result set is empty");
    int32_t sourceFileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string sourceFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string sourceDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    int32_t sourceSubtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t sourceCEAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t movingEnhanceType = GetInt32Val(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, resultSet);
    int32_t stageVideoTaskStatus = GetInt32Val(PhotoColumn::STAGE_VIDEO_TASK_STATUS, resultSet);
    string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
    CHECK_AND_PRINT_LOG((sourceCEAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL) ||
        sourceCEAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO)),
        "enhancement callback error: db CE_AVAILABLE status not processing, file_id: %{public}d", sourceFileId);
    // save 120 per
    shared_ptr<CloudEnhancementFileInfo> info = make_shared<CloudEnhancementFileInfo>(sourceFileId,
        sourceFilePath, sourceDisplayName, sourceSubtype, hidden);
    resultSet->Close();
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::DEAL_WITH_SUCCESSED_BUSSINESS_NAME);
    int32_t newFileId = SaveCloudEnhancementPhoto(info, task, assetRefresh);
    CHECK_AND_RETURN_LOG(newFileId > 0, "invalid file id");
    if (movingEnhanceType == BOTH) {
        int32_t successSave = SaveCloudEnhancementMovingPhotoVideo(info, task, assetRefresh);
        CHECK_AND_RETURN_LOG(successSave > 0, "invalid video id");
        RemoveVideo(stageVideoTaskStatus, photoId);
    }
    assetRefresh->RefreshAlbum(NotifyAlbumType::SYS_ALBUM);

    int32_t taskType = EnhancementTaskManager::QueryTaskTypeByPhotoId(taskId);
    EnhancementTaskManager::RemoveEnhancementTask(taskId);
    CloudEnhancementGetCount::GetInstance().Report("SuccessType", taskId, taskType, movingEnhanceType);
    string fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(sourceFileId),
        MediaFileUtils::GetExtraUri(sourceDisplayName, sourceFilePath));
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch != nullptr) {
        watch->Notify(fileUri, NotifyType::NOTIFY_UPDATE);
    }
    assetRefresh->Notify();
    MEDIA_INFO_LOG("DealWithSuccessedTask success, photo_id: %{public}s", taskId.c_str());
}

void EnhancementServiceCallback::DealWithFailedTask(CloudEnhancementThreadTask& task)
{
    string taskId = task.taskId;
    MEDIA_INFO_LOG("DealWithFailedTask start, photo_id: %{public}s", taskId.c_str());
    int32_t statusCode = task.statusCode;
    NativeRdb::RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.EqualTo(PhotoColumn::PHOTO_ID, taskId);
    vector<string> columns { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("enhancement callback error: query result set is empty");
        return;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    int32_t movingEnhanceType = GetInt32Val(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, resultSet);
    resultSet->Close();
    CHECK_AND_PRINT_LOG((ceAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL) ||
        ceAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO)),
        "enhancement callback error: db CE_AVAILABLE status not processing, file_id: %{public}d", fileId);
    NativeRdb::ValuesBucket valueBucket;
    if (statusCode == static_cast<int32_t>(CEErrorCodeType::EXECUTE_FAILED) ||
        statusCode == static_cast<int32_t>(CEErrorCodeType::NON_RECOVERABLE)) {
        valueBucket.Put(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::FAILED));
    } else {
        valueBucket.Put(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    }
    valueBucket.Put(PhotoColumn::PHOTO_CE_STATUS_CODE, statusCode);
    servicePredicates.NotEqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS));
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::DEAL_WITH_FAILED_BUSSINESS_NAME);
    int32_t ret = EnhancementDatabaseOperations::Update(valueBucket, servicePredicates, assetRefresh);
    CHECK_AND_RETURN_LOG(ret == E_OK, "enhancement callback error: db CE_AVAILABLE status update failed");
    int32_t taskType = EnhancementTaskManager::QueryTaskTypeByPhotoId(taskId);
    EnhancementTaskManager::RemoveEnhancementTask(taskId);
    CloudEnhancementGetCount::GetInstance().Report("FailedType", taskId, taskType, movingEnhanceType);
    string fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
        MediaFileUtils::GetExtraUri(displayName, filePath));
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch != nullptr) {
        watch->Notify(fileUri, NotifyType::NOTIFY_UPDATE);
    }
    assetRefresh->Notify();
    MEDIA_INFO_LOG("DealWithFailedTask success, photo_id: %{public}s", taskId.c_str());
}

void EnhancementServiceCallback::UpdateAlbumsForCloudEnhancement()
{
    MEDIA_INFO_LOG("UpdateAlbumsForCloudEnhancement start");
    bool sourceAlbumNotify = true;
    if (!needUpdateUris.empty()) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
        MediaLibraryRdbUtils::UpdateSystemAlbumsByUris(rdbStore, AlbumOperationType::DEFAULT,
            needUpdateUris, NotifyAlbumType::SYS_ALBUM);
        MediaLibraryRdbUtils::UpdateCommonAlbumByUri(rdbStore, needUpdateUris, true);
        needUpdateUris.clear();
    } else {
        MEDIA_INFO_LOG("no uris need to update albums");
    }
    MEDIA_INFO_LOG("UpdateAlbumsForCloudEnhancement end");
}
#endif
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP