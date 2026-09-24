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

#include "mock_medialibrary_subscriber.h"

#include "medialibrary_subscriber.h"
#include "mock_medialibrary_subscriber.h"

namespace {
int32_t g_trueLimit = -1;  // -1 = always true
int32_t g_callCount = 0;
}

namespace {
int32_t g_trueLimit = -1;  // -1 = always true
int32_t g_callCount = 0;
}

namespace OHOS {
namespace Media {
namespace test {
void SetSubscriberTrueLimit(int32_t limit)
{
    g_trueLimit = limit;
    g_callCount = 0;
}

void ResetSubscriberMock()
{
    g_trueLimit = -1;
    g_callCount = 0;
}
}  // namespace test

bool MedialibrarySubscriber::IsCurrentStatusOn()
{
    if (g_trueLimit < 0) {
        return true;
    }
    return g_callCount++ < g_trueLimit;
}
}  // namespace Media
}  // namespace OHOS