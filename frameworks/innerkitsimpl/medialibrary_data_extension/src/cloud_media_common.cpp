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

#include "cloud_media_common.h"
#include "media_log.h"
#include <sstream>

namespace OHOS::Media {
std::string CloudMediaCommon::ToStringWithComma(const std::vector<std::string> &fileIds)
{
    std::stringstream os;
    for (size_t i = 0; i < fileIds.size(); ++i) {
        os << fileIds[i];
        if (i != fileIds.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string CloudMediaCommon::FillParams(const std::string &sql, const std::vector<std::string> &bindArgs)
{
    std::stringstream os;
    std::string flag;
    const std::string leftBrace = "{";
    const std::string rightBrace = "}";
    std::string val;
    std::string result = sql;
    for (size_t i = 0; i < bindArgs.size(); i++) {
        flag = leftBrace + std::to_string(i) + rightBrace;
        val = bindArgs[i];
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

int32_t CloudMediaCommon::ToInt32(const std::string &str)
{
    char *end;
    long number = std::strtol(str.c_str(), &end, 10);
    if (*end != '\0') {
        MEDIA_ERR_LOG("ToNumber failed, has invalid char. str: %{public}s", str.c_str());
        return 0;
    } else if (number < INT_MIN || number > INT_MAX) {
        MEDIA_ERR_LOG("ToNumber failed, number overflow. str: %{public}s", str.c_str());
        return 0;
    }
    return static_cast<int32_t>(number);
}
}