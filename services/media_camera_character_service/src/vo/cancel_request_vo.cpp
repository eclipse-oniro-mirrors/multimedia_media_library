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

#define MLOG_TAG "MediaCancelRequest"
 
#include "cancel_request_vo.h"
 
#include <sstream>
 
#include "media_log.h"

namespace OHOS::Media {
bool CancelRequestReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool CancelRequestReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->mediaType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string CancelRequestReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"photoId\": \"" << this->photoId << "\","
       << "\"mediaType\": \"" << std::to_string(this->mediaType)
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media