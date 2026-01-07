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

#include "media_asset_manager_capi.h"

#include <cstring>
#include <securec.h>

#include "media_log.h"
#include "media_asset_base_capi.h"
#include "media_asset_data_handler_capi.h"
#include "media_asset_magic.h"

using namespace OHOS::Media;

const std::string ERROR_REQUEST_ID = "00000000-0000-0000-0000-000000000000";

struct MediaAssetMangerObject : public OH_MediaAssetManager {
    explicit MediaAssetMangerObject(const std::shared_ptr<MediaAssetManager> &manager)
        : manager_(manager) {}
    ~MediaAssetMangerObject() = default;

    const std::shared_ptr<MediaAssetManager> manager_ = nullptr;
};

void OH_MediaAssetManager_Convert(const MediaLibrary_RequestOptions &srcRequestOption,
    NativeRequestOptions &dstRequestOption)
{
    dstRequestOption.deliveryMode = static_cast<NativeDeliveryMode>(srcRequestOption.deliveryMode);
}

OH_MediaAssetManager *OH_MediaAssetManager_Create(void)
{
    std::shared_ptr<MediaAssetManager> manager = MediaAssetManagerFactory::CreateMediaAssetManager();
    CHECK_AND_PRINT_LOG(manager != nullptr, "failed to MediaAssetManagerFactory::CreateMediaAssetManager");
    MediaAssetMangerObject *object = new(std::nothrow) MediaAssetMangerObject(manager);
    CHECK_AND_PRINT_LOG(object != nullptr, "failed to new MediaAssetMangerObject");
    return object;
}

MediaLibrary_RequestId OH_MediaAssetManager_RequestImageForPath(OH_MediaAssetManager* manager, const char* uri,
    MediaLibrary_RequestOptions requestOptions, const char* destPath, OH_MediaLibrary_OnDataPrepared callback)
{
    MediaLibrary_RequestId requestId;
    strncpy_s(requestId.requestId, UUID_STR_LENGTH, ERROR_REQUEST_ID.c_str(), UUID_STR_LENGTH);

    CHECK_AND_RETURN_RET_LOG(manager != nullptr, requestId, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, requestId, "managerObj is null");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, requestId, "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(uri != nullptr, requestId, "uri is null");
    CHECK_AND_RETURN_RET_LOG(destPath != nullptr, requestId, "destPath is null");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, requestId, "callback is null");
    NativeRequestOptions nativeRequestOptions;
    OH_MediaAssetManager_Convert(requestOptions, nativeRequestOptions);
    NativeOnDataPrepared nativeCallback = reinterpret_cast<NativeOnDataPrepared>(callback);
    CHECK_AND_RETURN_RET_LOG(nativeCallback != nullptr, requestId, "nativeCallback is null");

    std::string requestIdStr = managerObj->manager_->NativeRequestImage(uri, nativeRequestOptions, destPath,
        nativeCallback);
    strncpy_s(requestId.requestId, UUID_STR_LENGTH, requestIdStr.c_str(), UUID_STR_LENGTH);
    return requestId;
}

MediaLibrary_RequestId OH_MediaAssetManager_RequestVideoForPath(OH_MediaAssetManager* manager, const char* uri,
    MediaLibrary_RequestOptions requestOptions, const char* destPath, OH_MediaLibrary_OnDataPrepared callback)
{
    MediaLibrary_RequestId requestId;
    strncpy_s(requestId.requestId, UUID_STR_LENGTH, ERROR_REQUEST_ID.c_str(), UUID_STR_LENGTH);

    CHECK_AND_RETURN_RET_LOG(manager != nullptr, requestId, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, requestId, "imanagerObj is nullptr!");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, requestId, "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(uri != nullptr, requestId, "uri is null");
    CHECK_AND_RETURN_RET_LOG(destPath != nullptr, requestId, "destPath is null");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, requestId, "callback is null");
    NativeRequestOptions nativeRequestOptions;
    OH_MediaAssetManager_Convert(requestOptions, nativeRequestOptions);
    NativeOnDataPrepared nativeCallback = reinterpret_cast<NativeOnDataPrepared>(callback);
    CHECK_AND_RETURN_RET_LOG(nativeCallback != nullptr, requestId, "nativeCallback is null");

    std::string requestIdStr = managerObj->manager_->NativeRequestVideo(uri, nativeRequestOptions, destPath,
        nativeCallback);
    strncpy_s(requestId.requestId, UUID_STR_LENGTH, requestIdStr.c_str(), UUID_STR_LENGTH);
    return requestId;
}

bool OH_MediaAssetManager_CancelRequest(OH_MediaAssetManager* manager, const MediaLibrary_RequestId requestId)
{
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, false, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, false, "managerObj is nullptr!");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, false, "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(strlen(requestId.requestId) > 0, false, "requestId is empty");
    return managerObj->manager_->NativeCancelRequest(requestId.requestId);
}

MediaLibrary_ErrorCode OH_MediaAssetManager_RequestImage(OH_MediaAssetManager* manager, OH_MediaAsset* mediaAsset,
    MediaLibrary_RequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnImageDataPrepared callback)
{
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "managerObj is null");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "mediaAsset is nullptr!");
    CHECK_AND_RETURN_RET_LOG(requestId != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "requestId is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "callback is nullptr!");

    NativeRequestOptions nativeRequestOptions;
    OH_MediaAssetManager_Convert(requestOptions, nativeRequestOptions);
    return managerObj->manager_->NativeRequestImageSource(mediaAsset, nativeRequestOptions, requestId, callback);
}

MediaLibrary_ErrorCode OH_MediaAssetManager_RequestMovingPhoto(OH_MediaAssetManager* manager, OH_MediaAsset* mediaAsset,
    MediaLibrary_RequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnMovingPhotoDataPrepared callback)
{
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "managerObj is null");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "mediaAsset is nullptr!");
    CHECK_AND_RETURN_RET_LOG(requestId != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "requestId is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "callback is nullptr!");

    NativeRequestOptions nativeRequestOptions;
    OH_MediaAssetManager_Convert(requestOptions, nativeRequestOptions);
    return managerObj->manager_->NativeRequestMovingPhoto(mediaAsset, nativeRequestOptions, requestId, callback);
}

MediaLibrary_ErrorCode OH_MediaAssetManager_Release(OH_MediaAssetManager* manager)
{
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MEDIA_LIBRARY_PARAMETER_ERROR, "input manager is nullptr!");

    delete manager;
    manager = nullptr;
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode OH_MediaAssetManager_QuickRequestImage(OH_MediaAssetManager* manager, OH_MediaAsset* mediaAsset,
    MediaLibrary_RequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnQuickImageDataPrepared callback)
{
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "input manager is nullptr!");
    struct MediaAssetMangerObject *managerObj = reinterpret_cast<MediaAssetMangerObject *>(manager);
    CHECK_AND_RETURN_RET_LOG(managerObj != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "managerObj is null");
    CHECK_AND_RETURN_RET_LOG(managerObj->manager_ != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "manager_ is null");
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "mediaAsset is nullptr!");
    CHECK_AND_RETURN_RET_LOG(requestId != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "requestId is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "callback is nullptr!");

    NativeRequestOptions nativeRequestOptions;
    OH_MediaAssetManager_Convert(requestOptions, nativeRequestOptions);
    return managerObj->manager_->NativeQuickRequestImage(mediaAsset, nativeRequestOptions, requestId, callback);
}