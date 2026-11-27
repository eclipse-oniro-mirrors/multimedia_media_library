/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

// Test stub: shadows the upstream AccountEnableChecker header (delivered by
// the multi-account prerequisite PR, not yet in this repo). The real one
// answers "is this the driver screen (user fixed at 100)"; the stub is
// controllable via AccountDeletionMock::SetMultiAccountEnabled.
#ifndef OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_MOCK_H
#define OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_MOCK_H

#define ACCOUNT_DELETION_STUB_ENABLE_CHECKER 1

namespace OHOS {
namespace Media {
class AccountEnableChecker {
public:
    static bool IsMultiAccountEnabled();
};
}  // namespace Media
}  // namespace OHOS

#endif  // OHOS_MEDIA_ACCOUNT_ENABLE_CHECKER_MOCK_H
