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

#define MLOG_TAG "Media_Cloud_Vo"

#include "query_data_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "cloud_media_sync_const.h"
#include "itypes_util.h"
#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool QueryDataReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = DataShare::DataSharePredicates::Unmarshal(predicates, parcel);

    CHECK_AND_RETURN_RET_LOG(status, false, "predicates Unmarshal failed");

    return ITypesUtil::Unmarshal(parcel, columnNames, tableName);
}

bool QueryDataReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = DataShare::DataSharePredicates::Marshal(predicates, parcel);

    CHECK_AND_RETURN_RET_LOG(status, false, "predicates Marshal failed");

    return ITypesUtil::Marshal(parcel, columnNames, tableName);
}

bool QueryDataRespBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshalMapVec(queryResults, parcel);
}

bool QueryDataRespBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshalMapVec(queryResults, parcel);
}

std::string QueryDataRespBody::ToString() const
{
    std::stringstream ss;
    ss << "QueryResults\":[";
    for (auto &map : queryResults) {
        ss << "{";
        bool isFirstEntry = true;
        for (auto &entry : map) {
            if (!isFirstEntry) {
                ss << ",";
            }
            ss << entry.first << ": " <<entry.second;
            isFirstEntry = false;
        }
        ss << "}";
    }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync