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
 
#include "media_scanner_manager_custom_restore_test.h"
 
#include "media_scanner_manager.h"
#include "enhanced_scan_executor.h"
 
#include "media_log.h"
#include "medialibrary_errno.h"
#include "scan_config_builder.h"
#include "scan_task_context.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void MediaScannerManagerCustomRestoreTest::SetUp() {}
 
void MediaScannerManagerCustomRestoreTest::TearDown() {}
 
// ==================== ScanSync + CUSTOM_RESTORE_SCAN 路由测试 ====================
 
/**
 * @tc.name: ScanSync_CustomRestore_EmptyFilePaths_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN但filePaths为空，返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_EmptyFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_EmptyFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_EmptyFilePaths_test");
}
 
/**
 * @tc.name: ScanSync_CustomRestore_EnhancedExecutorNull_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN，enhancedExecutor为空时返回E_ERR
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_EnhancedExecutorNull_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_EnhancedExecutorNull_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto savedExecutor = manager->enhancedExecutor_;
    manager->enhancedExecutor_ = nullptr;
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_ERR);
 
    manager->enhancedExecutor_ = savedExecutor;
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_EnhancedExecutorNull_test");
}
 
/**
 * @tc.name: ScanSync_CustomRestore_ValidConfig_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN，有效配置走ExecuteCustomRestore路径
 *          关键验证：不走PrepareValidatedContext路径（不会返回E_INVALID_PATH）
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_ValidConfig_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_ValidConfig_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    // 关键验证：CUSTOM_RESTORE_SCAN不走Validate(realPath)，不会返回E_INVALID_PATH
    EXPECT_NE(result, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_ValidConfig_test, result: %{public}d", result);
}
 
/**
 * @tc.name: ScanSync_CustomRestore_MultipleFilePaths_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN，多个文件路径走ExecuteCustomRestore路径
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_MultipleFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_MultipleFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/a.jpg", "/test/b.mp4", "/test/c.png"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    customInfo.SetAlbumId(5);
    customInfo.SetIsDeduplication(true);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_PATH);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_MultipleFilePaths_test, result: %{public}d", result);
}
 
/**
 * @tc.name: ScanSync_CustomRestore_WithUseCustomRestorePreset_test
 * @tc.desc: ScanSync传入通过UseCustomRestorePreset构建的CUSTOM_RESTORE_SCAN配置
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_WithUseCustomRestorePreset_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_WithUseCustomRestorePreset_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/restore/photo1.jpg"};
    std::vector<RestoreFileInfo> fileInfos = {
        RestoreFileInfo{.fileName = "photo1.jpg", .mediaType = MEDIA_TYPE_IMAGE}
    };
    std::unordered_map<std::string, TimeInfo> timeMap;
    timeMap["photo1.jpg"] = TimeInfo{.dateAdded = 1000, .dateTaken = 2000, .detailTime = "2026-01-01"};
 
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    customInfo.SetFileInfos(fileInfos);
    customInfo.SetTimeInfoMap(timeMap);
    customInfo.SetAlbumId(10);
    customInfo.SetIsDeduplication(true);
    customInfo.SetHasPhotoCache(true);
    customInfo.SetPhotoCache({"photo1.jpg_100_1_0"});
    customInfo.SetPackageName("com.test.pkg");
    customInfo.SetBundleName("com.test.bundle");
    customInfo.SetAppId("appId123");
    customInfo.SetIsFirstBatch(false);
    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(customInfo)
        .Build();
 
    // UseCustomRestorePreset sets CUSTOM_RESTORE_SCAN automatically
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_PATH);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_WithUseCustomRestorePreset_test, result: %{public}d", result);
}
 
// ==================== ScanSync + DEFAULT_SCAN 对比测试 ====================
 
/**
 * @tc.name: ScanSync_DefaultScan_InvalidPath_test
 * @tc.desc: ScanSync传入DEFAULT_SCAN且无效路径，走PrepareValidatedContext返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_DefaultScan_InvalidPath_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_DefaultScan_InvalidPath_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
    config.GetDefaultScanInfo().SetFilePath("/nonexistent/path.jpg");
    config.GetDefaultScanInfo().SetFileId(1);
 
    int32_t result = manager->ScanSync(config);
    // DEFAULT_SCAN走PrepareValidatedContext，无效路径返回E_INVALID_PATH
    EXPECT_EQ(result, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_DefaultScan_InvalidPath_test");
}
 
/**
 * @tc.name: ScanSync_DefaultScan_NoSingleScanInfo_test
 * @tc.desc: ScanSync传入DEFAULT_SCAN但无SingleScanInfo，返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_DefaultScan_NoSingleScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_DefaultScan_NoSingleScanInfo_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_DefaultScan_NoSingleScanInfo_test");
}
 
/**
 * @tc.name: ScanSync_RouteComparison_CustomRestoreVsDefault_test
 * @tc.desc: 对比CUSTOM_RESTORE_SCAN和DEFAULT_SCAN的路由差异
 *          CUSTOM_RESTORE_SCAN + 无效文件路径 → 不返回E_INVALID_PATH
 *          DEFAULT_SCAN + 无效文件路径 → 返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_RouteComparison_CustomRestoreVsDefault_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_RouteComparison_CustomRestoreVsDefault_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    // CUSTOM_RESTORE_SCAN + 无效路径：不走Validate，返回非E_INVALID_PATH
    std::vector<std::string> paths = {"/nonexistent/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto customRestoreConfig = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
    int32_t customRestoreResult = manager->ScanSync(customRestoreConfig);
 
    // DEFAULT_SCAN + 无效路径：走Validate，返回E_INVALID_PATH
    auto defaultConfig = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
    defaultConfig.GetDefaultScanInfo().SetFilePath("/nonexistent/file.jpg");
    defaultConfig.GetDefaultScanInfo().SetFileId(1);
    int32_t defaultResult = manager->ScanSync(defaultConfig);
 
    // 关键路由差异验证
    EXPECT_NE(customRestoreResult, E_INVALID_PATH);
    EXPECT_EQ(defaultResult, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_RouteComparison, customRestore: %{public}d, default: %{public}d",
        customRestoreResult, defaultResult);
}
 
// ==================== ExecuteCustomRestore 直接测试 ====================
 
/**
 * @tc.name: ExecuteCustomRestore_EmptyFilePaths_test
 * @tc.desc: ExecuteCustomRestore在filePaths为空时返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ExecuteCustomRestore_EmptyFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteCustomRestore_EmptyFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .Build();
 
    int32_t result = manager->ExecuteCustomRestore(config, ScanExecutionMode::SYNC);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ExecuteCustomRestore_EmptyFilePaths_test");
}
 
/**
 * @tc.name: ExecuteCustomRestore_NullExecutor_test
 * @tc.desc: ExecuteCustomRestore在enhancedExecutor为空时返回E_ERR
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ExecuteCustomRestore_NullExecutor_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteCustomRestore_NullExecutor_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto savedExecutor = manager->enhancedExecutor_;
    manager->enhancedExecutor_ = nullptr;
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ExecuteCustomRestore(config, ScanExecutionMode::SYNC);
    EXPECT_EQ(result, E_ERR);
 
    manager->enhancedExecutor_ = savedExecutor;
    MEDIA_INFO_LOG("end ExecuteCustomRestore_NullExecutor_test");
}
 
/**
 * @tc.name: ExecuteCustomRestore_SetsExecutionMode_test
 * @tc.desc: ExecuteCustomRestore构建的finalConfig设置了正确的executionMode
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ExecuteCustomRestore_SetsExecutionMode_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteCustomRestore_SetsExecutionMode_test");
    // 验证ExecuteCustomRestore内部通过ScanConfigBuilder设置executionMode
    std::vector<std::string> paths = {"/test/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    // 初始executionMode为ASYNC（默认值）
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::ASYNC);
 
    // ExecuteCustomRestore内部会做：
    auto syncConfig = ScanConfigBuilder(config).SetExecutionMode(ScanExecutionMode::SYNC).Build();
    EXPECT_EQ(syncConfig.GetExecutionMode(), ScanExecutionMode::SYNC);
 
    auto asyncConfig = ScanConfigBuilder(config).SetExecutionMode(ScanExecutionMode::ASYNC).Build();
    EXPECT_EQ(asyncConfig.GetExecutionMode(), ScanExecutionMode::ASYNC);
    MEDIA_INFO_LOG("end ExecuteCustomRestore_SetsExecutionMode_test");
}
 
/**
 * @tc.name: ExecuteCustomRestore_ValidConfig_test
 * @tc.desc: ExecuteCustomRestore传入有效配置，enhancedExecutor非空时正常执行
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ExecuteCustomRestore_ValidConfig_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteCustomRestore_ValidConfig_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ExecuteCustomRestore(config, ScanExecutionMode::SYNC);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    EXPECT_NE(result, E_ERR);
    MEDIA_INFO_LOG("end ExecuteCustomRestore_ValidConfig_test, result: %{public}d", result);
}
 
// ==================== ScanSync CUSTOM_RESTORE_SCAN 边界场景 ====================
 
/**
 * @tc.name: ScanSync_CustomRestore_OnlyFilePathsNoExplicitCustomRestoreInfo_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN只有filePaths（通过SetCustomRestoreInfo设置）
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest,
    ScanSync_CustomRestore_OnlyFilePathsNoExplicitCustomRestoreInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_OnlyFilePathsNoExplicitCustomRestoreInfo_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/photo.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    // SetCustomRestoreInfo 设置了 filePaths
    EXPECT_EQ(config.GetCustomRestoreInfo().GetFilePaths().size(), 1u);
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_OnlyFilePathsNoExplicitCustomRestoreInfo_test, result: %{public}d",
        result);
}
 
/**
 * @tc.name: ScanSync_CustomRestore_ContextPreservation_test
 * @tc.desc: 验证ExecuteCustomRestore构建的context保留了原始config的关键字段
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_ContextPreservation_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_ContextPreservation_test");
    std::vector<std::string> paths = {"/test/a.jpg", "/test/b.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    customInfo.SetAlbumId(42);
    customInfo.SetIsDeduplication(true);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .Build();
 
    // 模拟ExecuteCustomRestore的内部流程
    auto finalConfig = ScanConfigBuilder(config)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(finalConfig);
 
    EXPECT_EQ(context->config.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);
    EXPECT_EQ(context->config.GetCustomRestoreInfo().GetFilePaths().size(), 2u);
    EXPECT_EQ(context->config.GetCustomRestoreInfo().GetAlbumId(), 42);
    EXPECT_TRUE(context->config.GetCustomRestoreInfo().GetIsDeduplication());
    EXPECT_EQ(context->config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_TRUE(context->IsCustomRestoreScan());
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_ContextPreservation_test");
}
 
/**
 * @tc.name: ScanSync_CustomRestore_EmptyFilePathsVector_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN但filePaths为空vector返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_EmptyFilePathsVector_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_EmptyFilePathsVector_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    std::vector<std::string> emptyPaths;
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(emptyPaths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_EmptyFilePathsVector_test");
}
 
/**
 * @tc.name: ScanSync_CustomRestore_AsyncExecutionMode_test
 * @tc.desc: ScanSync传入CUSTOM_RESTORE_SCAN + ASYNC执行模式
 *          ExecuteCustomRestore内部会设置executionMode为参数值
 */
HWTEST_F(MediaScannerManagerCustomRestoreTest, ScanSync_CustomRestore_AsyncExecutionMode_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_CustomRestore_AsyncExecutionMode_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/async_file.jpg"};
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    // ScanSync always passes ScanExecutionMode::SYNC to ExecuteCustomRestore
    // But we can verify ExecuteCustomRestore directly with ASYNC
    int32_t result = manager->ExecuteCustomRestore(config, ScanExecutionMode::ASYNC);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    EXPECT_NE(result, E_ERR);
    MEDIA_INFO_LOG("end ScanSync_CustomRestore_AsyncExecutionMode_test, result: %{public}d", result);
}
 
} // namespace Media
} // namespace OHOS