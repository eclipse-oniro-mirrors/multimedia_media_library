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

#include "enhancement_database_operations.h"
#include "enhancement_manager.h"
#include "enhancement_task_manager.h"
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

using namespace std;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif
namespace OHOS {
namespace Media {
EnhancementServiceCallback::EnhancementServiceCallback()
{}

EnhancementServiceCallback::~EnhancementServiceCallback()
{}

bool checkStatusCode(int32_t statusCode)
{
    return (statusCode >= static_cast<int32_t>(StatusCode::LIMIT_USAGE)
        && statusCode <= static_cast<int32_t>(StatusCode::TASK_CANNOT_EXECUTE))
        || statusCode == static_cast<int32_t>(StatusCode::NON_RECOVERABLE);
}

void EnhancementServiceCallback::OnSuccess(string taskId, MediaEnhanceBundle& bundle)
{
    CHECK_AND_RETURN_LOG(!taskId.empty(), "enhancement callback error: taskId is empty");
    vector<RawData> resultBuffers = bundle.GetResultBuffers();
    CHECK_AND_RETURN_LOG(!resultBuffers.empty(), "enhancement callback error: resultBuffers is empty");
    EnhancementTaskManager::SetTaskRequestCount(taskId, 1);
    MEDIA_INFO_LOG("callback start, photo_id: %{public}s", taskId.c_str());
    // query 100 per
    string where = PhotoColumn::PHOTO_ID + " = ? ";
    vector<string> whereArgs { taskId };
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.SetWhereClause(where);
    servicePredicates.SetWhereArgs(whereArgs);
    vector<string> columns { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_HIDDEN, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_CE_AVAILABLE};
    auto resultSet = MediaLibraryRdbStore::Query(servicePredicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == E_OK,
        "enhancement callback error: query result set is empty");
    int32_t sourceFileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string sourceFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string sourceDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    int32_t sourceSubtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t sourceCEAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    CHECK_AND_PRINT_LOG(sourceCEAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING),
        "enhancement callback error: db CE_AVAILABLE status not processing, file_id: %{public}d", sourceFileId);
    // save 120 per
    shared_ptr<CloudEnhancementFileInfo> info = make_shared<CloudEnhancementFileInfo>(sourceFileId,
        sourceFilePath, sourceDisplayName, sourceSubtype, hidden);
    int32_t newFileId = SaveCloudEnhancementPhoto(info, resultBuffers.front());
    CHECK_AND_RETURN_LOG(newFileId > 0, "invalid file id");
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS));
    rdbValues.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        static_cast<int32_t>(StrongAssociationType::NORMAL));
    rdbValues.PutInt(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, newFileId);
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("update source photo failed. ret: %{public}d, photoId: %{public}s", ret, taskId.c_str());
        ret = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
    }
    CHECK_AND_RETURN_LOG(ret == E_OK, "update source photo again failed. ret: %{public}d, photoId: %{public}s",
        ret, taskId.c_str());
    EnhancementTaskManager::RemoveEnhancementTask(taskId);
    CloudEnhancementGetCount::GetInstance().Report("SuccessType", taskId);
    string fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(sourceFileId),
        MediaFileUtils::GetExtraUri(sourceDisplayName, sourceFilePath));
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(fileUri, NotifyType::NOTIFY_ADD);
    MEDIA_INFO_LOG("callback success, photo_id: %{public}s", taskId.c_str());
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

static int32_t SetAssetPathInCreate(FileAsset &fileAsset)
{
    if (!fileAsset.GetPath().empty()) {
        return E_OK;
    }
    string extension = MediaFileUtils::GetExtensionFromPath(fileAsset.GetDisplayName());
    string filePath;
    int32_t uniqueId = MediaLibraryAssetOperations::CreateAssetUniqueId(fileAsset.GetMediaType());
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

int32_t EnhancementServiceCallback::SaveCloudEnhancementPhoto(shared_ptr<CloudEnhancementFileInfo> info,
    const RawData &rawData)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(info->displayName) == E_OK,
        E_ERR, "display name not valid");
    auto pos = info->displayName.rfind('.');
    string prefix = info->displayName.substr(0, pos);
    string suffix = info->displayName.substr(pos);
    string newDisplayName = prefix + "_enhanced" + suffix;
    string newFilePath;
    int32_t newFileId = -1;
    shared_ptr<CloudEnhancementFileInfo> newFileInfo = make_shared<CloudEnhancementFileInfo>(0, newFilePath,
        newDisplayName, info->subtype, info->hidden);
    newFileId = CreateCloudEnhancementPhoto(info->fileId, newFileInfo);
    if (newFileId <= 0) {
        MEDIA_ERR_LOG("insert file in db failed, error = %{public}d", newFileId);
        return newFileId;
    }
    const uint8_t *addr = rawData.GetBuffer();
    const uint32_t bytes = rawData.GetSize();
    if (addr == nullptr || bytes == 0) {
        MEDIA_ERR_LOG("addr is nullptr or bytes(%{public}u) is 0", bytes);
        return E_ERR;
    }
    int32_t ret = FileUtils::SaveImage(newFileInfo->filePath, (void*)addr, static_cast<size_t>(bytes));
    if (ret != E_OK) {
        MEDIA_ERR_LOG("save cloud enhancement image failed. ret=%{public}d, errno=%{public}d",
            ret, errno);
        return ret;
    }
    if (info->subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(info->filePath);
        string newVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(newFileInfo->filePath);
        bool copyResult = MediaFileUtils::CopyFileUtil(sourceVideoPath, newVideoPath);
        if (!copyResult) {
            MEDIA_ERR_LOG(
                "save moving photo video failed. file_id: %{public}d, errno=%{public}d", newFileId, errno);
        }
    }
    MediaLibraryObjectUtils::ScanFileSyncWithoutAlbumUpdate(newFileInfo->filePath,
        to_string(newFileId), MediaLibraryApi::API_10);
    string newFileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(newFileId),
        MediaFileUtils::GetExtraUri(newFileInfo->displayName, newFileInfo->filePath));
    vector<string> uris = { newFileUri };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw();
    MediaLibraryRdbUtils::UpdateAllAlbums(rdbStore, uris);
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(newFileUri, NotifyType::NOTIFY_ADD);
    return newFileId;
}

int32_t EnhancementServiceCallback::CreateCloudEnhancementPhoto(int32_t sourceFileId,
    shared_ptr<CloudEnhancementFileInfo> info)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE);
    FileAsset fileAsset;
    fileAsset.SetDisplayName(info->displayName);
    fileAsset.SetTimePending(UNCREATE_FILE_TIMEPENDING);
    fileAsset.SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    // Check rootdir
    int32_t errCode = CheckDisplayNameWithType(info->displayName, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
    errCode = transactionOprn.Start();
    CHECK_AND_RETURN_RET(errCode == E_OK, errCode);
    errCode = SetAssetPathInCreate(fileAsset);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode,
        "Failed to Solve FileAsset Path and Name, displayName=%{private}s", info->displayName.c_str());
    
    int32_t outRow = EnhancementDatabaseOperations::InsertCloudEnhancementImageInDb(cmd, fileAsset,
        sourceFileId, info);
    CHECK_AND_RETURN_RET_LOG(outRow > 0, E_HAS_DB_ERROR, "insert file in db failed, error = %{public}d", outRow);
    fileAsset.SetId(outRow);
    transactionOprn.Finish();
    info->filePath = fileAsset.GetPath();
    return outRow;
}

void EnhancementServiceCallback::OnFailed(string taskId, MediaEnhanceBundle& bundle)
{
    CHECK_AND_RETURN_LOG(!taskId.empty(), "enhancement callback error: taskId is empty");
    int32_t statusCode = bundle.GetInt(MediaEnhanceBundleKey::ERROR_CODE);
    CHECK_AND_RETURN_LOG(checkStatusCode(statusCode),
        "status code is invalid, task id:%{public}s, statusCode: %{public}d", taskId.c_str(), statusCode);

    MEDIA_INFO_LOG("callback start, photo_id: %{public}s enter, status code: %{public}d", taskId.c_str(), statusCode);
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.EqualTo(PhotoColumn::PHOTO_ID, taskId);
    vector<string> columns { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_CE_AVAILABLE};
    auto resultSet = MediaLibraryRdbStore::Query(servicePredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("enhancement callback error: query result set is empty");
        return;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    CHECK_AND_PRINT_LOG(ceAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING),
        "enhancement callback error: db CE_AVAILABLE status not processing, file_id: %{public}d", fileId);
    ValuesBucket valueBucket;
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
    int32_t changedRows = MediaLibraryRdbStore::Update(valueBucket, servicePredicates);
    CHECK_AND_RETURN_LOG(changedRows > 0, "enhancement callback error: db CE_AVAILABLE status update failed");
    EnhancementTaskManager::RemoveEnhancementTask(taskId);
    CloudEnhancementGetCount::GetInstance().Report("FailedType", taskId);
    string fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
        MediaFileUtils::GetExtraUri(displayName, filePath));
    auto watch = MediaLibraryNotify::GetInstance();
    watch->Notify(fileUri, NotifyType::NOTIFY_UPDATE);
    MEDIA_INFO_LOG("callback onfailed success, photo_id: %{public}s", taskId.c_str());
}

void EnhancementServiceCallback::OnServiceReconnected()
{
    MEDIA_INFO_LOG("Cloud enhancement service is reconnected, try to submit processing tasks");
    EnhancementManager::GetInstance().Init();
}
} // namespace Media
} // namespace OHOS