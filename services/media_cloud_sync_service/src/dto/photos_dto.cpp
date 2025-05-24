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

#define MLOG_TAG "MEDIA_CLOUD_DTO"

#include "photos_dto.h"

#include <sstream>

namespace OHOS::Media::CloudSync {
void PhotosDto::GetAttachment(std::stringstream &ss) const
{
    bool first = true;
    for (auto &node : attachment) {
        if (!first) {
            ss << ", ";
        }
        ss << "\"" << node.first << "\": " << node.second.ToString();
        first = false;
    }
}

void PhotosDto::GetBasicInfo(std::stringstream &ss) const
{
    ss << "\"cloudId\": \"" << this->cloudId << "\", "
       << "\"data\": \"" << this->data << "\", "
       << "\"path\": \"" << this->path << "\", "
       << "\"size\": " << this->size << ", "
       << "\"dateModified\": " << this->dateModified << ", "
       << "\"dirty\": " << this->dirty << ", "
       << "\"dateTrashed\": " << this->dateTrashed << ", "
       << "\"position\": " << this->position << ", "
       << "\"localId\": " << this->localId << ", "
       << "\"dkRecordId\": \"" << this->dkRecordId << "\", "
       << "\"cloudVersion\": " << this->cloudVersion << ", "
       << "\"fileId\": " << this->fileId << ", "
       << "\"fileType\": " << this->fileType << ", "
       << "\"relativePath\": \"" << this->relativePath << "\", ";
}

void PhotosDto::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"ownerAlbumId\": " << this->ownerAlbumId << ", "
       << "\"syncStatus\": " << this->syncStatus << ", "
       << "\"thumbStatus\": " << this->thumbStatus << ", "
       << "\"displayName\": \"" << this->displayName << "\", "
       << "\"fileName\": \"" << this->fileName << "\", "
       << "\"orientation\": " << this->orientation << ", "
       << "\"subtype\": " << this->subtype << ", "
       << "\"movingPhotoEffectMode\": " << this->movingPhotoEffectMode << ", "
       << "\"originalSubtype\": " << this->originalSubtype << ", "
       << "\"sourcePath\": \"" << this->sourcePath << "\", "
       << "\"livePhotoCachePath\": \"" << this->livePhotoCachePath << "\", "
       << "\"mimeType\": " << this->mimeType << ", "
       << "\"mediaType\": " << this->mediaType << ", "
       << "\"serverErrorCode\": " << this->serverErrorCode << ", "
       << "\"cloudAlbumId\": " << this->cloudAlbumId << ", "
       << "\"rotation\": " << this->rotation << ", "
       << "\"version\": " << this->version << ", "
       << "\"errorType\": " << this->errorType << ", ";
}

void PhotosDto::GetPropertiesInfo(std::stringstream &ss) const
{
    ss << "\"metaDateModified\": " << this->metaDateModified << ", "
       << "\"editedTimeMs\": " << this->editedTimeMs << ", "
       << "\"modifyTime\": " << this->modifiedTime << ", "
       << "\"createTime\": " << this->createTime << ", "
       << "\"dateAdded\": " << this->dateAdded << ", ";
}

std::string PhotosDto::ToString() const
{
    std::stringstream ss;
    ss << "{";
    this->GetBasicInfo(ss);
    this->GetAttributesInfo(ss);
    this->GetPropertiesInfo(ss);
    ss << "\"isSuccess\": " << std::to_string(this->isSuccess) << ", \"errorDetails\":[";
    for (uint32_t i = 0; i < errorDetails.size(); ++i) {
        if (i != errorDetails.size() - 1) {
            ss << errorDetails[i].ToString() << ",";
            continue;
        }
        ss << errorDetails[i].ToString();
    }
    ss << "]";
    ss << "\"attachment\": {";
    this->GetAttachment(ss);
    ss << "}"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync