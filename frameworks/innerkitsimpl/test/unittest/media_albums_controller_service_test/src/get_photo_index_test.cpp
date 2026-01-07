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
 
#include "get_photo_index_test.h"
 
#include <string>
#include <vector>
 
#define private public
#define protected public
#include "datashare_result_set.h"
#include "media_albums_controller_service.h"
#include "media_assets_service.h"
#include "rdb_utils.h"
#undef private
#undef protected

#include "vision_db_sqls.h"
#include "story_db_sqls.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "result_set_utils.h"
#include "get_photo_index_vo.h"
#include "query_result_vo.h"
#include "media_file_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    PhotoColumn::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_TAB_ANALYSIS_TOTAL,
    CREATE_USER_PHOTOGRAPHY_INFO_TABLE,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
    CREATE_ANALYSIS_ALBUM_MAP,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    VISION_TOTAL_TABLE,
    USER_PHOTOGRAPHY_INFO_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
};

void GetPhotoIndexTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("GetPhotoIndexTest SetUpTestCase");
}
 
void GetPhotoIndexTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("GetPhotoIndexTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}
 
void GetPhotoIndexTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("GetPhotoIndexTest SetUp");
}
 
void GetPhotoIndexTest::TearDown(void) {}
 
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

// ANALYSIS_PHOTO_MAP_TABLE
static int InsertAnalysisPhotoMap(int mediaId)
{
    int ret = g_rdbStore->ExecuteSql(
        "insert into AnalysisPhotoMap(map_album, map_asset) values(" + std::to_string(mediaId) + ", 1000012)");
    return ret;
}

// PhotoAlbumColumns::TABLE
static int InsertPhotoAlbum()
{
    int ret = g_rdbStore->ExecuteSql(
        "insert into PhotoAlbum(album_id, album_type, album_subtype) values(1, 1024, 1025)");
    return ret;
}

HWTEST_F(GetPhotoIndexTest, GetPhotoIndexTest_Test_001, TestSize.Level0)
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
    EXPECT_EQ(InsertAnalysisPhotoMap(GetInt32Val(MediaColumn::MEDIA_ID, resSet)), NativeRdb::E_OK) ;
    RdbPredicates analysisPhotoMapPredicates(ANALYSIS_PHOTO_MAP_TABLE);
    columns = { "map_album" };
    resSet = MediaLibraryRdbStore::Query(analysisPhotoMapPredicates, columns);
    int rowsCount = 0;
    while (resSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string val = "";
        resSet->GetString(0, val);
        std::cout << "map_album = " << val << std::endl;
        ++rowsCount;
    }
    std::cout << "analysisPhotoMap rowsCount = " << rowsCount << std::endl;

    EXPECT_EQ(InsertPhotoAlbum(), NativeRdb::E_OK);
    RdbPredicates photoAlbumPredicates(PhotoAlbumColumns::TABLE);
    columns = { "album_id", "album_type" };
    resSet = MediaLibraryRdbStore::Query(photoAlbumPredicates, columns);
    EXPECT_NE(resSet, nullptr);
    EXPECT_EQ(resSet->GoToNextRow(), NativeRdb::E_OK);
    std::string album_id = to_string(GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resSet));
    
    GetPhotoIndexReqBody reqBody;
    DataShare::DataSharePredicates dataSharePredicates;
    reqBody.predicates = dataSharePredicates;
    reqBody.photoId = photoId;
    reqBody.albumId = album_id;
    reqBody.isAnalysisAlbum = false;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    auto service = make_shared<MediaAlbumsControllerService>();
    service->GetPhotoIndex(data, reply);
    IPC::MediaRespVo<QueryResultRespBody> resp;
    bool isValid = resp.Unmarshalling(reply);
    ASSERT_EQ(isValid, true);
    EXPECT_NE(resp.GetErrCode(), E_SUCCESS);
}
 
}  // namespace OHOS::Media