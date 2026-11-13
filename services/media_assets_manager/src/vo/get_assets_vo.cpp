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

#include "get_assets_vo.h"

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {

bool GetAssetsReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = DataShare::DataSharePredicates::Unmarshal(predicates, parcel);

    CHECK_AND_RETURN_RET_LOG(status, false, "predicates Unmarshal failed");

    return ITypesUtil::Unmarshal(parcel, columns, burstKey, uriOnly);
}

bool GetAssetsReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = DataShare::DataSharePredicates::Marshal(predicates, parcel);

    CHECK_AND_RETURN_RET_LOG(status, false, "predicates Marshal failed");

    return ITypesUtil::Marshal(parcel, columns, burstKey, uriOnly);
}

bool GetAssetsRespBody::Unmarshalling(MessageParcel &parcel)
{
    resultSet = DataShare::DataShareResultSet::Unmarshal(parcel);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet Unmarshal failed");
    return true;
}

bool GetAssetsRespBody::Marshalling(MessageParcel &parcel) const
{
    bool status = resultSet == nullptr || !DataShare::DataShareResultSet::Marshal(resultSet, parcel);
    CHECK_AND_RETURN_RET_LOG(!status, false, "resultSet Marshal failed");
    return true;
}
}  // namespace OHOS::Media