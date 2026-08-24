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

#include "validate_latest_pair_vo.h"

#include <sstream>
#include "media_log.h"

namespace OHOS::Media {

bool ValidateLatestPairReqBody::Unmarshalling(MessageParcel &parcel)
{
    this->uniqueId = parcel.ReadString();
    return true;
}

bool ValidateLatestPairReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteString(this->uniqueId);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool ValidateLatestPairRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->assetExists = parcel.ReadBool();
    return true;
}

bool ValidateLatestPairRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteBool(this->assetExists);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string ValidateLatestPairRespBody::ToString() const
{
    std::stringstream ss;
    ss << "assetExists: " << this->assetExists;
    return ss.str();
}
} // namespace OHOS::Media
