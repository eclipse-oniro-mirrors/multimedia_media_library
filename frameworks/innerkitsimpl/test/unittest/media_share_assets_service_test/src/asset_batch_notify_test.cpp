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

#define MLOG_TAG "AssetBatchNotifyTest"

#include "media_share_assets_test_utils.h"

#include <string>
#include <vector>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "notify/asset_batch_notify.h"

using namespace std;
using namespace testing::ext;

namespace OHOS::Media {

namespace {
std::vector<std::string> BuildFileIds(size_t count)
{
    std::vector<std::string> fileIds;
    fileIds.reserve(count);
    for (size_t i = 0; i < count; i++) {
        fileIds.emplace_back(std::to_string(i + 1));
    }
    return fileIds;
}
} // namespace

/**
 * @tc.name: TryNotifyAssetsChange_SmallBatch_KeepsPendingData
 * @tc.desc: 未达到批量阈值时数据仅缓存不通知(覆盖阈值判断为 false 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryNotifyAssetsChange_SmallBatch_KeepsPendingData, TestSize.Level1)
{
    AssetBatchNotify batchNotify;
    std::vector<std::string> fileIds = BuildFileIds(2);
    EXPECT_EQ(batchNotify.TryNotifyAssetsChange(fileIds), E_OK);
    EXPECT_EQ(batchNotify.fileIds_.size(), 2u);
    EXPECT_EQ(batchNotify.totalFileCount_, 2);
    EXPECT_EQ(batchNotify.FinalNotifyAssetsChange(), E_OK);
}

/**
 * @tc.name: TryNotifyAssetsChange_ReachBatchLimit_FlushesCache
 * @tc.desc: 缓存达到 BATCH_NOTIFY_CLOUD_FILE 时立即批量通知并清空缓存(覆盖阈值判断为 true 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryNotifyAssetsChange_ReachBatchLimit_FlushesCache, TestSize.Level1)
{
    AssetBatchNotify batchNotify;
    std::vector<std::string> fileIds = BuildFileIds(static_cast<size_t>(batchNotify.BATCH_NOTIFY_CLOUD_FILE));
    EXPECT_EQ(batchNotify.TryNotifyAssetsChange(fileIds), E_OK);

    // 达到阈值即触发一次批量通知(内部走 NotifyAssetsChange 的非空入参路径)并清空缓存
    EXPECT_TRUE(batchNotify.fileIds_.empty());
    EXPECT_EQ(batchNotify.totalFileCount_, batchNotify.BATCH_NOTIFY_CLOUD_FILE);
}

/**
 * @tc.name: FinalNotifyAssetsChange_NoPendingData_ReturnsOk
 * @tc.desc: 没有待通知数据时直接返回成功(覆盖提前返回分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, FinalNotifyAssetsChange_NoPendingData_ReturnsOk, TestSize.Level1)
{
    AssetBatchNotify batchNotify;
    EXPECT_EQ(batchNotify.FinalNotifyAssetsChange(), E_OK);
    EXPECT_EQ(batchNotify.totalFileCount_, 0);
}

/**
 * @tc.name: FinalNotifyAssetsChange_WithPendingData_NotifiesAndUpdatesAlbums
 * @tc.desc: 收尾时通知剩余资产并触发全量相册刷新(覆盖非空分支与相册刷新分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, FinalNotifyAssetsChange_WithPendingData_NotifiesAndUpdatesAlbums, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(1, "share_album_1"), E_OK);

    AssetBatchNotify batchNotify;
    std::vector<std::string> fileIds = BuildFileIds(3);
    ASSERT_EQ(batchNotify.TryNotifyAssetsChange(fileIds), E_OK);

    EXPECT_EQ(batchNotify.FinalNotifyAssetsChange(), E_OK);
    EXPECT_TRUE(batchNotify.fileIds_.empty());
    EXPECT_EQ(batchNotify.totalFileCount_, 3);
}

/**
 * @tc.name: TryUpdateAllAlbums_NoFileCount_ReturnsOk
 * @tc.desc: 未统计到任何文件时不做全量相册刷新(覆盖 totalFileCount_ 为 0 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryUpdateAllAlbums_NoFileCount_ReturnsOk, TestSize.Level1)
{
    AssetBatchNotify batchNotify;
    EXPECT_EQ(batchNotify.TryUpdateAllAlbums(), E_OK);
    EXPECT_EQ(batchNotify.totalFileCount_, 0);
}

/**
 * @tc.name: TryUpdateAllAlbums_WithFileCount_ReturnsOk
 * @tc.desc: 已统计到文件时执行全量相册刷新(覆盖 totalFileCount_ 大于 0 的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryUpdateAllAlbums_WithFileCount_ReturnsOk, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(1, "share_album_1"), E_OK);

    AssetBatchNotify batchNotify;
    std::vector<std::string> fileIds = BuildFileIds(1);
    ASSERT_EQ(batchNotify.TryNotifyAssetsChange(fileIds), E_OK);
    EXPECT_EQ(batchNotify.TryUpdateAllAlbums(), E_OK);
    EXPECT_EQ(batchNotify.totalFileCount_, 1);
}

} // namespace OHOS::Media