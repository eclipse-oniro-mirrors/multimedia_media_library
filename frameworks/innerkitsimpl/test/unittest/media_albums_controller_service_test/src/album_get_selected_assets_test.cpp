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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "album_get_selected_assets_test.h"

#include <string>
#include <vector>

#include "media_albums_controller_service.h"
#include "album_get_selected_assets_vo.h"
#include "album_get_selected_assets_dto.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "vision_db_sqls_more.h"
#include "vision_db_sqls.h"
#include "medialibrary_data_manager.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::vector<std::string> createTableSqlLists = {
    // PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_TAB_IMAGE_FACE,
    CREATE_TAB_ANALYSIS_AFFECTIVE,
};

static std::vector<std::string> testTables = {
    // PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    VISION_IMAGE_FACE_TABLE,
    VISION_AFFECTIVE_TABLE,
};

void AlbumGetSelectedAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("AlbumGetSelectedAssetsTest SetUpTestCase succeed");
}

void AlbumGetSelectedAssetsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, false);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumGetSelectedAssetsTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AlbumGetSelectedAssetsTest::SetUp()
{
    MEDIA_INFO_LOG("AlbumGetSelectedAssetsTest SetUp");
}

void AlbumGetSelectedAssetsTest::TearDown(void)
{
    MEDIA_INFO_LOG("AlbumGetSelectedAssetsTest TearDown");
}

static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_POSITION + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " +
    PhotoColumn::PHOTO_OWNER_ALBUM_ID + ", " + PhotoColumn::PHOTO_SYNC_STATUS +
    ", clean_flag, hidden, time_pending, is_temp, burst_cover_level)";

static void InsertAssetIntoPhotosTable(const string &data, const string &title, int32_t albumId)
{
    MEDIA_INFO_LOG("InsertAssetIntoPhotosTable");
    // data, size, title, display_name, media_type,position
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, position, shooting_mode, owner_album_id, sync_status, clean_flag, hidden, time_pending,
    // is_temp, burst_cover_level
    g_rdbStore->ExecuteSql(
        SQL_INSERT_PHOTO + "VALUES ('" + data + "', 175258, '" + title + "', '" + title +
        ".jpg', 1, 'com.ohos.camera', '相机', 1748423617814, 1748424146785, 1748424146785, 0, 0, 0, 0, " +
        "1280, 960, 0, 1, '1', " + to_string(albumId) + ", 0, 0, 0, 0, false, 1)");
}

static shared_ptr<NativeRdb::ResultSet> QueryAsset(
    const string &table, const string &key, const string &value, const vector<string> &columns)
{
    MEDIA_INFO_LOG("QueryAsset");
    RdbPredicates rdbPredicates(table);
    rdbPredicates.EqualTo(key, value);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file asset");
        return nullptr;
    }
    return resultSet;
}

static bool ExecutionSql(const std::string &sql)
{
    int32_t ret = g_rdbStore->ExecuteSql(sql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", sql.c_str());
        return false;
    }
    MEDIA_INFO_LOG("Execute sql %{public}s success", sql.c_str());
    return true;
}

static bool InsertAnalysisAlbum(string albumName, string groupTag)
{
    // std::string insertSql = "INSERT INTO AnalysisAlbum (album_name, album_subtype, group_tag) VALUES "
    //                         "('"+ albumName + "', 4096, '" + groupTag + "')";
    std::string insertSql = "INSERT INTO AnalysisAlbum (album_name, album_subtype, group_tag, tag_id) "
        "VALUES ('test01', 4102, 'ser_1755057945560169000', 'ser_1755057945560169000')";
    return ExecutionSql(insertSql);
}

static bool InsertAnalysisPhotoMap(int mediaId, int fileId)
{
    MEDIA_INFO_LOG("InsertAnalysisPhotoMap");
    std::string insertSql = "insert into AnalysisPhotoMap(map_album, map_asset) values(" + std::to_string(mediaId) +
                            ", " + std::to_string(fileId) + ")";
    return ExecutionSql(insertSql);
}

static bool InsertAnalysisImageFace(int fileId, string tagID)
{
    MEDIA_INFO_LOG("InsertAnalysisImageFace");
    std::string insertSql =
        "insert into tab_analysis_image_face(file_id, tag_id, aesthetics_score, is_excluded) values(" +
        std::to_string(fileId) + ", '" + tagID + "', 60, 4095)";
    return ExecutionSql(insertSql);
}

static bool InsertTabAnalysisVlm(int fileId)
{
    MEDIA_INFO_LOG("InsertTabAnalysisVlm");
    std::string insertSql = "INSERT INTO tab_analysis_affective "
                            "(file_id, valence, arousal) "
                            "VALUES(" +
                            std::to_string(fileId) + ", 0, 9)";
    return ExecutionSql(insertSql);
}

static int32_t QueryAlbumIdByAlbumName(const string &albumName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(ANALYSIS_ALBUM_TABLE);
    rdbPredicates.EqualTo("album_name", albumName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return -1;
    }
    int32_t albumId = GetInt32Val("album_id", resultSet);
    return albumId;
}

static bool AlbumGetSelectedAssetsPrepare(int32_t &albumId)
{
    string groupTag = "ser_1755057945560169000";
    string albumName = "test01";
    bool ret = InsertAnalysisAlbum(albumName, groupTag);
    if (!ret) {
        return false;
    }

    vector<string> columns;
    auto resultSet = QueryAsset("AnalysisAlbum", PhotoAlbumColumns::ALBUM_NAME, albumName, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryAsset fail");
        return false;
    }

    albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    if (albumId <= 0) {
        return false;
    }
    MEDIA_INFO_LOG("albumId = %{public}d", albumId);

    string title = "cam_pic";
    string data = "/storage/cloud/files/Photo/9/IMG_1748505946_009.jpg";
    InsertAssetIntoPhotosTable(data, title, albumId);
    resultSet = QueryAsset(PhotoColumn::PHOTOS_TABLE, MediaColumn::MEDIA_NAME, title + ".jpg", columns);
    if (resultSet == nullptr) {
        return false;
    }

    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    if (displayName.size() <= 0) {
        return false;
    }

    int32_t fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    MEDIA_INFO_LOG("fileId = %{public}d", fileId);
    ret = InsertAnalysisPhotoMap(albumId, fileId);
    if (!ret) {
        return false;
    }

    ret = InsertAnalysisImageFace(fileId, groupTag);
    if (!ret) {
        return false;
    }

    ret = InsertTabAnalysisVlm(fileId);
    if (!ret) {
        return false;
    }
    return true;
}

static int32_t AlbumGetSelectedAssets(int32_t albumId)
{
    AlbumGetSelectedAssetsReqBody reqBody;
    reqBody.albumId = albumId;
    std::vector<std::string> cols;
    cols.push_back("display_name");
    reqBody.columns = cols;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    auto service = make_shared<MediaAlbumsControllerService>();
    MessageParcel reply;
    service->AlbumGetSelectAssets(data, reply);

    IPC::MediaRespVo<AlbumGetSelectedAssetsRespBody> resp;
    if (resp.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("resp.Unmarshalling failed");
        return -1;
    }
    auto err = resp.GetErrCode();
    if (err != E_OK) {
        MEDIA_ERR_LOG("resp.GetErrCode is not E_OK");
        return -1;
    }
    AlbumGetSelectedAssetsRespBody respBody = resp.GetBody();
    if (respBody.resultSet == nullptr) {
        MEDIA_ERR_LOG("respBody.resultSet is nullptr");
        return -1;
    }
    int count = 0;

    auto errCode = respBody.resultSet->GetRowCount(count);
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ResultSet GetRowCount failed, errCode=%{public}d", errCode);
        return count;
    }
    return count;
}

HWTEST_F(AlbumGetSelectedAssetsTest, GetSelectedAssets_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start GetSelectedAssets_Test_001");
    // prepare
    int32_t albumId = -1;
    bool ret = AlbumGetSelectedAssetsPrepare(albumId);
    EXPECT_EQ(ret, true);
    EXPECT_GT(albumId, 0);

    // sucess
    int32_t count = AlbumGetSelectedAssets(albumId);
    EXPECT_GT(count, 0);

    // invalid
    int invalidId = albumId + 1;
    count = AlbumGetSelectedAssets(invalidId);
    EXPECT_EQ(count, 0);

    MEDIA_INFO_LOG("end GetSelectedAssets_Test_001");
}

HWTEST_F(AlbumGetSelectedAssetsTest, GetSelectedAssets_Test_002, TestSize.Level0)
{
    MessageParcel data;
    auto service = make_shared<MediaAlbumsControllerService>();
    MessageParcel reply;
    int32_t ret = service->AlbumGetSelectAssets(data, reply);
    EXPECT_EQ(ret, E_IPC_SEVICE_UNMARSHALLING_FAIL);
}

HWTEST_F(AlbumGetSelectedAssetsTest, GetSelectedAssets_Test_003, TestSize.Level0)
{
    AlbumGetSelectedAssetsReqBody reqBody;
    reqBody.albumId = 0;
    std::vector<std::string> cols;
    cols.push_back("display_name");
    reqBody.columns = cols;
    std::string invaildWhereClause = "WHERE Photos.sync_status = 0; Photos.clean_flag = 0";
    reqBody.predicates.SetWhereClause(invaildWhereClause);
    MessageParcel data;
    bool ret1 = reqBody.Marshalling(data);
    EXPECT_EQ(ret1, true);

    auto service = make_shared<MediaAlbumsControllerService>();
    MessageParcel reply;
    int32_t ret2 = service->AlbumGetSelectAssets(data, reply);
    EXPECT_EQ(ret2, E_INVALID_VALUES);
}
}  // namespace OHOS::Media