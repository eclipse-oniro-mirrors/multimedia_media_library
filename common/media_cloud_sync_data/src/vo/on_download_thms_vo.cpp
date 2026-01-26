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

#include "on_download_thms_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool OnDownloadThmsReqBody::DownloadThmsData::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadInt32(this->thumbStatus);
    return true;
}

bool OnDownloadThmsReqBody::DownloadThmsData::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteInt32(this->thumbStatus);
    return true;
}

std::string OnDownloadThmsReqBody::DownloadThmsData::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << this->cloudId << "\", "
       << "\"thumbStatus\": " << this->thumbStatus << "\""
       << "}";
    return ss.str();
}

bool OnDownloadThmsReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<DownloadThmsData>(this->downloadThmsDataList, parcel);
}

bool OnDownloadThmsReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<DownloadThmsData>(this->downloadThmsDataList, parcel);
}

std::string OnDownloadThmsReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"DownloadThmsData\": [";
    for (size_t i = 0; i < this->downloadThmsDataList.size(); i++) {
        ss << this->downloadThmsDataList[i].ToString();
        if (i != this->downloadThmsDataList.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync