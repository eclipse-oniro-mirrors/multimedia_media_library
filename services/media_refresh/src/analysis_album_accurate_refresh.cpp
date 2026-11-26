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

#define MLOG_TAG "AccurateRefresh::AnalysisAlbumAccurateRefresh"
#include "analysis_album_accurate_refresh.h"

#include <cstdint>

#include "analysis_strategy_registry.h"
#include "media_string_utils.h"
#include "analysis_album_pipeline.h"
#include "analysis_analyzer_context.h"
#include "accurate_debug_log.h"
#include "dfx_refresh_hander.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify_new.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "vision_column.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

int32_t AnalysisAlbumAccurateRefresh::Init()
{
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AnalysisAlbumAccurateRefresh::Init(const AbsRdbPredicates &predicates)
{
    if (!IsValidTable(predicates.GetTableName())) {
        return ACCURATE_REFRESH_RDB_INVALITD_TABLE;
    }
    return dataManager_.Init(predicates);
}

int32_t AnalysisAlbumAccurateRefresh::Init(const string &sql, const vector<ValueObject> bindArgs)
{
    return dataManager_.Init(sql, bindArgs);
}

int32_t AnalysisAlbumAccurateRefresh::Init(const vector<int32_t> &albumIds)
{
    return dataManager_.Init(albumIds);
}

void AnalysisAlbumAccurateRefresh::InitForAdd(const vector<int> &albumIds)
{
    Init(albumIds);
    UpdateModifiedDatasInner(albumIds, RdbOperation::RDB_OPERATION_ADD);
}

void AnalysisAlbumAccurateRefresh::InitForRemove(const vector<int> &albumIds)
{
    Init(albumIds);
    UpdateModifiedDatasInner(albumIds, RdbOperation::RDB_OPERATION_REMOVE);
}

void AnalysisAlbumAccurateRefresh::GenerateDataAfterCustomizedUpdate(const vector<int> &albumIds)
{
    UpdateModifiedDatasInner(albumIds, RdbOperation::RDB_OPERATION_UPDATE);
}

string AnalysisAlbumAccurateRefresh::GetReturningKeyName()
{
    return PhotoAlbumColumns::ALBUM_ID;
}

int32_t AnalysisAlbumAccurateRefresh::CustomUpdateAlbumsWithDeltaCount(int32_t deltaCount,
    const vector<string> &updateAlbumIds, const vector<string> &deletedAssetIds)
{
    if (updateAlbumIds.empty()) {
        MEDIA_WARN_LOG("No album to update");
        return E_OK;
    }

    // 1. Init
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ACCURATE_REFRESH_RDB_NULL, "rdbStore null");
    vector<string> tempAlbumIds = updateAlbumIds;
    MediaLibraryRdbUtils::GetAlbumIdsForPortrait(rdbStore, tempAlbumIds);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, tempAlbumIds);
    if (InitBeforeUpdate(predicates) != E_OK) {
        MEDIA_ERR_LOG("InitBeforeUpdate failed");
        return E_HAS_DB_ERROR;
    }

    // 2. 计算变化并生成 BatchUpdateItem
    vector<BatchUpdateItem> items;
    BuildRefreshItems(deltaCount, tempAlbumIds, deletedAssetIds, items);
    if (items.empty()) return E_OK;

    // 3. 生成 SQL 并执行
    string sql;
    vector<NativeRdb::ValueObject> bindArgs;
    if (!AnalysisAlbumBatchUpdateHelper::BuildCaseSql(items, sql, bindArgs)) {
        MEDIA_ERR_LOG("BuildCaseSql failed");
        return ACCURATE_REFRESH_BUILD_SQL_ERR;
    }
    return AccurateRefreshBase::ExecuteSql(sql, bindArgs, RdbOperation::RDB_OPERATION_UPDATE);
}

void AnalysisAlbumAccurateRefresh::BuildRefreshItems(int32_t deltaCount, const vector<string> &updateAlbumIds,
    const vector<string> &deletedAssetIds, vector<BatchUpdateItem> &items)
{
    auto changeDatas = dataManager_.GetChangeDatas();
    items.reserve(changeDatas.size());

    for (auto &changeData : changeDatas) {
        UpdateAlbumData base;
        base.albumId = changeData.infoBeforeChange_.albumId_;
        base.albumSubtype = changeData.infoBeforeChange_.albumSubType_;
        base.albumCount = changeData.infoBeforeChange_.count_;
        base.albumCoverUri = changeData.infoBeforeChange_.coverUri_;

        AnalysisAlbumRefreshInfo refresh;
        refresh.deltaCount_ = deltaCount;
        for (auto &idStr : deletedAssetIds) {
            CHECK_AND_CONTINUE(MediaStringUtils::IsValidInteger(idStr));
            refresh.removeFileIds_.emplace(std::stoi(idStr));
        }

        auto analyzer = AnalysisStrategyRegistry::GetAnalyzer(base.albumSubtype);
        CHECK_AND_CONTINUE(analyzer != nullptr);

        auto ctxOpt = AnalysisAnalyzerContextBuilder{}
            .SetBaseInfo(base)
            .SetRefreshInfo(refresh)
            .Build();

        CHECK_AND_CONTINUE(ctxOpt.has_value());

        analyzer->Analyze(*ctxOpt, PipelineFlow::ApplyCoverChangePhase());

        BatchUpdateItem item;
        item.albumId = base.albumId;
        item.albumSubType = base.albumSubtype;

        item.shouldUpdateCount = deltaCount != 0;
        if (item.shouldUpdateCount) {
            item.newCount = base.albumCount + deltaCount;
        }

        if (ctxOpt->NeedCoverRefresh()) {
            item.shouldUpdateCover = true;
            item.newCover = refresh.refreshCover_;
        }

        items.emplace_back(item);
    }
}

void AnalysisAlbumAccurateRefresh::Notify()
{
    if (dataManager_.CheckIsForRecheck()) {
        return NotifyForAnalysisAlbumReCheck();
    }
    auto albumChangeDatas = dataManager_.GetChangeDatas();
    CHECK_AND_RETURN_WARN_LOG(!albumChangeDatas.empty(), "albumChangeDatas is empty");
    notifyExe_.Notify(albumChangeDatas);
}

// 当前刷新中若存在纯智慧资产变更，则可以调用本接口发送类型为NOTIFY_CHANGE_ADD_ANALYSIS/NOTIFY_CHANGE_REMOVE_ANALYSIS的通知
void AnalysisAlbumAccurateRefresh::NotifyAnalysisAssetChange(const vector<int32_t> &assetIds, RdbOperation operation)
{
    auto albumChangeDatas = dataManager_.GetChangeDatas();
    CHECK_AND_RETURN_WARN_LOG(!albumChangeDatas.empty(), "albumChangeDatas is empty");
    vector<shared_ptr<AlbumChangeInfo>> albumChangeInfosBefore;
    vector<shared_ptr<AlbumChangeInfo>> albumChangeInfosAfter;
    for (const auto &data : albumChangeDatas) {
        albumChangeInfosBefore.emplace_back(make_shared<AlbumChangeInfo>(data.infoBeforeChange_));
        albumChangeInfosAfter.emplace_back(make_shared<AlbumChangeInfo>(data.infoAfterChange_));
    }

    assetDataManager_.Init(assetIds);
    vector<PhotoAssetChangeData> assetChangeDatas = assetDataManager_.GetChangeDatas();
    CHECK_AND_RETURN_WARN_LOG(!assetChangeDatas.empty(), "assetChangeDatas is empty");
    for (auto &data : assetChangeDatas) {
        data.operation_ = operation;
        data.infoBeforeChange_.albumChangeInfos_ = albumChangeInfosBefore;
        data.infoAfterChange_ = data.infoBeforeChange_;
        data.infoAfterChange_.albumChangeInfos_ = albumChangeInfosAfter;
        data.version_ =  MediaFileUtils::UTCTimeMilliSeconds();
    }
    notifyExe_.Notify(assetChangeDatas);
}

void AnalysisAlbumAccurateRefresh::NotifyForAnalysisAssetReCheck()
{
    Notification::NotifyInfoInner notifyInfo;
    notifyInfo.tableType = Notification::NotifyTableType::PHOTOS;
    notifyInfo.operationType = Notification::ANALYSIS_ASSET_OPERATION_RECHECK;
    Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    ACCURATE_DEBUG("Analysis asset recheck");
}

void AnalysisAlbumAccurateRefresh::NotifyForAnalysisAlbumReCheck()
{
    Notification::NotifyInfoInner notifyInfo;
    notifyInfo.tableType = Notification::NotifyTableType::ANALYSIS_ALBUM;
    notifyInfo.operationType = Notification::ANALYSIS_ALBUM_OPERATION_RECHECK;
    Notification::MediaLibraryNotifyNew::AddItem(notifyInfo);
    ACCURATE_DEBUG("Analysis album recheck");
}

void AnalysisAlbumAccurateRefresh::NotifyAnalysisPhotoChange(vector<PhotoAssetChangeData> &assetChangeDatas)
{
    CHECK_AND_RETURN_WARN_LOG(!assetChangeDatas.empty(), "assetChangeDatas is empty");
    notifyExe_.Notify(assetChangeDatas);
}

void AnalysisAlbumAccurateRefresh::NotifyAnalysisAlbumChange(vector<AlbumChangeData> &albumChangeDatas)
{
    CHECK_AND_RETURN_WARN_LOG(!albumChangeDatas.empty(), "albumChangeDatas is empty");
    notifyExe_.Notify(albumChangeDatas);
}

bool AnalysisAlbumAccurateRefresh::IsValidTable(string tableName)
{
    return ANALYSIS_ALBUM_TABLE == tableName;
}

int32_t AnalysisAlbumAccurateRefresh::UpdateModifiedDatasInner(const vector<int> &albumIds, RdbOperation operation,
    PendingInfo pendingInfo)
{
    auto modifiedAlbumIds = albumIds;
    if (modifiedAlbumIds.empty()) {
        MEDIA_WARN_LOG("modifiedAlbumIds empty.");
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }
    int32_t err = dataManager_.UpdateModifiedDatasInner(modifiedAlbumIds, operation, pendingInfo);
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "UpdateModifiedDatasInner failed, err:%{public}d", err);
    err = dataManager_.PostProcessModifiedDatas(modifiedAlbumIds);
    CHECK_AND_RETURN_RET_WARN_LOG(err == ACCURATE_REFRESH_RET_OK, err,
        "PostProcessModifiedDatas failed, err:%{public}d", err);
    return ACCURATE_REFRESH_RET_OK;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
