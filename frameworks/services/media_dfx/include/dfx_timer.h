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

namespace OHOS {
namespace Media {
class DfxTimer {
public:
    DfxTimer(int32_t type, int32_t object, int64_t timeOut, bool isReport);
    ~DfxTimer();
    void End();

private:
    int32_t type_;
    int32_t object_;
    int64_t start_;
    int64_t timeCost_;
    int64_t timeOut_;
    bool isReport_;
    bool isEnd_;
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_TIMER_H
