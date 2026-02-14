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
#define MLOG_TAG "Media_ORM"

#include "photos_po.h"

#include "media_log.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"
#include "medialibrary_db_const.h"

namespace OHOS::Media::ORM {
void PhotosPo::GetAlbumInfo(std::stringstream &ss) const
{
    ss << "\"albumCloudId\": \"" << albumCloudId.value_or("") << "\", "
       << "\"albumLPath\": \"" << albumLPath.value_or("") << "\", ";
}

void PhotosPo::GetBasicInfo(std::stringstream &ss) const
{
    ss << "\"fileId\": " << fileId.value_or(0) << ", "
       << "\"cloudId\": \"" << cloudId.value_or("") << "\", "
       << "\"size\": " << size.value_or(0) << ", "
       << "\"displayName\": \"" << displayName.value_or("") << "\", "
       << "\"isFavorite\": " << isFavorite.value_or(0) << ", "
       << "\"hidden\": " << hidden.value_or(0) << ", "
       << "\"hiddenTime\": " << hiddenTime.value_or(0) << ", "
       << "\"dateTrashed\": " << dateTrashed.value_or(0) << ", "
       << "\"orientation\": " << orientation.value_or(0) << ", "
       << "\"sourcePath\": \"" << sourcePath.value_or("") << "\", ";
}

void PhotosPo::GetPropertiesInfo(std::stringstream &ss) const
{
    ss << "\"deviceName\": \"" << deviceName.value_or("") << "\", "
       << "\"dateAdded\": " << dateAdded.value_or(0) << ", "
       << "\"dateModified\": " << dateModified.value_or(0) << ", "
       << "\"dateTaken\": " << dateTaken.value_or(0) << ", "
       << "\"duration\": " << duration.value_or(0) << ", "
       << "\"dateYear\": \"" << dateYear.value_or("") << "\", "
       << "\"dateMonth\": \"" << dateMonth.value_or("") << "\", "
       << "\"dateDay\": \"" << dateDay.value_or("") << "\", "
       << "\"detailTime\": \"" << detailTime.value_or("") << "\", "
       << "\"editTime\": " << editTime.value_or(0) << ", ";
}

void PhotosPo::GetAttributesInfo(std::stringstream &ss) const
{
    ss << "\"ownerAlbumId\": " << ownerAlbumId.value_or(0) << ", "
       << "\"data\": \"" << data.value_or("") << "\", "
       << "\"title\": \"" << title.value_or("") << "\", "
       << "\"mediaType\": " << mediaType.value_or(0) << ", "
       << "\"mimeType\": \"" << mimeType.value_or("") << "\", "
       << "\"relativePath\": \"" << relativePath.value_or("") << "\", "
       << "\"virtualPath\": \"" << virtualPath.value_or("") << "\", "
       << "\"latitude\": " << latitude.value_or(0.0) << ", "
       << "\"longitude\": " << longitude.value_or(0.0) << ", "
       << "\"height\": " << height.value_or(0) << ", "
       << "\"width\": " << width.value_or(0) << ", "
       << "\"subtype\": " << subtype.value_or(0) << ", "
       << "\"burstCoverLevel\": " << burstCoverLevel.value_or(1) << ", "
       << "\"burstKey\": \"" << burstKey.value_or("") << "\", "
       << "\"userComment\": \"" << userComment.value_or("") << "\", "
       << "\"thumbStatus\": " << thumbStatus.value_or(0) << ", "
       << "\"syncStatus\": " << syncStatus.value_or(0) << ", "
       << "\"shootingMode\": \"" << shootingMode.value_or("") << "\", "
       << "\"shootingModeTag\": \"" << shootingModeTag.value_or("") << "\", "
       << "\"dynamicRangeType\": " << dynamicRangeType.value_or(0) << ", "
       << "\"frontCamera\": \"" << frontCamera.value_or("") << "\", "
       << "\"coverPosition\": " << coverPosition.value_or(0) << ", "
       << "\"isRectificationCover\": " << isRectificationCover.value_or(0) << ", "
       << "\"movingPhotoEffectMode\": " << movingPhotoEffectMode.value_or(0) << ", "
       << "\"supportedWatermarkType\": " << supportedWatermarkType.value_or(0) << ", "
       << "\"isStylePhoto\": " << isStylePhoto.value_or(0) << ", "
       << "\"strongAssociation\": " << strongAssociation.value_or(0) << ", "
       << "\"southDeviceType\": " << southDeviceType.value_or(0) << ", ";
}

void PhotosPo::GetCloudInfo(std::stringstream &ss) const
{
    ss << "\"position\": \"" << position.value_or(-1) << "\", "
       << "\"metaDateModified\": " << metaDateModified.value_or(0) << ", "
       << "\"originalSubtype\": " << originalSubtype.value_or(0) << ", "
       << "\"dirty\": \"" << dirty.value_or(-1) << "\", "
       << "\"baseVersion\": \"" << baseVersion.value_or(-1) << "\", "
       << "\"cloudVersion\": \"" << cloudVersion.value_or(-1) << "\", "
       << "\"originalAssetCloudId\": \"" << originalAssetCloudId.value_or("") << "\", ";
}

void PhotosPo::GetRemoveAlbumCloudInfo(std::stringstream &ss) const
{
    ss << "\"removeAlbumCloudId\": ";
    ss << "[";
    for (auto &albumId : removeAlbumCloudId) {
        ss << "\"" << albumId << "\",";
    }
    ss << "]";
}

std::string PhotosPo::ToString() const
{
    std::stringstream ss;
    ss << "{";
    this->GetAlbumInfo(ss);
    this->GetBasicInfo(ss);
    this->GetCloudInfo(ss);
    this->GetPropertiesInfo(ss);
    this->GetAttributesInfo(ss);
    ss << "}";
    return ss.str();
}

bool PhotosPo::IsCloudAsset() const
{
    bool isCloud = this->dirty.value_or(1) != static_cast<int32_t>(DirtyType::TYPE_NEW);
    isCloud = isCloud && !this->cloudId.value_or("").empty();
    isCloud = isCloud && this->position.value_or(1) != static_cast<int32_t>(PhotoPositionType::LOCAL);
    return isCloud;
}

int32_t PhotosPo::TryGetMdirty() const
{
    const int32_t oldDirty = this->dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_NEW));
    bool isValid = oldDirty == static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    isValid = isValid || oldDirty == static_cast<int32_t>(DirtyType::TYPE_SDIRTY);
    isValid = isValid || oldDirty == static_cast<int32_t>(DirtyType::TYPE_TDIRTY);
    CHECK_AND_RETURN_RET_INFO_LOG(
        isValid, oldDirty, "Can not get mdirty, oldDirty: %{public}d is available.", oldDirty);
    return static_cast<int32_t>(DirtyType::TYPE_MDIRTY);
}

bool PhotosPo::ShouldHandleAsMediaFile() const
{
    return this->fileSourceType.value_or(0) == static_cast<int32_t>(FileSourceType::MEDIA);
}

bool PhotosPo::ShouldHandleAsLakeFile() const
{
    return !this->storagePath.value_or("").empty();
}

std::string PhotosPo::BuildFileUri() const
{
    std::string filePath = this->data.value_or("");
    std::string displayName = this->displayName.value_or("");
    int32_t fileId = this->fileId.value_or(0);
    size_t lastSlashInData = filePath.rfind('/');
    std::string fileNameWithExtensionInData =
        (lastSlashInData != std::string::npos) ? filePath.substr(lastSlashInData + 1) : filePath;
    size_t dotPos = fileNameWithExtensionInData.rfind('.');
    std::string fileNameWithoutExtensionInData = fileNameWithExtensionInData;
    if (dotPos != std::string::npos) {
        fileNameWithoutExtensionInData = fileNameWithoutExtensionInData.substr(0, dotPos);
    }
    std::stringstream ss;
    ss << PhotoColumn::PHOTO_URI_PREFIX << fileId << "/" << fileNameWithoutExtensionInData << "/" << displayName;
    return ss.str();
}
}  // namespace OHOS::Media::ORM