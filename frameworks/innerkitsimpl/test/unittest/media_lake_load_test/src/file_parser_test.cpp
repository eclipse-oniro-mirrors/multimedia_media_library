/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "FileParserTest"
#define private public

#include "file_parser_test.h"

#include <fcntl.h>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "lake_file_parser.h"
#include "file_scan_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "media_string_utils.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace NativeRdb;

static const std::string TEST_DIR_PATH = "/data/test/file_parser_test";
static const std::string TEST_IMAGE_PATH = "/data/test/file_parser_test/test_image.jpg";
static const std::string TEST_VIDEO_PATH = "/data/test/file_parser_test/test_video.mp4";
static const std::string TEST_HIDDEN_PATH = "/data/test/file_parser_test/.hidden.jpg";
static const std::string TEST_SMALL_FILE = "/data/test/file_parser_test/small.jpg";
static const std::string TEST_BURST_COVER = "/data/test/file_parser_test/IMG_BURST_COVER.jpg";
static const std::string TEST_BURST_MEMBER = "/data/test/file_parser_test/IMG_BURST001.jpg";
static const std::string TEST_ENHANCED_IMAGE = "/data/test/file/IMG_001_enhanced.jpg";

void FileParserTest::SetUpTestCase()
{
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
}

void FileParserTest::TearDownTestCase()
{
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
}

void FileParserTest::SetUp()
{
}

void FileParserTest::TearDown()
{
}

HWTEST_F(FileParserTest, PhotosRowData_IsExist_Test_001, TestSize.Level1)
{
    FileParser::PhotosRowData rowData;
    rowData.fileId = 0;
    bool result = rowData.IsExist();
    EXPECT_EQ(result, false);
}

HWTEST_F(FileParserTest, PhotosRowData_IsExist_Test_002, TestSize.Level1)
{
    FileParser::PhotosRowData rowData;
    rowData.fileId = -1;
    bool result = rowData.IsExist();
    EXPECT_EQ(result, false);
}

HWTEST_F(FileParserTest, PhotosRowData_IsExist_Test_003, TestSize.Level1)
{
    FileParser::PhotosRowData rowData;
    rowData.fileId = 1;
    bool result = rowData.IsExist();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, PhotosRowData_IsExist_Test_004, TestSize.Level1)
{
    FileParser::PhotosRowData rowData;
    rowData.fileId = 100;
    bool result = rowData.IsExist();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, PhotosRowData_ToString_Test_001, TestSize.Level1)
{
    FileParser::PhotosRowData rowData;
    rowData.fileId = 1;
    rowData.mediaType = 1;
    rowData.fileSourceType = 1;
    rowData.size = 1024;
    rowData.dateModified = 1234567890;
    rowData.dateTaken = 1234567890;
    rowData.inode = "12345";
    rowData.mimeType = "image/jpeg";
    rowData.storagePath = "/storage/media/local/files/test.jpg";
    rowData.ownerAlbumId = 1;
    rowData.ownerPackage = "com.test";
    rowData.packageName = "com.test";
    rowData.data = "/storage/cloud/files/test.jpg";

    std::string result = rowData.ToString();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("fileId: 1"), std::string::npos);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_001, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, false);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_002, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = true;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_003, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = true;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_004, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = true;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_005, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = true;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_006, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = true;
    metaStatus.isInvisible = false;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_IsChanged_Test_007, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = false;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = false;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = false;
    metaStatus.isInvisible = true;

    bool result = metaStatus.IsChanged();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, MetaStatus_ToString_Test_001, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = true;
    metaStatus.isSizeChanged = false;
    metaStatus.isDateModifiedChanged = true;
    metaStatus.isMimeTypeChanged = false;
    metaStatus.isStoragePathChanged = true;
    metaStatus.isInvisible = false;

    std::string result = metaStatus.ToString();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("isMediaTypeChanged: 1"), std::string::npos);
    EXPECT_NE(result.find("isSizeChanged: 0"), std::string::npos);
}

HWTEST_F(FileParserTest, Constructor_Path_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content = "test image content";
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_image.jpg");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_Path_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content = "test video content";
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::FULL);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_VIDEO_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_video.mp4");

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, Constructor_Path_Test_003, TestSize.Level1)
{
    std::string nonExistentPath = "/data/test/nonexistent_file.jpg";
    LakeFileParser parser(nonExistentPath, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_TRUE(fileInfo.filePath.empty());
}

HWTEST_F(FileParserTest, Constructor_NotifyInfo_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content = "test image content";
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_NotifyInfo_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content = "test image content";
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    std::string newPath = "/data/test/file_parser_test/new_image.jpg";
    MediaFileUtils::CreateFile(newPath);
    MediaFileUtils::WriteStrToFile(newPath, content);

    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    notifyInfo.beforePath = TEST_IMAGE_PATH;
    notifyInfo.afterPath = newPath;

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, newPath);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
    MediaFileUtils::DeleteFile(newPath);
}

HWTEST_F(FileParserTest, CheckTypeValid_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content = "test image content";
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckTypeValid_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content = "test video content";
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, CheckTypeValid_Test_003, TestSize.Level1)
{
    std::string testPath = "/data/test/file_parser_test/test.txt";
    MediaFileUtils::CreateFile(testPath);
    std::string content = "test text content";
    MediaFileUtils::WriteStrToFile(testPath, content);

    LakeFileParser parser(testPath, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(testPath);
}

HWTEST_F(FileParserTest, CheckSizeValid_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckSizeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckSizeValid_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_SMALL_FILE);
    std::string content(512, 'a');
    MediaFileUtils::WriteStrToFile(TEST_SMALL_FILE, content);

    LakeFileParser parser(TEST_SMALL_FILE, ScanMode::INCREMENT);
    bool result = parser.CheckSizeValid();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_SMALL_FILE);
}

HWTEST_F(FileParserTest, CheckSizeValid_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(1025, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckSizeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckIsNotHidden_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content = "test image content";
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckIsNotHidden();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckIsNotHidden_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_HIDDEN_PATH);
    std::string content = "test hidden content";
    MediaFileUtils::WriteStrToFile(TEST_HIDDEN_PATH, content);

    LakeFileParser parser(TEST_HIDDEN_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckIsNotHidden();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_HIDDEN_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_HIDDEN_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_HIDDEN_PATH, content);

    LakeFileParser parser(TEST_HIDDEN_PATH, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_HIDDEN_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_SMALL_FILE);
    std::string content(512, 'a');
    MediaFileUtils::WriteStrToFile(TEST_SMALL_FILE, content);

    LakeFileParser parser(TEST_SMALL_FILE, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_SMALL_FILE);
}

HWTEST_F(FileParserTest, GetUniqueId_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    int32_t uniqueId = parser.GetUniqueId();
    EXPECT_GE(uniqueId, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetUniqueId_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    int32_t uniqueId = parser.GetUniqueId();
    EXPECT_GE(uniqueId, 0);

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, PrintInfo_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    std::string result = parser.PrintInfo(fileInfo);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("InnerFileInfo"), std::string::npos);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, IsNotifyInfoValid_Test_001, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    bool result = parser.IsNotifyInfoValid();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, IsNotifyInfoValid_Test_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    bool result = parser.IsNotifyInfoValid();
    EXPECT_EQ(result, false);
}

HWTEST_F(FileParserTest, IsNotifyInfoValid_Test_003, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    bool result = parser.IsNotifyInfoValid();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, IsNotifyInfoValid_Test_004, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    bool result = parser.IsNotifyInfoValid();
    EXPECT_EQ(result, false);
}

HWTEST_F(FileParserTest, IsNotifyInfoValid_Test_005, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = static_cast<FileNotifyOperationType>(99);
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    bool result = parser.IsNotifyInfoValid();
    EXPECT_EQ(result, true);
}

HWTEST_F(FileParserTest, GetFileUpdateType_Test_001, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = static_cast<FileNotifyOperationType>(99);
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    FileUpdateType updateType = parser.GetFileUpdateType();
    EXPECT_EQ(updateType, FileUpdateType::INSERT);
}

HWTEST_F(FileParserTest, GetFileUpdateType_Test_002, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    FileUpdateType updateType = parser.GetFileUpdateType();
    EXPECT_EQ(updateType, FileUpdateType::NO_CHANGE);
}

HWTEST_F(FileParserTest, GetFileUpdateType_Test_003, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    FileUpdateType updateType = parser.GetFileUpdateType();
    EXPECT_EQ(updateType, FileUpdateType::NO_CHANGE);
}

HWTEST_F(FileParserTest, GetFileInfo_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_image.jpg");
    EXPECT_GT(fileInfo.fileSize, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, ToString_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    std::string result = parser.ToString();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("InnerFileInfo"), std::string::npos);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetFileId_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(100);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 100);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetFileId_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(0);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetFileId_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(-1);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, -1);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetAlbumInfo_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetAlbumInfo(1, "com.test.bundle", "TestAlbum");
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.ownerAlbumId, 1);
    EXPECT_EQ(fileInfo.bundleName, "com.test.bundle");
    EXPECT_EQ(fileInfo.packageName, "TestAlbum");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetAlbumInfo_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetAlbumInfo(0, "", "");
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.ownerAlbumId, 0);
    EXPECT_EQ(fileInfo.bundleName, "");
    EXPECT_EQ(fileInfo.packageName, "");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetAlbumInfo_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetAlbumInfo(999, "com.example.test", "ExampleAlbum");
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.ownerAlbumId, 999);
    EXPECT_EQ(fileInfo.bundleName, "com.example.test");
    EXPECT_EQ(fileInfo.packageName, "ExampleAlbum");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetByPhotosRowData_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    FileParser::PhotosRowData rowData;
    rowData.fileId = 100;
    rowData.ownerAlbumId = 1;
    rowData.ownerPackage = "com.test";
    rowData.packageName = "TestAlbum";
    rowData.data = "/storage/cloud/files/test.jpg";
    rowData.editTime = 1234567890;
    rowData.dateTaken = 9876543210;

    parser.SetByPhotosRowData(rowData);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 100);
    EXPECT_EQ(fileInfo.ownerAlbumId, 1);
    EXPECT_EQ(fileInfo.bundleName, "com.test");
    EXPECT_EQ(fileInfo.packageName, "TestAlbum");
    EXPECT_EQ(fileInfo.cloudPath, "/storage/cloud/files/test.jpg");
    EXPECT_EQ(fileInfo.editTime, 1234567890);
    EXPECT_EQ(fileInfo.dateTaken, 9876543210);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetByPhotosRowData_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    FileParser::PhotosRowData rowData;
    rowData.fileId = 0;
    rowData.ownerAlbumId = 0;
    rowData.ownerPackage = "";
    rowData.packageName = "";
    rowData.data = "";
    rowData.editTime = 0;
    rowData.dateTaken = 0;

    parser.SetByPhotosRowData(rowData);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 0);
    EXPECT_EQ(fileInfo.ownerAlbumId, 0);
    EXPECT_EQ(fileInfo.bundleName, "");
    EXPECT_EQ(fileInfo.packageName, "");
    EXPECT_EQ(fileInfo.cloudPath, "");
    EXPECT_EQ(fileInfo.editTime, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(1, "com.test", "TestAlbum");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(0, "", "");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(1, "com.test", "TestAlbum");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, GetAssetInsertValues_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetCloudPath();
    NativeRdb::ValuesBucket values = parser.GetAssetInsertValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetAssetUpdateValues_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.GetAssetUpdateValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetAssetCommonValues_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.GetAssetCommonValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileAssetUri_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(100);
    parser.SetCloudPath();
    std::string uri = parser.GetFileAssetUri();
    EXPECT_FALSE(uri.empty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GenerateThumbnail_Test_001, TestSize.Level1)
{
    std::vector<std::string> inodes;
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::FULL, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Test_002, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890"};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::FULL, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Test_003, TestSize.Level1)
{
    std::vector<std::string> inodes;
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::INCREMENT, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Test_004, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345"};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::INCREMENT, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Test_005, TestSize.Level1)
{
    std::vector<std::string> inodes;
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::VALIDATION, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Test_001, TestSize.Level1)
{
    std::vector<std::string> inodes;
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Test_002, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Test_003, TestSize.Level1)
{
    std::vector<std::string> inodes = {"0"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Test_004, TestSize.Level1)
{
    std::vector<std::string> inodes = {"-1"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Test_005, TestSize.Level1)
{
    std::vector<std::string> inodes = {"999999"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_Test_001, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = 1;
    info.displayName = "test.jpg";
    info.path = "/storage/cloud/files/test.jpg";
    info.dateTaken = 1234567890;
    info.dateModified = 1234567890;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_Test_002, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = 0;
    info.displayName = "";
    info.path = "";
    info.dateTaken = 0;
    info.dateModified = 0;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_Test_003, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = -1;
    info.displayName = "test.mp4";
    info.path = "/storage/cloud/files/test.mp4";
    info.dateTaken = 0;
    info.dateModified = 0;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, Constructor_Path_Full_Scan_Mode, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::FULL);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_Path_Validation_Scan_Mode, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::VALIDATION);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_NotifyInfo_Full_Scan_Mode, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::FULL);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_NotifyInfo_Validation_Scan_Mode, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::MOD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::VALIDATION);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckBurst_Test_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_BURST_COVER);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_BURST_COVER, content);

    LakeFileParser parser(TEST_BURST_COVER, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.isBurst, IsBurstType::BURST_COVER_TYPE);

    MediaFileUtils::DeleteFile(TEST_BURST_COVER);
}

HWTEST_F(FileParserTest, CheckBurst_Test_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_BURST_MEMBER);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_BURST_MEMBER, content);

    LakeFileParser parser(TEST_BURST_MEMBER, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.isBurst, IsBurstType::BURST_MEMBER_TYPE);

    MediaFileUtils::DeleteFile(TEST_BURST_MEMBER);
}

HWTEST_F(FileParserTest, CheckBurst_Test_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.isBurst, IsBurstType::OTHER_TYPE);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileInfo_Video_File, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.filePath, TEST_VIDEO_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_video.mp4");
    EXPECT_GT(fileInfo.fileSize, 0);

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, GetUniqueId_Increment_Check, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser1(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    int32_t id1 = parser1.GetUniqueId();

    LakeFileParser parser2(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    int32_t id2 = parser2.GetUniqueId();

    EXPECT_GE(id2, id1);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileUpdateType_Invalid_NotifyInfo, TestSize.Level1)
{
    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = "";

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    FileUpdateType updateType = parser.GetFileUpdateType();
    EXPECT_EQ(updateType, FileUpdateType::NO_CHANGE);
}

HWTEST_F(FileParserTest, SetByPhotosRowData_With_Zero_DateTaken, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    FileParser::PhotosRowData rowData;
    rowData.fileId = 100;
    rowData.ownerAlbumId = 1;
    rowData.ownerPackage = "com.test";
    rowData.packageName = "TestAlbum";
    rowData.data = "/storage/cloud/files/test.jpg";
    rowData.editTime = 1234567890;
    rowData.dateTaken = 0;

    parser.SetByPhotosRowData(rowData);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 100);
    EXPECT_EQ(fileInfo.editTime, 1234567890);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetByPhotosRowData_With_Negative_DateTaken, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    FileParser::PhotosRowData rowData;
    rowData.fileId = 100;
    rowData.ownerAlbumId = 1;
    rowData.ownerPackage = "com.test";
    rowData.packageName = "TestAlbum";
    rowData.data = "/storage/cloud/files/test.jpg";
    rowData.editTime = 1234567890;
    rowData.dateTaken = -1;

    parser.SetByPhotosRowData(rowData);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, 100);
    EXPECT_EQ(fileInfo.editTime, 1234567890);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_With_Large_AlbumId, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(2147483647, "com.test", "TestAlbum");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_With_Negative_AlbumId, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(-1, "com.test", "TestAlbum");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileUris_With_Large_Inodes, TestSize.Level1)
{
    std::vector<std::string> inodes;
    for (int i = 0; i < 100; i++) {
        inodes.push_back(std::to_string(i));
    }
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_With_Empty_String_Inodes, TestSize.Level1)
{
    std::vector<std::string> inodes = {""};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_With_Mixed_Inodes, TestSize.Level1)
{
    std::vector<std::string> inodes = {"0", "-1", "999999", "abc", "123"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_With_Large_Inodes, TestSize.Level1)
{
    std::vector<std::string> inodes;
    for (int i = 0; i < 50; i++) {
        inodes.push_back(std::to_string(i));
    }
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::INCREMENT, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_With_Empty_String_Inodes, TestSize.Level1)
{
    std::vector<std::string> inodes = {""};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::INCREMENT, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_With_Large_FileId, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = 2147483647;
    info.displayName = "test.jpg";
    info.path = "/storage/cloud/files/test.jpg";
    info.dateTaken = 1234567890;
    info.dateModified = 1234567890;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_With_Negative_FileId, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = -1;
    info.displayName = "test.jpg";
    info.path = "/storage/cloud/files/test.jpg";
    info.dateTaken = 1234567890;
    info.dateModified = 1234567890;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_With_Empty_Paths, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = 1;
    info.displayName = "";
    info.path = "";
    info.dateTaken = 0;
    info.dateModified = 0;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, GetFileAssetUri_With_Large_FileId, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(2147483647);
    parser.SetCloudPath();
    std::string uri = parser.GetFileAssetUri();
    EXPECT_FALSE(uri.empty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileAssetUri_With_Negative_FileId, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(-1);
    parser.SetCloudPath();
    std::string uri = parser.GetFileAssetUri();
    EXPECT_FALSE(uri.empty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetCheckSizeValid_Boundary_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(1024, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckSizeValid();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetCheckSizeValid_Just_Above_Boundary_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(1025, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckSizeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_All_Conditions_True_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Hidden_File_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_HIDDEN_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_HIDDEN_PATH, content);

    LakeFileParser parser(TEST_HIDDEN_PATH, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_HIDDEN_PATH);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Small_File_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_SMALL_FILE);
    std::string content(512, 'a');
    MediaFileUtils::WriteStrToFile(TEST_SMALL_FILE, content);

    LakeFileParser parser(TEST_SMALL_FILE, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_SMALL_FILE);
}

HWTEST_F(FileParserTest, IsFileValidAsset_Unsupported_Type_Test, TestSize.Level1)
{
    std::string testPath = "/data/test/file_parser_test/test.txt";
    MediaFileUtils::CreateFile(testPath);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(testPath, content);

    LakeFileParser parser(testPath, ScanMode::INCREMENT);
    bool result = parser.IsFileValidAsset();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(testPath);
}

HWTEST_F(FileParserTest, CheckIsNotHidden_Dot_Prefix_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_HIDDEN_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_HIDDEN_PATH, content);

    LakeFileParser parser(TEST_HIDDEN_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckIsNotHidden();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(TEST_HIDDEN_PATH);
}

HWTEST_F(FileParserTest, CheckIsNotHidden_Normal_File_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckIsNotHidden();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckTypeValid_Image_Type_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, CheckTypeValid_Video_Type_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, true);

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

HWTEST_F(FileParserTest, CheckTypeValid_Unsupported_Type_Test, TestSize.Level1)
{
    std::string testPath = "/data/test/file_parser_test/test.txt";
    MediaFileUtils::CreateFile(testPath);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(testPath, content);

    LakeFileParser parser(testPath, ScanMode::INCREMENT);
    bool result = parser.CheckTypeValid();
    EXPECT_EQ(result, false);

    MediaFileUtils::DeleteFile(testPath);
}

HWTEST_F(FileParserTest, ToString_Verify_Output_Format_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    std::string result = parser.ToString();

    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("InnerFileInfo"), std::string::npos);
    EXPECT_NE(result.find("fileId:"), std::string::npos);
    EXPECT_NE(result.find("storagePath:"), std::string::npos);
    EXPECT_NE(result.find("displayName:"), std::string::npos);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, MetaStatus_ToString_Verify_All_Fields_Test, TestSize.Level1)
{
    FileParser::MetaStatus metaStatus;
    metaStatus.isMediaTypeChanged = true;
    metaStatus.isSizeChanged = true;
    metaStatus.isDateModifiedChanged = true;
    metaStatus.isMimeTypeChanged = true;
    metaStatus.isStoragePathChanged = true;
    metaStatus.isInvisible = true;

    std::string result = metaStatus.ToString();

    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("MetaStatus"), std::string::npos);
    EXPECT_NE(result.find("isMediaTypeChanged:"), std::string::npos);
    EXPECT_NE(result.find("isSizeChanged:"), std::string::npos);
    EXPECT_NE(result.find("isDateModifiedChanged:"), std::string::npos);
    EXPECT_NE(result.find("isMimeTypeChanged:"), std::string::npos);
    EXPECT_NE(result.find("isStoragePathChanged:"), std::string::npos);
    EXPECT_NE(result.find("isInvisible:"), std::string::npos);
}

HWTEST_F(FileParserTest, SetFileId_Verify_Assignment_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);

    int32_t testId = 12345;
    parser.SetFileId(testId);
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, testId);

    testId = 0;
    parser.SetFileId(testId);
    fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, testId);

    testId = -1;
    parser.SetFileId(testId);
    fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.fileId, testId);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetAlbumInfo_Verify_All_Fields_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetAlbumInfo(1, "com.test.bundle", "TestAlbum");
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.ownerAlbumId, 1);
    EXPECT_EQ(fileInfo.bundleName, "com.test.bundle");
    EXPECT_EQ(fileInfo.packageName, "TestAlbum");

    parser.SetAlbumInfo(0, "", "");
    fileInfo = parser.GetFileInfo();
    EXPECT_EQ(fileInfo.ownerAlbumId, 0);
    EXPECT_EQ(fileInfo.bundleName, "");
    EXPECT_EQ(fileInfo.packageName, "");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetCloudPath_Verify_Path_Generation_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetCloudPath();
    InnerFileInfo fileInfo = parser.GetFileInfo();
    EXPECT_FALSE(fileInfo.cloudPath.empty());

    parser.SetCloudPath();
    fileInfo = parser.GetFileInfo();
    EXPECT_FALSE(fileInfo.cloudPath.empty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, SetByPhotosRowData_Verify_All_Fields_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    FileParser::PhotosRowData rowData;
    rowData.fileId = 100;
    rowData.ownerAlbumId = 1;
    rowData.ownerPackage = "com.test";
    rowData.packageName = "TestAlbum";
    rowData.data = "/storage/cloud/files/test.jpg";
    rowData.editTime = 1234567890;
    rowData.dateTaken = 9876543210;

    parser.SetByPhotosRowData(rowData);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.fileId, 100);
    EXPECT_EQ(fileInfo.ownerAlbumId, 1);
    EXPECT_EQ(fileInfo.bundleName, "com.test");
    EXPECT_EQ(fileInfo.packageName, "TestAlbum");
    EXPECT_EQ(fileInfo.cloudPath, "/storage/cloud/files/test.jpg");
    EXPECT_EQ(fileInfo.editTime, 1234567890);
    EXPECT_EQ(fileInfo.dateTaken, 9876543210);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetAssetInsertValues_Verify_Bucket_Content_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetCloudPath();
    NativeRdb::ValuesBucket values = parser.GetAssetInsertValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetAssetUpdateValues_Verify_Bucket_Content_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.GetAssetUpdateValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetAssetCommonValues_Verify_Bucket_Content_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.GetAssetCommonValues();
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, TransFileInfoToBucket_Verify_Bucket_Content_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    NativeRdb::ValuesBucket values = parser.TransFileInfoToBucket(1, "com.test", "TestAlbum");
    EXPECT_FALSE(values.IsEmpty());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileAssetUri_Verify_Uri_Format_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    parser.SetFileId(100);
    parser.SetCloudPath();
    std::string uri = parser.GetFileAssetUri();
    EXPECT_FALSE(uri.empty());
    EXPECT_NE(uri.find("file://"), std::string::npos);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GenerateThumbnail_Verify_Full_Scan_Mode_Test, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890"};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::FULL, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Verify_Increment_Scan_Mode_Test, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890"};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::INCREMENT, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateThumbnail_Verify_Validation_Scan_Mode_Test, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890"};
    std::vector<std::string> result = FileParser::GenerateThumbnail(ScanMode::VALIDATION, inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Verify_Empty_Input_Test, TestSize.Level1)
{
    std::vector<std::string> inodes;
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Verify_Single_Inode_Test, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GetFileUris_Verify_Multiple_Inodes_Test, TestSize.Level1)
{
    std::vector<std::string> inodes = {"12345", "67890", "11111"};
    std::vector<std::string> result = FileParser::GetFileUris(inodes);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(FileParserTest, GenerateSingleThumbnail_Verify_Call_Test, TestSize.Level1)
{
    ThumbnailInfo info;
    info.fileId = 1;
    info.displayName = "test.jpg";
    info.path = "/storage/cloud/files/test.jpg";
    info.dateTaken = 1234567890;
    info.dateModified = 1234567890;

    int32_t result = FileParser::GenerateSingleThumbnail(info);
    EXPECT_GE(result, 0);
}

HWTEST_F(FileParserTest, Constructor_Path_Verify_File_Info_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_image.jpg");
    EXPECT_GT(fileInfo.fileSize, 0);
    EXPECT_GE(fileInfo.dateModified, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, Constructor_NotifyInfo_Verify_File_Info_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    MediaNotifyInfo notifyInfo;
    notifyInfo.objType = FileNotifyObjectType::FILE;
    notifyInfo.optType = FileNotifyOperationType::ADD;
    notifyInfo.beforePath = "";
    notifyInfo.afterPath = TEST_IMAGE_PATH;

    LakeFileParser parser(notifyInfo, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_image.jpg");

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileInfo_Verify_Image_File_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.filePath, TEST_IMAGE_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_image.jpg");
    EXPECT_GT(fileInfo.fileSize, 0);

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

HWTEST_F(FileParserTest, GetFileInfo_Verify_Video_File_Test, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    InnerFileInfo fileInfo = parser.GetFileInfo();

    EXPECT_EQ(fileInfo.filePath, TEST_VIDEO_PATH);
    EXPECT_EQ(fileInfo.displayName, "test_video.mp4");
    EXPECT_GT(fileInfo.fileSize, 0);

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

// 电影模式V2(CINEMATIC_VIDEO_V2)资产识别: 依赖数据库可查询
class FileParserCinematicV2Test : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    int32_t insertedFileId_ = -1;
};

static std::shared_ptr<MediaLibraryRdbStore> g_cinematicV2RdbStore = nullptr;

void FileParserCinematicV2Test::SetUpTestCase()
{
    MediaFileUtils::CreateDirectory(TEST_DIR_PATH);
    MediaLibraryUnitTestUtils::Init();
    g_cinematicV2RdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_cinematicV2RdbStore, nullptr);
}

void FileParserCinematicV2Test::TearDownTestCase()
{
    MediaFileUtils::DeleteDir(TEST_DIR_PATH);
}

void FileParserCinematicV2Test::SetUp()
{
    insertedFileId_ = -1;
}

void FileParserCinematicV2Test::TearDown()
{
    // 清理用例插入的Photos记录, 避免残留影响同进程其他用例
    if (g_cinematicV2RdbStore != nullptr && insertedFileId_ > 0) {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, insertedFileId_);
        int32_t deletedRows = -1;
        g_cinematicV2RdbStore->Delete(deletedRows, predicates);
    }
    insertedFileId_ = -1;
}

static int32_t InsertCinematicVideoRow(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, PhotoSubType subtype,
    const std::string &displayName)
{
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/1/" + displayName);
    values.Put(MediaColumn::MEDIA_NAME, displayName);
    values.Put(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(subtype));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != E_OK) {
        return -1;
    }
    return static_cast<int32_t>(rowId);
}

/**
 * @tc.name: IsCinematicVideoV2Asset_VideoV2_001
 * @tc.desc: 视频且DB subtype=CINEMATIC_VIDEO_V2时返回true
 * @tc.type: FUNC
 */
HWTEST_F(FileParserCinematicV2Test, IsCinematicVideoV2Asset_VideoV2_001, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    insertedFileId_ = InsertCinematicVideoRow(g_cinematicV2RdbStore, PhotoSubType::CINEMATIC_VIDEO_V2,
        "cinematic_v2_parser.mp4");
    ASSERT_GT(insertedFileId_, 0);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    parser.SetFileId(insertedFileId_);
    EXPECT_TRUE(parser.IsCinematicVideoV2Asset());

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

/**
 * @tc.name: IsCinematicVideoV2Asset_VideoV1_002
 * @tc.desc: 视频但DB subtype=CINEMATIC_VIDEO(V1)时返回false
 * @tc.type: FUNC
 */
HWTEST_F(FileParserCinematicV2Test, IsCinematicVideoV2Asset_VideoV1_002, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    insertedFileId_ = InsertCinematicVideoRow(g_cinematicV2RdbStore, PhotoSubType::CINEMATIC_VIDEO,
        "cinematic_v1_parser.mp4");
    ASSERT_GT(insertedFileId_, 0);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    parser.SetFileId(insertedFileId_);
    EXPECT_FALSE(parser.IsCinematicVideoV2Asset());

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}

/**
 * @tc.name: IsCinematicVideoV2Asset_NonVideo_003
 * @tc.desc: 非视频文件(fileType!=VIDEO)时直接返回false
 * @tc.type: FUNC
 */
HWTEST_F(FileParserCinematicV2Test, IsCinematicVideoV2Asset_NonVideo_003, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_IMAGE_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_IMAGE_PATH, content);

    LakeFileParser parser(TEST_IMAGE_PATH, ScanMode::INCREMENT);
    EXPECT_FALSE(parser.IsCinematicVideoV2Asset());

    MediaFileUtils::DeleteFile(TEST_IMAGE_PATH);
}

/**
 * @tc.name: IsCinematicVideoV2Asset_NoRow_004
 * @tc.desc: 视频但DB无对应记录时返回false
 * @tc.type: FUNC
 */
HWTEST_F(FileParserCinematicV2Test, IsCinematicVideoV2Asset_NoRow_004, TestSize.Level1)
{
    MediaFileUtils::CreateFile(TEST_VIDEO_PATH);
    std::string content(2048, 'a');
    MediaFileUtils::WriteStrToFile(TEST_VIDEO_PATH, content);

    LakeFileParser parser(TEST_VIDEO_PATH, ScanMode::INCREMENT);
    parser.SetFileId(-1); // 不存在的fileId, GoToNextRow失败
    EXPECT_FALSE(parser.IsCinematicVideoV2Asset());

    MediaFileUtils::DeleteFile(TEST_VIDEO_PATH);
}
} // namespace Media
} // namespace OHOS
