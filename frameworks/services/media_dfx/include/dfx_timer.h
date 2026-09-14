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

#ifndef OHOS_MEDIA_DFX_TIMER_H
#define OHOS_MEDIA_DFX_TIMER_H

#include <string>
#include <map>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class DfxTimer {
private:
    int32_t type_;
    int32_t object_;
    std::string detail_;
    int64_t start_;
    int64_t timeCost_ {0};
    int64_t timeOut_;
    bool isReport_;
    bool isEnd_;
    int32_t uid_;
    static const std::map<uint32_t, int64_t> operationCodeTimeoutMap;

public:
    EXPORT DfxTimer(int32_t object, int64_t timeOut, bool isReport);
    EXPORT DfxTimer(int32_t type, int32_t object, int64_t timeOut, bool isReport,
        const std::string &detail = "");
    EXPORT ~DfxTimer();
    void End();
    EXPORT void SetCallerUid(int32_t uid);
    static int64_t GetOperationCodeTimeout(uint32_t operationCode);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_TIMER_H
