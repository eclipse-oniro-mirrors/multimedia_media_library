/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_HELPER_IMPL_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_HELPER_IMPL_H

#include "datashare_helper.h"
#include "file_asset.h"
#include "nocopyable.h"
#include "media_asset_helper.h"
#include "photo_proxy.h"

namespace OHOS {
namespace Media {

class MediaAssetHelperImpl : public MediaAssetHelper, public NoCopyable {
public:
    MediaAssetHelperImpl();
    ~MediaAssetHelperImpl();

    OH_MediaAsset* GetMediaAsset(std::string uri, int32_t cameraShotType, std::string burstKey) override;

    OH_MediaAsset *GetOhMediaAsset(const std::string &uri) override;

private:
    void InitFileAsset(std::shared_ptr<FileAsset> fileAsset);
    std::shared_ptr<DataShare::DataShareResultSet> QueryFileAsset(int32_t mediaId);
    void UpdateFileAsset(std::shared_ptr<DataShare::DataShareResultSet> resultSet,
        std::shared_ptr<FileAsset> fileAsset);
};

} // Media
} // OHOS
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_HELPER_IMPL_H