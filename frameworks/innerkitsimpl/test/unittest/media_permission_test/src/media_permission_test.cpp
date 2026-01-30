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

#define MLOG_TAG "MediaPermissionHandlerTest"

#include "media_permission_test.h"

#include "media_log.h"
#include "media_tool_permission_handler.h"
#include "grant_permission_handler.h"
#include "read_write_permission_handler.h"
#include "db_permission_handler.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_uripermission_operations.h"
#include "permission_utils.h"
#include "parameters.h"
#include "medialibrary_operation.h"
#include "medialibrary_rdbstore.h"
#include "rdb_utils.h"
#include "context.h"
#include "ability_context_impl.h"
#include "get_self_permissions.h"
#include "medialibrary_unittest_utils.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

void MediaPermissionTest::SetUpTestCase(void) {}

void MediaPermissionTest::TearDownTestCase(void) {}

void MediaPermissionTest::SetUp(void) {}

void MediaPermissionTest::TearDown(void) {}

// MediaTool鉴权成功
HWTEST_F(MediaPermissionTest, MediaPermissionTest_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_001 begin");
    Uri uri("uri_permission");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::TOOL_PHOTO);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<MediaToolPermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    EXPECT_EQ(err, 0);
    MEDIA_INFO_LOG("MediaPermissionTest_001 end");
}

// MediaTool鉴权失败
HWTEST_F(MediaPermissionTest, MediaPermissionTest_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_002 begin");
    Uri uri("uri_permission");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<MediaToolPermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    EXPECT_LT(err, 0);
    MEDIA_INFO_LOG("MediaPermissionTest_002 end");
}

// readWrite鉴权成功
HWTEST_F(MediaPermissionTest, MediaPermissionTest_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_003 begin");
    Uri uri(CONST_URI_CLOSE_FILE); // HandleNoPermCheck 使得鉴权通过
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<ReadWritePermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    EXPECT_EQ(err, 0);
    MEDIA_INFO_LOG("MediaPermissionTest_003 end");
}


// readWrite鉴权失败
HWTEST_F(MediaPermissionTest, MediaPermissionTest_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_004 begin");
    Uri uri("uri_permission");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<ReadWritePermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    EXPECT_EQ(err, 0);
    MEDIA_INFO_LOG("MediaPermissionTest_004 end");
}

// grant鉴权失败
HWTEST_F(MediaPermissionTest, MediaPermissionTest_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_005 begin");
    Uri uri("uri_permission");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::UNKNOWN_OBJECT);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<GrantPermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    // unknown object will return E_PERMISSION_DENIED
    EXPECT_EQ(err, E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("MediaPermissionTest_005 end");
}

// grant鉴权失败
HWTEST_F(MediaPermissionTest, MediaPermissionTest_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionTest_006 begin");
    Uri uri("uri_permission");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    PermParam permParam = {
        .isWrite = true,
    };
    permissionHandler_ = std::make_shared<GrantPermissionHandler>();
    int err = permissionHandler_->CheckPermission(cmd, permParam);
    EXPECT_LT(err, 0);
    MEDIA_INFO_LOG("MediaPermissionTest_006 end");
}

//Media_Tool_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Media_Tool_Permission_Handler_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Media_Tool_Permission_Handler_Test_001 begin");
    Uri uri("test_tool");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<MediaToolPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("Media_Tool_Permission_Handler_Test_001 end");
}

//Media_Tool_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Media_Tool_Permission_Handler_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Media_Tool_Permission_Handler_Test_002 begin");
    Uri uri("test_tool");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = false,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<MediaToolPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("Media_Tool_Permission_Handler_Test_002 end");
}

//Read_Write_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Read_Write_Permission_Handler_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Read_Write_Permission_Handler_Test_001 begin");
    Uri uri("test_read_write");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = true,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<ReadWritePermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Read_Write_Permission_Handler_Test_001 end");
}

//Read_Write_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Read_Write_Permission_Handler_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Read_Write_Permission_Handler_Test_002 begin");
    Uri uri("test_read_write");
    MediaLibraryCommand cmd(uri);
    PermParam permParam = {
        .isWrite = false,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<ReadWritePermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Read_Write_Permission_Handler_Test_002 end");
}

//Grant_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Grant_Permission_Handler_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Grant_Permission_Handler_Test_001 begin");
    Uri uri("test_grant");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_DIR);
    PermParam permParam = {
        .isWrite = true,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<GrantPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Grant_Permission_Handler_Test_001 end");
}

//Grant_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Grant_Permission_Handler_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Grant_Permission_Handler_Test_002 begin");
    Uri uri("test_grant");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_DIR);
    PermParam permParam = {
        .isWrite = false,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<GrantPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Grant_Permission_Handler_Test_002 end");
}

//Db_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Db_Permission_Handler_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Db_Permission_Handler_Test_001 begin");
    Uri uri("test_grant");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_DIR);
    PermParam permParam = {
        .isWrite = true,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<DbPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Db_Permission_Handler_Test_001 end");
}

//Db_Permission_Handler_Test
HWTEST_F(MediaPermissionTest, Db_Permission_Handler_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Db_Permission_Handler_Test_002 begin");
    Uri uri("test_grant");
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_DIR);
    PermParam permParam = {
        .isWrite = false,
    };
    int32_t ret = -1;
    permissionHandler_ = std::make_shared<DbPermissionHandler>();
    ret = permissionHandler_->ExecuteCheckPermission(cmd, permParam);
    EXPECT_NE(ret, -1);
    MEDIA_INFO_LOG("Db_Permission_Handler_Test_002 end");
}
} // namespace Media
} // namespace OHOS
