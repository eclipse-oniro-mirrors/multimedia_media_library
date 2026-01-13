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
#ifndef OHOS_MTP_MANAGER_H
#define OHOS_MTP_MANAGER_H

#include <stdint.h>
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MtpManager {
public:
    EXPORT MtpManager() = default;
    EXPORT virtual ~MtpManager() = default;
    EXPORT static MtpManager &GetInstance();

    enum class MtpMode {
        NONE_MODE,
        MTP_MODE,
        PTP_MODE
    };
    EXPORT bool IsMtpMode() const { return mtpMode_ == MtpMode::MTP_MODE; }
    MtpMode mtpMode_ { MtpMode::NONE_MODE };
};

extern "C" {
    EXPORT void StartMtpService(const uint32_t mode);
    EXPORT void StopMtpService();
}
} // namespace Media
} // namespace OHOS
#endif // OHOS_MTP_MANAGER_H
