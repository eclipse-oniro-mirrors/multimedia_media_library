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

#define MLOG_TAG "PhotosCloneTest"

#include "photos_clone_test.h"

#include <fstream>

#define private public
#include "database_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_log.h"
#include "photos_clone.h"
#include "userfile_manager_types.h"
#include "media_file_utils.h"
#include "rdb_store_config.h"
#include "rdb_helper.h"
#include "rdb_open_callback.h"
#undef private

using namespace testing::ext;

namespace OHOS::Media {
const int32_t TEST_FILE_ID = 1;
const int32_t TEST_ALBUM_ID = 100;
const int64_t TEST_FILE_SIZE = 1024;
const std::string TEST_ALBUM_NAME = "Camera";
const std::string TEST_ALBUM_LPATH = "/DCIM/Camera";
const std::string TEST_DATA = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
const std::string TEST_DISPLAY_NAME = "test.jpg";
const std::string TEST_CLOUD_ID = "cloudid";

const std::string CLONE_RESTORE_TEST_DB_DIR = "/storage/media/local/files/.backup/restore/";
const std::string CLONE_RESTORE_TEST_DB_PATH = CLONE_RESTORE_TEST_DB_DIR + CLONE_FILE_INFO_RESTORE_DB;

class CloneRestoreTestDbCallBack : public NativeRdb::RdbOpenCallback {
public:
    int32_t OnCreate(NativeRdb::RdbStore &rdbStore) override
    {
        rdbStore.ExecuteSql("CREATE TABLE IF NOT EXISTS " + LAKE_FILE_INFO_DEDUPLICATION_TABLE +
            " (path TEXT, new_path TEXT)");
        rdbStore.ExecuteSql("CREATE TABLE IF NOT EXISTS " + LAKE_FILE_INFO_FAIL_TABLE +
            " (path TEXT)");
        return NativeRdb::E_OK;
    }
    int32_t OnUpgrade(NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override
    {
        return NativeRdb::E_OK;
    }
};

static void CreateCloneRestoreTestDb()
{
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    MediaFileUtils::CreateDirectory(CLONE_RESTORE_TEST_DB_DIR);
    
    CloneRestoreTestDbCallBack callBack;
    NativeRdb::RdbStoreConfig config("");
    config.SetName(CLONE_FILE_INFO_RESTORE_DB);
    config.SetPath(CLONE_RESTORE_TEST_DB_PATH);
    int32_t err = 0;
    auto rdbStore = NativeRdb::RdbHelper::GetRdbStore(config, 1, callBack, err);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("CreateCloneRestoreTestDb failed, err: %{public}d", err);
        return;
    }
    
    rdbStore->ExecuteSql("INSERT INTO " + LAKE_FILE_INFO_DEDUPLICATION_TABLE +
        " (path, new_path) VALUES ('/storage/emulated/0/Pictures/test_dedup.jpg', "
        "'/storage/emulated/0/Pictures/test_dedup_new.jpg')");
    rdbStore->ExecuteSql("INSERT INTO " + LAKE_FILE_INFO_DEDUPLICATION_TABLE +
        " (path, new_path) VALUES ('/storage/emulated/0/DCIM/test_lake.mp4', "
        "'/storage/emulated/0/DCIM/test_lake_new.mp4')");
    rdbStore->ExecuteSql("INSERT INTO " + LAKE_FILE_INFO_FAIL_TABLE +
        " (path) VALUES ('/storage/emulated/0/Pictures/test_fail.jpg')");
    
    MEDIA_INFO_LOG("CreateCloneRestoreTestDb success");
}

static void DeleteCloneRestoreTestDb()
{
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    MediaFileUtils::DeleteDir(CLONE_RESTORE_TEST_DB_DIR);
    MEDIA_INFO_LOG("DeleteCloneRestoreTestDb success");
}

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void PhotosCloneTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MEDIA_INFO_LOG("Start Init");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaFileUtils::CreateDirectory(CLONE_RESTORE_TEST_DB_DIR);
}

void PhotosCloneTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    PhotosCloneTestUtils::ClearAllData();
    DeleteCloneRestoreTestDb();
}

// SetUp:Execute before each test case
void PhotosCloneTest::SetUp()
{
    PhotosCloneTestUtils::ClearAllData();
}

void PhotosCloneTest::TearDown(void)
{}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_1_lPath_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/Screenrecords");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_1_lPath_Not_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/Screenshots";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/Screenrecords");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_2_lPath_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/其它");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_LPATH_EMPTY_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_Screenshots_Video_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/其它");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_TRASHED_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/其它");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_TRASHED_LPATH_EMPTY_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/其它");
}

HWTEST_F(PhotosCloneTest, GetPhotosRowCountInPhotoMap_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetPhotosRowCountInPhotoMap_Test start");
    auto count = PhotosClone().GetPhotosRowCountInPhotoMap();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetPhotosRowCountInPhotoMap_Test end");
}

HWTEST_F(PhotosCloneTest, GetCloudPhotosRowCountInPhotoMap_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetCloudPhotosRowCountInPhotoMap_Test start");
    auto count = PhotosClone().GetCloudPhotosRowCountInPhotoMap();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetCloudPhotosRowCountInPhotoMap_Test end");
}

HWTEST_F(PhotosCloneTest, GetPhotosRowCountNotInPhotoMap_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetPhotosRowCountNotInPhotoMap_Test start");
    auto count = PhotosClone().GetPhotosRowCountNotInPhotoMap();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetPhotosRowCountNotInPhotoMap_Test end");
}

HWTEST_F(PhotosCloneTest, GetCloudPhotosRowCountNotInPhotoMap_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetCloudPhotosRowCountNotInPhotoMap_Test start");
    auto count = PhotosClone().GetCloudPhotosRowCountNotInPhotoMap();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetCloudPhotosRowCountNotInPhotoMap_Test end");
}

HWTEST_F(PhotosCloneTest, FindlPath_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindlPath_Test start");
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    std::string lPath = PhotosClone().FindlPath(fileInfo);
    EXPECT_EQ(lPath, "/Pictures/其它");
    MEDIA_INFO_LOG("FindlPath_Test end");
}

HWTEST_F(PhotosCloneTest, FindAlbumId_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindAlbumId_Test start");
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    auto albumId = PhotosClone().FindAlbumId(fileInfo);
    EXPECT_EQ(albumId, static_cast<int32_t>(PhotoAlbumId::TRASH));
    MEDIA_INFO_LOG("FindAlbumId_Test end");
}

HWTEST_F(PhotosCloneTest, FindPackageName_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindPackageName_Test start");
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    std::string albumName = PhotosClone().FindPackageName(fileInfo);
    EXPECT_TRUE(albumName.empty()); // nullptr, source album cannot be created
    MEDIA_INFO_LOG("FindPackageName_Test end");
}

HWTEST_F(PhotosCloneTest, FindBundleName_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindBundleName_Test start");
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    std::string bundleName = PhotosClone().FindBundleName(fileInfo);
    EXPECT_TRUE(bundleName.empty());
    MEDIA_INFO_LOG("FindBundleName_Test end");
}

HWTEST_F(PhotosCloneTest, GenerateUuid_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GenerateUuid_Test start");
    std::string uuid = PhotosClone().GenerateUuid();
    EXPECT_FALSE(uuid.empty());
    MEDIA_INFO_LOG("GenerateUuid_Test end");
}

HWTEST_F(PhotosCloneTest, FindDuplicateBurstKey_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindDuplicateBurstKey_Test start");
    std::vector<PhotosDao::PhotosRowData> duplicateBurstKeyList = PhotosClone().FindDuplicateBurstKey();
    uint64_t number = static_cast<uint64_t>(duplicateBurstKeyList.size());
    EXPECT_EQ(number, 0);
    MEDIA_INFO_LOG("FindDuplicateBurstKey_Test end");
}

HWTEST_F(PhotosCloneTest, FindSourcePath_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSourcePath_Test start");
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/SourceTemp";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.displayName = "Sp.jpg";
    std::string sourcePath = PhotosClone().FindSourcePath(fileInfo);
    EXPECT_FALSE(sourcePath.empty());
    EXPECT_EQ(sourcePath, "/storage/emulated/0/Pictures/SourceTemp/Sp.jpg");
    MEDIA_INFO_LOG("FindSourcePath_Test end");
}

HWTEST_F(PhotosCloneTest, GetNoNeedMigrateCount_Test, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetNoNeedMigrateCount_Test start");
    auto count = PhotosClone().GetNoNeedMigrateCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("GetNoNeedMigrateCount_Test end");
}

HWTEST_F(PhotosCloneTest, FindSameFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSameFile_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    PhotosDao::PhotosRowData rowData = photosClone.FindSameFile(fileInfo);
    EXPECT_EQ(rowData.IsValid(), false); // no data in target db
    MEDIA_INFO_LOG("FindSameFile_Test_001 end");
}

HWTEST_F(PhotosCloneTest, FindSameFile_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSameFile_Test_002 start");
    PhotosCloneTestUtils::InsertAlbum();
    PhotosCloneTestUtils::InsertPhoto();
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.lPath = TEST_ALBUM_LPATH;

    PhotosDao::PhotosRowData rowData = photosClone.FindSameFile(fileInfo);
    EXPECT_EQ(rowData.IsValid(), true); // source: without cloud_id - dst: without cloud_id
    MEDIA_INFO_LOG("FindSameFile_Test_002 end");
}

HWTEST_F(PhotosCloneTest, FindSameFile_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSameFile_Test_003 start");
    PhotosCloneTestUtils::InsertAlbum();
    PhotosCloneTestUtils::InsertPhoto(TEST_CLOUD_ID);
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.uniqueId = TEST_CLOUD_ID;

    PhotosDao::PhotosRowData rowData = photosClone.FindSameFile(fileInfo);
    EXPECT_EQ(rowData.IsValid(), true); // source: with cloud_id - dst: with cloud_id
    MEDIA_INFO_LOG("FindSameFile_Test_003 end");
}

HWTEST_F(PhotosCloneTest, FindSameFile_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSameFile_Test_004 start");
    PhotosCloneTestUtils::InsertAlbum();
    PhotosCloneTestUtils::InsertPhoto();
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.uniqueId = TEST_CLOUD_ID;
    fileInfo.lPath = TEST_ALBUM_LPATH;

    PhotosDao::PhotosRowData rowData = photosClone.FindSameFile(fileInfo);
    EXPECT_EQ(rowData.IsValid(), true); // source: with cloud_id - dst: without cloud_id
    MEDIA_INFO_LOG("FindSameFile_Test_004 end");
}

HWTEST_F(PhotosCloneTest, FindSameFile_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("FindSameFile_Test_005 start");
    PhotosCloneTestUtils::InsertAlbum();
    PhotosCloneTestUtils::InsertPhoto();
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.uniqueId = TEST_CLOUD_ID;

    PhotosDao::PhotosRowData rowData = photosClone.FindSameFile(fileInfo);
    EXPECT_EQ(rowData.IsValid(), false); // source: with cloud_id, no lpath - dst: not found
    MEDIA_INFO_LOG("FindSameFile_Test_005 end");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Test_001, TestSize.Level0)
{
    // normal photo, album not exist, will create
    MEDIA_INFO_LOG("FindAlbumInfo_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.lPath = TEST_ALBUM_LPATH;

    int32_t albumId = photosClone.FindAlbumId(fileInfo);
    EXPECT_GT(albumId, 0);
    std::string packageName = photosClone.FindPackageName(fileInfo);
    EXPECT_EQ(packageName.empty(), false);
    MEDIA_INFO_LOG("FindAlbumId_Test_001 end");
}

HWTEST_F(PhotosCloneTest, FindAlbumId_Test_002, TestSize.Level0)
{
    // hidden photo, album not exist, not create, set owner_album_id = HIDDEN
    MEDIA_INFO_LOG("FindAlbumId_Test_002 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.lPath = TEST_ALBUM_LPATH;
    fileInfo.hidden = 1;

    int32_t albumId = photosClone.FindAlbumId(fileInfo);
    EXPECT_EQ(albumId, static_cast<int32_t>(PhotoAlbumId::HIDDEN));
    std::string packageName = photosClone.FindPackageName(fileInfo);
    EXPECT_EQ(packageName.empty(), true);
    MEDIA_INFO_LOG("FindAlbumId_Test_002 end");
}

HWTEST_F(PhotosCloneTest, FindAlbumId_Test_003, TestSize.Level0)
{
    // hidden & trashed photo, album not exist, not create, set owner_album_id = TRASH
    MEDIA_INFO_LOG("FindAlbumId_Test_003 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = TEST_DISPLAY_NAME;
    fileInfo.lPath = TEST_ALBUM_LPATH;
    fileInfo.hidden = 1;
    fileInfo.recycledTime = 1;

    int32_t albumId = photosClone.FindAlbumId(fileInfo);
    EXPECT_EQ(albumId, static_cast<int32_t>(PhotoAlbumId::TRASH));
    std::string packageName = photosClone.FindPackageName(fileInfo);
    EXPECT_EQ(packageName.empty(), true);
    MEDIA_INFO_LOG("FindAlbumId_Test_003 end");
}

HWTEST_F(PhotosCloneTest, SetFilePath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetFilePath_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.fileSourceType = FileSourceType::MEDIA;
    fileInfo1.storagePath = "/storage/emulated/0/Pictures/test1.jpg";
    fileInfo1.oldPath = "/storage/cloud/files/Photo/16/test1.jpg";
    fileInfos.push_back(fileInfo1);

    FileInfo fileInfo2;
    fileInfo2.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo2.storagePath = "/storage/emulated0/Pictures/test2.jpg";
    fileInfo2.oldPath = "/storage/cloud/files/Photo/16/test2.jpg";
    fileInfos.push_back(fileInfo2);

    photosClone.SetFilePath(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), 1); // lake file not exist
    MEDIA_INFO_LOG("SetFilePath_Test_001 end");
}

HWTEST_F(PhotosCloneTest, SetFilePath_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetFilePath_Test_002 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);

    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA;
    fileInfo.storagePath = "";
    fileInfos.push_back(fileInfo);

    size_t originalSize = fileInfos.size();
    photosClone.SetFilePath(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    MEDIA_INFO_LOG("SetFilePath_Test_002 end");
}

HWTEST_F(PhotosCloneTest, GetNumberedStoragePath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_001 start");
    PhotosClone photosClone;

    std::string storagePath = "/storage/emulated/0/Pictures/test.jpg";
    uint32_t number = 1;

    std::string result = photosClone.GetNumberedStoragePath(storagePath, number);
    EXPECT_EQ(result, "/storage/emulated/0/Pictures/test(1).jpg");
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_001 end");
}

HWTEST_F(PhotosCloneTest, GetNumberedStoragePath_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_002 start");
    PhotosClone photosClone;

    std::string storagePath = "/storage/emulated/0/Pictures/test.jpg";
    uint32_t number = 0;

    std::string result = photosClone.GetNumberedStoragePath(storagePath, number);
    EXPECT_EQ(result, storagePath);
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_002 end");
}

HWTEST_F(PhotosCloneTest, GetNumberedStoragePath_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_003 start");
    PhotosClone photosClone;

    std::string storagePath = "/storage/emulated/0/Pictures/test";
    uint32_t number = 1;

    std::string result = photosClone.GetNumberedStoragePath(storagePath, number);
    EXPECT_EQ(result, "/storage/emulated/0/Pictures/test(1)");
    MEDIA_INFO_LOG("GetNumberedStoragePath_Test_003 end");
}

HWTEST_F(PhotosCloneTest, ShouldDeleteDuplicateLakeFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_001 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfo.needMove = true;

    bool result = photosClone.ShouldDeleteDuplicateLakeFile(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ShouldDeleteDuplicateLakeFile_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_002 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfo.isStoragePathExistInDb = true;

    bool result = photosClone.ShouldDeleteDuplicateLakeFile(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_002 end");
}

HWTEST_F(PhotosCloneTest, ShouldDeleteDuplicateLakeFile_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_003 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfo.isStoragePathExistInDb = false;
    fileInfo.needMove = true;

    bool result = photosClone.ShouldDeleteDuplicateLakeFile(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ShouldDeleteDuplicateLakeFile_Test_003 end");
}

HWTEST_F(PhotosCloneTest, IsCloudPathExist_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsCloudPathExist_Test_001 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA;
    fileInfo.cloudPath = "/storage/cloud/files/Photo/16/test.jpg";

    bool result = photosClone.IsCloudPathExist(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("IsCloudPathExist_Test_001 end");
}

HWTEST_F(PhotosCloneTest, IsCloudPathExist_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsCloudPathExist_Test_002 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.cloudPath = "/storage/cloud/files/Photo/16/test.jpg";
    fileInfo.isCloudPathExistInDb = true;

    bool result = photosClone.IsCloudPathExist(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("IsCloudPathExist_Test_002 end");
}

HWTEST_F(PhotosCloneTest, IsCloudPathExist_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsCloudPathExist_Test_003 start");
    PhotosClone photosClone;

    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.cloudPath = "/storage/cloud/files/Photo/16/test.jpg";
    fileInfo.isCloudPathExistInDb = false;

    bool result = photosClone.IsCloudPathExist(fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("IsCloudPathExist_Test_003 end");
}

HWTEST_F(PhotosCloneTest, CreateCloudPath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloudPath_Test_001 start");
    PhotosClone photosClone;

    int32_t uniqueId = 100;
    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = "test.jpg";

    int32_t result = photosClone.CreateCloudPath(uniqueId, fileInfo);
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(fileInfo.cloudPath.empty());
    MEDIA_INFO_LOG("CreateCloudPath_Test_001 end");
}

HWTEST_F(PhotosCloneTest, CreateCloudPath_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateCloudPath_Test_002 start");
    PhotosClone photosClone;

    int32_t uniqueId = 100;
    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.fileType = MediaType::MEDIA_TYPE_IMAGE;
    fileInfo.displayName = "test.jpg";

    int32_t result = photosClone.CreateCloudPath(uniqueId, fileInfo);
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(fileInfo.cloudPath.empty());
    MEDIA_INFO_LOG("CreateCloudPath_Test_002 end");
}

void PhotosCloneTestUtils::ClearAllData()
{
    ClearPhotosData();
    ClearPhotoAlbumData();
}

void PhotosCloneTestUtils::ClearPhotosData()
{
    const std::string CLEAR_PHOTOS_SQL = "DELETE FROM Photos";
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), CLEAR_PHOTOS_SQL);
}

void PhotosCloneTestUtils::ClearPhotoAlbumData()
{
    const std::string CLEAR_PHOTO_ALBUM_SQL = "DELETE FROM PhotoAlbum WHERE album_id >= ?";
    const std::vector<NativeRdb::ValueObject> BIND_ARGS = { TEST_ALBUM_ID };
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), CLEAR_PHOTO_ALBUM_SQL, BIND_ARGS);
}

void PhotosCloneTestUtils::InsertPhoto(const std::string &cloudId)
{
    const std::string INSERT_SQL = "INSERT INTO Photos (file_id, data, size, display_name, owner_album_id, cloud_id) "
        " VALUES (?, ?, ?, ?, ?, ?)";
    const std::vector<NativeRdb::ValueObject> BIND_ARGS = { TEST_FILE_ID, TEST_DATA, TEST_FILE_SIZE, TEST_DISPLAY_NAME,
        TEST_ALBUM_ID, cloudId };
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), INSERT_SQL, BIND_ARGS);
}

void PhotosCloneTestUtils::InsertAlbum()
{
    const std::string INSERT_SQL = "INSERT INTO PhotoAlbum (album_id, album_name, lpath, album_type, album_subtype) "
        " VALUES (?, ?, ?, ?, ?)";
    const std::vector<NativeRdb::ValueObject> BIND_ARGS = { TEST_ALBUM_ID, TEST_ALBUM_NAME, TEST_ALBUM_LPATH,
        static_cast<int32_t>(PhotoAlbumType::SOURCE), static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC) };
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), INSERT_SQL, BIND_ARGS);
}

HWTEST_F(PhotosCloneTest, SetRestoreInfo_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetRestoreInfo_Test_001 start");
    PhotosClone photosClone;
    
    int32_t sceneCode = CLONE_RESTORE_ID;
    std::string taskId = "task_clone_001";
    
    PhotosClone& result = photosClone.SetRestoreInfo(sceneCode, taskId);
    EXPECT_EQ(&result, &photosClone);
    MEDIA_INFO_LOG("SetRestoreInfo_Test_001 end");
}

HWTEST_F(PhotosCloneTest, SetRestoreInfo_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetRestoreInfo_Test_002 start");
    PhotosClone photosClone;
    
    int32_t sceneCode = DUAL_FRAME_CLONE_RESTORE_ID;
    std::string taskId = "";
    
    PhotosClone& result = photosClone.SetRestoreInfo(sceneCode, taskId);
    EXPECT_EQ(&result, &photosClone);
    MEDIA_INFO_LOG("SetRestoreInfo_Test_002 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    
    std::vector<FileInfo> fileInfos;
    size_t originalSize = fileInfos.size();
    
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    EXPECT_TRUE(fileInfos.empty());
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_002 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_002");
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileIdOld = 1;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfo.filePath = "";
    fileInfos.push_back(fileInfo);
    
    size_t originalSize = fileInfos.size();
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    EXPECT_EQ(fileInfos[0].filePath, fileInfos[0].storagePath);
    EXPECT_EQ(fileInfos[0].filePath, "/storage/emulated/0/Pictures/test.jpg");
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_002 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_003 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_003");
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.fileIdOld = 1;
    fileInfo1.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo1.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfo1.filePath = "";
    fileInfos.push_back(fileInfo1);
    
    FileInfo fileInfo2;
    fileInfo2.fileIdOld = 2;
    fileInfo2.fileSourceType = FileSourceType::MEDIA;
    fileInfo2.storagePath = "/storage/emulated/0/Pictures/normal.jpg";
    fileInfo2.filePath = "/storage/cloud/files/Photo/16/normal.jpg";
    fileInfos.push_back(fileInfo2);
    
    size_t originalSize = fileInfos.size();
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    EXPECT_EQ(fileInfos[0].filePath, fileInfos[0].storagePath);
    EXPECT_EQ(fileInfos[1].filePath, "/storage/cloud/files/Photo/16/normal.jpg");
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Test_003 end");
}

HWTEST_F(PhotosCloneTest, QueryLakeFileFailInfo_Basic_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_Basic_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_lake_fail_info_001");
    
    std::unordered_map<std::string, FailedFileInfo> lakePhotoFailedFiles;
    std::unordered_map<std::string, FailedFileInfo> lakeVideoFailedFiles;
    photosClone.QueryLakeFileFailInfo(lakePhotoFailedFiles, lakeVideoFailedFiles);
    EXPECT_EQ(lakePhotoFailedFiles.size(), 0);
    EXPECT_EQ(lakeVideoFailedFiles.size(), 0);
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_Basic_Test_001 end");
}

HWTEST_F(PhotosCloneTest, QueryLakeFileFailInfo_NoRdbStore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_NoRdbStore_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_lake_fail_info_002");
    
    std::unordered_map<std::string, FailedFileInfo> lakePhotoFailedFiles;
    std::unordered_map<std::string, FailedFileInfo> lakeVideoFailedFiles;
    photosClone.QueryLakeFileFailInfo(lakePhotoFailedFiles, lakeVideoFailedFiles);
    EXPECT_EQ(lakePhotoFailedFiles.size(), 0);
    EXPECT_EQ(lakeVideoFailedFiles.size(), 0);
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_NoRdbStore_Test_001 end");
}

HWTEST_F(PhotosCloneTest, QueryLakeFileFailInfo_ClearInput_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_ClearInput_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_lake_fail_info_003");
    
    std::unordered_map<std::string, FailedFileInfo> lakePhotoFailedFiles;
    std::unordered_map<std::string, FailedFileInfo> lakeVideoFailedFiles;
    FailedFileInfo failedFileInfo(CLONE_RESTORE_ID, FileInfo(), RestoreError::ANCO_TRANSFER_FAILED);
    lakePhotoFailedFiles.emplace("/test/path.jpg", failedFileInfo);
    lakeVideoFailedFiles.emplace("/test/path.mp4", failedFileInfo);
    
    photosClone.QueryLakeFileFailInfo(lakePhotoFailedFiles, lakeVideoFailedFiles);
    EXPECT_EQ(lakePhotoFailedFiles.size(), 0);
    EXPECT_EQ(lakeVideoFailedFiles.size(), 0);
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_ClearInput_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_EmptyNewPath_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_EmptyNewPath_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_dedup_001");
    
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    
    PhotosClone::DeduplicationInfo deduplicationInfo;
    deduplicationInfo.path = "/storage/emulated/0/Pictures/test.jpg";
    deduplicationInfo.newPath = "";
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, deduplicationInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_EmptyNewPath_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_StatFail_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_StatFail_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_dedup_002");
    
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    
    PhotosClone::DeduplicationInfo deduplicationInfo;
    deduplicationInfo.path = "/storage/emulated/0/Pictures/old.jpg";
    deduplicationInfo.newPath = "/storage/emulated/0/Pictures/nonexistent_new.jpg";
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, deduplicationInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_StatFail_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_SizeMismatch_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_SizeMismatch_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_dedup_003");
    
    FileInfo fileInfo;
    fileInfo.fileSize = 2048;
    
    PhotosClone::DeduplicationInfo deduplicationInfo;
    deduplicationInfo.path = "/storage/emulated/0/Pictures/old.jpg";
    deduplicationInfo.newPath = "/storage/emulated/0/Pictures/new.jpg";
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, deduplicationInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_SizeMismatch_Test_001 end");
}

HWTEST_F(PhotosCloneTest, QueryDeduplicationFileInfo_Basic_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryDeduplicationFileInfo_Basic_Test_001 start");
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_query_dedup_001");
    
    std::unordered_map<std::string, PhotosClone::DeduplicationInfo> deduplicationMap;
    photosClone.QueryDeduplicationFileInfo(deduplicationMap);
    EXPECT_TRUE(deduplicationMap.empty());
    MEDIA_INFO_LOG("QueryDeduplicationFileInfo_Basic_Test_001 end");
}

HWTEST_F(PhotosCloneTest, InitCloneRestoreRdbStore_Basic_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("InitCloneRestoreRdbStore_Basic_Test_001 start");
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_rdb_001");
    
    int32_t ret = photosClone.InitCloneRestoreRdbStore();
    EXPECT_EQ(ret, E_ERR);
    EXPECT_EQ(photosClone.cloneRestoreRdbStore_, nullptr);
    MEDIA_INFO_LOG("InitCloneRestoreRdbStore_Basic_Test_001 end");
}

HWTEST_F(PhotosCloneTest, InitDeduplicationInfo_Basic_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("InitDeduplicationInfo_Basic_Test_001 start");
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_init_dedup_001");
    
    photosClone.InitDeduplicationInfo();
    EXPECT_TRUE(photosClone.deduplicationMap_.empty());
    MEDIA_INFO_LOG("InitDeduplicationInfo_Basic_Test_001 end");
}

HWTEST_F(PhotosCloneTest, InitDeduplicationInfo_NullRdbStore_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("InitDeduplicationInfo_NullRdbStore_Test_001 start");
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_RESTORE_TEST_DB_PATH);
    
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_init_dedup_003");
    
    photosClone.InitDeduplicationInfo();
    EXPECT_TRUE(photosClone.deduplicationMap_.empty());
    MEDIA_INFO_LOG("InitDeduplicationInfo_NullRdbStore_Test_001 end");
}

HWTEST_F(PhotosCloneTest, IsFileSizeMatched_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsFileSizeMatched_Success_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_size_match_001");
    
    FileInfo fileInfo;
    fileInfo.fileSize = 0;
    
    std::string testFilePath = "/data/local/tmp/test_size_match.jpg";
    std::ofstream testFile(testFilePath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testFilePath.c_str());
    }
    testFile.close();
    
    int64_t actualSize = 0;
    RestoreError result = photosClone.IsFileSizeMatched(fileInfo, testFilePath, actualSize);
    EXPECT_EQ(result, RestoreError::SUCCESS);
    EXPECT_EQ(actualSize, 0);
    
    MediaFileUtils::DeleteFile(testFilePath);
    MEDIA_INFO_LOG("IsFileSizeMatched_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, IsFileSizeMatched_PathInvalid_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsFileSizeMatched_PathInvalid_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_size_match_002");
    
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    
    int64_t actualSize = 0;
    RestoreError result = photosClone.IsFileSizeMatched(fileInfo, "/nonexistent/path/file.jpg", actualSize);
    EXPECT_EQ(result, RestoreError::PATH_INVALID);
    EXPECT_EQ(actualSize, 0);
    MEDIA_INFO_LOG("IsFileSizeMatched_PathInvalid_Test_001 end");
}

HWTEST_F(PhotosCloneTest, IsFileSizeMatched_SizeMismatch_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsFileSizeMatched_SizeMismatch_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_size_match_003");
    
    FileInfo fileInfo;
    fileInfo.fileSize = TEST_FILE_SIZE;
    
    std::string testFilePath = "/data/local/tmp/test_size_mismatch.jpg";
    std::ofstream testFile(testFilePath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testFilePath.c_str());
    }
    testFile << "test content";
    testFile.close();
    
    int64_t actualSize = 0;
    RestoreError result = photosClone.IsFileSizeMatched(fileInfo, testFilePath, actualSize);
    EXPECT_EQ(result, RestoreError::DEDUPLICATION_FILE_SIZE_MISMATCH);
    EXPECT_GT(actualSize, 0);
    
    MediaFileUtils::DeleteFile(testFilePath);
    MEDIA_INFO_LOG("IsFileSizeMatched_SizeMismatch_Test_001 end");
}

HWTEST_F(PhotosCloneTest, IsFileSizeMatched_RealFileMatch_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsFileSizeMatched_RealFileMatch_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_real_match_001");
    
    std::string testFilePath = "/data/local/tmp/test_real_match.jpg";
    std::ofstream testFile(testFilePath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testFilePath.c_str());
    }
    std::string content = "This is real test content for file size matching test";
    testFile << content;
    testFile.close();
    
    FileInfo fileInfo;
    fileInfo.fileSize = static_cast<int64_t>(content.size());
    
    int64_t actualSize = 0;
    RestoreError result = photosClone.IsFileSizeMatched(fileInfo, testFilePath, actualSize);
    EXPECT_EQ(result, RestoreError::SUCCESS);
    EXPECT_EQ(actualSize, static_cast<int64_t>(content.size()));
    
    MediaFileUtils::DeleteFile(testFilePath);
    MEDIA_INFO_LOG("IsFileSizeMatched_RealFileMatch_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_RealFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_RealFile_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_dedup_real_001");
    
    std::string testFilePath = "/data/local/tmp/test_dedup_real.jpg";
    std::ofstream testFile(testFilePath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testFilePath.c_str());
    }
    std::string content = "Real file content for deduplication test";
    testFile << content;
    testFile.close();
    
    FileInfo fileInfo;
    fileInfo.fileSize = static_cast<int64_t>(content.size());
    fileInfo.fileIdOld = 1;
    
    PhotosClone::DeduplicationInfo deduplicationInfo;
    deduplicationInfo.path = "/storage/emulated/0/Pictures/old.jpg";
    deduplicationInfo.newPath = testFilePath;
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, deduplicationInfo);
    EXPECT_TRUE(result);
    EXPECT_EQ(fileInfo.filePath, testFilePath);
    EXPECT_EQ(fileInfo.storagePath, testFilePath);
    EXPECT_EQ(fileInfo.displayName, "test_dedup_real.jpg");
    EXPECT_FALSE(fileInfo.inode.empty());
    
    MediaFileUtils::DeleteFile(testFilePath);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_RealFile_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_Success_Test_001 start");
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_dedup_success_001");
    
    std::string testFilePath = "/data/local/tmp/test_dedup_success.jpg";
    std::ofstream testFile(testFilePath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testFilePath.c_str());
    }
    testFile.close();
    
    FileInfo fileInfo;
    fileInfo.fileSize = 0;
    fileInfo.fileIdOld = 1;
    
    PhotosClone::DeduplicationInfo deduplicationInfo;
    deduplicationInfo.path = "/storage/emulated/0/Pictures/old.jpg";
    deduplicationInfo.newPath = testFilePath;
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, deduplicationInfo);
    EXPECT_TRUE(result);
    EXPECT_EQ(fileInfo.filePath, testFilePath);
    EXPECT_EQ(fileInfo.storagePath, testFilePath);
    EXPECT_EQ(fileInfo.displayName, "test_dedup_success.jpg");
    EXPECT_FALSE(fileInfo.inode.empty());
    
    MediaFileUtils::DeleteFile(testFilePath);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_Uninitialized_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Uninitialized_Test_001 start");
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test.jpg";
    fileInfos.push_back(fileInfo);
    
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), 1);
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Uninitialized_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_LakeFileNoMatch_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_LakeFileNoMatch_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_no_match_001");
    photosClone.InitDeduplicationInfo();
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileIdOld = 1;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/no_match_test.jpg";
    fileInfo.fileSize = 0;
    fileInfos.push_back(fileInfo);
    
    size_t originalSize = fileInfos.size();
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    EXPECT_EQ(fileInfos[0].filePath, fileInfos[0].storagePath);
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_LakeFileNoMatch_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_NonLakeFile_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_NonLakeFile_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_non_lake_001");
    photosClone.InitDeduplicationInfo();
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileIdOld = 1;
    fileInfo.fileSourceType = FileSourceType::MEDIA;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/normal_file.jpg";
    fileInfo.filePath = "/storage/cloud/files/Photo/16/normal_file.jpg";
    fileInfos.push_back(fileInfo);
    
    std::string originalFilePath = fileInfo.filePath;
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), 1);
    EXPECT_EQ(fileInfos[0].filePath, originalFilePath);
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_NonLakeFile_Test_001 end");
}

HWTEST_F(PhotosCloneTest, InitCloneRestoreRdbStore_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("InitCloneRestoreRdbStore_Success_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_rdb_success_001");
    
    int32_t ret = photosClone.InitCloneRestoreRdbStore();
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(photosClone.cloneRestoreRdbStore_, nullptr);
    MEDIA_INFO_LOG("InitCloneRestoreRdbStore_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, QueryDeduplicationFileInfo_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryDeduplicationFileInfo_Success_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_query_dedup_success_001");
    
    int32_t ret = photosClone.InitCloneRestoreRdbStore();
    ASSERT_EQ(ret, E_OK);
    
    std::unordered_map<std::string, PhotosClone::DeduplicationInfo> deduplicationMap;
    photosClone.QueryDeduplicationFileInfo(deduplicationMap);
    EXPECT_FALSE(deduplicationMap.empty());
    EXPECT_EQ(deduplicationMap.size(), 2);
    MEDIA_INFO_LOG("QueryDeduplicationFileInfo_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, InitDeduplicationInfo_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("InitDeduplicationInfo_Success_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_init_dedup_success_001");
    
    photosClone.InitDeduplicationInfo();
    EXPECT_FALSE(photosClone.deduplicationMap_.empty());
    EXPECT_EQ(photosClone.deduplicationMap_.size(), 2);
    MEDIA_INFO_LOG("InitDeduplicationInfo_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, ApplyDeduplicationFileInfo_WithDb_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_WithDb_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_apply_db_001");
    photosClone.InitDeduplicationInfo();
    
    std::string testNewPath = "/storage/emulated/0/Pictures/test_dedup_new.jpg";
    MediaFileUtils::CreateDirectory("/storage/emulated/0/Pictures/");
    std::ofstream testFile(testNewPath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testNewPath.c_str());
    }
    std::string content = "Real file content for deduplication with db test";
    testFile << content;
    testFile.close();
    
    FileInfo fileInfo;
    fileInfo.fileSize = static_cast<int64_t>(content.size());
    fileInfo.fileIdOld = 1;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test_dedup.jpg";
    
    auto dedupIt = photosClone.deduplicationMap_.find(fileInfo.storagePath);
    ASSERT_NE(dedupIt, photosClone.deduplicationMap_.end());
    
    bool result = photosClone.ApplyDeduplicationFileInfo(fileInfo, dedupIt->second);
    EXPECT_TRUE(result);
    EXPECT_EQ(fileInfo.storagePath, testNewPath);
    EXPECT_EQ(fileInfo.filePath, testNewPath);
    EXPECT_EQ(fileInfo.displayName, "test_dedup_new.jpg");
    
    MediaFileUtils::DeleteFile(testNewPath);
    MEDIA_INFO_LOG("ApplyDeduplicationFileInfo_WithDb_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_Match_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Match_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_match_001");
    photosClone.InitDeduplicationInfo();
    
    std::string testNewPath = "/storage/emulated/0/Pictures/test_dedup_new.jpg";
    MediaFileUtils::CreateDirectory("/storage/emulated/0/Pictures/");
    std::ofstream testFile(testNewPath);
    if (!testFile.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testNewPath.c_str());
    }
    std::string content = "Real file content for update fileInfo test";
    testFile << content;
    testFile.close();
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileIdOld = 1;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test_dedup.jpg";
    fileInfo.fileSize = static_cast<int64_t>(content.size());
    fileInfos.push_back(fileInfo);
    
    size_t originalSize = fileInfos.size();
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    EXPECT_EQ(fileInfos.size(), originalSize);
    EXPECT_EQ(fileInfos[0].storagePath, testNewPath);
    EXPECT_EQ(fileInfos[0].filePath, testNewPath);
    
    MediaFileUtils::DeleteFile(testNewPath);
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_Match_Test_001 end");
}

HWTEST_F(PhotosCloneTest, QueryLakeFileFailInfo_Success_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_Success_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_fail_info_001");
    
    int32_t ret = photosClone.InitCloneRestoreRdbStore();
    ASSERT_EQ(ret, E_OK);
    ASSERT_NE(photosClone.cloneRestoreRdbStore_, nullptr);
    
    std::unordered_map<std::string, FailedFileInfo> lakePhotoFailedFiles;
    std::unordered_map<std::string, FailedFileInfo> lakeVideoFailedFiles;
    photosClone.QueryLakeFileFailInfo(lakePhotoFailedFiles, lakeVideoFailedFiles);
    
    EXPECT_EQ(lakePhotoFailedFiles.size(), 0);
    EXPECT_EQ(lakeVideoFailedFiles.size(), 0);
    
    MEDIA_INFO_LOG("QueryLakeFileFailInfo_Success_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_MatchAndApplyFail_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_MatchAndApplyFail_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_match_fail_001");
    photosClone.InitDeduplicationInfo();
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo;
    fileInfo.fileIdOld = 1;
    fileInfo.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo.storagePath = "/storage/emulated/0/Pictures/test_dedup.jpg";
    fileInfo.fileSize = 1024;
    fileInfos.push_back(fileInfo);
    
    size_t originalSize = fileInfos.size();
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    
    EXPECT_EQ(fileInfos.size(), 0);
    
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_MatchAndApplyFail_Test_001 end");
}

HWTEST_F(PhotosCloneTest, UpdateFileInfoFromCloneRestoreDb_MultiFiles_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_MultiFiles_Test_001 start");
    CreateCloneRestoreTestDb();
    
    PhotosClone photosClone;
    photosClone.OnStart(g_rdbStore->GetRaw(), nullptr);
    photosClone.SetRestoreInfo(CLONE_RESTORE_ID, "task_multi_files_001");
    photosClone.InitDeduplicationInfo();
    
    std::string testNewPath1 = "/storage/emulated/0/Pictures/test_dedup_new.jpg";
    std::ofstream testFile1(testNewPath1);
    if (!testFile1.is_open()) {
        MEDIA_ERR_LOG("Failed to create test file: %{public}s", testNewPath1.c_str());
    }
    testFile1 << "test content";
    testFile1.close();
    
    std::vector<FileInfo> fileInfos;
    FileInfo fileInfo1;
    fileInfo1.fileIdOld = 1;
    fileInfo1.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    fileInfo1.storagePath = "/storage/emulated/0/Pictures/test_dedup.jpg";
    fileInfo1.fileSize = 12;
    fileInfos.push_back(fileInfo1);
    
    FileInfo fileInfo2;
    fileInfo2.fileIdOld = 3;
    fileInfo2.fileSourceType = FileSourceType::MEDIA;
    fileInfo2.storagePath = "/storage/emulated/0/Pictures/normal.jpg";
    fileInfo2.filePath = "/storage/cloud/files/Photo/16/normal.jpg";
    fileInfos.push_back(fileInfo2);
    
    photosClone.UpdateFileInfoFromCloneRestoreDb(fileInfos, AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED);
    
    EXPECT_EQ(fileInfos.size(), 2);
    EXPECT_EQ(fileInfos[0].storagePath, testNewPath1);
    EXPECT_EQ(fileInfos[1].filePath, "/storage/cloud/files/Photo/16/normal.jpg");
    
    MediaFileUtils::DeleteFile(testNewPath1);
    MEDIA_INFO_LOG("UpdateFileInfoFromCloneRestoreDb_MultiFiles_Test_001 end");
}
}  // namespace OHOS::Media
