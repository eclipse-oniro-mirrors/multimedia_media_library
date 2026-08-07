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

#include "cloud_dentry_helper.h"

#include "userfile_manager_types.h"
#include "cloud_sync_manager.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "thumbnail_const.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_utils.h"
#include "result_set_utils.h"

const std::string DENTRY_INFO_ORIGIN = "CONTENT";
const std::string DENTRY_INFO_LCD = "LCD";
const std::string DENTRY_INFO_THM = "THM";
const std::string DENTRY_INFO_LCD_EX = "THM_EX/LCD";
const std::string DENTRY_INFO_THM_EX = "THM_EX/THM";

namespace OHOS::Media {

using namespace OHOS::FileManagement::CloudSync;

const int64_t THUMB_DENTRY_SIZE = 2 * 1024 * 1024;
const std::string CLOUD_FILES_PREFIX = "/storage/cloud/files/";
const std::string CLOUD_MEDIA_PREFIX = "/storage/media/cloud/files/";
bool CloudDentryHelper::NeedCreateDentry(const std::string &filePath)
{
    if (filePath.empty()) {
        MEDIA_ERR_LOG("File path is empty");
        return false;
    }
    return access(filePath.c_str(), E_OK) != 0;
}

static void GetThumbStatusByFileId(const std::string &fileId, int32_t &hasExThumbnail)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null");

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(
        predicates, {PhotoColumn::PHOTO_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE, MediaColumn::MEDIA_TYPE});
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query database for fileId=%{public}s", fileId.c_str());
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_DEBUG_LOG("No record found for fileId=%{public}s", fileId.c_str());
        return;
    }
    int32_t exifRotate = GetInt32Val(PhotoColumn::PHOTO_EXIF_ROTATE, resultSet);
    int32_t orientation = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
    int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    hasExThumbnail = (mediaType == MediaType::MEDIA_TYPE_IMAGE) && (orientation != 0 || exifRotate > 1);
}

int32_t CloudDentryHelper::CreateDentryForThumbnail(const std::string &fileId, const std::string &originalPath,
                                                    ThumbnailType thumbType)
{
    if (thumbType == ThumbnailType::THUMB_ASTC) {
        return E_OK;
    }
    std::string thumbSuffix = GetThumbSuffix(thumbType);
    std::string thumbDentryPath;
    if (originalPath.find(CLOUD_FILES_PREFIX) == 0) {
        thumbDentryPath = CLOUD_MEDIA_PREFIX + ".thumbs/" + originalPath.substr(CLOUD_FILES_PREFIX.length()) + "/" +
                          thumbSuffix + ".jpg";
    } else {
        MEDIA_ERR_LOG("Invalid original path format: %{public}s", originalPath.c_str());
        return E_ERR;
    }
    if (!NeedCreateDentry(thumbDentryPath)) {
        MEDIA_DEBUG_LOG("Dentry already exists: path=%{public}s", thumbDentryPath.c_str());
        return E_OK;
    }

    std::vector<ThumbnailType> typesToCheck;
    if (thumbType == ThumbnailType::LCD || thumbType == ThumbnailType::LCD_EX) {
        typesToCheck = {ThumbnailType::LCD_EX, ThumbnailType::LCD};
    } else if (thumbType == ThumbnailType::THUMB || thumbType == ThumbnailType::THUMB_EX) {
        typesToCheck = {ThumbnailType::THUMB_EX, ThumbnailType::THUMB};
    }
    for (const auto &type : typesToCheck) {
        std::string localThumbPath = ThumbnailFileUtils::GetLocalThumbnailFilePath(originalPath, type);
        if (!localThumbPath.empty() && access(localThumbPath.c_str(), F_OK) == 0) {
            MEDIA_DEBUG_LOG("Local thumbnail file already exists: fileId=%{public}s", fileId.c_str());
            return E_OK;
        }
    }

    MEDIA_INFO_LOG("Need create Dentry for %{public}s", thumbSuffix.c_str());
    int32_t hasExThumbnail = 0;
    GetThumbStatusByFileId(fileId, hasExThumbnail);
    std::string fileType = DENTRY_INFO_THM;
    std::string thumbFileName = "THM.jpg";
    if ((thumbType == ThumbnailType::LCD || thumbType == ThumbnailType::LCD_EX)) {
        fileType = (hasExThumbnail == 0) ? DENTRY_INFO_LCD : DENTRY_INFO_LCD_EX;
        thumbFileName = "LCD.jpg";
    } else if ((thumbType == ThumbnailType::THUMB || thumbType == ThumbnailType::THUMB_EX)) {
        fileType = (hasExThumbnail == 0) ? DENTRY_INFO_THM : DENTRY_INFO_THM_EX;
    }

    return CreateDentryInternal(fileId, thumbFileName, THUMB_DENTRY_SIZE, fileType);
}

int32_t CloudDentryHelper::CreateDentryForOrigin(const std::string &fileId,
    const std::string &filePath)
{
    std::string dentryPath;
    if (filePath.find(CLOUD_FILES_PREFIX) == 0) {
        dentryPath = CLOUD_MEDIA_PREFIX + filePath.substr(CLOUD_FILES_PREFIX.length());
    } else {
        MEDIA_ERR_LOG("Invalid filePath format: %{public}s", filePath.c_str());
        return E_ERR;
    }
    if (!NeedCreateDentry(dentryPath)) {
        MEDIA_DEBUG_LOG("Dentry already exists: path=%{public}s", filePath.c_str());
        return E_OK;
    }
    std::string fileName = "";

    return CreateDentryInternal(fileId, fileName, 0, DENTRY_INFO_ORIGIN);
}

int32_t CloudDentryHelper::CreateDentryInternal(const std::string &fileId,
    const std::string &fileName, int64_t size, const std::string &fileType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = rdbStore->Query(predicates, {PhotoColumn::PHOTO_CLOUD_ID, MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_DATE_MODIFIED,
        PhotoColumn::PHOTO_POSITION});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Failed to query database for fileId=%{public}s",
                             fileId.c_str());
    if (resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_DEBUG_LOG("No record found for fileId=%{public}s", fileId.c_str());
        return E_ERR;
    }
    std::string cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
    std::string cloudPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    std::string mediaName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    int64_t fileSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    int64_t dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    
    CHECK_AND_RETURN_RET_LOG(position == static_cast<int32_t>(PhotoPositionType::CLOUD), E_ERR,
        "position is not CLOUD, fileId=%{public}s, position=%{public}d", fileId.c_str(), position);

    CHECK_AND_RETURN_RET_LOG(!cloudId.empty(), E_ERR, "cloudId is empty");
    CHECK_AND_RETURN_RET_LOG(!cloudPath.empty(), E_ERR, "cloudPath is empty");

    DentryFileInfo dentryInfo;
    dentryInfo.cloudId = cloudId;
    dentryInfo.path = cloudPath;
    dentryInfo.fileName = fileName;
    if (dentryInfo.fileName.empty()) {
        dentryInfo.fileName = mediaName;
    }
    dentryInfo.size = size;
    if (size == 0) {
        dentryInfo.size = fileSize;
    }
    dentryInfo.modifiedTime = dateModified;
    dentryInfo.fileType = fileType;

    std::vector<DentryFileInfo> dentryFileInfos;
    dentryFileInfos.push_back(dentryInfo);

    std::vector<std::string> failCloudIds;
    int32_t ret = CloudSyncManager::GetInstance().BatchDentryFileInsert(dentryFileInfos, failCloudIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "BatchDentryFileInsert failed, ret: %{public}d.", ret);
    MEDIA_INFO_LOG("Create dentry success: cloudid=%{public}s", cloudId.c_str());
    return ret;
}
}  // namespace OHOS::Media::Thumbnail