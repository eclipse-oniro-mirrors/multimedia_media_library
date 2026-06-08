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

#define MLOG_TAG "ConsistencyCheckDataTypesTest"

#include "consistency_check_data_types_test.h"

#include "consistency_check_data_types.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void ConsistencyCheckDataTypesTest::SetUpTestCase() {}
void ConsistencyCheckDataTypesTest::TearDownTestCase() {}
void ConsistencyCheckDataTypesTest::SetUp() {}
void ConsistencyCheckDataTypesTest::TearDown() {}

// ==================== ScenarioProgress Tests ====================

/**
 * @tc.name: ScenarioProgress_ToString_ContainsAllFields_001
 * @tc.desc: Test ScenarioProgress::ToString contains photo, album, timeInMs
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, ScenarioProgress_ToString_ContainsAllFields_001, TestSize.Level1)
{
    ConsistencyCheck::ScenarioProgress progress;
    progress.lastFileId = 100;
    progress.lastAlbumId = 200;
    std::string str = progress.ToString();
    EXPECT_TRUE(str.find("ScenarioProgress") != std::string::npos);
    EXPECT_TRUE(str.find("100") != std::string::npos);
    EXPECT_TRUE(str.find("200") != std::string::npos);
}

/**
 * @tc.name: ScenarioProgress_DefaultValues_002
 * @tc.desc: Test ScenarioProgress default values are all zero
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, ScenarioProgress_DefaultValues_002, TestSize.Level1)
{
    ConsistencyCheck::ScenarioProgress progress;
    EXPECT_EQ(progress.lastFileId, 0);
    EXPECT_EQ(progress.lastAlbumId, 0);
}

// ==================== DeviceStatus Tests ====================

/**
 * @tc.name: DeviceStatus_ToString_ContainsAllFields_001
 * @tc.desc: Test DeviceStatus::ToString contains all status fields
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, DeviceStatus_ToString_ContainsAllFields_001, TestSize.Level1)
{
    ConsistencyCheck::DeviceStatus status;
    status.isScreenOff = true;
    status.isCharging = false;
    status.isBackgroundTaskAllowed = true;
    status.batteryCapacity = 85;
    status.temperature = 35;
    std::string str = status.ToString();
    EXPECT_TRUE(str.find("DeviceStatus") != std::string::npos);
    EXPECT_TRUE(str.find("85") != std::string::npos);
    EXPECT_TRUE(str.find("35") != std::string::npos);
}

/**
 * @tc.name: DeviceStatus_DefaultValues_002
 * @tc.desc: Test DeviceStatus default values
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, DeviceStatus_DefaultValues_002, TestSize.Level1)
{
    ConsistencyCheck::DeviceStatus status;
    EXPECT_FALSE(status.isScreenOff);
    EXPECT_FALSE(status.isCharging);
    EXPECT_FALSE(status.isBackgroundTaskAllowed);
    EXPECT_EQ(status.batteryCapacity, -1);
    EXPECT_EQ(status.temperature, -1);
}

// ==================== DfxStats Tests ====================

/**
 * @tc.name: DfxStats_ToString_ContainsAllFields_001
 * @tc.desc: Test DfxStats::ToString contains photo, album, timeInMs fields
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, DfxStats_ToString_ContainsAllFields_001, TestSize.Level1)
{
    ConsistencyCheck::DfxStats stats;
    stats.photoAddCount = 10;
    stats.photoUpdateCount = 20;
    stats.photoDeleteCount = 30;
    stats.albumAddCount = 1;
    stats.albumUpdateCount = 2;
    stats.albumDeleteCount = 3;
    stats.startTimeInMs = 1000;
    stats.endTimeInMs = 2000;
    std::string str = stats.ToString();
    EXPECT_TRUE(str.find("DfxStats") != std::string::npos);
    EXPECT_TRUE(str.find("10") != std::string::npos);
    EXPECT_TRUE(str.find("20") != std::string::npos);
    EXPECT_TRUE(str.find("30") != std::string::npos);
}

/**
 * @tc.name: DfxStats_DefaultValues_002
 * @tc.desc: Test DfxStats default values are all zero
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, DfxStats_DefaultValues_002, TestSize.Level1)
{
    ConsistencyCheck::DfxStats stats;
    EXPECT_EQ(stats.photoAddCount, 0);
    EXPECT_EQ(stats.photoUpdateCount, 0);
    EXPECT_EQ(stats.photoDeleteCount, 0);
    EXPECT_EQ(stats.albumAddCount, 0);
    EXPECT_EQ(stats.albumUpdateCount, 0);
    EXPECT_EQ(stats.albumDeleteCount, 0);
    EXPECT_EQ(stats.startTimeInMs, 0u);
    EXPECT_EQ(stats.endTimeInMs, 0u);
}

// ==================== AlbumRecord Tests ====================

/**
 * @tc.name: AlbumRecord_ToString_ContainsFields_001
 * @tc.desc: Test AlbumRecord::ToString contains albumId, lpath, albumSubtype
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, AlbumRecord_ToString_ContainsFields_001, TestSize.Level1)
{
    ConsistencyCheck::AlbumRecord record;
    record.albumId = 42;
    record.albumSubtype = 1;
    record.lpath = "/test/path";
    std::string str = record.ToString();
    EXPECT_TRUE(str.find("AlbumRecord") != std::string::npos);
    EXPECT_TRUE(str.find("42") != std::string::npos);
    EXPECT_TRUE(str.find("albumSubtype: 1") != std::string::npos);
}

/**
 * @tc.name: AlbumRecord_DefaultValues_002
 * @tc.desc: Test AlbumRecord default values
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, AlbumRecord_DefaultValues_002, TestSize.Level1)
{
    ConsistencyCheck::AlbumRecord record;
    EXPECT_EQ(record.albumId, -1);
    EXPECT_EQ(record.albumSubtype, -1);
    EXPECT_TRUE(record.lpath.empty());
}

// ==================== PhotoRecord Tests ====================

/**
 * @tc.name: PhotoRecord_ToString_ContainsFields_001
 * @tc.desc: Test PhotoRecord::ToString contains fileId, storagePath, albumRecord
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, PhotoRecord_ToString_ContainsFields_001, TestSize.Level1)
{
    ConsistencyCheck::PhotoRecord record;
    record.fileId = 123;
    record.storagePath = "/storage/test/photo.jpg";
    record.albumRecord.albumId = 456;
    std::string str = record.ToString();
    EXPECT_TRUE(str.find("PhotoRecord") != std::string::npos);
    EXPECT_TRUE(str.find("123") != std::string::npos);
    EXPECT_TRUE(str.find("456") != std::string::npos);
}

/**
 * @tc.name: PhotoRecord_DefaultValues_002
 * @tc.desc: Test PhotoRecord default values
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, PhotoRecord_DefaultValues_002, TestSize.Level1)
{
    ConsistencyCheck::PhotoRecord record;
    EXPECT_EQ(record.fileId, -1);
    EXPECT_EQ(record.position, -1);
    EXPECT_EQ(record.subtype, 0);
    EXPECT_EQ(record.fileSourceType, -1);
    EXPECT_EQ(record.dateModified, 0);
    EXPECT_EQ(record.dateTaken, 0);
    EXPECT_TRUE(record.storagePath.empty());
    EXPECT_TRUE(record.data.empty());
    EXPECT_TRUE(record.displayName.empty());
    EXPECT_TRUE(record.cloudId.empty());
}

/**
 * @tc.name: PhotoRecord_ToString_NestedAlbumRecord_003
 * @tc.desc: Test PhotoRecord::ToString includes nested AlbumRecord info
 * @tc.type: FUNC
 */
HWTEST_F(ConsistencyCheckDataTypesTest, PhotoRecord_ToString_NestedAlbumRecord_003, TestSize.Level1)
{
    ConsistencyCheck::PhotoRecord record;
    record.fileId = 10;
    record.albumRecord.albumId = 20;
    record.albumRecord.albumSubtype = 5;
    record.albumRecord.lpath = "/album/path";
    std::string str = record.ToString();
    // Should contain AlbumRecord nested toString
    EXPECT_TRUE(str.find("AlbumRecord") != std::string::npos);
    EXPECT_TRUE(str.find("20") != std::string::npos);
}

} // namespace Media
} // namespace OHOS
