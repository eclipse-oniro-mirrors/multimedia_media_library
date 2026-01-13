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

#define MLOG_TAG "MediaProcessVideo"

#include "process_video_vo.h"

#include <sstream>
 
#include "media_log.h"

namespace OHOS::Media {
bool ProcessVideoReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->deliveryMode);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->requestId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool ProcessVideoReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->deliveryMode);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->requestId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string ProcessVideoReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(this->fileId) << "\","
       << "\"deliveryMode\": \"" << std::to_string(this->deliveryMode) << "\","
       << "\"photoId\": \"" << this->photoId << "\","
       << "\"requestId\": \"" << this->requestId << "\","
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media