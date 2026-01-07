/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "height_width_correct_operation_test.h"
#include <string>
#include "media_log.h"
#include "height_width_correct_operation.h"
#include "userfile_manager_types.h"
#include "medialibrary_errno.h"

using namespace testing::ext;

namespace OHOS::Media {

const int32_t TEST_NUMBER = 100;
const int32_t TEST_NUMBER_ONE = 1;
const int32_t TEST_NUMBER_TWO = 2;
const int32_t TEST_NUMBER_THREE = 3;
const int32_t TEST_NUMBER_FOUR = 4;
const int32_t TEST_NUMBER_FIVE = 5;
const int32_t ROTATE_ANGLE_90 = 90;
const int32_t ROTATE_ANGLE_270 = 270;

void HeightWidthCorrectOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void HeightWidthCorrectOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void HeightWidthCorrectOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void HeightWidthCorrectOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_001, TestSize.Level1)
{
    HeightWidthCorrectOperation::UpdateHeightAndWidth();
    std::unordered_set<int32_t> failFileIds;
    HeightWidthCorrectOperation::RemoveInvalidFromFailIds(failFileIds);
    failFileIds.insert(TEST_NUMBER_ONE);
    failFileIds.insert(TEST_NUMBER_TWO);
    failFileIds.insert(TEST_NUMBER_THREE);
    failFileIds.insert(TEST_NUMBER_FOUR);
    HeightWidthCorrectOperation::RemoveInvalidFromFailIds(failFileIds);

    int32_t fileId = TEST_NUMBER;
    int32_t count = HeightWidthCorrectOperation::QueryNoCheckPhotoCount(fileId);
    EXPECT_EQ(count, 0);
    std::vector<CheckPhotoInfo> photoInfos;
    photoInfos = HeightWidthCorrectOperation::QueryNoCheckPhotoInfo(fileId);
    EXPECT_EQ(photoInfos.size(), 0);
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_003, TestSize.Level1)
{
    std::vector<CheckPhotoInfo> photoInfos;
    std::vector<int32_t> failIds;
    photoInfos = HeightWidthCorrectOperation::QueryCheckFailPhotoInfo(failIds);
    EXPECT_EQ(photoInfos.size(), 0);

    failIds.push_back(100);
    failIds.push_back(200);
    photoInfos = HeightWidthCorrectOperation::QueryCheckFailPhotoInfo(failIds);
    EXPECT_EQ(photoInfos.size(), 0);
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_004, TestSize.Level1)
{
    std::vector<CheckPhotoInfo> photoInfos;
    int32_t curFileId = 2;
    std::unordered_set<int32_t> failedIds;
    int32_t count = 0;
    HeightWidthCorrectOperation::HandlePhotoInfos(photoInfos, curFileId, failedIds, count);

    CheckPhotoInfo photoInfoFirst;
    photoInfoFirst.fileId = 3;
    photoInfos.push_back(photoInfoFirst);
    HeightWidthCorrectOperation::HandlePhotoInfos(photoInfos, curFileId, failedIds, count);

    CheckPhotoInfo photoInfo;
    photoInfo.height = -1;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    bool ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_005, TestSize.Level1)
{
    CheckPhotoInfo photoInfo;
    photoInfo.height = -1;
    photoInfo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    bool ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.height = 0;
    photoInfo.width = -1;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.width = 0;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.width = TEST_NUMBER_ONE;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_006, TestSize.Level1)
{
    CheckPhotoInfo photoInfo;
    photoInfo.height = TEST_NUMBER_ONE;
    photoInfo.width = TEST_NUMBER_ONE;
    photoInfo.lcdSize = "";
    bool ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = ":123";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = "123";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = "123:";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = "12r3:234";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = "123:23r4";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.lcdSize = "123:234";
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);
}

HWTEST_F(HeightWidthCorrectOperationTest, Height_Width_Correct_Operation_Test_007, TestSize.Level1)
{
    CheckPhotoInfo photoInfo;
    photoInfo.height = TEST_NUMBER_ONE;
    photoInfo.width = TEST_NUMBER_ONE;
    photoInfo.lcdSize = "123:234";
    photoInfo.exifRotate = TEST_NUMBER_ONE;
    bool ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.exifRotate = TEST_NUMBER_FIVE;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.exifRotate = 0;
    photoInfo.orientation = ROTATE_ANGLE_90;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.exifRotate = 0;
    photoInfo.orientation = ROTATE_ANGLE_270;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);

    photoInfo.exifRotate = 0;
    photoInfo.orientation = 0;
    ret = HeightWidthCorrectOperation::UpdatePhotoHeightWidth(photoInfo);
    EXPECT_FALSE(ret);
}
}  // namespace OHOS::Media