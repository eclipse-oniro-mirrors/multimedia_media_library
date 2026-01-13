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

#define MLOG_TAG "CloudAssetBatchDownloadUnitTest"

#include "medialibrary_cloud_asset_batch_download_test.h"

#include "rdb_predicates.h"
#include "rdb_store.h"
#include "medialibrary_type_const.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "download_resources_column.h"
#include "download_resources_po.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "media_assets_controller_service.h"
#include "database_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore.h"
#include "cloud_media_dao_utils.h"
#include "start_batch_download_cloud_resources_vo.h"
#include "pause_batch_download_cloud_resources_vo.h"
#include "resume_batch_download_cloud_resources_vo.h"
#include "cancel_batch_download_cloud_resources_vo.h"
#include "get_batch_download_cloud_resources_status_vo.h"
#include "get_batch_download_cloud_resources_count_vo.h"
#include "batch_download_resources_task_dao.h"
#define private public
#define protected public
#include "cloud_media_asset_manager.h"
#undef protected
#undef private
#include "rdb_predicates.h"
#include "media_upgrade.h"


using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {

static std::vector<std::string> validUrisData_ = {
    "file://media/Photo/1/IMG_1755046662_091/IMG_001.jpg",
    "file://media/Photo/2/IMG_1755046662_092/IMG_002.jpg",
    "file://media/Photo/3/IMG_1755046662_093/IMG_003.jpg",
    "file://media/Photo/4/IMG_1755046662_094/IMG_004.jpg",
    "file://media/Photo/5/IMG_1755046662_095/IMG_005.jpg",
    "file://media/Photo/6/IMG_1755046662_095/IMG_006.jpg",
    "file://media/Photo/7/IMG_1755046662_096/IMG_007.jpg",
    "file://media/Photo/8/IMG_1755046662_097/IMG_008.jpg",
};

DataShare::DataSharePredicates predicates;

static const int IMG_SIZE = 175258;
static std::atomic<int> number_(0);
static const string IMG_BASE_NAME = "IMG_00";
static const string IMG_BASE_PATH = "/storage/cloud/files/Photo/16/";
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static BatchDownloadResourcesTaskDao dao;

int IncrementNum()
{
    return ++number_;
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return seconds.count() + IncrementNum();
}

string GetTitle(int64_t &timestamp)
{
    IncrementNum();
    return "IMG_" + to_string(timestamp) + "_" + to_string(number_.load());
}

static int32_t InsertPhoto(const MediaType &mediaType, int32_t position, int32_t coverLevel, std::string burstKey)
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = mediaType == MEDIA_TYPE_VIDEO ? (title + ".mp4") : (title + ".jpg");
    string path = "/storage/cloud/files/photo/1/" + displayName;
    int64_t videoSize = 1 * 1000 * 1000 * 1000;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t videoDuration = 0;
    int32_t imageDuration = 2560;
    int32_t videoWidth = 3072;
    int32_t imageWidth = 1920;
    int32_t videoHeight = 4096;
    int32_t imageHeight = 1080;
    string videoMimeType = "video/mp4";
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, mediaType == MEDIA_TYPE_VIDEO ? videoSize : imageSize);
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, mediaType == MEDIA_TYPE_VIDEO ? videoDuration : imageDuration);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, mediaType == MEDIA_TYPE_VIDEO ? videoWidth : imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, mediaType == MEDIA_TYPE_VIDEO ? videoHeight : imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, mediaType == MEDIA_TYPE_VIDEO ? videoMimeType : imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, coverLevel);
    valuesBucket.PutString(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    if (burstKey != "") {
        valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
    }
    g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    return static_cast<int32_t>(fileId);
}

static void PrepareInsertPhotos()
{
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 1, "");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 1,
        "339de221-75c5-436d-a41f-0608e58dc750");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 2, // 2 burst
        "339de221-75c5-436d-a41f-0608e58dc750");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 2, // 2 burst
        "339de221-75c5-436d-a41f-0608e58dc750");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::LOCAL), 1, "");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD), 1, "");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 1, "");
}

static void PrepareBatchDownloadDatas()
{
    for (int32_t index = 0; index < 6; index++) { // 6 count
        NativeRdb::ValuesBucket values;
        values.PutInt(DownloadResourcesColumn::MEDIA_ID, index);
        values.PutString(DownloadResourcesColumn::MEDIA_NAME, "filename");
        values.PutLong(DownloadResourcesColumn::MEDIA_SIZE, 555); // 555 size
        values.PutString(DownloadResourcesColumn::MEDIA_URI, validUrisData_.at(index));
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, 0);
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, 0);
        values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, 0);
        values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, 0);
        values.PutInt(DownloadResourcesColumn::MEDIA_COVER_LEVEL, 1);
        int64_t rowId = 0;
        g_rdbStore->Insert(rowId, DownloadResourcesColumn::TABLE, values);
        MEDIA_INFO_LOG("batch insert RowId %{public}" PRId64, rowId);
    }
}

static void InsertBatchDownloadData(int32_t fileId, std::string uri, int32_t status, int32_t percent)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(DownloadResourcesColumn::MEDIA_ID, fileId);
    values.PutString(DownloadResourcesColumn::MEDIA_NAME, "filename");
    values.PutLong(DownloadResourcesColumn::MEDIA_SIZE, 555); // 555 size
    values.PutString(DownloadResourcesColumn::MEDIA_URI, uri);
    values.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, 0);
    values.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, 0);
    values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, status);
    values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, percent);
    values.PutInt(DownloadResourcesColumn::MEDIA_COVER_LEVEL, 1);
    int64_t rowId = 0;
    g_rdbStore->Insert(rowId, DownloadResourcesColumn::TABLE, values);
}

static void PrepareBatchDownloadDatasWithDiffStatus()
{
    int32_t fileId = 1;
    int32_t pos = 0;
    InsertBatchDownloadData(fileId, validUrisData_.at(pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING), 0);
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING), 0);
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE), 0);
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL), 0);
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS), 0);
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE), 10); // 10 percent
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_PAUSE), 10); // 10 percent
    InsertBatchDownloadData(++fileId, validUrisData_.at(++pos),
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING), 10); // 10 percent
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        MEDIALIBRARY_TABLE,
        PhotoAlbumColumns::TABLE,
        DownloadResourcesColumn::TABLE,
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

void ResetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        DownloadResourcesColumn::CREATE_TABLE,
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

void ClearAndResetTable()
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
    ResetTables();
}

void MediaLibraryCloudAssetBatchDownloadTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "MediaLibraryCloudAssetBatchDownloadTest SetUpTestCase";
    MEDIA_INFO_LOG("MediaLibraryCloudAssetBatchDownloadTest SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryCloudAssetBatchDownloadTest SetUpTestCase failed, can not get g_rdbStore");
        exit(1);
    }
    ClearAndResetTable();
    MEDIA_INFO_LOG("End SetUpTestCase");
}

void MediaLibraryCloudAssetBatchDownloadTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryCloudAssetBatchDownloadTest TearDownTestCase");
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    ::system("rm -rf /storage/cloud/files/*");
    ClearAndResetTable();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    this_thread::sleep_for(chrono::seconds(1));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryCloudAssetBatchDownloadTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryCloudAssetBatchDownloadTest failed, can not get rdbstore");
        exit(1);
    }
    MEDIA_INFO_LOG("MediaLibraryCloudAssetBatchDownloadTest SetUp");
}

void MediaLibraryCloudAssetBatchDownloadTest::TearDown(void)
{
    ClearAndResetTable();
    MEDIA_INFO_LOG("MediaLibraryCloudAssetBatchDownloadTest TearDown");
}


void QueryInt(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &querySql, const string &columnName,
    int32_t &result)
{
    ASSERT_NE(rdbStore, nullptr);
    auto resultSet = rdbStore->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    result = GetInt32Val(columnName, resultSet);
    MEDIA_INFO_LOG("Query %{public}s result: %{public}d", querySql.c_str(), result);
}

int32_t QueryTasksCount()
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    const string sql = "SELECT COUNT(1) FROM " + DownloadResourcesColumn::TABLE;
    int32_t result = -1;
    QueryInt(g_rdbStore->GetRaw(), sql, "count(1)", result);
    return result;
}

int32_t QueryTasksCountByStatus(string &status)
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    const string sql = "SELECT COUNT(1) FROM " + DownloadResourcesColumn::TABLE + " where download_status = " + status;
    int32_t result = -1;
    QueryInt(g_rdbStore->GetRaw(), sql, "count(1)", result);
    return result;
}

vector<NativeRdb::ValuesBucket> GenTaskValues()
{
    int64_t timestamp = GetTimestamp();
    string displayName = "IMG_001.jpg";
    std::vector<NativeRdb::ValuesBucket> batchValues;

    for (const auto& uri : validUrisData_) {
        NativeRdb::ValuesBucket values;
        size_t tmp = uri.find_last_of('/');
        std::string fileName = uri.substr(tmp + 1);
        // "file://media/Photo/1/IMG_001/IMG_001.jpg",
        string fileId = MediaFileUri::GetPhotoId(uri);
        values.PutInt(DownloadResourcesColumn::MEDIA_ID, std::stoi(fileId)); //1
        values.PutString(DownloadResourcesColumn::MEDIA_NAME, fileName); //IMG_001.jpg
        values.PutLong(DownloadResourcesColumn::MEDIA_SIZE, IMG_SIZE);
        values.PutString(DownloadResourcesColumn::MEDIA_URI, IMG_BASE_PATH + fileName);
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, timestamp);
        values.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, 0);
        values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, 0);
        values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, -1);
        values.PutInt(DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON, 0);
        batchValues.push_back(values);
    }
    return batchValues;
}

// controller_service
HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, StartDownloadCloudResources_Test_Basic, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StartDownloadCloudResources_Test_Basic");
    MessageParcel data;
    MessageParcel reply;
    StartBatchDownloadCloudResourcesReqBody reqBody;
    reqBody.uris = validUrisData_;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->StartBatchDownloadCloudResources(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End StartDownloadCloudResources_Test_Basic");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, StartDownloadCloudResources_Test_InvalidUris, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start StartDownloadCloudResources_Test_InvalidUris");
    MessageParcel data;
    MessageParcel reply;
    StartBatchDownloadCloudResourcesReqBody reqBody;
    std::vector<std::string> invalidUris = {"file://media/Cloud/xxx", "file://media/Cloud/2"};
    reqBody.uris = invalidUris;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->StartBatchDownloadCloudResources(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End StartDownloadCloudResources_Test_InvalidUris");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, PauseDownloadCloudResources_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start PauseDownloadCloudResources_Test");
    MessageParcel data;
    MessageParcel reply;
    PauseBatchDownloadCloudResourcesReqBody reqBody;
    reqBody.uris = validUrisData_;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->PauseBatchDownloadCloudResources(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End PauseDownloadCloudResources_Test");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, ResumeDownloadCloudResources_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start ResumeDownloadCloudResources_Test");
    MessageParcel data;
    MessageParcel reply;
    ResumeBatchDownloadCloudResourcesReqBody reqBody;
    reqBody.uris = validUrisData_;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->ResumeBatchDownloadCloudResources(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End ResumeDownloadCloudResources_Test");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, CancelDownloadCloudResources_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start CancelDownloadCloudResources_Test");
    MessageParcel data;
    MessageParcel reply;
    CancelBatchDownloadCloudResourcesReqBody reqBody;
    reqBody.uris = validUrisData_;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->CancelBatchDownloadCloudResources(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End CancelDownloadCloudResources_Test");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, GetDownloadCloudResourcesStatus_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetDownloadCloudResourcesStatus_Test");
    MessageParcel data;
    MessageParcel reply;
    GetBatchDownloadCloudResourcesStatusReqBody reqBody;
    
    reqBody.predicates = predicates;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCloudMediaBatchDownloadResourcesStatus(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End GetDownloadCloudResourcesStatus_Test");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, GetDownloadCloudResourcesCount_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start GetDownloadCloudResourcesCount_Test");
    MessageParcel data;
    MessageParcel reply;
    GetBatchDownloadCloudResourcesCountReqBody reqBody;
    
    reqBody.predicates = predicates;
    auto ret = reqBody.Marshalling(data);
    EXPECT_EQ(ret, true);

    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCloudMediaBatchDownloadResourcesCount(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ret = respVo.Unmarshalling(reply);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("End GetDownloadCloudResourcesCount_Test");
}

// DAO
HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_BatchInsert, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_BatchInsert");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    EXPECT_EQ(ret, E_OK);
    auto count = QueryTasksCount();
    EXPECT_EQ(count, validUrisData_.size());
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_BatchInsert");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_FromUriToAllFileIds_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_FromUriToAllFileIds");
    std::vector<std::string> fileIds;
    auto ret = dao.FromUriToAllFileIds({}, fileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_FromUriToAllFileIds");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_FromUriToAllFileIds_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_FromUriToAllFileIds_02");
    std::vector<std::string> fileIds;
    auto ret = dao.FromUriToAllFileIds(validUrisData_, fileIds);
    EXPECT_EQ(ret, E_OK);

    std::vector<std::string> urisInValidMix = {"file://media/Photo/1/IMG_1755046662_091/IMG_001.jpg",
        "file://media/Photo/X/IMG_1755046662_095/IMG_005.jpg",
        "file://media/Photo/6/IMG_1755046662_095/IMG_006.jpg"};
    fileIds.clear();
    ret = dao.FromUriToAllFileIds(urisInValidMix, fileIds);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_FromUriToAllFileIds_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_01");
    std::vector<std::string> fileIds;
    auto ret = dao.AddOtherBurstIdsToFileIds(fileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_02");
    int32_t burstCoverId = InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 1, "abc");
    InsertPhoto(MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD), 0, "abc");
    std::vector<std::string> fileIds = { std::to_string(burstCoverId) };
    auto ret = dao.AddOtherBurstIdsToFileIds(fileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(fileIds.size(), 0);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_QueryDownloadResourcesInfoByUri_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_01");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    std::vector<DownloadResourcesTaskPo> newTaskPos;
    ret = dao.QueryValidBatchDownloadPoFromPhotos(validUrisData_, newTaskPos);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_AddOtherBurstIdsToFileIds_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_ClassifyExistedDownloadTasks_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_ClassifyExistedDownloadTasks_01");
    std::vector<std::string> allFileIds = {};
    std::vector<std::string> newTaskFileIds;
    std::vector<std::string> existedFileIds;
    auto ret = dao.ClassifyExistedDownloadTasks(allFileIds, newTaskFileIds, existedFileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_ClassifyExistedDownloadTasks_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_ClassifyExistedDownloadTasks_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_ClassifyExistedDownloadTasks_02");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    std::vector<std::string> allFileIds = {"1", "2", "3"};
    std::vector<std::string> newTaskFileIds;
    std::vector<std::string> existedFileIds;
    ret = dao.ClassifyExistedDownloadTasks(allFileIds, newTaskFileIds, existedFileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(existedFileIds.size(), 0);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_ClassifyExistedDownloadTasks_02");
}


HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_ClassifyInvalidDownloadTasks_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_ClassifyInvalidDownloadTasks_02");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    std::vector<std::string> allFileIds = {"1", "2", "7"};
    std::vector<std::string> invalidIds;
    ret = dao.ClassifyInvalidDownloadTasks(allFileIds, invalidIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(allFileIds.size(), 0);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_ClassifyInvalidDownloadTasks_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateExistedTasksStatus_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateExistedTasksStatus_01");
    std::vector<std::string> fileIds = {"4", "5", "6"};
    int32_t ret = dao.UpdateExistedTasksStatus(fileIds,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING), false);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateExistedTasksStatus_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateExistedTasksStatus_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateExistedTasksStatus_02");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    std::vector<std::string> allFileIds = {"1", "2", "7"};
    ret = dao.UpdateExistedTasksStatus(allFileIds, static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING),
        true);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateExistedTasksStatus_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateExistedTasksStatus_03, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateExistedTasksStatus_03");
    PrepareInsertPhotos();
    vector<NativeRdb::ValuesBucket> values = GenTaskValues();
    int64_t insertCount = 0;
    auto ret = dao.BatchInsert(insertCount, DownloadResourcesColumn::TABLE, values);
    std::vector<std::string> allFileIds = {"1", "2", "7"};
    ret = dao.UpdateExistedTasksStatus(allFileIds, static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_WAITING),
        false);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateExistedTasksStatus_03");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_HandleDuplicateAddTask_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_HandleDuplicateAddTask_01");
    PrepareBatchDownloadDatas();
    std::vector<DownloadResourcesTaskPo> taskPos;
    for (int32_t index = 0; index < 5; index++) {
        DownloadResourcesTaskPo taskPo;
        taskPo.fileId = index;
        taskPo.fileName = "filename" + to_string(index);
        taskPo.fileSize = 555;
        taskPo.dateAdded = 0;
        taskPo.dateFinish = 0;
        taskPo.downloadStatus = index; // TYPE_WAITING to TYPE_AUTO_PAUSE
        taskPo.percent = 0;
        taskPo.autoPauseReason = 0;
        taskPos.emplace_back(taskPo);
    }
    int32_t ret = dao.HandleDuplicateAddTask(taskPos);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_HandleDuplicateAddTask_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_HandleAddExistedDownloadTasks_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_HandleAddExistedDownloadTasks_01");
    std::vector<std::string> allFileIds = {};
    int32_t ret = dao.HandleAddExistedDownloadTasks(allFileIds);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_HandleAddExistedDownloadTasks_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_HandleAddExistedDownloadTasks_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_HandleAddExistedDownloadTasks_02");
    PrepareBatchDownloadDatas();
    std::vector<std::string> allFileIds = {"1", "2", "7"};
    int32_t ret = dao.HandleAddExistedDownloadTasks(allFileIds);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_HandleAddExistedDownloadTasks_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_01");
    std::vector<std::string> allFileIds = {};
    int32_t ret = dao.UpdateResumeDownloadResourcesInfo(allFileIds);
    EXPECT_EQ(ret, E_OK);
    PrepareBatchDownloadDatasWithDiffStatus();
    ret = dao.UpdateResumeDownloadResourcesInfo(validUrisData_);
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_02");
    PrepareBatchDownloadDatasWithDiffStatus();
    //resume part of tasks
    std::vector<string> lastThree(validUrisData_.begin() + 3, validUrisData_.end());
    int32_t ret = dao.UpdateResumeDownloadResourcesInfo(lastThree);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateResumeDownloadResourcesInfo_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateResumeAllDownloadResourcesInfo_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateResumeAllDownloadResourcesInfo_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    int32_t ret = dao.UpdateResumeAllDownloadResourcesInfo();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateResumeAllDownloadResourcesInfo_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdateAllPauseDownloadResourcesInfo_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdateAllPauseDownloadResourcesInfo_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    int32_t ret = dao.UpdateAllPauseDownloadResourcesInfo();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdateAllPauseDownloadResourcesInfo_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_QueryPauseDownloadingStatusResources_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_QueryPauseDownloadingStatusResources_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    std::vector<std::string> fileIdsDownloading;
    std::vector<std::string> fileIdsNotInDownloading;
    int32_t ret = dao.QueryPauseDownloadingStatusResources(validUrisData_, fileIdsDownloading,
        fileIdsNotInDownloading);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_QueryPauseDownloadingStatusResources_01");
}


HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    std::vector<std::string> allFileIds;
    dao.FromUriToAllFileIds(validUrisData_, allFileIds);
    int32_t ret = dao.UpdatePauseDownloadResourcesInfo(allFileIds);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_02,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_02");
    PrepareBatchDownloadDatasWithDiffStatus();
    //pause part of tasks
    std::vector<string> firstThree = {"1", "2", "3"};
    int32_t ret = dao.UpdatePauseDownloadResourcesInfo(firstThree);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_UpdatePauseDownloadResourcesInfo_02");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_DeleteAllDownloadResourcesInfo_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_DeleteAllDownloadResourcesInfo_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    int32_t ret = dao.DeleteAllDownloadResourcesInfo();
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_DeleteAllDownloadResourcesInfo_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_QueryCancelDownloadingStatusResources_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_QueryCancelDownloadingStatusResources_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    std::vector<std::string> fileIdsDownloading;
    std::vector<std::string> fileIdsNotInDownloading;
    int32_t ret = dao.QueryCancelDownloadingStatusResources(validUrisData_, fileIdsDownloading,
        fileIdsNotInDownloading);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_QueryCancelDownloadingStatusResources_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_DeleteCancelStateDownloadResources_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_DeleteCancelStateDownloadResources_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    int32_t ret = dao.DeleteCancelStateDownloadResources(validUrisData_);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_DeleteCancelStateDownloadResources_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesStatus_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesStatus_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    NativeRdb::RdbPredicates rdbPredicates(DownloadResourcesColumn::TABLE);
    std::vector<DownloadResourcesTaskPo> downloadResourcesTasks;
    int32_t ret = dao.QueryCloudMediaBatchDownloadResourcesStatus(rdbPredicates, downloadResourcesTasks);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesStatus_01");
}

HWTEST_F(MediaLibraryCloudAssetBatchDownloadTest, BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesCount_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Start BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesCount_01");
    PrepareBatchDownloadDatasWithDiffStatus();
    NativeRdb::RdbPredicates rdbPredicates(DownloadResourcesColumn::TABLE);
    int32_t count;
    int32_t ret = dao.QueryCloudMediaBatchDownloadResourcesCount(rdbPredicates, count);
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(count, 0);
    MEDIA_INFO_LOG("End BatchDownloadDaoTest_QueryCloudMediaBatchDownloadResourcesCount_01");
}
}
