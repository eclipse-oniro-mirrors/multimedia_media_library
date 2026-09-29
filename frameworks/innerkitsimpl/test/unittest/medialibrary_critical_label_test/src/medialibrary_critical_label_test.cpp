/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCriticalLabelTest"

#include "medialibrary_critical_label_test.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#define private public
#include "media_critical_label_task.h"
#include "medialibrary_subscriber.h"
#undef private
#include "media_upgrade.h"

using namespace testing::ext;
using OHOS::Media::MediaType;

namespace OHOS::Media::Background {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> g_number(0);
static const int32_t SLEEP_SECONDS = 1;

static int GetNumber()
{
    return ++g_number;
}

static std::string GetTitle(int64_t &timestamp)
{
    return "IMG_" + std::to_string(timestamp) + "_" + std::to_string(GetNumber());
}

static std::string GetDisplayName(const std::string &title)
{
    return title + ".jpg";
}

static int32_t InsertPhoto(int32_t mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE),
    int32_t criticalType = 0)
{
    MEDIA_INFO_LOG("InsertPhoto");
    int64_t fileId = -1;
    int64_t current = MediaFileUtils::UTCTimeMilliSeconds();
    std::string title = GetTitle(current);
    std::string displayName = GetDisplayName(title);
    MEDIA_INFO_LOG("title is: %{public}s, displayName is: %{public}s",
        title.c_str(), displayName.c_str());
    std::string data = "/storage/cloud/files/photo/1/" + displayName;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_RISK_STATUS, criticalType);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, current);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, current);
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", std::to_string(fileId).c_str());
    return fileId;
}

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE
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

void MediaCriticalLabelTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaCriticalLabelTest failed, can not get g_rdbStore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaCriticalLabelTest SetUpTestCase");
}

void MediaCriticalLabelTest::TearDownTestCase()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaCriticalLabelTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void MediaCriticalLabelTest::SetUp()
{
    CleanTestTables();
    SetTables();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("MediaCriticalLabelTest SetUp");
}

void MediaCriticalLabelTest::TearDown()
{
    MEDIA_INFO_LOG("MediaCriticalLabelTest TearDown");
}

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_Accept_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_Accept_Test Start");
    auto task = std::make_shared<MediaCriticalLabelTask>();
    MedialibrarySubscriber::checkCriticalTypeStatus_ = true;
    EXPECT_EQ(task->Accept(), true);
    MedialibrarySubscriber::checkCriticalTypeStatus_ = false;
    EXPECT_EQ(task->Accept(), false);
    MEDIA_INFO_LOG("MediaCriticalLabelTask_Accept_Test End");
}

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_QueryPhotosBatch_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_QueryPhotosBatch_Test Start");
    int32_t insertPhotoNum = 10;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhoto();
    }
    auto task = std::make_shared<MediaCriticalLabelTask>();
    EXPECT_NE(g_rdbStore, nullptr);
    PhotoBatchInfo batchInfo = task->QueryPhotosBatch(g_rdbStore, 10);
    EXPECT_EQ(batchInfo.size(), insertPhotoNum);
    MEDIA_INFO_LOG("MediaCriticalLabelTask_QueryPhotosBatch_Test End");
}
#endif

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_ConstructPhotoUri_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_ConstructPhotoUri_Test Start");
    auto task = std::make_shared<MediaCriticalLabelTask>();
    
    // Test image URI
    int32_t imageFileId = 123;
    std::string imageData = "/storage/cloud/files/photo/1/test_image.jpg";
    std::string imageDisplayName = "test_image.jpg";
    std::string imageUri = task->ConstructPhotoUri(imageData, imageDisplayName, imageFileId);
    EXPECT_NE(imageUri.find("Photo"), std::string::npos);
    EXPECT_NE(imageUri.find("123"), std::string::npos);
    
    // Test video URI
    int32_t videoFileId = 456;
    std::string videoData = "/storage/cloud/files/photo/1/test_video.mp4";
    std::string videoDisplayName = "test_video.mp4";
    std::string videoUri = task->ConstructPhotoUri(videoData, videoDisplayName, videoFileId);
    EXPECT_NE(videoUri.find("Photo"), std::string::npos);
    EXPECT_NE(videoUri.find("456"), std::string::npos);
    
    MEDIA_INFO_LOG("MediaCriticalLabelTask_ConstructPhotoUri_Test End");
}

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_HandleCriticalLabelProcessing_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_HandleCriticalLabelProcessing_Test Start");
    // Insert test photos
    int32_t insertPhotoNum = 15;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhoto();
    }
    // Insert some videos
    for (int num = 0; num < 5; num++) {
        InsertPhoto(static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    }
    
    auto task = std::make_shared<MediaCriticalLabelTask>();
    MedialibrarySubscriber::currentStatus_ = true;
    task->HandleCriticalLabelProcessing();
    MEDIA_INFO_LOG("MediaCriticalLabelTask_HandleCriticalLabelProcessing_Test End");
}

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_Execute_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_Execute_Test Start");
    // Insert test photos
    int32_t insertPhotoNum = 5;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhoto();
    }
    
    auto task = std::make_shared<MediaCriticalLabelTask>();
    MedialibrarySubscriber::currentStatus_ = true;
    task->Execute();
    MEDIA_INFO_LOG("MediaCriticalLabelTask_Execute_Test End");
}

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_EmptyDatabase_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_EmptyDatabase_Test Start");
    auto task = std::make_shared<MediaCriticalLabelTask>();
    MedialibrarySubscriber::currentStatus_ = true;
    task->HandleCriticalLabelProcessing();
    MEDIA_INFO_LOG("MediaCriticalLabelTask_EmptyDatabase_Test End");
}

HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_LargeBatch_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_LargeBatch_Test Start");
    // Insert more than 200 photos to test pagination
    int32_t insertPhotoNum = 250;
    for (int num = 0; num < insertPhotoNum; num++) {
        InsertPhoto();
    }
    
    auto task = std::make_shared<MediaCriticalLabelTask>();
    MedialibrarySubscriber::currentStatus_ = true;
    task->HandleCriticalLabelProcessing();
    MEDIA_INFO_LOG("MediaCriticalLabelTask_LargeBatch_Test End");
}

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
HWTEST_F(MediaCriticalLabelTest, MediaCriticalLabelTask_FilterCriticalType_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaCriticalLabelTask_FilterCriticalType_Test Start");
    // Insert photos with critical_type = 0 (should be processed)
    int32_t criticalTypeZeroCount = 10;
    for (int num = 0; num < criticalTypeZeroCount; num++) {
        InsertPhoto(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE), 0);
    }
    
    // Insert photos with critical_type != 0 (should NOT be processed)
    int32_t criticalTypeNonZeroCount = 5;
    for (int num = 0; num < criticalTypeNonZeroCount; num++) {
        InsertPhoto(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE), 1);
    }
    
    auto task = std::make_shared<MediaCriticalLabelTask>();
    EXPECT_NE(g_rdbStore, nullptr);
    PhotoBatchInfo batchInfo = task->QueryPhotosBatch(g_rdbStore, 10);
    // Should only return photos with critical_type = 0
    EXPECT_EQ(batchInfo.size(), criticalTypeZeroCount);
    
    // Verify that HandleCriticalLabelProcessing only processes critical_type = 0 photos
    MedialibrarySubscriber::currentStatus_ = true;
    task->HandleCriticalLabelProcessing();
    MEDIA_INFO_LOG("MediaCriticalLabelTask_FilterCriticalType_Test End");
}
#endif
}  // namespace OHOS::Media::Background