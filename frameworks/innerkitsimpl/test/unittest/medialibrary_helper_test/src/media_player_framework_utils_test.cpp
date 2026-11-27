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

#include "media_player_framework_utils_test.h"
#include "media_pure_file_utils.h"

#include "media_file_utils.h"
#include "media_player_framework_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaPlayerFrameWorkUtilsTest::SetUpTestCase(void) {}
void MediaPlayerFrameWorkUtilsTest::TearDownTestCase(void) {}
void MediaPlayerFrameWorkUtilsTest::SetUp(void) {}
void MediaPlayerFrameWorkUtilsTest::TearDown(void) {}

static const string VIDEO_PATH = "/data/local/tmp/test_not_exist.mp4";

HWTEST_F(MediaPlayerFrameWorkUtilsTest, GetAVMetadataHelper_Test_001, TestSize.Level1)
{
    string path = VIDEO_PATH;
    MEDIA_INFO_LOG("file %{public}s exist: %{public}d", path.c_str(), MediaPureFileUtils::IsFileExists(path));
    auto ret = MediaPlayerFrameWorkUtils::GetAVMetadataHelper(path, AV_META_USAGE_META_ONLY);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaPlayerFrameWorkUtilsTest, GetExifRotate_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExifRotate_Test_001");
    string path = VIDEO_PATH;
    MEDIA_INFO_LOG("file %{public}s exist: %{public}d", path.c_str(), MediaPureFileUtils::IsFileExists(path));
    int32_t exifRotate;
    auto ret = MediaPlayerFrameWorkUtils::GetExifRotate(path, exifRotate);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetExifRotate_Test_001 end");
}

} // namespace Media
} // namespace OHOS