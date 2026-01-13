/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "MediaLibraryVideoModeTest"
#include "medialibrary_video_mode_test.h"
 
#include "media_video_mode_task.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "metadata_extractor.h"
#include "result_set_utils.h"
#include "media_upgrade.h"
 
using namespace testing::ext;
 
namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> number(0);
static constexpr int32_t SLEEP_SECONDS = 1;
 
static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE " + dropTable + ";";
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
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
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
 
void MediaLibraryVideoModeTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryVideoModeTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryVideoModeTest SetUpTestCase");
}
 
void MediaLibraryVideoModeTest::TearDownTestCase()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaLibraryVideoModeTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}
 
void MediaLibraryVideoModeTest::SetUp()
{
    CleanTestTables();
    SetTables();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("MediaLibraryVideoModeTest SetUp");
}
 
void MediaLibraryVideoModeTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryVideoModeTest TearDown");
}
 
static int GetNumber()
{
    return ++number;
}
 
static std::string GetTitle()
{
    return "IMG_tdd_" + std::to_string(GetNumber());
}
 
static std::string GetDisplayName(const std::string &title)
{
    return title + ".mp4";
}
 
static int32_t InsertPhotoForNotScannered()
{
    MEDIA_INFO_LOG("InsertPhotoForNotScannered start");
    int64_t fileId = -1;
    int32_t mediaType = 2;
    int32_t videoMode = -1;
    std::string title = GetTitle();
    std::string displayName = GetDisplayName(title);
    MEDIA_INFO_LOG("title is: %{public}s, displayName is: %{public}s",
        title.c_str(), displayName.c_str());
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(PhotoColumn::PHOTO_VIDEO_MODE, videoMode);
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", std::to_string(fileId).c_str());
    return fileId;
}
 
static int32_t InsertLogVideoNotScannered()
{
    MEDIA_INFO_LOG("InsertLogVideoNotScannered start");
    int64_t fileId = -1;
    int32_t mediaType = 2;
    int32_t videoMode = -1;
    std::string displayName = "hw_log.mp4";
    MEDIA_INFO_LOG("displayName is: %{public}s", displayName.c_str());
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_VIDEO_MODE, videoMode);
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", std::to_string(fileId).c_str());
    return fileId;
}
 
HWTEST_F(MediaLibraryVideoModeTest, MediaLibraryVideoMode_BatchStatus_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryVideoMode_BatchStatus_Test Start");
    auto mediaVideoModeTestTask = std::make_shared<MediaVideoModeTask>();
    int32_t startFileId = 0;
    mediaVideoModeTestTask->SetBatchStatus(startFileId);
    EXPECT_EQ(mediaVideoModeTestTask->GetBatchStatus(), startFileId);
    mediaVideoModeTestTask->HandleMediaFileManagerVideoMode();
    MEDIA_INFO_LOG("MediaLibraryVideoMode_BatchStatus_Test End");
}
 
HWTEST_F(MediaLibraryVideoModeTest, MediaLibraryVideoMode_QueryFiles_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryVideoMode_QueryFiles_Test Start");
    //photos中预制一些未扫描的数据
    VideoModeInfo videoModeInfo;
    int32_t insertPhotoNum = 10;
    for (int num = 0; num < insertPhotoNum; num++) {
        int32_t fileId = InsertPhotoForNotScannered();
        std::string filePath = "/storage/cloud/files/Photo/1/" + std::to_string(fileId) + ".mp4";
    }
    
    //对未扫描的数据进行查询
    EXPECT_NE(g_rdbStore, nullptr);
    auto mediaVideoModeTask = std::make_shared<MediaVideoModeTask>();
    int32_t startFileId = 1;
    VideoModeInfo queryVideoModeInfo = mediaVideoModeTask->QueryFiles(g_rdbStore, startFileId);
    EXPECT_EQ(queryVideoModeInfo.fileIds.size(), insertPhotoNum);
    MEDIA_INFO_LOG("MediaLibraryVideoMode_QueryFiles_Test End");
}
 
HWTEST_F(MediaLibraryVideoModeTest, MediaLibraryVideoMode_UpdateVideoMode_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryVideoMode_UpdateVideoMode_Test Start");
    const int32_t logVideoMode = 1;
    int32_t fileId = InsertLogVideoNotScannered();
    //扫描未扫描文件
    auto mediaVideoModeTask = std::make_shared<MediaVideoModeTask>();
    int32_t startFileId = 1;
    VideoModeInfo queryVideoModeInfo = mediaVideoModeTask->QueryFiles(g_rdbStore, startFileId);
    //更新数据库
    EXPECT_NE(g_rdbStore, nullptr);
    mediaVideoModeTask->UpdateVideoMode(queryVideoModeInfo);
    std::string sqlPhotos = "SELECT video_mode FROM Photos WHERE file_id = " + std::to_string(fileId);
    std::shared_ptr<NativeRdb::ResultSet> photoResultSet = g_rdbStore->QuerySql(sqlPhotos);
    while (photoResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t videoMode = GetInt32Val(PhotoColumn::PHOTO_VIDEO_MODE, photoResultSet);
        EXPECT_EQ(videoMode, logVideoMode);
    }
    photoResultSet->Close();
    MEDIA_INFO_LOG("MediaLibraryVideoMode_UpdateVideoMode_Test End");
}
}