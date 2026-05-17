/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "ScannerContextConverter"

#include "scanner_context_converter.h"

#include "media_log.h"

namespace OHOS {
namespace Media {
MediaScannerObj::ScanType ScannerContextConverter::DetermineScanType(const ScanTaskContext& context)
{
    if (context.config.GetIsMovingPhoto()) {
        return MediaScannerObj::ScanType::CAMERA_SHOT_MOVING_PHOTO;
    }

    return MediaScannerObj::ScanType::FILE;
}

std::unique_ptr<MediaScannerObj> ScannerContextConverter::Convert(const ScanTaskContext& context)
{
    MediaScannerObj::ScanType scanType = DetermineScanType(context);

    auto scannerObj = std::make_unique<MediaScannerObj>(
        context.config.GetFilePath(),
        context.config.GetCallback(),
        scanType,
        context.config.GetApiVersion()
    );

    if (context.config.GetFileId() != 0) {
        scannerObj->SetFileId(context.config.GetFileId());
    }

    scannerObj->SetForceScan(context.config.GetForceScan());
    scannerObj->SetIsSkipAlbumUpdate(context.config.GetSkipAlbumUpdate());
    scannerObj->SetCameraShotMovingPhoto(context.config.GetIsMovingPhoto());

    MEDIA_INFO_LOG("ScannerContextConverter::Convert: %{public}s", context.config.ToString().c_str());

    return scannerObj;
}

} // namespace Media
} // namespace OHOS