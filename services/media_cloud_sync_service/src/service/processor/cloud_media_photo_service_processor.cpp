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

#include "cloud_media_photo_service_processor.h"

#include "cloud_media_sync_utils.h"
#include "cloud_media_file_utils.h"
#include "photos_po.h"
#include "photos_dto.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
PhotosDto CloudMediaPhotoServiceProcessor::Parse(const PhotosPo &photosPo)
{
    PhotosDto photosDto;
    photosDto.data = photosPo.data.value_or("");
    photosDto.size = photosPo.size.value_or(0);
    photosDto.dateModified = photosPo.dateModified.value_or(0);
    photosDto.dirty = photosPo.dirty.value_or(0);
    photosDto.dateTrashed = photosPo.dateTrashed.value_or(0);
    photosDto.position = photosPo.position.value_or(0);
    photosDto.cloudId = photosPo.cloudId.value_or("");
    photosDto.cloudVersion = photosPo.cloudVersion.value_or(0);
    photosDto.fileId = photosPo.fileId.value_or(0);
    photosDto.relativePath = photosPo.relativePath.value_or("");
    photosDto.dateAdded = photosPo.dateAdded.value_or(0);
    photosDto.ownerAlbumId = photosPo.ownerAlbumId.value_or(0);
    photosDto.metaDateModified = photosPo.metaDateModified.value_or(0);
    photosDto.syncStatus = photosPo.syncStatus.value_or(0);
    photosDto.thumbStatus = photosPo.thumbStatus.value_or(0);
    photosDto.displayName = photosPo.displayName.value_or("");
    photosDto.orientation = photosPo.orientation.value_or(0);
    photosDto.subtype = photosPo.subtype.value_or(0);
    photosDto.movingPhotoEffectMode = photosPo.movingPhotoEffectMode.value_or(0);
    photosDto.originalSubtype = photosPo.originalSubtype.value_or(0);
    photosDto.sourcePath = photosPo.sourcePath.value_or("");
    photosDto.mimeType = photosPo.mimeType.value_or("");
    photosDto.mediaType = photosPo.mediaType.value_or(0);
    CloudMediaSyncUtils::FillPhotosDto(photosDto, photosDto.data, photosDto.orientation,
        photosPo.exifRotate.value_or(0), photosDto.thumbStatus);
    photosDto.fileSourceType = photosPo.fileSourceType.value_or(0);
    photosDto.storagePath = photosPo.storagePath.value_or("");
    CloudMediaFileUtils::GetParentPathAndFilename(photosDto.data, photosDto.path, photosDto.fileName);
    MEDIA_DEBUG_LOG("photosDto: %{public}s", photosDto.ToString().c_str());
    return photosDto;
}

std::vector<PhotosDto> CloudMediaPhotoServiceProcessor::GetPhotosDtos(const std::vector<PhotosPo> &photosPos)
{
    std::vector<PhotosDto> photosDtoList;
    for (const auto &photosPo : photosPos) {
        photosDtoList.emplace_back(this->Parse(photosPo));
    }
    return photosDtoList;
}
}  // namespace OHOS::Media::CloudSync