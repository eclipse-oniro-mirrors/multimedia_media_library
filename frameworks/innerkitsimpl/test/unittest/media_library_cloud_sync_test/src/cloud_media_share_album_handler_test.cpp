/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_share_album_handler_test.h"

#include "cloud_media_data_handler.h"
#include "i_cloud_media_data_handler.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "media_library_database.h"
#include "share_album_cloud_sync_test_utils.h"

using namespace testing::ext;
using namespace OHOS::Media::ORM;

namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaShareAlbumHandlerTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;
// 一次下行的共享相册个数(真实数据: 1 个共享相册, 下发两条记录用于新增/更新用例)
static constexpr int32_t SHARE_ALBUM_COUNT = 1;
// 共享相册名下落库的共享资产条数(真实资产记录的前两条, 第三条留给元数据/自动归属用例)
static constexpr int32_t SHARE_ASSET_COUNT = 2;

void CloudMediaShareAlbumHandlerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest CheckPoint ret: " << ret;
    // 先清一次历史运行可能残留的本套数据(含不带 cloud_id 的源相册), 避免污染后续相册归属
    ShareAlbumCloudSyncTestUtils::CleanShareTddData();

    // 通过 inner 层接口下行共享相册与共享资产, 后续用例直接基于这批数据断言
    std::vector<int32_t> albumStats{0, 0, 0, 0, 0};
    ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbums(albumStats);
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest DownLinkShareAlbums ret:" << ret
                     << ", newAlbum:" << albumStats[StatsIndex::NEW_RECORDS_COUNT];
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(albumStats[StatsIndex::NEW_RECORDS_COUNT], SHARE_ALBUM_COUNT);
    // 共享资产通过 dentry 通路写入媒体库, 与既有用例(OnDentryFileInsert)的约定一致
    ret = ShareAlbumCloudSyncTestUtils::InsertSharePhotosByDentry();
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest InsertSharePhotosByDentry ret:" << ret;
    EXPECT_EQ(ret, E_OK) << "共享资产 dentry 下行失败, ret: " << ret;
    // dentry 通路的相册归属受生产侧相册映射缓存影响, 这里显式建立资产到共享相册的归属
    ret = ShareAlbumCloudSyncTestUtils::BindSharePhotosToShareAlbums();
    EXPECT_EQ(ret, E_OK) << "共享资产绑定共享相册失败, ret: " << ret;
    PhotosPo insertedPhoto;
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, insertedPhoto)) << "共享资产未写入媒体库";
    PhotosPo uplinkPhoto;
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID, uplinkPhoto)) << "待上行共享资产未写入媒体库";
}

void CloudMediaShareAlbumHandlerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest TearDownTestCase";
    ShareAlbumCloudSyncTestUtils::CleanShareTddData();
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest TearDownTestCase ret: " << ret;
}

void CloudMediaShareAlbumHandlerTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaShareAlbumHandlerTest SetUp";
}

void CloudMediaShareAlbumHandlerTest::TearDown()
{}

/**
 * 测试功能: 共享相册下行(新增)后, 相册类型是否被刷新为共享相册类型.
 * 期望结果: album_type = 8192(SHARE), album_subtype = 8193(SHARE_GENERIC), lpath/相册名与云侧一致
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbum_AlbumTypeIsShare, TestSize.Level1)
{
    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album))
        << "共享相册未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID;
    EXPECT_EQ(album.albumType.value_or(0), ShareAlbumCloudSyncTestUtils::ALBUM_TYPE_SHARE);
    EXPECT_EQ(album.albumSubtype.value_or(0), ShareAlbumCloudSyncTestUtils::ALBUM_SUBTYPE_SHARE_GENERIC);
    EXPECT_EQ(album.lpath.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_LPATH);
    EXPECT_EQ(album.albumName.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_NAME);
}

/**
 * 测试功能: 共享相册下行后, 名下资产的上行开关是否已打开.
 * 期望结果: upload_status = 1(OPEN), 否则名下共享资产上不了行
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbum_UploadStatusOpened, TestSize.Level1)
{
    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album))
        << "共享相册未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID;
    EXPECT_EQ(album.uploadStatus.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::UPLOAD_STATUS_OPEN);
}

/**
 * 测试功能: 共享相册属主(ownerId)是否随下行一起落库.
 * 期望结果: share_album_owner 与云侧 ownerId 一致
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbum_ShareAlbumOwnerPersisted, TestSize.Level1)
{
    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album))
        << "共享相册未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID;
    EXPECT_EQ(album.shareAlbumOwner.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER);
}

/**
 * 测试功能: 收尾反例——非 SDIRTY(同步态)的共享相册不应被误删.
 * 期望结果: 相册与其名下共享资产都保留
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, OnCompletePull_SyncedShareAlbum_KeepsAlbum, TestSize.Level1)
{
    int32_t albumId = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumId, 0) << "共享相册未下行到媒体库";

    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SYNCED);
    MediaOperateResult optRet = {"", 0, ""};
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeShareAlbumHandler()->OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK) << "OnCompletePull 失败, ret: " << ret;

    PhotoAlbumPo album;
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "SYNCED 共享相册被误删";
    EXPECT_EQ(ShareAlbumCloudSyncTestUtils::CountShareAssetsOfAlbum(albumId), SHARE_ASSET_COUNT)
        << "SYNCED 共享资产被误删";
}

/**
 * 测试功能: 收尾反例——云侧返回异常错误码时不应触发任何删除.
 * 期望结果: SDIRTY 共享相册依然保留
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, OnCompletePull_ErrorCode_ReturnsOkWithoutDelete, TestSize.Level1)
{
    int32_t albumId = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumId, 0) << "共享相册未下行到媒体库";

    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SDIRTY);
    MediaOperateResult optRet = {"", ShareAlbumCloudSyncTestUtils::MOCK_PULL_ERROR_CODE, "mock pull failed"};
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeShareAlbumHandler()->OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK) << "OnCompletePull 错误码分支返回值不符合预期, ret: " << ret;

    PhotoAlbumPo album;
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "异常错误码下共享相册被误删";
    EXPECT_EQ(ShareAlbumCloudSyncTestUtils::CountShareAssetsOfAlbum(albumId), SHARE_ASSET_COUNT)
        << "异常错误码下共享资产被误删";
    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SYNCED);
}

/**
 * 测试功能: 共享相册已存在时, 云侧下发的更新(相册名变化)能否刷新到本地相册.
 * 期望结果: 不新建相册(条数仍为1且 id 不变), 相册名更新(属主不变), 类型仍为共享相册, dirty 回到同步态, 更新计数=1
 * 说明: 走服务端独立处理的共享相册下行(OnFetchRecords + sceneType=SHARE), lPath 未变时按 lPath 匹配本地相册.
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbumUpdate_AlbumInfoRefreshed, TestSize.Level1)
{
    int32_t albumIdBefore = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumIdBefore, 0) << "共享相册未下行到媒体库";

    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATE_JSON, stats);
    EXPECT_EQ(ret, E_OK) << "共享相册更新下行失败, ret: " << ret;
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], 1) << "更新计数不符合预期";
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], 0) << "已存在的相册不应被当成新增";

    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "更新后共享相册不存在";
    EXPECT_EQ(album.albumId.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX), albumIdBefore)
        << "更新不应新建相册";
    EXPECT_EQ(ShareAlbumCloudSyncTestUtils::CountAlbumsByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID), 1) << "同一 cloudId 出现重复相册";
    EXPECT_EQ(album.albumName.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATED_NAME);
    EXPECT_EQ(album.shareAlbumOwner.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER)
        << "属主不应被云侧更新改变";
    EXPECT_EQ(album.albumType.value_or(0), ShareAlbumCloudSyncTestUtils::ALBUM_TYPE_SHARE);
    EXPECT_EQ(album.dirty.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SYNCED) << "更新后相册应回到同步态";
}

/**
 * 测试功能: 云侧更新记录带新 lPath 时, 能否通过 cloudId 匹配到本地同一个共享相册(而不是新建相册).
 * 期望结果: 该 cloudId 仍只有 1 条相册且 相册id 不变, lpath/相册名已刷新为云侧新值, 更新计数=1
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbumUpdate_MatchLocalAlbumByCloudId, TestSize.Level1)
{
    int32_t albumIdBefore = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumIdBefore, 0) << "共享相册未下行到媒体库";

    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATE_LPATH_JSON, stats);
    EXPECT_EQ(ret, E_OK) << "共享相册改路径更新下行失败, ret: " << ret;
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], 1) << "更新计数不符合预期";
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], 0) << "lpath 变化时不应新建相册";

    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "更新后共享相册不存在";
    EXPECT_EQ(album.albumId.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX), albumIdBefore)
        << "lpath 变化时应按 cloudId 匹配到原相册";
    EXPECT_EQ(ShareAlbumCloudSyncTestUtils::CountAlbumsByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID), 1) << "同一 cloudId 出现重复相册";
    EXPECT_EQ(album.lpath.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_MOVED_LPATH);
    EXPECT_EQ(album.albumName.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATED_NAME)
        << "仅迁移 lpath 不应改变相册名";
}

/**
 * 测试功能: 云侧下发"已删除的共享相册"时, 本地共享相册能否被标记为待清理并计入删除计数.
 * 期望结果: 相册仍存在但 dirty=SDIRTY, DELETE_RECORDS_COUNT=1(相册与资产的彻底清理由 OnCompletePull 收尾)
 * 说明: 与 OnCompletePull_SdirtyShareAlbum_RemovesAlbumAndAssets 配套, 前者负责打标, 后者负责收尾.
 *       删除记录携带的 localPath 与本地当前值一致(改路径更新后的值), 生产按 lPath 优先匹配的本条相册.
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbumDelete_MarksAlbumSdirty, TestSize.Level1)
{
    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_DELETE_JSON, stats);
    EXPECT_EQ(ret, E_OK) << "共享相册删除下行失败, ret: " << ret;
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1) << "删除计数不符合预期";
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], 0) << "删除不应计入更新";
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], 0) << "删除不应计入新增";

    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "打删除标记后相册不应直接消失";
    EXPECT_EQ(album.dirty.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SDIRTY) << "云侧删除应把本地相册标记为 SDIRTY";
    // 复位为同步态, 避免影响后续用例(反例/脏保护/收尾清理按需自行设置)
    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SYNCED);
}

/**
 * 测试功能: 本地共享相册有未上行的修改(dirty=MDIRTY)时, 云侧的更新/删除是否会被跳过以免覆盖本地修改.
 * 期望结果: 相册名/lpath/dirty 均保持本地值, 更新与删除计数都为 0
 * 说明: 云侧记录携带的 localPath 与本地当前值一致, 保证生产"先按 lPath 匹配"命中的必然是这条 MDIRTY 相册;
 *       cloudId 兜底匹配路径由前一条 MatchLocalAlbumByCloudId 用例覆盖.
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, DownLink_ShareAlbum_MdirtyProtectedFromUpdateAndDelete, TestSize.Level1)
{
    // 前置: 本地共享相册有未上行修改
    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_MDIRTY);
    PhotoAlbumPo preAlbum;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, preAlbum)) << "共享相册不存在";
    ASSERT_EQ(preAlbum.dirty.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_MDIRTY) << "前置: 本地待上行标记未写入";

    // 云侧的更新记录与本地当前 lpath 一致, 这里应该被跳过
    std::vector<int32_t> updateStats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATE_LPATH_JSON, updateStats);
    EXPECT_EQ(ret, E_OK) << "共享相册更新下行失败, ret: " << ret;
    EXPECT_EQ(updateStats[StatsIndex::META_MODIFY_RECORDS_COUNT], 0) << "本地脏相册不应被云侧更新覆盖";
    EXPECT_EQ(updateStats[StatsIndex::NEW_RECORDS_COUNT], 0) << "本地脏相册不应被当成新增";

    // 云侧的删除记录同样应该被跳过
    std::vector<int32_t> deleteStats{0, 0, 0, 0, 0};
    ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbumRecords(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_DELETE_JSON, deleteStats);
    EXPECT_EQ(ret, E_OK) << "共享相册删除下行失败, ret: " << ret;
    EXPECT_EQ(deleteStats[StatsIndex::DELETE_RECORDS_COUNT], 0) << "本地脏相册不应被云侧删除";

    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "共享相册不存在";
    EXPECT_EQ(album.albumName.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_UPDATED_NAME)
        << "相册名被云侧更新覆盖";
    EXPECT_EQ(album.lpath.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_MOVED_LPATH)
        << "相册 lpath 被云侧更新覆盖";
    EXPECT_EQ(album.dirty.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_MDIRTY) << "本地待上行标记被覆盖";
}

/**
 * 测试功能: 共享相册被云侧标记删除(SDIRTY)后, OnCompletePull 收尾能否把相册及其名下共享资产清理干净.
 * 期望结果: 相册删除, 名下共享资产被异步清理干净
 * 说明: 本用例会删除共享相册及其名下共享资产, 因此排在所有依赖该数据的用例之后执行.
 */
HWTEST_F(CloudMediaShareAlbumHandlerTest, OnCompletePull_SdirtyShareAlbum_RemovesAlbumAndAssets, TestSize.Level1)
{
    int32_t albumId = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumId, 0) << "共享相册未下行到媒体库";
    ASSERT_EQ(ShareAlbumCloudSyncTestUtils::CountShareAssetsOfAlbum(albumId), SHARE_ASSET_COUNT)
        << "共享相册下的共享资产数量不符合预期";

    ShareAlbumCloudSyncTestUtils::UpdateAlbumDirty(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SDIRTY);
    MediaOperateResult optRet = {"", 0, ""};
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeShareAlbumHandler()->OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK) << "OnCompletePull 失败, ret: " << ret;

    // 资产清理是异步任务, 这里轮询等待数据库状态收敛
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::WaitForShareAssetsRemoved(albumId)) << "共享资产异步清理超时";
    PhotoAlbumPo album;
    EXPECT_FALSE(ShareAlbumCloudSyncTestUtils::GetAlbumByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID, album)) << "SDIRTY 共享相册未被删除";
}
}  // namespace OHOS::Media::CloudSync
