/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaOpenAssetCompressVo"

#include "open_asset_compress_vo.h"

#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool OpenAssetCompressReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->uri);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->version);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->type);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool OpenAssetCompressReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->uri);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->version);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->type);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

void OpenAssetCompressReqBody::Convert2Dto(OpenAssetCompressDto &dto)
{
    dto.uri = this->uri;
    dto.version = this->version;
    dto.type = this->type;
    return;
}

bool OpenAssetCompressRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->fileDescriptor = parcel.ReadFileDescriptor();
    CHECK_AND_RETURN_RET_LOG(this->fileDescriptor >= 0, false, "Unmarshalling fd is invalid");
    return true;
}

bool OpenAssetCompressRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteFileDescriptor(this->fileDescriptor);
    close(this->fileDescriptor);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
} // namespace OHOS::Media