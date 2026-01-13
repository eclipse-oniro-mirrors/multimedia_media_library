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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_CANCEL_REQUEST_DTO_H
#define OHOS_MEDIA_ASSETS_MANAGER_CANCEL_REQUEST_DTO_H

#include "cancel_request_vo.h"

namespace OHOS::Media {
class CancelRequestDto {
public:
    std::string photoId;
    int32_t mediaType;

public:
    static CancelRequestDto Create(const CancelRequestReqBody &req);
    std::string ToString() const;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_ASSET_CHANGE_CREATE_ASSET_DTO_H