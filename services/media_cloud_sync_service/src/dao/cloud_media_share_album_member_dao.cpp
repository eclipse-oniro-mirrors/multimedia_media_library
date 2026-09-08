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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_share_album_member_dao.h"

#include "cloud_media_sync_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaShareAlbumMemberDao::HandleAlbumMembers(
    int32_t albumId, const std::vector<ShareMemberDataDto> &members)
{
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_INVALID_VALUES, "invalid albumId %{public}d", albumId);
    CHECK_AND_RETURN_RET_LOG(!members.empty(), E_OK, "shareMember is empty. albumId: %{public}d", albumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "get rdbStore failed.");

    std::vector<NativeRdb::ValuesBucket> valuesList;
    for (const auto &member : members) {
        NativeRdb::ValuesBucket values;
        values.PutInt(ShareMemberColumn::COLUMN_ALBUM_ID, albumId);
        values.PutString(ShareMemberColumn::COLUMN_SHARE_MEMBER, member.userId);
        values.PutInt(ShareMemberColumn::COLUMN_SHARE_MEMBER_STATUS, member.status);
        valuesList.emplace_back(std::move(values));
    }

    std::shared_ptr<TransactionOperations> trans = std::make_shared<TransactionOperations>(__func__);
    std::function<int(void)> func = [&]() -> int {
        int32_t deletedRows = -1;
        int32_t ret = rdbStore->Delete(deletedRows, ShareMemberColumn::TABLE_NAME,
            ShareMemberColumn::COLUMN_ALBUM_ID + " = ?", {std::to_string(albumId)});
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_RDB, "delete old failed, ret: %{public}d", ret);
        int64_t insertRows = 0;
        ret = rdbStore->BatchInsert(insertRows, ShareMemberColumn::TABLE_NAME, valuesList);
        MEDIA_INFO_LOG("HandleAlbumMembers completed, "
                        "ret: %{public}d, deletedRows: %{public}d, insertRows: %{public}s",
            ret,
            deletedRows,
            std::to_string(insertRows).c_str());
        return ret;
    };
    return trans->RetryTrans(func);
}

int32_t CloudMediaShareAlbumMemberDao::DeleteAlbumMembers(int32_t albumId)
{
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_INVALID_VALUES, "invalid albumId %{public}d", albumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "get rdbStore failed.");

    int32_t deletedRows = -1;
    int32_t ret = rdbStore->Delete(deletedRows, ShareMemberColumn::TABLE_NAME,
        ShareMemberColumn::COLUMN_ALBUM_ID + " = ?", {std::to_string(albumId)});
    MEDIA_INFO_LOG("DeleteAlbumMembers completed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    return ret;
}
}  // namespace OHOS::Media::CloudSync
