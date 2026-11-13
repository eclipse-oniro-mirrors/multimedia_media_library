/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AnalysisDataMetadataDao"

#include "analysis_data_metadata_dao.h"

#include <unordered_set>
#include <utility>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "value_object.h"

namespace OHOS::Media::AnalysisData {
static bool BuildAnalysisMetadataFileIdClause(const std::vector<int32_t> &fileIds, std::string &inClause,
    std::vector<NativeRdb::ValueObject> &bindArgs)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty() && fileIds.size() <= ANALYSIS_METADATA_MAX_BATCH_SIZE, false,
        "Invalid analysis metadata file id count: %{public}zu", fileIds.size());
    std::unordered_set<int32_t> seenFileIds;
    for (int32_t fileId : fileIds) {
        CHECK_AND_RETURN_RET_LOG(fileId > 0, false, "Invalid analysis metadata file id");
        if (!seenFileIds.emplace(fileId).second) {
            continue;
        }
        if (!inClause.empty()) {
            inClause += ", ";
        }
        inClause += '?';
        bindArgs.emplace_back(fileId);
    }
    CHECK_AND_RETURN_RET_LOG(!inClause.empty(), false, "Empty analysis metadata file id clause");
    return true;
}

int32_t AnalysisDataMetadataDao::GetAssetsLabel(const std::vector<int32_t> &fileIds,
    std::vector<int32_t> &resultFileIds, std::vector<std::string> &values)
{
    std::string inClause;
    std::vector<NativeRdb::ValueObject> bindArgs;
    CHECK_AND_RETURN_RET_LOG(BuildAnalysisMetadataFileIdClause(fileIds, inClause, bindArgs),
        E_INVALID_ARGUMENTS, "invalid analysis label file ids");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdbStore failed");

    const std::string querySql =
        "SELECT DISTINCT m.map_asset AS file_id, a.album_name "
        "FROM AnalysisPhotoMap m "
        "JOIN AnalysisAlbum a ON m.map_album = a.album_id "
        "WHERE a.album_type = 4096 AND a.album_subtype = 4097 "
        "AND m.map_asset IN (" + inClause + ") "
        "ORDER BY file_id, a.album_name";
    auto resultSet = rdbStore->QueryByStep(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR,
        "query analysis labels failed, resultSet is null");

    int32_t fileIdIndex = -1;
    int32_t albumNameIndex = -1;
    bool failed = resultSet->GetColumnIndex("file_id", fileIdIndex) != NativeRdb::E_OK ||
        resultSet->GetColumnIndex("album_name", albumNameIndex) != NativeRdb::E_OK;
    int32_t rowStatus = NativeRdb::E_OK;
    while (!failed && (rowStatus = resultSet->GoToNextRow()) == NativeRdb::E_OK) {
        int32_t fileId = 0;
        bool isNull = false;
        if (resultSet->GetInt(fileIdIndex, fileId) != NativeRdb::E_OK ||
            resultSet->IsColumnNull(albumNameIndex, isNull) != NativeRdb::E_OK) {
            failed = true;
            break;
        }
        if (isNull) {
            continue;
        }
        std::string albumName;
        if (resultSet->GetString(albumNameIndex, albumName) != NativeRdb::E_OK) {
            failed = true;
            break;
        }
        resultFileIds.emplace_back(fileId);
        values.emplace_back(std::move(albumName));
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(!failed && rowStatus == NativeRdb::E_ROW_OUT_RANGE,
        E_HAS_DB_ERROR, "query analysis labels result failed, status: %{public}d", rowStatus);
    MEDIA_INFO_LOG("GetAssetsLabel SQL result row count: %{public}zu", resultFileIds.size());
    return E_OK;
}

int32_t AnalysisDataMetadataDao::GetAssetsCaption(const std::vector<int32_t> &fileIds,
    std::vector<int32_t> &resultFileIds, std::vector<std::string> &values)
{
    std::string inClause;
    std::vector<NativeRdb::ValueObject> bindArgs;
    CHECK_AND_RETURN_RET_LOG(BuildAnalysisMetadataFileIdClause(fileIds, inClause, bindArgs),
        E_INVALID_ARGUMENTS, "invalid analysis caption file ids");

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "get rdbStore failed");

    const std::string querySql =
        "SELECT file_id, COALESCE(caption, '') AS caption "
        "FROM tab_analysis_caption WHERE file_id IN (" + inClause + ") ORDER BY file_id";
    auto resultSet = rdbStore->QueryByStep(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR,
        "query analysis captions failed, resultSet is null");

    int32_t fileIdIndex = -1;
    int32_t captionIndex = -1;
    bool failed = resultSet->GetColumnIndex("file_id", fileIdIndex) != NativeRdb::E_OK ||
        resultSet->GetColumnIndex("caption", captionIndex) != NativeRdb::E_OK;
    int32_t rowStatus = NativeRdb::E_OK;
    while (!failed && (rowStatus = resultSet->GoToNextRow()) == NativeRdb::E_OK) {
        int32_t fileId = 0;
        bool isNull = false;
        if (resultSet->GetInt(fileIdIndex, fileId) != NativeRdb::E_OK ||
            resultSet->IsColumnNull(captionIndex, isNull) != NativeRdb::E_OK) {
            failed = true;
            break;
        }
        std::string caption;
        if (!isNull && resultSet->GetString(captionIndex, caption) != NativeRdb::E_OK) {
            failed = true;
            break;
        }
        resultFileIds.emplace_back(fileId);
        values.emplace_back(std::move(caption));
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(!failed && rowStatus == NativeRdb::E_ROW_OUT_RANGE,
        E_HAS_DB_ERROR, "query analysis captions result failed, status: %{public}d", rowStatus);
    MEDIA_INFO_LOG("GetAssetsCaption SQL result row count: %{public}zu", resultFileIds.size());
    return E_OK;
}

} // namespace OHOS::Media::AnalysisData
