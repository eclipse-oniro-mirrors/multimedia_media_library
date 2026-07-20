/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudAssetDownloadUnitTest"

#include "medialibrary_cloud_asset_download_test.h"

#include <chrono>
#include <thread>

#include "image_source.h"
#include "media_exif.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "values_bucket.h"

#define private public
#define protected public
#include "file_utils.h"
#include "cloud_media_asset_manager.h"
#include "cloud_media_asset_download_operation.h"
#include "cloud_media_asset_callback.h"
#include "cloud_media_asset_observer.h"
#include "cloud_media_asset_types.h"
#undef private
#undef protected
#include "cloud_media_asset_uri.h"
#include "data_secondary_directory_uri.h"
#include "parameters.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> number(0);
static const int32_t EXIT_TASK = 1;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t ALBUM_FROMCLOUD = 2;

static const std::string CLOUD_RETIAN_STATUS_KEY = "persist.multimedia.medialibrary.retain.cloud.status";
static const std::string HDC_RETIAN_STATUS_KEY = "persist.multimedia.medialibrary.retain.hdc.status";
static const std::string MEIDA_RESTORE_FLAG = "multimedia.medialibrary.restoreFlag";
static const std::string MEIDA_BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
static const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status"; // ms
static const int64_t INVALID_TIME_STAMP = -1;
static const int64_t DEFAULT_TIME_STAMP = 0;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        CONST_MEDIALIBRARY_TABLE,
        PhotoAlbumColumns::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    ::system("rm -rf /storage/cloud/files/*");
    ::system("rm -rf /storage/cloud/files/.thumbs");
    ::system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

int GetNumber()
{
    return ++number;
}

std::string GetTitle(int64_t &timestamp)
{
    return "IMG_" + to_string(timestamp) + "_" + to_string(GetNumber());
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return seconds.count() + GetNumber();
}

int32_t InsertCloudAssetINDb(int64_t &fileId, std::string &data)
{
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    data = "/storage/cloud/files/photo/1/" + title + ".jpg";
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    valuesBucket.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertCloudAsset fileId is %{public}s", to_string(fileId).c_str());
    return ret;
}

int32_t InsertCloudAndLocalAssetINDb(int64_t &fileId, std::string &data)
{
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    data = "/storage/cloud/files/photo/1/" + title + ".jpg";
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertCloudAndLocalAsset fileId is %{public}s", to_string(fileId).c_str());
    return ret;
}

int32_t SetPosition(const int64_t fileId)
{
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t rows = 0;
    int32_t ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    return ret;
}

int32_t InsertCloudAlbumINDb()
{
    int64_t albumId = 0;
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_IS_LOCAL, ALBUM_FROMCLOUD);
    int32_t ret = g_rdbStore->Insert(albumId, PhotoAlbumColumns::TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Insert cloud albumId is %{public}s", to_string(albumId).c_str());
    return ret;
}

void MediaLibraryCloudAssetDownloadTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();

    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.GET_NETWORK_INFO");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryCloudAssetDownloadTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    ::system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");

    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibraryCloudAssetDownloadTest::SetUp()
{
    ASSERT_NE(g_rdbStore, nullptr);
    ClearAndRestart();
}

void MediaLibraryCloudAssetDownloadTest::TearDown(void)
{
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(CLOUDSYNC_SWITCH_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_001 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_001 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_002 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = instance.PauseDownloadCloudAsset(CloudMediaTaskPauseCause::USER_PAUSED);
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    ret = instance.PauseDownloadCloudAsset(CloudMediaTaskPauseCause::USER_PAUSED);
    EXPECT_EQ(ret, E_OK);
    operation = nullptr;
    ret = instance.PauseDownloadCloudAsset(CloudMediaTaskPauseCause::USER_PAUSED);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_003 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = instance.CancelDownloadCloudAsset();
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    ret = instance.CancelDownloadCloudAsset();
    EXPECT_EQ(ret, E_OK);
    operation = nullptr;
    ret = instance.CancelDownloadCloudAsset();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_003 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_004 Start");
    std::vector<std::string> idVec;
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    idVec.push_back(std::to_string(fileId1));

    int64_t fileId2 = 0;
    std::string data2 = "";
    ret = InsertCloudAssetINDb(fileId2, data2);
    EXPECT_EQ(ret, E_OK);
    idVec.push_back(std::to_string(fileId2));

    int64_t fileId3 = 0;
    std::string data3 = "";
    ret = InsertCloudAssetINDb(fileId3, data3);
    EXPECT_EQ(ret, E_OK);
    idVec.push_back(std::to_string(fileId3));

    ret = CloudMediaAssetManager::GetInstance().DeleteBatchCloudFile(idVec);
    EXPECT_EQ(ret, E_OK);
    idVec.clear();
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_004 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_005 Start");
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    Uri uriStartForce(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_FORCE);
    MediaLibraryCommand cmdStartForce(uriStartForce);
    int32_t ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdStartForce);
    EXPECT_EQ(ret, E_ERR);
    Uri uriStartGentle(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_GENTLE);
    MediaLibraryCommand cmdStartGentle(uriStartGentle);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdStartGentle);
    EXPECT_EQ(ret, E_OK);
    Uri uriPause(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_PAUSE);
    MediaLibraryCommand cmdPause(uriPause);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdPause);
    EXPECT_EQ(ret, E_OK);
    Uri uriCancel(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_CANCEL);
    MediaLibraryCommand cmdCancel(uriCancel);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdCancel);
    EXPECT_EQ(ret, E_OK);
    Uri uriRetain(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE);
    MediaLibraryCommand cmdRetain(uriRetain);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdRetain);
    EXPECT_EQ(ret, E_OK);
    Uri uriOther(CONST_MEDIALIBRARY_AUDIO_URI);
    MediaLibraryCommand cmdOther(uriOther);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetUpdateOperations(cmdOther);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_005 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_006 Start");
    Uri uriType(CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY);
    MediaLibraryCommand cmdType(uriType);
    std::string ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetGetTypeOperations(cmdType);
    EXPECT_EQ(ret, "2,0,0,0,0,0");
    Uri uriOther(CONST_MEDIALIBRARY_AUDIO_URI);
    MediaLibraryCommand cmdOther(uriOther);
    ret = CloudMediaAssetManager::GetInstance().HandleCloudMediaAssetGetTypeOperations(cmdOther);
    EXPECT_EQ(ret, "");
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_006 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_007 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.CheckDownloadTypeOfTask(CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(ret, E_OK);
    ret = instance.CheckDownloadTypeOfTask(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_OK);
    CloudMediaDownloadType type = static_cast<CloudMediaDownloadType>(-1);
    ret = instance.CheckDownloadTypeOfTask(type);
    EXPECT_EQ(ret, E_ERR);
    type = static_cast<CloudMediaDownloadType>(10);
    ret = instance.CheckDownloadTypeOfTask(type);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_007 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_008 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    CloudMediaDownloadType type = static_cast<CloudMediaDownloadType>(-1);
    int32_t ret = instance.StartDownloadCloudAsset(type);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_NE(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_ERR);
    CloudMediaAssetTaskStatus status = static_cast<CloudMediaAssetTaskStatus>(-1);
    operation->taskStatus_ = status;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_008 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_009 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.CheckDownloadTypeOfTask(CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(ret, E_OK);
    ret = instance.CheckDownloadTypeOfTask(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_OK);
    CloudMediaDownloadType type = static_cast<CloudMediaDownloadType>(-1);
    ret = instance.CheckDownloadTypeOfTask(type);
    EXPECT_EQ(ret, E_ERR);
    type = static_cast<CloudMediaDownloadType>(10);
    ret = instance.CheckDownloadTypeOfTask(type);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_009 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_010 Start");
    std::vector<std::string> idVec = { "id1", "id2" };
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.DeleteBatchCloudFile(idVec);
    EXPECT_EQ(ret, E_ERR);
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    ret = instance.ForceRetainDownloadCloudMedia(CloudMediaRetainType::RETAIN_FORCE);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_010 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_011 Start");
    std::string result = "2,0,0,0,0,0";
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::IDLE;
    std::string ret = instance.GetCloudMediaAssetTaskStatus();
    EXPECT_EQ(ret, result);
    operation = nullptr;
    ret = instance.GetCloudMediaAssetTaskStatus();
    EXPECT_EQ(ret, result);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_011 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_012 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::DOWNLOADING;
    bool ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, true);
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::IDLE;
    ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, false);
    operation = nullptr;
    ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_012 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_013 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = instance.GetTaskStatus();
    EXPECT_EQ(ret, 0);
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    ret = instance.GetDownloadType();
    EXPECT_EQ(ret, 1);
    operation = nullptr;
    ret = instance.GetTaskStatus();
    EXPECT_EQ(ret, 0);
    ret = instance.GetDownloadType();
    EXPECT_EQ(ret, 1);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_013 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_014 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::IDLE;
    bool ret = instance.SetBgDownloadPermission(true);
    EXPECT_EQ(ret, false);
    operation->taskStatus_ =  CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = instance.SetBgDownloadPermission(true);
    EXPECT_EQ(ret, true);
    operation = nullptr;
    ret = instance.SetBgDownloadPermission(true);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_014 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_015 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.UpdateCloudMediaAssets();
    EXPECT_EQ(ret, E_OK);

    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);

    int64_t fileId2 = 0;
    std::string data2 = "";
    ret = InsertCloudAssetINDb(fileId2, data2);
    EXPECT_EQ(ret, E_OK);

    ret = instance.UpdateCloudMediaAssets();
    EXPECT_EQ(ret, E_OK);
    ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    InsertCloudAlbumINDb();
    ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    ret = instance.UpdateLocalAlbums();
    EXPECT_EQ(ret, E_OK);

    ret = instance.UpdateBothLocalAndCloudAssets();
    EXPECT_EQ(ret, E_OK);
    int64_t fileId3 = 0;
    std::string data3 = "";
    ret = InsertCloudAndLocalAssetINDb(fileId3, data3);
    EXPECT_EQ(ret, E_OK);
    ret = instance.UpdateBothLocalAndCloudAssets();
    EXPECT_EQ(ret, E_OK);

    ret = instance.ClearDeletedDbData();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_015 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_016 Start");
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(ret, E_OK);

    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);

    int64_t fileId2 = 0;
    std::string data2 = "";
    ret = InsertCloudAssetINDb(fileId2, data2);
    EXPECT_EQ(ret, E_OK);

    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(ret, E_OK);
    ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    InsertCloudAlbumINDb();
    ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    ret = instance.UpdateLocalAlbums();
    EXPECT_EQ(ret, E_OK);

    ret = instance.UpdateBothLocalAndCloudAssets(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(ret, E_OK);
    int64_t fileId3 = 0;
    std::string data3 = "";
    ret = InsertCloudAndLocalAssetINDb(fileId3, data3);
    EXPECT_EQ(ret, E_OK);
    ret = instance.UpdateBothLocalAndCloudAssets(CloudMediaRetainType::HDC_RETAIN_FORCE);
    EXPECT_EQ(ret, E_OK);

    ret = instance.ClearDeletedDbData();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_016 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_017 Start");
    std::vector<std::string> idVec = { "id1", "id2" };
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.DeleteBatchCloudFile(idVec);
    EXPECT_EQ(ret, E_ERR);
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_017 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_018 Start");
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, true), E_OK);

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(CloudMediaRetainType::RETAIN_FORCE,
        true), E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_018 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_019 Start");
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(CloudMediaRetainType::RETAIN_FORCE,
        true), E_OK);
    EXPECT_NE(system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, true), E_OK);
    EXPECT_NE(system::GetIntParameter(HDC_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_019 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_020 Start");
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));

    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, true), E_OK);

    EXPECT_EQ(system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(HDC_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);

    MEDIA_INFO_LOG("cloud_asset_download_manager_test_020 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_021, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_021 Start");
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));

    std::thread t([&]() -> void {
        std::this_thread::sleep_for(chrono::milliseconds(1000));
        EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
            ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, true), E_OK);
        ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
        ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    });
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(CloudMediaRetainType::RETAIN_FORCE,
        true), E_OK);

    EXPECT_EQ(system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(HDC_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    t.join();
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_021 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_022, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_022 Start");
    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));

    std::thread t([&]() -> void {
        std::this_thread::sleep_for(chrono::milliseconds(1000));
        EXPECT_EQ(CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(
            CloudMediaRetainType::RETAIN_FORCE, true), E_OK);
        ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
        ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    });
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, true), E_OK);

    EXPECT_EQ(system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(HDC_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    t.join();
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_022 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_manager_test_023, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_023 Start");
    ASSERT_TRUE(system::SetParameter(MEIDA_RESTORE_FLAG, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(MEIDA_BACKUP_FLAG, std::to_string(DEFAULT_TIME_STAMP)));

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, false), E_OK);

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, false), E_OK);

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(MediaFileUtils::UTCTimeMilliSeconds())));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, false), E_OK);

    ASSERT_TRUE(system::SetParameter(CLOUD_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    ASSERT_TRUE(system::SetParameter(HDC_RETIAN_STATUS_KEY, std::to_string(DEFAULT_TIME_STAMP)));
    EXPECT_EQ(CloudMediaAssetManager::GetInstance().\
        ForceRetainDownloadCloudMedia(CloudMediaRetainType::HDC_RETAIN_FORCE, false), E_OK);

    EXPECT_EQ(system::GetIntParameter(CLOUD_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(HDC_RETIAN_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    EXPECT_EQ(system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, INVALID_TIME_STAMP), DEFAULT_TIME_STAMP);
    MEDIA_INFO_LOG("cloud_asset_download_manager_test_023 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    CloudMediaAssetDownloadOperation::DownloadFileData datas;
    int32_t ret = operation->SubmitBatchDownload(datas, true);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    ret = operation->SubmitBatchDownload(datas, true);
    EXPECT_EQ(ret, E_ERR);
    operation->downloadId_ = -1;
    ret = operation->SubmitBatchDownload(datas, true);
    EXPECT_EQ(ret, E_OK);
    ret = operation->SubmitBatchDownload(datas, false);
    EXPECT_EQ(ret, EXIT_TASK);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, EXIT_TASK);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_003 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    int32_t ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE));
    EXPECT_NE(ret, E_OK);
    ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;

    ret = operation->DoRecoverExecute();
    EXPECT_EQ(ret, E_ERR);

    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test001.jpg", 1024);
    ret = operation->DoRecoverExecute();
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_004 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_005 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->ManualActiveRecoverTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    ret = operation->ManualActiveRecoverTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE));
    EXPECT_EQ(ret, E_OK);
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test001.jpg", 1024);
    ret = operation->ManualActiveRecoverTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_005 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_006 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = false;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test001.jpg", 1024);
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    EXPECT_EQ(ret, E_ERR);
    operation->pauseCause_ = CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_006 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_007 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    EXPECT_EQ(ret, E_OK);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    EXPECT_EQ(ret, E_ERR);
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    EXPECT_EQ(ret, E_ERR);
    ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_007 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_008 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test001.jpg", 1024);
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    int32_t ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    operation->isBgDownloadPermission_ = false;
    ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_OK);
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->isBgDownloadPermission_ = true;
    ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_008 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_operation_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_009 Start");
    std::shared_ptr<NetConnectObserver> netObserver = make_shared<NetConnectObserver>();
    ASSERT_NE(netObserver, nullptr);
    sptr<NetManagerStandard::NetHandle> handle = sptr(new NetManagerStandard::NetHandle());
    ASSERT_NE(handle, nullptr);
    sptr<NetManagerStandard::NetAllCapabilities> netAllCap = sptr(new NetManagerStandard::NetAllCapabilities());
    ASSERT_NE(netAllCap, nullptr);
    int32_t ret = netObserver->NetCapabilitiesChange(handle, netAllCap);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("cloud_asset_download_operation_test_009 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_delete_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_delete_test_001 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    instance.StartDeleteCloudMediaAssets();
    instance.StopDeleteCloudMediaAssets();
    MEDIA_INFO_LOG("cloud_asset_delete_test_001 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_delete_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_delete_test_002 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;
    
    int32_t ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(fileIds.size(), 0);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, "cloud_media_asset_deleted");
    valuesBucket.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    valuesBucket.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId1);
    int32_t rows = 0;
    ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(fileIds.size(), 0);
    
    MEDIA_INFO_LOG("cloud_asset_delete_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_delete_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_delete_test_003 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, "cloud_media_asset_deleted");
    valuesBucket.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    valuesBucket.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId1);
    int32_t rows = 0;
    ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    
    bool processRet = instance.ProcessDeleteBatch(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(processRet, true);
    
    MEDIA_INFO_LOG("cloud_asset_delete_test_003 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_delete_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_delete_test_004 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    std::string editDataPath = data1 + "/editData";
    bool mkdirRet = MediaFileUtils::CreateDirectory(editDataPath);
    CHECK_AND_PRINT_LOG(mkdirRet, "create editData dir failed");
    
    ret = instance.DeleteEditdata(data1);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_delete_test_004 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_delete_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_delete_test_005 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> fileIds = {"test_id_1", "test_id_2"};
    int32_t ret = instance.DeleteBatchCloudFile(fileIds);
    EXPECT_EQ(ret, E_ERR);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    fileIds.clear();
    fileIds.push_back(std::to_string(fileId1));
    ret = instance.DeleteBatchCloudFile(fileIds);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_delete_test_005 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_update_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_update_test_001 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> updateFileIds;
    std::string lastFileId = "0";
    bool hasData = instance.HasDataForUpdate(CloudMediaRetainType::RETAIN_FORCE, updateFileIds, lastFileId,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(hasData, false);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    hasData = instance.HasDataForUpdate(CloudMediaRetainType::RETAIN_FORCE, updateFileIds, lastFileId,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(hasData, true);
    
    MEDIA_INFO_LOG("cloud_asset_update_test_001 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_update_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_update_test_002 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> updateFileIds;
    int32_t ret = instance.UpdateCloudAssets(updateFileIds, SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_ERR);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    updateFileIds.push_back(std::to_string(fileId1));
    ret = instance.UpdateCloudAssets(updateFileIds, SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    updateFileIds.clear();
    updateFileIds.push_back(std::to_string(fileId1));
    ret = instance.UpdateCloudAssets(updateFileIds, SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    updateFileIds.clear();
    updateFileIds.push_back(std::to_string(fileId1));
    ret = instance.UpdateCloudAssets(updateFileIds, SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_update_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_update_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_update_test_003 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> notifyFileIds;
    instance.NotifyUpdateAssetsChange(notifyFileIds);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    notifyFileIds.push_back(std::to_string(fileId1));
    instance.NotifyUpdateAssetsChange(notifyFileIds);
    
    MEDIA_INFO_LOG("cloud_asset_update_test_003 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_update_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_update_test_004 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<int32_t> albumIds;
    std::vector<int32_t> result = instance.QueryEmptyAlbumsAndBackup();
    EXPECT_GE(result.size(), 0);
    
    MEDIA_INFO_LOG("cloud_asset_update_test_004 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_update_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_update_test_005 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<int32_t> albumIds = {1, 2, 3};
    std::string whereClause = instance.BuildEmptyAlbumsWhereClause(albumIds);
    EXPECT_NE(whereClause, "");
    
    albumIds.clear();
    whereClause = instance.BuildEmptyAlbumsWhereClause(albumIds);
    EXPECT_NE(whereClause, "");
    
    MEDIA_INFO_LOG("cloud_asset_update_test_005 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_local_cloud_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_local_cloud_test_001 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> updateFileIds;
    std::string lastFileId = "0";
    bool hasData = instance.HasLocalAndCloudAssets(CloudMediaRetainType::RETAIN_FORCE, updateFileIds, lastFileId);
    EXPECT_EQ(hasData, false);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAndLocalAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    hasData = instance.HasLocalAndCloudAssets(CloudMediaRetainType::RETAIN_FORCE, updateFileIds, lastFileId);
    EXPECT_EQ(hasData, true);
    
    MEDIA_INFO_LOG("cloud_asset_local_cloud_test_001 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_local_cloud_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_local_cloud_test_002 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> updateFileIds;
    int32_t ret = instance.UpdateLocalAndCloudAssets(updateFileIds);
    EXPECT_EQ(ret, E_ERR);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAndLocalAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    updateFileIds.push_back(std::to_string(fileId1));
    ret = instance.UpdateLocalAndCloudAssets(updateFileIds);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_local_cloud_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_batch_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_batch_test_001 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> fileIds = {"file1", "file2"};
    std::map<std::string, int32_t> uriStatusMap;
    int32_t ret = instance.UpdateAddTaskStatus(fileIds,
        CloudMediaTaskDownloadCloudAssetCode::ADD_DOWNLOAD_TASK_SUCC, uriStatusMap);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(uriStatusMap.size(), 2);
    
    fileIds.clear();
    ret = instance.UpdateAddTaskStatus(fileIds,
        CloudMediaTaskDownloadCloudAssetCode::ADD_DOWNLOAD_TASK_SUCC, uriStatusMap);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_batch_test_001 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_batch_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_batch_test_002 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    
    bool ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, true);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, false);
    
    MEDIA_INFO_LOG("cloud_asset_batch_test_002 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_batch_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_batch_test_003 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::ROM_LIMIT;
    instance.CheckStorageAndRecoverDownloadTask();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    instance.CheckStorageAndRecoverDownloadTask();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    instance.CheckStorageAndRecoverDownloadTask();
    
    MEDIA_INFO_LOG("cloud_asset_batch_test_003 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_batch_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_batch_test_004 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes = {REAL_LCD_VISIT_TIME_INVALID};
    std::vector<int32_t> subTypes;
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, "cloud_media_asset_deleted");
    valuesBucket.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    valuesBucket.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId1);
    int32_t rows = 0;
    ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    
    lcdVisitTimes.clear();
    lcdVisitTimes.push_back(REAL_LCD_VISIT_TIME_DELETED);
    bool processRet = instance.ProcessDeleteBatch(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(processRet, true);
    
    MEDIA_INFO_LOG("cloud_asset_batch_test_004 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_batch_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_batch_test_005 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, "cloud_media_asset_deleted");
    valuesBucket.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    valuesBucket.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId1);
    int32_t rows = 0;
    ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    EXPECT_EQ(ret, E_OK);
    
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    std::vector<int64_t> lcdVisitTimes;
    std::vector<int32_t> subTypes;
    lcdVisitTimes.push_back(100);
    ret = instance.ReadyDataForDelete(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(ret, E_OK);
    
    bool processRet = instance.ProcessDeleteBatch(fileIds, paths, dateTakens, lcdVisitTimes, subTypes);
    EXPECT_EQ(processRet, true);
    
    MEDIA_INFO_LOG("cloud_asset_batch_test_005 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_024, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_024 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    int32_t ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_ERR);
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_024 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_025, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_025 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    
    int32_t ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    ret = instance.RecoverDownloadCloudAsset(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_025 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_026, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_026 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    
    int32_t ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(ret, E_ERR);
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    ret = instance.StartDownloadCloudAsset(CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_026 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_027, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_027 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_027 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_028, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_028 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_028 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_029, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_029 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::RETAIN_FORCE, SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_029 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_030, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_030 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_030 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_031, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_031 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE, SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_031 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_test_032, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_test_032 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.UpdateCloudMediaAssets(CloudMediaRetainType::HDC_RETAIN_FORCE, SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_download_test_032 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_010 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    EXPECT_EQ(ret, E_ERR);
    
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_010 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_011 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = false;
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_OK);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test001.jpg", 1024);
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->isBgDownloadPermission_ = true;
    ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_011 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_012 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    EXPECT_EQ(ret, E_ERR);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_012 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_013 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    int32_t ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = false;
    ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_OK);
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->isBgDownloadPermission_ = false;
    ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_013 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_016 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    operation->remainCount_ = 0;
    operation->remainSize_ = 0;
    int32_t ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_OK);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = -1;
    ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_016 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_018 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_ERR);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_OK);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, EXIT_TASK);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_018 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_019 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    int32_t ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE));
    EXPECT_EQ(ret, E_ERR);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_019 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_operation_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_operation_test_020 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    operation->remainCount_ = 0;
    operation->remainSize_ = 0;
    CloudMediaAssetDownloadOperation::DownloadFileData datas;
    int32_t ret = operation->SubmitBatchDownload(datas, true);
    EXPECT_EQ(ret, E_ERR);
    
    operation->downloadId_ = -1;
    ret = operation->SubmitBatchDownload(datas, false);
    EXPECT_EQ(ret, EXIT_TASK);
    
    MEDIA_INFO_LOG("cloud_asset_operation_test_020 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_033, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_033 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    
    int32_t ret = instance.GetTaskStatus();
    EXPECT_EQ(ret, static_cast<int32_t>(CloudMediaAssetTaskStatus::DOWNLOADING));
    
    ret = instance.GetDownloadType();
    EXPECT_EQ(ret, static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE));
    
    instance.operation_ = nullptr;
    ret = instance.GetTaskStatus();
    EXPECT_EQ(ret, static_cast<int32_t>(CloudMediaAssetTaskStatus::IDLE));
    
    MEDIA_INFO_LOG("cloud_asset_test_033 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_034, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_034 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::ROM_LIMIT;
    
    instance.CheckStorageAndRecoverDownloadTask();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    instance.CheckStorageAndRecoverDownloadTask();
    
    MEDIA_INFO_LOG("cloud_asset_test_034 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_035, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_035 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::string result = instance.GetCloudMediaAssetTaskStatus();
    EXPECT_NE(result, "");
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 123;
    operation->remainCount_ = 10;
    operation->remainSize_ = 1024;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    
    result = instance.GetCloudMediaAssetTaskStatus();
    EXPECT_NE(result, "");
    
    MEDIA_INFO_LOG("cloud_asset_test_035 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_036, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_036 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    
    bool ret = instance.SetBgDownloadPermission(true);
    EXPECT_EQ(ret, true);
    
    ret = instance.SetBgDownloadPermission(false);
    EXPECT_EQ(ret, true);
    
    instance.operation_ = nullptr;
    ret = instance.SetBgDownloadPermission(true);
    EXPECT_EQ(ret, false);
    
    MEDIA_INFO_LOG("cloud_asset_test_036 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_037, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_037 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->isThumbnailUpdate_ = false;
    
    bool ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, true);
    EXPECT_EQ(operation->isThumbnailUpdate_, true);
    
    ret = instance.SetIsThumbnailUpdate();
    EXPECT_EQ(ret, false);
    
    MEDIA_INFO_LOG("cloud_asset_test_037 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_038, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_038 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    instance.operation_ = operation;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->isThumbnailUpdate_ = false;
    
    int32_t ret = operation->InitDownloadTaskInfo();
    EXPECT_EQ(ret, E_OK);
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ret = operation->InitDownloadTaskInfo();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_038 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_039, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_039 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    instance.StartDeleteCloudMediaAssets();
    this_thread::sleep_for(chrono::milliseconds(100));
    instance.StopDeleteCloudMediaAssets();
    
    MEDIA_INFO_LOG("cloud_asset_test_039 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_040, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_040 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int64_t fileId1 = 0;
    std::string data1 = "";
    int32_t ret = InsertCloudAssetINDb(fileId1, data1);
    EXPECT_EQ(ret, E_OK);
    
    ValuesBucket valuesBucket;
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId1);
    int32_t rows = 0;
    ret = g_rdbStore->Update(rows, valuesBucket, predicates);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ClearDeletedDbData();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_040 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_041, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_041 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.UpdateLocalAlbums();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_041 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_042, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_042 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    
    int64_t albumId = 0;
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::USER);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_IS_LOCAL, ALBUM_FROMCLOUD);
    ret = g_rdbStore->Insert(albumId, PhotoAlbumColumns::TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.DeleteEmptyCloudAlbums();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_042 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_043, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_043 Start");
    int32_t ret = CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(
        CloudMediaRetainType::RETAIN_FORCE, false);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_043 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_044, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_044 Start");
    int32_t ret = CloudMediaAssetManager::GetInstance().ForceRetainDownloadCloudMedia(
        CloudMediaRetainType::HDC_RETAIN_FORCE, false);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_044 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_045, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_045 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::RETAIN_FORCE,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::RETAIN_FORCE,
        SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::RETAIN_FORCE,
        SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_045 End");
}

HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_test_046, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_test_046 Start");
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    
    int32_t ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::RETAIN);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::RECOVER);
    EXPECT_EQ(ret, E_OK);
    
    ret = instance.ForceRetainDownloadCloudMediaEx(CloudMediaRetainType::HDC_RETAIN_FORCE,
        SmartDataProcessingMode::NONE);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_test_046 End");
}

// 测试目标: 测试HandleSuccessCallback处理成功下载回调时正确更新remainCount和remainSize
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_success_callback_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_success_001.jpg";
    progress.batchSuccNum = 1;
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 2048);
    operation->remainCount_ = 10;
    operation->remainSize_ = 10240;
    operation->totalCount_ = 20;
    operation->totalSize_ = 20480;
    
    operation->HandleSuccessCallback(progress);
    EXPECT_EQ(operation->remainCount_, 9);
    EXPECT_EQ(operation->remainSize_, 8192);
    
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_001 End");
}

// 测试目标: 测试HandleSuccessCallback处理size为0的情况不减少remainSize
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_success_callback_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_success_002.jpg";
    progress.batchSuccNum = 1;
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 0);
    operation->remainCount_ = 5;
    operation->remainSize_ = 5000;
    
    operation->HandleSuccessCallback(progress);
    EXPECT_EQ(operation->remainCount_, 5);
    EXPECT_EQ(operation->remainSize_, 5000);
    
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_002 End");
}

// 测试目标: 测试HandleSuccessCallback处理downloadId不匹配的情况
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_success_callback_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 99;
    progress.path = "/storage/cloud/files/test_success_003.jpg";
    progress.batchSuccNum = 1;
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->remainCount_ = 5;
    operation->remainSize_ = 5120;
    
    operation->HandleSuccessCallback(progress);
    EXPECT_EQ(operation->remainCount_, 5);
    EXPECT_EQ(operation->remainSize_, 5120);
    
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_003 End");
}

// 测试目标: 测试HandleSuccessCallback处理path不在map中的情况
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_success_callback_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/not_exist.jpg";
    progress.batchSuccNum = 1;
    
    operation->downloadId_ = 1;
    operation->remainCount_ = 5;
    operation->remainSize_ = 5120;
    
    operation->HandleSuccessCallback(progress);
    EXPECT_EQ(operation->remainCount_, 5);
    EXPECT_EQ(operation->remainSize_, 5120);
    
    MEDIA_INFO_LOG("cloud_asset_success_callback_test_004 End");
}

// 测试目标: 测试HandleFailedCallback处理UNKNOWN_ERROR错误类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_001.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::UNKNOWN_ERROR;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::CLOUD_ERROR);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_001 End");
}

// 测试目标: 测试HandleFailedCallback处理NETWORK_UNAVAILABLE达到最大重试次数
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_003.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::NETWORK_UNAVAILABLE;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->downloadTryTime_ = 3;
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_003 End");
}

// 测试目标: 测试HandleFailedCallback处理LOCAL_STORAGE_FULL错误类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_004.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::LOCAL_STORAGE_FULL;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::ROM_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_004 End");
}

// 测试目标: 测试HandleFailedCallback处理CONTENT_NOT_FOUND错误类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_005 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_005.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::CONTENT_NOT_FOUND;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->notFoundForDownload_.fileDownloadMap.Size(), 1);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_005 End");
}

// 测试目标: 测试HandleFailedCallback处理FREQUENT_USER_REQUESTS错误类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_006 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_006.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::FREQUENT_USER_REQUESTS;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_006 End");
}

// 测试目标: 测试HandleFailedCallback处理未知错误类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_007 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_007.jpg";
    progress.downloadErrorType = 999;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_007 End");
}

// 测试目标: 测试HandleFailedCallback在用户暂停状态下不处理
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_callback_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_008 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_008.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::UNKNOWN_ERROR;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::USER_PAUSED);
    
    MEDIA_INFO_LOG("cloud_asset_failed_callback_test_008 End");
}

// 测试目标: 测试HandleStoppedCallback处理停止的下载
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_stopped_callback_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_stopped_callback_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->ClearData(operation->cacheForDownload_);
    operation->ClearData(operation->dataForDownload_);

    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_stopped_001.jpg";
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleStoppedCallback(progress);
    EXPECT_EQ(operation->cacheForDownload_.fileDownloadMap.Size(), 1);
    EXPECT_EQ(operation->dataForDownload_.fileDownloadMap.Size(), 0);
    
    MEDIA_INFO_LOG("cloud_asset_stopped_callback_test_001 End");
}

// 测试目标: 测试SetTaskStatus设置FORCE_DOWNLOADING状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::FORCE_DOWNLOADING);
    EXPECT_EQ(operation->downloadType_, CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::NO_PAUSE);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_001 End");
}

// 测试目标: 测试SetTaskStatus设置GENTLE_DOWNLOADING状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::GENTLE_DOWNLOADING);
    EXPECT_EQ(operation->downloadType_, CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_002 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_TEMPERATURE_LIMIT状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_TEMPERATURE_LIMIT);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::TEMPERATURE_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_003 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_ROM_LIMIT状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_ROM_LIMIT);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::ROM_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_004 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_NETWORK_FLOW_LIMIT状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_005 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_NETWORK_FLOW_LIMIT);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_005 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_WIFI_UNAVAILABLE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_006 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_WIFI_UNAVAILABLE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_006 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_POWER_LIMIT状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_007 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_POWER_LIMIT);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::POWER_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_007 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_008 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_008 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_FREQUENT_USER_REQUESTS状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_009 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_FREQUENT_USER_REQUESTS);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_009 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_CLOUD_ERROR状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_010 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_CLOUD_ERROR);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::CLOUD_ERROR);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_010 End");
}

// 测试目标: 测试SetTaskStatus设置PAUSE_FOR_USER_PAUSE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_011 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_USER_PAUSE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::USER_PAUSED);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_011 End");
}

// 测试目标: 测试SetTaskStatus设置RECOVER_FOR_MANAUL_ACTIVE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_012 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::RECOVER_FOR_MANAUL_ACTIVE);
    EXPECT_EQ(operation->downloadType_, CloudMediaDownloadType::DOWNLOAD_FORCE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_012 End");
}

// 测试目标: 测试SetTaskStatus设置RECOVER_FOR_PASSIVE_STATUS状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_013 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    auto downloadType = operation->downloadType_;
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::RECOVER_FOR_PASSIVE_STATUS);
    EXPECT_EQ(operation->downloadType_, downloadType);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_013 End");
}

// 测试目标: 测试SetTaskStatus设置IDLE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_test_014 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::IDLE);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::IDLE);
    
    MEDIA_INFO_LOG("cloud_asset_status_test_014 End");
}

// 测试目标: 测试IsDataEmpty判断空数据
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_data_empty_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_data_empty_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    bool ret = operation->IsDataEmpty(data);
    EXPECT_EQ(ret, true);
    
    MEDIA_INFO_LOG("cloud_asset_data_empty_test_001 End");
}

// 测试目标: 测试IsDataEmpty判断非空数据
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_data_empty_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_data_empty_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    data.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    bool ret = operation->IsDataEmpty(data);
    EXPECT_EQ(ret, false);
    
    MEDIA_INFO_LOG("cloud_asset_data_empty_test_002 End");
}

// 测试目标: 测试ClearData清理数据
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_clear_data_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_clear_data_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    data.pathVec.push_back("/test/path1.jpg");
    data.fileDownloadMap.EnsureInsert("/test/path1.jpg", 1024);
    data.batchFileIdNeedDownload.push_back("1");
    data.batchSizeNeedDownload = 1024;
    data.batchCountNeedDownload = 1;
    
    operation->ClearData(data);
    EXPECT_EQ(data.pathVec.size(), 0);
    EXPECT_EQ(data.fileDownloadMap.Size(), 0);
    EXPECT_EQ(data.batchFileIdNeedDownload.size(), 0);
    EXPECT_EQ(data.batchSizeNeedDownload, 0);
    EXPECT_EQ(data.batchCountNeedDownload, 0);
    
    MEDIA_INFO_LOG("cloud_asset_clear_data_test_001 End");
}

// 测试目标: 测试ResetParameter重置参数
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_reset_parameter_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_reset_parameter_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->readyForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    operation->notFoundForDownload_.fileDownloadMap.EnsureInsert("test2.jpg", 1024);
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test3.jpg", 1024);
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test4.jpg", 1024);
    operation->downloadId_ = 100;
    operation->isThumbnailUpdate_ = false;
    operation->isBgDownloadPermission_ = true;
    operation->totalCount_ = 50;
    operation->totalSize_ = 50000;
    operation->remainCount_ = 25;
    operation->remainSize_ = 25000;
    operation->downloadTryTime_ = 5;
    
    operation->ResetParameter();
    EXPECT_EQ(operation->downloadId_, -1);
    EXPECT_EQ(operation->isThumbnailUpdate_, true);
    EXPECT_EQ(operation->isBgDownloadPermission_, false);
    EXPECT_EQ(operation->totalCount_, 0);
    EXPECT_EQ(operation->totalSize_, 0);
    EXPECT_EQ(operation->remainCount_, 0);
    EXPECT_EQ(operation->remainSize_, 0);
    EXPECT_EQ(operation->downloadTryTime_, 0);
    
    MEDIA_INFO_LOG("cloud_asset_reset_parameter_test_001 End");
}

// 测试目标: 测试HandleOnRemoteDied在DOWNLOADING状态下取消任务
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_remote_died_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_remote_died_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->HandleOnRemoteDied();
    EXPECT_EQ(operation->cloudRemoteObject_, nullptr);
    
    MEDIA_INFO_LOG("cloud_asset_remote_died_test_001 End");
}

// 测试目标: 测试HandleOnRemoteDied在非DOWNLOADING状态不做处理
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_remote_died_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_remote_died_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    operation->HandleOnRemoteDied();
    EXPECT_EQ(operation->cloudRemoteObject_, nullptr);
    
    MEDIA_INFO_LOG("cloud_asset_remote_died_test_002 End");
}

// 测试目标: 测试ResetDownloadTryTime重置下载重试次数
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_reset_retry_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_reset_retry_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->downloadTryTime_ = 5;
    operation->ResetDownloadTryTime();
    EXPECT_EQ(operation->downloadTryTime_, 0);
    
    MEDIA_INFO_LOG("cloud_asset_reset_retry_test_001 End");
}

// 测试目标: 测试SubmitBatchDownloadAgain在dataForDownload非空时返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_again_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    int32_t ret = operation->SubmitBatchDownloadAgain();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_001 End");
}

// 测试目标: 测试SubmitBatchDownloadAgain在cacheForDownload非空时提交
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_again_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->SubmitBatchDownloadAgain();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_002 End");
}

// 测试目标: 测试SubmitBatchDownloadAgain在readyForDownload非空时提交
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_again_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.Clear();
    operation->readyForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->SubmitBatchDownloadAgain();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_submit_again_test_003 End");
}

// 测试目标: 测试InitStartDownloadTaskStatus前台模式
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_init_status_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_init_status_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    operation->InitStartDownloadTaskStatus(true);
    
    MEDIA_INFO_LOG("cloud_asset_init_status_test_001 End");
}

// 测试目标: 测试InitStartDownloadTaskStatus后台模式
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_init_status_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_init_status_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    operation->InitStartDownloadTaskStatus(false);
    
    MEDIA_INFO_LOG("cloud_asset_init_status_test_002 End");
}

// 测试目标: 测试MoveDownloadFileToCache将文件移到缓存
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_cache_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_cache_001.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 2048);
    
    operation->MoveDownloadFileToCache(progress, false);
    EXPECT_EQ(operation->cacheForDownload_.fileDownloadMap.Size(), 1);
    EXPECT_EQ(operation->dataForDownload_.fileDownloadMap.Size(), 0);
    
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_001 End");
}

// 测试目标: 测试MoveDownloadFileToCache处理downloadId不匹配
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_cache_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->ClearData(operation->cacheForDownload_);
    operation->ClearData(operation->dataForDownload_);
    
    DownloadProgressObj progress;
    progress.downloadId = 99;
    progress.path = "/storage/cloud/files/test_cache_002.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->MoveDownloadFileToCache(progress, false);
    EXPECT_EQ(operation->cacheForDownload_.fileDownloadMap.Size(), 0);
    EXPECT_EQ(operation->dataForDownload_.fileDownloadMap.Size(), 1);
    
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_002 End");
}

// 测试目标: 测试MoveDownloadFileToCache文件已在缓存中
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_cache_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_cache_003.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->MoveDownloadFileToCache(progress, false);
    
    MEDIA_INFO_LOG("cloud_asset_move_cache_test_003 End");
}

// 测试目标: 测试MoveDownloadFileToNotFound将文件移到未找到列表
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_notfound_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->ClearData(operation->notFoundForDownload_);
    operation->ClearData(operation->dataForDownload_);
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_notfound_001.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->MoveDownloadFileToNotFound(progress);
    EXPECT_EQ(operation->notFoundForDownload_.fileDownloadMap.Size(), 1);
    EXPECT_EQ(operation->dataForDownload_.fileDownloadMap.Size(), 0);
    
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_001 End");
}

// 测试目标: 测试MoveDownloadFileToNotFound处理downloadId不匹配
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_notfound_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->ClearData(operation->notFoundForDownload_);
    operation->ClearData(operation->dataForDownload_);
    
    DownloadProgressObj progress;
    progress.downloadId = 99;
    progress.path = "/storage/cloud/files/test_notfound_002.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->MoveDownloadFileToNotFound(progress);
    EXPECT_EQ(operation->notFoundForDownload_.fileDownloadMap.Size(), 0);
    EXPECT_EQ(operation->dataForDownload_.fileDownloadMap.Size(), 1);
    
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_002 End");
}

// 测试目标: 测试MoveDownloadFileToNotFound文件已在notFound中
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_notfound_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_notfound_003.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->notFoundForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->MoveDownloadFileToNotFound(progress);
    
    MEDIA_INFO_LOG("cloud_asset_move_notfound_test_003 End");
}

// 测试目标: 测试PauseDownloadTask在IDLE状态下返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_pause_test_001 End");
}

// 测试目标: 测试PauseDownloadTask在USER_PAUSED状态下返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_pause_test_002 End");
}

// 测试目标: 测试PauseDownloadTask在BACKGROUND_TASK_UNAVAILABLE状态下非用户暂停返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_pause_test_003 End");
}

// 测试目标: 测试PauseDownloadTask在DOWNLOADING状态下成功暂停
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::ROM_LIMIT);
    
    MEDIA_INFO_LOG("cloud_asset_pause_test_004 End");
}

// 测试目标: 测试PauseDownloadTask在DOWNLOADING状态下无downloadId
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_test_005 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = -1;
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::ROM_LIMIT);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    
    MEDIA_INFO_LOG("cloud_asset_pause_test_005 End");
}

// 测试目标: 测试CancelDownloadTask在IDLE状态下返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_cancel_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_cancel_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_cancel_test_001 End");
}

// 测试目标: 测试CancelDownloadTask成功取消任务
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_cancel_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_cancel_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    operation->notFoundForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::IDLE);
    EXPECT_EQ(operation->downloadId_, -1);
    EXPECT_EQ(operation->downloadCallback_, nullptr);
    
    MEDIA_INFO_LOG("cloud_asset_cancel_test_002 End");
}

// 测试目标: 测试ManualActiveRecoverTask非PAUSED状态返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_manual_recover_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_manual_recover_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->ManualActiveRecoverTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_manual_recover_test_001 End");
}

// 测试目标: 测试ManualActiveRecoverTask使用DOWNLOAD_GENTLE类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_manual_recover_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_manual_recover_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->ManualActiveRecoverTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_GENTLE));
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_manual_recover_test_002 End");
}

// 测试目标: 测试PassiveStatusRecover GENTLE类型无后台权限
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = false;
    
    int32_t ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_001 End");
}

// 测试目标: 测试PassiveStatusRecover GENTLE类型有后台权限
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_002 End");
}

// 测试目标: 测试PassiveStatusRecover FORCE类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->isBgDownloadPermission_ = true;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecover();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_test_003 End");
}

// 测试目标: 测试PassiveStatusRecoverTask非PAUSED状态返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_task_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask USER_PAUSED状态返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_task_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_002 End");
}

// 测试目标: 测试PassiveStatusRecoverTask NETWORK_NORMAL恢复条件
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_task_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::WIFI_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = false;
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_003 End");
}

// 测试目标: 测试PassiveStatusRecoverTask无效的恢复原因
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_task_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::ROM_LIMIT;
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_004 End");
}

// 测试目标: 测试PassiveStatusRecoverTask FOREGROUND_TEMPERATURE_PROPER恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_passive_recover_task_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_005 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::TEMPERATURE_LIMIT;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::FOREGROUND_TEMPERATURE_PROPER);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_passive_recover_task_test_005 End");
}

// 测试目标: 测试DoRecoverExecute dataForDownload非空返回错误
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_do_recover_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_do_recover_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->dataForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    int32_t ret = operation->DoRecoverExecute();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_do_recover_test_001 End");
}

// 测试目标: 测试DoRecoverExecute cacheForDownload非空
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_do_recover_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_do_recover_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;

    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->DoRecoverExecute();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_do_recover_test_002 End");
}

// 测试目标: 测试GetDownloadType获取下载类型
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_get_info_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_get_info_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    CloudMediaDownloadType type = operation->GetDownloadType();
    EXPECT_EQ(type, CloudMediaDownloadType::DOWNLOAD_FORCE);
    
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    type = operation->GetDownloadType();
    EXPECT_EQ(type, CloudMediaDownloadType::DOWNLOAD_GENTLE);
    
    MEDIA_INFO_LOG("cloud_asset_get_info_test_001 End");
}

// 测试目标: 测试GetTaskStatus获取任务状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_get_info_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_get_info_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    CloudMediaAssetTaskStatus status = operation->GetTaskStatus();
    EXPECT_EQ(status, CloudMediaAssetTaskStatus::IDLE);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    status = operation->GetTaskStatus();
    EXPECT_EQ(status, CloudMediaAssetTaskStatus::DOWNLOADING);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    status = operation->GetTaskStatus();
    EXPECT_EQ(status, CloudMediaAssetTaskStatus::PAUSED);
    
    MEDIA_INFO_LOG("cloud_asset_get_info_test_002 End");
}

// 测试目标: 测试GetTaskPauseCause获取暂停原因
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_get_info_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_get_info_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    CloudMediaTaskPauseCause cause = operation->GetTaskPauseCause();
    EXPECT_EQ(cause, CloudMediaTaskPauseCause::NO_PAUSE);
    
    operation->pauseCause_ = CloudMediaTaskPauseCause::USER_PAUSED;
    cause = operation->GetTaskPauseCause();
    EXPECT_EQ(cause, CloudMediaTaskPauseCause::USER_PAUSED);
    
    MEDIA_INFO_LOG("cloud_asset_get_info_test_003 End");
}

// 测试目标: 测试GetTaskInfo获取任务信息
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_get_info_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_get_info_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->totalCount_ = 100;
    operation->totalSize_ = 1024000;
    operation->remainCount_ = 50;
    operation->remainSize_ = 512000;
    
    std::string info = operation->GetTaskInfo();
    EXPECT_EQ(info, "100,1024000,50,512000");
    
    MEDIA_INFO_LOG("cloud_asset_get_info_test_004 End");
}

// 测试目标: 测试InitDownloadTaskInfo isThumbnailUpdate为false直接返回
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_init_task_info_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_init_task_info_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->isThumbnailUpdate_ = false;
    int32_t ret = operation->InitDownloadTaskInfo();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_init_task_info_test_001 End");
}

// 测试目标: 测试SubmitBatchDownload任务状态不是DOWNLOADING
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_batch_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    int32_t ret = operation->SubmitBatchDownload(data, true);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_001 End");
}

// 测试目标: 测试SubmitBatchDownload downloadId不为-1
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_batch_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    int32_t ret = operation->SubmitBatchDownload(data, true);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_002 End");
}

// 测试目标: 测试SubmitBatchDownload数据为空且isCache为false
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_batch_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = -1;
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    int32_t ret = operation->SubmitBatchDownload(data, false);
    EXPECT_EQ(ret, EXIT_TASK);
    
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_003 End");
}

// 测试目标: 测试SubmitBatchDownload数据为空且isCache为true
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_batch_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_004 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = -1;
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    int32_t ret = operation->SubmitBatchDownload(data, true);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_submit_batch_test_004 End");
}

// 测试目标: 测试DoForceTaskExecute IDLE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_do_force_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_do_force_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    int32_t ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_do_force_test_001 End");
}

// 测试目标: 测试DoForceTaskExecute PAUSED状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_do_force_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_do_force_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    int32_t ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_do_force_test_002 End");
}

// 测试目标: 测试DoForceTaskExecute DOWNLOADING状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_do_force_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_do_force_test_003 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->DoForceTaskExecute();
    EXPECT_EQ(ret, EXIT_TASK);
    
    MEDIA_INFO_LOG("cloud_asset_do_force_test_003 End");
}

// 测试目标: 测试StartDownloadTask非IDLE状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_start_download_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_start_download_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    int32_t ret = operation->StartDownloadTask(static_cast<int32_t>(CloudMediaDownloadType::DOWNLOAD_FORCE));
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_start_download_test_001 End");
}

// 测试目标: 测试InitDownloadTaskInfo返回错误当无数据
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_init_task_info_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_init_task_info_test_002 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->isThumbnailUpdate_ = true;
    operation->totalCount_ = 10;
    operation->totalSize_ = 10240;
    operation->remainCount_ = 5;
    operation->remainSize_ = 5120;
    
    int32_t ret = operation->InitDownloadTaskInfo();
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_init_task_info_test_002 End");
}

// 测试目标: 测试多个状态转换场景
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_status_transition_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_status_transition_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::FORCE_DOWNLOADING);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    EXPECT_EQ(operation->downloadType_, CloudMediaDownloadType::DOWNLOAD_FORCE);
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_ROM_LIMIT);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::PAUSED);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::ROM_LIMIT);

    MEDIA_INFO_LOG("cloud_asset_status_transition_test_001 End");
}

// 测试目标: 测试多个暂停原因的状态转换
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_cause_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_cause_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_TEMPERATURE_LIMIT);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::TEMPERATURE_LIMIT);
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_NETWORK_FLOW_LIMIT);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_WIFI_UNAVAILABLE);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_POWER_LIMIT);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::POWER_LIMIT);
    
    operation->SetTaskStatus(CloudMediaAssetDownloadOperation::Status::PAUSE_FOR_BACKGROUND_TASK_UNAVAILABLE);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE);
    
    MEDIA_INFO_LOG("cloud_asset_pause_cause_test_001 End");
}

// 测试目标: 测试MoveDownloadFileToCache在tryDownload为true且数据为空时增加重试次数
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_move_cache_retry_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_move_cache_retry_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_retry_cache.jpg";
    
    operation->downloadId_ = 1;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->downloadTryTime_ = 0;
    
    operation->MoveDownloadFileToCache(progress, true);
    EXPECT_EQ(operation->downloadTryTime_, 1);
    
    MEDIA_INFO_LOG("cloud_asset_move_cache_retry_test_001 End");
}

// 测试目标: 测试ReadyDataForBatchDownload准备下载数据
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_ready_batch_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_ready_batch_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->isThumbnailUpdate_ = true;
    CloudMediaAssetDownloadOperation::DownloadFileData data = operation->ReadyDataForBatchDownload();
    
    MEDIA_INFO_LOG("cloud_asset_ready_batch_test_001 End");
}

// 测试目标: 测试CancelDownloadTask清理所有资源
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_cancel_cleanup_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_cancel_cleanup_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    operation->notFoundForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test2.jpg", 2048);
    operation->readyForDownload_.fileDownloadMap.EnsureInsert("test3.jpg", 4096);
    
    int32_t ret = operation->CancelDownloadTask();
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::IDLE);
    EXPECT_EQ(operation->downloadId_, -1);
    EXPECT_EQ(operation->notFoundForDownload_.fileDownloadMap.Size(), 0);
    EXPECT_EQ(operation->cacheForDownload_.fileDownloadMap.Size(), 0);
    EXPECT_EQ(operation->readyForDownload_.fileDownloadMap.Size(), 0);
    EXPECT_EQ(operation->downloadCallback_, nullptr);
    EXPECT_EQ(operation->cloudRemoteObject_, nullptr);
    EXPECT_EQ(operation->cloudHelper_, nullptr);
    EXPECT_EQ(operation->cloudMediaAssetObserver_, nullptr);
    
    MEDIA_INFO_LOG("cloud_asset_cancel_cleanup_test_001 End");
}

// 测试目标: 测试SubmitBatchDownload数据不为空时正常提交
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_submit_data_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_submit_data_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = -1;
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    data.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->SubmitBatchDownload(data, false);
    EXPECT_EQ(ret, E_OK);
    
    MEDIA_INFO_LOG("cloud_asset_submit_data_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask NETWORK_FLOW_LIMIT恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_recover_network_flow_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_recover_network_flow_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::NETWORK_FLOW_UNLIMIT);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_recover_network_flow_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask BACKGROUND_TASK恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_recover_bg_task_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_recover_bg_task_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::BACKGROUND_TASK_UNAVAILABLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::BACKGROUND_TASK_AVAILABLE);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_recover_bg_task_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask RETRY_FOR_FREQUENT_REQUESTS恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_recover_freq_request_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_recover_freq_request_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::RETRY_FOR_FREQUENT_REQUESTS);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_recover_freq_request_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask RETRY_FOR_CLOUD_ERROR恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_recover_cloud_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_recover_cloud_error_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::CLOUD_ERROR;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_FORCE;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::RETRY_FOR_CLOUD_ERROR);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_recover_cloud_error_test_001 End");
}

// 测试目标: 测试PassiveStatusRecoverTask STORAGE_NORMAL恢复
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_recover_storage_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_recover_storage_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->pauseCause_ = CloudMediaTaskPauseCause::ROM_LIMIT;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->isBgDownloadPermission_ = true;
    operation->dataForDownload_.fileDownloadMap.Clear();
    operation->cacheForDownload_.fileDownloadMap.EnsureInsert("test1.jpg", 1024);
    
    int32_t ret = operation->PassiveStatusRecoverTask(CloudMediaTaskRecoverCause::STORAGE_NORMAL);
    EXPECT_EQ(ret, E_ERR);
    
    MEDIA_INFO_LOG("cloud_asset_recover_storage_test_001 End");
}

// 测试目标: 测试HandleFailedCallback处理网络错误有网络的情况
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_failed_network_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_failed_network_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_failed_network.jpg";
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::NETWORK_UNAVAILABLE;
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    operation->downloadTryTime_ = 1;
    
    operation->HandleFailedCallback(progress);
    EXPECT_EQ(operation->taskStatus_, CloudMediaAssetTaskStatus::DOWNLOADING);
    EXPECT_EQ(operation->downloadTryTime_, 2);
    
    MEDIA_INFO_LOG("cloud_asset_failed_network_test_001 End");
}

// 测试目标: 测试DownloadFileData结构体的各个字段
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_download_file_data_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_download_file_data_test_001 Start");
    
    CloudMediaAssetDownloadOperation::DownloadFileData data;
    EXPECT_EQ(data.pathVec.size(), 0);
    EXPECT_EQ(data.fileDownloadMap.Size(), 0);
    EXPECT_EQ(data.batchFileIdNeedDownload.size(), 0);
    EXPECT_EQ(data.batchSizeNeedDownload, 0);
    EXPECT_EQ(data.batchCountNeedDownload, 0);
    
    data.pathVec.push_back("/test/path1.jpg");
    data.fileDownloadMap.EnsureInsert("/test/path1.jpg", 1024);
    data.batchFileIdNeedDownload.push_back("1");
    data.batchSizeNeedDownload = 1024;
    data.batchCountNeedDownload = 1;
    
    EXPECT_EQ(data.pathVec.size(), 1);
    EXPECT_EQ(data.fileDownloadMap.Size(), 1);
    EXPECT_EQ(data.batchFileIdNeedDownload.size(), 1);
    EXPECT_EQ(data.batchSizeNeedDownload, 1024);
    EXPECT_EQ(data.batchCountNeedDownload, 1);
    
    MEDIA_INFO_LOG("cloud_asset_download_file_data_test_001 End");
}

// 测试目标: 测试多个任务状态组合
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_multi_status_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_multi_status_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::IDLE;
    operation->downloadType_ = CloudMediaDownloadType::DOWNLOAD_GENTLE;
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    operation->isThumbnailUpdate_ = true;
    operation->isBgDownloadPermission_ = false;
    operation->downloadId_ = -1;
    operation->totalCount_ = 0;
    operation->totalSize_ = 0;
    operation->remainCount_ = 0;
    operation->remainSize_ = 0;
    operation->downloadTryTime_ = 0;
    
    EXPECT_EQ(operation->GetTaskStatus(), CloudMediaAssetTaskStatus::IDLE);
    EXPECT_EQ(operation->GetDownloadType(), CloudMediaDownloadType::DOWNLOAD_GENTLE);
    EXPECT_EQ(operation->GetTaskPauseCause(), CloudMediaTaskPauseCause::NO_PAUSE);
    EXPECT_EQ(operation->GetTaskInfo(), "0,0,0,0");
    
    MEDIA_INFO_LOG("cloud_asset_multi_status_test_001 End");
}

// 测试目标: 测试HandleStoppedCallback在非DOWNLOADING状态
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_stopped_non_downloading_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_stopped_non_downloading_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.path = "/storage/cloud/files/test_stopped_non_dl.jpg";
    
    operation->downloadId_ = 1;
    operation->taskStatus_ = CloudMediaAssetTaskStatus::PAUSED;
    operation->dataForDownload_.fileDownloadMap.EnsureInsert(progress.path, 1024);
    
    operation->HandleStoppedCallback(progress);
    EXPECT_EQ(operation->cacheForDownload_.fileDownloadMap.Size(), 1);
    
    MEDIA_INFO_LOG("cloud_asset_stopped_non_downloading_test_001 End");
}

// 测试目标: 测试PauseDownloadTask多个暂停原因
HWTEST_F(MediaLibraryCloudAssetDownloadTest, cloud_asset_pause_multi_cause_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("cloud_asset_pause_multi_cause_test_001 Start");
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation = CloudMediaAssetDownloadOperation::GetInstance();
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    operation->downloadId_ = 1;
    
    operation->pauseCause_ = CloudMediaTaskPauseCause::NO_PAUSE;
    int32_t ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::TEMPERATURE_LIMIT);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::TEMPERATURE_LIMIT);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::NETWORK_FLOW_LIMIT);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::WIFI_UNAVAILABLE);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::POWER_LIMIT);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::POWER_LIMIT);
    
    operation->taskStatus_ = CloudMediaAssetTaskStatus::DOWNLOADING;
    ret = operation->PauseDownloadTask(CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(operation->pauseCause_, CloudMediaTaskPauseCause::FREQUENT_USER_REQUESTS);
    
    MEDIA_INFO_LOG("cloud_asset_pause_multi_cause_test_001 End");
}
}
}