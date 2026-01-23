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

#include "media_operate_result_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {

bool MediaOperateResultRespBodyResultNode::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadInt32(this->errorCode);
    parcel.ReadString(this->errorMsg);
    return true;
}
bool MediaOperateResultRespBodyResultNode::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteInt32(this->errorCode);
    parcel.WriteString(this->errorMsg);
    return true;
}

std::string MediaOperateResultRespBodyResultNode::ToString() const
{
    std::stringstream ss;
    ss << "{";
    ss << "cloudId:" << this->cloudId << ",";
    ss << "errorCode:" << this->errorCode << ",";
    ss << "errorMsg:" << this->errorMsg;
    ss << "}";
    return ss.str();
}

bool MediaOperateResultRespBody::Unmarshalling(MessageParcel &parcel)
{
    IPC::ITypeMediaUtil::UnmarshallingParcelable<MediaOperateResultRespBodyResultNode>(this->result, parcel);
    return true;
}

bool MediaOperateResultRespBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::MarshallingParcelable<MediaOperateResultRespBodyResultNode>(this->result, parcel);
    return true;
}

int32_t MediaOperateResultRespBody::GetFailSize() const
{
    int32_t failSize = 0;
    for (auto &item : this->result) {
        failSize += item.errorCode != 0 ? 1 : 0;
    }
    return failSize;
}

std::string MediaOperateResultRespBody::ToString() const
{
    std::stringstream ss;
    ss << "[";
    for (uint32_t i = 0; i < result.size(); ++i) {
        ss << this->result[i].ToString();
        if (i != result.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync