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

#define MLOG_TAG "MediaLibraryAlbumCoverOrderTest"

#include "medialibrary_album_cover_order_test.h"
#include <vector>
#include <memory>
#include "default_cover_order_info.h"
#include "medialibrary_album_operations.h"
#include "media_log.h"
#include "userfile_manager_types.h"
#include "medialibrary_rdb_utils.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
void MediaLibraryAlbumCoverOrderTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void MediaLibraryAlbumCoverOrderTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void MediaLibraryAlbumCoverOrderTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaLibraryAlbumCoverOrderTest::TearDown()
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaLibraryAlbumCoverOrderTest, ModifyAlbumDefaultCoverOrder_EmptyCoverOrderInfos_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ModifyAlbumDefaultCoverOrder_EmptyCoverOrderInfos_Test");
    std::vector<DefaultCoverOrderInfo> coverOrderInfos;
    int32_t ret = MediaLibraryAlbumOperations::ModifyAlbumDefaultCoverOrder(coverOrderInfos, false, false);
    EXPECT_EQ(ret, E_INVALID_ARGS);
}

HWTEST_F(MediaLibraryAlbumCoverOrderTest, ModifyHiddenAlbumDefaultCoverOrder_EmptyCoverOrderInfos_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ModifyHiddenAlbumDefaultCoverOrder_EmptyCoverOrderInfos_Test");
    std::vector<DefaultCoverOrderInfo> coverOrderInfos;
    int32_t ret = MediaLibraryAlbumOperations::ModifyHiddenAlbumDefaultCoverOrder(coverOrderInfos, false, false);
    EXPECT_EQ(ret, E_INVALID_ARGS);
}

HWTEST_F(MediaLibraryAlbumCoverOrderTest, ModifyAlbumDefaultCoverOrder_SystemAlbum_ValidOrder_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ModifyAlbumDefaultCoverOrder_SystemAlbum_ValidOrder_Test");
    std::vector<DefaultCoverOrderInfo> coverOrderInfos;
    DefaultCoverOrderInfo info;
    info.albumType = static_cast<int32_t>(PhotoAlbumType::SYSTEM);
    info.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::CAMERA);
    info.lpath = "";
    info.orderKey = "date_taken";
    info.orderSubKey = "display_name";
    info.orderType = 0;
    coverOrderInfos.push_back(info);

    int32_t ret = MediaLibraryAlbumOperations::ModifyAlbumDefaultCoverOrder(coverOrderInfos, false, false);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}
} // namespace OHOS::Media