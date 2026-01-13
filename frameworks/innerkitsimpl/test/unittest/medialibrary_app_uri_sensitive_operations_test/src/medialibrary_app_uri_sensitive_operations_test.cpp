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

#define MLOG_TAG "AppUriSensitiveOperationsTest"

#include "medialibrary_app_uri_sensitive_operations_test.h"

#include <vector>
#include <map>
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "medialibrary_app_uri_sensitive_operations.h"
#include "datashare_predicates_objects.h"
#include "medialibrary_appstate_observer.h"
#include "media_file_uri.h"
#include "rdb_utils.h"
#include "datashare_predicates.h"
#include "medialibrary_rdb_utils.h"
#include "abs_rdb_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "photo_album_column.h"
#include "media_audio_column.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        AudioColumn::AUDIOS_TABLE,
        AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE,
        MEDIALIBRARY_TABLE,
        ASSET_UNIQUE_NUMBER_TABLE,
        PhotoExtColumn::PHOTOS_EXT_TABLE
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

struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void PrepareUniqueNumberTable()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbstore");
        return;
    }
    auto store = g_rdbStore;
    if (store == nullptr) {
        MEDIA_ERR_LOG("can not get store");
        return;
    }
    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = store->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return;
    }

    UniqueMemberValuesBucket imageBucket = { IMAGE_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket videoBucket = { VIDEO_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket audioBucket = { AUDIO_ASSET_TYPE, 1 };

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {
        imageBucket, videoBucket, audioBucket
    };

    for (const auto& uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = store->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
        }
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        AudioColumn::CREATE_AUDIO_TABLE,
        AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE,
        CREATE_MEDIA_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE
        // todo: album tables
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    PrepareUniqueNumberTable();
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void MediaLibraryAppUriSensitiveOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAppUriSensitiveOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryAppUriSensitiveOperationsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryAppUriSensitiveOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAppUriSensitiveOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryAppUriSensitiveOperationsTest::TearDown()
{}

int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}

int TestInsert(DataShareValuesBucket &dataShareValue)
{
    dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket rdbValue = RdbUtils::ToValuesBucket(dataShareValue);
    cmd.SetValueBucket(rdbValue);
    int ret = MediaLibraryAppUriSensitiveOperations::HandleInsertOperation(cmd);
    return ret;
}

/**
 * normal, update
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_001");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue02;
    dataShareValue02.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue02.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue02.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_NO_DESENSITIZE);
    dataShareValue02.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue02);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_001");
}

/**
 * no sensitiveType
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_002");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_002");
}

/**
 * not valid
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_003");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, 4);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_003");
}

/**
 * no appid
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_004");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    int64_t targetId = 21;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, targetId);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::SUCCEED);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_004");
}

/**
 * no fileId
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_005");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_005");
}

/**
 * no permissionType
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_006");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_006");
}

/**
 * extra column fail
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_007");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue.Put("extraColumn", "extraColumn");
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_007");
}

int TestDelete(OHOS::DataShare::DataSharePredicates &dataSharePredicate)
{
    // MEDIALIBRARY_TABLE just fro RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(dataSharePredicate,
        AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    int ret = MediaLibraryAppUriSensitiveOperations::DeleteOperation(rdbPredicate);
    return ret;
}

/**
 * cancel a data.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_008");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
        AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriSensitiveColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriSensitiveColumn::FILE_ID, photoId);
    ret = TestDelete(dataSharePredicate);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_008");
}

int TestBatchInsert(std::vector<DataShare::DataShareValuesBucket> &dataShareValues)
{
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    return MediaLibraryAppUriSensitiveOperations::BatchInsert(cmd, dataShareValues);
}

/**
 * batch insert two.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_009");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
            AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
            AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_009");
}

/**
 * batch insert two no sensitiveType.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_0010, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_0010");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
            AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_0010");
}

/**
 * batch update two no sensitiveType.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, app_uri_sensitive_oprn_api12_test_0011, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_sensitive_oprn_api12_test_0011");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
            AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, MediaLibraryAppUriSensitiveOperations::ERROR);

    MEDIA_INFO_LOG("end tdd app_uri_sensitive_oprn_api12_test_0011");
}

/**
 * BeForceSensitive_test.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, Mausot_BeForceSensitive_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd Mausot_BeForceSensitive_test_001");
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    bool ret = false;
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
            AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    ret = MediaLibraryAppUriSensitiveOperations::BeForceSensitive(cmd, dataShareValues);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("end tdd Mausot_BeForceSensitive_test_001");
}

/**
 * BeForceSensitive_test.
 */
HWTEST_F(MediaLibraryAppUriSensitiveOperationsTest, Mausot_BeForceSensitive_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd Mausot_BeForceSensitive_test_002");
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    bool ret = false;
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriSensitiveColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriSensitiveColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE,
            AppUriSensitiveColumn::SENSITIVE_ALL_DESENSITIZE);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE,
            AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriSensitiveColumn::URI_TYPE, AppUriSensitiveColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    ret = MediaLibraryAppUriSensitiveOperations::BeForceSensitive(cmd, dataShareValues);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("end tdd Mausot_BeForceSensitive_test_002");
}
} // namespace Media
} // namespace OHOS