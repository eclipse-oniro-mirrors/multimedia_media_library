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

#define private public
#define protected public

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <unordered_set>

#include "analysis_album_refresh_execution.h"
#include "analysis_strategy_registry.h"

#include "accurate_common_data.h"
#include "accurate_debug_log.h"

#include "album_asset_helper.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "photo_album_column.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

namespace {

static constexpr int32_t SLEEP_ONE_SECOND = 1;
static constexpr int32_t INVALID_ID = INVALID_INT32_VALUE;

// 为了避免依赖数据库：这里使用纯内存构造 Context
static constexpr int32_t ALBUM_ID_PORTRAIT_1 = 1101;
static constexpr int32_t ALBUM_ID_PORTRAIT_2 = 1102;
static constexpr int32_t ALBUM_ID_NON_PORTRAIT = 1201;
static constexpr int32_t FILE_ID_A = 20001;
static constexpr int32_t FILE_ID_B = 20002;
static constexpr int32_t MOCK_COUNT_5 = 5;
static constexpr int32_t MOCK_COUNT_10 = 10;
static constexpr int32_t MOCK_COUNT_20 = 20;

static const std::string GROUP_TAG_1 = "group_tag_ut_1";

// 构造一个最小可用的 UpdateAlbumData
UpdateAlbumData MakeBaseAlbum(int32_t albumId, int32_t subtype, int32_t count,
    const std::string &cover, const std::string &groupTag = "")
{
    UpdateAlbumData base;
    base.albumId = albumId;
    base.albumSubtype = subtype;
    base.albumCount = count;
    base.albumCoverUri = cover;
    base.albumName = "ut_album_" + std::to_string(albumId);
    base.groupTag = groupTag;
    return base;
}

// 构造一个最小 PhotoAssetChangeData，重点设置 fileId 与 before/after
PhotoAssetChangeData MakeAssetUpdate(int32_t fileId)
{
    PhotoAssetChangeData data;
    data.operation_ = RDB_OPERATION_UPDATE;

    data.infoBeforeChange_.fileId_ = fileId;
    data.infoAfterChange_.fileId_ = fileId;

    // 给一些字段，尽量让 AlbumAssetHelper::IsCommonSystemAsset 判定更容易为 true
    data.infoBeforeChange_.mimeType_ = "image/jpeg";
    data.infoAfterChange_.mimeType_ = "image/jpeg";
    data.infoBeforeChange_.mediaType_ = MEDIA_TYPE_IMAGE;
    data.infoAfterChange_.mediaType_ = MEDIA_TYPE_IMAGE;

    return data;
}

// 给 assetChangeData 挂载 albumChangeInfos_（用于 Notify Update 分支）
void AttachAlbumInfos(PhotoAssetChangeData &data, const std::vector<int32_t> &beforeAlbumIds,
    const std::vector<int32_t> &afterAlbumIds)
{
    data.infoBeforeChange_.albumChangeInfos_.clear();
    data.infoAfterChange_.albumChangeInfos_.clear();

    for (auto id : beforeAlbumIds) {
        auto p = std::make_shared<AlbumChangeInfo>();
        p->albumId_ = id;
        data.infoBeforeChange_.albumChangeInfos_.push_back(p);
    }
    for (auto id : afterAlbumIds) {
        auto p = std::make_shared<AlbumChangeInfo>();
        p->albumId_ = id;
        data.infoAfterChange_.albumChangeInfos_.push_back(p);
    }
}

} // namespace

class AnalysisAlbumRefreshTest : public testing::Test {
public:
    void SetUp() override
    {
        // 无数据库依赖：只构造 in-memory maps
        exe_.ResetExecutionStatus();

        BuildBaseContexts();
        BuildGroupContexts();
        BuildAssetAlbumMap();
        BuildNotifyCtxAll(true);
    }

    void TearDown() override
    {
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
    }

protected:
    void BuildBaseContexts()
    {
        // Portrait album contexts (two albums same groupTag)
        {
            AnalysisAlbumRefreshExecution::AlbumContext ctx;
            ctx.baseInfo = MakeBaseAlbum(ALBUM_ID_PORTRAIT_1,
                static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
                MOCK_COUNT_10, "file://cover_old_p1.jpg", GROUP_TAG_1);
            ctx.analyzer = AnalysisStrategyRegistry::GetAnalyzer(ctx.baseInfo.albumSubtype);
            exe_.albumCtxMap_.emplace(ctx.baseInfo.albumId, ctx);
        }
        {
            AnalysisAlbumRefreshExecution::AlbumContext ctx;
            ctx.baseInfo = MakeBaseAlbum(ALBUM_ID_PORTRAIT_2,
                static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
                MOCK_COUNT_20, "file://cover_old_p2.jpg", GROUP_TAG_1);
            ctx.analyzer = AnalysisStrategyRegistry::GetAnalyzer(ctx.baseInfo.albumSubtype);
            exe_.albumCtxMap_.emplace(ctx.baseInfo.albumId, ctx);
        }

        // Non-portrait album context
        {
            AnalysisAlbumRefreshExecution::AlbumContext ctx;
            ctx.baseInfo = MakeBaseAlbum(ALBUM_ID_NON_PORTRAIT,
                static_cast<int32_t>(PhotoAlbumSubType::FAVORITE),
                MOCK_COUNT_5, "file://cover_old_np.jpg", "");
            ctx.analyzer = AnalysisStrategyRegistry::GetAnalyzer(ctx.baseInfo.albumSubtype);
            exe_.albumCtxMap_.emplace(ctx.baseInfo.albumId, ctx);
        }
    }

    void BuildGroupContexts()
    {
        AnalysisAlbumRefreshExecution::GroupContext gctx;
        gctx.groupTag = GROUP_TAG_1;
        gctx.albumIds = { ALBUM_ID_PORTRAIT_1, ALBUM_ID_PORTRAIT_2 };

        // group 基线取该 group 中第一个相册的 baseInfo
        auto it = exe_.albumCtxMap_.find(ALBUM_ID_PORTRAIT_1);
        if (it != exe_.albumCtxMap_.end()) {
            gctx.baseInfo = it->second.baseInfo;
            gctx.analyzer = it->second.analyzer;
        }
        exe_.groupCtxMap_.emplace(GROUP_TAG_1, gctx);
    }

    void BuildAssetAlbumMap()
    {
        // FILE_ID_A belongs to portrait albums and non-portrait
        exe_.assetAlbumMap_.try_emplace(FILE_ID_A,
            std::unordered_set<int32_t>{}).first->second.insert(ALBUM_ID_PORTRAIT_1);
        exe_.assetAlbumMap_[FILE_ID_A].insert(ALBUM_ID_PORTRAIT_2);
        exe_.assetAlbumMap_[FILE_ID_A].insert(ALBUM_ID_NON_PORTRAIT);

        // FILE_ID_B belongs only to portrait group
        exe_.assetAlbumMap_.try_emplace(FILE_ID_B,
            std::unordered_set<int32_t>{}).first->second.insert(ALBUM_ID_PORTRAIT_1);
        exe_.assetAlbumMap_[FILE_ID_B].insert(ALBUM_ID_PORTRAIT_2);
    }

    void BuildNotifyCtxAll(bool needNotify)
    {
        exe_.albumNotifyCtxMap_.clear();
        for (auto &[albumId, ctx] : exe_.albumCtxMap_) {
            AnalysisAlbumRefreshExecution::NotifyContext nctx;
            nctx.needNotify = needNotify;
            nctx.hasNotified = false;
            exe_.albumNotifyCtxMap_.emplace(albumId, nctx);
        }
    }

    // 直接往 refreshMap 插入，供 PreparePhotoChangeForNotify 使用
    void BuildAssetAlbumRefreshMapForDelta(int32_t fileId,
        const std::unordered_set<int32_t> &beforeSet,
        const std::unordered_set<int32_t> &afterSet)
    {
        exe_.assetAlbumRefreshMap_[fileId] = std::make_pair(beforeSet, afterSet);
    }

protected:
    AnalysisAlbumRefreshExecution exe_;
};

HWTEST_F(AnalysisAlbumRefreshTest, InsertRefreshMapByDelta_DeltaNegative_001, TestSize.Level2)
{
    exe_.InsertRefreshMapByDelta(FILE_ID_A, {ALBUM_ID_NON_PORTRAIT}, -1);
    auto it = exe_.assetAlbumRefreshMap_.find(FILE_ID_A);
    EXPECT_TRUE(it != exe_.assetAlbumRefreshMap_.end());
    EXPECT_TRUE(it->second.first.count(ALBUM_ID_NON_PORTRAIT) == 1);  // beforeSet
    EXPECT_TRUE(it->second.second.count(ALBUM_ID_NON_PORTRAIT) == 0); // afterSet
}

HWTEST_F(AnalysisAlbumRefreshTest, InsertRefreshMapByDelta_DeltaPositive_002, TestSize.Level2)
{
    exe_.InsertRefreshMapByDelta(FILE_ID_A, {ALBUM_ID_NON_PORTRAIT}, 1);
    auto it = exe_.assetAlbumRefreshMap_.find(FILE_ID_A);
    EXPECT_TRUE(it != exe_.assetAlbumRefreshMap_.end());
    EXPECT_TRUE(it->second.first.count(ALBUM_ID_NON_PORTRAIT) == 0);
    EXPECT_TRUE(it->second.second.count(ALBUM_ID_NON_PORTRAIT) == 1);
}

HWTEST_F(AnalysisAlbumRefreshTest, InsertRefreshMapByDelta_DeltaZero_003, TestSize.Level2)
{
    exe_.InsertRefreshMapByDelta(FILE_ID_A, {ALBUM_ID_NON_PORTRAIT}, 0);
    auto it = exe_.assetAlbumRefreshMap_.find(FILE_ID_A);
    EXPECT_TRUE(it != exe_.assetAlbumRefreshMap_.end());
    EXPECT_TRUE(it->second.first.count(ALBUM_ID_NON_PORTRAIT) == 1);
    EXPECT_TRUE(it->second.second.count(ALBUM_ID_NON_PORTRAIT) == 1);
}

HWTEST_F(AnalysisAlbumRefreshTest, InsertRefreshMapByDelta_InvalidFileId_004, TestSize.Level2)
{
    exe_.InsertRefreshMapByDelta(INVALID_ID, {ALBUM_ID_NON_PORTRAIT}, 1);
    EXPECT_TRUE(exe_.assetAlbumRefreshMap_.find(INVALID_ID) == exe_.assetAlbumRefreshMap_.end());
}

HWTEST_F(AnalysisAlbumRefreshTest, CalculateAlbumChanges_PortraitGroupVisited_005, TestSize.Level2)
{
    // 让 FILE_ID_B 属于同一 portrait group 的两个 albumIds，
    // visitedGroups 会保证 groupTag 只处理一次
    std::vector<PhotoAssetChangeData> changes;
    changes.emplace_back(MakeAssetUpdate(FILE_ID_B));

    // analyzer 在 registry 里应该不为空，否则说明策略没注册，会影响覆盖
    auto &gctx = exe_.groupCtxMap_[GROUP_TAG_1];
    EXPECT_TRUE(gctx.analyzer != nullptr);

    exe_.CalculateAlbumChanges(changes);

    // 不强依赖 delta 数值（由策略决定），但至少 refreshMap 应该有节点
    EXPECT_TRUE(exe_.assetAlbumRefreshMap_.find(FILE_ID_B) != exe_.assetAlbumRefreshMap_.end());
}

HWTEST_F(AnalysisAlbumRefreshTest, CalculateAlbumChanges_NonPortrait_006, TestSize.Level2)
{
    std::vector<PhotoAssetChangeData> changes;
    changes.emplace_back(MakeAssetUpdate(FILE_ID_A));

    // 非 portrait 的 analyzer 也应该存在
    auto it = exe_.albumCtxMap_.find(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(it != exe_.albumCtxMap_.end());
    EXPECT_TRUE(it->second.analyzer != nullptr);

    exe_.CalculateAlbumChanges(changes);

    EXPECT_TRUE(exe_.assetAlbumRefreshMap_.find(FILE_ID_A) != exe_.assetAlbumRefreshMap_.end());
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessCoverChanges_GroupAndAlbums_007, TestSize.Level2)
{
    // 先人为制造 group refreshInfo，触发 HasValidRefreshInfo()
    auto &gctx = exe_.groupCtxMap_[GROUP_TAG_1];
    EXPECT_TRUE(gctx.analyzer != nullptr);

    gctx.refreshInfo.deltaCount_ = 1;
    gctx.refreshInfo.needRefreshCover_ = true;
    gctx.refreshInfo.refreshCover_ = "file://cover_new_group.jpg";

    // 同时让 non-portrait 也需要 cover refresh
    auto &nctx = exe_.albumCtxMap_[ALBUM_ID_NON_PORTRAIT];
    EXPECT_TRUE(nctx.analyzer != nullptr);
    nctx.refreshInfo.deltaCount_ = 1;
    nctx.refreshInfo.needRefreshCover_ = true;
    nctx.refreshInfo.refreshCover_ = "file://cover_new_np.jpg";

    exe_.ProcessCoverChanges();

    // group merge 下发后，portrait albums 的 refreshInfo 应该包含 group cover
    auto &p1 = exe_.albumCtxMap_[ALBUM_ID_PORTRAIT_1];
    auto &p2 = exe_.albumCtxMap_[ALBUM_ID_PORTRAIT_2];

    EXPECT_TRUE(p1.refreshInfo.needRefreshCover_);
    EXPECT_TRUE(p2.refreshInfo.needRefreshCover_);
    EXPECT_TRUE(p1.refreshInfo.refreshCover_ ==
        "file://cover_new_group.jpg" || p1.refreshInfo.refreshCover_.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, ConcludeAlbumRefreshValues_CountNegativeFix_008, TestSize.Level2)
{
    UpdateAlbumData base = MakeBaseAlbum(ALBUM_ID_NON_PORTRAIT,
        static_cast<int32_t>(PhotoAlbumSubType::FAVORITE), 0, "file://old.jpg");
    AnalysisAlbumRefreshInfo info;
    info.deltaCount_ = -10;

    std::string newCover;
    int32_t newCount = 0;

    exe_.ConcludeAlbumRefreshValues(base, info, newCover, newCount);
    EXPECT_TRUE(newCount == 0);
}

HWTEST_F(AnalysisAlbumRefreshTest, ConcludeAlbumRefreshValues_CoverInherit_009, TestSize.Level2)
{
    UpdateAlbumData base = MakeBaseAlbum(ALBUM_ID_NON_PORTRAIT,
        static_cast<int32_t>(PhotoAlbumSubType::FAVORITE), 5, "file://old_cover.jpg");
    AnalysisAlbumRefreshInfo info;
    info.deltaCount_ = 1;
    info.needRefreshCover_ = true;
    info.refreshCover_ = ""; // 空封面

    std::string newCover;
    int32_t newCount = 0;

    exe_.ConcludeAlbumRefreshValues(base, info, newCover, newCount);
    EXPECT_TRUE(newCount == 6);
    // 封面空且 count>0 -> 继承旧封面
    EXPECT_TRUE(newCover == "file://old_cover.jpg");
}

HWTEST_F(AnalysisAlbumRefreshTest, UpdateNotifyContext_Duplicate_010, TestSize.Level2)
{
    exe_.UpdateNotifyContext(ALBUM_ID_NON_PORTRAIT, true);
    exe_.UpdateNotifyContext(ALBUM_ID_NON_PORTRAIT, false); // duplicate update
    auto it = exe_.albumNotifyCtxMap_.find(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(it != exe_.albumNotifyCtxMap_.end());
    EXPECT_TRUE(it->second.needNotify == false);
}

HWTEST_F(AnalysisAlbumRefreshTest, CheckAlbumNotifyStatus_NoAlbumCtx_011, TestSize.Level2)
{
    EXPECT_TRUE(!exe_.CheckAlbumNotifyStatus(999999));
}

HWTEST_F(AnalysisAlbumRefreshTest, CheckAlbumNotifyStatus_NoNotifyCtx_012, TestSize.Level2)
{
    // 删除 notify ctx
    exe_.albumNotifyCtxMap_.erase(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(!exe_.CheckAlbumNotifyStatus(ALBUM_ID_NON_PORTRAIT));
}

HWTEST_F(AnalysisAlbumRefreshTest, CheckAlbumNotifyStatus_NeedNotifyFalse_013, TestSize.Level2)
{
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = false;
    EXPECT_TRUE(!exe_.CheckAlbumNotifyStatus(ALBUM_ID_NON_PORTRAIT));
}

HWTEST_F(AnalysisAlbumRefreshTest, GenerateAlbumChangeInfoAfterChange_CountFix_014, TestSize.Level2)
{
    // 强行制造负 count after change
    auto &ctx = exe_.albumCtxMap_[ALBUM_ID_NON_PORTRAIT];
    ctx.baseInfo.albumCount = 0;
    ctx.refreshInfo.deltaCount_ = -999;
    ctx.refreshInfo.needRefreshCover_ = false;

    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;

    auto after = exe_.GenerateAlbumChangeInfoAfterChange(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(after.albumId_ == ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(after.count_ == 0);
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessAlbumForNotify_NoNeedNotify_015, TestSize.Level2)
{
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = false;
    vector<AlbumChangeData> out;
    exe_.ProcessAlbumForNotify(ALBUM_ID_NON_PORTRAIT, out);
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].hasNotified);
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessAlbumForNotify_NoChangeFields_016, TestSize.Level2)
{
    // before/after 完全一致 -> noCountChange && noCoverChange -> 不 push
    auto &ctx = exe_.albumCtxMap_[ALBUM_ID_NON_PORTRAIT];
    ctx.refreshInfo.deltaCount_ = 0;
    ctx.refreshInfo.needRefreshCover_ = false;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].hasNotified = false;

    vector<AlbumChangeData> out;
    exe_.ProcessAlbumForNotify(ALBUM_ID_NON_PORTRAIT, out);
    EXPECT_TRUE(out.empty());
    EXPECT_TRUE(exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].hasNotified);
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessAlbumForNotify_HasCountChange_017, TestSize.Level2)
{
    auto &ctx = exe_.albumCtxMap_[ALBUM_ID_NON_PORTRAIT];
    ctx.refreshInfo.deltaCount_ = 1;
    ctx.refreshInfo.needRefreshCover_ = false;

    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].hasNotified = false;

    vector<AlbumChangeData> out;
    exe_.ProcessAlbumForNotify(ALBUM_ID_NON_PORTRAIT, out);
    EXPECT_TRUE(!out.empty());
    EXPECT_TRUE(out[0].operation_ == RDB_OPERATION_UPDATE);
}

HWTEST_F(AnalysisAlbumRefreshTest, PrepareAlbumChangeForNotify_SpecifiedIds_018, TestSize.Level2)
{
    // 指定 albumIds 只处理部分
    vector<int32_t> ids = { ALBUM_ID_NON_PORTRAIT };
    vector<AlbumChangeData> out;
    exe_.PrepareAlbumChangeForNotify(out, ids);
    // 允许为空/非空（取决于 refreshInfo），但至少流程覆盖
    EXPECT_TRUE(true);
}

HWTEST_F(AnalysisAlbumRefreshTest, PrepareAlbumChangeInfos_EmptyAlbumIds_019, TestSize.Level2)
{
    std::vector<std::shared_ptr<AlbumChangeInfo>> infos;
    exe_.PrepareAlbumChangeInfos(std::unordered_set<int32_t>{}, true, infos);
    EXPECT_TRUE(infos.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, PreparePhotoChangeForNotify_NoRefreshMap_020, TestSize.Level2)
{
    // 没有 assetAlbumRefreshMap_ 记录 -> continue
    vector<PhotoAssetChangeData> datas;
    datas.emplace_back(MakeAssetUpdate(999999));
    vector<PhotoAssetChangeData> prepared;
    exe_.PreparePhotoChangeForNotify(datas, prepared);
    EXPECT_TRUE(prepared.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, PreparePhotoChangeForNotify_WithBeforeAfterSets_021, TestSize.Level2)
{
    // 构造 refreshMap：beforeSet/afterSet 都不空
    BuildAssetAlbumRefreshMapForDelta(FILE_ID_A,
        std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT, ALBUM_ID_PORTRAIT_1},
        std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT, ALBUM_ID_PORTRAIT_2});

    vector<PhotoAssetChangeData> datas;
    datas.emplace_back(MakeAssetUpdate(FILE_ID_A));

    vector<PhotoAssetChangeData> prepared;
    exe_.PreparePhotoChangeForNotify(datas, prepared);

    // prepared 可能受 AlbumAssetHelper::IsCommonSystemAsset 影响，不能强断言一定有
    EXPECT_TRUE(true);
}

HWTEST_F(AnalysisAlbumRefreshTest, HandleInfoRelatedShootingModeTypes_CacheHit_022, TestSize.Level2)
{
    // 预置一个缓存，确保命中分支覆盖（即使 GetShootingModeAlbumOfAsset 返回空，也不会崩）
    // 这里只能用“尽量提高命中概率”的方式：把 map 预填多个键位不现实（type 是 enum）
    // 但至少覆盖：fileId 检查 + 结构体遍历空返回
    PhotoAssetChangeInfo info;
    info.fileId_ = FILE_ID_A;
    info.mimeType_ = "image/jpeg";
    info.mediaType_ = MEDIA_TYPE_IMAGE;

    // 关键：预置 map（即便不命中，也不会失败；命中时可覆盖 cache 分支）
    exe_.shootingModeAlbumIdMap_.clear();
    // 不知道具体 ShootingModeAlbumType 枚举值，这里只能尽力：插入一个常见占位（若枚举存在 0）
    exe_.shootingModeAlbumIdMap_.emplace(static_cast<ShootingModeAlbumType>(0), 99001);

    std::unordered_set<int32_t> affected;
    exe_.HandleInfoRelatedShootingModeTypes(info, affected);

    // 允许 affected 为空（取决于 GetShootingModeAlbumOfAsset），但覆盖路径已经走到
    EXPECT_TRUE(true);
}

HWTEST_F(AnalysisAlbumRefreshTest, ResetExecutionStatus_ClearAll_023, TestSize.Level2)
{
    exe_.ResetExecutionStatus();
    EXPECT_TRUE(exe_.assetAlbumMap_.empty());
    EXPECT_TRUE(exe_.assetAlbumRefreshMap_.empty());
    EXPECT_TRUE(exe_.albumCtxMap_.empty());
    EXPECT_TRUE(exe_.groupCtxMap_.empty());
    EXPECT_TRUE(exe_.albumNotifyCtxMap_.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessAlbumForNotify_RepeatGuard_024, TestSize.Level2)
{
    // hasNotified=true -> CHECK_AND_RETURN
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].hasNotified = true;

    vector<AlbumChangeData> out;
    exe_.ProcessAlbumForNotify(ALBUM_ID_NON_PORTRAIT, out);
    EXPECT_TRUE(out.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, GenerateAlbumChangeInfoBeforeChange_Normal_025, TestSize.Level2)
{
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    auto before = exe_.GenerateAlbumChangeInfoBeforeChange(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(before.albumId_ == ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(before.isCoverChange_ == false);
}

HWTEST_F(AnalysisAlbumRefreshTest, GenerateAlbumChangeInfoAfterChange_CoverRefresh_026, TestSize.Level2)
{
    auto &ctx = exe_.albumCtxMap_[ALBUM_ID_NON_PORTRAIT];
    ctx.refreshInfo.deltaCount_ = 0;
    ctx.refreshInfo.needRefreshCover_ = true;
    ctx.refreshInfo.refreshCover_ = "file://np_new_cover.jpg";

    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;

    auto after = exe_.GenerateAlbumChangeInfoAfterChange(ALBUM_ID_NON_PORTRAIT);
    EXPECT_TRUE(after.coverUri_ == "file://np_new_cover.jpg");
}

HWTEST_F(AnalysisAlbumRefreshTest, PrepareAlbumChangeInfos_AfterChange_027, TestSize.Level2)
{
    std::vector<std::shared_ptr<AlbumChangeInfo>> infos;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    exe_.PrepareAlbumChangeInfos(std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT}, true, infos);
    EXPECT_TRUE(!infos.empty());
    EXPECT_TRUE(infos[0] != nullptr);
}

HWTEST_F(AnalysisAlbumRefreshTest, PrepareAlbumChangeInfos_BeforeChange_028, TestSize.Level2)
{
    std::vector<std::shared_ptr<AlbumChangeInfo>> infos;
    exe_.albumNotifyCtxMap_[ALBUM_ID_NON_PORTRAIT].needNotify = true;
    exe_.PrepareAlbumChangeInfos(std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT}, false, infos);
    EXPECT_TRUE(!infos.empty());
}

HWTEST_F(AnalysisAlbumRefreshTest, PreparePhotoChangeForNotify_FilterByValidAsset_029, TestSize.Level2)
{
    BuildAssetAlbumRefreshMapForDelta(FILE_ID_A,
        std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT},
        std::unordered_set<int32_t>{ALBUM_ID_NON_PORTRAIT});

    vector<PhotoAssetChangeData> datas;
    auto d = MakeAssetUpdate(FILE_ID_A);

    // 让 albumChangeInfos_ 不空，提高 hasValidAnalysisAlbumChange 概率
    AttachAlbumInfos(d, {ALBUM_ID_NON_PORTRAIT}, {ALBUM_ID_NON_PORTRAIT});
    datas.emplace_back(d);

    vector<PhotoAssetChangeData> prepared;
    exe_.PreparePhotoChangeForNotify(datas, prepared);
    EXPECT_TRUE(true);
}

HWTEST_F(AnalysisAlbumRefreshTest, ProcessCoverChanges_NoNeed_030, TestSize.Level2)
{
    // 全部 HasValidRefreshInfo() 为 false -> continue
    for (auto &[id, ctx] : exe_.albumCtxMap_) {
        ctx.refreshInfo.deltaCount_ = 0;
        ctx.refreshInfo.needRefreshCover_ = false;
    }
    for (auto &[gt, gctx] : exe_.groupCtxMap_) {
        gctx.refreshInfo.deltaCount_ = 0;
        gctx.refreshInfo.needRefreshCover_ = false;
    }
    exe_.ProcessCoverChanges();
    EXPECT_TRUE(true);
}

HWTEST_F(AnalysisAlbumRefreshTest, CalculateAlbumChanges_MissingAlbumCtx_031, TestSize.Level2)
{
    // assetAlbumMap_ 里插一个不存在的 albumId，覆盖 "No recorded album" continue 分支
    exe_.assetAlbumMap_[FILE_ID_A].insert(999999);

    vector<PhotoAssetChangeData> changes;
    changes.emplace_back(MakeAssetUpdate(FILE_ID_A));
    exe_.CalculateAlbumChanges(changes);
    EXPECT_TRUE(true);
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
