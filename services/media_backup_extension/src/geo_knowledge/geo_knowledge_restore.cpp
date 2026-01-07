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
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "geo_knowledge_restore.h"

#include "backup_database_utils.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "locale_config.h"
#include "upgrade_restore_task_report.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
const int32_t PAGE_SIZE = 200;
const int32_t UPDATE_GEO = 3;
const string NOT_MATCH = "NOT MATCH";
const string LATITUDE = "latitude";
const string LONGITUDE = "longitude";
const string LOCATION_KEY = "location_key";
const string LANGUAGE = "language";
const string COUNTRY = "country";
const string ADMIN_AREA = "admin_area";
const string SUB_ADMIN_AREA = "sub_admin_area";
const string LOCALITY = "locality";
const string SUB_LOCALITY = "sub_locality";
const string THOROUGHFARE = "thoroughfare";
const string SUB_THOROUGHFARE = "sub_thoroughfare";
const string FEATURE_NAME = "feature_name";
const string ADDRESS_DESCRIPTION = "address_description";
const string FILE_ID = "file_id";
const string DATA = "data";
const string SINGLE_CH = "zh-Hans";
const string SINGLE_EN = "en-Latn-US";
const string DOUBLE_CH = "zh";
const string DOUBLE_EN = "en";
const int32_t GEO_STATUS_SUCCESS = 1;
constexpr double DOUBLE_EPSILON = 1e-15;

// LCOV_EXCL_START
void GeoKnowledgeRestore::Init(int32_t sceneCode, std::string taskId,
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb)
{
    sceneCode_ = sceneCode;
    taskId_ = taskId;
    mediaLibraryRdb_ = mediaLibraryRdb;
    galleryRdb_ = galleryRdb;
    batchCnt_ = 0;
    successInsertCnt_ = 0;
    successUpdateCnt_ = 0;
    failInsertCnt_ = 0;
    failUpdateCnt_ = 0;
    systemLanguage_ = Global::I18n::LocaleConfig::GetSystemLanguage();
}

void GeoKnowledgeRestore::RestoreGeo(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    CHECK_AND_RETURN_LOG(galleryRdb_ != nullptr && mediaLibraryRdb_ != nullptr, "rdbStore is nullptr");
    GetGeoKnowledgeInfos();
    RestoreMaps(photoInfoMap);
    ReportGeoRestoreTask();
}

void GeoKnowledgeRestore::GetGeoKnowledgeInfos()
{
    const std::string QUERY_SQL = "SELECT " + LATITUDE + ", " + LONGITUDE + ", " + LOCATION_KEY + ", " + LANGUAGE + ", "
        + COUNTRY + ", " + ADMIN_AREA + ", " + SUB_ADMIN_AREA + ", " + LOCALITY + ", " + SUB_LOCALITY + ", "
        + THOROUGHFARE + ", " + SUB_THOROUGHFARE + ", " + FEATURE_NAME + " "
        "FROM t_geo_knowledge WHERE COALESCE(" + LANGUAGE + ", '') <> ' ' AND rowid > ? ORDER BY rowid LIMIT ?";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = galleryRdb_->QuerySql(QUERY_SQL, params);
        CHECK_AND_BREAK_ERR_LOG(resultSet != nullptr, "resultSet is nullptr");
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GeoKnowledgeInfo info;
            info.latitude = GetDoubleVal(LATITUDE, resultSet);
            info.longitude = GetDoubleVal(LONGITUDE, resultSet);
            info.locationKey = GetInt64Val(LOCATION_KEY, resultSet);
            info.language = GetStringVal(LANGUAGE, resultSet);
            info.country = GetStringVal(COUNTRY, resultSet);
            info.adminArea = GetStringVal(ADMIN_AREA, resultSet);
            info.subAdminArea = GetStringVal(SUB_ADMIN_AREA, resultSet);
            info.locality = GetStringVal(LOCALITY, resultSet);
            info.subLocality = GetStringVal(SUB_LOCALITY, resultSet);
            info.thoroughfare = GetStringVal(THOROUGHFARE, resultSet);
            info.subThoroughfare = GetStringVal(SUB_THOROUGHFARE, resultSet);
            info.featureName = GetStringVal(FEATURE_NAME, resultSet);
            albumInfos_.emplace_back(info);
        }
        resultSet->GetRowCount(rowCount);
        offset += PAGE_SIZE;
        resultSet->Close();
    } while (rowCount > 0);
}

void GeoKnowledgeRestore::RestoreMaps(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    if (mediaLibraryRdb_ == nullptr || galleryRdb_ == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        batchCnt_ = 0;
        return;
    }
    std::string querySql = "SELECT _id, latitude, longitude FROM gallery_media "
        "WHERE (ABS(latitude) >= 1e-15 OR ABS(longitude) >= 1e-15) AND _id > ? ORDER BY _id ASC LIMIT ?;";
    int rowCount = 0;
    int offset = 0;
    do {
        std::vector<std::string> fileIds;
        std::vector<NativeRdb::ValuesBucket> values;
        std::vector<NativeRdb::ValueObject> params = {offset, PAGE_SIZE};
        auto resultSet = BackupDatabaseUtils::QuerySql(galleryRdb_, querySql, params);
        CHECK_AND_RETURN(resultSet != nullptr);
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GeoMapInfo geoMapInfo;
            geoMapInfo.fileIdOld = GetInt32Val("_id", resultSet);
            offset = geoMapInfo.fileIdOld;
            CHECK_AND_CONTINUE(photoInfoMap.find(geoMapInfo.fileIdOld) != photoInfoMap.end());
            geoMapInfo.photoInfo = photoInfoMap.at(geoMapInfo.fileIdOld);
            geoMapInfo.latitude = GetDoubleVal("latitude", resultSet);
            geoMapInfo.longitude = GetDoubleVal("longitude", resultSet);
            std::string fileIdString = UpdateMapInsertValues(values, geoMapInfo);
            CHECK_AND_EXECUTE(fileIdString == NOT_MATCH, fileIds.push_back(fileIdString));
        }
        resultSet->GetRowCount(rowCount);
        resultSet->Close();
        UpdateMaps(values, fileIds);
    } while (rowCount == PAGE_SIZE);
}

void GeoKnowledgeRestore::UpdateMaps(std::vector<NativeRdb::ValuesBucket> &values, std::vector<std::string> &fileIds)
{
    int64_t rowNum = 0;
    int32_t errCodeGeo = BatchInsertWithRetry("tab_analysis_geo_knowledge", values, rowNum);
    if (errCodeGeo != E_OK) {
        MEDIA_ERR_LOG("GeoKnowledge: RestoreMaps insert fail");
        ErrorInfo errorInfo(RestoreError::INSERT_FAILED, values.size(),
            "errCodeGeo: " + std::to_string(errCodeGeo), "GeoKnowledge: RestoreMaps insert fail");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        failInsertCnt_ += batchCnt_;
        batchCnt_ = 0;
        return;
    }
    successInsertCnt_ += batchCnt_;
    int32_t errCodeUpdate = BatchUpdate("tab_analysis_total", fileIds);
    if (errCodeUpdate != E_OK) {
        MEDIA_ERR_LOG("AnalysisTotal: RestoreMaps update fail");
        ErrorInfo errorInfo(RestoreError::UPDATE_FAILED, values.size(),
            "errCodeUpdate: " + std::to_string(errCodeUpdate), "AnalysisTotal: RestoreMaps update fail");
        UpgradeRestoreTaskReport().SetSceneCode(this->sceneCode_).SetTaskId(this->taskId_).ReportError(errorInfo);
        failUpdateCnt_ += batchCnt_;
        batchCnt_ = 0;
        return;
    }
    successUpdateCnt_ += batchCnt_;
    batchCnt_ = 0;
}

std::string GeoKnowledgeRestore::UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
    const GeoMapInfo &geoMapInfo)
{
    // no need restore or info missing
    CHECK_AND_RETURN_RET(geoMapInfo.photoInfo.fileIdNew > 0, NOT_MATCH);
    return UpdateByGeoLocation(values, geoMapInfo);
}

std::string GeoKnowledgeRestore::UpdateByGeoLocation(
    std::vector<NativeRdb::ValuesBucket> &values, const GeoMapInfo &geoMapInfo)
{
    std::string language = systemLanguage_;
    CHECK_AND_EXECUTE(!language.empty(), language = DOUBLE_CH);
    language = (language == SINGLE_EN) ? DOUBLE_EN : DOUBLE_CH;

    auto it = std::find_if(albumInfos_.begin(), albumInfos_.end(),
        [geoMapInfo, language](const GeoKnowledgeInfo& info) {
            return std::fabs(info.latitude - geoMapInfo.latitude) < 0.0001
        && std::fabs(info.longitude - geoMapInfo.longitude) < 0.0001 && info.language == language;
        });
    CHECK_AND_RETURN_RET(it != albumInfos_.end(), NOT_MATCH);
    values.push_back(GetMapInsertValue(it, geoMapInfo.photoInfo.fileIdNew));
    batchCnt_++;
    return std::to_string(geoMapInfo.photoInfo.fileIdNew);
}

NativeRdb::ValuesBucket GeoKnowledgeRestore::GetMapInsertValue(std::vector<GeoKnowledgeInfo>::iterator it,
    int32_t fileId)
{
    NativeRdb::ValuesBucket value;
    value.PutDouble(LATITUDE, it->latitude);
    value.PutDouble(LONGITUDE, it->longitude);
    value.PutLong(LOCATION_KEY, it->locationKey);
    value.PutString(COUNTRY, it->country);
    value.PutString(ADMIN_AREA, it->adminArea);
    value.PutString(SUB_ADMIN_AREA, it->subAdminArea);
    value.PutString(LOCALITY, it->locality);
    value.PutString(SUB_LOCALITY, it->subLocality);
    value.PutString(THOROUGHFARE, it->thoroughfare);
    value.PutString(SUB_THOROUGHFARE, it->subThoroughfare);
    value.PutString(FEATURE_NAME, it->featureName);
    value.PutInt(FILE_ID, fileId);
    if (it->adminArea == it->locality) {
        value.PutString(ADDRESS_DESCRIPTION, it->locality + it->subLocality + it->thoroughfare
        + it->subThoroughfare + it->featureName);
    } else {
        value.PutString(ADDRESS_DESCRIPTION, it->adminArea + it->subAdminArea + it->locality
        + it->subLocality + it->thoroughfare + it->subThoroughfare + it->featureName);
    }

    if (it->language == DOUBLE_CH) {
        value.PutString(LANGUAGE, SINGLE_CH);
        return value;
    }

    value.PutString(LANGUAGE, SINGLE_EN);
    return value;
}

int32_t GeoKnowledgeRestore::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(mediaLibraryRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(rowNum, tableName, values);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)rowNum);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}

int32_t GeoKnowledgeRestore::BatchUpdate(const std::string &tableName, std::vector<std::string> &fileIds)
{
    NativeRdb::RdbPredicates updatePredicates(tableName);
    updatePredicates.In(FILE_ID, fileIds);
    NativeRdb::ValuesBucket rdbValues;
    rdbValues.PutInt("geo", UPDATE_GEO);
    int32_t changedRows = -1;
    int32_t errCode = E_OK;
    errCode = mediaLibraryRdb_->Update(changedRows, rdbValues, updatePredicates);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "Database update failed, errCode = %{public}d", errCode);
    return errCode;
}

void GeoKnowledgeRestore::ReportGeoRestoreTask()
{
    MEDIA_INFO_LOG("GeoKnowledge Insert successInsertCnt_: %{public}d, failInsertCnt_: %{public}d, "
        "AnalysisTotal Update successUpdateCnt_: %{public}d, failUpdateCnt_: %{public}d, "
        "Current System Language: %{public}s",
        successInsertCnt_.load(), failInsertCnt_.load(), successUpdateCnt_.load(),
        failUpdateCnt_.load(), systemLanguage_.c_str());
    UpgradeRestoreTaskReport().SetSceneCode(sceneCode_).SetTaskId(taskId_)
        .Report("GeoKnowledge Restore", std::to_string(GEO_STATUS_SUCCESS),
        "successInsertCnt_: " + std::to_string(successInsertCnt_) +
        ", failInsertCnt_: " + std::to_string(failInsertCnt_) +
        ", successUpdateCnt_: " + std::to_string(successUpdateCnt_) +
        ", failUpdateCnt_: " + std::to_string(failUpdateCnt_) +
        ", Current System Language: " + systemLanguage_);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media