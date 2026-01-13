/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "medialibrary_cover_position_parser_test.h"

#define private public
#include "cover_position_parser.h"
#include "picture_data_operations.h"
#undef private
#include "abs_rdb_predicates.h"
#include "duplicate_photo_operation.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_operation.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "media_audio_column.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataSharePredicates;
using OHOS::DataShare::DataShareValuesBucket;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static shared_ptr<PictureDataOperations> s_pictureDataOperations = nullptr;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const unsigned char FILE_TEST_LIVE_PHOTO[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02, 0x03, 0x04, 0xFF, 0xD9, 0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70,
    0x69, 0x73, 0x6F, 0x6D, 0x01, 0x02, 0x03, 0x04, 0x76, 0x33, 0x5f, 0x66, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x3a, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x4c, 0x49, 0x56, 0x45, 0x5f, 0x33,
    0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE, AudioColumn::AUDIOS_TABLE, MEDIALIBRARY_TABLE,
        ASSET_UNIQUE_NUMBER_TABLE, PhotoExtColumn::PHOTOS_EXT_TABLE, PhotoAlbumColumns::TABLE};
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

void PrepareUniqueNumberTable()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbstore");
        return;
    }

    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = g_rdbStore->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return;
    }

    UniqueMemberValuesBucket imageBucket = {IMAGE_ASSET_TYPE, 1};
    UniqueMemberValuesBucket videoBucket = {VIDEO_ASSET_TYPE, 1};
    UniqueMemberValuesBucket audioBucket = {AUDIO_ASSET_TYPE, 1};

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {imageBucket, videoBucket, audioBucket};

    for (const auto &uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = g_rdbStore->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
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
        CREATE_MEDIA_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
        PhotoAlbumColumns::CREATE_TABLE
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

static tuple<int32_t, string> QueryPhotoDataByDisplayName(const string &displayName)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = {MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Read moving photo query photo id by display name failed");
        return {-1, ""};
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    return {fileId, filePath};
}

static string CreateMovingPhoto()
{
    // create moving photo with live photo
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);
    ValuesBucket valuesBucket;
    valuesBucket.PutString(ASSET_EXTENTION, "jpg");
    valuesBucket.PutString(PhotoColumn::MEDIA_TITLE, "moving_photo_metadata_test");
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MediaType::MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int>(PhotoSubType::DEFAULT));
    cmd.SetValueBucket(valuesBucket);
    cmd.SetBundleName("test_bundle_name");
    MediaLibraryPhotoOperations::Create(cmd);
    auto [fileId, filePath] = QueryPhotoDataByDisplayName("moving_photo_metadata_test.jpg");

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    string fileUriStr = fileUri.ToString();
    Uri uri(fileUriStr);
    MediaLibraryCommand openLivePhotoCmd(uri, Media::OperationType::OPEN);
    int32_t livePhotoFd = MediaLibraryPhotoOperations::Open(openLivePhotoCmd, "w");
    int32_t resWrite = write(livePhotoFd, FILE_TEST_LIVE_PHOTO, sizeof(FILE_TEST_LIVE_PHOTO));
    close(livePhotoFd);

    MediaLibraryCommand closeFileCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    ValuesBucket closeValues;
    closeValues.PutString(MEDIA_DATA_DB_URI, fileUriStr);
    closeFileCmd.SetValueBucket(closeValues);
    MediaLibraryPhotoOperations::Close(closeFileCmd);
    return filePath;
}

void MediaLibraryCoverPositionParserTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    s_pictureDataOperations = make_shared<PictureDataOperations>();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryCoverPositionParserTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void MediaLibraryCoverPositionParserTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryCoverPositionParserTest::TearDown(void) {}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : AddTask
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add task success
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_AddTask_test_001, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->processing_ = true;
    bool ret = instance->AddTask("/test", "file://media");
    EXPECT_EQ(ret, true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : AddTask
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add task fail
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_AddTask_test_002, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->processing_ = true;
    bool ret = false;
    for (int i = 0; i < 101; i++) {
        ret = instance->AddTask("/test" + to_string(i), "file://media/" + to_string(i));
    }
    EXPECT_EQ(ret, false);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : AddTask
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_AddTask_test_003, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    string path = "/storage/cloud/files/Photo/1/IMG_000000000_000.jpg";
    string fileUri = "file://media/Photo/38/IMG_000000000_000/111.jpg";
    bool ret = instance->AddTask(path, fileUri);
    EXPECT_EQ(ret, true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : AddTask
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_AddTask_test_004, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    string path = "/storage/cloud/files/Photo/1/IMG_000000000_000.jpg";
    string fileUri = "file://media/Photo/38/IMG_000000000_000/111.jpg";
    instance->tasks_.push(make_pair(path, fileUri));
    bool ret = instance->AddTask(path, fileUri);
    EXPECT_EQ(ret, true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : StartTask
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_StartTask_test_001, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->StartTask();
    EXPECT_TRUE(true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : ProcessCoverPosition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_ProcessCoverPosition_test_001, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->ProcessCoverPosition();
    EXPECT_TRUE(true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : ProcessCoverPosition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_ProcessCoverPosition_test_002, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->processing_ = true;
    string path = "/storage/cloud/files/Photo/1/IMG_000000000_000.jpg";
    string fileUri = "file://media/Photo/38/IMG_000000000_000/111.jpg";
    instance->tasks_.push(make_pair(path, fileUri));
    instance->ProcessCoverPosition();
    EXPECT_EQ(instance->tasks_.size(), 0);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : ProcessCoverPosition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_ProcessCoverPosition_test_003, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->processing_ = true;
    string path = "/storage/cloud/files/Photo/1/IMG_000000000_000.jpg";
    string fileUri = "file://media/Photo/38/IMG_000000000_000/111.jpg";
    bool ret = instance->AddTask(path, fileUri);
    instance->ProcessCoverPosition();
    EXPECT_EQ(ret, true);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : UpdateCoverPosition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_UpdateCoverPosition_test_001, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    string path = "/storage/cloud/files/Photo/1/IMG_000000000_000.jpg";
    instance->UpdateCoverPosition(path);
    EXPECT_EQ(instance->tasks_.size(), 0);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : UpdateCoverPosition
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_UpdateCoverPosition_test_002, TestSize.Level1)
{
    string path = CreateMovingPhoto();
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    instance->UpdateCoverPosition(path);
    EXPECT_EQ(instance->tasks_.size(), 0);
}

/*
 * Feature : MediaLibraryCoverPositionParserTest
 * Function : SendUpdateNotify
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : add and start task
 */
HWTEST_F(MediaLibraryCoverPositionParserTest, medialib_SendUpdateNotify_test_001, TestSize.Level1)
{
    shared_ptr<CoverPositionParser> instance = make_shared<CoverPositionParser>();
    string fileUri = "file://media/Photo/38/IMG_000000000_000/111.jpg";
    instance->SendUpdateNotify(fileUri);
    EXPECT_EQ(instance->tasks_.size(), 0);
}
} // namespace Media
} // namespace OHOS