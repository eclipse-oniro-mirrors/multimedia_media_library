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

#ifndef OHOS_MEDIA_MEDIALIBRARY_ASPECT_RATIO_OPERATION_H
#define OHOS_MEDIA_MEDIALIBRARY_ASPECT_RATIO_OPERATION_H

#include <string>
#include <vector>

#include "medialibrary_rdbstore.h"
#include "file_asset.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct AssetAspectRatio {
    int32_t fileId;
    double aspect_ratio;
};

class MediaLibraryAspectRatioOperation {
public:
    static void Stop();
    static void UpdateAspectRatioValue();
    static int32_t QueryUnfilledValueCount();
    static std::vector<AssetAspectRatio> GetUnfilledValues();
    static void HandleAspectRatio(const std::vector<AssetAspectRatio> &photoInfos);
private:
    static std::atomic<bool> isContinue_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_MEDIALIBRARY_ASPECT_RATIO_OPERATION_H