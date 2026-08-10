/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudDentryBatch"

#include "media_cloud_dentry_batch_task.h"

#include "cloud_sync_manager.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_subscriber.h"
#include "photo_map_operations.h"
#include "power_efficiency_manager.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "photos_po_writer.h"
#include "photos_po.h"
#include "result_set_reader.h"
#include "thumbnail_file_utils.h"

namespace OHOS::Media::Background {

using namespace OHOS::Media::CloudSync;
using namespace OHOS::FileManagement::CloudSync;

const std::string DENTRY_INFO_ORIGIN = "CONTENT";
const std::string DENTRY_INFO_LCD = "LCD";
const std::string DENTRY_INFO_THM = "THM";
const std::string DENTRY_INFO_LCD_EX = "THM_EX/LCD";
const std::string DENTRY_INFO_THM_EX = "THM_EX/THM";

const int64_t THUMB_DENTRY_SIZE = 2 * 1024 * 1024;  // 2MB
const int32_t BATCH_SIZE = 100;  // 每批处理100个图片

const std::string CLOUD_DENTRY_PREFS = "/data/storage/el2/base/preferences/cloud_dentry_config.xml";
const std::string BACKGROUND_CLOUD_FILE_CONFIG = "/data/storage/el2/base/preferences/background_cloud_file_config.xml";
const std::string LAST_PROCESSED_FILE_ID = "last_processed_file_id";
const std::string NEED_CREATE_DENTRY_AFTER_CLONE = "need_create_dentry_after_clone";

const std::string CLOUD_FILES_PREFIX = "/storage/cloud/files/";
const std::string CLOUD_DENTRY_PREFIX = "/storage/media/cloud/files/";

std::mutex MediaCloudDentryBatchTask::batchDentryMutex_;

std::vector<FileManagement::CloudSync::DentryFileInfo> MediaCloudDentryBatchTask::dentryOrigin_;
std::vector<FileManagement::CloudSync::DentryFileInfo> MediaCloudDentryBatchTask::dentryLcd_;
std::vector<FileManagement::CloudSync::DentryFileInfo> MediaCloudDentryBatchTask::dentryThm_;


bool NeedCreateDentry(const std::string &filePath)
{
    if (filePath.empty()) {
        MEDIA_ERR_LOG("File path is empty");
        return false;
    }
    return access(filePath.c_str(), E_OK) != 0;
}

std::string GetDentryPath(const std::string &cloudPath, const std::string &suffix)
{
    if (cloudPath.find(CLOUD_FILES_PREFIX) != 0) {
        MEDIA_ERR_LOG("Invalid cloud path format: %{public}s", cloudPath.c_str());
        return "";
    }

    if (suffix.empty()) {
        return CLOUD_DENTRY_PREFIX + cloudPath.substr(CLOUD_FILES_PREFIX.length());
    }
    return CLOUD_DENTRY_PREFIX + ".thumbs/" + cloudPath.substr(CLOUD_FILES_PREFIX.length()) +
           "/" + suffix + ".jpg";
}

void CheckAndAddDentry(const PhotosPo &photosPo, const std::string &suffix, const std::string &fileType,
                       std::vector<FileManagement::CloudSync::DentryFileInfo> &dentryList)
{
    std::string filePath = photosPo.data.value_or("");
    std::string dentryPath = GetDentryPath(filePath, suffix);
    CHECK_AND_RETURN(NeedCreateDentry(dentryPath));

    DentryFileInfo dentryInfo;
    dentryInfo.cloudId = photosPo.cloudId.value_or("");
    dentryInfo.path = filePath;
    dentryInfo.fileName = suffix.empty() ? photosPo.displayName.value_or("") : suffix + ".jpg";
    dentryInfo.size = suffix.empty() ? photosPo.size.value_or(0) : THUMB_DENTRY_SIZE;
    dentryInfo.modifiedTime = photosPo.dateModified.value_or(0);
    dentryInfo.fileType = fileType;
    dentryList.push_back(dentryInfo);
}

void MediaCloudDentryBatchTask::BatchInsertDentry(
    const std::vector<FileManagement::CloudSync::DentryFileInfo> &dentryList, const std::string &type)
{
    CHECK_AND_RETURN(!dentryList.empty());
    MEDIA_INFO_LOG("BatchInsertDentry count %{public}d, filetype = %{public}s",
        static_cast<int>(dentryList.size()), type.c_str());
    std::vector<std::string> failCloudIds;
    int32_t ret = CloudSyncManager::GetInstance().BatchDentryFileInsert(dentryList, failCloudIds);
    CHECK_AND_RETURN_LOG(ret == E_OK, "BatchDentryFileInsert failed, ret: %{public}d", ret);
}

bool MediaCloudDentryBatchTask::NeedCreateDentryForPhoto(const PhotosPo &photosPo)
{
    std::string filePath = photosPo.data.value_or("");
    std::string cloudId = photosPo.cloudId.value_or("");
    int32_t mediaType = photosPo.mediaType.value_or(0);
    int32_t orientation = photosPo.orientation.value_or(0);
    int32_t exifRotate = photosPo.exifRotate.value_or(0);

    CHECK_AND_RETURN_RET_LOG(!cloudId.empty(), false, "cloudId is empty");
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), false, "filePath is empty");

    int32_t hasExThumbnail = (mediaType == MediaType::MEDIA_TYPE_IMAGE) &&
        (orientation != 0 || exifRotate > 1);

    CheckAndAddDentry(photosPo, "", DENTRY_INFO_ORIGIN, dentryOrigin_);

    ThumbnailType lcdType = hasExThumbnail ? ThumbnailType::LCD_EX : ThumbnailType::LCD;
    std::string lcdPath = ThumbnailFileUtils::GetLocalThumbnailFilePath(filePath, lcdType);
    if (lcdPath.empty() || access(lcdPath.c_str(), F_OK) != 0) {
        std::string lcdFileType = hasExThumbnail ? DENTRY_INFO_LCD_EX : DENTRY_INFO_LCD;
        CheckAndAddDentry(photosPo, "LCD", lcdFileType, dentryLcd_);
    }
    ThumbnailType thmType = hasExThumbnail ? ThumbnailType::THUMB_EX : ThumbnailType::THUMB;
    std::string thmPath = ThumbnailFileUtils::GetLocalThumbnailFilePath(filePath, thmType);
    if (thmPath.empty() || access(thmPath.c_str(), F_OK) != 0) {
        std::string thmFileType = hasExThumbnail ? DENTRY_INFO_THM_EX : DENTRY_INFO_THM;
        CheckAndAddDentry(photosPo, "THM", thmFileType, dentryThm_);
    }
    return true;
}

int32_t GetDentryCreateData(const int32_t lastRecord, std::vector<PhotosPo> &photosPoVec)
{
    const std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH,
                                              PhotoColumn::PHOTO_THUMB_STATUS, PhotoColumn::PHOTO_CLOUD_ID,
                                              MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_SIZE,
                                              MediaColumn::MEDIA_DATE_MODIFIED, PhotoColumn::PHOTO_ORIENTATION,
                                              PhotoColumn::PHOTO_EXIF_ROTATE, MediaColumn::MEDIA_TYPE};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.GreaterThan(PhotoColumn::MEDIA_ID, lastRecord);
    predicates.OrderByAsc(PhotoColumn::MEDIA_ID);
    predicates.Limit(BATCH_SIZE);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query.");

    return ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPoVec);
}

void MediaCloudDentryBatchTask::DoDentryCreate(int32_t &currentLastFileId, bool &terminate,
                                               std::vector<PhotosPo> &photosPoVec)
{
    for (PhotosPo &photosPo : photosPoVec) {
        int32_t fileId = photosPo.fileId.value_or(0);
        std::string filePath = photosPo.data.value_or("");
        if (filePath.empty()) {
            currentLastFileId = fileId;
            MEDIA_ERR_LOG("filePath is empty, skip");
            continue;
        }
        NeedCreateDentryForPhoto(photosPo);
        currentLastFileId = fileId;
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Break repair location cause invalid status");
            terminate = true;
            BatchInsertDentry(dentryOrigin_, "origin");
            BatchInsertDentry(dentryLcd_, "lcd");
            BatchInsertDentry(dentryThm_, "thm");
            return;
        }
    }
    BatchInsertDentry(dentryOrigin_, "origin");
    BatchInsertDentry(dentryLcd_, "lcd");
    BatchInsertDentry(dentryThm_, "thm");
}

void MediaCloudDentryBatchTask::HandleBatchDentryCreation(const int32_t lastFileId)
{
    std::unique_lock<std::mutex> lock(batchDentryMutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(), "Batch dentry creation has started, skipping this operation");
    MEDIA_INFO_LOG("Start batch dentry creation from %{public}d", lastFileId);

    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preference error: %{public}d", errCode);

    bool terminate = false;
    int32_t currentLastFileId = lastFileId;
    std::vector<PhotosPo> photosPoVec;
    GetDentryCreateData(currentLastFileId, photosPoVec);

    do {
        MEDIA_INFO_LOG("need create dentry count %{public}d", static_cast<int>(photosPoVec.size()));
        DoDentryCreate(currentLastFileId, terminate, photosPoVec);
        prefs->PutInt(LAST_PROCESSED_FILE_ID, currentLastFileId);
        prefs->FlushSync();
        MEDIA_INFO_LOG("repair dentry to %{public}d", currentLastFileId);
        photosPoVec.clear();
        dentryOrigin_.clear();
        dentryLcd_.clear();
        dentryThm_.clear();
        GetDentryCreateData(currentLastFileId, photosPoVec);
        if (photosPoVec.empty()) {
            MEDIA_INFO_LOG("No more data to process, reset need_insert_dentry_after_clone to 0");
            NativePreferences::PreferencesHelper::DeletePreferences(CLOUD_DENTRY_PREFS);
            break;
        }
    } while (photosPoVec.size() > 0 && !terminate && MedialibrarySubscriber::IsCurrentStatusOn());

    MEDIA_INFO_LOG("End batch dentry creation");
}


void MediaCloudDentryBatchTask::CheckDentryCreation()
{
    int32_t errCode = 0;
    int32_t defaultCnt = -1;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(CLOUD_DENTRY_PREFS, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);

    int32_t needInsertDentry = prefs->GetInt(NEED_CREATE_DENTRY_AFTER_CLONE, defaultCnt);
    if (needInsertDentry != 1) {
        MEDIA_INFO_LOG("need_insert_dentry_after_clone is %{public}d, skip dentry creation", needInsertDentry);
        return;
    }
    prefs = NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);

    int32_t lastFileId = prefs->GetInt(LAST_PROCESSED_FILE_ID, 0);

    std::thread([lastFileId]() { HandleBatchDentryCreation(lastFileId); }).detach();
}


}  // namespace OHOS::Media::Background