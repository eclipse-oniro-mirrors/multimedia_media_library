/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){return 0;}
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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_UPDATE_ASSETS_VO_H
#define OHOS_MEDIA_ASSETS_MANAGER_UPDATE_ASSETS_VO_H

#include <stdint.h>
#include <string>
#include <vector>

#include "i_media_parcelable.h"

namespace OHOS::Media {

class ModifyAssetsReqBody : public IPC::IMediaParcelable {
public:
    int32_t pending{-1};
    int32_t favorite{-1};
    int32_t hiddenStatus{-1};
    int32_t recentShowStatus{-1};
    std::string title;
    std::string userComment;
    std::vector<int32_t> fileIds;

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};

} // namespace OHOS::Media
#endif // OHOS_MEDIA_ASSETS_MANAGER_UPDATE_ASSETS_VO_H