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

#include "start_asset_analysis_vo.h"

#include <sstream>
#include "media_log.h"
#include "itypes_util.h"

namespace OHOS::Media {
// LCOV_EXCL_START
bool StartAssetAnalysisReqBody::Unmarshalling(MessageParcel &parcel)
{
    if (!DataShare::DataSharePredicates::Unmarshal(this->predicates, parcel)) {
        return false;
    }
    return parcel.ReadString(this->uri);
}

bool StartAssetAnalysisReqBody::Marshalling(MessageParcel &parcel) const
{
    if (!DataShare::DataSharePredicates::Marshal(this->predicates, parcel)) {
        return false;
    }
    return parcel.WriteString(this->uri);
}


std::string StartAssetAnalysisReqBody::ToString() const
{
    std::stringstream ss;
    // todo: add the content of GetAssetsReqBody
    return ss.str();
}

bool StartAssetAnalysisRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->resultSet = DataShare::DataShareResultSet::Unmarshal(parcel);
    if (this->resultSet == nullptr) {
        MEDIA_ERR_LOG("StartAssetAnalysisRespBody ReadFromParcel failed");
        return false;
    }
    MEDIA_INFO_LOG("StartAssetAnalysisRespBody ReadFromParcel success");
    return true;
}

bool StartAssetAnalysisRespBody::Marshalling(MessageParcel &parcel) const
{
    if (this->resultSet == nullptr || !DataShare::DataShareResultSet::Marshal(this->resultSet, parcel)) {
        MEDIA_ERR_LOG("StartAssetAnalysisRespBody Marshalling failed");
        return false;
    }
    MEDIA_INFO_LOG("StartAssetAnalysisRespBody Marshalling success");
    return true;
}

std::string StartAssetAnalysisRespBody::ToString() const
{
    std::stringstream ss;
    // todo: add the content of GetAssetsRespBody
    return ss.str();
}
// LCOV_EXCL_STOP
};