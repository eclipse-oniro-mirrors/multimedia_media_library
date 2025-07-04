/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_ANI_ERROR_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_ANI_ERROR_H

#include <string>
#include "ani.h"
#include "datashare_result_set.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct AniError {
    int32_t error = 0;
    std::string apiName;
    EXPORT void SetApiName(const std::string &Name);
    void SaveError(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet);
    EXPORT void SaveError(int32_t ret);
    EXPORT void HandleError(ani_env *env, ani_object &errorObj);
    EXPORT static void ThrowError(ani_env *env, int32_t err, const std::string &errMsg = "");
    EXPORT static void ThrowError(ani_env *env, int32_t err, const char *func, int32_t line,
        const std::string &errMsg = "");
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_ANI_SRC_INCLUDE_ANI_ERROR_H