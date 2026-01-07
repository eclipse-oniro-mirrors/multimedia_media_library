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

#include "shooting_mode_operation_test.h"
#include <string>
#include "media_log.h"
#include "shooting_mode_album_operation.h"

using namespace testing::ext;

namespace OHOS::Media {
void ShootingModeOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void ShootingModeOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void ShootingModeOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void ShootingModeOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(ShootingModeOperationTest, Shooting_Mode_Operation_Test_001, TestSize.Level1)
{
    ShootingModeAlbumOperation::UpdateShootingModeAlbum();
    int32_t testStartFileId = 1000000;
    int32_t testMaxFileId = 2000000;
    auto infos = ShootingModeAlbumOperation::QueryShootingAssetsInfo(testStartFileId, testMaxFileId);
    EXPECT_TRUE(infos.empty());

    std::vector<CheckedShootingAssetsInfo> testInfos;
    CheckedShootingAssetsInfo testInfo;
    testInfos.push_back(testInfo);
    std::unordered_set<string> testAlbumIds;
    ShootingModeAlbumOperation::HandleInfos(testInfos);
    EXPECT_FALSE(ShootingModeAlbumOperation::ScanAndUpdateAssetShootingMode(testInfo, testAlbumIds));
}
}  // namespace OHOS::Media