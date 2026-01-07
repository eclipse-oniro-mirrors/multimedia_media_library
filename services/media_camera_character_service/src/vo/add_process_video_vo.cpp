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
 
#include "add_process_video_vo.h"
 
#include <sstream>
 
#include "media_log.h"

namespace OHOS::Media {
bool AddProcessVideoReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->photoQuality);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->videoCount);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->VideoEnhancementType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool AddProcessVideoReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->photoQuality);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->videoCount);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->VideoEnhancementType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string AddProcessVideoReqBody::ToString() const
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