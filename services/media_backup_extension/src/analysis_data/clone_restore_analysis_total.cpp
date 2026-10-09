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
#include "clone_restore_analysis_total.h"

#include "backup_database_utils.h"
#include "media_backup_report_data_type.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"

namespace OHOS::Media {
void CloneRestoreAnalysisTotal::Init(const std::string &type, int32_t pageSize,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::string &totalTableName)
{
    Init(std::vector<std::string>{type}, pageSize, mediaRdb, mediaLibraryRdb, totalTableName);
}

void CloneRestoreAnalysisTotal::Init(const std::vector<std::string> &types, int32_t pageSize,
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const std::string &totalTableName)
{
    pageSize_ = pageSize;
    mediaRdb_ = mediaRdb;
    mediaLibraryRdb_ = mediaLibraryRdb;
    totalTableName_ = totalTableName;
    types_ = types;
    columnsValidated_ = false;
}

void CloneRestoreAnalysisTotal::FilterValidColumns()
{
    CHECK_AND_RETURN(!columnsValidated_);
    columnsValidated_ = true;
    if (mediaRdb_ == nullptr || mediaLibraryRdb_ == nullptr) {
        return;
    }
    std::unordered_map<std::string, std::string> srcColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaRdb_, totalTableName_);
    std::unordered_map<std::string, std::string> dstColumnInfoMap =
        BackupDatabaseUtils::GetColumnInfoMap(mediaLibraryRdb_, totalTableName_);
    std::vector<std::string> validTypes;
    for (const auto &type : types_) {
        CHECK_AND_CONTINUE(srcColumnInfoMap.count(type) > 0 && dstColumnInfoMap.count(type) > 0);
        validTypes.emplace_back(type);
    }
    MEDIA_INFO_LOG("CloneRestoreAnalysisTotal::FilterValidColumns types %{public}zu -> %{public}zu",
        types_.size(), validTypes.size());
    types_.swap(validTypes);
}

int32_t CloneRestoreAnalysisTotal::GetTotalNumber()
{
    FilterValidColumns();
    if (types_.empty()) {
        totalCnt_ = 0;
        return totalCnt_;
    }
    const std::string QUERY_SQL = "SELECT count(1) as count FROM " + totalTableName_;
    totalCnt_ = BackupDatabaseUtils::QueryInt(mediaRdb_, QUERY_SQL, "count");
    return totalCnt_;
}

void CloneRestoreAnalysisTotal::GetInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap)
{
    FilterValidColumns();
    analysisTotalInfos_.clear();
    CHECK_AND_RETURN(!types_.empty());
    std::string columns = types_[0];
    for (size_t i = 1; i < types_.size(); i++) {
        columns += ", " + types_[i];
    }
    std::string querySql = "SELECT file_id, " + columns + " FROM " + totalTableName_ +
        " WHERE file_id > ? ORDER BY file_id LIMIT ?";
    std::vector<NativeRdb::ValueObject> params = { lastId_, pageSize_ };
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaRdb_, querySql, params);
    CHECK_AND_RETURN(resultSet != nullptr);

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileIdOld = GetInt32Val("file_id", resultSet);
        lastId_ = fileIdOld;
        if (photoInfoMap.count(fileIdOld) == 0) {
            MEDIA_ERR_LOG("Cannot find %{public}d", fileIdOld);
            continue;
        }
        AnalysisTotalInfo info;
        info.fileIdOld = fileIdOld;
        info.fileIdNew = photoInfoMap.at(fileIdOld).fileIdNew;
        info.status = GetInt32Val(types_.front(), resultSet);
        for (size_t i = 1; i < types_.size(); i++) {
            info.statusMap_[types_[i]] = GetInt32Val(types_[i], resultSet);
        }
        analysisTotalInfos_.emplace_back(info);
    }
    resultSet->Close();
}

void CloneRestoreAnalysisTotal::SetPlaceHoldersAndParamsByFileIdOld(std::string &placeHolders,
    std::vector<NativeRdb::ValueObject> &params)
{
    int32_t count = 0;
    for (const auto &info : analysisTotalInfos_) {
        CHECK_AND_CONTINUE(info.fileIdOld > 0);
        placeHolders += (count++ > 0 ? "," : "");
        placeHolders += "?";
        params.emplace_back(info.fileIdOld);
    }
}

void CloneRestoreAnalysisTotal::SetPlaceHoldersAndParamsByFileIdNew(std::string &placeHolders,
    std::vector<NativeRdb::ValueObject> &params)
{
    int32_t count = 0;
    for (const auto &info : analysisTotalInfos_) {
        CHECK_AND_CONTINUE(info.fileIdNew > 0);
        placeHolders += (count++ > 0 ? "," : "");
        placeHolders += "?";
        params.emplace_back(info.fileIdNew);
    }
}

size_t CloneRestoreAnalysisTotal::FindIndexByFileIdOld(int32_t fileIdOld)
{
    auto it = std::find_if(analysisTotalInfos_.begin(), analysisTotalInfos_.end(),
        [fileIdOld](const AnalysisTotalInfo &analysisTotalInfo) {
            return analysisTotalInfo.fileIdOld == fileIdOld;
        });
    return it != analysisTotalInfos_.end() ? static_cast<size_t>(std::distance(analysisTotalInfos_.begin(), it)) :
        std::string::npos;
}

int32_t CloneRestoreAnalysisTotal::GetFileIdNewByIndex(size_t index)
{
    CHECK_AND_RETURN_RET(index < analysisTotalInfos_.size(), -1);
    return analysisTotalInfos_[index].fileIdNew;
}

void CloneRestoreAnalysisTotal::UpdateRestoreStatusAsDuplicateByIndex(size_t index)
{
    CHECK_AND_RETURN(index < analysisTotalInfos_.size());
    analysisTotalInfos_[index].restoreStatus = RestoreStatus::DUPLICATE;
    duplicateCnt_++;
}

void CloneRestoreAnalysisTotal::UpdateRestoreStatusAsFailed()
{
    for (auto &info : analysisTotalInfos_) {
        info.restoreStatus = RestoreStatus::FAILED;
        failedCnt_++;
    }
}

void CloneRestoreAnalysisTotal::UpdateDatabase()
{
    FilterValidColumns();
    std::unordered_map<int32_t, std::vector<std::string>> statusFileIdsMap = GetStatusFileIdsMap();
    for (auto it : statusFileIdsMap) {
        int32_t updatedRows = UpdateDatabaseByStatus(it.first, it.second);
        successCnt_ += updatedRows;
        MEDIA_INFO_LOG("status: %{public}d, size: %{public}zu, updatedRows: %{public}d", it.first,
            it.second.size(), updatedRows);
    }
    for (size_t i = 1; i < types_.size(); i++) {
        const std::string &type = types_[i];
        std::unordered_map<int32_t, std::vector<std::string>> extraStatusFileIdsMap;
        for (const auto &info : analysisTotalInfos_) {
            if (info.restoreStatus != RestoreStatus::SUCCESS) {
                continue;
            }
            auto it = info.statusMap_.find(type);
            CHECK_AND_CONTINUE(it != info.statusMap_.end());
            extraStatusFileIdsMap[it->second].emplace_back(std::to_string(info.fileIdNew));
        }
        int32_t typeUpdatedRows = 0;
        for (auto it : extraStatusFileIdsMap) {
            int32_t updatedRows = UpdateDatabaseByStatus(type, it.first, it.second);
            typeUpdatedRows += updatedRows;
        }
        typeSuccessCnt_[type] += typeUpdatedRows;
        MEDIA_INFO_LOG("type: %{public}s, totalUpdatedRows: %{public}d, will report TOTAL",
            type.c_str(), typeUpdatedRows);
    }
    typeSuccessCnt_[types_.front()] = successCnt_;
}

std::unordered_map<int32_t, std::vector<std::string>> CloneRestoreAnalysisTotal::GetStatusFileIdsMap()
{
    std::unordered_map<int32_t, std::vector<std::string>> statusFileIdsMap;
    CHECK_AND_RETURN_RET(!types_.empty(), statusFileIdsMap);
    for (const auto &info : analysisTotalInfos_) {
        if (info.restoreStatus != RestoreStatus::SUCCESS) {
            continue;
        }
        auto &fileIds = statusFileIdsMap[info.status];
        fileIds.emplace_back(std::to_string(info.fileIdNew));
    }
    return statusFileIdsMap;
}

int32_t CloneRestoreAnalysisTotal::UpdateDatabaseByStatus(int32_t status, const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET(!fileIds.empty() && !types_.empty(), 0);
    return UpdateDatabaseByStatus(types_.front(), status, fileIds);
}

int32_t CloneRestoreAnalysisTotal::UpdateDatabaseByStatus(const std::string &type, int32_t status,
    const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET(!fileIds.empty(), 0);

    int32_t updatedRows = 0;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(type, status);
    std::unique_ptr<NativeRdb::AbsRdbPredicates> updatePredicates =
        std::make_unique<NativeRdb::AbsRdbPredicates>(totalTableName_);
    updatePredicates->In("file_id", fileIds);
    int32_t errCode = BackupDatabaseUtils::Update(mediaLibraryRdb_, updatedRows, valuesBucket, updatePredicates);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "UpdateDatabaseyStatus failed, errCode = %{public}d", errCode);
    return updatedRows;
}

void CloneRestoreAnalysisTotal::SetRestoreTaskInfo(RestoreTaskInfo &info)
{
    info.successCount = successCnt_;
    info.failedCount = failedCnt_;
    info.duplicateCount = duplicateCnt_;
}

void CloneRestoreAnalysisTotal::AddSuccessVideoFileIds()
{
    std::vector<int32_t> fields;
    for (const auto &info : analysisTotalInfos_) {
        CHECK_AND_CONTINUE(info.restoreStatus == RestoreStatus::SUCCESS && info.fileIdNew > 0);
        fields.emplace_back(info.fileIdNew);
    }
    CHECK_AND_RETURN(!fields.empty());

    std::stringstream querySql;
    querySql << "SELECT file_id FROM Photos WHERE file_id IN (" << BackupDatabaseUtils::JoinSQLValues(fields, ",")
             << ") AND media_type = 2";
    auto resultSet = BackupDatabaseUtils::QuerySql(mediaLibraryRdb_, querySql.str());
    CHECK_AND_RETURN(resultSet != nullptr);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t field = GetInt32Val("file_id", resultSet);
        successVideoFileIds_.emplace_back(field);
    }
}

std::vector<int32_t> CloneRestoreAnalysisTotal::GetSuccessVideoFileIds()
{
    return successVideoFileIds_;
}

const std::vector<std::string>& CloneRestoreAnalysisTotal::GetTypes() const
{
    return types_;
}

const std::unordered_map<std::string, int32_t>& CloneRestoreAnalysisTotal::GetTypeSuccessCnt() const
{
    return typeSuccessCnt_;
}
}