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
 
#include "sca_detail_vo.h"
 
#include <sstream>
 
#include "media_itypes_utils.h"
#include "media_log.h"
 
namespace OHOS::Media::CloudSync {
bool ScaDetailVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->usage), false, "usage");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->riskResult), false, "riskResult");
    return true;
}
 
bool ScaDetailVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->usage), false, "usage");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->riskResult), false, "riskResult");
    return true;
}
 
std::string ScaDetailVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"usage\": \"" << usage << "\","
       << "\"riskResult\": \"" << riskResult << "\""
       << "}";
    return ss.str();
}
 
bool SharePhotoDetailVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->currentUserId), false, "currentUserId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->mediaCreateId), false, "mediaCreateId");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::UnmarshallingParcelable<ScaDetailVo>(this->scaDetailList, parcel), false, "scaDetailList");
    return true;
}
 
bool SharePhotoDetailVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->currentUserId), false, "currentUserId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->mediaCreateId), false, "mediaCreateId");
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::MarshallingParcelable<ScaDetailVo>(this->scaDetailList, parcel), false, "scaDetailList");
    return true;
}
 
std::string SharePhotoDetailVo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"currentUserId\": \"" << currentUserId << "\","
       << "\"mediaCreateId\": \"" << mediaCreateId << "\","
       << "\"scaDetailList\": [";
    for (size_t i = 0; i < scaDetailList.size(); i++) {
        ss << scaDetailList[i].ToString();
        if (i != scaDetailList.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync