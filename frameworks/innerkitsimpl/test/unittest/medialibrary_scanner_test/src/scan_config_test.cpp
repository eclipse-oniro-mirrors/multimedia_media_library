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

} // namespace Media
} // namespace OHOS