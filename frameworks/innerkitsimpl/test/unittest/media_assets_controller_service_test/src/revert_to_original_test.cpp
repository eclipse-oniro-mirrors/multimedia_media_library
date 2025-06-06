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

#include "revert_to_original_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "message_parcel.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "medialibrary_errno.h"
#include "revert_to_original_vo.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";

static int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

static void ClearPhotosTables()
{
    string clearPhotosSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    vector<string> executeSqlStrs = {
        clearPhotosSql,
    };
    MEDIA_INFO_LOG("start clear data in all tables");
    ExecSqls(executeSqlStrs);
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(const string& displayName, const vector<string>& columns)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

static void InsertAssetIntoPhotosTable()
{
    // file_id,
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + "VALUES (" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1' )"); // cam, pic, shootingmode = 1
}

static void SetAllTestTables()
{
    vector<string> createTableSqlList = {
        PhotoColumn::CREATE_PHOTO_TABLE
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

void RevertToOriginalTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    SetAllTestTables();
    ClearPhotosTables();
    MEDIA_INFO_LOG("SetUpTestCase");
}

void RevertToOriginalTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearPhotosTables();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void RevertToOriginalTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearPhotosTables();
}

void RevertToOriginalTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(RevertToOriginalTest, RevertToOriginal_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start RevertToOriginal_Test_001");
    InsertAssetIntoPhotosTable();
    vector<string> columns;
    auto resultSet = QueryAsset("cam_pic.jpg", columns);
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    MessageParcel data;
    MessageParcel reply;
    RevertToOriginalReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.fileUri = "file://media/Photo/" + to_string(fileId);
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->RevertToOriginal(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t errCode = resp.GetErrCode();
    EXPECT_EQ(errCode, E_SUCCESS);
    MEDIA_INFO_LOG("End RevertToOriginal_Test_001");
}
}  // namespace OHOS::Media