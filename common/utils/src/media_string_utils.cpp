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

#include <algorithm>
#include <charconv>
#include <cctype>
#include <sstream>

#include "media_log.h"

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

bool MediaStringUtils::EndsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.rfind(suffix) == str.length() - suffix.length();
}

std::string MediaStringUtils::ToLower(const std::string &str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

inline char ToLowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ?
        static_cast<char>(static_cast<int32_t>(c) + (static_cast<int32_t>('a') - static_cast<int32_t>('A'))) : c;
}

inline bool CaseInsensitiveCharCompare(char a, char b)
{
    return ToLowerAscii(a) == ToLowerAscii(b);
}

bool MediaStringUtils::StartsWithIgnoreCase(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin(), CaseInsensitiveCharCompare);
}

bool MediaStringUtils::EqualToIgnoreCase(const std::string &strA, const std::string &strB)
{
    if (strA.size() != strB.size()) {
        return false;
    }
    return std::equal(strA.begin(), strA.end(), strB.begin(), CaseInsensitiveCharCompare);
}

std::string MediaStringUtils::FillParams(const std::string &content, const std::vector<std::string> &bindArgs)
{
    std::stringstream os;
    std::string flag;
    const std::string leftBrace = "{";
    const std::string rightBrace = "}";
    std::string val;
    std::string result = content;
    for (size_t i = 0; i < bindArgs.size(); i++) {
        flag = leftBrace + std::to_string(i) + rightBrace;
        val = bindArgs[i];
        // Dead loop check: the value should not be replaced by the flag itself.
        CHECK_AND_CONTINUE(val.find(flag) == std::string::npos);
        size_t pos = result.find(flag);
        while (pos != std::string::npos) {
            os.str("");
            os << result.substr(0, pos) << bindArgs[i];
            os << (pos + flag.length() <= result.length() ? result.substr(pos + flag.length()) : "");
            result = os.str();
            os.str("");
            pos = result.find(flag);
        }
    }
    return result;
}
} // namespace OHOS::Media