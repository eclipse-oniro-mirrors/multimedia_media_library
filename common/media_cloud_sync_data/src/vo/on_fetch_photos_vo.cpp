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

#include "on_fetch_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media::CloudSync {
bool OnFetchPhotosVo::MarshallingBasicInfo(Parcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileSourcePath), false, "fileSourcePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->mimeType), false, "mimeType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->firstVisitTime), false, "firstVisitTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->detailTime), false, "detailTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->frontCamera), false, "frontCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->editDataCamera), false, "editDataCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->title), false, "title");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->relativePath), false, "relativePath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->virtualPath), false, "virtualPath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->dateYear), false, "dateYear");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->dateMonth), false, "dateMonth");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->dateDay), false, "dateDay");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->shootingMode), false, "shootingMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->shootingModeTag), false, "shootingModeTag");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->burstKey), false, "burstKey");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->localPath), false, "localPath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteDouble(this->latitude), false, "latitude");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteDouble(this->longitude), false, "longitude");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->description), false, "description");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->source), false, "source");
    return true;
}

bool OnFetchPhotosVo::MarshallingAttributesInfo(Parcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileType), false, "fileType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->rotation), false, "rotation");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->photoHeight), false, "photoHeight");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->photoWidth), false, "photoWidth");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->duration), false, "duration");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->burstCoverLevel), false, "burstCoverLevel");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->subtype), false, "subtype");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->originalSubtype), false, "originalSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->dynamicRangeType), false, "dynamicRangeType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->hdrMode), false, "hdrMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->videoMode), false, "videoMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->movingPhotoEffectMode), false, "movingPhotoEffectMode");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->supportedWatermarkType), false, "supportedWatermarkType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->strongAssociation), false, "strongAssociation");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->fixVersion), false, "fixVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->version), false, "version");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->lcdSize), false, "lcdSize");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->thmSize), false, "thmSize");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->createTime), false, "createTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->dualEditTime), false, "dualEditTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->editTime), false, "editTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->editedTimeMs), false, "editedTimeMs");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->recycledTime), false, "recycledTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->hiddenTime), false, "hiddenTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->coverPosition), false, "coverPosition");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->isRectificationCover), false, "isRectificationCover");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->exifRotate), false, "exifRotate");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->isDelete), false, "isDelete");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->packageName), false, "packageName");
    // Safe Album: risk status for children's watch
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->isCritical), false, "isCritical");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->compressionQuality), false, "compressionQuality");
    return true;
}

bool OnFetchPhotosVo::ReadBasicInfo(Parcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileSourcePath), false, "fileSourcePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->mimeType), false, "mimeType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->firstVisitTime), false, "firstVisitTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->detailTime), false, "detailTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->frontCamera), false, "frontCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->editDataCamera), false, "editDataCamera");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->title), false, "title");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->relativePath), false, "relativePath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->virtualPath), false, "virtualPath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->dateYear), false, "dateYear");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->dateMonth), false, "dateMonth");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->dateDay), false, "dateDay");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->shootingMode), false, "shootingMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->shootingModeTag), false, "shootingModeTag");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->burstKey), false, "burstKey");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->localPath), false, "localPath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadDouble(this->latitude), false, "latitude");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadDouble(this->longitude), false, "longitude");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->description), false, "description");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->source), false, "source");
    return true;
}

bool OnFetchPhotosVo::ReadAttributesInfo(Parcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->mediaType), false, "mediaType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileType), false, "fileType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->rotation), false, "rotation");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->photoHeight), false, "photoHeight");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->photoWidth), false, "photoWidth");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->duration), false, "duration");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->hidden), false, "hidden");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->burstCoverLevel), false, "burstCoverLevel");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->subtype), false, "subtype");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->originalSubtype), false, "originalSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->dynamicRangeType), false, "dynamicRangeType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->hdrMode), false, "hdrMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->videoMode), false, "videoMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->movingPhotoEffectMode), false, "movingPhotoEffectMode");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->supportedWatermarkType), false, "supportedWatermarkType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->strongAssociation), false, "strongAssociation");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->fixVersion), false, "fixVersion");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->version), false, "version");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->lcdSize), false, "lcdSize");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->thmSize), false, "thmSize");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->createTime), false, "createTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->dualEditTime), false, "dualEditTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->editTime), false, "editTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->editedTimeMs), false, "editedTimeMs");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->recycledTime), false, "recycledTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->hiddenTime), false, "hiddenTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->coverPosition), false, "coverPosition");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->isRectificationCover), false, "isRectificationCover");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->exifRotate), false, "exifRotate");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->isDelete), false, "isDelete");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->packageName), false, "packageName");
    // Safe Album: risk status for children's watch
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->photoRiskStatus), false, "photoRiskStatus");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->isCritical), false, "isCritical");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->compressionQuality), false, "compressionQuality");
    return true;
}

bool OnFetchPhotosVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(this->ReadBasicInfo(parcel), false, "ReadBasicInfo");
    CHECK_AND_RETURN_RET_LOG(this->ReadAttributesInfo(parcel), false, "ReadAttributesInfo");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->hasAttributes), false, "hasAttributes");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->hasproperties), false, "hasproperties");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->isFavorite), false, "isFavorite");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->isRecycle), false, "isRecycle");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->sourceAlbumIds, parcel), false, "sourceAlbumIds");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Unmarshalling(stringfields, parcel), false, "stringfields");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Unmarshalling(int64fields, parcel), false, "int64fields");
    return true;
}

bool OnFetchPhotosVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(this->MarshallingBasicInfo(parcel), false, "MarshallingBasicInfo");
    CHECK_AND_RETURN_RET_LOG(this->MarshallingAttributesInfo(parcel), false, "MarshallingAttributesInfo");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->hasAttributes), false, "hasAttributes");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->hasproperties), false, "hasproperties");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->isFavorite), false, "isFavorite");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->isRecycle), false, "isRecycle");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::Marshalling<std::string>(this->sourceAlbumIds, parcel), false, "sourceAlbumIds");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(stringfields, parcel), false, "stringfields");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(int64fields, parcel), false, "int64fields");
    return true;
}

void OnFetchPhotosVo::GetBasicInfo(std::stringstream &ss) const
{
    ss << "\"cloudId\": \"" << cloudId << "\","
       << "\"mimeType\": \"" << mimeType << "\","
       << "\"firstVisitTime\": \"" << firstVisitTime << "\","
       << "\"detailTime\": \"" << detailTime << "\","
       << "\"frontCamera\": \"" << frontCamera << "\","
       << "\"editDataCamera\": \"" << editDataCamera << "\","
       << "\"relativePath\": \"" << relativePath << "\","
       << "\"virtualPath\": \"" << virtualPath << "\","
       << "\"dateYear\": \"" << dateYear << "\","
       << "\"dateMonth\": \"" << dateMonth << "\","
       << "\"dateDay\": \"" << dateDay << "\","
       << "\"shootingMode\": \"" << shootingMode << "\","
       << "\"shootingModeTag\": \"" << shootingModeTag << "\","
       << "\"burstKey\": \"" << burstKey << "\","
       << "\"fileId\": \"" << fileId << "\","
       << "\"mediaType\": \"" << mediaType << "\","
       << "\"fileType\": \"" << fileType << "\","
       << "\"rotation\": \"" << rotation << "\","
       << "\"fixVersion\": \"" << fixVersion << "\","
       << "\"photoHeight\": \"" << photoHeight << "\","
       << "\"photoWidth\": \"" << photoWidth << "\","
       << "\"duration\": \"" << duration << "\","
       << "\"hidden\": \"" << hidden << "\",";
    return;
}

void OnFetchPhotosVo::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"burstCoverLevel\": \"" << burstCoverLevel << "\","
       << "\"subtype\": \"" << subtype << "\","
       << "\"originalSubtype\": \"" << originalSubtype << "\","
       << "\"dynamicRangeType\": \"" << dynamicRangeType << "\","
       << "\"movingPhotoEffectMode\": \"" << movingPhotoEffectMode << "\","
       << "\"version\": \"" << version << "\","
       << "\"size\": \"" << size << "\","
       << "\"lcdSize\": \"" << lcdSize << "\","
       << "\"thmSize\": \"" << thmSize << "\","
       << "\"createTime\": \"" << createTime << "\","
       << "\"metaDateModified\": \"" << metaDateModified << "\","
       << "\"dualEditTime\": \"" << dualEditTime << "\","
       << "\"editTime\": \"" << editTime << "\","
       << "\"editedTimeMs\": \"" << editedTimeMs << "\","
       << "\"recycledTime\": \"" << recycledTime << "\","
       << "\"hiddenTime\": \"" << hiddenTime << "\","
       << "\"coverPosition\": \"" << coverPosition << "\","
       << "\"isRectificationCover\": \"" << isRectificationCover << "\","
       << "\"exifRotate\": \"" << exifRotate << "\","
       << "\"isDelete\": \"" << isDelete << "\","
       << "\"hasAttributes\": \"" << hasAttributes << "\","
       << "\"hasproperties\": \"" << hasproperties << "\","
       << "\"isFavorite\": \"" << isFavorite << "\","
       << "\"isRecycle\": \"" << isRecycle << "\","
       << "\"description\": \"" << description << "\","
       << "\"DeviceName\": \"" << source << "\","
       << "\"videoMode\": \"" << videoMode << "\","
       << "\"uniqueId\": \"" << uniqueId << "\","
       << "\"packageName\": \"" << packageName << "\","
       << "\"photoRiskStatus\": \"" << photoRiskStatus << "\","
       << "\"isCritical\": \"" << isCritical << "\","
       << "\"compressionQuality\": \"" << compressionQuality << "\"";
    return;
}

std::string OnFetchPhotosVo::ToString() const
{
    std::stringstream ss;
    ss << "{";
    this->GetBasicInfo(ss);
    this->GetAttributesInfo(ss);
    this->GetAttributesHashMap(ss);
    ss << "\"AlbumIds\":[";
    for (uint32_t i = 0; i < sourceAlbumIds.size(); i++) {
        if (i != sourceAlbumIds.size() - 1) {
            ss << "\"" << sourceAlbumIds[i] << "\",";
            continue;
        }
        ss << "\"" << sourceAlbumIds[i] << "\"";
    }
    ss << "]}";
    return ss.str();
}

void OnFetchPhotosVo::GetAttributesHashMap(std::stringstream &ss) const
{
    ss << "\"stringfields\": {";
    for (const auto &node : this->stringfields) {
        ss << "\"" << node.first << "\": ";
        ss << "\"" << node.second << "\", ";
    }
    ss << "},";
    ss << "\"int64fields\": {";
    for (const auto &node : this->int64fields) {
        ss << "\"" << node.first << "\": ";
        ss << node.second << ", ";
    }
    ss << "}";
    return;
}
}  // namespace OHOS::Media::CloudSync