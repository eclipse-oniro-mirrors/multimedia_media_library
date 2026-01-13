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
 
#define MLOG_TAG "MediaDeleteHighLightAlbumsVo"
 
#include "delete_highlight_albums_vo.h"
 
#include <sstream>
 
#include "media_itypes_utils.h"
#include "media_log.h"
 
namespace OHOS::Media {
using namespace std;
bool DeleteHighLightAlbumsReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = IPC::ITypeMediaUtil::UnmarshalStrVec(this->albumIds, parcel);
    CHECK_AND_RETURN_RET(status, status);
    IPC::ITypeMediaUtil::Unmarshalling<int32_t>(this->photoAlbumTypes, parcel);
    CHECK_AND_RETURN_RET(status, status);
    IPC::ITypeMediaUtil::Unmarshalling<int32_t>(this->photoAlbumSubtypes, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
 
bool DeleteHighLightAlbumsReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = IPC::ITypeMediaUtil::MarshalStrVec(this->albumIds, parcel);
    CHECK_AND_RETURN_RET(status, status);
    status = IPC::ITypeMediaUtil::Marshalling<int32_t>(this->photoAlbumTypes, parcel);
    CHECK_AND_RETURN_RET(status, status);
    status = IPC::ITypeMediaUtil::Marshalling<int32_t>(this->photoAlbumSubtypes, parcel);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}
} // namespace OHOS::Media