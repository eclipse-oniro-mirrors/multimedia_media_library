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
#define EXPORT __attribute__ ((visibility ("default")))
class MediaAnalysisHelper {
public:
    EXPORT static void StartMediaAnalysisServiceAsync(int32_t code, const std::vector<std::string> &uris = {});
    EXPORT static void StartMediaAnalysisServiceSync(int32_t code, const std::vector<std::string> &fileIds = {});
    EXPORT static void AsyncStartMediaAnalysisService(int32_t code, const std::vector<std::string> &albumIds = {});
    static void StartPortraitCoverSelectionAsync(const std::string albumId);
    static bool ParseGeoInfo(const std::vector<std::string> geoInfo, const bool isForceQuery);
    static void StartForegroundAnalysisServiceSync(int32_t code, const std::vector<std::string> &fileIds,
        int32_t taskId);
    static void PortraitDisplayGraphChange(int32_t code, const std::vector<std::string> &albumId);

private:
    static void StartMediaAnalysisServiceInternal(int32_t code, MessageOption option,
        std::vector<std::string> fileIds = {});
    static void AnalysePortraitCover(const std::string albumId);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_ANALYSIS_HELPER_H