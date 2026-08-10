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

#define MLOG_TAG "DefaultScanInfo"

#include "default_scan_info.h"

// LCOV_EXCL_START
namespace OHOS {
namespace Media {

const std::string& DefaultScanInfo::GetFilePath() const
{
    return filePath_;
}

void DefaultScanInfo::SetFilePath(const std::string& path)
{
    filePath_ = path;
}

int32_t DefaultScanInfo::GetFileId() const
{
    return fileId_;
}

void DefaultScanInfo::SetFileId(int32_t id)
{
    fileId_ = id;
}

bool DefaultScanInfo::GetIsMovingPhoto() const
{
    return isMovingPhoto_;
}

void DefaultScanInfo::SetIsMovingPhoto(bool isMoving)
{
    isMovingPhoto_ = isMoving;
}

} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
