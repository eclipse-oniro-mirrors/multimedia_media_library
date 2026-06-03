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
       << "\"basicDisplayName\": \"" << MediaFileUtils::DesensitizeName(basicDisplayName) << "\","
       << "\"basicIsDelete\": " << std::to_string(basicIsDelete) << ","
       << "\"basicSize\": " << basicSize << ","
       << "\"basicMimeType\": \"" << basicMimeType << "\","
       << "\"basicDeviceName\": \"" << basicDeviceName << "\","
       << "\"basicEditedTime\": " << basicEditedTime << ","
       << "\"basicCreatedTime\": " << basicCreatedTime << ","
       << "\"dateTaken\": " << dateTaken << ","
       << "\"basicIsFavorite\": " << basicIsFavorite << ","
       << "\"basicIsRecycle\": " << basicIsRecycle << ","
       << "\"basicRecycledTime\": " << basicRecycledTime << ","
       << "\"basicDescription\": \"" << basicDescription << "\","
       << "\"basicFileType\": " << basicFileType << ","
       << "\"basicCloudVersion\": " << basicCloudVersion << ","
       << "\"hasAttributes\": " << std::to_string(hasAttributes) << ","
       << "\"attributesMediaType\": " << attributesMediaType << ","
       << "\"duration\": " << duration << ","
       << "\"attributesHidden\": " << attributesHidden << ","
       << "\"attributesHiddenTime\": " << attributesHiddenTime << ","
       << "\"attributesRelativePath\": \"" << attributesRelativePath << "\","
       << "\"attributesVirtualPath\": \"" << attributesVirtualPath << "\","
       << "\"attributesMetaDateModified\": " << attributesMetaDateModified << ","
       << "\"attributesSubtype\": " << attributesSubtype << ",";
}
void CloudMediaPullDataDto::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"attributesBurstCoverLevel\": " << attributesBurstCoverLevel << ","
       << "\"attributesBurstKey\": \"" << attributesBurstKey << "\","
       << "\"attributesDateYear\": \"" << attributesDateYear << "\","
       << "\"attributesDateMonth\": \"" << attributesDateMonth << "\","
       << "\"attributesDateDay\": \"" << attributesDateDay << "\","
       << "\"attributesShootingMode\": \"" << attributesShootingMode << "\","
       << "\"attributesShootingModeTag\": \"" << attributesShootingModeTag << "\","
       << "\"attributesDynamicRangeType\": " << attributesDynamicRangeType << ","
       << "\"attributesHdrMode\": " << attributesHdrMode << ","
       << "\"attributesVideoMode\": " << attributesVideoMode << ","
       << "\"attributesFrontCamera\": " << attributesFrontCamera << ","
       << "\"attributesEditTime\": " << attributesEditTime << ","
       << "\"attributesOriginalSubtype\": " << attributesOriginalSubtype << ","
       << "\"attributesCoverPosition\": " << attributesCoverPosition << ","
       << "\"attributesIsRectificationCover\": " << attributesIsRectificationCover << ","
       << "\"exifRotate\": " << exifRotate << ","
       << "\"attributesMovingPhotoEffectMode\": " << attributesMovingPhotoEffectMode << ","
       << "\"attributesSupportedWatermarkType\": " << attributesSupportedWatermarkType << ","
       << "\"attributesStrongAssociation\": " << attributesStrongAssociation << ","
       << "\"attributesFileId\": " << attributesFileId << ","
       << "\"attributesEditedTimeMs\": " << attributesEditedTimeMs << ","
       << "\"attributesFixVersion\": " << attributesFixVersion << ","
       << "\"attributesEditDataCamera\": \"" << attributesEditDataCamera.empty() << "\","
       << "\"deferredEffectsStatus\": " << deferredEffectsStatus << ","
       << "\"fileSourceType\": " << attributesFileSourceType << ","
       << "\"storagePath\": " << MediaFileUtils::DesensitizePath(attributesStoragePath) << ",";
}
void CloudMediaPullDataDto::GetPropertiesInfo(std::stringstream &ss) const
{
    ss << "\"hasProperties\": " << std::to_string(hasProperties) << ","
       << "\"propertiesRotate\": " << propertiesRotate << ","
       << "\"propertiesHeight\": " << propertiesHeight << ","
       << "\"propertiesWidth\": " << propertiesWidth << ","
       << "\"propertiesFirstUpdateTime\": \"" << propertiesFirstUpdateTime << "\","
       << "\"propertiesDetailTime\": \"" << propertiesDetailTime << "\","
       << "\"localFileId\": " << localFileId << ","
       << "\"localPath\": \"" << MediaFileUtils::DesensitizePath(localPath) << "\","
       << "\"localSize\": " << localSize << ","
       << "\"lcdSize\": " << lcdSize << ","
       << "\"thmSize\": " << thmSize << ","
       << "\"localMediaType\": " << localMediaType << ","
       << "\"localDateAdded\": \"" << localDateAdded << "\","
       << "\"localDateModified\": \"" << localDateModified << "\","
       << "\"localDirty\": " << localDirty << ","
       << "\"localPosition\": " << localPosition << ","
       << "\"localOwnerAlbumId\": \"" << localOwnerAlbumId << "\","
       << "\"localOrientation\": " << localOrientation << ","
       << "\"localThumbState\": " << localThumbState;
}
void CloudMediaPullDataDto::GetCloudInfo(std::stringstream &ss) const
{
    ss << "\"attributesCloudId\": \"" << attributesCloudId << "\","
       << "\"attributesOriginCloudId\": \"" << attributesOriginCloudId << "\",";
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