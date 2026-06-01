/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "media_container_types.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_scanner_test.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "mimetype_utils.h"
#include "scanner_utils.h"
#include "userfile_manager_types.h"
#include "medialibrary_errno.h"
#include "permission_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
shared_ptr<FileAsset> g_pictures = nullptr;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
void MediaLibraryScannerTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
}

void MediaLibraryScannerTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    PermissionUtils::ClearBundleInfoInCache();
}

// SetUp:Execute before each test case
void MediaLibraryScannerTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::InitRootDirs();
    g_pictures = MediaLibraryUnitTestUtils::GetRootAsset(TEST_PICTURES);
}

void MediaLibraryScannerTest::TearDown(void) {}


HWTEST_F(MediaLibraryScannerTest, medialib_IsExists_test_001, TestSize.Level0)
{
    string path = "";
    bool ret = ScannerUtils::IsExists(path);
    EXPECT_EQ(ret, false);
    path = "medialib_GetFileName_test_001";
    ret = ScannerUtils::IsExists(path);
    EXPECT_EQ(ret, false);

    path= "/storage/cloud/files/Pictures/IsExists_test_001.jpg";
    remove(path.c_str());
    EXPECT_EQ(MediaFileUtils::CreateAsset(path), E_OK);
    ret = ScannerUtils::IsExists(path);
    EXPECT_EQ(MediaFileUtils::DeleteFile(path), true);
    EXPECT_EQ(ret, true);
}


HWTEST_F(MediaLibraryScannerTest, medialib_GetFileNameFromUri_test_001, TestSize.Level0)
{
    string path = "";
    string ret = ScannerUtils::GetFileNameFromUri(path);
    EXPECT_EQ(ret, "");
    path = "medialib_GetFileName_test_001/test";
    ret = ScannerUtils::GetFileNameFromUri(path);
    EXPECT_EQ(ret, "test");
    path = "medialib_GetFileName_test_001";
    ret = ScannerUtils::GetFileNameFromUri(path);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetFileExtension_test_001, TestSize.Level0)
{
    string path = "";
    string ret = ScannerUtils::GetFileExtension(path);
    EXPECT_EQ(ret, "");
    path = "medialib_GetFileExtension_001.test";
    ret = ScannerUtils::GetFileExtension(path);
    EXPECT_EQ(ret, "test");
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetMediatypeFromMimetype_test_001, TestSize.Level0)
{
    string path = "text/html";
    MediaType mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(path);
    EXPECT_EQ(mediaType, MEDIA_TYPE_FILE);
    path = "audio/mp3";
    mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(path);
    EXPECT_EQ(mediaType, MEDIA_TYPE_AUDIO);
    path = "video/mp4";
    mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(path);
    EXPECT_EQ(mediaType, MEDIA_TYPE_VIDEO);
    path = "image/jpeg";
    mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(path);
    EXPECT_EQ(mediaType, MEDIA_TYPE_IMAGE);
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetMimeTypeFromExtension_test_001, TestSize.Level0)
{
    string extension = "html";
    string mediaType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    EXPECT_EQ(mediaType, "text/html");
    extension = "medialib_GetMimeTypeFromExtension_test";
    mediaType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    EXPECT_EQ(mediaType, "application/octet-stream");
    extension = AUDIO_CONTAINER_TYPE_WAV;
    mediaType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    EXPECT_EQ(mediaType, "audio/wav");
}

HWTEST_F(MediaLibraryScannerTest, medialib_IsDirectory_test_001, TestSize.Level0)
{
    std::string path = "";
    bool ret = ScannerUtils::IsDirectory(path);
    EXPECT_EQ(ret, false);
    path = "medialib_IsDirectory_test_001";
    ret = ScannerUtils::IsDirectory(path);
    EXPECT_EQ(ret, false);
    path = "/storage/media";
    ret = ScannerUtils::IsDirectory(path);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryScannerTest, medialib_IsRegularFile_test_001, TestSize.Level0)
{
    std::string path = "";
    bool ret = ScannerUtils::IsRegularFile(path);
    EXPECT_EQ(ret, false);
    path = "medialib_IsRegularFile_test_001";
    ret = ScannerUtils::IsRegularFile(path);
    EXPECT_EQ(ret, false);

    path = "/storage/cloud/files/Pictures/IsRegularFile_test_001.jpg";
    remove(path.c_str());
    EXPECT_EQ(MediaFileUtils::CreateAsset(path), E_OK);
    ret = ScannerUtils::IsRegularFile(path);
    EXPECT_EQ(MediaFileUtils::DeleteFile(path), true);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryScannerTest, medialib_IsFileHidden_test_001, TestSize.Level0)
{
    std::string path = "";
    bool ret = ScannerUtils::IsFileHidden(path);
    EXPECT_EQ(ret, false);
    path = "medialib_IsFileHidden_test_001/.test";
    ret = ScannerUtils::IsFileHidden(path);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetParentPath_test_001, TestSize.Level0)
{
    string path = "";
    string ret = ScannerUtils::GetParentPath(path);
    EXPECT_EQ(ret, "");
    path = "medialib_GetParentPath_test_001/test";
    ret = ScannerUtils::GetParentPath(path);
    EXPECT_EQ(ret, "medialib_GetParentPath_test_001");
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetRootMediaDir_test_001, TestSize.Level0)
{
    std::string path = "";
    ScannerUtils::GetRootMediaDir(path);
    EXPECT_EQ(path, ROOT_MEDIA_DIR);
}

HWTEST_F(MediaLibraryScannerTest, medialib_GetFileTitle_test_001, TestSize.Level0)
{
    std::string path = "medialib_GetFileTitle_test_001";
    string ret = ScannerUtils::GetFileTitle(path);
    EXPECT_EQ(path, ret);
}

HWTEST_F(MediaLibraryScannerTest, medialib_IsDirHidden_test_001, TestSize.Level0)
{
    string path = "medialib_IsDirHidden_test_001/.test";
    bool ret = ScannerUtils::IsDirHidden(path);
    EXPECT_EQ(ret, true);
    path = "";
    ret = ScannerUtils::IsDirHidden(path);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryScannerTest, medialib_IsDirHiddenRecursive_test_001, TestSize.Level0)
{
    std::string path = "medialib_IsDirHiddenRecursive_test_001/.test";
    bool ret = ScannerUtils::IsDirHiddenRecursive(path);
    EXPECT_EQ(ret, true);
    path = "";
    ret = ScannerUtils::IsDirHiddenRecursive(path);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryScannerTest, medialib_CheckSkipScanList_test_001, TestSize.Level0)
{
    std::string path = "medialib_test";
    bool ret = ScannerUtils::CheckSkipScanList(path);
    EXPECT_EQ(ret, false);
    path = "medialib_CheckSkipScanList_test_001";
    ret = ScannerUtils::CheckSkipScanList(path);
    EXPECT_EQ(ret, true);
}

} // namespace Media
} // namespace OHOS