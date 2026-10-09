/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "BackgroundCloudFileProcessorTest"

#include "background_cloud_file_processor_test.h"

#include <chrono>
#include <thread>

#define private public
#include "background_cloud_file_processor.h"
#undef private

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

static shared_ptr<MediaLibraryRdbStore> rdbStore;
static std::atomic<int> num{ 0 };

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

void ClearTables()
{
    string clearPhoto = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    vector<string> executeSqls = { clearPhoto };
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(executeSqls);
    num = 0;
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

int32_t PrepareAbnormalPhotos(const string &column)
{
    EXPECT_NE((rdbStore == nullptr), true);
    string updateSql;
    if (column == MediaColumn::MEDIA_MIME_TYPE) {
        updateSql = "update " + PhotoColumn::PHOTOS_TABLE + " set " + column + " = '' ";
    } else {
        updateSql = "update " + PhotoColumn::PHOTOS_TABLE + " set " + column + " = 0 ";
    }
    vector<string> executeSqls = {updateSql};
    int32_t ret = ExecSqls(executeSqls);
    EXPECT_EQ(ret, E_OK);
    return ret;
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

std::vector<std::string> QueryCurDownloadFiles()
{
    std::vector<std::string> curDownloadFiles;
    double freeRatio = 0.5;
    auto resultSet = BackgroundCloudFileProcessor::QueryCloudFiles(freeRatio);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query cloud files!");
        return curDownloadFiles;
    }

    BackgroundCloudFileProcessor::DownloadFiles downloadFiles;
    BackgroundCloudFileProcessor::ParseDownloadFiles(resultSet, downloadFiles);
    if (downloadFiles.uris.empty()) {
        MEDIA_INFO_LOG("No cloud files need to be downloaded");
        return curDownloadFiles;
    }
    return downloadFiles.uris;
}

void BackgroundCloudFileProcessorTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("BackgroundCloudFileProcessorTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
}

void BackgroundCloudFileProcessorTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("BackgroundCloudFileProcessorTest TearDownTestCase");
    ClearTables();
}

void BackgroundCloudFileProcessorTest::SetUp()
{
    MEDIA_INFO_LOG("BackgroundCloudFileProcessorTest SetUp");
    ClearTables();
}

void BackgroundCloudFileProcessorTest::TearDown()
{
    MEDIA_INFO_LOG("BackgroundCloudFileProcessorTest TearDown");
}

// Scenario6: Test how many images can be downloaded in one minute
HWTEST_F(BackgroundCloudFileProcessorTest, background_cloud_file_processor_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("background_cloud_file_processor_test_006 Start");
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
    EXPECT_EQ(QueryPhotosCount(), 10);

    std::vector<std::string> curDownloadFiles = QueryCurDownloadFiles();
    EXPECT_NE(curDownloadFiles.size(), 0);
    MEDIA_INFO_LOG("background_cloud_file_processor_test_006 End");
}

// Scenario8: Test how many image can be updated in one minute
HWTEST_F(BackgroundCloudFileProcessorTest, background_cloud_file_processor_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("background_cloud_file_processor_test_008 Start");
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
    PrepareAbnormalPhotos(MediaColumn::MEDIA_SIZE);
    auto resultSet = BackgroundCloudFileProcessor::QueryUpdateData(true, false);
    int32_t rowCount;
    int32_t ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 1);
    MEDIA_INFO_LOG("background_cloud_file_processor_test_008 End");
}

// Scenario9: Test how many video can be updated in one minute
HWTEST_F(BackgroundCloudFileProcessorTest, background_cloud_file_processor_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("background_cloud_file_processor_test_009 Start");
    PreparePhotos(10, MEDIA_TYPE_VIDEO, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
    PrepareAbnormalPhotos(MediaColumn::MEDIA_SIZE);
    auto resultSet = BackgroundCloudFileProcessor::QueryUpdateData(true, true);
    int32_t rowCount;
    int32_t ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 1);
    MEDIA_INFO_LOG("background_cloud_file_processor_test_009 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, background_cloud_file_processor_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("background_cloud_file_processor_test_010 Start");
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::LOCAL)); // 10 count
    PrepareAbnormalPhotos(MediaColumn::MEDIA_SIZE);
    auto resultSet = BackgroundCloudFileProcessor::QueryUpdateData(false, false);
    int32_t rowCount;
    int32_t ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 1);
    MEDIA_INFO_LOG("background_cloud_file_processor_test_010 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, background_cloud_file_processor_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("background_cloud_file_processor_test_011 Start");
    PreparePhotos(10, MEDIA_TYPE_VIDEO, static_cast<int32_t>(PhotoPositionType::LOCAL)); // 10 count
    PrepareAbnormalPhotos(MediaColumn::MEDIA_SIZE);
    auto resultSet = BackgroundCloudFileProcessor::QueryUpdateData(false, true);
    int32_t rowCount;
    int32_t ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 1);
    MEDIA_INFO_LOG("background_cloud_file_processor_test_011 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_SetDownloadLatestFinished_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BCFPT_SetDownloadLatestFinished_Test_001 Start");
    bool downloadLatestFinished = true;
    BackgroundCloudFileProcessor::SetDownloadLatestFinished(downloadLatestFinished);
    EXPECT_EQ(downloadLatestFinished, true);
    downloadLatestFinished = false;
    BackgroundCloudFileProcessor::SetDownloadLatestFinished(downloadLatestFinished);
    EXPECT_EQ(downloadLatestFinished, false);
    MEDIA_INFO_LOG("BCFPT_SetDownloadLatestFinished_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_GetDownloadLatestFinished_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BCFPT_GetDownloadLatestFinished_Test_001 Start");
    bool downloadLatestFinished;
    downloadLatestFinished = BackgroundCloudFileProcessor::GetDownloadLatestFinished();
    EXPECT_EQ(downloadLatestFinished, false);
    MEDIA_INFO_LOG("BCFPT_GetDownloadLatestFinished_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_SetGetLastDownloadMilliSecond_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BCFPT_SetLastDownloadMilliSecond_Test_001 Start");
    int64_t lastDownloadMilliSecond = 100;
    BackgroundCloudFileProcessor::SetLastDownloadMilliSecond(lastDownloadMilliSecond);
    lastDownloadMilliSecond = BackgroundCloudFileProcessor::GetLastDownloadMilliSecond();
    EXPECT_NE(lastDownloadMilliSecond, 0);
    MEDIA_INFO_LOG("BCFPT_SetLastDownloadMilliSecond_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_DownloadCnt_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_DownloadCnt_Test_001 Start");
    std::string uri = "test";
    int64_t cnt = 100;
    BackgroundCloudFileProcessor::UpdateDownloadCnt(uri, cnt);
    cnt = BackgroundCloudFileProcessor::GetDownloadCnt(uri);
    EXPECT_EQ(cnt, 100);
    BackgroundCloudFileProcessor::ClearDownloadCnt();
    cnt = BackgroundCloudFileProcessor::GetDownloadCnt(uri);
    EXPECT_EQ(cnt, 0);
    MEDIA_INFO_LOG("Bcfpt_DownloadCnt_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_GetStorageFreeRatio_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_GetStorageFreeRatio_Test_001 Start");
    double freeRatio = 1024;
    bool ret;
    ret = BackgroundCloudFileProcessor::GetStorageFreeRatio(freeRatio);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("Bcfpt_GetStorageFreeRatio_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_QueryCloudFiles_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_GetStorageFreeRatio_Test_001 Start");
    double freeRatio = 0.5;
    auto resultSet = BackgroundCloudFileProcessor::QueryCloudFiles(freeRatio);
    EXPECT_NE(resultSet, nullptr);
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
    EXPECT_EQ(QueryPhotosCount(), 10);
    resultSet = BackgroundCloudFileProcessor::QueryCloudFiles(freeRatio);
    EXPECT_NE(resultSet, nullptr);
    MEDIA_INFO_LOG("Bcfpt_GetStorageFreeRatio_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_CheckAndUpdateDownloadCnt_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_CheckAndUpdateDownloadCnt_Test_001 Start");
    std::string uri = "testuri";
    int64_t cnt = 100;
    double freeRatio = 0.5;
    BackgroundCloudFileProcessor::CheckAndUpdateDownloadCnt(uri, cnt);
    auto resultSet = BackgroundCloudFileProcessor::QueryCloudFiles(freeRatio);
    EXPECT_NE(resultSet, nullptr);
    MEDIA_INFO_LOG("Bcfpt_CheckAndUpdateDownloadCnt_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_GetDownloadNum_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_CheckAndUpdateDownloadCnt_Test_001 Start");
    int64_t downloadNum = 0;
    BackgroundCloudFileProcessor::GetDownloadNum(downloadNum);
    EXPECT_NE(downloadNum, 0);
    downloadNum = 1;
    BackgroundCloudFileProcessor::GetDownloadNum(downloadNum);
    EXPECT_NE(downloadNum, 1);
    downloadNum = 30;
    BackgroundCloudFileProcessor::GetDownloadNum(downloadNum);
    EXPECT_EQ(downloadNum, 30);
    MEDIA_INFO_LOG("Bcfpt_CheckAndUpdateDownloadCnt_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_AddDownloadTask_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_AddDownloadTask_Test_001 Start");
    BackgroundCloudFileProcessor::DownloadFiles downloadFiles;
    downloadFiles.uris = { "test_uri1", "test_uri2", "test_uri3" };
    downloadFiles.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t ret;
    ret = BackgroundCloudFileProcessor::AddDownloadTask(downloadFiles);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Bcfpt_AddDownloadTask_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_DownloadCloudFilesExecutor_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_DownloadCloudFilesExecutor_Test_001 Start");
    BackgroundCloudFileProcessor::DownloadFiles downloadFiles;
    downloadFiles.uris = { "test_uri1", "test_uri2", "test_uri3" };
    downloadFiles.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    BackgroundCloudFileProcessor::DownloadCloudFilesData *data =
        new BackgroundCloudFileProcessor::DownloadCloudFilesData(downloadFiles);
    BackgroundCloudFileProcessor::DownloadCloudFilesExecutor(data);
    EXPECT_NE(downloadFiles.uris.size(), 0);
    MEDIA_INFO_LOG("Bcfpt_DownloadCloudFilesExecutor_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_QueryUpdateData_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_QueryUpdateData_Test_001 Start");
    BackgroundCloudFileProcessor::downloadId_ = 30;
    BackgroundCloudFileProcessor::StopDownloadFiles();
    bool isCloud = false;
    bool isVideo = false;
    EXPECT_NE(BackgroundCloudFileProcessor::QueryUpdateData(isCloud, isVideo), nullptr);
    MEDIA_INFO_LOG("Bcfpt_QueryUpdateData_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_UpdateCurrentOffset_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_001 Start");
    bool isCloud = true;
    bool isVideo = false;
    BackgroundCloudFileProcessor::cloudRetryCount_ = 2;
    BackgroundCloudFileProcessor::UpdateCurrentOffset(isCloud, isVideo);
    EXPECT_EQ(BackgroundCloudFileProcessor::cloudRetryCount_, 0);
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_UpdateCurrentOffset_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_002 Start");
    bool isCloud = true;
    bool isVideo = true;
    BackgroundCloudFileProcessor::cloudRetryCount_ = 1;
    BackgroundCloudFileProcessor::UpdateCurrentOffset(isCloud, isVideo);
    EXPECT_NE(BackgroundCloudFileProcessor::cloudRetryCount_, 0);
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_002 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_StopUpdateData_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_002 Start");
    BackgroundCloudFileProcessor::isUpdating_ = true;
    BackgroundCloudFileProcessor::StopUpdateData();
    EXPECT_EQ(BackgroundCloudFileProcessor::isUpdating_, false);
    MEDIA_INFO_LOG("Bcfpt_UpdateCurrentOffset_Test_002 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_StartTimerStopTimer_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_StartTimerStopTimer_Test_001 Start");
    BackgroundCloudFileProcessor::isUpdating_ = false;
    BackgroundCloudFileProcessor::StartTimer();
    EXPECT_EQ(BackgroundCloudFileProcessor::isUpdating_, true);
    BackgroundCloudFileProcessor::StopTimer();
    EXPECT_EQ(BackgroundCloudFileProcessor::stopTimerId_, 0);
    MEDIA_INFO_LOG("Bcfpt_StartTimerStopTimer_Test_001 End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_OnDownloadProcessTest_VacantObj, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_OnDownloadProcessTest_VacantObj Start");
    auto callBack = make_shared<BackgroundCloudFileDownloadCallback>();
    ASSERT_TRUE(callBack);

    DownloadProgressObj progress;
    progress.state = DownloadProgressObj::Status::COMPLETED;
    progress.downloadErrorType = DownloadProgressObj::DownloadErrorType::NO_ERROR;
    callBack->OnDownloadProcess(progress);

    progress.state = DownloadProgressObj::Status::FAILED;
    callBack->OnDownloadProcess(progress);
    progress.state = DownloadProgressObj::Status::STOPPED;
    callBack->OnDownloadProcess(progress);
    MEDIA_INFO_LOG("Bcfpt_OnDownloadProcessTest_VacantObj End");
}

HWTEST_F(BackgroundCloudFileProcessorTest, Bcfpt_OnDownloadProcessTest_NormalObj, TestSize.Level1)
{
    MEDIA_INFO_LOG("Bcfpt_OnDownloadProcessTest_NormalObj Start");
    BackgroundCloudFileProcessor::downloadId_ = 30;
    BackgroundCloudFileProcessor::downloadResult_ = {
        {"file1", BackgroundCloudFileProcessor::INIT},
        {"file2", BackgroundCloudFileProcessor::SUCCESS},
        {"file3", BackgroundCloudFileProcessor::NETWORK_UNAVAILABLE}
    };

    auto callBack = make_shared<BackgroundCloudFileDownloadCallback>();
    ASSERT_TRUE(callBack);
    DownloadProgressObj progress;
    progress.downloadId = 30;
    progress.path = "file1";
    progress.state =  DownloadProgressObj::Status::COMPLETED;
    callBack->OnDownloadProcess(progress);

    progress.state = DownloadProgressObj::Status::FAILED;
    callBack->OnDownloadProcess(progress);
    progress.state = DownloadProgressObj::Status::STOPPED;
    callBack->OnDownloadProcess(progress);
    MEDIA_INFO_LOG("Bcfpt_OnDownloadProcessTest_NormalObj End");
}
} // namespace Media
} // namespace OHOS