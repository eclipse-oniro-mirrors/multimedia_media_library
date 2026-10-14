/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "medialibrary_meta_recovery_test.h"
#include "medialibrary_unittest_utils.h"

#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>

#include "medialibrary_uripermission_operations.h"
#include "media_log.h"

#include "fetch_result.h"
#include "get_self_permissions.h"
#include "media_file_utils.h"
#include "media_path_utils.h"
#include "media_smart_map_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unittest_utils.h"
#include "uri.h"
#include "photo_album_column.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "photo_map_column.h"
#include "scanner_utils.h"
#include "medialibrary_photo_operations.h"
#include "media_file_uri.h"
#include "photo_file_utils.h"
#include "medialibrary_db_const_sqls.h"
#include "media_upgrade.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_restore.h"
#include "file_asset.h"
#include "photo_album.h"
#include "rdb_helper.h"
#include "nlohmann/json.hpp"

#define private public
#include "medialibrary_meta_recovery.h"
#undef private

using namespace std;
using namespace testing::ext;

namespace {
    const string META_RECOVERY_PHOTO_RELATIVE_PATH = "/Photo/";
    const string META_RECOVERY_META_RELATIVE_PATH = "/.meta/Photo/";
    const string META_RECOVERY_META_FILE_SUFFIX = ".json";
    const string META_RECOVERY_ALBUM_PATH = "/storage/cloud/files/.meta/album.json";
    const string META_STATUS_PATH = "/storage/cloud/files/.meta/status.json";
}  // namespace

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_MEDIA_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
    CONST_MEDIALIBRARY_TABLE,
};

void MediaLibraryMetaRecoveryUnitTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);

    vector<string> perms;
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryMetaRecoveryUnitTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
}

int32_t ClearPhotoApi10();
void MediaLibraryMetaRecoveryUnitTest::TearDownTestCase(void)
{
    ClearPhotoApi10();
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryMetaRecoveryUnitTest::SetUp(void)
{
    // Each case starts a new background window; synchronous metadata work has already returned.
    MediaLibraryMetaRecovery::GetInstance().ResetRecoveryInterrupt();
}

void MediaLibraryMetaRecoveryUnitTest::TearDown(void) {}

int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
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

int32_t ClearPhotoApi10()
{
    std::string clearPhotos = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int err = rdbStore->ExecuteSql(clearPhotos);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", clearPhotos.c_str());
    }
    return err;
}

static const unsigned char FILE_CONTENT_JPG[] = {0xFF, 0xD8};

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
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

static int32_t fileId = 0;
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_Backup_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_Backup_001::Start");
    // open
    fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "backup1.jpg");
    EXPECT_TRUE(fileId > 0);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(fileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);

    // write
    int32_t resWrite = write(fd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    NativeRdb::ValuesBucket closeValues;
    closeValues.PutString(CONST_MEDIA_DATA_DB_URI, fileUri.ToString());
    closeCmd.SetValueBucket(closeValues);
    auto ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);

    auto &recovery = MediaLibraryMetaRecovery::GetInstance();
    recovery.ResetRecoveryInterrupt();
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    recovery.DoBackupMetadata();
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;

    // meta file exist
    string path = GetFilePath(fileId);
    string metaPath;
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath), 0);
    EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);

    // album.json exist
    remove(META_RECOVERY_ALBUM_PATH.c_str());
    const string META_RECOVERY_ALBUM_PATH = "/storage/cloud/files/.meta/album.json";
    auto result = MediaFileUtils::CreateFile(META_RECOVERY_ALBUM_PATH);
    EXPECT_EQ(result, true);
    EXPECT_EQ(access(META_RECOVERY_ALBUM_PATH.c_str(), F_OK), 0);
    result = MediaFileUtils::DeleteFile(META_RECOVERY_ALBUM_PATH);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("MetaRecovery_Backup_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_Backup_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_Backup_002::Start");
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
    const string META_RECOVERY_ALBUM_PATH = "/storage/cloud/files/.meta/album.json";
    remove(META_RECOVERY_ALBUM_PATH.c_str());

    string path = GetFilePath(fileId);
    string metaPath;
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath), 0);
    remove(metaPath.c_str());

    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().ResetAllMetaDirty(), 0);
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();
    sleep(1); // for async backup
    EXPECT_EQ(access(META_RECOVERY_ALBUM_PATH.c_str(), F_OK), 0);
    EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);
    MEDIA_INFO_LOG("MetaRecovery_Backup_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WriteSingleMetaDataById_Test_001, TestSize.Level1)
{
    auto ret = MediaLibraryMetaRecovery::GetInstance().WriteSingleMetaDataById(-1);
    EXPECT_EQ(ret, E_OK);

    string path = GetFilePath(fileId);
    string metaPath;
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath), 0);
    ret = MediaLibraryMetaRecovery::GetInstance().WriteSingleMetaDataById(fileId);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);
    remove(metaPath.c_str());
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_DeleteMetaDataByPath_Test_001, TestSize.Level1)
{
    int ret = E_OK;
    string photoPath = "/storage/cloud/files/Photo/test_file.jpg";
    string photoMetaPath = "/storage/cloud/files/.meta/Photo/test_file.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");
    MediaFileUtils::CreateFile(photoMetaPath);
    ret = MediaLibraryMetaRecovery::GetInstance().DeleteMetaDataByPath(photoPath);
    EXPECT_EQ(ret, E_OK);

    photoPath = "/storage/cloud/files/test_file.jpg";
    ret = MediaLibraryMetaRecovery::GetInstance().DeleteMetaDataByPath(photoPath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    remove(photoMetaPath.c_str());
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_SetRdbRebuiltStatus_Test_001, TestSize.Level1)
{
    int32_t ret = MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(true);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().rdbRebuilt_, true);
    ret = MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(false);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().rdbRebuilt_, false);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_StartAsyncRecovery_Test_001, TestSize.Level1)
{
    int32_t ret;

    // 1.test no need to recovery
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(false);
    ret = MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    EXPECT_EQ(ret, E_OK);

    // 2.test already running recovery thread
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING;
    ret = MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    EXPECT_EQ(ret, E_OK);

    // 3.test STATE_NONE to STATE_RECOVERING
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(true);
    ret = MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    EXPECT_EQ(ret, E_OK);
    sleep(2);

    // 4.test STATE_RECOVERING_ABORT SIG11
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT;
    ret = MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_AlbumRecovery_Test_001, TestSize.Level1)
{
    int ret = E_OK;
    const string albumMetaPath = "/storage/cloud/files/not_exist_file";
    remove(albumMetaPath.c_str());
    ret = MediaLibraryMetaRecovery::GetInstance().AlbumRecovery(albumMetaPath);
    EXPECT_NE(ret, E_OK);

    ret = MediaLibraryMetaRecovery::GetInstance().AlbumRecovery(META_RECOVERY_ALBUM_PATH);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_PhotoRecovery_Test_001, TestSize.Level1)
{
    int ret = E_OK;
    string photoMetaPath = "/storage/cloud/files/test_file";
    EXPECT_EQ(MediaLibraryUnitTestUtils::CreateFileFS(photoMetaPath.c_str()), true);
    ret = MediaLibraryMetaRecovery::GetInstance().PhotoRecovery(photoMetaPath);
    EXPECT_EQ(ret, E_INVALID_PATH);
    remove(photoMetaPath.c_str());

    photoMetaPath = "/storage/cloud/files/.meta/Photo";
    ret = MediaLibraryMetaRecovery::GetInstance().PhotoRecovery(photoMetaPath);
    EXPECT_EQ(ret, E_OK);
    ClearPhotoApi10();
    ret = MediaLibraryMetaRecovery::GetInstance().PhotoRecovery(photoMetaPath);
    EXPECT_EQ(ret, E_OK);

    // test no meta file
    remove(photoMetaPath.c_str());
    ret = MediaLibraryMetaRecovery::GetInstance().PhotoRecovery(photoMetaPath);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ScanMetaDir_Test_001, TestSize.Level1)
{
    int ret = E_OK;
    string photoMetaPath = "/storage/cloud/files/test_file";
    photoMetaPath.resize(FILENAME_MAX);
    ret = MediaLibraryMetaRecovery::GetInstance().ScanMetaDir(photoMetaPath, -1);
    EXPECT_EQ(ret, ERR_INCORRECT_PATH);

    photoMetaPath = "/storage/cloud/files/test_file";
    remove(photoMetaPath.c_str());
    ret = MediaLibraryMetaRecovery::GetInstance().ScanMetaDir(photoMetaPath, -1);
    EXPECT_EQ(ret, ERR_NOT_ACCESSIBLE);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_LoadAlbumMaps_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_LoadAlbumMaps_Test_001::Start");

    string albumMetaPath = "/storage/cloud/files/.meta/album.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetLPath("/storage/cloud/files/Pictures/Camera");
    vecPhotoAlbum.push_back(album1);

    MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumMetaPath, vecPhotoAlbum);

    MediaLibraryMetaRecovery::GetInstance().LoadAlbumMaps(albumMetaPath);

    EXPECT_FALSE(MediaLibraryMetaRecovery::GetInstance().oldAlbumIdToLpath.empty());
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().oldAlbumIdToLpath[1], "/storage/cloud/files/Pictures/Camera");

    remove(albumMetaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_LoadAlbumMaps_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WriteSingleMetaData_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_WriteSingleMetaData_Test_001::Start");

    int32_t newFileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "write_single_test.jpg");
    EXPECT_TRUE(newFileId > 0);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(newFileId), "", MEDIA_API_VERSION_V10);
    Uri uri(fileUri.ToString());
    MediaLibraryCommand openCmd(uri);
    int32_t fd = MediaLibraryPhotoOperations::Open(openCmd, "rw");
    EXPECT_GE(fd, 0);

    int32_t resWrite = write(fd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    EXPECT_GT(resWrite, 0);

    MediaLibraryCommand closeCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CLOSE);
    NativeRdb::ValuesBucket closeValues;
    closeValues.PutString(CONST_MEDIA_DATA_DB_URI, fileUri.ToString());
    closeCmd.SetValueBucket(closeValues);
    auto ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);

    auto asset = MediaLibraryAssetOperations::QuerySinglePhoto(newFileId);
    ASSERT_NE(asset, nullptr);

    int32_t writeRet = MediaLibraryMetaRecovery::GetInstance().WriteSingleMetaData(*asset);
    EXPECT_EQ(writeRet, E_OK);

    string path = GetFilePath(newFileId);
    string metaPath;
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath), 0);
    EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(newFileId));
    vector<string> columns = {PhotoColumn::PHOTO_METADATA_FLAGS};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    int32_t metadataFlags = GetInt32Val(PhotoColumn::PHOTO_METADATA_FLAGS, resultSet);
    EXPECT_EQ(metadataFlags, static_cast<int32_t>(MetadataFlags::TYPE_UPTODATE));

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_WriteSingleMetaData_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_InsertMetadataInDb_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_InsertMetadataInDb_Test_001::Start");

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/test_insert.jpg");
    testAsset.SetDisplayName("test_insert.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(123456789);
    testAsset.SetDateModified(123456789);
    testAsset.SetSize(1024);

    string metaPath = "/storage/cloud/files/.meta/Photo/test_insert.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");
    MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);

    ClearPhotoApi10();

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(testAsset);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/test_insert.jpg");
    vector<string> columns = {PhotoColumn::MEDIA_ID, MediaColumn::MEDIA_NAME};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    EXPECT_EQ(displayName, "test_insert.jpg");

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_InsertMetadataInDb_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_InsertMetadataInDbRetry_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_InsertMetadataInDbRetry_Test_001::Start");

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/test_retry.jpg");
    testAsset.SetDisplayName("test_retry.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(123456789);
    testAsset.SetDateModified(123456789);
    testAsset.SetSize(2048);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDbRetry(testAsset);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/test_retry.jpg");
    vector<string> columns = {PhotoColumn::MEDIA_ID};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    MEDIA_INFO_LOG("MetaRecovery_InsertMetadataInDbRetry_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_UpdatePhotoOwnerAlbumId_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_UpdatePhotoOwnerAlbumId_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().oldAlbumIdToLpath[100] = "/storage/cloud/files/Pictures/Camera";
    MediaLibraryMetaRecovery::GetInstance().lpathToNewAlbumId["/storage/cloud/files/Pictures/Camera"] = 200;

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, 100);

    bool ret = MediaLibraryMetaRecovery::GetInstance().UpdatePhotoOwnerAlbumId(values);
    EXPECT_TRUE(ret);

    NativeRdb::ValueObject valueObject;
    ASSERT_TRUE(values.GetObject(PhotoColumn::PHOTO_OWNER_ALBUM_ID, valueObject));

    int32_t newAlbumId = 0;
    valueObject.GetInt(newAlbumId);
    EXPECT_EQ(newAlbumId, 200);

    MEDIA_INFO_LOG("MetaRecovery_UpdatePhotoOwnerAlbumId_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_UpdatePhotoOwnerAlbumId_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_UpdatePhotoOwnerAlbumId_Test_002::Start");

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, 999);

    bool ret = MediaLibraryMetaRecovery::GetInstance().UpdatePhotoOwnerAlbumId(values);
    EXPECT_FALSE(ret);

    NativeRdb::ValuesBucket values2;
    values2.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, 100);

    MediaLibraryMetaRecovery::GetInstance().oldAlbumIdToLpath[100] = "/storage/cloud/files/Pictures/Camera";

    ret = MediaLibraryMetaRecovery::GetInstance().UpdatePhotoOwnerAlbumId(values2);
    EXPECT_TRUE(ret);

    MEDIA_INFO_LOG("MetaRecovery_UpdatePhotoOwnerAlbumId_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_InsertAlbumMetadataInDb_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_InsertAlbumMetadataInDb_Test_001::Start");

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("TestAlbum1");
    album1->SetLPath("/storage/cloud/files/Pictures/TestAlbum1");
    album1->SetDateAdded(123456789);
    album1->SetDateModified(123456789);
    album1->SetOrder(1);
    album1->SetBundleName("com.example.test");
    album1->SetLocalLanguage("en");
    album1->SetIsLocal(1);
    album1->SetPriority(0);
    album1->SetUploadStatus(0);
    vecPhotoAlbum.push_back(album1);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "TestAlbum1");
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME,
                           PhotoAlbumColumns::ALBUM_LPATH};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    string albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    string lpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    EXPECT_EQ(albumName, "TestAlbum1");
    EXPECT_EQ(lpath, "/storage/cloud/files/Pictures/TestAlbum1");

    MEDIA_INFO_LOG("MetaRecovery_InsertAlbumMetadataInDb_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_InsertAlbumMetadataInDb_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_InsertAlbumMetadataInDb_Test_002::Start");

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("DuplicateAlbum");
    album1->SetLPath("/storage/cloud/files/Pictures/DuplicateAlbum");
    album1->SetDateAdded(123456789);
    album1->SetDateModified(123456789);
    album1->SetOrder(1);
    album1->SetBundleName("com.example.test");
    album1->SetLocalLanguage("en");
    album1->SetIsLocal(1);
    album1->SetPriority(0);
    album1->SetUploadStatus(0);
    vecPhotoAlbum.push_back(album1);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);

    ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "DuplicateAlbum");
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    EXPECT_EQ(count, 1);

    MEDIA_INFO_LOG("MetaRecovery_InsertAlbumMetadataInDb_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WriteMetadataToFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_WriteMetadataToFile_Test_001::Start");

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/write_test.jpg");
    testAsset.SetDisplayName("write_test.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(123456789);
    testAsset.SetDateModified(123456789);
    testAsset.SetSize(4096);
    testAsset.SetWidth(1920);
    testAsset.SetHeight(1080);
    testAsset.SetMimeType("image/jpeg");

    string metaPath = "/storage/cloud/files/.meta/Photo/write_test.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);

    FileAsset readAsset;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadMetadataFromFile(metaPath, readAsset);
    EXPECT_EQ(ret, E_SYSCALL);
    EXPECT_EQ(readAsset.GetDisplayName(), "write_test.jpg");
    EXPECT_EQ(readAsset.GetSize(), 4096);
    EXPECT_EQ(readAsset.GetWidth(), 1920);
    EXPECT_EQ(readAsset.GetHeight(), 1080);

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_WriteMetadataToFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ReadMetadataFromFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ReadMetadataFromFile_Test_001::Start");

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/read_test.jpg");
    testAsset.SetDisplayName("read_test.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(987654321);
    testAsset.SetDateModified(987654321);
    testAsset.SetSize(8192);
    testAsset.SetWidth(2560);
    testAsset.SetHeight(1440);
    testAsset.SetMimeType("image/jpeg");
    testAsset.SetTitle("Read Test");
    testAsset.SetArtist("Test Artist");
    testAsset.SetAlbum("Test Album");

    string metaPath = "/storage/cloud/files/.meta/Photo/read_test.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);
    EXPECT_EQ(ret, E_OK);

    FileAsset readAsset;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadMetadataFromFile(metaPath, readAsset);
    EXPECT_EQ(ret, E_SYSCALL);
    EXPECT_EQ(readAsset.GetDisplayName(), "read_test.jpg");
    EXPECT_EQ(readAsset.GetSize(), 8192);
    EXPECT_EQ(readAsset.GetWidth(), 2560);
    EXPECT_EQ(readAsset.GetHeight(), 1440);
    EXPECT_EQ(readAsset.GetTitle(), "Read Test");
    EXPECT_EQ(readAsset.GetDateAdded(), 987654321);
    EXPECT_EQ(readAsset.GetDateModified(), 987654321);

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_ReadMetadataFromFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ReadMetadataFromFile_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ReadMetadataFromFile_Test_002::Start");

    FileAsset readAsset;
    string invalidPath = "/storage/cloud/files/.meta/Photo/nonexistent.json";

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().ReadMetadataFromFile(invalidPath, readAsset);
    EXPECT_NE(ret, E_OK);

    MEDIA_INFO_LOG("MetaRecovery_ReadMetadataFromFile_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WritePhotoAlbumToFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_WritePhotoAlbumToFile_Test_001::Start");

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(10);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("WriteTestAlbum");
    album1->SetLPath("/storage/cloud/files/Pictures/WriteTestAlbum");
    album1->SetDateAdded(111111111);
    album1->SetDateModified(111111111);
    album1->SetOrder(10);
    album1->SetBundleName("com.example.writetest");
    album1->SetLocalLanguage("zh");
    album1->SetIsLocal(1);
    album1->SetPriority(1);
    album1->SetUploadStatus(1);
    album1->SetContainsHidden(0);
    vecPhotoAlbum.push_back(album1);

    string albumPath = "/storage/cloud/files/.meta/album_write_test.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumPath, vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(access(albumPath.c_str(), F_OK), 0);

    vector<shared_ptr<PhotoAlbum>> readAlbums;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadPhotoAlbumFromFile(albumPath, readAlbums);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(readAlbums.size(), 1);
    EXPECT_EQ(readAlbums[0]->GetAlbumName(), "WriteTestAlbum");
    EXPECT_EQ(readAlbums[0]->GetLPath(), "/storage/cloud/files/Pictures/WriteTestAlbum");
    EXPECT_EQ(readAlbums[0]->GetBundleName(), "com.example.writetest");
    EXPECT_EQ(readAlbums[0]->GetLocalLanguage(), "zh");
    EXPECT_EQ(readAlbums[0]->GetOrder(), 10);

    remove(albumPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_WritePhotoAlbumToFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ReadPhotoAlbumFromFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ReadPhotoAlbumFromFile_Test_001::Start");

    vector<shared_ptr<PhotoAlbum>> readAlbums;
    string invalidPath = "/storage/cloud/files/.meta/nonexistent_album.json";

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().ReadPhotoAlbumFromFile(invalidPath, readAlbums);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("MetaRecovery_ReadPhotoAlbumFromFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_GetDataType_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_GetDataType_Test_001::Start");

    ResultSetDataType type1 = MediaLibraryMetaRecovery::GetInstance().GetDataType(PhotoColumn::MEDIA_ID);
    EXPECT_EQ(type1, TYPE_INT32);

    ResultSetDataType type2 = MediaLibraryMetaRecovery::GetInstance().GetDataType(PhotoColumn::MEDIA_DATE_ADDED);
    EXPECT_EQ(type2, TYPE_INT64);

    ResultSetDataType type3 = MediaLibraryMetaRecovery::GetInstance().GetDataType(PhotoColumn::MEDIA_FILE_PATH);
    EXPECT_EQ(type3, TYPE_STRING);

    ResultSetDataType type5 = MediaLibraryMetaRecovery::GetInstance().GetDataType("invalid_column");
    EXPECT_EQ(type5, TYPE_NULL);
    MEDIA_INFO_LOG("MetaRecovery_GetDataType_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WriteMetaStatusToFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_WriteMetaStatusToFile_Test_001::Start");

    string statusPath = "/storage/cloud/files/.meta/status.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WriteMetaStatusToFile("1", 10);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(access(statusPath.c_str(), F_OK), 0);

    ret = MediaLibraryMetaRecovery::GetInstance().WriteMetaStatusToFile("2", 20);
    EXPECT_EQ(ret, E_OK);

    ret = MediaLibraryMetaRecovery::GetInstance().WriteMetaStatusToFile("3", 30);
    EXPECT_EQ(ret, E_OK);

    set<int32_t> readStatus;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadMetaStatusFromFile(readStatus);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(readStatus.size(), 3);
    EXPECT_TRUE(readStatus.find(1) != readStatus.end());
    EXPECT_TRUE(readStatus.find(2) != readStatus.end());
    EXPECT_TRUE(readStatus.find(3) != readStatus.end());

    int32_t totalCount = MediaLibraryMetaRecovery::GetInstance().ReadMetaRecoveryCountFromFile();
    EXPECT_EQ(totalCount, 60);

    remove(statusPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_WriteMetaStatusToFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ReadMetaStatusFromFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ReadMetaStatusFromFile_Test_001::Start");

    set<int32_t> readStatus;
    string invalidPath = "/storage/cloud/files/.meta/nonexistent_status.json";

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().ReadMetaStatusFromFile(readStatus);
    EXPECT_NE(ret, E_OK);

    MEDIA_INFO_LOG("MetaRecovery_ReadMetaStatusFromFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_CheckRecoveryState_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_CheckRecoveryState_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();

    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load(),
        MediaLibraryMetaRecoveryState::STATE_NONE);

    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();

    MEDIA_INFO_LOG("MetaRecovery_CheckRecoveryState_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_InterruptRecovery_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_InterruptRecovery_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
    EXPECT_EQ(static_cast<int32_t>(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load()),
             static_cast<int32_t>(MediaLibraryMetaRecoveryState::STATE_BACKING_UP));
    EXPECT_TRUE(MediaLibraryMetaRecovery::GetInstance().cancelCurrentWork_.load());

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING;
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();

    MEDIA_INFO_LOG("MetaRecovery_InterruptRecovery_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_DoDataBaseRecovery_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_DoDataBaseRecovery_Test_001::Start");

    string albumMetaPath = "/storage/cloud/files/.meta/album.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("RecoveryTestAlbum");
    album1->SetLPath("/storage/cloud/files/Pictures/RecoveryTestAlbum");
    album1->SetDateAdded(222222222);
    album1->SetDateModified(222222222);
    album1->SetOrder(1);
    album1->SetBundleName("com.example.recovery");
    album1->SetLocalLanguage("en");
    album1->SetIsLocal(1);
    album1->SetPriority(0);
    album1->SetUploadStatus(0);
    vecPhotoAlbum.push_back(album1);

    MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumMetaPath, vecPhotoAlbum);

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);

    MediaLibraryMetaRecovery::GetInstance().DoDataBaseRecovery();

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "RecoveryTestAlbum");
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    remove(albumMetaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_DoDataBaseRecovery_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_AlbumRecovery_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_AlbumRecovery_Test_002::Start");

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);

    string albumMetaPath = "/storage/cloud/files/.meta/album_recovery.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("AlbumRecoveryTest");
    album1->SetLPath("/storage/cloud/files/Pictures/AlbumRecoveryTest");
    album1->SetDateAdded(333333333);
    album1->SetDateModified(333333333);
    album1->SetOrder(1);
    album1->SetBundleName("com.example.albumtest");
    album1->SetLocalLanguage("en");
    album1->SetIsLocal(1);
    album1->SetPriority(0);
    album1->SetUploadStatus(0);
    vecPhotoAlbum.push_back(album1);

    MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumMetaPath, vecPhotoAlbum);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().AlbumRecovery(albumMetaPath);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "AlbumRecoveryTest");
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    string albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    EXPECT_EQ(albumName, "AlbumRecoveryTest");

    remove(albumMetaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_AlbumRecovery_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_PhotoBackup_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_PhotoBackup_Test_001::Start");

    vector<shared_ptr<FileAsset>> photoVector;

    int32_t testFileId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "backup_test1.jpg");
    int32_t testFileId2 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "backup_test2.jpg");
    EXPECT_TRUE(testFileId1 > 0);
    EXPECT_TRUE(testFileId2 > 0);

    auto asset1 = MediaLibraryAssetOperations::QuerySinglePhoto(testFileId1);
    auto asset2 = MediaLibraryAssetOperations::QuerySinglePhoto(testFileId2);

    if (asset1) {
        photoVector.push_back(asset1);
    }
    if (asset2) {
        photoVector.push_back(asset2);
    }

    int32_t processCount = 0;
    int32_t successCount = 0;

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    MediaLibraryMetaRecovery::GetInstance().PhotoBackup(photoVector, processCount, successCount);

    EXPECT_EQ(processCount, 2);
    EXPECT_EQ(successCount, 2);

    for (auto &asset : photoVector) {
        if (asset) {
            string path = asset->GetFilePath();
            string metaPath;
            if (PhotoFileUtils::GetMetaPathFromOrignalPath(path, metaPath) == E_OK) {
                EXPECT_EQ(access(metaPath.c_str(), F_OK), 0);
                remove(metaPath.c_str());
            }
        }
    }

    MEDIA_INFO_LOG("MetaRecovery_PhotoBackup_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_PhotoBackup_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_PhotoBackup_Test_002::Start");

    vector<shared_ptr<FileAsset>> photoVector;

    shared_ptr<FileAsset> nullAsset = nullptr;
    photoVector.push_back(nullAsset);

    int32_t processCount = 0;
    int32_t successCount = 0;

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    MediaLibraryMetaRecovery::GetInstance().PhotoBackup(photoVector, processCount, successCount);

    EXPECT_EQ(processCount, 1);
    EXPECT_EQ(successCount, 0);

    MEDIA_INFO_LOG("MetaRecovery_PhotoBackup_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_StatisticSave_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_StatisticSave_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_ = 100;
    MediaLibraryMetaRecovery::GetInstance().backupCostTime_ = 5000;
    MediaLibraryMetaRecovery::GetInstance().reBuiltCount_ = 2;
    MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_ = 150;
    MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_ = 120;
    MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_ = 3000;

    MediaLibraryMetaRecovery::GetInstance().StatisticSave();

    MEDIA_INFO_LOG("MetaRecovery_StatisticSave_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_StatisticRestore_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_StatisticRestore_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_ = 200;
    MediaLibraryMetaRecovery::GetInstance().backupCostTime_ = 6000;
    MediaLibraryMetaRecovery::GetInstance().reBuiltCount_ = 3;
    MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_ = 250;
    MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_ = 220;
    MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_ = 4000;

    MediaLibraryMetaRecovery::GetInstance().StatisticSave();

    MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_ = 0;
    MediaLibraryMetaRecovery::GetInstance().backupCostTime_ = 0;
    MediaLibraryMetaRecovery::GetInstance().reBuiltCount_ = 0;
    MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_ = 0;
    MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_ = 0;
    MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_ = 0;

    MediaLibraryMetaRecovery::GetInstance().StatisticRestore();

    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_, 200);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupCostTime_, 6000);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_, 250);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_, 220);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_, 4000);

    MEDIA_INFO_LOG("MetaRecovery_StatisticRestore_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_StatisticReset_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_StatisticReset_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_ = 300;
    MediaLibraryMetaRecovery::GetInstance().backupCostTime_ = 7000;
    MediaLibraryMetaRecovery::GetInstance().reBuiltCount_ = 4;
    MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_ = 350;
    MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_ = 320;
    MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_ = 5000;

    MediaLibraryMetaRecovery::GetInstance().StatisticReset();

    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupCostTime_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().reBuiltCount_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_, 0);

    MEDIA_INFO_LOG("MetaRecovery_StatisticReset_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_RecoveryStatistic_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_RecoveryStatistic_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_ = 50;
    MediaLibraryMetaRecovery::GetInstance().backupCostTime_ = 2500;
    MediaLibraryMetaRecovery::GetInstance().reBuiltCount_ = 1;
    MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_ = 75;
    MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_ = 60;
    MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_ = 1500;

    MediaLibraryMetaRecovery::GetInstance().RecoveryStatistic();

    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupSuccCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().backupCostTime_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().reBuiltCount_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryTotalBackupCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoverySuccCnt_, 0);
    EXPECT_EQ(MediaLibraryMetaRecovery::GetInstance().recoveryCostTime_, 0);

    MEDIA_INFO_LOG("MetaRecovery_RecoveryStatistic_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_WriteJsonFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_WriteJsonFile_Test_001::Start");

    nlohmann::json testJson;
    testJson["key1"] = "value1";
    testJson["key2"] = 123;
    testJson["key3"] = 456.789;

    string testPath = "/storage/cloud/files/.meta/test_write.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    bool ret = MediaLibraryMetaRecovery::GetInstance().WriteJsonFile(testPath, testJson);
    EXPECT_TRUE(ret);
    EXPECT_EQ(access(testPath.c_str(), F_OK), 0);

    nlohmann::json readJson;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadJsonFile(testPath, readJson);
    EXPECT_TRUE(ret);
    EXPECT_EQ(readJson["key1"], "value1");
    EXPECT_EQ(readJson["key2"], 123);
    EXPECT_EQ(readJson["key3"], 456.789);

    remove(testPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_WriteJsonFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ReadJsonFile_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ReadJsonFile_Test_001::Start");

    nlohmann::json readJson;
    string invalidPath = "/storage/cloud/files/.meta/nonexistent.json";

    bool ret = MediaLibraryMetaRecovery::GetInstance().ReadJsonFile(invalidPath, readJson);
    EXPECT_FALSE(ret);

    MEDIA_INFO_LOG("MetaRecovery_ReadJsonFile_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_DeleteMetaDataByPath_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_DeleteMetaDataByPath_Test_002::Start");

    string photoPath = "/storage/cloud/files/Photo/delete_test.jpg";
    string photoMetaPath = "/storage/cloud/files/.meta/Photo/delete_test.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");
    MediaFileUtils::CreateFile(photoMetaPath);

    EXPECT_EQ(access(photoMetaPath.c_str(), F_OK), 0);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().DeleteMetaDataByPath(photoPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(access(photoMetaPath.c_str(), F_OK), -1);

    MEDIA_INFO_LOG("MetaRecovery_DeleteMetaDataByPath_Test_002::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_MultipleAlbums_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_MultipleAlbums_Test_001::Start");

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    for (int i = 0; i < 5; i++) {
        auto album = make_shared<PhotoAlbum>();
        album->SetAlbumId(i + 1);
        album->SetPhotoAlbumType(PhotoAlbumType::USER);
        album->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
        album->SetAlbumName("MultiAlbum" + to_string(i));
        album->SetLPath("/storage/cloud/files/Pictures/MultiAlbum" + to_string(i));
        album->SetDateAdded(999999999 + i);
        album->SetDateModified(999999999 + i);
        album->SetOrder(i);
        album->SetBundleName("com.example.multi");
        album->SetLocalLanguage("en");
        album->SetIsLocal(1);
        album->SetPriority(0);
        album->SetUploadStatus(0);
        vecPhotoAlbum.push_back(album);
    }

    string albumPath = "/storage/cloud/files/.meta/multi_albums.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumPath, vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);

    ret = MediaLibraryMetaRecovery::GetInstance().AlbumRecovery(albumPath);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    EXPECT_EQ(count, 5);

    remove(albumPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_MultipleAlbums_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_EdgeCases_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_EdgeCases_Test_001::Start");

    FileAsset emptyAsset;
    string emptyMetaPath = "/storage/cloud/files/.meta/Photo/empty_test.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(emptyMetaPath, emptyAsset);
    EXPECT_EQ(ret, E_OK);

    FileAsset readEmptyAsset;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadMetadataFromFile(emptyMetaPath, readEmptyAsset);
    EXPECT_EQ(ret, E_SYSCALL);

    remove(emptyMetaPath.c_str());

    FileAsset largeValueAsset;
    largeValueAsset.SetFilePath("/storage/cloud/files/Photo/large_test.jpg");
    largeValueAsset.SetDisplayName(string(1000, 'A'));
    largeValueAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    largeValueAsset.SetDateAdded(INT64_MAX);
    largeValueAsset.SetDateModified(INT64_MAX);
    largeValueAsset.SetSize(INT64_MAX);
    largeValueAsset.SetWidth(INT32_MAX);
    largeValueAsset.SetHeight(INT32_MAX);

    string largeMetaPath = "/storage/cloud/files/.meta/Photo/large_test.json";
    ret = MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(largeMetaPath, largeValueAsset);
    EXPECT_EQ(ret, E_OK);

    FileAsset readLargeAsset;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadMetadataFromFile(largeMetaPath, readLargeAsset);
    EXPECT_EQ(ret, E_SYSCALL);
    EXPECT_EQ(readLargeAsset.GetDisplayName().length(), 1000);
    EXPECT_EQ(readLargeAsset.GetDateAdded(), INT64_MAX);
    EXPECT_EQ(readLargeAsset.GetSize(), INT64_MAX);
    EXPECT_EQ(readLargeAsset.GetWidth(), INT32_MAX);

    remove(largeMetaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_EdgeCases_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ConcurrentAccess_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ConcurrentAccess_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;

    std::thread thread1([]() {
        MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();
    });

    std::thread thread2([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
    });

    thread1.join();
    thread2.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    MEDIA_INFO_LOG("MetaRecovery_ConcurrentAccess_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_ErrorHandling_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_ErrorHandling_Test_001::Start");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WriteSingleMetaDataById(-999);
    EXPECT_EQ(ret, E_OK);

    string invalidPath = "/invalid/path/to/file.jpg";
    ret = MediaLibraryMetaRecovery::GetInstance().DeleteMetaDataByPath(invalidPath);
    EXPECT_NE(ret, E_OK);

    string nonExistentAlbumPath = "/storage/cloud/files/.meta/nonexistent_album.json";
    vector<shared_ptr<PhotoAlbum>> emptyAlbums;
    ret = MediaLibraryMetaRecovery::GetInstance().ReadPhotoAlbumFromFile(nonExistentAlbumPath, emptyAlbums);
    EXPECT_NE(ret, E_OK);

    MEDIA_INFO_LOG("MetaRecovery_ErrorHandling_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_StateTransitions_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_StateTransitions_Test_001::Start");

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    EXPECT_EQ(static_cast<int32_t>(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load()),
             static_cast<int32_t>(MediaLibraryMetaRecoveryState::STATE_NONE));

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    EXPECT_EQ(static_cast<int32_t>(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load()),
             static_cast<int32_t>(MediaLibraryMetaRecoveryState::STATE_BACKING_UP));

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING;
    EXPECT_EQ(static_cast<int32_t>(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load()),
             static_cast<int32_t>(MediaLibraryMetaRecoveryState::STATE_RECOVERING));

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT;
    EXPECT_EQ(static_cast<int32_t>(MediaLibraryMetaRecovery::GetInstance().recoveryState_.load()),
             static_cast<int32_t>(MediaLibraryMetaRecoveryState::STATE_RECOVERING_ABORT));

    MEDIA_INFO_LOG("MetaRecovery_StateTransitions_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_DuplicateHandling_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_DuplicateHandling_Test_001::Start");

    ClearPhotoApi10();

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/duplicate_test.jpg");
    testAsset.SetDisplayName("duplicate_test.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(4444444444);
    testAsset.SetDateModified(4444444444);
    testAsset.SetSize(16384);

    string metaPath = "/storage/cloud/files/.meta/Photo/duplicate_test.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");
    MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);

    int32_t ret1 = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(testAsset);
    EXPECT_EQ(ret1, E_OK);

    int32_t ret2 = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDb(testAsset);
    EXPECT_EQ(ret2, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_NAME, "duplicate_test.jpg");
    vector<string> columns = {PhotoColumn::MEDIA_ID};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count++;
    }
    EXPECT_EQ(count, 1);

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_DuplicateHandling_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_RetryMechanism_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_RetryMechanism_Test_001::Start");

    FileAsset testAsset;
    testAsset.SetFilePath("/storage/cloud/files/Photo/retry_test.jpg");
    testAsset.SetDisplayName("retry_test.jpg");
    testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
    testAsset.SetDateAdded(5555555555);
    testAsset.SetDateModified(5555555555);
    testAsset.SetSize(32768);

    string metaPath = "/storage/cloud/files/.meta/Photo/retry_test.jpg.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta/Photo");
    MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().InsertMetadataInDbRetry(testAsset);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_NAME, "retry_test.jpg");
    vector<string> columns = {PhotoColumn::MEDIA_ID};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);

    remove(metaPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_RetryMechanism_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_Cleanup_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_Cleanup_Test_001::Start");

    string testMetaDir = "/storage/cloud/files/.meta/Photo/cleanup_test";
    MediaFileUtils::CreateDirectory(testMetaDir);

    for (int i = 0; i < 5; i++) {
        FileAsset testAsset;
        testAsset.SetFilePath("/storage/cloud/files/Photo/cleanup_test" + to_string(i) + ".jpg");
        testAsset.SetDisplayName("cleanup_test" + to_string(i) + ".jpg");
        testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
        testAsset.SetDateAdded(6666666666 + i * 1000);
        testAsset.SetDateModified(6666666666 + i * 1000);
        testAsset.SetSize(1024 * (i + 1));

        string metaPath = testMetaDir + "/cleanup_test" + to_string(i) + ".jpg.json";
        MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);
    }

    ClearPhotoApi10();

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING;
    int32_t ret = MediaLibraryMetaRecovery::GetInstance().ScanMetaDir(testMetaDir, -1);
    EXPECT_EQ(ret, E_OK);

    for (int i = 0; i < 5; i++) {
        string photoPath = "/storage/cloud/files/Photo/cleanup_test" + to_string(i) + ".jpg";
        MediaLibraryMetaRecovery::GetInstance().DeleteMetaDataByPath(photoPath);
    }

    for (int i = 0; i < 5; i++) {
        string metaPath = testMetaDir + "/cleanup_test" + to_string(i) + ".jpg.json";
        EXPECT_EQ(access(metaPath.c_str(), F_OK), -1);
    }

    MEDIA_INFO_LOG("MetaRecovery_Cleanup_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_Comprehensive_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_Comprehensive_Test_001::Start");

    const string CLEAR_ALBUMS = "DELETE FROM " + PhotoAlbumColumns::TABLE;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    rdbStore->ExecuteSql(CLEAR_ALBUMS);
    ClearPhotoApi10();

    vector<shared_ptr<PhotoAlbum>> vecPhotoAlbum;
    auto album1 = make_shared<PhotoAlbum>();
    album1->SetAlbumId(1);
    album1->SetPhotoAlbumType(PhotoAlbumType::USER);
    album1->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    album1->SetAlbumName("ComprehensiveTestAlbum");
    album1->SetLPath("/storage/cloud/files/Pictures/ComprehensiveTestAlbum");
    album1->SetDateAdded(7777777777);
    album1->SetDateModified(7777777777);
    album1->SetOrder(1);
    album1->SetBundleName("com.example.comprehensive");
    album1->SetLocalLanguage("en");
    album1->SetIsLocal(1);
    album1->SetPriority(0);
    album1->SetUploadStatus(0);
    vecPhotoAlbum.push_back(album1);

    string albumPath = "/storage/cloud/files/.meta/comprehensive_album.json";
    MediaFileUtils::CreateDirectory("/storage/cloud/files/.meta");

    int32_t ret = MediaLibraryMetaRecovery::GetInstance().WritePhotoAlbumToFile(albumPath, vecPhotoAlbum);
    EXPECT_EQ(ret, E_OK);

    ret = MediaLibraryMetaRecovery::GetInstance().AlbumRecovery(albumPath);
    EXPECT_EQ(ret, E_OK);

    string testMetaDir = "/storage/cloud/files/.meta/Photo/comprehensive_test";
    MediaFileUtils::CreateDirectory(testMetaDir);

    for (int i = 0; i < 3; i++) {
        FileAsset testAsset;
        testAsset.SetFilePath("/storage/cloud/files/Photo/comprehensive_test" + to_string(i) + ".jpg");
        testAsset.SetDisplayName("comprehensive_test" + to_string(i) + ".jpg");
        testAsset.SetMediaType(MEDIA_TYPE_IMAGE);
        testAsset.SetDateAdded(8888888888 + i * 1000);
        testAsset.SetDateModified(8888888888 + i * 1000);
        testAsset.SetSize(2048 * (i + 1));
        testAsset.SetWidth(1920);
        testAsset.SetHeight(1080);

        string metaPath = testMetaDir + "/comprehensive_test" + to_string(i) + ".jpg.json";
        MediaLibraryMetaRecovery::GetInstance().WriteMetadataToFile(metaPath, testAsset);
    }

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_RECOVERING;
    ret = MediaLibraryMetaRecovery::GetInstance().ScanMetaDir(testMetaDir, -1);
    EXPECT_EQ(ret, E_OK);

    NativeRdb::RdbPredicates albumPredicates(PhotoAlbumColumns::TABLE);
    albumPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "ComprehensiveTestAlbum");
    vector<string> albumColumns = {PhotoAlbumColumns::ALBUM_ID};
    auto albumResultSet = MediaLibraryRdbStore::QueryWithFilter(albumPredicates, albumColumns);
    ASSERT_NE(albumResultSet, nullptr);
    EXPECT_EQ(albumResultSet->GoToFirstRow(), NativeRdb::E_OK);

    NativeRdb::RdbPredicates photoPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> photoColumns = {PhotoColumn::MEDIA_ID};
    auto photoResultSet = MediaLibraryRdbStore::QueryWithFilter(photoPredicates, photoColumns);
    ASSERT_NE(photoResultSet, nullptr);

    remove(albumPath.c_str());
    MEDIA_INFO_LOG("MetaRecovery_Comprehensive_Test_001::End");
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetaRecovery_GetInstance_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MetaRecovery_GetInstance_Test_001::Start");

    MediaLibraryMetaRecovery &instance1 = MediaLibraryMetaRecovery::GetInstance();
    MediaLibraryMetaRecovery &instance2 = MediaLibraryMetaRecovery::GetInstance();

    EXPECT_EQ(&instance1, &instance2);

    MEDIA_INFO_LOG("MetaRecovery_GetInstance_Test_001::End");
}

class RecoveryReadinessCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        return store.Execute("CREATE TABLE PhotoAlbum(album_id INTEGER PRIMARY KEY)").first;
    }
    int OnUpgrade(NativeRdb::RdbStore &, int, int) override
    {
        return NativeRdb::E_OK;
    }
};

/** @tc.desc: Readiness uses an isolated database and both required tables, never the shared fixture. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, RecoveryReadiness_ActualDatabase, TestSize.Level1)
{
    const int32_t version = 1;
    NativeRdb::RdbStoreConfig config("/data/test/recovery-readiness-" + MediaFileUtils::GenerateUUID() + ".db");
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetAllowRebuild(true);
    RecoveryReadinessCallback callback;
    int error = E_OK;
    auto store = NativeRdb::RdbHelper::GetRdbStore(config, version, callback, error);
    ASSERT_EQ(error, E_OK);
    ASSERT_NE(store, nullptr);
    EXPECT_NE(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, version), E_OK); // Photos is missing.
    ASSERT_EQ(store->Execute("CREATE TABLE Photos(file_id INTEGER PRIMARY KEY)").first, E_OK);
    EXPECT_EQ(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, version), E_OK); // Empty is valid.
    EXPECT_EQ(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, version, false), E_OK);
    EXPECT_EQ(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, 0), NativeRdb::E_INVALID_ARGS);
    EXPECT_NE(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, version + 1), E_OK);
    ASSERT_EQ(store->Execute("DROP TABLE PhotoAlbum").first, E_OK);
    EXPECT_NE(MediaLibraryMetaRecovery::CheckDatabaseUsability(*store, version), E_OK);
    store.reset();
    EXPECT_EQ(NativeRdb::RdbHelper::DeleteRdbStore(config), E_OK);
}

static nlohmann::json RecoveryMetadataFixture()
{
    return {{"data", ROOT_MEDIA_DIR + "Photo/legacy.jpg"}, {"media_type", MEDIA_TYPE_IMAGE},
        {"size", 100}, {"date_modified", 7000}, {"hidden", 0}, {"date_trashed", 0}, {"date_deleted", 0},
        {"time_pending", 0}, {"position", 1}, {"cloud_id", ""}, {"file_source_type", 0}, {"is_temp", 0}};
}

/** @tc.desc: Legacy metadata needs no risk fields; explicit unsafe or malformed values remain rejected. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalMetadata_LegacyAndSafety, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    auto metadata = RecoveryMetadataFixture();
    FileAsset asset;
    ASSERT_TRUE(recovery.DecodeIncrementalMetadata(metadata, asset));
    EXPECT_TRUE(recovery.IsOrdinaryLocalMetadata(asset));
    for (const auto &key : {"hidden", "date_trashed", "date_deleted", "file_source_type"}) {
        auto missing = metadata;
        missing.erase(key);
        EXPECT_FALSE(recovery.DecodeIncrementalMetadata(missing, asset)) << key;
    }
    metadata["width"] = "invalid optional field";
    EXPECT_TRUE(recovery.DecodeIncrementalMetadata(metadata, asset));
    for (const auto &key : {"is_shared", "is_critical", "photo_risk_status", "hidden"}) {
        auto invalid = metadata;
        invalid[key] = "invalid safety field";
        EXPECT_FALSE(recovery.DecodeIncrementalMetadata(invalid, asset)) << key;
        invalid[key] = 4294967296LL;
        EXPECT_FALSE(recovery.DecodeIncrementalMetadata(invalid, asset)) << key;
    }
    for (const auto &[key, value] : std::map<std::string, int32_t>{{"is_shared", 1}, {"is_critical", 1},
        {"photo_risk_status", 2}, {"date_deleted", 1}, {"file_source_type", 1}, {"time_pending", 1}}) {
        auto unsafe = metadata;
        unsafe[key] = value;
        FileAsset decoded;
        ASSERT_TRUE(recovery.DecodeIncrementalMetadata(unsafe, decoded));
        EXPECT_FALSE(recovery.IsOrdinaryLocalMetadata(decoded)) << key;
    }
}

/** @tc.desc: Retain visibility and dates, but never replay stale IDs, cloud state or thumbnail caches. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalMetadata_IdentityAndValues, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    FileAsset asset;
    auto metadata = RecoveryMetadataFixture();
    metadata["is_critical"] = 0;
    metadata["hidden"] = 1;
    metadata["date_trashed"] = 1000;
    ASSERT_TRUE(recovery.DecodeIncrementalMetadata(metadata, asset));
    ASSERT_TRUE(recovery.IsOrdinaryLocalMetadata(asset));
    EXPECT_TRUE(recovery.IsMetadataMatchingAsset(asset.GetFilePath(), asset));
    EXPECT_FALSE(recovery.IsMetadataMatchingAsset(asset.GetFilePath() + ".other", asset));
    asset.SetMediaType(MEDIA_TYPE_VIDEO);
    EXPECT_FALSE(recovery.IsMetadataMatchingAsset(asset.GetFilePath(), asset));
    auto values = recovery.BuildRecoveryPhotoValues(asset);
    for (const auto &name : {"data", "hidden", "date_trashed", "owner_album_id", "detail_time", "date_year",
        "date_added_year"}) {
        EXPECT_TRUE(values.HasColumn(name)) << name;
    }
    for (const auto &name : {"file_id", "cloud_id", "dirty", "thumbnail_ready", "sync_status", "latitude",
        "longitude"}) {
        EXPECT_FALSE(values.HasColumn(name)) << name;
    }
    NativeRdb::ValueObject hidden, trashed;
    ASSERT_TRUE(values.GetObject("hidden", hidden));
    ASSERT_TRUE(values.GetObject("date_trashed", trashed));
    int32_t hiddenValue = 0;
    int64_t trashedValue = 0;
    EXPECT_EQ(hidden.GetInt(hiddenValue), E_OK);
    EXPECT_EQ(trashed.GetLong(trashedValue), E_OK);
    EXPECT_EQ(hiddenValue, 1);
    EXPECT_EQ(trashedValue, 1000);
}

/** @tc.desc: A legacy SOURCE album is created/reused by lpath; explicit sharing and ambiguous IDs are rejected. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalAlbum_LegacyMapping, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    const std::string lpath = "/Pictures/recovery-" + MediaFileUtils::GenerateUUID();
    nlohmann::json metadata = {{"album_id", 999999}, {"album_type", PhotoAlbumType::SOURCE},
        {"album_subtype", PhotoAlbumSubType::SOURCE_GENERIC}, {"lpath", lpath}};
    auto album = std::make_shared<PhotoAlbum>();
    ASSERT_TRUE(recovery.GetPhotoAlbumFromJson(metadata, *album));
    EXPECT_EQ(album->GetShareType(), SHARE_TYPE_NONEALBUM);
    ASSERT_EQ(recovery.EnsureAlbumAndMapId(album), E_OK);
    int32_t id = recovery.lpathToNewAlbumId.at(lpath);
    EXPECT_GT(id, 0);
    EXPECT_EQ(recovery.EnsureAlbumAndMapId(album), E_OK);
    EXPECT_EQ(recovery.lpathToNewAlbumId.at(lpath), id);
    album->SetLPath(lpath + "Other");
    EXPECT_NE(recovery.EnsureAlbumAndMapId(album), E_OK);
    album->SetLPath(lpath);
    album->SetShareType(SHARE_TYPE_SHAREALBUM);
    EXPECT_EQ(recovery.EnsureAlbumAndMapId(album), E_OPERATION_NOT_SUPPORT);
    metadata["share_type"] = "unknown";
    EXPECT_FALSE(recovery.GetPhotoAlbumFromJson(metadata, *album));
    EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM PhotoAlbum WHERE album_id=?", {id}), E_OK);
}

/** @tc.desc: Repair only a missing owner, preserve healthy fields, and never overwrite explicit deletion. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalPhoto_OwnerAndDeletion, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    auto album = std::make_shared<PhotoAlbum>();
    album->SetAlbumId(999999);
    album->SetLPath("/Pictures/recovery-" + MediaFileUtils::GenerateUUID());
    ASSERT_EQ(recovery.EnsureAlbumAndMapId(album), E_OK);
    FileAsset asset;
    asset.SetFilePath(ROOT_MEDIA_DIR + "Photo/" + MediaFileUtils::GenerateUUID() + ".jpg");
    asset.SetDisplayName("keep.jpg");
    asset.SetMediaType(MEDIA_TYPE_IMAGE);
    auto initial = recovery.BuildRecoveryPhotoValues(asset);
    int64_t initialId = 0;
    ASSERT_EQ(g_rdbStore->Insert(initialId, PhotoColumn::PHOTOS_TABLE, initial), E_OK);
    int32_t id = 0;
    bool hasAlbum = false;
    ASSERT_EQ(recovery.QueryPhotoRecoveryState(asset.GetFilePath(), id, hasAlbum), E_OK);
    ASSERT_GT(id, 0);
    EXPECT_FALSE(hasAlbum);
    asset.SetOwnerAlbumId(recovery.lpathToNewAlbumId.at(album->GetLPath()));
    asset.SetDisplayName("stale.jpg");
    EXPECT_EQ(recovery.WriteRecoveredPhotoRecord(asset, id), E_OK);
    EXPECT_EQ(recovery.RepairPhotoRecord(asset.GetFilePath()), E_OK); // No file/meta read for a healthy row.
    auto result = g_rdbStore->QuerySql("SELECT display_name FROM Photos WHERE file_id=?", std::vector<std::string>{
        std::to_string(id)});
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->GoToFirstRow(), E_OK);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, result), "keep.jpg");
    result->Close();
    EXPECT_EQ(g_rdbStore->ExecuteSql("UPDATE Photos SET date_deleted=1 WHERE file_id=?", {id}), E_OK);
    EXPECT_EQ(recovery.RepairPhotoRecord(asset.GetFilePath()), E_DELETE_DENIED);
    EXPECT_EQ(recovery.WriteRecoveredPhotoRecord(asset, id), E_DELETE_DENIED);
    EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM Photos WHERE file_id=?", {id}), E_OK);
    EXPECT_NE(recovery.WriteRecoveredPhotoRecord(asset, id), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM PhotoAlbum WHERE album_id=?", {asset.GetOwnerAlbumId()}), E_OK);
}

/** @tc.desc: Current sidecar deletion is terminal; no historical evidence directory can resurrect it. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalMetadata_DeletedOrMissingSidecar, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    const std::string original = ROOT_MEDIA_DIR + "Photo/" + MediaFileUtils::GenerateUUID() + ".jpg";
    std::string path;
    ASSERT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(original, path), E_OK);
    FileAsset asset;
    EXPECT_EQ(recovery.ReadIncrementalMetadata(original, asset), E_OPERATION_NOT_SUPPORT);
    auto metadata = RecoveryMetadataFixture();
    metadata["data"] = original;
    metadata["date_deleted"] = 1000;
    ASSERT_TRUE(recovery.WriteJsonFile(path, metadata));
    EXPECT_EQ(recovery.ReadIncrementalMetadata(original, asset), E_DELETE_DENIED);
    metadata["date_deleted"] = 0;
    metadata["dirty"] = static_cast<int32_t>(DirtyTypes::TYPE_DELETED);
    ASSERT_TRUE(recovery.WriteJsonFile(path, metadata));
    EXPECT_EQ(recovery.ReadIncrementalMetadata(original, asset), E_DELETE_DENIED);
    EXPECT_EQ(remove(path.c_str()), 0);
}

/** @tc.desc: Stop/reset belongs to the background owner; invalid/contended requests never enter recovery. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalRecovery_BackgroundLifecycle, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    recovery.incrementalStore_ = MediaLibraryRdbStore::GetRaw();
    ASSERT_NE(recovery.incrementalStore_, nullptr);
    recovery.recoveryRequest_ = {recovery.incrementalStore_->GetPath(), MEDIA_RDB_VERSION, "test-date"};
    EXPECT_EQ(recovery.CheckMetadataBackup(), NativeRdb::E_DATABASE_BUSY);
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_REPAIRING;
    recovery.InterruptRecovery();
    EXPECT_FALSE(recovery.IsIncrementalRecoveryAllowed());
    recovery.RecoverPendingRecords();
    EXPECT_EQ(recovery.recoveryRequest_.date, "test-date");
    EXPECT_NE(recovery.StartAsyncRecovery(), E_OK);
    recovery.CheckRecoveryState();
    EXPECT_EQ(recovery.recoveryState_.load(), MediaLibraryMetaRecoveryState::STATE_REPAIRING);
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    recovery.ResetRecoveryInterrupt();
    recovery.PrepareIncrementalRecovery(nullptr, "");
    EXPECT_EQ(recovery.recoveryRequest_.date, "test-date");
    std::lock_guard<std::mutex> lock(recovery.metadataMutex_);
    recovery.RecoverPendingRecords();
    EXPECT_EQ(recovery.recoveryRequest_.date, "test-date");
    EXPECT_EQ(recovery.recoveryState_.load(), MediaLibraryMetaRecoveryState::STATE_NONE);
}

static std::vector<NativeRdb::ValuesBucket> ProtectedMetadataRows(const std::string &prefix, int32_t count)
{
    std::vector<NativeRdb::ValuesBucket> rows;
    for (int32_t i = 0; i < count; ++i) {
        NativeRdb::ValuesBucket values;
        values.PutString(MediaColumn::MEDIA_FILE_PATH, prefix + "-" + std::to_string(i) + ".jpg");
        values.PutString(MediaColumn::MEDIA_NAME, "pending.jpg");
        values.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
        values.PutInt(PhotoColumn::PHOTO_POSITION, 1);
        values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int32_t>(MetadataFlags::TYPE_NEW));
        rows.push_back(values);
    }
    return rows;
}

static void ExpectProtectedMetadataRows(const std::string &prefix, int32_t expected)
{
    auto result = g_rdbStore->QuerySql("SELECT COUNT(*) FROM Photos WHERE data LIKE ? AND metadata_flags=?",
        std::vector<std::string>{prefix + "%", std::to_string(static_cast<int32_t>(MetadataFlags::TYPE_NEW))});
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->GoToFirstRow(), E_OK);
    int64_t count = 0;
    EXPECT_EQ(result->GetLong(0, count), E_OK);
    EXPECT_EQ(count, expected);
    result->Close();
}

/** @tc.desc: Protected rows remain dirty without starving later healthy metadata in the same window. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, MetadataBackup_ProtectedBatchDoesNotStarveHealthyRows, TestSize.Level1)
{
    for (int32_t protectedCount : {499, 500, 1000}) {
        const std::string prefix = ROOT_MEDIA_DIR + "Photo/0/recovery-batch-" + MediaFileUtils::GenerateUUID();
        auto rows = ProtectedMetadataRows(prefix, protectedCount);
        int64_t inserted = 0;
        ASSERT_EQ(g_rdbStore->BatchInsert(inserted, PhotoColumn::PHOTOS_TABLE, rows), E_OK);
        ASSERT_EQ(inserted, protectedCount);
        MediaLibraryMetaRecovery recovery;
        auto album = std::make_shared<PhotoAlbum>();
        album->SetAlbumId(999999);
        album->SetLPath("/Pictures/recovery-" + MediaFileUtils::GenerateUUID());
        ASSERT_EQ(recovery.EnsureAlbumAndMapId(album), E_OK);
        NativeRdb::ValuesBucket healthy = rows.back();
        const std::string healthyPath = prefix + "-healthy.jpg";
        healthy.PutString(MediaColumn::MEDIA_FILE_PATH, healthyPath);
        healthy.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, static_cast<int32_t>(MetadataFlags::TYPE_DIRTY));
        int32_t albumId = recovery.lpathToNewAlbumId.at(album->GetLPath());
        healthy.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
        int64_t healthyId = 0;
        ASSERT_EQ(g_rdbStore->Insert(healthyId, PhotoColumn::PHOTOS_TABLE, healthy), E_OK);
        auto store = MediaLibraryRdbStore::GetRaw();
        ASSERT_NE(store, nullptr);
        recovery.recoveryRequest_ = {store->GetPath(), MEDIA_RDB_VERSION, "test-date"};
        recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
        recovery.PhotoBackupBatch();
        auto photo = MediaLibraryAssetOperations::QuerySinglePhoto(healthyId);
        ASSERT_NE(photo, nullptr);
        EXPECT_EQ(photo->GetInt32Member(PhotoColumn::PHOTO_METADATA_FLAGS),
            static_cast<int32_t>(MetadataFlags::TYPE_UPTODATE));
        std::string metaPath;
        ASSERT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(healthyPath, metaPath), E_OK);
        EXPECT_EQ(access(metaPath.c_str(), R_OK), 0);
        ExpectProtectedMetadataRows(prefix, protectedCount);
        EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM Photos WHERE data LIKE ?", {prefix + "%"}), E_OK);
        EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM PhotoAlbum WHERE album_id=?", {albumId}), E_OK);
        EXPECT_EQ(remove(metaPath.c_str()), 0);
    }
}

/** @tc.desc: Hard DB errors remain failures even when recovery has stopped the current pass. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalRecovery_ReportErrorPrecedence, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    recovery.InterruptRecovery();
    recovery.recoveryReport_.failed = 1;
    // An empty DATE prevents event submission; this test checks classification and raw errors only.
    int64_t begin = MediaFileUtils::UTCTimeMilliSeconds();
    for (int32_t error : {NativeRdb::E_SQLITE_CORRUPT, NativeRdb::E_SQLITE_IOERR,
        NativeRdb::E_ALREADY_CLOSED, E_HAS_DB_ERROR}) {
        EXPECT_EQ(recovery.RecordRecoveryScanResult(error, begin), error);
        EXPECT_EQ(recovery.recoveryReport_.result, DatabaseRecoveryResult::FAILED);
        EXPECT_EQ(recovery.recoveryReport_.error, error);
        EXPECT_EQ(recovery.recoveryReport_.failed, 1);
    }
    for (int32_t error : {NativeRdb::E_DATABASE_BUSY, NativeRdb::E_SQLITE_BUSY, NativeRdb::E_SQLITE_LOCKED}) {
        EXPECT_EQ(recovery.RecordRecoveryScanResult(error, begin), error);
        EXPECT_EQ(recovery.recoveryReport_.result, DatabaseRecoveryResult::WAITING);
    }
    EXPECT_EQ(recovery.RecordRecoveryScanResult(E_ERR, begin), E_ERR);
    EXPECT_EQ(recovery.recoveryReport_.result, DatabaseRecoveryResult::PAUSED);
    recovery.ResetRecoveryInterrupt();
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_REPAIRING;
    recovery.recoveryReport_.failed = 0;
    recovery.recoveryReport_.deferred = 1;
    EXPECT_EQ(recovery.RecordRecoveryScanResult(E_OK, begin), E_OK);
    EXPECT_EQ(recovery.recoveryReport_.result, DatabaseRecoveryResult::PARTIAL);
}

/** @tc.desc: Resume by re-enumeration and committed rows, without a separate directory cursor or page cache. */
HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalProgress_CommittedRows, TestSize.Level1)
{
    MediaLibraryMetaRecovery recovery;
    const std::string cloud = ROOT_MEDIA_DIR + "Photo/recovery-" + MediaFileUtils::GenerateUUID() + "/test.jpg";
    const std::string local = MediaPathUtils::ConvertCloudPathToLocalPath(cloud);
    const std::string directory = MediaPathUtils::GetParentPath(local);
    ASSERT_TRUE(MediaPureFileUtils::CreateDirectory(directory));
    ASSERT_TRUE(MediaFileUtils::CreateFile(local));
    recovery.incrementalStore_ = MediaLibraryRdbStore::GetRaw();
    recovery.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_REPAIRING;
    EXPECT_EQ(recovery.RepairPhotoRecordsInDirectory(directory), E_OK);
    EXPECT_GT(recovery.recoveryReport_.deferred, 0);
    auto album = std::make_shared<PhotoAlbum>();
    album->SetAlbumId(999999);
    album->SetLPath("/Pictures/recovery-" + MediaFileUtils::GenerateUUID());
    ASSERT_EQ(recovery.EnsureAlbumAndMapId(album), E_OK);
    FileAsset asset;
    asset.SetFilePath(cloud);
    asset.SetOwnerAlbumId(recovery.lpathToNewAlbumId.at(album->GetLPath()));
    asset.SetMediaType(MEDIA_TYPE_IMAGE);
    ASSERT_EQ(recovery.WriteRecoveredPhotoRecord(asset, 0), E_OK);
    recovery.recoveryReport_ = {};
    EXPECT_EQ(recovery.RepairPhotoRecordsInDirectory(directory), E_OK);
    EXPECT_EQ(recovery.RepairPhotoRecordsInDirectory(directory), E_OK);
    EXPECT_EQ(recovery.recoveryReport_.repaired, 0);
    EXPECT_NE(recovery.RepairPhotoRecordsInDirectory(local), E_OK);
    recovery.InterruptRecovery();
    EXPECT_NE(recovery.RepairPhotoRecordsInDirectory(directory), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM Photos WHERE data=?", {cloud}), E_OK);
    EXPECT_EQ(g_rdbStore->ExecuteSql("DELETE FROM PhotoAlbum WHERE album_id=?", {asset.GetOwnerAlbumId()}), E_OK);
    EXPECT_EQ(remove(local.c_str()), 0);
    EXPECT_EQ(rmdir(directory.c_str()), 0);
}

} // namespace Media
} // namespace OHOS
