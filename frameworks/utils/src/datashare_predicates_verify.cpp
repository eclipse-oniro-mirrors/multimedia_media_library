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
#define LOG_TAG "datashare_predicates_verify"

#include "datashare_predicates_verify.h"

#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

std::pair<int, int> DataSharePredicatesVerify::VerifyPredicates(
    const DataShare::DataSharePredicates &predicates)
{
    const auto &operations = predicates.GetOperationList();
    for (const auto &oper : operations) {
        int32_t errCode = registry_.Validate(static_cast<DataShare::OperationType>(oper.operation), oper);
        if (errCode != E_OK) {
            int predicatesType = static_cast<int>(oper.operation);
            return std::make_pair(predicatesType, errCode);
        }
    }
    return std::make_pair(E_OK, E_OK);
}

} // namespace Media
} // namespace OHOS
