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

#include "media_clear_invalid_user_comment_task_test.h"

#define private public
#include "media_clear_invalid_user_comment_task.h"
#undef private
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace OHOS::Media::Background;

void MediaClearInvalidUserCommentTaskTest::SetUpTestCase(void) {}

void MediaClearInvalidUserCommentTaskTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaClearInvalidUserCommentTaskTest::SetUp() {}

void MediaClearInvalidUserCommentTaskTest::TearDown(void) {}

HWTEST_F(MediaClearInvalidUserCommentTaskTest, ClearInvalidUserComment_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ClearInvalidUserComment_test_001");
    MediaClearInvalidUserCommentTask task;
    auto ret = task.ClearInvalidUserComment();
    EXPECT_TRUE(ret);
    MEDIA_INFO_LOG("ClearInvalidUserComment_test_001 end");
}

HWTEST_F(MediaClearInvalidUserCommentTaskTest, GetLongUserCommentCount_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetLongUserCommentCount_test_001");
    MediaClearInvalidUserCommentTask task;
    auto count = task.GetLongUserCommentCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetLongUserCommentCount_test_001 end");
}

HWTEST_F(MediaClearInvalidUserCommentTaskTest, UpdateLongUserCommentsToEmpty_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateLongUserCommentsToEmpty_test_001");
    MediaClearInvalidUserCommentTask task;
    auto ret = task.UpdateLongUserCommentsToEmpty();
    EXPECT_FALSE(ret);
    MEDIA_INFO_LOG("UpdateLongUserCommentsToEmpty_test_001 end");
}

} // namespace Media
} // namespace OHOS