/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryMediaFileAccessUtilsTestExtra"
#include "media_file_access_utils_extra_test.h"

#include <functional>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <unordered_set>
#include <unistd.h>

#include "ability_context_impl.h"
#include "base_data_uri.h"
#include "directory_ex.h"
#include "file_asset.h"
#include "get_self_permissions.h"
#include "media_file_access_utils.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_type_const.h"
#include "media_audio_column.h"
#include "media_column.h"
#include "media_upgrade.h"
#include "media_unique_number_column.h"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "media_library_manager.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"

using namespace testing;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
mutex MediaLibraryMediaFileAccessUtilsTestExtra::MutexExtra_;
bool MediaLibraryMediaFileAccessUtilsTestExtra::isValidExtra_ = false;
bool MediaLibraryMediaFileAccessUtilsTestExtra::dbIsSupportedExtra_ = false;
const std::string TEST_FILE_ROOT_EXTRA = "/data/local/tmp/file_access_utils/";
const std::string TEST_FILE_PATH_EXTRA = TEST_FILE_ROOT_EXTRA + "CreateImageLcdTest_001.jpg";
const std::string FILE_MANAGER_STORAGE_DIR_EXTRA = "/storage/media/local/files/Docs/TestFileManager/";
const std::string LAKE_STORAGE_DIR_EXTRA = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
const std::string MEDIA_PHOTO_DIR_TEST_EXTRA = ROOT_MEDIA_DIR + "Photo/";
const std::string MEDIALIBRARY_ZERO_BUCKET_PATH_EXTRA = MEDIA_PHOTO_DIR_TEST_EXTRA + "0/";
constexpr int STORAGE_MANAGER_MANAGER_ID_EXTRA = 5003;
constexpr int32_t MAX_DELETE_ATTEMPTS_EXTRA = 1;

static shared_ptr<MediaLibraryRdbStore> g_rdbStoreExtra;

static void PrepareUniqueNumberTableForTestExtra()
{
    if (g_rdbStoreExtra == nullptr) {
        MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: can not get g_rdbStore");
        return;
    }
    std::string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = g_rdbStoreExtra->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: can not get AssetUniqueNumberTable count");
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        MEDIA_DEBUG_LOG("PrepareUniqueNumberTableForTest: AssetUniqueNumberTable already inited");
        return;
    }
    NativeRdb::ValuesBucket imageBucket;
    imageBucket.PutString(ASSET_MEDIA_TYPE, CONST_IMAGE_ASSET_TYPE);
    imageBucket.PutInt(UNIQUE_NUMBER, 1);
    NativeRdb::ValuesBucket videoBucket;
    videoBucket.PutString(ASSET_MEDIA_TYPE, CONST_VIDEO_ASSET_TYPE);
    videoBucket.PutInt(UNIQUE_NUMBER, 1);
    NativeRdb::ValuesBucket audioBucket;
    audioBucket.PutString(ASSET_MEDIA_TYPE, CONST_AUDIO_ASSET_TYPE);
    audioBucket.PutInt(UNIQUE_NUMBER, 1);
    std::vector<NativeRdb::ValuesBucket> buckets = { imageBucket, videoBucket, audioBucket };
    for (auto &bucket : buckets) {
        int64_t outRowId = -1;
        int32_t insertResult = g_rdbStoreExtra->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, bucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            MEDIA_ERR_LOG("PrepareUniqueNumberTableForTest: insert AssetUniqueNumberTable failed, ret=%{public}d",
                insertResult);
        }
    }
    MEDIA_INFO_LOG("PrepareUniqueNumberTableForTest: init AssetUniqueNumberTable done");
}

MediaLibraryManager* mediaLibraryManagerExtra = MediaLibraryManager::GetMediaLibraryManager();
std::vector<std::string> MediaLibraryMediaFileAccessUtilsTestExtra::initAssetFileIdsExtra_;

const static vector<string> PHOTO_COLUMN_VECTOR_EXTRA = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_STORAGE_PATH,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
};

static void SafeLogExtra(const std::string &tag, const std::string &msg)
{
    (void)tag;
    (void)msg;
}

void MediaLibraryMediaFileAccessUtilsTestExtra::CleanAssetResource()
{
    std::vector<std::string> cleanIds;
    {
        std::lock_guard<std::mutex> lock(MutexExtra_);
        cleanIds.swap(initAssetFileIdsExtra_);
    }
    if (cleanIds.empty()) {
        return;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return;
    }
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, cleanIds);
    int32_t changedRows = 0;
    int32_t ret = rdbStore->Delete(changedRows, predicates);
    if (ret != E_OK) {
        return;
    }
    std::unordered_set<std::string> cleanupPathSet;
    for (const auto &id : cleanIds) {
        if (id.empty()) {
            continue;
        }
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR_EXTRA);
        if (fileAsset == nullptr) {
            continue;
        }
        std::string dataPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        if (!dataPath.empty()) {
            cleanupPathSet.emplace(dataPath);
        }
        if (!storagePath.empty()) {
            cleanupPathSet.emplace(storagePath);
        }
    }
    for (const auto &path : cleanupPathSet) {
        if (path.empty() || !MediaFileUtils::IsFileExists(path)) {
            continue;
        }
        for (int32_t attempt = 1; attempt <= MAX_DELETE_ATTEMPTS_EXTRA; ++attempt) {
            if (MediaFileUtils::DeleteFile(path) || !MediaFileUtils::IsFileExists(path)) {
                break;
            }
        }
    }
}

void MediaLibraryMediaFileAccessUtilsTestExtra::CreateDataHelper(int32_t systemAbilityId)
{
    MEDIA_INFO_LOG("CreateDataHelper start");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    ASSERT_NE(remoteObj, nullptr);

    mediaLibraryManagerExtra->InitMediaLibraryManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");
}

void MediaLibraryMediaFileAccessUtilsTestExtra::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTestExtra SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    MediaLibraryMediaFileAccessUtilsTestExtra::InitMediaLibrary();
    g_rdbStoreExtra = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStoreExtra, nullptr);
    std::vector<std::string> createTableSqlLists = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        AudioColumn::CREATE_AUDIO_TABLE,
        CREATE_MEDIA_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlLists) {
        int32_t ret = g_rdbStoreExtra->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("CreateTestTables execute sql failed, ret=%{public}d", ret);
        }
    }
    PrepareUniqueNumberTableForTestExtra();
    MediaLibraryMediaFileAccessUtilsTestExtra::CreateDataHelper(STORAGE_MANAGER_MANAGER_ID_EXTRA);
}

void MediaLibraryMediaFileAccessUtilsTestExtra::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTestExtra TearDownTestCase");

    MediaLibraryMediaFileAccessUtilsTestExtra::CleanAssetResource();

    bool deleteRet = MediaFileUtils::DeleteDir(TEST_FILE_ROOT_EXTRA);
    bool isRootCleared = deleteRet || !MediaFileUtils::IsDirExists(TEST_FILE_ROOT_EXTRA);
    CHECK_AND_PRINT_LOG(isRootCleared,
        "Delete test file root failed, path: %{public}s, errno: %{public}d", TEST_FILE_ROOT_EXTRA.c_str(), errno);
    ASSERT_TRUE(isRootCleared);
    MEDIA_INFO_LOG("TearDownTestCase end");
}

void MediaLibraryMediaFileAccessUtilsTestExtra::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTestExtra SetUp");
    if (!MediaLibraryMediaFileAccessUtilsTestExtra::IsValid()) {
        MediaLibraryMediaFileAccessUtilsTestExtra::InitMediaLibrary();
    }
}

void MediaLibraryMediaFileAccessUtilsTestExtra::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTestExtra TearDown");
}

bool MediaLibraryMediaFileAccessUtilsTestExtra::IsValid()
{
    return isValidExtra_;
}

void MediaLibraryMediaFileAccessUtilsTestExtra::InitMediaLibrary()
{
    std::lock_guard<std::mutex> lock(MutexExtra_);
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    EXPECT_EQ(ret, E_OK);
    isValidExtra_ = true;
}

int32_t MediaLibraryMediaFileAccessUtilsTestExtra::CreatePhotoApi10(int mediaType, const string &displayName,
    bool isPhotoEdited, bool isMovingPhoto)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    if (isMovingPhoto) {
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    }
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    MEDIA_INFO_LOG("CreatePhotoApi10 ret = %{public}d", ret);
    return ret;
}

bool CheckAndCreateDirExtra(const std::string &assetPath)
{
    std::string assetParentPath = MediaFileUtils::GetParentPath(assetPath);
    if (!MediaFileUtils::IsDirExists(assetParentPath)) {
        MediaFileUtils::CreateDirectory(assetParentPath);
    }
    bool isParentDirExist = MediaFileUtils::IsDirExists(assetParentPath);
    if (!isParentDirExist) {
        std::error_code ec;
        std::filesystem::create_directories(assetParentPath, ec);
        isParentDirExist = MediaFileUtils::IsDirExists(assetParentPath);
    }
    return isParentDirExist;
}

bool CreateFileWithDataExtra(const std::string &filePath, const std::string &data)
{
    std::filesystem::path parentPath = std::filesystem::path(filePath).parent_path();
    std::error_code ec;
    std::filesystem::create_directories(parentPath, ec);
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.is_open()) {
        MEDIA_ERR_LOG("Failed to open file: %{public}s", filePath.c_str());
        return false;
    }
    ofs << data;
    return true;
}

bool CreateSparseFileExtra(const std::string &filePath, size_t fileSize)
{
    if (fileSize == 0) {
        return false;
    }
    int32_t fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        MEDIA_ERR_LOG("Failed to create sparse file, path: %{public}s, errno: %{public}d", filePath.c_str(), errno);
        return false;
    }
    UniqueFd fdGuard(fd);
    off_t seekRet = lseek(fd, static_cast<off_t>(fileSize - 1), SEEK_SET);
    if (seekRet < 0) {
        MEDIA_ERR_LOG("Failed to lseek sparse file, path: %{public}s, errno: %{public}d", filePath.c_str(), errno);
        return false;
    }
    uint8_t tail = 0;
    ssize_t writeRet = write(fd, &tail, sizeof(tail));
    if (writeRet != static_cast<ssize_t>(sizeof(tail))) {
        MEDIA_ERR_LOG("Failed to finalize sparse file, path: %{public}s, errno: %{public}d", filePath.c_str(), errno);
        return false;
    }
    return true;
}

bool ReadFileToStringExtra(const std::string &filePath, std::string &content)
{
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        MEDIA_ERR_LOG("Failed to open file for reading: %{public}s", filePath.c_str());
        return false;
    }
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return true;
}

void MediaLibraryMediaFileAccessUtilsTestExtra::InitTestFileAsset(const std::string &path, FileSourceType sourceType)
{
    EXPECT_NE(path, "");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);
    NativeRdb::ValuesBucket values;
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, path);
    string fileName = MediaFileUtils::GetFileName(path);
    EXPECT_NE(fileName, "");
    if (sourceType == FileSourceType::FILE_MANAGER) {
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, FILE_MANAGER_STORAGE_DIR_EXTRA + fileName);
    } else if (sourceType == FileSourceType::MEDIA_HO_LAKE) {
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
        values.PutString(MediaColumn::MEDIA_FILE_PATH, MEDIALIBRARY_ZERO_BUCKET_PATH_EXTRA + fileName);
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, LAKE_STORAGE_DIR_EXTRA + fileName);
    } else {
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));
    }
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("InitTestFileAsset update failed, file_path: %{public}s", path.c_str());
    } else {
        MEDIA_INFO_LOG("InitTestFileAsset update success, file_path: %{public}s, changedRows: %{public}d",
            path.c_str(), changedRows);
    }
}

void MediaLibraryMediaFileAccessUtilsTestExtra::InitTestFileAsset(const std::string &path,
    const std::string &albumOwnerId, const std::string &displayName)
{
    EXPECT_NE(path, "");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);
    NativeRdb::ValuesBucket values;
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, path);
    string fileName = MediaFileUtils::GetFileName(path);
    EXPECT_NE(fileName, "");
    values.PutString(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumOwnerId);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("InitTestFileAsset update failed, file_path: %{public}s", path.c_str());
    } else {
        MEDIA_INFO_LOG("InitTestFileAsset update success, file_path: %{public}s, changedRows: %{public}d",
            path.c_str(), changedRows);
    }
}

void MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(std::string &dataFileUri, FileSourceType sourceType)
{
    std::string testFileName = MediaFileUtils::GetFileName(TEST_FILE_PATH_EXTRA);
    int32_t dataFileId = 0;

    dataFileId = MediaLibraryMediaFileAccessUtilsTestExtra::CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, testFileName,
        false);
    ASSERT_GT(dataFileId, 0);
    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(dataFileId), "", MEDIA_API_VERSION_V10);
    dataFileUri = fileUri.ToString();
    string id = MediaFileUtils::GetIdFromUri(dataFileUri);
    {
        std::lock_guard<std::mutex> lock(MutexExtra_);
        initAssetFileIdsExtra_.push_back(id);
    }
    std::vector<std::string> queryColumns = { PhotoColumn::MEDIA_FILE_PATH };
    std::shared_ptr<FileAsset> fileAssetA = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, queryColumns);
    ASSERT_NE(fileAssetA, nullptr);
    std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR_EXTRA);
    if (fileAsset == nullptr) {
        dbIsSupportedExtra_ = false;
        return;
    } else {
        dbIsSupportedExtra_ = true;
    }
    std::string assetPath = fileAsset->GetPath();
    MediaLibraryMediaFileAccessUtilsTestExtra::InitTestFileAsset(assetPath, sourceType);
    fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR_EXTRA);
    ASSERT_NE(fileAsset, nullptr);
    if (sourceType == FileSourceType::FILE_MANAGER || sourceType == FileSourceType::MEDIA_HO_LAKE) {
        assetPath = fileAsset->GetStoragePath();
        EXPECT_NE(assetPath, "");
    } else {
        assetPath = fileAsset->GetPath();
        EXPECT_NE(assetPath, "");
    }
    GTEST_LOG_(INFO) << "InitAsset sourceType: " << static_cast<int32_t>(sourceType) << ", assetPath: " << assetPath;
    bool isParentDirExist = CheckAndCreateDirExtra(assetPath);
    EXPECT_TRUE(isParentDirExist);

    if (!MediaFileUtils::IsFileExists(TEST_FILE_PATH_EXTRA)) {
        ASSERT_TRUE(CreateFileWithDataExtra(TEST_FILE_PATH_EXTRA, "init_asset_seed"));
    }
    MediaFileUtils::CopyFileSafe(TEST_FILE_PATH_EXTRA, assetPath);
    bool isfileExist = MediaFileUtils::IsFileExists(assetPath);
    EXPECT_TRUE(isfileExist);
}

bool MediaLibraryMediaFileAccessUtilsTestExtra::CheckDBIsSupported()
{
    return dbIsSupportedExtra_;
}

void MediaLibraryMediaFileAccessUtilsTestExtra::CopyToDestPath(int32_t srcFd, const std::string &destPath)
{
    MEDIA_INFO_LOG("CopyToDestPath start, destPath: %{public}s", destPath.c_str());
    int32_t ret = E_OK;
    std::string destDir = MediaFileUtils::GetParentPath(destPath);
    if (!MediaFileUtils::IsDirExists(destDir)) {
        MEDIA_INFO_LOG("CopyToDestPath destDir is not exist, create dir: %{public}s", destDir.c_str());
        ret = MediaFileUtils::CreateDirectory(destDir);
        if (ret != E_OK) {
            std::filesystem::create_directories(destDir);
            MEDIA_INFO_LOG("destDir is exist? : %{public}d", MediaFileUtils::IsDirExists(destDir));
        }
    }
    if (MediaFileUtils::IsFileExists(destPath)) {
        ret = MediaFileUtils::DeleteFile(destPath);
    }

    int outFd = open(destPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    bool copySuccess = MediaFileUtils::CopyFile(srcFd, outFd);
    UniqueFd outFdGuard(outFd);
    EXPECT_TRUE(copySuccess);
    MEDIA_INFO_LOG("CopyToDestPath end");
}

void MediaLibraryMediaFileAccessUtilsTestExtra::RunSameNameRenameCase(const std::string &sameNamePath,
    const std::vector<std::string> &existingPaths, const std::string &expectedPath)
{
    std::vector<std::string> cleanupPaths = existingPaths;
    cleanupPaths.push_back(sameNamePath);
    cleanupPaths.push_back(expectedPath);
    for (const auto &path : cleanupPaths) {
        MediaFileUtils::DeleteFile(path);
    }

    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "same_name"));
    for (const auto &path : existingPaths) {
        ASSERT_TRUE(CreateFileWithDataExtra(path, "same_name"));
    }

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName);
    EXPECT_EQ(ret, E_OK);
    EXPECT_NE(renamePath, sameNamePath);
    EXPECT_EQ(renamePath, expectedPath);
    EXPECT_NE(renameDisplayName, MediaFileUtils::GetFileName(sameNamePath));
    EXPECT_NE(renameTitle, "");

    for (const auto &path : cleanupPaths) {
        MediaFileUtils::DeleteFile(path);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_all_accessors_001, TestSize.Level0)
{
    AssetOperationInfo info = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    info.SetFileId("10");
    info.SetAssetPath("/data/local/tmp/a.jpg");
    info.SetStoragePath("/data/local/tmp/s.jpg");
    info.SetFileSourceType(FileSourceType::FILE_MANAGER);
    info.SetSubType(PhotoSubType::BURST);
    info.SetOwnerAlbumId("20");
    info.SetBurstCoverLevel(BurstCoverLevelType::MEMBER);
    info.SetBurstKey("bk");
    auto fakeAsset = std::make_shared<FileAsset>();
    info.SetAssetInfo(fakeAsset);
    auto fakeRefresh = std::make_shared<AssetAccurateRefresh>();
    info.SetAssetRefresh(fakeRefresh);

    EXPECT_EQ(info.GetFileId(), "10");
    EXPECT_EQ(info.GetAssetPath(), "/data/local/tmp/a.jpg");
    EXPECT_EQ(info.GetStoragePath(), "/data/local/tmp/s.jpg");
    EXPECT_EQ(info.GetFileSourceType(), FileSourceType::FILE_MANAGER);
    EXPECT_EQ(info.GetSubType(), PhotoSubType::BURST);
    EXPECT_EQ(info.GetOwnerAlbumId(), "20");
    EXPECT_EQ(info.GetBurstCoverLevel(), BurstCoverLevelType::MEMBER);
    EXPECT_EQ(info.GetBurstKey(), "bk");
    EXPECT_EQ(info.GetAssetInfo(), fakeAsset);
    EXPECT_EQ(info.GetAssetRefresh(), fakeRefresh);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_is_valid_combos_001, TestSize.Level0)
{
    AssetOperationInfo empty = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    EXPECT_FALSE(empty.IsValid());

    AssetOperationInfo onlyFileId = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    onlyFileId.SetFileId("1");
    EXPECT_TRUE(onlyFileId.IsValid());

    AssetOperationInfo onlyData = AssetOperationInfo::CreateFromPath("/data/local/tmp/x.txt",
        AssetPathType::NORMAL_PATH);
    EXPECT_TRUE(onlyData.IsValid());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_init_by_path_null_001, TestSize.Level0)
{
    AssetOperationInfo info = AssetOperationInfo::CreateFromPath("/data/local/tmp/not_exist_path.jpg",
        AssetPathType::NORMAL_PATH);
    info.Reset();
    EXPECT_FALSE(info.Init());
    EXPECT_FALSE(info.IsInfoAvailable());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_init_by_id_null_001, TestSize.Level0)
{
    AssetOperationInfo info = AssetOperationInfo::CreateFromFileId("999999");
    info.Reset();
    EXPECT_FALSE(info.Init());
    EXPECT_FALSE(info.IsInfoAvailable());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_init_empty_001, TestSize.Level0)
{
    AssetOperationInfo info = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    info.Reset();
    EXPECT_FALSE(info.Init());
    EXPECT_FALSE(info.IsInfoAvailable());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_init_already_success_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        AssetOperationInfo info = AssetOperationInfo::CreateFromFileId(id);
        EXPECT_TRUE(info.IsInfoAvailable());
        EXPECT_TRUE(info.Init());
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_subtype_moving_photo_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        ASSERT_NE(rdbStore, nullptr);
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        int32_t changedRows = -1;
        EXPECT_EQ(rdbStore->Update(changedRows, values, predicates), E_OK);

        AssetOperationInfo info = AssetOperationInfo::CreateFromFileId(id);
        EXPECT_TRUE(info.IsInfoAvailable());
        EXPECT_EQ(info.GetSubType(), PhotoSubType::MOVING_PHOTO);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_init_burst_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        ASSERT_NE(rdbStore, nullptr);
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        int32_t changedRows = -1;
        EXPECT_EQ(rdbStore->Update(changedRows, values, predicates), E_OK);

        AssetOperationInfo info = AssetOperationInfo::CreateFromFileId(id);
        EXPECT_TRUE(info.IsInfoAvailable());
        EXPECT_EQ(info.GetSubType(), PhotoSubType::BURST);
        EXPECT_EQ(info.GetBurstKey(), "");
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, asset_operation_info_subtype_effect_mode_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        ASSERT_NE(rdbStore, nullptr);
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        int32_t changedRows = -1;
        EXPECT_EQ(rdbStore->Update(changedRows, values, predicates), E_OK);

        AssetOperationInfo info = AssetOperationInfo::CreateFromFileId(id);
        EXPECT_TRUE(info.IsInfoAvailable());
        EXPECT_EQ(info.GetSubType(), PhotoSubType::MOVING_PHOTO);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_get_real_path_no_convert_001, TestSize.Level0)
{
    std::string path = "/data/local/tmp/file_access_utils_noconvert.txt";
    MediaFileUtils::DeleteFile(path);
    ASSERT_TRUE(CreateFileWithDataExtra(path, "noconvert"));
    std::string realPath = MediaFileAccessUtils::GetAssetRealPath(path);
    EXPECT_EQ(realPath, path);
    MediaFileUtils::DeleteFile(path);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_get_real_path_not_in_db_001, TestSize.Level0)
{
    std::string path = ROOT_MEDIA_DIR + "Photo/1/file_access_utils_not_in_db.jpg";
    std::string realPath = MediaFileAccessUtils::GetAssetRealPath(path);
    EXPECT_EQ(realPath, path);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_get_real_path_cloud_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        ASSERT_NE(rdbStore, nullptr);
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        int32_t changedRows = -1;
        EXPECT_EQ(rdbStore->Update(changedRows, values, predicates), E_OK);

        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR_EXTRA);
        ASSERT_NE(fileAsset, nullptr);
        std::string assetPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_EQ(realPath, assetPath);
        EXPECT_NE(realPath, storagePath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_get_real_path_by_id_empty_001, TestSize.Level0)
{
    EXPECT_EQ(MediaFileAccessUtils::GetAssetRealPathById(""), "");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_get_real_path_by_id_nonexistent_001, TestSize.Level0)
{
    EXPECT_EQ(MediaFileAccessUtils::GetAssetRealPathById("999999"), "");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_get_real_path_by_id_cloud_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        ASSERT_NE(rdbStore, nullptr);
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
        NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        int32_t changedRows = -1;
        EXPECT_EQ(rdbStore->Update(changedRows, values, predicates), E_OK);

        std::string realPath = MediaFileAccessUtils::GetAssetRealPathById(id);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR_EXTRA);
        ASSERT_NE(fileAsset, nullptr);
        EXPECT_EQ(realPath, fileAsset->GetPath());
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_get_real_path_from_obj_invalid_001, TestSize.Level0)
{
    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    EXPECT_EQ(MediaFileAccessUtils::GetAssetRealPath(srcObj), "");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_update_modify_time_empty_001, TestSize.Level0)
{
    MediaFileAccessUtils::UpdateModifyTime("", 1739386800000);
    EXPECT_TRUE(true);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_move_asset_empty_dest_001, TestSize.Level0)
{
    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath("/data/local/tmp/file_access_utils_me.txt",
        AssetPathType::NORMAL_PATH);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, "", FileSourceType::FILE_MANAGER);
    EXPECT_EQ(result.errCode, E_ERR);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_move_asset_invalid_src_001, TestSize.Level0)
{
    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, "/data/local/tmp/file_access_utils_d.txt",
        FileSourceType::FILE_MANAGER);
    EXPECT_EQ(result.errCode, E_ERR);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_move_asset_moving_photo_same_type_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_mp_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_mp_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
    ASSERT_TRUE(CreateFileWithDataExtra(srcPath, "mp"));

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(srcPath, AssetPathType::NORMAL_PATH);
    srcObj.SetSubType(PhotoSubType::MOVING_PHOTO);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA);
    EXPECT_EQ(result.errCode, E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(destPath));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(srcPath));

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_move_asset_src_not_exist_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_nonexist_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_nonexist_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(srcPath, AssetPathType::NORMAL_PATH);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER);
    EXPECT_NE(result.errCode, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(destPath));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_same_name_rename_4arg_no_conflict_001,
    TestSize.Level0)
{
    std::string sameNamePath = "/data/local/tmp/file_access_utils_hsr.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "data"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    auto checker = [](const std::string &) { return false; };
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName, checker);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(renamePath, sameNamePath);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_same_name_rename_4arg_empty_checker_001,
    TestSize.Level0)
{
    std::string sameNamePath = "/data/local/tmp/file_access_utils_hsr.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "data"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    std::function<bool(const std::string &)> emptyChecker;
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName, emptyChecker);
    EXPECT_EQ(ret, E_ERR);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_same_name_rename_4arg_exhausted_001,
    TestSize.Level1)
{
    std::string sameNamePath = "/data/local/tmp/file_access_utils_hsr_exh.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "data"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    auto alwaysChecker = [](const std::string &) { return true; };
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName, alwaysChecker);
    EXPECT_EQ(ret, E_ERR);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_same_name_rename_empty_path_001,
    TestSize.Level0)
{
    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename("", renamePath, renameTitle, renameDisplayName);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_burst_same_name_no_conflict_001,
    TestSize.Level0)
{
    std::string sameNamePath = "/data/local/tmp/file_access_utils_burst.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "data"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    auto checker = [](const std::string &) { return false; };
    int32_t ret = MediaFileAccessUtils::HandleBurstSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName, checker);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(renamePath, sameNamePath);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_handle_burst_same_name_exhausted_001,
    TestSize.Level1)
{
    std::string sameNamePath = "/data/local/tmp/file_access_utils_burst_exh.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithDataExtra(sameNamePath, "data"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    auto alwaysChecker = [](const std::string &) { return true; };
    int32_t ret = MediaFileAccessUtils::HandleBurstSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName, alwaysChecker);
    EXPECT_EQ(ret, E_ERR);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_copy_file_empty_src_001, TestSize.Level0)
{
    std::string destPath = "/data/local/tmp/file_access_utils_copy_empty_dest.txt";
    MediaFileUtils::DeleteFile(destPath);
    EXPECT_EQ(MediaFileAccessUtils::CopyFile("", destPath), E_INNER_FAIL);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_copy_file_nonexistent_src_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_copy_nonexist_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_copy_nonexist_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
    EXPECT_EQ(MediaFileAccessUtils::CopyFile(srcPath, destPath), E_INNER_FAIL);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(destPath));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_copy_file_empty_dest_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_copy_ed_src.txt";
    std::string destPath;
    MediaFileUtils::DeleteFile(srcPath);
    ASSERT_TRUE(CreateFileWithDataExtra(srcPath, "copy"));
    EXPECT_EQ(MediaFileAccessUtils::CopyFile(srcPath, destPath), E_INNER_FAIL);
    MediaFileUtils::DeleteFile(srcPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_copy_file_with_progress_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_copy_p_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_copy_p_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
    ASSERT_TRUE(CreateFileWithDataExtra(srcPath, "copy_with_progress"));

    auto callback = [](uint64_t) {};
    EXPECT_EQ(MediaFileAccessUtils::CopyFile(srcPath, destPath, callback), E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(destPath));

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_delete_asset_invalid_001, TestSize.Level0)
{
    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    EXPECT_FALSE(MediaFileAccessUtils::DeleteAsset(srcObj));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_move_file_empty_dest_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_mfe_src.txt";
    MediaFileUtils::DeleteFile(srcPath);
    ASSERT_TRUE(CreateFileWithDataExtra(srcPath, "mfe"));
    EXPECT_EQ(MediaFileAccessUtils::MoveFileInEditScene(srcPath, ""), E_ERR);
    MediaFileUtils::DeleteFile(srcPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_utils_move_asset_rename_meta_fail_001, TestSize.Level1)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_rm_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_rm_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
    ASSERT_TRUE(CreateFileWithDataExtra(srcPath, "rm"));
    ASSERT_TRUE(CreateFileWithDataExtra(destPath, "rm"));

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(srcPath, AssetPathType::NORMAL_PATH);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER);
    EXPECT_NE(result.errCode, E_OK);

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTestExtra, file_access_move_asset_rename_meta_fileid_001, TestSize.Level1)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTestExtra::InitAsset(uri, FileSourceType::MEDIA);
    std::string destPath = "/data/local/tmp/file_access_utils_rm_fileid_dest.txt";
    MediaFileUtils::DeleteFile(destPath);
    ASSERT_TRUE(CreateFileWithDataExtra(destPath, "rm"));
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        srcObj.SetAssetRefresh(std::make_shared<AssetAccurateRefresh>());
        std::string realAsset = "/data/local/tmp/file_access_utils_rm_fileid_src.txt";
        MediaFileUtils::DeleteFile(realAsset);
        ASSERT_TRUE(CreateFileWithDataExtra(realAsset, "rm"));
        srcObj.SetAssetPath(realAsset);
        srcObj.SetStoragePath(realAsset);
        MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER);
        EXPECT_NE(result.errCode, E_ERR);
        MediaFileUtils::DeleteFile(realAsset);
    }
    MediaFileUtils::DeleteFile(destPath);
    MediaFileUtils::DeleteFile(destPath + "(1)");
}
} // namespace Media
} // namespace OHOS
