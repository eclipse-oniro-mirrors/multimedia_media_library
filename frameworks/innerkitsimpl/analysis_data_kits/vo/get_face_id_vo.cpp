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
 
#define MLOG_TAG "MediaGetFaceIdVo"
 
#include "get_face_id_vo.h"
 
#include <sstream>
 
#include "media_log.h"
 
namespace OHOS::Media {
using namespace std;
bool GetFaceIdReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->albumId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->albumSubType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
 
bool GetFaceIdReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->albumId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->albumSubType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool GetFaceIdRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->groupTag);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
 
bool GetFaceIdRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->groupTag);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
} // namespace OHOS::Media