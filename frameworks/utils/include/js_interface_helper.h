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

#ifndef INTERFACES_KITS_JS_INCLUDE_JS_INTERFACE_HELPER_H
#define INTERFACES_KITS_JS_INCLUDE_JS_INTERFACE_HELPER_H

#include "datashare_predicates.h"

#include <string>
#include <vector>
#include <memory>

namespace OHOS {
namespace Media {
class JsInterfaceHelper {
public:
    static std::string PredicateToStringSafe(const std::shared_ptr<DataShare::DataShareAbsPredicates>& predicate);
    static std::string GetSafeUri(const std::string& uri);
    static std::string GetSafeDisplayName(const std::string& displayName);
    static std::string MaskString(const std::string& str);
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_INCLUDE_JS_INTERFACE_HELPER_H