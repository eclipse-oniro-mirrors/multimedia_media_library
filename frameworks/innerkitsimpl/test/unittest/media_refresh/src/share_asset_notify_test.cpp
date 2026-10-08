/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "share_asset_notify_test.h"

#include "message_parcel.h"
#include "medialibrary_type_const.h"
#include "photo_album.h"
#include "userfile_manager_types.h"

#include "notify_task_worker.h"
#include "album_change_notify_execution.h"
#include "asset_change_notify_execution.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::Media::AccurateRefresh;
using namespace OHOS::Media::Notification;

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

void ShareAssetNotifyTest::SetUpTestCase(void) {}

void ShareAssetNotifyTest::TearDownTestCase(void) {}

void ShareAssetNotifyTest::SetUp()
{
    // Notify() 末尾会走 MediaLibraryNotifyNew::AddItem，而 NotifyTaskWorker::StartWorker()
    // 会 detach 一个常驻线程，该线程活过 main 后访问已析构的静态对象会导致进程信号 11。
    // 这里把 isThreadRunning_ 预置为 true，使 AddItem 认为 worker 已在运行、不再启动线程
    // （本组用例只校验 notifyInfos_ 的分类结果，不需要真正派发通知）。
    auto worker = NotifyTaskWorker::GetInstance();
    if (worker != nullptr) {
        worker->isThreadRunning_.store(true);
    }
}

void ShareAssetNotifyTest::TearDown() {}

namespace {
constexpr int32_t TEST_ALBUM_ID = 100;
constexpr int32_t TEST_FILE_ID = 1;
constexpr int32_t TEST_SHARE_RISK_STATUS = 3;
constexpr int32_t TEST_PHOTO_VISIBILITY = 1;
constexpr int64_t TEST_SHARE_DATE_DAY = 20260101;
constexpr int64_t TEST_SHARE_GROUP = 200;

AlbumChangeInfo MakeShareAlbumInfo()
{
    AlbumChangeInfo albumInfo;
    albumInfo.albumId_ = TEST_ALBUM_ID;
    albumInfo.albumType_ = static_cast<int32_t>(PhotoAlbumType::SHARE);
    albumInfo.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC);
    albumInfo.dirty_ = static_cast<int32_t>(DirtyType::TYPE_SYNCED);
    return albumInfo;
}

// 构造共享相册的变更数据，before/after 的 dirty 由入参决定
AlbumChangeData MakeShareAlbumChangeData(int32_t beforeDirty, int32_t afterDirty)
{
    AlbumChangeData changeData;
    changeData.infoBeforeChange_ = MakeShareAlbumInfo();
    changeData.infoBeforeChange_.dirty_ = beforeDirty;
    changeData.infoAfterChange_ = MakeShareAlbumInfo();
    changeData.infoAfterChange_.dirty_ = afterDirty;
    return changeData;
}

// 构造一个满足 AlbumAssetHelper::IsCommonSystemAsset 的资产，isShared 决定是否共享资产
PhotoAssetChangeInfo MakeCommonSystemAsset(int32_t isShared)
{
    PhotoAssetChangeInfo assetInfo;
    assetInfo.fileId_ = TEST_FILE_ID;
    assetInfo.syncStatus_ = static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
    assetInfo.cleanFlag_ = static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN);
    assetInfo.dateTrashedMs_ = 0;
    assetInfo.isHidden_ = false;
    assetInfo.timePending_ = 0;
    assetInfo.isTemp_ = false;
    assetInfo.burstCoverLevel_ = static_cast<int32_t>(BurstCoverLevelType::COVER);
    assetInfo.isShared_ = isShared;
    return assetInfo;
}
} // namespace

/*
 * 用例说明：共享相册判定，type 与 subType 同时命中
 * 覆盖分支点：IsShareAlbum 两个条件均为真
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbum_test_001, TestSize.Level0)
{
    EXPECT_TRUE(PhotoAlbum::IsShareAlbum(PhotoAlbumType::SHARE, PhotoAlbumSubType::SHARE_GENERIC));
}

/*
 * 用例说明：共享相册判定，subType 不匹配
 * 覆盖分支点：IsShareAlbum 第二个条件为假
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbum_test_002, TestSize.Level0)
{
    EXPECT_FALSE(PhotoAlbum::IsShareAlbum(PhotoAlbumType::SHARE, PhotoAlbumSubType::USER_GENERIC));
}

/*
 * 用例说明：共享相册判定，type 不匹配
 * 覆盖分支点：IsShareAlbum 第一个条件为假（短路）
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbum_test_003, TestSize.Level0)
{
    EXPECT_FALSE(PhotoAlbum::IsShareAlbum(PhotoAlbumType::USER, PhotoAlbumSubType::SHARE_GENERIC));
}

/*
 * 用例说明：变更前的相册是共享相册
 * 覆盖分支点：IsShareAlbumChangeInfo 第一个 IsShareAlbum 为真
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbumChangeInfo_test_001, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));

    EXPECT_TRUE(execution.IsShareAlbumChangeInfo(changeData));
}

/*
 * 用例说明：只有变更后的相册是共享相册
 * 覆盖分支点：IsShareAlbumChangeInfo 第一个 IsShareAlbum 为假、第二个为真
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbumChangeInfo_test_002, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.infoBeforeChange_.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    changeData.infoBeforeChange_.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    EXPECT_TRUE(execution.IsShareAlbumChangeInfo(changeData));
}

/*
 * 用例说明：变更前后都不是共享相册
 * 覆盖分支点：IsShareAlbumChangeInfo 两个 IsShareAlbum 均为假
 */
HWTEST_F(ShareAssetNotifyTest, IsShareAlbumChangeInfo_test_003, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.infoBeforeChange_.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    changeData.infoBeforeChange_.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    changeData.infoAfterChange_.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    changeData.infoAfterChange_.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    EXPECT_FALSE(execution.IsShareAlbumChangeInfo(changeData));
}

/*
 * 用例说明：共享相册新增 -> 插入 ALBUM_OPERATION_ADD_SHARE，且变更前 albumId 置无效
 * 覆盖分支点：HandleInsertShareNotifyInfo 中 operation_ == RDB_OPERATION_ADD
 */
HWTEST_F(ShareAssetNotifyTest, HandleInsertShareNotifyInfo_test_001, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_ADD;

    execution.HandleInsertShareNotifyInfo(changeData);

    ASSERT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_[ALBUM_OPERATION_ADD_SHARE].size(), 1);
    EXPECT_EQ(execution.notifyInfos_[ALBUM_OPERATION_ADD_SHARE][0].infoBeforeChange_.albumId_, INVALID_INT32_VALUE);
}

/*
 * 用例说明：共享相册移除 -> 插入 ALBUM_OPERATION_REMOVE_SHARE，且标记删除、变更后 albumId 置无效
 * 覆盖分支点：HandleInsertShareNotifyInfo 中 operation_ == RDB_OPERATION_REMOVE
 */
HWTEST_F(ShareAssetNotifyTest, HandleInsertShareNotifyInfo_test_002, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_REMOVE;

    execution.HandleInsertShareNotifyInfo(changeData);

    EXPECT_TRUE(changeData.isDelete_);
    ASSERT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_REMOVE_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_[ALBUM_OPERATION_REMOVE_SHARE][0].infoAfterChange_.albumId_, INVALID_INT32_VALUE);
}

/*
 * 用例说明：共享相册更新，dirty 由非删除变为删除 -> 走删除逻辑
 * 覆盖分支点：HandleInsertShareNotifyInfo 中 UPDATE 的 isDelete 为真
 */
HWTEST_F(ShareAssetNotifyTest, HandleInsertShareNotifyInfo_test_003, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_DELETED));
    changeData.operation_ = RDB_OPERATION_UPDATE;

    execution.HandleInsertShareNotifyInfo(changeData);

    EXPECT_TRUE(changeData.isDelete_);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_REMOVE_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_UPDATE_SHARE), 0);
}

/*
 * 用例说明：共享相册更新，dirty 由删除变为非删除 -> 走新增逻辑
 * 覆盖分支点：HandleInsertShareNotifyInfo 中 UPDATE 的 isCreate 为真
 */
HWTEST_F(ShareAssetNotifyTest, HandleInsertShareNotifyInfo_test_004, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_DELETED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_UPDATE;

    execution.HandleInsertShareNotifyInfo(changeData);

    EXPECT_FALSE(changeData.isDelete_);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_UPDATE_SHARE), 0);
}

/*
 * 用例说明：共享相册更新，dirty 未跨越删除状态 -> 走普通更新
 * 覆盖分支点：HandleInsertShareNotifyInfo 中 UPDATE 的 isDelete/isCreate 均为假
 */
HWTEST_F(ShareAssetNotifyTest, HandleInsertShareNotifyInfo_test_005, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_UPDATE;

    execution.HandleInsertShareNotifyInfo(changeData);

    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_UPDATE_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD_SHARE), 0);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_REMOVE_SHARE), 0);
}

/*
 * 用例说明：共享资产新增 -> 返回 ASSET_OPERATION_ADD_SHARE
 * 覆盖分支点：GetAddOperation 中 ShareAssetHelper::IsAsset 为真
 */
HWTEST_F(ShareAssetNotifyTest, GetAddOperation_test_001, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;

    EXPECT_EQ(execution.GetAddOperation(MakeCommonSystemAsset(1)), ASSET_OPERATION_ADD_SHARE);
}

/*
 * 用例说明：非共享的普通系统资产新增 -> 仍返回 ASSET_OPERATION_ADD
 * 覆盖分支点：GetAddOperation 中 ShareAssetHelper::IsAsset 为假
 */
HWTEST_F(ShareAssetNotifyTest, GetAddOperation_test_002, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;

    EXPECT_EQ(execution.GetAddOperation(MakeCommonSystemAsset(0)), ASSET_OPERATION_ADD);
}

/*
 * 用例说明：共享资产移除 -> 返回 ASSET_OPERATION_REMOVE_SHARE
 * 覆盖分支点：GetRemoveOperation 中 ShareAssetHelper::IsAsset 为真
 */
HWTEST_F(ShareAssetNotifyTest, GetRemoveOperation_test_001, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;

    EXPECT_EQ(execution.GetRemoveOperation(MakeCommonSystemAsset(1)), ASSET_OPERATION_REMOVE_SHARE);
}

/*
 * 用例说明：非共享的普通系统资产移除 -> 仍返回 ASSET_OPERATION_REMOVE
 * 覆盖分支点：GetRemoveOperation 中 ShareAssetHelper::IsAsset 为假
 */
HWTEST_F(ShareAssetNotifyTest, GetRemoveOperation_test_002, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;

    EXPECT_EQ(execution.GetRemoveOperation(MakeCommonSystemAsset(0)), ASSET_OPERATION_REMOVE);
}

/*
 * 用例说明：共享资产序列化/反序列化，共享字段完整传递
 * 覆盖分支点：PhotoAssetChangeInfo::Marshalling/ReadFromParcel 中 isShared_ != 0
 */
HWTEST_F(ShareAssetNotifyTest, PhotoAssetChangeInfoMarshalling_test_001, TestSize.Level0)
{
    PhotoAssetChangeInfo assetInfo = MakeCommonSystemAsset(1);
    assetInfo.shareDateDay_ = TEST_SHARE_DATE_DAY;
    assetInfo.shareGroup_ = TEST_SHARE_GROUP;
    assetInfo.shareRiskStatus_ = TEST_SHARE_RISK_STATUS;
    assetInfo.photoVisibility_ = TEST_PHOTO_VISIBILITY;

    MessageParcel parcel;
    ASSERT_TRUE(assetInfo.Marshalling(parcel, true));
    parcel.RewindRead(0);
    PhotoAssetChangeInfo readInfo;
    ASSERT_TRUE(readInfo.ReadFromParcel(parcel));

    EXPECT_EQ(readInfo.isShared_, 1);
    EXPECT_EQ(readInfo.shareDateDay_, TEST_SHARE_DATE_DAY);
    EXPECT_EQ(readInfo.shareGroup_, TEST_SHARE_GROUP);
    EXPECT_EQ(readInfo.shareRiskStatus_, TEST_SHARE_RISK_STATUS);
    EXPECT_EQ(readInfo.photoVisibility_, TEST_PHOTO_VISIBILITY);
}

/*
 * 用例说明：非共享资产序列化/反序列化，不写共享字段，读回为默认值
 * 覆盖分支点：PhotoAssetChangeInfo::Marshalling/ReadFromParcel 中 isShared_ == 0
 */
HWTEST_F(ShareAssetNotifyTest, PhotoAssetChangeInfoMarshalling_test_002, TestSize.Level0)
{
    PhotoAssetChangeInfo assetInfo = MakeCommonSystemAsset(0);
    assetInfo.shareDateDay_ = TEST_SHARE_DATE_DAY;

    MessageParcel parcel;
    ASSERT_TRUE(assetInfo.Marshalling(parcel, true));
    parcel.RewindRead(0);
    PhotoAssetChangeInfo readInfo;
    ASSERT_TRUE(readInfo.ReadFromParcel(parcel));

    EXPECT_EQ(readInfo.isShared_, 0);
    EXPECT_EQ(readInfo.shareDateDay_, static_cast<int64_t>(INVALID_INT64_VALUE));
}

/*
 * 用例说明：共享相册序列化/反序列化，shareRiskStatus 完整传递
 * 覆盖分支点：AlbumChangeInfo::Marshalling/ReadFromParcel 中 IsShareAlbum 为真
 */
HWTEST_F(ShareAssetNotifyTest, AlbumChangeInfoMarshalling_test_001, TestSize.Level0)
{
    AlbumChangeInfo albumInfo = MakeShareAlbumInfo();
    albumInfo.shareRiskStatus_ = TEST_SHARE_RISK_STATUS;

    MessageParcel parcel;
    ASSERT_TRUE(albumInfo.Marshalling(parcel, true));
    parcel.RewindRead(0);
    AlbumChangeInfo readInfo;
    ASSERT_TRUE(readInfo.ReadFromParcel(parcel));

    EXPECT_EQ(readInfo.shareRiskStatus_, TEST_SHARE_RISK_STATUS);
}

/*
 * 用例说明：普通相册序列化/反序列化，不写 shareRiskStatus，读回为默认值
 * 覆盖分支点：AlbumChangeInfo::Marshalling/ReadFromParcel 中 IsShareAlbum 为假
 */
HWTEST_F(ShareAssetNotifyTest, AlbumChangeInfoMarshalling_test_002, TestSize.Level0)
{
    AlbumChangeInfo albumInfo;
    albumInfo.albumId_ = TEST_ALBUM_ID;
    albumInfo.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    albumInfo.shareRiskStatus_ = TEST_SHARE_RISK_STATUS;

    MessageParcel parcel;
    ASSERT_TRUE(albumInfo.Marshalling(parcel, true));
    parcel.RewindRead(0);
    AlbumChangeInfo readInfo;
    ASSERT_TRUE(readInfo.ReadFromParcel(parcel));

    EXPECT_EQ(readInfo.shareRiskStatus_, 0);
}

/*
 * 用例说明：共享资产更新 -> 走共享更新分支，插入 ASSET_OPERATION_UPDATE_SHARE
 * 覆盖分支点：AssetChangeNotifyExecution::Notify 中 UPDATE 的 isShare 为真
 */
HWTEST_F(ShareAssetNotifyTest, NotifyUpdateShareAsset_test_001, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;
    vector<PhotoAssetChangeData> changeDatas;
    PhotoAssetChangeData changeData;
    changeData.operation_ = RDB_OPERATION_UPDATE;
    changeData.infoBeforeChange_ = MakeCommonSystemAsset(1);
    changeData.infoAfterChange_ = MakeCommonSystemAsset(1);
    changeDatas.push_back(changeData);

    execution.Notify(changeDatas);

    EXPECT_EQ(execution.notifyInfos_.count(ASSET_OPERATION_UPDATE_SHARE), 1);
}

/*
 * 用例说明：非共享资产更新 -> 走原有普通/回收站/隐藏更新分支，不插入共享操作
 * 覆盖分支点：AssetChangeNotifyExecution::Notify 中 UPDATE 的 isShare 为假
 */
HWTEST_F(ShareAssetNotifyTest, NotifyUpdateShareAsset_test_002, TestSize.Level0)
{
    AssetChangeNotifyExecution execution;
    vector<PhotoAssetChangeData> changeDatas;
    PhotoAssetChangeData changeData;
    changeData.operation_ = RDB_OPERATION_UPDATE;
    changeData.infoBeforeChange_ = MakeCommonSystemAsset(0);
    changeData.infoAfterChange_ = MakeCommonSystemAsset(0);
    changeDatas.push_back(changeData);

    execution.Notify(changeDatas);

    EXPECT_EQ(execution.notifyInfos_.count(ASSET_OPERATION_UPDATE_SHARE), 0);
}

/*
 * 用例说明：共享相册变更经 Notify 入口 -> 走共享分支并 continue，不再走普通相册分支
 * 覆盖分支点：AlbumChangeNotifyExecution::Notify 中 IsShareAlbumChangeInfo 为真
 */
HWTEST_F(ShareAssetNotifyTest, NotifyShareAlbumChange_test_001, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    vector<AlbumChangeData> changeDatas;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_ADD;
    changeDatas.push_back(changeData);

    execution.Notify(changeDatas);

    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD_SHARE), 1);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD), 0);
}

/*
 * 用例说明：非共享相册变更经 Notify 入口 -> 仍走原有相册分支
 * 覆盖分支点：AlbumChangeNotifyExecution::Notify 中 IsShareAlbumChangeInfo 为假
 */
HWTEST_F(ShareAssetNotifyTest, NotifyShareAlbumChange_test_002, TestSize.Level0)
{
    AlbumChangeNotifyExecution execution;
    vector<AlbumChangeData> changeDatas;
    AlbumChangeData changeData = MakeShareAlbumChangeData(static_cast<int32_t>(DirtyType::TYPE_SYNCED),
        static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    changeData.operation_ = RDB_OPERATION_ADD;
    changeData.infoBeforeChange_.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    changeData.infoBeforeChange_.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    changeData.infoAfterChange_.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    changeData.infoAfterChange_.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    changeDatas.push_back(changeData);

    execution.Notify(changeDatas);

    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD), 1);
    EXPECT_EQ(execution.notifyInfos_.count(ALBUM_OPERATION_ADD_SHARE), 0);
}

/*
 * 用例说明：资产差异描述，fileId 不同 -> 提前返回，只输出 fileId 差异
 * 覆盖分支点：CompareAssetFileIdAndUri 中 fileId 不一致为真
 */
HWTEST_F(ShareAssetNotifyTest, GetAssetDiff_test_001, TestSize.Level0)
{
    PhotoAssetChangeInfo assetInfo = MakeCommonSystemAsset(0);
    assetInfo.uri_ = "file://media/Photo/1";
    PhotoAssetChangeInfo compareInfo = MakeCommonSystemAsset(0);
    compareInfo.fileId_ = TEST_FILE_ID + 1;
    compareInfo.uri_ = "file://media/Photo/2";

    string diff = assetInfo.GetAssetDiff(assetInfo, compareInfo);

    EXPECT_NE(diff.find("diff asset info fileId"), string::npos);
    // 提前返回：连 uri_ 都不再比较
    EXPECT_EQ(diff.find("uri_"), string::npos);
}

/*
 * 用例说明：资产差异描述，fileId 相同但 uri 不同 -> 输出 uri 差异并继续比对其它字段
 * 覆盖分支点：CompareAssetFileIdAndUri 中 fileId 一致（返回 true）、uri 不一致为真
 */
HWTEST_F(ShareAssetNotifyTest, GetAssetDiff_test_002, TestSize.Level0)
{
    PhotoAssetChangeInfo assetInfo = MakeCommonSystemAsset(0);
    assetInfo.uri_ = "file://media/Photo/1";
    PhotoAssetChangeInfo compareInfo = MakeCommonSystemAsset(0);
    compareInfo.uri_ = "file://media/Photo/2";

    string diff = assetInfo.GetAssetDiff(assetInfo, compareInfo);

    EXPECT_NE(diff.find("uri_"), string::npos);
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
