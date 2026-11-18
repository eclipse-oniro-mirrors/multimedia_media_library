/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "medialibrary_dfx_test.h"

#include <string>
#include <unordered_set>

#include "dfx_anco_manager.h"
#include "dfx_cloud_manager.h"
#include "dfx_collector.h"
#include "dfx_const.h"
#include "dfx_database_utils.h"
#include "dfx_manager.h"
#include "dfx_reporter.h"
#include "dfx_system_photo_keys.h"
#include "dfx_utils.h"
#include "hisysevent.h"
#include "medialibrary_astc_stat.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "parameters.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
static constexpr int32_t SLEEP_THREE_SECONDS = 3;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static int32_t ClearTable(const string &table)
{
    NativeRdb::RdbPredicates predicates(table);
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    MEDIA_INFO_LOG("clear table: %{public}s, rows: %{public}d, err: %{public}d", table.c_str(), rows, err);
    EXPECT_EQ(err, E_OK);
    return E_OK;
}

static int32_t InsertPhotoAlbum(const string &albumName, const int32_t albumType, const int32_t uploadStatus)
{
    EXPECT_NE((g_rdbStore == nullptr), true);

    int64_t albumId = -1;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, uploadStatus);
    int32_t ret = g_rdbStore->Insert(albumId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhotoAlbum albumId is %{public}s", to_string(albumId).c_str());
    return E_OK;
}

void MediaLibraryDfxTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE((g_rdbStore == nullptr), true);
    DfxManager::GetInstance();
    int32_t errCode;
    const std::vector<std::string> dfxXmlFiles = {
        THUMBNAIL_ERROR_XML, DELETE_BEHAVIOR_XML, COMMON_BEHAVIOR_XML,
        INVALID_BEHAVIOR_XML, COMMON_BEHAVIOR_FOR_FUSE_GETATTER_XML,
        COMMON_BEHAVIOR_FOR_FUSE_TRUNCATE_XML
    };
    for (const auto &xmlFile::dfxXmlFiles) {
        shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(xmlFile, true), errCode);
        if (prefs) {
            prefs->Clear();
            prefs->FlushSync();
        }
    }
}

void MediaLibraryDfxTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearTable(PhotoAlbumColumns::TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_THREE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibraryDfxTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    DfxManager::GetInstance()->isInitSuccess_ = true;
    ClearTable(PhotoAlbumColumns::TABLE);
}

void MediaLibraryDfxTest::TearDown(void) {}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_common_image_test, TestSize.Level0)
{
    std::string path = "common.jpg";
    int32_t width = 256;
    int32_t height = 256;
    int32_t mediaType = 1;
    int32_t result = DfxManager::GetInstance()->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, COMMON_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_common_image_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    std::string path = "common.jpg";
    int32_t width = 256;
    int32_t height = 256;
    int32_t mediaType = 1;
    dfxManager->isInitSuccess_ = false;
    int32_t result = dfxManager->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, NOT_INIT);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_png_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    std::string path = "other.png";
    int32_t width = 256;
    int32_t height = 256;
    int32_t mediaType = 1;
    int32_t result = dfxManager->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, OTHER_FORMAT_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_big_image_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    std::string path = "big_image.jpg";
    int32_t width = 10000;
    int32_t height = 10000;
    int32_t mediaType = 1;
    int32_t result = dfxManager->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, BIG_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_common_video_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    std::string path = "common.mp4";
    int32_t width = 256;
    int32_t height = 256;
    int32_t mediaType = 2;
    int32_t result = dfxManager->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, COMMON_VIDEO);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_8K_video_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    std::string path = "8K.mp4";
    int32_t width = 7680;
    int32_t height = 7680;
    int32_t mediaType = 2;
    int32_t result = dfxManager->HandleHighMemoryThumbnail(path, mediaType, width, height);
    EXPECT_EQ(result, BIG_VIDEO);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_timeout_operation_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    ASSERT_NE(dfxManager->dfxReporter_, nullptr);
    string bundleName = "bundleName";
    int32_t type = 1;
    int32_t object = 0;
    int32_t time = 0;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleTimeOutOperation(bundleName, type, object, time);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
    auto thumbnailError = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(thumbnailError.empty(), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_timeout_operation_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string bundleName = "bundleName";
    int32_t type = 1;
    int32_t object = 0;
    int32_t time = 0;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleTimeOutOperation(bundleName, type, object, time);
}


HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_error_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    std::string path = "";
    int32_t method = 1;
    int32_t errorCode = 1;
    dfxManager->HandleThumbnailError(path, method, errorCode);
    auto thumbnailError = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(thumbnailError.empty(), false);
    EXPECT_EQ(thumbnailError.count(path), 1);
    EXPECT_EQ(thumbnailError[path].errCode, errCode);
    EXPECT_EQ(thumbnailError[path].method, method);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_error_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    std::string path = "";
    int32_t method = 1;
    int32_t errorCode = 1;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleThumbnailError(path, method, errorCode);
    auto thumbnailError = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(thumbnailError.count(path), 0);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_generation_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    ThumbnailData::GenerateStats stats;
    dfxManager->HandleThumbnailGeneration(stats);
    EXPECT_NE(dfxManager->dfxReporter_, nullptr);
    auto thumbnailError = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(thumbnailError.empty(), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_thumbnail_generation_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    ThumbnailData::GenerateStats stats;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleThumbnailGeneration(stats);
    auto thumbnailError = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(thumbnailError.empty(), true);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_common_behavior_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string bundleName = "bundleName";
    int32_t type = 1;
    dfxManager->HandleCommonBehavior(bundleName, type);
    auto commonBehavior = dfxManager->dfxCollector_->GetCommonBehavior();
    EXPECT_EQ(commonBehavior.empty(), false);
    EXPECT_EQ(commonBehavior.count(bundleName), 1);
    EXPECT_EQ(commonBehavior[bundleName].times, 1);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_common_behavior_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string bundleName = "bundleName_behavior_notinit";
    int32_t type = 1;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleCommonBehavior(bundleName, type);
    auto commonBehavior = dfxManager->dfxCollector_->GetCommonBehavior();
    EXPECT_EQ(commonBehavior.count(bundleName), 0);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_half_day_missions_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    ASSERT_NE(dfxManager->dfxWorker_, nullptr);
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_COMMON_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutLong(LAST_HALF_DAY_REPORT_TIME, 0);
    prefs->FlushSync();
    int64_t expectedTime = MediaFileUtils::UTCTimeSeconds();
    dfxManager->HandleHalfDayMissions();
    int64_t afterTime = prefs->GetLong(LAST_HALF_DAY_REPORT_TIME, 0);
    EXPECT_EQ(afterTime, expectedTime);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_half_day_missions_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_COMMON_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    int64_t sentinelTime = 12345;
    prefs->PutLong(LAST_HALF_DAY_REPORT_TIME, sentinelTime);
    prefs->FlushSync();
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleHalfDayMissions();
    int64_t afterTime = prefs->GetLong(LAST_HALF_DAY_REPORT_TIME, 0);
    EXPECT_EQ(afterTime, sentinelTime);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_half_day_missions_test_003, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_COMMON_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    int64_t sentinelTime = 67890;
    prefs->PutLong(LAST_HALF_DAY_REPORT_TIME, sentinelTime);
    prefs->FlushSync();
    dfxManager->dfxWorker_ = nullptr;
    dfxManager->HandleHalfDayMissions();
    int64_t afterTime = prefs->GetLong(LAST_HALF_DAY_REPORT_TIME, 0);
    EXPECT_EQ(afterTime, sentinelTime);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_directory_exist_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    string dirName = "/storage/cloud/files/test";
    dfxManager->IsDirectoryExist(dirName);
    struct stat statInfo {};
    EXPECT_NE(stat(dirName.c_str(), &statInfo), E_SUCCESS);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_five_minute_task_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string bundleName = "bundleName_fivemin_notinit";
    int32_t type = 1;
    dfxManager->dfxCollector_->AddCommonBahavior(bundleName, type);
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleFiveMinuteTask();
    auto commonBehavior = dfxManager->dfxCollector_->GetCommonBehavior();
    EXPECT_EQ(commonBehavior.count(bundleName), 1);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_five_minute_task_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    dfxManager->isInitSuccess_ = false;
    int64_t result = MediaFileUtils::UTCTimeSeconds();
    EXPECT_EQ(dfxManager->HandleMiddleReport(), result);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_one_day_report_test_001, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    dfxManager->isInitSuccess_ = false;
    int64_t result = MediaFileUtils::UTCTimeSeconds();
    EXPECT_EQ(dfxManager->HandleOneDayReport(), result);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_one_day_report_test_002, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string appName = "appName_notinit";
    bool adapted = true;
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleAdaptationToMovingPhoto(appName, adapted);
    auto adaptationInfo = dfxManager->dfxCollector_->GetAdaptationToMovingPhotoInfo();
    EXPECT_EQ(adaptationInfo.adaptedAppPackages.count(appName), 0);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_one_day_report_test_003, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    string appName = "appName_adapted";
    bool adapted = true;
    dfxManager->HandleAdaptationToMovingPhoto(appName, adapted);
    auto adaptationInfo = dfxManager->dfxCollector_->GetAdaptationToMovingPhotoInfo();
    EXPECT_EQ(adaptationInfo.adaptedAppPackages.count(appName), 1);
    EXPECT_EQ(adaptationInfo.unadaptedAppPackages.count(appName), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_001, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::BEGIN)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::INIT_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_002, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::FIRST_FIVE_HUNDRED)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::START_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_003, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::INCREMENT_DOWNLOAD)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::START_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_004, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::TOTAL_DOWNLOAD)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::START_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_005, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::TOTAL_DOWNLOAD_FINISH)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::END_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_sync_state_test_006, TestSize.Level0)
{
    CloudSyncDfxManager* cloudSyncDfxManager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::SYNC_SWITCHED_OFF)));
    cloudSyncDfxManager->InitSyncState();
    EXPECT_EQ(cloudSyncDfxManager->syncState_, SyncState::INIT_STATE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_state_switch_test_001, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::FIRST_FIVE_HUNDRED)));
    EXPECT_EQ(InitState::StateSwitch(*manager), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_state_switch_test_002, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::TOTAL_DOWNLOAD_FINISH)));
    EXPECT_EQ(InitState::StateSwitch(*manager), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_init_state_switch_test_003, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::SYNC_SWITCHED_OFF)));
    EXPECT_EQ(InitState::StateSwitch(*manager), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_start_state_switch_test_001, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::FIRST_FIVE_HUNDRED)));
    EXPECT_EQ(StartState::StateSwitch(*manager), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_start_state_switch_test_002, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::TOTAL_DOWNLOAD_FINISH)));
    EXPECT_EQ(StartState::StateSwitch(*manager), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_start_state_switch_test_003, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::SYNC_SWITCHED_OFF)));
    EXPECT_EQ(StartState::StateSwitch(*manager), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_end_state_switch_test_001, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::FIRST_FIVE_HUNDRED)));
    EXPECT_EQ(EndState::StateSwitch(*manager), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_end_state_switch_test_002, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::TOTAL_DOWNLOAD_FINISH)));
    EXPECT_EQ(EndState::StateSwitch(*manager), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_end_state_switch_test_003, TestSize.Level0)
{
    CloudSyncDfxManager* manager = &CloudSyncDfxManager::GetInstance();
    system::SetParameter(CLOUDSYNC_STATUS_KEY,
        std::to_string(static_cast<int>(CloudSyncStatus::SYNC_SWITCHED_OFF)));
    EXPECT_EQ(EndState::StateSwitch(*manager), true);
    EndState::Process(*manager);
    EXPECT_EQ(manager->timerId_, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_safe_path_test, TestSize.Level0)
{
    std::string path = "/storage/cloud/files/DCIM/123";
    std::string safePath = DfxUtils::GetSafePath(path);
    ASSERT_TRUE(safePath == "*DCIM");
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_join_strings_test, TestSize.Level0)
{
    unordered_set<string> set {"string1", "string2"};

    EXPECT_EQ(DfxUtils::JoinStrings(set, ';'), "string2;string1");
    EXPECT_EQ(DfxUtils::JoinStrings(set, '!'), "string2!string1");
    EXPECT_EQ(DfxUtils::JoinStrings({"string1"}, ';'), "string1");
    EXPECT_EQ(DfxUtils::JoinStrings({}, ';'), "");
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_split_test, TestSize.Level0)
{
    vector<string> result = {};
    string input = "";
    string pattern = "pattern";
    EXPECT_EQ(DfxUtils::Split(input, pattern), result);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_get_safeuri_test, TestSize.Level0)
{
    string safeUri = "";
    string uri = "";
    EXPECT_EQ(DfxUtils::GetSafeUri(uri), safeUri);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_get_safealbumname_test_001, TestSize.Level0)
{
    string albumName = ";";
    uint32_t length = albumName.size();
    string safeAlbumName = GARBLE + albumName.substr(length - GARBLE_LAST_ONE);
    EXPECT_EQ(DfxUtils::GetSafeAlbumName(albumName), safeAlbumName);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_get_safealbumname_test_002, TestSize.Level0)
{
    string albumName = "test";
    uint32_t length = albumName.size();
    string safeAlbumName = GARBLE + albumName.substr(length - GARBLE_LAST_TWO);
    EXPECT_EQ(DfxUtils::GetSafeAlbumName(albumName), safeAlbumName);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_split_string_test, TestSize.Level0)
{
    unordered_set<string> set {"string1", "string2"};

    EXPECT_EQ(DfxUtils::SplitString("string1;string2", ';'), set);
    EXPECT_EQ(DfxUtils::SplitString("string1!string2", '!'), set);
    EXPECT_EQ(DfxUtils::SplitString("string1", ';'), unordered_set<string>{"string1"});
    EXPECT_EQ(DfxUtils::SplitString("", ';'), unordered_set<string>{});
}

HWTEST_F(MediaLibraryDfxTest, medialib_cloud_manager_test, TestSize.Level0)
{
    auto &manager = CloudSyncDfxManager::GetInstance();
    manager.RunDfx();
    int32_t downloadedThumb = 0;
    int32_t generatedThumb = 0;
    int32_t totalDownload = 0;
    DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb);
    DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);

    EXPECT_EQ(downloadedThumb, 0);
    EXPECT_EQ(generatedThumb, 0);
    EXPECT_EQ(totalDownload, 0);

    InitState::StateSwitch(manager);
    EXPECT_EQ(manager.syncState_, SyncState::INIT_STATE);
    InitState::Process(manager);
    EXPECT_EQ(manager.timerId_, 0);
    StartState::StateSwitch(manager);
    EXPECT_EQ(manager.syncState_, SyncState::INIT_STATE);
    StartState::Process(manager);
    EXPECT_EQ(manager.syncState_, SyncState::INIT_STATE);
    EndState::StateSwitch(manager);
    EXPECT_EQ(manager.syncState_, SyncState::INIT_STATE);
    EndState::Process(manager);
    EXPECT_EQ(manager.syncState_, SyncState::INIT_STATE);
    manager.ShutDownTimer();
    EXPECT_EQ(manager.timerId_, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_one_day_report_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    int64_t expected = MediaFileUtils::UTCTimeSeconds();
    int64_t result = dfxManager->HandleOneDayReport();
    EXPECT_GT(result, expected);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportTimeOutOperation_test, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    dfxManager->isInitSuccess_ = false;
    std::string bundleName = "bundleName_timout";
    auto errorsBefore = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(errorsBefore.empty(), true);
    dfxManager->HandleTimeOutOperation(bundleName, 1, 0, 0);
    auto errorsAfter = dfxManager->dfxCollector_->GetThumbnailError();
    EXPECT_EQ(errorsAfter.empty(), true);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportHighMemoryImageThumbnail_test_001, TestSize.Level0)
{
    std::string path = "";
    std::string suffix = "jpg";
    int32_t width = 4096;
    int32_t height = 4096;
    DfxReporter dfxReporter;
    EXPECT_EQ(dfxReporter.ReportHighMemoryImageThumbnail(path,
        suffix, width, height), COMMON_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportHighMemoryImageThumbnail_test_002, TestSize.Level0)
{
    std::string path = "";
    std::string suffix = "png";
    int32_t width = 1;
    int32_t height = 1;
    DfxReporter dfxReporter;
    EXPECT_EQ(dfxReporter.ReportHighMemoryImageThumbnail(path,
        suffix, width, height), OTHER_FORMAT_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportHighMemoryImageThumbnail_test_003, TestSize.Level0)
{
    std::string path = "";
    std::string suffix = "jpg";
    int32_t width = 4097;
    int32_t height = 4097;
    DfxReporter dfxReporter;
    EXPECT_EQ(dfxReporter.ReportHighMemoryImageThumbnail(path,
        suffix, width, height), BIG_IMAGE);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportHighMemoryVideoThumbnail_test_001, TestSize.Level0)
{
    std::string path = "";
    std::string suffix = "mp4";
    int32_t width = 7680;
    int32_t height = 7680;
    DfxReporter dfxReporter;
    EXPECT_EQ(dfxReporter.ReportHighMemoryVideoThumbnail(path,
        suffix, width, height), BIG_VIDEO);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportHighMemoryVideoThumbnail_test_002, TestSize.Level0)
{
    std::string path = "";
    std::string suffix = "mp4";
    int32_t width = 4096;
    int32_t height = 4096;
    DfxReporter dfxReporter;
    EXPECT_EQ(dfxReporter.ReportHighMemoryVideoThumbnail(path,
        suffix, width, height), COMMON_VIDEO);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportDeleteBehavior_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DELETE_BEHAVIOR_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    std::string testkey = "testBundle_del_001|1";
    int64_t testTimes = 5;
    prefs->PutInt(testkey, testTimes);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt(testkey, 0), testTimes);

    DfxReporter dfxReporter;
    dfxReporter.ReportDeleteStatistic();
    EXPECT_EQ(prefs->GetInt(testkey, 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportDeleteBehavior_test_002, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DELETE_BEHAVIOR_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    std::string key1 = "testBundle_del_002a|2";
    std::string key2 = "testBundle_del_002b|3";
    int64_t times1 = 3;
    int64_t times2 = 7;
    prefs->PutInt(key1, times1);
    prefs->PutInt(key2, times2);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt(key1, 0), times1);
    EXPECT_EQ(prefs->GetInt(key2, 0), times2);

    DfxReporter dfxReporter;
    dfxReporter.ReportDeleteStatistic();
    EXPECT_EQ(prefs->GetInt(key1, 0), 0);
    EXPECT_EQ(prefs->GetInt(key2, 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportPhotoInfo_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DELETE_BEHAVIOR_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutInt("testBundle_photo_001", 5);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt("testBundle_photo_001", 0), 5);

    DfxReporter dfxReporter;
    dfxReporter.ReportCommonBehavior();
    EXPECT_EQ(prefs->GetInt("testBundle_photo_001", 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAstcInfo_test_001, TestSize.Level0)
{
    MediaLibraryAstcStat::GetInstance().SetDoubleUpgradeInfo(100, "test_uid");
    string jsonBefore = MediaLibraryAstcStat::GetInstance().GetJson();
    EXPECT_NE(jsonBefore.find("\"uid\":\"test_uid\""), string::npos);
    LcdAndAstcCount count = {};
    count.localLcdCount = 100;
    count.localAstCount = 50;
    count.cloudLcdCount = 30;
    count.cloudAstCount = 20;
    DfxReporter dfxReporter;
    dfxReporter.ReportAstcInfo(count);

    string jsonAfter = MediaLibraryAstcStat::GetInstance().GetJson();
    EXPECT_EQ(jsonAfter.find("\"uid\":\"test_uid\""), string::npos);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAlbumInfo_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(INVALID_BEHAVIOR_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    std::string testkey = "testBundle_album_001|1";
    prefs->PutString(testkey, "/data/test/invalid_album");
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetString(testkey, ""), "/data/test/invalid_album");

    DfxReporter dfxReporter;
    dfxReporter.ReportInvalidBehvior();

    EXPECT_EQ(prefs->GetString(testkey, ""), "");
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportDirtyCloudPhoto_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(THUMBNAIL_ERROR_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    std::string testkey = "/data/test/thumb_dirty|1|1001";
    std::string testValue = "12345";
    prefs->PutString(testkey, testValue);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetString(testkey, ""), testValue);

    DfxReporter dfxReporter;
    dfxReporter.ReportThumbnailError();

    EXPECT_EQ(prefs->GetString(testkey, ""), "");
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportCommonVersion_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(COMMON_BEHAVIOR_FOR_FUSE_GETATTER_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutInt("testBundle_version_001", 3);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt("testBundle_version_001", 0), 3);

    DfxReporter dfxReporter;
    dfxReporter.ReportCommonBehaviorForFuseGetAttr();
    EXPECT_EQ(prefs->GetInt("testBundle_version_001", 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAnalysisVersion_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(COMMON_BEHAVIOR_FOR_FUSE_OPEN_TRUNCATE_XML, true),
                     errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutInt("testBundle_analysis_001", 5);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt("testBundle_analysis_001", 0), 5);

    DfxReporter dfxReporter;
    dfxReporter.ReportCommonBehaviorForFuseOpenTruncate();

    EXPECT_EQ(prefs->GetInt("testBundle_analysis_001", 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportStartResult_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(ADAPTATION_TO_MOVING_PHOTO_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutInt(MOVING_PHOTO_KEY_UNADAPTED_NUM, 2);
    prefs->PutString(MOVING_PHOTO_KEY_UNADAPTED_PACKAGE, "com.test.unadapted");
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt((MOVING_PHOTO_KEY_UNADAPTED_NUM, 0), 2);
    EXPECT_EQ(prefs->GetString((MOVING_PHOTO_KEY_UNADAPTED_PACKAGE, ""), "com.test.unadapted");

    DfxReporter dfxReporter;
    dfxReporter.ReportAdaptationToMovingPhoto();
    
    EXPECT_EQ(prefs->GetInt((MOVING_PHOTO_KEY_UNADAPTED_NUM, 0), 0);
    EXPECT_EQ(prefs->GetString((MOVING_PHOTO_KEY_UNADAPTED_PACKAGE, ""), "");
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportCloudSyncThumbGenerationStatus_test_001, TestSize.Level0)
{
    int32_t downloadedThumb = 1;
    int32_t generatedThumb = 1;
    int32_t totalDownload = 0;
    int32_t southDeviceType = 0;
    DfxReporter dfxReporter;
    int32_t result = dfxReporter.ReportCloudSyncThumbGenerationStatus(downloadedThumb,
        generatedThumb, totalDownload, southDeviceType);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportCloudSyncThumbGenerationStatus_test_002, TestSize.Level0)
{
    int32_t downloadedThumb = 1;
    int32_t generatedThumb = 1;
    int32_t totalDownload = 1;
    int32_t southDeviceType = 0;
    DfxReporter dfxReporter;
    int32_t result = dfxReporter.ReportCloudSyncThumbGenerationStatus(downloadedThumb,
        generatedThumb, totalDownload, southDeviceType);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportMedialibraryAPI_test_001, TestSize.Level0)
{
    string callerPackage = "";
    string saveUri = "saveUri";
    DfxReporter dfxReporter;
    int32_t result = dfxReporter.ReportMedialibraryAPI(callerPackage, saveUri);
    EXPECT_EQ(result, E_SUCCESS);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportCustomRestoreFusion_test_001, TestSize.Level0)
{
    CustomRestoreDfxDataPoint dfxDataPoint;
    dfxDataPoint.customRestorePackageName = "customRestorePackageName";
    dfxDataPoint.albumLPath = "albumLPath";
    dfxDataPoint.keyPath = "keyPath";
    dfxDataPoint.totalNum = 11;
    dfxDataPoint.successNum = 12;
    dfxDataPoint.failedNum = 13;
    dfxDataPoint.sameNum = 14;
    dfxDataPoint.cancelNum = 16;
    dfxDataPoint.totalTime = 102221;
    DfxReporter dfxReporter;
    int32_t result = dfxReporter.ReportCustomRestoreFusion(dfxDataPoint);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryFromPhotos_test_001, TestSize.Level0)
{
    int32_t mediaType = 1;
    int32_t position = 1;
    DfxDatabaseUtils dfxDatabaseUtils;
    int32_t result = dfxDatabaseUtils.QueryFromPhotos(mediaType, position);
    EXPECT_EQ(result, E_SUCCESS);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportCinematicInfo_test_001, TestSize.Level0)
{
    DfxReporter dfxReporter;
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_CINEMATIC_VIDEO_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES, 5);
    prefs->FlushSync();
    EXPECT_EQ(prefs->GetInt((CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES, 0), 5);

    dfxReporter.ReportCinematicVideo();
    EXPECT_EQ(prefs->GetInt((CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES, 0), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_cinematic_caul_waittime_test_001, TestSize.Level0)
{
    DfxAnalyzer dfxAnalyzer;
    CinematicVideoInfo mockVideoInfo;

    mockVideoInfo.accessTimesLow = 1;
    mockVideoInfo.accessTimesHigh = 2;
    mockVideoInfo.uriAccessTimesLow = 3;
    mockVideoInfo.uriAccessTimesHigh = 4;
    mockVideoInfo.multistageSuccessTimes = 5;
    mockVideoInfo.multistageFailedTimes = 0;
    mockVideoInfo.cancelWaitTimeMap["videoidcancel1"] = {100, 250};  // startTime=100, endTime=250
    mockVideoInfo.cancelWaitTimeMap["videoidcancel2"] = {300, 500};
    mockVideoInfo.processWaitTimeMap["videoidprocess1"] = {100, 200};

    int32_t oldNum = 0;
    int32_t oldWaitAvgTime = 0;

    int32_t avgWaitTime = dfxAnalyzer.CalculateAvgWaitTime(CinematicWaitType::CANCEL_CINEMATIC,
        mockVideoInfo, oldNum, oldWaitAvgTime);
    EXPECT_EQ(avgWaitTime, 175);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_cinematic_caul_waittime_test_002, TestSize.Level0)
{
    DfxAnalyzer dfxAnalyzer;
    CinematicVideoInfo mockVideoInfo;

    mockVideoInfo.accessTimesLow = 1;
    mockVideoInfo.accessTimesHigh = 2;
    mockVideoInfo.uriAccessTimesLow = 3;
    mockVideoInfo.uriAccessTimesHigh = 4;
    mockVideoInfo.multistageSuccessTimes = 5;
    mockVideoInfo.multistageFailedTimes = 0;
    mockVideoInfo.cancelWaitTimeMap["videoidcancel1"] = {1000, 2000};  // startTime=1000, endTime=2000
    mockVideoInfo.processWaitTimeMap["videoidprocess1"] = {500, 700};
    mockVideoInfo.processWaitTimeMap["videoidprocess2"] = {9000, 8000};

    int32_t oldNum = 8;
    int32_t oldWaitAvgTime = 225; // 1800

    int32_t avgWaitTime = dfxAnalyzer.CalculateAvgWaitTime(CinematicWaitType::PROCESS_CINEMATIC,
        mockVideoInfo, oldNum, oldWaitAvgTime);
    EXPECT_EQ(avgWaitTime, 196);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_001, TestSize.Level0)
{
    DfxCollector dfxCollector;
    dfxCollector.CollectCinematicVideoAccessTimes(true, true);   // ByUri, HighQaulity
    dfxCollector.CollectCinematicVideoAccessTimes(false, true);  // notByUri, HighQaulity
    dfxCollector.CollectCinematicVideoAccessTimes(true, false);  // ByUri, LowQaulity
    dfxCollector.CollectCinematicVideoAccessTimes(false, false); // notByUri, LowQaulity

    CinematicVideoInfo mockVideoInfo = dfxCollector.GetCinematicVideoInfo();

    EXPECT_EQ(mockVideoInfo.accessTimesLow, 1);
    EXPECT_EQ(mockVideoInfo.accessTimesHigh, 1);
    EXPECT_EQ(mockVideoInfo.uriAccessTimesLow, 1);
    EXPECT_EQ(mockVideoInfo.uriAccessTimesHigh, 1);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_002, TestSize.Level0)
{
    DfxCollector dfxCollector;
    int64_t expectedCancelStart = MediaFileUtils::UTCTimeMilliSeconds();
    dfxCollector.CollectCinematicVideoAddStartTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel4");
    int64_t expectedProcessStart = MediaFileUtils::UTCTimeMilliSeconds();
    dfxCollector.CollectCinematicVideoAddStartTime(CinematicWaitType::PROCESS_CINEMATIC, "videoidprocess4");

    CinematicVideoInfo mockVideoInfo = dfxCollector.GetCinematicVideoInfo();

    EXPECT_EQ(mockVideoInfo.cancelWaitTimeMap.count("videoidcancel4"), 1);
    EXPECT_EQ(mockVideoInfo.processWaitTimeMap.count("videoidprocess4"), 1);
    EXPECT_EQ(static_cast<int64_t>(mockVideoInfo.cancelWaitTimeMap["videoidcancel4"].startTime),
         expectedCancelStart);
    EXPECT_EQ(static_cast<int64_t>(mockVideoInfo.cancelWaitTimeMap["videoidprocess4"].startTime),
         expectedProcessStart);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_003, TestSize.Level0)
{
    DfxCollector dfxCollector;
    dfxCollector.CollectCinematicVideoAddEndTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel5");
    dfxCollector.CollectCinematicVideoAddEndTime(CinematicWaitType::PROCESS_CINEMATIC, "videoidprocess5");

    CinematicVideoInfo mockVideoInfo = dfxCollector.GetCinematicVideoInfo();

    EXPECT_EQ(mockVideoInfo.cancelWaitTimeMap.count("videoidcancel5"), 1);
    EXPECT_EQ(mockVideoInfo.processWaitTimeMap.count("videoidprocess5"), 1);
    EXPECT_EQ(mockVideoInfo.cancelWaitTimeMap["videoidcancel5"].startTime, 0);
    EXPECT_EQ(mockVideoInfo.processWaitTimeMap["videoidprocess5"].startTime, 0);
    EXPECT_EQ(mockVideoInfo.cancelWaitTimeMap.count("videoidprocess5"), 0);
    EXPECT_EQ(mockVideoInfo.processWaitTimeMap.count("videoidcancel5"), 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_004, TestSize.Level0)
{
    DfxCollector dfxCollector;
    dfxCollector.CollectCinematicVideoMultistageResult(true);  // success
    dfxCollector.CollectCinematicVideoMultistageResult(false); // failed
    dfxCollector.CollectCinematicVideoMultistageResult(false);

    CinematicVideoInfo mockVideoInfo = dfxCollector.GetCinematicVideoInfo();

    EXPECT_EQ(mockVideoInfo.multistageSuccessTimes, 1);
    EXPECT_EQ(mockVideoInfo.multistageFailedTimes, 2);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_005, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    dfxManager->isInitSuccess_ = false;
    dfxManager->HandleCinematicVideoAccessTimes(true, true);
    dfxManager->HandleCinematicVideoAddStartTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel1");
    dfxManager->HandleCinematicVideoAddEndTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel1");
    dfxManager->HandleCinematicVideoMultistageResult(true);
    CinematicVideoInfo info = dfxManager->dfxCollector_->GetCinematicVideoInfo();
    EXPECT_EQ(info.accessTimesHigh, 0);
    EXPECT_EQ(info.accessTimesLow, 0);
    EXPECT_EQ(info.multistageSuccessTimes, 0);
    EXPECT_EQ(info.cancelWaitTimeMap.count("videoidcancel1"), 0);
    EXPECT_EQ(dfxManager->isInitSuccess_, false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_collection_cinematic_videoinfo_test_006, TestSize.Level0)
{
    auto dfxManager = DfxManager::GetInstance();
    ASSERT_NE(dfxManager, nullptr);
    dfxManager->HandleCinematicVideoAccessTimes(true, true);
    dfxManager->HandleCinematicVideoAddStartTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel1");
    dfxManager->HandleCinematicVideoAddEndTime(CinematicWaitType::CANCEL_CINEMATIC, "videoidcancel1");
    dfxManager->HandleCinematicVideoMultistageResult(true);
    CinematicVideoInfo info = dfxManager->dfxCollector_->GetCinematicVideoInfo();
    EXPECT_EQ(info.accessTimesHigh, 1);
    EXPECT_EQ(info.multistageSuccessTimes, 1);
    EXPECT_EQ(info.cancelWaitTimeMap.count("videoidcancel1"), 1);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryAlbumInfoBySubtype_test_001, TestSize.Level0)
{
    int32_t albumSubtype = 1;
    DfxDatabaseUtils dfxDatabaseUtils;
    EXPECT_EQ(dfxDatabaseUtils.QueryAlbumInfoBySubtype(albumSubtype).count, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryDirtyCloudPhoto_test_001, TestSize.Level0)
{
    vector<PhotoInfo> photoInfoList;
    DfxDatabaseUtils dfxDatabaseUtils;
    dfxDatabaseUtils.QueryDirtyCloudPhoto();
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, 1);
    predicates.NotEqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t> (DirtyType::TYPE_SYNCED));
    predicates.Limit(DIRTY_PHOTO_COUNT);
    std::vector<std::string> columns = { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_DIRTY,
        PhotoColumn::PHOTO_CLOUD_ID };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    EXPECT_NE(resultSet, nullptr);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryAnalysisVersion_test_001, TestSize.Level0)
{
    std::string table = "";
    std::string column = "";
    DfxDatabaseUtils dfxDatabaseUtils;
    int32_t result = dfxDatabaseUtils.QueryAnalysisVersion(table, column);
    EXPECT_EQ(result, E_SUCCESS);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryDouble_test_001, TestSize.Level0)
{
    NativeRdb::AbsRdbPredicates dirAbsPred(PhotoColumn::PHOTOS_TABLE);
    dirAbsPred.EqualTo(CONST_MEDIA_DATA_DB_ID, to_string(0));
    vector<string> columns = {};
    std::string queryColumn = "queryColumn";
    double value = 1;
    DfxDatabaseUtils dfxDatabaseUtils;
    int32_t result = dfxDatabaseUtils.QueryDouble(dirAbsPred, columns, queryColumn, value);
    EXPECT_EQ(result, E_DB_FAIL);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_QueryLCDThumb_test_001, TestSize.Level0)
{
    DfxDatabaseUtils dfxDatabaseUtils;
    int32_t result_1 = dfxDatabaseUtils.QueryLCDThumb(true);
    EXPECT_EQ(result_1, 0);
    int32_t result_2 = dfxDatabaseUtils.QueryLCDThumb(true);
    EXPECT_EQ(result_2, 0);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_AddCommonBahavior_test_001, TestSize.Level0)
{
    int32_t type = 1;
    string bundleName = "test";
    DfxCollector dfxCollector;
    dfxCollector.AddCommonBahavior(bundleName, type);
    dfxCollector.AddCommonBahavior(bundleName, type);
    EXPECT_EQ(dfxCollector.commonBehaviorMap_[bundleName].times, 2);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_CollectDeleteBehavior_test_001, TestSize.Level0)
{
    string bundleName = "medialib_dfx";
    int32_t type = DfxType::TRASH_PHOTO;
    int32_t size = 10;
    DfxCollector dfxCollector;
    dfxCollector.CollectDeleteBehavior(bundleName, type, size);
    EXPECT_EQ(dfxCollector.deleteToTrashMap_[bundleName], 1);
    type = DfxType::ALBUM_DELETE_ASSETS;
    dfxCollector.CollectDeleteBehavior(bundleName, type, size);
    EXPECT_EQ(dfxCollector.deleteToTrashMap_[bundleName], 1);
    type = DfxType::ALBUM_REMOVE_PHOTOS;
    dfxCollector.CollectDeleteBehavior(bundleName, type, size);
    EXPECT_EQ(dfxCollector.deleteToTrashMap_[bundleName], 1);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_CollectAdaptationToMovingPhotoInfo_test_001, TestSize.Level0)
{
    string appName = "appName";
    DfxCollector dfxCollector;
    dfxCollector.CollectAdaptationToMovingPhotoInfo(appName, true);
    auto result = dfxCollector.adaptationToMovingPhotoInfo_.adaptedAppPackages;
    EXPECT_NE(result.find(appName), result.end());
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_CollectAdaptationToMovingPhotoInfo_test_002, TestSize.Level0)
{
    string appName = "medialib";
    DfxCollector dfxCollector;
    dfxCollector.CollectAdaptationToMovingPhotoInfo(appName, false);
    auto result = dfxCollector.adaptationToMovingPhotoInfo_.unadaptedAppPackages;
    EXPECT_NE(result.find(appName), result.end());
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_FlushCommonBehavior_test_001, TestSize.Level0)
{
    string bundleName = "medialib_dfx";
    std::unordered_map<string, CommonBehavior> commonBehaviorMap;
    CommonBehavior commonBehavior = { 0 };
    commonBehaviorMap[bundleName] = commonBehavior;
    DfxAnalyzer dfxAnalyzer;
    dfxAnalyzer.FlushCommonBehavior(commonBehaviorMap);
    EXPECT_EQ(commonBehaviorMap.empty(), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_FlushCommonBehavior_test_002, TestSize.Level0)
{
    string bundleName = "";
    std::unordered_map<string, CommonBehavior> commonBehaviorMap;
    CommonBehavior commonBehavior = { 0 };
    commonBehaviorMap[bundleName] = commonBehavior;
    DfxAnalyzer dfxAnalyzer;
    dfxAnalyzer.FlushCommonBehavior(commonBehaviorMap);
    EXPECT_EQ(commonBehaviorMap.empty(), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_FlushDeleteBehavior_test_001, TestSize.Level0)
{
    string bundleName = "medialib_dfx";
    std::unordered_map<string, int32_t> deleteBehaviorMap;
    deleteBehaviorMap[bundleName] = 0;
    int32_t type = 1;
    DfxAnalyzer dfxAnalyzer;
    dfxAnalyzer.FlushDeleteBehavior(deleteBehaviorMap, type);
    EXPECT_EQ(deleteBehaviorMap.empty(), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_FlushAdaptationToMovingPhoto_test_001, TestSize.Level0)
{
    string bundleName = "medialib_dfx";
    AdaptationToMovingPhotoInfo newAdaptationInfo;
    newAdaptationInfo.unadaptedAppPackages.emplace(bundleName);
    DfxAnalyzer dfxAnalyzer;
    dfxAnalyzer.FlushAdaptationToMovingPhoto(newAdaptationInfo);
    EXPECT_EQ(newAdaptationInfo.unadaptedAppPackages.empty(), false);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_test_001, TestSize.Level0)
{
    std::string photoMimeType;
    DfxDatabaseUtils::GetPhotoMimeType(photoMimeType);
    QuerySizeAndResolution queySizeAndResolution;
    DfxDatabaseUtils::GetSizeAndResolutionInfo(queySizeAndResolution);
    bool ret = DfxDatabaseUtils::CheckChargingAndScreenOff(true);
    EXPECT_FALSE(ret);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_test_002, TestSize.Level0)
{
    QuerySizeAndResolution querySizeAndResolution;
    querySizeAndResolution.localImageSize = "";
    querySizeAndResolution.localVideoSize = "";
    querySizeAndResolution.cloudImageSize = "";
    querySizeAndResolution.cloudVideoSize = "";
    querySizeAndResolution.localImageResolution = "";
    querySizeAndResolution.localVideoResolution = "";
    querySizeAndResolution.cloudImageResolution = "";
    querySizeAndResolution.cloudVideoResolution = "";
    std::string photoMimeType = "";
    DfxReporter dfxReporter;
    dfxReporter.ReportPhotoSizeAndResolutionInfo(querySizeAndResolution, photoMimeType);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "PHOTO_INFO_EXT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "LOCAL_IMAGE_SIZE", querySizeAndResolution.localImageSize,
        "LOCAL_VIDEO_SIZE", querySizeAndResolution.localVideoSize,
        "CLOUD_IMAGE_SIZE", querySizeAndResolution.cloudImageSize,
        "CLOUD_VIDEO_SIZE", querySizeAndResolution.cloudVideoSize,
        "PHOTO_MIMETYPE", photoMimeType,
        "LOCAL_IMAGE_RESOLUTION", querySizeAndResolution.localImageResolution,
        "LOCAL_VIDEO_RESOLUTION", querySizeAndResolution.localVideoResolution,
        "CLOUD_IMAGE_RESOLUTION", querySizeAndResolution.cloudImageResolution,
        "CLOUD_VIDEO_RESOLUTION", querySizeAndResolution.cloudVideoResolution);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAncoCheckInfo_test_001, TestSize.Level0)
{
    AncoCheckInfo reportData;
    reportData.checkStartTime = 111;
    reportData.checkEndTime = 211;
    reportData.checkAdd = 24;
    reportData.checkUpdate = 121;
    reportData.checkDelete = 78;
    int32_t result = DfxReporter::ReportAncoCheckInfo(reportData);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAncoOperationChangeInfo_test_001, TestSize.Level0)
{
    AncoOperationChangeInfo reportData;
    reportData.photoOptAddCount = 41;
    reportData.photoOptUpdateCount = 42;
    reportData.photoOptDeleteCount = 43;
    reportData.albumOptAddCount = 44;
    reportData.albumOptUpdateCount = 45;
    reportData.albumOptDeleteCount = 46;
    reportData.totalOptCount = 147;
    int32_t result = DfxReporter::ReportAncoOperationChangeInfo(reportData);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryDfxTest, medialib_dfx_ReportAncoCountFormatInfo_test_001, TestSize.Level0)
{
    AncoCountFormatInfo reportData;
    reportData.loadStartTime = 41;
    reportData.loadEndTime = 42;
    reportData.albumCount = 48;
    reportData.imageCount = 46;
    reportData.videoCount = 44;
    map<std::string, int32_t> formatCountMap = {
        {"gif", 1},
        {"jpg", 2},
        {"png", 4},
        {"mp4", 11},
        {"3gp", 12},
        {"avi", 13},
        {"rmvb", 14},
    };
    nlohmann::json staticsJson(formatCountMap);
    reportData.assetFormatDistribution = staticsJson.dump();
    int32_t result = DfxReporter::ReportAncoCountFormatInfo(reportData);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryDfxTest, AncoDfxManager_test_001, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_COMMON_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    prefs->PutBool("anco_first_load_flag", false);
    prefs->FlushSync();
    AncoDfxManager::GetInstance().ReportFirstLoadInfo(100, 200);
    EXPECT_EQ(prefs->GetBool(("anco_first_load_flag", false), true);
    AncoDfxManager::GetInstance().ReportFirstLoadInfo(300, 400);
    EXPECT_EQ(prefs->GetBool(("anco_first_load_flag", false), true);
}

HWTEST_F(MediaLibraryDfxTest, AncoDfxManager_test_003, TestSize.Level0)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
            NativePreferences::PreferencesHelper::GetPreferences(
                MediaPathUtils::AdaptPerferencesPathForAllDevice(DFX_COMMON_XML, true), errCode);
    ASSERT_NE(prefs, nullptr);
    int64_t sentinelTime = 1;
    prefs->PutLong("anco_opt_change_info_last_report_time", sentinelTime);
    prefs->FlushSync();
    int64_t expectedNow = MediaFileUtils::UTCTimeSeconds();
    AncoDfxManager::GetInstance().InnerReportAndResetOptChangeInfo();
    int64_t afterTime = prefs->GetLong("anco_opt_change_info_last_report_time", 0);
    EXPECT_EQ(afterTime, expectedNow);
}

HWTEST_F(MediaLibraryDfxTest, QueryAlbumNames_dfx_test_001, TestSize.Level0)
{
    InsertPhotoAlbum("album1", PhotoAlbumType::SOURCE, 0);
    InsertPhotoAlbum("album2", PhotoAlbumType::SOURCE, 1);
    InsertPhotoAlbum("album3", PhotoAlbumType::USER, 0);
    InsertPhotoAlbum("album4", PhotoAlbumType::USER, 1);
    std::vector<std::string> supportedAlbumNames = DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(1);
    EXPECT_EQ(supportedAlbumNames.size(), 1);
    std::vector<std::string> notSupportedAlbumNames = DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(0);
    EXPECT_EQ(notSupportedAlbumNames.size(), 1);
}

HWTEST_F(MediaLibraryDfxTest, QueryAlbumNames_dfx_test_002, TestSize.Level0)
{
    int32_t maxAlbumCount = 55;
    for (int32_t i = 0; i < maxAlbumCount; i++) {
        InsertPhotoAlbum("album" + to_string(i), PhotoAlbumType::SOURCE, 1);
    }
    std::vector<std::string> supportedAlbumNames = DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(1);
    EXPECT_EQ(supportedAlbumNames.size(), 2);
}

HWTEST_F(MediaLibraryDfxTest, DfxSystemPhotoKeys_ReportIfSystemKey_Test_01, TestSize.Level0)
{
    MEDIA_INFO_LOG("DfxSystemPhotoKeys_ReportIfSystemKey_Test_01 Start");
    DfxSystemPhotoKeys::reportedKeyMap_.EnsureInsert(PhotoColumn::PHOTO_DIRTY + "/napi", 1);

    std::vector<std::pair<std::string, int32_t>> testCases{
        {MediaColumn::MEDIA_NAME, E_SUCCESS},
        {MediaColumn::MEDIA_HIDDEN, E_INVALID_BUNDLENAME},
        {MediaColumn::MEDIA_SIZE, E_SUCCESS},
        {PhotoColumn::PHOTO_HIDDEN_TIME, E_INVALID_BUNDLENAME},
        {PhotoColumn::PHOTO_DIRTY, E_SUCCESS},
    };

    for (const auto &testCase : testCases) {
        std::string photoKey = testCase.first;
        int32_t expect = testCase.second;
        int32_t actual = DfxSystemPhotoKeys::ReportIfSystemKey("napi", photoKey);
        MEDIA_INFO_LOG("photoKey: %{public}s, Actual: %{public}d", photoKey.c_str(), actual);
        EXPECT_EQ(actual, expect);
    }

    DfxSystemPhotoKeys::reportedKeyMap_.Clear();
    MEDIA_INFO_LOG("DfxSystemPhotoKeys_ReportIfSystemKey_Test_01 end");
}
} // namespace Media
} // namespace OHOS