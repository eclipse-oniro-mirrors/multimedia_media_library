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
#define MLOG_TAG "AlbumCountCoverTest"

#include "album_count_cover_test.h"

#include <utility>

#include "fetch_result.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_business_record_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#define private public
#include "medialibrary_rdbstore.h"
#undef private
#include "album_operation_uri.h"
#include "media_upgrade.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
constexpr int32_t SLEEP_INTERVAL = 1;   // in seconds
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

const vector<bool> HIDDEN_STATE = {
    true,
    false
};

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

const string SQL_INSERT_ALBUM = "INSERT OR IGNORE INTO " + PhotoAlbumColumns::TABLE + "(" +
    PhotoAlbumColumns::ALBUM_ID + ", " + PhotoAlbumColumns::ALBUM_TYPE + ", " +
    PhotoAlbumColumns::ALBUM_SUBTYPE + ") ";

void InsertSystemAlbums()
{
    string insertImageAlbumSql = SQL_INSERT_ALBUM +  "VALUES (7, 1024, 1031)";
    string insertVideoAlbumSql = SQL_INSERT_ALBUM +  "VALUES (2, 1024, 1026)";
    string insertHiddenAlbumSql = SQL_INSERT_ALBUM +  "VALUES (3, 1024, 1027)";
    string insertTrashAlbumSql = SQL_INSERT_ALBUM +  "VALUES (4, 1024, 1028)";

    vector<string> insertSqls = {
        insertImageAlbumSql,
        insertVideoAlbumSql,
        insertHiddenAlbumSql,
        insertTrashAlbumSql
    };
    for (auto &sql : insertSqls) {
        int32_t ret = g_rdbStore->ExecuteSql(sql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{public}s failed", sql.c_str());
            return;
        }
    }
    return;
}

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

int32_t ResetSystemAlbums()
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
    ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_COUNT, 0);
    values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
    values.PutInt(PhotoAlbumColumns::HIDDEN_COUNT, 0);
    values.PutString(PhotoAlbumColumns::HIDDEN_COVER, "");
    values.PutInt(PhotoAlbumColumns::CONTAINS_HIDDEN, 0);
    values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, 0);
    values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, 0);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Update(rows, values, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to reset system album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t ClearEnv()
{
    ClearUserAlbums();
    ResetSystemAlbums();
    ClearTable(PhotoMap::TABLE);
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    return E_OK;
}

static int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
}


string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = MediaLibraryUnistoreManager::GetInstance().GetRdbStore()->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakePhotoUnpending(int fileId, bool isRefresh)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return E_INVALID_FILEID;
    }

    string path = GetFilePath(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }

    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = MediaLibraryUnistoreManager::GetInstance().GetRdbStore()->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    if (isRefresh) {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
            MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
        MediaLibraryRdbUtils::UpdateUserAlbumInternal(
            MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    }
    return E_OK;
}

int32_t SetDefaultPhotoApi10(int mediaType, const std::string &displayName, bool isFresh = true)
{
    int fileId = CreatePhotoApi10(mediaType, displayName);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create photo failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakePhotoUnpending(fileId, isFresh);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

void UpdateRefreshFlag()
{
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    MediaLibraryRdbUtils::UpdateUserAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
}

void CheckIfNeedRefresh(bool isNeedRefresh)
{
    bool signal = false;
    int32_t ret = MediaLibraryRdbUtils::IsNeedRefreshByCheckTable(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), signal);
    EXPECT_EQ(ret, 0);
    if (ret != 0) {
        MEDIA_ERR_LOG("IsNeedRefreshByCheckTable false");
        return;
    }
    EXPECT_EQ(isNeedRefresh, signal);

    EXPECT_EQ(true, MediaLibraryRdbUtils::IsNeedRefreshAlbum());
}

void RefreshTable()
{
    int32_t ret = MediaLibraryRdbUtils::RefreshAllAlbums(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(),
        [] (PhotoAlbumType type, PhotoAlbumSubType a, int b) {}, [] () {});
    EXPECT_EQ(ret, 0);
}

unique_ptr<PhotoAlbum> QueryAlbumInfo(const RdbPredicates &predicates, const bool hiddenOnly)
{
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, { });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query album info");
        return nullptr;
    }

    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    auto fetchResult = make_unique<FetchResult<PhotoAlbum>>(make_shared<DataShareResultSet>(bridge));
    EXPECT_NE(fetchResult, nullptr);
    if (fetchResult == nullptr) {
        MEDIA_ERR_LOG("Failed to get fetch result");
        return nullptr;
    }
    fetchResult->SetHiddenOnly(hiddenOnly);
    return fetchResult->GetFirstObject();
}

unique_ptr<FileAsset> QueryFileAssetInfo(const RdbPredicates &predicates)
{
    auto resultSet = g_rdbStore->Query(predicates, { });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query asset info");
        return nullptr;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    auto fetchResult = make_unique<FetchResult<FileAsset>>(make_shared<DataShareResultSet>(bridge));
    EXPECT_NE(fetchResult, nullptr);
    if (fetchResult == nullptr) {
        MEDIA_ERR_LOG("Failed to get fetch result");
        return nullptr;
    }
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return fetchResult->GetFirstObject();
}

unique_ptr<PhotoAlbum> QuerySystemAlbumInfo(const int32_t subtype, const bool hiddenOnly)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subtype));
    return QueryAlbumInfo(predicates, hiddenOnly);
}

unique_ptr<PhotoAlbum> QueryUserAlbumInfo(const int32_t albumId, const bool hiddenOnly)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return QueryAlbumInfo(predicates, hiddenOnly);
}

/**
 * @brief Query the album contains_hidden value
 *
 * @param albumId The album to query
 * @return Returns album contains_hidden:
 *   o 1: If contains_hidden is true
 *   o 0: If contains_hidden is false
 *   o negative values: If error occurs.
 */
int32_t QueryAlbumContainsHidden(const int32_t albumId)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    auto resultSet = g_rdbStore->Query(predicates, { PhotoAlbumColumns::CONTAINS_HIDDEN });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query asset info");
        return E_HAS_DB_ERROR;
    }
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int ret = E_FAIL;
    EXPECT_EQ(resultSet->GetInt(0, ret), NativeRdb::E_OK);
    return ret;
}

unique_ptr<FileAsset> QueryFileAssetInfo(const int32_t fileId)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    return QueryFileAssetInfo(predicates);
}

void AlbumInfo::CheckUserAlbum(const int32_t albumId) const
{
    for (const auto hiddenState : HIDDEN_STATE) {
        auto album = QueryUserAlbumInfo(albumId, hiddenState);
        ASSERT_NE(album, nullptr);
        MEDIA_ERR_LOG("Expect result %{public}d of user album.", album != nullptr);
        CheckAlbum(album, hiddenState);
    }
}

void AlbumInfo::CheckSystemAlbum(const int32_t subtype) const
{
    for (const auto hiddenState : HIDDEN_STATE) {
        auto album = QuerySystemAlbumInfo(subtype, hiddenState);
        ASSERT_NE(album, nullptr);
        MEDIA_ERR_LOG("Expect result %{public}d of system album.", album != nullptr);
        CheckAlbum(album, hiddenState);
    }
}

void AlbumInfo::CheckAlbum(const unique_ptr<PhotoAlbum> &album, const bool hiddenOnly) const
{
    EXPECT_EQ(album->GetCount(), hiddenOnly ? hiddenCount_ : count_);
    MEDIA_ERR_LOG("Expect result %{public}d of count, left: %{public}d, right: %{public}d",
        album->GetCount() == count_, album->GetCount(), count_);

    EXPECT_EQ(album->GetCoverUri(), hiddenOnly ? hiddenCover_ : cover_);
    MEDIA_ERR_LOG("Expect result %{public}d of cover, left: %{public}s, right: %{public}s",
        album->GetCoverUri() == cover_, album->GetCoverUri().c_str(), cover_.c_str());

    int32_t containsHidden = QueryAlbumContainsHidden(album->GetAlbumId());
    EXPECT_EQ(containsHidden, containsHidden_);
    MEDIA_ERR_LOG("Expect result %{public}d of contains_hidden, left: %{public}d, right: %{public}d",
        containsHidden == containsHidden_, containsHidden, containsHidden_);
}

unique_ptr<FileAsset> CreateImageAsset(const string &displayName, bool isFreshAlbum = true)
{
    int32_t fileId = SetDefaultPhotoApi10(MEDIA_TYPE_IMAGE, displayName, isFreshAlbum);
    MEDIA_ERR_LOG("Expect result %{public}d of CreateImageAsset, left: %{public}d, right: %{public}d",
        fileId > 0, fileId, 0);
    EXPECT_GT(fileId, 0);
    return QueryFileAssetInfo(fileId);
}

unique_ptr<FileAsset> CreateVideoAsset(const string &displayName, bool isFreshAlbum = true)
{
    int32_t fileId = SetDefaultPhotoApi10(MEDIA_TYPE_VIDEO, displayName, isFreshAlbum);
    MEDIA_ERR_LOG("Expect result %{public}d of CreateVideoAsset, left: %{public}d, right: %{public}d",
        fileId > 0, fileId, 0);
    EXPECT_GT(fileId, 0);
    return QueryFileAssetInfo(fileId);
}

int32_t TrashFileAsset(const unique_ptr<FileAsset> &fileAsset)
{
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());

    int32_t changedRows;
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::TRASH_PHOTO, MediaLibraryApi::API_10);
    changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(PhotoColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    auto resultSet = g_rdbStore->Query(rdbPredicates, {});
    int64_t dateTrashed;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dateTrashed = get<int64_t>(ResultSetUtils::GetValFromColumn("date_trashed", resultSet, TYPE_INT64));
    }
    resultSet->Close();
    EXPECT_NE(dateTrashed, 0);
    MEDIA_ERR_LOG("Expect date_trashed after TrashFileAsset, left: %{public} " PRId64 ", right: %{public}d",
        dateTrashed, 0);
    return changedRows;
}

int32_t RecoverFileAsset(const unique_ptr<FileAsset> &fileAsset)
{
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, 0);

    int32_t changedRows;
    MediaLibraryCommand cmd(
        OperationObject::PHOTO_ALBUM, OperationType::ALBUM_RECOVER_ASSETS, MediaLibraryApi::API_10);
    changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(PhotoColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    auto resultSet = g_rdbStore->Query(rdbPredicates, {});
    int64_t dateTrashed;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dateTrashed = get<int64_t>(ResultSetUtils::GetValFromColumn("date_trashed", resultSet, TYPE_INT64));
    }
    resultSet->Close();
    EXPECT_EQ(dateTrashed, 0);
    MEDIA_ERR_LOG("Expect date_trashed after RecoverFileAsset, left: %{public} " PRId64 ", right: %{public}d",
        dateTrashed, 0);
    return changedRows;
}

int32_t HideFileAsset(const unique_ptr<FileAsset> &fileAsset, bool hiddenState)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::HIDE, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_HIDDEN, hiddenState ? 1 : 0);
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
    EXPECT_EQ(changedRows, 1);
    MEDIA_ERR_LOG("Expect result %{public}d of HideFileAsset, left: %{public}d, right: %{public}d",
        changedRows == 1, changedRows, 1);
    return changedRows;
}

void BatchFavoriteFileAsset(const vector<string>& uriArray, bool favoriteState)
{
    DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, uriArray);
    DataShareValuesBucket values;
    values.Put(PhotoColumn::MEDIA_IS_FAV, favoriteState ? 1 : 0);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO,
        OperationType::BATCH_UPDATE_FAV, MediaLibraryApi::API_10);
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
    EXPECT_EQ(changedRows, uriArray.size());
}

int32_t FavoriteFileAsset(const int32_t fileId, bool favoriteState)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_IS_FAV, favoriteState ? 1 : 0);
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
    EXPECT_EQ(changedRows, 1);
    MEDIA_ERR_LOG("Expect result %{public}d of FavoriteFileAsset, left: %{public}d, right: %{public}d",
        changedRows == 1, changedRows, 1);
    return changedRows;
}

int32_t DeletePermanentlyFileAsset(const int32_t fileId)
{
    MediaLibraryCommand cmd((Uri(URI_DELETE_PHOTOS)));
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    DataShareValuesBucket values;
    values.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
    EXPECT_EQ(changedRows, 1);
    MEDIA_ERR_LOG("Expect result %{public}d of DeletePermanentlyFileAsset, left: %{public}d, right: %{public}d",
        changedRows == 1, changedRows, 1);
    return changedRows;
}

unique_ptr<PhotoAlbum> CreatePhotoAlbum(const string &albumName)
{
    MediaLibraryCommand cmd(OperationObject::PHOTO_ALBUM, OperationType::CREATE, MediaLibraryApi::API_10);
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    int32_t albumId = MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
    EXPECT_GT(albumId, 0);
    MEDIA_ERR_LOG("Expect result %{public}d of CreatePhotoAlbum, left: %{public}d, right: %{public}d",
        albumId > 0, albumId, 1);
    return QueryUserAlbumInfo(albumId, false);
}

int32_t AlbumAddAssets(unique_ptr<PhotoAlbum> &album, unique_ptr<FileAsset> &fileAsset)
{
    MediaLibraryCommand cmd((Uri(PAH_PHOTO_ALBUM_ADD_ASSET)));
    DataShareValuesBucket values;
    values.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, album->GetAlbumId());
    values.Put(MediaColumn::MEDIA_ID, fileAsset->GetUri());
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->BatchInsert(cmd, { values });
    EXPECT_EQ(changedRows, 1);
    MEDIA_ERR_LOG("Expect result %{public}d of AlbumAddAssets, left: %{public}d, right: %{public}d",
        changedRows == 1, changedRows, 1);
    return changedRows;
}


int32_t AlbumRemoveAssets(unique_ptr<PhotoAlbum> &album, unique_ptr<FileAsset> &fileAsset)
{
    MediaLibraryCommand cmd((Uri(URI_QUERY_PHOTO_MAP)));
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(album->GetAlbumId()));
    predicates.EqualTo(MediaColumn::MEDIA_ID, MediaFileUri::GetPhotoId(fileAsset->GetUri()));
    int32_t changedRows = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(changedRows, 1);
    MEDIA_ERR_LOG("Expect result %{public}d of AlbumRemoveAssets, left: %{public}d, right: %{public}d",
        changedRows == 1, changedRows, 1);
    return changedRows;
}

int32_t InitPhotoTrigger()
{
    MEDIA_INFO_LOG("start Init Photo Trigger");
    static const vector<string> executeSqlStrs = {
        PhotoUpgrade::INDEX_SCTHP_ADDTIME,
        PhotoUpgrade::CREATE_SCHPT_MEDIA_TYPE_INDEX,
        PhotoUpgrade::CREATE_SCHPT_HIDDEN_TIME_INDEX,
        PhotoUpgrade::CREATE_PHOTO_FAVORITE_INDEX
    };
    MEDIA_INFO_LOG("start Init Photo Trigger");
    
    int32_t err = E_OK;
    for (const auto &sql : executeSqlStrs) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void AlbumCountCoverTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ClearEnv();
    InsertSystemAlbums();
    InitPhotoTrigger();
}

void AlbumCountCoverTest::TearDownTestCase()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void AlbumCountCoverTest::SetUp() {}

void AlbumCountCoverTest::TearDown() {}

/**
 * @tc.name: album_count_cover_001
 * @tc.desc: Basic tool functions of this test
 *  1. Create asset/album.
 *  2. Album add/remove photos.
 *  3. Trash/Hide/(Batch)Favorite/Delete photos.
 * @tc.type: FUNC
 * @tc.require: issueI89E9N
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_001, TestSize.Level2)
{
    MEDIA_ERR_LOG("album_count_cover_001 start");
    ClearEnv();

    MEDIA_INFO_LOG("Test create asset begin");
    auto fileAsset = CreateImageAsset("Test_Create_Image_001.jpg");
    EXPECT_NE(fileAsset, nullptr);
    auto fileAsset2 = CreateVideoAsset("Test_Create_Video_001.mp4");
    EXPECT_NE(fileAsset2, nullptr);
    MEDIA_INFO_LOG("Test create asset end");
    EXPECT_NE(fileAsset, nullptr);

    MEDIA_INFO_LOG("Test create album begin");
    auto album = CreatePhotoAlbum("Test_PhotoAlbum_001");
    MEDIA_INFO_LOG("Test create album end");
    EXPECT_NE(album, nullptr);

    MEDIA_INFO_LOG("Test album add assets begin");
    AlbumAddAssets(album, fileAsset);
    MEDIA_INFO_LOG("Test album add assets end");

    MEDIA_INFO_LOG("Test album remove assets begin");
    AlbumRemoveAssets(album, fileAsset);
    MEDIA_INFO_LOG("Test album remove assets end");

    MEDIA_INFO_LOG("Test trash asset begin");
    TrashFileAsset(fileAsset);
    RecoverFileAsset(fileAsset);
    MEDIA_INFO_LOG("Test trash asset end");

    MEDIA_INFO_LOG("Test hide asset begin");
    HideFileAsset(fileAsset, true);
    HideFileAsset(fileAsset, false);
    MEDIA_INFO_LOG("Test hide asset end");

    MEDIA_INFO_LOG("Test favorite asset begin");
    FavoriteFileAsset(fileAsset->GetId(), true);
    FavoriteFileAsset(fileAsset->GetId(), false);
    MEDIA_INFO_LOG("Test favorite asset end");

    MEDIA_INFO_LOG("Test batchFavorite asset begin");
    vector<string> fileAssetUriArray = { fileAsset->GetUri(), fileAsset2->GetUri() };
    BatchFavoriteFileAsset(fileAssetUriArray, true);
    BatchFavoriteFileAsset(fileAssetUriArray, false);
    MEDIA_INFO_LOG("Test batchFavorite asset end");

    TrashFileAsset(fileAsset);
    DeletePermanentlyFileAsset(fileAsset->GetId());

    MEDIA_ERR_LOG("album_count_cover_001 end");
}

/**
 * @tc.name: album_count_cover_002
 * @tc.desc: Hidden albums count and cover test
 *  1. Query hidden album info, count should be 0, cover should be empty.
 *  2. Create a photo, and then hide it.
 *  3. Create another photo, and then hide it. The cover of hidden album should be updated.
 *  4. Un-hide the cover photo, the count & cover of hidden album should be updated.
 *  5. Un-hide all the photo, the count & cover of hidden album should be reset.
 * @tc.type: FUNC
 * @tc.require: issueI89E9N
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_002, TestSize.Level2)
{
    MEDIA_INFO_LOG("album_count_cover_002 begin");
    // Clear all tables.
    MEDIA_INFO_LOG("Clear all tables.");
    ClearEnv();

    // 1. Query hidden album info, count should be 0, cover should be empty
    MEDIA_INFO_LOG("Check initialized info of hidden album");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 2. Create a photo, and then hide it.
    MEDIA_INFO_LOG("Create a photo, and then hide it.");
    auto fileAsset = CreateImageAsset("Test_Hidden_Image_001.jpg");
    EXPECT_NE(fileAsset, nullptr);
    HideFileAsset(fileAsset, true);
    AlbumInfo(1, fileAsset->GetUri(), 1, fileAsset->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 3. Create another photo, and then hide it. The cover of hidden album should be updated.
    sleep(SLEEP_INTERVAL);
    MEDIA_INFO_LOG("Create another photo, and then hide it.");
    auto fileAsset2 = CreateImageAsset("Test_Create_Image_002.jpg");
    EXPECT_NE(fileAsset2, nullptr);
    HideFileAsset(fileAsset2, true);
    AlbumInfo(2, fileAsset2->GetUri(), 2, fileAsset2->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 4. Un-hide the cover photo, the count & cover of hidden album should be updated.
    HideFileAsset(fileAsset2, false);
    AlbumInfo(1, fileAsset->GetUri(), 1, fileAsset->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 5. Un-hide all the photo, the count & cover of hidden album should be reset.
    HideFileAsset(fileAsset, false);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);
    MEDIA_INFO_LOG("album_count_cover_002 end");
}

/**
 * @tc.name: album_count_cover_003
 * @tc.desc: Trash album count and cover test.
 *   1. Query trash album info, count should be 0, cover should be empty.
 *   2. Create a photo, and then trash it.
 *   3. Create another photo, and then trash it. The cover of trash album should be updated.
 *   4. Un-trash the cover photo, the count & cover of trash album should be updated.
 *   5. Delete a photo permanently.
 *   6. Un-trash all photos, the count & cover of trash album should be reset.
 * @tc.type: FUNC
 * @tc.require: issueI89E9N
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_003, TestSize.Level2)
{
    MEDIA_INFO_LOG("album_count_cover_003 begin");
    // Clear all tables.
    MEDIA_INFO_LOG("Step: Clear all tables.");
    ClearEnv();

    // 1. Query trash album info, count should be 0, cover should be empty
    MEDIA_INFO_LOG("Step: Check initialized info of trash album");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 2. Create a photo, and then trash it.
    MEDIA_INFO_LOG("Step: Create a photo, and then trash it.");
    auto fileAsset = CreateImageAsset("Test_Trash_Image_001.jpg");
    ASSERT_NE(fileAsset, nullptr);

    TrashFileAsset(fileAsset);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 3. Create another photo, and then trash it. The cover of trash album should be updated.
    sleep(SLEEP_INTERVAL);
    MEDIA_INFO_LOG("Step: Create another photo, and then trash it.");
    auto fileAsset2 = CreateImageAsset("Test_Create_Image_002.jpg");
    ASSERT_NE(fileAsset2, nullptr);

    TrashFileAsset(fileAsset2);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 4. Un-trash the cover photo, the count & cover of trash album should be updated.
    MEDIA_INFO_LOG("Step: un-trash the cover photo");
    TrashFileAsset(fileAsset2);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 5. Delete a photo permanently.
    TrashFileAsset(fileAsset2);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("Step: delete a photo permanently");
    DeletePermanentlyFileAsset(fileAsset2->GetId());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 6. Un-trash all photos, the count & cover of trash album should be reset.
    MEDIA_INFO_LOG("Step: un-trash all photos");
    RecoverFileAsset(fileAsset);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("album_count_cover_003 end");
}

/**
 * @tc.name: album_count_cover_004
 * @tc.desc: Images album count and cover test.
 *   1. Query album info, count should be 0, cover should be empty.
 *   2. Create a photo.
 *   3. Create another photo.
 *   4. Hide a photo.
 *   5. Un-hide a photo.
 *   6. Trash a photo.
 *   7. Un-trash a photo.
 * @tc.type: FUNC
 * @tc.require: issueI89E9N
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_004, TestSize.Level2)
{
    MEDIA_INFO_LOG("album_count_cover_004 begin");
    // Clear all tables.
    MEDIA_INFO_LOG("Step: Clear all tables.");
    ClearEnv();

    // 1. Query album info, count should be 0, cover should be empty
    MEDIA_INFO_LOG("Step: Check initialized info of system albums");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 2. Create a photo.
    MEDIA_INFO_LOG("Step: Create a photo.");
    auto fileAsset = CreateImageAsset("Test_Images_001.jpg");
    EXPECT_NE(fileAsset, nullptr);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);

    // 3. Create another photo.
    sleep(SLEEP_INTERVAL);
    MEDIA_INFO_LOG("Step: Create another photo.");
    auto fileAsset2 = CreateImageAsset("Test_Images_002.jpg");
    EXPECT_NE(fileAsset2, nullptr);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);

    // 4. Hide a photo.
    MEDIA_INFO_LOG("Step: Hide a photo");
    HideFileAsset(fileAsset2, true);
    AlbumInfo(1, fileAsset->GetUri(), 1, fileAsset2->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);
    AlbumInfo(1, fileAsset2->GetUri(), 1, fileAsset2->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 5. Un-hide a photo.
    MEDIA_INFO_LOG("Step: Un-hide a photo");
    HideFileAsset(fileAsset2, false);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 6. Trash a photo.
    MEDIA_INFO_LOG("Step: Trash a photo");
    TrashFileAsset(fileAsset2);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);
    AlbumInfo(1, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 7. Un-trash a photo.
    MEDIA_INFO_LOG("Step: Un-trash a photo");
    RecoverFileAsset(fileAsset2);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::IMAGE);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("album_count_cover_004 end");
}

/**
 * @tc.name: album_count_cover_007
 * @tc.desc: Video album count and cover test
 *  1. Query album info, count should be 0, cover should be empty
 *  2. Create a photo.
 *  3. Create another photo.
 *  4. Hide a photo.
 *  5. Un-hide a photo.
 *  6. Trash a photo.
 *  7. Un-trash a photo.
 * @tc.type: FUNC
 * @tc.require: issueI89E9N
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_007, TestSize.Level2)
{
    MEDIA_INFO_LOG("album_count_cover_007 begin");
    // Clear all tables.
    MEDIA_INFO_LOG("Step: Clear all tables.");
    ClearEnv();

    // 1. Query album info, count should be 0, cover should be empty
    MEDIA_INFO_LOG("Step: Check initialized info of system albums");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 2. Create a photo.
    MEDIA_INFO_LOG("Step: Create a video.");
    auto fileAsset = CreateVideoAsset("Test_Videos_001.mp4");
    EXPECT_NE(fileAsset, nullptr);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);

    // 3. Create another photo.
    sleep(SLEEP_INTERVAL);
    MEDIA_INFO_LOG("Step: Create another photo.");
    auto fileAsset2 = CreateVideoAsset("Test_Videos_002.mp4");
    EXPECT_NE(fileAsset2, nullptr);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);

    // 4. Hide a photo.
    MEDIA_INFO_LOG("Step: Hide a photo");
    HideFileAsset(fileAsset2, true);
    AlbumInfo(1, fileAsset->GetUri(), 1, fileAsset2->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(1, fileAsset2->GetUri(), 1, fileAsset2->GetUri(), 1).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 5. Un-hide a photo.
    MEDIA_INFO_LOG("Step: Un-hide a photo");
    HideFileAsset(fileAsset2, false);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);

    // 6. Trash a photo.
    MEDIA_INFO_LOG("Step: Trash a photo");
    TrashFileAsset(fileAsset2);
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(1, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 7. Un-trash a photo.
    MEDIA_INFO_LOG("Step: Un-trash a photo");
    RecoverFileAsset(fileAsset2);
    AlbumInfo(2, fileAsset2->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);
    MEDIA_INFO_LOG("album_count_cover_007 end");
}

/**
 * @tc.name: album_count_cover_008
 * @tc.desc: Sys system refresh test.
 *   1. Check initialized info of system albums.
 *   2. Create a video but not fresh.
 *   3. Only set refresh table but not set flags.
 *   4. SetFlags but not refresh.
 *   5. Check if is need refresh.
 *   6. Refresh and query again.
 * @tc.type: FUNC
 * @tc.require: issueI8YPJA
 */
HWTEST_F(AlbumCountCoverTest, album_count_cover_008, TestSize.Level2)
{
    MEDIA_INFO_LOG("album_count_cover_008 begin");
    // Clear all tables.
    MEDIA_INFO_LOG("Step: Clear all tables.");
    ClearEnv();

    // 1. Query album info, count should be 0, cover should be empty
    MEDIA_INFO_LOG("Step: Check initialized info of system albums.");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::HIDDEN);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::TRASH);

    // 2. Create a photo but not fresh.
    MEDIA_INFO_LOG("Step: Create a video but not fresh.");
    auto fileAsset = CreateVideoAsset("Test_Videos_001.mp4", false);
    ASSERT_NE(fileAsset, nullptr);

    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);

    // 3. Only set refresh table but not set flags.
    MEDIA_INFO_LOG("Step: Only set refresh table but not set flags.");
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);

    // 4. SetFlags but not refresh.
    MEDIA_INFO_LOG("Step: SetFlags but not refresh.");
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    AlbumInfo(0, "", 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);

    // 5. Check if is need refresh.
    MEDIA_INFO_LOG("Step: Check if is need refresh.");
    CheckIfNeedRefresh(false);

    // 6. Refresh and query again.
    MEDIA_INFO_LOG("Step: Refresh and query again.");
    UpdateRefreshFlag();
    AlbumInfo(1, fileAsset->GetUri(), 0, "", 0).CheckSystemAlbum(PhotoAlbumSubType::VIDEO);
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(false);
}

int QueryCountForBussinessTable()
{
    RdbPredicates predicates(MedialibraryBusinessRecordColumn::TABLE);
    auto resultSet = g_rdbStore->Query(predicates, { MedialibraryBusinessRecordColumn::BUSINESS_TYPE });
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query bussiness table");
        return 0;
    }
    int32_t count = 0;
    resultSet->GetRowCount(count);
    return count;
}

HWTEST_F(AlbumCountCoverTest, refresh_analysis_album001, TestSize.Level2)
{
    MEDIA_INFO_LOG("refresh_analysis_album001 begin");
    MediaLibraryRdbUtils::UpdateAllAlbumsCountForCloud(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    int count = QueryCountForBussinessTable();
    EXPECT_GT(count, 0);
}

HWTEST_F(AlbumCountCoverTest, refresh_analysis_album002, TestSize.Level2)
{
    MEDIA_INFO_LOG("refresh_analysis_album002 begin");
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    RefreshTable();
    int count = QueryCountForBussinessTable();
    EXPECT_EQ(count, 0);
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(false);
}
} // namespace OHOS::Media
