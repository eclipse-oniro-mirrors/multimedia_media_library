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

#define MLOG_TAG "AccurateRefresh::AnalysisAlbumRefreshExecution"

#include "analysis_album_refresh_execution.h"

#include <sstream>

#include "accurate_common_data.h"
#include "album_asset_helper.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

mutex AnalysisAlbumRefreshExecution::albumRefreshMtx_;

int32_t AnalysisAlbumRefreshExecution::RefreshAlbum(const vector<PhotoAssetChangeData> &assetChangeDatas,
    NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    MediaLibraryTracer tracer;
    tracer.Start("AnalysisAlbumRefreshExecution::RefreshAlbum");
    lock_guard<mutex> lock(albumRefreshMtx_);
    isRefreshWithDateModified_ = isRefreshWithDateModified;

    ResetExecutionStatus();
    int32_t ret = CalAnalysisRefreshInfos(assetChangeDatas);
    CHECK_AND_RETURN_RET_LOG(ret == ACCURATE_REFRESH_RET_OK, ret,
        "CalAnalysisRefreshInfos failed");

    CHECK_AND_RETURN_RET_INFO_LOG(!albumCtxMap_.empty(),
        ACCURATE_REFRESH_RET_OK, "RefreshAnalysisAlbum: no album needs refresh");

    return CommitAnalysisAlbumRefreshResults(notifyAlbumType);
}

int32_t AnalysisAlbumRefreshExecution::CalAnalysisRefreshInfos(const vector<PhotoAssetChangeData> &assetChangeDatas)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!assetChangeDatas.empty(),
        ACCURATE_REFRESH_RET_OK, "No asset changes. Skip analysis refresh.");

    // Step 1 & 2: 查询数据库，获取 affected fileIds 与 affectedAlbumIds
    AlbumIdSet affectedAlbumIds;
    PrepareAffectedAssets(assetChangeDatas, affectedAlbumIds);
    CHECK_AND_RETURN_RET_INFO_LOG(!affectedAlbumIds.empty(),
        ACCURATE_REFRESH_RET_OK, "No affected analysis albums found.");

    // Step 3 & 4: 加载 album 数据 -> 创建 Context
    BuildContexts(affectedAlbumIds);

    // Step 5: Phase 1 — 根据不同策略本地计算资产变更带来的变化信息，按 assetChange 聚合变化
    CalculateAlbumChanges(assetChangeDatas);

    // Step 6: Phase 2 — 查询数据库获取最新封面，Portrait group 统一计算封面并回填
    ProcessCoverChanges();

    MEDIA_DEBUG_LOG("CalAnalysisRefreshInfos finish");
    return ACCURATE_REFRESH_RET_OK;
}

void AnalysisAlbumRefreshExecution::HandleInfoRelatedShootingModeTypes(const PhotoAssetChangeInfo &info,
    AlbumIdSet &affectedAlbumIds)
{
    CHECK_AND_RETURN(info.fileId_ != INVALID_INT32_VALUE);
    vector<ShootingModeAlbumType> albumTypes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        info.subType_, info.mimeType_, info.movingPhotoEffectMode_, info.frontCamera_, info.shootingMode_);
    for (const auto &type : albumTypes) {
        int32_t albumId;
        auto it = shootingModeAlbumIdMap_.find(type);
        if (it == shootingModeAlbumIdMap_.end()) {
            CHECK_AND_CONTINUE_ERR_LOG(MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(type, albumId),
                "Query corresponding shooting mode album id failed, type: %{public}d", static_cast<int32_t>(type));
            shootingModeAlbumIdMap_.emplace(type, albumId);
        } else {
            albumId = it->second;
        }

        // 记录到 asset -> albumIds 映射
        auto &albumSet = assetAlbumMap_.try_emplace(info.fileId_, std::unordered_set<int32_t>{}).first->second;
        albumSet.insert(albumId);

        // 记录到总的 albumIds
        affectedAlbumIds.insert(albumId);
    }
}

void AnalysisAlbumRefreshExecution::PrepareAffectedAssets(const vector<PhotoAssetChangeData> &assetChangeDatas,
    AlbumIdSet &affectedAlbumIds)
{
    AlbumIdSet fileIdSet;
    for (const auto &assetChangeData : assetChangeDatas) {
        // 轮询读取所有资产对应fileId，准备查询数据库
        int32_t fileId = assetChangeData.GetFileId();
        CHECK_AND_CONTINUE(fileId != INVALID_INT32_VALUE);
        fileIdSet.insert(fileId);

        // 单独处理拍摄模式相册信息
        // 考虑最大变更场景，拍摄模式可能发生变化，分别处理修改前后资产信息
        HandleInfoRelatedShootingModeTypes(assetChangeData.infoBeforeChange_, affectedAlbumIds);
        HandleInfoRelatedShootingModeTypes(assetChangeData.infoAfterChange_, affectedAlbumIds);
    }

    vector<string> outFileIdStrs;
    outFileIdStrs.reserve(fileIdSet.size());
    for (int32_t fileId : fileIdSet) {
        outFileIdStrs.emplace_back(to_string(fileId));
    }

    CHECK_AND_PRINT_LOG(MediaLibraryRdbUtils::QueryAnalysisAlbumMapByAssets(outFileIdStrs,
        assetAlbumMap_, affectedAlbumIds) == E_OK, "QueryAnalysisAlbumMapByAssets failed");
}

void AnalysisAlbumRefreshExecution::BuildContexts(const AlbumIdSet &affectedAlbumIds)
{
    vector<UpdateAlbumData> albumDatas;
    unordered_map<string, vector<int32_t>> portraitGroupMap;

    std::vector<int32_t> vecAlbumIds(affectedAlbumIds.begin(), affectedAlbumIds.end());
    CHECK_AND_RETURN_LOG(
        MediaLibraryRdbUtils::QueryAnalysisAlbumsForAccurateRefresh(vecAlbumIds, albumDatas, portraitGroupMap) == E_OK,
        "QueryAnalysisAlbumsForAccurateRefresh failed");

    // 构造 albumContext
    for (auto &data : albumDatas) {
        AlbumContext ctx;
        ctx.baseInfo = data;
        ctx.analyzer = AnalysisStrategyRegistry::GetAnalyzer(data.albumSubtype);
        albumCtxMap_.emplace(data.albumId, move(ctx));
    }

    // 构造 Portrait group context
    for (auto &kv : portraitGroupMap) {
        const string &groupTag = kv.first;
        const vector<int32_t> &albumIds = kv.second;
        CHECK_AND_CONTINUE(!albumIds.empty());

        GroupContext gctx;
        gctx.groupTag = groupTag;
        gctx.albumIds = albumIds;

        // group 基线取该 group 中第一个相册的 baseInfo
        if (auto it = albumCtxMap_.find(albumIds.front()); it != albumCtxMap_.end()) {
            gctx.baseInfo = it->second.baseInfo;
            gctx.analyzer = it->second.analyzer;
        }

        groupCtxMap_.emplace(groupTag, move(gctx));
    }
}

void AnalysisAlbumRefreshExecution::CalculateAlbumChanges(const vector<PhotoAssetChangeData> &assetChangeDatas)
{
    for (const auto &data : assetChangeDatas) {
        int32_t fileId = data.GetFileId();
        auto it = assetAlbumMap_.find(fileId);
        CHECK_AND_CONTINUE(it != assetAlbumMap_.end());

        const AlbumIdSet &albumIds = it->second;
        unordered_set<string> visitedGroups;

        for (const auto &albumId : albumIds) {
            auto aIt = albumCtxMap_.find(albumId);
            CHECK_AND_CONTINUE_ERR_LOG(aIt != albumCtxMap_.end(), "No recorded album, id: %{public}d", albumId);

            AlbumContext &actx = aIt->second;
            auto subtype = static_cast<PhotoAlbumSubType>(actx.baseInfo.albumSubtype);

            if (subtype == PhotoAlbumSubType::PORTRAIT) {
                const string &groupTag = actx.baseInfo.groupTag;
                CHECK_AND_CONTINUE(!groupTag.empty() && visitedGroups.insert(groupTag).second);

                auto gIt = groupCtxMap_.find(groupTag);
                CHECK_AND_CONTINUE(gIt != groupCtxMap_.end() && gIt->second.analyzer != nullptr);

                int32_t delta = gIt->second.analyzer->CalcDataChange(data,
                    gIt->second.baseInfo, gIt->second.refreshInfo);
                InsertRefreshMapByDelta(fileId, gIt->second.albumIds, delta);
                continue;
            }

            // 非 portrait
            int32_t delta = actx.analyzer->CalcDataChange(data, actx.baseInfo, actx.refreshInfo);
            InsertRefreshMapByDelta(fileId, {albumId}, delta);
        }
    }
}

void AnalysisAlbumRefreshExecution::InsertRefreshMapByDelta(int32_t fileId,
    const vector<int32_t> &albumIds, int32_t delta)
{
    CHECK_AND_RETURN(fileId != INVALID_INT32_VALUE);

    // 1. 查找 fileId 是否已有节点
    auto it = assetAlbumRefreshMap_.find(fileId);
    if (it == assetAlbumRefreshMap_.end()) {
        it = assetAlbumRefreshMap_.emplace(fileId,
            std::make_pair(std::unordered_set<int32_t>{}, std::unordered_set<int32_t>{})).first;
    }

    // 2. 引用两个 set
    auto &beforeSet  = it->second.first;
    auto &afterSet  = it->second.second;

    // 3. 根据 delta 操作
    for (auto albumId : albumIds) {
        if (delta <= 0) {
            beforeSet.insert(albumId);
        }
        if (delta >= 0) {
            afterSet.insert(albumId);
        }
    }
}


void AnalysisAlbumRefreshExecution::ProcessCoverChanges()
{
    // Step 6-1：Portrait group 内统一封面计算
    for (auto &[gt, gctx] : groupCtxMap_) {
        CHECK_AND_CONTINUE_INFO_LOG(gctx.refreshInfo.HasValidRefreshInfo(),
            "No need to process cover tag: %{public}s", gctx.groupTag.c_str());
        CHECK_AND_CONTINUE_ERR_LOG(gctx.analyzer != nullptr, "Analyzer is invalid");
        gctx.analyzer->ApplyCoverChange(gctx.baseInfo, gctx.refreshInfo);

        // 将 group 结果下发给组内所有相册
        for (int32_t albumId : gctx.albumIds) {
            if (auto it = albumCtxMap_.find(albumId); it != albumCtxMap_.end()) {
                it->second.refreshInfo.MergeFromGroup(gctx.refreshInfo);
            }
        }
    }

    // Step 6-2：其它相册计算封面
    for (auto &[albumId, actx] : albumCtxMap_) {
        CHECK_AND_CONTINUE_DEBUG_LOG(actx.refreshInfo.HasValidRefreshInfo(),
            "No need to process cover albumId: %{public}d", actx.baseInfo.albumId);

        bool isPortrait =
            actx.baseInfo.albumSubtype == static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

        if (!isPortrait && actx.analyzer) {
            actx.analyzer->ApplyCoverChange(actx.baseInfo, actx.refreshInfo);
        }
    }
}

int32_t AnalysisAlbumRefreshExecution::CommitAnalysisAlbumRefreshResults(NotifyAlbumType notifyAlbumType)
{
    std::vector<BatchUpdateItem> batchItems;

    for (auto &[albumId, actx] : albumCtxMap_) {
        CHECK_AND_CONTINUE_INFO_LOG(actx.refreshInfo.deltaCount_ != 0 || actx.refreshInfo.needRefreshCover_,
            "No need to commit refresh, albumId: %{public}d", actx.baseInfo.albumId);

        string newCover = "";
        int32_t newCount = INVALID_INT32_VALUE;
        ConcludeAlbumRefreshValues(actx.baseInfo, actx.refreshInfo, newCover, newCount);

        bool needUpdateCount = newCount != actx.baseInfo.albumCount;
        bool needUpdateCover = actx.refreshInfo.needRefreshCover_ &&
            !(newCover == actx.baseInfo.albumCoverUri && newCount > 0);

        UpdateNotifyContext(actx.baseInfo.albumId, needUpdateCount || needUpdateCover);

        CHECK_AND_CONTINUE_INFO_LOG(needUpdateCount || needUpdateCover,
            "No field has changed, albumId: %{public}d", actx.baseInfo.albumId);

        MEDIA_INFO_LOG("Commit result, albumId: %{public}d, subType: %{public}d, needRefreshCover: %{public}d, "
            "count: %{public}d -> %{public}d, cover: {%{public}s} -> {%{public}s}, refreshCover: %{public}s",
            actx.baseInfo.albumId, actx.baseInfo.albumSubtype, actx.refreshInfo.needRefreshCover_,
            actx.baseInfo.albumCount, newCount, MediaFileUtils::DesensitizeUri(actx.baseInfo.albumCoverUri).c_str(),
            MediaFileUtils::DesensitizeUri(newCover).c_str(), actx.refreshInfo.refreshCover_.c_str());

        batchItems.push_back({
            albumId,
            needUpdateCount,
            needUpdateCover,
            newCount,
            newCover,
            actx.baseInfo.albumSubtype
        });

        CheckBatchCountForUpdateAndNotify(batchItems);
    }

    if (!batchItems.empty()) {
        std::string sql;
        vector<NativeRdb::ValueObject> bindArgs;
        if (AnalysisAlbumBatchUpdateHelper::BuildCaseSql(batchItems, sql, bindArgs)) {
            albumRefresh_.ExecuteSql(sql, bindArgs, RdbOperation::RDB_OPERATION_UPDATE);
        }
    }

    MEDIA_INFO_LOG("CommitAnalysisAlbumRefreshResults done. affected: %{public}zu", batchItems.size());
    return ACCURATE_REFRESH_RET_OK;
}

void AnalysisAlbumRefreshExecution::CheckBatchCountForUpdateAndNotify(std::vector<BatchUpdateItem> &batchItems)
{
    CHECK_AND_RETURN(batchItems.size() >= MAX_AFFECTED_ALBUM_LENGTH);
    
    // 完成刷新
    std::string sql;
    vector<NativeRdb::ValueObject> bindArgs;
    if (AnalysisAlbumBatchUpdateHelper::BuildCaseSql(batchItems, sql, bindArgs)) {
        albumRefresh_.ExecuteSql(sql, bindArgs, RdbOperation::RDB_OPERATION_UPDATE);
    }

    // 完成通知，与系统/用户相册刷新时通知逻辑保持一致
    vector<int32_t> albumIds;
    albumIds.reserve(MAX_AFFECTED_ALBUM_LENGTH);
    for (auto item : batchItems) {
        albumIds.emplace_back(item.albumId);
    }

    vector<AlbumChangeData> albumChangeDatas;
    albumChangeDatas.reserve(MAX_AFFECTED_ALBUM_LENGTH);
    PrepareAlbumChangeForNotify(albumChangeDatas, albumIds);
    albumRefresh_.NotifyAnalysisAlbumChange(albumChangeDatas);
    batchItems.clear();
}

void AnalysisAlbumRefreshExecution::ConcludeAlbumRefreshValues(const UpdateAlbumData &base,
    const AnalysisAlbumRefreshInfo &info, std::string &newCover, int32_t &newCount)
{
    newCount = base.albumCount + info.deltaCount_;
    if (newCount < 0) {
        // 异常场景，避免数据库刷新出现负值
        newCount = 0;
    }

    if (info.needRefreshCover_) {
        newCover = info.refreshCover_;
    }

    // 封面空但 count > 0 -> 继承旧封面
    if (newCover.empty() && newCount > 0) {
        newCover = base.albumCoverUri;
    }
}

void AnalysisAlbumRefreshExecution::UpdateNotifyContext(int32_t albumId, bool needNotify)
{
    auto iter = albumNotifyCtxMap_.find(albumId);
    if (iter != albumNotifyCtxMap_.end()) {
        MEDIA_INFO_LOG("Duplicate album, refresh notify context");
        iter->second.needNotify = needNotify;
    } else {
        NotifyContext ctx;
        ctx.needNotify = needNotify;
        albumNotifyCtxMap_.emplace(albumId, std::move(ctx));
    }
}

void AnalysisAlbumRefreshExecution::RefreshAllAlbum(const vector<string> &albumIdList)
{
    MEDIA_INFO_LOG("force update all analysis albums");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null");
    AccurateRefresh::AnalysisAlbumAccurateRefresh albumRefresh;
    vector<int32_t> affectedAlbumIds;
    if (albumIdList.size() < MAX_ANALYSIS_ALBUM_OPERATION_SIZE) {
        // 1. 构造受影响的albumId
        for (auto albumIdStr : albumIdList) {
            CHECK_AND_CONTINUE(MediaFileUtils::IsValidInteger(albumIdStr));
            affectedAlbumIds.emplace_back(std::stoi(albumIdStr));
        }

        // 2. 初始化精准刷新上下文
        albumRefresh.Init(affectedAlbumIds);
    }

    // 3. 执行全量查询刷新
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdList);

    // 4. 达到MAX_ANALYSIS_ALBUM_OPERATION_SIZE时发送全查通知
    if (albumIdList.size() < MAX_ANALYSIS_ALBUM_OPERATION_SIZE) {
        albumRefresh.GenerateDataAfterCustomizedUpdate(affectedAlbumIds);
        albumRefresh.Notify();
        MEDIA_INFO_LOG("refresh analysis album batch: %{public}zu", albumIdList.size());
    } else {
        albumRefresh.NotifyForAnalysisAlbumReCheck();
    }
}

void AnalysisAlbumRefreshExecution::ResetExecutionStatus()
{
    MEDIA_INFO_LOG("AnalysisAlbumRefreshExecution reset status");
    assetAlbumMap_.clear();
    assetAlbumRefreshMap_.clear();
    albumCtxMap_.clear();
    groupCtxMap_.clear();
    albumNotifyCtxMap_.clear();
}

void AnalysisAlbumRefreshExecution::Notify(vector<PhotoAssetChangeData> &assetChangeDatas)
{
    // 通知行为与刷新行为不能同时发生，因为通知后将清空当前缓存的计算数据
    lock_guard<mutex> lock(albumRefreshMtx_);
    MEDIA_INFO_LOG("AnalysisAlbumRefreshExecution Notify, datas size: %{public}zu", assetChangeDatas.size());
    MediaLibraryTracer tracer;
    tracer.Start("AnalysisAlbumRefreshExecution::Notify");
    
    // 对于普通资产变更引发的变更，需要接收来自asset_accurate_refresh的数据填补智慧信息后进行通知
    vector<PhotoAssetChangeData> preparedAssetChangeDatas;
    preparedAssetChangeDatas.reserve(assetChangeDatas.size());
    PreparePhotoChangeForNotify(assetChangeDatas, preparedAssetChangeDatas);
    albumRefresh_.NotifyAnalysisPhotoChange(preparedAssetChangeDatas);

    vector<AlbumChangeData> albumChangeDatas;
    PrepareAlbumChangeForNotify(albumChangeDatas);
    albumRefresh_.NotifyAnalysisAlbumChange(albumChangeDatas);

    albumNotifyCtxMap_.clear();
}

void AnalysisAlbumRefreshExecution::NotifyAssetForReCheck()
{
    albumRefresh_.NotifyForAnalysisAssetReCheck();
}

void AnalysisAlbumRefreshExecution::PreparePhotoChangeForNotify(vector<PhotoAssetChangeData> &assetChangeDatas,
    vector<PhotoAssetChangeData> &preparedAssetChangeDatas)
{
    preparedAssetChangeDatas.clear();
    for (auto &data : assetChangeDatas) {
        int32_t fileId = data.GetFileId();
        CHECK_AND_CONTINUE(fileId != INVALID_INT32_VALUE);
        auto it = assetAlbumRefreshMap_.find(fileId);
        CHECK_AND_CONTINUE_ERR_LOG(it != assetAlbumRefreshMap_.end(), "No refresh info, fileId: %{public}d", fileId);

        const auto &beforeSet = it->second.first;
        PrepareAlbumChangeInfos(beforeSet, false, data.infoBeforeChange_.albumChangeInfos_);
        const auto &afterSet = it->second.second;
        PrepareAlbumChangeInfos(afterSet, true, data.infoAfterChange_.albumChangeInfos_);

        bool hasValidAnalysisAlbumChange = !data.infoBeforeChange_.albumChangeInfos_.empty() ||
            !data.infoAfterChange_.albumChangeInfos_.empty();
        bool hasValidAsset = AlbumAssetHelper::IsCommonSystemAsset(data.infoBeforeChange_, false) ||
            AlbumAssetHelper::IsCommonSystemAsset(data.infoAfterChange_, false);
        if (hasValidAnalysisAlbumChange && hasValidAsset) {
            preparedAssetChangeDatas.emplace_back(data);
        }
    }
}

void AnalysisAlbumRefreshExecution::PrepareAlbumChangeInfos(const AlbumIdSet &albumIds,
    bool isAfterChange, std::vector<std::shared_ptr<AlbumChangeInfo>> &albumChangeInfos)
{
    CHECK_AND_RETURN_LOG(!albumIds.empty(), "No related album to be handled, isAfterChange: %{public}d", isAfterChange);
    albumChangeInfos.reserve(albumIds.size());
    for (const auto &albumId : albumIds) {
        AlbumChangeInfo info = isAfterChange ?
            GenerateAlbumChangeInfoAfterChange(albumId) : GenerateAlbumChangeInfoBeforeChange(albumId);
        CHECK_AND_CONTINUE_ERR_LOG(info.albumId_ != INVALID_INT32_VALUE,
            "No invalid info, albumId: %{public}d", albumId);
        albumChangeInfos.emplace_back(make_shared<AlbumChangeInfo>(info));
    }
}

AlbumChangeInfo AnalysisAlbumRefreshExecution::GenerateAlbumChangeInfoBase(
    int32_t albumId, const UpdateAlbumData &baseInfo)
{
    AlbumChangeInfo info;
    info.albumId_ = albumId;
    info.albumType_ = PhotoAlbumType::SMART;
    info.albumSubType_ = baseInfo.albumSubtype;
    info.albumName_ = baseInfo.albumName;
    info.albumUri_ = MediaFileUtils::GetUriByExtrConditions(
        PhotoAlbumColumns::ALBUM_URI_PREFIX, std::to_string(albumId));
    return info;
}

AlbumChangeInfo AnalysisAlbumRefreshExecution::GenerateAlbumChangeInfoBeforeChange(int32_t albumId)
{
    AlbumChangeInfo info;
    CHECK_AND_RETURN_RET_LOG(CheckAlbumNotifyStatus(albumId), info, "Check failed");

    auto &ctx = albumCtxMap_[albumId];
    UpdateAlbumData &baseInfo = ctx.baseInfo;
    info = GenerateAlbumChangeInfoBase(albumId, baseInfo);
    info.count_ = baseInfo.albumCount;
    info.coverUri_ = baseInfo.albumCoverUri;
    info.isCoverChange_ = false;
    return info;
}

AlbumChangeInfo AnalysisAlbumRefreshExecution::GenerateAlbumChangeInfoAfterChange(int32_t albumId)
{
    AlbumChangeInfo info;
    CHECK_AND_RETURN_RET_DEBUG_LOG(CheckAlbumNotifyStatus(albumId), info, "Check failed");

    auto &ctx = albumCtxMap_[albumId];
    UpdateAlbumData &baseInfo = ctx.baseInfo;
    AnalysisAlbumRefreshInfo &refreshInfo = ctx.refreshInfo;
    info = GenerateAlbumChangeInfoBase(albumId, baseInfo);
    info.count_ = baseInfo.albumCount + refreshInfo.deltaCount_;
    if (info.count_ < 0) {
        info.count_ = 0;
    }
    info.coverUri_ = refreshInfo.needRefreshCover_ ? refreshInfo.refreshCover_ : baseInfo.albumCoverUri;
    info.isCoverChange_ = (baseInfo.albumCoverUri != refreshInfo.refreshCover_);
    return info;
}


bool AnalysisAlbumRefreshExecution::CheckAlbumNotifyStatus(int32_t albumId)
{
    auto iter = albumCtxMap_.find(albumId);
    CHECK_AND_RETURN_RET_LOG(iter != albumCtxMap_.end(), false,
        "No valid album detail recorded, albumId: %{public}d", albumId);

    auto notifyMapIter = albumNotifyCtxMap_.find(albumId);
    CHECK_AND_RETURN_RET_DEBUG_LOG(notifyMapIter != albumNotifyCtxMap_.end(), false,
        "No valid notify detail recorded, albumId: %{public}d", albumId);

    CHECK_AND_RETURN_RET_LOG(notifyMapIter->second.needNotify, false,
        "No need to notify, albumId: %{public}d", albumId);

    return true;
}

void AnalysisAlbumRefreshExecution::PrepareAlbumChangeForNotify(vector<AlbumChangeData> &albumChangeDatas,
    const vector<int32_t> &albumIds)
{
    if (!albumIds.empty()) {
        for (auto albumId : albumIds) {
            ProcessAlbumForNotify(albumId, albumChangeDatas);
        }
    } else {
        for (auto &[albumId, actx] : albumCtxMap_) {
            ProcessAlbumForNotify(albumId, albumChangeDatas);
        }
    }
}

void AnalysisAlbumRefreshExecution::ProcessAlbumForNotify(int32_t albumId, vector<AlbumChangeData> &albumChangeDatas)
{
    auto iter = albumCtxMap_.find(albumId);
    CHECK_AND_RETURN_INFO_LOG(iter != albumCtxMap_.end(),
        "No valid album detail recorded, albumId: %{public}d", albumId);

    auto notifyMapIter = albumNotifyCtxMap_.find(albumId);
    CHECK_AND_RETURN_DEBUG_LOG(notifyMapIter != albumNotifyCtxMap_.end(),
        "No valid notify detail recorded, albumId: %{public}d", albumId);

    CHECK_AND_RETURN(!notifyMapIter->second.hasNotified);

    if (!notifyMapIter->second.needNotify) {
        notifyMapIter->second.hasNotified = true;
        return;
    }

    AlbumChangeData data;
    data.infoBeforeChange_ = GenerateAlbumChangeInfoBeforeChange(albumId);
    data.infoAfterChange_  = GenerateAlbumChangeInfoAfterChange(albumId);

    bool noCountChange = data.infoBeforeChange_.count_ == data.infoAfterChange_.count_;
    bool noCoverChange = data.infoBeforeChange_.coverUri_ == data.infoAfterChange_.coverUri_;
    if (noCountChange && noCoverChange) {
        notifyMapIter->second.hasNotified = true;
        return;
    }

    // analysis album 默认操作类型为更新，预期在操作数据库前已存在对应智慧相册
    data.operation_ = RDB_OPERATION_UPDATE;
    data.version_ = MediaFileUtils::UTCTimeMilliSeconds();
    albumChangeDatas.emplace_back(std::move(data));
    notifyMapIter->second.hasNotified = true;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
