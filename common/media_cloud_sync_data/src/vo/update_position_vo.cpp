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

#define MLOG_TAG "Media_Cloud_Vo"

#include "update_position_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool UpdatePositionReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->position);
    parcel.ReadInt32(this->fileSourceType);
    return IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->cloudIds, parcel);
}

bool UpdatePositionReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->position);
    parcel.WriteInt32(this->fileSourceType);
    IPC::ITypeMediaUtil::Marshalling<std::string>(this->cloudIds, parcel);
    return true;
}

std::string UpdatePositionReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{";
    ss << "\"position\": " << this->position << ", ";
    ss << "[";
    for (uint32_t i = 0; i < this->cloudIds.size(); i++) {
        ss << this->cloudIds[i];
        if (i != this->cloudIds.size() - 1) {
            ss << ",";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync