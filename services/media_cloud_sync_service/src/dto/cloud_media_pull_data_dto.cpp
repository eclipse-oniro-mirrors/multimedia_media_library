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

#include "cloud_media_pull_data_dto.h"

#include <sstream>

#include "cloud_media_dao_const.h"
#include "cloud_media_sync_const.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
void CloudMediaPullDataDto::GetBasicInfo(std::stringstream &ss) const
{
    ss << "\"cloudId\": \"" << cloudId << "\","
       << "\"cDisplayName\": \"" << MediaFileUtils::DesensitizeName(basicDisplayName) << "\","
       << "\"cIsDelete\": " << std::to_string(basicIsDelete) << ","
       << "\"cSize\": " << basicSize << ","
       << "\"cMimeType\": \"" << basicMimeType << "\","
       << "\"cDeviceName\": \"" << basicDeviceName << "\","
       << "\"cEditedTime\": " << basicEditedTime << ","
       << "\"cCreatedTime\": " << basicCreatedTime << ","
       << "\"dateTaken\": " << dateTaken << ","
       << "\"cIsFavorite\": " << basicIsFavorite << ","
       << "\"cIsRecycle\": " << basicIsRecycle << ","
       << "\"cRecycledTime\": " << basicRecycledTime << ","
       << "\"cFileType\": " << basicFileType << ","
       << "\"cCloudVersion\": " << basicCloudVersion << ","
       << "\"hasAttributes\": " << std::to_string(hasAttributes) << ","
       << "\"cMediaType\": " << attributesMediaType << ","
       << "\"duration\": " << duration << ","
       << "\"cHidden\": " << attributesHidden << ","
       << "\"cHiddenTime\": " << attributesHiddenTime << ","
       << "\"cMetaDateModified\": " << attributesMetaDateModified << ","
       << "\"cSubtype\": " << attributesSubtype << ",";
}
void CloudMediaPullDataDto::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"cDateYear\": \"" << attributesDateYear << "\","
       << "\"cDateMonth\": \"" << attributesDateMonth << "\","
       << "\"cDateDay\": \"" << attributesDateDay << "\","
       << "\"exifRotate\": " << exifRotate << ","
       << "\"cEditedTimeMs\": " << attributesEditedTimeMs << ","
       << "\"cFixVersion\": " << attributesFixVersion << ",";
}
void CloudMediaPullDataDto::GetPropertiesInfo(std::stringstream &ss) const
{
    ss << "\"hasProperties\": " << std::to_string(hasProperties) << ","
       << "\"cRotate\": " << propertiesRotate << ","
       << "\"cHeight\": " << propertiesHeight << ","
       << "\"cWidth\": " << propertiesWidth << ","
       << "\"cFirstUpdateTime\": \"" << propertiesFirstUpdateTime << "\","
       << "\"cDetailTime\": \"" << propertiesDetailTime << "\","
       << "\"lFileId\": " << localFileId << ","
       << "\"cPath\": \"" << MediaFileUtils::DesensitizePath(localPath) << "\","
       << "\"lSize\": " << localSize << ","
       << "\"lcdSize\": " << lcdSize << ","
       << "\"thmSize\": " << thmSize << ","
       << "\"lMediaType\": " << localMediaType << ","
       << "\"lDateAdded\": \"" << localDateAdded << "\","
       << "\"lDateModified\": \"" << localDateModified << "\","
       << "\"lDirty\": " << localDirty << ","
       << "\"lPosition\": " << localPosition << ","
       << "\"lOwnerAlbumId\": \"" << localOwnerAlbumId << "\","
       << "\"lOrientation\": " << localOrientation << ","
       << "\"lThumbState\": " << localThumbState;
}
void CloudMediaPullDataDto::GetCloudInfo(std::stringstream &ss) const
{
    ss << "\"cCloudId\": \"" << attributesCloudId << "\","
       << "\"cOriginCloudId\": \"" << attributesOriginCloudId << "\",";
}
void CloudMediaPullDataDto::GetAlbumIds(std::stringstream &ss) const
{
    ss << "[";
    for (uint32_t i = 0; i < attributesSrcAlbumIds.size(); ++i) {
        if (i != attributesSrcAlbumIds.size() - 1) {
            ss << "\"" << attributesSrcAlbumIds[i] << "\",";
            continue;
        }
        ss << "\"" << attributesSrcAlbumIds[i] << "\"";
    }
    ss << "],";
}
std::string CloudMediaPullDataDto::ToString() const
{
    std::stringstream ss;
    ss << "{";
    this->GetBasicInfo(ss);
    this->GetAttributesInfo(ss);
    this->GetAttributesHashMap(ss);
    this->GetPropertiesInfo(ss);
    this->GetCloudInfo(ss);
    this->GetAlbumIds(ss);
    ss << "}";
    return ss.str();
}

bool CloudMediaPullDataDto::IsHiddenAsset() const
{
    auto it = std::find_if(this->attributesSrcAlbumIds.begin(),
        this->attributesSrcAlbumIds.end(),
        [](const std::string &cloudId) { return cloudId == HIDDEN_ALBUM_CLOUD_ID; });
    return it != this->attributesSrcAlbumIds.end();
}

bool CloudMediaPullDataDto::IsVideoAsset()
{
    return this->basicFileType == FILE_TYPE_VIDEO;
}

bool CloudMediaPullDataDto::FindAlbumCloudId(std::string &albumCloudId)
{
    auto it = std::find_if(this->attributesSrcAlbumIds.begin(),
        this->attributesSrcAlbumIds.end(),
        [](const std::string &cloudId) { return cloudId != HIDDEN_ALBUM_CLOUD_ID; });
    bool isFound = it != this->attributesSrcAlbumIds.end();
    CHECK_AND_RETURN_RET(isFound, false);
    albumCloudId = *it;
    return true;
}

bool CloudMediaPullDataDto::FindAlbumUploadStatus() const
{
    CHECK_AND_RETURN_RET_LOG(!this->IsHiddenAsset(), false, "this is a hidden asset");
    CHECK_AND_RETURN_RET_LOG(this->albumInfoOp.has_value(), false, "albumInfoOp is not init");
    PhotoAlbumPo albumInfo = this->albumInfoOp.value();
    CHECK_AND_RETURN_RET_LOG(!albumInfo.IsCamera(), true, "this is a camera album");
    return albumInfo.uploadStatus.value_or(0) == 1;
}

bool CloudMediaPullDataDto::GetIsRecycleUpdated() const
{
    return this->isRecycleUpdated;
}

void CloudMediaPullDataDto::SetIsRecycleUpdated(bool isUpdated)
{
    this->isRecycleUpdated = isUpdated;
}

void CloudMediaPullDataDto::GetAttributesHashMap(std::stringstream &ss) const
{
    ss << "\"stringfields\": {";
    for (const auto &node : this->stringfields) {
        ss << "\"" << node.first << "\": ";
        ss << "\"" << node.second << "\", ";
    }
    ss << "}, ";
    ss << "\"int64fields\": {";
    for (const auto &node : this->int64fields) {
        ss << "\"" << node.first << "\": ";
        ss << node.second << ", ";
    }
    ss << "}";
    return;
}
}  // namespace OHOS::Media::CloudSync