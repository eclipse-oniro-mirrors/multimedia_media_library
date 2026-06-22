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

#define MLOG_TAG "CheckDfxCollectorTest"

#include "check_dfx_collector_test.h"

#include "check_dfx_collector.h"
#include "check_scene.h"
#include "consistency_check_data_types.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;

void CheckDfxCollectorTest::SetUpTestCase() {}
void CheckDfxCollectorTest::TearDownTestCase() {}
void CheckDfxCollectorTest::SetUp() {}
void CheckDfxCollectorTest::TearDown() {}

// Constructor Tests

/**
 * @tc.name: Constructor_LakeScene_001
 * @tc.desc: Test CheckDfxCollector construction with LAKE scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, Constructor_LakeScene_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("scene: 2") != std::string::npos); // LAKE = 2
}

/**
 * @tc.name: Constructor_FileManagerScene_002
 * @tc.desc: Test CheckDfxCollector construction with FILE_MANAGER scene
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, Constructor_FileManagerScene_002, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::FILE_MANAGER);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("scene: 3") != std::string::npos); // FILE_MANAGER = 3
}

// OnCheckStart / OnCheckEnd Tests

/**
 * @tc.name: OnCheckStart_SetsTimestamp_001
 * @tc.desc: Test OnCheckStart records a non-zero start time
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnCheckStart_SetsTimestamp_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnCheckStart();
    collector.OnCheckEnd();
    std::string str = collector.ToString();
    // startTimeInMs should be non-zero after OnCheckStart
    EXPECT_TRUE(str.find("timeInMs: 0|0") == std::string::npos);
}

/**
 * @tc.name: OnCheckEnd_SetsTimestamp_002
 * @tc.desc: Test OnCheckEnd records end time >= start time
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnCheckEnd_SetsTimestamp_002, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnCheckStart();
    collector.OnCheckEnd();
    // Verify ToString contains non-zero times
    std::string str = collector.ToString();
    EXPECT_FALSE(str.empty());
}

// Counter Accumulation Tests

/**
 * @tc.name: OnPhotoAdd_AccumulatesDelta_001
 * @tc.desc: Test OnPhotoAdd accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnPhotoAdd_AccumulatesDelta_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnPhotoAdd(5);
    collector.OnPhotoAdd(3);
    std::string str = collector.ToString();
    // photoAddCount should be 8
    EXPECT_TRUE(str.find("photo: 8|") != std::string::npos);
}

/**
 * @tc.name: OnPhotoUpdate_AccumulatesDelta_002
 * @tc.desc: Test OnPhotoUpdate accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnPhotoUpdate_AccumulatesDelta_002, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnPhotoUpdate(10);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("|10|") != std::string::npos);
}

/**
 * @tc.name: OnPhotoDelete_AccumulatesDelta_003
 * @tc.desc: Test OnPhotoDelete accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnPhotoDelete_AccumulatesDelta_003, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnPhotoDelete(7);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("|7,") != std::string::npos);
}

/**
 * @tc.name: OnAlbumAdd_AccumulatesDelta_004
 * @tc.desc: Test OnAlbumAdd accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnAlbumAdd_AccumulatesDelta_004, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnAlbumAdd(2);
    collector.OnAlbumAdd(1);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("album: 3|") != std::string::npos);
}

/**
 * @tc.name: OnAlbumUpdate_AccumulatesDelta_005
 * @tc.desc: Test OnAlbumUpdate accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnAlbumUpdate_AccumulatesDelta_005, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnAlbumUpdate(4);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("|4|") != std::string::npos);
}

/**
 * @tc.name: OnAlbumDelete_AccumulatesDelta_006
 * @tc.desc: Test OnAlbumDelete accumulates delta correctly
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, OnAlbumDelete_AccumulatesDelta_006, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnAlbumDelete(6);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("|6,") != std::string::npos);
}

// Reset Tests

/**
 * @tc.name: Reset_ClearsAllCounters_001
 * @tc.desc: Test Reset clears all accumulated counters to zero
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, Reset_ClearsAllCounters_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnPhotoAdd(10);
    collector.OnPhotoUpdate(20);
    collector.OnPhotoDelete(30);
    collector.OnAlbumAdd(40);
    collector.OnAlbumUpdate(50);
    collector.OnAlbumDelete(60);
    collector.Reset();
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("photo: 0|0|0") != std::string::npos);
    EXPECT_TRUE(str.find("album: 0|0|0") != std::string::npos);
}

// ToString Tests

/**
 * @tc.name: ToString_ContainsAllFields_001
 * @tc.desc: Test ToString output contains scene, photo, album, timeInMs fields
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, ToString_ContainsAllFields_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::FILE_MANAGER);
    std::string str = collector.ToString();
    EXPECT_TRUE(str.find("scene:") != std::string::npos);
    EXPECT_TRUE(str.find("DfxStats") != std::string::npos);
    EXPECT_TRUE(str.find("photo:") != std::string::npos);
    EXPECT_TRUE(str.find("album:") != std::string::npos);
    EXPECT_TRUE(str.find("timeInMs:") != std::string::npos);
}

/**
 * @tc.name: MultipleCounters_CombinedAccumulation_001
 * @tc.desc: Test multiple counter types accumulated simultaneously
 * @tc.type: FUNC
 */
HWTEST_F(CheckDfxCollectorTest, MultipleCounters_CombinedAccumulation_001, TestSize.Level1)
{
    CheckDfxCollector collector(CheckScene::LAKE);
    collector.OnPhotoAdd(1);
    collector.OnPhotoAdd(2);
    collector.OnPhotoUpdate(5);
    collector.OnPhotoDelete(3);
    collector.OnAlbumAdd(10);
    collector.OnAlbumUpdate(20);
    collector.OnAlbumDelete(30);
    std::string str = collector.ToString();
    // photo: 3|5|3
    EXPECT_TRUE(str.find("photo: 3|5|3") != std::string::npos);
    // album: 10|20|30
    EXPECT_TRUE(str.find("album: 10|20|30") != std::string::npos);
}

} // namespace Media
} // namespace OHOS
