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

#include "scan_config_test.h"

#include "media_log.h"
#include "medialibrary_unittest_utils.h"
#include "surface_buffer.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void ScanConfigTest::SetUp() {}

void ScanConfigTest::TearDown() {}

/**
 * @tc.name: ScanConfigBuilder_DefaultValues_test01
 * @tc.desc: 构建最小配置，验证所有默认值
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_DefaultValues_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_DefaultValues_test01");
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test/path")
        .Build();

    EXPECT_EQ(config.GetFileId(), 1);
    EXPECT_EQ(config.GetFilePath(), "/test/path");
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::ASYNC);
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    EXPECT_EQ(config.GetQuality(), ScanQuality::DEFAULT);
    EXPECT_EQ(config.GetConflictPolicy(), ConflictPolicy::DEFAULT);
    EXPECT_TRUE(config.GetForceScan());
    EXPECT_FALSE(config.GetSkipAlbumUpdate());
    EXPECT_FALSE(config.GetIsMovingPhoto());
    EXPECT_FALSE(config.GetCreateThumbSync());
    EXPECT_TRUE(config.GetInvalidateThumb());
    EXPECT_EQ(config.GetOriginalPicture(), nullptr);
    EXPECT_TRUE(config.GetNeedGenerateThumbnail());
    EXPECT_EQ(config.GetUpdateDirtyCallback(), nullptr);
    EXPECT_EQ(config.GetApiVersion(), MediaLibraryApi::API_10);
    MEDIA_INFO_LOG("end ScanConfigBuilder_DefaultValues_test01");
}

/**
 * @tc.name: ScanConfigBuilder_Build_test02
 * @tc.desc: 链式调用设置所有字段并验证
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_Build_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_Build_test02");
    auto picture = std::make_shared<Picture>();
    auto config = ScanConfigBuilder()
        .SetFileId(100)
        .SetFilePath("/data/test.jpg")
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .SetQuality(ScanQuality::FULL)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetForceScan(true)
        .SetSkipAlbumUpdate(true)
        .SetIsMovingPhoto(true)
        .SetOriginalPicture(picture)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .Build();

    EXPECT_EQ(config.GetFileId(), 100);
    EXPECT_EQ(config.GetFilePath(), "/data/test.jpg");
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    EXPECT_EQ(config.GetQuality(), ScanQuality::FULL);
    EXPECT_EQ(config.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_TRUE(config.GetForceScan());
    EXPECT_TRUE(config.GetSkipAlbumUpdate());
    EXPECT_TRUE(config.GetIsMovingPhoto());
    EXPECT_NE(config.GetOriginalPicture(), nullptr);
    EXPECT_TRUE(config.GetCreateThumbSync());
    EXPECT_FALSE(config.GetInvalidateThumb());
    MEDIA_INFO_LOG("end ScanConfigBuilder_Build_test02");
}

/**
 * @tc.name: ScanConfigBuilder_UseCameraShotPreset_test01
 * @tc.desc: 使用CameraShot预设，验证预设值
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_UseCameraShotPreset_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_UseCameraShotPreset_test01");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .UseCameraShotPreset(true)
        .Build();

    EXPECT_TRUE(config1.GetIsMovingPhoto());
    EXPECT_FALSE(config1.GetSkipAlbumUpdate());
    EXPECT_EQ(config1.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);

    auto config2 = ScanConfigBuilder()
        .SetFileId(2)
        .SetFilePath("/test2")
        .UseCameraShotPreset(false, ScanQuality::FULL)
        .Build();

    EXPECT_FALSE(config2.GetIsMovingPhoto());
    EXPECT_EQ(config2.GetQuality(), ScanQuality::FULL);
    EXPECT_EQ(config2.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);
    MEDIA_INFO_LOG("end ScanConfigBuilder_UseCameraShotPreset_test01");
}

/**
 * @tc.name: ScanConfig_Merge_test01
 * @tc.desc: 合并配置验证各字段合并逻辑
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_test01");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetIsMovingPhoto(true)
        .SetNeedGenerateThumbnail(true)
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetConflictPolicy(ConflictPolicy::DEFAULT)
        .Build();

    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetIsMovingPhoto(false)
        .SetNeedGenerateThumbnail(false)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();

    auto merged = config1.Merge(config2, ScanExecutionMode::SYNC);

    EXPECT_TRUE(merged.GetForceScan());
    EXPECT_FALSE(merged.GetSkipAlbumUpdate());
    EXPECT_TRUE(merged.GetIsMovingPhoto());
    EXPECT_TRUE(merged.GetNeedGenerateThumbnail());
    EXPECT_EQ(merged.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_EQ(merged.GetConflictPolicy(), ConflictPolicy::DEFAULT);
    EXPECT_EQ(merged.GetApiVersion(), MediaLibraryApi::API_10);
    MEDIA_INFO_LOG("end ScanConfig_Merge_test01");
}

/**
 * @tc.name: ScanConfig_Merge_test02
 * @tc.desc: 合并配置，IsMovingPhoto都为false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_test02");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetIsMovingPhoto(false)
        .Build();

    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetIsMovingPhoto(false)
        .Build();

    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);

    EXPECT_FALSE(merged.GetIsMovingPhoto());
    MEDIA_INFO_LOG("end ScanConfig_Merge_test02");
}

/**
 * @tc.name: ScanConfigBuilder_FromConfig_test01
 * @tc.desc: 从配置复制所有字段并验证
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_FromConfig_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_FromConfig_test01");
    auto picture = std::make_shared<Picture>();
    auto originalConfig = ScanConfigBuilder()
        .SetFileId(100)
        .SetFilePath("/original/path")
        .SetOriginalPicture(picture)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .SetForceScan(true)
        .SetSkipAlbumUpdate(true)
        .SetIsMovingPhoto(true)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetQuality(ScanQuality::FULL)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();

    auto copiedConfig = ScanConfigBuilder(originalConfig).Build();

    EXPECT_EQ(copiedConfig.GetFileId(), originalConfig.GetFileId());
    EXPECT_EQ(copiedConfig.GetFilePath(), originalConfig.GetFilePath());
    EXPECT_EQ(copiedConfig.GetOriginalPicture(), originalConfig.GetOriginalPicture());
    EXPECT_EQ(copiedConfig.GetCreateThumbSync(), originalConfig.GetCreateThumbSync());
    EXPECT_EQ(copiedConfig.GetInvalidateThumb(), originalConfig.GetInvalidateThumb());
    EXPECT_EQ(copiedConfig.GetForceScan(), originalConfig.GetForceScan());
    EXPECT_EQ(copiedConfig.GetSkipAlbumUpdate(), originalConfig.GetSkipAlbumUpdate());
    EXPECT_EQ(copiedConfig.GetIsMovingPhoto(), originalConfig.GetIsMovingPhoto());
    EXPECT_EQ(copiedConfig.GetExecutionMode(), originalConfig.GetExecutionMode());
    EXPECT_EQ(copiedConfig.GetQuality(), originalConfig.GetQuality());
    EXPECT_EQ(copiedConfig.GetConflictPolicy(), originalConfig.GetConflictPolicy());

    auto modifiedConfig = ScanConfigBuilder(originalConfig)
        .SetForceScan(false)
        .Build();

    EXPECT_EQ(modifiedConfig.GetFileId(), 100);
    EXPECT_EQ(modifiedConfig.GetFilePath(), "/original/path");
    EXPECT_FALSE(modifiedConfig.GetForceScan());
    EXPECT_EQ(modifiedConfig.GetCallback(), originalConfig.GetCallback());
    MEDIA_INFO_LOG("end ScanConfigBuilder_FromConfig_test01");
}

// ==================== Validate 测试 ====================
 
/**
 * @tc.name: ScanConfig_Validate_NoSingleScanInfo_test
 * @tc.desc: 没有SingleScanInfo时Validate返回false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Validate_NoSingleScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Validate_NoSingleScanInfo_test");
    auto config = ScanConfigBuilder().Build();
    std::string realPath;
    EXPECT_FALSE(config.Validate(realPath));
    MEDIA_INFO_LOG("end ScanConfig_Validate_NoSingleScanInfo_test");
}
 
/**
 * @tc.name: ScanConfig_Validate_EmptyFilePath_test
 * @tc.desc: filePath为空时Validate返回false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Validate_EmptyFilePath_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Validate_EmptyFilePath_test");
    auto config = ScanConfigBuilder().SetFilePath("").Build();
    std::string realPath;
    EXPECT_FALSE(config.Validate(realPath));
    MEDIA_INFO_LOG("end ScanConfig_Validate_EmptyFilePath_test");
}
 
/**
 * @tc.name: ScanConfig_Validate_InvalidPath_test
 * @tc.desc: 无效路径（不存在）时Validate返回false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Validate_InvalidPath_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Validate_InvalidPath_test");
    auto config = ScanConfigBuilder()
        .SetFilePath("/nonexistent/path/to/file.jpg")
        .Build();
    std::string realPath;
    EXPECT_FALSE(config.Validate(realPath));
    MEDIA_INFO_LOG("end ScanConfig_Validate_InvalidPath_test");
}
 
/**
 * @tc.name: ScanConfig_Validate_DirectoryPath_test
 * @tc.desc: 路径指向目录而非文件时Validate返回false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Validate_DirectoryPath_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Validate_DirectoryPath_test");
    auto config = ScanConfigBuilder()
        .SetFilePath("/tmp")
        .Build();
    std::string realPath;
    EXPECT_FALSE(config.Validate(realPath));
    MEDIA_INFO_LOG("end ScanConfig_Validate_DirectoryPath_test");
}
 
// ==================== ToString 测试 ====================
 
/**
 * @tc.name: ScanConfig_ToString_DefaultConfig_test
 * @tc.desc: 默认配置的ToString输出包含所有字段
 */
HWTEST_F(ScanConfigTest, ScanConfig_ToString_DefaultConfig_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_ToString_DefaultConfig_test");
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .Build();
 
    std::string str = config.ToString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("strategyType"), std::string::npos);
    EXPECT_NE(str.find("conflictPolicy"), std::string::npos);
    EXPECT_NE(str.find("executionMode"), std::string::npos);
    EXPECT_NE(str.find("fileId"), std::string::npos);
    EXPECT_NE(str.find("isMovingPhoto"), std::string::npos);
    EXPECT_NE(str.find("isSkipAlbumUpdate"), std::string::npos);
    EXPECT_NE(str.find("needGenerateThumbnail"), std::string::npos);
    EXPECT_NE(str.find("hasSingleScanInfo"), std::string::npos);
    EXPECT_NE(str.find("hasBatchScanInfo"), std::string::npos);
    MEDIA_INFO_LOG("end ScanConfig_ToString_DefaultConfig_test");
}
 
/**
 * @tc.name: ScanConfig_ToString_WithBatchScanInfo_test
 * @tc.desc: 包含BatchScanInfo时hasBatchScanInfo为true
 */
HWTEST_F(ScanConfigTest, ScanConfig_ToString_WithBatchScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_ToString_WithBatchScanInfo_test");
    auto batchInfo = std::make_shared<BatchScanInfo>();
    batchInfo->filePaths = {"/a.jpg", "/b.jpg"};
    auto config = ScanConfigBuilder()
        .SetBatchScanInfo(batchInfo)
        .Build();
 
    std::string str = config.ToString();
    EXPECT_NE(str.find("hasBatchScanInfo\": true"), std::string::npos);
    MEDIA_INFO_LOG("end ScanConfig_ToString_WithBatchScanInfo_test");
}
 
// ==================== SingleScanInfo 惰性初始化测试 ====================
 
/**
 * @tc.name: ScanConfig_SingleScanInfo_Defaults_test
 * @tc.desc: 没有设置SingleScanInfo时，GetFilePath返回空串，GetFileId返回0
 */
HWTEST_F(ScanConfigTest, ScanConfig_SingleScanInfo_Defaults_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_SingleScanInfo_Defaults_test");
    auto config = ScanConfigBuilder().Build();
 
    EXPECT_FALSE(config.HasSingleScanInfo());
    EXPECT_TRUE(config.GetFilePath().empty());
    EXPECT_EQ(config.GetFileId(), 0);
    EXPECT_FALSE(config.GetIsMovingPhoto());
    MEDIA_INFO_LOG("end ScanConfig_SingleScanInfo_Defaults_test");
}
 
/**
 * @tc.name: ScanConfig_SingleScanInfo_LazyInit_test
 * @tc.desc: 设置单个字段后SingleScanInfo被创建
 */
HWTEST_F(ScanConfigTest, ScanConfig_SingleScanInfo_LazyInit_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_SingleScanInfo_LazyInit_test");
    auto config = ScanConfigBuilder()
        .SetFileId(42)
        .Build();
 
    EXPECT_TRUE(config.HasSingleScanInfo());
    EXPECT_EQ(config.GetFileId(), 42);
    EXPECT_TRUE(config.GetFilePath().empty());
    EXPECT_FALSE(config.GetIsMovingPhoto());
    MEDIA_INFO_LOG("end ScanConfig_SingleScanInfo_LazyInit_test");
}
 
/**
 * @tc.name: ScanConfig_SingleScanInfo_SetAllFields_test
 * @tc.desc: 设置所有SingleScanInfo字段
 */
HWTEST_F(ScanConfigTest, ScanConfig_SingleScanInfo_SetAllFields_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_SingleScanInfo_SetAllFields_test");
    auto config = ScanConfigBuilder()
        .SetFileId(99)
        .SetFilePath("/data/photo.jpg")
        .SetIsMovingPhoto(true)
        .Build();
 
    EXPECT_TRUE(config.HasSingleScanInfo());
    EXPECT_EQ(config.GetFileId(), 99);
    EXPECT_EQ(config.GetFilePath(), "/data/photo.jpg");
    EXPECT_TRUE(config.GetIsMovingPhoto());
    MEDIA_INFO_LOG("end ScanConfig_SingleScanInfo_SetAllFields_test");
}
 
// ==================== BatchScanInfo 测试 ====================
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_Defaults_test
 * @tc.desc: 没有设置BatchScanInfo时，所有batch字段返回默认值
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_Defaults_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_Defaults_test");
    auto config = ScanConfigBuilder().Build();
 
    EXPECT_FALSE(config.HasBatchScanInfo());
    EXPECT_TRUE(config.GetFilePaths().empty());
    EXPECT_TRUE(config.GetFileInfos().empty());
    EXPECT_TRUE(config.GetTimeInfoMap().empty());
    EXPECT_EQ(config.GetAlbumId(), 0);
    EXPECT_FALSE(config.GetIsDeduplication());
    EXPECT_FALSE(config.GetHasPhotoCache());
    EXPECT_TRUE(config.GetPhotoCache().empty());
    EXPECT_TRUE(config.GetPackageName().empty());
    EXPECT_TRUE(config.GetBundleName().empty());
    EXPECT_TRUE(config.GetAppId().empty());
    EXPECT_TRUE(config.GetIsFirstBatch());
    EXPECT_TRUE(config.GetOutFileInfos().empty());
    EXPECT_EQ(config.GetOutSameFileNum(), 0);
    EXPECT_EQ(config.GetOutSuccessFileNum(), 0);
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_Defaults_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetFilePaths_test
 * @tc.desc: 设置并获取filePaths
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetFilePaths_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetFilePaths_test");
    std::vector<std::string> paths = {"/a.jpg", "/b.mp4", "/c.png"};
    auto config = ScanConfigBuilder()
        .SetFilePaths(paths)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    const auto& result = config.GetFilePaths();
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "/a.jpg");
    EXPECT_EQ(result[1], "/b.mp4");
    EXPECT_EQ(result[2], "/c.png");
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetFilePaths_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetFileInfos_test
 * @tc.desc: 设置并获取fileInfos
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetFileInfos_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetFileInfos_test");
    std::vector<RestoreFileInfo> fileInfos = {
        RestoreFileInfo{.fileName = "photo1.jpg", .mediaType = MEDIA_TYPE_IMAGE},
        RestoreFileInfo{.fileName = "video1.mp4", .mediaType = MEDIA_TYPE_VIDEO}
    };
    auto config = ScanConfigBuilder()
        .SetFileInfos(fileInfos)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    const auto& result = config.GetFileInfos();
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].fileName, "photo1.jpg");
    EXPECT_EQ(result[1].fileName, "video1.mp4");
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetFileInfos_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetTimeInfoMap_test
 * @tc.desc: 设置并获取timeInfoMap
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetTimeInfoMap_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetTimeInfoMap_test");
    std::unordered_map<std::string, TimeInfo> timeMap;
    timeMap["photo.jpg"] = TimeInfo{.dateAdded = 1000, .dateTaken = 2000, .detailTime = "2026-01-01"};
    auto config = ScanConfigBuilder()
        .SetTimeInfoMap(timeMap)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    const auto& result = config.GetTimeInfoMap();
    EXPECT_EQ(result.size(), 1u);
    EXPECT_NE(result.find("photo.jpg"), result.end());
    EXPECT_EQ(result.at("photo.jpg").dateAdded, 1000);
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetTimeInfoMap_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetAlbumId_test
 * @tc.desc: 设置并获取albumId
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetAlbumId_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetAlbumId_test");
    auto config = ScanConfigBuilder()
        .SetAlbumId(42)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_EQ(config.GetAlbumId(), 42);
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetAlbumId_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetIsDeduplication_test
 * @tc.desc: 设置并获取isDeduplication
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetIsDeduplication_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetIsDeduplication_test");
    auto config = ScanConfigBuilder()
        .SetIsDeduplication(true)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_TRUE(config.GetIsDeduplication());
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetIsDeduplication_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetHasPhotoCache_test
 * @tc.desc: 设置并获取hasPhotoCache
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetHasPhotoCache_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetHasPhotoCache_test");
    auto config = ScanConfigBuilder()
        .SetHasPhotoCache(true)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_TRUE(config.GetHasPhotoCache());
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetHasPhotoCache_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetPhotoCache_test
 * @tc.desc: 设置并获取photoCache
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetPhotoCache_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetPhotoCache_test");
    std::unordered_set<std::string> cache = {"photo1_100_1_0", "photo2_200_1_90"};
    auto config = ScanConfigBuilder()
        .SetPhotoCache(cache)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    const auto& result = config.GetPhotoCache();
    EXPECT_EQ(result.size(), 2u);
    EXPECT_NE(result.find("photo1_100_1_0"), result.end());
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetPhotoCache_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetPackageBundleAppId_test
 * @tc.desc: 设置并获取packageName, bundleName, appId
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetPackageBundleAppId_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetPackageBundleAppId_test");
    auto config = ScanConfigBuilder()
        .SetPackageName("com.test.app")
        .SetBundleName("com.test.bundle")
        .SetAppId("app_id_123")
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_EQ(config.GetPackageName(), "com.test.app");
    EXPECT_EQ(config.GetBundleName(), "com.test.bundle");
    EXPECT_EQ(config.GetAppId(), "app_id_123");
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetPackageBundleAppId_test");
}
 
/**
 * @tc.name: ScanConfig_BatchScanInfo_SetIsFirstBatch_test
 * @tc.desc: 设置并获取isFirstBatch
 */
HWTEST_F(ScanConfigTest, ScanConfig_BatchScanInfo_SetIsFirstBatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_BatchScanInfo_SetIsFirstBatch_test");
    auto config = ScanConfigBuilder()
        .SetIsFirstBatch(false)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_FALSE(config.GetIsFirstBatch());
    MEDIA_INFO_LOG("end ScanConfig_BatchScanInfo_SetIsFirstBatch_test");
}
 
// ==================== Merge 详细测试 ====================
 
/**
 * @tc.name: ScanConfig_Merge_StrategyTypeMatch_test
 * @tc.desc: 相同strategyType合并后保留原策略类型
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_StrategyTypeMatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_StrategyTypeMatch_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    MEDIA_INFO_LOG("end ScanConfig_Merge_StrategyTypeMatch_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_StrategyTypeMismatch_test
 * @tc.desc: 不同strategyType合并后回退到DEFAULT_SCAN
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_StrategyTypeMismatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_StrategyTypeMismatch_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    MEDIA_INFO_LOG("end ScanConfig_Merge_StrategyTypeMismatch_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_ConflictPolicyMatch_test
 * @tc.desc: 相同conflictPolicy合并后保留原策略
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_ConflictPolicyMatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_ConflictPolicyMatch_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);
    MEDIA_INFO_LOG("end ScanConfig_Merge_ConflictPolicyMatch_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_ConflictPolicyMismatch_test
 * @tc.desc: 不同conflictPolicy合并后回退到DEFAULT
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_ConflictPolicyMismatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_ConflictPolicyMismatch_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetConflictPolicy(ConflictPolicy::DEFAULT)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetConflictPolicy(), ConflictPolicy::DEFAULT);
    MEDIA_INFO_LOG("end ScanConfig_Merge_ConflictPolicyMismatch_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_FilePathOverride_test
 * @tc.desc: 合并时other的filePath非空则覆盖
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_FilePathOverride_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_FilePathOverride_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/old/path")
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/new/path")
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetFilePath(), "/new/path");
    MEDIA_INFO_LOG("end ScanConfig_Merge_FilePathOverride_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_FilePathFallback_test
 * @tc.desc: 合并时other的filePath为空则使用this的filePath
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_FilePathFallback_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_FilePathFallback_test");
    // other has no SingleScanInfo (empty filePath)
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/original/path")
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetFilePath(), "/original/path");
    MEDIA_INFO_LOG("end ScanConfig_Merge_FilePathFallback_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_CallbackSyncPriority_test
 * @tc.desc: 合并时SYNC模式的callback优先
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_CallbackSyncPriority_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_CallbackSyncPriority_test");
    auto callback1 = std::make_shared<TestScannerCallback>();
    auto callback2 = std::make_shared<TestScannerCallback>();
 
    // config1是SYNC，callback1优先
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetCallback(callback1)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback2)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::SYNC);
    EXPECT_EQ(merged.GetCallback(), callback1);
    MEDIA_INFO_LOG("end ScanConfig_Merge_CallbackSyncPriority_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_CallbackOtherSyncPriority_test
 * @tc.desc: 合并时other是SYNC模式则使用other的callback
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_CallbackOtherSyncPriority_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_CallbackOtherSyncPriority_test");
    auto callback1 = std::make_shared<TestScannerCallback>();
    auto callback2 = std::make_shared<TestScannerCallback>();
 
    // config1是ASYNC，config2是SYNC，callback2优先
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback1)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetCallback(callback2)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::SYNC);
    EXPECT_EQ(merged.GetCallback(), callback2);
    MEDIA_INFO_LOG("end ScanConfig_Merge_CallbackOtherSyncPriority_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_CallbackBothAsyncFallback_test
 * @tc.desc: 两个都是ASYNC时，优先使用非空callback
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_CallbackBothAsyncFallback_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_CallbackBothAsyncFallback_test");
    auto callback2 = std::make_shared<TestScannerCallback>();
 
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback2)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetCallback(), callback2);
    MEDIA_INFO_LOG("end ScanConfig_Merge_CallbackBothAsyncFallback_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_CallbackBothNull_test
 * @tc.desc: 两个callback都为空时，合并后也为空
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_CallbackBothNull_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_CallbackBothNull_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetCallback(), nullptr);
    MEDIA_INFO_LOG("end ScanConfig_Merge_CallbackBothNull_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_ForceScanAlwaysTrue_test
 * @tc.desc: 合并后forceScan始终为true
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_ForceScanAlwaysTrue_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_ForceScanAlwaysTrue_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetForceScan(false)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetForceScan(false)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_TRUE(merged.GetForceScan());
    MEDIA_INFO_LOG("end ScanConfig_Merge_ForceScanAlwaysTrue_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_SkipAlbumUpdateAlwaysFalse_test
 * @tc.desc: 合并后skipAlbumUpdate始终为false
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_SkipAlbumUpdateAlwaysFalse_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_SkipAlbumUpdateAlwaysFalse_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetSkipAlbumUpdate(true)
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetSkipAlbumUpdate(true)
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_FALSE(merged.GetSkipAlbumUpdate());
    MEDIA_INFO_LOG("end ScanConfig_Merge_SkipAlbumUpdateAlwaysFalse_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_FileIdMismatch_test
 * @tc.desc: fileId不匹配时合并仍能执行（仅打印警告）
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_FileIdMismatch_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_FileIdMismatch_test");
    auto config1 = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test1")
        .Build();
 
    auto config2 = ScanConfigBuilder()
        .SetFileId(2)
        .SetFilePath("/test2")
        .Build();
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    // fileId uses config1's value
    EXPECT_EQ(merged.GetFileId(), 1);
    // filePath uses config2's value (non-empty)
    EXPECT_EQ(merged.GetFilePath(), "/test2");
    MEDIA_INFO_LOG("end ScanConfig_Merge_FileIdMismatch_test");
}
 
// ==================== UseCustomRestorePreset 测试 ====================
 
/**
 * @tc.name: ScanConfigBuilder_UseCustomRestorePreset_test01
 * @tc.desc: 使用全参数预设构建批量恢复配置
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_UseCustomRestorePreset_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_UseCustomRestorePreset_test01");
    std::vector<std::string> paths = {"/restore/a.jpg", "/restore/b.mp4"};
    std::vector<RestoreFileInfo> fileInfos = {
        RestoreFileInfo{.fileName = "a.jpg", .mediaType = MEDIA_TYPE_IMAGE},
        RestoreFileInfo{.fileName = "b.mp4", .mediaType = MEDIA_TYPE_VIDEO}
    };
    std::unordered_map<std::string, TimeInfo> timeMap;
    timeMap["a.jpg"] = TimeInfo{.dateAdded = 100, .dateTaken = 200, .detailTime = "2026-01-01"};
    std::unordered_set<std::string> photoCache = {"a.jpg_100_1_0"};
 
    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(
            paths, fileInfos, timeMap,
            5,       // albumId
            true,    // isDeduplication
            true,    // hasPhotoCache
            photoCache,
            "com.test.pkg",
            "com.test.bundle",
            "app123",
            false)   // isFirstBatch
        .Build();
 
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
    EXPECT_TRUE(config.HasBatchScanInfo());
 
    const auto& resultPaths = config.GetFilePaths();
    EXPECT_EQ(resultPaths.size(), 2u);
    EXPECT_EQ(resultPaths[0], "/restore/a.jpg");
 
    const auto& resultInfos = config.GetFileInfos();
    EXPECT_EQ(resultInfos.size(), 2u);
 
    const auto& resultTimeMap = config.GetTimeInfoMap();
    EXPECT_EQ(resultTimeMap.size(), 1u);
 
    EXPECT_EQ(config.GetAlbumId(), 5);
    EXPECT_TRUE(config.GetIsDeduplication());
    EXPECT_TRUE(config.GetHasPhotoCache());
 
    const auto& resultCache = config.GetPhotoCache();
    EXPECT_EQ(resultCache.size(), 1u);
 
    EXPECT_EQ(config.GetPackageName(), "com.test.pkg");
    EXPECT_EQ(config.GetBundleName(), "com.test.bundle");
    EXPECT_EQ(config.GetAppId(), "app123");
    EXPECT_FALSE(config.GetIsFirstBatch());
    MEDIA_INFO_LOG("end ScanConfigBuilder_UseCustomRestorePreset_test01");
}
 
/**
 * @tc.name: ScanConfigBuilder_UseCustomRestorePreset_DefaultArgs_test
 * @tc.desc: 使用默认参数的CustomRestorePreset
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_UseCustomRestorePreset_DefaultArgs_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_UseCustomRestorePreset_DefaultArgs_test");
    std::vector<std::string> paths = {"/a.jpg"};
    std::vector<RestoreFileInfo> fileInfos;
    std::unordered_map<std::string, TimeInfo> timeMap;
 
    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(paths, fileInfos, timeMap)
        .Build();
 
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
    EXPECT_EQ(config.GetAlbumId(), 0);
    EXPECT_FALSE(config.GetIsDeduplication());
    EXPECT_FALSE(config.GetHasPhotoCache());
    EXPECT_TRUE(config.GetPhotoCache().empty());
    EXPECT_TRUE(config.GetPackageName().empty());
    EXPECT_TRUE(config.GetBundleName().empty());
    EXPECT_TRUE(config.GetAppId().empty());
    EXPECT_TRUE(config.GetIsFirstBatch());
    MEDIA_INFO_LOG("end ScanConfigBuilder_UseCustomRestorePreset_DefaultArgs_test");
}
 
// ==================== UseThumbnailCallbackPreset 测试 ====================
 
/**
 * @tc.name: ScanConfigBuilder_UseThumbnailCallbackPreset_test01
 * @tc.desc: 使用缩略图回调预设
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_UseThumbnailCallbackPreset_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_UseThumbnailCallbackPreset_test01");
    auto picture = std::make_shared<Picture>();
    auto dirtyCallback = std::make_shared<TestScannerCallback>();
 
    auto config = ScanConfigBuilder()
        .UseThumbnailCallbackPreset(true, false, picture, dirtyCallback)
        .Build();
 
    EXPECT_TRUE(config.GetCreateThumbSync());
    EXPECT_FALSE(config.GetInvalidateThumb());
    EXPECT_EQ(config.GetOriginalPicture(), picture);
    EXPECT_EQ(config.GetUpdateDirtyCallback(), dirtyCallback);
    MEDIA_INFO_LOG("end ScanConfigBuilder_UseThumbnailCallbackPreset_test01");
}
 
/**
 * @tc.name: ScanConfigBuilder_UseThumbnailCallbackPreset_DefaultArgs_test
 * @tc.desc: 使用默认参数的缩略图回调预设
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_UseThumbnailCallbackPreset_DefaultArgs_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_UseThumbnailCallbackPreset_DefaultArgs_test");
    auto config = ScanConfigBuilder()
        .UseThumbnailCallbackPreset(false, true)
        .Build();
 
    EXPECT_FALSE(config.GetCreateThumbSync());
    EXPECT_TRUE(config.GetInvalidateThumb());
    EXPECT_EQ(config.GetOriginalPicture(), nullptr);
    EXPECT_EQ(config.GetUpdateDirtyCallback(), nullptr);
    MEDIA_INFO_LOG("end ScanConfigBuilder_UseThumbnailCallbackPreset_DefaultArgs_test");
}
 
// ==================== Builder链式调用测试 ====================
 
/**
 * @tc.name: ScanConfigBuilder_Chaining_test
 * @tc.desc: 验证所有setter方法返回引用支持链式调用
 */
HWTEST_F(ScanConfigTest, ScanConfigBuilder_Chaining_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfigBuilder_Chaining_test");
    auto config = ScanConfigBuilder()
        .SetFileId(1)
        .SetFilePath("/test")
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetNeedGenerateThumbnail(false)
        .SetIsMovingPhoto(true)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .SetStrategyType(ScanStrategyType::BATCH_SCAN)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .SetQuality(ScanQuality::FULL)
        .Build();
 
    EXPECT_EQ(config.GetFileId(), 1);
    EXPECT_EQ(config.GetFilePath(), "/test");
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_FALSE(config.GetForceScan());
    EXPECT_TRUE(config.GetSkipAlbumUpdate());
    EXPECT_FALSE(config.GetNeedGenerateThumbnail());
    EXPECT_TRUE(config.GetIsMovingPhoto());
    EXPECT_TRUE(config.GetCreateThumbSync());
    EXPECT_FALSE(config.GetInvalidateThumb());
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::BATCH_SCAN);
    EXPECT_EQ(config.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(config.GetQuality(), ScanQuality::FULL);
    MEDIA_INFO_LOG("end ScanConfigBuilder_Chaining_test");
}
 
// ==================== GetApiVersion 测试 ====================
 
/**
 * @tc.name: ScanConfig_GetApiVersion_AlwaysApi10_test
 * @tc.desc: GetApiVersion始终返回API_10
 */
HWTEST_F(ScanConfigTest, ScanConfig_GetApiVersion_AlwaysApi10_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_GetApiVersion_AlwaysApi10_test");
    auto config = ScanConfigBuilder().Build();
    EXPECT_EQ(config.GetApiVersion(), MediaLibraryApi::API_10);
    MEDIA_INFO_LOG("end ScanConfig_GetApiVersion_AlwaysApi10_test");
}
 
// ==================== BatchScanInfo直接设置测试 ====================
 
/**
 * @tc.name: ScanConfig_SetBatchScanInfo_test
 * @tc.desc: 通过SetBatchScanInfo设置整个BatchScanInfo
 */
HWTEST_F(ScanConfigTest, ScanConfig_SetBatchScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_SetBatchScanInfo_test");
    auto batchInfo = std::make_shared<BatchScanInfo>();
    batchInfo->filePaths = {"/x.jpg"};
    batchInfo->albumId = 99;
    batchInfo->isDeduplication = true;
    batchInfo->outSameFileNum = 3;
 
    auto config = ScanConfigBuilder()
        .SetBatchScanInfo(batchInfo)
        .Build();
 
    EXPECT_TRUE(config.HasBatchScanInfo());
    EXPECT_EQ(config.GetFilePaths().size(), 1u);
    EXPECT_EQ(config.GetAlbumId(), 99);
    EXPECT_TRUE(config.GetIsDeduplication());
    EXPECT_EQ(config.GetOutSameFileNum(), 3);
    MEDIA_INFO_LOG("end ScanConfig_SetBatchScanInfo_test");
}

} // namespace Media
} // namespace OHOS