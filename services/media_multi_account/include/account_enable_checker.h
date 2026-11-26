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

#ifndef OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_H
#define OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_H

#include <cstdint>

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#ifndef EXPORT
#define EXPORT __attribute__ ((visibility ("default")))
#endif

namespace OHOS {
namespace Media {

class AccountEnableChecker {
public:
    AccountEnableChecker() = delete;

    EXPORT static bool IsMultiAccountEnabled();
};

}  // namespace Media
}  // namespace OHOS

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#endif  // OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_H
