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

#ifndef OHOS_CLOUD_MEDIA_ASSET_OBSERVER_H
#define OHOS_CLOUD_MEDIA_ASSET_OBSERVER_H

#include "datashare_helper.h"
#include "datashare_observer.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class CloudMediaAssetDownloadOperation;

class CloudMediaAssetObserver : public DataShare::DataShareObserver {
public:
    CloudMediaAssetObserver(std::shared_ptr<CloudMediaAssetDownloadOperation> operation) : operation_(operation) {}
    ~CloudMediaAssetObserver() {}
    void OnChange(const ChangeInfo &changeInfo) override;

private:
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_CLOUD_MEDIA_ASSET_OBSERVER_H