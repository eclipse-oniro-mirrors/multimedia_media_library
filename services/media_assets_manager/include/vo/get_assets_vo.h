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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_GET_ASSETS_VO_H
#define OHOS_MEDIA_ASSETS_MANAGER_GET_ASSETS_VO_H

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "i_media_parcelable.h"

namespace OHOS::Media {
class GetAssetsReqBody : public IPC::IMediaParcelable {
public:
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> columns;
    std::string burstKey;
    bool uriOnly = false;

    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};

class GetAssetsRespBody : public IPC::IMediaParcelable {
public:
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;

    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_GET_ASSETS_VO_H