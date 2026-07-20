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

#include "delete_asset_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "delete_photos_vo.h"
#include "trash_photos_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "medialibrary_asset_operations.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
static const string VALUES_END = ") ";

static const string SQL_INSERT_BURST_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " +
    PhotoColumn::PHOTO_BURST_COVER_LEVEL + ", " + PhotoColumn::PHOTO_BURST_KEY + ", " +
    PhotoColumn::PHOTO_SUBTYPE +
    ")";

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

void DeleteAssetTest::SetUpTestCase(void)
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

void DeleteAssetTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void DeleteAssetTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("SetUp");
}

void DeleteAssetTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

int32_t PublicDeleteAsset(std::vector<std::string> &testUris)
{
    TrashPhotosReqBody reqBody;
    reqBody.uris = testUris;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->TrashPhotos(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("Public ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

int32_t SystemDeleteAsset(std::vector<std::string> &testUris)
{
    TrashPhotosReqBody reqBody;
    reqBody.uris = testUris;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SysTrashPhotos(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("System ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static void InsertAsset()
{
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    std::string insertSql =
        SQL_INSERT_PHOTO + " VALUES (" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " + "1280, 960, 0, '1'" + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static void InsertBurstAsset()
{
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode, burst_cover_level, burst_key, subtype
    std::string insertSql = SQL_INSERT_BURST_PHOTO + " VALUES (" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic_burst_cover', " +
        "'cam_pic_burst_cover.jpg', 1, 'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1', 1, 'c628b4a3-828e-4f05-9781-487bf4de7f73', 4), " +
        "('/storage/cloud/files/Photo/16/IMG_1501924305_001.jpg', 175258, 'cam_pic_burst_001', " +
        "'cam_pic_burst_001.jpg', 1, 'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1', 2, 'c628b4a3-828e-4f05-9781-487bf4de7f73', 4)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static void InsertAssetSystem()
{
    std::string insertSql =
        SQL_INSERT_PHOTO + " VALUES (" +
        "'/storage/cloud/files/Photo/20/IMG_1501924306_222.jpg', 175259, 'system_cam_pic', 'system_cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205219, 0, 1501924206, 0, 0, 0, 0, " + "1280, 960, 0, '1'" + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t QueryPhotoIdByDisplayName(const string &displayName)
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

HWTEST_F(DeleteAssetTest, PublicDeleteAsset_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("PublicDeleteAsset_Test_001 Begin");
    std::vector<std::string> testUris = {"file://media/Invalid/200"};
    int32_t result = PublicDeleteAsset(testUris);
    ASSERT_LE(result, 0);

    testUris = {"file://media/Photo/200"};
    result = PublicDeleteAsset(testUris);
    ASSERT_LE(result, 0);
}

HWTEST_F(DeleteAssetTest, PublicDeleteAsset_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("PublicDeleteAsset_Test_002 Begin");
    InsertAsset();
    int32_t fileId = QueryPhotoIdByDisplayName("cam_pic.jpg");
    MEDIA_INFO_LOG("PublicDeleteAsset_Test_002 fileId = %{public}d", fileId);
    ASSERT_GT(fileId, 0);

    auto uri = "file://media/Photo/" + to_string(fileId);
    std::vector<std::string> testUris = {uri};
    int32_t result = PublicDeleteAsset(testUris);
    ASSERT_GT(result, 0);
}

HWTEST_F(DeleteAssetTest, SystemDeleteAsset_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("SystemDeleteAsset_Test_001 Begin");
    std::vector<std::string> testUris = {"file://media/Invalid/200"};
    int32_t result = SystemDeleteAsset(testUris);
    ASSERT_LE(result, 0);

    testUris = {"file://media/Photo/200"};
    result = SystemDeleteAsset(testUris);
    ASSERT_LE(result, 0);
}

HWTEST_F(DeleteAssetTest, SystemDeleteAsset_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("SystemDeleteAsset_Test_002 Begin");
    InsertAssetSystem();
    int32_t fileId = QueryPhotoIdByDisplayName("system_cam_pic.jpg");
    MEDIA_INFO_LOG("SystemDeleteAsset_Test_003 new system fileId = %{public}d", fileId);
    ASSERT_GT(fileId, 0);

    auto uri = "file://media/Photo/" + to_string(fileId);
    std::vector<std::string> testUris = {uri};
    int32_t result = SystemDeleteAsset(testUris);
    ASSERT_GT(result, 0);
}
}  // namespace OHOS::Media