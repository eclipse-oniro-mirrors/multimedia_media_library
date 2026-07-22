/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#include "medialibrary_helper_test.h"

#include <fcntl.h>
#include <fstream>
#include <iterator>

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_type_const.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"
#include "moving_photo_file_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const int32_t ASPECT_RATIO_UNSUPPORT = -1;
const int32_t ASPECT_RATIO_MAX = 1000;
const double ASPECT_RATIO_MIN = 0.001;
const double ASPECT_RATIO_MAX_TEST = 10000;

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileExists_Test_001, TestSize.Level1)
{
    string filePath = "/data/test/isfileexists_001";
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirEmpty_Test_001, TestSize.Level1)
{
    string dirPath = "/data/test/isdirempty_001";
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirEmpty_Test_002, TestSize.Level1)
{
    string dirPath = "/data/test/isdirempty_002";
    string subPath = dirPath + "/isdirempty_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(subPath), true);
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirEmpty_Test_003, TestSize.Level1)
{
    string dirPath = "/data/test/isdirempty_003";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateFile_Test_001, TestSize.Level1)
{
    string filePath = "/data/test/createfile_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateFile_Test_002, TestSize.Level1)
{
    string filePath = "";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateFile_Test_003, TestSize.Level1)
{
    string filePath = "/data/test/createfile_003";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateFile_Test_004, TestSize.Level1)
{
    string filePath = "/data/test/test/test/test/createfile_004";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteFile_Test_001, TestSize.Level1)
{
    string filePath = "/data/test/deletefile_001";
    EXPECT_EQ(MediaFileUtils::DeleteFile(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteDir_Test_001, TestSize.Level1)
{
    string dirPath = "/data/test/deletedir_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteDir_Test_002, TestSize.Level1)
{
    string dirPath = "/data/test/deletedir_002";
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFile_Test_001, TestSize.Level1)
{
    string oldPath = "/data/test/movefile_001";
    string newPath = "/data/test/movefile_001_move";
    EXPECT_EQ(MediaFileUtils::CreateFile(oldPath), true);
    EXPECT_EQ(MediaFileUtils::MoveFile(oldPath, newPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFile_Test_002, TestSize.Level1)
{
    string oldPath = "/data/test/movefile_002";
    string newPath = "/data/test/movefile_002_move";
    EXPECT_EQ(MediaFileUtils::CreateFile(oldPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(newPath), true);
    EXPECT_EQ(MediaFileUtils::MoveFile(oldPath, newPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFile_Test_003, TestSize.Level1)
{
    string oldPath = "/data/test/movefile_003";
    string newPath = "/data/test/movefile_003_move";
    EXPECT_EQ(MediaFileUtils::MoveFile(oldPath, newPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafe_Test_001, TestSize.Level1)
{
    string oldPath = "/data/test/cfs_001_s.mp4";
    string newPath = "/data/test/cfs_001_t.mp4";
    EXPECT_EQ(MediaFileUtils::CreateFile(oldPath), true);
    EXPECT_EQ(MediaFileUtils::CopyFileSafe(oldPath, newPath), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(oldPath), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(newPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateDirectory_Test_001, TestSize.Level1)
{
    string dirPath = "/data/test/createdir_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
}

static bool CheckFileString(const string &filePath, const string &text)
{
    ifstream inputFile(filePath);
    if (inputFile.is_open()) {
        string context((istreambuf_iterator<char>(inputFile)), (istreambuf_iterator<char>()));
        inputFile.close();
        if (context == text) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_WriteStrToFile_Test_001, TestSize.Level1)
{
    string testString = "123456";
    string testPath = "/data/test/WriteStrToFileTest_001";
    EXPECT_EQ(MediaFileUtils::WriteStrToFile("", testString), false);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(testPath, ""), false);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(testPath, testString), false);
    EXPECT_EQ(MediaFileUtils::CreateFile(testPath), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(testPath, ""), false);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(testPath, testString), true);
    EXPECT_EQ(CheckFileString(testPath, testString), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyByFd_Test_001, TestSize.Level1)
{
    string oldPath = "/data/test/CopyByFd_001";
    string newPath = "/data/test/CopyByFd_002";
    string testString = "123456";

    EXPECT_EQ(MediaFileUtils::CreateFile(oldPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(newPath), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(oldPath, testString), true);

    int32_t rfd = open(oldPath.c_str(), O_RDONLY);
    int32_t wfd = open(newPath.c_str(), O_RDWR);
    EXPECT_EQ(MediaFileUtils::CopyFile(rfd, wfd), true);
    close(rfd);
    close(wfd);
    EXPECT_EQ(CheckFileString(newPath, testString), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_RenameDir_Test_001, TestSize.Level1)
{
    string oldPath = "/data/test/renamedir_001";
    string newPath = "/data/test/renamedir_001_renamed";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(oldPath), true);
    EXPECT_EQ(MediaFileUtils::RenameDir(oldPath, newPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_RenameDir_Test_002, TestSize.Level1)
{
    string oldPath = "/data/test/renamedir_002";
    string newPath = "";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(oldPath), true);
    EXPECT_EQ(MediaFileUtils::RenameDir(oldPath, newPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFileTitle_test_001, TestSize.Level1)
{
    string displayName = "";
    string ret = MediaFileUtils::GetTitleFromDisplayName(displayName);
    EXPECT_EQ(ret, "");
    displayName = "medialib.test";
    ret = MediaFileUtils::GetTitleFromDisplayName(displayName);
    EXPECT_NE(ret, "");
    displayName = "medialib.";
    ret = MediaFileUtils::GetTitleFromDisplayName(displayName);
    EXPECT_NE(ret, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_001, TestSize.Level1)
{
    string displayName = "";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_002, TestSize.Level1)
{
    string displayName = ".nofile";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_003, TestSize.Level1)
{
    string displayName = "test";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_004, TestSize.Level1)
{
    string displayName = "test:*\'";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_005, TestSize.Level1)
{
    string displayName = "test.test.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_006, TestSize.Level1)
{
    string displayName = "test.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_007, TestSize.Level1)
{
    string displayName = ".hidden.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, false, true), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_008, TestSize.Level1)
{
    string displayName = "test..jpg";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName, false, true), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_009, TestSize.Level1)
{
    string displayName = "test.test.jpg";
    EXPECT_LT(MediaFileUtils::CheckDisplayName(displayName, true, false), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDisplayName_Test_010, TestSize.Level1)
{
    string displayName = "test_test.jpg";
    EXPECT_EQ(MediaFileUtils::CheckDisplayName(displayName, true, false), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckFileDisplayName_Test_001, TestSize.Level1)
{
    string displayName = "test.test.jpg";
    EXPECT_EQ(MediaFileUtils::CheckFileDisplayName(displayName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_001, TestSize.Level1)
{
    EXPECT_LT(MediaFileUtils::CheckAlbumName(".", true), 0);
    EXPECT_LT(MediaFileUtils::CheckAlbumName("..", true), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_002, TestSize.Level1)
{
    EXPECT_LT(MediaFileUtils::CheckAlbumName("..jpg", true), 0);
    EXPECT_LT(MediaFileUtils::CheckAlbumName("...jpg", true), 0);
    EXPECT_LT(MediaFileUtils::CheckAlbumName("good..jpg", true), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_003, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CheckAlbumName("good.good.jpg", true), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAlbumDateModified_Test_001, TestSize.Level1)
{
    string dirPath = "/data/test/getalbumdatemodified_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::GetAlbumDateModified(dirPath) > 0, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAlbumDateModified_Test_002, TestSize.Level1)
{
    string dirPath = "";
    EXPECT_EQ(MediaFileUtils::GetAlbumDateModified(dirPath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAlbumDateModified_Test_003, TestSize.Level1)
{
    string dirPath = "/data/test/getalbumdatemodified_003";
    EXPECT_EQ(MediaFileUtils::GetAlbumDateModified(dirPath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UTCTimeSeconds_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::UTCTimeSeconds() > 0, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkIdFromUri_Test_001, TestSize.Level1)
{
    string tempNetworkId = "1d3cb099659d53b3ee15faaab3c00a8ff983382ebc8b01aabde039ed084e167b";
    string uri = CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX + tempNetworkId + CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::GetNetworkIdFromUri(uri), tempNetworkId);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkIdFromUri_Test_002, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(MediaFileUtils::GetNetworkIdFromUri(uri), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkIdFromUri_Test_003, TestSize.Level1)
{
    string uri = CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::GetNetworkIdFromUri(uri), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkIdFromUri_Test_004, TestSize.Level1)
{
    string uri = CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX;
    EXPECT_EQ(MediaFileUtils::GetNetworkIdFromUri(uri), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkIdFromUri_Test_005, TestSize.Level1)
{
    string uri = string(CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX) + CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::GetNetworkIdFromUri(uri), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdatePath_Test_002, TestSize.Level1)
{
    string path = "";
    string uri = "";
    EXPECT_EQ(MediaFileUtils::UpdatePath(path, uri), path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdatePath_Test_003, TestSize.Level1)
{
    string path = "/storage/cloud/100/files";
    string uri = string(CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX) + CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::UpdatePath(path, uri), path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdatePath_Test_005, TestSize.Level1)
{
    string path = "local/files";
    string tempNetworkId = "1d3cb099659d53b3ee15faaab3c00a8ff983382ebc8b01aabde039ed084e167b";
    string uri = CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX + tempNetworkId + CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::UpdatePath(path, uri), path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdatePath_Test_006, TestSize.Level1)
{
    string path = "/storage/cloud/100";
    string tempNetworkId = "1d3cb099659d53b3ee15faaab3c00a8ff983382ebc8b01aabde039ed084e167b";
    string uri = CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX + tempNetworkId + CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER;
    EXPECT_EQ(MediaFileUtils::UpdatePath(path, uri), path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdatePath_Test_007, TestSize.Level1)
{
    string path = "/storage/cloud/100";
    string uri = "";
    EXPECT_EQ(MediaFileUtils::UpdatePath(path, uri), path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_001, TestSize.Level1)
{
    string displayName = "";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_ALL);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_002, TestSize.Level1)
{
    string displayName = "test";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_FILE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_003, TestSize.Level1)
{
    string displayName = "test.mp3";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_AUDIO);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_004, TestSize.Level1)
{
    string displayName = "test.mp4";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_VIDEO);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_005, TestSize.Level1)
{
    string displayName = "test.jpg";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_IMAGE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaType_Test_006, TestSize.Level1)
{
    string displayName = "test.txt";
    EXPECT_EQ(MediaFileUtils::GetMediaType(displayName), MEDIA_TYPE_FILE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFilename_Test_001, TestSize.Level1)
{
    string filePath = "";
    EXPECT_EQ(MediaFileUtils::GetFileName(filePath), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFilename_Test_002, TestSize.Level1)
{
    string filePath = "test";
    EXPECT_EQ(MediaFileUtils::GetFileName(filePath), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFilename_Test_003, TestSize.Level1)
{
    string filePath = "test/";
    EXPECT_EQ(MediaFileUtils::GetFileName(filePath), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFilename_Test_004, TestSize.Level1)
{
    string filePath = "test/test";
    EXPECT_EQ(MediaFileUtils::GetFileName(filePath), "test");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileTablePath_Test_001, TestSize.Level1)
{
    string filePath = "";
    EXPECT_EQ(MediaFileUtils::IsFileTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileTablePath_Test_002, TestSize.Level1)
{
    string filePath = "/1234546541315645464545165";
    EXPECT_EQ(MediaFileUtils::IsFileTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileTablePath_Test_003, TestSize.Level1)
{
    string filePath = ROOT_MEDIA_DIR;
    EXPECT_EQ(MediaFileUtils::IsFileTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileTablePath_Test_004, TestSize.Level1)
{
    string filePath = ROOT_MEDIA_DIR + DOCS_PATH;
    EXPECT_EQ(MediaFileUtils::IsFileTablePath(filePath), true);
    filePath = ROOT_MEDIA_DIR + AUDIO_DIR_VALUES;
    EXPECT_EQ(MediaFileUtils::IsFileTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsPhotoTablePath_Test_001, TestSize.Level1)
{
    string filePath = "";
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsPhotoTablePath_Test_002, TestSize.Level1)
{
    string filePath = "/1234546541315645464545165";
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsPhotoTablePath_Test_003, TestSize.Level1)
{
    string filePath = ROOT_MEDIA_DIR;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsPhotoTablePath_Test_004, TestSize.Level1)
{
    string filePath = ROOT_MEDIA_DIR + PHOTO_BUCKET;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), true);
    filePath = ROOT_MEDIA_DIR + PIC_DIR_VALUES;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), true);
    filePath = ROOT_MEDIA_DIR + VIDEO_DIR_VALUES;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), true);
    filePath = ROOT_MEDIA_DIR + CAMERA_DIR_VALUES;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsPhotoTablePath_Test_005, TestSize.Level1)
{
    string filePath = ROOT_MEDIA_DIR + AUDIO_DIR_VALUES;
    EXPECT_EQ(MediaFileUtils::IsPhotoTablePath(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_001, TestSize.Level1)
{
    const string virtualUri = "file://media/image/12";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, "file://media/image/3");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_002, TestSize.Level1)
{
    const string virtualUri = "file://media/audio/12";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, "file://media/audio/3");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_003, TestSize.Level1)
{
    const string virtualUri = "file://media/video/12";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, "file://media/video/3");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_004, TestSize.Level1)
{
    const string virtualUri = "file://media/Photo/12/VID_2023_001/VID_2023.jpg";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_005, TestSize.Level1)
{
    const string virtualUri = "file://media/Audio/12/VID_169_001/VID_2023.mp3";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_006, TestSize.Level1)
{
    const string virtualUri = "file://com.demo.a/data/storage/e12/base/files/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_007, TestSize.Level1)
{
    const string virtualUri = "file://docs/storage/Users/currentUsers/Documents/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_008, TestSize.Level1)
{
    const string virtualUri = "file://docs/storage/Users/currentUsers/Desktop/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_009, TestSize.Level1)
{
    const string virtualUri = "file://docs/storage/Users/currentUsers/Download/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_010, TestSize.Level1)
{
    const string virtualUri = "file://docs/storage/External/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealUriFromVirtualUri_Test_011, TestSize.Level1)
{
    const string virtualUri = "file://docs/storage/Share/12.txt";
    string realUri = MediaFileUtils::GetRealUriFromVirtualUri(virtualUri);
    EXPECT_EQ(realUri, virtualUri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ModifyAsset_Test_001, TestSize.Level1)
{
    string oldPath;
    string newPath;
    int32_t ret = MediaFileUtils::ModifyAsset(oldPath, newPath);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldPath = "";
    newPath = "datashare://test";
    ret = MediaFileUtils::ModifyAsset(oldPath, newPath);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldPath = "datashare://test";
    newPath = "";
    ret = MediaFileUtils::ModifyAsset(oldPath, newPath);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldPath = "datashare://test";
    newPath = "datashare://test/Photo";
    ret = MediaFileUtils::ModifyAsset(oldPath, newPath);
    EXPECT_EQ(ret, E_NO_SUCH_FILE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ModifyAsset_Test_002, TestSize.Level1)
{
    string oldPath = "datashare://test/Photo";
    string newPath = "datashare://test/Download";
    int32_t ret = MediaFileUtils::ModifyAsset(oldPath, newPath);
    EXPECT_EQ(ret, E_NO_SUCH_FILE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateAsset_Test_001, TestSize.Level1)
{
    string filePath;
    int32_t ret = MediaFileUtils::CreateAsset(filePath);
    EXPECT_EQ(ret, E_VIOLATION_PARAMETERS);

    filePath = "datashare://test";
    MediaFileUtils::CreateAsset(filePath);
    EXPECT_EQ(ret, E_VIOLATION_PARAMETERS);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsUriV10_Test_001, TestSize.Level1)
{
    string mediaType = CONST_URI_TYPE_PHOTO;
    bool ret = MediaFileUtils::IsUriV10(mediaType);
    EXPECT_EQ(ret, true);

    mediaType = CONST_URI_TYPE_PHOTO_ALBUM;
    ret = MediaFileUtils::IsUriV10(mediaType);
    EXPECT_EQ(ret, true);

    mediaType = CONST_URI_TYPE_AUDIO_V10;
    ret = MediaFileUtils::IsUriV10(mediaType);
    EXPECT_EQ(ret, true);

    mediaType = "";
    ret = MediaFileUtils::IsUriV10(mediaType);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UriAppendKeyValue_Test_001, TestSize.Level1)
{
    string uri;
    string key;
    string value;
    MediaFileUtils::UriAppendKeyValue(uri, key, value);
    EXPECT_EQ((uri == ""), false);
    EXPECT_EQ((key == ""), true);
    EXPECT_EQ((value == ""), true);

    uri = "datashare://test";
    key = "test";
    value = "testFile";
    MediaFileUtils::UriAppendKeyValue(uri, key, value);
    EXPECT_EQ((uri == ""), false);
    EXPECT_EQ((key == ""), false);
    EXPECT_EQ((value == ""), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Encode_Test_001, TestSize.Level1)
{
    string uri;
    string str = MediaFileUtils::Encode(uri);
    EXPECT_EQ((str == ""), true);

    uri = "test";
    str = MediaFileUtils::Encode(uri);
    EXPECT_EQ((str == ""), false);

    uri = "test/Photo";
    str = MediaFileUtils::Encode(uri);
    EXPECT_EQ((str == ""), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetUriByExtrConditions_Test_001, TestSize.Level1)
{
    string prefix;
    string fileId;
    string suffix;
    string str = MediaFileUtils::GetUriByExtrConditions(prefix, fileId, suffix);
    EXPECT_EQ(str, "");

    prefix = "test";
    fileId = "1234567890";
    suffix = "file";
    str = MediaFileUtils::GetUriByExtrConditions(prefix, fileId, suffix);
    EXPECT_EQ(str, "test1234567890file");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMovingPhotoVideoPath_Test_001, TestSize.Level1)
{
    string path = "/storage/cloud/files/Photo/1/IMG_test.jpg";
    string videoPath = "/storage/cloud/files/Photo/1/IMG_test.mp4";
    EXPECT_EQ(MediaFileUtils::GetMovingPhotoVideoPath(path), videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMovingPhotoVideoPath_Test_002, TestSize.Level1)
{
    string path = "/storage/cloud/files/Photo/1/invalidPath";
    string videoPath = "";
    EXPECT_EQ(MediaFileUtils::GetMovingPhotoVideoPath(path), videoPath);
    EXPECT_EQ(MediaFileUtils::GetMovingPhotoVideoPath(""), videoPath);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoExtension_Test_001, TestSize.Level1)
{
    vector<string> validExtensions = { "jpg", "jpeg", "jpe", "heif", "hif", "heic" };
    for (const auto& extension : validExtensions) {
        EXPECT_EQ(MediaFileUtils::CheckMovingPhotoExtension(extension), true);
    }
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoExtension_Test_002, TestSize.Level1)
{
    vector<string> invalidExtensions = { "arw", "avif", "bm", "bmp", "cur", "dng", "gif", "heics", "heifs",
        "ico", "nrw", "pef", "png", "raf", "raw", "rw2", "srw", "svg", "webp" };
    for (const auto& extension : invalidExtensions) {
        EXPECT_EQ(MediaFileUtils::CheckMovingPhotoExtension(extension), false);
    }
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoVideoExtension_Test_001, TestSize.Level1)
{
    vector<string> validExtensions = { "m4v", "f4v", "mp4v", "mpeg4", "mp4" };
    for (const auto& extension : validExtensions) {
        EXPECT_EQ(MediaFileUtils::CheckMovingPhotoVideoExtension(extension), true);
    }
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoVideoExtension_Test_002, TestSize.Level1)
{
    vector<string> invalidExtensions = { "3gpp2", "3gp2", "3g2", "3gpp", "3gp", "avi", "m2ts", "mts",
        "ts", "yt", "wrf", "mpeg", "mpeg2", "mpv2", "mp2v", "m2v", "m2t", "mpeg1", "mpv1", "mp1v", "m1v",
        "mpg", "mov", "mkv", "webm", "h264" };
    for (const auto& extension : invalidExtensions) {
        EXPECT_EQ(MediaFileUtils::CheckMovingPhotoVideoExtension(extension), false);
    }
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoImage_Test_001, TestSize.Level1)
{
    string videoPath = "/storage/cloud/files/Photo/1/IMG_test_invalid.gif";
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoImage(videoPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoVideo_Test_001, TestSize.Level1)
{
    string videoPath = "/storage/cloud/files/Photo/1/IMG_test_invalid.mp4";
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideo(videoPath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoVideoDuration_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-11000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-10000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-5000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-4000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-3000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-2500), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-2000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(-1000), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(0), false);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(1000), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(2000), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(2500), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(3000), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(5000), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(10000), true);
    EXPECT_EQ(MovingPhotoFileUtils::CheckMovingPhotoVideoDuration(11000), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMovingPhotoEffectMode_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(-10), false);
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(-1), false);

    for (int32_t i = 0; i <= 4; i++) {
        EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(i), true);
    }

    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(5), true);
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(6), false);
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(10), true);
    EXPECT_EQ(MediaFileUtils::CheckMovingPhotoEffectMode(20), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsMediaLibraryUri_Test_001, TestSize.Level1)
{
    string uri = "file://media/Photo/12/IMG_2023_001/IMG_2023.jpg";
    EXPECT_EQ(MediaFileUtils::IsMediaLibraryUri(uri), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsMediaLibraryUri_Test_002, TestSize.Level1)
{
    string uri("file://com.example.testdemo/data/storage/el2/base/haps/test.jpg");
    uri.append(";");
    uri.append("file://com.example.testdemo/data/storage/el2/base/haps/test.mp4");
    EXPECT_EQ(MediaFileUtils::IsMediaLibraryUri(uri), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsMediaLibraryUri_Test_003, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(MediaFileUtils::IsMediaLibraryUri(uri), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SplitMovingPhotoUri_Test_001, TestSize.Level1)
{
    string uri = "file://media/Photo/12/IMG_2023_001/IMG_2023.jpg";
    std::vector<std::string> ret;
    EXPECT_EQ(MediaFileUtils::SplitMovingPhotoUri(uri, ret), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SplitMovingPhotoUri_Test_002, TestSize.Level1)
{
    string uri("file://com.example.testdemo/data/storage/el2/base/haps/test.jpg");
    uri.append(";");
    uri.append("file://com.example.testdemo/data/storage/el2/base/haps/test.mp4");
    std::vector<std::string> ret;
    EXPECT_EQ(MediaFileUtils::SplitMovingPhotoUri(uri, ret), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SplitMovingPhotoUri_Test_003, TestSize.Level1)
{
    string uri = "";
    std::vector<std::string> ret;
    EXPECT_EQ(MediaFileUtils::SplitMovingPhotoUri(uri, ret), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFileSize_Test_001, TestSize.Level1)
{
    bool success = true;
    size_t size = -1;
    string invalidPath = "/storage/cloud/files/Photo/1/IMG_test_invalid.gif";
    success = MediaFileUtils::GetFileSize(invalidPath, size);
    EXPECT_EQ(success, false);
    EXPECT_EQ(size, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Test_001, TestSize.Level1)
{
    const std::string filePath;
    std::string fileContent;
    auto res = MediaFileUtils::ReadStrFromFile(filePath, fileContent);
    EXPECT_EQ(res, false);
    const std::string filePath2 = "a";
    auto res2 = MediaFileUtils::ReadStrFromFile(filePath2, fileContent);
    EXPECT_EQ(res2, false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Test_002, TestSize.Level1)
{
    const string uri = "datashare:///media";
    auto res = MediaFileUtils::GetHighlightPath(uri);
    EXPECT_EQ(res, "/storage/cloud/files");
    const string uri2 = "file://media";
    auto res2 = MediaFileUtils::GetHighlightPath(uri2);
    EXPECT_EQ(res2, "/storage/cloud/files");
    const string uri3 = "";
    auto res3 = MediaFileUtils::GetHighlightPath(uri3);
    EXPECT_EQ(res3, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Test_003, TestSize.Level1)
{
    const string filePath;
    const string mode;
    auto res = MediaFileUtils::OpenAsset(filePath, mode);
    EXPECT_EQ(res, -209);
    const string filePath2 = "a";
    auto res2 = MediaFileUtils::OpenAsset(filePath2, mode);
    EXPECT_EQ(res2, -217);
    const string filePath3 = "a";
    const string mode3 = "r";
    auto res3 = MediaFileUtils::OpenAsset(filePath3, mode3);
    EXPECT_EQ(res3, -209);
    const string filePath4 = "b";
    const string mode4 = "w";
    auto res4 = MediaFileUtils::OpenAsset(filePath4, mode4);
    EXPECT_EQ(res4, -209);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Test_004, TestSize.Level1)
{
    MediaType mediaType = MEDIA_TYPE_VIDEO;
    auto res = MediaFileUtils::GetMediaTypeUri(mediaType);
    EXPECT_EQ(res, "datashare:///media/video");
    MediaType mediaType2 = MEDIA_TYPE_SMARTALBUM;
    auto res2 = MediaFileUtils::GetMediaTypeUri(mediaType2);
    EXPECT_EQ(res2, "datashare:///media/smartalbum");
    auto res2_2 = MediaFileUtils::GetMediaTypeUriV10(mediaType2);
    EXPECT_EQ(res2_2, "datashare:///media/smartalbum");
    MediaType mediaType3 = MEDIA_TYPE_DEVICE;
    auto res3 = MediaFileUtils::GetMediaTypeUri(mediaType3);
    EXPECT_EQ(res3, "datashare:///media/device");
    auto res3_2 = MediaFileUtils::GetMediaTypeUriV10(mediaType3);
    EXPECT_EQ(res3_2, "datashare:///media/device");
    MediaType mediaType4 = MEDIA_TYPE_FILE;
    auto res4_2 = MediaFileUtils::GetMediaTypeUriV10(mediaType4);
    EXPECT_EQ(res4_2, "datashare:///media/file");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Test_005, TestSize.Level1)
{
    const std::string displayName = "a.mp3";
    auto res = MediaFileUtils::GetTableNameByDisplayName(displayName);
    EXPECT_EQ(res, "Audios");
    const std::string displayName2 = "b.mp4";
    auto res2 = MediaFileUtils::GetTableNameByDisplayName(displayName2);
    EXPECT_EQ(res2, "Photos");
    const std::string displayName3 = "c.jpg";
    auto res3 = MediaFileUtils::GetTableNameByDisplayName(displayName3);
    EXPECT_EQ(res3, "Photos");
    const std::string displayName4 = "d.txt";
    auto res4 = MediaFileUtils::GetTableNameByDisplayName(displayName4);
    EXPECT_EQ(res4, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyDirectory_Test_001, TestSize.Level1)
{
    string oldDir;
    string newDir;
    int32_t ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldDir = "";
    newDir = "datashare://test";
    ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldDir = "datashare://test";
    newDir = "";
    ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_MODIFY_DATA_FAIL);

    oldDir = "datashare://test";
    newDir = "datashare://test/Photo";
    ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_NO_SUCH_FILE);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyDirectory_Test_002, TestSize.Level1)
{
    string testPath = "/data/test/copydirectory_002";
    string oldDir = testPath + "/copydirectory_002_srcdir";
    string newDir = testPath + "/copydirectory_002_dstdir";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(oldDir), true);
    int32_t ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyDirectory_Test_003, TestSize.Level1)
{
    string testPath = "/data/test/copydirectory_003";
    string oldDir = testPath + "/copydirectory_003_srcdir";
    string newDir = testPath + "/copydirectory_003_dstdir";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(oldDir), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(newDir), true);
    int32_t ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_FILE_EXIST);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyDirectory_Test_004, TestSize.Level1)
{
    string testPath = "/data/test/copydirectory_004";
    string oldDir = testPath + "/copydirectory_004_srcdir";
    string newDir = testPath + "/copydirectory_004_dstdir";
    string subdirectory = oldDir + "/copydirectory_subdirectory";
    string subfile = oldDir + "/copydirectory_subfile";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(oldDir), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(subdirectory), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(subfile), true);
    int32_t ret = MediaFileUtils::CopyDirectory(oldDir, newDir);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GenerateKvStoreKey_Test_001, TestSize.Level1)
{
    std::string fileId;
    std::string datekey;
    std::string key;
    fileId = "";
    auto res = MediaFileUtils::GenerateKvStoreKey(fileId, datekey, key);
    EXPECT_EQ(res, false);

    fileId = "a";
    datekey = "";
    res = MediaFileUtils::GenerateKvStoreKey(fileId, datekey, key);
    EXPECT_EQ(res, false);
    
    fileId = "0000000000a";
    datekey = "a";
    res = MediaFileUtils::GenerateKvStoreKey(fileId, datekey, key);
    EXPECT_EQ(res, false);

    fileId = "0000000000";
    datekey = "0000000000000a";
    res = MediaFileUtils::GenerateKvStoreKey(fileId, datekey, key);
    EXPECT_EQ(res, false);

    fileId = "0000000000";
    datekey = "0000000000000";
    res = MediaFileUtils::GenerateKvStoreKey(fileId, datekey, key);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetDateModified_Test_001, TestSize.Level1)
{
    string filePath = "/data/GetDateModified_Test_001.jpg";
    int64_t dateModified = 0;
    EXPECT_EQ(MediaFileUtils::GetDateModified(filePath, dateModified), false);
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    EXPECT_EQ(MediaFileUtils::GetDateModified(filePath, dateModified), true);
    EXPECT_GT(dateModified, 0);
    EXPECT_EQ(MediaFileUtils::DeleteFile(filePath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAllTypes_Test_001, TestSize.Level1)
{
    int32_t extension = MEDIA_TYPE_IMAGE;
    vector<std::string> allTypesOut = MediaFileUtils::GetAllTypes(extension);
    EXPECT_GT(allTypesOut.size(), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAllTypes_Test_002, TestSize.Level1)
{
    int32_t extension = MEDIA_TYPE_VIDEO;
    vector<std::string> allTypesOut = MediaFileUtils::GetAllTypes(extension);
    EXPECT_GT(allTypesOut.size(), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetAllTypes_Test_003, TestSize.Level1)
{
    int32_t extension = MEDIA_TYPE_FILE;
    vector<std::string> allTypesOut = MediaFileUtils::GetAllTypes(extension);
    EXPECT_TRUE(allTypesOut.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckSupportedWatermarkType_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(-10), false);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(-1), false);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(0), false);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(1), true);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(2), true);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(3), true);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(4), false);
    EXPECT_EQ(MediaFileUtils::CheckSupportedWatermarkType(10), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckCompositeDisplayMode_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(-10), false);
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(-1), false);
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(0), true);
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(1), true);
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(2), false);
    EXPECT_EQ(MediaFileUtils::CheckCompositeDisplayMode(10), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateAssetRealName_Test_001, TestSize.Level1)
{
    int32_t fileId = 1;
    int32_t mediaType = MediaType::MEDIA_TYPE_IMAGE;
    string extension = "jpg";
    string name = "";
    int32_t res = MediaFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
    EXPECT_EQ(res, E_OK);
    size_t lastPos = name.rfind('.');
    string str = name.substr(lastPos + 1);
    EXPECT_EQ(extension, str);

    fileId = 1000;
    extension = "mp4";
    mediaType = MediaType::MEDIA_TYPE_VIDEO;
    res = MediaFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
    EXPECT_EQ(res, E_OK);
    lastPos = name.rfind('.');
    str = name.substr(lastPos + 1);
    EXPECT_EQ(extension, str);

    extension = "mp3";
    mediaType = MediaType::MEDIA_TYPE_AUDIO;
    res = MediaFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
    EXPECT_EQ(res, E_OK);
    lastPos = name.rfind('.');
    str = name.substr(lastPos + 1);
    EXPECT_EQ(extension, str);

    mediaType = MediaType::MEDIA_TYPE_MEDIA;
    res = MediaFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
    EXPECT_EQ(res, E_INVALID_VALUES);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateAssetRealName_Test_002, TestSize.Level1)
{
    int32_t fileId = 1;
    int32_t mediaType = MediaType::MEDIA_TYPE_IMAGE;
    string extension = "";
    string name = "";
    int32_t res = MediaFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
    EXPECT_EQ(res, E_OK);
    size_t lastPos = name.rfind('.');
    EXPECT_EQ(std::string::npos, lastPos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateAssetRealName_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CreateAssetRealName_Test_003 begin");
    string localDir = "/storage/media/local/files/custom_restore/test003/";
    MediaFileUtils::CreateDirectory(localDir);
    string cloudDir = "/storage/cloud/files/custom_restore/test003/";
    string fileName = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.JPG";
    string localPath = localDir + fileName;
    string cloudPath = cloudDir + fileName;
    int result = MediaFileUtils::CreateAsset(localPath);
    EXPECT_EQ(result, E_OK);
    bool localExists = MediaFileUtils::IsFileExists(localPath);
    EXPECT_EQ(localExists, true);
    bool cloudExists = MediaFileUtils::IsFileExists(cloudPath);
    EXPECT_EQ(cloudExists, false);
    MediaFileUtils::RemoveDirectory(localDir);
    MEDIA_INFO_LOG("MediaFileUtils_CreateAssetRealName_Test_003 finish");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateAssetRealName_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CreateAssetRealName_Test_004 begin");
    string localDir = "/storage/media/local/files/custom_restore/test004/";
    MediaFileUtils::CreateDirectory(localDir);
    string cloudDir = "/storage/cloud/files/custom_restore/test004/";
    string fileName = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.JPG";
    string localPath = localDir + fileName;
    string cloudPath = cloudDir + fileName;
    int result = MediaFileUtils::CreateAsset(localPath);
    EXPECT_EQ(result, E_OK);
    bool localExists = MediaFileUtils::IsFileExists(localPath);
    EXPECT_EQ(localExists, true);
    bool cloudExists = MediaFileUtils::IsFileExists(cloudPath);
    EXPECT_EQ(cloudExists, true);
    MediaFileUtils::RemoveDirectory(localDir);
    MEDIA_INFO_LOG("MediaFileUtils_CreateAssetRealName_Test_004 finish");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeUri_Test_01, TestSize.Level1)
{
    MediaType mediaType = MEDIA_TYPE_SMARTALBUM;
    string res = MediaFileUtils::GetMediaTypeUri(mediaType);
    EXPECT_EQ(res, "datashare:///media/smartalbum");

    mediaType = MEDIA_TYPE_DEVICE;
    res = MediaFileUtils::GetMediaTypeUri(mediaType);
    EXPECT_EQ(res, "datashare:///media/device");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFileModificationTime_Test_01, TestSize.Level1)
{
    int64_t res = MediaFileUtils::GetFileModificationTime("");
    EXPECT_EQ(res, 0);
    res = MediaFileUtils::GetFileModificationTime("/data/CreateImageThumbnailTest_001.jpg");
    EXPECT_NE(res, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StrToInt64_Test_01, TestSize.Level1)
{
    int64_t res = MediaFileUtils::StrToInt64("");
    EXPECT_EQ(res, 0);
    res = MediaFileUtils::StrToInt64("1");
    EXPECT_EQ(res, 1);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirExists_Test_01, TestSize.Level1)
{
    EXPECT_FALSE(MediaFileUtils::IsDirExists(""));
    EXPECT_TRUE(MediaFileUtils::IsDirExists("/data"));
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAppLink_Test_01, TestSize.Level1)
{
    string linkInfo = "";
    EXPECT_EQ(MediaFileUtils::CheckAppLink(linkInfo), -EINVAL);
}
 
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAppLink_Test_02, TestSize.Level1)
{
    string linkInfo = "https://item.taobao.com/item.htm?id=899930519163";
    EXPECT_EQ(MediaFileUtils::CheckAppLink(linkInfo), E_OK);
}
 
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAppLink_Test_03, TestSize.Level1)
{
    string linkInfo(512, 'a');
    EXPECT_EQ(MediaFileUtils::CheckAppLink(linkInfo), E_OK);
}
 
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAppLink_Test_04, TestSize.Level1)
{
    string linkInfo(513, 'a');
    EXPECT_EQ(MediaFileUtils::CheckAppLink(linkInfo), -ENAMETOOLONG);
}
 
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckHasAppLink_Test_01, TestSize.Level1)
{
    EXPECT_FALSE(MediaFileUtils::CheckHasAppLink(-1));
    EXPECT_TRUE(MediaFileUtils::CheckHasAppLink(0));
    EXPECT_TRUE(MediaFileUtils::CheckHasAppLink(1));
    EXPECT_TRUE(MediaFileUtils::CheckHasAppLink(2));
    EXPECT_FALSE(MediaFileUtils::CheckHasAppLink(3));
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CalculateAspectRatio_Test_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_01 Start");
    int32_t height = ASPECT_RATIO_UNSUPPORT;
    int32_t width = 1;
    EXPECT_EQ(MediaFileUtils::CalculateAspectRatio(height, width), ASPECT_RATIO_UNSUPPORT);
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_01 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CalculateAspectRatio_Test_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_02 Start");
    int32_t height = 1;
    int32_t width = ASPECT_RATIO_MAX + 1;
    double result = MediaFileUtils::CalculateAspectRatio(height, width);
    MEDIA_INFO_LOG("result is :%{public}f", result);
    EXPECT_EQ(result, ASPECT_RATIO_MAX);
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_02 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CalculateAspectRatio_Test_03, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_03 Start");
    int32_t height = ASPECT_RATIO_MAX_TEST;
    int32_t width = 1;
    double result = MediaFileUtils::CalculateAspectRatio(height, width);
    MEDIA_INFO_LOG("result is :%{public}f", result);
    EXPECT_EQ(result, ASPECT_RATIO_MIN);
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_03 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CalculateAspectRatio_Test_04, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_04 Start");
    int32_t height = 16;
    int32_t width = 8;
    double result = MediaFileUtils::CalculateAspectRatio(height, width);
    MEDIA_INFO_LOG("result is :%{public}f", result);
    EXPECT_EQ(result, 0.5);
    MEDIA_INFO_LOG("MediaFileUtils_CalculateAspectRatio_Test_04 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizeName_Test_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_01 Start");

    std::vector<std::pair<std::string, std::string>> testCases{
        {"", ""}, {"a", "*"}, {"ab", "*b"}, {"abc", "a*c"}, {"abcd", "a**d"}, {"abcdefghij", "abc***ghij"}};

    for (const auto &testCase : testCases) {
        std::string displayName = testCase.first;
        std::string expect = testCase.second;
        std::string actual = MediaFileUtils::DesensitizeName(displayName);
        MEDIA_DEBUG_LOG("DisplayName: %{public}s, Actual: %{public}s", displayName.c_str(), actual.c_str());
        EXPECT_EQ(actual, expect);
    }

    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_01 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizeName_Test_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_02 Start");

    std::vector<std::pair<std::string, std::string>> testCases{
        {"文", "*"}, {"文字", "*字"}, {"文字国", "文*国"}, {"文字国际", "文**际"}, {"文字国际化测试", "文字***测试"}};

    for (const auto &testCase : testCases) {
        std::string displayName = testCase.first;
        std::string expect = testCase.second;
        std::string actual = MediaFileUtils::DesensitizeName(displayName);
        MEDIA_DEBUG_LOG("DisplayName: %{public}s, Actual: %{public}s", displayName.c_str(), actual.c_str());
        EXPECT_EQ(actual, expect);
    }

    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_02 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizeName_Test_03, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_03 Start");

    std::vector<std::pair<std::string, std::string>> testCases{{"👨‍👩‍👧‍👦", "*"},
        {"ñ🚀", "*🚀"},
        {"👨‍💻ạ̇👍🏽", "👨‍💻*👍🏽"},
        {"👷🏾⚠️ﬁ🇨🇳", "👷🏾**🇨🇳"},
        {"💻👨👩👧👦⚠👍", "💻👨***⚠👍"}};

    for (const auto &testCase : testCases) {
        std::string displayName = testCase.first;
        std::string expect = testCase.second;
        std::string actual = MediaFileUtils::DesensitizeName(displayName);
        MEDIA_DEBUG_LOG("DisplayName: %{public}s, Actual: %{public}s", displayName.c_str(), actual.c_str());
        EXPECT_EQ(actual, expect);
    }

    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_03 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizeName_Test_04, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_04 Start");

    std::vector<std::pair<std::string, std::string>> testCases{{"后缀测试🇺🇳.jpg", "后***🇺🇳.jpg"},
        {"Suffix test é.mp4", "Suffi***est é.mp4"},
        {"3️⃣👌🥩👆👇🎫.png", "3️⃣***👇🎫.png"},
        {"文3️3test.heic", "文3️***st.heic"},
        {"👌test🈶.3g2", "👌***t🈶.3g2"}};

    for (const auto &testCase : testCases) {
        std::string displayName = testCase.first;
        std::string expect = testCase.second;
        std::string actual = MediaFileUtils::DesensitizeName(displayName);
        MEDIA_DEBUG_LOG("DisplayName: %{public}s, Actual: %{public}s", displayName.c_str(), actual.c_str());
        EXPECT_EQ(actual, expect);
    }

    MEDIA_INFO_LOG("MediaFileUtils_DesensitizeName_Test_04 end");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileAndDelSrc_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CopyFileAndDelSrc_Test_001 Start");
    string dirPath = "/data/test/CopyFileAndDelSrc";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string srcFile = dirPath + "/test1.jpg";
    string destFile = dirPath + "/test2.jpg";
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(destFile), true);
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), false);

    // When dest file not exist
    EXPECT_EQ(MediaFileUtils::CopyFileAndDelSrc(srcFile, destFile), true);
    MEDIA_INFO_LOG("MediaFileUtils_CopyFileAndDelSrc_Test_001 End");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileAndDelSrc_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_CopyFileAndDelSrc_Test_002 Start");
    string dirPath = "/data/test/CopyFileAndDelSrc";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string srcFile = dirPath + "/test1.jpg";
    string destFile = dirPath + "/test1.jpg";
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), false);

    // When src and dest are same file, operation should fail
    EXPECT_EQ(MediaFileUtils::CopyFileAndDelSrc(srcFile, destFile), false);
    MEDIA_INFO_LOG("MediaFileUtils_CopyFileAndDelSrc_Test_002 End");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatCopy_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_001 Start");
    string dirPath = "/data/test/CopyFileAndDelSrc";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    string srcFile = dirPath + "/test3.jpg";
    string destFile = dirPath + "/test4.jpg";
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(destFile), true);
    EXPECT_EQ(MediaFileUtils::IsDirEmpty(dirPath), false);
    auto result = MediaFileUtils::ConvertFormatCopy(srcFile, destFile, "jpg");
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_001 End");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatCopy_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_002 Start");
    string dirPath = "/data/test/";
    dirPath.resize(4100, 'a');
    string srcFile = dirPath + "/test3.jpg";
    auto result = MediaFileUtils::ConvertFormatCopy(srcFile, "", "jpg");
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_002 End");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatCopy_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_003 Start");
    string dirPath = "/data/test/x/y/z/x/y/z";
    string srcFile = "../../..";
    auto result = MediaFileUtils::ConvertFormatCopy(srcFile, "", "jpg");
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_003 End");
}
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatCopy_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_004 Start");
    auto result = MediaFileUtils::ConvertFormatCopy("", "", "jpg");
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_004 End");
}
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatCopy_Test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_005 Start");
    string dirPath = "/data/test/";
    string srcFile = dirPath + "/test5.jpg";
    auto result = MediaFileUtils::ConvertFormatCopy(srcFile, "", "jpg");
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("MediaFileUtils_ConvertFormatCopy_Test_005 End");
}
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileValid_Test_001, TestSize.Level1)
{
    string filePath = "/data/test/isfilevalid_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    EXPECT_EQ(MediaFileUtils::IsFileValid(filePath), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(filePath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileValid_Test_002, TestSize.Level1)
{
    string filePath = "/data/test/isfilevalid_002";
    EXPECT_EQ(MediaFileUtils::IsFileValid(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileValid_Test_003, TestSize.Level1)
{
    string dirPath = "/data/test/isfilevalid_003";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::IsFileValid(dirPath), false);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteFileWithRetry_Test_001, TestSize.Level1)
{
    string filePath = "/data/test/deletefilewithretry_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    EXPECT_EQ(MediaFileUtils::DeleteFileWithRetry(filePath), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteFileWithRetry_Test_002, TestSize.Level1)
{
    string filePath = "/data/test/deletefilewithretry_002";
    EXPECT_EQ(MediaFileUtils::DeleteFileWithRetry(filePath), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetParentPath_Test_001, TestSize.Level1)
{
    string path = "/data/test/file.txt";
    string parentPath = MediaFileUtils::GetParentPath(path);
    EXPECT_EQ(parentPath, "/data/test");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetParentPath_Test_002, TestSize.Level1)
{
    string path = "file.txt";
    string parentPath = MediaFileUtils::GetParentPath(path);
    EXPECT_EQ(parentPath, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetParentPath_Test_003, TestSize.Level1)
{
    string path = "";
    string parentPath = MediaFileUtils::GetParentPath(path);
    EXPECT_EQ(parentPath, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirectory_Test_001, TestSize.Level1)
{
    string dirPath = "/data/test/isdirectory_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::IsDirectory(dirPath), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dirPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsDirectory_Test_002, TestSize.Level1)
{
    string filePath = "/data/test/isdirectory_002";
    EXPECT_EQ(MediaFileUtils::CreateFile(filePath), true);
    EXPECT_EQ(MediaFileUtils::IsDirectory(filePath), false);
    EXPECT_EQ(MediaFileUtils::DeleteFile(filePath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileUtil_Test_001, TestSize.Level1)
{
    string srcFile = "/data/test/copyfileutil_001_src";
    string dstFile = "/data/test/copyfileutil_001_dst";
    string testString = "test content";
    
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testString), true);
    EXPECT_EQ(MediaFileUtils::CopyFileUtil(srcFile, dstFile), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(dstFile), true);
    
    EXPECT_EQ(MediaFileUtils::DeleteFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(dstFile), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileUtil_Test_002, TestSize.Level1)
{
    string srcFile = "/data/test/copyfileutil_002_src";
    string dstFile = "/data/test/copyfileutil_002_dst";
    
    EXPECT_EQ(MediaFileUtils::CopyFileUtil(srcFile, dstFile), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_001, TestSize.Level1)
{
    string albumName = "Test Album";
    EXPECT_EQ(MediaFileUtils::CheckAlbumName(albumName), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_002, TestSize.Level1)
{
    string albumName = "";
    EXPECT_LT(MediaFileUtils::CheckAlbumName(albumName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckAlbumName_Test_003, TestSize.Level1)
{
    string albumName = "Test/Album";
    EXPECT_LT(MediaFileUtils::CheckAlbumName(albumName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckHighlightSubtitle_Test_001, TestSize.Level1)
{
    string subtitle = "Test Subtitle";
    EXPECT_EQ(MediaFileUtils::CheckHighlightSubtitle(subtitle), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckHighlightSubtitle_Test_002, TestSize.Level1)
{
    string subtitle = "";
    int32_t result = MediaFileUtils::CheckHighlightSubtitle(subtitle);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDentryName_Test_001, TestSize.Level1)
{
    string dentryName = "valid_name";
    EXPECT_EQ(MediaFileUtils::CheckDentryName(dentryName), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckDentryName_Test_002, TestSize.Level1)
{
    string dentryName = "invalid/name";
    EXPECT_LT(MediaFileUtils::CheckDentryName(dentryName), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckTitle_Test_001, TestSize.Level1)
{
    string title = "valid_title";
    EXPECT_EQ(MediaFileUtils::CheckTitle(title), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckTitle_Test_002, TestSize.Level1)
{
    string title = "invalid/title";
    EXPECT_LT(MediaFileUtils::CheckTitle(title), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckTitleCompatible_Test_001, TestSize.Level1)
{
    string title = "valid_title";
    EXPECT_EQ(MediaFileUtils::CheckTitleCompatible(title), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckTitleCompatible_Test_002, TestSize.Level1)
{
    string title = "invalid/title";
    EXPECT_LT(MediaFileUtils::CheckTitleCompatible(title), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFileAssetUri_Test_001, TestSize.Level1)
{
    string fileAssetData = "/storage/cloud/files/Photo/1/test";
    string displayName = "test.jpg";
    int32_t fileId = 123;
    string uri = MediaFileUtils::GetFileAssetUri(fileAssetData, displayName, fileId);
    EXPECT_FALSE(uri.empty());
    EXPECT_NE(uri.find(to_string(fileId)), string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckRelativePath_Test_001, TestSize.Level1)
{
    string relativePath = "Photo/test";
    EXPECT_EQ(MediaFileUtils::CheckRelativePath(relativePath), E_OK);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckRelativePath_Test_002, TestSize.Level1)
{
    string relativePath = "";
    EXPECT_LT(MediaFileUtils::CheckRelativePath(relativePath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckRelativePath_Test_003, TestSize.Level1)
{
    string relativePath = "Photo//test";
    EXPECT_LT(MediaFileUtils::CheckRelativePath(relativePath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtilsFormatRelativePath_Test_001, TestSize.Level1)
{
    string relativePath = "Photo/test";
    MediaFileUtils::FormatRelativePath(relativePath);
    EXPECT_EQ(relativePath, "Photo/test/");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_FormatRelativePath_Test_002, TestSize.Level1)
{
    string relativePath = "/Photo/test";
    MediaFileUtils::FormatRelativePath(relativePath);
    EXPECT_EQ(relativePath, "Photo/test/");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_FormatRelativePath_Test_003, TestSize.Level1)
{
    string relativePath = "";
    MediaFileUtils::FormatRelativePath(relativePath);
    EXPECT_EQ(relativePath, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRootDirFromRelativePath_Test_001, TestSize.Level1)
{
    string relativePath = "Photo/test";
    string rootDir;
    MediaFileUtils::GetRootDirFromRelativePath(relativePath, rootDir);
    EXPECT_EQ(rootDir, "Photo/");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRootDirFromRelativePath_Test_002, TestSize.Level1)
{
    string relativePath = "/Photo/test";
    string rootDir;
    MediaFileUtils::GetRootDirFromRelativePath(relativePath, rootDir);
    EXPECT_EQ(rootDir, "Photo/");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRootDirFromRelativePath_Test_003, TestSize.Level1)
{
    string relativePath = "";
    string rootDir;
    MediaFileUtils::GetRootDirFromRelativePath(relativePath, rootDir);
    EXPECT_EQ(rootDir, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UTCTimeMilliSeconds_Test_001, TestSize.Level1)
{
    int64_t time = MediaFileUtils::UTCTimeMilliSeconds();
    EXPECT_GT(time, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UTCTimeNanoSeconds_Test_001, TestSize.Level1)
{
    int64_t time = MediaFileUtils::UTCTimeNanoSeconds();
    EXPECT_GT(time, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetIdFromUri_Test_001, TestSize.Level1)
{
    string uri = "datashare:///media/Photo/123";
    string id = MediaFileUtils::GetIdFromUri(uri);
    EXPECT_EQ(id, "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetIdFromUri_Test_002, TestSize.Level1)
{
    string uri = "";
    string id = MediaFileUtils::GetIdFromUri(uri);
    EXPECT_EQ(id, "-1");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeNotSupported_Test_001, TestSize.Level1)
{
    string filePath = "test.mid";
    MediaType type = MediaFileUtils::GetMediaTypeNotSupported(filePath);
    EXPECT_EQ(type, MEDIA_TYPE_AUDIO);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SplitByChar_Test_001, TestSize.Level1)
{
    string str = "path/to/file.txt";
    char split = '/';
    string result = MediaFileUtils::SplitByChar(str, split);
    EXPECT_EQ(result, "file.txt");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SplitByChar_Test_002, TestSize.Level1)
{
    string str = "file.txt";
    char split = '/';
    string result = MediaFileUtils::SplitByChar(str, split);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetExtensionFromPath_Test_001, TestSize.Level1)
{
    string path = "test.JPG";
    string ext = MediaFileUtils::GetExtensionFromPath(path);
    EXPECT_EQ(ext, "jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetExtensionFromPath_Test_002, TestSize.Level1)
{
    string path = "test";
    string ext = MediaFileUtils::GetExtensionFromPath(path);
    EXPECT_EQ(ext, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMode_Test_001, TestSize.Level1)
{
    string mode = MEDIA_FILEMODE_READONLY;
    EXPECT_EQ(MediaFileUtils::CheckMode(mode), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CheckMode_Test_002, TestSize.Level1)
{
    string mode = "invalid";
    EXPECT_EQ(MediaFileUtils::CheckMode(mode), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_FindIgnoreCase_Test_001, TestSize.Level1)
{
    string str = "Hello World";
    string key = "world";
    size_t pos = MediaFileUtils::FindIgnoreCase(str, key);
    EXPECT_NE(pos, string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_FindIgnoreCase_Test_002, TestSize.Level1)
{
    string str = "Hello World";
    string key = "xyz";
    size_t pos = MediaFileUtils::FindIgnoreCase(str, key);
    EXPECT_EQ(pos, string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetVirtualIdByType_Test_001, TestSize.Level1)
{
    int32_t id = 10;
    MediaType type = MEDIA_TYPE_IMAGE;
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(id, type);
    EXPECT_GT(virtualId, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetRealIdByTable_Test_001, TestSize.Level1)
{
    int32_t virtualId = 45;
    string tableName = "Photos";
    double realId = MediaFileUtils::GetRealIdByTable(virtualId, tableName);
    EXPECT_GT(realId, 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetVirtualUriFromRealUri_Test_001, TestSize.Level1)
{
    string uri = "datashare:///media/Photo/123";
    string virtualUri = MediaFileUtils::GetVirtualUriFromRealUri(uri);
    EXPECT_FALSE(virtualUri.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReplaceAll_Test_001, TestSize.Level1)
{
    string str = "hello world hello";
    string from = "hello";
    string to = "hi";
    MediaFileUtils::ReplaceAll(str, from, to);
    EXPECT_EQ(str, "hi world hi");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetExtraUri_Test_001, TestSize.Level1)
{
    string displayName = "test.jpg";
    string path = "/storage/cloud/files/Photo/1/test.jpg";
    string extraUri = MediaFileUtils::GetExtraUri(displayName, path, false);
    EXPECT_FALSE(extraUri.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_AddDocsToRelativePath_Test_001, TestSize.Level1)
{
    string relativePath = "Documents/test";
    string result = MediaFileUtils::AddDocsToRelativePath(relativePath);
    EXPECT_EQ(result, "Docs/Documents/test");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_RemoveDocsFromRelativePath_Test_001, TestSize.Level1)
{
    string relativePath = "/storage/cloud/files/Documents/test";
    string result = MediaFileUtils::RemoveDocsFromRelativePath(relativePath);
    EXPECT_EQ(result, "/storage/cloud/files/Documents/test");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_Timespec2Millisecond_Test_001, TestSize.Level1)
{
    struct timespec time;
    time.tv_sec = 1;
    time.tv_nsec = 500000000;
    int64_t milliseconds = MediaFileUtils::Timespec2Millisecond(time);
    EXPECT_EQ(milliseconds, 1500);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTempOriMovingPhotoVideoPath_Test_001, TestSize.Level1)
{
    string imagePath = "/storage/cloud/files/Photo/1/IMG_test.jpg";
    string videoPath = MediaFileUtils::GetTempMovingPhotoVideoPath(imagePath);
    EXPECT_FALSE(videoPath.empty());
    EXPECT_EQ(videoPath, "/storage/cloud/files/Photo/1/temp_IMG_test.mp4");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsMovingPhotoMimeType_Test_001, TestSize.Level1)
{
    string mimeType = "image/jpeg";
    EXPECT_EQ(MediaFileUtils::IsMovingPhotoMimeType(mimeType), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsMovingPhotoMimeType_Test_002, TestSize.Level1)
{
    string mimeType = "image/png";
    EXPECT_EQ(MediaFileUtils::IsMovingPhotoMimeType(mimeType), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizePath_Test_001, TestSize.Level1)
{
    string path = "/storage/cloud/files/Photo/test.jpg";
    string result = MediaFileUtils::DesensitizePath(path);
    EXPECT_NE(result.find("*"), string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizePath_Test_002, TestSize.Level1)
{
    string path = "/data/test/test.jpg";
    string result = MediaFileUtils::DesensitizePath(path);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CreateDirectoryAndCopyFiles_Test_001, TestSize.Level1)
{
    string srcDir = "/data/test/createdirectoryandcopyfiles_src";
    string dstDir = "/data/test/createdirectoryandcopyfiles_dst";
    
    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    string srcFile = srcDir + "/test.txt";
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    
    int32_t ret = MediaFileUtils::CreateDirectoryAndCopyFiles(srcDir, dstDir);
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_EQ(MediaFileUtils::DeleteDir(srcDir), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dstDir), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetUriWithoutDisplayname_Test_001, TestSize.Level1)
{
    string uri = "datashare:///media/Photo/123/test.jpg";
    string result = MediaFileUtils::GetUriWithoutDisplayname(uri);
    EXPECT_EQ(result, "datashare:///media/Photo/123/");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveDirectory_Test_001, TestSize.Level1)
{
    string srcDir = "/data/test/movedirectory_src";
    string dstDir = "/data/test/movedirectory_dst";
    
    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    int32_t ret = MediaFileUtils::MoveDirectory(srcDir, dstDir);
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_EQ(MediaFileUtils::DeleteDir(dstDir), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ConvertFormatExtraDataDirectory_Test_001, TestSize.Level1)
{
    string srcDir = "/data/test/convertformatextradatadirectory_src";
    string dstDir = "/data/test/convertformatextradatadirectory_dst";
    
    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    bool ret = MediaFileUtils::ConvertFormatExtraDataDirectory(srcDir, dstDir, "jpg");
    EXPECT_EQ(ret, false);
    
    EXPECT_EQ(MediaFileUtils::DeleteDir(srcDir), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir(dstDir), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetReplacedPathByPrefix_Test_001, TestSize.Level1)
{
    string srcPrefix = "/storage/cloud";
    string dstPrefix = "/storage/local";
    string path = "/storage/cloud/files/Photo/test.jpg";
    string result = MediaFileUtils::GetReplacedPathByPrefix(srcPrefix, dstPrefix, path);
    EXPECT_EQ(result, "/storage/local/files/Photo/test.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetLocalPath_Test_001, TestSize.Level1)
{
    string path = "/storage/cloud/files/Photo/test.jpg";
    string result = MediaFileUtils::GetLocalPath(path);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/test.jpg");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetLocalPath_Test_002, TestSize.Level1)
{
    string path = "/data/test/test.jpg";
    string result = MediaFileUtils::GetLocalPath(path);
    EXPECT_EQ(result, path);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsCalledBySelf_Test_001, TestSize.Level1)
{
    bool result = MediaFileUtils::IsCalledBySelf();
    EXPECT_FALSE(result);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CloseAsset_Test_001, TestSize.Level1)
{
    string path = "/data/test/closeasset_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(path), true);
    
    int32_t fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0) {
        int32_t ret = MediaFileUtils::CloseAsset(fd);
        EXPECT_EQ(ret, 0);
    }
    
    EXPECT_EQ(MediaFileUtils::DeleteFile(path), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteFileOrFolder_Test_001, TestSize.Level1)
{
    string path = "/data/test/deletefileorfolder_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(path), true);
    EXPECT_EQ(MediaFileUtils::DeleteFileOrFolder(path, true), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(path), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DeleteFileOrFolder_Test_002, TestSize.Level1)
{
    string path = "/data/test/deletefileorfolder_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(path), true);
    EXPECT_EQ(MediaFileUtils::DeleteFileOrFolder(path, false), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(path), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyDirAndDelSrc_Test_001, TestSize.Level1)
{
    string srcPath = "/data/test/copydiranddelsrc_src";
    string destPath = "/data/test/copydiranddelsrc_dst";
    
    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcPath), true);
    string srcFile = srcPath + "/test.txt";
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    
    bool ret = MediaFileUtils::CopyDirAndDelSrc(srcPath, destPath);
    EXPECT_EQ(ret, true);
    
    EXPECT_EQ(MediaFileUtils::DeleteDir(destPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_UpdateModifyTimeInMsec_Test_001, TestSize.Level1)
{
    string path = "/data/test/updatemodifytimeinmsec_001";
    EXPECT_EQ(MediaFileUtils::CreateFile(path), true);
    
    int64_t modifiedTime = 1234567890;
    int32_t ret = MediaFileUtils::UpdateModifyTimeInMsec(path, modifiedTime);
    EXPECT_EQ(ret, E_OK);
    
    EXPECT_EQ(MediaFileUtils::DeleteFile(path), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMimeTypeFromDisplayName_Test_001, TestSize.Level1)
{
    string displayName = "test.jpg";
    string mimeType = MediaFileUtils::GetMimeTypeFromDisplayName(displayName);
    EXPECT_FALSE(mimeType.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMimeTypeFromDisplayName_Test_002, TestSize.Level1)
{
    string displayName = "test";
    string mimeType = MediaFileUtils::GetMimeTypeFromDisplayName(displayName);
    EXPECT_EQ(mimeType, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_DesensitizeUri_Test_001, TestSize.Level1)
{
    string fileUri = "datashare:///media/Photo/123/test.jpg";
    string result = MediaFileUtils::DesensitizeUri(fileUri);
    EXPECT_NE(result.find("*"), string::npos);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StrCreateTime_Test_001, TestSize.Level1)
{
    string format = "%Y-%m-%d %H:%M:%S";
    int64_t time = 1234567890;
    string result = MediaFileUtils::StrCreateTime(format, time);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StrCreateTimeByMilliseconds_Test_001, TestSize.Level1)
{
    string format = "%Y-%m-%d %H:%M:%S";
    int64_t time = 1234567890000;
    string result = MediaFileUtils::StrCreateTimeByMilliseconds(format, time);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StatDirSize_Test_001, TestSize.Level1)
{
    string rootPath = "/data/test/statdirsize_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(rootPath), true);
    
    string file1 = rootPath + "/test1.txt";
    string file2 = rootPath + "/test2.txt";
    EXPECT_EQ(MediaFileUtils::CreateFile(file1), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(file2), true);
    
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(file1, "test content 1"), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(file2, "test content 2"), true);
    
    size_t totalSize = 0;
    MediaFileUtils::StatDirSize(rootPath, totalSize);
    EXPECT_GT(totalSize, 0);
    
    EXPECT_EQ(MediaFileUtils::DeleteDir(rootPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StatDirSize_Test_002, TestSize.Level1)
{
    string rootPath = "/data/test/statdirsize_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(rootPath), true);

    string fileEditData = rootPath + "/editdata";
    string fileSourceJpg = rootPath + "/source.jpg";
    string fileSourceMp4 = rootPath + "/source.mp4";
    string fileOther = rootPath + "/other.bin";

    EXPECT_EQ(MediaFileUtils::CreateFile(fileEditData), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(fileSourceJpg), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(fileSourceMp4), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(fileOther), true);

    EXPECT_EQ(MediaFileUtils::WriteStrToFile(fileEditData, "1234567890"), true);   // 10
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(fileSourceJpg, "1234567890123"), true); // 13
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(fileSourceMp4, "1234567"), true);       // 7
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(fileOther, "1234"), true);               // 4

    size_t totalSize = 0;
    size_t matchedSize = 0;
    auto predicate = [](const std::string &fileName) {
        if (fileName == "editdata") {
            return true;
        }
        size_t dotPos = fileName.find('.');
        std::string baseName = (dotPos == std::string::npos) ? fileName : fileName.substr(0, dotPos);
        return baseName == "source";
    };

    MediaFileUtils::StatDirSize(rootPath, totalSize, matchedSize, predicate);
    EXPECT_EQ(totalSize, 34);
    EXPECT_EQ(matchedSize, 30);

    EXPECT_EQ(MediaFileUtils::DeleteDir(rootPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_StatDirSize_Test_003, TestSize.Level1)
{
    string rootPath = "/data/test/statdirsize_003";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(rootPath), true);

    string file1 = rootPath + "/a.txt";
    string file2 = rootPath + "/b.txt";
    EXPECT_EQ(MediaFileUtils::CreateFile(file1), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(file2), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(file1, "12345"), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(file2, "1234567"), true);

    size_t totalSizeOldApi = 0;
    MediaFileUtils::StatDirSize(rootPath, totalSizeOldApi);

    size_t totalSizeNewApi = 0;
    size_t matchedSize = 0;
    std::function<bool(const std::string&)> emptyPredicate;
    MediaFileUtils::StatDirSize(rootPath, totalSizeNewApi, matchedSize, emptyPredicate);

    EXPECT_EQ(totalSizeOldApi, 12);
    EXPECT_EQ(totalSizeNewApi, totalSizeOldApi);
    EXPECT_EQ(matchedSize, 0);

    EXPECT_EQ(MediaFileUtils::DeleteDir(rootPath), true);
}

/**
 * @tc.name: SwapExtensionInPath_JpgToHeic
 * @tc.desc: 验证SwapExtensionInPath将路径中的.jpg替换为.heic
 *           [覆盖分支] rfind定位到.jpg并replace为.heic
 *           [触发条件] 输入含.jpg的editData路径，fromExt=".jpg"，toExt=".heic"
 *           [业务验证] 返回路径中.jpg被替换为.heic，其他部分不变
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SwapExtensionInPath_Test_001, TestSize.Level1)
{
    string path = "/storage/cloud/files/.editData/Photo/X/IMG_123_456.jpg/editdata_camera";
    string result = MediaFileUtils::SwapExtensionInPath(path, ".jpg", ".heic");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/Photo/X/IMG_123_456.heic/editdata_camera");
}

/**
 * @tc.name: SwapExtensionInPath_HeicToJpg
 * @tc.desc: 验证SwapExtensionInPath将路径中的.heic替换为.jpg
 *           [覆盖分支] rfind定位到.heic并replace为.jpg
 *           [触发条件] 输入含.heic的editData路径，fromExt=".heic"，toExt=".jpg"
 *           [业务验证] 返回路径中.heic被替换为.jpg
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SwapExtensionInPath_Test_002, TestSize.Level1)
{
    string path = "/storage/cloud/files/.editData/Photo/X/IMG_123_456.heic/editdata_camera";
    string result = MediaFileUtils::SwapExtensionInPath(path, ".heic", ".jpg");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/Photo/X/IMG_123_456.jpg/editdata_camera");
}

/**
 * @tc.name: SwapExtensionInPath_NoMatch
 * @tc.desc: 验证SwapExtensionInPath在路径不含目标扩展名时返回原路径不变
 *           [覆盖分支] rfind返回npos，不执行replace
 *           [触发条件] 输入含.png的路径，fromExt=".jpg"，toExt=".heic"
 *           [业务验证] 返回路径与输入完全一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SwapExtensionInPath_Test_003, TestSize.Level1)
{
    string path = "/storage/cloud/files/.editData/Photo/X/IMG_123_456.png/editdata_camera";
    string result = MediaFileUtils::SwapExtensionInPath(path, ".jpg", ".heic");
    EXPECT_EQ(result, path);
}

/**
 * @tc.name: SwapExtensionInPath_MultipleOccurrence
 * @tc.desc: 验证SwapExtensionInPath只替换最后一个匹配的扩展名(rfind而非find)
 *           [覆盖分支] rfind定位最后一个.jpg，前面的.jpg不被替换
 *           [触发条件] 输入含两个.jpg的路径(.jpg_thumbnail.jpg)
 *           [业务验证] 只有最后一个.jpg被替换为.heic
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SwapExtensionInPath_Test_004, TestSize.Level1)
{
    string path = "/data/test/IMG_123.jpg_thumbnail.jpg";
    string result = MediaFileUtils::SwapExtensionInPath(path, ".jpg", ".heic");
    EXPECT_EQ(result, "/data/test/IMG_123.jpg_thumbnail.heic");
}

/**
 * @tc.name: SwapExtensionInPath_EditDataSourceMp4
 * @tc.desc: 验证SwapExtensionInPath在editData source.mp4路径中只替换目录部分的扩展名
 *           [覆盖分支] rfind定位目录中的.jpg，source.mp4后缀不受影响
 *           [触发条件] 输入含.jpg/source.mp4的editData路径
 *           [业务验证] 返回.heic/source.mp4，.mp4部分不变
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_SwapExtensionInPath_Test_005, TestSize.Level1)
{
    string path = "/storage/cloud/files/.editData/Photo/X/IMG_123_456.jpg/source.mp4";
    string result = MediaFileUtils::SwapExtensionInPath(path, ".jpg", ".heic");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/Photo/X/IMG_123_456.heic/source.mp4");
}

/**
 * @tc.name: ReadStrFromFileWithExtRetry_FallbackJpgToHeic
 * @tc.desc: 验证并发rename后ReadStrFromFileWithExtRetry通过fallback成功读取文件内容
 *           [覆盖分支] 原路径(.heic)读取失败，fallback swap .heic->.jpg后读取成功
 *           [触发条件] 创建.jpg目录下文件写入内容，用.heic路径调用ReadStrFromFileWithExtRetry
 *           [业务验证] 函数返回true，读取内容与写入内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReadStrFromFileWithExtRetry_Test_001, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0001.jpg";
    string heicDir = "/data/test/IMG_0001.heic";
    string jpgPath = jpgDir + "/editdata_camera";
    string heicPath = heicDir + "/editdata_camera";
    string testContent = "test_edit_data_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(jpgPath), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(jpgPath, testContent), true);

    string fileContent;
    string inputPath = heicDir + "/editdata_camera";
    EXPECT_EQ(MediaFileUtils::ReadStrFromFileWithExtRetry(inputPath, fileContent), true);
    EXPECT_EQ(fileContent, testContent);

    MediaFileUtils::DeleteDir("/data/test/IMG_0001.jpg");
    MediaFileUtils::DeleteDir("/data/test/IMG_0001.heic");
}

/**
 * @tc.name: ReadStrFromFileWithExtRetry_FallbackHeicToJpg
 * @tc.desc: 验证反向并发rename场景下fallback成功读取
 *           [覆盖分支] 原路径(.jpg)读取失败，fallback swap .jpg->.heic后读取成功
 *           [触发条件] 创建.heic目录下文件写入内容，用.jpg路径调用ReadStrFromFileWithExtRetry
 *           [业务验证] 函数返回true，读取内容与写入内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReadStrFromFileWithExtRetry_Test_002, TestSize.Level1)
{
    string heicDir = "/data/test/IMG_0002.heic";
    string jpgDir = "/data/test/IMG_0002.jpg";
    string heicPath = heicDir + "/editdata_camera";
    string jpgPath = jpgDir + "/editdata_camera";
    string testContent = "test_edit_data_heic_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(heicPath), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(heicPath, testContent), true);

    string fileContent;
    string inputPath = jpgDir + "/editdata_camera";
    EXPECT_EQ(MediaFileUtils::ReadStrFromFileWithExtRetry(inputPath, fileContent), true);
    EXPECT_EQ(fileContent, testContent);

    MediaFileUtils::DeleteDir("/data/test/IMG_0002.heic");
    MediaFileUtils::DeleteDir("/data/test/IMG_0002.jpg");
}

/**
 * @tc.name: ReadStrFromFileWithExtRetry_BothFail
 * @tc.desc: 验证路径不含.jpg/.heic且文件不存在时两轮尝试均失败
 *           [覆盖分支] 原路径失败且fallback路径无扩展名可swap，最终返回false
 *           [触发条件] 输入不含.jpg/.heic的不存在路径
 *           [业务验证] 函数返回false
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReadStrFromFileWithExtRetry_Test_003, TestSize.Level1)
{
    string fileContent;
    string inputPath = "/data/test/extretry_read_nonexist/noext/editdata_camera";
    EXPECT_EQ(MediaFileUtils::ReadStrFromFileWithExtRetry(inputPath, fileContent), false);
}

/**
 * @tc.name: ReadStrFromFileWithExtRetry_NormalSuccess
 * @tc.desc: 验证正常路径下直接读取成功，不走fallback
 *           [覆盖分支] 原路径(.jpg)文件存在，ReadStrFromFile直接成功
 *           [触发条件] 创建.jpg文件并写入内容，用.jpg路径调用
 *           [业务验证] 函数返回true，内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReadStrFromFileWithExtRetry_Test_004, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0003.jpg";
    string jpgPath = jpgDir + "/editdata_camera";
    string testContent = "normal_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(jpgPath), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(jpgPath, testContent), true);

    string fileContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFileWithExtRetry(jpgPath, fileContent), true);
    EXPECT_EQ(fileContent, testContent);

    MediaFileUtils::DeleteDir("/data/test/IMG_0003.jpg");
}

/**
 * @tc.name: ReadStrFromFileWithExtRetry_NoRetryWhenFallbackPathNotExist
 * @tc.desc: 验证filePath不存在且fallbackPath也不存在时不触发ExtRetry fallback，
 *           IsFileExists(fallbackPath)前置条件避免误触发
 *           [覆盖分支] errno==ENOENT但IsFileExists(fallbackPath)为false，
 *           跳过fallback分支直接返回false
 *           [触发条件] filePath含.jpg不存在，fallbackPath含.heic也不存在
 *           [业务验证] 函数返回false，不进入fallback
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ReadStrFromFileWithExtRetry_Test_005, TestSize.Level1)
{
    string nonExistJpgPath = "/data/test/read_noretry_src.jpg/editdata_camera";

    string fileContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFileWithExtRetry(nonExistJpgPath, fileContent), false);
}

/**
 * @tc.name: CopyFileSafeWithExtRetry_FallbackNewPathHeicToJpg
 * @tc.desc: 验证并发rename后CopyFileSafeWithExtRetry自行处理fallback，完整CopyFileSafe流程成功
 *           [覆盖分支] retryExtOnSrcPath=false，errno==ENOENT且IsFileExists(filePath)为true，
 *           第一次CopyFileUtil+rename失败，fallback swap newPath .heic->.jpg后第二次完整流程成功
 *           [触发条件] 创建.jpg目录作为目标目录，用.heic newPath调用，retryExtOnSrcPath=false
 *           [业务验证] 复制成功，目标文件内容与源文件一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafeWithExtRetry_Test_001, TestSize.Level1)
{
    string srcDir = "/data/test/extretry_copysafe_src";
    string jpgDir = "/data/test/IMG_0004.jpg";
    string srcFile = srcDir + "/video.mp4";
    string jpgDstDir = jpgDir + "/source.mp4";
    string testContent = "safecopy_content_data";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);

    string heicDstDir = "/data/test/IMG_0004.heic/source.mp4";
    EXPECT_EQ(MediaFileUtils::CopyFileSafeWithExtRetry(srcFile, heicDstDir, false), true);
    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(jpgDstDir, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir(srcDir);
    MediaFileUtils::DeleteDir(jpgDir);
    MediaFileUtils::DeleteDir("/data/test/IMG_0004.heic");
}

/**
 * @tc.name: CopyFileSafeWithExtRetry_FallbackFilePathJpgToHeic
 * @tc.desc: 验证并发rename后CopyFileSafeWithExtRetry自行处理fallback，完整CopyFileSafe流程成功
 *           [覆盖分支] retryExtOnSrcPath=true，errno==ENOENT且IsFileExists(fallbackFilePath)为true，
 *           第一次CopyFileUtil失败(PathToRealPath(filePath)失败)，fallback swap filePath .jpg->.heic后成功
 *           [触发条件] 创建.heic源文件，用.jpg filePath调用，retryExtOnSrcPath=true
 *           [业务验证] 复制成功，目标文件内容与源文件一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafeWithExtRetry_Test_002, TestSize.Level1)
{
    string heicDir = "/data/test/IMG_0005.heic";
    string dstDir = "/data/test/extretry_copysafe_dst2";
    string heicFile = heicDir + "/video.mp4";
    string dstFile = dstDir + "/source.mp4";
    string testContent = "safecopy_heic_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(heicFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(heicFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dstDir), true);

    string jpgFile = "/data/test/IMG_0005.jpg/video.mp4";
    EXPECT_EQ(MediaFileUtils::CopyFileSafeWithExtRetry(jpgFile, dstFile, true), true);
    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(dstFile, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir(heicDir);
    MediaFileUtils::DeleteDir(dstDir);
    MediaFileUtils::DeleteDir("/data/test/IMG_0005.jpg");
}

/**
 * @tc.name: CopyFileSafeWithExtRetry_NormalSuccess
 * @tc.desc: 验证正常路径下CopyFileSafeWithExtRetry直接成功，不走fallback
 *           [覆盖分支] 原路径文件均存在，CopyFileSafe直接成功
 *           [触发条件] 创建有效的源文件和目标目录
 *           [业务验证] 复制成功，内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafeWithExtRetry_Test_003, TestSize.Level1)
{
    string srcFile = "/data/test/extretry_copysafe_normal_src/video.mp4";
    string dstFile = "/data/test/extretry_copysafe_normal_dst/source.mp4";
    string testContent = "safecopy_normal_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/data/test/extretry_copysafe_normal_src"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/data/test/extretry_copysafe_normal_dst"), true);

    EXPECT_EQ(MediaFileUtils::CopyFileSafeWithExtRetry(srcFile, dstFile, false), true);

    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(dstFile, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir("/data/test/extretry_copysafe_normal_src");
    MediaFileUtils::DeleteDir("/data/test/extretry_copysafe_normal_dst");
}

/**
 * @tc.name: CopyFileSafeWithExtRetry_NoRetryWhenSrcNotExist
 * @tc.desc: 验证filePath不存在时不触发ExtRetry fallback，IsFileExists(filePath)前置条件避免误触发
 *           [覆盖分支] retryExtOnSrcPath=false，errno==ENOENT但IsFileExists(filePath)为false，
 *           跳过fallback分支直接返回false
 *           [触发条件] filePath不存在，newPath含.heic且其父目录存在，调用CopyFileSafeWithExtRetry
 *           [业务验证] 函数返回false，不进入fallback
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafeWithExtRetry_Test_004, TestSize.Level1)
{
    string nonExistSrc = "/data/test/copysafe_noretry_src/video.mp4";
    string heicDstDir = "/data/test/copysafe_noretry_dst.heic";
    string heicDstFile = heicDstDir + "/source.mp4";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDstDir), true);

    EXPECT_EQ(MediaFileUtils::CopyFileSafeWithExtRetry(nonExistSrc, heicDstFile, false), false);

    MediaFileUtils::DeleteDir(heicDstDir);
}

/**
 * @tc.name: CopyFileSafeWithExtRetry_NoRetryWhenFallbackSrcNotExist
 * @tc.desc: 验证fallbackFilePath不存在时不触发ExtRetry fallback，
 *           IsFileExists(fallbackFilePath)前置条件避免retryExtOnSrcPath=true时误触发
 *           [覆盖分支] retryExtOnSrcPath=true，errno==ENOENT但IsFileExists(fallbackFilePath)为false，
 *           跳过fallback分支直接返回false
 *           [触发条件] filePath含.jpg不存在，fallbackFilePath含.heic也不存在，newPath有效
 *           [业务验证] 函数返回false，不进入fallback
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_CopyFileSafeWithExtRetry_Test_005, TestSize.Level1)
{
    string nonExistJpgSrc = "/data/test/copysafe_noretry_src.jpg/video.mp4";
    string dstDir = "/data/test/copysafe_noretry_dst3";
    string dstFile = dstDir + "/source.mp4";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(dstDir), true);

    EXPECT_EQ(MediaFileUtils::CopyFileSafeWithExtRetry(nonExistJpgSrc, dstFile, true), false);

    MediaFileUtils::DeleteDir(dstDir);
}

/**
 * @tc.name: MoveFileWithExtRetry_FallbackDestHeicToJpg
 * @tc.desc: 验证并发rename后MoveFileWithExtRetry通过fallback destPath成功移动文件
 *           [覆盖分支] 原destPath(.heic)父目录不存在导致errno==ENOENT，
 *           fallback swap destPath .heic->.jpg后MoveFile成功
 *           [触发条件] 创建.jpg目录作为目标目录，用.heic destPath调用MoveFileWithExtRetry
 *           [业务验证] 移动成功，目标文件内容与源文件一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFileWithExtRetry_Test_001, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0006.jpg";
    string srcDir = "/data/test/move_extretry_src";
    string srcFile = srcDir + "/video.mp4";
    string jpgDestFile = jpgDir + "/source.mp4";
    string testContent = "move_test_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);

    string heicDestFile = "/data/test/IMG_0006.heic/source.mp4";
    EXPECT_EQ(MediaFileUtils::MoveFileWithExtRetry(srcFile, heicDestFile), true);

    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(jpgDestFile, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir(srcDir);
    MediaFileUtils::DeleteDir(jpgDir);
    MediaFileUtils::DeleteDir("/data/test/IMG_0006.heic");
}

/**
 * @tc.name: MoveFileWithExtRetry_FallbackDestJpgToHeic
 * @tc.desc: 验证反向并发rename场景下fallback成功移动
 *           [覆盖分支] 原destPath(.jpg)父目录不存在导致errno==ENOENT，
 *           fallback swap destPath .jpg->.heic后MoveFile成功
 *           [触发条件] 创建.heic目录，用.jpg destPath调用MoveFileWithExtRetry
 *           [业务验证] 移动成功，内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFileWithExtRetry_Test_002, TestSize.Level1)
{
    string heicDir = "/data/test/IMG_0007.heic";
    string srcDir = "/data/test/move_extretry_src2";
    string srcFile = srcDir + "/video.mp4";
    string heicDestFile = heicDir + "/source.mp4";
    string testContent = "move_heic_test_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDir), true);

    string jpgDestFile = "/data/test/IMG_0007.jpg/source.mp4";
    EXPECT_EQ(MediaFileUtils::MoveFileWithExtRetry(srcFile, jpgDestFile), true);

    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(heicDestFile, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir(srcDir);
    MediaFileUtils::DeleteDir(heicDir);
    MediaFileUtils::DeleteDir("/data/test/IMG_0007.jpg");
}

/**
 * @tc.name: MoveFileWithExtRetry_NormalSuccess
 * @tc.desc: 验证正常路径下MoveFileWithExtRetry直接成功，不走fallback
 *           [覆盖分支] 原路径均有效，MoveFile直接成功，不触发ENOENT
 *           [触发条件] 创建有效源文件和目标目录
 *           [业务验证] 移动成功，内容一致
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFileWithExtRetry_Test_003, TestSize.Level1)
{
    string srcDir = "/data/test/move_extretry_normal_src";
    string srcFile = srcDir + "/video.mp4";
    string dstDir = "/data/test/move_extretry_normal_dst";
    string dstFile = dstDir + "/source.mp4";
    string testContent = "move_normal_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory(dstDir), true);

    EXPECT_EQ(MediaFileUtils::MoveFileWithExtRetry(srcFile, dstFile), true);

    string dstContent;
    EXPECT_EQ(MediaFileUtils::ReadStrFromFile(dstFile, dstContent), true);
    EXPECT_EQ(dstContent, testContent);

    MediaFileUtils::DeleteDir(srcDir);
    MediaFileUtils::DeleteDir(dstDir);
}

/**
 * @tc.name: MoveFileWithExtRetry_NoRetryWhenSrcNotExist
 * @tc.desc: 验证srcPath不存在时不触发ExtRetry fallback，避免误触发
 *           [覆盖分支] MoveFile因!IsFileExists(srcPath)返回false且errno==ENOENT，
 *           IsFileExists(srcPath)前置条件不满足，跳过fallback分支
 *           [触发条件] srcPath不存在，destPath父目录含.heic扩展名，调用MoveFileWithExtRetry
 *           [业务验证] 函数返回false，不进入fallback（无fallback日志）
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MoveFileWithExtRetry_Test_004, TestSize.Level1)
{
    string heicDstDir = "/data/test/move_noretry_heic_dst";
    string heicDstFile = heicDstDir + "/source.mp4";
    string nonExistSrcFile = "/data/test/move_noretry_nonexist_src/video.mp4";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDstDir), true);

    EXPECT_EQ(MediaFileUtils::MoveFileWithExtRetry(nonExistSrcFile, heicDstFile), false);

    MediaFileUtils::DeleteDir(heicDstDir);
}

/**
 * @tc.name: OpenFileWithExtRetry_FallbackHeicToJpg
 * @tc.desc: 验证并发rename后OpenFileWithExtRetry通过fallback成功打开文件
 *           [覆盖分支] 原filePath(.heic)PathToRealPath失败，
 *           fallback swap .heic->.jpg后PathToRealPath+open成功
 *           [触发条件] 创建.jpg目录下文件，用.heic路径调用OpenFileWithExtRetry
 *           [业务验证] 返回fd>=0，absFilePath指向.jpg版本的realpath
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_OpenFileWithExtRetry_Test_001, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0008.jpg";
    string jpgFile = jpgDir + "/source.mp4";
    string testContent = "open_test_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(jpgFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(jpgFile, testContent), true);

    string heicDir = "/data/test/IMG_0008.heic";
    string heicFile = heicDir + "/source.mp4";
    string absFilePath;
    int32_t fd = MediaFileUtils::OpenFileWithExtRetry(heicFile, absFilePath);
    EXPECT_GE(fd, 0);
    EXPECT_NE(absFilePath.find("IMG_0008.jpg"), string::npos);
    close(fd);

    MediaFileUtils::DeleteDir(jpgDir);
    MediaFileUtils::DeleteDir(heicDir);
}

/**
 * @tc.name: OpenFileWithExtRetry_FallbackJpgToHeic
 * @tc.desc: 验证反向并发rename场景下fallback成功打开
 *           [覆盖分支] 原filePath(.jpg)失败，fallback swap .jpg->.heic成功
 *           [触发条件] 创建.heic目录下文件，用.jpg路径调用
 *           [业务验证] 返回fd>=0，absFilePath指向.heic版本
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_OpenFileWithExtRetry_Test_002, TestSize.Level1)
{
    string heicDir = "/data/test/IMG_0009.heic";
    string heicFile = heicDir + "/source.mp4";
    string testContent = "open_heic_test_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(heicDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(heicFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(heicFile, testContent), true);

    string jpgDir = "/data/test/IMG_0009.jpg";
    string jpgFile = jpgDir + "/source.mp4";
    string absFilePath;
    int32_t fd = MediaFileUtils::OpenFileWithExtRetry(jpgFile, absFilePath);
    EXPECT_GE(fd, 0);
    EXPECT_NE(absFilePath.find("IMG_0009.heic"), string::npos);
    close(fd);

    MediaFileUtils::DeleteDir(heicDir);
    MediaFileUtils::DeleteDir(jpgDir);
}

/**
 * @tc.name: OpenFileWithExtRetry_BothFail
 * @tc.desc: 验证文件不存在时两轮尝试均失败，返回-1
 *           [覆盖分支] 原路径PathToRealPath失败，fallback路径也失败
 *           [触发条件] 输入不存在文件路径
 *           [业务验证] 返回fd=-1
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_OpenFileWithExtRetry_Test_003, TestSize.Level1)
{
    string absFilePath;
    int32_t fd = MediaFileUtils::OpenFileWithExtRetry("/data/test/nonexist_file.mp4", absFilePath);
    EXPECT_EQ(fd, -1);
}

/**
 * @tc.name: OpenFileWithExtRetry_NormalSuccess
 * @tc.desc: 验证正常路径下直接PathToRealPath+open成功，不走fallback
 *           [覆盖分支] 原路径PathToRealPath成功且open返回有效fd
 *           [触发条件] 创建有效文件并用正确路径调用
 *           [业务验证] 返回fd>=0，absFilePath为文件realpath
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_OpenFileWithExtRetry_Test_004, TestSize.Level1)
{
    string srcDir = "/data/test/open_extretry_normal";
    string srcFile = srcDir + "/source.mp4";
    string testContent = "open_normal_content";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(srcDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(srcFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(srcFile, testContent), true);

    string absFilePath;
    int32_t fd = MediaFileUtils::OpenFileWithExtRetry(srcFile, absFilePath);
    EXPECT_GE(fd, 0);
    EXPECT_NE(absFilePath.find("open_extretry_normal"), string::npos);
    close(fd);

    MediaFileUtils::DeleteDir(srcDir);
}

/**
 * @tc.name: OpenFileWithExtRetry_NoRetryWhenFallbackPathNotExist
 * @tc.desc: 验证filePath不存在且fallbackPath也不存在时不触发ExtRetry fallback，
 *           IsFileExists(fallbackPath)前置条件避免误触发
 *           [覆盖分支] errno==ENOENT但IsFileExists(fallbackPath)为false，
 *           跳过fallback分支直接返回fd=-1
 *           [触发条件] filePath含.jpg不存在，fallbackPath含.heic也不存在
 *           [业务验证] 函数返回-1，不进入fallback
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_OpenFileWithExtRetry_Test_005, TestSize.Level1)
{
    string nonExistJpgPath = "/data/test/open_noretry_src.jpg/source.mp4";

    string absFilePath;
    int32_t fd = MediaFileUtils::OpenFileWithExtRetry(nonExistJpgPath, absFilePath);
    EXPECT_EQ(fd, -1);
}

/**
 * @tc.name: IsFileExistsWithExtRetry_NormalExists
 * @tc.desc: 验证文件存在于原路径时直接返回true，不触发fallback
 *           [覆盖分支] 原路径IsFileExists返回true，直接返回true
 *           [触发条件] 创建.jpg目录下文件，用.jpg路径调用IsFileExistsWithExtRetry
 *           [业务验证] 函数返回true
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileExistsWithExtRetry_Test_001, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0012.jpg";
    string jpgFile = jpgDir + "/editdata_camera";
    string testContent = "isfileexists_normal";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(jpgFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(jpgFile, testContent), true);

    EXPECT_EQ(MediaFileUtils::IsFileExistsWithExtRetry(jpgFile), true);

    MediaFileUtils::DeleteDir(jpgDir);
}

/**
 * @tc.name: IsFileExistsWithExtRetry_FallbackExists
 * @tc.desc: 验证并发rename后fallback路径存在时返回true
 *           [覆盖分支] 原路径IsFileExists返回false，SwapExtensionInPath .heic→.jpg后IsFileExists返回true
 *           [触发条件] 创建.jpg目录下文件，用.heic路径调用IsFileExistsWithExtRetry
 *           [业务验证] 函数返回true（fallback成功）
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileExistsWithExtRetry_Test_002, TestSize.Level1)
{
    string jpgDir = "/data/test/IMG_0012.jpg";
    string jpgFile = jpgDir + "/editdata_camera";
    string heicDir = "/data/test/IMG_0012.heic";
    string heicFile = heicDir + "/editdata_camera";
    string testContent = "isfileexists_fallback";

    EXPECT_EQ(MediaFileUtils::CreateDirectory(jpgDir), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(jpgFile), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(jpgFile, testContent), true);

    EXPECT_EQ(MediaFileUtils::IsFileExistsWithExtRetry(heicFile), true);

    MediaFileUtils::DeleteDir(jpgDir);
}

/**
 * @tc.name: IsFileExistsWithExtRetry_BothNotExist
 * @tc.desc: 验证原路径和fallback路径均不存在时返回false
 *           [覆盖分支] 原路径IsFileExists返回false，SwapExtensionInPath .jpg→.heic后IsFileExists也返回false
 *           [触发条件] 路径含.jpg但文件不存在且.heic fallback也不存在
 *           [业务验证] 函数返回false
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileExistsWithExtRetry_Test_003, TestSize.Level1)
{
    string nonExistJpgFile = "/data/test/IMG_nonexist.jpg/editdata_camera";

    EXPECT_EQ(MediaFileUtils::IsFileExistsWithExtRetry(nonExistJpgFile), false);
}

/**
 * @tc.name: IsFileExistsWithExtRetry_NoExtension
 * @tc.desc: 验证路径不含.jpg/.heic时SwapExtensionInPath无法swap，返回false
 *           [覆盖分支] 路径不含.jpg/.heic，rfind均返回npos，fallbackPath为空，
 *           跳过fallback分支直接返回IsFileExists(原路径)=false
 *           [触发条件] 输入不含扩展名的路径
 *           [业务验证] 函数返回false，不触发fallback
 */
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsFileExistsWithExtRetry_Test_004, TestSize.Level1)
{
    string noExtPath = "/data/test/noext_dir/editdata_camera";

    EXPECT_EQ(MediaFileUtils::IsFileExistsWithExtRetry(noExtPath), false);
}
} // namespace Media
} // namespace OHOS