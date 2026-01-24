/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileExtUnitTest"

#include <thread>
#include "ability_context_impl.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_smartalbum_operations.h"
#include "medialibrary_smartalbum_operations_test.h"
#include "smart_album_column.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

void MediaLibrarySmartalbumOperationTest::SetUpTestCase(void) {}

void MediaLibrarySmartalbumOperationTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibrarySmartalbumOperationTest::SetUp() {}

void MediaLibrarySmartalbumOperationTest::TearDown(void) {}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_HandleSmartAlbumOperation_test_001, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd1);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmd2(OperationObject::FILESYSTEM_ASSET, OperationType::UNKNOWN_TYPE);
    ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd2);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_HandleSmartAlbumOperation_test_002, TestSize.Level0)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    string name = "medialib_HandleSmartAlbumOperation";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(cmd);
    EXPECT_GT(ret, 0);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_CreateSmartAlbumOperation_test_001, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_PARENT_SMARTALBUM_IS_NOT_EXISTED);
    MediaLibraryUnitTestUtils::InitUnistore();
    ret = MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    string name = "CreateSmartAlbumOperation/test";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_INVALID_VALUES);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_CreateSmartAlbumOperation_test_002, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    MediaLibraryUnitTestUtils::InitUnistore();
    string name = "medialib_CreateSmartAlbumOperation";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(cmd);
    EXPECT_GT(ret, 0);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_DeleteSmartAlbumOperation_test_001, TestSize.Level0)
{
    MediaLibraryUnitTestUtils::InitUnistore();
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t ret = MediaLibrarySmartAlbumOperations::DeleteSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUM_DB_ID, DEFAULT_DIR_TYPE);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumOperations::DeleteSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_GET_VALUEBUCKET_FAIL);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumOperationTest, medialib_DeleteSmartAlbumOperation_test_002, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket values;
    cmd.SetTableName(CONST_MEDIALIBRARY_TABLE);
    values.PutInt(SMARTALBUM_DB_ID, TYPE_TRASH);
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.GetAbsRdbPredicates()->SetWhereClause("1 <> 0");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibrarySmartAlbumOperations::DeleteSmartAlbumOperation(cmd);
    EXPECT_EQ(ret, E_DELETE_SMARTALBUM_MAP_FAIL);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}
} // namespace Media
} // namespace OHOS