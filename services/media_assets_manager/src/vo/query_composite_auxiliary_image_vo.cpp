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

#include "query_composite_auxiliary_image_vo.h"
#include <sstream>
#include "media_log.h"

namespace OHOS::Media {
bool QueryCompositeAuxiliaryImageReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryCompositeAuxiliaryImageReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryCompositeAuxiliaryImageRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->fd = parcel.ReadFileDescriptor();
    if (this->fd < 0) {
        MEDIA_ERR_LOG("Failed to read file descriptor in QueryCompositeAuxiliaryImageRespBody");
        return false;
    }
    MEDIA_INFO_LOG("QueryCompositeAuxiliaryImageRespBody ReadFromParcel fd: %{public}d", this->fd);
    return true;
}

bool QueryCompositeAuxiliaryImageRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteFileDescriptor(this->fd);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
} // namespace OHOS::Media