/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_SCREEN_DETECTOR_H
#define OHOS_MEDIA_SCREEN_DETECTOR_H

#include <atomic>
#include <cstdint>
#include <vector>

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "os_account_info.h"

namespace OHOS {
namespace Media {

enum class ScreenType : int32_t {
    UNDEFINED = -1,
    CENTRAL_CONTROL = 0,        // 中控屏
    CO_DRIVER = 6,              // 副驾屏
    REAR = 4,                   // 后排屏（光场屏、吸顶屏、投影光机）
};

class ScreenDetector {
public:
    /**
     * @brief Get the singleton instance
     */
    static ScreenDetector& GetInstance();

    /**
     * @brief Initialize screen detector, detect current screen type
     */
    void Init();

    /**
     * @brief Check if current screen is central control screen
     */
    bool IsCentralControlScreen() const;

private:
    ScreenDetector() = default;
    ~ScreenDetector() = default;
    ScreenDetector(const ScreenDetector&) = delete;
    ScreenDetector& operator=(const ScreenDetector&) = delete;
    ScreenDetector(ScreenDetector&&) = delete;
    ScreenDetector& operator=(ScreenDetector&&) = delete;

    /**
     * @brief Convert display ID to screen type
     */
    ScreenType ChangeDisplayIdToScreenType(int32_t userId, uint64_t displayId);

    /**
     * @brief Query display ID for the specified user
     */
    uint64_t QueryDisplayId(int32_t userId);

    std::atomic<ScreenType> currentScreenType_{ScreenType::UNDEFINED};
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_SCREEN_DETECTOR_H
