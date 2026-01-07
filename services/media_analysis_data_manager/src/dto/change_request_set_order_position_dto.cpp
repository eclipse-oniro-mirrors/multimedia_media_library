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

#define MLOG_TAG "MediaChangeRequestSetOrderPositionDto"

#include "change_request_set_order_position_dto.h"

namespace OHOS::Media {
using namespace std;
void ChangeRequestSetOrderPositionDto::FromVo(const ChangeRequestSetOrderPositionReqBody &reqBody)
{
    this->albumId = reqBody.albumId;
    this->orderString = reqBody.orderString;
    this->assetIds = reqBody.assetIds;
}
} // namespace OHOS::Media