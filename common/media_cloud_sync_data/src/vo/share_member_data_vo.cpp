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

#define MLOG_TAG "Media_Cloud_Vo"

#include "share_member_data_vo.h"

#include <sstream>

#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool ShareMemberDataVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->userId), false, "userId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->status), false, "status");
    return true;
}

bool ShareMemberDataVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->userId), false, "userId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->status), false, "status");
    return true;
}

std::string ShareMemberDataVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"userId\": \"" << userId << "\","
       << "\"status\": \"" << status << "\""
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync
