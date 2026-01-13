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

#ifndef CLONE_RESTORE_GEO_H
#define CLONE_RESTORE_GEO_H

#include <string>

#include "backup_const.h"
#include "clone_restore_analysis_total.h"
#include "rdb_store.h"

namespace OHOS::Media {
class CloneRestoreGeo {
public:
    void Init(int32_t sceneCode, const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> mediaRdb);
    void Restore(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection);

private:
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

    void GetMaxIds();
    void RestoreBatch(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void RestoreMaps();
    void ReportRestoreTask();
    void ReportRestoreTaskOfTotal();
    void ReportRestoreTaskofData();

    void GetInfos(std::vector<GeoCloneInfo> &infos);
    void DeduplicateInfos(std::vector<GeoCloneInfo> &infos);
    std::unordered_set<int32_t> GetExistingFileIds(const std::string &tableName);
    void RemoveDuplicateInfos(std::vector<GeoCloneInfo> &infos, const std::unordered_set<int32_t> &existingFileIds);
    void InsertIntoTable(std::vector<GeoCloneInfo> &infos);

    void GetInfo(GeoCloneInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetMapInsertValue(NativeRdb::ValuesBucket &value, GeoCloneInfo info,
        const std::unordered_set<std::string> &intersection);

    bool CheckTableColumns(const std::string& tableName, std::unordered_map<std::string, std::string>& columns);
    std::unordered_set<std::string> GetCommonColumns(const std::string &tableName);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
    std::string analysisType_;
    std::string systemLanguage_{"zh-Hans"};
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    int32_t maxId_{0};
    std::atomic<int32_t> successCnt_{0};
    std::atomic<int32_t> failedCnt_{0};
    std::atomic<int32_t> duplicateCnt_{0};
    std::atomic<int64_t> restoreTimeCost_{0};
    CloneRestoreAnalysisTotal cloneRestoreAnalysisTotal_;
};

template<typename T>
void CloneRestoreGeo::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
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
void CloneRestoreGeo::PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(columnName) > 0) {
        PutIfPresent<T>(values, columnName, optionalValue);
        return;
    }
}
} // namespace OHOS::Media
#endif // CLONE_RESTORE_GEO_H