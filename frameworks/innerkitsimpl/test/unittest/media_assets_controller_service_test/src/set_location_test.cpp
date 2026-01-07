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

#include "set_location_test.h"

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
#include "media_file_utils.h"
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

void SetLocationTest::SetUpTestCase(void)
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

void SetLocationTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void SetLocationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void SetLocationTest::TearDown(void)
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

static void InsertAsset()
{
    std::string insertSql = R"S(INSERT INTO Photos(data, size, title, display_name, media_type, position, is_temp,
        time_pending, hidden, date_trashed, transcode_time, trans_code_file_size, exist_compatible_duplicate)
        VALUES ('/storage/cloud/files/Photo/665/test.heic', 7879, 'test',
        'test.heic', 1, 0, 0, 0, 0, 0, 1501838589870, 348113, 1))S";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

HWTEST_F(SetLocationTest, SetLocationTest_Test_001, TestSize.Level0)
{
    bool createDirRes = MediaFileUtils::CreateDirectory("/data/local/tmp");
    EXPECT_EQ(createDirRes, true);
    std::string filePath = "/data/local/tmp/IMG_1501924305_001.jpg";
    if (!MediaFileUtils::IsFileExists(filePath)) {
        bool createFileRes = MediaFileUtils::CreateFile(filePath);
        EXPECT_EQ(createFileRes, true);
    }
    InsertAssetIntoPhotosTable(filePath);

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns = { PhotoColumn::PHOTO_LATITUDE, PhotoColumn::PHOTO_LONGITUDE,
        MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};

    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
    std::string mediaFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resSet);
    EXPECT_EQ(mediaFilePath, filePath);

    AssetChangeReqBody reqBody;
    reqBody.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resSet);
    EXPECT_GT(reqBody.fileId, 0);
    reqBody.path = filePath;
    reqBody.latitude = 10;
    reqBody.longitude = 10;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);

    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeSetLocation(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_OK);

    resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);

    double latitude = GetDoubleVal(PhotoColumn::PHOTO_LATITUDE, resSet);
    double longitude = GetDoubleVal(PhotoColumn::PHOTO_LONGITUDE, resSet);
    EXPECT_EQ(latitude, reqBody.latitude);
    EXPECT_EQ(longitude, reqBody.longitude);
}

HWTEST_F(SetLocationTest, SetLocationTest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetLocationTest_Test_002");
    system("mkdir -p /storage/cloud/files/Photo/665/");
    system("touch /storage/cloud/files/Photo/665/test.heic");
    system("mkdir -p /storage/cloud/files/.editData/Photo/665/test.heic");
    system("touch /storage/cloud/files/.editData/Photo/665/test.heic/transcode.jpg");
    InsertAsset();
    std::string filePath = "/storage/cloud/files/Photo/665/test.heic";
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns = { PhotoColumn::PHOTO_LATITUDE, PhotoColumn::PHOTO_LONGITUDE,
        MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_TRANSCODE_TIME,
        PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE };
    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
    std::string mediaFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resSet);
    EXPECT_EQ(mediaFilePath, filePath);
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resSet);
    ASSERT_GT(fileId, 0);

    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    EXPECT_GT(reqBody.fileId, 0);
    reqBody.path = filePath;
    reqBody.latitude = 10;
    reqBody.longitude = 10;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeSetLocation(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_OK);
    resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
    double latitude = GetDoubleVal(PhotoColumn::PHOTO_LATITUDE, resSet);
    double longitude = GetDoubleVal(PhotoColumn::PHOTO_LONGITUDE, resSet);
    EXPECT_EQ(latitude, reqBody.latitude);
    EXPECT_EQ(longitude, reqBody.longitude);

    system("rm -rf /storage/cloud/files/Photo/665/test.heic");
    system("rm -rf /storage/cloud/files/.editData/Photo/665/test.heic/transcode.jpg");
}
}  // namespace OHOS::Media