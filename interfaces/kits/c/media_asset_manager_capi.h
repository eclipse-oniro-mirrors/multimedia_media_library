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

/**
 * @addtogroup MediaAssetManager
 * @{
 *
 * @brief Provides APIs of request capability for Media Source.
 *
 * @since 12
 */

/**
 * @file media_asset_manager_capi.h
 *
 * @brief Defines the media asset manager APIs.
 *
 * Uses the Native APIs provided by Media Asset Manager
 * to reqeust media source.
 *
 * @kit MediaLibraryKit
 * @syscap SystemCapability.FileManagement.PhotoAccessHelper.Core
 * @library libmedia_asset_manager.so
 * @since 12
 */

#ifndef MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_MANAGER_H
#define MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_MANAGER_H

#include <stdbool.h>

#include "media_asset_base_capi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a media asset manager.
 *
 * @return Returns a pointer to an OH_MediaAssetManager instance.
 * @since 12
*/
OH_MediaAssetManager* OH_MediaAssetManager_Create(void);

/**
 * @brief Request image source with dest path.
 *
 * @permission ohos.permission.READ_IMAGEVIDEO
 * @param manager Pointer to an OH_MediaAssetManager instance.
 * @param uri The uri of the requested image resource.
 * @param requestOptions Options model for requesting resource.
 * @param destPath Destination address of the requested resource.
 * @param callback Called when a requested source is prepared.
 * @return Return Request id.
 * @since 12
*/
MediaLibrary_RequestId OH_MediaAssetManager_RequestImageForPath(OH_MediaAssetManager* manager, const char* uri,
    MediaLibrary_RequestOptions requestOptions, const char* destPath, OH_MediaLibrary_OnDataPrepared callback);

/**
 * @brief Request video source with dest path.
 *
 * @permission ohos.permission.READ_IMAGEVIDEO
 * @param manager Pointer to an OH_MediaAssetManager instance.
 * @param uri The uri of the requested video resource.
 * @param requestOptions Options model for requesting resource.
 * @param destPath Destination address of the requested resource.
 * @param callback Called when a requested source is prepared.
 * @return Return Request id.
 * @since 12
*/
MediaLibrary_RequestId OH_MediaAssetManager_RequestVideoForPath(OH_MediaAssetManager* manager, const char* uri,
    MediaLibrary_RequestOptions requestOptions, const char* destPath, OH_MediaLibrary_OnDataPrepared callback);

/**
 * @brief Cancel request by request id.
 *
 * @permission ohos.permission.READ_IMAGEVIDEO
 * @param manager Pointer to an OH_MediaAssetManager instance.
 * @param requestId The request id to be canceled.
 * @return Returns true if the request is canceled successfully; returns false otherwise.
 * @since 12
*/
bool OH_MediaAssetManager_CancelRequest(OH_MediaAssetManager* manager, const MediaLibrary_RequestId requestId);

/**
 * @brief Request image resources based on different strategy modes.
 *
 * @permission ohos.permission.READ_IMAGEVIDEO
 * @param manager the pointer to {@link OH_MediaAssetManager} instance.
 * @param mediaAsset the {@link OH_MediaAsset} instance of media file object to be requested.
 * @param requestOptions the {@link MediaLibrary_RequestOptions} for image request strategy mode.
 * @param requestId indicates the {@link MediaLibrary_RequestId} of the request, which is an output parameter.
 * @param callback the {@link OH_MediaLibrary_OnImageDataPrepared} that will be called
 *                 when the requested source is prepared.
 * @return {@link #MEDIA_LIBRARY_OK} if the method call succeeds.
 *         {@link #MEDIA_LIBRARY_PARAMETER_ERROR} Parameter error. Possible causes:
 *                                                1. Mandatory parameters are left unspecified.
 *                                                2. Incorrect parameter types.
 *                                                3. Parameter verification failed.
 *         {@link #MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED} if operation is not supported.
 *         {@link #MEDIA_LIBRARY_PERMISSION_DENIED} if permission is denied.
 *         {@link #MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR} if internal system error.
 * @since 12
 */
MediaLibrary_ErrorCode OH_MediaAssetManager_RequestImage(OH_MediaAssetManager* manager, OH_MediaAsset* mediaAsset,
    MediaLibrary_RequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnImageDataPrepared callback);

/**
 * @brief Request moving photo object.
 *
 * @permission ohos.permission.READ_IMAGEVIDEO
 * @param manager the pointer to {@link OH_MediaAssetManager} instance.
 * @param mediaAsset the {@link OH_MediaAsset} instance of media file object to be requested.
 * @param requestOptions the {@link MediaLibrary_RequestOptions} for image request strategy mode.
 * @param requestId indicates the {@link MediaLibrary_RequestId} of the request, which is an output parameter.
 * @param callback the {@link OH_MediaLibrary_OnMovingPhotoDataPrepared} that will be called
 *                 when the requested source is prepared.
 * @return {@link #MEDIA_LIBRARY_OK} if the method call succeeds.
 *         {@link #MEDIA_LIBRARY_PARAMETER_ERROR} Parameter error. Possible causes:
 *                                                1. Mandatory parameters are left unspecified.
 *                                                2. Incorrect parameter types.
 *                                                3. Parameter verification failed.
 *         {@link #MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED} if operation is not supported.
 *         {@link #MEDIA_LIBRARY_PERMISSION_DENIED} if permission is denied.
 *         {@link #MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR} if internal system error.
 * @since 13
 */
MediaLibrary_ErrorCode OH_MediaAssetManager_RequestMovingPhoto(OH_MediaAssetManager* manager, OH_MediaAsset* mediaAsset,
    MediaLibrary_RequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnMovingPhotoDataPrepared callback);

/**
 * @brief Release the {@link OH_MediaAssetManager} instance.
 *
 * @param manager the {@link OH_MediaAssetManager} instance.
 * @return {@link #MEDIA_LIBRARY_OK} if the method call succeeds.
 *         {@link #MEDIA_LIBRARY_PARAMETER_ERROR} Parameter error. Possible causes:
 *                                                1. Mandatory parameters are left unspecified.
 *                                                2. Incorrect parameter types.
 *                                                3. Parameter verification failed.
 * @since 13
 */
MediaLibrary_ErrorCode OH_MediaAssetManager_Release(OH_MediaAssetManager* manager);

#ifdef __cplusplus
}
#endif

#endif // MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_MANAGER_H
/** @} */
