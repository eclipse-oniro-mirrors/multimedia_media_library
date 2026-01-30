/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#ifndef OHOS_MEDIA_ASSETS_MANAGER_GET_BATCH_DOWNLOAD_CLOUD_RESOURCES_SIZE_VO_H
#define OHOS_MEDIA_ASSETS_MANAGER_GET_BATCH_DOWNLOAD_CLOUD_RESOURCES_SIZE_VO_H

#include <string>
#include <vector>
#include "i_media_parcelable.h"
#include "datashare_predicates.h"

namespace OHOS::Media {
using namespace OHOS::DataShare;
#define EXPORT __attribute__ ((visibility ("default")))
class GetBatchDownloadCloudResourcesSizeReqBody : public IPC::IMediaParcelable {
public:
    DataSharePredicates predicates;
    // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
public:  // basic functions
    std::string ToString() const;
};

class GetBatchDownloadCloudResourcesSizeRespBody : public IPC::IMediaParcelable {
public:
    int64_t size = -1;
    int64_t count = -1;

    // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_ASSETS_MANAGER_GET_BATCH_DOWNLOAD_CLOUD_RESOURCES_SIZE_VO_H