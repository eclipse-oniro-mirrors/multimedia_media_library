/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoAlbumMergeOperationTest"

#include "photo_album_merge_operation_test.h"

#include <string>
#include <vector>

#include "photo_album_merge_operation.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS::Media {
static constexpr int32_t OLD_ALBUM_ID = 1;
static constexpr int32_t NEW_ALBUM_ID = 2;

void PhotoAlbumMergeOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void PhotoAlbumMergeOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotoAlbumMergeOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoAlbumMergeOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoAlbumMergeOperationTest, SetRdbStore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetRdbStore_Test_001");
    PhotoAlbumMergeOperation operation;
    PhotoAlbumMergeOperation &result = operation.SetRdbStore(nullptr);
    EXPECT_EQ(&result, &operation);
    MEDIA_INFO_LOG("SetRdbStore_Test_001 End");
}

HWTEST_F(PhotoAlbumMergeOperationTest, SetRdbStore_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetRdbStore_Test_002");
    PhotoAlbumMergeOperation operation;
    auto g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    PhotoAlbumMergeOperation &result = operation.SetRdbStore(g_rdbStore);
    EXPECT_EQ(&result, &operation);
    MEDIA_INFO_LOG("SetRdbStore_Test_002 End");
}

HWTEST_F(PhotoAlbumMergeOperationTest, MergeAlbum_Null_RdbStore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start MergeAlbum_Null_RdbStore_Test_001");
    PhotoAlbumMergeOperation operation;
    int32_t ret = operation.SetRdbStore(nullptr).MergeAlbum(OLD_ALBUM_ID, NEW_ALBUM_ID);
    EXPECT_EQ(ret, NativeRdb::E_ERROR);
    MEDIA_INFO_LOG("MergeAlbum_Null_RdbStore_Test_001 End");
}
}  // namespace OHOS::Media