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

#include "media_string_utils.h"

#include <charconv>

namespace OHOS::Media {
bool MediaStringUtils::ConvertToInt(const std::string &number, int& value)
{
    if (number.empty()) {
        return false;
    }
    auto [ptr, ec] = std::from_chars(number.data(), number.data() + number.size(), value);
    return ec == std::errc{} && ptr == number.data() + number.size();
}

bool MediaStringUtils::StartsWith(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}
} // namespace OHOS::Media