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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_CAPTURE_FAULT_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_CAPTURE_FAULT_H

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class CaptureFaultType : int32_t {
    NO_SAVE_CMD,
    ASSET_FILE_CHECK_ERROR,
    UPDATE_DB_TIMEOUT,
};

class MultiStagesCaptureDfxCaptureFault {
public:
    EXPORT static void Report(const std::string &photoId, const int32_t mediaType,
        const CaptureFaultType faultType, const std::string &faultReason);
private:
    MultiStagesCaptureDfxCaptureFault();
    ~MultiStagesCaptureDfxCaptureFault();
};
}
}
#endif