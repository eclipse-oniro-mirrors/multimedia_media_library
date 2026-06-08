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

#include "media_file_management_test.h"

#include <fstream>
#include <string>
#include <memory>

#include "file_move_handle.h"
#include "media_progress_change_info.h"

using namespace testing::ext;
using namespace OHOS::Media::Notification;

namespace OHOS {
namespace Media {

void MediaFileMoveHandleTest::SetUpTestCase(void) {}
void MediaFileMoveHandleTest::TearDownTestCase(void) {}
void MediaFileMoveHandleTest::SetUp() {}
void MediaFileMoveHandleTest::TearDown() {}

/*
 * Feature : FileMoveHandle
 * Function : Destructor
 * SubFunction : NA
 * FunctionPoints : 验证progressTimerId_为0时不调用EndProgressTimer
 * EnvContions : NA
 * CaseDescription : 析构时progressTimerId_==0，不进入if分支，安全析构
 */
HWTEST_F(MediaFileMoveHandleTest, Destructor_NoTimer_001, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "1";
    changeInfo->totalSize = 1000;
    {
        FileMoveHandle handle(changeInfo, "test_timer_no_timer");
        // progressTimerId_ defaults to 0, so destructor won't call EndProgressTimer
        EXPECT_EQ(handle.progressTimerId_, static_cast<uint32_t>(0));
    }
    // If we reach here without crash, the test passes
    EXPECT_TRUE(true);
}

/*
 * Feature : FileMoveHandle
 * Function : OnMoveProgressTimer
 * SubFunction : NA
 * FunctionPoints : 验证requestId无效时返回E_ERR
 * EnvContions : NA
 * CaseDescription : requestId <= 0时CHECK_AND_RETURN_RET_LOG返回E_ERR
 */
HWTEST_F(MediaFileMoveHandleTest, OnMoveProgressTimer_InvalidRequestId_001, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "0"; // invalid
    FileMoveHandle handle(changeInfo, "test_timer_invalid_req");
    int32_t ret = handle.OnMoveProgressTimer();
    EXPECT_EQ(ret, E_ERR);
}

/*
 * Feature : FileMoveHandle
 * Function : OnMoveProgressTimer
 * SubFunction : NA
 * FunctionPoints : 验证requestId有效时正常通知进度
 * EnvContions : NA
 * CaseDescription : requestId > 0时调用NotifyProgress并返回E_OK
 */
HWTEST_F(MediaFileMoveHandleTest, OnMoveProgressTimer_ValidRequestId_002, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "100";
    changeInfo->totalSize = 5000;
    changeInfo->processedSize = 2000;
    FileMoveHandle handle(changeInfo, "test_timer_valid_req");
    int32_t ret = handle.OnMoveProgressTimer();
    EXPECT_EQ(ret, E_OK);
}

/*
 * Feature : FileMoveHandle
 * Function : CalculateProgress
 * SubFunction : NA
 * FunctionPoints : 验证targetPath_为空时的进度计算
 * EnvContions : NA
 * CaseDescription : targetPath_为空时realTimeprocessSize=processedSize,
 *                   remainSize和remainCount正常计算
 */
HWTEST_F(MediaFileMoveHandleTest, CalculateProgress_EmptyTarget_001, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "1";
    changeInfo->totalSize = 10000;
    changeInfo->processedSize = 3000;
    changeInfo->processedCount = 3;
    changeInfo->totalCount = 10;

    FileMoveHandle handle(changeInfo, "test_calc_empty");
    handle.targetPath_ = ""; // empty target path

    int32_t ret = handle.CalculateProgress();
    EXPECT_EQ(ret, E_OK);
    // targetPath_ empty: realTimeprocessSize = processedSize = 3000
    EXPECT_EQ(changeInfo->realTimeprocessSize, 3000);
    // remainSize = totalSize - realTimeprocessSize = 10000 - 3000 = 7000
    EXPECT_EQ(changeInfo->remainSize, 7000);
    // remainSize != 0: remainCount = totalCount - processedCount = 10 - 3 = 7
    EXPECT_EQ(changeInfo->remainCount, 7);
}

/*
 * Feature : FileMoveHandle
 * Function : CalculateProgress
 * SubFunction : NA
 * FunctionPoints : 验证targetPath_为空且remainSize为0时的边界条件
 * EnvContions : NA
 * CaseDescription : remainSize==0时remainCount应被置为0
 */
HWTEST_F(MediaFileMoveHandleTest, CalculateProgress_EmptyTarget_ZeroRemain_002, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "1";
    changeInfo->totalSize = 5000;
    changeInfo->processedSize = 5000; // all processed
    changeInfo->processedCount = 10;
    changeInfo->totalCount = 10;

    FileMoveHandle handle(changeInfo, "test_calc_zero_remain");
    handle.targetPath_ = "";

    int32_t ret = handle.CalculateProgress();
    EXPECT_EQ(ret, E_OK);
    // realTimeprocessSize = processedSize = 5000
    EXPECT_EQ(changeInfo->realTimeprocessSize, 5000);
    // remainSize = 5000 - 5000 = 0
    EXPECT_EQ(changeInfo->remainSize, 0);
    // remainSize == 0: remainCount = 0
    EXPECT_EQ(changeInfo->remainCount, 0);
}

/*
 * Feature : FileMoveHandle
 * Function : CalculateProgress
 * SubFunction : NA
 * FunctionPoints : 验证targetPath_非空且文件存在时的进度计算
 * EnvContions : 需要在可写目录创建临时文件
 * CaseDescription : targetPath_非空时stat获取文件大小并累加到realTimeprocessSize
 */
HWTEST_F(MediaFileMoveHandleTest, CalculateProgress_WithTarget_003, TestSize.Level1)
{
    std::string tmpPath = "/data/local/tmp/test_progress_calc.tmp";
    {
        std::ofstream ofs(tmpPath, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        std::string content(1024, 'A'); // 1024 bytes
        ofs.write(content.c_str(), content.size());
        ofs.close();
    }

    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "1";
    changeInfo->totalSize = 10000;
    changeInfo->processedSize = 2000;
    changeInfo->processedCount = 2;
    changeInfo->totalCount = 10;

    FileMoveHandle handle(changeInfo, "test_calc_with_target");
    handle.targetPath_ = tmpPath;

    int32_t ret = handle.CalculateProgress();
    EXPECT_EQ(ret, E_OK);
    // realTimeprocessSize = processedSize + stat.st_size = 2000 + 1024 = 3024
    EXPECT_EQ(changeInfo->realTimeprocessSize, 3024);
    // remainSize = 10000 - 3024 = 6976
    EXPECT_EQ(changeInfo->remainSize, 6976);
    // remainSize != 0: remainCount = 10 - 2 = 8
    EXPECT_EQ(changeInfo->remainCount, 8);

    remove(tmpPath.c_str());
}

/*
 * Feature : FileMoveHandle
 * Function : CalculateProgress
 * SubFunction : NA
 * FunctionPoints : 验证targetPath_非空但文件不存在时stat失败
 * EnvContions : NA
 * CaseDescription : stat()失败时返回E_ERR，不修改进度字段
 */
HWTEST_F(MediaFileMoveHandleTest, CalculateProgress_InvalidTarget_004, TestSize.Level1)
{
    auto changeInfo = std::make_shared<MediaProgressChangeInfo>();
    changeInfo->requestId = "1";
    changeInfo->totalSize = 10000;
    changeInfo->processedSize = 2000;

    FileMoveHandle handle(changeInfo, "test_calc_invalid_target");
    handle.targetPath_ = "/data/local/tmp/nonexistent_file_for_progress_test.dat";

    int32_t ret = handle.CalculateProgress();
    // stat fails, CHECK_AND_RETURN_RET_LOG returns E_ERR
    EXPECT_EQ(ret, E_ERR);
    // realTimeprocessSize should not have been updated (remains at initial value 0)
    EXPECT_EQ(changeInfo->realTimeprocessSize, 0);
    // remainSize should not have been updated (remains at initial value 0)
    EXPECT_EQ(changeInfo->remainSize, 0);
}

} // namespace Media
} // namespace OHOS
