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
 
#include "request_image_test.h"
 
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
 
#include "user_define_ipc_client.h"
#include "media_file_utils.h"
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
 
void RequestImageTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start RequestImageTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}
 
void RequestImageTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}
 
void RequestImageTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}
 
void RequestImageTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}
 
static void InsertAssetIntoPhotosTable(const std::string &filePath, const std::string &photoId, int photoQuality)
{
    const std::string sqlInsertPhoto = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "("  +
        MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
        MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
        MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
        MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
        MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
        PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " + PhotoColumn::PHOTO_ID + ", " +
        PhotoColumn::PHOTO_QUALITY + ")";
    g_rdbStore->ExecuteSql(sqlInsertPhoto + "VALUES (" +
        "'" + filePath + "', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1', '" + photoId  + "', " + std::to_string(photoQuality) + ")"); // cam, pic, shootingmode = 1
}
 
HWTEST_F(RequestImageTest, QueryPhotoStatus_Test_001, TestSize.Level0)
{
    bool createDirRes = MediaFileUtils::CreateDirectory("/data/local/tmp");
    EXPECT_EQ(createDirRes, true);
    std::string filePath = "/data/local/tmp/IMG_1501924305_001.jpg";
    if (!MediaFileUtils::IsFileExists(filePath)) {
        bool createFileRes = MediaFileUtils::CreateFile(filePath);
        EXPECT_EQ(createFileRes, true);
    }
    const std::string photoId = "photo_id_test";
    int photoQuality = 1;
    InsertAssetIntoPhotosTable(filePath, photoId, photoQuality);
 
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_QUALITY };
 
    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
 
    QueryPhotoReqBody reqBody;
    int fileId = GetInt32Val(MediaColumn::MEDIA_ID, resSet);
    EXPECT_GT(fileId, 0);
    reqBody.fileId = std::to_string(fileId);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
 
    auto service = make_shared<MediaAssetsControllerService>();
    service->QueryPhotoStatus(data, reply);
    IPC::MediaRespVo<QueryPhotoRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetBody().photoId, photoId);
    EXPECT_EQ(resp.GetBody().photoQuality, photoQuality);
}

HWTEST_F(RequestImageTest, LogMovingPhoto_TEST_001, TestSize.Level1)
{
    AdaptedReqBody reqBody;
    reqBody.adapted = 1;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<MediaAssetsControllerService>();
    service->LogMovingPhoto(data, reply);
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_EQ(resp.GetErrCode(), E_SUCCESS);
}
 
}  // namespace OHOS::Media