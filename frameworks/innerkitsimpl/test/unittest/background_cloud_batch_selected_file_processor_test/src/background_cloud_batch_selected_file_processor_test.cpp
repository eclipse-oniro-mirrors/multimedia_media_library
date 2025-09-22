/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "BackgroundCloudBatchSelectedFileProcessorTest"

#include "background_cloud_batch_selected_file_processor_test.h"

#include <chrono>
#include <thread>

#define private public
#include "background_cloud_batch_selected_file_processor.h"
#undef private

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include "download_resources_column.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

static shared_ptr<MediaLibraryRdbStore> rdbStore;
static std::atomic<int> num{ 0 };

static constexpr int64_t SEC_TO_MSEC = 1e3;

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((rdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        // MEDIALIBRARY_TABLE,
        // PhotoAlbumColumns::TABLE,
        DownloadResourcesColumn::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = rdbStore->ExecuteSql(dropSql);
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
        PhotoColumn::CREATE_PHOTO_TABLE,
        // CREATE_MEDIA_TABLE,
        // PhotoAlbumColumns::CREATE_TABLE,
        DownloadResourcesColumn::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = rdbStore->ExecuteSql(createTableSql);
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

inline void IncrementNum()
{
    ++num;
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    IncrementNum();
    return seconds.count() + num.load();
}

string GetTitle(int64_t &timestamp)
{
    IncrementNum();
    return "IMG_" + to_string(timestamp) + "_" + to_string(num.load());
}

string InsertPhoto(const MediaType &mediaType, int32_t position)
{
    EXPECT_NE((rdbStore == nullptr), true);

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
    int32_t ret = rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    return path;
}

vector<string> PreparePhotos(const int count, const MediaType &mediaType, int32_t position)
{
    vector<string> photos;
    for (size_t index = 0; index < count; ++index) {
        string path = InsertPhoto(mediaType, position);
        photos.push_back(path);
    }
    return photos;
}

void InsertBatchDownloadTask(int32_t fileId, std::string path, std::string displayName, int32_t status)
{
    EXPECT_NE((rdbStore == nullptr), true);
    int64_t rowId = -1;
    NativeRdb::ValuesBucket values;
    values.PutInt(DownloadResourcesColumn::MEDIA_ID, fileId);
    values.PutString(DownloadResourcesColumn::MEDIA_NAME, displayName);
    values.PutLong(DownloadResourcesColumn::MEDIA_SIZE, 3096); // 3096 size
    values.PutString(DownloadResourcesColumn::MEDIA_URI, path);
    values.PutLong(DownloadResourcesColumn::MEDIA_DATE_ADDED, 0);
    values.PutLong(DownloadResourcesColumn::MEDIA_DATE_FINISH, 0);
    values.PutInt(DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS, status);
    values.PutInt(DownloadResourcesColumn::MEDIA_PERCENT, -1);
    int32_t ret = rdbStore->Insert(rowId, DownloadResourcesColumn::TABLE, values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertBatchDownloadTask fileId is %{public}s", to_string(fileId).c_str());
}

vector<string> PrepareBatchDownloadTask(const int32_t count)
{
    vector<string> uris;
    for (int32_t index = 1; index <= count; ++index) {
        int64_t timestamp = GetTimestamp();
        string title = GetTitle(timestamp);
        string displayName = title + to_string(index) + ".jpg";
        string path = "file://media/Photo/" + to_string(index) + "/" + displayName;
        InsertBatchDownloadTask(index, path, displayName, 0);
        uris.push_back(path);
    }
    return uris;
}

int32_t QueryPhotosCount()
{
    EXPECT_NE((rdbStore == nullptr), true);

    const string sql = "SELECT COUNT( * ) AS count FROM " + PhotoColumn::PHOTOS_TABLE;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);

    int32_t count = GetInt32Val("count", resultSet);
    EXPECT_GE(count, 0);

    MEDIA_INFO_LOG("Photos Count is %{public}d", count);
    return count;
}

int32_t QueryTasksCountByStatus(Media::BatchDownloadStatusType status)
{
    EXPECT_NE((rdbStore == nullptr), true);
    const string sql = "SELECT COUNT(1) FROM " + DownloadResourcesColumn::TABLE + " WHERE " +
        DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS + " = " + to_string(static_cast<int32_t>(status));
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t count = GetInt32Val("count(1)", resultSet);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("Tasks Count is %{public}d", count);
    return count;
}

int32_t QueryBatchDownloadTasksCount()
{
    EXPECT_NE((rdbStore == nullptr), true);
    const string sql = "SELECT COUNT( * ) AS count FROM " + DownloadResourcesColumn::TABLE;
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(sql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t count = GetInt32Val("count", resultSet);
    EXPECT_GE(count, 0);
    MEDIA_INFO_LOG("Tasks Count is %{public}d", count);
    return count;
}

std::vector<std::string> QueryCanDownloadFiles()
{
    std::vector<std::string> curDownloadFiles;
    auto resultSet = BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedResourceFiles();
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query cloud files!");
        return curDownloadFiles;
    }

    vector<std::string> toDoUris;
    vector<int32_t> localFileIds;
    vector<int32_t> exceptionFileIds;
    BackgroundCloudBatchSelectedFileProcessor::ParseBatchSelectedToDoFiles(resultSet, toDoUris, localFileIds,
        exceptionFileIds);
    if (toDoUris.empty()) {
        MEDIA_INFO_LOG("No cloud files need to be downloaded");
        return curDownloadFiles;
    }
    return toDoUris;
}

void BackgroundCloudBatchSelectedFileProcessorTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("BackgroundCloudBatchSelectedFileProcessorTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
}

void BackgroundCloudBatchSelectedFileProcessorTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("BackgroundCloudBatchSelectedFileProcessorTest TearDownTestCase");
    ClearAndResetTable();
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.clear();
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_.clear();
    BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_.clear();
}

void BackgroundCloudBatchSelectedFileProcessorTest::SetUp()
{
    MEDIA_INFO_LOG("BackgroundCloudBatchSelectedFileProcessorTest SetUp");
    ClearAndResetTable();
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.clear();
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_.clear();
    BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_.clear();
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
}

void BackgroundCloudBatchSelectedFileProcessorTest::TearDown()
{
    MEDIA_INFO_LOG("BackgroundCloudBatchSelectedFileProcessorTest TearDown");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_QueryBatchSelectedResourceFiles_NormalObj_01,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_QueryBatchSelectedResourceFiles_NormalObj_01 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);

    PrepareBatchDownloadTask(10);
    std::vector<std::string> curDownloadFiles = QueryCanDownloadFiles();
    EXPECT_NE(curDownloadFiles.size(), 0);
    MEDIA_INFO_LOG("Bcbsfpt_QueryBatchSelectedResourceFiles_NormalObj_01 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_GetStorageFreeRatio_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_GetStorageFreeRatio_Test_001 Start");
    double freeRatio = 1024;
    bool ret;
    ret = BackgroundCloudBatchSelectedFileProcessor::GetStorageFreeRatio(freeRatio);
    BackgroundCloudBatchSelectedFileProcessor::CanAutoRestoreCondition();
    BatchDownloadAutoPauseReasonType autoPauseReason;
    BackgroundCloudBatchSelectedFileProcessor::CanAutoStopCondition(autoPauseReason);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("Bcbsfpt_GetStorageFreeRatio_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_RemoveFinishedResult_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_RemoveFinishedResult_Test_001 Start");
    std::vector<std::string> downloadingPaths = { "file1", "file2", "file3" };
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"file1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT},
        {"file2", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS},
        {"file3", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::NETWORK_UNAVAILABLE}
    };
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_ = {};
    BackgroundCloudBatchSelectedFileProcessor::RemoveFinishedResult();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::downloadResult_.empty(), true);
    MEDIA_INFO_LOG("Bcbsfpt_RemoveFinishedResult_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_GetCurrentRoundDownloading_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_GetCurrentRoundDownloading_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "30";
    currentDownloadFileInfo.percent = 100;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_ = {};
    std::string fileIdsStr = "";
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::GetCurrentRoundInDownloadingFileIdList(fileIdsStr), false);
    
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::GetCurrentRoundInDownloadingFileIdList(fileIdsStr), true);
    EXPECT_EQ(fileIdsStr, currentDownloadFileInfo.fileId);
    MEDIA_INFO_LOG("Bcbsfpt_GetCurrentRoundDownloading_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_ClassifyCurrentRountFileId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_ClassifyCurrentRountFileId_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "30";
    currentDownloadFileInfo.percent = 100;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_ = {};
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    std::vector<std::string> fileIds = {"20", "30"};
    std::vector<int64_t> needStopDownloadIds;
    BackgroundCloudBatchSelectedFileProcessor::ClassifyCurrentRoundFileIdInList(fileIds, needStopDownloadIds);
    EXPECT_EQ(needStopDownloadIds.size(), 1);
    MEDIA_INFO_LOG("Bcbsfpt_ClassifyCurrentRountFileId_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AddDownloadTask_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AddDownloadTask_Test_001 Start");
    std::string todoUri = "test_uri1";
    int32_t ret;
    ret = BackgroundCloudBatchSelectedFileProcessor::AddSelectedBatchDownloadTask(todoUri);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Bcbsfpt_AddDownloadTask_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_ClearRoundMapInfo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_ClearRoundMapInfo_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::ClearRoundMapInfos();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::downloadResult_.empty(), true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_.empty(), true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.empty(), true);
    MEDIA_INFO_LOG("Bcbsfpt_ClearRoundMapInfo_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_DownloadCloudFilesExecutor_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_DownloadCloudFilesExecutor_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::SingleDownloadFiles downloadFiles;
    downloadFiles.uri =  "test_uri1";
    downloadFiles.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    BackgroundCloudBatchSelectedFileProcessor::BatchDownloadCloudFilesData *data =
        new BackgroundCloudBatchSelectedFileProcessor::BatchDownloadCloudFilesData(downloadFiles);
    BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchFilesExecutor(data);
    EXPECT_EQ(downloadFiles.uri, "test_uri1");
    delete data;
    MEDIA_INFO_LOG("Bcbsfpt_DownloadCloudFilesExecutor_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_DownloadSelectedBatchResources_Test_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_DownloadSelectedBatchResources_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchResources(); // 无任务执行情况
    BackgroundCloudBatchSelectedFileProcessor::SingleDownloadFiles downloadFiles;
    downloadFiles.uri = "test_uri1";
    downloadFiles.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    BackgroundCloudBatchSelectedFileProcessor::BatchDownloadCloudFilesData *data =
        new BackgroundCloudBatchSelectedFileProcessor::BatchDownloadCloudFilesData(downloadFiles);
    BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchFilesExecutor(data);
    EXPECT_EQ(downloadFiles.uri, "test_uri1");
    delete data;
    MEDIA_INFO_LOG("Bcbsfpt_DownloadSelectedBatchResources_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_DownloadSelectedBatchResources_Test_002,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_DownloadSelectedBatchResources_Test_002 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    BackgroundCloudBatchSelectedFileProcessor::DownloadSelectedBatchResources(); // 有任务执行情况
    MEDIA_INFO_LOG("Bcbsfpt_DownloadSelectedBatchResources_Test_002 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HaveBatchDownloadResourcesTask_Test_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HaveBatchDownloadResourcesTask_Test_001 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    EXPECT_NE(BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedResourceFilesNum(), 0);

    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask(), true);
    MEDIA_INFO_LOG("Bcbsfpt_HaveBatchDownloadResourcesTask_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HaveBatchDownloadResourcesTask_Test_002,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HaveBatchDownloadResourcesTask_Test_002 Start");
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::HaveBatchDownloadResourcesTask(), false);
    MEDIA_INFO_LOG("Bcbsfpt_HaveBatchDownloadResourcesTask_Test_002 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_QueryBatchDownloadFinishStatusCountFromDB_Test_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_QueryBatchDownloadFinishStatusCountFromDB_Test_001 Start");
    InsertBatchDownloadTask(1, "file://media/Photo/1/1.jpg", "1.jpg",
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_DOWNLOADING));
    InsertBatchDownloadTask(2, "file://media/Photo/2/2.jpg", "2.jpg",
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
    InsertBatchDownloadTask(3, "file://media/Photo/3/3.jpg", "3.jpg",
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    EXPECT_NE(BackgroundCloudBatchSelectedFileProcessor::QueryBatchSelectedResourceFilesNum(), 0);
    int32_t totalValue = 0;
    int32_t completedValue = 0;
    int32_t failedValue = 0;  // 查失败不更新通知
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::QueryBatchDownloadFinishStatusCountFromDB(
                  totalValue, completedValue, failedValue), E_OK);
    EXPECT_EQ(totalValue, 3);
    EXPECT_EQ(completedValue, 1);
    EXPECT_EQ(failedValue, 1);
    MEDIA_INFO_LOG("total:%{public}d, cur:%{public}d , cur:%{public}d", totalValue, completedValue, failedValue);
    MEDIA_INFO_LOG("Bcbsfpt_QueryBatchDownloadFinishStatusCountFromDB_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_UpdateDBProgressStatusInfoForBatch_Test_002,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_UpdateDBProgressStatusInfoForBatch_Test_002 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    vector<int32_t> localFileIds = {1, 2};
    vector<int32_t> exceptionFileIds = {4, 5};
    BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressStatusInfoForBatch(localFileIds,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_SUCCESS));
    BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressStatusInfoForBatch(exceptionFileIds,
        static_cast<int32_t>(Media::BatchDownloadStatusType::TYPE_FAIL));
    MEDIA_INFO_LOG("Bcbsfpt_UpdateDBProgressStatusInfoForBatch_Test_002 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_UpdateDBProgressInfoForFileId_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_UpdateDBProgressInfoForFileId_001 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    std::string fileIdStr = "2";
    int32_t ret = BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(fileIdStr, 10, -1, -1);
    EXPECT_EQ(ret, 0);
    ret = BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(fileIdStr, -1, -1, -1);
    EXPECT_EQ(ret, 0);
    ret = BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(fileIdStr, 10, -1, 2);
    EXPECT_EQ(ret, 0);
    ret = BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(fileIdStr, 10, 1756371627207, -1);
    EXPECT_EQ(ret, 0);
    fileIdStr = "";
    ret= BackgroundCloudBatchSelectedFileProcessor::UpdateDBProgressInfoForFileId(fileIdStr, 20, 0, 1);
    EXPECT_NE(ret, 0);
    MEDIA_INFO_LOG("Bcbsfpt_UpdateDBProgressInfoForFileId_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_StartTimerStopTimer_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_StartTimerStopTimer_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning(), true);
    BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer();
    BackgroundCloudBatchSelectedFileProcessor::StopAllDownloadingTask();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::batchDownloadResourcesStartTimerId_, 0);
    MEDIA_INFO_LOG("Bcbsfpt_StartTimerStopTimer_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_StartTimerStopTimer_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_StartTimerStopTimer_Test_002 Start");
    BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
    BackgroundCloudBatchSelectedFileProcessor::DownloadLatestBatchSelectedFinished();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::batchDownloadResourcesStartTimerId_, 0);
    BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer();
    MEDIA_INFO_LOG("Bcbsfpt_StartTimerStopTimer_Test_002 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_LaunchBatchDownloadProcessor_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_LaunchBatchDownloadProcessor_Test_001 Start");
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), false);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor();  // 无任务
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), false);
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor();  // 有任务
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), true);
    BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(); // 有任务 无timer
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), false);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning(), false);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), true);
    BackgroundCloudBatchSelectedFileProcessor::TriggerStopBatchDownloadProcessor();
    BackgroundCloudBatchSelectedFileProcessor::StopAllDownloadingTask();
    MEDIA_INFO_LOG("Bcbsfpt_LaunchBatchDownloadProcessor_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_LaunchBatchDownloadProcessor_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_LaunchBatchDownloadProcessor_Test_002 Start");
    EXPECT_EQ(QueryPhotosCount(), 10);
    PrepareBatchDownloadTask(10);
    EXPECT_NE(QueryBatchDownloadTasksCount(), 0);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor();  // 有任务
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsStartTimerRunning(), true);
    BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(); //  有timer 无任务
    ClearAndResetTable();
    BackgroundCloudBatchSelectedFileProcessor::TriggerStopBatchDownloadProcessor();
    BackgroundCloudBatchSelectedFileProcessor::StopAllDownloadingTask();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::IsBatchDownloadProcessRunningStatus(), false);
    MEDIA_INFO_LOG("Bcbsfpt_LaunchBatchDownloadProcessor_Test_002 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_GetDownloadFileIdCnt_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_GetDownloadFileIdCnt_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_.clear();
    std::string fileIdStr = "11";
    int32_t cnt = BackgroundCloudBatchSelectedFileProcessor::GetDownloadFileIdCnt(fileIdStr); // init
    EXPECT_EQ(cnt, 0);
    BackgroundCloudBatchSelectedFileProcessor::CheckAndUpdateDownloadFileIdCnt(fileIdStr, cnt);
    cnt = BackgroundCloudBatchSelectedFileProcessor::GetDownloadFileIdCnt(fileIdStr); // get
    EXPECT_EQ(cnt, 1);
    std::string fileIdStr2 = "12";
    cnt = 0;
    BackgroundCloudBatchSelectedFileProcessor::CheckAndUpdateDownloadFileIdCnt(fileIdStr2, cnt);
    cnt = BackgroundCloudBatchSelectedFileProcessor::GetDownloadFileIdCnt(fileIdStr); // get
    EXPECT_EQ(cnt, 1);
    MEDIA_INFO_LOG("Bcbsfpt_GetDownloadFileIdCnt_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_OnDownloadProcessTest_VacantObj, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_OnDownloadProcessTest_VacantObj Start");
    auto callBack = make_shared<BackgroundBatchSelectedFileDownloadCallback>();
    ASSERT_TRUE(callBack);

    DownloadProgressObj progress;
    progress.state = DownloadProgressObj::Status::RUNNING;
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::NO_ERROR;
    callBack->OnDownloadProcess(progress);
    progress.state = DownloadProgressObj::Status::COMPLETED;
    callBack->OnDownloadProcess(progress);

    progress.state = DownloadProgressObj::Status::FAILED;
    callBack->OnDownloadProcess(progress);
    progress.state = DownloadProgressObj::Status::STOPPED;
    callBack->OnDownloadProcess(progress);
    MEDIA_INFO_LOG("Bcbsfpt_OnDownloadProcessTest_VacantObj End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_OnDownloadProcessTest_NormalObj, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_OnDownloadProcessTest_NormalObj Start");
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "30";
    currentDownloadFileInfo.percent = 100;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"file://media/Photo/1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT},
        {"file://media/Photo/2", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS},
        {"file://media/Photo/3", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::FAILED},
        {"file://media/Photo/4", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SKIP_UPDATE_DB}
    };

    auto callBack = make_shared<BackgroundBatchSelectedFileDownloadCallback>();
    ASSERT_TRUE(callBack);
    DownloadProgressObj progress;
    progress.downloadId = 30;
    progress.path = "file://media/Photo/1";
    progress.state =  DownloadProgressObj::Status::COMPLETED;
    callBack->OnDownloadProcess(progress);

    progress.state = DownloadProgressObj::Status::FAILED;
    callBack->OnDownloadProcess(progress);
    progress.state = DownloadProgressObj::Status::STOPPED;
    callBack->OnDownloadProcess(progress);
    MEDIA_INFO_LOG("Bcbsfpt_OnDownloadProcessTest_NormalObj End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AddTasksAndStarted_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AddTasksAndStarted_Test_001 Start");
    std::vector<std::string> uris = PrepareBatchDownloadTask(10);
    BackgroundCloudBatchSelectedFileProcessor::downloadLatestFinished_.store(true);
    BackgroundCloudBatchSelectedFileProcessor::AddTasksAndStarted(uris);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::downloadLatestFinished_.load(), false);
    MEDIA_INFO_LOG("Bcbsfpt_AddTasksAndStarted_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HandleCallbackRunning_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackRunning_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "1";
    currentDownloadFileInfo.percent = 0;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT}
    };
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.downloadedSize = 2 * 1024;
    progress.totalSize = 10 * 1024;
    progress.path = "file://media/Photo/1";
    progress.state = DownloadProgressObj::Status::RUNNING;
    BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedRunningCallback(progress);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1].percent > 0, true);
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackRunning_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HandleCallbackSuccess_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackSuccess_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "1";
    currentDownloadFileInfo.percent = 0;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT}
    };
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.downloadedSize = 10 * 1000 * 1000;
    progress.totalSize = 10 * 1000 * 1000;
    progress.path = "file://media/Photo/1";
    progress.state = DownloadProgressObj::Status::COMPLETED;
    BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedSuccessCallback(progress);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.find(1),
        BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.end());
    int32_t successCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_SUCCESS);
    EXPECT_EQ(successCount, 1);
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackSuccess_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HandleCallbackFailed_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackFailed_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "1";
    currentDownloadFileInfo.percent = 0;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT}
    };
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.downloadedSize = 1 * 1000 * 1000;
    progress.path = "file://media/Photo/1";
    progress.state = DownloadProgressObj::Status::FAILED;
    BackgroundCloudBatchSelectedFileProcessor::downloadFileIdAndCount_["1"] = 6;
    BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedFailedCallback(progress);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.find(1),
        BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.end());
    int32_t failedCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_FAIL);
    EXPECT_EQ(failedCount, 0);
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackFailed_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_HandleCallbackStopped_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackStopped_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "1";
    currentDownloadFileInfo.percent = 0;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    BackgroundCloudBatchSelectedFileProcessor::downloadResult_ = {
        {"1", BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::INIT}
    };
    DownloadProgressObj progress;
    progress.downloadId = 1;
    progress.downloadedSize = 1 * 1000 * 1000;
    progress.totalSize = 10 * 1000 * 1000;
    progress.path = "file://media/Photo/1";
    progress.state = DownloadProgressObj::Status::STOPPED;
    BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedStoppedCallback(progress);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.find(1),
        BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.end());
    int32_t pauseCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_PAUSE);
    EXPECT_EQ(pauseCount, 0);
    MEDIA_INFO_LOG("Bcbsfpt_HandleCallbackStopped_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_SetBatchDownloadAddedFlag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_SetBatchDownloadAddedFlag_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadAddedFlag(true);
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::batchDownloadTaskAdded_.load(), true);
    MEDIA_INFO_LOG("Bcbsfpt_SetBatchDownloadAddedFlag_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_ClassifyFileIdsInTable_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_ClassifyFileIdsInTable_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int32_t taskCount = QueryBatchDownloadTasksCount();
    EXPECT_EQ(taskCount, 10);
    std::vector<string> fileIds = {"1", "2", "3", "10", "11"};
    std::vector<string> existed;
    BackgroundCloudBatchSelectedFileProcessor::ClassifyFileIdsInDownloadResourcesTable(fileIds, existed);
    EXPECT_EQ(existed.size(), 4);
    MEDIA_INFO_LOG("Bcbsfpt_ClassifyFileIdsInTable_Test_001 End");
}


HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_DeleteCancelStateTask_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_DeleteCancelStateTask_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int32_t taskCount = QueryBatchDownloadTasksCount();
    EXPECT_EQ(taskCount, 10);
    std::vector<string> fileIds = {"1", "2"};
    BackgroundCloudBatchSelectedFileProcessor::DeleteCancelStateDownloadResources(fileIds);
    int32_t count = QueryBatchDownloadTasksCount();
    EXPECT_EQ(count, taskCount - fileIds.size());
    MEDIA_INFO_LOG("Bcbsfpt_DeleteCancelStateTask_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_ResetDownloadTimer_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_ResetDownloadTimer_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int32_t taskCount = QueryBatchDownloadTasksCount();
    EXPECT_EQ(taskCount, 10);
    BackgroundCloudBatchSelectedFileProcessor::LaunchBatchDownloadProcessor();
    EXPECT_EQ(BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_.empty(), true);
    MEDIA_INFO_LOG("Bcbsfpt_ResetDownloadTimer_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AutoStopAction_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AutoStopAction_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int32_t waitingCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_WAITING);
    EXPECT_EQ(waitingCount, 10);

    BatchDownloadAutoPauseReasonType autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_POWER_LOW;
    BackgroundCloudBatchSelectedFileProcessor::AutoStopAction(autoPauseReason);
    int32_t pauseCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE);
    EXPECT_EQ(pauseCount, 10);
    MEDIA_INFO_LOG("Bcbsfpt_AutoStopAction_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AutoResumeAction_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AutoResumeAction_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BatchDownloadAutoPauseReasonType autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_NETWORK_DISCONNECT;
    BackgroundCloudBatchSelectedFileProcessor::AutoStopAction(autoPauseReason);
    int32_t pauseCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_AUTO_PAUSE);
    EXPECT_EQ(pauseCount, 10);
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadProcessRunningStatus(true);
    BackgroundCloudBatchSelectedFileProcessor::StopBatchDownloadResourcesTimer(false);
    BackgroundCloudBatchSelectedFileProcessor::LaunchAutoResumeBatchDownloadProcessor();
    int32_t waitingCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_WAITING);
    EXPECT_EQ(waitingCount, 10);
    MEDIA_INFO_LOG("Bcbsfpt_AutoResumeAction_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AllAutoPauseToDownloading_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AllAutoPauseToDownloading_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BatchDownloadAutoPauseReasonType autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_NETWORK_DISCONNECT;
    BackgroundCloudBatchSelectedFileProcessor::AutoStopAction(autoPauseReason);
    BackgroundCloudBatchSelectedFileProcessor::UpdateAllStatusAutoPauseToDownloading();
    int32_t downloadingCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_DOWNLOADING);
    EXPECT_EQ(downloadingCount, 0);
    MEDIA_INFO_LOG("Bcbsfpt_AllAutoPauseToDownloading_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_AllAutoPauseToWaiting_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_AllAutoPauseToWaiting_Test_001 Start");
    PrepareBatchDownloadTask(10);
    BatchDownloadAutoPauseReasonType autoPauseReason = BatchDownloadAutoPauseReasonType::TYPE_NETWORK_DISCONNECT;
    BackgroundCloudBatchSelectedFileProcessor::AutoStopAction(autoPauseReason);
    BackgroundCloudBatchSelectedFileProcessor::UpdateAllStatusAutoPauseToWaiting();
    int32_t waitingCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_WAITING);
    EXPECT_EQ(waitingCount, 10);
    MEDIA_INFO_LOG("Bcbsfpt_AllAutoPauseToWaiting_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_SetProcessRunningStatus_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_SetProcessRunningStatus_Test_001 Start");
    BackgroundCloudBatchSelectedFileProcessor::SetBatchDownloadProcessRunningStatus(true);
    bool status = BackgroundCloudBatchSelectedFileProcessor::batchDownloadProcessRunningStatus_.load();
    EXPECT_EQ(status, true);
    MEDIA_INFO_LOG("Bcbsfpt_SetProcessRunningStatus_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_TriggerCancelTask_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_TriggerCancelTask_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int taskCount = QueryBatchDownloadTasksCount();
    EXPECT_EQ(taskCount, 10);

    BackgroundCloudBatchSelectedFileProcessor::InDownloadingFileInfo currentDownloadFileInfo;
    currentDownloadFileInfo.fileId = "1";
    currentDownloadFileInfo.percent = 10;
    currentDownloadFileInfo.status = BackgroundCloudBatchSelectedFileProcessor::BatchDownloadStatus::SUCCESS;
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_ = {};
    BackgroundCloudBatchSelectedFileProcessor::currentDownloadIdFileInfoMap_[1] = currentDownloadFileInfo;
    std::vector<std::string> fileIdsToCancel = {"1", "2", "3"};
    bool sendNotify = true;
    BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
    BackgroundCloudBatchSelectedFileProcessor::TriggerCancelBatchDownloadProcessor(fileIdsToCancel, sendNotify);
    EXPECT_EQ(QueryBatchDownloadTasksCount(), taskCount - fileIdsToCancel.size());
    MEDIA_INFO_LOG("Bcbsfpt_TriggerCancelTask_Test_001 End");
}

HWTEST_F(BackgroundCloudBatchSelectedFileProcessorTest, Bcbsfpt_TriggerPauseTask_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcbsfpt_TriggerPauseTask_Test_001 Start");
    PrepareBatchDownloadTask(10);
    int taskCount = QueryBatchDownloadTasksCount();
    EXPECT_EQ(taskCount, 10);
    std::vector<std::string> fileIdsToPause = {"4", "5", "6"};
    BackgroundCloudBatchSelectedFileProcessor::StartBatchDownloadResourcesTimer();
    BackgroundCloudBatchSelectedFileProcessor::TriggerPauseBatchDownloadProcessor(fileIdsToPause); //no update db
    int32_t pauseCount = QueryTasksCountByStatus(Media::BatchDownloadStatusType::TYPE_PAUSE);
    EXPECT_EQ(pauseCount, 0);
    MEDIA_INFO_LOG("Bcbsfpt_TriggerPauseTask_Test_001 End");
}

} // namespace Media
} // namespace OHOS