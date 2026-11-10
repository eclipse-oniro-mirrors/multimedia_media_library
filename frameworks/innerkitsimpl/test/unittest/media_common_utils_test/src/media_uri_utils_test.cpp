/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_uri_utils_test.h"

#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void MediaUriUtilsUnitTest::SetUpTestCase(void) {}

void MediaUriUtilsUnitTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaUriUtilsUnitTest::SetUp() {}

void MediaUriUtilsUnitTest::TearDown(void) {}

HWTEST_F(MediaUriUtilsUnitTest, medialib_append_key_value_test_001, TestSize.Level1)
{
    std::string uri;
    std::string key;
    std::string value;
    MediaUriUtils::AppendKeyValue(uri, key, value);
    EXPECT_EQ((uri == ""), false);
    EXPECT_EQ((key == ""), true);
    EXPECT_EQ((value == ""), true);

    uri = "datashare://test";
    key = "test";
    value = "testFile";
    MediaUriUtils::AppendKeyValue(uri, key, value);
    EXPECT_EQ((uri == ""), false);
    EXPECT_EQ((key == ""), false);
    EXPECT_EQ((value == ""), false);
    EXPECT_EQ((uri == "datashare://test?test=testFile"), true);
}

HWTEST_F(MediaUriUtilsUnitTest, medialib_create_asset_bucket_test_001, TestSize.Level1)
{
    int32_t bucketNum = -1;
    MediaUriUtils::CreateAssetBucket(-1, bucketNum);
    EXPECT_EQ(bucketNum, -1);
    MediaUriUtils::CreateAssetBucket(0, bucketNum);
    EXPECT_EQ(bucketNum, 16);
}

HWTEST_F(MediaUriUtilsUnitTest, medialib_get_uri_test_001, TestSize.Level1)
{
    std::string prefix1;
    std::string fileId1;
    std::string suffix1;
    std::string str1 = MediaUriUtils::GetUriByExtrConditions(prefix1, fileId1, suffix1);
    EXPECT_EQ(str1, "");

    std::string prefix2 = "test";
    std::string fileId2 = "1234567890";
    std::string suffix2 = "file";
    std::string str2 = MediaUriUtils::GetUriByExtrConditions(prefix2, fileId2, suffix2);
    EXPECT_EQ(str2, "test1234567890file");
}

HWTEST_F(MediaUriUtilsUnitTest, medialib_get_path_from_uri_test_001, TestSize.Level1)
{
    std::string fullUri = "file://media/Photo/1/IMG_123456789_000/test.jpg";
    std::string str1 = MediaUriUtils::GetPathFromUri(fullUri);
    EXPECT_EQ(str1, "");

    std::string uri;
    std::string str2 = MediaUriUtils::GetPathFromUri(uri);
    EXPECT_EQ(str2, "");
}

HWTEST_F(MediaUriUtilsUnitTest, medialib_get_file_id_test_001, TestSize.Level1)
{
    std::string fullUri = "file://media/Photo/1/IMG_123456789_000/test.jpg";
    int32_t id = MediaUriUtils::GetFileId(fullUri);
    EXPECT_EQ(id, 1);

    std::string uri;
    int32_t id2 = MediaUriUtils::GetFileId(uri);
    EXPECT_EQ(id2, -1);
}

/**
 * @tc.name: CheckUri_test01
 * @tc.desc: 有效的uri
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test01");
    std::string validUri = "file://media/Photo/1/IMG_123456789_000/test.jpg";
    EXPECT_EQ(MediaUriUtils::CheckUri(validUri), true);
    MEDIA_INFO_LOG("end CheckUri_test01");
}

/**
 * @tc.name: CheckUri_test02
 * @tc.desc: 包含"../"的URI
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test02");
    std::string invalidUri = "file://media/Photo/1/../path";
    EXPECT_EQ(MediaUriUtils::CheckUri(invalidUri), false);
    MEDIA_INFO_LOG("end CheckUri_test02");
}

/**
 * @tc.name: CheckUri_test03
 * @tc.desc: 不以"file://media"开头的URI
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test03");
    std::string invalidUri = "file://other/path";
    EXPECT_EQ(MediaUriUtils::CheckUri(invalidUri), false);
    MEDIA_INFO_LOG("end CheckUri_test03");
}

/**
 * @tc.name: CheckUri_test04
 * @tc.desc:  空字符串
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test04");
    std::string emptyUri = "";
    EXPECT_EQ(MediaUriUtils::CheckUri(emptyUri), false);
    MEDIA_INFO_LOG("end CheckUri_test04");
}

/**
 * @tc.name: CheckUri_test05
 * @tc.desc: 只有前缀没有路径
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test05, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test05");
    std::string uri = "file://media";
    EXPECT_EQ(MediaUriUtils::CheckUri(uri), true);
    MEDIA_INFO_LOG("end CheckUri_test05");
}

/**
 * @tc.name: CheckUri_test06
 * @tc.desc: 大小写敏感测试
 */
HWTEST_F(MediaUriUtilsUnitTest, CheckUri_test06, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter CheckUri_test06");
    std::string uri = "FILE://MEDIA/some/path";
    EXPECT_EQ(MediaUriUtils::CheckUri(uri), false);
    MEDIA_INFO_LOG("end CheckUri_test06");
}

HWTEST_F(MediaUriUtilsUnitTest, medialib_get_file_id_string_test_001, TestSize.Level1)
{
    std::string fullUri = "file://media/Photo/1/IMG_123456789_000/test.jpg";
    std::string id1 = MediaUriUtils::GetFileIdStr(fullUri);
    EXPECT_EQ(id1, "1");

    std::string uriNull;
    std::string id2 = MediaUriUtils::GetFileIdStr(uriNull);
    EXPECT_EQ(id2, "-1");

    std::string uriPhoto = "file://media/Photo";
    std::string id3 = MediaUriUtils::GetFileIdStr(uriPhoto);
    EXPECT_EQ(id3, "-1");
}

// IsLibraryUri 的四个判定条件统一用一张表覆盖，避免拆成多个结构完全相同的用例：
// 条件1 不含 ../；条件2 scheme 只允许 file/datashare；
// 条件3 fileId 必须位于类型段之后且为整数（相册等无类型段的 uri 取末段），不为表示无效的 -1；
// 条件4 authority 为 media 或 path 以 /media 开头。
HWTEST_F(MediaUriUtilsUnitTest, IsLibraryUri_JudgeLibraryUri_001, TestSize.Level1)
{
    struct LibraryUriCase {
        const char *uri;
        bool expectLibrary;
    };
    const LibraryUriCase cases[] = {
        // 条件1：路径穿越一律拒绝
        {"datashare:///media/Photo/../12", false},
        {"file://media/Photo/12/../13/IMG.jpg", false},
        // 条件2：scheme 只允许 file / datashare，普通路径、http 与空串均拒绝
        {"http://media/Photo/12", false},
        {"/data/storage/el2/media/Photo/12", false},
        {"", false},
        // 条件3：fileId 必须是整数且不为 -1
        {"file://media/Photo/abc", false},
        {"file://media/Photo/12a", false},
        {"file://media/Photo/-1", false},
        // 条件3：fileId 为空
        {"file://media/Photo/", false},
        // 条件3：末尾斜杠导致 fileId 为空
        {"file://media/Photo/12/", false},
        // 条件3：fileId 固定位于类型段之后第 1 段，仅 fileId、三段与四段形态都能取到
        {"file://media/Photo/0/-1", true},
        {"file://media/Photo/12/IMG_20251010_000000.jpg", true},
        {"file://media/Audio/12/ring.mp3", true},
        {"file://media/Audio/12/VID_169_001/VID_2023.mp3", true},
        {"file://media/hiddenPhoto/12/IMG.jpg", true},
        {"file://media/trashedPhoto/12/IMG.jpg", true},
        // 条件3：highlight 的 fileId 位于 /highlight/video|cover/ 之后
        {"file://media/highlight/video/12/a.mp4", true},
        {"file://media/highlight/cover/12/a.jpg", true},
        {"file://media/highlight/12/cover.jpg", false},
        // 条件3：无 path 分隔符、只有 /Photo 一段时取不到 fileId
        {"file://media", false},
        {"file://media/Photo", false},
        // 条件3：没有类型段时末段即 fileId，非数字则拒绝
        {"file://media/12/abc", false},
        // 条件4：authority 不是 media 且 path 不以 /media 开头
        {"file://other/13", false},
        {"datashare:///other/13", false},
        // 合法形态：仅 fileId、带标题与文件名、相册、无类型段与 datashare 前缀都要判为媒体库 uri
        {"file://media/Photo/12", true},
        {"datashare:///media/Photo/12", true},
        {"file://media/Photo/12/IMG_20251010_000000/IMG_20251010_000000.jpg", true},
        {"file://media/PhotoAlbum/12", true},
        {"file://media/12", true},
        {"datashare:///media/13", true},
    };
    for (const auto &item : cases) {
        SCOPED_TRACE(item.uri);
        EXPECT_EQ(MediaUriUtils::IsLibraryUri(item.uri), item.expectLibrary) << "uri: " << item.uri;
    }
}
} // namespace Media
} // namespace OHOS