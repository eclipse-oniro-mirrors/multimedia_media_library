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

#define MLOG_TAG "Media_Cloud_Controller"

#include "cloud_media_photo_controller_processor.h"

#include "media_log.h"
#include "cloud_file_data_vo.h"
#include "cloud_file_data_dto.h"
#include "photos_vo.h"
#include "photos_dto.h"
#include "photos_po_writer.h"
#include "cloud_media_sync_const.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
std::vector<PhotosVo> CloudMediaPhotoControllerProcessor::SetFdirtyDataVoFromDto(std::vector<PhotosDto> &fdirtyDataDtos)
{
    std::vector<PhotosVo> fdirtyDatas;
    if (fdirtyDataDtos.size() <= 0) {
        return fdirtyDatas;
    }
    for (auto fdirtyDataDto : fdirtyDataDtos) {
        PhotosVo fdirtyDataVo;
        fdirtyDataVo.cloudId = fdirtyDataDto.cloudId;
        fdirtyDataVo.fileName = fdirtyDataDto.displayName;
        fdirtyDataVo.path = fdirtyDataDto.path;
        fdirtyDataVo.type = fdirtyDataDto.mediaType;
        fdirtyDataVo.size = fdirtyDataDto.size;
        fdirtyDataVo.modifiedTime = fdirtyDataDto.modifiedTime;
        fdirtyDataVo.originalCloudId = fdirtyDataDto.originalCloudId;
        fdirtyDataVo.fileSourceType = fdirtyDataDto.fileSourceType;
        fdirtyDataVo.storagePath = fdirtyDataDto.storagePath;
        fdirtyDataVo.localPath = fdirtyDataDto.localPath;
        fdirtyDataVo.attributesMediaType = fdirtyDataDto.attributesMediaType;
        for (auto &nodePair : fdirtyDataDto.attachment) {
            CloudFileDataVo fileData;
            fileData.fileName = nodePair.second.fileName;
            fileData.filePath = nodePair.second.path;
            fileData.size = nodePair.second.size;
            fdirtyDataVo.attachment[nodePair.first] = fileData;
        }
        fdirtyDatas.emplace_back(fdirtyDataVo);
    }
    return fdirtyDatas;
}
std::vector<PhotosVo> CloudMediaPhotoControllerProcessor::SetNewDataVoFromDto(std::vector<PhotosDto> &newDataDtos)
{
    std::vector<PhotosVo> newDatas;
    if (newDataDtos.size() <= 0) {
        return newDatas;
    }
    for (auto newDataDto : newDataDtos) {
        PhotosVo newDataVo;
        newDataVo.cloudId = newDataDto.cloudId;
        newDataVo.fileName = newDataDto.fileName;
        newDataVo.path = newDataDto.path;
        newDataVo.type = newDataDto.mediaType;
        newDataVo.size = newDataDto.size;
        newDataVo.modifiedTime = newDataDto.modifiedTime;
        newDataVo.originalCloudId = newDataDto.originalCloudId;
        newDataVo.fileSourceType = newDataDto.fileSourceType;
        newDataDto.storagePath = newDataDto.storagePath;
        for (auto &nodePair : newDataDto.attachment) {
            CloudFileDataVo fileData;
            fileData.fileName = nodePair.second.fileName;
            fileData.filePath = nodePair.second.path;
            fileData.size = nodePair.second.size;
            newDataVo.attachment[nodePair.first] = fileData;
        }
        newDatas.emplace_back(newDataVo);
    }
    return newDatas;
}
using CheckData = GetCheckRecordsRespBodyCheckData;
std::unordered_map<std::string, CheckData> CloudMediaPhotoControllerProcessor::GetCheckRecordsRespBody(
    std::vector<PhotosDto> photosDtoVec)
{
    std::unordered_map<std::string, GetCheckRecordsRespBodyCheckData> checkDataList;
    for (auto &photosDto : photosDtoVec) {
        GetCheckRecordsRespBodyCheckData checkData;
        checkData.cloudId = photosDto.cloudId;
        checkData.size = photosDto.size;
        checkData.data = photosDto.path;
        checkData.displayName = photosDto.displayName;
        checkData.fileName = photosDto.fileName;
        checkData.mediaType = photosDto.mediaType;
        checkData.cloudVersion = photosDto.cloudVersion;
        checkData.position = photosDto.position;
        checkData.dateModified = photosDto.dateModified;
        checkData.dirty = photosDto.dirty;
        checkData.syncStatus = photosDto.syncStatus;
        checkData.thmStatus = photosDto.thumbStatus;
        checkData.fileSourceType = photosDto.fileSourceType;
        checkData.storagePath = photosDto.storagePath;
        for (auto &[key, value] : photosDto.attachment) {
            CloudFileDataVo vo;
            vo.fileName = value.fileName;
            vo.filePath = value.path;
            vo.size = value.size;
            checkData.attachment[key] = vo;
        }
        checkDataList[checkData.cloudId] = checkData;
        MEDIA_DEBUG_LOG("CloudMediaDataControllerProcessor CheckData: %{public}s", checkData.ToString().c_str());
    }
    return checkDataList;
}

bool CloudMediaPhotoControllerProcessor::GetBasicInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    photosVo.title = record.title.value_or("");
    photosVo.mediaType = record.mediaType.value_or(0);
    photosVo.hidden = record.hidden.value_or(0);
    photosVo.sourcePath = record.sourcePath.value_or("");
    photosVo.displayName = record.displayName.value_or("");
    photosVo.dateTrashed = record.dateTrashed.value_or(0);
    photosVo.isFavorite = record.isFavorite.value_or(0);
    photosVo.dirty = record.dirty.value_or(0);
    photosVo.orientation = record.orientation.value_or(0);
    photosVo.size = record.size.value_or(0);
    photosVo.mimeType = record.mimeType.value_or("");
    photosVo.albumCloudId = record.albumCloudId.value_or("");
    photosVo.albumLPath = record.albumLPath.value_or("");
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetAttributesInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    photosVo.subtype = record.subtype.value_or(0);
    photosVo.burstCoverLevel = record.burstCoverLevel.value_or(1);
    photosVo.burstKey = record.burstKey.value_or("");
    photosVo.dateYear = record.dateYear.value_or("");
    photosVo.dateMonth = record.dateMonth.value_or("");
    photosVo.dateDay = record.dateDay.value_or("");
    photosVo.duration = record.duration.value_or(0);
    photosVo.relativePath = record.relativePath.value_or("");
    photosVo.virtualPath = record.virtualPath.value_or("");
    photosVo.shootingMode = record.shootingMode.value_or("");
    photosVo.shootingModeTag = record.shootingModeTag.value_or("");
    photosVo.dynamicRangeType = record.dynamicRangeType.value_or(0);
    photosVo.hdrMode = record.hdrMode.value_or(0);
    photosVo.videoMode = record.videoMode.value_or(0);
    photosVo.frontCamera = record.frontCamera.value_or("");
    photosVo.originalSubtype = record.originalSubtype.value_or(0);
    photosVo.coverPosition = record.coverPosition.value_or(0);
    photosVo.isRectificationCover = record.isRectificationCover.value_or(0);
    photosVo.exifRotate = record.exifRotate.value_or(0);
    photosVo.movingPhotoEffectMode = record.movingPhotoEffectMode.value_or(0);
    photosVo.supportedWatermarkType = record.supportedWatermarkType.value_or(0);
    photosVo.strongAssociation = record.strongAssociation.value_or(0);
    photosVo.fileId = record.fileId.value_or(0);
    photosVo.data = record.data.value_or("");
    photosVo.ownerAlbumId = record.ownerAlbumId.value_or(0);
    photosVo.fileSourceType = record.fileSourceType.value_or(0);
    photosVo.storagePath = record.storagePath.value_or("");
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetPropertiesInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    photosVo.metaDateModified = record.metaDateModified.value_or(0);
    photosVo.hiddenTime = record.hiddenTime.value_or(0);
    photosVo.editTime = record.editTime.value_or(0);
    photosVo.dateAdded = record.dateAdded.value_or(0);
    photosVo.dateModified = record.dateModified.value_or(0);
    photosVo.dateTaken = record.dateTaken.value_or(0);
    photosVo.detailTime = record.detailTime.value_or("");
    photosVo.height = record.height.value_or(0);
    photosVo.width = record.width.value_or(0);
    photosVo.deviceName = record.deviceName.value_or("");
    photosVo.latitude = record.latitude.value_or(0);
    photosVo.longitude = record.longitude.value_or(0);
    this->HandleUserComment(record, photosVo);
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetCloudInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    photosVo.cloudId = record.cloudId.value_or("");
    photosVo.originalAssetCloudId = record.originalAssetCloudId.value_or("");
    photosVo.recordType = record.recordType.value_or("");
    photosVo.recordId = record.cloudId.value_or("");
    photosVo.baseVersion = record.baseVersion.value_or(0);
    photosVo.isNew = record.isNew.value_or(false);
    return true;
}

CloudMdkRecordPhotosVo CloudMediaPhotoControllerProcessor::ConvertRecordPoToVo(const PhotosPo &record)
{
    CloudMdkRecordPhotosVo photosVo;
    this->GetBasicInfo(record, photosVo);
    this->GetAttributesInfo(record, photosVo);
    this->GetPropertiesInfo(record, photosVo);
    this->GetCloudInfo(record, photosVo);
    this->GetAttributesHashMap(record, photosVo);
    return photosVo;
}

bool CloudMediaPhotoControllerProcessor::GetBasicInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.basicFileName = photosVo.fileName;
    data.basicDisplayName = photosVo.fileName;
    data.propertiesSourceFileName = photosVo.fileName;
    data.propertiesSourcePath = photosVo.fileSourcePath;
    data.basicFileType = photosVo.fileType;
    data.basicSize = photosVo.size;
    data.localSize = photosVo.size;
    data.lcdSize = photosVo.lcdSize;
    data.thmSize = photosVo.thmSize;
    data.propertiesRotate = photosVo.rotation;
    data.basicIsFavorite = photosVo.isFavorite;
    data.attributesHidden = photosVo.hidden;
    data.basicRecycledTime = photosVo.recycledTime;
    data.latitude = photosVo.latitude;
    data.longitude = photosVo.longitude;
    data.localPath = photosVo.localPath;
    data.basicDeviceName = photosVo.source;
    data.basicDescription = photosVo.description;
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetAttributesInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.attributesFileId = photosVo.fileId;
    data.basicMimeType = photosVo.mimeType;
    data.basicIsRecycle = photosVo.isRecycle;
    data.propertiesHeight = photosVo.photoHeight;
    data.propertiesWidth = photosVo.photoWidth;
    data.propertiesDetailTime = photosVo.detailTime;
    data.attributesTitle = photosVo.title;
    data.attributesMediaType = photosVo.mediaType;
    data.duration = photosVo.duration;
    data.attributesHiddenTime = photosVo.hiddenTime;
    data.attributesRelativePath = photosVo.relativePath;
    data.attributesVirtualPath = photosVo.virtualPath;
    data.attributesShootingMode = photosVo.shootingMode;
    data.attributesShootingModeTag = photosVo.shootingModeTag;
    data.attributesBurstKey = photosVo.burstKey;
    data.attributesBurstCoverLevel = photosVo.burstCoverLevel;
    data.attributesSubtype = photosVo.subtype;
    data.attributesOriginalSubtype = photosVo.originalSubtype;
    data.attributesDynamicRangeType = photosVo.dynamicRangeType;
    data.attributesHdrMode = photosVo.hdrMode;
    data.attributesVideoMode = photosVo.videoMode;
    data.attributesFrontCamera = photosVo.frontCamera;
    data.attributesMovingPhotoEffectMode = photosVo.movingPhotoEffectMode;
    data.attributesCoverPosition = photosVo.coverPosition;
    data.attributesIsRectificationCover = photosVo.isRectificationCover;
    data.exifRotate = photosVo.exifRotate;
    data.attributesEditDataCamera = photosVo.editDataCamera;
    data.attributesSupportedWatermarkType = photosVo.supportedWatermarkType;
    data.attributesStrongAssociation = photosVo.strongAssociation;
    data.attributesFileSourceType = photosVo.fileSourceType;
    data.attributesStoragePath = photosVo.storagePath;
    // Safe Album: risk status for children's watch
    data.attributesRiskStatus = photosVo.photoRiskStatus;
    data.attributesIsCritical = photosVo.isCritical;
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetPropertiesInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.basicCreatedTime = photosVo.createTime;
    data.basicEditedTime = photosVo.dualEditTime;
    data.attributesEditedTimeMs = photosVo.editedTimeMs;
    data.modifiedTime = photosVo.editedTimeMs;
    data.attributesDateYear = photosVo.dateYear;
    data.attributesDateMonth = photosVo.dateMonth;
    data.attributesDateDay = photosVo.dateDay;
    data.attributesEditTime = photosVo.editTime;
    data.propertiesFirstUpdateTime = photosVo.firstVisitTime;
    data.attributesMetaDateModified = photosVo.metaDateModified;
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetCloudInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.cloudId = photosVo.cloudId;
    data.attributesFixVersion = photosVo.fixVersion;
    data.basicCloudVersion = photosVo.version;
    data.basicIsDelete = photosVo.isDelete;
    data.hasAttributes = photosVo.hasAttributes;
    data.hasProperties = photosVo.hasproperties;
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetAlbumInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.attributesSrcAlbumIds = photosVo.sourceAlbumIds;
    return true;
}

CloudMediaPullDataDto CloudMediaPhotoControllerProcessor::ConvertToCloudMediaPullData(const OnFetchPhotosVo &photosVo)
{
    CloudMediaPullDataDto data;
    this->GetBasicInfo(photosVo, data);
    this->GetAttributesInfo(photosVo, data);
    this->GetPropertiesInfo(photosVo, data);
    this->GetCloudInfo(photosVo, data);
    this->GetAlbumInfo(photosVo, data);
    this->GetAttributesHashMap(photosVo, data);
    return data;
}

PhotosDto CloudMediaPhotoControllerProcessor::ConvertToPhotoDto(const OnCreateRecord &recordVo)
{
    PhotosDto record;
    record.fileId = recordVo.fileId;
    record.localId = recordVo.localId;
    record.rotation = recordVo.rotation;
    record.fileType = recordVo.fileType;
    record.size = recordVo.size;
    record.createTime = recordVo.createTime;
    record.modifiedTime = recordVo.modifiedTime;
    record.editedTimeMs = recordVo.editedTimeMs;
    record.metaDateModified = recordVo.metaDateModified;
    record.version = recordVo.version;
    record.cloudId = recordVo.cloudId;
    record.path = recordVo.path;
    record.fileName = recordVo.fileName;
    record.sourcePath = recordVo.sourcePath;
    record.isSuccess = recordVo.isSuccess;
    record.errorType = recordVo.errorType;
    record.serverErrorCode = recordVo.serverErrorCode;
    record.livePhotoCachePath = recordVo.livePhotoCachePath;
    record.errorDetails = recordVo.errorDetails;
    record.fileSourceType = recordVo.fileSourceType;
    record.storagePath = recordVo.storagePath;
    return record;
}

void CloudMediaPhotoControllerProcessor::ConvertToPhotosDto(const OnFileDirtyRecord &recordVo, PhotosDto &dto)
{
    dto.fileId = recordVo.fileId;
    dto.rotation = recordVo.rotation;
    dto.fileType = recordVo.fileType;
    dto.size = recordVo.size;
    dto.createTime = recordVo.createTime;
    dto.modifiedTime = recordVo.modifyTime;
    dto.version = recordVo.version;
    dto.cloudId = recordVo.cloudId;
    dto.path = recordVo.path;
    dto.fileName = recordVo.fileName;
    dto.sourcePath = recordVo.sourcePath;
    dto.metaDateModified = recordVo.metaDateModified;
    dto.errorDetails = recordVo.errorDetails;
    dto.serverErrorCode = recordVo.serverErrorCode;
    dto.errorType = recordVo.errorType;
    dto.isSuccess = recordVo.isSuccess;
    return;
}

void CloudMediaPhotoControllerProcessor::ConvertToPhotosDto(const OnModifyRecord &recordVo, PhotosDto &dto)
{
    dto.cloudId = recordVo.cloudId;
    dto.fileId = recordVo.fileId;
    dto.fileName = recordVo.fileName;
    dto.modifiedTime = recordVo.modifyTime;
    dto.metaDateModified = recordVo.metaDateModified;
    dto.path = recordVo.path;
    dto.version = recordVo.version;
    dto.isSuccess = recordVo.isSuccess;
    dto.errorType = recordVo.errorType;
    dto.serverErrorCode = recordVo.serverErrorCode;
    dto.errorDetails = recordVo.errorDetails;
    return;
}

ReportFailureDto CloudMediaPhotoControllerProcessor::GetReportFailureDto(const ReportFailureReqBody &reqBody)
{
    ReportFailureDto reportFailureDto;
    reportFailureDto.apiCode = reqBody.apiCode;
    reportFailureDto.errorCode = reqBody.errorCode;
    reportFailureDto.fileId = reqBody.fileId;
    reportFailureDto.cloudId = reqBody.cloudId;
    return reportFailureDto;
}

bool CloudMediaPhotoControllerProcessor::GetAttributesHashMap(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    PhotosPo photosInfo = record;
    PhotosPoWriter writer = PhotosPoWriter(photosInfo);
    std::unordered_map<std::string, std::string> stringfieldsMap = writer.ToMap(false);
    for (const auto &fieldName : PHOTOS_SYNC_COLUMN_STRING) {
        auto it = stringfieldsMap.find(fieldName);
        CHECK_AND_CONTINUE(it != stringfieldsMap.end());
        photosVo.stringfields[fieldName] = it->second;
    }
    return true;
}

bool CloudMediaPhotoControllerProcessor::GetAttributesHashMap(
    const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data)
{
    data.stringfields = photosVo.stringfields;
    return true;
}

bool CloudMediaPhotoControllerProcessor::HandleUserComment(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo)
{
    // Note. no need to transfer userComment which exceed 1024 byte, avoid IPC exceed 200KB
    CHECK_AND_RETURN_RET_WARN_LOG(record.userComment.value_or("").size() <= USER_COMMENT_LIMIT_SIZE,
        true,
        "userComment size %{public}zu exceed %{public}d, skip it",
        record.userComment.value_or("").size(),
        USER_COMMENT_LIMIT_SIZE);
    photosVo.userComment = record.userComment.value_or("");
    return true;
}

}  // namespace OHOS::Media::CloudSync