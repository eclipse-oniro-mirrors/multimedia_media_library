/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "FuseUnitTest"

#include "medialibrary_fuse_test.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

#include "media_fuse_high_daemon.h"
#include "media_fuse_manager.h"
#include "medialibrary_unittest_utils.h"
#include "mimetype_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "datashare_predicates_objects.h"
#include "medialibrary_appstate_observer.h"
#include "medialibrary_rdb_transaction.h"
#include "media_file_uri.h"
#include "rdb_utils.h"
#include "datashare_predicates.h"
#include "abs_rdb_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_db_const.h"
#include "medialibrary_inotify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "photo_album_column.h"
#include "medialibrary_app_uri_permission_operations.h"
#include "permission_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_object_utils.h"
#include "parameter.h"
#include "heif_transcoding_check_utils.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using namespace OHOS::RdbDataShareAdapter;
using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const char* PARAM_PRODUCT_MODEL = "const.product.model";
static constexpr int32_t PARAM_PRODUCT_MODEL_LENGTH = 512;
static constexpr int32_t TEST_UID = 60000;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
unordered_map<string, bool> fuseTestPermsMap = {
    { PERM_READ_IMAGEVIDEO, 1 },
    { PERM_WRITE_IMAGEVIDEO, 1 }
};

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        AppUriPermissionColumn::APP_URI_PERMISSION_TABLE,
        AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

struct UniqueMemberValuesBucket {
    string assetMediaType;
    int32_t startNumber;
};

void PrepareUniqueNumberTable()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get g_rdbstore");
        return;
    }
    auto store = g_rdbStore;
    if (store == nullptr) {
        MEDIA_ERR_LOG("can not get store");
        return;
    }
    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = store->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("AssetUniqueNumberTable is already inited");
        return;
    }

    UniqueMemberValuesBucket imageBucket = { IMAGE_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket videoBucket = { VIDEO_ASSET_TYPE, 1 };
    UniqueMemberValuesBucket audioBucket = { AUDIO_ASSET_TYPE, 1 };

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {
        imageBucket, videoBucket, audioBucket
    };

    for (const auto& uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = store->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            MEDIA_ERR_LOG("Prepare smartAlbum failed");
        }
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE,
        AppUriSensitiveColumn::CREATE_APP_URI_SENSITIVE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
    PrepareUniqueNumberTable();
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

std::string GetSystemProductModel()
{
    char param[PARAM_PRODUCT_MODEL_LENGTH] = {0};
    if (GetParameter(PARAM_PRODUCT_MODEL, "", param, PARAM_PRODUCT_MODEL_LENGTH) > 0) {
        return param;
    }
    return "";
}

void MediaLibraryFuseTest::SetUpTestCase()
{
    MediaFuseManager::GetInstance();
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryFuseTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void MediaLibraryFuseTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

void MediaLibraryFuseTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryFuseTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void MediaLibraryFuseTest::TearDown() {}

int32_t GetPathFromFileId(string &path, const string &fileId)
{
    path = "/Photo/" + fileId + "/IMG_1729841527_000/" + "photo.jpg";
    return 0;
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
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

int32_t MakePhotoUnpending(int fileId, bool isMovingPhoto = false)
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

    if (isMovingPhoto) {
        string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
        errCode = MediaFileUtils::CreateAsset(videoPath);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Can not create video asset");
            return errCode;
        }
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
    errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

int32_t CreatePhotoApi10(int mediaType, const string &displayName)
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

    int32_t errCode = MakePhotoUnpending(ret);
    if (errCode != E_OK) {
        return errCode;
    }
    return ret;
}

static int32_t GetFileMtimeTest(const string &filePath, time_t &mtime)
{
    MEDIA_INFO_LOG("run GetFileMtime_Test start,mtime %{public}" PRId64, mtime);
    struct stat statInfo {};
    if (stat(filePath.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("Get file mtime failed, path = %{private}s", filePath.c_str());
        return E_ERR;
    }
    mtime = statInfo.st_mtime;
    MEDIA_INFO_LOG("run GetFileMtime_Test end,mtime %{public}" PRId64, mtime);
    return E_OK;
}

void CreatAndPush()
{
    system("mkdir -p /storage/cloud/files/Photo/666");
    system("touch /storage/cloud/files/Photo/666/heif.heic");
    system("mkdir -p /storage/cloud/files/.editData/Photo/666/heif.heic");
    system("touch /storage/cloud/files/.editData/Photo/666/heif.heic/transcode.jpg");
    time_t mtime = 0;
    GetFileMtimeTest("/storage/cloud/files/Photo/666/heif.heic", mtime);
    
    MEDIA_INFO_LOG("Test images pushed successfully");
}

void SetTestTables()
{
    std::string createTableSql = R"S(
        CREATE TABLE IF NOT EXISTS Photos (
            file_id INTEGER PRIMARY KEY AUTOINCREMENT,
            data TEXT NOT NULL,
            size BIGINT,
            title TEXT,
            display_name TEXT,
            media_type INT,
            position INT DEFAULT 1,
            is_temp INT DEFAULT 0,
            time_pending BIGINT DEFAULT 0,
            hidden INT DEFAULT 0,
            date_trashed BIGINT DEFAULT 0,
            transcode_time BIGINT DEFAULT 0,
            trans_code_file_size BIGINT NOT NULL DEFAULT 0,
            storage_path TEXT,
            file_source_type INT DEFAULT 0
        )
    )S";

    int32_t dbRet = g_rdbStore->ExecuteSql(createTableSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK);
    MEDIA_INFO_LOG("Create table result: %{public}d", dbRet);
}

static void InsertHeifAsset()
{
    std::string insertSql = R"S(INSERT INTO Photos(file_id, data, size, title, display_name, media_type, position,
        is_temp, time_pending, hidden, date_trashed, transcode_time, trans_code_file_size, exist_compatible_duplicate)
        VALUES (666, '/storage/cloud/files/Photo/666/heif.heic',  7879, 'heif', 'heif.heic', 1, 0, 0, 0, 0, 0,
        1501838589870, 348113, 1))S";
    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK);
    MEDIA_INFO_LOG("db is %{public}d", dbRet);

    std::string selectSql = R"S(select data, date_trashed, hidden from Photos where file_id = "666" and
        date_trashed = "0" and hidden = "0")S";
    int32_t dbRet1 = g_rdbStore->ExecuteSql(selectSql);
    EXPECT_EQ(dbRet1, NativeRdb::E_OK);
    MEDIA_INFO_LOG("db is %{public}d", dbRet1);

    if (dbRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t QueryPhotoIdByDisplayName(const string& displayName)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Read moving photo query photo id by display name failed");
        return -1;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    return fileId;
}

static int32_t QueryTransCodeInfoByID(const int32_t fileId, std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    queryCmd.SetDataSharePred(predicates);
    vector<string> columns = { PhotoColumn::PHOTO_TRANSCODE_TIME, PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE,
        PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE };
    resultSet = g_rdbStore->Query(queryCmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Read moving photo query photo id by display name failed");
        return -1;
    }
    return 0;
}

int TestInsert(DataShareValuesBucket &dataShareValue)
{
    dataShareValue.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    MediaLibraryCommand cmd(OperationObject::MEDIA_APP_URI_PERMISSION, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket rdbValue = RdbUtils::ToValuesBucket(dataShareValue);
    cmd.SetValueBucket(rdbValue);
    int ret = MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
    return ret;
}

int32_t MediaLibraryRdbStore::GetInt(const shared_ptr<NativeRdb::ResultSet> &resultSet, const string &column)
{
    return get<int32_t>(ResultSetUtils::GetValFromColumn(column, resultSet, TYPE_INT32));
}

string MediaLibraryRdbStore::GetString(const shared_ptr<NativeRdb::ResultSet> &resultSet, const string &column)
{
    return get<string>(ResultSetUtils::GetValFromColumn(column, resultSet, TYPE_STRING));
}

string PermissionUtils::GetAppIdByBundleName(const string &bundleName, int32_t uid)
{
    string appid = "fuse_test_appid_000";
    return appid;
}

void MediaLibraryBundleManager::GetBundleNameByUID(const int32_t uid, string &bundleName)
{
    bundleName = "fuse_test_bundleName";
}

bool PermissionUtils::CheckPhotoCallerPermission(const vector<string> &perms, const int &uid,
    AccessTokenID &tokenCaller)
{
    tokenCaller = 1;
    for (const auto &perm : perms) {
        if (fuseTestPermsMap[perm] == false) {
            return false;
        }
    }
    return true;
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_open_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_open_test_001");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_open_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_open_test_002");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_ERR);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_open_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_open_test_003");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;
    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "fuse_test_appid_000");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_open_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_open_test_004");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;
    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "fuse_test_appid_001");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, "1");
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_open_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_open_test_005");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;
    int ret = -1;
    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "fuse_test_appid_000");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TEMPORARY_WRITE);
    dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, "1");
    ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_GetTranscodeUri_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_GetTranscodeUri_test_001");

    CreatAndPush();
    InsertHeifAsset();
    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_openfile_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_openfile_test_001");
    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();
    InsertHeifAsset();
    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
    EXPECT_GE(resultFd, 0)
        << "File descriptor invalid (fd: " << resultFd << "), check OpenOriginFd/OpenFilterProxyFd";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_HasTransCodeFile_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_HasTransCodeFile_test_001");
    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();
    CleanTestTables();
    SetTestTables();
    std::string insertSql = R"S(INSERT INTO Photos(file_id, data, size, title, display_name, media_type, position,
        is_temp, time_pending, hidden, date_trashed, transcode_time, trans_code_file_size)
        VALUES (666, '/storage/cloud/files/Photo/666/heif.heic',  7879, 'heif', 'heif.heic', 1, 0, 0, 0, 0, 0,
        1501838589870, 348113))S";
    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK) << "delet suc, insert suc";

    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;
    fuseTestPermsMap[PERM_WRITE_IMAGEVIDEO] = true;
    int flags = O_RDWR;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), flags, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
    EXPECT_GE(resultFd, 0)
        << "File descriptor invalid (fd: " << resultFd << "), check OpenOriginFd/OpenFilterProxyFd";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    SetTables();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_GetCompatibleModeFromFileId_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_GetCompatibleModeFromFileId_test_001");
    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();
    CleanTestTables();
    SetTestTables();
    std::string insertSql = R"S(INSERT INTO Photos(file_id, data, size, title, display_name, media_type, position,
        is_temp, time_pending, hidden, date_trashed, transcode_time, trans_code_file_size)
        VALUES (666, '/storage/cloud/files/Photo/666/heif.heic',  7879, 'heif', 'heif.heic', 1, 0, 0, 0, 0, 0,
        1501838589870, 348113))S";
    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK) << "delet suc, insert suc";

    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    SetTables();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_GetTranscodeUri_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_GetTranscodeUri_test_002");

    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();

    std::string insertSql = R"S(INSERT INTO Photos(file_id, data, size, title, display_name, media_type, position,
        is_temp, time_pending, hidden, date_trashed, transcode_time, trans_code_file_size, exist_compatible_duplicate)
        VALUES (666, '/storage/cloud/files/Photo/666/heif.heic',  7879, 'heif', 'heif.heic', 1, 0, 0, 0, 0, 0,
        1501838589870, 348113, 0))S";
    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK);

    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << ")";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_GetTranscodeUri_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_GetTranscodeUri_test_003");
    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();
    std::string insertSql = R"S(INSERT INTO Photos(file_id, data, size, title, display_name, media_type, position,
        is_temp, time_pending, hidden, date_trashed, transcode_time, trans_code_file_size, exist_compatible_duplicate)
        VALUES (666, '1/heif.heic',  7879, 'heif', 'heif.heic', 1, 0, 0, 0, 0, 0,
        1501838589870, 348113, 1))S";
    int32_t dbRet = g_rdbStore->ExecuteSql(insertSql);
    EXPECT_EQ(dbRet, NativeRdb::E_OK);
    MEDIA_INFO_LOG("db is %{public}d", dbRet);

    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_ERR)
        << "DoOpen suc (err: " << doOpenErr << ")";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_GetTranscodeUri_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_GetTranscodeUri_test_004");
    HeifTranscodingCheckUtils::InitCheckList();
    InsertHeifAsset();
    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_RDONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_ERR)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_close_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_close_test_001");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    string path;
    string fileId = to_string(photoId);
    GetPathFromFileId(path, fileId);

    OHOS::DataShare::DataShareValuesBucket dataShareValue;
    dataShareValue.Put(AppUriPermissionColumn::APP_ID, "fuse_test_appid_001");
    dataShareValue.Put(AppUriPermissionColumn::FILE_ID, photoId);
    dataShareValue.Put(AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ);
    dataShareValue.Put(AppUriPermissionColumn::TARGET_TOKENID, "1");
    auto ret = TestInsert(dataShareValue);
    EXPECT_EQ(ret, 0);

    int fd = 0;
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_OK);

    err = MediaFuseManager::GetInstance().DoRelease(path.c_str(), fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_close_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_close_test_002");
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoRelease(path.c_str(), fd);
    EXPECT_EQ(err, E_ERR);
}


HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_close_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start tdd MediaLibrary_fuse_close_test_003");
    HeifTranscodingCheckUtils::InitCheckList();
    CreatAndPush();
    InsertHeifAsset();
    std::string bundleName = "fuse_test_bundleName";
    MediaLibraryBundleManager::GetInstance()->GetBundleNameByUID(TEST_UID, bundleName);

    std::string testPath = "/Photo/666/";
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = true;

    int32_t fileId = QueryPhotoIdByDisplayName("heif.heic");
    ASSERT_GT(fileId, 0);

    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    ASSERT_EQ(QueryTransCodeInfoByID(fileId, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_TRANSCODE_TIME, resultSet), 1501838589870);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, resultSet), 348113);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, resultSet), 1);

    int resultFd = -1;
    int32_t doOpenErr = MediaFuseManager::GetInstance().DoOpen(testPath.c_str(), O_WRONLY, resultFd);
    EXPECT_EQ(doOpenErr, E_OK)
        << "DoOpen failed (err: " << doOpenErr << "), check MediaPrivacyManager::Open";
    MEDIA_INFO_LOG("fd %{public}d", resultFd);

    sleep(10);
    char data = 'A';
    int ret = write(resultFd, &data, 1);
    MEDIA_INFO_LOG("ret: %{public}d", ret);

    
    int32_t err = MediaFuseManager::GetInstance().DoRelease(testPath.c_str(), resultFd);
    EXPECT_EQ(err, E_OK);

    ASSERT_EQ(QueryTransCodeInfoByID(fileId, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_TRANSCODE_TIME, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, resultSet), 0);

    HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent();
    MEDIA_INFO_LOG("end tdd MediaLibrary_fuse_close_test_003");
}

// Test database operation failures
HWTEST_F(MediaLibraryFuseTest, MediaLibrary_PrepareUniqueNumberTable_test_001, TestSize.Level1)
{
    // Test when g_rdbStore is nullptr
    g_rdbStore = nullptr;
    PrepareUniqueNumberTable();
    // Should log error and return

    // Restore rdbStore for other tests
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(g_rdbStore, nullptr);
}

// Test different media types
HWTEST_F(MediaLibraryFuseTest, MediaLibrary_CreatePhoto_test_001, TestSize.Level1)
{
    // Test video creation
    int32_t videoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_VIDEO, "video.mp4");
    EXPECT_GE(videoId, E_OK);

    // Test audio creation
    int32_t audioId = CreatePhotoApi10(MediaType::MEDIA_TYPE_AUDIO, "audio.mp3");
    EXPECT_GE(audioId, E_OK);
}

// Test permission combinations
HWTEST_F(MediaLibraryFuseTest, MediaLibrary_Permission_test_001, TestSize.Level1)
{
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    EXPECT_GE(photoId, E_OK);

    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);

    // Test with both read and write permissions disabled
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;
    fuseTestPermsMap[PERM_WRITE_IMAGEVIDEO] = false;

    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, E_ERR);

    // Test with write permission but no read permission
    fuseTestPermsMap[PERM_WRITE_IMAGEVIDEO] = true;
    fuseTestPermsMap[PERM_READ_IMAGEVIDEO] = false;

    err = MediaFuseManager::GetInstance().DoOpen(path.c_str(), O_WRONLY, fd);
    EXPECT_EQ(err, E_OK);
}

// Test invalid paths
HWTEST_F(MediaLibraryFuseTest, MediaLibrary_InvalidPath_test_001, TestSize.Level1)
{
    int fd = -1;

    // Test with empty path
    int32_t err = MediaFuseManager::GetInstance().DoOpen("", O_RDONLY, fd);
    EXPECT_EQ(err, E_ERR);

    // Test with invalid path format
    err = MediaFuseManager::GetInstance().DoOpen("/invalid/path/format", O_RDONLY, fd);
    EXPECT_EQ(err, E_ERR);
}

// Test system device model
HWTEST_F(MediaLibraryFuseTest, MediaLibrary_CheckDevice_test_001, TestSize.Level1)
{
    bool isLinux = MediaFuseManager::GetInstance().CheckDeviceInLinux();

    string deviceModel = GetSystemProductModel();
    MEDIA_INFO_LOG("media library check device model:%{public}s", deviceModel.c_str());
    // klv(HYM-W5821) and rk(HH-SCDAYU200) are linux kernel
    bool isLinuxSys = (deviceModel == "HYM-W5821") || (deviceModel == "HH-SCDAYU200");
    EXPECT_NE(isLinux, isLinuxSys);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_Start_Fuse_test_001, TestSize.Level1) {
    std::string mountpoint = "/mnt/test_fuse";
    MediaFuseHighDaemon daemon(mountpoint);

    int32_t ret = daemon.StartFuse();
    EXPECT_EQ(ret, E_OK);
    MediaFuseManager::GetInstance().Start();
    MediaFuseManager::GetInstance().UMountFuse();
    MediaFuseManager::GetInstance().Stop();
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_DoGetAttr_test_001, TestSize.Level1) {
    std::string path = "/mnt/data/100/media_fuse/Photo/00000/00000/00000.jpg";

    struct stat stbuf;
    (void)memset_s(&stbuf, sizeof(stbuf), 0, sizeof(stbuf));
    int32_t ret = stat(path.c_str(), &stbuf);
    EXPECT_EQ(ret, E_ERR);

    ret = MediaFuseManager::GetInstance().DoGetAttr(path.c_str(), &stbuf);
    EXPECT_EQ(ret, E_ERR);
}


HWTEST_F(MediaLibraryFuseTest, MediaLibrary_DoHdcGetAttr_test_002, TestSize.Level1) {
    std::string path = "/Photo";
    struct stat stbuf;
    (void)memset_s(&stbuf, sizeof(stbuf), 0, sizeof(stbuf));
    fuse_file_info fi;

    int32_t ret = MediaFuseManager::GetInstance().DoHdcGetAttr(path.c_str(), &stbuf, &fi);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcOpen_test_002, TestSize.Level1)
{
    string path = "";
    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoHdcOpen(path.c_str(), O_RDONLY, fd);
    EXPECT_EQ(err, -EINVAL);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcCreate_test_001, TestSize.Level1)
{
    string path = "";
    fuse_file_info fi;
    int32_t err = MediaFuseManager::GetInstance().DoHdcCreate(path.c_str(), 0, &fi);
    EXPECT_EQ(err, -EINVAL);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcCreate_test_002, TestSize.Level1)
{
    string path = "/invalid/path";
    fuse_file_info fi;
    int32_t err = MediaFuseManager::GetInstance().DoHdcCreate(path.c_str(), 0, &fi);
    EXPECT_EQ(err, E_ERR);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcRelease_test_001, TestSize.Level1)
{
    string path = "";
    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoHdcRelease(path.c_str(), fd);
    EXPECT_EQ(err, -EINVAL);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcRelease_test_002, TestSize.Level1)
{
    string path = "/invalid/path";
    int fd = -1;
    int32_t err = MediaFuseManager::GetInstance().DoHdcRelease(path.c_str(), fd);
    EXPECT_EQ(err, -EBADF);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcRelease_test_003, TestSize.Level1)
{
    string path = "/Test";
    int fd = 0;
    int32_t err = MediaFuseManager::GetInstance().DoHdcRelease(path.c_str(), fd);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcUnlink_test_001, TestSize.Level1)
{
    string path = "";
    int32_t err = MediaFuseManager::GetInstance().DoHdcUnlink(path.c_str());
    EXPECT_EQ(err, E_ERR);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcUnlink_test_002, TestSize.Level1)
{
    string path = "/invalid/path";
    int32_t err = MediaFuseManager::GetInstance().DoHdcUnlink(path.c_str());
    EXPECT_EQ(err, E_ERR);
}


HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcUnlink_test_003, TestSize.Level1)
{
    int32_t photoId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(photoId, 0);

    MEDIA_INFO_LOG("creat photo succ");
    string fileId = to_string(photoId);
    string path;
    GetPathFromFileId(path, fileId);
    int32_t err = MediaFuseManager::GetInstance().DoHdcUnlink(path.c_str());
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcReadDir_test_001, TestSize.Level1)
{
    string path = "";
    void *buf = nullptr;
    fuse_fill_dir_t filler = nullptr;
    off_t offset = 0;
    enum fuse_readdir_flags flags = FUSE_READDIR_PLUS;
    int32_t err = MediaFuseManager::GetInstance().DoHdcReadDir(path.c_str(), buf, filler, offset, flags);
    EXPECT_EQ(err, -EINVAL);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_DoHdcReadDir_test_002, TestSize.Level1)
{
    string path = "/Photo";
    void *buf = nullptr;
    fuse_fill_dir_t filler = nullptr;
    off_t offset = 0;
    enum fuse_readdir_flags flags = FUSE_READDIR_PLUS;
    int32_t err = MediaFuseManager::GetInstance().DoHdcReadDir(path.c_str(), buf, filler, offset, flags);
    EXPECT_EQ(err, E_OK);
}

HWTEST_F(MediaLibraryFuseTest, MediaLibrary_fuse_SetUid_test_001, TestSize.Level1)
{
    int32_t testUid = 12345;
    MediaFuseManager::GetInstance().SetUid(testUid);
    EXPECT_EQ(MediaFuseManager::GetInstance().GetUid(), testUid);
}
} // namespace Media
} // namespace OHOS