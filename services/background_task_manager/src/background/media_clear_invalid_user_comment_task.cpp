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

#define MLOG_TAG "Media_Background"

#include "media_clear_invalid_user_comment_task.h"

#include "rdb_predicates.h"
#include "result_set_utils.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"

using namespace std;

namespace OHOS::Media::Background {

bool MediaClearInvalidUserCommentTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaClearInvalidUserCommentTask::Execute()
{
    std::thread([this]() {
        this->ClearInvalidUserComment();
    }).detach();
}

int32_t MediaClearInvalidUserCommentTask::GetLongUserCommentCount()
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan("LENGTH(" + PhotoColumn::PHOTO_USER_COMMENT + ")", USER_COMMENT_MAX_SIZE);
    vector<string> columns = { "count(1) AS count" };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "Query failed");
    int32_t err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, 0, "Go to first row failed, err: %{public}d", err);
    const string LONG_USER_COMMENT_COUNT_COLUMN = "count";
    int32_t longUserCommentCount = GetInt32Val(LONG_USER_COMMENT_COUNT_COLUMN, resultSet);
    resultSet->Close();
    return longUserCommentCount;
}

bool MediaClearInvalidUserCommentTask::UpdateLongUserCommentsToEmpty()
{
    string sqlSetLongUserCommentToEmpty = string(SQL_SET_LONG_USER_COMMENT_TO_EMPTY);
    vector<NativeRdb::ValueObject> bindArgs = { USER_COMMENT_MAX_SIZE };

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Rdb store is nullptr");
    int32_t err = rdbStore->ExecuteSql(sqlSetLongUserCommentToEmpty, bindArgs);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false, "Update db err: %{public}d", err);
    return true;
}

bool MediaClearInvalidUserCommentTask::ClearInvalidUserComment()
{
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    CHECK_AND_RETURN_RET_INFO_LOG(lock.try_lock(), false,
        "ClearInvalidUserComment is already running, skipping this operation");
    MEDIA_INFO_LOG("Start clear invalid user comment.");
    int32_t longUserCommentCount = GetLongUserCommentCount();
    
    // 无超长备注，不处理
    CHECK_AND_RETURN_RET_INFO_LOG(longUserCommentCount > 0, true,
        "Invalid user comment count: %{public}d, do nothing.", longUserCommentCount);

    // 批量置空超长备注
    CHECK_AND_RETURN_RET_INFO_LOG(UpdateLongUserCommentsToEmpty(), false,
        "Update long user comments to empty failed.");
    MEDIA_INFO_LOG("Clear invalid user comment finished, total invalid count: %{public}d", longUserCommentCount);
    return true;
}
}
