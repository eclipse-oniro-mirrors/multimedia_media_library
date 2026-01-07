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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_DATA_HANDLER_CAPI_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_DATA_HANDLER_CAPI_H

#include "stdint.h"
#include <string>

#include "media_asset_base_capi.h"

namespace OHOS {
namespace Media {

static const int32_t UUID_STR_LENGTH = 37;

/**
 * @brief Request Id
 *
 * This type is returned when requesting a media library resource.
 * The request id is used to cancel the request.
 * The value is all zero like "00000000-0000-0000-0000-000000000000" if the request fails.
 * @since 12
 */
typedef struct Native_RequestId {
    char requestId[UUID_STR_LENGTH];
} Native_RequestId;
/**
 * @brief Delivery Mode
 *
 * @since 12
 */
typedef enum NativeDeliveryMode {
    /*delivery fast mode*/
    FAST_MODE = 0,
    /*delivery high quality mode*/
    HIGH_QUALITY_MODE = 1,
    /*delivery balanced mode*/
    BALANCED_MODE = 2
} DeliveryMode;

/**
 * @brief Source Mode
 *
 * @since 12
 */
typedef enum NativeSourceMode {
    /*media source original mode*/
    ORIGINAL_MODE = 0,
    /*media source edited mode*/
    EDITED_MODE = 1
} NativeSourceMode;

/**
 * @brief Compatible Mode
 *
 * @since 12
 */
typedef enum NativeCompatibleMode {
    /*compatible media source mode*/
    ORIGINAL_FORMAT_MODE = 0,
    /*original media source mode*/
    COMPATIBLE_FORMAT_MODE = 1
} NativeCompatibleMode;

/**
 * @brief Notify Mode
 *
 * @since 12
 */
typedef enum NativeNotifyMode : int32_t {
    FAST_NOTIFY = 0,
    WAIT_FOR_HIGH_QUALITY = 1
} NativeNotifyMode;

/**
 * @brief Called when a requested source is prepared.
 *
 * @param result Results of the processing of the requested resources.
 * @param requestId Request ID.
 * @since 12
 */
typedef void (*NativeOnDataPrepared)(int32_t result, Native_RequestId requestId);

/**
 * @brief Callback when stage progress is reached.
 *
 * @param progress Rate of progress completion.
 * @since 12
 */
typedef void (*NativeOnProgressHandler)(uint32_t progress);

/**
 * @brief Request Options
 *
 * @since 12
 */
typedef struct NativeRequestOptions {
    NativeDeliveryMode deliveryMode;
    NativeSourceMode sourceMode;
    NativeCompatibleMode compatibleMode;
    NativeOnProgressHandler handler;
} NativeRequestOptions;

enum class ReturnDataType {
    TYPE_IMAGE_SOURCE = 0,
    TYPE_ARRAY_BUFFER,
    TYPE_MOVING_PHOTO,
    TYPE_TARGET_FILE,
    TYPE_PICTURE,
};

class CapiMediaAssetDataHandler {
public:
    CapiMediaAssetDataHandler(NativeOnDataPrepared dataHandler, ReturnDataType dataType, const std::string &uri,
        const std::string &destUri, NativeSourceMode sourceMode);
    CapiMediaAssetDataHandler(OH_MediaLibrary_OnImageDataPrepared imageDataHandler, ReturnDataType dataType,
        const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode);
    CapiMediaAssetDataHandler(OH_MediaLibrary_OnMovingPhotoDataPrepared photoDataHandler, ReturnDataType dataType,
        const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode);
    CapiMediaAssetDataHandler(OH_MediaLibrary_OnQuickImageDataPrepared photoDataHandler, ReturnDataType dataType,
        const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode);
    ~CapiMediaAssetDataHandler() = default;
    ReturnDataType GetReturnDataType();
    std::string GetRequestUri();
    std::string GetDestUri();
    NativeSourceMode GetSourceMode();
    void SetNotifyMode(NativeNotifyMode trigger);
    NativeNotifyMode GetNotifyMode();
    NativeOnDataPrepared onDataPreparedHandler_ = nullptr;
    OH_MediaLibrary_OnImageDataPrepared onRequestImageDataPreparedHandler_ = nullptr;
    OH_MediaLibrary_OnMovingPhotoDataPrepared onRequestMovingPhotoDataPreparedHandler_ = nullptr;
    OH_MediaLibrary_OnQuickImageDataPrepared onRequestQuickImageDataPreparedHandler_ = nullptr;
    int32_t GetPhotoQuality();
    void SetPhotoQuality(int32_t photoQuality);

private:
    ReturnDataType dataType_;
    std::string requestUri_;
    std::string destUri_;
    NativeSourceMode sourceMode_;
    NativeNotifyMode notifyMode_ = NativeNotifyMode::FAST_NOTIFY;
    int32_t photoQuality_ = 0;
};
} // Media
} // OHOS
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_DATA_HANDLER_CAPI_H
