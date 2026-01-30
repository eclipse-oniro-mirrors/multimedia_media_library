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

#define MLOG_TAG "AccurateRefresh::AssetAccurateRefresh"

#ifndef MEDIA_REFRESH_TEST
#include "cloud_sync_helper.h"
#endif
#include <cstdint>
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "asset_accurate_refresh.h"
#include "medialibrary_notify_new.h"
#include "accurate_debug_log.h"
#include "medialibrary_tracer.h"
#include "dfx_refresh_manager.h"
#include "dfx_refresh_hander.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

mutex AssetAccurateRefresh::assetQueryMutex_;

AssetAccurateRefresh::AssetAccurateRefresh(std::shared_ptr<TransactionOperations> trans) : AccurateRefreshBase(trans)
{
    dataManager_.SetTransaction(trans);
}

AssetAccurateRefresh::AssetAccurateRefresh(const std::string &targetBusiness,
    std::shared_ptr<TransactionOperations> trans) : AccurateRefreshBase(targetBusiness, trans)
{
    dataManager_.SetTransaction(trans);
}

int32_t AssetAccurateRefresh::Init()
{
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetAccurateRefresh::Init(const AbsRdbPredicates &predicates)
{
    if (!IsValidTable(predicates.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }

    std::lock_guard<std::mutex> lock(assetQueryMutex_);
    return dataManager_.Init(predicates);
}

int32_t AssetAccurateRefresh::Init(const string &sql, const vector<ValueObject> bindArgs)
{
    std::lock_guard<std::mutex> lock(assetQueryMutex_);
    return dataManager_.Init(sql, bindArgs);
}

// 增删场景下初始化数据
int32_t AssetAccurateRefresh::Init(const vector<int32_t> &fileIds)
{
    std::lock_guard<std::mutex> lock(assetQueryMutex_);
    return dataManager_.Init(fileIds);
}

// refresh album based on init datas and modified datas.
int32_t AssetAccurateRefresh::RefreshAlbum(NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    if (dfxRefreshManager_ != nullptr) {
        albumRefreshExe_.SetDfxRefreshManager(dfxRefreshManager_);
    }
    MediaLibraryTracer tracer;
    tracer.Start("AssetAccurateRefresh::RefreshAlbum");
    if (dataManager_.CheckIsForRecheck() || dataManager_.GetIsExceedStatus()) {
        MEDIA_INFO_LOG("enter ForRecheck");
        int32_t ret = RefreshAllAlbum(dataManager_.uniqueAlbumIds_, notifyAlbumType, isRefreshWithDateModified);
        dataManager_.ClearMultiThreadChangeDatas();
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return ret;
    }
    auto assetChangeDatas = dataManager_.GetChangeDatas(true);
    if (assetChangeDatas.empty()) {
        MEDIA_WARN_LOG("change data empty.");
        dataManager_.ClearMultiThreadChangeDatas();
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return ACCURATE_REFRESH_CHANGE_DATA_EMPTY;
    }
    auto ret = RefreshAlbum(assetChangeDatas, notifyAlbumType, isRefreshWithDateModified);
    dataManager_.ClearMultiThreadChangeDatas();
    return ret;
}

// 根据传递的assetChangeDatas更新相册，不需要dataManager_处理
int32_t AssetAccurateRefresh::RefreshAlbum(const vector<PhotoAssetChangeData> &assetChangeDatas,
    NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    if (assetChangeDatas.empty()) {
        MEDIA_WARN_LOG("input asset change datas empty.");
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }
    auto diffCount = assetChangeDatas.size();
    for (const auto &assetChangeData : assetChangeDatas) {
        if (assetChangeData.infoBeforeChange_ != assetChangeData.infoAfterChange_) {
            break;
        }
        diffCount--;
    }
    if (diffCount == 0) {
        MEDIA_WARN_LOG("asset change datas are same, no need refresh album.");
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return ACCURATE_REFRESH_RET_OK;
    }
    int32_t ret = albumRefreshExe_.RefreshAlbum(assetChangeDatas, notifyAlbumType, isRefreshWithDateModified);
    return ret;
}

int32_t AssetAccurateRefresh::RefreshAlbumNoDateModified(NotifyAlbumType notifyAlbumType)
{
    return RefreshAlbum(notifyAlbumType, false);
}

// notify assest change infos based on init datas and modified datas.
int32_t AssetAccurateRefresh::Notify()
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetAccurateRefresh::Notify");
    if (dataManager_.CheckIsForRecheck()) {
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return NotifyForReCheck();
    }
    // 相册通知
    albumRefreshExe_.Notify();

    // 资产通知
    return Notify(dataManager_.GetChangeDatas());
}

int32_t AssetAccurateRefresh::NotifyYuvReady(const int32_t fileId)
{
    const std::vector<int32_t> keys = { fileId };
    // 查询当前资产数据
    int32_t ret = dataManager_.Init(keys);
    CHECK_AND_RETURN_RET_WARN_LOG(ret == ACCURATE_REFRESH_RET_OK, ret,
        "Init dataManager_ failed, err: %{public}d", ret);
    UpdateModifiedDatasInner(keys, RDB_OPERATION_UPDATE);
    ret = dataManager_.PostProcessModifiedDatas(keys);
    CHECK_AND_RETURN_RET_WARN_LOG(ret == ACCURATE_REFRESH_RET_OK, ret,
        "UpdateModifiedDatasInner failed, err: %{public}d", ret);
    PhotoAssetChangeData changeData;
    ret = dataManager_.GetChangeDataByKey(fileId, changeData);
    CHECK_AND_RETURN_RET_WARN_LOG(ret == ACCURATE_REFRESH_RET_OK, ret,
        "GetChangeDataByKey failed, err: %{public}d", ret);
    notifyExe_.NotifyYuvReady(changeData);
    return ret;
}

// 根据传递的assetChangeDatas进行通知，不需要dataManager_处理
int32_t AssetAccurateRefresh::Notify(const std::vector<PhotoAssetChangeData> &assetChangeDatas)
{
    if (assetChangeDatas.empty()) {
        MEDIA_WARN_LOG("assetChangeDatas empty.");
        DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }

    notifyExe_.Notify(assetChangeDatas);
    DfxRefreshHander::SetEndTimeHander(dfxRefreshManager_);
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetAccurateRefresh::UpdateModifiedDatasInner(const std::vector<int> &fileIds, RdbOperation operation,
    PendingInfo pendingInfo)
{
    auto modifiedFileIds = fileIds;
    if (modifiedFileIds.empty()) {
        MEDIA_WARN_LOG("input modifiedFileIds empty.");
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }

    int32_t err = ACCURATE_REFRESH_RET_OK;
    {
        std::lock_guard<std::mutex> lock(assetQueryMutex_);
        err = dataManager_.UpdateModifiedDatasInner(modifiedFileIds, operation, pendingInfo);
    }
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "UpdateModifiedDatasInner failed, err:%{public}d", err);
    err = dataManager_.PostProcessModifiedDatas(modifiedFileIds);
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "PostProcessModifiedDatas failed, err:%{public}d", err);
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetAccurateRefresh::UpdateModifiedDatas()
{
    return dataManager_.UpdateModifiedDatas();
}

string AssetAccurateRefresh::GetReturningKeyName()
{
    return PhotoColumn::MEDIA_ID;
}

int32_t AssetAccurateRefresh::LogicalDeleteReplaceByUpdate(MediaLibraryCommand &cmd, int32_t &deletedRows)
{
    if (!IsValidTable(cmd.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }
    return DeleteCommon([&](ValuesBucket &values) {
        return Update(deletedRows, values, *(cmd.GetAbsRdbPredicates()), RDB_OPERATION_REMOVE);
    });
}

int32_t AssetAccurateRefresh::LogicalDeleteReplaceByUpdate(const AbsRdbPredicates &predicates, int32_t &deletedRows)
{
    DfxRefreshHander::SetOperationStartTimeHander(dfxRefreshManager_);
    if (!IsValidTable(predicates.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }
    int32_t ret = DeleteCommon(
        [&](ValuesBucket &values) { return Update(deletedRows, values, predicates, RDB_OPERATION_REMOVE); });
    DfxRefreshHander::SetOptEndTimeHander(predicates, dfxRefreshManager_);
    return ret;
}

int32_t AssetAccurateRefresh::DeleteCommon(function<int32_t(ValuesBucket &)> updateExe)
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetAccurateRefresh::DeleteCommon");
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
    valuesBucket.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    auto ret = updateExe(valuesBucket);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->Delete failed, ret = %{public}d", ret);
        return E_HAS_DB_ERROR;
    }

#ifndef MEDIA_REFRESH_TEST
    if (!trans_) {
        CloudSyncHelper::GetInstance()->StartSync();
        ACCURATE_DEBUG("Delete update done, start sync.");
    }
#endif

    return ACCURATE_REFRESH_RET_OK;
}

bool AssetAccurateRefresh::IsValidTable(std::string tableName)
{
    return PhotoColumn::PHOTOS_TABLE == tableName;
}

int32_t AssetAccurateRefresh::SetContentChanged(int32_t fileId, bool isChanged)
{
    return dataManager_.SetContentChanged(fileId, isChanged);
}

int32_t AssetAccurateRefresh::SetThumbnailStatus(int32_t fileId, int32_t status)
{
    // 函数调用错误
    return dataManager_.SetThumbnailStatus(fileId, status);
}

bool AssetAccurateRefresh::GetIsExceedStatus()
{
    return dataManager_.GetIsExceedStatus();
}

int32_t AssetAccurateRefresh::NotifyForReCheck()
{
    Notification::NotifyInfoInner notifyInfo;
    notifyInfo.tableType = Notification::NotifyTableType::PHOTOS;
    notifyInfo.operationType = Notification::ASSET_OPERATION_RECHECK;
    Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    ACCURATE_DEBUG("asset recheck");
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetAccurateRefresh::RefreshAllAlbum(std::unordered_set<int32_t> albumIds,
    NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    return albumRefreshExe_.RefreshAllAlbum(albumIds, notifyAlbumType, isRefreshWithDateModified);
}

int32_t AssetAccurateRefresh::AddAlbumIdForMoveOperation(const AbsRdbPredicates &predicates)
{
    return dataManager_.AddAlbumIdForMoveOperation(predicates);
}

std::shared_ptr<TransactionOperations> AssetAccurateRefresh::GetTransaction()
{
    return dataManager_.GetTransaction();
}
}  // namespace Media::AccurateRefresh
}  // namespace OHOS