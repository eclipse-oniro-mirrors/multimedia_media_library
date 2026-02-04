/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_data_service_processor.h"

#include "cloud_media_sync_utils.h"
#include "cloud_media_file_utils.h"
#include "photos_po.h"
#include "photos_dto.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
void CloudMediaDataServiceProcessor::GetPhotosDto(
    const std::vector<PhotosPo> &photosPos, std::vector<PhotosDto> &photosDtos)
{
    std::string path;
    std::string filePath;
    std::string fileName;
    bool ret = false;
    for (const auto &photo : photosPos) {
        PhotosDto photosDto;
        path = photo.data.value_or("");
        if (path.empty()) {
            MEDIA_ERR_LOG("photo data is empty, Photos: %{public}s", photo.ToString().c_str());
            continue;
        }
        ret = CloudMediaFileUtils::GetParentPathAndFilename(path, filePath, fileName);
        CHECK_AND_CONTINUE_ERR_LOG(ret, "GetParentPathAndFilename failed, path: %{public}s", path.c_str());
        photosDto.fileId = photo.fileId.value_or(0);
        photosDto.cloudId = photo.cloudId.value_or("");
        photosDto.data = filePath;
        photosDto.mediaType = photo.mediaType.value_or(1);
        photosDto.size = photo.size.value_or(0);
        photosDto.path = path;
        photosDto.modifiedTime = photo.editTime.value_or(0);
        photosDto.fileName = fileName;
        photosDto.originalCloudId = photo.originalAssetCloudId.value_or("");
        photosDto.storagePath = photo.storagePath.value_or("");
        photosDto.fileSourceType = photo.fileSourceType.value_or(0);
        photosDtos.push_back(photosDto);
    }
    return;
}

void CloudMediaDataServiceProcessor::GetPhotosDtoOfVideoCache(
    const std::vector<PhotosPo> &photosPos, std::vector<PhotosDto> &photosDtos)
{
    std::string filePath;
    std::string fileName;
    std::string path;
    for (const auto &photoPos : photosPos) {
        path = photoPos.data.value_or("");
        PhotosDto photosDto;
        CHECK_AND_CONTINUE_ERR_LOG(CloudMediaFileUtils::GetParentPathAndFilename(path, filePath, fileName),
            "GetParentPathAndFilename failed, path: %{public}s",
            path.c_str());
        photosDto.fileId = photoPos.fileId.value_or(0);
        photosDto.cloudId = photoPos.cloudId.value_or("");
        photosDto.data = filePath;
        photosDto.mediaType = photoPos.mediaType.value_or(0);
        photosDto.size = photoPos.size.value_or(0);
        photosDto.path = photoPos.data.value_or("");
        photosDto.modifiedTime = photoPos.editTime.value_or(0);
        photosDto.fileName = fileName;
        photosDto.originalCloudId = photoPos.originalAssetCloudId.value_or("");
        photosDtos.push_back(photosDto);
    }
    return;
}
}  // namespace OHOS::Media::CloudSync