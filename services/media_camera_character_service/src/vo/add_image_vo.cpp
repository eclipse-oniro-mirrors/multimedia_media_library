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

#define MLOG_TAG "MediaAddImage"
#include "add_image_vo.h"

#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {

bool AddImageReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->deferredProcType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->photoQuality);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->subType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->packageName);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool AddImageReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->deferredProcType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->photoQuality);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->subType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->packageName);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string AddImageReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(this->fileId) << "\","
       << "\"photoId\": \"" << this->photoId << "\","
       << "\"deferredProcType\": \"" << std::to_string(this->deferredProcType) << "\","
       << "\"photoQuality\": \"" << std::to_string(this->photoQuality) << "\","
       << "\"subType\": \"" << std::to_string(this->subType) << "\","
       << "\"packageName\": \"" << this->packageName
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media