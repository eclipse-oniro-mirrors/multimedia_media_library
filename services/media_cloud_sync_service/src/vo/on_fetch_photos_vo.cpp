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
    CHECK_AND_RETURN_RET(parcel.WriteString(this->cloudId), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->fileName), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->fileSourcePath), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->mimeType), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->firstVisitTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->detailTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->frontCamera), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->editDataCamera), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->title), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->relativePath), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->virtualPath), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->dateYear), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->dateMonth), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->dateDay), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->shootingMode), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->shootingModeTag), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->burstKey), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->localPath), false);
    CHECK_AND_RETURN_RET(parcel.WriteDouble(this->latitude), false);
    CHECK_AND_RETURN_RET(parcel.WriteDouble(this->longitude), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->description), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->source), false);
    return true;
}

bool OnFetchPhotosVo::MarshallingAttributesInfo(Parcel &parcel) const
{
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->fileId), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->mediaType), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->fileType), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->rotation), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->photoHeight), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->photoWidth), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->duration), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->hidden), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->burstCoverLevel), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->subtype), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->originalSubtype), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->dynamicRangeType), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->hdrMode), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->videoMode), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->movingPhotoEffectMode), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->supportedWatermarkType), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->strongAssociation), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->fixVersion), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->version), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->size), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->lcdSize), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->thmSize), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->createTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->metaDateModified), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->dualEditTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->editTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->editedTimeMs), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->recycledTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->hiddenTime), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt64(this->coverPosition), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->isRectificationCover), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->exifRotate), false);
    CHECK_AND_RETURN_RET(parcel.WriteBool(this->isDelete), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->fileSourceType), false);
    CHECK_AND_RETURN_RET(parcel.WriteString(this->storagePath), false);
    // Safe Album: risk status for children's watch
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->photoRiskStatus), false);
    CHECK_AND_RETURN_RET(parcel.WriteInt32(this->isCritical), false);
    return true;
}

bool OnFetchPhotosVo::ReadBasicInfo(Parcel &parcel)
{
    CHECK_AND_RETURN_RET(parcel.ReadString(this->cloudId), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->fileName), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->fileSourcePath), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->mimeType), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->firstVisitTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->detailTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->frontCamera), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->editDataCamera), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->title), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->relativePath), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->virtualPath), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->dateYear), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->dateMonth), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->dateDay), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->shootingMode), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->shootingModeTag), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->burstKey), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->localPath), false);
    CHECK_AND_RETURN_RET(parcel.ReadDouble(this->latitude), false);
    CHECK_AND_RETURN_RET(parcel.ReadDouble(this->longitude), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->description), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->source), false);
    return true;
}

bool OnFetchPhotosVo::ReadAttributesInfo(Parcel &parcel)
{
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->fileId), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->mediaType), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->fileType), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->rotation), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->photoHeight), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->photoWidth), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->duration), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->hidden), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->burstCoverLevel), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->subtype), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->originalSubtype), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->dynamicRangeType), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->hdrMode), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->videoMode), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->movingPhotoEffectMode), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->supportedWatermarkType), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->strongAssociation), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->fixVersion), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->version), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->size), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->lcdSize), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->thmSize), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->createTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->metaDateModified), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->dualEditTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->editTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->editedTimeMs), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->recycledTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->hiddenTime), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt64(this->coverPosition), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->isRectificationCover), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->exifRotate), false);
    CHECK_AND_RETURN_RET(parcel.ReadBool(this->isDelete), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->fileSourceType), false);
    CHECK_AND_RETURN_RET(parcel.ReadString(this->storagePath), false);
    // Safe Album: risk status for children's watch
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->photoRiskStatus), false);
    CHECK_AND_RETURN_RET(parcel.ReadInt32(this->isCritical), false);
    return true;
}

bool OnFetchPhotosVo::Unmarshalling(MessageParcel &parcel)
{
    this->ReadBasicInfo(parcel);
    this->ReadAttributesInfo(parcel);
    CHECK_AND_RETURN_RET(parcel.ReadBool(this->hasAttributes), false);
    CHECK_AND_RETURN_RET(parcel.ReadBool(this->hasproperties), false);
    CHECK_AND_RETURN_RET(parcel.ReadBool(this->isFavorite), false);
    CHECK_AND_RETURN_RET(parcel.ReadBool(this->isRecycle), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->sourceAlbumIds, parcel), false);
    CHECK_AND_RETURN_RET(ITypesUtil::Unmarshalling(stringfields, parcel), false);
    return true;
}

bool OnFetchPhotosVo::Marshalling(MessageParcel &parcel) const
{
    this->MarshallingBasicInfo(parcel);
    this->MarshallingAttributesInfo(parcel);
    CHECK_AND_RETURN_RET(parcel.WriteBool(this->hasAttributes), false);
    CHECK_AND_RETURN_RET(parcel.WriteBool(this->hasproperties), false);
    CHECK_AND_RETURN_RET(parcel.WriteBool(this->isFavorite), false);
    CHECK_AND_RETURN_RET(parcel.WriteBool(this->isRecycle), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Marshalling<std::string>(this->sourceAlbumIds, parcel), false);
    CHECK_AND_RETURN_RET(ITypesUtil::Marshalling(stringfields, parcel), false);
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
       << "\"fileSourceType\": \"" << fileSourceType << "\","
       << "\"storagePath\": \"" << storagePath << "\","
       << "\"photoRiskStatus\": \"" << photoRiskStatus << "\","
       << "\"isCritical\": \"" << isCritical << "\",";
    return;
}

std::string OnFetchPhotosVo::ToString() const
{
    std::stringstream ss;
    ss << "{";
    this->GetBasicInfo(ss);
    this->GetAttributesInfo(ss);
    this->GetAttributesHashMap(ss);
    ss << "AlbumIds:[";
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
    ss << "}";
    return;
}
}  // namespace OHOS::Media::CloudSync