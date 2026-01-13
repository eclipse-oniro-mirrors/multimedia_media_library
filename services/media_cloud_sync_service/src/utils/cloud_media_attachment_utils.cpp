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

#define MLOG_TAG "Media_Cloud_Utils"

#include "cloud_media_attachment_utils.h"

#include "directory_ex.h"
#include "media_log.h"
#include "cloud_file_data_dto.h"
#include "media_file_utils.h"
#include "cloud_media_file_utils.h"
#include "photo_file_utils.h"
#include "thumbnail_const.h"

namespace OHOS::Media::CloudSync {
// LCOV_EXCL_START
bool CloudMediaAttachmentUtils::AddRawIntoContent(const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    bool isAdded = false;
    std::string path = downloadData.path;
    std::string rawFilePath = PhotoFileUtils::GetEditDataSourcePath(path);
    std::string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(path);
    MEDIA_INFO_LOG("download rawFilePath %{public}s", rawFilePath.c_str());
    MEDIA_INFO_LOG("download editDataCameraPath %{public}s", editDataCameraPath.c_str());
    bool hasEditDataCamera = (!editDataCameraPath.empty() && access(editDataCameraPath.c_str(), F_OK) == 0);
    bool hasSource = PhotoFileUtils::HasSource(hasEditDataCamera,
        downloadData.editTime, downloadData.effectMode, downloadData.subtype);
    bool rawFileExist = access(rawFilePath.c_str(), F_OK) == 0;
    bool isValid = hasSource && !rawFileExist;
    if (!isValid) {
        MEDIA_INFO_LOG("download not add raw %{public}d, %{public}d", hasSource, rawFileExist);
        return isAdded;
    }
    MEDIA_INFO_LOG("enter add raw");
    std::string downLoadPath = rawFilePath;
    std::string editFilePath;
    std::string editFileName;
    CHECK_AND_RETURN_RET_LOG(CloudMediaFileUtils::GetParentPathAndFilename(downLoadPath, editFilePath, editFileName),
        isAdded,
        "failed to GetParentPathAndFilename");
    std::string rawFileKey = "raw";
    CloudFileDataDto rawDto;
    rawDto.path = editFilePath;
    rawDto.fileName = editFileName;
    size_t editFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(downLoadPath, editFileSize);
    rawDto.size = static_cast<int64_t>(editFileSize);
    MEDIA_INFO_LOG("rawDto.path %{public}s", editFilePath.c_str());
    MEDIA_INFO_LOG("rawDto.fileName %{public}s", editFileName.c_str());
    photosDto.attachment.insert(std::make_pair(rawFileKey, rawDto));
    isAdded = true;
    MEDIA_INFO_LOG("success add raw");
    return isAdded;
}

bool CloudMediaAttachmentUtils::AddEditDataIntoContent(const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    bool isAdded = false;
    std::string path = downloadData.path;
    std::string rawFilePath = PhotoFileUtils::GetEditDataSourcePath(path);
    std::string editDataPath = PhotoFileUtils::GetEditDataPath(path);
    std::string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(path);
    MEDIA_INFO_LOG("download rawFilePath %{public}s", rawFilePath.c_str());
    MEDIA_INFO_LOG("download editDataPath %{public}s", editDataPath.c_str());
    MEDIA_INFO_LOG("download editDataCameraPath %{public}s", editDataCameraPath.c_str());
    bool isValid = PhotoFileUtils::HasEditData(downloadData.editTime) && access(editDataPath.c_str(), F_OK) != 0;
    if (!isValid) {
        return isAdded;
    }
    MEDIA_INFO_LOG("enter add edit");
    std::string downLoadPath = editDataPath;
    std::string editFilePath = MediaFileUtils::GetParentPath(downLoadPath);
    std::string editFileName = MediaFileUtils::GetFileName(downLoadPath);
    std::string editFileKey = "editData";
    CloudFileDataDto editDto;
    editDto.path = editFilePath;
    editDto.fileName = editFileName;
    size_t editFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(downLoadPath, editFileSize);
    editDto.size = static_cast<int64_t>(editFileSize);
    MEDIA_INFO_LOG("editDto.path %{public}s", editFilePath.c_str());
    MEDIA_INFO_LOG("editDto.fileName %{public}s", editFileName.c_str());
    photosDto.attachment.insert(std::make_pair(editFileKey, editDto));
    isAdded = true;
    MEDIA_INFO_LOG("success add edit");
    return isAdded;
}

int32_t CloudMediaAttachmentUtils::GetContent(
    const std::string &fileKey, const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    MEDIA_INFO_LOG("enter GetContent");
    std::string filePath;
    std::string fileName;
    CHECK_AND_RETURN_RET_LOG(CloudMediaFileUtils::GetParentPathAndFilename(downloadData.path, filePath, fileName),
        E_ERR,
        "failed to GetParentPathAndFilename");
    MEDIA_INFO_LOG("filePath %{public}s", filePath.c_str());
    MEDIA_INFO_LOG("fileName %{public}s", fileName.c_str());
    CloudFileDataDto contentDto;
    contentDto.path = filePath;
    contentDto.fileName = fileName;
    contentDto.size = downloadData.fileSize;
    photosDto.attachment.insert(std::make_pair(fileKey, contentDto));

    bool added = false;
    added = CloudMediaAttachmentUtils::AddRawIntoContent(downloadData, photosDto);
    if (PhotoFileUtils::HasEditData(downloadData.editTime)) {
        bool editAdded = CloudMediaAttachmentUtils::AddEditDataIntoContent(downloadData, photosDto);
        added = added || editAdded;
    }
    if (added) {
        std::string parentPath;
        std::string parentName;
        std::string rawFilePath = PhotoFileUtils::GetEditDataSourcePath(downloadData.path);
        CHECK_AND_RETURN_RET_LOG(CloudMediaFileUtils::GetParentPathAndFilename(rawFilePath, parentPath, parentName),
            E_ERR,
            "failed to GetParentPathAndFilename");
        MEDIA_INFO_LOG("get parentPath %{public}s", parentPath.c_str());
        ForceCreateDirectory(parentPath);
    }
    return E_OK;
}

int32_t CloudMediaAttachmentUtils::GetThumbnail(
    const std::string &fileKey, const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    MEDIA_INFO_LOG("enter GetThumbnail");
    bool isRotation = downloadData.orientation != ROTATE_ANGLE_0;
    std::string thumbSuffix = isRotation ? THUMBNAIL_THUMB_EX_SUFFIX : THUMBNAIL_THUMB_SUFFIX;
    std::string thumbLocalPath = GetThumbnailPath(downloadData.path, thumbSuffix);
    CloudFileDataDto thmDto;
    CHECK_AND_RETURN_RET_LOG(
        CloudMediaFileUtils::GetParentPathAndFilename(thumbLocalPath, thmDto.path, thmDto.fileName),
        E_ERR,
        "failed to GetParentPathAndFilename");
    size_t thumbFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(thumbLocalPath, thumbFileSize);
    thmDto.size = static_cast<int64_t>(thumbFileSize);
    std::string thumFileKey = "thumbnail";
    MEDIA_INFO_LOG("thmDto.path %{public}s", thmDto.path.c_str());
    MEDIA_INFO_LOG("thmDto.fileName %{public}s", thmDto.fileName.c_str());
    photosDto.attachment.insert(std::make_pair(thumFileKey, thmDto));
    return E_OK;
}

int32_t CloudMediaAttachmentUtils::GetLcdThumbnail(
    const std::string &fileKey, const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    MEDIA_INFO_LOG("enter GetLcdThumbnail");
    bool isRotation = downloadData.orientation != ROTATE_ANGLE_0;
    std::string lcdSuffix = isRotation ? THUMBNAIL_LCD_EX_SUFFIX : THUMBNAIL_LCD_SUFFIX;
    std::string lcdLocalPath = GetThumbnailPath(downloadData.path, lcdSuffix);
    CloudFileDataDto lcdDto;
    CHECK_AND_RETURN_RET_LOG(CloudMediaFileUtils::GetParentPathAndFilename(lcdLocalPath, lcdDto.path, lcdDto.fileName),
        E_ERR,
        "failed to GetParentPathAndFilename");
    size_t lcdFileSize = 0;
    CloudMediaFileUtils::GetFileSizeV2(lcdLocalPath, lcdFileSize);
    lcdDto.size = static_cast<int64_t>(lcdFileSize);
    std::string lcdFileKey = "lcd";
    MEDIA_INFO_LOG("lcdDto.path %{public}s", lcdDto.path.c_str());
    MEDIA_INFO_LOG("lcdDto.fileName %{public}s", lcdDto.fileName.c_str());
    photosDto.attachment.insert(std::make_pair(lcdFileKey, lcdDto));
    return E_OK;
}

int32_t CloudMediaAttachmentUtils::GetAttachment(
    const std::string &fileKey, const DownloadAssetData &downloadData, PhotosDto &photosDto)
{
    if (fileKey == "content") {
        return GetContent(fileKey, downloadData, photosDto);
    } else if (fileKey == "thumbnail") {
        return GetThumbnail(fileKey, downloadData, photosDto);
    } else if (fileKey == "lcd") {
        return GetLcdThumbnail(fileKey, downloadData, photosDto);
    }
    MEDIA_ERR_LOG("Failed to GetAttachment, fileKey: %{public}s", fileKey.c_str());
    return E_ERR;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync