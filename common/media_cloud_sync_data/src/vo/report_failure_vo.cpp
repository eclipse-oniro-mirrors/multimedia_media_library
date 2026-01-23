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

#include "report_failure_vo.h"

#include <sstream>

namespace OHOS::Media::CloudSync {
ReportFailureReqBody &ReportFailureReqBody::SetApiCode(int32_t apiCode)
{
    this->apiCode = apiCode;
    return *this;
}
ReportFailureReqBody &ReportFailureReqBody::SetErrorCode(int32_t errorCode)
{
    this->errorCode = errorCode;
    return *this;
}
ReportFailureReqBody &ReportFailureReqBody::SetFileId(int32_t fileId)
{
    this->fileId = fileId;
    return *this;
}
ReportFailureReqBody &ReportFailureReqBody::SetCloudId(const std::string &cloudId)
{
    this->cloudId = cloudId;
    return *this;
}
bool ReportFailureReqBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->apiCode);
    parcel.ReadInt32(this->errorCode);
    parcel.ReadInt32(this->fileId);
    return parcel.ReadString(this->cloudId);
}

bool ReportFailureReqBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->apiCode);
    parcel.WriteInt32(this->errorCode);
    parcel.WriteInt32(this->fileId);
    return parcel.WriteString(this->cloudId);
}

std::string ReportFailureReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{";
    ss << "\"apiCode\":" << this->apiCode << ", "
       << "\"errorCode\":" << this->errorCode << ", "
       << "\"fileId\":" << this->fileId << ", "
       << "\"cloudId\":\"" << this->cloudId << "\"";
    ss << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync