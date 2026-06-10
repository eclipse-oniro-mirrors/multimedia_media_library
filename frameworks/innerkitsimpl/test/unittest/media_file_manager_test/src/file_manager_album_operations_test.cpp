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

// Forward declarations for static functions defined in .cpp files
namespace OHOS {
namespace Media {
std::string ReplaceLastSegment(const std::string &path, const std::string &newName);
std::string ConvertPath(const std::string &originalPath);
} // namespace Media
} // namespace OHOS

namespace OHOS {
namespace Media {
using namespace testing::ext;

void FileManagerAlbumOperationsTest::SetUpTestCase() {}
void FileManagerAlbumOperationsTest::TearDownTestCase() {}
void FileManagerAlbumOperationsTest::SetUp() {}
void FileManagerAlbumOperationsTest::TearDown() {}

// ==================== ReplaceLastSegment Tests ====================

/**
 * @tc.name: ReplaceLastSegment_NormalPath_001
 * @tc.desc: Test ReplaceLastSegment with a normal path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ReplaceLastSegment_NormalPath_001, TestSize.Level1)
{
    std::string result = ReplaceLastSegment("/storage/media/local/files/Docs/OldName", "NewName");
    EXPECT_EQ(result, "/storage/media/local/files/Docs/NewName");
}

/**
 * @tc.name: ReplaceLastSegment_RootSlash_002
 * @tc.desc: Test ReplaceLastSegment with root slash path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ReplaceLastSegment_RootSlash_002, TestSize.Level1)
{
    std::string result = ReplaceLastSegment("/OldName", "NewName");
    EXPECT_EQ(result, "/NewName");
}

/**
 * @tc.name: ReplaceLastSegment_TrailingSlash_003
 * @tc.desc: Test ReplaceLastSegment with path that ends with slash
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ReplaceLastSegment_TrailingSlash_003, TestSize.Level1)
{
    std::string result = ReplaceLastSegment("/storage/media/local/files/Docs/", "NewName");
    EXPECT_EQ(result, "/storage/media/local/files/Docs/NewName");
}

/**
 * @tc.name: ReplaceLastSegment_NoSlash_004
 * @tc.desc: Test ReplaceLastSegment with path without any slash, returns original path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ReplaceLastSegment_NoSlash_004, TestSize.Level1)
{
    std::string result = ReplaceLastSegment("NoSlashPath", "NewName");
    EXPECT_EQ(result, "NoSlashPath");
}

/**
 * @tc.name: ReplaceLastSegment_DeepPath_005
 * @tc.desc: Test ReplaceLastSegment with deeply nested path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ReplaceLastSegment_DeepPath_005, TestSize.Level1)
{
    std::string result = ReplaceLastSegment("/a/b/c/d/e/f", "newEnd");
    EXPECT_EQ(result, "/a/b/c/d/e/newEnd");
}

// ==================== ConvertPath Tests ====================

/**
 * @tc.name: ConvertPath_OldPrefix_001
 * @tc.desc: Test ConvertPath with old prefix replaces to new prefix
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ConvertPath_OldPrefix_001, TestSize.Level1)
{
    std::string result = ConvertPath("/storage/emulated/0/FromDocs/subdir/file.jpg");
    EXPECT_EQ(result, "/storage/media/local/files/Docs/subdir/file.jpg");
}

/**
 * @tc.name: ConvertPath_OtherPrefix_002
 * @tc.desc: Test ConvertPath with non-matching prefix returns original path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ConvertPath_OtherPrefix_002, TestSize.Level1)
{
    std::string path = "/storage/media/local/files/Docs/subdir/file.jpg";
    std::string result = ConvertPath(path);
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: ConvertPath_EmptyPath_003
 * @tc.desc: Test ConvertPath with empty path returns empty
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ConvertPath_EmptyPath_003, TestSize.Level1)
{
    std::string result = ConvertPath("");
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: ConvertPath_OldPrefixExact_004
 * @tc.desc: Test ConvertPath with exact old prefix and no sub-path
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ConvertPath_OldPrefixExact_004, TestSize.Level1)
{
    std::string result = ConvertPath("/storage/emulated/0/FromDocs/");
    EXPECT_EQ(result, "/storage/media/local/files/Docs/");
}

/**
 * @tc.name: ConvertPath_OldPrefixOnlyPart_005
 * @tc.desc: Test ConvertPath where old prefix is not exact match (extra chars)
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, ConvertPath_OldPrefixOnlyPart_005, TestSize.Level1)
{
    std::string result = ConvertPath("/storage/emulated/0/FromDocsExtra/file.jpg");
    EXPECT_EQ(result, "/storage/emulated/0/FromDocsExtra/file.jpg");
}

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
 * @tc.desc: Test MoveAssetsToFileManagerUpdateData default values
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, MoveAssetsToFileManagerUpdateData_DefaultValues_001, TestSize.Level1)
{
    MoveAssetsToFileManagerUpdateData data;
    // int32_t mediaId is uninitialized by default, test string fields are default empty
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

// ==================== Constants Tests ====================

/**
 * @tc.name: FileManagerConstants_RootPathPrefix_001
 * @tc.desc: Test file manager root path prefix constant value
 * @tc.type: FUNC
 */
HWTEST_F(FileManagerAlbumOperationsTest, FileManagerConstants_RootPathPrefix_001, TestSize.Level1)
{
    // ROOT_PATH_PREFIX is defined as a const in file_manager_album_operations.cpp
    // We verify the expected value through ConvertPath behavior
    std::string result = ConvertPath("/storage/emulated/0/FromDocs/test.jpg");
    EXPECT_TRUE(result.find("/storage/media/local/files/Docs/") == 0);
}

} // namespace Media
} // namespace OHOS
