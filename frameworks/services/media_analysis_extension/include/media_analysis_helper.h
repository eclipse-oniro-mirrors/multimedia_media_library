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

#ifndef OHOS_MEDIA_ANALYSIS_HELPER_H
#define OHOS_MEDIA_ANALYSIS_HELPER_H

#include <vector>

#include "iservice_registry.h"
#include "media_analysis_proxy.h"

namespace OHOS {
namespace Media {
class MediaAnalysisHelper {
public:
    static void StartMediaAnalysisServiceAsync(int32_t code, const std::vector<std::string> &uris = {});
    static void StartMediaAnalysisServiceSync(int32_t code, const std::vector<std::string> &fileIds = {});

private:
    static void StartMediaAnalysisServiceInternal(int32_t code, MessageOption option,
        std::vector<std::string> fileIds = {});
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_ANALYSIS_HELPER_H