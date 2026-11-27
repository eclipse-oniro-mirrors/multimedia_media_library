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

#define MLOG_TAG "ScreenDetector"

#include "screen_detector.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include <cinttypes>
#include <map>
#include <mutex>
#include <unistd.h>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "os_account_manager.h"

namespace OHOS {
namespace Media {
constexpr uint32_t BASE_USER_RANGE = 200000;

static constexpr uint64_t CENTER_ID = 0;
static constexpr uint64_t REAR_ID = 4;
static constexpr uint64_t COPILOT_ID = 6;

// DisplayId -> ScreenType mapping table
static const std::map<uint64_t, ScreenType> DISPLAY_ID_TO_SCREEN_TYPE = {
    {CENTER_ID,     ScreenType::CENTRAL_CONTROL},       // 中控屏
    {REAR_ID,       ScreenType::REAR},                  // 后排屏
    {COPILOT_ID,    ScreenType::CO_DRIVER},             // 副驾屏
};

ScreenDetector& ScreenDetector::GetInstance()
{
    static ScreenDetector instance;
    return instance;
}

void ScreenDetector::Init()
{
    int32_t userId = getuid() / BASE_USER_RANGE;
    uint64_t displayId = QueryDisplayId(userId);
    if (displayId == E_ERR) {
        MEDIA_ERR_LOG("Init: failed to get displayId for userId=%{public}d", userId);
        currentScreenType_.store(ScreenType::UNDEFINED, std::memory_order_relaxed);
        return;
    }

    ScreenType screenType = ChangeDisplayIdToScreenType(userId, displayId);
    currentScreenType_.store(screenType, std::memory_order_relaxed);
    MEDIA_INFO_LOG("initialized: userId=%{public}d, displayId=%{public}" PRIu64 ", currentScreenType=%{public}d",
        userId, displayId, static_cast<int32_t>(screenType));
}

uint64_t ScreenDetector::QueryDisplayId(int32_t userId)
{
    std::vector<AccountSA::ForegroundOsAccount> fgAccounts;
    ErrCode errCode = AccountSA::OsAccountManager::GetForegroundOsAccounts(fgAccounts);
    if (errCode != ERR_OK) {
        MEDIA_ERR_LOG("failed to GetForegroundOsAccounts, err=%{public}d", errCode);
        return E_ERR;
    }

    if (fgAccounts.empty()) {
        MEDIA_ERR_LOG("no foreground os accounts, userId=%{public}d", userId);
        return E_ERR;
    }

    for (const auto& account : fgAccounts) {
        if (account.localId == userId) {
            return account.displayId;
        }
    }
    MEDIA_ERR_LOG("userId=%{public}d not found in fgAccounts, size=%{public}zu", userId, fgAccounts.size());
    return E_ERR;
}

bool ScreenDetector::IsCentralControlScreen() const
{
    return currentScreenType_.load(std::memory_order_relaxed) == ScreenType::CENTRAL_CONTROL;
}

ScreenType ScreenDetector::ChangeDisplayIdToScreenType(int32_t userId, uint64_t displayId)
{
    auto it = DISPLAY_ID_TO_SCREEN_TYPE.find(displayId);
    if (it == DISPLAY_ID_TO_SCREEN_TYPE.end()) {
        MEDIA_ERR_LOG("unknown displayId=%{public}" PRIu64".", displayId);
        return ScreenType::UNDEFINED;
    }
    return it->second;
}

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT