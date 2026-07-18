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

#ifndef CLONE_RESTORE_GEO_BASE_H
#define CLONE_RESTORE_GEO_BASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include "rdb_store.h"
#include "values_bucket.h"
#include "vision_column.h"

namespace OHOS::Media {

constexpr int32_t PAGE_SIZE = 200;
constexpr int32_t GEO_STATUS_SUCCESS = 1;
constexpr int32_t GEO_STATUS_VALUE = 3;

const std::string GEO_TOTAL_TABLE = "tab_analysis_total";

struct GeoCloneInfo {
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<int64_t> locationKey;
    std::optional<std::string> cityId;
    std::optional<std::string> language;
    std::optional<std::string> country;
    std::optional<std::string> adminArea;
    std::optional<std::string> subAdminArea;
    std::optional<std::string> locality;
    std::optional<std::string> subLocality;
    std::optional<std::string> thoroughfare;
    std::optional<std::string> subThoroughfare;
    std::optional<std::string> featureName;
    std::optional<std::string> cityName;
    std::optional<std::string> addressDescription;
    std::optional<std::string> aoi;
    std::optional<std::string> poi;
    std::optional<std::string> firstAoi;
    std::optional<std::string> firstPoi;
    std::optional<std::string> locationVersion;
    std::optional<std::string> firstAoiCategory;
    std::optional<std::string> firstPoiCategory;
    std::optional<int32_t> fileIdOld;
    std::optional<int32_t> fileIdNew;
    std::optional<std::string> locationType;
};

struct GeoDictionaryCloneInfo {
    std::optional<std::string> cityId;
    std::optional<std::string> language;
    std::optional<std::string> cityName;
};

struct CityMapInfo {
    std::optional<int32_t> mapAlbum;
    std::optional<int32_t> mapAsset;
};

class CloneRestoreGeoBase {
public:
    virtual ~CloneRestoreGeoBase() = default;

protected:
    EXPORT void GetGeoInfoFromResultSet(GeoCloneInfo &info,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    EXPORT void GetGeoInsertValue(NativeRdb::ValuesBucket &value,
        GeoCloneInfo &info,
        const std::unordered_set<std::string> &intersection);
    void GetGeoDictionaryInsertValue(NativeRdb::ValuesBucket &value,
        GeoDictionaryCloneInfo &info,
        const std::unordered_set<std::string> &intersection);
    EXPORT std::unordered_set<std::string> GetCommonColumns(
        const std::string &tableName,
        std::shared_ptr<NativeRdb::RdbStore> srcRdb,
        std::shared_ptr<NativeRdb::RdbStore> dstRdb);
    EXPORT bool CheckTableColumns(const std::string &tableName,
        std::unordered_map<std::string, std::string> &columns,
        std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    EXPORT int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum,
        std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values,
        const std::string& columnName,
        const std::optional<T>& optionalValue);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket& values,
        const std::string& columnName,
        const std::optional<T>& optionalValue,
        const std::unordered_set<std::string> &intersection);

    int32_t sceneCode_{-1};
    std::string taskId_;
    std::string systemLanguage_{"zh-Hans"};
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
};

template<typename T>
void CloneRestoreGeoBase::PutIfPresent(NativeRdb::ValuesBucket& values,
    const std::string& columnName,
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
void CloneRestoreGeoBase::PutIfInIntersection(NativeRdb::ValuesBucket& values,
    const std::string& columnName,
    const std::optional<T>& optionalValue,
    const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(columnName) > 0) {
        PutIfPresent<T>(values, columnName, optionalValue);
    }
}

} // namespace OHOS::Media

#endif // CLONE_RESTORE_GEO_BASE_H