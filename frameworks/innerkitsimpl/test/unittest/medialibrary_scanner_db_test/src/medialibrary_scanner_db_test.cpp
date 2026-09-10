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
#include <cstdint>
#include <thread>

#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_scanner_db_test.h"
#include "medialibrary_unittest_utils.h"
#include "thumbnail_utils.h"
#include "media_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#define private public
#include "media_scanner_db.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
class ConfigTestOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const string CREATE_TABLE_TEST;
};

const string ConfigTestOpenCall::CREATE_TABLE_TEST = string("CREATE TABLE IF NOT EXISTS test ") +
    "(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, age INTEGER, salary REAL, blobType BLOB)";

int ConfigTestOpenCall::OnCreate(NativeRdb::RdbStore &store)
{
    return store.ExecuteSql(CREATE_TABLE_TEST);
}

int ConfigTestOpenCall::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

shared_ptr<NativeRdb::RdbStore> storePtr = nullptr;
void MediaLibraryScannerDbTest::SetUpTestCase(void)
{
    const string dbPath = "/data/test/medialibrary_scanner_db_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    ConfigTestOpenCall helper;
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    storePtr = store;

    MediaLibraryUnitTestUtils::Init();
}

void MediaLibraryScannerDbTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibraryScannerDbTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore != nullptr && !MediaLibraryUnitTestUtils::CreateBasicTables(rdbStore)) {
        MEDIA_ERR_LOG("failed to create basic tables for shared media library db");
    }
}

void MediaLibraryScannerDbTest::TearDown(void) {}

HWTEST_F(MediaLibraryScannerDbTest, medialib_DeleteMetadata_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    string tableName = CONST_MEDIALIBRARY_TABLE;
    vector<string> idList;
    bool ret = mediaScannerDb.DeleteMetadata(idList, tableName);
    EXPECT_NE(ret, true);
    vector<string> idListTest = {"DeleteMetadata"};
    ret = mediaScannerDb.DeleteMetadata(idListTest, tableName);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_NotifyDatabaseChange_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    MediaType mediaType = MEDIA_TYPE_AUDIO;
    mediaScannerDb.NotifyDatabaseChange(mediaType);
    auto scannerTest =  MediaScannerDb::GetDatabaseInstance();
    EXPECT_NE(scannerTest, nullptr);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ReadAlbums_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    unordered_map<string, Metadata> albumMap_;
    string path = "/storage/cloud/files/";
    int32_t ret = mediaScannerDb.ReadAlbums(path, albumMap_);
    EXPECT_EQ(ret, E_OK);
    string pathTest = "";
    ret = mediaScannerDb.ReadAlbums(pathTest, albumMap_);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ReadError_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    set<string> ret = mediaScannerDb.ReadError();
    EXPECT_EQ(ret.size(), 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_InsertMetadata_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    mediaScannerDb.SetRdbHelper();
    Metadata metadata;
    string tableName;
    string ret = mediaScannerDb.InsertMetadata(metadata, tableName);
    EXPECT_NE(ret, "");
    int32_t parentId = 1;
    string albumPath = "/storage/cloud/files/";
    string albumName = "InsertMetadata";
    int32_t albumId = UNKNOWN_ID;
    struct stat statInfo;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(statInfo.st_mtime);
    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);
    metadata.SetFileId(albumId);
    ret = mediaScannerDb.InsertMetadata(metadata, tableName);
    EXPECT_NE(ret, "");
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_UpdateAlbum_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    Metadata metadata;
    int32_t ret = mediaScannerDb.UpdateAlbum(metadata);
    EXPECT_NE(ret, 0);
    int32_t parentId = 1;
    string albumPath = "/storage/cloud/files/";
    string albumName = "UpdateAlbum";
    int32_t albumId = UNKNOWN_ID;
    struct stat statInfo;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(statInfo.st_mtime);
    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);
    metadata.SetFileId(albumId);
    ret = mediaScannerDb.UpdateAlbum(metadata);
    EXPECT_NE(ret, 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_DeleteError_test_001, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaScannerDb mediaScannerDb;
    string err = "";
    int32_t ret = mediaScannerDb.DeleteError(err);
    EXPECT_EQ(ret, E_OK);
    string errTest = "DeleteError";
    ret = mediaScannerDb.DeleteError(err);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_RecordError_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    string err = "";
    int32_t ret = mediaScannerDb.RecordError(err);
    EXPECT_EQ(ret, E_OK);
    ret = mediaScannerDb.RecordError(err);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_InsertMetadata_test_002, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::Init();
    MediaScannerDb mediaScannerDb;
    mediaScannerDb.SetRdbHelper();
    Metadata metadata;
    int32_t parentId = 1;
    string albumPath = "/storage/cloud/files/";
    string albumName = "InsertMetadata";
    int32_t albumId = UNKNOWN_ID;
    struct stat statInfo;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(statInfo.st_mtime);
    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);
    metadata.SetFileId(albumId);
    string tableName;
    string ret = mediaScannerDb.InsertMetadata(metadata, tableName);
    EXPECT_NE(ret, "");
    unordered_map<string, Metadata> albumMap_;
    string pathTest = "";
    unordered_map<int32_t, MediaType> prevIdMap = mediaScannerDb.GetIdsFromFilePath(pathTest, tableName);
    EXPECT_GT(prevIdMap.size(), 0);
    string path = "/storage/cloud/files/";
    prevIdMap = mediaScannerDb.GetIdsFromFilePath(path, tableName);
    EXPECT_GT(prevIdMap.size(), 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ReadAlbums_test_002, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    unordered_map<string, Metadata> albumMap_;
    string path = "/storage/cloud/files/";
    int32_t ret = mediaScannerDb.ReadAlbums(path, albumMap_);
    EXPECT_EQ(ret, E_OK);
    string pathTest = "";
    ret = mediaScannerDb.ReadAlbums(pathTest, albumMap_);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ReadError_test_002, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    set<string> ret = mediaScannerDb.ReadError();
    EXPECT_NE(ret.size(), 0);
    MediaLibraryUnitTestUtils::Init();
    ret = mediaScannerDb.ReadError();
    EXPECT_NE(ret.size(), 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_DeleteError_test_002, TestSize.Level1)
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaScannerDb mediaScannerDb;
    string err = "";
    int32_t ret = mediaScannerDb.DeleteError(err);
    EXPECT_EQ(ret, E_OK);
    string errTest = "DeleteError";
    ret = mediaScannerDb.DeleteError(err);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_GetFileDBUriFromPath_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    string path = "";
    string uri = mediaScannerDb.GetFileDBUriFromPath(path);
    EXPECT_EQ(uri, "");
    string pathTest = "/storage/cloud/files/";
    uri = mediaScannerDb.GetFileDBUriFromPath(pathTest);
    EXPECT_EQ(uri, "");
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_InsertAlbum_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    Metadata metadata;
    int32_t ret = mediaScannerDb.InsertAlbum(metadata);
    EXPECT_NE(ret, 0);
    int32_t parentId = 1;
    string albumPath = "/storage/cloud/files/";
    string albumName = "InsertAlbum";
    int32_t albumId = UNKNOWN_ID;
    struct stat statInfo;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(statInfo.st_mtime);
    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);
    metadata.SetFileId(albumId);
    ret = mediaScannerDb.InsertAlbum(metadata);
    EXPECT_NE(ret, 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_UpdateMetadata_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    Metadata metadata;
    string tableName;
    string ret = mediaScannerDb.UpdateMetadata(metadata, tableName);
    EXPECT_EQ(ret, "");
    int32_t parentId = 1;
    string albumPath = "/storage/cloud/files/";
    string albumName = "UpdateMetadata";
    int32_t albumId = UNKNOWN_ID;
    struct stat statInfo;
    metadata.SetFilePath(albumPath);
    metadata.SetFileName(ScannerUtils::GetFileNameFromUri(albumPath));
    metadata.SetFileTitle(ScannerUtils::GetFileTitle(metadata.GetFileName()));
    metadata.SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    metadata.SetFileSize(statInfo.st_size);
    metadata.SetFileDateModified(statInfo.st_mtime);
    string relativePath = ScannerUtils::GetParentPath(albumPath) + SLASH_CHAR;
    metadata.SetRelativePath(relativePath.erase(0, ROOT_MEDIA_DIR.length()));
    metadata.SetParentId(parentId);
    metadata.SetAlbumName(albumName);
    metadata.SetFileId(albumId);
    ret = mediaScannerDb.UpdateMetadata(metadata, tableName);
    EXPECT_EQ(ret, "");
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_GetIdFromPath_test_001, TestSize.Level1)
{
    MediaScannerDb mediaScannerDb;
    unordered_map<string, Metadata> albumMap_;
    string pathTest = "";
    int32_t id = mediaScannerDb.GetIdFromPath(pathTest);
    EXPECT_EQ(id, -1);
    string path = "/storage/cloud/files/";
    id = mediaScannerDb.GetIdFromPath(path);
    EXPECT_EQ(id, -1);
}

static string QueryPhotoStringColumn(const shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t fileId,
    const string &column)
{
    string sql = "SELECT " + column + " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        MediaColumn::MEDIA_ID + " = " + to_string(fileId);
    auto resultSet = rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return "";
    }
    return GetStringVal(column, resultSet);
}

static void ClearPhotoRowByPath(const shared_ptr<MediaLibraryRdbStore> &rdbStore, const string &filePath)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, filePath);
    int32_t deletedRows = 0;
    int32_t ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearPhotoRowByPath ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

/**
 * @tc.name: medialib_UpdateMetadata_cinematic_v2_skip_shooting_mode_test_001
 * @tc.desc: 电影模式V2视频由相机侧写入shooting_mode字段, 扫描器更新元数据时应跳过这两个字段
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryScannerDbTest, medialib_UpdateMetadata_cinematic_v2_skip_shooting_mode_test_001,
    TestSize.Level1)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    ClearPhotoRowByPath(rdbStore, "/storage/cloud/files/Photo/1/cinematic_v2_scan.mp4");

    // 1. 预置一条subtype=CINEMATIC_VIDEO_V2的视频记录(最小列, 对齐仓库既有成功先例)
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/1/cinematic_v2_scan.mp4");
    values.PutString(MediaColumn::MEDIA_NAME, "cinematic_v2_scan.mp4");
    values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO_V2));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    int32_t fileId = static_cast<int32_t>(rowId);

    // 2. 模拟相机侧Insert后Update写入shooting_mode/shooting_mode_tag(等价UpdatePhotoProxy时序)
    NativeRdb::RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, "24");
    updateValues.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, "2.0");
    int32_t changedRows = 0;
    EXPECT_EQ(rdbStore->Update(changedRows, updateValues, updatePredicates), NativeRdb::E_OK);

    // 3. 扫描器以空shooting_mode更新该视频元数据
    MediaScannerDb mediaScannerDb;
    Metadata metadata;
    string tableName;
    metadata.SetFilePath("/storage/cloud/files/Photo/1/cinematic_v2_scan.mp4");
    metadata.SetFileName("cinematic_v2_scan.mp4");
    metadata.SetFileTitle("cinematic_v2_scan");
    metadata.SetFileMediaType(MediaType::MEDIA_TYPE_VIDEO);
    metadata.SetFileMimeType("video/mp4");
    metadata.SetFileId(fileId);
    string retUri = mediaScannerDb.UpdateMetadata(metadata, tableName, MediaLibraryApi::API_10);
    MEDIA_INFO_LOG("UpdateMetadata retUri: %{public}s, tableName: %{public}s", retUri.c_str(), tableName.c_str());

    // 4. 相机侧写入的shooting_mode/shooting_mode_tag不应被扫描覆盖
    EXPECT_EQ(QueryPhotoStringColumn(rdbStore, fileId, PhotoColumn::PHOTO_SHOOTING_MODE), "24");
    EXPECT_EQ(QueryPhotoStringColumn(rdbStore, fileId, PhotoColumn::PHOTO_SHOOTING_MODE_TAG), "2.0");

    // 5. 清理用例数据
    NativeRdb::RdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
    deletePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t deletedRows = -1;
    rdbStore->Delete(deletedRows, deletePredicates);
}

/**
 * @tc.name: medialib_UpdateMetadata_cinematic_v2_skip_shooting_mode_test_002
 * @tc.desc: 非V2视频(DEFAULT)扫描更新时会覆盖shooting_mode字段(对照)
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryScannerDbTest, medialib_UpdateMetadata_cinematic_v2_skip_shooting_mode_test_002,
    TestSize.Level1)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
    ClearPhotoRowByPath(rdbStore, "/storage/cloud/files/Photo/1/normal_video_scan.mp4");

    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, "/storage/cloud/files/Photo/1/normal_video_scan.mp4");
    values.PutString(MediaColumn::MEDIA_NAME, "normal_video_scan.mp4");
    values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(ret, NativeRdb::E_OK);
    int32_t fileId = static_cast<int32_t>(rowId);

    // 模拟相机侧Insert后Update写入shooting_mode/shooting_mode_tag
    NativeRdb::RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutString(PhotoColumn::PHOTO_SHOOTING_MODE, "24");
    updateValues.PutString(PhotoColumn::PHOTO_SHOOTING_MODE_TAG, "2.0");
    int32_t changedRows = 0;
    EXPECT_EQ(rdbStore->Update(changedRows, updateValues, updatePredicates), NativeRdb::E_OK);

    MediaScannerDb mediaScannerDb;
    Metadata metadata;
    string tableName;
    metadata.SetFilePath("/storage/cloud/files/Photo/1/normal_video_scan.mp4");
    metadata.SetFileName("normal_video_scan.mp4");
    metadata.SetFileTitle("normal_video_scan");
    metadata.SetFileMediaType(MediaType::MEDIA_TYPE_VIDEO);
    metadata.SetFileMimeType("video/mp4");
    metadata.SetFileId(fileId);
    string retUri = mediaScannerDb.UpdateMetadata(metadata, tableName, MediaLibraryApi::API_10);
    MEDIA_INFO_LOG("UpdateMetadata retUri: %{public}s, tableName: %{public}s", retUri.c_str(), tableName.c_str());

    // 非V2视频: 扫描解析为空时会把shooting_mode覆盖为空
    EXPECT_EQ(QueryPhotoStringColumn(rdbStore, fileId, PhotoColumn::PHOTO_SHOOTING_MODE), "");

    // 清理用例数据
    NativeRdb::RdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
    deletePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t deletedRows = -1;
    rdbStore->Delete(deletedRows, deletePredicates);
}
} // namespace Media
} // namespace OHOS