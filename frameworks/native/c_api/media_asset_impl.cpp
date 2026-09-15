/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "media_asset_impl.h"

#include "media_file_utils.h"
#include "media_log.h"
#include <securec.h>
#include "nocopyable.h"
#include "userfile_manager_types.h"

using namespace OHOS::Media;

const static int64_t MILLI_TO_SECOND = 1000;
const static int32_t MAX_URI_LENGTH = 256;
const static int32_t MAX_DISPLAY_NAME_LENGTH = MAX_URI_LENGTH + 6;
const static int32_t MAX_TITLE_LENGTH = 256;

const std::unordered_map<MediaType, MediaLibrary_MediaType> g_mediaTypeMapping = {
    {MediaType::MEDIA_TYPE_IMAGE, MediaLibrary_MediaType::MEDIA_LIBRARY_IMAGE},
    {MediaType::MEDIA_TYPE_VIDEO, MediaLibrary_MediaType::MEDIA_LIBRARY_VIDEO},
};

const std::unordered_map<PhotoSubType, MediaLibrary_MediaSubType> g_photoSubTypeMapping = {
    {PhotoSubType::DEFAULT, MediaLibrary_MediaSubType::MEDIA_LIBRARY_DEFAULT},
    {PhotoSubType::CAMERA, MediaLibrary_MediaSubType::MEDIA_LIBRARY_DEFAULT},
    {PhotoSubType::MOVING_PHOTO, MediaLibrary_MediaSubType::MEDIA_LIBRARY_MOVING_PHOTO},
    {PhotoSubType::BURST, MediaLibrary_MediaSubType::MEDIA_LIBRARY_BURST},
};

std::shared_ptr<MediaAsset> MediaAssetFactory::CreateMediaAsset(
    std::shared_ptr<FileAsset> fileAsset)
{
    std::shared_ptr<MediaAssetImpl> impl = std::make_shared<MediaAssetImpl>(fileAsset);
    CHECK_AND_PRINT_LOG(impl != nullptr, "Failed to create MediaAssetManagerImpl instance.");

    return impl;
}

MediaAssetImpl::MediaAssetImpl(std::shared_ptr<FileAsset> fileAsset)
{
    MEDIA_DEBUG_LOG("MediaAssetImpl Constructor is called.");
    fileAsset_ = fileAsset;
    uri_ = new char[MAX_URI_LENGTH];
    displayName_ = new char[MAX_DISPLAY_NAME_LENGTH];
    title_ = new char[MAX_TITLE_LENGTH];
}

MediaAssetImpl::~MediaAssetImpl()
{
    if (fileAsset_ != nullptr) {
        fileAsset_ = nullptr;
    }

    if (uri_ != nullptr) {
        delete[] uri_;
        uri_ = nullptr;
    }

    if (displayName_ != nullptr) {
        delete[] displayName_;
        displayName_ = nullptr;
    }

    if (title_ != nullptr) {
        delete[] title_;
        title_ = nullptr;
    }
}

MediaLibrary_ErrorCode MediaAssetImpl::GetUri(const char** uri)
{
    if (uri_ == nullptr) {
        uri_ = new(std::nothrow) char[MAX_URI_LENGTH];
        CHECK_AND_RETURN_RET_LOG(uri_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "alloc memory failed!");
    }
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    const std::string fileUri = fileAsset_->GetUri();
    int32_t uriLen = static_cast<int32_t>(fileUri.length());
    int32_t len = uriLen < MAX_URI_LENGTH ? uriLen : MAX_URI_LENGTH - 1;
    strncpy_s(uri_, MAX_URI_LENGTH, fileUri.c_str(), len);
    MEDIA_INFO_LOG("MediaAssetImpl::GetUri, uri: %{public}s, return uri: %{public}s",
        fileUri.c_str(), uri_);
    *uri = uri_;
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetMediaType(MediaLibrary_MediaType* mediaType)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    MediaType type = fileAsset_->GetMediaType();
    MEDIA_INFO_LOG("GetMediaType type: %{public}d", static_cast<int32_t>(type));
    auto itr = g_mediaTypeMapping.find(type);
    if (itr != g_mediaTypeMapping.end()) {
        *mediaType = itr->second;
        return MEDIA_LIBRARY_OK;
    }
    return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetMediaSubType(MediaLibrary_MediaSubType* mediaSubType)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    PhotoSubType subType = static_cast<PhotoSubType>(fileAsset_->GetPhotoSubType());
    MEDIA_INFO_LOG("GetMediaSubType subType: %{public}d", static_cast<int32_t>(subType));
    auto itr = g_photoSubTypeMapping.find(subType);
    if (itr != g_photoSubTypeMapping.end()) {
        *mediaSubType = itr->second;
        return MEDIA_LIBRARY_OK;
    }
    return MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDisplayName(const char** displayName)
{
    if (displayName_ == nullptr) {
        displayName_ = new(std::nothrow) char[MAX_DISPLAY_NAME_LENGTH];
        CHECK_AND_RETURN_RET_LOG(displayName_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "alloc memory failed!");
    }
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    const std::string display = fileAsset_->GetDisplayName();
    int32_t displayNameLen = static_cast<int32_t>(display.length());
    int32_t len = displayNameLen < MAX_DISPLAY_NAME_LENGTH ? displayNameLen : MAX_DISPLAY_NAME_LENGTH - 1;
    strncpy_s(displayName_, MAX_DISPLAY_NAME_LENGTH, display.c_str(), len);
    MEDIA_INFO_LOG("MediaAssetImpl::GetDisplayName, display name: %{public}s, return display name: %{public}s",
        MediaFileUtils::DesensitizeName(display).c_str(), MediaFileUtils::DesensitizeName(displayName_).c_str());
    *displayName = displayName_;
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetSize(uint32_t* size)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *size = static_cast<uint32_t>(fileAsset_->GetSize());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDateAdded(uint32_t* dateAdded)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *dateAdded = static_cast<uint32_t>(fileAsset_->GetDateAdded() / MILLI_TO_SECOND);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDateModified(uint32_t* dateModified)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *dateModified = static_cast<uint32_t>(fileAsset_->GetDateModified() / MILLI_TO_SECOND);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDateAddedMs(uint32_t* dateAddedMs)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *dateAddedMs = static_cast<uint32_t>(fileAsset_->GetDateAdded());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDateModifiedMs(uint32_t* dateModifiedMs)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *dateModifiedMs = static_cast<uint32_t>(fileAsset_->GetDateModified());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDateTaken(uint32_t* dateTaken)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *dateTaken = static_cast<uint32_t>(fileAsset_->GetDateTaken());
    return MEDIA_LIBRARY_OK;
}

std::shared_ptr<FileAsset> MediaAssetImpl::GetFileAssetInstance() const
{
    return fileAsset_;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetDuration(uint32_t* duration)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *duration  = static_cast<uint32_t>(fileAsset_->GetDuration());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetWidth(uint32_t* width)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *width = static_cast<uint32_t>(fileAsset_->GetWidth());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetHeight(uint32_t* height)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *height = static_cast<uint32_t>(fileAsset_->GetHeight());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetOrientation(uint32_t* orientation)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *orientation = static_cast<uint32_t>(fileAsset_->GetOrientation());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::IsFavorite(uint32_t* favorite)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    *favorite = static_cast<uint32_t>(fileAsset_->IsFavorite());
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetImpl::GetTitle(const char** title)
{
    if (title_ == nullptr) {
        title_ = new(std::nothrow) char[MAX_TITLE_LENGTH];
        CHECK_AND_RETURN_RET_LOG(title_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "alloc memory failed!");
    }
    CHECK_AND_RETURN_RET_LOG(fileAsset_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");
    const std::string titleStr = fileAsset_->GetTitle();
    int32_t titleLen = static_cast<int32_t>(titleStr.length());
    int32_t len = titleLen < MAX_TITLE_LENGTH ? titleLen : MAX_TITLE_LENGTH - 1;
    strncpy_s(title_, MAX_TITLE_LENGTH, titleStr.c_str(), len);
    MEDIA_INFO_LOG("GetTitle, title: %{public}s, return title: %{public}s",
        MediaFileUtils::DesensitizeName(titleStr).c_str(),
        MediaFileUtils::DesensitizeName(title_).c_str());
    *title = title_;
    return MEDIA_LIBRARY_OK;
}
