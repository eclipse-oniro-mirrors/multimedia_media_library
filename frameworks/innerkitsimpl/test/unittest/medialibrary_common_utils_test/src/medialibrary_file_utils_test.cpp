/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "medialibrary_common_utils_test.h"
#include "medialibrary_file_utils_test.h"
#include "medialibrary_errno.h"
#include "thumbnail_utils.h"
#include "media_file_utils.h"
#include <fstream>
#include <sstream>
#include "medialibrary_common_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaFileUtilsTest::SetUpTestCase(void) {}

void MediaFileUtilsTest::TearDownTestCase(void) {}

void MediaFileUtilsTest::SetUp() {}

void MediaFileUtilsTest::TearDown(void) {}

static bool CheckFileString(const string &filePath, const string &text)
{
    ifstream file(filePath, ios::in);
    if (!file.is_open()) {
        return false;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    return content == text;
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_CheckAlbumNameCharacter_Test_001, TestSize.Level1)
{
    string regexRule = R"([\\/:*?"'`<>|{}\[\]])";
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("GoodName", regexRule), E_OK);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("Album_2023", regexRule), E_OK);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("", regexRule), E_OK);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("Bad/Name", regexRule), -EINVAL);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("a*b", regexRule), -EINVAL);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("name:1", regexRule), -EINVAL);
    EXPECT_EQ(MediaFileUtils::CheckAlbumNameCharacter("q?x", regexRule), -EINVAL);
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_SegmentedCopyFileUtile_Test_001, TestSize.Level1)
{
    string src = "/data/test/segcopy_001_src";
    string dst = "/data/test/segcopy_001_dst";
    string content = "segment copy content for ut";
    EXPECT_EQ(MediaFileUtils::CreateFile(src), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(src, content), true);

    auto cb = [](uint64_t) {};
    int32_t ret = MediaFileUtils::SegmentedCopyFileUtile(src, dst, cb, "req_seg_001");
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaFileUtils::IsFileExists(dst), true);
    EXPECT_EQ(CheckFileString(dst, content), true);

    EXPECT_EQ(MediaFileUtils::DeleteFile(src), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(dst), true);
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_SegmentedCopyFileUtile_Test_003, TestSize.Level1)
{
    string src = "/data/test/segcopy_not_exist_003_src";
    string dst = "/data/test/segcopy_not_exist_003_dst";
    auto cb = [](uint64_t) {};
    int32_t ret = MediaFileUtils::SegmentedCopyFileUtile(src, dst, cb, "req_seg_003");
    EXPECT_EQ(ret, E_INNER_FAIL);
    EXPECT_EQ(MediaFileUtils::IsFileExists(dst), false);
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_CloneToAlbumCancel_Test_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CloneToAlbumCancel("req_cancel_001"), E_OK);
    EXPECT_EQ(MediaFileUtils::CheckCancelCopy("req_cancel_001"), true);
    EXPECT_EQ(MediaFileUtils::CheckCancelCopy("req_cancel_001"), false);
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_CheckCancelCopy_Test_002, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CheckCancelCopy("req_cancel_not_exist"), false);
    EXPECT_EQ(MediaFileUtils::CheckCancelCopy(""), false);
}

HWTEST_F(MediaFileUtilsTest, MediaFileUtils_SegmentedCopyFileUtile_Cancel_Test_002, TestSize.Level1)
{
    string src = "/data/test/segcopy_cancel_002_src";
    string dst = "/data/test/segcopy_cancel_002_dst";
    EXPECT_EQ(MediaFileUtils::CreateFile(src), true);
    EXPECT_EQ(MediaFileUtils::WriteStrToFile(src, "to be cancelled"), true);

    EXPECT_EQ(MediaFileUtils::CloneToAlbumCancel("req_cancel_002"), E_OK);
    auto cb = [](uint64_t) {};
    int32_t ret = MediaFileUtils::SegmentedCopyFileUtile(src, dst, cb, "req_cancel_002");
    EXPECT_EQ(ret, E_SCENE_HAS_CANCEL);
    EXPECT_EQ(MediaFileUtils::IsFileExists(dst), false);

    EXPECT_EQ(MediaFileUtils::DeleteFile(src), true);
}
}  // namespace Media
}  // namespace OHOS
