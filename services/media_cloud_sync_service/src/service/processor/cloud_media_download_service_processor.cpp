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

#include "cloud_media_download_service_processor.h"

#include "media_log.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_sync_utils.h"

namespace OHOS::Media::CloudSync {
bool CloudMediaDownloadServiceProcessor::CheckPhotosPo(const PhotosPo &photosPo)
{
    bool isValid = true;
    isValid &= photosPo.fileId.has_value() && photosPo.fileId.value() > 0;
    isValid &= photosPo.data.has_value() && photosPo.data.value().size() > 0;
    isValid &= photosPo.size.has_value() && photosPo.size.value() > 0;
    isValid &= photosPo.mediaType.has_value() && photosPo.mediaType.value() > 0;
    isValid &= photosPo.cloudId.has_value() && photosPo.cloudId.value().size() > 0;
    isValid &= photosPo.thumbStatus.has_value() && photosPo.thumbStatus.value() >= 0;
    isValid &= photosPo.orientation.has_value() && photosPo.orientation.value() >= 0;
    return isValid;
}

std::vector<PhotosDto> CloudMediaDownloadServiceProcessor::GetPhotosDto(std::vector<PhotosPo> photosPos)
{
    std::vector<PhotosDto> photosDtos;
    bool isValid = true;
    for (auto photosPo : photosPos) {
        isValid = this->CheckPhotosPo(photosPo);
        CHECK_AND_CONTINUE_ERR_LOG(isValid, "Invalid Data, PhotosPo: %{public}s", photosPo.ToString().c_str());
        PhotosDto photosDto;
        std::string path = photosPo.data.value_or("");
        isValid = CloudMediaFileUtils::GetParentPathAndFilename(path, photosDto.data, photosDto.displayName);
        CHECK_AND_CONTINUE_ERR_LOG(
            isValid, "GetParentPathAndFilename failed, PhotosPo: %{public}s", photosPo.ToString().c_str());
        photosDto.fileId = photosPo.fileId.value_or(0);
        photosDto.cloudId = photosPo.cloudId.value_or("");
        photosDto.size = photosPo.size.value_or(0);
        photosDto.mediaType = photosPo.mediaType.value_or(0);
        int32_t orientation = photosPo.orientation.value_or(0);
        int32_t thumbState = photosPo.thumbStatus.value_or(0);
        int32_t exifRotate = photosPo.exifRotate.value_or(0);
        CloudMediaSyncUtils::FillPhotosDto(photosDto, path, orientation, exifRotate, thumbState);
        MEDIA_DEBUG_LOG("GetDownloadThms: %{public}s", photosDto.ToString().c_str());
        photosDtos.push_back(photosDto);
    }
    return photosDtos;
}

void CloudMediaDownloadServiceProcessor::GetDownloadAssetData(
    const PhotosPo &photosPo, DownloadAssetData &downloadAssetData)
{
    downloadAssetData.fileId = photosPo.fileId.value_or(0);
    downloadAssetData.cloudId = photosPo.cloudId.value_or("");
    downloadAssetData.fileSize = photosPo.size.value_or(0);
    downloadAssetData.mediaType = photosPo.mediaType.value_or(0);
    downloadAssetData.originalCloudId = photosPo.originalAssetCloudId.value_or("");
    downloadAssetData.path = photosPo.data.value_or("");
    downloadAssetData.editTime = photosPo.editTime.value_or(0);
    downloadAssetData.effectMode = photosPo.movingPhotoEffectMode.value_or(0);
    downloadAssetData.orientation = photosPo.orientation.value_or(0);
    downloadAssetData.fileSourceType = photosPo.fileSourceType.value_or(0);
    downloadAssetData.storagePath = photosPo.storagePath.value_or("");
    downloadAssetData.hidden = photosPo.hidden.value_or(0);
    downloadAssetData.dateTrashed = photosPo.dateTrashed.value_or(0);
    downloadAssetData.subtype = photosPo.subtype.value_or(0);
    return;
}

void CloudMediaDownloadServiceProcessor::GetDownloadAssetData(
    const std::vector<PhotosPo> &photosPos, std::vector<DownloadAssetData> &downloadAssetDatas)
{
    for (auto photosPo : photosPos) {
        DownloadAssetData assetData;
        this->GetDownloadAssetData(photosPo, assetData);
        downloadAssetDatas.emplace_back(assetData);
    }
    return;
}
}  // namespace OHOS::Media::CloudSync