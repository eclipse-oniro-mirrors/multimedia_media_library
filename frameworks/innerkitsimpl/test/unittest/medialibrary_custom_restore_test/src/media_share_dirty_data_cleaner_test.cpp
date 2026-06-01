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

#define MLOG_TAG "MediaShareDirtyDataCleanerUnitTest"

#include "media_share_dirty_data_cleaner_test.h"

#include "preferences_helper.h"

#define private public
#include "media_share_dirty_data_cleaner.h"
#undef private

#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "custom_restore_const.h"
#include "media_column.h"
#include "media_upgrade.h"
#include "media_time_utils.h"

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void MediaShareDirtyDataCleanerTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaShareDirtyDataCleanerTest SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("SetUpTestCase failed, can not get rdbstore");
        return;
    }
    int32_t ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Create photo table failed");
    }
}

void MediaShareDirtyDataCleanerTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("MediaShareDirtyDataCleanerTest TearDownTestCase");
    g_rdbStore = nullptr;
}

void MediaShareDirtyDataCleanerTest::SetUp()
{
    MEDIA_INFO_LOG("MediaShareDirtyDataCleanerTest SetUp");
}

void MediaShareDirtyDataCleanerTest::TearDown()
{
    MEDIA_INFO_LOG("MediaShareDirtyDataCleanerTest TearDown");
}

static int32_t InsertPhotoAsset(string packageName, int32_t pendingState, int64_t time)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::MediaColumn::MEDIA_OWNER_PACKAGE, packageName);
    values.PutInt(Media::MediaColumn::MEDIA_TIME_PENDING, pendingState);
    values.PutLong(Media::MediaColumn::MEDIA_DATE_ADDED, time);

    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, Media::PhotoColumn::PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

HWTEST_F(MediaShareDirtyDataCleanerTest, media_share_dirty_data_cleaner_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_001 Start");
    MediaShareDirtyDataCleaner::UpdateCleanFlag(true);
    int64_t lastShareTime = MediaTimeUtils::UTCTimeMilliSeconds();
    EXPECT_TRUE(MediaShareDirtyDataCleaner::IsNeedClean(lastShareTime));
    MediaShareDirtyDataCleaner::UpdateCleanFlag(false);
    EXPECT_FALSE(MediaShareDirtyDataCleaner::IsNeedClean(lastShareTime));
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_001 End");
}

HWTEST_F(MediaShareDirtyDataCleanerTest, media_share_dirty_data_cleaner_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_002 Start");
    MediaShareDirtyDataCleaner::UpdateShareTime(true);
    int32_t errCode;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get shared preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastShareTime = prefs->GetLong(LAST_SHARE_TIME, -1);
    EXPECT_NE(lastShareTime, -1);
    MediaShareDirtyDataCleaner::UpdateShareTime(false);
    lastShareTime = prefs->GetLong(LAST_SHARE_TIME, -1);
    EXPECT_EQ(lastShareTime, -1);
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_002 End");
}

HWTEST_F(MediaShareDirtyDataCleanerTest, media_share_dirty_data_cleaner_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_003 Start");
    MediaShareDirtyDataCleaner::UpdateCleanFlag(true);
    int64_t shareTime = MediaTimeUtils::UTCTimeMilliSeconds();
    MediaShareDirtyDataCleaner::UpdateShareTime(shareTime);
    InsertPhotoAsset(SHARE_PACKAGE_NAME, -1, shareTime + 1000);
    std::unordered_map<int32_t, std::string> dirtyDataMap = MediaShareDirtyDataCleaner::GetDirtyData(shareTime);
    EXPECT_EQ(dirtyDataMap.size(), 1);
    MediaShareDirtyDataCleaner::CheckDirtyData();
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_003 End");
}

HWTEST_F(MediaShareDirtyDataCleanerTest, media_share_dirty_data_cleaner_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_004 Start");
    MediaShareDirtyDataCleaner::SetSharingState(true);
    EXPECT_TRUE(MediaShareDirtyDataCleaner::GetSharingState());
    MediaShareDirtyDataCleaner::SetSharingState(false);
    EXPECT_FALSE(MediaShareDirtyDataCleaner::GetSharingState());
    MEDIA_INFO_LOG("media_share_dirty_data_cleaner_test_004 End");
}
}
}