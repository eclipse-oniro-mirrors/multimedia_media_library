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

#define MLOG_TAG "AccountEnableChecker"

#include "account_enable_checker.h"

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include "media_log.h"
#include "screen_detector.h"

namespace OHOS {
namespace Media {

bool AccountEnableChecker::IsMultiAccountEnabled()
{
    bool isCentralControl = ScreenDetector::GetInstance().IsCentralControlScreen();
    MEDIA_INFO_LOG("isCentralControl=%{public}s", isCentralControl ? "true" : "false");
    return isCentralControl;
}

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
