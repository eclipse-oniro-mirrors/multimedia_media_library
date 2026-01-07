/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ACCURATE_DEBUG_LOG_H
#define OHOS_MEDIALIBRARY_ACCURATE_DEBUG_LOG_H

#include "media_log.h"

namespace OHOS {
namespace Media::AccurateRefresh {
const int32_t accurateDebugLevelHigh = 3;
const int32_t accurateDebugLevelMid = 2;

__attribute__ ((visibility ("default"))) extern int32_t accurateDebugLevel;
#define IS_ACCURATE_DEBUG (AccurateRefresh::accurateDebugLevel >= 3)

#define ACCURATE_DEBUG(fmt, ...)            \
    do {                                               \
        if (AccurateRefresh::accurateDebugLevel >= 3) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);         \
        }                                              \
    } while (0)

#define ACCURATE_INFO(fmt, ...)            \
    do {                                               \
        if (AccurateRefresh::accurateDebugLevel >= 2) {                                 \
            MEDIA_INFO_LOG("##: " fmt, ##__VA_ARGS__);         \
        }                                              \
    } while (0)

#define ACCURATE_ERR(fmt, ...) MEDIA_ERR_LOG(fmt, ##__VA_ARGS__)

} // namespace Media
} // namespace OHOS

#endif