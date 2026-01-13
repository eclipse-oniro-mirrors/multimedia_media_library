/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileExtUnitTest"

#include "medialibrary_datamanager_test.h"
#include "fetch_result.h"
#include "get_self_permissions.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_smart_map_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_uripermission_operations.h"
#include "uri.h"
#include "vision_db_sqls.h"
#include "vision_db_sqls_more.h"
#include "medialibrary_db_const_sqls.h"
#include "vision_column.h"
#include "photo_album_column.h"
#include "vision_face_tag_column.h"
#include "medialibrary_unistore_manager.h"
#include "vision_total_column.h"
#include "vision_image_face_column.h"
#include "result_set_utils.h"
#include "photo_map_column.h"
#include "power_efficiency_manager.h"
#include "vision_photo_map_column.h"
#define private public
#include "medialibrary_data_manager.h"
#include "photo_day_month_year_operation.h"
#include "albums_refresh_manager.h"
#undef private
#include "userfilemgr_uri.h"
#include "data_secondary_directory_uri.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static constexpr int32_t NOT_DISPLAY_FOR_BACKGROUND = -1;
static constexpr int32_t ALBUM_IS_REMOVED = 1;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_TAB_IMAGE_FACE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_MEDIA_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    VISION_IMAGE_FACE_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    ANALYSIS_ALBUM_TABLE,
    MEDIALIBRARY_TABLE,
};

namespace {
    shared_ptr<FileAsset> g_pictures = nullptr;
    shared_ptr<FileAsset> g_download = nullptr;
}

struct PortraitData {
    int64_t fileId;
    string title;
    string path;
};

bool MockIsChargingAndScreenOff()
{
    MEDIA_INFO_LOG("enter");
    return true;
}

void MediaLibraryDataManagerUnitTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryUnitTestUtils::CreateTestTables(rdbStore, createTableSqlLists);

    vector<string> perms;
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

void MediaLibraryDataManagerUnitTest::TearDownTestCase(void)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryUnitTestUtils::CleanTestTables(rdbStore, testTables);
    MediaLibraryUnitTestUtils::CleanTestFiles();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryDataManagerUnitTest::SetUp(void)
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    g_pictures = MediaLibraryUnitTestUtils::GetRootAsset(TEST_PICTURES);
    g_download = MediaLibraryUnitTestUtils::GetRootAsset(TEST_DOWNLOAD);
    MediaLibraryUnitTestUtils::Init();
}

void MediaLibraryDataManagerUnitTest::TearDown(void)
{
    isChargingAndScreenOffPtr = PowerEfficiencyManager::IsChargingAndScreenOff;
}

string ReturnUri(string UriType, string MainUri, string SubUri = "")
{
    if (SubUri.empty()) {
        return (UriType + "/" + MainUri);
    } else {
        return (UriType + "/" + MainUri + "/" + SubUri);
    }
}

PortraitData InsertPortraitToPhotos(size_t val)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
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
    EXPECT_NE((rdbStore == nullptr), true);
    int32_t ret = rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
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
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int64_t rowId = -1;
    size_t totalFaces = tagIds.size();
    for (size_t faceId = 0; faceId < totalFaces; faceId++) {
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(MediaColumn::MEDIA_ID, fileId);
        valuesBucket.PutInt(FACE_ID, faceId);
        valuesBucket.PutInt(TOTAL_FACES, totalFaces);
        valuesBucket.PutString(TAG_ID, tagIds[faceId]);
        EXPECT_NE((rdbStore == nullptr), true);
        int32_t ret = rdbStore->Insert(rowId, VISION_IMAGE_FACE_TABLE, valuesBucket);
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
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int64_t rowId = -1;
    for (PortraitData data : portraitData) {
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(MAP_ALBUM, albumId);
        valuesBucket.PutInt(MAP_ASSET, data.fileId);
        EXPECT_NE((rdbStore == nullptr), true);
        int32_t ret = rdbStore->Insert(rowId, ANALYSIS_PHOTO_MAP_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
    }

    int32_t changedRows = -1;
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutString(COVER_URI, "coverUri");
    updateValues.PutInt(IS_COVER_SATISFIED, static_cast<int>(coverSatisfiedType));
    cmd.SetValueBucket(updateValues);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    int32_t ret = rdbStore->Update(cmd, changedRows);
    EXPECT_EQ(ret, E_OK);
}

int64_t CreateSmartAlbum(const string &tagId,
    const PhotoAlbumSubType &subtype = PhotoAlbumSubType::PORTRAIT)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int64_t albumId = -1;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, subtype);
    valuesBucket.PutString(TAG_ID, tagId);
    valuesBucket.PutString(GROUP_TAG, tagId);
    EXPECT_NE(rdbStore, nullptr);
    int32_t ret = rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertAlbum albumId is %{public}s", to_string(albumId).c_str());
    return albumId;
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CreateAsset_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CreateAsset_Test_001::Start");
    Uri createAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_FILEOPRN, MEDIA_FILEOPRN_CREATEASSET));
    DataShare::DataShareValuesBucket valuesBucket;
    string relativePath = "Pictures/";
    string displayName = "CreateAsset_Test_001.jpg";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    MediaLibraryCommand cmd(createAssetUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_CreateAsset_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CloseAsset_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CloseAsset_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("OpenFile_test_001.jpg", g_pictures, fileAsset), true);
    Uri closeAssetUri(URI_CLOSE_FILE);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_ID, fileAsset->GetId());
    MediaLibraryCommand cmd(closeAssetUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ(retVal, E_SUCCESS);
    MEDIA_INFO_LOG("DataManager_CloseAsset_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CreateAlbum_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CreateAlbum_Test_001::Start");
    Uri createAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN, MEDIA_ALBUMOPRN_CREATEALBUM));
    DataShare::DataShareValuesBucket valuesBucket;
    string dirPath = ROOT_MEDIA_DIR + "Pictures/CreateAlbum_Test_001/";
    valuesBucket.Put(MEDIA_DATA_DB_NAME, "CreateAlbum_Test_001");
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, dirPath);
    MediaLibraryCommand cmd(createAlbumUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_CreateAlbum_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CreateDir_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CreateDir_Test_001::Start");
    Uri createDirUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_DIROPRN, MEDIA_DIROPRN_FMS_CREATEDIR));
    DataShare::DataShareValuesBucket valuesBucket;
    string relativePath = "Pictures/CreateDir_Test_001/";
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    MediaLibraryCommand cmd(createDirUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_CreateDir_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashDir_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashDir_Test_001::Start");
    shared_ptr<FileAsset> albumAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateAlbum("TrashDir_Test_001", g_pictures, albumAsset), true);
    Uri trashDirUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_DIROPRN, MEDIA_DIROPRN_FMS_TRASHDIR));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_ID, albumAsset->GetId());
    MediaLibraryCommand cmd(trashDirUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_TrashDir_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Favorite_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Favorite_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("Favorite_Test_001.jpg", g_pictures, fileAsset), true);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, fileAsset->GetId());
    Uri addSmartAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_SMARTALBUMMAPOPRN,
        MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM));
    MediaLibraryCommand cmd(addSmartAlbumUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    Uri removeSmartAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_SMARTALBUMMAPOPRN,
        MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM));
    MediaLibraryCommand removeCmd(removeSmartAlbumUri);
    retVal = MediaLibraryDataManager::GetInstance()->Insert(removeCmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_Favorite_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Trash_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Trash_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("Trash_Test_001.jpg", g_pictures, fileAsset), true);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, TRASH_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, fileAsset->GetId());
    Uri addSmartAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_SMARTALBUMMAPOPRN,
        MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM));
    MediaLibraryCommand cmd(addSmartAlbumUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    Uri removeSmartAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_SMARTALBUMMAPOPRN,
        MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM));
    MediaLibraryCommand removeCmd(removeSmartAlbumUri);
    retVal = MediaLibraryDataManager::GetInstance()->Insert(removeCmd, valuesBucket);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_Trash_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_DeleteAsset_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_DeleteAsset_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("DeleteAsset_Test_001.jpg", g_pictures, fileAsset), true);
    Uri deleteAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_FILEOPRN, MEDIA_FILEOPRN_DELETEASSET));
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(fileAsset->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, E_SUCCESS);
    MEDIA_INFO_LOG("DataManager_DeleteAsset_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryDirTable_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryAlbum_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryAlbum_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN_QUERYALBUM));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryVolume_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryVolume_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_QUERYOPRN_QUERYVOLUME));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryFiles_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryFiles_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UpdateFileAsset_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UpdateFileAsset_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("UpdateFileAsset_Test_001.jpg", g_pictures, fileAsset), true);
    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, fileAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, fileAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "UpdateAsset_Test_001.jpg");
    valuesBucketUpdate.Put(MEDIA_DATA_DB_TITLE, "UpdateAsset_Test_001");
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, fileAsset->GetRelativePath());
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = " + to_string(fileAsset->GetId()));
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_FILEOPRN, MEDIA_FILEOPRN_MODIFYASSET));
    MediaLibraryCommand cmd(updateAssetUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_EQ((retVal > 0), true);
    MEDIA_INFO_LOG("DataManager_UpdateFileAsset_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UpdateAlbumAsset_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_001::Start");
    shared_ptr<FileAsset> albumAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateAlbum("UpdateAlbumAsset_Test_001", g_pictures, albumAsset), true);
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_ID + " = " + to_string(albumAsset->GetId());
    predicates.SetWhereClause(prefix);
    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, albumAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, albumAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, albumAsset->GetRelativePath());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "U" + albumAsset->GetDisplayName());
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_001::GetUri = %{public}s", albumAsset->GetUri().c_str());
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN, MEDIA_ALBUMOPRN_MODIFYALBUM));
    MediaLibraryCommand cmd(updateAssetUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Revert_Package_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Revert_Package_Test_001::Start");
    shared_ptr<FileAsset> fileAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateFile("Revert_Package_Test_001.jpg", g_pictures, fileAsset), true);

    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, fileAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, fileAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "Revert_Package_Test_001.jpg");
    valuesBucketUpdate.Put(MEDIA_DATA_DB_TITLE, "Revert_Package_Test_001");
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, fileAsset->GetRelativePath());
    int64_t time = MediaFileUtils::UTCTimeSeconds();
    valuesBucketUpdate.Put(MEDIA_DATA_DB_TIME_PENDING, to_string(time));
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = " + to_string(fileAsset->GetId()));
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_FILEOPRN, MEDIA_FILEOPRN_MODIFYASSET));
    MediaLibraryCommand cmd(updateAssetUri);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_GT(retVal, 0);

    string package = fileAsset->GetOwnerPackage();
    MEDIA_INFO_LOG("DataManager_Revert_Package_Test_001 package:%{publc}s", package.c_str());

    MediaLibraryDataManager::GetInstance()->RevertPendingByPackage(package);

    vector<string> columns;
    Uri queryUri(MEDIALIBRARY_DATA_URI);
    MediaLibraryCommand queryCmd(queryUri, OperationType::QUERY);
    int32_t errCode = 0;
    auto resultSetPtr = MediaLibraryDataManager::GetInstance()->QueryRdb(queryCmd, columns, predicates, errCode);
    EXPECT_NE(resultSetPtr, nullptr);
    int count = 0;
    EXPECT_EQ(resultSetPtr->GetRowCount(count), E_OK);

    auto fetchFileResult = make_shared<FetchResult<FileAsset>>();
    for (int32_t row = 0; row < count; row++) {
        unique_ptr<FileAsset> fileAssetObj = fetchFileResult->GetObjectFromRdb(resultSetPtr, row);
        EXPECT_NE(fileAssetObj, nullptr);
        EXPECT_EQ(fileAssetObj->GetTimePending(), 0);

        MediaLibraryCommand deleteCmd(queryUri, Media::OperationType::DELETE);
        int ret =  MediaLibraryDataManager::GetInstance()->Delete(deleteCmd, predicates);
        EXPECT_EQ(ret, E_FAIL);
    }
    MEDIA_INFO_LOG("DataManager_Revert_Package_Test_001::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_File_Test_001
 * @tc.name      : test trash and recovery file: normal case
 * @tc.desc      : 1.Create the parent dir: trashRecovery_File_001
 *                 2.Create file1 in trashRecovery_File_001: trashRecovery_File_001/file1
 *                 3.trash file1
 *                 4.recovery file1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_File_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_001::Start");
    shared_ptr<FileAsset> trashRecovery_File_001 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_001", g_download, trashRecovery_File_001));
    shared_ptr<FileAsset> file1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_001, file1));

    MediaLibraryUnitTestUtils::TrashFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), false);
    MediaLibraryUnitTestUtils::RecoveryFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_001::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_File_Test_002
 * @tc.name      : test trash and recovery file: there is the same name file in file system when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_File_002
 *                 2.Create file1 in trashRecovery_File_002: trashRecovery_File_002/file1
 *                 3.trash file1
 *                 4.recreate file1
 *                 5.recovery file1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_File_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_002::Start");
    shared_ptr<FileAsset> trashRecovery_File_002 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_002", g_download, trashRecovery_File_002));
    shared_ptr<FileAsset> file1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_002, file1));

    MediaLibraryUnitTestUtils::TrashFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), false);
    shared_ptr<FileAsset> sameFile = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_002, sameFile));
    MediaLibraryUnitTestUtils::RecoveryFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_002::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_File_Test_003
 * @tc.name      : test trash and recovery file: the parent dir is not existed in db(is_trash != 0) when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_File_003
 *                 2.Create file1 in trashRecovery_File_003: trashRecovery_File_003/file1
 *                 3.trash file1
 *                 4.trash parent dir trashRecovery_File_003
 *                 5.recovery file1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_File_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_003::Start");
    shared_ptr<FileAsset> trashRecovery_File_003 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_003", g_download, trashRecovery_File_003));
    shared_ptr<FileAsset> file1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_003, file1));

    MediaLibraryUnitTestUtils::TrashFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), false);
    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_File_003);
    MediaLibraryUnitTestUtils::RecoveryFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_003::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_File_Test_004
 * @tc.name      : test trash and recovery file: the parent dir is not existed in db(is_trash != 0) and
 *                                               there is the same name file in file system when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_File_004
 *                 2.Create file1 in trashRecovery_File_004: trashRecovery_File_004/file1
 *                 3.trash file1
 *                 4.trash parent dir trashRecovery_File_004
 *                 5.recreate trashRecovery_File_004
 *                 6.recreate file1
 *                 7.recovery file1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_File_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_004::Start");
    shared_ptr<FileAsset> trashRecovery_File_004 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_004", g_download, trashRecovery_File_004));
    shared_ptr<FileAsset> file1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_004, file1));

    MediaLibraryUnitTestUtils::TrashFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), false);
    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_File_004);
    shared_ptr<FileAsset> sameParent = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_004", g_download, sameParent));
    shared_ptr<FileAsset> sameFile = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", sameParent, sameFile));
    MediaLibraryUnitTestUtils::RecoveryFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_004::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_File_Test_005
 * @tc.name      : test trash and recovery file: the parent dir is not existed in db(is_trash != 0) and
 *                                               there is the same name file in file system when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_File_005
 *                 2.Create file1 in trashRecovery_File_005: trashRecovery_File_005/file1
 *                 3.trash file1
 *                 4.rename parent dir trashRecovery_Dir_005
 *                 5.recovery dir1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_File_Test_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_005::Start");
    shared_ptr<FileAsset> trashRecovery_File_005 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_File_005", g_download, trashRecovery_File_005));
    shared_ptr<FileAsset> file1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file1.txt", trashRecovery_File_005, file1));

    MediaLibraryUnitTestUtils::TrashFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), false);
    MediaLibraryUnitTestUtils::RecoveryFile(file1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(file1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_File_Test_005::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_Dir_Test_001
 * @tc.name      : test trash and recovery dir: normal case
 * @tc.desc      : 1.Create dir: trashRecovery_Dir_001
 *                 2.Create childAsset in trashRecovery_Dir_001
 *                 3.trash trashRecovery_Dir_001
 *                 4.recovery trashRecovery_Dir_001
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_Dir_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_001::Start");
    shared_ptr<FileAsset> trashRecovery_Dir_001 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_001", g_download, trashRecovery_Dir_001));
    shared_ptr<FileAsset> childAsset = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", trashRecovery_Dir_001, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", trashRecovery_Dir_001, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", childAsset, childAsset));

    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_Dir_001);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(trashRecovery_Dir_001->GetPath()), false);
    MediaLibraryUnitTestUtils::RecoveryFile(trashRecovery_Dir_001);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(trashRecovery_Dir_001->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_001::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_Dir_Test_002
 * @tc.name      : test trash and recovery dir: there is the same name dir in file system when recovering
 * @tc.desc      : 1.Create dir: trashRecovery_Dir_002
 *                 2.Create childAsset in trashRecovery_Dir_002
 *                 3.trash trashRecovery_Dir_002
 *                 4.recreate trashRecovery_Dir_002
 *                 5.recovery trashRecovery_Dir_002
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_Dir_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_002::Start");
    shared_ptr<FileAsset> trashRecovery_Dir_002 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_002", g_download, trashRecovery_Dir_002));
    shared_ptr<FileAsset> childAsset = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", trashRecovery_Dir_002, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", trashRecovery_Dir_002, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", childAsset, childAsset));

    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_Dir_002);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(trashRecovery_Dir_002->GetPath()), false);
    shared_ptr<FileAsset> sameDir = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_002", g_download, sameDir));
    MediaLibraryUnitTestUtils::RecoveryFile(trashRecovery_Dir_002);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(trashRecovery_Dir_002->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_002::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_Dir_Test_003
 * @tc.name      : test trash and recovery dir: the parent dir is not existed in db(is_trash != 0) when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_Dir_003
 *                 2.Create dir1 in trashRecovery_Dir_003: trashRecovery_Dir_003/dir1
 *                 3.Create childAsset in dir1
 *                 4.trash dir1
 *                 5.trash parent dir trashRecovery_Dir_003
 *                 6.recovery dir1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_Dir_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_003::Start");
    shared_ptr<FileAsset> trashRecovery_Dir_003 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_003", g_download, trashRecovery_Dir_003));
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", trashRecovery_Dir_003, dir1));
    shared_ptr<FileAsset> childAsset = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", childAsset, childAsset));

    MediaLibraryUnitTestUtils::TrashFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), false);
    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_Dir_003);
    MediaLibraryUnitTestUtils::RecoveryFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_003::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_Dir_Test_004
 * @tc.name      : test trash and recovery dir: the parent dir is not existed in db(is_trash != 0) and
 *                                               there is the same name dir in file system when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_Dir_004
 *                 2.Create dir1 in trashRecovery_Dir_004: trashRecovery_Dir_004/dir1
 *                 3.Create childAsset in dir1
 *                 4.trash dir1
 *                 5.trash parent dir trashRecovery_Dir_004
 *                 6.recreate trashRecovery_Dir_004
 *                 7.recreate dir1
 *                 8.recovery dir1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_Dir_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_004::Start");
    shared_ptr<FileAsset> trashRecovery_Dir_004 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_004", g_download, trashRecovery_Dir_004));
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", trashRecovery_Dir_004, dir1));
    shared_ptr<FileAsset> childAsset = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", childAsset, childAsset));

    MediaLibraryUnitTestUtils::TrashFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), false);
    MediaLibraryUnitTestUtils::TrashFile(trashRecovery_Dir_004);
    shared_ptr<FileAsset> sameParent = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_004", g_download, sameParent));
    shared_ptr<FileAsset> sameDir = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", sameParent, sameDir));
    MediaLibraryUnitTestUtils::RecoveryFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_004::End");
}

/**
 * @tc.number    : DataManager_TrashRecovery_Dir_Test_005
 * @tc.name      : test trash and recovery dir: the parent dir is renamed when recovering
 * @tc.desc      : 1.Create the parent dir: trashRecovery_Dir_005
 *                 2.Create dir1 in trashRecovery_Dir_005: trashRecovery_Dir_005/dir1
 *                 3.Create childAsset in dir1
 *                 4.trash dir1
 *                 5.rename parent dir trashRecovery_Dir_005
 *                 6.recovery dir1
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_TrashRecovery_Dir_Test_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_005::Start");
    shared_ptr<FileAsset> trashRecovery_Dir_005 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("trashRecovery_Dir_005", g_download, trashRecovery_Dir_005));
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", trashRecovery_Dir_005, dir1));
    shared_ptr<FileAsset> childAsset = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, childAsset));
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", childAsset, childAsset));

    MediaLibraryUnitTestUtils::TrashFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), false);
    MediaLibraryUnitTestUtils::RecoveryFile(dir1);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(dir1->GetPath()), true);
    MEDIA_INFO_LOG("DataManager_TrashRecovery_Dir_Test_005::End");
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_001
 * @tc.name      : test delete dir: normal case
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_001
 *                 2.Create dir1 in delete_Dir_001: delete_Dir_001/dir1
 *                 3.Create file2 in dir1: delete_Dir_001/dir1/file2.txt
 *                 4.Create dir2 in dir1: delete_Dir_001/dir1/dir2
 *                 5.Create file3 in dir2: delete_Dir_001/dir1/dir2/file3.txt
 *                 6.Delete delete_Dir_001
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_001::Start");
    shared_ptr<FileAsset> delete_Dir_001 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_001", g_download, delete_Dir_001));
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_001", delete_Dir_001, dir1));
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("delete_Dir_001.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_001", dir1, dir2));
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("delete_Dir_001.txt", dir2, file3));

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_001::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_001->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_001->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_001::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_002
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_002
 *                 2.Create dir in delete_Dir_002: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_002/dir1/file2.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_002/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_002
 *                 9.Delete delete_Dir_002
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_002::Start");
    shared_ptr<FileAsset> delete_Dir_002 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_002", g_download, delete_Dir_002));
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_002, dir1));
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_002::trash start");
    MediaLibraryUnitTestUtils::TrashFile(file3);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_002);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_002::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_002->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_002->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_002::delete end, retVal: %d", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UriPermission_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_001::Start");
    shared_ptr<FileAsset> UriPermission001 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("UriPermission001.txt", g_download, UriPermission001));

    int32_t fileId = UriPermission001->GetId();
    string bundleName = BUNDLE_NAME;
    int32_t tableType = static_cast<int32_t>(TableType::TYPE_FILES);
    for (const auto &mode : MEDIA_OPEN_MODES) {
        EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);
    }

    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PERMISSION_FILE_ID, to_string(fileId))->And()->EqualTo(PERMISSION_BUNDLE_NAME, bundleName);
    Uri queryUri(MEDIALIBRARY_BUNDLEPERM_URI);
    int errCode = 0;
    MediaLibraryCommand cmd(queryUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    ASSERT_NE(resultSet, nullptr);
    int count = -1;
    ASSERT_EQ(resultSet->GetRowCount(count), E_OK);
    EXPECT_EQ(count, 0);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UriPermission_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_002::Start");
    Uri addPermission(ReturnUri(MEDIALIBRARY_BUNDLEPERM_URI, BUNDLE_PERMISSION_INSERT));
    DataShare::DataShareValuesBucket values;
    MediaLibraryCommand cmd(addPermission);
    int retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
    EXPECT_EQ(retVal, E_INVALID_VALUES);
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_002::ret: %d", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UriPermission_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_003::Start");
    Uri addPermission(ReturnUri(MEDIALIBRARY_BUNDLEPERM_URI, BUNDLE_PERMISSION_INSERT));
    DataShare::DataShareValuesBucket values;
    values.Put(PERMISSION_FILE_ID, 1);
    MediaLibraryCommand cmd(addPermission);
    int retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
    EXPECT_EQ(retVal, E_OK);
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_003::ret: %d", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UriPermission_Test_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UriPermission_Test_005::Start");
    int32_t fileId = 1;
    string bundleName = BUNDLE_NAME;
    string mode = "ra";
    int32_t tableType = static_cast<int32_t>(TableType::TYPE_FILES);
    EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);

    mode = "rt";
    EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);

    fileId = -1;
    bundleName = "";
    mode = MEDIA_FILEMODE_READONLY;
    EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);
}

string GetFileMediaTypeUri(int32_t mediaType, const string &networkId)
{
    string uri = MEDIALIBRARY_DATA_ABILITY_PREFIX + networkId + MEDIALIBRARY_DATA_URI_IDENTIFIER;
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return uri + MEDIALIBRARY_TYPE_AUDIO_URI;
        case MEDIA_TYPE_VIDEO:
            return uri + MEDIALIBRARY_TYPE_VIDEO_URI;
        case MEDIA_TYPE_IMAGE:
            return uri + MEDIALIBRARY_TYPE_IMAGE_URI;
        case MEDIA_TYPE_FILE:
        default:
            return uri + MEDIALIBRARY_TYPE_FILE_URI;
    }
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CheckUriPermission_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CheckUriPermission_Test_003::Start");
    shared_ptr<FileAsset> file = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("CheckUriPermission003.txt", g_download, file));

    int32_t fileId = file->GetId();
    string bundleName = BUNDLE_NAME;
    string mode = MEDIA_FILEMODE_READWRITE;
    int32_t tableType = static_cast<int32_t>(TableType::TYPE_FILES);
    EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);

    string uri = GetFileMediaTypeUri(MEDIA_TYPE_FILE, "") + SLASH_CHAR + to_string(fileId);
    for (const auto &inputMode : MEDIA_OPEN_MODES) {
        auto ret = UriPermissionOperations::CheckUriPermission(uri, inputMode);
        EXPECT_EQ(ret, E_PERMISSION_DENIED);
        MEDIA_ERR_LOG("CheckUriPermission permissionMode: %{public}s, inputMode: %{public}s, ret: %{public}d",
            mode.c_str(), inputMode.c_str(), ret);
    }
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CheckUriPermission_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_CheckUriPermission_Test_004::Start");
    shared_ptr<FileAsset> file = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("CheckUriPermission004.txt", g_download, file));

    int32_t fileId = file->GetId();
    string bundleName = BUNDLE_NAME;
    string mode = "Rw";
    int32_t tableType = static_cast<int32_t>(TableType::TYPE_FILES);
    EXPECT_EQ(MediaLibraryUnitTestUtils::GrantUriPermission(fileId, bundleName, mode, tableType), E_SUCCESS);

    string uri = GetFileMediaTypeUri(MEDIA_TYPE_FILE, "") + SLASH_CHAR + to_string(fileId);
    string inputMode = "rWt";
    auto ret = UriPermissionOperations::CheckUriPermission(uri, inputMode);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
    MEDIA_ERR_LOG("CheckUriPermission permissionMode: %{public}s, inputMode: %{public}s, ret: %{public}d",
        mode.c_str(), inputMode.c_str(), ret);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_GetDirQuerySetMap_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    mediaLibraryDataManager->GetDirQuerySetMap();
    shared_ptr<MediaDataShareExtAbility> datashareExternsion =  nullptr;
    mediaLibraryDataManager->SetOwner(datashareExternsion);
    auto ret = mediaLibraryDataManager->GetOwner();
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_CreateThumbnailAsync_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    string uri = "";
    mediaLibraryDataManager->CreateThumbnailAsync(uri, "Download");
    EXPECT_NE(mediaLibraryDataManager->thumbnailService_, nullptr);
    string uriTest = "CreateThumbnailAsync";
    mediaLibraryDataManager->CreateThumbnailAsync(uriTest, "Download");
    EXPECT_NE(mediaLibraryDataManager->thumbnailService_, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_DoTrashAging_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    int32_t ret = mediaLibraryDataManager->DoTrashAging();
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_DoAging_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    int32_t ret = mediaLibraryDataManager->DoAging();
    EXPECT_EQ(ret, E_OK);
    shared_ptr<OHOS::AbilityRuntime::Context> extensionContext;
    mediaLibraryDataManager->InitialiseThumbnailService(extensionContext);
    mediaLibraryDataManager->GenerateThumbnailBackground();
    ret = mediaLibraryDataManager->DoAging();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_SolveInsertCmd_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmdOne(OperationObject::FILESYSTEM_ASSET, OperationType::CREATE);
    int32_t ret = mediaLibraryDataManager->SolveInsertCmd(cmdOne);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmdTwo(OperationObject::FILESYSTEM_PHOTO, OperationType::OPEN);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdTwo);
    EXPECT_EQ(ret, E_ERR);
    MediaLibraryCommand cmdThree(OperationObject::FILESYSTEM_AUDIO, OperationType::OPEN);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdThree);
    EXPECT_EQ(ret, E_ERR);
    MediaLibraryCommand cmdFour(OperationObject::FILESYSTEM_ALBUM, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdFour);
    EXPECT_EQ(ret, E_INVALID_PATH);
    MediaLibraryCommand cmdFive(OperationObject::PHOTO_ALBUM, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdFive);
    EXPECT_NE(ret, E_OK);
    MediaLibraryCommand cmdSix(OperationObject::FILESYSTEM_DIR, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdSix);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_SolveInsertCmd_Test_002, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmdOne(OperationObject::SMART_ALBUM, OperationType::CREATE);
    int32_t ret = mediaLibraryDataManager->SolveInsertCmd(cmdOne);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmdTwo(OperationObject::SMART_ALBUM_MAP, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdTwo);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);
    MediaLibraryCommand cmdThree(OperationObject::THUMBNAIL, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdThree);
    EXPECT_EQ(ret, E_FAIL);
    MediaLibraryCommand cmdFour(OperationObject::BUNDLE_PERMISSION, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdFour);
    EXPECT_EQ(ret, E_FAIL);
    MediaLibraryCommand cmdFive(OperationObject::UNKNOWN_OBJECT, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdFive);
    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_SetCmdBundleAndDevice_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmd(Uri(MEDIALIBRARY_BUNDLEPERM_URI), OperationType::QUERY);
    int32_t ret = mediaLibraryDataManager->SetCmdBundleAndDevice(cmd);
    EXPECT_EQ(ret, E_GET_CLIENTBUNDLE_FAIL);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_GetThumbnail_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    string uri = "GetThumbnail";
    auto ret = mediaLibraryDataManager->GetThumbnail(uri);
    EXPECT_LT(ret, 0);
    shared_ptr<OHOS::AbilityRuntime::Context> extensionContext;
    mediaLibraryDataManager->InitialiseThumbnailService(extensionContext);
    ret = mediaLibraryDataManager->GetThumbnail(uri);
    EXPECT_LT(ret, 0);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_001, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmd(uri);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_002, TestSize.Level2)
{
    Uri uri(UFM_PHOTO_ALBUM_ADD_ASSET);
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmd(uri);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_003, TestSize.Level2)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    MediaLibraryCommand cmd(uri);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryRdb_Test_001, TestSize.Level2)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri, OperationType::QUERY);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    int errCode = 0;
    auto ret = mediaLibraryDataManager->QueryRdb(cmd, columns, predicates, errCode);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_GetType_Test_001, TestSize.Level2)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    auto ret = mediaLibraryDataManager->GetType(uri);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_NotifyChange_Test_001, TestSize.Level2)
{
    Uri uri(MEDIALIBRARY_DATA_URI);
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    mediaLibraryDataManager->NotifyChange(uri);
    EXPECT_EQ(mediaLibraryDataManager->extension_, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_GenerateThumbnailBackground_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    int32_t ret = mediaLibraryDataManager->GenerateThumbnailBackground();
    EXPECT_EQ(ret <= 0, true);
    mediaLibraryDataManager->ClearMediaLibraryMgr();
    ret = mediaLibraryDataManager->GenerateThumbnailBackground();
    EXPECT_EQ(ret <= 0, true);
}

void ClearAnalysisAlbumTable()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    string sql = "DELETE FROM " + ANALYSIS_ALBUM_TABLE;
    int err = rdbStore->ExecuteSql(sql);
    EXPECT_EQ(err, E_OK);
}

int64_t CreateSmartAlbum(const string &albumName, const string &tag,
    const PhotoAlbumSubType &subtype = PhotoAlbumSubType::PORTRAIT, const int32_t &displayLevel = 1)
{
    MEDIA_INFO_LOG("Create smart album");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int64_t albumId = -1;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, subtype);
    valuesBucket.PutInt(USER_DISPLAY_LEVEL, displayLevel);
    valuesBucket.PutString(TAG_ID, tag);
    valuesBucket.PutString(GROUP_TAG, tag);
    if (albumName != "") {
        valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    }
    EXPECT_NE(rdbStore, nullptr);
    int32_t ret = rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertAlbum albumId is %{public}s", to_string(albumId).c_str());
    return albumId;
}

shared_ptr<DataShare::DataShareResultSet> QueryAlbumWithName()
{
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_NAME};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_NAME_NOT_NULL, to_string(1));
    predicates.OrderByAsc(PhotoAlbumColumns::ALBUM_ID);
    int errCode = 0;
    int count = -1;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_GE(count, 0);
    return resultSet;
}

void CheckResult(const shared_ptr<DataShare::DataShareResultSet> &resultSet, int count, const vector<string> &names)
{
    int albumCount = 0;
    resultSet->GetRowCount(albumCount);
    EXPECT_EQ(albumCount, count);
    if (count == 0) {
        return;
    }
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    for (string name : names) {
        string albumName;
        resultSet->GetString(0, albumName);
        EXPECT_EQ(albumName, name);
        resultSet->GoToNextRow();
    }
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Protrait_Album_NAME_NOT_NULL_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_NAME_NOT_NULL_test_001 Start");
    MEDIA_INFO_LOG("Clear analysis table");
    ClearAnalysisAlbumTable();

    MEDIA_INFO_LOG("Create portrait albums");
    const string albumName1 = "album_1";
    const string albumName2 = "album_2";
    vector<string> tags = {
        "ser_1711000000000000000",
        "ser_1711000000000000001",
        "ser_1711000000000000002",
        "ser_1711000000000000003"
    };
    CreateSmartAlbum(albumName1, tags[0]);
    CreateSmartAlbum("", tags[1]);
    CreateSmartAlbum("", tags[2]);
    CreateSmartAlbum(albumName2, tags[3]);

    MEDIA_INFO_LOG("Query albums and check result");
    CheckResult(QueryAlbumWithName(), 2, {albumName1, albumName2});
    MEDIA_INFO_LOG("Get_Protrait_Album_NAME_NOT_NULL_test_001 End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Protrait_Album_NAME_NOT_NULL_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_NAME_NOT_NULL_test_002 Start");
    MEDIA_INFO_LOG("Clear analysis table");
    ClearAnalysisAlbumTable();

    MEDIA_INFO_LOG("Create portrait albums");
    vector<string> tags = {
        "ser_1711000000000000000",
        "ser_1711000000000000001",
        "ser_1711000000000000002"
    };
    CreateSmartAlbum("", tags[0]);
    CreateSmartAlbum("", tags[1]);
    CreateSmartAlbum("", tags[2]);

    MEDIA_INFO_LOG("Query albums and check result");
    CheckResult(QueryAlbumWithName(), 0, {});
    MEDIA_INFO_LOG("Get_Protrait_Album_NAME_NOT_NULL_test_002 End");
}

void ClearAllTable()
{
    MediaLibraryUnitTestUtils::ClearTable(PhotoColumn::PHOTOS_TABLE);
    MediaLibraryUnitTestUtils::ClearTable(VISION_IMAGE_FACE_TABLE);
    MediaLibraryUnitTestUtils::ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
    MediaLibraryUnitTestUtils::ClearTable(ANALYSIS_ALBUM_TABLE);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Group_Photo_Album_NAME_COUNT_test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_001 Start");
    MEDIA_INFO_LOG("Clear all table");
    ClearAllTable();
    const string albumName1 = "album_1";
    const string albumName2 = "album_2";

    MEDIA_INFO_LOG("Create portrait albums");
    vector<string> tags = {
        "ser_1711000000000000000, ser_1711000000000000001",
        "ser_1711000000000000001, ser_1711000000000000002",
        "ser_1711000000000000002, ser_1711000000000000003",
        "ser_1711000000000000003, ser_1711000000000000003"
    };

    CreateSmartAlbum(albumName1, tags[0], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[1], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[2], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum(albumName2, tags[3], PhotoAlbumSubType::GROUP_PHOTO);

    MEDIA_INFO_LOG("Query albums and check result");
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_ID};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
    int errCode = 0;
    int count = -1;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 4);

    auto ret = resultSet->GoToFirstRow();
    EXPECT_EQ(ret, 0);
    int colIndex = -1;
    resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ID, colIndex);
    int32_t albumId = -1;
    ret = resultSet->GetInt(colIndex, albumId);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(albumId, 0);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto albumIdResultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(albumIdResultSet, nullptr);
    albumIdResultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_001 End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Group_Photo_Album_NAME_COUNT_test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_002 Start");
    MEDIA_INFO_LOG("Clear all table");
    ClearAllTable();
    const string albumName1 = "album_1";
    const string albumName2 = "album_2";

    MEDIA_INFO_LOG("Create portrait albums");
    vector<string> tags = {
        "ser_1711000000000000000, ser_1711000000000000001",
        "ser_1711000000000000001, ser_1711000000000000002",
        "ser_1711000000000000002, ser_1711000000000000003",
        "ser_1711000000000000003, ser_1711000000000000003"
    };

    CreateSmartAlbum(albumName1, tags[0], PhotoAlbumSubType::GROUP_PHOTO, -1);
    CreateSmartAlbum("", tags[1], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[2], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum(albumName2, tags[3], PhotoAlbumSubType::GROUP_PHOTO);

    MEDIA_INFO_LOG("Query albums and check result");
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_ID};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
    predicates.NotEqualTo(USER_DISPLAY_LEVEL, NOT_DISPLAY_FOR_BACKGROUND);
    int errCode = 0;
    int count = -1;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 3);
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_002 End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Group_Photo_Album_NAME_COUNT_test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_003 Start");
    MEDIA_INFO_LOG("Clear all table");
    ClearAllTable();
    const string albumName1 = "album_1";
    const string albumName2 = "album_2";

    MEDIA_INFO_LOG("Create portrait albums");
    vector<string> tags = {
        "ser_1711000000000000000, ser_1711000000000000001",
        "ser_1711000000000000001, ser_1711000000000000002",
        "ser_1711000000000000002, ser_1711000000000000003",
        "ser_1711000000000000003, ser_1711000000000000003"
    };

    CreateSmartAlbum(albumName1, tags[0], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[1], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[2], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum(albumName2, tags[3], PhotoAlbumSubType::GROUP_PHOTO);

    MEDIA_INFO_LOG("Query albums and check result");
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_ID};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
    predicates.SetWhereClause("user_display_level");
    int errCode = 0;
    int count = -1;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 4);

    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_003 End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, Get_Group_Photo_Album_NAME_COUNT_test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_004 Start");
    MEDIA_INFO_LOG("Clear all table");
    ClearAllTable();
    const string albumName1 = "album_1";
    const string albumName2 = "album_2";

    MEDIA_INFO_LOG("Create portrait albums");
    vector<string> tags = {
        "ser_1711000000000000000, ser_1711000000000000001",
        "ser_1711000000000000001, ser_1711000000000000002",
        "ser_1711000000000000002, ser_1711000000000000003",
        "ser_1711000000000000003, ser_1711000000000000003"
    };

    CreateSmartAlbum(albumName1, tags[0], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[1], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum("", tags[2], PhotoAlbumSubType::GROUP_PHOTO);
    CreateSmartAlbum(albumName2, tags[3], PhotoAlbumSubType::GROUP_PHOTO);

    MEDIA_INFO_LOG("Query albums and check result");
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_ID};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
    predicates.EqualTo(IS_REMOVED, ALBUM_IS_REMOVED);
    int errCode = 0;
    int count = -1;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 0);

    MEDIA_INFO_LOG("Get_Group_Photo_Album_NAME_COUNT_test_004 End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, GenerateThumbnailBackground_new_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    mediaLibraryDataManager->refCnt_.store(0);
    auto ret = mediaLibraryDataManager->GenerateThumbnailBackground();
    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, GenerateThumbnailBackground_new_001_2, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    mediaLibraryDataManager->refCnt_.store(1);
    mediaLibraryDataManager->thumbnailService_ = nullptr;
    auto ret = mediaLibraryDataManager->GenerateThumbnailBackground();
    EXPECT_EQ(ret, E_THUMBNAIL_SERVICE_NULLPTR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, GenerateThumbnailBackground_new_001_3, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    mediaLibraryDataManager->refCnt_.store(1);
    mediaLibraryDataManager->thumbnailService_ = std::make_shared<ThumbnailService>();
    auto ret = mediaLibraryDataManager->GenerateThumbnailBackground();
    EXPECT_EQ(ret, mediaLibraryDataManager->thumbnailService_->GenerateThumbnailBackground());
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpgradeThumbnailBackground_new_002, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    auto ret = mediaLibraryDataManager->UpgradeThumbnailBackground(false);
    EXPECT_EQ(ret<=0, true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, RestoreThumbnailDualFrame_new_003, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    auto ret = mediaLibraryDataManager->RestoreThumbnailDualFrame();
    mediaLibraryDataManager->SetStartupParameter();
    EXPECT_EQ(ret<=0, true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, GenerateHighlightThumbnailBackground_test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    auto ret = mediaLibraryDataManager->GenerateHighlightThumbnailBackground();
    EXPECT_EQ(ret <= 0, true);
}

struct BurstResult {
    int64_t fileId;
    string title;
    int32_t mediaType;
    int32_t subtype;
    int32_t isFavourite;
    int32_t burstCoverLevel;
    string burstKey;
    int32_t burstKeyLength;
    bool isCover;
    int32_t mapAlbum;
};

void InsertBurstAsset(BurstResult &result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, result.mediaType);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, result.title);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, result.subtype);
    valuesBucket.PutInt(MediaColumn::MEDIA_IS_FAV, result.isFavourite);
    if (result.burstKey != "") {
        valuesBucket.PutString(PhotoColumn::PHOTO_BURST_KEY, result.burstKey);
    }
    
    int32_t ret = rdbStore->Insert(result.fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

void InsertPhotomapForBurst(BurstResult result)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    int64_t fileId = -1;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoMap::ASSET_ID, result.fileId);
    valuesBucket.PutInt(PhotoMap::ALBUM_ID, result.mapAlbum);

    int32_t ret = rdbStore->Insert(fileId, PhotoMap::TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

void ValidBurstValue(BurstResult &exResult)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    string querySql = "SELECT p1." + MediaColumn::MEDIA_ID + ", p1." + MediaColumn::MEDIA_TITLE + ", p1." +
        PhotoColumn::PHOTO_SUBTYPE + ", p1." + MediaColumn::MEDIA_IS_FAV + ", p1." + PhotoColumn::PHOTO_BURST_KEY +
        ", p1." + PhotoColumn::PHOTO_BURST_COVER_LEVEL + ", p2." + PhotoMap::ALBUM_ID + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " AS p1 JOIN " + PhotoMap::TABLE + " AS p2 ON p1." + MediaColumn::MEDIA_ID +
        " = p2." + PhotoMap::ASSET_ID + " WHERE p1." + MediaColumn::MEDIA_ID + " = " + to_string(exResult.fileId);

    auto resultSet = rdbStore->QueryByStep(querySql);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    EXPECT_EQ(ret, E_OK);
    string titleValue = GetStringVal(MediaColumn::MEDIA_TITLE, resultSet);
    EXPECT_EQ(titleValue, exResult.title);
    int32_t subtypeValue = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    EXPECT_EQ(subtypeValue, exResult.subtype);
    int32_t isFavouriteValue = GetInt32Val(MediaColumn::MEDIA_IS_FAV, resultSet);
    EXPECT_EQ(isFavouriteValue, exResult.isFavourite);
    int32_t burstCoverLevelValue = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
    EXPECT_EQ(burstCoverLevelValue, exResult.burstCoverLevel);
    string burstKeyValue = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
    EXPECT_EQ(burstKeyValue.size(), exResult.burstKeyLength);
    int32_t mapAlbumValue = GetInt32Val(PhotoMap::ALBUM_ID, resultSet);
    EXPECT_EQ(mapAlbumValue, exResult.mapAlbum);

    if (exResult.isCover && exResult.burstKeyLength > 0) {
        exResult.burstKey = burstKeyValue;
    }
    if (!exResult.isCover && exResult.burstKeyLength > 0) {
        EXPECT_EQ(burstKeyValue, exResult.burstKey);
    }
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpdateBurstFromGallery_test_function_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("start UpdateBurstFromGallery_test_function_001");
    isChargingAndScreenOffPtr = MockIsChargingAndScreenOff;
    struct BurstResult burstCover = {-1, "IMG_12345678_123456_BURST001_COVER",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoSubType::DEFAULT), 0,
        static_cast<int32_t>(BurstCoverLevelType::COVER), "", 0, true, 8};
    InsertBurstAsset(burstCover);
    InsertPhotomapForBurst(burstCover);
    ValidBurstValue(burstCover);

    struct BurstResult burstMember = {-1, "IMG_12345678_123456_BURST002",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoSubType::DEFAULT), 0,
        static_cast<int32_t>(BurstCoverLevelType::COVER), "", 0, false, 8};
    InsertBurstAsset(burstMember);
    InsertPhotomapForBurst(burstMember);
    ValidBurstValue(burstMember);

    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    dataManager->refCnt_.store(1);
    auto result = dataManager->UpdateBurstFromGallery();
    EXPECT_EQ(result, E_OK);

    burstCover.subtype = static_cast<int32_t>(PhotoSubType::BURST);
    burstCover.burstKeyLength = 36;
    ValidBurstValue(burstCover);

    burstMember.subtype = static_cast<int32_t>(PhotoSubType::BURST);
    burstMember.burstCoverLevel = static_cast<int32_t>(BurstCoverLevelType::MEMBER);
    burstMember.burstKeyLength = 36;
    burstMember.burstKey = burstCover.burstKey;
    ValidBurstValue(burstMember);
    MEDIA_INFO_LOG("end UpdateBurstFromGallery_test_001");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpdateBurstFromGallery_test_function_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("start UpdateBurstFromGallery_test_function_002");
    isChargingAndScreenOffPtr = MockIsChargingAndScreenOff;
    struct BurstResult burstCover = {-1, "IMG_12345678_123456_BURST001_cover",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoSubType::DEFAULT), 0,
        static_cast<int32_t>(BurstCoverLevelType::COVER), "", 0, true, 8};
    InsertBurstAsset(burstCover);
    InsertPhotomapForBurst(burstCover);
    ValidBurstValue(burstCover);

    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    dataManager->refCnt_.store(1);
    auto result = dataManager->UpdateBurstFromGallery();
    EXPECT_EQ(result, E_OK);

    // IMG_12345678_123456_BURST001_cover is burst cover (case-insensitive to letters)
    burstCover.subtype = static_cast<int32_t>(PhotoSubType::BURST);
    burstCover.burstKeyLength = 36;
    ValidBurstValue(burstCover);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpdateBurstFromGallery_test_function_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("start UpdateBurstFromGallery_test_function_003");
    isChargingAndScreenOffPtr = MockIsChargingAndScreenOff;
    struct BurstResult burstCover = {-1, "IMG_12345678_123456_BURST_cover",
        static_cast<int32_t>(MEDIA_TYPE_IMAGE), static_cast<int32_t>(PhotoSubType::DEFAULT), 0,
        static_cast<int32_t>(BurstCoverLevelType::COVER), "", 0, true, 8};
    InsertBurstAsset(burstCover);
    InsertPhotomapForBurst(burstCover);
    ValidBurstValue(burstCover);

    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    dataManager->refCnt_.store(1);
    auto result = dataManager->UpdateBurstFromGallery();
    EXPECT_EQ(result, E_OK);

    // IMG_12345678_123456_BURST_cover is not burst cover (case-insensitive to letters)
    ValidBurstValue(burstCover);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpdateBurstFromGallery_test_001, TestSize.Level2)
{
    isChargingAndScreenOffPtr = MockIsChargingAndScreenOff;
    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    dataManager->refCnt_.store(0);
    auto result = dataManager->UpdateBurstFromGallery();
    EXPECT_EQ(result, E_FAIL);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, UpdateBurstFromGallery_test_002, TestSize.Level2)
{
    isChargingAndScreenOffPtr = MockIsChargingAndScreenOff;
    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    dataManager->refCnt_.store(1);
    auto result = dataManager->UpdateBurstFromGallery();
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, PhotoDayMonthYearOperation_test, TestSize.Level2)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t ret = PhotoDayMonthYearOperation::UpdatePhotosDateAndIdx(rdbStore);
    EXPECT_EQ(ret, E_OK);
    ret = PhotoDayMonthYearOperation::UpdatePhotosDateIdx(rdbStore);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, RefreshPhotoAlbums_test, TestSize.Level2)
{
    MEDIA_INFO_LOG("start RefreshPhotoAlbums_test");
    AlbumsRefreshManager &instance =  AlbumsRefreshManager::GetInstance();
    EXPECT_NE(instance.refreshWorker_, nullptr);

    SyncNotifyInfo info;
    info.taskType = TIME_BEGIN_SYNC;
    instance.RefreshPhotoAlbums(info);
    MEDIA_INFO_LOG("end RefreshPhotoAlbums_test");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_SolveInsertCmd_Test_003, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmdOne(OperationObject::SMART_ALBUM, OperationType::CREATE);
    int32_t ret = mediaLibraryDataManager->SolveInsertCmd(cmdOne);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmdTwo(OperationObject::SMART_ALBUM_MAP, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdTwo);
    EXPECT_EQ(ret, E_SMARTALBUM_IS_NOT_EXISTED);
    MediaLibraryCommand cmdThree(OperationObject::THUMBNAIL, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdThree);
    EXPECT_EQ(ret, E_FAIL);
    MediaLibraryCommand cmdFive(OperationObject::UNKNOWN_OBJECT, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdFive);
    EXPECT_EQ(ret, E_FAIL);
    MediaLibraryCommand cmdsix(OperationObject::APP_URI_PERMISSION_INNER, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdsix);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmdseven(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdseven);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_SolveInsertCmdSub_Test_001, TestSize.Level2)
{
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmdOne(OperationObject::SMART_ALBUM, OperationType::CREATE);
    int32_t ret = mediaLibraryDataManager->SolveInsertCmd(cmdOne);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MediaLibraryCommand cmdTwo(OperationObject::PAH_FORM_MAP, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdTwo);
    EXPECT_EQ(ret, E_GET_PRAMS_FAIL);
    MediaLibraryCommand cmdThree(OperationObject::TAB_FACARD_PHOTO, OperationType::CREATE);
    ret = mediaLibraryDataManager->SolveInsertCmd(cmdThree);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_004, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::ANALYSIS_PHOTO_MAP);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_005, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::ADD_ASSET_HIGHLIGHT_ALBUM);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_DB_FAIL);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_006, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::MEDIA_APP_URI_PERMISSION);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_007, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::APP_URI_PERMISSION_INNER);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_008, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::MTH_AND_YEAR_ASTC);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_BatchInsert_Test_009, TestSize.Level2)
{
    Uri uri("");
    vector<DataShare::DataShareValuesBucket> values;
    auto mediaLibraryDataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(mediaLibraryDataManager, nullptr);
    MediaLibraryCommand cmd(uri);
    cmd.SetOprnObject(OperationObject::UNKNOWN_OBJECT);
    int32_t ret = mediaLibraryDataManager->BatchInsert(cmd, values);
    EXPECT_EQ(ret, E_INVALID_URI);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_003
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_003
 *                 2.Create dir in delete_Dir_003: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_003/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_003/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_003
 *                 9.Delete delete_Dir_003
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_003::Start");
    shared_ptr<FileAsset> delete_Dir_003 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_003", g_download, delete_Dir_003));
    ASSERT_NE(delete_Dir_003, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_003, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_003::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_003, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_003);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_003::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(delete_Dir_003, nullptr);
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_003->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    cmd.SetOprnObject(OperationObject::HIGHLIGHT_DELETE);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_003->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_003::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_004
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_004
 *                 2.Create dir in delete_Dir_004: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_004/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_004/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_004
 *                 9.Delete delete_Dir_004
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_004::Start");
    shared_ptr<FileAsset> delete_Dir_004 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_004", g_download, delete_Dir_004));
    ASSERT_NE(delete_Dir_004, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_004, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_004::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_004, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_004);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_004::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(delete_Dir_004, nullptr);
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_004->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    cmd.SetOprnObject(OperationObject::PHOTO_MAP);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_004->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_004::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_005
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_005
 *                 2.Create dir in delete_Dir_005: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_005/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_005/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_005
 *                 9.Delete delete_Dir_005
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_005::Start");
    shared_ptr<FileAsset> delete_Dir_005 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_005", g_download, delete_Dir_005));
    ASSERT_NE(delete_Dir_005, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_005, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_005::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_005, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_005);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_005::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(delete_Dir_005, nullptr);
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_005->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    cmd.SetOprnObject(OperationObject::APP_URI_PERMISSION_INNER);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_005->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_005::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_006
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_006
 *                 2.Create dir in delete_Dir_006: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_006/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_006/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_006
 *                 9.Delete delete_Dir_006
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_006, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_006::Start");
    shared_ptr<FileAsset> delete_Dir_006 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_006", g_download, delete_Dir_006));
    ASSERT_NE(delete_Dir_006, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_006, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_006::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_006, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_006);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_006::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(delete_Dir_006, nullptr);
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_006->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    cmd.SetOprnObject(OperationObject::APP_URI_PERMISSION_INNER);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_006->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_006::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_007
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_007
 *                 2.Create dir in delete_Dir_007: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_007/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_007/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_007
 *                 9.Delete delete_Dir_007
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_007::Start");
    shared_ptr<FileAsset> delete_Dir_007 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_007", g_download, delete_Dir_007));
    ASSERT_NE(delete_Dir_007, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_007, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_007::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_007, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_007);

    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_007::deleteUri: %s", deleteUri.c_str());
    Uri deleteAssetUri(deleteUri);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(delete_Dir_007, nullptr);
    predicates.EqualTo(MEDIA_DATA_DB_ID, to_string(delete_Dir_007->GetId()));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    cmd.SetOprnObject(OperationObject::PAH_FORM_MAP);
    int retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, {});
    EXPECT_EQ(MediaLibraryUnitTestUtils::IsFileExists(delete_Dir_007->GetPath()), false);
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_007::delete end, retVal: %d", retVal);
}

/**
 * @tc.number    : DataManager_Delete_Dir_Test_008
 * @tc.name      : test delete dir: exists trashed children
 * @tc.desc      : 1.Create the parent dir to be deleted: delete_Dir_008
 *                 2.Create dir in delete_Dir_008: delete_Dir_002/dir1
 *                 3.Create file in dir1: delete_Dir_008/dir1/file3.txt
 *                 4.Create dir in dir1: delete_Dir_002/dir1/dir2
 *                 5.Create file in dir2: delete_Dir_008/dir1/dir2/file3.txt
 *                 6.Trash file3.txt
 *                 7.Trash dir1
 *                 8.Trash delete_Dir_008
 *                 9.Delete delete_Dir_008
 */
HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_Delete_Dir_Test_008, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_008::Start");
    shared_ptr<FileAsset> delete_Dir_008 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("delete_Dir_008", g_download, delete_Dir_008));
    ASSERT_NE(delete_Dir_008, nullptr);
    shared_ptr<FileAsset> dir1 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir1", delete_Dir_008, dir1));
    ASSERT_NE(dir1, nullptr);
    shared_ptr<FileAsset> file2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file2.txt", dir1, file2));
    shared_ptr<FileAsset> dir2 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateAlbum("dir2", dir1, dir2));
    ASSERT_NE(dir2, nullptr);
    shared_ptr<FileAsset> file3 = nullptr;
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateFile("file3.txt", dir2, file3));

    MEDIA_INFO_LOG("DataManager_Delete_Dir_Test_008::trash start");
    ASSERT_NE(file3, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(file3);
    ASSERT_NE(dir1, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(dir1);
    ASSERT_NE(delete_Dir_008, nullptr);
    MediaLibraryUnitTestUtils::TrashFile(delete_Dir_008);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UpdateAlbumAsset_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_002::Start");
    shared_ptr<FileAsset> albumAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateAlbum("UpdateAlbumAsset_Test_002", g_pictures, albumAsset), true);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(albumAsset, nullptr);
    string prefix = MEDIA_DATA_DB_ID + " = " + to_string(albumAsset->GetId());
    predicates.SetWhereClause(prefix);
    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, albumAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, albumAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, albumAsset->GetRelativePath());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "U" + albumAsset->GetDisplayName());
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_002::GetUri = %{public}s", albumAsset->GetUri().c_str());
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN, MEDIA_ALBUMOPRN_MODIFYALBUM));
    MediaLibraryCommand cmd(updateAssetUri);
    cmd.SetOprnObject(OperationObject::PAH_MULTISTAGES_CAPTURE);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_GE(retVal, 0);
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_002::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UpdateAlbumAsset_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_003::Start");
    shared_ptr<FileAsset> albumAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateAlbum("UpdateAlbumAsset_Test_003", g_pictures, albumAsset), true);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(albumAsset, nullptr);
    string prefix = MEDIA_DATA_DB_ID + " = " + to_string(albumAsset->GetId());
    predicates.SetWhereClause(prefix);
    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, albumAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, albumAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, albumAsset->GetRelativePath());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "U" + albumAsset->GetDisplayName());
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_003::GetUri = %{public}s", albumAsset->GetUri().c_str());
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN, MEDIA_ALBUMOPRN_MODIFYALBUM));
    MediaLibraryCommand cmd(updateAssetUri);
    cmd.SetOprnObject(OperationObject::PAH_BATCH_THUMBNAIL_OPERATE);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_NE(retVal, 0);
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_003::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_UpdateAlbumAsset_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_004::Start");
    shared_ptr<FileAsset> albumAsset = nullptr;
    ASSERT_EQ(MediaLibraryUnitTestUtils::CreateAlbum("UpdateAlbumAsset_Test_004", g_pictures, albumAsset), true);
    DataShare::DataSharePredicates predicates;
    ASSERT_NE(albumAsset, nullptr);
    string prefix = MEDIA_DATA_DB_ID + " = " + to_string(albumAsset->GetId());
    predicates.SetWhereClause(prefix);
    DataShare::DataShareValuesBucket valuesBucketUpdate;
    valuesBucketUpdate.Put(MEDIA_DATA_DB_MEDIA_TYPE, albumAsset->GetMediaType());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_URI, albumAsset->GetUri());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_RELATIVE_PATH, albumAsset->GetRelativePath());
    valuesBucketUpdate.Put(MEDIA_DATA_DB_NAME, "U" + albumAsset->GetDisplayName());
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_003::GetUri = %{public}s", albumAsset->GetUri().c_str());
    Uri updateAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIA_ALBUMOPRN, MEDIA_ALBUMOPRN_MODIFYALBUM));
    MediaLibraryCommand cmd(updateAssetUri);
    cmd.SetOprnObject(OperationObject::ANALYSIS_PHOTO_MAP);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucketUpdate, predicates);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("DataManager_UpdateAlbumAsset_Test_004::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryGeoAssets_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::ANALYSIS_ADDRESS_ASSETS);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_001, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::MEDIA_VOLUME);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::INDEX_CONSTRUCTION_STATUS);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::PHOTO_ALBUM);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::ANALYSIS_PHOTO_MAP);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_005, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::PAH_MULTISTAGES_CAPTURE);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_006, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::ANALYSIS_ADDRESS_ASSETS_ACTIVE);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, DataManager_QueryInternal_Test_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("DataManager_QueryDirTable_Test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, MEDIATYPE_DIRECTORY_OBJ));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    cmd.SetOprnObject(OperationObject::TAB_OLD_PHOTO);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_NE((resultSet == nullptr), true);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, CheckCloudThumbnailDownloadFinish_test_001, TestSize.Level0)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);

    int64_t totalFileSize = 0;
    dataManager->UploadDBFileInner(totalFileSize);

    totalFileSize = -1;
    dataManager->UploadDBFileInner(totalFileSize);

    totalFileSize = 201;
    dataManager->UploadDBFileInner(totalFileSize);

    dataManager->thumbnailService_ = nullptr;
    EXPECT_EQ(dataManager->CheckCloudThumbnailDownloadFinish(), E_THUMBNAIL_SERVICE_NULLPTR);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, AstcMthAndYearInsert_test_001, TestSize.Level0)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    std::string whereClause = "";
    std::vector<std::string> whereArgs = {"1", "2"};
    cmd.GetAbsRdbPredicates()->SetWhereClause(whereClause);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    cmd.SetOprnObject(OperationObject::MTH_AND_YEAR_ASTC);
    vector<DataShare::DataShareValuesBucket> values;
    DataShare::DataShareValuesBucket valuesBucket;
    string relativePath = "Pictures/";
    string displayName = "CreateAsset_Test_001.jpg";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    values.push_back(valuesBucket);
    auto result = dataManager->BatchInsert(cmd, values);
    EXPECT_EQ(result, -1);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, LSMediaFiles_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("LSMediaFiles_test_001::Start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);
    string result;
    DataShare::DataShareValuesBucket valuesBucket;
    string examplePath = "/storage/cloud/files/Photo";
    valuesBucket.Put(MediaColumn::MEDIA_FILE_PATH, examplePath);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::LS_MEDIA_FILES, MediaLibraryApi::API_10);
    int32_t ret = dataManager->InsertExt(cmd, valuesBucket, result);
    EXPECT_EQ(ret, E_INVALID_PATH);
    EXPECT_TRUE(result.empty());
    MEDIA_INFO_LOG("LSMediaFiles_test_001::End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, LSMediaFiles_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("LSMediaFiles_test_002::Start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);
    string result;
    DataShare::DataShareValuesBucket valuesBucket;
    string invalidPath = "/storage/media/local/files/Photo/invalid";
    valuesBucket.Put(MediaColumn::MEDIA_FILE_PATH, invalidPath);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::LS_MEDIA_FILES, MediaLibraryApi::API_10);
    int32_t ret = dataManager->InsertExt(cmd, valuesBucket, result);
    EXPECT_EQ(ret, E_INVALID_PATH);
    EXPECT_TRUE(result.empty());
    MEDIA_INFO_LOG("LSMediaFiles_test_002::End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, QueryActiveUserID_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryActiveUserID_test_001::Start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);
    string result;
    DataShare::DataShareValuesBucket valuesBucket;
    const string stubValue = "stub";
    valuesBucket.Put(stubValue, 0);
    MediaLibraryCommand cmd(OperationObject::MISCELLANEOUS, OperationType::QUERY_ACTIVE_USER_ID, MediaLibraryApi::API_10);
    int32_t ret = dataManager->InsertExt(cmd, valuesBucket, result);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(result.empty());
    MEDIA_INFO_LOG("QueryActiveUserID_test_001::End");
}

HWTEST_F(MediaLibraryDataManagerUnitTest, AddToMediaVisitCount_test_001, TestSize.Level0)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);

    MediaLibraryCommand cmdFile(OperationObject::FILESYSTEM_PHOTO,
        OperationType::QUERY_ORDER, MediaLibraryApi::API_10);
    int32_t ret = dataManager->OpenFile(cmdFile, "R");
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, AddToMediaVisitCount_test_002, TestSize.Level0)
{
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);

    MediaLibraryCommand cmdCustom(OperationObject::CUSTOM_RECORDS_OPERATION,
        OperationType::QUERY_ORDER, MediaLibraryApi::API_10);
    int32_t ret = dataManager->OpenFile(cmdCustom, "R");
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(MediaLibraryDataManagerUnitTest, RestoreInvalidHDCCloudDataPos_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("RestoreInvalidHDCCloudDataPos_test_001::Start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    ASSERT_NE(dataManager, nullptr);

    auto ret = dataManager->RestoreInvalidHDCCloudDataPos();
    EXPECT_EQ(ret, E_OK);
}
} // namespace Media
} // namespace OHOS
