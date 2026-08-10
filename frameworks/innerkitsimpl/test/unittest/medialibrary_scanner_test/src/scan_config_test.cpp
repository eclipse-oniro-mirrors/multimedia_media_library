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
        .Build();
    config.GetDefaultScanInfo().SetFileId(1);
    config.GetDefaultScanInfo().SetFilePath("/test/path");

    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 1);
    EXPECT_EQ(config.GetDefaultScanInfo().GetFilePath(), "/test/path");
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::ASYNC);
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    EXPECT_EQ(config.GetQuality(), ScanQuality::DEFAULT);
    EXPECT_EQ(config.GetConflictPolicy(), ConflictPolicy::DEFAULT);
    EXPECT_TRUE(config.GetForceScan());
    EXPECT_FALSE(config.GetSkipAlbumUpdate());
    EXPECT_FALSE(config.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .SetQuality(ScanQuality::FULL)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetForceScan(true)
        .SetSkipAlbumUpdate(true)
        .SetOriginalPicture(picture)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .Build();
    config.GetDefaultScanInfo().SetFileId(100);
    config.GetDefaultScanInfo().SetFilePath("/data/test.jpg");
    config.GetDefaultScanInfo().SetIsMovingPhoto(true);

    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 100);
    EXPECT_EQ(config.GetDefaultScanInfo().GetFilePath(), "/data/test.jpg");
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::DEFAULT_SCAN);
    EXPECT_EQ(config.GetQuality(), ScanQuality::FULL);
    EXPECT_EQ(config.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_TRUE(config.GetForceScan());
    EXPECT_TRUE(config.GetSkipAlbumUpdate());
    EXPECT_TRUE(config.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .UseCameraShotPreset()
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
    config1.GetDefaultScanInfo().SetIsMovingPhoto(true);

    EXPECT_TRUE(config1.GetDefaultScanInfo().GetIsMovingPhoto());
    EXPECT_FALSE(config1.GetSkipAlbumUpdate());
    EXPECT_EQ(config1.GetConflictPolicy(), ConflictPolicy::QUALITY_PRIORITY);

    auto config2 = ScanConfigBuilder()
        .UseCameraShotPreset(ScanQuality::FULL)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(2);
    config2.GetDefaultScanInfo().SetFilePath("/test2");
    config2.GetDefaultScanInfo().SetIsMovingPhoto(false);

    EXPECT_FALSE(config2.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetNeedGenerateThumbnail(true)
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetConflictPolicy(ConflictPolicy::DEFAULT)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
    config1.GetDefaultScanInfo().SetIsMovingPhoto(true);

    auto config2 = ScanConfigBuilder()
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetNeedGenerateThumbnail(false)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
    config2.GetDefaultScanInfo().SetIsMovingPhoto(false);

    auto merged = config1.Merge(config2, ScanExecutionMode::SYNC);

    EXPECT_TRUE(merged.GetForceScan());
    EXPECT_FALSE(merged.GetSkipAlbumUpdate());
    EXPECT_TRUE(merged.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
    config1.GetDefaultScanInfo().SetIsMovingPhoto(false);

    auto config2 = ScanConfigBuilder()
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
    config2.GetDefaultScanInfo().SetIsMovingPhoto(false);

    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);

    EXPECT_FALSE(merged.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .SetOriginalPicture(picture)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .SetForceScan(true)
        .SetSkipAlbumUpdate(true)
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetQuality(ScanQuality::FULL)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    originalConfig.GetDefaultScanInfo().SetFileId(100);
    originalConfig.GetDefaultScanInfo().SetFilePath("/original/path");
    originalConfig.GetDefaultScanInfo().SetIsMovingPhoto(true);

    auto copiedConfig = ScanConfigBuilder(originalConfig).Build();

    EXPECT_EQ(copiedConfig.GetDefaultScanInfo().GetFileId(), originalConfig.GetDefaultScanInfo().GetFileId());
    EXPECT_EQ(copiedConfig.GetDefaultScanInfo().GetFilePath(), originalConfig.GetDefaultScanInfo().GetFilePath());
    EXPECT_EQ(copiedConfig.GetOriginalPicture(), originalConfig.GetOriginalPicture());
    EXPECT_EQ(copiedConfig.GetCreateThumbSync(), originalConfig.GetCreateThumbSync());
    EXPECT_EQ(copiedConfig.GetInvalidateThumb(), originalConfig.GetInvalidateThumb());
    EXPECT_EQ(copiedConfig.GetForceScan(), originalConfig.GetForceScan());
    EXPECT_EQ(copiedConfig.GetSkipAlbumUpdate(), originalConfig.GetSkipAlbumUpdate());
    EXPECT_EQ(copiedConfig.GetDefaultScanInfo().GetIsMovingPhoto(),
        originalConfig.GetDefaultScanInfo().GetIsMovingPhoto());
    EXPECT_EQ(copiedConfig.GetExecutionMode(), originalConfig.GetExecutionMode());
    EXPECT_EQ(copiedConfig.GetQuality(), originalConfig.GetQuality());
    EXPECT_EQ(copiedConfig.GetConflictPolicy(), originalConfig.GetConflictPolicy());

    auto modifiedConfig = ScanConfigBuilder(originalConfig)
        .SetForceScan(false)
        .Build();

    EXPECT_EQ(modifiedConfig.GetDefaultScanInfo().GetFileId(), 100);
    EXPECT_EQ(modifiedConfig.GetDefaultScanInfo().GetFilePath(), "/original/path");
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
    auto config = ScanConfigBuilder().Build();
    config.GetDefaultScanInfo().SetFilePath("");
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
        .Build();
    config.GetDefaultScanInfo().SetFilePath("/nonexistent/path/to/file.jpg");
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
        .Build();
    config.GetDefaultScanInfo().SetFilePath("/tmp");
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
        .Build();
    config.GetDefaultScanInfo().SetFileId(1);
    config.GetDefaultScanInfo().SetFilePath("/test");
 
    std::string str = config.ToString();
    EXPECT_FALSE(str.empty());
    EXPECT_NE(str.find("strategyType"), std::string::npos);
    EXPECT_NE(str.find("conflictPolicy"), std::string::npos);
    EXPECT_NE(str.find("executionMode"), std::string::npos);
    EXPECT_NE(str.find("fileId"), std::string::npos);
    EXPECT_NE(str.find("isMovingPhoto"), std::string::npos);
    EXPECT_NE(str.find("isSkipAlbumUpdate"), std::string::npos);
    EXPECT_NE(str.find("needGenerateThumbnail"), std::string::npos);
    MEDIA_INFO_LOG("end ScanConfig_ToString_DefaultConfig_test");
}
 
/**
 * @tc.name: ScanConfig_ToString_WithBatchScanInfo_test
 * @tc.desc: 包含BatchScanInfo时hasBatchScanInfo为true
 */
HWTEST_F(ScanConfigTest, ScanConfig_ToString_WithBatchScanInfo_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_ToString_WithBatchScanInfo_test");
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/a.jpg", "/b.jpg"});
    auto config = ScanConfigBuilder()
        .SetCustomRestoreInfo(customInfo)
        .Build();
 
    std::string str = config.ToString();
    EXPECT_FALSE(str.empty());
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
 
    EXPECT_TRUE(config.GetDefaultScanInfo().GetFilePath().empty());
    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 0);
    EXPECT_FALSE(config.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .Build();
    config.GetDefaultScanInfo().SetFileId(42);
 
    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 42);
    EXPECT_TRUE(config.GetDefaultScanInfo().GetFilePath().empty());
    EXPECT_FALSE(config.GetDefaultScanInfo().GetIsMovingPhoto());
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
        .Build();
    config.GetDefaultScanInfo().SetFileId(99);
    config.GetDefaultScanInfo().SetFilePath("/data/photo.jpg");
    config.GetDefaultScanInfo().SetIsMovingPhoto(true);
 
    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 99);
    EXPECT_EQ(config.GetDefaultScanInfo().GetFilePath(), "/data/photo.jpg");
    EXPECT_TRUE(config.GetDefaultScanInfo().GetIsMovingPhoto());
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
    const auto& info = config.GetCustomRestoreInfo();
 
    EXPECT_TRUE(info.GetFilePaths().empty());
    EXPECT_TRUE(info.GetFileInfos().empty());
    EXPECT_TRUE(info.GetTimeInfoMap().empty());
    EXPECT_EQ(info.GetAlbumId(), 0);
    EXPECT_FALSE(info.GetIsDeduplication());
    EXPECT_FALSE(info.GetHasPhotoCache());
    EXPECT_TRUE(info.GetPhotoCache().empty());
    EXPECT_TRUE(info.GetPackageName().empty());
    EXPECT_TRUE(info.GetBundleName().empty());
    EXPECT_TRUE(info.GetAppId().empty());
    EXPECT_TRUE(info.GetIsFirstBatch());
    EXPECT_TRUE(info.GetOutFileInfos().empty());
    EXPECT_EQ(info.GetOutSameFileNum(), 0);
    EXPECT_EQ(info.GetOutSuccessFileNum(), 0);
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
        .Build();
    config.GetCustomRestoreInfo().SetFilePaths(paths);
 
    const auto& result = config.GetCustomRestoreInfo().GetFilePaths();
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
        .Build();
    config.GetCustomRestoreInfo().SetFileInfos(fileInfos);
 
    const auto& result = config.GetCustomRestoreInfo().GetFileInfos();
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
        .Build();
    config.GetCustomRestoreInfo().SetTimeInfoMap(timeMap);
 
    const auto& result = config.GetCustomRestoreInfo().GetTimeInfoMap();
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
        .Build();
    config.GetCustomRestoreInfo().SetAlbumId(42);
 
    EXPECT_EQ(config.GetCustomRestoreInfo().GetAlbumId(), 42);
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
        .Build();
    config.GetCustomRestoreInfo().SetIsDeduplication(true);
 
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetIsDeduplication());
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
        .Build();
    config.GetCustomRestoreInfo().SetHasPhotoCache(true);
 
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetHasPhotoCache());
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
        .Build();
    config.GetCustomRestoreInfo().SetPhotoCache(cache);
 
    const auto& result = config.GetCustomRestoreInfo().GetPhotoCache();
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
        .Build();
    config.GetCustomRestoreInfo().SetPackageName("com.test.app");
    config.GetCustomRestoreInfo().SetBundleName("com.test.bundle");
    config.GetCustomRestoreInfo().SetAppId("app_id_123");
 
    EXPECT_EQ(config.GetCustomRestoreInfo().GetPackageName(), "com.test.app");
    EXPECT_EQ(config.GetCustomRestoreInfo().GetBundleName(), "com.test.bundle");
    EXPECT_EQ(config.GetCustomRestoreInfo().GetAppId(), "app_id_123");
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
        .Build();
    config.GetCustomRestoreInfo().SetIsFirstBatch(false);
 
    EXPECT_FALSE(config.GetCustomRestoreInfo().GetIsFirstBatch());
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
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetStrategyType(ScanStrategyType::DEFAULT_SCAN)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetConflictPolicy(ConflictPolicy::DEFAULT)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/old/path");
 
    auto config2 = ScanConfigBuilder()
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/new/path");
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetDefaultScanInfo().GetFilePath(), "/new/path");
    MEDIA_INFO_LOG("end ScanConfig_Merge_FilePathOverride_test");
}
 
/**
 * @tc.name: ScanConfig_Merge_FilePathFallback_test
 * @tc.desc: 合并时other的filePath为空则使用this的filePath
 */
HWTEST_F(ScanConfigTest, ScanConfig_Merge_FilePathFallback_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ScanConfig_Merge_FilePathFallback_test");
    // other has empty filePath
    auto config1 = ScanConfigBuilder()
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/original/path");
 
    auto config2 = ScanConfigBuilder()
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    EXPECT_EQ(merged.GetDefaultScanInfo().GetFilePath(), "/original/path");
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
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetCallback(callback1)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback2)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback1)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetCallback(callback2)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .SetCallback(callback2)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetExecutionMode(ScanExecutionMode::ASYNC)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetForceScan(false)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetForceScan(false)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .SetSkipAlbumUpdate(true)
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test");
 
    auto config2 = ScanConfigBuilder()
        .SetSkipAlbumUpdate(true)
        .Build();
    config2.GetDefaultScanInfo().SetFileId(1);
    config2.GetDefaultScanInfo().SetFilePath("/test");
 
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
        .Build();
    config1.GetDefaultScanInfo().SetFileId(1);
    config1.GetDefaultScanInfo().SetFilePath("/test1");
 
    auto config2 = ScanConfigBuilder()
        .Build();
    config2.GetDefaultScanInfo().SetFileId(2);
    config2.GetDefaultScanInfo().SetFilePath("/test2");
 
    auto merged = config1.Merge(config2, ScanExecutionMode::ASYNC);
    // fileId uses config1's value
    EXPECT_EQ(merged.GetDefaultScanInfo().GetFileId(), 1);
    // filePath uses config2's value (non-empty)
    EXPECT_EQ(merged.GetDefaultScanInfo().GetFilePath(), "/test2");
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

    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    customInfo.SetFileInfos(fileInfos);
    customInfo.SetTimeInfoMap(timeMap);
    customInfo.SetAlbumId(5);
    customInfo.SetIsDeduplication(true);
    customInfo.SetHasPhotoCache(true);
    customInfo.SetPhotoCache(photoCache);
    customInfo.SetPackageName("com.test.pkg");
    customInfo.SetBundleName("com.test.bundle");
    customInfo.SetAppId("app123");
    customInfo.SetIsFirstBatch(false);

    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(customInfo)
        .Build();

    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);

    const auto& resultPaths = config.GetCustomRestoreInfo().GetFilePaths();
    EXPECT_EQ(resultPaths.size(), 2u);
    EXPECT_EQ(resultPaths[0], "/restore/a.jpg");

    const auto& resultInfos = config.GetCustomRestoreInfo().GetFileInfos();
    EXPECT_EQ(resultInfos.size(), 2u);

    const auto& resultTimeMap = config.GetCustomRestoreInfo().GetTimeInfoMap();
    EXPECT_EQ(resultTimeMap.size(), 1u);

    EXPECT_EQ(config.GetCustomRestoreInfo().GetAlbumId(), 5);
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetIsDeduplication());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetHasPhotoCache());

    const auto& resultCache = config.GetCustomRestoreInfo().GetPhotoCache();
    EXPECT_EQ(resultCache.size(), 1u);

    EXPECT_EQ(config.GetCustomRestoreInfo().GetPackageName(), "com.test.pkg");
    EXPECT_EQ(config.GetCustomRestoreInfo().GetBundleName(), "com.test.bundle");
    EXPECT_EQ(config.GetCustomRestoreInfo().GetAppId(), "app123");
    EXPECT_FALSE(config.GetCustomRestoreInfo().GetIsFirstBatch());
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

    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths(paths);
    customInfo.SetFileInfos(fileInfos);
    customInfo.SetTimeInfoMap(timeMap);

    auto config = ScanConfigBuilder()
        .UseCustomRestorePreset(customInfo)
        .Build();

    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);
    EXPECT_EQ(config.GetCustomRestoreInfo().GetAlbumId(), 0);
    EXPECT_FALSE(config.GetCustomRestoreInfo().GetIsDeduplication());
    EXPECT_FALSE(config.GetCustomRestoreInfo().GetHasPhotoCache());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetPhotoCache().empty());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetPackageName().empty());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetBundleName().empty());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetAppId().empty());
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetIsFirstBatch());
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
        .SetExecutionMode(ScanExecutionMode::SYNC)
        .SetForceScan(false)
        .SetSkipAlbumUpdate(true)
        .SetNeedGenerateThumbnail(false)
        .SetCreateThumbSync(true)
        .SetInvalidateThumb(false)
        .SetStrategyType(ScanStrategyType::CUSTOM_RESTORE_SCAN)
        .SetConflictPolicy(ConflictPolicy::QUALITY_PRIORITY)
        .SetQuality(ScanQuality::FULL)
        .Build();
    config.GetDefaultScanInfo().SetFileId(1);
    config.GetDefaultScanInfo().SetFilePath("/test");
    config.GetDefaultScanInfo().SetIsMovingPhoto(true);

    EXPECT_EQ(config.GetDefaultScanInfo().GetFileId(), 1);
    EXPECT_EQ(config.GetDefaultScanInfo().GetFilePath(), "/test");
    EXPECT_EQ(config.GetExecutionMode(), ScanExecutionMode::SYNC);
    EXPECT_FALSE(config.GetForceScan());
    EXPECT_TRUE(config.GetSkipAlbumUpdate());
    EXPECT_FALSE(config.GetNeedGenerateThumbnail());
    EXPECT_TRUE(config.GetDefaultScanInfo().GetIsMovingPhoto());
    EXPECT_TRUE(config.GetCreateThumbSync());
    EXPECT_FALSE(config.GetInvalidateThumb());
    EXPECT_EQ(config.GetStrategyType(), ScanStrategyType::CUSTOM_RESTORE_SCAN);
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
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/x.jpg"});
    customInfo.SetAlbumId(99);
    customInfo.SetIsDeduplication(true);
    customInfo.SetOutSameFileNum(3);

    auto config = ScanConfigBuilder()
        .SetCustomRestoreInfo(customInfo)
        .Build();

    EXPECT_EQ(config.GetCustomRestoreInfo().GetFilePaths().size(), 1u);
    EXPECT_EQ(config.GetCustomRestoreInfo().GetAlbumId(), 99);
    EXPECT_TRUE(config.GetCustomRestoreInfo().GetIsDeduplication());
    EXPECT_EQ(config.GetCustomRestoreInfo().GetOutSameFileNum(), 3);
    MEDIA_INFO_LOG("end ScanConfig_SetBatchScanInfo_test");
}

} // namespace Media
} // namespace OHOS