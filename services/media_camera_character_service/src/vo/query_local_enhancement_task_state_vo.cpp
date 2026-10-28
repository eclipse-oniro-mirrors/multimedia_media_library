/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaQueryLocalEnhancementTaskStateVo"

#include "query_local_enhancement_task_state_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media {
using namespace std;
bool QueryLocalEnhancementTaskStateReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadString(this->photoUri);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryLocalEnhancementTaskStateReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->photoUri);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

string QueryLocalEnhancementTaskStateReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"photoUri\": \"" << photoUri
       << "}";
    return ss.str();
}

bool QueryLocalEnhancementTaskStateRespBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->progress);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool QueryLocalEnhancementTaskStateRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->progress);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

string QueryLocalEnhancementTaskStateRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
        << "\"progress\": \"" << to_string(this->progress) << "\""
        << "}";
    return ss.str();
}
} // namespace OHOS::Media