/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_GET_ASSETS_ANALYSIS_METADATA_VO_H
#define OHOS_GET_ASSETS_ANALYSIS_METADATA_VO_H

#include <cstdint>
#include <string>
#include <vector>

#include "i_media_parcelable.h"

namespace OHOS::Media {
class GetAssetsAnalysisMetadataReqBody : public IPC::IMediaParcelable {
public:
    std::vector<int32_t> fileIds;

    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};

class GetAssetsAnalysisMetadataRespBody : public IPC::IMediaParcelable {
public:
    std::vector<int32_t> fileIds;
    std::vector<std::string> values;

    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

    bool IsValid() const
    {
        return isValid_;
    }

private:
    bool isValid_ = false;
};
} // namespace OHOS::Media
#endif // OHOS_GET_ASSETS_ANALYSIS_METADATA_VO_H
