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

#ifndef MEDIA_ANALYSIS_DATA_KITS_START_ASSET_ANALYSIS_VO_H
#define MEDIA_ANALYSIS_DATA_KITS_START_ASSET_ANALYSIS_VO_H

#include <string>
#include <vector>
#include "i_media_parcelable.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "data_ability_predicates.h"

namespace OHOS::Media {
class StartAssetAnalysisReqBody : public IPC::IMediaParcelable {
public:
    std::string uri;
    DataShare::DataSharePredicates predicates;

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class StartAssetAnalysisRespBody : public IPC::IMediaParcelable {
public:
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
} // namespace OHOS::Media
#endif // MEDIA_ANALYSIS_DATA_KITS_START_ASSET_ANALYSIS_VO_H