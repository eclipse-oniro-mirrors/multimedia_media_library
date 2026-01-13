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

#define MLOG_TAG "AppUriPermissionOperationsTest"

#include "medialibrary_app_uri_permission_operations_test.h"

#include <vector>
#include <map>
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "medialibrary_app_uri_permission_operations.h"
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
#include "medialibrary_uripermission_operations.h"
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
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE,
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
        AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
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

void MediaLibraryAppUriPermissionOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAppUriPermissionOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryAppUriPermissionOperationsTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryAppUriPermissionOperationsTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryAppUriPermissionOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryAppUriPermissionOperationsTest::TearDown()
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
    dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket rdbValue = RdbUtils::ToValuesBucket(dataShareValue);
    cmd.SetValueBucket(rdbValue);
    int ret = MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
    return ret;
}

/**
 * insert a new persist read data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_001");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_001");
}

/**
 * insert a new temporary read data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_002");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_002");
}

/**
 * insert a repeat persist read data, and the origin data is temporary read.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_003");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    // step 1: insert a new temporary read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue01;
    dataShareValue01.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue01.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue01.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    ret = TestInsert(dataShareValue01);
    EXPECT_EQ(ret, 0);

    // step 2: insert a repeat persist read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue02;
    dataShareValue02.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue02.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue02.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue02);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_003");
}

/**
 * insert a repeat temporary read data, and the origin data is persist read.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_004");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    int64_t sourceId = 21;
    int64_t targetId = 12;
    // step 1: insert a new persist read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue01;
    dataShareValue01.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue01.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue01.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue01.Put(AppUriPermissionColumn::SOURCE_TOKENID, sourceId);
    dataShareValue01.Put(AppUriPermissionColumn::TARGET_TOKENID, targetId);
    ret = TestInsert(dataShareValue01);
    EXPECT_EQ(ret, 0);

    // step 2: insert a repeat temporary read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue02;
    dataShareValue02.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue02.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue02.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue02.Put(AppUriPermissionColumn::SOURCE_TOKENID, sourceId);
    dataShareValue02.Put(AppUriPermissionColumn::TARGET_TOKENID, targetId);
    ret = TestInsert(dataShareValue02);
    // expected result: 1, alread exist.
    EXPECT_EQ(ret, 1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_004");
}

/**
 * insert a repeat persist read data, and the origin data is persist read, too.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_005");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    int64_t sourceId = 21;
    int64_t targetId = 12;
    // step 1: insert a new persist read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue01;
    dataShareValue01.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue01.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue01.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue01.Put(AppUriPermissionColumn::SOURCE_TOKENID, sourceId);
    dataShareValue01.Put(AppUriPermissionColumn::TARGET_TOKENID, targetId);
    ret = TestInsert(dataShareValue01);
    // expected result: 0, success
    EXPECT_EQ(ret, 0);

    // step 2: insert a repeat persist read data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue02;
    dataShareValue02.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue02.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue02.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue02.Put(AppUriPermissionColumn::SOURCE_TOKENID, sourceId);
    dataShareValue02.Put(AppUriPermissionColumn::TARGET_TOKENID, targetId);
    ret = TestInsert(dataShareValue02);
    // expected result: 1, alread exist
    EXPECT_EQ(ret, 1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_005");
}

/**
 * insert a new data, but this data lacks the fileId.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_006");

    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue01;
    dataShareValue01.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue01.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue01);
    // expected result: -1, failed.
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_006");
}

int TestDelete(OHOS::DataShare::DataSharePredicates &dataSharePredicate)
{
    // MEDIALIBRARY_TABLE just fro RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(dataSharePredicate,
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    int ret = MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
    return ret;
}

/**
 * cancel a data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_007");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    int ret = -1;
    // step 1: insert a new data.
    OHOS::DataShare::DataShareValuesBucket dataShareValue01;
    dataShareValue01.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue01.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue01.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue01);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    // step 2: cancel the data.
    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestDelete(dataSharePredicate);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_007");
}

int TestBatchInsert(std::vector<DataShare::DataShareValuesBucket> &dataShareValues)
{
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    return MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, dataShareValues);
}

/**
 * batch insert two new temporary read data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_008");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_008");
}

/**
 * batch insert two repeat persist read data, and the two origin data is temporary read.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_009");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues02;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues02.push_back(dataShareValue);
    }
    ret = TestBatchInsert(dataShareValues02);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_009");
}

/**
 * batch insert two repeat persist read data, and the two origin data is persist read, too.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_010");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues02;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues02.push_back(dataShareValue);
    }
    ret = TestBatchInsert(dataShareValues02);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_010");
}

/**
 * batch insert two temporary read data.
 * The first data is repeat data, and the permission of origin data is persist read.
 * The second data is new.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_011");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo2.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId1);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue);
    int ret = TestBatchInsert(dataShareValues);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    std::vector<int32_t> photoIds = { photoId1, photoId2 };
    std::vector<DataShare::DataShareValuesBucket> dataShareValues02;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues02.push_back(dataShareValue);
    }
    ret = TestBatchInsert(dataShareValues02);
    // expected result: 0, success.
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_011");
}

/**
 * batchinsert a empty data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_012");

    std::vector<DataShare::DataShareValuesBucket> values;
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::function<int(void)> func = [&]()->int {
        return MediaLibraryAppUriPermissionOperations::BatchInsertInner(cmd, values, trans);
    };
    auto ret = trans->RetryTrans(func);
    // expected result: -1. empty buckets
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_012");
}

HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, medialibrary_grant_permission_test_001, TestSize.Level1)
{
    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::SOURCE_TOKENID, 1);
        dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, 2);
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, to_string(i));
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_WRITE);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    MediaLibraryCommand cmd(OperationObject::APP_URI_PERMISSION_INNER, OperationType::CREATE,
        MediaLibraryApi::API_10);
    int32_t ret = UriPermissionOperations::GrantUriPermission(cmd, dataShareValues);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}
} // namespace Media
} // namespace OHOS