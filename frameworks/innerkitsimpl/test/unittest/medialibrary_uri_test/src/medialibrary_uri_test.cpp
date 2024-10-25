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
#define MLOG_TAG "FileExtUnitTest"
#include "medialibrary_uri_test.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "thumbnail_const.h"
#define private public
#include "thumbnail_uri_utils.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
const std::string TEST_STRING = "GetDateAddedFromUri&";

void MediaLibraryUriTest::SetUpTestCase(void) {}

void MediaLibraryUriTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryUriTest::SetUp() {}

void MediaLibraryUriTest::TearDown(void) {}

HWTEST_F(MediaLibraryUriTest, medialib_ParseFileUri_test_001, TestSize.Level0)
{
    string uriString = "";
    string outFileId = "";
    string outNetworkId = "";
    string outTableName = "";
    bool ret = ThumbnailUriUtils::ParseFileUri(uriString, outFileId, outNetworkId, outTableName);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryUriTest, medialib_ParseThumbnailInfo_test_001, TestSize.Level0)
{
    string uriString;
    string outFileId;
    Size outSize;
    string outNetworkId;
    string outTableName;
    bool ret = ThumbnailUriUtils::ParseThumbnailInfo(uriString, outFileId, outSize,
        outNetworkId, outTableName);
    EXPECT_EQ(ret, false);
    uriString = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_THUMBNAIL;
    ret = ThumbnailUriUtils::ParseThumbnailInfo(uriString, outFileId, outSize,
        outNetworkId, outTableName);
    EXPECT_EQ(ret, true);
    uriString = "ParseThumbnailInfo?=" + MEDIA_DATA_DB_THUMBNAIL + "&" + THUMBNAIL_WIDTH + "=&" +
        THUMBNAIL_HEIGHT + "=";
    ret = ThumbnailUriUtils::ParseThumbnailInfo(uriString, outFileId, outSize,
        outNetworkId, outTableName);
    EXPECT_EQ(ret, false);
    uriString = "ParseThumbnailInfo?test=" + MEDIA_DATA_DB_THUMBNAIL + "&" + THUMBNAIL_WIDTH + "=&" +
        THUMBNAIL_HEIGHT + "=";
    ret = ThumbnailUriUtils::ParseThumbnailInfo(uriString, outFileId, outSize,
        outNetworkId, outTableName);
    EXPECT_EQ(ret, false);
    uriString = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    ret = ThumbnailUriUtils::ParseThumbnailInfo(uriString, outFileId, outSize,
        outNetworkId, outTableName);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryUriTest, medialib_GetDateAddedFromUri_test_001, TestSize.Level0)
{
    string testDateAdded = "0001";
    string uriString = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_THUMBNAIL;
    string output = ThumbnailUriUtils::GetDateAddedFromUri(uriString);
    EXPECT_EQ(output, "");

    uriString = TEST_STRING + ML_URI_DATE_ADDED + testDateAdded;
    output = ThumbnailUriUtils::GetDateAddedFromUri(uriString);
    EXPECT_EQ(output, "");

    uriString = TEST_STRING + THUMBNAIL_OPERN_KEYWORD + "=" + testDateAdded;
    output = ThumbnailUriUtils::GetDateAddedFromUri(uriString);
    EXPECT_EQ(output, "");

    uriString = TEST_STRING + ML_URI_DATE_ADDED + "=" + testDateAdded;
    output = ThumbnailUriUtils::GetDateAddedFromUri(uriString);
    EXPECT_EQ(output, testDateAdded);
}

HWTEST_F(MediaLibraryUriTest, medialib_GetFileUriFromUri_test_001, TestSize.Level0)
{
    string testFileUri = "file://media/Photo/5664/IMG_1705635971_5359";
    string uriString = testFileUri + "&" + THUMBNAIL_OPERN_KEYWORD;
    string output = ThumbnailUriUtils::GetFileUriFromUri(uriString);
    EXPECT_EQ(output, "");

    uriString = testFileUri;
    output = ThumbnailUriUtils::GetFileUriFromUri(uriString);
    EXPECT_EQ(output, "");

    uriString = testFileUri + "?" + THUMBNAIL_OPERN_KEYWORD;
    output = ThumbnailUriUtils::GetFileUriFromUri(uriString);
    EXPECT_EQ(output, testFileUri);
}
} // namespace Media
} // namespace OHOS