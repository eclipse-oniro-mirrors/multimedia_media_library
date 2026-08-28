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

#include "delete_share_album_vo.h"
#include "media_log.h"
#include "media_itypes_utils.h"

namespace OHOS::Media {

bool DeleteShareAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Unmarshalling(this->owner, parcel), false, "owner");
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Unmarshalling(this->albumIds, parcel), false, "albumIds");
    return true;
}

bool DeleteShareAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Marshalling(this->owner, parcel), false, "owner");
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::Marshalling(this->albumIds, parcel), false, "albumIds");
    return true;
}
} // namespace OHOS::Media
