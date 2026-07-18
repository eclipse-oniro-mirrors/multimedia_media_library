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

#define MLOG_TAG "CloneRestoreGeoBase"

#include "clone_restore_geo_base.h"

#include "backup_database_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "location_column.h"
#include "vision_column_comm.h"

namespace OHOS::Media {

const std::string TEXT = "TEXT";
const std::string INTEGER = "INTEGER";

const std::unordered_map<std::string, std::unordered_set<std::string>> COMPARED_COLUMNS_MAP = {
    { GEO_KNOWLEDGE_TABLE,
        {
            "latitude", "longitude", "location_key", "city_id", "language",
            "country", "admin_area", "sub_admin_area", "locality", "sub_locality",
            "thoroughfare", "sub_thoroughfare", "feature_name", "city_name",
            "address_description", "aoi", "poi", "first_aoi", "first_poi",
            "location_version", "first_aoi_category", "first_poi_category",
            "file_id", "location_type"
        }
    },
    { GEO_DICTIONARY_TABLE,
        {
            "city_id", "language", "city_name"
        }
    }
};
// LCOV_EXCL_START
void CloneRestoreGeoBase::GetGeoInfoFromResultSet(GeoCloneInfo &info,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    CHECK_AND_RETURN(resultSet != nullptr);
    info.latitude = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, LATITUDE);
    info.longitude = BackupDatabaseUtils::GetOptionalValue<double>(resultSet, LONGITUDE);
    info.locationKey = BackupDatabaseUtils::GetOptionalValue<int64_t>(resultSet, LOCATION_KEY);
    info.cityId = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CITY_ID);
    info.language = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LANGUAGE);
    info.country = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, COUNTRY);
    info.adminArea = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ADMIN_AREA);
    info.subAdminArea = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_ADMIN_AREA);
    info.locality = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LOCALITY);
    info.subLocality = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_LOCALITY);
    info.thoroughfare = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, THOROUGHFARE);
    info.subThoroughfare = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, SUB_THOROUGHFARE);
    info.featureName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FEATURE_NAME);
    info.cityName = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, CITY_NAME);
    info.addressDescription = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ADDRESS_DESCRIPTION);
    info.aoi = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, AOI);
    info.poi = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, POI);
    info.firstAoi = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FIRST_AOI);
    info.firstPoi = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FIRST_POI);
    info.locationVersion = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LOCATION_VERSION);
    info.firstAoiCategory = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FIRST_AOI_CATEGORY);
    info.firstPoiCategory = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, FIRST_POI_CATEGORY);
    info.fileIdOld = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, FILE_ID);
    info.locationType = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, LOCATION_TYPE);
}

void CloneRestoreGeoBase::GetGeoInsertValue(NativeRdb::ValuesBucket &value,
    GeoCloneInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, LATITUDE, info.latitude, intersection);
    PutIfInIntersection(value, LONGITUDE, info.longitude, intersection);
    PutIfInIntersection(value, LOCATION_KEY, info.locationKey, intersection);
    PutIfInIntersection(value, CITY_ID, info.cityId, intersection);
    PutIfInIntersection(value, LANGUAGE, info.language, intersection);
    PutIfInIntersection(value, COUNTRY, info.country, intersection);
    PutIfInIntersection(value, ADMIN_AREA, info.adminArea, intersection);
    PutIfInIntersection(value, SUB_ADMIN_AREA, info.subAdminArea, intersection);
    PutIfInIntersection(value, LOCALITY, info.locality, intersection);
    PutIfInIntersection(value, SUB_LOCALITY, info.subLocality, intersection);
    PutIfInIntersection(value, THOROUGHFARE, info.thoroughfare, intersection);
    PutIfInIntersection(value, SUB_THOROUGHFARE, info.subThoroughfare, intersection);
    PutIfInIntersection(value, FEATURE_NAME, info.featureName, intersection);
    PutIfInIntersection(value, CITY_NAME, info.cityName, intersection);
    PutIfInIntersection(value, ADDRESS_DESCRIPTION, info.addressDescription, intersection);
    PutIfInIntersection(value, AOI, info.aoi, intersection);
    PutIfInIntersection(value, POI, info.poi, intersection);
    PutIfInIntersection(value, FIRST_AOI, info.firstAoi, intersection);
    PutIfInIntersection(value, FIRST_POI, info.firstPoi, intersection);
    PutIfInIntersection(value, LOCATION_VERSION, info.locationVersion, intersection);
    PutIfInIntersection(value, FIRST_AOI_CATEGORY, info.firstAoiCategory, intersection);
    PutIfInIntersection(value, FIRST_POI_CATEGORY, info.firstPoiCategory, intersection);
    PutIfInIntersection(value, FILE_ID, info.fileIdNew, intersection);
    PutIfInIntersection(value, LOCATION_TYPE, info.locationType, intersection);
}

void CloneRestoreGeoBase::GetGeoDictionaryInsertValue(NativeRdb::ValuesBucket &value,
    GeoDictionaryCloneInfo &info,
    const std::unordered_set<std::string> &intersection)
{
    PutIfInIntersection(value, CITY_ID, info.cityId, intersection);
    PutIfInIntersection(value, LANGUAGE, info.language, intersection);
    PutIfInIntersection(value, CITY_NAME, info.cityName, intersection);
}

std::unordered_set<std::string> CloneRestoreGeoBase::GetCommonColumns(
    const std::string &tableName,
    std::shared_ptr<NativeRdb::RdbStore> srcRdb,
    std::shared_ptr<NativeRdb::RdbStore> dstRdb)
{
    auto srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(srcRdb, tableName);
    auto dstColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(dstRdb, tableName);
    std::unordered_set<std::string> result;

    auto comparedColumnsIt = COMPARED_COLUMNS_MAP.find(tableName);
    if (comparedColumnsIt == COMPARED_COLUMNS_MAP.end()) {
        return result;
    }
    const auto& comparedColumns = comparedColumnsIt->second;

    for (auto it = dstColumnInfoMap.begin(); it != dstColumnInfoMap.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) != srcColumnInfoMap.end() &&
            comparedColumns.count(it->first) > 0) {
            result.insert(it->first);
        }
    }
    return result;
}

bool CloneRestoreGeoBase::CheckTableColumns(const std::string &tableName,
    std::unordered_map<std::string, std::string> &columns,
    std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    auto srcColumnInfoMap = BackupDatabaseUtils::GetColumnInfoMap(rdbStore, tableName);
    for (auto it = columns.begin(); it != columns.end(); ++it) {
        if (srcColumnInfoMap.find(it->first) == srcColumnInfoMap.end()) {
            return false;
        }
    }
    return true;
}

int32_t CloneRestoreGeoBase::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values,
    int64_t &rowNum,
    std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(rdbStore);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK,
            "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK,
        "BatchInsertWithRetry: trans finish fail!, ret:%{public}d", errCode);
    return errCode;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media