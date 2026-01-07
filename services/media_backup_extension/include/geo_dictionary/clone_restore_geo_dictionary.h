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

#ifndef CLONE_RESTORE_GEO_DICTIONARY_H
#define CLONE_RESTORE_GEO_DICTIONARY_H

#include <string>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreGeoDictionary {
public:
    void Init(int32_t sceneCode, const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb);
    void RestoreAlbums();
    void ReportGeoRestoreTask();

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection);

private:
    struct GeoDictionaryCloneInfo {
        std::optional<std::string> cityId;
        std::optional<std::string> language;
        std::optional<std::string> cityName;
    };

    void GetGeoDictionaryInfos();
    void InsertIntoGeoDictionaryAlbums();

    bool CheckTableColumns(const std::string& tableName, std::unordered_map<std::string, std::string>& columns);
    void GetGeoDictionaryInfo(GeoDictionaryCloneInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GeoDictionaryDeduplicate();

    std::unordered_set<std::string> GetCommonColumns(const std::string &tableName);
    void GetGeoDictionaryInsertValue(NativeRdb::ValuesBucket &value, const GeoDictionaryCloneInfo &info,
        const std::unordered_set<std::string> &intersection);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
    std::string systemLanguage_{"zh-Hans"};
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::vector<GeoDictionaryCloneInfo> geoDictionaryInfos_;
    std::vector<GeoDictionaryCloneInfo> dstGeoDictionaryInfos_;
    std::atomic<int32_t> successInsertCnt_{0};
    std::atomic<int32_t> failInsertCnt_{0};
};

template<typename T>
void CloneRestoreGeoDictionary::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue)
{
    if (optionalValue.has_value()) {
        if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
            values.PutInt(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
            values.PutLong(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            values.PutString(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
            values.PutDouble(columnName, optionalValue.value());
        }
    }
}

template<typename T>
void CloneRestoreGeoDictionary::PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(columnName) > 0) {
        PutIfPresent<T>(values, columnName, optionalValue);
    }
}
} // namespace OHOS::Media
#endif // CLONE_RESTORE_GEO_DICTIONARY_H