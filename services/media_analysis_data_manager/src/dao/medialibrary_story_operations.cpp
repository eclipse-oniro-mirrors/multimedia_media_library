/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "StoryOperation"

#include "medialibrary_story_operations.h"

#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_notify.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "story_album_column.h"
#include "vision_column_comm.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static int32_t GetHighlightId(const string &whereClause, const vector<string> &whereArgs)
{
    if (whereArgs.empty()) {
        MEDIA_ERR_LOG("whereArgs is empty");
        return E_HAS_DB_ERROR;
    }
    size_t pos = whereClause.find(ID);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause is invalid");
        return E_HAS_DB_ERROR;
    }
    size_t argsIndex = 0;
    for (size_t i = 0; i < pos; i++) {
        if (whereClause[i] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex > whereArgs.size() - 1) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return E_HAS_DB_ERROR;
    }
    auto albumId = whereArgs[argsIndex];
    if (MediaLibraryDataManagerUtils::IsNumber(albumId)) {
        return atoi(albumId.c_str());
    }
    return E_HAS_DB_ERROR;
}

static void GetHighlightAlbumId(const string &id, int32_t &albumId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");
    const std::string queryAlbumId = "SELECT album_id FROM " + HIGHLIGHT_ALBUM_TABLE + " WHERE id = " + id;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryAlbumId);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr on get highlight album_id");
    CHECK_AND_EXECUTE(resultSet->GoToNextRow() != NativeRdb::E_OK,
        albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet));
    resultSet->Close();
}

static void NotifyStoryAlbum(MediaLibraryCommand &cmd)
{
    auto whereClause = cmd.GetAbsRdbPredicates()->GetWhereClause();
    auto whereArgs = cmd.GetAbsRdbPredicates()->GetWhereArgs();
    auto id = GetHighlightId(whereClause, whereArgs);
    CHECK_AND_RETURN_LOG(id > 0, "highlight id invalid");
    int32_t albumId = 0;
    GetHighlightAlbumId(to_string(id), albumId);
    CHECK_AND_RETURN_LOG(albumId > 0, "highlight album_id invalid");
    MEDIA_INFO_LOG("NotifyStoryAlbum, album id is %{public}d", albumId);

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(
        PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
}

int32_t MediaLibraryStoryOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Story insert operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    int64_t outRowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK || outRowId < 0) {
        MEDIA_ERR_LOG("Story Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    NotifyStoryAlbum(cmd);
    return static_cast<int32_t>(outRowId);
}

int32_t MediaLibraryStoryOperations::UpdateOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Story update operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(cmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Story Update db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    NotifyStoryAlbum(cmd);
    return static_cast<int32_t>(updateRows);
}

int32_t MediaLibraryStoryOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Story delete operation, rdbStore is null.");
        return E_HAS_DB_ERROR;
    }
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    if (errCode != NativeRdb::E_OK || deleteRows < 0) {
        MEDIA_ERR_LOG("Story Delete db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(deleteRows);
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryStoryOperations::QueryOperation(MediaLibraryCommand &cmd,
    const std::vector<std::string> &columns)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Story query operation, rdbStore is null.");
        return nullptr;
    }
    return rdbStore->Query(cmd, columns);
}
} // namespace Media
} // namespace OHOS
