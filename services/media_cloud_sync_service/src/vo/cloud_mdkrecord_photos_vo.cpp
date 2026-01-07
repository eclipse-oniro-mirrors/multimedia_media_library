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

#define MLOG_TAG "Media_Cloud_Vo"

#include "cloud_mdkrecord_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media::CloudSync {
bool CloudMdkRecordPhotosVo::MarshallingBasicInfo(Parcel &parcel) const
{
    parcel.WriteString(title);
    parcel.WriteInt32(mediaType);
    parcel.WriteInt32(duration);
    parcel.WriteInt32(hidden);
    parcel.WriteInt64(hiddenTime);
    parcel.WriteString(relativePath);
    parcel.WriteString(virtualPath);
    parcel.WriteInt64(metaDateModified);
    parcel.WriteInt32(subtype);
    parcel.WriteInt32(burstCoverLevel);
    parcel.WriteString(burstKey);
    parcel.WriteString(dateYear);
    parcel.WriteString(dateMonth);
    parcel.WriteString(dateDay);
    parcel.WriteString(shootingMode);
    parcel.WriteString(shootingModeTag);
    parcel.WriteInt32(dynamicRangeType);
    parcel.WriteInt32(hdrMode);
    parcel.WriteInt32(videoMode);
    parcel.WriteString(frontCamera);
    parcel.WriteInt64(editTime);
    parcel.WriteInt32(originalSubtype);  //
    parcel.WriteInt64(coverPosition);
    parcel.WriteInt32(isRectificationCover);
    parcel.WriteInt32(exifRotate);
    parcel.WriteInt32(movingPhotoEffectMode);
    parcel.WriteInt32(supportedWatermarkType);
    parcel.WriteInt32(strongAssociation);
    return true;
}
bool CloudMdkRecordPhotosVo::MarshallingAttributesInfo(MessageParcel &parcel) const
{
    parcel.WriteInt32(fileId);
    parcel.WriteString(cloudId);
    parcel.WriteString(originalAssetCloudId);
    parcel.WriteString(data);
    parcel.WriteInt64(dateAdded);
    parcel.WriteInt64(dateModified);
    parcel.WriteInt32(ownerAlbumId);
    parcel.WriteDouble(latitude);
    parcel.WriteDouble(longitude);
    parcel.WriteString(sourcePath);
    parcel.WriteString(displayName);
    parcel.WriteInt64(dateTaken);
    parcel.WriteString(detailTime);
    parcel.WriteInt32(height);
    parcel.WriteInt32(width);
    parcel.WriteString(deviceName);
    parcel.WriteInt64(dateTrashed);
    parcel.WriteInt32(isFavorite);
    parcel.WriteString(userComment);
    parcel.WriteInt32(dirty);
    parcel.WriteInt32(orientation);
    parcel.WriteInt64(size);
    parcel.WriteInt64(baseVersion);
    parcel.WriteString(mimeType);
    parcel.WriteString(albumCloudId);
    parcel.WriteString(albumLPath);
    parcel.WriteString(recordType);
    parcel.WriteString(recordId);
    parcel.WriteInt32(fileSourceType);
    parcel.WriteString(storagePath);
    ITypesUtil::Marshalling(stringfields, parcel);
    return true;
}
bool CloudMdkRecordPhotosVo::ReadBasicInfo(Parcel &parcel)
{
    parcel.ReadString(title);
    parcel.ReadInt32(mediaType);
    parcel.ReadInt32(duration);
    parcel.ReadInt32(hidden);
    parcel.ReadInt64(hiddenTime);
    parcel.ReadString(relativePath);
    parcel.ReadString(virtualPath);
    parcel.ReadInt64(metaDateModified);
    parcel.ReadInt32(subtype);
    parcel.ReadInt32(burstCoverLevel);
    parcel.ReadString(burstKey);
    parcel.ReadString(dateYear);
    parcel.ReadString(dateMonth);
    parcel.ReadString(dateDay);
    parcel.ReadString(shootingMode);
    parcel.ReadString(shootingModeTag);
    parcel.ReadInt32(dynamicRangeType);
    parcel.ReadInt32(hdrMode);
    parcel.ReadInt32(videoMode);
    parcel.ReadString(frontCamera);
    parcel.ReadInt64(editTime);
    parcel.ReadInt32(originalSubtype);
    parcel.ReadInt64(coverPosition);
    parcel.ReadInt32(isRectificationCover);
    parcel.ReadInt32(exifRotate);
    parcel.ReadInt32(movingPhotoEffectMode);
    parcel.ReadInt32(supportedWatermarkType);
    parcel.ReadInt32(strongAssociation);
    return true;
}
bool CloudMdkRecordPhotosVo::ReadAttributesInfo(MessageParcel &parcel)
{
    parcel.ReadInt32(fileId);
    parcel.ReadString(cloudId);
    parcel.ReadString(originalAssetCloudId);
    parcel.ReadString(data);
    parcel.ReadInt64(dateAdded);
    parcel.ReadInt64(dateModified);
    parcel.ReadInt32(ownerAlbumId);
    parcel.ReadDouble(latitude);
    parcel.ReadDouble(longitude);
    parcel.ReadString(sourcePath);
    parcel.ReadString(displayName);
    parcel.ReadInt64(dateTaken);
    parcel.ReadString(detailTime);
    parcel.ReadInt32(height);
    parcel.ReadInt32(width);
    parcel.ReadString(deviceName);
    parcel.ReadInt64(dateTrashed);
    parcel.ReadInt32(isFavorite);
    parcel.ReadString(userComment);
    parcel.ReadInt32(dirty);
    parcel.ReadInt32(orientation);
    parcel.ReadInt64(size);
    parcel.ReadInt64(baseVersion);
    parcel.ReadString(mimeType);
    parcel.ReadString(albumCloudId);
    parcel.ReadString(albumLPath);
    parcel.ReadString(recordType);
    parcel.ReadString(recordId);
    parcel.ReadInt32(fileSourceType);
    parcel.ReadString(storagePath);
    ITypesUtil::Unmarshalling(stringfields, parcel);
    return true;
}
bool CloudMdkRecordPhotosVo::Marshalling(MessageParcel &parcel) const
{
    this->MarshallingBasicInfo(parcel);
    this->MarshallingAttributesInfo(parcel);
    IPC::ITypeMediaUtil::Marshalling<std::string>(this->removeAlbumCloudId, parcel);
    return true;
}

bool CloudMdkRecordPhotosVo::Unmarshalling(MessageParcel &parcel)
{
    this->ReadBasicInfo(parcel);
    this->ReadAttributesInfo(parcel);
    IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->removeAlbumCloudId, parcel);
    return true;
}

void CloudMdkRecordPhotosVo::GetAlbumInfo(std::stringstream &ss) const
{
    ss << "\"albumCloudId\": " << albumCloudId << "\","
       << "\"albumLPath\": " << albumLPath << ",";
}

void CloudMdkRecordPhotosVo::GetBasicInfo(std::stringstream &ss) const
{
    ss << "\"orientation\": \"" << orientation << "\","
       << "\"size\": " << size << ","
       << "\"hidden\": " << hidden << ","
       << "\"dirty\": " << dirty << ","
       << "\"dateTrashed\": " << dateTrashed << ","
       << "\"isFavorite\": " << isFavorite << ","
       << "\"mediaType\": " << mediaType << ","
       << "\"subtype\": " << subtype << ","
       << "\"ownerAlbumId\": " << ownerAlbumId << ",";
}

void CloudMdkRecordPhotosVo::GetPropertiesInfo(std::stringstream &ss) const
{
    ss << "\"editTime\": " << editTime << ","
       << "\"fileId\": " << fileId << ","
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"originalAssetCloudId\": \"" << originalAssetCloudId << "\","
       << "\"dateAdded\": " << dateAdded << ","
       << "\"dateModified\": " << dateModified << ","
       << "\"dateTaken\": " << dateTaken << ","
       << "\"dateYear\": \"" << dateYear << "\","
       << "\"dateMonth\": \"" << dateMonth << "\","
       << "\"dateDay\": \"" << dateDay << "\","
       << "\"detailTime\": \"" << detailTime << "\","
       << "\"metaDateModified\": " << metaDateModified << ",";
}

void CloudMdkRecordPhotosVo::GetCloudInfo(std::stringstream &ss) const
{
    ss << "\"recordType\": \"" << recordType << "\","
       << "\"recordId\": \"" << recordId << ",\""
       << "\"isNew\": " << isNew << ","
       << "\"baseVersion\": " << baseVersion << ","
       << "\"originalSubtype\": " << originalSubtype << ","
       << "\"hiddenTime\": " << hiddenTime << ",";
}

void CloudMdkRecordPhotosVo::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"duration\": " << duration << ","
       << "\"relativePath\": \"" << relativePath << "\","
       << "\"virtualPath\": \"" << virtualPath << "\","
       << "\"burstCoverLevel\": " << burstCoverLevel << ","
       << "\"burstKey\": \"" << burstKey << "\","
       << "\"shootingMode\": \"" << shootingMode << "\","
       << "\"shootingModeTag\": \"" << shootingModeTag << "\","
       << "\"dynamicRangeType\": " << dynamicRangeType << ","
       << "\"hdrMode\": " << hdrMode << ","
       << "\"videoMode\": " << videoMode << ","
       << "\"frontCamera\": \"" << frontCamera << "\","
       << "\"coverPosition\": " << coverPosition << ","
       << "\"isRectificationCover\": " << isRectificationCover << ","
       << "\"exifRotate\": " << exifRotate << ","
       << "\"movingPhotoEffectMode\": " << movingPhotoEffectMode << ","
       << "\"supportedWatermarkType\": " << supportedWatermarkType << ","
       << "\"strongAssociation\": " << strongAssociation << ","
       << "\"data\": \"" << data << "\","
       << "\"latitude_has_value\": " << (latitude != 0) << ","
       << "\"longitude_has_value\": " << (longitude != 0) << ","
       << "\"height\": " << height << ","
       << "\"width\": " << width << ","
       << "\"deviceName\": \"" << deviceName << "\","
       << "\"userComment\": \"" << userComment << "\","
       << "\"mimeType\": " << mimeType << ",";
}

void CloudMdkRecordPhotosVo::GetRemoveAlbumInfo(std::stringstream &ss) const
{
    ss << "[";
    for (uint32_t i = 0; i < removeAlbumCloudId.size(); i++) {
        ss << removeAlbumCloudId[i];
        if (i != removeAlbumCloudId.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
}

std::string CloudMdkRecordPhotosVo::ToString()
{
    std::stringstream ss;
    ss << "{";
    this->GetAlbumInfo(ss);
    this->GetBasicInfo(ss);
    this->GetPropertiesInfo(ss);
    this->GetCloudInfo(ss);
    this->GetAttributesInfo(ss);
    this->GetRemoveAlbumInfo(ss);
    this->GetAttributesHashMap(ss);
    ss << "}";
    return ss.str();
}

bool CloudMdkRecordPhotosReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->size);
    parcel.ReadInt32(this->dirtyType);
    return true;
}

bool CloudMdkRecordPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->size);
    parcel.WriteInt32(this->dirtyType);
    return true;
}

std::string CloudMdkRecordPhotosReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{";
    ss << "\"size\":" << this->size << ",";
    ss << "\"dirtyType\":" << this->dirtyType << ",";
    ss << "}";
    return ss.str();
}

std::vector<CloudMdkRecordPhotosVo> CloudMdkRecordPhotosRespBody::GetPhotosRecords()
{
    return this->cloudPhotosUploadRecord;
}

bool CloudMdkRecordPhotosRespBody::Marshalling(MessageParcel &parcel) const
{
    if (cloudPhotosUploadRecord.size() == 0) {
        return false;
    }
    CHECK_AND_RETURN_RET(parcel.WriteInt32(static_cast<int32_t>(cloudPhotosUploadRecord.size())), false);
    for (const auto &entry : cloudPhotosUploadRecord) {
        if (!entry.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

// 服务端->客户端；在客户端反序列化
bool CloudMdkRecordPhotosRespBody::GetRecords(std::vector<CloudMdkRecordPhotosVo> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }
    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.clear();
    bool isValid;
    for (size_t i = 0; i < size; i++) {
        CloudMdkRecordPhotosVo nodeObj;
        isValid = nodeObj.Unmarshalling(parcel);
        CHECK_AND_RETURN_RET(isValid, false);
        val.emplace_back(nodeObj);
    }
    return true;
}

bool CloudMdkRecordPhotosRespBody::Unmarshalling(MessageParcel &parcel)
{
    return GetRecords(this->cloudPhotosUploadRecord, parcel);
}

std::string CloudMdkRecordPhotosRespBody::ToString() const
{
    return "";
}

void CloudMdkRecordPhotosVo::GetAttributesHashMap(std::stringstream &ss) const
{
    ss << "\"stringfields\": {";
    for (const auto &node : this->stringfields) {
        ss << "\"" << node.first << "\": ";
        ss << "\"" << node.second << "\", ";
    }
    ss << "}";
    return;
}

size_t CloudMdkRecordPhotosRespBody::GetDataSize() const
{
    return this->cloudPhotosUploadRecord.size();
}

bool CloudMdkRecordPhotosRespBody::TruncateDataBy200K()
{
    CHECK_AND_RETURN_RET(!this->cloudPhotosUploadRecord.empty(), false);
    const size_t parcelGap = 4800;
    const size_t maxCapacity = 204800;
    const size_t parcelCapacity = maxCapacity - parcelGap;
    const size_t originalSize = this->cloudPhotosUploadRecord.size();
    size_t parcelSize = 0;
    size_t elementSize = 0;
    std::vector<CloudMdkRecordPhotosVo> resultList;
    for (size_t index = 0; index < originalSize; index++) {
        MessageParcel tempParcel;
        // Try marshalling into MessageParcel.
        CHECK_AND_BREAK_ERR_LOG(this->cloudPhotosUploadRecord[index].Marshalling(tempParcel),
            "Marshalling error, truncate stop. "
            "index: %{public}zu, resultList: %{public}zu, originalSize: %{public}zu",
            index,
            resultList.size(),
            originalSize);
        // Check the dataSize not exceed capacity.
        elementSize = tempParcel.GetDataSize();
        parcelSize += elementSize;
        CHECK_AND_BREAK_ERR_LOG(parcelSize <= parcelCapacity,
            "exceed capacity, truncate it. "
            "elementSize: %{public}zu, index: %{public}zu, resultList: %{public}zu, originalSize: %{public}zu, "
            "parcelSize: %{public}zu, parcelCapacity: %{public}zu",
            elementSize,
            index,
            resultList.size(),
            originalSize,
            parcelSize,
            parcelCapacity);
        resultList.emplace_back(this->cloudPhotosUploadRecord[index]);
    }
    // No need to truncate body.
    CHECK_AND_RETURN_RET(resultList.size() != originalSize, true);
    this->cloudPhotosUploadRecord = resultList;
    MEDIA_INFO_LOG("TruncateDataBy200K completed, "
        "resultList: %{public}zu, originalSize: %{public}zu, "
        "parcelSize: %{public}zu, parcelCapacity: %{public}zu",
        resultList.size(),
        originalSize,
        parcelSize,
        parcelCapacity);
    return true;
}
}  // namespace OHOS::Media::CloudSync