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
 
#ifndef CUSTOM_RESTORE_UTILS_H
#define CUSTOM_RESTORE_UTILS_H
 
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
 
namespace OHOS {
namespace NativeRdb {
class ValuesBucket;
class ResultSet;
class ValueObject;
}
namespace Media {
 
class CustomRestoreInfo;
struct RestoreFileInfo;
struct TimeInfo;
class Metadata;
 
#define EXPORT __attribute__ ((visibility ("default")))
 
class EXPORT CustomRestoreUtils {
public:
    static bool IsDuplication(const CustomRestoreInfo &info, const RestoreFileInfo &fileInfo);
    static std::unordered_set<std::string> BatchQueryDuplicates(const CustomRestoreInfo &info,
        const std::vector<RestoreFileInfo> &fileInfos);
    static bool IsDuplicateInCache(const CustomRestoreInfo &info, const RestoreFileInfo &fileInfo);
    static int32_t FillMetadata(const std::unordered_map<std::string, TimeInfo> &timeInfoMap,
        const RestoreFileInfo &fileInfo, std::unique_ptr<Metadata> &data);
    static int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    static void SetTimeInfo(const std::unique_ptr<Metadata> &data, NativeRdb::ValuesBucket &value);

private:
    static std::string MakeDuplicateKey(const std::string &name, int64_t size,
        int32_t type, int32_t orient);
    static void BuildDuplicateQuerySql(const CustomRestoreInfo &info,
        const std::vector<RestoreFileInfo> &fileInfos, std::string &sql,
        std::vector<NativeRdb::ValueObject> &params);
    static void CollectDuplicateKeys(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unordered_set<std::string> &dupKeys);
};
 
} // namespace Media
} // namespace OHOS
 
#endif // CUSTOM_RESTORE_UTILS_H