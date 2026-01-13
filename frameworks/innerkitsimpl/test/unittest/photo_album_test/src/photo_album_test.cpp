/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "medialibrary_command.h"
#define MLOG_TAG "PhotoAlbumTest"

#include "photo_album_test.h"

#include "fetch_result.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::string URI_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_CREATE;
const std::string URI_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_UPDATE;
const std::string URI_ORDER_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM;
constexpr int32_t WAIT_TIME = 3;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
};

int32_t ClearTable(const string &table)
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

int32_t ClearUserAlbums()
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

inline void CheckColumn(shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet, const string &column,
    ResultSetDataType type, const variant<int32_t, string, int64_t, double> &expected)
{
    EXPECT_EQ(ResultSetUtils::GetValFromColumn(column, resultSet, type), expected);
}

inline int32_t CreatePhotoAlbum(const string &albumName)
{
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    Uri uri(URI_CREATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
}

inline int32_t DeletePhotoAlbum(DataSharePredicates &predicates)
{
    Uri uri(URI_CREATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri, OperationType::DELETE);
    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

inline string GetLastDentry(const string &path)
{
    string dentry = path;
    size_t slashIndex = path.rfind('/');
    if (slashIndex != string::npos) {
        dentry = path.substr(slashIndex + 1);
    }
    return dentry;
}

void DoCheckAlbumData(const string &name, const bool isRelativePath)
{
    string albumName = isRelativePath ? GetLastDentry(name) : name;
    string relativePath = isRelativePath ? name : "";

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    if (isRelativePath) {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_RELATIVE_PATH, relativePath);
    } else {
        predicates.IsNull(PhotoAlbumColumns::ALBUM_RELATIVE_PATH);
    }

    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_TYPE,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_RELATIVE_PATH,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
    };

    shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to get count! err: %{public}d", ret);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to GoToFirstRow! err: %{public}d", ret);

    int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
        TYPE_INT32));
    EXPECT_GT(albumId, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_TYPE, TYPE_INT32, PhotoAlbumType::USER);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SUBTYPE, TYPE_INT32, PhotoAlbumSubType::USER_GENERIC);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_NAME, TYPE_STRING, albumName);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_COVER_URI, TYPE_STRING, "");
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_COUNT, TYPE_INT32, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_RELATIVE_PATH, TYPE_STRING, relativePath);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_IMAGE_COUNT, TYPE_INT32, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_VIDEO_COUNT, TYPE_INT32, 0);
}

inline void CheckAlbumData(const string &albumName)
{
    DoCheckAlbumData(albumName, false);
}

inline void CreatePhotoAlbumAndCheck(const string &albumName)
{
    MEDIA_INFO_LOG("Creating album with albumName: %{public}s", albumName.c_str());
    EXPECT_GT(CreatePhotoAlbum(albumName), 0);
    CheckAlbumData(albumName);
}

int32_t QueryAlbumById(int32_t id, string &albumName, string &cover)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(id));
    auto resultSet = g_rdbStore->Query(predicates, { });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t count = -1;
    CHECK_AND_RETURN_RET(resultSet->GetRowCount(count) == E_OK, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    CHECK_AND_RETURN_RET(resultSet->GoToFirstRow() == E_OK, E_HAS_DB_ERROR);
    albumName = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME, resultSet, TYPE_STRING));
    cover = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet, TYPE_STRING));
    return E_OK;
}

inline int32_t UpdatePhotoAlbum(const DataShareValuesBucket &values, const DataSharePredicates &predicates)
{
    Uri uri(URI_UPDATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri, OperationType::UPDATE);
    return MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
}

inline int32_t OrderAlbums(const DataShareValuesBucket &values, const DataSharePredicates &predicates)
{
    Uri uri(URI_ORDER_ALBUM);
    MediaLibraryCommand cmd(uri, OperationType::ALBUM_ORDER);
    return MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
}

void CheckUpdatedAlbum(int32_t albumId, const string &expectedName, const string &expectedCover)
{
    string coverUri;
    string albumName;
    int32_t ret = QueryAlbumById(albumId, albumName, coverUri);
    if (ret < 0) {
        return;
    }
    EXPECT_EQ(albumName, expectedName);
    EXPECT_EQ(coverUri, expectedCover);
}

void CheckUpdatedSystemAlbum(PhotoAlbumSubType subType, const string &expectedName, const string &expectedCover)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subType));

    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_COVER_URI,
    };

    auto resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = -1;
    CHECK_AND_RETURN_LOG(resultSet->GetRowCount(count) == E_OK, "Failed to get count!");
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == E_OK, "Failed to GoToFirstRow!");
    string albumName = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME, resultSet,
        TYPE_STRING));
    string cover = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet,
        TYPE_STRING));
    EXPECT_EQ(albumName, expectedName);
    EXPECT_EQ(cover, expectedCover);
}

int32_t GetAlbumOrder(int32_t albumId)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    auto resultSet = g_rdbStore->Query(predicates, { });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t count = -1;
    CHECK_AND_RETURN_RET(resultSet->GetRowCount(count) == E_OK, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    CHECK_AND_RETURN_RET(resultSet->GoToFirstRow() == E_OK, E_HAS_DB_ERROR);
    int32_t albumOrder = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ORDER,
        resultSet, TYPE_INT32));
    EXPECT_GE(albumOrder, 0);
    return albumOrder;
}

void GetMaxAlbumOrder(int32_t &maxAlbumOrder)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    auto resultSet = g_rdbStore->Query(predicates, { "Max(album_order)" });
    int32_t ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to GoToFirstRow! err: %{public}d", ret);
    resultSet->GetInt(0, maxAlbumOrder);
}

void PhotoAlbumTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ClearUserAlbums();
    ClearTable(PhotoMap::TABLE);
}

void PhotoAlbumTest::TearDownTestCase()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
}

// SetUp:Execute before each test case
void PhotoAlbumTest::SetUp() {}

void PhotoAlbumTest::TearDown() {}

/**
 * @tc.name: photoalbum_create_album_001
 * @tc.desc: Create photo albums test
 *           1. Create an album called "photoalbum_create_album_001"
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_001 enter");
    CreatePhotoAlbumAndCheck("photoalbum_create_album_001");
    MEDIA_INFO_LOG("photoalbum_create_album_001 exit");
}

/**
 * @tc.name: photoalbum_create_album_002
 * @tc.desc: Create photo albums test
 *           1. Create an album with special characters
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_002 enter");
    string albumName = "photoalbum_create_album_002";
    const static string ALBUM_NAME_REGEX = R"([\.\\/:*?"'`<>|{}\[\]])";
    for (const auto &ch : ALBUM_NAME_REGEX) {
        albumName.append(1, ch);
        EXPECT_EQ(CreatePhotoAlbum(albumName), -EINVAL);
        albumName.pop_back();
    }
    MEDIA_INFO_LOG("photoalbum_create_album_002 exit");
}

/**
 * @tc.name: photoalbum_create_album_003
 * @tc.desc: Create an album which name contains dot
 *           1. Create an album with a leading dot
 *           2. Create an album end with a dot
 *           3. Create an album contains dot in the middle way
 *           4. Create an album with two leading dots
 *           5. Create an album contains two dots in the middle way
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_003 enter");
    const vector<string> testAlbumNames = {
        ".photoalbum_create_album_003",
        "photoalbum_create_album_003.",
        "photoalbum_.create_album_003",
        "..photoalbum_create_album_003",
        "photoalbum_.create._album_003"
    };

    for (const auto &albumName : testAlbumNames) {
        MEDIA_INFO_LOG("Creating album: %{public}s", albumName.c_str());
        EXPECT_EQ(CreatePhotoAlbum(albumName), -EINVAL);
    }
    MEDIA_INFO_LOG("photoalbum_create_album_003 exit");
}

/**
 * @tc.name: photoalbum_create_album_004
 * @tc.desc: Create photo albums test
 *           1. Create albums with several super long names
 *           2. Create an album with an empty name
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_004 enter");
    constexpr size_t displayNameMax = 255;
    string albumName(displayNameMax, 'a');
    CreatePhotoAlbumAndCheck(albumName);
    albumName.resize(0);
    albumName.resize(displayNameMax - 1, 'b');
    CreatePhotoAlbumAndCheck(albumName);
    albumName.resize(0);
    albumName.resize(displayNameMax + 1, 'c');
    EXPECT_EQ(CreatePhotoAlbum(albumName), -ENAMETOOLONG);
    albumName.resize(0);
    EXPECT_EQ(CreatePhotoAlbum(albumName), -EINVAL);
    MEDIA_INFO_LOG("photoalbum_create_album_004 exit");
}

/**
 * @tc.name: photoalbum_create_album_005
 * @tc.desc: Create an existed album
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_005 enter");
    const string albumName = "photoalbum_create_album_005";
    CreatePhotoAlbumAndCheck(albumName);
    EXPECT_EQ(CreatePhotoAlbum(albumName), -1);
    EXPECT_EQ(CreatePhotoAlbum(albumName), -1);
    EXPECT_EQ(CreatePhotoAlbum(albumName), -1);
    MEDIA_INFO_LOG("photoalbum_create_album_005 exit");
}

/**
 * @tc.name: photoalbum_create_album_006
 * @tc.desc: Create an mark album
 * @tc.type: FUNC
 * @tc.require: issueI97YYD
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_006 enter");
    const string albumName = "photoalbum_create_album_006";
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.Put(PhotoAlbumColumns::ALBUM_IS_LOCAL, 1);
    Uri uri(URI_CREATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri);
    int32_t result = MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
    EXPECT_GT(result, 0);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_IS_LOCAL, 1);

    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_IS_LOCAL,
    };

    shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, columns);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to get count! err: %{public}d", ret);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to GoToFirstRow! err: %{public}d", ret);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_IS_LOCAL, TYPE_INT32, 1);
    MEDIA_INFO_LOG("photoalbum_create_album_006 exit");
}

/**
 * @tc.name: photoalbum_create_album_007
 * @tc.desc: Create an album with the same album name as the existed deleted album
 * @tc.type: FUNC
 * @tc.require: issueI9KEDW
 */
HWTEST_F(PhotoAlbumTest, photoalbum_create_album_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_007 enter");
    const string albumName = "photoalbum_create_album_007";
    string insertSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + " (" + PhotoAlbumColumns::ALBUM_TYPE + ", " +
        PhotoAlbumColumns::ALBUM_SUBTYPE + ", " + PhotoAlbumColumns::ALBUM_NAME + ", " +
        PhotoAlbumColumns::ALBUM_DIRTY + ") VALUES (" + to_string(static_cast<int32_t>(PhotoAlbumType::USER)) + ", " +
        to_string(static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC)) + ", '" + albumName + "', " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED)) + ")";
    int32_t insertRet = g_rdbStore->ExecuteSql(insertSql);
    ASSERT_EQ(insertRet, E_OK);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    const vector<string> columns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_TYPE,
        PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumColumns::ALBUM_COUNT, PhotoAlbumColumns::ALBUM_DIRTY };
    shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to get count! err: %{public}d", ret);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    EXPECT_GT(count, 0);
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to GoToFirstRow! err: %{public}d", ret);

    int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
        TYPE_INT32));
    EXPECT_GT(albumId, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_TYPE, TYPE_INT32, PhotoAlbumType::USER);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SUBTYPE, TYPE_INT32, PhotoAlbumSubType::USER_GENERIC);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_COUNT, TYPE_INT32, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_DIRTY, TYPE_INT32, static_cast<int32_t>(DirtyTypes::TYPE_DELETED));

    EXPECT_GT(CreatePhotoAlbum(albumName), 0); // creation succeeded for the first time
    EXPECT_EQ(CreatePhotoAlbum(albumName), -1); // creation failed because of the newly created album
    EXPECT_EQ(CreatePhotoAlbum(albumName), -1); // creation failed because of the newly created album
    MEDIA_INFO_LOG("photoalbum_create_album_007 exit");
}

/**
 * @tc.name: photoalbum_delete_album_001
 * @tc.desc: Delete a photo album.
 *           1. Create an album and then delete it.
 * @tc.type: FUNC
 * @tc.require: issueI6O6FE
 */
HWTEST_F(PhotoAlbumTest, photoalbum_delete_album_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_delete_album_001 enter");

    const vector<string> albumNames = {
        "photoalbum_delete_album_001_001",
        "photoalbum_delete_album_001_002",
        "photoalbum_delete_album_001_003",
        "photoalbum_delete_album_001_004",
        "photoalbum_delete_album_001_005",
    };

    vector<string> albumIds;
    for (const auto &albumName : albumNames) {
        int32_t albumId = CreatePhotoAlbum(albumName);
        ASSERT_GT(albumId, 0);
        albumIds.push_back(to_string(albumId));
    }
    DataSharePredicates predicates;
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    EXPECT_EQ(DeletePhotoAlbum(predicates), albumNames.size());
    MEDIA_INFO_LOG("photoalbum_delete_album_001 end");
}

/**
 * @tc.name: photoalbum_delete_album_001
 * @tc.desc: Update photo album info.
 *           1. Rename an album
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_update_album_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_update_album_001 enter");
    const string albumName = "photoalbum_update_album_001";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    DataShareValuesBucket values;
    const string newName = "NewAlbumName1";
    const string newCover = "file://media/asset/10";
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, newCover);
    values.Put(PhotoAlbumColumns::ALBUM_NAME, newName);

    constexpr int32_t changedRows = 1;
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), changedRows);
    CheckUpdatedAlbum(albumId, newName, "");

    MEDIA_INFO_LOG("photoalbum_update_album_001 end");
}

/**
 * @tc.name: photoalbum_delete_album_002
 * @tc.desc: Update photo album info.
 *           1. Update coverUri
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_update_album_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_update_album_002 enter");
    const string albumName = "photoalbum_update_album_002";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    DataShareValuesBucket values;
    const string newCover = "file://media/asset/10";
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, newCover);
    constexpr int32_t changedRows = 1;
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), changedRows);
    CheckUpdatedAlbum(albumId, albumName, newCover);

    MEDIA_INFO_LOG("photoalbum_update_album_002 end");
}

/**
 * @tc.name: photoalbum_update_album_004
 * @tc.desc: Update photo album info.
 *           1. Update albumName and coverUri
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_update_album_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_update_album_003 enter");
    const string albumName = "photoalbum_update_album_003";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    // Build value buckets
    const string newName = "NewAlbumName3";
    const string newCover = "file://media/asset/10";
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, newName);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, newCover);

    // Build predicates
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    constexpr int32_t changedRows = 1;
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), changedRows);
    CheckUpdatedAlbum(albumId, newName, newCover);

    MEDIA_INFO_LOG("photoalbum_update_album_003 end");
}

/**
 * @tc.name: photoalbum_update_album_004
 * @tc.desc: Update photo album info.
 *           1. Update with empty values, this should return an error.
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_update_album_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_update_album_004 enter");
    const string albumName = "photoalbum_update_album_004";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    // Build empty values
    DataShareValuesBucket values;

    // Build predicates
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), E_INVALID_VALUES);
    CheckUpdatedAlbum(albumId, albumName, "");

    MEDIA_INFO_LOG("photoalbum_update_album_004 end");
}

/**
 * @tc.name: photoalbum_order_album_006
 * @tc.desc: order photo album.
 *           move current album before reference album
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_order_album_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_order_album_006 enter");

    // Build empty values
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_ID, 2);
    values.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, 5); // 2\5:album_id
    // Try to update favorite system
    DataSharePredicates predicates;
    EXPECT_NE(OrderAlbums(values, predicates), 0);
    int32_t currentOrder = GetAlbumOrder(2);
    MEDIA_INFO_LOG("current order is %{public}d", currentOrder);
    int32_t referenceOrder = GetAlbumOrder(5);
    MEDIA_INFO_LOG("reference order is %{public}d", referenceOrder);
    EXPECT_LE(currentOrder, referenceOrder);
    MEDIA_INFO_LOG("photoalbum_order_album_006 end");
}

/**
 * @tc.name: photoalbum_order_album_007
 * @tc.desc: repeat order same photo album, to see if order deranged.
 *           move current album before reference album
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_order_album_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_order_album_007 enter");

    // Build empty values
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_ID, 2); // 2\5:album_id
    values.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, 5);
    // Try to update favorite system
    DataSharePredicates predicates;
    EXPECT_NE(OrderAlbums(values, predicates), 0);
    int32_t currentOrder = GetAlbumOrder(2);
    MEDIA_INFO_LOG("current order is %{public}d", currentOrder);
    int32_t referenceOrder = GetAlbumOrder(5);
    MEDIA_INFO_LOG("reference order is %{public}d", referenceOrder);
    EXPECT_LE(currentOrder, referenceOrder);
    MEDIA_INFO_LOG("photoalbum_order_album_007 end");
}

/**
 * @tc.name: photoalbum_order_album_007
 * @tc.desc: order photo album, move the album to the end.
 *           move current album before reference album
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_order_album_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_order_album_008 enter");

    // Build empty values
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_ID, 3); // 3:album_id
    values.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, -1);
    // Try to update favorite system
    DataSharePredicates predicates;
    EXPECT_NE(OrderAlbums(values, predicates), 0);
    int32_t currentOrder = GetAlbumOrder(3);
    int32_t maxAlbumOrder = -1;
    GetMaxAlbumOrder(maxAlbumOrder);
    EXPECT_EQ(currentOrder, maxAlbumOrder);
    MEDIA_INFO_LOG("photoalbum_order_album_008 end");
}

/**
 * @tc.name: photoalbum_order_album_009
 * @tc.desc: order photo album, move the album to the end.
 *           move current album before reference album
 * @tc.type: FUNC
 * @tc.require: issueI6P7NG
 */
HWTEST_F(PhotoAlbumTest, photoalbum_order_album_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_order_album_009 enter");

    // Build empty values
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_ID, 5); // 5\2:  album_id
    values.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, 2);
    // Try to update favorite system
    DataSharePredicates predicates;
    EXPECT_NE(OrderAlbums(values, predicates), 0);
    int32_t currentOrder = GetAlbumOrder(2);
    int32_t referenceOrder = GetAlbumOrder(5);
    EXPECT_GE(currentOrder, referenceOrder);
    MEDIA_INFO_LOG("photoalbum_order_album_009 end");
}
} // namespace OHOS::Media
