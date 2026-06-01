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

#define MLOG_TAG "MediaGetDeferredPictureInfo"

#include "get_deferred_picture_info_vo.h"

#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool GetDeferredPictureInfoReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetDeferredPictureInfoReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->photoId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string GetDeferredPictureInfoReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"photoId\": \"" << this->photoId
       << "}";
    return ss.str();
}

bool GetDeferredPictureInfoRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->editData);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->mimeType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->orientation);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetDeferredPictureInfoRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->editData);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->mimeType);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->orientation);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string GetDeferredPictureInfoRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"editData\": \"" << this->editData << "\","
       << "\"mimeType\": \"" << this->mimeType << "\","
       << "\"orientation\": \"" << std::to_string(this->orientation)
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media