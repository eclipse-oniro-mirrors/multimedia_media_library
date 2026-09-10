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

#include "share_album_detail_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool ShareAlbumDetailVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::UnmarshallingParcelable<ShareMemberDataVo>(this->shareMemberData, parcel),
        false, "shareMemberData");
    return true;
}

bool ShareAlbumDetailVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(
        IPC::ITypeMediaUtil::MarshallingParcelable<ShareMemberDataVo>(this->shareMemberData, parcel),
        false, "shareMemberData");
    return true;
}

std::string ShareAlbumDetailVo::ToString() const
{
    std::stringstream ss;
    ss << "\"shareMemberData\": [";
        for (size_t i = 0; i < shareMemberData.size(); i++) {
            ss << shareMemberData[i].ToString();
            if (i != shareMemberData.size() - 1) {
                ss << ", ";
            }
        }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync
