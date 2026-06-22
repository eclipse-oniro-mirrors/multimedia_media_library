/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaGetDeferredPictureInfo"

#include "get_deferred_picture_info_dto.h"

#include <sstream>

#include "media_log.h"

namespace OHOS::Media {
GetDeferredPictureInfoDto GetDeferredPictureInfoDto::Create(const GetDeferredPictureInfoReqBody &req)
{
    MEDIA_INFO_LOG("IPC::Camera GetDeferredPictureInfo_vo: %{public}s.", req.ToString().c_str());
    GetDeferredPictureInfoDto dto;
    dto.photoId = req.photoId;
    MEDIA_INFO_LOG("IPC::Camera GetDeferredPictureInfo_dto: %{public}s.", dto.ToString().c_str());
    return dto;
}

std::string GetDeferredPictureInfoDto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"photoId\": \"" << this->photoId << "\","
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media
