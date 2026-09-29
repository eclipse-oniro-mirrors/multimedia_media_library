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

#define MLOG_TAG "PortraitFriendIdRepo"

#include "portrait_friend_id_repository.h"

#include "analysis_album_attribute_const.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "analysis_album_accurate_refresh.h"
#include "medialibrary_notify.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "vision_album_column.h"
#include "medialibrary_analysis_album_operations.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"
#include "analysis_album_accurate_refresh.h"

namespace OHOS::Media {
using namespace OHOS::NativeRdb;

PortraitFriendIdRepository::PortraitFriendIdRepository(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
    : rdbStore_(rdbStore)
{
}

static void NotifyUpdateAlbum(const vector<int32_t> &changedAlbumIds)
{
    if (changedAlbumIds.size() <= 0) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (int32_t albumId : changedAlbumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, to_string(albumId)), NotifyType::NOTIFY_UPDATE);
    }
}

static int32_t UpdateFriendIdWithAccurateRefresh(const std::vector<int32_t> &albumIds,
    const std::string &friendId)
{
    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), E_INNER_FAIL, "albumIds is empty");

    AccurateRefresh::AnalysisAlbumAccurateRefresh albumRefresh;
    int32_t ret = albumRefresh.Init(albumIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_INNER_FAIL, "UpdateFriendId init failed");

    std::string placeholders;
    std::vector<ValueObject> bindArgs;
    bindArgs.push_back(ValueObject(friendId));

    for (size_t i = 0; i < albumIds.size(); ++i) {
        if (i > 0) {
            placeholders += ",";
        }
        placeholders += "?";
        bindArgs.push_back(ValueObject(albumIds[i]));
    }

    std::string sql = "UPDATE " + ANALYSIS_ALBUM_TABLE + " SET " + FRIEND_ID + " = ? WHERE " +
                      ALBUM_ID + " IN (" + placeholders + ")";

    ret = albumRefresh.ExecuteSql(sql, bindArgs, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_INNER_FAIL, "UpdateFriendId execute sql failed");

    albumRefresh.Notify();
    return E_OK;
}

bool PortraitFriendIdRepository::Exists(const std::string &albumId) const
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, false, "rdbStore is nullptr");
    const std::string sql =
        "SELECT album_id FROM AnalysisAlbum WHERE album_id = ? AND album_type = ? AND album_subtype = ? LIMIT 1";
    std::vector<std::string> bindArgs = {
        albumId,
        std::to_string(static_cast<int32_t>(PhotoAlbumType::SMART)),
        std::to_string(static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT)),
    };
    auto resultSet = rdbStore_->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query portrait album failed");
    bool ret = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    resultSet->Close();
    return ret;
}

static int32_t GetPortraitAlbumIds(const std::string &albumId,
    std::vector<int32_t> &portraitAlbumIdsInt)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_INNER_FAIL, "uniStore is nullptr! failed query album order");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(albumId), E_INVALID_VALUES,
        "invalid albumId: %{public}s", albumId.c_str());
    const std::string queryPortraitAlbumIds = "SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = ?" + " AND " + ALBUM_SUBTYPE + " = ?" +");";
    std::vector<std::string> bindArgs = {
        albumId,
        std::to_string(static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT)),
    };
    auto resultSet = uniStore->QuerySql(queryPortraitAlbumIds, bindArgs);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_INNER_FAIL);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        portraitAlbumIdsInt.push_back(GetInt32Val(ALBUM_ID, resultSet));
    }
    resultSet->Close();
    return E_OK;
}

int32_t PortraitFriendIdRepository::UpdateFriendId(const std::string &albumId,
    const std::string &friendId) const
{
    MEDIA_INFO_LOG("UpdateFriendId start");
    std::vector<int32_t> portraitAlbumIdsInt;
    CHECK_AND_RETURN_RET_LOG(GetPortraitAlbumIds(albumId, portraitAlbumIdsInt) == E_OK,
        E_INNER_FAIL, "Failed to get portrait album ids by albumId: %{public}s", albumId.c_str());
    CHECK_AND_RETURN_RET_LOG(!portraitAlbumIdsInt.empty(),
        E_INNER_FAIL, "No portrait album found for albumId: %{public}s", albumId.c_str());

    CHECK_AND_RETURN_RET_LOG(UpdateFriendIdWithAccurateRefresh(portraitAlbumIdsInt, friendId) == E_OK,
        E_INNER_FAIL, "Failed to update friend_id for portrait album, albumId: %{public}s", albumId.c_str());

    NotifyUpdateAlbum(portraitAlbumIdsInt);
    return E_OK;
}

int32_t PortraitFriendIdRepository::GetFriendId(const int32_t &albumId,
    std::string &friendId) const
{
    MEDIA_INFO_LOG("GetFriendId start");
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(albumId));
    std::vector<std::string> fetchColumn{FRIEND_ID};

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, fetchColumn);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INNER_FAIL, "query resultSet is nullptr");

    int count = 0;
    int ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK || count <= 0) {
        friendId = "";
        resultSet->Close();
        return NativeRdb::E_OK;
    }
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        friendId = get<std::string>(ResultSetUtils::GetValFromColumn(
            FRIEND_ID, resultSet, TYPE_STRING));
    } else {
        friendId = "";
        resultSet->Close();
        return NativeRdb::E_OK;
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}
} // namespace OHOS::Media
