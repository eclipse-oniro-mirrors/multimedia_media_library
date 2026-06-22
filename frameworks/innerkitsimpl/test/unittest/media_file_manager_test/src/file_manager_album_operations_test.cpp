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

#define MLOG_TAG "FileManagerAlbumOperationsTest"

#include "file_manager_album_operations_test.h"

#include "file_manager_album_operations.h"
#include "file_manager_asset_operations.h"
#include "medialibrary_errno.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void FileManagerAlbumOperationsTest::SetUpTestCase() {}
void FileManagerAlbumOperationsTest::TearDownTestCase() {}
void FileManagerAlbumOperationsTest::SetUp() {}
void FileManagerAlbumOperationsTest::TearDown() {}

// ==================== RenameFileManagerAlbum Parameter Validation Tests ====================

/**
 * @tc.name: RenameFileManagerAlbum_InvalidAlbumId_001
 * @tc.desc: Test RenameFileManagerAlbum with invalid album id (0), returns E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, RenameFileManagerAlbum_InvalidAlbumId_001, TestSize.Level1)
{
    int32_t ret = FileManagerAlbumOperations::RenameFileManagerAlbum("/storage/path", 0, "NewName");
    EXPECT_EQ(ret, E_ERR);
}

/**
 * @tc.name: RenameFileManagerAlbum_NegativeAlbumId_002
 * @tc.desc: Test RenameFileManagerAlbum with negative album id, returns E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, RenameFileManagerAlbum_NegativeAlbumId_002, TestSize.Level1)
{
    int32_t ret = FileManagerAlbumOperations::RenameFileManagerAlbum("/storage/path", -1, "NewName");
    EXPECT_EQ(ret, E_ERR);
}

/**
 * @tc.name: RenameFileManagerAlbum_EmptyNewName_003
 * @tc.desc: Test RenameFileManagerAlbum with empty new album name, returns E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, RenameFileManagerAlbum_EmptyNewName_003, TestSize.Level1)
{
    int32_t ret = FileManagerAlbumOperations::RenameFileManagerAlbum("/storage/path", 1, "");
    EXPECT_EQ(ret, E_ERR);
}

/**
 * @tc.name: RenameFileManagerAlbum_EmptyOldPath_004
 * @tc.desc: Test RenameFileManagerAlbum with empty old album path, returns E_ERR
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, RenameFileManagerAlbum_EmptyOldPath_004, TestSize.Level1)
{
    int32_t ret = FileManagerAlbumOperations::RenameFileManagerAlbum("", 1, "NewName");
    EXPECT_EQ(ret, E_ERR);
}

// ==================== MoveAssetsToFileManagerUpdateData Tests ====================

/**
 * @tc.name: MoveAssetsToFileManagerUpdateData_DefaultValues_001
 * @tc.desc: Test MoveAssetsToFileManagerUpdateData string fields are default empty
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, MoveAssetsToFileManagerUpdateData_DefaultValues_001, TestSize.Level1)
{
    MoveAssetsToFileManagerUpdateData data;
    EXPECT_TRUE(data.title.empty());
    EXPECT_TRUE(data.displayName.empty());
    EXPECT_TRUE(data.storagePath.empty());
    EXPECT_TRUE(data.sourcePath.empty());
}

/**
 * @tc.name: MoveAssetsToFileManagerUpdateData_AssignFields_002
 * @tc.desc: Test MoveAssetsToFileManagerUpdateData field assignment
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, MoveAssetsToFileManagerUpdateData_AssignFields_002, TestSize.Level1)
{
    MoveAssetsToFileManagerUpdateData data;
    data.mediaId = 42;
    data.title = "test_title";
    data.displayName = "test_file.jpg";
    data.storagePath = "/storage/media/local/files/Docs/test_file.jpg";
    data.sourcePath = "/storage/cloud/files/Photo/1/test_file.jpg";

    EXPECT_EQ(data.mediaId, 42);
    EXPECT_EQ(data.title, "test_title");
    EXPECT_EQ(data.displayName, "test_file.jpg");
    EXPECT_EQ(data.storagePath, "/storage/media/local/files/Docs/test_file.jpg");
    EXPECT_EQ(data.sourcePath, "/storage/cloud/files/Photo/1/test_file.jpg");
}

} // namespace Media
} // namespace OHOS
