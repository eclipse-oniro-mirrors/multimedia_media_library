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

#include "set_title_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "datashare_result_set.h"
#include "media_assets_controller_service.h"
#include "media_assets_service.h"
#include "rdb_utils.h"
#undef private
#undef protected

#include "create_asset_vo.h"
#include "medialibrary_errno.h"
#include "modify_assets_vo.h"
#include "parameter_utils.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void SetTitleTest::SetUpTestCase(void)
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

void SetTitleTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void SetTitleTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void SetTitleTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

static void InsertAssetIntoPhotosTable(const std::string &filePath)
{
    const std::string sqlInsertPhoto = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "("  +
        MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
        MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
        MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
        MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
        PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
    // file_id,
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    g_rdbStore->ExecuteSql(sqlInsertPhoto + "VALUES (" +
        "'" + filePath + "', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1' )"); // cam, pic, shootingmode = 1
}

HWTEST_F(SetTitleTest, SetTitleTest_Test_001, TestSize.Level0)
{
    std::string filePath = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";

    InsertAssetIntoPhotosTable(filePath);

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_TITLE };

    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);

    AssetChangeReqBody reqBody;
    reqBody.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resSet);
    reqBody.title = "title_test";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeSetTitle(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), 1); // 更新数量为1

    resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);

    const std::string &title = GetStringVal(MediaColumn::MEDIA_TITLE, resSet);
    EXPECT_EQ(title, reqBody.title);
}


// ---- ParameterUtils 标题校验（需求：允许中间 '.',禁止 '.' 开头与非法字符）----
HWTEST_F(SetTitleTest, CheckCreateAssetTitle_Empty_001, TestSize.Level1)
{
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle(""), E_OK);
}

HWTEST_F(SetTitleTest, CheckCreateAssetTitle_DotInMiddle_001, TestSize.Level1)
{
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("com.123.photo"), E_OK);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("test..name"), E_OK);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("my.photo.2026"), E_OK);
}

HWTEST_F(SetTitleTest, CheckCreateAssetTitle_LeadingDot_001, TestSize.Level1)
{
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle(".hidden"), E_INVALID_DISPLAY_NAME);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("."), E_INVALID_DISPLAY_NAME);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle(".."), E_INVALID_DISPLAY_NAME);
}

HWTEST_F(SetTitleTest, CheckCreateAssetTitle_IllegalChar_001, TestSize.Level1)
{
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("test'file"), E_INVALID_DISPLAY_NAME);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("test`file"), E_INVALID_DISPLAY_NAME);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("test{file}"), E_INVALID_DISPLAY_NAME);
    EXPECT_EQ(ParameterUtils::CheckCreateAssetTitle("test[file]"), E_INVALID_DISPLAY_NAME);
}

HWTEST_F(SetTitleTest, CheckSetAssetTitle_DotInMiddle_001, TestSize.Level1)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds = { 1 };
    reqBody.title = "com.123.photo";
    EXPECT_EQ(ParameterUtils::CheckSetAssetTitle(reqBody), E_OK);
}

HWTEST_F(SetTitleTest, CheckSetAssetTitle_LeadingDot_001, TestSize.Level1)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds = { 1 };
    reqBody.title = ".hidden";
    EXPECT_EQ(ParameterUtils::CheckSetAssetTitle(reqBody), E_INVALID_DISPLAY_NAME);
}

HWTEST_F(SetTitleTest, CheckSetAssetTitle_Empty_001, TestSize.Level1)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds = { 1 };
    EXPECT_EQ(ParameterUtils::CheckSetAssetTitle(reqBody), E_INVALID_DISPLAY_NAME);
}

}  // namespace OHOS::Media