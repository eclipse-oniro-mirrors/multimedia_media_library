/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "LakeFileOperationsTest"
#define private public

#include "lake_file_operations_test.h"

#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "lake_file_operations.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "media_string_utils.h"
#include "media_edit_utils.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace NativeRdb;

static const std::string TEST_SRC_PATH = "/data/test/lake_src_file.jpg";
static const std::string TEST_DEST_PATH = "/data/test/lake_dest_file.jpg";
static const std::string TEST_DIR_PATH = "/data/test/lake_dir";
static const std::string TEST_LAKE_PATH = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/test.jpg";
static const std::string TEST_CLOUD_DIR = "/storage/cloud/files";

void LakeFileOperationsTest::SetUpTestCase()
{
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

void LakeFileOperationsTest::TearDownTestCase()
{
    MediaFileUtils::DeleteFile(TEST_SRC_PATH);
    MediaFileUtils::DeleteFile(TEST_DEST_PATH);
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
}

void LakeFileOperationsTest::SetUp()
{
}

void LakeFileOperationsTest::TearDown()
{
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_001, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DEST_PATH;
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "test content";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaFileUtils::IsFileExists(srcPath), false);
    EXPECT_EQ(MediaFileUtils::IsFileExists(destPath), true);
    
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_002, TestSize.Level1)
{
    std::string srcPath = "/data/test/nonexistent_file.jpg";
    std::string destPath = TEST_DEST_PATH;
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_003, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DIR_PATH + "/subdir/file.jpg";
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "test content";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaFileUtils::IsFileExists(destPath), true);
    
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_004, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DEST_PATH;
    
    MediaFileUtils::CreateFile(srcPath);
    MediaFileUtils::CreateFile(destPath);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_001, TestSize.Level1)
{
    std::vector<std::string> ids;
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_002, TestSize.Level1)
{
    std::vector<std::string> ids = {"1", "2", "3"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_003, TestSize.Level1)
{
    std::vector<std::string> ids = {"999999"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_004, TestSize.Level1)
{
    std::vector<std::string> ids = {"-1", "0", "abc"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_005, TestSize.Level1)
{
    std::vector<std::string> ids;
    for (int i = 0; i < 100; i++) {
        ids.push_back(std::to_string(i));
    }
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_001, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids;
    int32_t targetAlbumId = 1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_002, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1"};
    int32_t targetAlbumId = 0;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_003, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1", "2", "3"};
    int32_t targetAlbumId = 100;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_004, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"999999"};
    int32_t targetAlbumId = 1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_005, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1"};
    int32_t targetAlbumId = -1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveAssetsToLake_Test_001, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids;
    
    int32_t ret = LakeFileOperations::MoveAssetsToLake(refresh, ids);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(LakeFileOperationsTest, MoveAssetsFromLake_Test_001, TestSize.Level1)
{
    std::vector<std::string> ids;
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t ret = LakeFileOperations::MoveAssetsFromLake(refresh, ids);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_001, TestSize.Level1)
{
    std::string fileUri = "";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_002, TestSize.Level1)
{
    std::string fileUri = "/storage/media/local/files/test.jpg";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_003, TestSize.Level1)
{
    std::string fileUri = "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/test.jpg";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_004, TestSize.Level1)
{
    std::string fileUri = "HO_DATA_EXT_MISC/test.jpg";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_005, TestSize.Level1)
{
    std::string fileUri = "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/video.mp4";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_001, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_002, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_003, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_004, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 0;
    std::string displayName = "test.jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_005, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "invalid_path_without_slash";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_006, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name_without_extension";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_007, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.png";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_008, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.mp4";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_009, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 999;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_010, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = -1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_011, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.gif";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_012, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.bmp";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_013, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.webp";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_014, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.heic";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_015, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name(1).jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_016, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name(100).jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_017, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "very_long_file_name_that_exceeds_normal_length_for_testing.jpg";
    std::string storagePath = "/storage/media/local/files/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_018, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/Pictures/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_019, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/Videos/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, RenamePhoto_Test_020, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    int32_t fileId = 1;
    std::string displayName = "new_name.jpg";
    std::string storagePath = "/storage/media/local/files/Docs/test.jpg";
    std::string data = "";
    
    int32_t ret = LakeFileOperations::RenamePhoto(refresh, fileId, displayName, storagePath, data);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_005, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DIR_PATH + "/test.jpg";
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "test content for move";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaFileUtils::IsFileExists(destPath), true);
    
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_006, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DIR_PATH + "/test.mp44";
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "video content";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_007, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DIR_PATH + "/test.png";
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "image content";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

HWTEST_F(LakeFileOperationsTest, MoveLakeFile_Test_008, TestSize.Level1)
{
    std::string srcPath = TEST_SRC_PATH;
    std::string destPath = TEST_DIR_PATH + "/subdir1/subdir2/test.jpg";
    
    MediaFileUtils::CreateFile(srcPath);
    std::string testContent = "test content";
    MediaFileUtils::WriteStrToFile(srcPath, testContent);
    
    int32_t ret = LakeFileOperations::MoveLakeFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_006, TestSize.Level1)
{
    std::vector<std::string> ids = {"1"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_007, TestSize.Level1)
{
    std::vector<std::string> ids = {"0"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_008, TestSize.Level1)
{
    std::vector<std::string> ids = {"-1"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_009, TestSize.Level1)
{
    std::vector<std::string> ids = {"2147483647"};
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, GetInnerLakeAssets_Test_010, TestSize.Level1)
{
    std::vector<std::string> ids;
    for (int i = 1; i <= 10; i++) {
        ids.push_back(std::to_string(i));
    }
    std::vector<MoveAssetsToLakeUpdateData> result = LakeFileOperations::GetInnerLakeAssets(ids);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_006, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1", "2"};
    int32_t targetAlbumId = 1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_007, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1"};
    int32_t targetAlbumId = 2147483647;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewNewAlbum_Test_001, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1", "2", "3", "4", "5"};
    int32_t targetAlbumId = 1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_009, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"abc"};
    int32_t targetAlbumId = 1;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, MoveInnerLakeAssetsToNewAlbum_Test_010, TestSize.Level1)
{
    AccurateRefresh::AssetAccurateRefresh refresh;
    std::vector<std::string> ids = {"1"};
    int32_t targetAlbumId = 999999;
    
    int32_t ret = LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(refresh, ids, targetAlbumId);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_006, TestSize.Level1)
{
    std::string fileUri = "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/test.png";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_007, TestSize.Level1)
{
    std::string fileUri = "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/test.mp4";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_008, TestSize.Level1)
{
    std::string fileUri = "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/test.gif";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_009, TestSize.Level1)
{
    std::string fileUri = "/storage/cloud/files/test.jpg";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(LakeFileOperationsTest, UpdateMediaAssetEditData_Test_010, TestSize.Level1)
{
    std::string fileUri = "/storage/media/local/files/Pictures/test.jpg";
    
    int32_t ret = LakeFileOperations::UpdateMediaAssetEditData(fileUri);
    EXPECT_EQ(ret, E_OK);
}

} // namespace Media
} // namespace OHOS
