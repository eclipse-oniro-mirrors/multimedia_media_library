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

#define MLOG_TAG "FileManagerTempFileAgingTest"

#include "medialibrary_file_manager_temp_file_aging_test.h"
#include "medialibrary_unittest_utils.h"
#include "rdb_predicates.h"
#include  "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#define private public
#include "media_file_manager_temp_file_aging_task.h"
#include "medialibrary_subscriber.h"
#undef private
#include "media_upgrade.h"

using namespace testing::ext;

namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> number(0);
static const int32_t SLEEP_SECONDS = 1;

static int GetNumber()
{
    return ++number;
}

static std::string GetTitle(int64_t &timestamp)
{
    return "IMG_" + std::to_string(timestamp) + "_" + std::to_string(GetNumber());
}

static std::string GetDisplayName(const std::string &title)
{
    return title + ".jpg";
}

static int32_t InsertPhotoForAging()
{
    MEDIA_INFO_LOG("InsertPhotoForAging");
    int64_t fileId = -1;
    int64_t current = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timeStampBefore25Hours = current - 25 * 60 * 60 * 1000;
    std::string title = GetTitle(timeStampBefore25Hours);
    std::string displayName = GetDisplayName(title);
    MEDIA_INFO_LOG("title is: %{public}s, displayName is: %{public}s",
        title.c_str(), displayName.c_str());
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timeStampBefore25Hours);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timeStampBefore25Hours);
    valuesBucket.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER));
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", std::to_string(fileId).c_str());
    return fileId;
}

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE
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

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE
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

void FileManagerTempFileAgingTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start FileManagerTempFileAgingTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("FileManagerTempFileAgingTest SetUpTestCase");
}

void FileManagerTempFileAgingTest::TearDownTestCase()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("FileManagerTempFileAgingTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void FileManagerTempFileAgingTest::SetUp()
{
    CleanTestTables();
    SetTables();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("FileManagerTempFileAgingTest SetUp");
}

void FileManagerTempFileAgingTest::TearDown()
{
    MEDIA_INFO_LOG("FileManagerTempFileAgingTest TearDown");
}

HWTEST_F(FileManagerTempFileAgingTest, FileManagerTempFileAging_BatchStatus_Test, TestSize.Level1)
{
    auto mediaFileManagerTempFileAgingTask = std::make_shared<MediaFileManagerTempFileAgingTask>();
    int32_t startFileId = 0;
    mediaFileManagerTempFileAgingTask->SetBatchStatus(startFileId);
    EXPECT_EQ(mediaFileManagerTempFileAgingTask->GetBatchStatus(), startFileId);
}

HWTEST_F(FileManagerTempFileAgingTest, FileManagerTempFileAging_QueryAgingFiles_Test, TestSize.Level1)
{
    int32_t insertPhotoNum = 10;
    int32_t startFileId = 0;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhotoForAging();
    }
    auto mediaFileManagerTempFileAgingTask = std::make_shared<MediaFileManagerTempFileAgingTask>();
    AgingFilesInfo agingFilesInfo = mediaFileManagerTempFileAgingTask->QueryAgingFiles(g_rdbStore, startFileId, false);
    EXPECT_EQ(agingFilesInfo.fileIds.size(), insertPhotoNum);
}

HWTEST_F(FileManagerTempFileAgingTest, FileManagerTempFileAging_QueryAgingFiles_CheckOmitted_Test, TestSize.Level1)
{
    int32_t insertPhotoNum = 10;
    int32_t startFileId = insertPhotoNum + 1;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhotoForAging();
    }
    auto mediaFileManagerTempFileAgingTask = std::make_shared<MediaFileManagerTempFileAgingTask>();
    AgingFilesInfo agingFilesInfo = mediaFileManagerTempFileAgingTask->QueryAgingFiles(g_rdbStore, startFileId, true);
    EXPECT_EQ(agingFilesInfo.fileIds.size(), insertPhotoNum);
}

HWTEST_F(FileManagerTempFileAgingTest, FileManagerTempFileAging_DeleteTempFiles_And_Flow_Test, TestSize.Level1)
{
    AgingFilesInfo agingFilesInfo;
    int32_t insertPhotoNum = 10;
    for (int num = 0; num < insertPhotoNum; num++) {
        int32_t fileId = InsertPhotoForAging();
        agingFilesInfo.fileIds.push_back(std::to_string(fileId));
        std::string filePath = "/storage/cloud/files/Photo/1/" + std::to_string(fileId) + ".jpg";
        agingFilesInfo.filePaths.push_back(filePath);
        agingFilesInfo.dateTakens.push_back("0");
    }
    EXPECT_NE(g_rdbStore, nullptr);
    int64_t current = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timeBefore24Hours = current - 24 * 60 * 60 * 1000;
    auto mediaFileManagerTempFileAgingTask = std::make_shared<MediaFileManagerTempFileAgingTask>();
    mediaFileManagerTempFileAgingTask->DeleteTempFiles(g_rdbStore, agingFilesInfo);
    const std::string QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO =
        "SELECT file_id, data, date_taken FROM Photos WHERE " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE +
        " = " + std::to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) + " AND " +
        PhotoColumn::MEDIA_DATE_ADDED + " <= " + std::to_string(timeBefore24Hours);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->QuerySql(
        QUERY_FILE_MANAGER_TEMP_FILE_24H_BEFORE_INFO);
    EXPECT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhotoForAging();
    }
    mediaFileManagerTempFileAgingTask->HandleMediaFileManagerTempFileAging();
    MedialibrarySubscriber::currentStatus_ = true;
    mediaFileManagerTempFileAgingTask->HandleMediaFileManagerTempFileAging();
}
}  // namespace OHOS::Media::Background