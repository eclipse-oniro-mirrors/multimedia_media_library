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

#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_smartalbum_map_operations_test.h"
#include "medialibrary_smartalbum_map_operations.h"
#include "medialibrary_unistore_manager.h"
#include "ability_context_impl.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void MediaLibrarySmartalbumMapOperationTest::SetUpTestCase(void) {}

void MediaLibrarySmartalbumMapOperationTest::TearDownTestCase(void) {}

//SetUp:Execute before each test case
void MediaLibrarySmartalbumMapOperationTest::SetUp() {}

void MediaLibrarySmartalbumMapOperationTest::TearDown(void) {}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleSmartAlbumMapOperation_test_001, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);

    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd1);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);

    MediaLibraryCommand cmd2(OperationObject::FILESYSTEM_ASSET, OperationType::AGING);
    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd1);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);

    MediaLibraryCommand cmd3(OperationObject::FILESYSTEM_ASSET, OperationType::UNKNOWN_TYPE);
    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd2);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleSmartAlbumMapOperation_test_002, TestSize.Level0)
{
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);

    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    EXPECT_EQ(ret, E_GET_VALUEBUCKET_FAIL);

    string name = "smartAlbumMap";
    values.PutString(SMARTALBUMMAP_TABLE_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    EXPECT_EQ(ret, E_GET_VALUEBUCKET_FAIL);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleSmartAlbumMapOperation_test_003, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    NativeRdb::ValuesBucket values;
    cmd.SetValueBucket(values);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);

    ret = MediaLibrarySmartAlbumMapOperations::HandleSmartAlbumMapOperation(cmd);
    cmd.SetValueBucket(values);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleAddAssetOperation_test_001, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(0, 0, 0, cmd);
    EXPECT_EQ(ret, E_CHILD_CAN_NOT_ADD_SMARTALBUM);

    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(0, 0, 0, cmd);
    EXPECT_EQ(ret, E_CHILD_CAN_NOT_ADD_SMARTALBUM);

    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(0, 0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(-1, -1, -1, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    string name = "HandleAddAssetOperation/test";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(0, 0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleAddAssetOperation_test_002, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    string name = "medialib_HandleAddAssetOperation";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(100, 0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleAddAssetOperation_test_003, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);

    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(2, -1, -1, cmd);
    EXPECT_EQ(ret, E_GET_ASSET_FAIL);

    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(2, 0, -1, cmd);
    EXPECT_EQ(ret, E_GET_ASSET_FAIL);

    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(2, -1, 0, cmd);
    EXPECT_EQ(ret, E_GET_ASSET_FAIL);

    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(1, -1, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    
    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(1, 0, -1, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    
    ret = MediaLibrarySmartAlbumMapOperations::HandleAddAssetOperation(1, 0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleRemoveAssetOperation_test_001, TestSize.Level0)
{
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUM_DB_ID, DEFAULT_DIR_TYPE);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(2, 0, cmd);
    EXPECT_EQ(ret, E_GET_ASSET_FAIL);

    ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(2, -1, cmd);
    EXPECT_EQ(ret, E_GET_ASSET_FAIL);

    ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(1, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    
    ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(1, -1, cmd);
    EXPECT_EQ(ret, E_INVALID_FILEID);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleRemoveAssetOperation_test_002, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::DELETE);
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    NativeRdb::ValuesBucket values;
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    values.PutInt(SMARTALBUM_DB_ID, TYPE_TRASH);
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.GetAbsRdbPredicates()->SetWhereClause("1 <> 0");
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleRemoveAssetOperation(0, 0, cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleAgingOperation_test_001, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::AGING);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
    EXPECT_EQ(ret, E_OK);
    string name = "HandleAgingOperation/test";
    values.PutString(SMARTALBUM_DB_NAME, name);
    cmd.SetValueBucket(values);
    ret = MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
    EXPECT_EQ(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibrarySmartalbumMapOperationTest, medialib_HandleAgingOperation_test_002, TestSize.Level0)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::AGING);
    cmd.SetTableName(MEDIALIBRARY_TABLE);
    NativeRdb::ValuesBucket values;
    values.PutInt(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    auto context = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    MediaLibraryUnistoreManager::GetInstance().Init(context);
    string name = "medialib_HandleAgingOperation";
    values.PutString(SMARTALBUMMAP_TABLE_NAME, name);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
    EXPECT_EQ(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}
} // namespace Media
} // namespace OHOSfu