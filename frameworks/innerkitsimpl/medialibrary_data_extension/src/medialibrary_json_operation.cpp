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

#include "medialibrary_json_operation.h"

#include <iostream>
#include <map>
#include <fstream>

#include "dfx_utils.h"
#include "file_asset.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS::Media {
constexpr int JSON_INDENT_WIDTH = 4;

bool MediaJsonOperation::IsValid(const nlohmann::json &jsonObj)
{
    return !(jsonObj.is_discarded() || jsonObj.is_null() || jsonObj.empty());
}

int32_t MediaJsonOperation::MapToJsonFile(
    const std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> &externalMap,
    const std::vector<std::string> &columns, const std::string &outputPath)
{
    MEDIA_INFO_LOG("Start to convert map to json file");
    nlohmann::json jsonData;
    CHECK_AND_RETURN_RET_LOG(!externalMap.empty(), E_ERR, "externalMap is empty");
    CHECK_AND_RETURN_RET_LOG(!columns.empty(), E_ERR, "columns is empty");
    CHECK_AND_RETURN_RET_LOG(!outputPath.empty(), E_ERR, "outputPath is empty");
    for (const auto &column : columns) {
        auto it = externalMap.find(column);
        if (it != externalMap.end()) {
            std::visit([&](auto&& value) { jsonData[column] = value; }, it->second);
        }
    }
    if (!MediaFileUtils::IsFileExists(outputPath)) {
        std::string parentPath = MediaFileUtils::GetParentPath(outputPath);
        if (!MediaFileUtils::IsDirExists(parentPath)) {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(parentPath), E_ERR,
                "Failed to create directory %{private}s", parentPath.c_str());
        }
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(outputPath), E_ERR,
            "Failed to create file %{private}s", outputPath.c_str());
    }
    std::ofstream map_file(outputPath);
    CHECK_AND_RETURN_RET_LOG(map_file.is_open(), E_ERR,
        "Failed to convert json file: %{public}s", DfxUtils::GetSafePath(outputPath).c_str());
    map_file << jsonData.dump(JSON_INDENT_WIDTH);
    map_file.close();
    MEDIA_INFO_LOG("Convert map to json successfully, outputPath: %{public}s",
        DfxUtils::GetSafePath(outputPath).c_str());
    return E_OK;
}

bool MediaJsonOperation::CheckPathAndLoadJson(const std::string &jsonFilePath, nlohmann::json &outJsonData)
{
    CHECK_AND_RETURN_RET_LOG(!jsonFilePath.empty(), false, "Invalid input: json file path is empty");
    std::ifstream jsonFile(jsonFilePath);
    CHECK_AND_RETURN_RET_LOG(jsonFile.is_open(), false, "Failed to open json file");
    std::string jsonContent((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
    jsonFile.close();
    outJsonData = nlohmann::json::parse(jsonContent, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(MediaJsonOperation::IsValid(outJsonData), false, "Failed to parse json content");
    return true;
}

NativeRdb::ValuesBucket MediaJsonOperation::ReadJsonToValuesBucket(const std::string &jsonFilePath,
    const std::vector<std::string> &columns)
{
    NativeRdb::ValuesBucket values;
    nlohmann::json jsonData;
    CHECK_AND_RETURN_RET_LOG(MediaJsonOperation::CheckPathAndLoadJson(jsonFilePath, jsonData), values,
        "Failed to load json from path: %{public}s", DfxUtils::GetSafePath(jsonFilePath).c_str());
    const auto &memberTypeMap = GetFileAssetMemberMap();
    for (const auto &key : columns) {
        CHECK_AND_CONTINUE_ERR_LOG(memberTypeMap.count(key) > 0,
            "Key not defined in member type map: %{public}s", key.c_str());
        int memberType = memberTypeMap.at(key);
        ParseAndPutKey(key, jsonData, memberType, values);
    }
    return values;
}

void MediaJsonOperation::ParseAndPutKey(const std::string &key, const nlohmann::json &jsonData, const int memberType,
    NativeRdb::ValuesBucket &values)
{
    CHECK_AND_RETURN_LOG(jsonData.contains(key), "Key not found in json: %{public}s", key.c_str());
    const nlohmann::json &value = jsonData[key];
    bool parsed = false;
    switch (memberType) {
        case MEMBER_TYPE_STRING:
            if (value.is_string()) {
                values.Put(key, value.get<std::string>());
                parsed = true;
            }
            break;
        case MEMBER_TYPE_INT32:
            if (value.is_number_integer()) {
                values.Put(key, value.get<int32_t>());
                parsed = true;
            }
            break;
        case MEMBER_TYPE_INT64:
            if (value.is_number_integer()) {
                values.Put(key, value.get<int64_t>());
                parsed = true;
            }
            break;
        case MEMBER_TYPE_DOUBLE:
            if (value.is_number_float()) {
                values.Put(key, value.get<double>());
                parsed = true;
            }
            break;
        default:
            MEDIA_ERR_LOG("Unsupported member type for key: %{public}s", key.c_str());
            break;
    }
    if (!parsed) {
        MEDIA_ERR_LOG("Failed to parse value for key: %{public}s, type mismatch", key.c_str());
    }
}
}  // namespace OHOS::Media