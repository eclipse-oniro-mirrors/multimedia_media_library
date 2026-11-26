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

// Test stub: shadows the real os_account_manager.h via include-path priority
// (mock/include comes first in the test target). Only the surface actually
// used by account_deletion_handle.cpp is declared; ErrCode/ERR_OK come from
// the real account_error_no.h (resolved through os_account external_deps).
#ifndef ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_MANAGER_H
#define ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_MANAGER_H

#include "account_error_no.h"

#define ACCOUNT_DELETION_STUB_OS_ACCOUNT_MANAGER 1

namespace OHOS {
namespace AccountSA {
class OsAccountManager {
public:
    static ErrCode GetForegroundOsAccountLocalId(int32_t &localId);
};
}  // namespace AccountSA
}  // namespace OHOS

#endif  // ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_MANAGER_H
