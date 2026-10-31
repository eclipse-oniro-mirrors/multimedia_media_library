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
#ifndef OHOS_MEDIA_RECOVERY_JSON_UTILS_H
#define OHOS_MEDIA_RECOVERY_JSON_UTILS_H

#include <string>

#include <nlohmann/json.hpp>
#include "values_bucket.h"

namespace OHOS::Media {
enum class RecoveryJsonStatus { OK, READ_ERROR, INVALID };
RecoveryJsonStatus ReadRecoveryJsonFile(const std::string &path, nlohmann::json &value);
// Invalid or absent fields are not added to the update bucket.
bool PutRecoveryJsonValue(NativeRdb::ValuesBucket &values, const std::string &name,
    const nlohmann::json &value, const std::string &type);
} // namespace OHOS::Media
#endif // OHOS_MEDIA_RECOVERY_JSON_UTILS_H
