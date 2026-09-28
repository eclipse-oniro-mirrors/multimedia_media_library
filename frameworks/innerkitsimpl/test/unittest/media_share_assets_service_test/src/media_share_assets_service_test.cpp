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

#define MLOG_TAG "MediaShareAssetsServiceTest"

#include "media_share_assets_test_utils.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "cloud_sync_notify_handler.h"
#include "dao/media_share_assets_dao.h"
#include "media_column.h"
#include "media_log.h"
#include "media_share_assets_service.h"
#include "media_share_assets_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "share_member_column.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::ORM;

namespace OHOS::Media {

static constexpr int32_t SHARE_ALBUM_ID = 3001;
static constexpr int32_t OTHER_SHARE_ALBUM_ID = 3002;
static constexpr int32_t USER_ALBUM_ID = 3003;
static constexpr int32_t SHARED_ASSET_FLAG = 1;
static constexpr int32_t NOT_SHARED_ASSET_FLAG = 0;
static constexpr int32_t ASYNC_WAIT_MAX_TIMES = 100;
static constexpr int32_t ASYNC_WAIT_INTERVAL_MS = 100;
// 重启清理为异步触发, 留出若干轮询周期给线程启动
static constexpr int32_t RESTART_SETTLE_WAIT_TIMES = 3;

namespace {
// 等待共享资产清理流程整体结束: 已标记的待删除资产被清掉, 且清理状态已复位为关闭
// RemoveShareAlbumAndAsset / RestartRemoveShareAlbumAndAsset 均为 detached 线程,
// 必须等完整流程跑完, 否则其 AfterRemoveShareAlbumAndAsset 会复位清理状态、污染后续用例断言
// 返回 false 表示超时, 由调用方断言失败, 避免异步未完成时继续执行产生误判
bool WaitForShareAssetsCleanFlowFinished()
{
    for (int32_t i = 0; i < ASYNC_WAIT_MAX_TIMES; i++) {
        bool assetsCleared = MediaShareAssetsTestUtils::CountDeletedMarkedAssets() <= 0;
        bool statusReset = !MediaShareAssetsCloudExitUtils::IsShareAssetCleaning();
        if (assetsCleared && statusReset) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ASYNC_WAIT_INTERVAL_MS));
    }
    MEDIA_ERR_LOG("WaitForShareAssetsCleanFlowFinished timeout, async clean flow is not finished");
    return false;
}
} // namespace

/**
 * @tc.name: GetInstance_ReturnsSameInstance
 * @tc.desc: 共享资产清理服务为进程内单例
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, GetInstance_ReturnsSameInstance, TestSize.Level1)
{
    MediaShareAssetsService &instance1 = MediaShareAssetsService::GetInstance();
    MediaShareAssetsService &instance2 = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @tc.name: MarkShareAssetsToRemove_NoShareAsset_ReturnsOk
 * @tc.desc: 没有共享资产时标记流程直接结束(覆盖 while 条件为 false 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkShareAssetsToRemove_NoShareAsset_ReturnsOk, TestSize.Level1)
{
    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.MarkShareAssetsToRemove(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: MarkShareAssetsToRemove_WithShareAsset_MarksAssets
 * @tc.desc: 存在共享资产时进入 while 循环批量标记(覆盖 while 循环体分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkShareAssetsToRemove_WithShareAsset_MarksAssets, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(101, "IMG_101.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.MarkShareAssetsToRemove(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("101", MediaColumn::MEDIA_NAME),
        MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME);
}

/**
 * @tc.name: MarkShareAssetsToRemoveByAlbumId_WithShareAsset_MarksAssets
 * @tc.desc: 按相册维度标记共享资产, 其它相册资产不受影响
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkShareAssetsToRemoveByAlbumId_WithShareAsset_MarksAssets, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(111, "IMG_111.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(112, "IMG_112.jpg", OTHER_SHARE_ALBUM_ID,
        SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.MarkShareAssetsToRemoveByAlbumId(SHARE_ALBUM_ID), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("111", MediaColumn::MEDIA_NAME),
        MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("112", MediaColumn::MEDIA_NAME), "IMG_112.jpg");
}

/**
 * @tc.name: MarkShareAssetsToRemoveByAlbumId_NoShareAsset_ReturnsOk
 * @tc.desc: 相册下没有共享资产时直接返回成功, 其它相册的资产不会被误标记(覆盖 while 条件为 false 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, MarkShareAssetsToRemoveByAlbumId_NoShareAsset_ReturnsOk, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(124, "IMG_124.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.MarkShareAssetsToRemoveByAlbumId(OTHER_SHARE_ALBUM_ID), E_OK);

    // 目标相册下无资产: 其它相册的共享资产名称保持原样, 未被标记为待删除
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("124", MediaColumn::MEDIA_NAME), "IMG_124.jpg");
    EXPECT_EQ(MediaShareAssetsTestUtils::CountDeletedMarkedAssets(), 0);
}

/**
 * @tc.name: RemoveShareAssets_EmptyList_ReturnsTrue
 * @tc.desc: 待删除列表为空时直接返回 true, 且不会误删数据库中的待删除资产(覆盖 for 循环不执行的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssets_EmptyList_ReturnsTrue, TestSize.Level1)
{
    // 先造一条已标记待删除的共享资产, 用于验证空列表不会误删库内记录
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(122, "IMG_122.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    MediaShareAssetsDao dao;
    std::vector<std::string> markIds = { "122" };
    ASSERT_EQ(dao.MarkDeletedAndClearCloudInfo(markIds), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::CountDeletedMarkedAssets(), 1);

    std::vector<PhotosPo> emptyList;
    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_TRUE(service.RemoveShareAssets(emptyList));

    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 1);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountDeletedMarkedAssets(), 1);
}

/**
 * @tc.name: RemoveShareAssets_IncompletePhoto_ReturnsTrue
 * @tc.desc: 资产关键字段缺失时 DeletePhoto 失败并 continue, 数据库记录不会被误删
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssets_IncompletePhoto_ReturnsTrue, TestSize.Level1)
{
    // 库中存在一条已标记待删除的共享资产, 但传入的 PhotosPo 缺少 data/displayName 等字段
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(121, "IMG_121.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    MediaShareAssetsDao dao;
    std::vector<std::string> markIds = { "121" };
    ASSERT_EQ(dao.MarkDeletedAndClearCloudInfo(markIds), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::CountDeletedMarkedAssets(), 1);

    std::vector<PhotosPo> photoList;
    PhotosPo incompletePhoto;
    incompletePhoto.fileId = 121;
    photoList.emplace_back(incompletePhoto);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_TRUE(service.RemoveShareAssets(photoList));

    // 字段缺失的资产被跳过: 数据库记录仍然存在, 且保持待删除标记
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 1);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountDeletedMarkedAssets(), 1);
}

/**
 * @tc.name: RemoveShareAssets_ValidPhoto_DeletesDbRecord
 * @tc.desc: 字段完整的共享资产会被删除文件、删库并返回 true(覆盖 for 循环体成功分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssets_ValidPhoto_DeletesDbRecord, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(131, "IMG_131.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    // 前置校验: 被清理的资产对应的物理文件真实存在
    ASSERT_TRUE(MediaShareAssetsTestUtils::IsPhotoFileExists("IMG_131.jpg"));

    MediaShareAssetsDao dao;
    std::vector<std::string> markIds = { "131" };
    ASSERT_EQ(dao.MarkDeletedAndClearCloudInfo(markIds), E_OK);

    std::vector<PhotosPo> photoList;
    ASSERT_EQ(dao.GetShareAssetToRemove(photoList), E_OK);
    ASSERT_EQ(photoList.size(), 1u);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_TRUE(service.RemoveShareAssets(photoList));

    // 物理文件与数据库记录都应被删除
    EXPECT_FALSE(MediaShareAssetsTestUtils::IsPhotoFileExists("IMG_131.jpg"));
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: RemoveShareAssetsTask_NoMarkedAsset_ExitsLoop
 * @tc.desc: 没有待清理资产时 while 循环首次判断即退出(覆盖 break 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssetsTask_NoMarkedAsset_ExitsLoop, TestSize.Level1)
{
    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    service.RemoveShareAssetsTask();
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: RemoveShareAssetsTask_WithMarkedAsset_DeletesAsset
 * @tc.desc: 存在待清理资产时循环处理一批并删除数据库记录(覆盖 while 循环体分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssetsTask_WithMarkedAsset_DeletesAsset, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(141, "IMG_141.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsDao dao;
    std::vector<std::string> markIds = { "141" };
    ASSERT_EQ(dao.MarkDeletedAndClearCloudInfo(markIds), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    service.RemoveShareAssetsTask();
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: RemoveShareAssetsInner_WithShareData_RemovesAlbumAndMember
 * @tc.desc: 内部流程会标记资产、删除共享相册与共享成员
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssetsInner_WithShareData_RemovesAlbumAndMember, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(SHARE_ALBUM_ID, "member_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(151, "IMG_151.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.RemoveShareAssetsInner(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 0);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(ShareMemberColumn::TABLE_NAME), 0);

    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "共享资产清理异步流程未在超时时间内结束";
    // 已标记的资产最终会被异步清理掉
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
}

/**
 * @tc.name: RemoveShareAlbumAndAsset_NoShareData_ReturnsOk
 * @tc.desc: 无共享数据时清理流程正常返回并复位状态, 普通相册与普通资产不受影响
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAlbumAndAsset_NoShareData_ReturnsOk, TestSize.Level1)
{
    // 账号退出清理只应影响共享数据, 这里准备普通相册与普通资产用于验证
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertUserAlbum(USER_ALBUM_ID, "user_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(125, "IMG_125.jpg", USER_ALBUM_ID, NOT_SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.RemoveShareAlbumAndAsset(), E_OK);
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());

    // 普通相册与普通资产均保持原样
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("125", MediaColumn::MEDIA_NAME), "IMG_125.jpg");

    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "共享资产清理异步流程未在超时时间内结束";
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 1);
}

/**
 * @tc.name: RemoveShareAlbumAndAsset_WithShareData_RemovesAll
 * @tc.desc: 账号退出/关闭开关场景: 共享相册、共享成员、共享资产均被清理
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAlbumAndAsset_WithShareData_RemovesAll, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(SHARE_ALBUM_ID, "member_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(161, "IMG_161.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.RemoveShareAlbumAndAsset(), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 0);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(ShareMemberColumn::TABLE_NAME), 0);

    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "共享资产清理异步流程未在超时时间内结束";
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoColumn::PHOTOS_TABLE), 0);
    // 共享资产的物理文件也随账号退出被清理
    EXPECT_FALSE(MediaShareAssetsTestUtils::IsPhotoFileExists("IMG_161.jpg"));
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
}

/**
 * @tc.name: RemoveShareAssetsByAlbumIds_EmptyAlbumIds_ReturnsOk
 * @tc.desc: 入参为空时直接返回成功, 相册与资产均保持原样(覆盖提前返回分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssetsByAlbumIds_EmptyAlbumIds_ReturnsOk, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(123, "IMG_123.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    std::vector<int32_t> albumIds;
    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.RemoveShareAssetsByAlbumIds(albumIds), E_OK);

    // 空入参提前返回: 共享相册未被删除, 资产未被标记
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("123", MediaColumn::MEDIA_NAME), "IMG_123.jpg");
}

/**
 * @tc.name: RemoveShareAssetsByAlbumIds_WithAlbum_RemovesSpecifiedAlbum
 * @tc.desc: 指定共享相册及其资产被清理, 其它共享相册保留(覆盖 for 循环体分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RemoveShareAssetsByAlbumIds_WithAlbum_RemovesSpecifiedAlbum, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(OTHER_SHARE_ALBUM_ID, "share_album_2"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(171, "IMG_171.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);

    std::vector<int32_t> albumIds = { SHARE_ALBUM_ID };
    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    EXPECT_EQ(service.RemoveShareAssetsByAlbumIds(albumIds), E_OK);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
    EXPECT_EQ(MediaShareAssetsTestUtils::QueryPhotoString("171", MediaColumn::MEDIA_NAME),
        MediaShareAssetsTestUtils::DELETED_DISPLAY_NAME);

    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "共享资产清理异步流程未在超时时间内结束";
}

/**
 * @tc.name: RestartRemoveShareAlbumAndAsset_NotCleaning_NoRemoveFlow
 * @tc.desc: 上次清理已完成时重启不再触发删除流程(覆盖清理状态判断为 false 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RestartRemoveShareAlbumAndAsset_NotCleaning_NoRemoveFlow, TestSize.Level1)
{
    // 先等前序用例的异步清理线程结束, 避免残留线程影响本用例的相册数量断言
    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "前序用例的清理异步流程未结束";

    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::SYNC_SWITCHED_OFF);

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    service.RestartRemoveShareAlbumAndAsset();
    std::this_thread::sleep_for(std::chrono::milliseconds(ASYNC_WAIT_INTERVAL_MS * RESTART_SETTLE_WAIT_TIMES));

    // 清理状态为关闭时不会执行删除, 共享相册保持不变
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 1);
}

/**
 * @tc.name: RestartRemoveShareAlbumAndAsset_Cleaning_ResumesRemoveFlow
 * @tc.desc: 上次清理未完成时重启续跑删除流程(覆盖清理状态判断为 true 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RestartRemoveShareAlbumAndAsset_Cleaning_ResumesRemoveFlow, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(SHARE_ALBUM_ID, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(SHARE_ALBUM_ID, "member_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertPhoto(181, "IMG_181.jpg", SHARE_ALBUM_ID, SHARED_ASSET_FLAG), E_OK);
    MediaShareAssetsCloudExitUtils::SetShareAssetCleanStatus(CloudSyncStatus::CLOUD_CLEANING);
    ASSERT_TRUE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    service.RestartRemoveShareAlbumAndAsset();

    // 等待续跑流程整体结束: 共享相册与成员被删除、待删除资产被清理、清理状态复位
    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "共享资产清理异步流程未在超时时间内结束";
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(PhotoAlbumColumns::TABLE), 0);
    EXPECT_EQ(MediaShareAssetsTestUtils::CountRows(ShareMemberColumn::TABLE_NAME), 0);
}

/**
 * @tc.name: BeforeAndAfterRemoveShareAlbumAndAsset_UpdatesCleanStatus
 * @tc.desc: 清理前置状态写入 CLOUD_CLEANING, 清理后置状态复位为关闭
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, BeforeAndAfterRemoveShareAlbumAndAsset_UpdatesCleanStatus, TestSize.Level1)
{
    // 先等前序用例的异步清理线程结束, 否则其 AfterRemoveShareAlbumAndAsset 会并发复位清理状态
    ASSERT_TRUE(WaitForShareAssetsCleanFlowFinished()) << "前序用例的清理异步流程未结束";

    MediaShareAssetsService &service = MediaShareAssetsService::GetInstance();
    service.BeforeRemoveShareAlbumAndAsset();
    EXPECT_TRUE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());

    service.AfterRemoveShareAlbumAndAsset();
    EXPECT_FALSE(MediaShareAssetsCloudExitUtils::IsShareAssetCleaning());
}

} // namespace OHOS::Media