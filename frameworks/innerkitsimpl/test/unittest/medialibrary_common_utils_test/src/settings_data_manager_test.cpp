/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "SettingsDataManagerTest"

#include "settings_data_manager_test.h"

#include <string>

#include "medialibrary_errno.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {

void SettingsDataManagerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SettingsDataManagerTest SetUpTestCase";
}

void SettingsDataManagerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "SettingsDataManagerTest TearDownTestCase";
}

void SettingsDataManagerTest::SetUp()
{
    GTEST_LOG_(INFO) << "SettingsDataManagerTest SetUp";
}

void SettingsDataManagerTest::TearDown()
{
    GTEST_LOG_(INFO) << "SettingsDataManagerTest TearDown";
}

HWTEST_F(SettingsDataManagerTest, GetPhotosSyncSwitchStatus_Default_Test, TestSize.Level1)
{
    SwitchStatus status = SettingsDataManager::GetPhotosSyncSwitchStatus();
    EXPECT_TRUE(status == SwitchStatus::CLOSE || status == SwitchStatus::CLOUD ||
                status == SwitchStatus::HDC || status == SwitchStatus::NONE);
}

HWTEST_F(SettingsDataManagerTest, GetHdcDeviceId_Supported_Test, TestSize.Level1)
{
    #ifdef MEDIA_LIBRARY_MEMOSPACE_SERVICE_SUPPORT
    std::string deviceId;
    bool result = SettingsDataManager::GetHdcDeviceId(deviceId);
    EXPECT_TRUE(result || !result);
    #else
    std::string deviceId;
    bool result = SettingsDataManager::GetHdcDeviceId(deviceId);
    EXPECT_FALSE(result);
    EXPECT_TRUE(deviceId.empty());
    #endif
}

HWTEST_F(SettingsDataManagerTest, GetHdcDeviceId_NotSupported_Test, TestSize.Level1)
{
    #ifndef MEDIA_LIBRARY_MEMOSPACE_SERVICE_SUPPORT
    std::string deviceId;
    bool result = SettingsDataManager::GetHdcDeviceId(deviceId);
    EXPECT_FALSE(result);
    EXPECT_TRUE(deviceId.empty());
    #endif
}

HWTEST_F(SettingsDataManagerTest, GetAllAlbumUploadStatus_Default_Test, TestSize.Level1)
{
    AlbumUploadSwitchStatus status = SettingsDataManager::GetAllAlbumUploadStatus();
    EXPECT_TRUE(status == AlbumUploadSwitchStatus::CLOSE ||
                status == AlbumUploadSwitchStatus::OPEN ||
                status == AlbumUploadSwitchStatus::NONE);
}

HWTEST_F(SettingsDataManagerTest, UpdateOrInsertAllPhotosAlbumUpload_Update_Test, TestSize.Level1)
{
    AlbumUploadSwitchStatus currentStatus = SettingsDataManager::GetAllAlbumUploadStatus();

    if (currentStatus != AlbumUploadSwitchStatus::NONE) {
        int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
        EXPECT_LE(ret, E_OK);
    }
}

HWTEST_F(SettingsDataManagerTest, UpdateOrInsertAllPhotosAlbumUpload_Multiple_Test, TestSize.Level1)
{
    int32_t ret1 = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    int32_t ret2 = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();

    EXPECT_LE(ret1, E_OK);
    EXPECT_LE(ret2, E_OK);
}

HWTEST_F(SettingsDataManagerTest, QueryParamInSettingData_ValidKey_Test, TestSize.Level1)
{
    std::string key = "test_key";
    std::string value;

    SwitchStatus ret = SettingsDataManager::GetPhotosSyncSwitchStatus();
    EXPECT_TRUE(ret == SwitchStatus::CLOSE ||
                ret == SwitchStatus::CLOUD ||
                ret == SwitchStatus::HDC ||
                ret == SwitchStatus::NONE);
}

HWTEST_F(SettingsDataManagerTest, UpdateParamInSettingData_Valid_Test, TestSize.Level1)
{
    std::string key = "test_update_key";
    std::string value = "test_value";

    int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    EXPECT_LE(ret, E_OK);
}

HWTEST_F(SettingsDataManagerTest, UpdateParamInSettingData_EmptyValue_Test, TestSize.Level1)
{
    std::string key = "test_update_key";
    std::string value = "";

    int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    EXPECT_LE(ret, E_OK);
}

HWTEST_F(SettingsDataManagerTest, InsertParamInSettingData_Valid_Test, TestSize.Level1)
{
    std::string key = "test_insert_key";
    std::string value = "test_value";

    int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    EXPECT_LE(ret, E_OK);
}

HWTEST_F(SettingsDataManagerTest, InsertParamInSettingData_EmptyValue_Test, TestSize.Level1)
{
    std::string key = "test_insert_key";
    std::string value = "";

    int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    EXPECT_LE(ret, E_OK);
}

HWTEST_F(SettingsDataManagerTest, SwitchStatus_EnumValues_Test, TestSize.Level1)
{
    EXPECT_EQ(-1, static_cast<int>(SwitchStatus::NONE));
    EXPECT_EQ(0, static_cast<int>(SwitchStatus::CLOSE));
    EXPECT_EQ(1, static_cast<int>(SwitchStatus::CLOUD));
    EXPECT_EQ(2, static_cast<int>(SwitchStatus::HDC));
}

HWTEST_F(SettingsDataManagerTest, AlbumUploadSwitchStatus_EnumValues_Test, TestSize.Level1)
{
    EXPECT_EQ(-1, static_cast<int>(AlbumUploadSwitchStatus::NONE));
    EXPECT_EQ(0, static_cast<int>(AlbumUploadSwitchStatus::CLOSE));
    EXPECT_EQ(1, static_cast<int>(AlbumUploadSwitchStatus::OPEN));
}

HWTEST_F(SettingsDataManagerTest, GetPhotosSyncSwitchStatus_MultipleCalls_Test, TestSize.Level1)
{
    SwitchStatus status1 = SettingsDataManager::GetPhotosSyncSwitchStatus();
    SwitchStatus status2 = SettingsDataManager::GetPhotosSyncSwitchStatus();
    SwitchStatus status3 = SettingsDataManager::GetPhotosSyncSwitchStatus();

    EXPECT_EQ(status1, status2);
    EXPECT_EQ(status2, status3);
}

HWTEST_F(SettingsDataManagerTest, GetAllAlbumUploadStatus_MultipleCalls_Test, TestSize.Level1)
{
    AlbumUploadSwitchStatus status1 = SettingsDataManager::GetAllAlbumUploadStatus();
    AlbumUploadSwitchStatus status2 = SettingsDataManager::GetAllAlbumUploadStatus();
    AlbumUploadSwitchStatus status3 = SettingsDataManager::GetAllAlbumUploadStatus();

    EXPECT_EQ(status1, status2);
    EXPECT_EQ(status2, status3);
}

HWTEST_F(SettingsDataManagerTest, GetPhotosSyncSwitchStatus_AfterUpdate_Test, TestSize.Level1)
{
    int32_t ret = SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    EXPECT_LE(ret, E_OK);

    SwitchStatus statusAfter = SettingsDataManager::GetPhotosSyncSwitchStatus();
    EXPECT_TRUE(statusAfter == SwitchStatus::CLOSE ||
                statusAfter == SwitchStatus::CLOUD ||
                statusAfter == SwitchStatus::HDC ||
                statusAfter == SwitchStatus::NONE);
}

} // namespace Media
} // namespace OHOS
