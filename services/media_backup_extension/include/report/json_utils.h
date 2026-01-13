/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_BACKUP_JSON_UTILS_H
#define OHOS_MEDIA_BACKUP_JSON_UTILS_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "media_log.h"

namespace OHOS::Media {
class JsonUtils {
public:
    bool IsValid(const nlohmann::json &jsonObj)
    {
        return !(jsonObj.is_discarded() || jsonObj.is_null() || jsonObj.empty());
    }

    nlohmann::json Parse(const std::string &jsonStr)
    {
        if (!nlohmann::json::accept(jsonStr)) {
            return nlohmann::json();
        }
        nlohmann::json jsonObj = nlohmann::json::parse(jsonStr, nullptr, false);
        if (!this->IsValid(jsonObj)) {
            MEDIA_ERR_LOG("parse JSON failed, %{public}s", jsonStr.c_str());
            return nlohmann::json();
        }
        return jsonObj;
    }

    bool IsExists(const nlohmann::json &jsonObj, const std::string &key)
    {
        return jsonObj.find(key) != jsonObj.end();
    }

    int32_t GetInt(const nlohmann::json &jsonObj, const std::string &key, int32_t defaultValue = 0)
    {
        auto iter = jsonObj.find(key);
        if (iter != jsonObj.end() && iter->is_number()) {
            return iter->get<int32_t>();
        }
        return defaultValue;
    }

    std::string GetString(const nlohmann::json &jsonObj, const std::string &key, const std::string defaultValue = "")
    {
        auto iter = jsonObj.find(key);
        if (iter != jsonObj.end() && iter->is_string()) {
            return iter->get<std::string>();
        }
        return defaultValue;
    }

    std::vector<nlohmann::json> GetArray(const nlohmann::json &jsonObj, const std::string &key)
    {
        auto iter = jsonObj.find(key);
        if (iter != jsonObj.end() && iter->is_array()) {
            return iter->get<std::vector<nlohmann::json>>();
        }
        return {};
    }
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_JSON_UTILS_H