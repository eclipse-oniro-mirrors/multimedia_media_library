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

#include "media_log.h"
#include "medialibrary_rdb_test.h"
#include "medialibrary_dir_operations.h"
#include "medialibrary_unistore_manager.h"
#include "ability_context_impl.h"
#include "rdb_utils.h"
#include "uri.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const string TDD_TEST_PATH = "/storage/cloud/files/Photo/";

HWTEST_F(MediaLibraryRdbTest, medialib_HandleDirOperation_test_001, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_ASSET, OperationType::TRASH);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd1);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryCommand cmd2(OperationObject::FILESYSTEM_ASSET, OperationType::UNKNOWN_TYPE);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd2);
    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(MediaLibraryRdbTest, medialib_HandleDirOperation_test_002, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket values;
    cmd.SetValueBucket(values);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    values.PutString(CONST_MEDIA_DATA_DB_NAME, ".nofile");
    values.PutString(CONST_MEDIA_DATA_DB_RELATIVE_PATH, "medialib_HandleDirOperation_testCase_002");
    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    cmd1.SetValueBucket(values);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd1);
    EXPECT_NE(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryRdbTest, medialib_HandleDirOperation_test_003, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::TRASH);
    int32_t ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket valuesBucket;
    cmd.SetValueBucket(valuesBucket);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    valuesBucket.PutString(TDD_TEST_PATH, "medialib_handleTestCase_01");
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_ID, 0);
    cmd.SetValueBucket(valuesBucket);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_GET_VALUEBUCKET_FAIL);

    MediaLibraryCommand cmd1(OperationObject::FILESYSTEM_ASSET, OperationType::TRASH);
    NativeRdb::ValuesBucket values;
    values.PutString(TDD_TEST_PATH, "medialib_handleTestCase_02");
    values.PutInt(CONST_MEDIA_DATA_DB_ID, 10);
    cmd1.SetValueBucket(values);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd1);
    EXPECT_NE(ret, E_OK);

    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryRdbTest, medialib_HandleDirOperation_test_004, TestSize.Level1)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_ASSET, OperationType::UNKNOWN_TYPE);
    int32_t ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_FAIL);

    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket values;
    values.PutString(TDD_TEST_PATH, "medialib_HandleDirOperation_test_004");
    values.PutInt(CONST_MEDIA_DATA_DB_ID, 11);
    cmd.SetValueBucket(values);
    ret = MediaLibraryDirOperations::HandleDirOperation(cmd);
    EXPECT_EQ(ret, E_FAIL);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryRdbTest, medialib_CreateDirOperation_test_001, TestSize.Level1)
{
    string queryUri = MEDIALIBRARY_DATA_URI;
    Uri uri(queryUri);
    MediaLibraryCommand cmd(uri, OperationType::CREATE);
    int32_t ret = MediaLibraryDirOperations::CreateDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket values;
    MediaLibraryCommand cmd1(uri, OperationType::CREATE);
    cmd1.SetValueBucket(values);
    ret = MediaLibraryDirOperations::CreateDirOperation(cmd1);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    values.PutString(CONST_MEDIA_DATA_DB_NAME, ".nofile");
    values.PutString(CONST_MEDIA_DATA_DB_RELATIVE_PATH, "medialib_CreateDirOperation_test_001");
    MediaLibraryCommand cmd2(uri, OperationType::CREATE);
    cmd2.SetValueBucket(values);
    ret = MediaLibraryDirOperations::CreateDirOperation(cmd2);
    EXPECT_EQ((ret > E_OK || ret == E_FILE_EXIST), true);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryRdbTest, medialib_TrashDirOperation_test_001, TestSize.Level1)
{
    string queryUri = MEDIALIBRARY_DATA_URI;
    Uri uri(queryUri);
    MediaLibraryCommand cmd(uri, OperationType::QUERY);
    int32_t ret = MediaLibraryDirOperations::TrashDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    MediaLibraryUnitTestUtils::InitUnistore();
    NativeRdb::ValuesBucket values;
    cmd.SetValueBucket(values);
    ret = MediaLibraryDirOperations::TrashDirOperation(cmd);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);

    values.PutString(TDD_TEST_PATH, "medialib_TrashTestCase_01");
    values.PutInt(CONST_MEDIA_DATA_DB_ID, 0);
    cmd.SetValueBucket(values);
    ret = MediaLibraryDirOperations::TrashDirOperation(cmd);
    EXPECT_EQ(ret, E_GET_VALUEBUCKET_FAIL);

    MediaLibraryCommand cmd1(uri, OperationType::QUERY);
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(TDD_TEST_PATH, "medialib_TrashTestCase_02");
    valuesBucket.PutInt(CONST_MEDIA_DATA_DB_ID, 12);
    cmd1.SetValueBucket(valuesBucket);
    ret = MediaLibraryDirOperations::TrashDirOperation(cmd1);
    EXPECT_NE(ret, E_OK);
    MediaLibraryUnistoreManager::GetInstance().Stop();
}
}
}
