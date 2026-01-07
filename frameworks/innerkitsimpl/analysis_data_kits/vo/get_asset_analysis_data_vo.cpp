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

#include "get_asset_analysis_data_vo.h"

#include "media_log.h"

namespace OHOS::Media {

bool GetAssetAnalysisDataReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET_LOG(status, status, "Read fileId failed");
    status = parcel.ReadInt32(this->analysisType);
    CHECK_AND_RETURN_RET_LOG(status, status, "Read analysisType failed");
    status = parcel.ReadString(this->language);
    CHECK_AND_RETURN_RET_LOG(status, status, "Read language failed");
    return parcel.ReadBool(this->analysisTotal);
}

bool GetAssetAnalysisDataReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET_LOG(status, status, "Write fileId failed");
    status = parcel.WriteInt32(this->analysisType);
    CHECK_AND_RETURN_RET_LOG(status, status, "Write analysisType failed");
    status = parcel.WriteString(this->language);
    CHECK_AND_RETURN_RET_LOG(status, status, "Write language failed");
    return parcel.WriteBool(this->analysisTotal);
}

bool GetAssetAnalysisDataRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->resultSet = DataShare::DataShareResultSet::Unmarshal(parcel);
    CHECK_AND_RETURN_RET_LOG(this->resultSet != nullptr, false, "resultSet nullptr");
    return true;
}

bool GetAssetAnalysisDataRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(this->resultSet != nullptr, false, "resultSet nullptr");
    return DataShare::DataShareResultSet::Marshal(this->resultSet, parcel);
}
} // namespace OHOS::Media