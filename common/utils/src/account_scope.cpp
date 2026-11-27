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

#include "account_scope.h"

namespace OHOS {
namespace Media {

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

AccountScope::AccountScope(int32_t accountId) : prev_(Current())
{
    Current() = accountId;
}

AccountScope::~AccountScope()
{
    Current() = prev_;
}

int32_t AccountScope::Get()
{
    return Current();
}

int32_t &AccountScope::Current()
{
    thread_local int32_t id = INVALID_ACCOUNT_ID;
    return id;
}

#endif  // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

}  // namespace Media
}  // namespace OHOS
