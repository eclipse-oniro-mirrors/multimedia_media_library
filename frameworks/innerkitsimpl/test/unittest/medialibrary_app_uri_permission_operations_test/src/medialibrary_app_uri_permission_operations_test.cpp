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
        CONST_MEDIALIBRARY_TABLE,
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

    UniqueMemberValuesBucket imageBucket = { CONST_IMAGE_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket videoBucket = { CONST_VIDEO_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket audioBucket = { CONST_AUDIO_ASSET_TYPE, 1 };

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
    // CONST_MEDIALIBRARY_TABLE just fro RdbPredicates
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

/**
 * QueryOperation_test - query existing data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_013");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    std::vector<std::string> fetchColumns = {AppUriPermissionColumn::APP_ID, AppUriPermissionColumn::FILE_ID};
    auto resultSet = MediaLibraryAppUriPermissionOperations::QueryOperation(dataSharePredicate, fetchColumns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_013");
}

/**
 * QueryOperation_test - query non-existing data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_014");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "nonexist_appid");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    std::vector<std::string> fetchColumns = {AppUriPermissionColumn::APP_ID, AppUriPermissionColumn::FILE_ID};
    auto resultSet = MediaLibraryAppUriPermissionOperations::QueryOperation(dataSharePredicate, fetchColumns);
    EXPECT_EQ(resultSet, nullptr);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_014");
}

/**
 * DeleteOperation_test - delete non-existing data.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_015");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "nonexist_appid");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    int ret = TestDelete(dataSharePredicate);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_015");
}

/**
 * InsertOperation_test - insert with URI_AUDIO.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_016");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_AUDIO);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_016");
}

/**
 * InsertOperation_test - insert with different PERMISSION_TYPE.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_017");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::vector<int> permissionTypes = {
        AppUriPermissionColumn::PERMISSION_TEMPORARY_WRITE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ_WRITE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE,
        AppUriPermissionColumn::PERMISSION_PERSIST_WRITE
    };

    for (auto permissionType : permissionTypes) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
        int ret = TestInsert(dataShareValue);
        EXPECT_EQ(ret, 0);
    }

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_017");
}

/**
 * InsertOperation_test - insert with SOURCE_TOKENID and TARGET_TOKENID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_018");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put(AppUriPermissionColumn::SOURCE_TOKENID, (int64_t)1001);
    dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, (int64_t)2001);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_018");
}

/**
 * BatchInsert_test - insert with different URI_TYPE.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_019");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    
    OHOS::DataShare::DataShareValuesBucket dataShareValue1;
    dataShareValue1.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
    dataShareValue1.Put(AppUriPermissionColumn::FILE_ID, photoId1);
    dataShareValue1.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue1.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue1);

    OHOS::DataShare::DataShareValuesBucket dataShareValue2;
    dataShareValue2.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
    dataShareValue2.Put(AppUriPermissionColumn::FILE_ID, photoId2);
    dataShareValue2.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue2.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_AUDIO);
    dataShareValues.push_back(dataShareValue2);

    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_019");
}

/**
 * BatchInsert_test - insert with different PERMISSION_TYPE.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_020");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::vector<int> permissionTypes = {
        AppUriPermissionColumn::PERMISSION_TEMPORARY_WRITE,
        AppUriPermissionColumn::PERMISSION_TEMPORARY_READ_WRITE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE,
        AppUriPermissionColumn::PERMISSION_PERSIST_WRITE
    };

    for (auto permissionType : permissionTypes) {
        std::vector<DataShare::DataShareValuesBucket> dataShareValues;
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
        int ret = TestBatchInsert(dataShareValues);
        EXPECT_EQ(ret, 0);
    }

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_020");
}

/**
 * InsertOperation_test - insert with extra column.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_021, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_021");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put("extraColumn", "extraColumn");
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_021");
}

/**
 * QueryOperation_test - query with empty fetchColumns.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_022, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_022");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    std::vector<std::string> fetchColumns;
    auto resultSet = MediaLibraryAppUriPermissionOperations::QueryOperation(dataSharePredicate, fetchColumns);
    ASSERT_NE(resultSet, nullptr);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_022");
}

/**
 * DeleteOperation_test - delete with empty predicates.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_023, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_023");
    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    int ret = TestDelete(dataSharePredicate);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_023");
}

/**
 * BatchInsert_test - insert multiple items with same FILE_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_024, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_024");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 3; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid" + to_string(i));
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_024");
}

/**
 * InsertOperation_test - insert with large FILE_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_025, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_025");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, 2147483647);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_025");
}

/**
 * BatchInsert_test - insert with TOKENID instead of APP_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_026, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_026");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::SOURCE_TOKENID, 4);
        dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, 40);
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, to_string(i == 0 ? photoId1 : photoId2));
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_026");
}

/**
 * InsertOperation_test - insert with negative FILE_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_027, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_027");

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, -1);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_027");
}

/**
 * BatchInsert_test - insert with mixed APP_ID and TOKENID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_028, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_028");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    
    OHOS::DataShare::DataShareValuesBucket dataShareValue1;
    dataShareValue1.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue1.Put(AppUriPermissionColumn::FILE_ID, photoId1);
    dataShareValue1.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue1.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue1);

    OHOS::DataShare::DataShareValuesBucket dataShareValue2;
    dataShareValue2.Put(AppUriPermissionColumn::SOURCE_TOKENID, 4);
    dataShareValue2.Put(AppUriPermissionColumn::TARGET_TOKENID, 40);
    dataShareValue2.Put(AppUriPermissionColumn::FILE_ID, to_string(photoId2));
    dataShareValue2.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue2.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue2);

    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_028");
}

/**
 * InsertOperation_test - insert with zero FILE_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_029, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_029");

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, 0);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_029");
}

/**
 * BatchInsert_test - insert with duplicate items.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_030, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_030");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 2; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_030");
}

/**
 * InsertOperation_test - insert without APP_ID or TOKENID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_031, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_031");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_031");
}

/**
 * DeleteOperation_test - delete with multiple conditions.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_032, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_032");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestDelete(dataSharePredicate);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_032");
}

/**
 * BatchInsert_test - insert with invalid URI_TYPE.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_033, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_033");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, 999);
    dataShareValues.push_back(dataShareValue);
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_033");
}

/**
 * InsertOperation_test - insert with invalid PERMISSION_TYPE.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_034, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_034");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, 999);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, -1);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_034");
}

/**
 * QueryOperation_test - query with multiple conditions.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_035, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_035");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::PERMISSION_TYPE,
        AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    std::vector<std::string> fetchColumns = {AppUriPermissionColumn::APP_ID, AppUriPermissionColumn::FILE_ID};
    auto resultSet = MediaLibraryAppUriPermissionOperations::QueryOperation(dataSharePredicate, fetchColumns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_035");
}

/**
 * BatchInsert_test - insert with large number of items.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_036, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_036");
    std::vector<int32_t> photoIds;
    for (int i = 0; i < 10; ++i) {
        int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
        ASSERT_GT(photoId, 0);
        photoIds.push_back(photoId);
    }

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    for (int i = 0; i < 10; ++i) {
        OHOS::DataShare::DataShareValuesBucket dataShareValue;
        dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appidBatch01");
        dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoIds[i]);
        dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
        dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        dataShareValues.push_back(dataShareValue);
    }
    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_036");
}

/**
 * InsertOperation_test - insert then delete then insert again.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_037, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_037");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::FILE_ID, photoId);
    ret = TestDelete(dataSharePredicate);
    EXPECT_EQ(ret, 0);

    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_037");
}

/**
 * BatchInsert_test - insert with different APP_IDs.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_038, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_038");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    std::vector<DataShare::DataShareValuesBucket> dataShareValues;
    
    OHOS::DataShare::DataShareValuesBucket dataShareValue1;
    dataShareValue1.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue1.Put(AppUriPermissionColumn::FILE_ID, photoId1);
    dataShareValue1.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue1.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue1);

    OHOS::DataShare::DataShareValuesBucket dataShareValue2;
    dataShareValue2.Put(AppUriPermissionColumn::APP_ID, "appid02");
    dataShareValue2.Put(AppUriPermissionColumn::FILE_ID, photoId2);
    dataShareValue2.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_READ);
    dataShareValue2.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    dataShareValues.push_back(dataShareValue2);

    int ret = TestBatchInsert(dataShareValues);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_038");
}

/**
 * InsertOperation_test - insert with very long APP_ID.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_039, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_039");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    std::string longAppId(1000, 'a');
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, longAppId);
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_039");
}

/**
 * DeleteOperation_test - delete by APP_ID only.
 */
HWTEST_F(MediaLibraryAppUriPermissionOperationsTest, app_uri_permission_oprn_api12_test_040, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd app_uri_permission_oprn_api12_test_040");
    int32_t photoId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    int32_t photoId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId1, 0);
    ASSERT_GT(photoId2, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue1;
    dataShareValue1.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue1.Put(AppUriPermissionColumn::FILE_ID, photoId1);
    dataShareValue1.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    int ret = TestInsert(dataShareValue1);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataShareValuesBucket dataShareValue2;
    dataShareValue2.Put(AppUriPermissionColumn::APP_ID, "appid01");
    dataShareValue2.Put(AppUriPermissionColumn::FILE_ID, photoId2);
    dataShareValue2.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue2);
    EXPECT_EQ(ret, 0);

    OHOS::DataShare::DataSharePredicates dataSharePredicate;
    dataSharePredicate.And()->EqualTo(AppUriPermissionColumn::APP_ID, "appid01");
    ret = TestDelete(dataSharePredicate);
    EXPECT_EQ(ret, 0);

    MEDIA_INFO_LOG("end tdd app_uri_permission_oprn_api12_test_040");
}

} // namespace Media
} // namespace OHOS