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

#include "query_local_enhancement_status_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media {
bool QueryLocalEnhancementStatusReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryLocalEnhancementStatusReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryLocalEnhancementStatusRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->localEnhancementAbility);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->localEnhancementStatus);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryLocalEnhancementStatusRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->localEnhancementAbility);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->localEnhancementStatus);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string QueryLocalEnhancementStatusRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"localEnhancementAbility\": \"" << std::to_string(this->localEnhancementAbility) << "\","
       << "\"localEnhancementStatus\": \"" << std::to_string(this->localEnhancementStatus) << "\","
       << "\"photoId\": \"" << this->photoId
       << "}";
    return ss.str();
}
} // namespace OHOS::Media