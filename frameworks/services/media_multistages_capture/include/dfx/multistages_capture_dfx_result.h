/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_RESULT_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_RESULT_H

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const int32_t MULTISTAGES_CAPTURE_RESULT_ERR_CODE_BASE = 10000;
enum class MultiStagesCaptureResultErrCode : int32_t {
    SUCCESS = MULTISTAGES_CAPTURE_RESULT_ERR_CODE_BASE,
    SAVE_IMAGE_FAIL,
    SQL_ERR,
};

class MultiStagesCaptureDfxResult {
public:
    MultiStagesCaptureDfxResult();
    ~MultiStagesCaptureDfxResult();

    EXPORT static void Report(const std::string &photoId, const int32_t result);
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_RESULT_H