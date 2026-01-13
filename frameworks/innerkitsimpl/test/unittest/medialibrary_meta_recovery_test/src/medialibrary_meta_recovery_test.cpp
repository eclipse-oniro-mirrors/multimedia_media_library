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

#include "medialibrary_uripermission_operations.h"
#include "media_log.h"

#include "fetch_result.h"
#include "get_self_permissions.h"
#include "media_file_utils.h"
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
#define private public
#include "medialibrary_meta_recovery.h"
#undef private
#include "media_upgrade.h"

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
    MEDIALIBRARY_TABLE,
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

// SetUp:Execute before each test case
void MediaLibraryMetaRecoveryUnitTest::SetUp(void) {}

void MediaLibraryMetaRecoveryUnitTest::TearDown(void) {}

//static inline bool IsExistFile(string filepath) { return bool(access(META_STATUS_PATH.c_str(), F_OK) == E_OK);}

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
    const std::string CLEAR_PHOTOS =
        "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int err = rdbStore->ExecuteSql(CLEAR_PHOTOS);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Fatal error! Failed to exec: %{public}s", CLEAR_PHOTOS.c_str());
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
    closeValues.PutString(MEDIA_DATA_DB_URI, fileUri.ToString());
    closeCmd.SetValueBucket(closeValues);
    auto ret = MediaLibraryPhotoOperations::Close(closeCmd);
    EXPECT_EQ(ret, 0);

    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_BACKING_UP;
    MediaLibraryMetaRecovery::GetInstance().DoBackupMetadata();
    MediaLibraryMetaRecovery::GetInstance().recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;

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
    EXPECT_EQ(access(META_RECOVERY_ALBUM_PATH.c_str(), F_OK), -1);
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
} // namespace Media
} // namespace OHOS
