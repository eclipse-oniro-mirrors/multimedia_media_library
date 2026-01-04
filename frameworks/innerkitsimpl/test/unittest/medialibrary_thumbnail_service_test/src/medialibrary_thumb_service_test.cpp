/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <thread>
#include "foundation/ability/form_fwk/test/mock/include/mock_single_kv_store.h"
#include "kvstore.h"
#include "medialibrary_thumb_service_test.h"
#define private public
#include "thumbnail_service.h"
#include "ithumbnail_helper.h"
#include "thumbnail_generate_helper.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_restore_manager.h"
#undef private
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "highlight_column.h"
using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t SLEEP_FIVE_MS = 5000;
static constexpr int64_t TOTALTASKS = 10;
static constexpr int64_t COMPLETEDTASKS = 5;

const int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;

const int32_t E_GETROUWCOUNT_ERROR = 27394103;

shared_ptr<MediaLibraryRdbStore> storePtr_ = nullptr;

void MediaLibraryThumbServiceTest::SetUpTestCase(void) {}

void MediaLibraryThumbServiceTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryThumbServiceTest::SetUp() {}

void MediaLibraryThumbServiceTest::TearDown() {}

HWTEST_F(MediaLibraryThumbServiceTest, GetThumbFd_ShouldReturnValidFd_WhenThumbTypeIsThumb, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string path = "/path/to/thumbnail";
    std::string table = "table";
    std::string id = "id";
    std::string uri = "uri";
    Size size = {100, 100};
    bool isAstc = false;
    int fd = thumbnailService.GetThumbFd(path, table, id, uri, size, isAstc);
    EXPECT_GE(fd, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, GetThumbFd_ShouldReturnValidFd_WhenThumbTypeIsThumbAstc, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string path = "/path/to/thumbnail";
    std::string table = "table";
    std::string id = "id";
    std::string uri = "uri";
    Size size = {100, 100};
    bool isAstc = true;
    int fd = thumbnailService.GetThumbFd(path, table, id, uri, size, isAstc);
    EXPECT_GE(fd, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, GetThumbFd_ShouldReturnValidFd_ThumbTypeIsNotThumbOrThumbAstc, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string path = "/path/to/thumbnail";
    std::string table = "table";
    std::string id = "id";
    std::string uri = "uri";
    Size size = {100, 1000}; // This size will result in a thumbType other than Thumb or ThumbAstc
    bool isAstc = false;
    int fd = thumbnailService.GetThumbFd(path, table, id, uri, size, isAstc);
    EXPECT_GE(fd, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, GetKeyFrameThumbFd_Success_Test, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    string path = "/test/path";
    string table = "test_table";
    string id = "test_id";
    string uri = "test_uri";
    int32_t beginStamp = 0;
    int32_t type = 0;
    int fd = thumbnailService.GetKeyFrameThumbFd(path, table, id, uri, beginStamp, type);
    EXPECT_GT(fd, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, CreateThumbnailPastDirtyDataFix_Success, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string fileId = "testFileId";
    int32_t result = thumbnailService.CreateThumbnailPastDirtyDataFix(fileId);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, CreateThumbnailPastDirtyDataFix_QueryFailure, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string fileId = "invalidFileId";
    int32_t result = thumbnailService.CreateThumbnailPastDirtyDataFix(fileId);
    EXPECT_NE(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_CreateLcdPastDirtyDataFix_001, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string fileId = "testFileId";
    int32_t result = thumbnailService.CreateLcdPastDirtyDataFix(fileId);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_CreateThumbnailFileScaned_ParseThumbnailParam_Error, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string uri = "invalid_uri";
    std::string path = "/invalid/path";
    bool isSync = true;
    // This test will fail if ParseThumbnailParam is not public
    // If it's not public, we need to find a public method that indirectly tests this functionality
    int32_t result = thumbnailService.CreateThumbnailFileScaned(uri, path, isSync);
    // Verify that an error was posted
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_CreateThumbnailFileScaned_CreateThumbnailFileScaned_Error, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string uri = "valid_uri";
    std::string path = "/valid/path";
    bool isSync = true;
    // This test will fail if CreateThumbnailFileScaned is not public
    // If it's not public, we need to find a public method that indirectly tests this functionality
    int32_t result = thumbnailService.CreateThumbnailFileScaned(uri, path, isSync);
    // Verify that an error was posted
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_CreateThumbnailFileScaned_Success, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string uri = "valid_uri";
    std::string path = "/valid/path";
    bool isSync = true;
    // This test will fail if CreateThumbnailFileScaned is not public
    // If it's not public, we need to find a public method that indirectly tests this functionality
    int32_t result = thumbnailService.CreateThumbnailFileScaned(uri, path, isSync);
    // Verify that an error was posted
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, UpgradeThumbnailBackground_WhenWifiNotConnected, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    int32_t result = thumbnailService.UpgradeThumbnailBackground(false);
    EXPECT_NE(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, UpgradeThumbnailBackground_WhenWifiConnected, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    int32_t result = thumbnailService.UpgradeThumbnailBackground(true);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_TriggerHighlightThumbnail_Success, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string id = "testId";
    std::string tracks = "testTracks";
    std::string trigger = "testTrigger";
    std::string genType = "testGenType";
    int32_t result = thumbnailService.TriggerHighlightThumbnail(id, tracks, trigger, genType);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, LcdAging_ShouldReturnE_OK_WhenAllTablesAgingSuccess, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    int32_t result = thumbnailService.LcdAging();
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_DeleteThumbnailDirAndAstc_Success, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string id = "123";
    std::string tableName = "testTable";
    std::string path = "/test/path";
    std::string dateTaken = "2022-01-01";
    bool result = thumbnailService.DeleteThumbnailDirAndAstc(id, tableName, path, dateTaken);
    EXPECT_TRUE(result);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_BatchDeleteThumbnailDirAndAstc_Success, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string tableName = "test_table";
    std::vector<std::string> ids = {"id1", "id2"};
    std::vector<std::string> paths = {"path1", "path2"};
    std::vector<std::string> dateTakens = {"date1", "date2"};
    // Call the public method directly
    bool result = thumbnailService.BatchDeleteThumbnailDirAndAstc(tableName, ids, paths, dateTakens);
    // Expect the method to return true if the internal call is successful
    EXPECT_TRUE(result);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_BatchDeleteThumbnailDirAndAstc_Failure, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    std::string tableName = "test_table";
    std::vector<std::string> ids = {};
    std::vector<std::string> paths = {};
    std::vector<std::string> dateTakens = {};
    // Call the public method directly
    bool result = thumbnailService.BatchDeleteThumbnailDirAndAstc(tableName, ids, paths, dateTakens);
    // Expect the method to return false if the input is empty
    EXPECT_FALSE(result);
}

HWTEST_F(MediaLibraryThumbServiceTest, ATC_CreateAstcBatchOnDemand_InvalidRequestId_Test, TestSize.Level0)
{
    ThumbnailService thumbnailService;
    NativeRdb::RdbPredicates rdbPredicate { PhotoColumn::PHOTOS_TABLE };
    int32_t requestId = -1;
    int32_t result = thumbnailService.CreateAstcBatchOnDemand(rdbPredicate, requestId);
    EXPECT_EQ(result, E_INVALID_VALUES);
}


HWTEST_F(MediaLibraryThumbServiceTest, medialib_GetThumbnail_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    string uri = "";
    auto fd = serverTest->GetThumbnailFd(uri);
    EXPECT_LT(fd, 0);
    uri = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    fd = serverTest->GetThumbnailFd(uri);
    EXPECT_LT(fd, 0);
    shared_ptr<DistributedKv::SingleKvStore> kvStorePtr = make_shared<MockSingleKvStore>();
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
    fd = serverTest->GetThumbnailFd(uri);
    EXPECT_LT(fd, 0);
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_GetKeyFrameThumbnail_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    string uri = "";
    auto fd = serverTest->GetKeyFrameThumbnailFd(uri);
    EXPECT_LT(fd, 0);
    uri = "ParseKeyFrameThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_KEY_FRAME + "&" +
          THUMBNAIL_BEGIN_STAMP + "=1&" + THUMBNAIL_TYPE + "=1";
    fd = serverTest->GetKeyFrameThumbnailFd(uri);
    EXPECT_LT(fd, 0);
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_LcdAging_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    int32_t ret = serverTest->LcdAging();
    EXPECT_EQ(ret, 0);
    shared_ptr<DistributedKv::SingleKvStore> kvStorePtr = make_shared<MockSingleKvStore>();
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
    ret = serverTest->LcdAging();
    EXPECT_EQ(ret, 0);
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_GenerateThumbnailBackground_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    int32_t ret = serverTest->GenerateThumbnailBackground();
    EXPECT_EQ(ret <= 0, true);
    shared_ptr<DistributedKv::SingleKvStore> kvStorePtr = make_shared<MockSingleKvStore>();
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(nullptr, context);
    ret = serverTest->GenerateThumbnailBackground();
    EXPECT_NE(ret, 0);
    serverTest->Init(storePtr_, context);
    ret = serverTest->GenerateThumbnailBackground();
    EXPECT_NE(ret, 0);
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_InterruptBgworker_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    EXPECT_NE(serverTest, nullptr);
    serverTest->InterruptBgworker();
    shared_ptr<DistributedKv::SingleKvStore> kvStorePtr = make_shared<MockSingleKvStore>();
    EXPECT_NE(kvStorePtr, nullptr);
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
    serverTest->InterruptBgworker();
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_StopAllWorker_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    EXPECT_NE(serverTest, nullptr);
    serverTest->StopAllWorker();
    shared_ptr<DistributedKv::SingleKvStore> kvStorePtr = make_shared<MockSingleKvStore>();
    EXPECT_NE(kvStorePtr, nullptr);
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
    serverTest->StopAllWorker();
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_CreateThumbnailAsync_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    string url = "";
    ThumbnailService serverTest;
    int32_t ret = serverTest.CreateThumbnailFileScaned(url, "", true);
    EXPECT_EQ(ret, E_OK);
    serverTest.ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_CreateAstcBatchOnDemand_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
 
    NativeRdb::RdbPredicates predicate { PhotoColumn::PHOTOS_TABLE };
    int32_t requestId = 1;
    int32_t result = serverTest->CreateAstcBatchOnDemand(predicate, requestId);
    EXPECT_EQ(result, E_GETROUWCOUNT_ERROR);
    serverTest->ReleaseService();
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_CancelAstcBatchTask_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    EXPECT_NE(serverTest, nullptr);
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
 
    NativeRdb::RdbPredicates predicate { PhotoColumn::PHOTOS_TABLE };
    int32_t requestId = 1;
    serverTest->CancelAstcBatchTask(requestId);
    serverTest->CancelAstcBatchTask(++requestId);
    serverTest->ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_001, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::CreateLcdAndThumbnail(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_002, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::CreateLcd(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_003, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::CreateThumbnail(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_004, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::CreateAstc(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_005, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::CreateAstcEx(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_006, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::DeleteMonthAndYearAstc(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_007, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::TryLoadSource(opts, data);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_008, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::TryLoadSource(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_009, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto ret = IThumbnailHelper::DoCreateLcd(opts, data);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_010, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::IsCreateLcdSuccess(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_011, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.isLocalFile = false;
    auto res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_012, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_013, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_014, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB;
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_015, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::LCD;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_016, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_017, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB_ASTC;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_018, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    data.dateTaken = "default value";
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_019, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::YEAR_ASTC;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    data.dateTaken = "default value";
    auto res = IThumbnailHelper::GenThumbnail(opts, data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_020, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.isLocalFile = false;
    auto res = IThumbnailHelper::GenThumbnailEx(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_021, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::GenThumbnailEx(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_022, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::LCD;
    auto res = IThumbnailHelper::GenMonthAndYearAstcData(data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_023, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::DoCreateThumbnail(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_024, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.isLocalFile = false;
    auto res = IThumbnailHelper::IsCreateThumbnailExSuccess(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_025, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::DoRotateThumbnail(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_026, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::DoRotateThumbnail(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_027, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::DoCreateAstc(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_028, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    data.loaderOpts.needUpload = true;
    auto res = IThumbnailHelper::DoCreateAstc(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_029, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.path = "/storage/cloud/files/";
    auto res = IThumbnailHelper::DoCreateAstcEx(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_030, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = IThumbnailHelper::DoCreateAstcEx(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_031, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    int32_t fd = 0;
    ThumbnailType thumbType = ThumbnailType::THUMB;
    auto res = IThumbnailHelper::DoRotateThumbnailEx(opts, data, fd, thumbType);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_032, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = IThumbnailHelper::IsPureCloudImage(opts);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_033, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.row = "a";
    auto res = IThumbnailHelper::IsPureCloudImage(opts);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_034, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.table = "b";
    auto res = IThumbnailHelper::IsPureCloudImage(opts);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_035, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.row = "a";
    opts.table = "b";
    auto res = IThumbnailHelper::IsPureCloudImage(opts);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_036, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    IThumbnailHelper::UpdateAstcDateTaken(data);
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    IThumbnailHelper::CreateLcdAndThumbnail(dataValue);
    EXPECT_EQ(requestId, dataValue->requestId_);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_005, TestSize.Level1)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    ThumbnailData data;
    Size desiredSize;
    uint32_t errCode = 0;
    auto res = ThumbnailUtils::LoadAudioFileInfo(avMetadataHelper, data, desiredSize, errCode);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_006, TestSize.Level1)
{
    ThumbnailData data;
    Size desiredSize;
    auto res = ThumbnailUtils::LoadAudioFile(data, desiredSize);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_007, TestSize.Level1)
{
    string path = "";
    string suffix;
    string fileName;
    auto res = ThumbnailUtils::SaveFileCreateDir(path, suffix, fileName);
    EXPECT_EQ(res, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_008, TestSize.Level1)
{
    ThumbnailData data;
    data.thumbnail.push_back(0x12);
    string fileName;
    uint8_t *output = data.thumbnail.data();
    int writeSize = 0;
    auto res = ThumbnailUtils::ToSaveFile(data, fileName, output, writeSize);
    EXPECT_EQ(res<0, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_009, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::MTH;
    auto res = ThumbnailUtils::TrySaveFile(data, type);
    EXPECT_EQ(res, -223);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_010, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::MTH_ASTC;
    data.monthAstc.push_back(1);
    data.monthAstc.resize(1);
    auto res = ThumbnailUtils::TrySaveFile(data, type);
    EXPECT_EQ(res, -1);
    ThumbnailType type2 = ThumbnailType::YEAR_ASTC;
    data.yearAstc.push_back(1);
    data.yearAstc.resize(1);
    auto res2 = ThumbnailUtils::TrySaveFile(data, type2);
    EXPECT_EQ(res2, -1);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_011, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB;
    auto res = ThumbnailUtils::TrySaveFile(data, type);
    EXPECT_EQ(res, -2302);
    ThumbnailType type2 = ThumbnailType::THUMB_ASTC;
    auto res2 = ThumbnailUtils::TrySaveFile(data, type2);
    EXPECT_EQ(res2, -2302);
    ThumbnailType type3 = ThumbnailType::LCD;
    auto res3 = ThumbnailUtils::TrySaveFile(data, type3);
    EXPECT_EQ(res3, -2302);
    ThumbnailType type4 = ThumbnailType::LCD_EX;
    auto res4 = ThumbnailUtils::TrySaveFile(data, type4);
    EXPECT_EQ(res4, -2302);
    ThumbnailType type5 = ThumbnailType::THUMB_EX;
    auto res5 = ThumbnailUtils::TrySaveFile(data, type5);
    EXPECT_EQ(res5, -2302);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_012, TestSize.Level1)
{
    ThumbnailData data;
    data.thumbnail.push_back(0x12);
    std::string suffix;
    uint8_t *output = data.thumbnail.data();
    int writeSize = 0;
    auto res = ThumbnailUtils::SaveThumbDataToLocalDir(data, suffix, output, writeSize);
    EXPECT_EQ(res<0, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_013, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB;
    auto res = ThumbnailUtils::SaveAstcDataToKvStore(data, type);
    EXPECT_EQ(res, -1);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_014, TestSize.Level1)
{
    ThumbnailData data;
    data.id = "a";
    data.dateTaken = "b";
    const ThumbnailType type = ThumbnailType::LCD;
    auto res = ThumbnailUtils::SaveAstcDataToKvStore(data, type);
    EXPECT_EQ(res, E_ERR);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_017, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.dateTaken = "a";
    auto res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_019, TestSize.Level1)
{
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    bool isSourceEx = false;
    auto res = ThumbnailUtils::ScaleThumbnailFromSource(data, isSourceEx);
    EXPECT_EQ(res, false);
    bool isSourceEx2 = true;
    auto res2 = ThumbnailUtils::ScaleThumbnailFromSource(data, isSourceEx2);
    EXPECT_EQ(res2, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_020, TestSize.Level1)
{
    NativeRdb::ValuesBucket values;
    Size size;
    size.height = 1;
    size.width = 0;
    std::string column;
    ThumbnailUtils::SetThumbnailSizeValue(values, size, column);
    NativeRdb::ValuesBucket values2;
    Size size2;
    size2.height = 0;
    size2.width = 1;
    ThumbnailUtils::SetThumbnailSizeValue(values2, size2, column);
    EXPECT_NE(size.height, size2.height);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_022, TestSize.Level1)
{
    string path = "";
    string suffix;
    string fileName;
    string timeStamp = "1";
    auto res = ThumbnailUtils::SaveFileCreateDirHighlight(path, suffix, fileName, timeStamp);
    EXPECT_EQ(res, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_001, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailType thumbType = ThumbnailType::LCD;
    ThumbnailData data;
    auto res = ThumbnailGenerateHelper::GetThumbnailPixelMap(data, opts, thumbType);
    EXPECT_EQ(res, -2302);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_002, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = ThumbnailGenerateHelper::UpgradeThumbnailBackground(opts, false);
    EXPECT_NE(res, 0);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_003, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    opts.table = "test";
    auto res = ThumbnailGenerateHelper::UpgradeThumbnailBackground(opts, false);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, UpgradeThumbnailBackground_test_001, TestSize.Level1)
{
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    auto res = serverTest->UpgradeThumbnailBackground(false);
    EXPECT_EQ(res <= 0, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, GenerateHighlightThumbnailBackground_test_001, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = ThumbnailGenerateHelper::GenerateHighlightThumbnailBackground(opts);
    EXPECT_EQ(res, -1);
}

HWTEST_F(MediaLibraryThumbServiceTest, GenerateHighlightThumbnailBackground_test_002, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    opts.table = "tab_analysis_video_label";
    auto res = ThumbnailGenerateHelper::GenerateHighlightThumbnailBackground(opts);
    EXPECT_EQ(res < 0, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, GenerateHighlightThumbnailBackground_test_003, TestSize.Level1)
{
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    auto res = serverTest->GenerateHighlightThumbnailBackground();
    EXPECT_EQ(res < 0, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_023, TestSize.Level1)
{
    ThumbnailData data;
    std::string tempOutputPath;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    bool isSourceEx = true;
    std::shared_ptr<Picture> pictureEx = Picture::Create(pixelMap);
    data.source.SetPictureEx(pictureEx);
    auto res = ThumbnailUtils::CompressPicture(data, pictureEx, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
    bool isSourceEx2 = false;
    auto res2 = ThumbnailUtils::CompressPicture(data, pictureEx, isSourceEx2, tempOutputPath);
    EXPECT_EQ(res2, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_024, TestSize.Level1)
{
    ThumbnailData data;
    ThumbRdbOpt opts;
    auto res = ThumbnailUtils::DoUpdateAstcDateTaken(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_026, TestSize.Level1)
{
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    ThumbRdbOpt opts;
    auto res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_027, TestSize.Level1)
{
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    ThumbRdbOpt opts;
    auto res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
}

 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_032, TestSize.Level1)
{
    ThumbnailData data;
    ThumbRdbOpt opts;
    int32_t err = E_ERR;
    auto res = ThumbnailUtils::UpdateLcdReadyStatus(opts, data, err, LcdReady::GENERATE_LCD_COMPLETED);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_005, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    auto res = ThumbnailGenerateHelper::GenerateHighlightThumbnailBackground(opts);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_006, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    int32_t timeStamp = 100;
    int32_t type = 1;
    auto res = ThumbnailGenerateHelper::GetKeyFrameThumbnailPixelMap(opts, timeStamp, type);
    EXPECT_NE(res, E_ERR);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_007, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    ThumbnailData data;data.path = "path";
    data.timeStamp = "timeStamp";
    int32_t thumbType = 1;
    std::string fileName;
    auto res = ThumbnailGenerateHelper::GetAvailableKeyFrameFile(opts, data, thumbType, fileName);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_008, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    opts.table = "test";
    auto res = ThumbnailGenerateHelper::UpgradeThumbnailBackground(opts, false);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_010, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    ThumbnailType thumbType = ThumbnailType::THUMB_ASTC;
    std::string fileName;
    auto res = ThumbnailGenerateHelper::GetAvailableFile(opts, data, thumbType, fileName);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_011, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    int32_t thumbType = 2;
    std::string fileName;
    auto res = ThumbnailGenerateHelper::GetAvailableKeyFrameFile(opts, data, thumbType, fileName);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_012, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    std::string id = "id";
    std::string tracks = "tracks";
    std::string trigger = "trigger";
    std::string genType = "update";
    auto res = ThumbnailGenerateHelper::TriggerHighlightThumbnail(opts, id, tracks, trigger, genType);
    EXPECT_EQ(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_019, TestSize.Level1)
{
    ThumbnailData data;
    ThumbnailType thumbType = ThumbnailType::THUMB_ASTC;
    std::string fileName = ThumbnailGenerateHelper::GetAvailablePath(data.path, thumbType);
    EXPECT_EQ(fileName, "");
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_037, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    opts.row = "row";
    opts.table = "table";
    auto res = IThumbnailHelper::IsPureCloudImage(opts);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_038, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    ThumbnailData data;
    data.path = "/storage/cloud/files/";
    auto res = IThumbnailHelper::CacheSuccessState(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_039, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = std::make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::IsCreateThumbnailSuccess(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_040, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMapEX = std::make_shared<PixelMap>();
    data.source.SetPixelMapEx(pixelMapEX);
    auto res = IThumbnailHelper::GenThumbnailEx(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_041, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = std::make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    auto res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
    std::shared_ptr<PixelMap> pixelMapEX = std::make_shared<PixelMap>();
    data.source.SetPixelMapEx(pixelMapEX);
    res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
    std::shared_ptr<Picture> picture = Picture::Create(pixelMap);
    data.source.SetPicture(picture);
    res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_042, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = std::make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    std::shared_ptr<PixelMap> pixelMapEX = std::make_shared<PixelMap>();
    data.source.SetPixelMapEx(pixelMapEX);
    auto res = IThumbnailHelper::IsCreateLcdExSuccess(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_test_043, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = std::make_shared<PixelMap>();
    std::shared_ptr<Picture> picture = Picture::Create(pixelMap);
    data.source.SetPicture(picture);
    auto res = IThumbnailHelper::IsCreateLcdSuccess(opts, data);
    EXPECT_EQ(res, false);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, GetThumbFd_test_001, TestSize.Level1)
{
    string path = " ";
    string table = PhotoColumn::PHOTOS_TABLE;
    string id = "0";
    string uri = " ";
    Size desiredSize;
    bool isAstc = false;
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    int res = serverTest->GetThumbFd(path, table, id, uri, desiredSize, isAstc);
    EXPECT_EQ(res < 0, true);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, GetKeyFrameThumbFd_test_001, TestSize.Level1)
{
    string path = " ";
    string table = PhotoColumn::PHOTOS_TABLE;
    string id = "0";
    string uri = " ";
    int32_t beginStamp = 0;
    int32_t type = 1;
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    shared_ptr<OHOS::AbilityRuntime::Context> context;
    serverTest->Init(storePtr_, context);
    int32_t res = serverTest->GetKeyFrameThumbFd(path, table, id, uri, beginStamp, type);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, GetAgingDataSize_test_001, TestSize.Level1)
{
    int64_t time = -100;
    int count = 0;
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    int32_t res = serverTest->GetAgingDataSize(time, count);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, CreateAstcCloudDownload_test_001, TestSize.Level1)
{
    shared_ptr<ThumbnailService> serverTest = ThumbnailService::GetInstance();
    string id = "invalidId";
    bool isCloudInsertTaskPriorityHigh = false;
    int32_t res = serverTest->CreateAstcCloudDownload(id, isCloudInsertTaskPriorityHigh);
    EXPECT_NE(res, E_OK);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_034, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = ThumbnailUtils::QueryThumbnailSet(opts);
    EXPECT_EQ(res, nullptr);
}
 
HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumnail_utils_test_035, TestSize.Level1)
{
    ThumbRdbOpt opts;
    int outLcdCount = 0;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_CreateThumbnailWithPictureAsync_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr_, nullptr);
    string url = "";
    ThumbnailService serverTest;
    std::shared_ptr<Picture> originalPhotoPicture = nullptr;
    int32_t ret = serverTest.CreateThumbnailFileScanedWithPicture(url, "", originalPhotoPicture, true);
    EXPECT_NE(ret, E_OK);
    serverTest.ReleaseService();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_014, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = ThumbnailGenerateHelper::CreateAstcMthAndYear(opts);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_CreateAstcMthAndYear_Test_001, TestSize.Level1)
{
    ThumbnailService serverTest;
    string id = "invalid";
    auto res = serverTest.CreateAstcMthAndYear(id);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_015, TestSize.Level1)
{
    ThumbRdbOpt opts;
    auto res = ThumbnailGenerateHelper::CheckLcdSizeAndUpdateStatus(opts);
    EXPECT_NE(res, E_OK);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailGenerateHelper::CheckLcdSizeAndUpdateStatus(opts);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_016, TestSize.Level1)
{
    ThumbRdbOpt opts;
    int outLcdCount;
    auto res = ThumbnailGenerateHelper::GetLcdCount(opts, outLcdCount);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_017, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    auto res = ThumbnailGenerateHelper::GetLocalNoLcdData(opts, infos);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_generate_helper_test_018, TestSize.Level1)
{
    ThumbRdbOpt opts;
    const int64_t time = 0;
    int count = 0;
    auto res = ThumbnailGenerateHelper::GetNewThumbnailCount(opts, time, count);
    EXPECT_NE(res, E_OK);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_031, TestSize.Level1)
{
    ThumbnailData data;
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    int32_t err = E_ERR;
    auto res = ThumbnailUtils::UpdateLcdReadyStatus(opts, data, err, LcdReady::GENERATE_LCD_COMPLETED);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_036, TestSize.Level1)
{
    int64_t time = 0;
    bool before = false;
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    int outLcdCount = 0;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);

    opts.store = nullptr;
    res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);

    opts.table = PhotoColumn::PHOTOS_TABLE;
    res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);

    before = true;
    res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_037, TestSize.Level1)
{
    ThumbRdbOpt opts;
    int outLcdCount = 0;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryDistributeLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_038, TestSize.Level1)
{
    ThumbRdbOpt opts;
    int outLcdCount = 0;
    int LcdLimit = 0;
    vector<ThumbnailData> infos;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryAgingLcdInfos(opts, LcdLimit, infos, err);
    EXPECT_EQ(res, false);

    opts.table = PhotoColumn::PHOTOS_TABLE;
    res = ThumbnailUtils::QueryAgingLcdInfos(opts, LcdLimit, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_039, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryNoLcdInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_040, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryNoThumbnailInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_041, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    bool isWifiConnected = false;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryUpgradeThumbnailInfos(opts, infos, isWifiConnected, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_042, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    int err = E_ERR;
    const int32_t restoreAstcCount = 0;
    auto res = ThumbnailUtils::QueryNoAstcInfosRestored(opts, infos, err, restoreAstcCount);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_043, TestSize.Level1)
{
    ThumbRdbOpt opts;
    vector<ThumbnailData> infos;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryNoAstcInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_044, TestSize.Level1)
{
    ThumbRdbOpt opts;
    opts.store = storePtr_;
    int64_t time = 0;
    int count = 0;
    int err = E_ERR;
    auto res = ThumbnailUtils::QueryNewThumbnailCount(opts, time, count, err);
    EXPECT_EQ(res, false);

    opts.store = nullptr;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    res = ThumbnailUtils::QueryNewThumbnailCount(opts, time, count, err);
    EXPECT_EQ(res, false);

    opts.table = MEDIALIBRARY_TABLE;
    res = ThumbnailUtils::QueryNewThumbnailCount(opts, time, count, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_045, TestSize.Level1)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    auto res = ThumbnailUtils::CacheLcdInfo(opts, data);
    EXPECT_NE(res, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_CacheLcdInfo_test, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.row = "a";
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    opts.table = "tab_analysis_video_label";
    ThumbnailData data;
    auto res = ThumbnailUtils::CacheLcdInfo(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_047, TestSize.Level1)
{
    ThumbRdbOpt opts;
    bool withThumb = false;
    bool withLcd = false;
    opts.table = MEDIALIBRARY_TABLE;
    auto res = ThumbnailUtils::CleanThumbnailInfo(opts, withThumb, withLcd);
    EXPECT_EQ(res, false);

    opts.table = PhotoColumn::PHOTOS_TABLE;
    res = ThumbnailUtils::CleanThumbnailInfo(opts, withThumb, withLcd);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_048, TestSize.Level1)
{
    string path = "";
    auto res = ThumbnailUtils::SetSource(nullptr, path);
    EXPECT_EQ(res, E_ERR);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_utils_test_049, TestSize.Level1)
{
    uint8_t Value = 1;
    vector<uint8_t> data;
    data.push_back(Value);
    Size size;
    unique_ptr<PixelMap> pixelMap;
    auto res = ThumbnailUtils::ResizeImage(data, size, pixelMap);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_TrySavePixelMap_test, TestSize.Level0)
{
    ThumbnailData thumbData;
    thumbData.id = "0";
    thumbData.dateModified = "data_modified";
    auto ret = IThumbnailHelper::TrySavePixelMap(thumbData, ThumbnailType::LCD);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, medialib_thumbnail_helper_TrySavePicture_test, TestSize.Level0)
{
    ThumbnailData thumbData;
    thumbData.id = "0";
    thumbData.dateModified = "data_modified";
    string tempOutputPath = "path";
    auto ret = IThumbnailHelper::TrySavePicture(thumbData, true, tempOutputPath);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_001, TestSize.Level0)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    thumbnailRestoreManager.InitializeRestore(TOTALTASKS);
    EXPECT_EQ(thumbnailRestoreManager.totalTasks_.load(), TOTALTASKS);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_002, TestSize.Level0)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    thumbnailRestoreManager.ReportProgressBegin();
    thumbnailRestoreManager.totalTasks_.store(TOTALTASKS);
    thumbnailRestoreManager.ReportProgressBegin();
    thumbnailRestoreManager.AddCompletedTasks(0);
    thumbnailRestoreManager.AddCompletedTasks(COMPLETEDTASKS);
    thumbnailRestoreManager.AddCompletedTasks(COMPLETEDTASKS);
    EXPECT_EQ(thumbnailRestoreManager.completedTasks_.load(), TOTALTASKS);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_003, TestSize.Level0)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    thumbnailRestoreManager.StartProgressReporting(SLEEP_FIVE_MS);
    thumbnailRestoreManager.isReporting_.store(true);

    thumbnailRestoreManager.StartProgressReporting(SLEEP_FIVE_MS);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    EXPECT_EQ(thumbnailRestoreManager.isReporting_.load(), true);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_004, TestSize.Level0)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    thumbnailRestoreManager.OnScreenStateChanged(false);

    thumbnailRestoreManager.isRestoreActive_.store(true);
    thumbnailRestoreManager.OnScreenStateChanged(false);

    thumbnailRestoreManager.lastScreenState_.store(true);
    thumbnailRestoreManager.OnScreenStateChanged(false);

    thumbnailRestoreManager.OnScreenStateChanged(false);

    thumbnailRestoreManager.isRestoreActive_.store(false);
    thumbnailRestoreManager.OnScreenStateChanged(true);

    thumbnailRestoreManager.OnScreenStateChanged(true);

    thumbnailRestoreManager.isRestoreActive_.store(true);
    thumbnailRestoreManager.OnScreenStateChanged(true);

    thumbnailRestoreManager.lastScreenState_.store(false);
    thumbnailRestoreManager.OnScreenStateChanged(true);
    EXPECT_EQ(thumbnailRestoreManager.lastScreenState_.load(), true);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_005, TestSize.Level0)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    thumbnailRestoreManager.ReportProgress(false);
    thumbnailRestoreManager.ReportProgress(true);
    thumbnailRestoreManager.completedTasks_.store(COMPLETEDTASKS);

    thumbnailRestoreManager.ReportProgress(true);
    thumbnailRestoreManager.totalTasks_.store(TOTALTASKS);
    thumbnailRestoreManager.ReportProgress(true);
    EXPECT_EQ(thumbnailRestoreManager.readyAstc_.load(), COMPLETEDTASKS);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_006, TestSize.Level1)
{
    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    ThumbRdbOpt opts;
    auto res = thumbnailRestoreManager.RestoreAstcDualFrame(opts);
    EXPECT_NE(res, E_OK);

    opts.store = storePtr_;
    res = thumbnailRestoreManager.RestoreAstcDualFrame(opts);
    EXPECT_NE(res, E_OK);
    thumbnailRestoreManager.Reset();
}

HWTEST_F(MediaLibraryThumbServiceTest, thumbnail_restore_manager_test_007, TestSize.Level1)
{
    std::shared_ptr<ThumbnailTaskData> data;
    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    int32_t requestId;
    std::shared_ptr<ThumbnailTaskData> dataValue = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    ThumbnailRestoreManager::RestoreAstcDualFrameTask(data);

    ThumbnailRestoreManager::RestoreAstcDualFrameTask(dataValue);

    auto& thumbnailRestoreManager = ThumbnailRestoreManager::GetInstance();
    EXPECT_EQ(thumbnailRestoreManager.readyAstc_.load(), 0);
    thumbnailRestoreManager.Reset();
}
}
}
