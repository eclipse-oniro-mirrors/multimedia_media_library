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

#define MLOG_TAG "MediaAlbumControllerServiceTest"

#include "album_dismiss_assets_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "vision_db_sqls_more.h"
#include "change_request_dismiss_assets_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "vision_image_face_column.h"
#include "vision_photo_map_column.h"
#include "medialibrary_data_manager.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
struct PortraitData {
    int64_t fileId;
    string title;
    string path;
};

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_TAB_IMAGE_FACE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    VISION_IMAGE_FACE_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    ANALYSIS_ALBUM_TABLE,
};

void AlbumDismissAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("AlbumDismissAssetsTest SetUpTestCase done");
}

void AlbumDismissAssetsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("AlbumDismissAssetsTest TearDownTestCase");
}

void AlbumDismissAssetsTest::SetUp()
{
    MEDIA_INFO_LOG("AlbumDismissAssetsTest SetUp");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void AlbumDismissAssetsTest::TearDown(void) {}

PortraitData InsertPortraitToPhotos(size_t val)
{
    int64_t fileId = -1;
    int64_t timestamp = 1752000000000;
    string title = "IMG_000";
    string displayName = title + ".jpg";
    string data = "/storage/cloud/files/photo/" + to_string(val) + "/" + title + ".jpg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);
    valuesBucket.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, 1);
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    PortraitData portraitData;
    portraitData.fileId = fileId;
    portraitData.title = title;
    portraitData.path = data;
    return portraitData;
}

void InsertGroupPhotoToImageFace(int64_t fileId, const vector<string> &tagIds)
{
    int64_t rowId = -1;
    size_t totalFaces = tagIds.size();
    for (size_t faceId = 0; faceId < totalFaces; faceId++) {
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(MediaColumn::MEDIA_ID, fileId);
        valuesBucket.PutInt(FACE_ID, faceId);
        valuesBucket.PutInt(TOTAL_FACES, totalFaces);
        valuesBucket.PutString(TAG_ID, tagIds[faceId]);
        EXPECT_NE((g_rdbStore == nullptr), true);
        int32_t ret = g_rdbStore->Insert(rowId, VISION_IMAGE_FACE_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
    }
}

vector<PortraitData> PrepareGroupPhotoData(const vector<string> &tagIds)
{
    const int imageCount = 5;
    vector<PortraitData> portraits;
    for (size_t index = 0; index < imageCount; index++) {
        PortraitData portrait = InsertPortraitToPhotos(index);
        portraits.push_back(portrait);
        InsertGroupPhotoToImageFace(portrait.fileId, tagIds);
    }
    return portraits;
}

void InsertPortraitsToAlbum(const vector<PortraitData> &portraitData, int64_t albumId, int64_t coverIndex,
    CoverSatisfiedType coverSatisfiedType)
{
    int64_t rowId = -1;
    for (PortraitData data : portraitData) {
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(MAP_ALBUM, albumId);
        valuesBucket.PutInt(MAP_ASSET, data.fileId);
        EXPECT_NE((g_rdbStore == nullptr), true);
        int32_t ret = g_rdbStore->Insert(rowId, ANALYSIS_PHOTO_MAP_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
    }

    int32_t changedRows = -1;
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutString(COVER_URI, "coverUri");
    updateValues.PutInt(IS_COVER_SATISFIED, static_cast<int>(coverSatisfiedType));
    cmd.SetValueBucket(updateValues);
    cmd.GetAbsRdbPredicates()->EqualTo(ALBUM_ID, to_string(albumId));
    int32_t ret = g_rdbStore->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
}

int64_t CreateSmartAlbum(const string &tagId,
    const PhotoAlbumSubType &subtype = PhotoAlbumSubType::PORTRAIT)
{
    int64_t albumId = -1;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.PutInt(ALBUM_SUBTYPE, subtype);
    valuesBucket.PutString(TAG_ID, tagId);
    valuesBucket.PutString(GROUP_TAG, tagId);
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertAlbum albumId is %{public}s", to_string(albumId).c_str());
    return albumId;
}

int32_t CreatAssetsAndAlbumsForGroupPhoto(vector<PortraitData> &portraits)
{
    const string groupTag = "ser_1711000000000000000,ser_1711000000000000001";
    const vector<string> tagIds = {"ser_1711000000000000000", "ser_1711000000000000001"};
    portraits = PrepareGroupPhotoData(tagIds);
    int64_t portraitAlbumId1 = CreateSmartAlbum(tagIds[0]);
    int64_t portraitAlbumId2 = CreateSmartAlbum(tagIds[1]);
    int64_t albumId = CreateSmartAlbum(groupTag, PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_GT(portraitAlbumId1, 0);
    EXPECT_GT(portraitAlbumId2, 0);
    EXPECT_GT(albumId, 0);

    InsertPortraitsToAlbum(portraits, portraitAlbumId1, 1, CoverSatisfiedType::DEFAULT_SETTING);
    InsertPortraitsToAlbum(portraits, portraitAlbumId2, 1, CoverSatisfiedType::DEFAULT_SETTING);
    InsertPortraitsToAlbum(portraits, albumId, 1, CoverSatisfiedType::DEFAULT_SETTING);
    return albumId;
}

int GetGroupPhotoAssetsCount(int32_t albumId)
{
    string querySql = "SELECT DISTINCT Photos.file_id ";
    querySql += "FROM Photos ";
    querySql += "INNER JOIN AnalysisPhotoMap ON (file_id = map_asset) ";
    querySql += "INNER JOIN AnalysisAlbum ON (album_id = map_album) ";
    querySql += "INNER JOIN (SELECT group_tag FROM AnalysisAlbum WHERE album_id = " + std::to_string(albumId) + ") ag ";
    querySql += "ON ag.group_tag = AnalysisAlbum.group_tag ";
    querySql += "WHERE Photos.sync_status = 0 ";
    querySql += "AND Photos.clean_flag = 0 ";
    querySql += "AND date_trashed = 0 ";
    querySql += "AND hidden = 0 ";
    querySql += "AND time_pending = 0 ";
    querySql += "AND is_temp = 0 ";
    querySql += "AND is_temp = 0 ";
    querySql += "AND burst_cover_level = 1 ";

    EXPECT_NE(g_rdbStore, nullptr);
    auto resultSet = g_rdbStore->QuerySql(querySql);
    EXPECT_NE(resultSet, nullptr);

    int32_t resultSetCount = -1;
    int32_t ret = resultSet->GetRowCount(resultSetCount);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    EXPECT_GE(resultSetCount, 0);
    return resultSetCount;
}

HWTEST_F(AlbumDismissAssetsTest, DismissAsstes_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("DismissAsstes_Test_001 Start");
    vector<PortraitData> poraitData;
    int32_t albumId = CreatAssetsAndAlbumsForGroupPhoto(poraitData);
    size_t photoCount = poraitData.size();
    EXPECT_EQ(photoCount, 5);
    MEDIA_INFO_LOG("Query albums and check result");
    int count = GetGroupPhotoAssetsCount(albumId);
    EXPECT_EQ(count, 5);

    MessageParcel data;
    MessageParcel reply;
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    reqBody.assets = {std::to_string(poraitData[0].fileId)};
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
    }
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->DismissAssets(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_GT(resp.GetErrCode(), 0);
    count = GetGroupPhotoAssetsCount(albumId);
    EXPECT_EQ(count, 4);
    MEDIA_INFO_LOG("DismissAsstes_Test_001 End");
}

HWTEST_F(AlbumDismissAssetsTest, DismissAsstes_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("DismissAsstes_Test_002 Start");
    vector<PortraitData> poraitData;
    int32_t albumId = CreatAssetsAndAlbumsForGroupPhoto(poraitData);
    size_t photoCount = poraitData.size();
    EXPECT_EQ(photoCount, 5);
    MEDIA_INFO_LOG("Query albums and check result");
    int count = GetGroupPhotoAssetsCount(albumId);
    EXPECT_EQ(count, 5);

    MessageParcel data;
    MessageParcel reply;
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    reqBody.assets = {std::to_string(poraitData[0].fileId),
    std::to_string(poraitData[1].fileId),
    std::to_string(poraitData[2].fileId),
    std::to_string(poraitData[3].fileId),
    std::to_string(poraitData[4].fileId)};
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->DismissAssets(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> resp;
    ASSERT_EQ(resp.Unmarshalling(reply), true);
    ASSERT_GT(resp.GetErrCode(), 0);
    count = GetGroupPhotoAssetsCount(albumId);
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("DismissAsstes_Test_002 End");
}
}  // namespace OHOS::Media