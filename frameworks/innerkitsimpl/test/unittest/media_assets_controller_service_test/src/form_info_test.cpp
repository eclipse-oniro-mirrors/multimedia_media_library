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

#include "form_info_test.h"

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
#include "media_facard_photos_column.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "medialibrary_errno.h"
#include "parameter_utils.h"
#include "form_info_vo.h"
#include "form_map.h"
#include "media_upgrade.h"

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
const std::string CREATE_FACARD_TABLE_SQL = "CREATE TABLE IF NOT EXISTS tab_facard_photos \
    (form_id TEXT, asset_uri TEXT);";

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

static void ClearFormInfoTables()
{
    string clearFormInfoSql = "DELETE FROM " + FormMap::FORM_MAP_TABLE;
    string clearPhotosSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    string clearPhototabCardSql = "DELETE FROM " + TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE;
    vector<string> executeSqlStrs = {
        clearPhotosSql,
        clearFormInfoSql,
        clearPhototabCardSql
    };
    MEDIA_INFO_LOG("start clear data in all tables");
    ExecSqls(executeSqlStrs);
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
        FormMap::CREATE_FORM_MAP_TABLE,
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_FACARD_TABLE_SQL
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

void FormInfoTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    SetAllTestTables();
    ClearFormInfoTables();
    MEDIA_INFO_LOG("SetUpTestCase");
}

void FormInfoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearFormInfoTables();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void FormInfoTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearFormInfoTables();
}

void FormInfoTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static void PrepareNormalFormInfoReq(FormInfoReqBody &reqBody)
{
    InsertAssetIntoPhotosTable();
    vector<string> columns;
    auto resultSet = QueryAsset("cam_pic.jpg", columns);
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    vector<string> formIds = { "1057934318" };
    reqBody.formIds = formIds;
    string uri = "file://media/Photo/" + to_string(fileId);
    vector<string> fileUris = { uri };
    reqBody.fileUris = fileUris;
}

HWTEST_F(FormInfoTest, Form_Util_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Form_Util_Test_001");
    vector<string> formIds = { "202505231755202505231755202505231755" };
    int32_t errorCode = ParameterUtils::CheckFormIds(formIds);
    EXPECT_EQ(errorCode, E_INVALID_ARGUMENTS);
    formIds = { "aaa" };
    errorCode = ParameterUtils::CheckFormIds(formIds);
    EXPECT_EQ(errorCode, E_INVALID_ARGUMENTS);
    formIds = { "9223372036854775808" };
    errorCode = ParameterUtils::CheckFormIds(formIds);
    EXPECT_EQ(errorCode, E_INVALID_ARGUMENTS);
    formIds = { "1057934318" };
    errorCode = ParameterUtils::CheckFormIds(formIds);
    EXPECT_EQ(errorCode, E_OK);
    MEDIA_INFO_LOG("End Form_Util_Test_001");
}

HWTEST_F(FormInfoTest, Save_Form_Info_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Form_Info_Test_001");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);
    MEDIA_INFO_LOG("End Save_Form_Info_Test_001");
}

HWTEST_F(FormInfoTest, Save_Form_Info_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Form_Info_Test_002");
    MessageParcel data;
    MessageParcel reply;
    FormInfoReqBody reqBody;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_LT(formId, 0);
    MEDIA_INFO_LOG("End Save_Form_Info_Test_002");
}

HWTEST_F(FormInfoTest, Save_Form_Info_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Form_Info_Test_003");
    MessageParcel data;
    MessageParcel reply;
    FormInfoReqBody reqBody;
    reqBody.formIds = { "" };
    reqBody.fileUris = { "" };
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_LT(formId, 0);
    MEDIA_INFO_LOG("End Save_Form_Info_Test_003");
}

HWTEST_F(FormInfoTest, Save_Form_Info_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Form_Info_Test_004");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);

    errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    service->SaveFormInfo(data, reply);
    isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t changedRows = resp.GetErrCode();
    EXPECT_EQ(changedRows, 1);
    MEDIA_INFO_LOG("End Save_Form_Info_Test_004");
}

HWTEST_F(FormInfoTest, Remove_Form_Info_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Remove_Form_Info_Test_001");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);

    errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    service->RemoveFormInfo(data, reply);
    isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t changedRows = resp.GetErrCode();
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("End Remove_Form_Info_Test_001");
}

HWTEST_F(FormInfoTest, Save_Gallery_Form_Info_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Gallery_Form_Info_Test_001");
    MessageParcel data;
    MessageParcel reply;
    FormInfoReqBody reqBody;
    vector<string> formIds = { "1057934318" };
    reqBody.formIds = formIds;
    vector<string> fileUris = { "file://media/Photo/1" };
    reqBody.fileUris = fileUris;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveGalleryFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);
    MEDIA_INFO_LOG("End Save_Gallery_Form_Info_Test_001");
}

HWTEST_F(FormInfoTest, Save_Gallery_Form_Info_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Save_Gallery_Form_Info_Test_002");
    MessageParcel data;
    MessageParcel reply;
    FormInfoReqBody reqBody;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveGalleryFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t err = resp.GetErrCode();
    EXPECT_LT(err, 0);
    MEDIA_INFO_LOG("End Save_Gallery_Form_Info_Test_002");
}

HWTEST_F(FormInfoTest, Remove_Gallery_Form_Info_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Remove_Gallery_Form_Info_Test_001");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveGalleryFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);

    errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    service->RemoveGalleryFormInfo(data, reply);
    isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t changedRows = resp.GetErrCode();
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("End Remove_Gallery_Form_Info_Test_001");
}

HWTEST_F(FormInfoTest, Update_Gallery_Form_Info_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Update_Gallery_Form_Info_Test_001");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveGalleryFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);

    PrepareNormalFormInfoReq(reqBody);
    errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    service->UpdateGalleryFormInfo(data, reply);
    isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t changedRows = resp.GetErrCode();
    EXPECT_GT(changedRows, 0);
    MEDIA_INFO_LOG("End Update_Gallery_Form_Info_Test_001");
}

HWTEST_F(FormInfoTest, Update_Gallery_Form_Info_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start Update_Gallery_Form_Info_Test_002");
    FormInfoReqBody reqBody;
    PrepareNormalFormInfoReq(reqBody);
    MessageParcel data;
    MessageParcel reply;
    bool errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    auto service = make_shared<MediaAssetsControllerService>();
    service->SaveGalleryFormInfo(data, reply);
    IPC::MediaEmptyObjVo respVo;
    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t formId = resp.GetErrCode();
    EXPECT_GT(formId, 0);

    vector<string> fileUris;
    reqBody.fileUris = fileUris;
    errConn = !reqBody.Marshalling(data);
    ASSERT_EQ(errConn, false);
    service->UpdateGalleryFormInfo(data, reply);
    isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    int32_t changedRows = resp.GetErrCode();
    EXPECT_EQ(changedRows, E_GET_PRAMS_FAIL);
    MEDIA_INFO_LOG("End Update_Gallery_Form_Info_Test_002");
}
}  // namespace OHOS::Media