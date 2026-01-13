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

#ifndef OHOS_MEDIA_ALBUMS_MANAGER_CHANGE_REQUEST_SET_ORDER_POSITION_VO_H
#define OHOS_MEDIA_ALBUMS_MANAGER_CHANGE_REQUEST_SET_ORDER_POSITION_VO_H

#include <string>
#include <vector>

#include "i_media_parcelable.h"

namespace OHOS::Media {
class ChangeRequestSetOrderPositionReqBody : public IPC::IMediaParcelable {
public:
    int32_t albumId {-1};
    std::string orderString;
    std::vector<std::string> assetIds;

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;

    bool Marshalling(MessageParcel &parcel) const override;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ALBUMS_MANAGER_CHANGE_REQUEST_SET_ORDER_POSITION_VO_H