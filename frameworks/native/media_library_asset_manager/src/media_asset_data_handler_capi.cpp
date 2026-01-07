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

#include "media_asset_data_handler_capi.h"

#include "cstring"
#include "medialibrary_client_errno.h"

namespace OHOS {
namespace Media {
static const std::string MEDIA_ASSET_DATA_HANDLER_CLASS = "MediaAssetDataHandler";

CapiMediaAssetDataHandler::CapiMediaAssetDataHandler(NativeOnDataPrepared dataHandler, ReturnDataType dataType,
    const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode)
{
    onDataPreparedHandler_ = dataHandler;
    dataType_ = dataType;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
}

CapiMediaAssetDataHandler::CapiMediaAssetDataHandler(OH_MediaLibrary_OnImageDataPrepared imageDataHandler,
    ReturnDataType dataType, const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode)
{
    onRequestImageDataPreparedHandler_ = imageDataHandler;
    dataType_ = dataType;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
}

CapiMediaAssetDataHandler::CapiMediaAssetDataHandler(OH_MediaLibrary_OnMovingPhotoDataPrepared photoDataHandler,
    ReturnDataType dataType, const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode)
{
    onRequestMovingPhotoDataPreparedHandler_ = photoDataHandler;
    dataType_ = dataType;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
}

CapiMediaAssetDataHandler::CapiMediaAssetDataHandler(OH_MediaLibrary_OnQuickImageDataPrepared photoDataHandler,
    ReturnDataType dataType, const std::string &uri, const std::string &destUri, NativeSourceMode sourceMode)
{
    onRequestQuickImageDataPreparedHandler_ = photoDataHandler;
    dataType_ = dataType;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
}

int32_t CapiMediaAssetDataHandler::GetPhotoQuality()
{
    return photoQuality_;
}

void CapiMediaAssetDataHandler::SetPhotoQuality(int32_t photoQuality)
{
    photoQuality_ = photoQuality;
}

ReturnDataType CapiMediaAssetDataHandler::GetReturnDataType()
{
    return dataType_;
}

std::string CapiMediaAssetDataHandler::GetRequestUri()
{
    return requestUri_;
}

std::string CapiMediaAssetDataHandler::GetDestUri()
{
    return destUri_;
}

NativeSourceMode CapiMediaAssetDataHandler::GetSourceMode()
{
    return sourceMode_;
}

void CapiMediaAssetDataHandler::SetNotifyMode(NativeNotifyMode notifyMode)
{
    notifyMode_ = notifyMode;
}

NativeNotifyMode CapiMediaAssetDataHandler::GetNotifyMode()
{
    return notifyMode_;
}
} // namespace Media
} // namespace OHOS
