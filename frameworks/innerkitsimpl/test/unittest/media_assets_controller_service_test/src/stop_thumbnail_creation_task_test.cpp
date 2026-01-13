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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "stop_thumbnail_creation_task_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "stop_thumbnail_creation_task_vo.h"
#include "start_thumbnail_creation_task_vo.h"
#include "thumbnail_const.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static constexpr pid_t TEST_PID = 2;
static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " + PhotoColumn::PHOTO_POSITION +
    ", " + PhotoColumn::PHOTO_THUMB_STATUS + ")";
static const string VALUES_END = ") ";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear photos table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void StopThumbnailCreationTaskTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void StopThumbnailCreationTaskTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void StopThumbnailCreationTaskTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("SetUp");
}

void StopThumbnailCreationTaskTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static std::atomic<int32_t> requestId_ = 1;
static int32_t AssignRequestId()
{
    return ++requestId_;
}

static int32_t StartThumbCreationTask(int32_t requestId)
{
    StartThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = requestId;
    reqBody.pid = TEST_PID;
    MEDIA_INFO_LOG("StartThumbCreationTask requestId = %{public}d, pid = %{public}d",
        reqBody.requestId, reqBody.pid);
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->StartThumbnailCreationTask(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("StartThumbCreationTask ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

int32_t StopThumbCreationTask(int32_t requestId)
{
    StopThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = requestId;
    reqBody.pid = TEST_PID;
    MEDIA_INFO_LOG("StopThumbCreationTask requestId = %{public}d", reqBody.requestId);
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->StopThumbnailCreationTask(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("StopThumbCreationTask ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static void InsertAssets(const std::vector<std::string> &displayNames)
{
    std::string insertSql = SQL_INSERT_PHOTO + " VALUES ";
    bool first = true;

    for (const auto &displayName : displayNames) {
        if (!first) {
            insertSql += ", ";
        }
        insertSql += "('/storage/cloud/files/Photo/16/" + displayName + ".jpg', 175258, '" + displayName + "', '" +
                     displayName + ".jpg', 1, " +
                     "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " + "1280, 960, 0, '1', 2, 3)";
        first = false;
    }

    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t CheckThumbnailReadyAllEnd()
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Can not get thumbnail_ready");
        return -1;
    }
    // 移动到第一条记录
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not move to the first row");
        return -1;
    }
    do {
        int32_t thumbnailReady = GetInt32Val(PhotoColumn::PHOTO_THUMBNAIL_READY, resultSet);
        if (thumbnailReady == 0) {
            return 0;
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);  // 移动到下一条记录
    return 1;
}

HWTEST_F(StopThumbnailCreationTaskTest, StopThumbnailCreationTask_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("StopThumbnailCreationTask_Test_001 Begin");
    int32_t result = StopThumbCreationTask(10000000);
    ASSERT_EQ(result, 0);
}

HWTEST_F(StopThumbnailCreationTaskTest, StopThumbnailCreationTask_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("StopThumbnailCreationTask_Test_002 Begin");
    std::vector<std::string> displayNames;
    for (int i = 1; i <= THUMBNAIL_GENERATE_BATCH_COUNT; ++i) {
        displayNames.push_back("cam_pic" + std::to_string(i));
    }
    InsertAssets(displayNames);
    ASSERT_EQ(CheckThumbnailReadyAllEnd(), 0);

    int32_t requestId = AssignRequestId();
    int32_t result = StartThumbCreationTask(requestId);
    ASSERT_EQ(result, 0);
}

HWTEST_F(StopThumbnailCreationTaskTest, StopThumbnailCreationTask_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("StopThumbnailCreationTask_Test_003 Begin");
    std::vector<std::string> displayNames;
    for (int i = 1; i <= THUMBNAIL_GENERATE_BATCH_COUNT; ++i) {
        displayNames.push_back("cam_pic" + std::to_string(i));
    }
    InsertAssets(displayNames);
    ASSERT_EQ(CheckThumbnailReadyAllEnd(), 0);

    int32_t requestId = AssignRequestId();
    int32_t result = StartThumbCreationTask(requestId);
    ASSERT_EQ(result, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    result = StopThumbCreationTask(requestId);
    ASSERT_EQ(result, 0);
}
}  // namespace OHOS::Media