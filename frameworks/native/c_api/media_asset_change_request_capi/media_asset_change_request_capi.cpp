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

#include "media_asset_change_request_capi.h"

#include "media_log.h"
#include "oh_media_asset.h"
#include "oh_media_asset_change_request.h"
#include "media_asset_change_request.h"
#include "userfile_manager_types.h"

using namespace OHOS::Media;

MediaLibrary_ErrorCode OH_MediaAssetChangeRequest_GetWriteCacheHandler(OH_MediaAssetChangeRequest* changeRequest,
    int32_t* fd)
{
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");
    CHECK_AND_RETURN_RET_LOG(changeRequest->request_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "request_ is nullptr!");

    return changeRequest->request_->GetWriteCacheHandler(fd);
}

MediaLibrary_ErrorCode OH_MediaAssetChangeRequest_SaveCameraPhoto(OH_MediaAssetChangeRequest* changeRequest,
    MediaLibrary_ImageFileType imageFileType)
{
    MEDIA_INFO_LOG("OH_MediaAssetChangeRequest_SaveCameraPhoto begin.");
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");
    CHECK_AND_RETURN_RET_LOG(changeRequest->request_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "request_ is nullptr!");

    return changeRequest->request_->SaveCameraPhoto(imageFileType);
}

MediaLibrary_ErrorCode OH_MediaAssetChangeRequest_DiscardCameraPhoto(OH_MediaAssetChangeRequest* changeRequest)
{
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");
    CHECK_AND_RETURN_RET_LOG(changeRequest->request_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "request_ is nullptr!");

    return changeRequest->request_->DiscardCameraPhoto();
}

OH_MediaAssetChangeRequest* OH_MediaAssetChangeRequest_Create(OH_MediaAsset* mediaAsset)
{
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, nullptr, "mediaAsset is nullptr!");
    CHECK_AND_RETURN_RET_LOG(mediaAsset->mediaAsset_ != nullptr, nullptr, "mediaAsset_ is nullptr!");
    auto fileAssetPtr = mediaAsset->mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAssetPtr != nullptr, nullptr, "fileAssetPtr is nullptr!");

    OH_MediaAssetChangeRequest* changeRequest = nullptr;
    if (fileAssetPtr->GetMediaType() == OHOS::Media::MEDIA_TYPE_IMAGE ||
        fileAssetPtr->GetMediaType() == OHOS::Media::MEDIA_TYPE_VIDEO) {
        auto mediaAssetChangeRequest =
            MediaAssetChangeRequestFactory::CreateMediaAssetChangeRequest(mediaAsset->mediaAsset_);
        CHECK_AND_RETURN_RET_LOG(mediaAssetChangeRequest != nullptr, nullptr,
            "create CreateMediaAssetChangeRequest failed!");
        changeRequest = new OH_MediaAssetChangeRequest(mediaAssetChangeRequest);
        CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, nullptr, "create OH_MediaAssetChangeRequest failed!");
    }
    return changeRequest;
}

MediaLibrary_ErrorCode  OH_MediaAssetChangeRequest_AddResourceWithUri(OH_MediaAssetChangeRequest* changeRequest,
    MediaLibrary_ResourceType resourceType, char* fileUri)
{
    MEDIA_INFO_LOG("OH_MediaAssetChangeRequest_AddResourceWithUri begin.");
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");
    CHECK_AND_RETURN_RET_LOG(changeRequest->request_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "request_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(fileUri != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "fileUri is nullptr!");

    return changeRequest->request_->AddResourceWithUri(resourceType, fileUri);
}

MediaLibrary_ErrorCode OH_MediaAssetChangeRequest_AddResourceWithBuffer(OH_MediaAssetChangeRequest* changeRequest,
    MediaLibrary_ResourceType resourceType, uint8_t* buffer, uint32_t length)
{
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");
    CHECK_AND_RETURN_RET_LOG(changeRequest->request_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "request_ is nullptr!");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "buffer is null");
    CHECK_AND_RETURN_RET_LOG(length > 0, MEDIA_LIBRARY_PARAMETER_ERROR, "length is zero");

    return changeRequest->request_->AddResourceWithBuffer(resourceType, buffer, length);
}

MediaLibrary_ErrorCode OH_MediaAssetChangeRequest_Release(OH_MediaAssetChangeRequest* changeRequest)
{
    CHECK_AND_RETURN_RET_LOG(changeRequest != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "changeRequest is nullptr!");

    delete changeRequest;
    changeRequest = nullptr;
    return MEDIA_LIBRARY_OK;
}