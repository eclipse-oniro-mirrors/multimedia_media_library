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

#define MLOG_TAG "MultiStagesCapture"

#include "high_quality_scan_file_callback.h"

#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS::Media {
std::shared_ptr<IMediaScannerCallback> HighQualityScanFileCallback::Create(const int32_t fileId)
{
    std::shared_ptr<IMediaScannerCallback> callback = std::make_shared<HighQualityScanFileCallback>(fileId);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, nullptr, "new callback failed");
    return callback;
}

int32_t HighQualityScanFileCallback::OnScanFinished(
    const int32_t status, const std::string &uri, const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(this->fileId_ > 0, E_INVAL_ARG, "HighQualityScanFileCallback, image is empty");
    return this->multiStagesCaptureDao_.UpdatePhotoDirtyNew(this->fileId_);
}
}  // namespace OHOS::Media