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
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
bool CloudMdkRecordPhotosVo::MarshallingBasicInfo(Parcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(title), false, "title");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(duration), false, "duration");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(hiddenTime), false, "hiddenTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(relativePath), false, "relativePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(virtualPath), false, "virtualPath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(subtype), false, "subtype");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(burstCoverLevel), false, "burstCoverLevel");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(burstKey), false, "burstKey");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(dateYear), false, "dateYear");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(dateMonth), false, "dateMonth");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(dateDay), false, "dateDay");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(shootingMode), false, "shootingMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(shootingModeTag), false, "shootingModeTag");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(dynamicRangeType), false, "dynamicRangeType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(hdrMode), false, "hdrMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(videoMode), false, "videoMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(frontCamera), false, "frontCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(editTime), false, "editTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(originalSubtype), false, "originalSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(coverPosition), false, "coverPosition");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(isRectificationCover), false, "isRectificationCover");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(exifRotate), false, "exifRotate");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(movingPhotoEffectMode), false, "movingPhotoEffectMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(supportedWatermarkType), false, "supportedWatermarkType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(strongAssociation), false, "strongAssociation");
    return true;
}
bool CloudMdkRecordPhotosVo::MarshallingAttributesInfo(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(originalAssetCloudId), false, "originalAssetCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(data), false, "data");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateAdded), false, "dateAdded");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(ownerAlbumId), false, "ownerAlbumId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteDouble(latitude), false, "latitude");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteDouble(longitude), false, "longitude");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(sourcePath), false, "sourcePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(displayName), false, "displayName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateTaken), false, "dateTaken");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(detailTime), false, "detailTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(height), false, "height");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(width), false, "width");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(deviceName), false, "deviceName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateTrashed), false, "dateTrashed");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(isFavorite), false, "isFavorite");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(userComment), false, "userComment");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(dirty), false, "dirty");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(orientation), false, "orientation");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(baseVersion), false, "baseVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(mimeType), false, "mimeType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(albumCloudId), false, "albumCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(albumLPath), false, "albumLPath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(recordType), false, "recordType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(recordId), false, "recordId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(storagePath), false, "storagePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(packageName), false, "packageName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(stringfields, parcel), false, "stringfields");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(int64fields, parcel), false, "int64fields");
    return true;
}
bool CloudMdkRecordPhotosVo::ReadBasicInfo(Parcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(title), false, "title");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(duration), false, "duration");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(hiddenTime), false, "hiddenTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(relativePath), false, "relativePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(virtualPath), false, "virtualPath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(subtype), false, "subtype");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(burstCoverLevel), false, "burstCoverLevel");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(burstKey), false, "burstKey");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(dateYear), false, "dateYear");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(dateMonth), false, "dateMonth");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(dateDay), false, "dateDay");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(shootingMode), false, "shootingMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(shootingModeTag), false, "shootingModeTag");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(dynamicRangeType), false, "dynamicRangeType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(hdrMode), false, "hdrMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(videoMode), false, "videoMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(frontCamera), false, "frontCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(editTime), false, "editTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(originalSubtype), false, "originalSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(coverPosition), false, "coverPosition");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(isRectificationCover), false, "isRectificationCover");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(exifRotate), false, "exifRotate");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(movingPhotoEffectMode), false, "movingPhotoEffectMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(supportedWatermarkType), false, "supportedWatermarkType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(strongAssociation), false, "strongAssociation");
    return true;
}
bool CloudMdkRecordPhotosVo::ReadAttributesInfo(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(originalAssetCloudId), false, "originalAssetCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(data), false, "data");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateAdded), false, "dateAdded");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(ownerAlbumId), false, "ownerAlbumId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadDouble(latitude), false, "latitude");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadDouble(longitude), false, "longitude");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(sourcePath), false, "sourcePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(displayName), false, "displayName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateTaken), false, "dateTaken");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(detailTime), false, "detailTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(height), false, "height");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(width), false, "width");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(deviceName), false, "deviceName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateTrashed), false, "dateTrashed");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(isFavorite), false, "isFavorite");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(userComment), false, "userComment");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(dirty), false, "dirty");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(orientation), false, "orientation");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(baseVersion), false, "baseVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(mimeType), false, "mimeType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(albumCloudId), false, "albumCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(albumLPath), false, "albumLPath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(recordType), false, "recordType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(recordId), false, "recordId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(fileSourceType), false, "fileSourceType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(storagePath), false, "storagePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(packageName), false, "packageName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Unmarshalling(stringfields, parcel), false, "stringfields");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Unmarshalling(int64fields, parcel), false, "int64fields");
    return true;
}
bool CloudMdkRecordPhotosVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(this->MarshallingBasicInfo(parcel), false, "MarshallingBasicInfo");
    CHECK_AND_RETURN_RET_LOG(this->MarshallingAttributesInfo(parcel), false, "MarshallingAttributesInfo");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::Marshalling<std::string>(this->removeAlbumCloudId, parcel), false, "removeAlbumCloudId");
    return true;
}

bool CloudMdkRecordPhotosVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(this->ReadBasicInfo(parcel), false, "ReadBasicInfo");
    CHECK_AND_RETURN_RET_LOG(this->ReadAttributesInfo(parcel), false, "ReadAttributesInfo");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->removeAlbumCloudId, parcel), false, "removeAlbumCloudId");
    return true;
}

void CloudMdkRecordPhotosVo::GetAlbumInfo(std::stringstream &ss) const
{
    ss << "\"albumCloudId\": \"" << albumCloudId << "\","
       << "\"albumLPath\": \"" << MediaFileUtils::DesensitizePath(albumLPath) << "\"";
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
       << "\"recordId\": \"" << recordId << "\","
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
       << "\"data\": \"" << MediaFileUtils::DesensitizePath(data) << "\","
       << "\"latitude_has_value\": " << (latitude != 0) << ","
       << "\"longitude_has_value\": " << (longitude != 0) << ","
       << "\"height\": " << height << ","
       << "\"width\": " << width << ","
       << "\"deviceName\": \"" << deviceName << "\","
       << "\"userComment\": \"" << userComment << "\","
       << "\"mimeType\": " << mimeType << ","
       << "\"uniqueId\": " << uniqueId << ","
       << "\"packageName\": " << packageName << ","
       << "\"photoRiskStatus\": " << photoRiskStatus << ",";
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

std::string CloudMdkRecordPhotosVo::ToString() const
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
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->size), false, "size");
    return true;
}

bool CloudMdkRecordPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->size), false, "size");
    return true;
}

std::string CloudMdkRecordPhotosReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{";
    ss << "\"size\": " << this->size << ",";
    ss << "}";
    return ss.str();
}

std::vector<CloudMdkRecordPhotosVo> CloudMdkRecordPhotosRespBody::GetPhotosRecords()
{
    return this->cloudPhotosUploadRecord_;
}

bool CloudMdkRecordPhotosRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(static_cast<int32_t>(cloudPhotosUploadRecord_.size())),
                             false,
                             "cloudPhotosUploadRecord.size()");
    for (const auto &entry : cloudPhotosUploadRecord_) {
        CHECK_AND_RETURN_RET_LOG(entry.Marshalling(parcel), false, "CloudMdkRecordPhotosVo");
    }
    return true;
}

// 服务端->客户端；在客户端反序列化
bool CloudMdkRecordPhotosRespBody::UnmarshallRecords(std::vector<CloudMdkRecordPhotosVo> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(len >= 0, false, "len >= 0");
    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    bool isInValid = (size > readAbleSize) || (size > val.max_size());
    CHECK_AND_RETURN_RET_LOG(!isInValid, false, "size invalid");
    val.clear();
    for (size_t i = 0; i < size; i++) {
        CloudMdkRecordPhotosVo nodeObj;
        CHECK_AND_RETURN_RET_LOG(nodeObj.Unmarshalling(parcel), false, "CloudMdkRecordPhotosVo");
        val.emplace_back(nodeObj);
    }
    return true;
}

bool CloudMdkRecordPhotosRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(
        UnmarshallRecords(this->cloudPhotosUploadRecord_, parcel), false, "cloudPhotosUploadRecord_");
    return true;
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
        ss << "\"" << node.second << "\"";
    }
    ss << "},";
    ss << "\"int64fields\": {";
    for (const auto &node : this->int64fields) {
        ss << "\"" << node.first << "\": ";
        ss << node.second << ", ";
    }
    ss << "}";
}

size_t CloudMdkRecordPhotosRespBody::GetDataSize() const
{
    return this->cloudPhotosUploadRecord_.size();
}

bool CloudMdkRecordPhotosRespBody::TruncateDataBy200K()
{
    CHECK_AND_RETURN_RET_LOG(!this->cloudPhotosUploadRecord_.empty(), true, "cloudPhotosUploadRecord not empty");
    constexpr size_t PARCEL_GAP = 4800;
    constexpr size_t MAX_CAPACITY = 204800;
    const size_t parcelCapacity = MAX_CAPACITY - PARCEL_GAP;
    const size_t originalSize = this->cloudPhotosUploadRecord_.size();
    size_t parcelSize = 0;
    std::vector<CloudMdkRecordPhotosVo> resultList;
    for (size_t index = 0; index < originalSize; index++) {
        MessageParcel tempParcel;
        // Try marshalling into MessageParcel.
        CHECK_AND_BREAK_ERR_LOG(this->cloudPhotosUploadRecord_[index].Marshalling(tempParcel),
            "Marshalling error, truncate stop. "
            "index: %{public}zu, resultList: %{public}zu, originalSize: %{public}zu",
            index,
            resultList.size(),
            originalSize);
        // Check the dataSize not exceed capacity.
        size_t elementSize = tempParcel.GetDataSize();
        parcelSize += elementSize;
        CHECK_AND_BREAK_ERR_LOG(
            parcelSize <= parcelCapacity,
            "exceed capacity, truncate it. "
            "elementSize: %{public}zu, index: %{public}zu, resultList: %{public}zu, originalSize: %{public}zu, "
            "parcelSize: %{public}zu, parcelCapacity: %{public}zu",
            elementSize,
            index,
            resultList.size(),
            originalSize,
            parcelSize,
            parcelCapacity);
        resultList.emplace_back(this->cloudPhotosUploadRecord_[index]);
    }
    // No need to truncate body.
    CHECK_AND_RETURN_RET_LOG(resultList.size() != originalSize, true, "resultList.size() != originalSize");
    this->cloudPhotosUploadRecord_ = resultList;
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