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
 
#ifndef ASSET_MAP_CLONE_H
#define ASSET_MAP_CLONE_H
 
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>
 
#include "backup_const.h"
#include "rdb_store.h"
 
 
namespace OHOS {
namespace Media {
struct AssetMapTbl {
    std::optional<int32_t> fileId;
    std::optional<std::string> data;
    std::optional<int32_t> OldFileId;
    std::optional<std::string> OldData;
    std::optional<int32_t> cloneSequence;
};
 
class AssetMapClone {
public:
    AssetMapClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap);
 
    bool CloneAssetMapInfo();
 
    int64_t GetMigratedCount() const { return migrateNum_; }
    int64_t GetInsertNum() const { return insertNum_; }
    int64_t GetTotalTimeCost() const { return migrateTotalTimeCost_; }
 
private:
    bool CloneAssetMapInBatches(const std::vector<int32_t>& oldFileIds);
    std::vector<AssetMapTbl> QueryAssetMapTbl(const std::string &fileIdClause);
    std::optional<std::string> ReadDataFromDestPhotosTable(int32_t fileId);
    std::optional<int32_t> GetLastCloneSequence();
    void ParseAssetMapResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        AssetMapTbl& assetMapTbl);
    std::vector<AssetMapTbl> ProcessAssetMapTbls(
        const std::vector<AssetMapTbl>& assetMapTbls);
    void BatchInsertAssetMaps(const std::vector<AssetMapTbl>& assetMapTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromAssetMapTbl(
        const AssetMapTbl& assetMapTbl);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values, int64_t &changedRows);
 
    template<typename T>
    void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
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
 
    template<typename T, typename U>
    void PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const U& defaultValue);
 
private:
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap_;
    int32_t nextCloneSequence_ {0};
    int64_t migrateNum_ = 0;
    int64_t insertNum_ = 0;
    int64_t migrateTotalTimeCost_ = 0;
};
 
template<typename T, typename U>
void AssetMapClone::PutWithDefault(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const U& defaultValue)
{
    if (optionalValue.has_value()) {
        PutIfPresent(values, columnName, optionalValue);
    } else {
        PutIfPresent(values, columnName, std::optional<U>(defaultValue));
    }
}
 
} // namespace Media
} // namespace OHOS
 
#endif // ASSET_MAP_CLONE_H