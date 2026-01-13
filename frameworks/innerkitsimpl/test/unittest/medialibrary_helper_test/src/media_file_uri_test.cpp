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

#include "image_type.h"
#include "medialibrary_helper_test.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "string_ex.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"
#include "medialibrary_errno.h"
#include "uri.h"
#include "media_audio_column.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static const string networkId_ = "1d3cb099659d53b3ee15faaab3c00a8ff983382ebc8b01aabde039ed084e167b";
static const string DEFAULT_EXTR_PATH = "/IMG_1690336600_001/test.jpg";

static const string OLD_URI_PRE = "file://media/video/1";
static const string NEW_URI_PRE = "file://media/Photo/2";
static const string OLD_URI = "file://media/video/1/thumbnail/1080/1920";
static const string NEW_URI = "file://media/Photo/2/VID_169_001/VID_2023.mp4/thumbnail/1080/1920";
static const string OLD_URI_NEW_VER = "file://media/video/1?operation=thumbnail&width=1080&height=1920&"
    "path=/storage/cloud/files/video/16/VID_1690336600_001.mp4";
static const string NEW_URI_NEW_VER = "file://media/Photo/2/VID_169_001/VID_2023.mp4?operation=thumbnail&width=1080&"
    "height=1920&path=/storage/cloud/files/Photo/16/VID_1690336600_001.mp4";
static const string OLD_URI_PATH = "/storage/cloud/files/video/16/VID_1690336600_001.mp4";
static const string NEW_URI_PATH = "/storage/cloud/files/Photo/16/VID_1690336600_001.mp4";
static const Size THUMB_NAIL_SIZE = Size{1080, 1920};
static const int32_t fd_ = 12;

const int32_t TYPE_URI = 1;
const int32_t TYPE_URI_FD = 2;
const int32_t TYPE_URI_PREFIX = 3;

string PathSplicing(string subpath, int32_t type, string extrPath = "")
{
    string Uri = "";
    switch (type) {
        case TYPE_URI:
            Uri = (ML_FILE_URI_PREFIX + subpath + "/" + to_string(fd_) + ML_URI_NETWORKID_EQUAL + networkId_ +
                extrPath);
            break;
        case TYPE_URI_FD:
            Uri = (ML_FILE_URI_PREFIX + subpath + "/" + to_string(fd_) + extrPath);
            break;
        case TYPE_URI_PREFIX:
            Uri = (subpath + to_string(fd_) + ML_URI_NETWORKID_EQUAL + networkId_ + extrPath);
            break;
        default:
            MEDIA_ERR_LOG("ERROR: No such type");
            break;
    }
    return Uri;
}

/**
 * Get the file uri prefix with id
 * eg. Input: file://media/Photo/10/IMG_xxx/01.jpg
 *     Output: file://media/Photo/10
 */
static void GetUriIdPrefix(std::string &fileUri)
{
    MediaFileUri mediaUri(fileUri);
    if (!mediaUri.IsApi10()) {
        return;
    }
    auto slashIdx = fileUri.rfind('/');
    if (slashIdx == std::string::npos) {
        return;
    }
    auto tmpUri = fileUri.substr(0, slashIdx);
    slashIdx = tmpUri.rfind('/');
    if (slashIdx == std::string::npos) {
        return;
    }
    fileUri = tmpUri.substr(0, slashIdx);
}

bool GetParamsFromUriTest(const string &uri, string &fileUri, const bool isOldVer, Size &size, string &path)
{
    MediaFileUri mediaUri(uri);
    if (!mediaUri.IsValid()) {
        return false;
    }
    if (isOldVer) {
        auto index = uri.find("thumbnail");
        if (index == string::npos) {
            return false;
        }
        fileUri = uri.substr(0, index - 1);
        GetUriIdPrefix(fileUri);
        index += strlen("thumbnail");
        index = uri.find('/', index);
        if (index == string::npos) {
            return false;
        }
        index += 1;
        auto tmpIdx = uri.find('/', index);
        if (tmpIdx == string::npos) {
            return false;
        }

        int32_t width = 0;
        StrToInt(uri.substr(index, tmpIdx - index), width);
        int32_t height = 0;
        StrToInt(uri.substr(tmpIdx + 1), height);
        size = { .width = width, .height = height };
    } else {
        auto qIdx = uri.find('?');
        if (qIdx == string::npos) {
            return false;
        }
        fileUri = uri.substr(0, qIdx);
        GetUriIdPrefix(fileUri);
        auto &queryKey = mediaUri.GetQueryKeys();
        if (queryKey.count(THUMBNAIL_PATH) != 0) {
            path = queryKey[THUMBNAIL_PATH];
        }
        if (queryKey.count(THUMBNAIL_WIDTH) != 0) {
            size.width = stoi(queryKey[THUMBNAIL_WIDTH]);
        }
        if (queryKey.count(THUMBNAIL_HEIGHT) != 0) {
            size.height = stoi(queryKey[THUMBNAIL_HEIGHT]);
        }
    }
    return true;
}

HWTEST_F(MediaLibraryHelperUnitTest, GetParamsFromUri_Test_001, TestSize.Level1)
{
    string path;
    string fileUri;
    Size size;
    GetParamsFromUriTest(OLD_URI, fileUri, true, size, path);
    EXPECT_EQ(OLD_URI_PRE, fileUri);
    EXPECT_EQ("", path);
    EXPECT_EQ(THUMB_NAIL_SIZE.height, size.height);
    EXPECT_EQ(THUMB_NAIL_SIZE.width, size.width);
}
HWTEST_F(MediaLibraryHelperUnitTest, GetParamsFromUri_Test_002, TestSize.Level1)
{
    string path;
    string fileUri;
    Size size;
    GetParamsFromUriTest(NEW_URI, fileUri, true, size, path);
    EXPECT_EQ(NEW_URI_PRE, fileUri);
    EXPECT_EQ("", path);
    EXPECT_EQ(THUMB_NAIL_SIZE.height, size.height);
    EXPECT_EQ(THUMB_NAIL_SIZE.width, size.width);
}

HWTEST_F(MediaLibraryHelperUnitTest, GetParamsFromUri_Test_003, TestSize.Level1)
{
    string path;
    string fileUri;
    Size size;
    GetParamsFromUriTest(OLD_URI_NEW_VER, fileUri, false, size, path);
    EXPECT_EQ(OLD_URI_PRE, fileUri);
    EXPECT_EQ(OLD_URI_PATH, path);
    EXPECT_EQ(THUMB_NAIL_SIZE.height, size.height);
    EXPECT_EQ(THUMB_NAIL_SIZE.width, size.width);
}
HWTEST_F(MediaLibraryHelperUnitTest, GetParamsFromUri_Test_004, TestSize.Level1)
{
    string path;
    string fileUri;
    Size size;
    GetParamsFromUriTest(NEW_URI_NEW_VER, fileUri, false, size, path);
    EXPECT_EQ(NEW_URI_PRE, fileUri);
    EXPECT_EQ(NEW_URI_PATH, path);
    EXPECT_EQ(THUMB_NAIL_SIZE.height, size.height);
    EXPECT_EQ(THUMB_NAIL_SIZE.width, size.width);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_001, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_AUDIO_URI, TYPE_URI);
    MediaFileUri fileUri(uri);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_002, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_IMAGE_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_IMAGE, to_string(fd_), networkId_);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_003, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_AUDIO_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_AUDIO, to_string(fd_), networkId_);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_004, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_VIDEO_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_VIDEO, to_string(fd_), networkId_);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_005, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_FILE_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_FILE, to_string(fd_), networkId_);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);

    MediaFileUri fileUri_(MEDIA_TYPE_ALBUM, to_string(fd_), networkId_);
    targetUri = fileUri_.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_006, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_IMAGE_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_IMAGE, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_007, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_AUDIO_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_AUDIO, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_008, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_VIDEO_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_VIDEO, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_009, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_FILE_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_FILE, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);

    MediaFileUri fileUri_(MEDIA_TYPE_MEDIA, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    targetUri = fileUri_.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_010, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_ALBUM_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_ALBUM, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_011, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_SMART_URI, TYPE_URI);
    MediaFileUri fileUri(MEDIA_TYPE_SMARTALBUM, to_string(fd_), networkId_, MEDIA_API_VERSION_DEFAULT);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_012, TestSize.Level1)
{
    string uri = PathSplicing(PhotoColumn::PHOTO_TYPE_URI, TYPE_URI, DEFAULT_EXTR_PATH);
    MediaFileUri fileUri(MEDIA_TYPE_IMAGE, to_string(fd_), networkId_, MEDIA_API_VERSION_V10, DEFAULT_EXTR_PATH);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);

    MediaFileUri fileUri_(MEDIA_TYPE_VIDEO, to_string(fd_), networkId_, MEDIA_API_VERSION_V10, DEFAULT_EXTR_PATH);
    targetUri = fileUri_.ToString();
    EXPECT_EQ(targetUri, uri);
}

    
HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_013, TestSize.Level1)
{
    string uri = PathSplicing(AudioColumn::AUDIO_URI_PREFIX, TYPE_URI_PREFIX, DEFAULT_EXTR_PATH);
    MediaFileUri fileUri(MEDIA_TYPE_AUDIO, to_string(fd_), networkId_, MEDIA_API_VERSION_V10, DEFAULT_EXTR_PATH);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_014, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_FILE_URI, TYPE_URI, DEFAULT_EXTR_PATH);
    MediaFileUri fileUri(MEDIA_TYPE_FILE, to_string(fd_), networkId_, MEDIA_API_VERSION_V10, DEFAULT_EXTR_PATH);
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);

    MediaFileUri fileUri_(MEDIA_TYPE_ALBUM, to_string(fd_), networkId_, MEDIA_API_VERSION_V10, DEFAULT_EXTR_PATH);
    targetUri = fileUri_.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_Test_015, TestSize.Level1)
{
    string uri = PathSplicing(MEDIALIBRARY_TYPE_FILE_URI, TYPE_URI_FD);
    MediaFileUri fileUri(MEDIA_TYPE_FILE, to_string(fd_));
    string targetUri = fileUri.ToString();
    EXPECT_EQ(targetUri, uri);

    MediaFileUri fileUri_(MEDIA_TYPE_ALBUM, to_string(fd_));
    targetUri = fileUri_.ToString();
    EXPECT_EQ(targetUri, uri);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test_001, TestSize.Level1)
{
    string uri = "/data/test/";
    EXPECT_EQ(MediaFileUri(uri).IsValid(), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test_002, TestSize.Level1)
{
    string uri = "datashare://data/test/";
    EXPECT_EQ(MediaFileUri(uri).IsValid(), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test_003, TestSize.Level1)
{
    string uri = "datashare://media/test/";
    EXPECT_EQ(MediaFileUri(uri).IsValid(), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test_004, TestSize.Level1)
{
    string uri = "datashare://media/test/6";
    EXPECT_EQ(MediaFileUri(uri).IsValid(), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_001, TestSize.Level1)
{
    string uri = "/data/test/";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_002, TestSize.Level1)
{
    string uri = "file://data/test/";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_003, TestSize.Level1)
{
    string uri = "datashare://data/test/";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "data");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_004, TestSize.Level1)
{
    string uri = "file://data/test/?networkid=123";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_005, TestSize.Level1)
{
    string uri = "file://data/test/?networkid=123&test=456";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetNetworkId_Test_006, TestSize.Level1)
{
    string uri = "file://data/test/?networkid=123&test=456/path/test/";
    EXPECT_EQ(MediaFileUri(uri).GetNetworkId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_001, TestSize.Level1)
{
    string uri = "/data/test/";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "-1");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_002, TestSize.Level1)
{
    string uri = "file://data/test/";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "-1");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_003, TestSize.Level1)
{
    string uri = "file://data/test/tt";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "-1");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_004, TestSize.Level1)
{
    string uri = "file://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_005, TestSize.Level1)
{
    string uri = "datashare://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFileId_Test_006, TestSize.Level1)
{
    string uri = "data://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFileId(), "123");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUri_GetFilePath_Test_001, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(MediaFileUri(uri).GetFilePath(), "");
    uri = "data://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFilePath(), "");
    uri = "file://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFilePath(), "");
    uri = "datashare://data/test/123";
    EXPECT_EQ(MediaFileUri(uri).GetFilePath(), "");
    uri = "file://data/test/tt";
    EXPECT_EQ(MediaFileUri(uri).GetFilePath(), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsApi10_Test_001, TestSize.Level1)
{
    string uri = "/data/test/";
    EXPECT_EQ(MediaFileUri(uri).IsApi10(), false);
    uri = "datashare://data/test/";
    EXPECT_EQ(MediaFileUri(uri).IsApi10(), false);
    uri = "datashare://media/test/";
    EXPECT_EQ(MediaFileUri(uri).IsApi10(), false);
    uri = "datashare://media/test/6";
    EXPECT_EQ(MediaFileUri(uri).IsApi10(), false);
    uri = "";
    EXPECT_EQ(MediaFileUri(uri).IsApi10(), false);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetQueryKeys_Test_001, TestSize.Level1)
{
    string uri = "";
    auto &queryKey = MediaFileUri(uri).GetQueryKeys();
    EXPECT_EQ(queryKey.count("operation"), 0);
    uri = "file://data/test/123";
    queryKey = MediaFileUri(uri).GetQueryKeys();
    EXPECT_EQ(queryKey.count("operation"), 0);
    uri = "datashare://media/test/";
    queryKey = MediaFileUri(uri).GetQueryKeys();
    EXPECT_GE(queryKey.count("operation"), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTableName_Test_001, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(MediaFileUri(uri).GetTableName(), "");

    uri = "file://data/test/testCase";
    EXPECT_EQ(MediaFileUri(uri).GetTableName(), "");

    uri = "datashare://media/test/";
    EXPECT_EQ(MediaFileUri(uri).GetTableName(), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeFromUri_Test_001, TestSize.Level1)
{
    string uri = "file://data/test";
    EXPECT_EQ(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = "file://data/test/testCase";
    EXPECT_EQ(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = "datashare://media/test/";
    EXPECT_EQ(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_RemoveAllFragment_Test_001, TestSize.Level1)
{
    string uri = "file://data/test/testCase/123";
    MediaFileUri::RemoveAllFragment(uri);
    EXPECT_EQ(uri, "file://data/test/testCase/123");

    uri = "datashare://media/test/#";
    MediaFileUri::RemoveAllFragment(uri);
    EXPECT_EQ(uri, "datashare://media/test/");

    uri = "#datashare://media/test/";
    MediaFileUri::RemoveAllFragment(uri);
    EXPECT_EQ(uri, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetUriType_Test_001, TestSize.Level1)
{
    string uri = "file://data/test/testCase/";
    MediaFileUri mediaFileUri(uri);
    auto ret = mediaFileUri.GetUriType();
    EXPECT_EQ(ret, MEDIA_TYPE_AUDIO);

    uri = "file://media/Photo/test";
    ret = mediaFileUri.GetUriType();
    EXPECT_EQ(ret, MEDIA_TYPE_AUDIO);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetPhotoId_Test_001, TestSize.Level1)
{
    string uri = "file://data/test/testCase/";
    EXPECT_EQ(MediaFileUri::GetPhotoId(uri), "");

    uri = "file://media/Photo/test";
    EXPECT_EQ(MediaFileUri::GetPhotoId(uri), "test");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetPathFirstDentry_Test_001, TestSize.Level1)
{
    Uri uri("file://data/test/testCase/");
    EXPECT_EQ(MediaFileUri::GetPathFirstDentry(uri), "");

    Uri uri1("file://media/photo_operation/query");
    EXPECT_EQ(MediaFileUri::GetPathFirstDentry(uri1), "photo_operation");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeUri_Test_001, TestSize.Level1)
{
    MediaType mediaType = MEDIA_TYPE_DEVICE;
    int32_t apiVersion = MEDIA_API_VERSION_DEFAULT;
    auto ret = MediaFileUri::GetMediaTypeUri(mediaType, apiVersion);
    EXPECT_EQ(ret, "/file");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeUri_Test_002, TestSize.Level1)
{
    MediaType mediaType = MEDIA_TYPE_FILE;
    int32_t apiVersion = MEDIA_API_VERSION_DEFAULT;
    auto ret = MediaFileUri::GetMediaTypeUri(mediaType, apiVersion);
    EXPECT_EQ(ret, "/file");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTimeIdFromUri_Test_001, TestSize.Level1)
{
    std::vector<std::string> uriBatch;
    std::vector<std::string> timeIdBatch;
    int32_t start = 0;
    int32_t count = 0;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    EXPECT_NE(uriBatch.size(), 2);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTimeIdFromUri_Test_002, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"", "&offset="};
    std::vector<std::string> timeIdBatch;
    int32_t start = 0;
    int32_t count = 0;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    EXPECT_EQ(uriBatch.size(), 2);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTimeIdFromUri_Test_003, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"&time_id=", "&time_id=1&offset=1"};
    std::vector<std::string> timeIdBatch;
    int32_t start = 0;
    int32_t count = 0;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    EXPECT_EQ(uriBatch.size(), 2);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_ParseUri_Test_001, TestSize.Level1)
{
    enum {
        API10_PHOTO_URI_TEST,
        API10_PHOTOALBUM_URI_TEST,
        API10_AUDIO_URI_TEST,
        API9_URI_TEST,
        API10_ANALYSISALBUM_URI_TEST,
    };
    std::string uri_test = "file://media/PhotoAlbum/";
    std::string uri_test2 = "file://media/AnalysisAlbum/";
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>("1");
    ASSERT_NE(mediaFileUri, nullptr);
    mediaFileUri->ParseUri(uri_test);
    EXPECT_EQ(mediaFileUri->uriType_, API10_PHOTOALBUM_URI_TEST);

    std::shared_ptr<MediaFileUri> mediaFileUri2 = std::make_shared<MediaFileUri>("3");
    ASSERT_NE(mediaFileUri2, nullptr);
    mediaFileUri->ParseUri(uri_test2);
    EXPECT_EQ(mediaFileUri->uriType_, API10_ANALYSISALBUM_URI_TEST);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_MediaFileUriConstruct_Test_001, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>("1");
    ASSERT_NE(mediaFileUri, nullptr);
    MediaType mediatype = MEDIA_TYPE_FILE;
    EXPECT_NE(mediaFileUri->MediaFileUriConstruct(mediatype, OLD_URI_PATH, OLD_URI_PATH, MEDIA_API_VERSION_V10,
        DEFAULT_EXTR_PATH), "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetNetworkId_Test_001, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>("1");
    ASSERT_NE(mediaFileUri, nullptr);
    mediaFileUri->networkId_ = "No empty";
    std::string test = mediaFileUri->GetNetworkId();
    EXPECT_NE(test, "empty");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetFilePath_Test_001, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>("1");
    ASSERT_NE(mediaFileUri, nullptr);
    mediaFileUri->networkId_ = "No empty";
    std::string test = mediaFileUri->GetFilePath();
    EXPECT_EQ(test, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test2_001, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>(OLD_URI_PRE);
    std::shared_ptr<MediaFileUri> mediaFileUri2 = std::make_shared<MediaFileUri>(NEW_URI_PRE);
    ASSERT_NE(mediaFileUri, nullptr);
    ASSERT_NE(mediaFileUri2, nullptr);
    mediaFileUri->networkId_ = "No empty";
    bool test = mediaFileUri->IsValid();
    EXPECT_EQ(test, true);

    test = mediaFileUri2->IsValid();
    EXPECT_EQ(test, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_IsValid_Test2_002, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>(OLD_URI);
    std::shared_ptr<MediaFileUri> mediaFileUri2 = std::make_shared<MediaFileUri>(NEW_URI);
    ASSERT_NE(mediaFileUri, nullptr);
    ASSERT_NE(mediaFileUri2, nullptr);
    mediaFileUri->networkId_ = "No empty";
    bool test = mediaFileUri->IsValid();
    EXPECT_EQ(test, true);

    test = mediaFileUri2->IsValid();
    EXPECT_EQ(test, true);

    mediaFileUri->ParseUri(OLD_URI_NEW_VER);
    test = mediaFileUri->IsValid();
    EXPECT_EQ(test, true);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetUriType_Test_002, TestSize.Level1)
{
    std::shared_ptr<MediaFileUri> mediaFileUri = std::make_shared<MediaFileUri>("1");
    ASSERT_NE(mediaFileUri, nullptr);
    int test = mediaFileUri->GetUriType();
    EXPECT_EQ(test, 3);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeFromUri_Test_002, TestSize.Level1)
{
    string uri = PhotoColumn::PHOTO_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = AudioColumn::AUDIO_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = "file://media/PhotoAlbum/";
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = AUDIO_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = VIDEO_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = IMAGE_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = ALBUM_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = FILE_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);

    uri = HIGHLIGHT_URI_PREFIX;
    EXPECT_NE(MediaFileUri::GetMediaTypeFromUri(uri), MEDIA_TYPE_DEFAULT);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTimeIdFromUri_Test2_001, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"123", "", "&offset="};
    std::vector<std::string> timeIdBatch;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch);
    EXPECT_EQ(uriBatch.size(), 3);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetTimeIdFromUri_Test2_002, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"&time_id=", "&time_id=1&offset=1"};
    std::vector<std::string> timeIdBatch;
    int32_t start = 0;
    int32_t count = 0;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    EXPECT_EQ(uriBatch.size(), 2);

    std::vector<std::string> uriBatch2 = {"", "&time_id=", "&time_id=1&offset=1"};
    std::vector<std::string> timeIdBatch2;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    EXPECT_EQ(uriBatch.size(), 2);
}

HWTEST_F(MediaLibraryHelperUnitTest, MediaFileUtils_GetMediaTypeUri_Test_003, TestSize.Level1)
{
    MediaType mediaType = MEDIA_TYPE_VIDEO;
    int32_t apiVersion = MEDIA_API_VERSION_DEFAULT;
    auto ret = MediaFileUri::GetMediaTypeUri(mediaType, apiVersion);
    EXPECT_EQ(ret, "/video");

    mediaType = MEDIA_TYPE_AUDIO;
    apiVersion = MEDIA_API_VERSION_DEFAULT;
    ret = MediaFileUri::GetMediaTypeUri(mediaType, apiVersion);
    EXPECT_EQ(ret, "/audio");

    mediaType = MEDIA_TYPE_IMAGE;
    apiVersion = MEDIA_API_VERSION_DEFAULT;
    ret = MediaFileUri::GetMediaTypeUri(mediaType, apiVersion);
    EXPECT_EQ(ret, "/image");
}
} // namespace Media
} // namespace OHOS