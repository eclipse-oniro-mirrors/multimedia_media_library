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

// Test stub: shadows the real os_account_subprofile_client.h via include-path
// priority. SubProfileEventData / callback base / event enum come from the
// REAL os_account_sub_profile_subscribe_callback.h (not shadowed).
#ifndef ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_SUBPROFILE_CLIENT_H
#define ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_SUBPROFILE_CLIENT_H

#include <memory>
#include <set>
#include <vector>

#include "account_error_no.h"
#include "os_account_sub_profile_subscribe_callback.h"

#define ACCOUNT_DELETION_STUB_SUBPROFILE_CLIENT 1

namespace OHOS {
namespace AccountSA {
class OsAccountSubProfileClient {
public:
    static OsAccountSubProfileClient &GetInstance();

    ErrCode GetOsAccountSubProfileIds(int32_t osAccountId, std::vector<int32_t> &subProfileIds);
    ErrCode GetOsAccountLocalIdForSubProfile(int32_t subProfileId, int32_t &osAccountId);

    ErrCode SubscribeOsAccountSubProfileEvents(
        const std::set<OsAccountSubProfileEventType> &types,
        const std::shared_ptr<OsAccountSubProfileSubscribeCallback> &callback);

    ErrCode UnsubscribeOsAccountSubProfileEvents(
        const std::shared_ptr<OsAccountSubProfileSubscribeCallback> &callback);

private:
    OsAccountSubProfileClient() = default;
    ~OsAccountSubProfileClient() = default;
    OsAccountSubProfileClient(const OsAccountSubProfileClient &) = delete;
    OsAccountSubProfileClient &operator=(const OsAccountSubProfileClient &) = delete;
};
}  // namespace AccountSA
}  // namespace OHOS

#endif  // ACCOUNT_DELETION_TEST_MOCK_OS_ACCOUNT_SUBPROFILE_CLIENT_H
