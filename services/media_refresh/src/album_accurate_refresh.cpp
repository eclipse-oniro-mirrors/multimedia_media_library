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

#define MLOG_TAG "AccurateRefresh::AlbumAccurateRefresh"
#include <cstdint>

#ifndef MEDIA_REFRESH_TEST
    #include "cloud_sync_helper.h"
#endif

#include "medialibrary_errno.h"
#include "album_accurate_refresh.h"
#include "medialibrary_notify_new.h"
#include "accurate_debug_log.h"
#include "medialibrary_tracer.h"
#include "dfx_refresh_hander.h"
#include "result_set_utils.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

AlbumAccurateRefresh::AlbumAccurateRefresh(const std::string &targetBusiness,
    std::shared_ptr<TransactionOperations> trans) : AccurateRefreshBase(targetBusiness, trans)
{
    dataManager_.SetTransaction(trans);
}

AlbumAccurateRefresh::AlbumAccurateRefresh(std::shared_ptr<TransactionOperations> trans) : AccurateRefreshBase(trans)
{
    dataManager_.SetTransaction(trans);
}

int32_t AlbumAccurateRefresh::Init()
{
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumAccurateRefresh::Init(const AbsRdbPredicates &predicates)
{
    if (!IsValidTable(predicates.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }

    return dataManager_.Init(predicates);
}

int32_t AlbumAccurateRefresh::Init(const string &sql, const vector<ValueObject> bindArgs)
{
    return dataManager_.Init(sql, bindArgs);
}

int32_t AlbumAccurateRefresh::Init(const std::vector<int32_t> &albumIds)
{
    return dataManager_.InitAlbumInfos(albumIds);
}

int32_t AlbumAccurateRefresh::Notify(std::shared_ptr<DfxRefreshManager> dfxRefreshManager)
{
    if (dataManager_.CheckIsForRecheck()) {
        return NotifyForReCheck();
    }
    return Notify(dataManager_.GetChangeDatas(), dfxRefreshManager);
}

int32_t AlbumAccurateRefresh::Notify(vector<AlbumChangeData> albumChangeDatas,
    std::shared_ptr<DfxRefreshManager> dfxRefreshManager)
{
    if (albumChangeDatas.empty()) {
        MEDIA_WARN_LOG("albumChangeDatas empty.");
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }
    notifyExe_.Notify(albumChangeDatas);
    if (dfxRefreshManager != nullptr) {
        dfxRefreshManager->SetEndTime();
    } else if (dfxRefreshManager_ != nullptr) {
        dfxRefreshManager_->SetEndTime();
    }

    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumAccurateRefresh::NotifyAddAlbums(const vector<string> &albumIdsStr)
{
    return Notify(dataManager_.GetAlbumDatasFromAddAlbum(albumIdsStr));
}

int32_t AlbumAccurateRefresh::UpdateModifiedDatasInner(const std::vector<int> &albumIds, RdbOperation operation,
    PendingInfo pendingInfo)
{
    auto modifiedAlbumIds = albumIds;
    if (modifiedAlbumIds.empty()) {
        MEDIA_WARN_LOG("modifiedAlbumIds empty.");
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }
    DfxRefreshHander::SetAlbumIdHander(albumIds, dfxRefreshManager_);
    int32_t err = dataManager_.UpdateModifiedDatasInner(modifiedAlbumIds, operation, pendingInfo);
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "UpdateModifiedDatasInner failed, err:%{public}d", err);
    err = dataManager_.PostProcessModifiedDatas(modifiedAlbumIds);
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "PostProcessModifiedDatas failed, err:%{public}d", err);
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumAccurateRefresh::UpdateModifiedDatas()
{
    return dataManager_.UpdateModifiedDatas();
}

unordered_map<int32_t, AlbumChangeInfo> AlbumAccurateRefresh::GetInitAlbumInfos()
{
    return dataManager_.GetInitAlbumInfos();
}

string AlbumAccurateRefresh::GetReturningKeyName()
{
    return PhotoAlbumColumns::ALBUM_ID;
}

int32_t AlbumAccurateRefresh::LogicalDeleteReplaceByUpdate(MediaLibraryCommand &cmd, int32_t &deletedRows)
{
    if (!IsValidTable(cmd.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }
    return DeleteCommon([&](ValuesBucket &values) {
        return Update(deletedRows, values, *(cmd.GetAbsRdbPredicates()), RDB_OPERATION_REMOVE);
    });
}

int32_t AlbumAccurateRefresh::LogicalDeleteReplaceByUpdate(const AbsRdbPredicates &predicates, int &deletedRows)
{
    DfxRefreshHander::SetOperationStartTimeHander(dfxRefreshManager_);
    if (!IsValidTable(predicates.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }
    int32_t ret = DeleteCommon([&](ValuesBucket& values) {
        return Update(deletedRows, values, predicates, RDB_OPERATION_REMOVE);
    });
    DfxRefreshHander::SetOptEndTimeHander(predicates, dfxRefreshManager_);
    return ret;
}

int32_t AlbumAccurateRefresh::DeleteCommon(function<int32_t(ValuesBucket&)> updateExe)
{
    MEDIA_INFO_LOG("DeleteCommon enter");
    MediaLibraryTracer tracer;
    tracer.Start("AlbumAccurateRefresh::DeleteCommon");
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    auto ret = updateExe(valuesBucket);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Delete failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }

    #ifndef MEDIA_REFRESH_TEST
        if (!trans_) {
            CloudSyncHelper::GetInstance()->StartSync(); // 事务场景不需要执行
            ACCURATE_DEBUG("Delete update done, start sync.");
        }
    #endif
    
    return ACCURATE_REFRESH_RET_OK;
}

void AlbumAccurateRefresh::ClearChangeInfos()
{
    dataManager_.ClearChangeInfos();
}

bool AlbumAccurateRefresh::IsValidTable(std::string tableName)
{
    return PhotoAlbumColumns::TABLE == tableName;
}

int32_t AlbumAccurateRefresh::NotifyForReCheck()
{
    Notification::NotifyInfoInner notifyInfo;
    notifyInfo.tableType = Notification::NotifyTableType::PHOTO_ALBUM;
    notifyInfo.operationType = Notification::ALBUM_OPERATION_RECHECK;
    Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    ACCURATE_DEBUG("album recheck");
    return ACCURATE_REFRESH_RET_OK;
}

bool AlbumAccurateRefresh::IsCoverContentChange(const string &fileId)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(fileId), false, "invalid input param");
    CHECK_AND_RETURN_RET_LOG(atoi(fileId.c_str()) > 0, false, "fileId is invalid");
    MEDIA_INFO_LOG("IsCoverContentChange in, fileId: %{public}s", fileId.c_str());

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::HIDDEN_COVER};
    shared_ptr<ResultSet> resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "resultSet is null!");
    int32_t rowCount = 0;
    resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(rowCount > 0, false, "result rowCount is: %{public}d", rowCount);

    vector<int32_t> albumIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID,
            resultSet, TYPE_INT32));
        string coverUri = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI,
            resultSet, TYPE_STRING));
        string hiddenCover = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::HIDDEN_COVER,
            resultSet, TYPE_STRING));
        if ((coverUri != "" && MediaFileUtils::GetIdFromUri(coverUri) == fileId)
            || (hiddenCover != "" && MediaFileUtils::GetIdFromUri(hiddenCover) == fileId)) {
            albumIds.push_back(albumId);
        }
    }
    resultSet->Close();

    if (!albumIds.empty()) {
        NotifyAlbumsCoverChange(fileId, albumIds);
        return true;
    }
    return false;
}

void AlbumAccurateRefresh::NotifyAlbumsCoverChange(const string &fileId, vector<int32_t> &albumIds)
{
    CHECK_AND_RETURN_LOG(MediaFileUtils::IsValidInteger(fileId), "invalid input param");
    CHECK_AND_RETURN_LOG(atoi(fileId.c_str()) > 0, "fileId is invalid");
    CHECK_AND_RETURN_LOG(!albumIds.empty(), "no album cover has changed");
    Init(albumIds);
    UpdateModifiedDatasInner(albumIds, RDB_OPERATION_UPDATE);
    vector<AlbumChangeData> albumChangeDatas = dataManager_.GetChangeDatas();
    for (auto &albumChangeData : albumChangeDatas) {
        if (MediaFileUtils::GetIdFromUri(albumChangeData.infoAfterChange_.coverUri_) == fileId) {
            albumChangeData.infoAfterChange_.isCoverChange_ = true;
        }
        if (MediaFileUtils::GetIdFromUri(albumChangeData.infoAfterChange_.hiddenCoverUri_) == fileId) {
            albumChangeData.infoAfterChange_.isHiddenCoverChange_ = true;
        }
    }
    Notify(albumChangeDatas);
}

vector<AlbumChangeData> AlbumAccurateRefresh::GetAlbumChangeDatas()
{
    return dataManager_.GetChangeDatas();
}

int32_t AlbumAccurateRefresh::AddAlbumIdForMoveOperation(const AbsRdbPredicates &predicates)
{
    return dataManager_.AddAlbumIdForMoveOperation(predicates);
}

} // namespace Media
} // namespace OHOS