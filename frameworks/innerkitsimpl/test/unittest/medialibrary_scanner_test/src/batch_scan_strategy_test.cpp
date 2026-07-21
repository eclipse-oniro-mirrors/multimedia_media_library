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
 
#include "batch_scan_strategy_test.h"

#include "batch_scan_strategy.h"
#include "batch_scanner_obj.h"
 
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_config.h"
#include "scan_config_builder.h"
#include "scan_task_context.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void BatchScanStrategyTest::SetUp() {}
void BatchScanStrategyTest::TearDown() {}
 
/**
 * @tc.name: GetStrategyType_test01
 * @tc.desc: 返回 BATCH_SCAN
 */
HWTEST_F(BatchScanStrategyTest, GetStrategyType_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetStrategyType_test01");
    BatchScanStrategy strategy;
    EXPECT_EQ(strategy.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
    MEDIA_INFO_LOG("end GetStrategyType_test01");
}
 
/**
 * @tc.name: ValidateBatchContext_NullContext_test01
 * @tc.desc: nullptr → false
 */
HWTEST_F(BatchScanStrategyTest, ValidateBatchContext_NullContext_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateBatchContext_NullContext_test01");
    BatchScanStrategy strategy;
    bool result = strategy.ValidateBatchContext(nullptr);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateBatchContext_NullContext_test01");
}
 
/**
 * @tc.name: ValidateBatchContext_EmptyFilePaths_test02
 * @tc.desc: 空 filePaths → false
 */
HWTEST_F(BatchScanStrategyTest, ValidateBatchContext_EmptyFilePaths_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateBatchContext_EmptyFilePaths_test02");
    BatchScanStrategy strategy;
    auto config = ScanConfigBuilder().SetFileId(1).SetFilePath("").Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateBatchContext(context);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateBatchContext_EmptyFilePaths_test02");
}
 
/**
 * @tc.name: ValidateBatchContext_NoBatchScanInfo_test03
 * @tc.desc: 无 BatchScanInfo → false
 */
HWTEST_F(BatchScanStrategyTest, ValidateBatchContext_NoBatchScanInfo_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateBatchContext_NoBatchScanInfo_test03");
    BatchScanStrategy strategy;
    auto config = ScanConfigBuilder().SetFileId(1).SetFilePath("/test/path").Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateBatchContext(context);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateBatchContext_NoBatchScanInfo_test03");
}
 
/**
 * @tc.name: ValidateBatchContext_ValidContext_test04
 * @tc.desc: 有效上下文 → true
 */
HWTEST_F(BatchScanStrategyTest, ValidateBatchContext_ValidContext_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateBatchContext_ValidContext_test04");
    BatchScanStrategy strategy;
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = {"/test/path1.jpg"};
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test/path1.jpg")
        .SetBatchScanInfo(batchScanInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateBatchContext(context);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("end ValidateBatchContext_ValidContext_test04");
}
 
/**
 * @tc.name: Scan_NullContext_test01
 * @tc.desc: Scan(nullptr) → E_ERR
 */
HWTEST_F(BatchScanStrategyTest, Scan_NullContext_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_NullContext_test01");
    BatchScanStrategy strategy;
    int32_t result = strategy.Scan(nullptr);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_NullContext_test01");
}
 
/**
 * @tc.name: Scan_EmptyFilePaths_test02
 * @tc.desc: 空 filePaths → E_ERR
 */
HWTEST_F(BatchScanStrategyTest, Scan_EmptyFilePaths_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_EmptyFilePaths_test02");
    BatchScanStrategy strategy;
    auto config = ScanConfigBuilder().SetFileId(1).SetFilePath("").Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_EmptyFilePaths_test02");
}
 
/**
 * @tc.name: Scan_MissingBatchScanInfo_test03
 * @tc.desc: 无 BatchScanInfo → E_ERR
 */
HWTEST_F(BatchScanStrategyTest, Scan_MissingBatchScanInfo_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_MissingBatchScanInfo_test03");
    BatchScanStrategy strategy;
    auto config = ScanConfigBuilder().SetFileId(1).SetFilePath("/test").Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_MissingBatchScanInfo_test03");
}
 
/**
 * @tc.name: Scan_CreateScannerObj_test04
 * @tc.desc: CreateScannerObj 返回非空 BatchScannerObj
 */
HWTEST_F(BatchScanStrategyTest, Scan_CreateScannerObj_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_CreateScannerObj_test04");
    BatchScanStrategy strategy;
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = {"/test/file.jpg"};
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test/file.jpg")
        .SetBatchScanInfo(batchScanInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    auto scannerObj = strategy.CreateScannerObj(context);
    EXPECT_NE(scannerObj, nullptr);
    MEDIA_INFO_LOG("end Scan_CreateScannerObj_test04");
}
 
/**
 * @tc.name: Scan_ValidContextReturnsExecuteResult_test05
 * @tc.desc: 空文件列表的 BatchScanInfo → Execute 返回 E_OK
 */
HWTEST_F(BatchScanStrategyTest, Scan_ValidContextReturnsExecuteResult_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_ValidContextReturnsExecuteResult_test05");
    BatchScanStrategy strategy;
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = {"/test/file.jpg"};
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test/file.jpg")
        .SetBatchScanInfo(batchScanInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_TRUE(result == E_OK);
    MEDIA_INFO_LOG("end Scan_ValidContextReturnsExecuteResult_test05, result: %{public}d", result);
}
 
} // namespace Media
} // namespace OHOS