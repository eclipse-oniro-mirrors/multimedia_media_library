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

#include "clean_attachment_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool CleanAttachmentReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->cloudIdList, parcel);
}

bool CleanAttachmentReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::Marshalling<std::string>(this->cloudIdList, parcel);
}

bool CleanAttachmentRespBody::Unmarshalling(MessageParcel &parcel)
{
    return parcel.ReadInt64(this->attachmentSize);
}

bool CleanAttachmentRespBody::Marshalling(MessageParcel &parcel) const
{
    return parcel.WriteInt64(this->attachmentSize);
}

std::string CleanAttachmentReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudIdList\": [";
    for (size_t i = 0; i < cloudIdList.size(); i++) {
        ss << cloudIdList[i];
        if (i != cloudIdList.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}
} // namespace OHOS::Media::CloudSync