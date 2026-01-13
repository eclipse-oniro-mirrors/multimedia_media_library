/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaAddProcessVideo"

#include "add_process_video_dto.h"

#include <sstream>

#include "media_log.h"

namespace OHOS::Media {
AddProcessVideoDto AddProcessVideoDto::Create(const AddProcessVideoReqBody &req)
{
    MEDIA_INFO_LOG("IPC::Camera addprocessvideo_vo: %{public}s.", req.ToString().c_str());
    AddProcessVideoDto dto;
    dto.fileId = req.fileId;
    dto.photoId = req.photoId;
    dto.photoQuality = req.photoQuality;
    dto.videoCount = req.videoCount;
    dto.VideoEnhancementType = req.VideoEnhancementType;
    MEDIA_INFO_LOG("IPC::Camera addprocessvideo_dto: %{public}s.", dto.ToString().c_str());
    return dto;
}

std::string AddProcessVideoDto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(this->fileId) << "\","
       << "\"photoId\": \"" << this->photoId << "\","
       << "\"photoQuality\": \"" << std::to_string(this->photoQuality) << "\","
       << "\"videoCount\": \"" << std::to_string(this->videoCount) << "\","
       << "\"VideoEnhancementType\": \"" << std::to_string(this->VideoEnhancementType)
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media