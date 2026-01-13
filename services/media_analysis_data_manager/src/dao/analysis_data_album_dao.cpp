/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Analysis_Data_Dao"

#include "analysis_data_album_dao.h"

#include "vision_column.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "vision_face_tag_column.h"
#include "result_set_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "story_album_column.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::AnalysisData {
static constexpr int32_t HIGHLIGHT_DELETED = -3;

int32_t AnalysisDataAlbumDao::GetFaceIdByAlbumId(int32_t albumId, string& groupTag)
{
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    vector<string> columns = { GROUP_TAG };
    auto resultSet = MediaLibraryRdbStore::StepQueryWithoutCheck(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "Failed to query group tag!");
    groupTag = GetStringVal(GROUP_TAG, resultSet);
    return E_OK;
}

int32_t AnalysisDataAlbumDao::SetHighlightUserActionData(const SetHighlightUserActionDataDto& dto)
{
    RdbPredicates queryPredicates(HIGHLIGHT_ALBUM_TABLE);
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);

    vector<string> columns = { dto.userActionType };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("highlight user action data resultSet is null");
        return E_ERR;
    }
    auto count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK || count == 0 || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("highlight user action data get rdbstore failed");
        resultSet->Close();
        return E_ERR;
    }
    int64_t userActionDataCount = GetInt64Val(dto.userActionType, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("userActionDataCount: %{public}lld, dto.actionData: %{public}d",
        userActionDataCount, dto.actionData);
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(dto.userActionType, to_string(userActionDataCount + dto.actionData));
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Story update operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(updateRows, valuesBucket, queryPredicates);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Story Update db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(updateRows);
}

int32_t AnalysisDataAlbumDao::DeleteHighlightAlbums(const vector<string>& albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR,
        "rdbStore is nullptr");
    NativeRdb::RdbPredicates predicates(HIGHLIGHT_ALBUM_TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    ValuesBucket values;
    values.PutInt(HIGHLIGHT_STATUS, HIGHLIGHT_DELETED);
    int32_t changedRows = 0;
    int32_t result = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Delete highlight album failed, result is %{private}d", result);
    return changedRows;
}
} // namespace OHOS::Media::AnalysisData