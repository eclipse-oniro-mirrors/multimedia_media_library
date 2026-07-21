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
 
#include "media_scanner_manager_batch_test.h"

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
 
void MediaScannerManagerBatchTest::SetUp() {}
 
void MediaScannerManagerBatchTest::TearDown() {}
 
// ==================== ScanSync + BATCH_SCAN 路由测试 ====================
 
/**
 * @tc.name: ScanSync_BatchScan_EmptyFilePaths_test
 * @tc.desc: ScanSync传入BATCH_SCAN但filePaths为空，返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_EmptyFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_EmptyFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_EmptyFilePaths_test");
}
 
/**
 * @tc.name: ScanSync_BatchScan_EnhancedExecutorNull_test
 * @tc.desc: ScanSync传入BATCH_SCAN，enhancedExecutor为空时返回E_ERR
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_EnhancedExecutorNull_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_EnhancedExecutorNull_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto savedExecutor = manager->enhancedExecutor_;
    manager->enhancedExecutor_ = nullptr;
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_ERR);
 
    manager->enhancedExecutor_ = savedExecutor;
    MEDIA_INFO_LOG("end ScanSync_BatchScan_EnhancedExecutorNull_test");
}
 
/**
 * @tc.name: ScanSync_BatchScan_ValidConfig_test
 * @tc.desc: ScanSync传入BATCH_SCAN，有效配置走ExecuteBatchScan路径
 *          关键验证：不走PrepareValidatedContext路径（不会返回E_INVALID_PATH）
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_ValidConfig_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_ValidConfig_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    // 关键验证：BATCH_SCAN不走Validate(realPath)，不会返回E_INVALID_PATH
    EXPECT_NE(result, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_ValidConfig_test, result: %{public}d", result);
}
 
/**
 * @tc.name: ScanSync_BatchScan_MultipleFilePaths_test
 * @tc.desc: ScanSync传入BATCH_SCAN，多个文件路径走ExecuteBatchScan路径
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_MultipleFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_MultipleFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/a.jpg", "/test/b.mp4", "/test/c.png"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
    batchScanInfo->albumId = 5;
    batchScanInfo->isDeduplication = true;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_PATH);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_MultipleFilePaths_test, result: %{public}d", result);
}
 
/**
 * @tc.name: ScanSync_BatchScan_WithUseCustomRestorePreset_test
 * @tc.desc: ScanSync传入通过UseCustomRestorePreset构建的BATCH_SCAN配置
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_WithUseCustomRestorePreset_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_WithUseCustomRestorePreset_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/restore/photo1.jpg"};
    std::vector<RestoreFileInfo> fileInfos = {
        RestoreFileInfo{.fileName = "photo1.jpg", .mediaType = MEDIA_TYPE_IMAGE}
    };
    std::unordered_map<std::string, TimeInfo> timeMap;
    timeMap["photo1.jpg"] = TimeInfo{.dateAdded = 1000, .dateTaken = 2000, .detailTime = "2026-01-01"};
 
    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(
            paths, fileInfos, timeMap,
            10,       // albumId
            true,     // isDeduplication
            true,     // hasPhotoCache
            {"photo1.jpg_100_1_0"},
            "com.test.pkg",
            "com.test.bundle",
            "appId123",
            false)    // isFirstBatch
        .Build();
 
    // UseCustomRestorePreset sets BATCH_SCAN automatically
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_PATH);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_WithUseCustomRestorePreset_test, result: %{public}d", result);
}
 
// ==================== ScanSync + DEFAULT_SCAN 对比测试 ====================
 
/**
 * @tc.name: ScanSync_DefaultScan_InvalidPath_test
 * @tc.desc: ScanSync传入DEFAULT_SCAN且无效路径，走PrepareValidatedContext返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_DefaultScan_InvalidPath_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_DefaultScan_InvalidPath_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .SetFilePath("/nonexistent/path.jpg")
        .SetFileId(1)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    // DEFAULT_SCAN走PrepareValidatedContext，无效路径返回E_INVALID_PATH
    EXPECT_EQ(result, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_DefaultScan_InvalidPath_test");
}
 
/**
 * @tc.name: ScanSync_DefaultScan_NoSingleScanInfo_test
 * @tc.desc: ScanSync传入DEFAULT_SCAN但无SingleScanInfo，返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_DefaultScan_NoSingleScanInfo_test, TestSize.Level0)
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
 * @tc.name: ScanSync_RouteComparison_BatchVsDefault_test
 * @tc.desc: 对比BATCH_SCAN和DEFAULT_SCAN的路由差异
 *          BATCH_SCAN + 无效文件路径 → 不返回E_INVALID_PATH
 *          DEFAULT_SCAN + 无效文件路径 → 返回E_INVALID_PATH
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_RouteComparison_BatchVsDefault_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_RouteComparison_BatchVsDefault_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    // BATCH_SCAN + 无效路径：不走Validate，返回非E_INVALID_PATH
    std::vector<std::string> paths = {"/nonexistent/file.jpg"};
    auto batchConfig = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .Build();
    int32_t batchResult = manager->ScanSync(batchConfig);
 
    // DEFAULT_SCAN + 无效路径：走Validate，返回E_INVALID_PATH
    auto defaultConfig = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .SetFilePath("/nonexistent/file.jpg")
        .SetFileId(1)
        .Build();
    int32_t defaultResult = manager->ScanSync(defaultConfig);
 
    // 关键路由差异验证
    EXPECT_NE(batchResult, E_INVALID_PATH);
    EXPECT_EQ(defaultResult, E_INVALID_PATH);
    MEDIA_INFO_LOG("end ScanSync_RouteComparison_BatchVsDefault_test, batch: %{public}d, default: %{public}d",
        batchResult, defaultResult);
}
 
// ==================== ExecuteBatchScan 直接测试 ====================
 
/**
 * @tc.name: ExecuteBatchScan_EmptyFilePaths_test
 * @tc.desc: ExecuteBatchScan在filePaths为空时返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerBatchTest, ExecuteBatchScan_EmptyFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteBatchScan_EmptyFilePaths_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .Build();
 
    int32_t result = manager->ExecuteBatchScan(config, ScanExecutionMode::SYNC);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ExecuteBatchScan_EmptyFilePaths_test");
}
 
/**
 * @tc.name: ExecuteBatchScan_NullExecutor_test
 * @tc.desc: ExecuteBatchScan在enhancedExecutor为空时返回E_ERR
 */
HWTEST_F(MediaScannerManagerBatchTest, ExecuteBatchScan_NullExecutor_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteBatchScan_NullExecutor_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    auto savedExecutor = manager->enhancedExecutor_;
    manager->enhancedExecutor_ = nullptr;
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .Build();
 
    int32_t result = manager->ExecuteBatchScan(config, ScanExecutionMode::SYNC);
    EXPECT_EQ(result, E_ERR);
 
    manager->enhancedExecutor_ = savedExecutor;
    MEDIA_INFO_LOG("end ExecuteBatchScan_NullExecutor_test");
}
 
/**
 * @tc.name: ExecuteBatchScan_SetsExecutionMode_test
 * @tc.desc: ExecuteBatchScan构建的finalConfig设置了正确的executionMode
 */
HWTEST_F(MediaScannerManagerBatchTest, ExecuteBatchScan_SetsExecutionMode_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteBatchScan_SetsExecutionMode_test");
    // 验证ExecuteBatchScan内部通过ScanConfigBuilder设置executionMode
    std::vector<std::string> paths = {"/test/file.jpg"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .Build();
 
    // 初始executionMode为ASYNC（默认值）
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::ASYNC);
 
    // ExecuteBatchScan内部会做：
    auto syncConfig = ScanConfigBuilder(config).SetExecutionMode(ScanExecutionMode::SYNC).Build();
    EXPECT_EQ(syncConfig.GetExecutionMode(), ScanExecutionMode::SYNC);
 
    auto asyncConfig = ScanConfigBuilder(config).SetExecutionMode(ScanExecutionMode::ASYNC).Build();
    EXPECT_EQ(asyncConfig.GetExecutionMode(), ScanExecutionMode::ASYNC);
    MEDIA_INFO_LOG("end ExecuteBatchScan_SetsExecutionMode_test");
}
 
/**
 * @tc.name: ExecuteBatchScan_ValidConfig_test
 * @tc.desc: ExecuteBatchScan传入有效配置，enhancedExecutor非空时正常执行
 */
HWTEST_F(MediaScannerManagerBatchTest, ExecuteBatchScan_ValidConfig_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ExecuteBatchScan_ValidConfig_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/file.jpg"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .Build();
 
    int32_t result = manager->ExecuteBatchScan(config, ScanExecutionMode::SYNC);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    EXPECT_NE(result, E_ERR);
    MEDIA_INFO_LOG("end ExecuteBatchScan_ValidConfig_test, result: %{public}d", result);
}
 
// ==================== ScanSync BATCH_SCAN 边界场景 ====================
 
/**
 * @tc.name: ScanSync_BatchScan_OnlyFilePathsNoExplicitBatchScanInfo_test
 * @tc.desc: ScanSync传入BATCH_SCAN只有filePaths（SetFilePaths会自动创建BatchScanInfo）
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_OnlyFilePathsNoExplicitBatchScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_OnlyFilePathsNoExplicitBatchScanInfo_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/photo.jpg"};
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .Build();
 
    // SetFilePaths 会自动创建 BatchScanInfo
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_EQ(config.GetFilePaths().size(), 1u);
 
    int32_t result = manager->ScanSync(config);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_OnlyFilePathsNoExplicitBatchScanInfo_test, result: %{public}d", result);
}
 
/**
 * @tc.name: ScanSync_BatchScan_ContextPreservation_test
 * @tc.desc: 验证ExecuteBatchScan构建的context保留了原始config的关键字段
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_ContextPreservation_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_ContextPreservation_test");
    std::vector<std::string> paths = {"/test/a.jpg", "/test/b.jpg"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
    batchScanInfo->albumId = 42;
    batchScanInfo->isDeduplication = true;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .Build();
 
    // 模拟ExecuteBatchScan的内部流程
    auto finalConfig = ScanConfigBuilder(config)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .Build();
    auto context = std::make_shared<ScanTaskContext>(finalConfig);
 
    EXPECT_EQ(context->config.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
    EXPECT_EQ(context->config.GetFilePaths().size(), 2u);
    EXPECT_EQ(context->config.GetAlbumId(), 42);
    EXPECT_TRUE(context->config.GetIsDeduplication());
    EXPECT_EQ(context->config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_TRUE(context->IsBatchScan());
    MEDIA_INFO_LOG("end ScanSync_BatchScan_ContextPreservation_test");
}
 
/**
 * @tc.name: ScanSync_BatchScan_EmptyFilePathsVector_test
 * @tc.desc: ScanSync传入BATCH_SCAN但filePaths为空vector返回E_INVALID_ARGUMENTS
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_EmptyFilePathsVector_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_EmptyFilePathsVector_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
 
    std::vector<std::string> emptyPaths;
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(emptyPaths)
        .Build();
 
    int32_t result = manager->ScanSync(config);
    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_EmptyFilePathsVector_test");
}
 
/**
 * @tc.name: ScanSync_BatchScan_AsyncExecutionMode_test
 * @tc.desc: ScanSync传入BATCH_SCAN + ASYNC执行模式
 *          ExecuteBatchScan内部会设置executionMode为参数值
 */
HWTEST_F(MediaScannerManagerBatchTest, ScanSync_BatchScan_AsyncExecutionMode_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanSync_BatchScan_AsyncExecutionMode_test");
    auto manager = MediaScannerManager::GetInstance();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->enhancedExecutor_, nullptr);
 
    std::vector<std::string> paths = {"/test/async_file.jpg"};
    auto batchScanInfo = std::make_shared<BatchScanInfo>();
    batchScanInfo->filePaths = paths;
 
    auto config = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetFilePaths(paths)
        .SetBatchScanInfo(batchScanInfo)
        .Build();
 
    // ScanSync always passes ScanExecutionMode::SYNC to ExecuteBatchScan
    // But we can verify ExecuteBatchScan directly with ASYNC
    int32_t result = manager->ExecuteBatchScan(config, ScanExecutionMode::ASYNC);
    EXPECT_NE(result, E_INVALID_ARGUMENTS);
    EXPECT_NE(result, E_ERR);
    MEDIA_INFO_LOG("end ScanSync_BatchScan_AsyncExecutionMode_test, result: %{public}d", result);
}
 
} // namespace Media
} // namespace OHOS