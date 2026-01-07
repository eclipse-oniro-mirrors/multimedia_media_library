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
 * The OH_MediaAssetManager structure and MediaLibrary_RequestId type are used to request media library resources.
 * The request can be cancelled using the request ID.
 *
 * @since 12
 */

/**
 * @file media_asset_base_capi.h
 *
 * @brief Defines the structure and enumeration for Media Asset Manager.
 *
 * OH_MediaAssetManager structure: This structure provides the ability to request resources from a media library. \n
 * MediaLibrary_RequestId type: This type is returned when requesting a media library resource.
 * The request ID is used to cancel the request. \n
 * MediaLibrary_DeliveryMode enumeration: This enumeration defines the delivery mode of the requested resources. \n
 * OH_MediaLibrary_OnDataPrepared function pointer: This function is called when the requested source is prepared. \n
 * MediaLibrary_RequestOptions structure: This structure provides options for requesting media library resources. \n
 *
 * @kit MediaLibraryKit
 * @syscap SystemCapability.FileManagement.PhotoAccessHelper.Core
 * @library libmedia_asset_manager.so
 * @since 12
 */

#ifndef MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_BASE_H
#define MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_BASE_H

#include <stdint.h>

#include "image_source_native.h"
#include "picture_native.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define UUID max length
 *
 * This constant defines the maximum length of a UUID string.
 *
 * @since 12
 */
static const int32_t UUID_STR_MAX_LENGTH = 37;

/**
 * @brief Define Media Asset Manager
 *
 * This structure provides the ability to request media library resources.
 * Null pointer is returned if the creation fails.
 *
 * @since 12
 */
typedef struct OH_MediaAssetManager OH_MediaAssetManager;

/**
 * @brief Define Media Asset Change Request
 *
 * This structure provides the ability to handle a media asset change request.
 *
 * @since 12
 */
typedef struct OH_MediaAssetChangeRequest OH_MediaAssetChangeRequest;

/**
 * @brief Define Moving Photo
 *
 * This structure provides the ability to obtain information about moving photo.
 *
 * @since 13
 */
typedef struct OH_MovingPhoto OH_MovingPhoto;

/**
 * @brief Define Media Asset
 *
 * This structure provides the ability to encapsulate file asset attributes.
 *
 * @since 12
 */
typedef struct OH_MediaAsset OH_MediaAsset;

/**
 * @brief Define MediaLibrary_RequestId
 *
 * This type is returned when requesting a media library resource.
 * The request id is used to cancel the request.
 * The value is all zero like "00000000-0000-0000-0000-000000000000" if the request fails.
 *
 * @since 12
 */
typedef struct MediaLibrary_RequestId {
    /*request id*/
    char requestId[UUID_STR_MAX_LENGTH];
} MediaLibrary_RequestId;

/**
 * @brief Enum for media library error code.
 *
 * @since 12
 */
typedef enum MediaLibrary_ErrorCode {
    /**
     * @error Media library result is ok.
     */
    MEDIA_LIBRARY_OK = 0,

    /**
     * @error Permission is denied.
     */
    MEDIA_LIBRARY_PERMISSION_DENIED = 201,

    /**
     * @error Mandatory parameters are left unspecified
     * or incorrect parameter types or parameter verification failed.
     */
    MEDIA_LIBRARY_PARAMETER_ERROR = 401,

    /**
     * @error File does not exist.
     */
    MEDIA_LIBRARY_NO_SUCH_FILE = 23800101,

    /**
     * @error Invalid display name.
     */
    MEDIA_LIBRARY_INVALID_DISPLAY_NAME = 23800102,

    /**
     * @error Invalid asset uri.
     */
    MEDIA_LIBRARY_INVALID_ASSET_URI = 23800103,

    /**
     * @error Member is not a valid PhotoKey.
     */
    MEDIA_LIBRARY_INVALID_PHOTO_KEY = 23800104,

    /**
     * @error The scenario parameter verification fails.
     * It is recommended to retry and check the logs.
     * Possible causes:
     * 1. The type is not fixed at 'hiddenPhotoChange'.
     * 2. The same callback is registered repeatedly.
     */
    MEDIA_LIBRARY_INVALID_PARAMETER_ERROR = 23800151,

    /**
     * @error Operation is not supported.
     */
    MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED = 23800201,

    /**
     * @error Internal system error.
     * It is recommended to retry and check the logs.
     * Possible causes:
     * 1. Database corrupted.
     * 2. The file system is abnormal.
     * 3. The IPC request timed out.
     */
    MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR = 23800301,
} MediaLibrary_ErrorCode;

/**
 * @brief Delivery Mode
 *
 * This enumeration defines the delivery mode of the requested resources.
 * The delivery mode can be set to fast mode, high quality mode, or balanced mode.
 *
 * @since 12
 */
typedef enum MediaLibrary_DeliveryMode {
    /*delivery fast mode*/
    MEDIA_LIBRARY_FAST_MODE = 0,
    /*delivery high quality mode*/
    MEDIA_LIBRARY_HIGH_QUALITY_MODE = 1,
    /*delivery balanced mode*/
    MEDIA_LIBRARY_BALANCED_MODE = 2
} MediaLibrary_DeliveryMode;

/**
 * @brief Request Options
 *
 * This structure provides options for requesting media library resources.
 *
 * @since 12
 */
typedef struct MediaLibrary_RequestOptions {
    /*delivery mode*/
    MediaLibrary_DeliveryMode deliveryMode;
} MediaLibrary_RequestOptions;

/**
 * @brief Enum for media type.
 *
 * @since 12
 */
typedef enum MediaLibrary_MediaType {
    /*image asset*/
    MEDIA_LIBRARY_IMAGE = 1,
    /**
     * @brief HEIF type
     *
     * @since 23
     */
    /*video asset*/
    MEDIA_LIBRARY_VIDEO = 2
} MediaLibrary_MediaType;

/**
 * @brief Enum for media asset subtype.
 *
 * @since 12
 */
typedef enum MediaLibrary_MediaSubType {
    /*default Photo Type*/
    MEDIA_LIBRARY_DEFAULT = 0,
    /*moving Photo Type*/
    MEDIA_LIBRARY_MOVING_PHOTO = 3,
    /*burst Photo Type*/
    MEDIA_LIBRARY_BURST = 4
} MediaLibrary_MediaSubType;

/**
 * @brief Enum for resource types.
 *
 * @since 12
 */
typedef enum MediaLibrary_ResourceType {
    /*image resource*/
    MEDIA_LIBRARY_IMAGE_RESOURCE = 1,
    /*video resource*/
    MEDIA_LIBRARY_VIDEO_RESOURCE = 2
} MediaLibrary_ResourceType;

/**
 * @brief Enum for image file Type.
 *
 * @since 12
 */
typedef enum MediaLibrary_ImageFileType {
    /*JPEG type*/
    MEDIA_LIBRARY_IMAGE_JPEG = 1,
    /*HEIF type*/
    MEDIA_LIBRARY_IMAGE_HEIF = 2,
    /*MPEG type*/
    MEDIA_LIBRARY_FILE_VIDEO = 3
} MediaLibrary_ImageFileType;

/**
 * @brief Enum for media quality.
 *
 * @since 12
 */
typedef enum MediaLibrary_MediaQuality {
    /*fast quality*/
    MEDIA_LIBRARY_QUALITY_FAST = 1,
    /*full quality*/
    MEDIA_LIBRARY_QUALITY_FULL = 2
} MediaLibrary_MediaQuality;

/**
 * @brief Enum for media content type.
 *
 * @since 12
 */
typedef enum MediaLibrary_MediaContentType {
    /*compressed media content type*/
    MEDIA_LIBRARY_COMPRESSED = 1,
    /*picture object media content type*/
    MEDIA_LIBRARY_PICTURE_OBJECT = 2
} MediaLibrary_MediaContentType;

/**
 * @brief Called when a requested source is prepared.
 *
 * This function is called when the requested source is prepared.
 *
 * @param result Results of the processing of the requested resources.
 * @param requestId Request ID.
 * @since 12
 */
typedef void (*OH_MediaLibrary_OnDataPrepared)(int32_t result, MediaLibrary_RequestId requestId);

/**
 * @brief Called when a requested image source is prepared.
 *
 * This function is called when the requested image source is prepared.
 *
 * @param result results {@link MediaLibrary_ErrorCode} of the processing of the requested resources.
 * @param requestId indicates the {@link MediaLibrary_RequestId} of the request.
 * @param mediaQuality the {@link MediaLibrary_MediaQuality} of the requested source.
 * @param type the {@link MediaLibrary_MediaContentType} of the requested source.
 * @param imageSourceNative it used to obtain {@link OH_ImageSourceNative} information when image source is prepared.
 * @since 12
 */
typedef void (*OH_MediaLibrary_OnImageDataPrepared)(MediaLibrary_ErrorCode result,
    MediaLibrary_RequestId requestId, MediaLibrary_MediaQuality mediaQuality, MediaLibrary_MediaContentType type,
    OH_ImageSourceNative* imageSourceNative);

/**
 * @brief Called when a requested moving photo is prepared.
 *
 * This function is called when the requested moving photo is prepared.
 *
 * @param result results {@link MediaLibrary_ErrorCode} of the processing of the requested resources.
 * @param requestId indicates the {@link MediaLibrary_RequestId} of the request.
 * @param mediaQuality the {@link MediaLibrary_MediaQuality} of the requested source.
 * @param type the {@link MediaLibrary_MediaContentType} of the requested source.
 * @param movingPhoto it used to obtain {@link OH_MovingPhoto} information when the data is prepared.
 * @since 13
 */
typedef void (*OH_MediaLibrary_OnMovingPhotoDataPrepared)(MediaLibrary_ErrorCode result,
    MediaLibrary_RequestId requestId, MediaLibrary_MediaQuality mediaQuality, MediaLibrary_MediaContentType type,
    OH_MovingPhoto* movingPhoto);

/**
 * @brief Called when a requested quickly image is prepared.
 *
 * This function is called when the requested quickly image is prepared.When there is an image buffer in the system,
 * a picture object will be returned, reducing the decoding time.
 *
 * @param result results {@link MediaLibrary_ErrorCode} of the processing of the requested resources.
 * @param requestId indicates the {@link MediaLibrary_RequestId} of the request.
 * @param mediaQuality the {@link MediaLibrary_MediaQuality} of the requested source.
 * @param type the {@link MediaLibrary_MediaContentType} of the requested source.
 * @param imageSourceNative it used to obtain {@link OH_ImageSourceNative} information when image file is prepared,
 *                          otherwise imageSourceNative is null.
 * @param pictureNative it used to obtain {@link OH_PictureNative} information when image source is prepared,
 *                      otherwise pictureNative is null.
 * @since 23
 */
typedef void (*OH_MediaLibrary_OnQuickImageDataPrepared)(MediaLibrary_ErrorCode result,
    MediaLibrary_RequestId requestId, MediaLibrary_MediaQuality mediaQuality, MediaLibrary_MediaContentType type,
    OH_ImageSourceNative* imageSourceNative, OH_PictureNative* pictureNative);

#ifdef __cplusplus
}
#endif

#endif // MULTIMEDIA_MEDIA_LIBRARY_NATIVE_MEDIA_ASSET_BASE_H
/** @} */
