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
 
#include "custom_restore_strategy_test.h"
 
#include "custom_restore_strategy.h"
#include "custom_restore_scanner_obj.h"
 
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_config.h"
#include "scan_config_builder.h"
#include "scan_task_context.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void CustomRestoreStrategyTest::SetUp() {}
void CustomRestoreStrategyTest::TearDown() {}
 
/**
 * @tc.name: GetStrategyType_test01
 * @tc.desc: 返回 CUSTOM_RESTORE_SCAN
 */
HWTEST_F(CustomRestoreStrategyTest, GetStrategyType_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetStrategyType_test01");
    CustomRestoreStrategy strategy;
    EXPECT_EQ(strategy.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);
    MEDIA_INFO_LOG("end GetStrategyType_test01");
}
 
/**
 * @tc.name: ValidateCustomRestoreContext_NullContext_test01
 * @tc.desc: nullptr → false
 */
HWTEST_F(CustomRestoreStrategyTest, ValidateCustomRestoreContext_NullContext_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateCustomRestoreContext_NullContext_test01");
    CustomRestoreStrategy strategy;
    bool result = strategy.ValidateCustomRestoreContext(nullptr);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateCustomRestoreContext_NullContext_test01");
}
 
/**
 * @tc.name: ValidateCustomRestoreContext_EmptyFilePaths_test02
 * @tc.desc: 空 filePaths → false
 */
HWTEST_F(CustomRestoreStrategyTest, ValidateCustomRestoreContext_EmptyFilePaths_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateCustomRestoreContext_EmptyFilePaths_test02");
    CustomRestoreStrategy strategy;
    auto config = ScanConfigBuilder().Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateCustomRestoreContext(context);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateCustomRestoreContext_EmptyFilePaths_test02");
}
 
/**
 * @tc.name: ValidateCustomRestoreContext_DefaultScanInfoFilePathSet_test03
 * @tc.desc: DefaultScanInfo 的 filePath 已设置但 CustomRestoreInfo 的 filePaths 为空 → false
 */
HWTEST_F(CustomRestoreStrategyTest, ValidateCustomRestoreContext_DefaultScanInfoFilePathSet_test03, TestSize.Level0)
{
    CustomRestoreStrategy strategy;
    DefaultScanInfo defaultInfo;
    defaultInfo.SetFileId(1);
    defaultInfo.SetFilePath("/test/path");
    auto config = ScanConfigBuilder().SetDefaultScanInfo(defaultInfo).Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateCustomRestoreContext(context);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end ValidateCustomRestoreContext_DefaultScanInfoFilePathSet_test03");
}
 
/**
 * @tc.name: ValidateCustomRestoreContext_ValidContext_test04
 * @tc.desc: 有效上下文 → true
 */
HWTEST_F(CustomRestoreStrategyTest, ValidateCustomRestoreContext_ValidContext_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ValidateCustomRestoreContext_ValidContext_test04");
    CustomRestoreStrategy strategy;
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/test/path1.jpg"});
    auto config = ScanConfigBuilder()
        .SetCustomRestoreInfo(customInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    bool result = strategy.ValidateCustomRestoreContext(context);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("end ValidateCustomRestoreContext_ValidContext_test04");
}
 
/**
 * @tc.name: Scan_NullContext_test01
 * @tc.desc: Scan(nullptr) → E_ERR
 */
HWTEST_F(CustomRestoreStrategyTest, Scan_NullContext_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_NullContext_test01");
    CustomRestoreStrategy strategy;
    int32_t result = strategy.Scan(nullptr);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_NullContext_test01");
}
 
/**
 * @tc.name: Scan_EmptyFilePaths_test02
 * @tc.desc: 空 filePaths → E_ERR
 */
HWTEST_F(CustomRestoreStrategyTest, Scan_EmptyFilePaths_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_EmptyFilePaths_test02");
    CustomRestoreStrategy strategy;
    auto config = ScanConfigBuilder().Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_EmptyFilePaths_test02");
}
 
 /**
 * @tc.name: Scan_DefaultScanInfoFilePathSet_test03
 * @tc.desc: DefaultScanInfo 的 filePath 已设置但 CustomRestoreInfo 的 filePaths 为空 → E_ERR
 */
HWTEST_F(CustomRestoreStrategyTest, Scan_DefaultScanInfoFilePathSet_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_DefaultScanInfoFilePathSet_test03");
    CustomRestoreStrategy strategy;
    DefaultScanInfo defaultInfo;
    defaultInfo.SetFileId(1);
    defaultInfo.SetFilePath("/test");
    auto config = ScanConfigBuilder().SetDefaultScanInfo(defaultInfo).Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_EQ(result, E_ERR);
    MEDIA_INFO_LOG("end Scan_DefaultScanInfoFilePathSet_test03");
}
 
/**
 * @tc.name: Scan_CreateScannerObj_test04
 * @tc.desc: CreateScannerObj 返回非空 CustomRestoreScannerObj
 */
HWTEST_F(CustomRestoreStrategyTest, Scan_CreateScannerObj_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_CreateScannerObj_test04");
    CustomRestoreStrategy strategy;
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/test/file.jpg"});
    auto config = ScanConfigBuilder()
        .SetCustomRestoreInfo(customInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    auto scannerObj = strategy.CreateScannerObj(context);
    EXPECT_NE(scannerObj, nullptr);
    MEDIA_INFO_LOG("end Scan_CreateScannerObj_test04");
}
 
 /**
 * @tc.name: Scan_ValidContextReturnsExecuteResult_test05
 * @tc.desc: 空文件列表的 CustomRestoreInfo → Execute 返回 E_OK
 */
HWTEST_F(CustomRestoreStrategyTest, Scan_ValidContextReturnsExecuteResult_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Scan_ValidContextReturnsExecuteResult_test05");
    CustomRestoreStrategy strategy;
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/test/file.jpg"});
    auto config = ScanConfigBuilder()
        .SetCustomRestoreInfo(customInfo)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(config);
    int32_t result = strategy.Scan(context);
    EXPECT_TRUE(result == E_OK);
    MEDIA_INFO_LOG("end Scan_ValidContextReturnsExecuteResult_test05, result: %{public}d", result);
}
 
} // namespace Media
} // namespace OHOS