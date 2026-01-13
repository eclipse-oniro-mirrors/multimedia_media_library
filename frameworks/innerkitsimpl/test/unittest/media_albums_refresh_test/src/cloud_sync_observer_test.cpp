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

#define MLOG_TAG "CloudSyncObserverTest"

#include "cloud_sync_observer_test.h"

#include "result_set.h"
#include "result_set_utils.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_photo_operations.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using ChangeInfo = DataShare::DataShareObserver::ChangeInfo;
using ChangeType = DataShare::DataShareObserver::ChangeType;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
constexpr static int CHANGETYPES = 4;

static std::array<ChangeType, CHANGETYPES> g_ChangeTypes = {{
    ChangeType::OTHER,
    ChangeType::INSERT,
    ChangeType::UPDATE,
    ChangeType::DELETE,
}};

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
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

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
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
}

static void ClearAndRestart()
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

void CloudSyncObserverTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void CloudSyncObserverTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Clean is finish");
}

void CloudSyncObserverTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void CloudSyncObserverTest::TearDown() {}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Ordinary_Uri, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudSyncObsOnChange_Ordinary_Uri");
    auto fileId1 = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(fileAsset->GetUri()));
    infos.data_ = "test";
    infos.changeType_ = ChangeType::OTHER;
    obs->OnChange(infos);
    MEDIA_INFO_LOG("end CloudSyncObsOnChange_Ordinary_Uri");
}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Gallery_Sync_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudSyncObsOnChange_Gallery_Sync_Prefix");
    auto fileId1 =  CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    string uri = fileAsset->GetUri();
    uri = PhotoAlbumColumns::PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX + uri.substr(uri.find_last_of('/')+1);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(uri));
    infos.data_ = "test";
    infos.changeType_ = ChangeType::OTHER;
    obs->OnChange(infos);

    infos.data_ = R"({"name": "John", "age": 30, "city": "New York"})";
    obs->OnChange(infos);

    infos.data_ = R"({"taskType": 1, "syncId": "2", "syncType": 3, "totalAssets": 1, "totalAlbums": 1})";
    obs->OnChange(infos);

    infos.data_ = R"({"taskType": 0, "syncId": "2", "syncType": 3, "totalAssets": 1, "totalAlbums": 1})";
    obs->OnChange(infos);
    MEDIA_INFO_LOG("end CloudSyncObsOnChange_Gallery_Sync_Prefix");
}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Cloud_Photo_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("start  CloudSyncObsOnChange_Cloud_Photo_Prefix");
    auto fileId1 =  CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    string uri = fileAsset->GetUri();
    uri = PhotoColumn::PHOTO_CLOUD_URI_PREFIX + uri.substr(uri.find_last_of('/')+1);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(uri));
    infos.data_ = "test";
    infos.changeType_ = ChangeType::OTHER;
    obs->OnChange(infos);
    MEDIA_INFO_LOG("end CloudSyncObsOnChange_Cloud_Photo_Prefix");
}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Album_GalleryUri_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("start  CloudSyncObsOnChange_Album_GalleryUri_Prefix");
    auto fileId1 =  CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    string uri = fileAsset->GetUri();
    uri = PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX + uri.substr(uri.find_last_of('/')+1);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(uri));
    infos.data_ = "test";

    for (const auto& type : g_ChangeTypes) {
        infos.changeType_ = type;
        obs->OnChange(infos);
    }
    MEDIA_INFO_LOG("end  CloudSyncObsOnChange_Album_GalleryUri_Prefix");
}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Gallery_Cloud_Uri_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudSyncObsOnChange_Gallery_Cloud_Uri_Prefix");
    auto fileId1 =  CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    string uri = fileAsset->GetUri();
    uri = PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + uri.substr(uri.find_last_of('/')+1);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(uri));
    infos.data_ = "test";

    for (const auto& type : g_ChangeTypes) {
        infos.changeType_ = type;
        obs->OnChange(infos);
    }

    MEDIA_INFO_LOG("end CloudSyncObsOnChange_Gallery_Cloud_Uri_Prefix");
}

HWTEST_F(CloudSyncObserverTest, CloudSyncObsOnChange_Gallery_Download_Uri_Prefix, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudSyncObsOnChange_Gallery_Download_Uri_Prefix");
    auto fileId1 =  CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo1.jpg");
    ASSERT_GT(fileId1, 0);

    shared_ptr<FileAsset> fileAsset = make_shared<FileAsset>();
    MediaLibraryUnitTestUtils::GetFileAsset(fileId1, fileAsset);
    string uri = fileAsset->GetUri();
    uri = PhotoAlbumColumns::PHOTO_GALLERY_DOWNLOAD_URI_PREFIX + uri.substr(uri.find_last_of('/')+1);
    shared_ptr<CloudSyncObserver> obs = make_shared<CloudSyncObserver>();
    EXPECT_NE(obs, nullptr);

    ChangeInfo infos;
    infos.uris_.push_back(Uri(uri));
    infos.data_ = "test";

    for (const auto& type : g_ChangeTypes) {
        infos.changeType_ = type;
        obs->OnChange(infos);
    }
    MEDIA_INFO_LOG("end CloudSyncObsOnChange_Gallery_Download_Uri_Prefix");
}
} // namespace Media
} // namespace OHOS
