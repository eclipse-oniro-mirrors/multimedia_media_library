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

#define MLOG_TAG "CreateTemporaryCompatibleDuplicateTest"

#include "create_temporary_compatible_duplicate_test.h"

#include "create_tmp_compatible_dup_vo.h"
#include "create_tmp_compatible_dup_dto.h"
#include "media_assets_controller_service.h"
#include "media_file_utils.h"
#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "medialibrary_transcode_data_aging_operation.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace testing::ext;
using namespace OHOS::NativeRdb;
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
} // namespace

static void ClearPhotosTables()
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear table, err: %{public}d", err);
    }
}

static void CreatePhotoTable()
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

static void InsertAsset()
{
    std::string insertSql = R"S(INSERT INTO Photos(data, size, title, display_name, media_type, position, is_temp,
        time_pending, hidden, mime_type, date_trashed) VALUES ('/storage/cloud/files/Photo/666/test.heic', 7879,
        'test', 'test.heic', 1, 0, 0, 0, 0, 'image/heic', 0))S";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static void InsertAssetForAging()
{
    constexpr int64_t transcodeTimeThreshold = 24 * 60 * 60 * 1000;
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t transcodeTime = now - transcodeTimeThreshold - 1000;
    std::string insertSql = "INSERT INTO Photos(data, size, title, display_name, media_type, transcode_time,"
        " trans_code_file_size, exist_compatible_duplicate) VALUES ('/storage/cloud/files/Photo/666/test.heic', 7879,"
        "'test', 'test.heic', 1, " + std::to_string(transcodeTime) + ", 1024, 1)";

    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t QueryFileIdByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return -1;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    return fileId;
}

static int32_t CreateTmpCompatibleDup(int32_t fileId, const std::string &path)
{
    CreateTmpCompatibleDupReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.path = path;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = std::make_shared<MediaAssetsControllerService>();
    if (service == nullptr) {
        MEDIA_ERR_LOG("service is nullptr");
        return -1;
    }
    service->CreateTmpCompatibleDup(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("CreateTmpCompatibleDup ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

void CreateTemporaryCompatibleDuplicateTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("CreateTemporaryCompatibleDuplicateTest::SetUpTestCase:: invoked");

    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start CreateTemporaryCompatibleDuplicateTest failed, can not get g_rdbStore");
        exit(1);
    }
    CreatePhotoTable();
    MEDIA_INFO_LOG("SetUpTestCase end");
}

void CreateTemporaryCompatibleDuplicateTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearPhotosTables();
}

void CreateTemporaryCompatibleDuplicateTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearPhotosTables();
}

void CreateTemporaryCompatibleDuplicateTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(CreateTemporaryCompatibleDuplicateTest, CreateTmpCompatibleDup_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_001 for CreateTmpCompatibleDup Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("test.heic");
    ASSERT_GT(fileId, 0);

    int32_t result = CreateTmpCompatibleDup(fileId, "/storage/cloud/files/Photo/666/test.heic");
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_001 result:%{public}d", result);
#if CONVERT_FORMAT_SUPPORT == 0
    ASSERT_NE(result, 0);
#else
    ASSERT_EQ(result, 0);
#endif
}

HWTEST_F(CreateTemporaryCompatibleDuplicateTest, CreateTmpCompatibleDup_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_002 for CreateTmpCompatibleDup Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("test.heic");
    ASSERT_GT(fileId, 0);

    int32_t result = CreateTmpCompatibleDup(fileId, "/storage/cloud/files/Photo/666/test.heic");
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_002 result:%{public}d", result);
#if CONVERT_FORMAT_SUPPORT == 0
    ASSERT_NE(result, 0);
#else
    ASSERT_EQ(result, 0);
#endif

    result = CreateTmpCompatibleDup(fileId, "/storage/cloud/files/Photo/666/test.heic");
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_002 result:%{public}d", result);
#if CONVERT_FORMAT_SUPPORT == 0
    ASSERT_NE(result, 0);
#else
    ASSERT_EQ(result, 0);
#endif
}

HWTEST_F(CreateTemporaryCompatibleDuplicateTest, CreateTmpCompatibleDup_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_003 for CreateTmpCompatibleDup Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("test.heic");
    ASSERT_GT(fileId, 0);

    int32_t result = CreateTmpCompatibleDup(fileId, "/storage/cloud/files/Photo/606/test.heic");
    MEDIA_INFO_LOG("CreateTmpCompatibleDup_Test_003 result:%{public}d", result);
    ASSERT_NE(result, 0);
}

HWTEST_F(CreateTemporaryCompatibleDuplicateTest, AgingTmpCompatibleDup_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("AgingTmpCompatibleDup_Test_001 for AgingTmpCompatibleDup Begin");
    InsertAssetForAging();
    int32_t fileId = QueryFileIdByDisplayName("test.heic");
    ASSERT_GT(fileId, 0);

    auto dataAging = MediaLibraryTranscodeDataAgingOperation::GetInstance();
    ASSERT_NE(dataAging, nullptr);
    dataAging->AgingTmpCompatibleDuplicates();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    dataAging->InterruptAgingTmpCompatibleDuplicates();
    ASSERT_GT(fileId, 0);
}
} // namespace OHOS::Media