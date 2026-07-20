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

#define MLOG_TAG "MediaLibraryMediaFileAccessUtilsTest"
#include "media_file_access_utils_test.h"

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
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_type_const.h"
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
mutex MediaLibraryMediaFileAccessUtilsTest::Mutex_;
bool MediaLibraryMediaFileAccessUtilsTest::isValid_ = false;
bool MediaLibraryMediaFileAccessUtilsTest::dbIsSupported_ = false;
const std::string TEST_FILE_ROOT = "/data/local/tmp/file_access_utils/";
const std::string TEST_FILE_PATH = TEST_FILE_ROOT + "CreateImageLcdTest_001.jpg";
const std::string FILE_MANAGER_STORAGE_DIR = "/storage/media/local/files/Docs/TestFileManager/";
const std::string LAKE_STORAGE_DIR = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
const std::string MEDIA_PHOTO_DIR_TEST = ROOT_MEDIA_DIR + "Photo/";
const std::string MEDIALIBRARY_ZERO_BUCKET_PATH = MEDIA_PHOTO_DIR_TEST + "0/";
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();
std::vector<std::string> MediaLibraryMediaFileAccessUtilsTest::initAssetFileIds_;

const static vector<string> PHOTO_COLUMN_VECTOR = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_STORAGE_PATH,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
};

void MediaLibraryMediaFileAccessUtilsTest::CleanAssetResource()
{
    std::vector<std::string> cleanIds;
    {
        std::lock_guard<std::mutex> lock(Mutex_);
        cleanIds.swap(initAssetFileIds_);
    }
    CHECK_AND_RETURN_LOG(!cleanIds.empty(), "CleanAssetResource no file ids to clean");

    std::unordered_set<std::string> cleanupPathSet;
    for (const auto &id : cleanIds) {
        auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        CHECK_AND_CONTINUE(fileAsset != nullptr);
        std::string dataPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        if (!dataPath.empty()) {
            cleanupPathSet.emplace(dataPath);
        }
        if (!storagePath.empty()) {
            cleanupPathSet.emplace(storagePath);
        }
    }
    constexpr int32_t MAX_DELETE_ATTEMPTS = 2;
    for (const auto &path : cleanupPathSet) {
        CHECK_AND_CONTINUE(MediaFileUtils::IsFileExists(path));
        bool deleteSuccess = false;
        for (int32_t attempt = 1; attempt <= MAX_DELETE_ATTEMPTS; ++attempt) {
            if (MediaFileUtils::DeleteFile(path) || !MediaFileUtils::IsFileExists(path)) {
                deleteSuccess = true;
                break;
            }
            MEDIA_WARN_LOG("CleanAssetResource delete file failed, path: %{public}s, attempt: %{public}d, "
                "errno: %{public}d", path.c_str(), attempt, errno);
        }
        EXPECT_TRUE(deleteSuccess) << "CleanAssetResource failed to delete file after retries, path: " << path;
        if (!deleteSuccess) {
            MEDIA_ERR_LOG("CleanAssetResource delete file failed after retries, path: %{public}s", path.c_str());
        }
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "CleanAssetResource rdbStore is nullptr");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, cleanIds);
    int32_t changedRows = 0;
    int32_t ret = rdbStore->Delete(changedRows, predicates);
    CHECK_AND_RETURN_LOG(ret == E_OK, "CleanAssetResource delete db failed, err: %{public}d", ret);
    MEDIA_INFO_LOG("CleanAssetResource delete db success, changedRows: %{public}d", changedRows);
}

void MediaLibraryMediaFileAccessUtilsTest::CreateDataHelper(int32_t systemAbilityId)
{
    MEDIA_INFO_LOG("CreateDataHelper start");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    ASSERT_NE(remoteObj, nullptr);

    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");
}

void MediaLibraryMediaFileAccessUtilsTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTest SetUpTestCase");
    MediaLibraryMediaFileAccessUtilsTest::InitMediaLibrary();
    MediaLibraryMediaFileAccessUtilsTest::CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
}

void MediaLibraryMediaFileAccessUtilsTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTest TearDownTestCase");

    MediaLibraryMediaFileAccessUtilsTest::CleanAssetResource();

    bool deleteRet = MediaFileUtils::DeleteDir(TEST_FILE_ROOT);
    bool isRootCleared = deleteRet || !MediaFileUtils::IsDirExists(TEST_FILE_ROOT);
    CHECK_AND_PRINT_LOG(isRootCleared,
        "Delete test file root failed, path: %{public}s, errno: %{public}d", TEST_FILE_ROOT.c_str(), errno);
    ASSERT_TRUE(isRootCleared);
    MEDIA_INFO_LOG("TearDownTestCase end");
}

void MediaLibraryMediaFileAccessUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTest SetUp");
    if (!MediaLibraryMediaFileAccessUtilsTest::IsValid()) {
        MediaLibraryMediaFileAccessUtilsTest::InitMediaLibrary();
    }
}

void MediaLibraryMediaFileAccessUtilsTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryMediaFileAccessUtilsTest TearDown");
}

bool MediaLibraryMediaFileAccessUtilsTest::IsValid()
{
    return isValid_;
}

void MediaLibraryMediaFileAccessUtilsTest::InitMediaLibrary()
{
    std::lock_guard<std::mutex> lock(Mutex_);
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    EXPECT_EQ(ret, E_OK);
    isValid_ = true;
}

int32_t MediaLibraryMediaFileAccessUtilsTest::CreatePhotoApi10(int mediaType, const string &displayName,
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

bool CheckAndCreateDir(const std::string &assetPath)
{
    std::string assetParentPath = MediaFileUtils::GetParentPath(assetPath);
    if (!MediaFileUtils::IsDirExists(assetParentPath)) {
        MediaFileUtils::CreateDirectory(assetParentPath);
    }
    bool isParentDirExist = MediaFileUtils::IsDirExists(assetParentPath);
    return isParentDirExist;
}

bool CreateFileWithData(const std::string &filePath, const std::string &data)
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

bool CreateSparseFile(const std::string &filePath, size_t fileSize)
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

bool ReadFileToString(const std::string &filePath, std::string &content)
{
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        MEDIA_ERR_LOG("Failed to open file for reading: %{public}s", filePath.c_str());
        return false;
    }
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return true;
}


void MediaLibraryMediaFileAccessUtilsTest::InitTestFileAsset(const std::string &path, FileSourceType sourceType)
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
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, FILE_MANAGER_STORAGE_DIR + fileName);
    } else if (sourceType == FileSourceType::MEDIA_HO_LAKE) {
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
        values.PutString(MediaColumn::MEDIA_FILE_PATH, MEDIALIBRARY_ZERO_BUCKET_PATH + fileName);
        values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, LAKE_STORAGE_DIR + fileName);
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

void MediaLibraryMediaFileAccessUtilsTest::InitTestFileAsset(const std::string &path, const std::string &albumOwnerId,
    const std::string &displayName)
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

void MediaLibraryMediaFileAccessUtilsTest::InitAsset(std::string &dataFileUri, FileSourceType sourceType)
{
    std::string testFileName = MediaFileUtils::GetFileName(TEST_FILE_PATH);
    int32_t dataFileId = 0;

    dataFileId = MediaLibraryMediaFileAccessUtilsTest::CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, testFileName,
        false);
    EXPECT_GT(dataFileId, 0);
    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(dataFileId), "", MEDIA_API_VERSION_V10);
    dataFileUri = fileUri.ToString();
    string id = MediaFileUtils::GetIdFromUri(dataFileUri);
    {
        std::lock_guard<std::mutex> lock(Mutex_);
        initAssetFileIds_.push_back(id);
    }
    std::vector<std::string> queryColumns = { PhotoColumn::MEDIA_FILE_PATH };
    std::shared_ptr<FileAsset> fileAssetA = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, queryColumns);
    ASSERT_NE(fileAssetA, nullptr);
    std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        dbIsSupported_ = false;
        return;
    } else {
        dbIsSupported_ = true;
    }
    std::string assetPath = fileAsset->GetPath();
    MediaLibraryMediaFileAccessUtilsTest::InitTestFileAsset(assetPath, sourceType);
    fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
    ASSERT_NE(fileAsset, nullptr);
    if (sourceType == FileSourceType::FILE_MANAGER || sourceType == FileSourceType::MEDIA_HO_LAKE) {
        assetPath = fileAsset->GetStoragePath();
        EXPECT_NE(assetPath, "");
    } else {
        assetPath = fileAsset->GetPath();
        EXPECT_NE(assetPath, "");
    }
    GTEST_LOG_(INFO) << "InitAsset sourceType: " << static_cast<int32_t>(sourceType) << ", assetPath: " << assetPath;
    bool isParentDirExist = CheckAndCreateDir(assetPath);
    EXPECT_TRUE(isParentDirExist);
    
    MediaFileUtils::CopyFileSafe(TEST_FILE_PATH, assetPath);
    bool isfileExist = MediaFileUtils::IsFileExists(assetPath);
    EXPECT_TRUE(isfileExist);
}

bool MediaLibraryMediaFileAccessUtilsTest::CheckDBIsSupported()
{
    return dbIsSupported_;
}

void MediaLibraryMediaFileAccessUtilsTest::CopyToDestPath(int32_t srcFd, const std::string &destPath)
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

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_test_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);
        std::string assetPath = fileAsset->GetPath();
        std::string realPathById = MediaFileAccessUtils::GetAssetRealPathById(id);
        EXPECT_NE(realPathById, "");
        std::string realPathByPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_NE(realPathByPath, "");
        EXPECT_EQ(realPathById, realPathByPath);
        bool isFileManagerPath = PhotoFileUtils::CheckFileManagerRealPath(realPathById);
        EXPECT_TRUE(isFileManagerPath);
        int32_t fd = MediaFileAccessUtils::OpenAssetFile(realPathById, "r");
        EXPECT_GT(fd, 0);
        close(fd);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_check_real_path_001, TestSize.Level0)
{
    std::string docsDir = "/storage/media/local/files/" + DOCS_PATH;
    std::string nonDocsPath = ROOT_MEDIA_DIR + "Photo/1/test.jpg";
    std::string excludedDir = docsDir + ".Trash/test.jpg";
    std::string allowedDir = docsDir + "TestFileManager/test.jpg";

    EXPECT_FALSE(PhotoFileUtils::CheckFileManagerRealPath(nonDocsPath));
    EXPECT_TRUE(PhotoFileUtils::CheckFileManagerRealPath(docsDir));
    EXPECT_FALSE(PhotoFileUtils::CheckFileManagerRealPath(excludedDir));
    EXPECT_TRUE(PhotoFileUtils::CheckFileManagerRealPath(allowedDir));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string assetPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_EQ(realPath, storagePath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_002, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string assetPath = fileAsset->GetPath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_EQ(realPath, assetPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_003, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA_HO_LAKE);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string assetPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_EQ(realPath, storagePath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_by_id_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string assetPath = fileAsset->GetPath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPathById(id);
        EXPECT_EQ(realPath, assetPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_by_id_002, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPathById(id);
        EXPECT_EQ(realPath, storagePath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_by_id_003, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA_HO_LAKE);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPathById(id);
        EXPECT_EQ(realPath, storagePath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_file_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        std::string assetPath = fileAsset->GetPath();
        std::string storagePath = fileAsset->GetStoragePath();
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(assetPath);
        EXPECT_EQ(realPath, storagePath);
        std::string tempPath = TEST_FILE_ROOT + "test_for_move.jpg";
        if (!MediaFileUtils::IsDirExists(TEST_FILE_ROOT)) {
            MediaFileUtils::CreateDirectory(TEST_FILE_ROOT);
        }
        int32_t ret = MediaFileAccessUtils::MoveFileInEditScene(realPath, tempPath);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(MediaFileUtils::IsFileExists(realPath));
        EXPECT_TRUE(MediaFileUtils::IsFileExists(tempPath));
        ret = MediaFileAccessUtils::MoveFileInEditScene(tempPath, realPath);
        EXPECT_EQ(ret, E_OK);
        EXPECT_TRUE(MediaFileUtils::IsFileExists(realPath));
        EXPECT_FALSE(MediaFileUtils::IsFileExists(tempPath));
        MediaFileUtils::DeleteFile(tempPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_file_002, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_move_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_move_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    ASSERT_TRUE(CreateFileWithData(srcPath, "move_test"));
    int32_t ret = MediaFileAccessUtils::MoveFileInEditScene(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(srcPath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(destPath));

    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_large_file_copy_001, TestSize.Level0)
{
    constexpr size_t TEST_SEGMENT_BOUNDARY = 0x7FFFF000;
    constexpr size_t TEST_LARGE_FILE_SIZE = TEST_SEGMENT_BOUNDARY + 4096;
    std::string srcPath = "/data/local/tmp/file_access_utils_large_src.bin";
    std::string destPath = "/data/local/tmp/file_access_utils_large_dest.bin";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    ASSERT_TRUE(CreateSparseFile(srcPath, TEST_LARGE_FILE_SIZE));
    EXPECT_TRUE(MediaFileUtils::CopyFileUtil(srcPath, destPath));

    size_t srcSize = 0;
    size_t destSize = 0;
    EXPECT_TRUE(MediaFileUtils::GetFileSize(srcPath, srcSize));
    EXPECT_TRUE(MediaFileUtils::GetFileSize(destPath, destSize));
    EXPECT_EQ(srcSize, TEST_LARGE_FILE_SIZE);
    EXPECT_EQ(destSize, srcSize);

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_copy_empty_file_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_empty_src.bin";
    std::string destPath = "/data/local/tmp/file_access_utils_empty_dest.bin";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    ASSERT_TRUE(CreateFileWithData(srcPath, ""));
    EXPECT_TRUE(MediaFileUtils::CopyFileUtil(srcPath, destPath));

    size_t srcSize = 0;
    size_t destSize = 0;
    EXPECT_TRUE(MediaFileUtils::GetFileSize(srcPath, srcSize));
    EXPECT_TRUE(MediaFileUtils::GetFileSize(destPath, destSize));
    EXPECT_EQ(srcSize, 0);
    EXPECT_EQ(destSize, 0);

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_copy_nonexistent_src_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_nonexistent_src.bin";
    std::string destPath = "/data/local/tmp/file_access_utils_nonexistent_dest.bin";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    EXPECT_FALSE(MediaFileUtils::CopyFileUtil(srcPath, destPath));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(destPath));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_copy_dest_is_directory_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_dir_src.bin";
    std::string destDir = "/data/local/tmp/file_access_utils_dest_dir";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteDir(destDir);

    ASSERT_TRUE(CreateFileWithData(srcPath, "abc"));
    ASSERT_TRUE(MediaFileUtils::CreateDirectory(destDir));

    EXPECT_FALSE(MediaFileUtils::CopyFileUtil(srcPath, destDir));

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteDir(destDir);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_copy_overwrite_truncate_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_overwrite_src.bin";
    std::string destPath = "/data/local/tmp/file_access_utils_overwrite_dest.bin";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);

    const std::string srcData = "new";
    const std::string oldDestData = "old_content_should_be_truncated";
    ASSERT_TRUE(CreateFileWithData(srcPath, srcData));
    ASSERT_TRUE(CreateFileWithData(destPath, oldDestData));

    EXPECT_TRUE(MediaFileUtils::CopyFileUtil(srcPath, destPath));

    std::string destData;
    ASSERT_TRUE(ReadFileToString(destPath, destData));
    EXPECT_EQ(destData, srcData);

    size_t destSize = 0;
    EXPECT_TRUE(MediaFileUtils::GetFileSize(destPath, destSize));
    EXPECT_EQ(destSize, srcData.size());

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_manager_is_directory_empty_001, TestSize.Level0)
{
    std::string testRoot = "/data/local/tmp/empty_dir_test_unique/";
    std::string emptyDir = testRoot + "empty_dir/";
    std::string dirWithFile = testRoot + "dir_with_file/";
    std::string dirWithSubdir = testRoot + "dir_with_subdir/";
    std::string nonExistentDir = testRoot + "non_existent_dir/";
    std::string testFile = dirWithFile + "test_file.txt";
    std::string subdir = dirWithSubdir + "subdir/";
    
    MediaFileUtils::DeleteDir(testRoot);
    
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(emptyDir));
    EXPECT_TRUE(MediaFileAccessUtils::IsDirectoryEmpty(emptyDir));
    
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(dirWithFile));
    EXPECT_TRUE(CreateFileWithData(testFile, "test_data"));
    EXPECT_FALSE(MediaFileAccessUtils::IsDirectoryEmpty(dirWithFile));
    
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(dirWithSubdir));
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(subdir));
    EXPECT_FALSE(MediaFileAccessUtils::IsDirectoryEmpty(dirWithSubdir));
    
    EXPECT_TRUE(MediaFileAccessUtils::IsDirectoryEmpty(nonExistentDir));

    if (MediaFileUtils::IsDirectory(testRoot)) {
        bool deleteResult = MediaFileUtils::DeleteDir(testRoot);
        if (!deleteResult) {
            MEDIA_ERR_LOG("Failed to delete test directory: %{public}s, errno: %{public}d",
                        testRoot.c_str(), errno);
        } else {
            MEDIA_INFO_LOG("Successfully deleted test directory: %{public}s", testRoot.c_str());
        }
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_manager_update_modify_time_001, TestSize.Level0)
{
    std::string testRoot = "/storage/media/local/files/Docs/Download/";
    std::string testFile = testRoot + "test_file.txt";
    
    MediaFileUtils::DeleteDir(testRoot);
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(testRoot));
    EXPECT_TRUE(CreateFileWithData(testFile, "test_data"));

    int64_t origMtime = 0;
    EXPECT_TRUE(MediaFileUtils::GetDateModified(testFile, origMtime));
    int64_t newMtime = 1739386800000;
    MediaFileAccessUtils::UpdateModifyTime(testFile, newMtime);
    int64_t updatedMtime = 0;
    EXPECT_TRUE(MediaFileUtils::GetDateModified(testFile, updatedMtime));
    EXPECT_NE(updatedMtime, origMtime);
    EXPECT_EQ(updatedMtime, newMtime);
    
    if (MediaFileUtils::IsDirectory(testRoot)) {
        bool deleteResult = MediaFileUtils::DeleteDir(testRoot);
        if (!deleteResult) {
            MEDIA_ERR_LOG("Failed to delete test directory: %{public}s, errno: %{public}d",
                        testRoot.c_str(), errno);
        } else {
            MEDIA_INFO_LOG("Successfully deleted test directory: %{public}s", testRoot.c_str());
        }
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_from_obj_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(srcObj);
        EXPECT_EQ(realPath, fileAsset->GetStoragePath());
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_from_obj_002, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);
        std::string testPath = fileAsset->GetPath();

        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(testPath, AssetPathType::ASSET_PATH);
        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(srcObj);
        EXPECT_EQ(realPath, fileAsset->GetStoragePath());
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_real_path_from_obj_003, TestSize.Level0)
{
    std::string testPath = "/data/local/tmp/file_access_utils_obj_path.txt";
    MediaFileUtils::DeleteFile(testPath);
    ASSERT_TRUE(CreateFileWithData(testPath, "obj_path"));

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(testPath, AssetPathType::NORMAL_PATH);
    std::string realPath = MediaFileAccessUtils::GetAssetRealPath(srcObj);
    EXPECT_EQ(realPath, testPath);

    MediaFileUtils::DeleteFile(testPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_asset_002, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    std::string destPath = "/data/local/tmp/file_access_utils_move_asset_dest.txt";
    MediaFileUtils::DeleteFile(destPath);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        std::string srcPath = srcObj.GetAssetPath();
        srcObj.SetBurstCoverLevel(BurstCoverLevelType::COVER);
        srcObj.SetSubType(PhotoSubType::BURST);
        MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER, true);
        EXPECT_EQ(result.errCode, E_OK);
        EXPECT_TRUE(MediaFileUtils::IsFileExists(destPath));
        EXPECT_FALSE(MediaFileUtils::IsFileExists(srcPath));
        srcObj.SetBurstCoverLevel(BurstCoverLevelType::MEMBER);
        srcObj.SetAssetPath(destPath);
        result = MediaFileAccessUtils::MoveAsset(srcObj, srcPath, FileSourceType::FILE_MANAGER, true);
        EXPECT_EQ(result.errCode, E_ERR);

        MediaFileUtils::DeleteFile(destPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_asset_003, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    std::string destPath = "/data/local/tmp/file_access_utils_move_asset_dest.txt";
    MediaFileUtils::DeleteFile(destPath);
    auto ResetPath = [](AssetOperationInfo &srcObj, std::string &destPath, int32_t moveResultErrCode) {
        CHECK_AND_RETURN(moveResultErrCode == E_OK);
        std::string srcPath = srcObj.GetAssetPath();
        srcObj.SetAssetPath(destPath);
        destPath = srcPath;
    };
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        srcObj.SetFileSourceType(FileSourceType::MEDIA_HO_LAKE);
        srcObj.SetStoragePath(srcObj.GetAssetPath());
        result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA_HO_LAKE, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        srcObj.SetFileSourceType(FileSourceType::FILE_MANAGER);
        srcObj.SetStoragePath(srcObj.GetAssetPath());
        result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        MediaFileUtils::DeleteFile(destPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_asset_004, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    std::string destPath = "/data/local/tmp/file_access_utils_move_asset_dest.txt";
    MediaFileUtils::DeleteFile(destPath);
    auto ResetPath = [](AssetOperationInfo &srcObj, std::string &destPath, int32_t moveResultErrCode) {
        CHECK_AND_RETURN(moveResultErrCode == E_OK);
        std::string srcPath = srcObj.GetAssetPath();
        srcObj.SetAssetPath(destPath);
        destPath = srcPath;
    };
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);

        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        srcObj.SetFileSourceType(FileSourceType::MEDIA);
        MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        srcObj.SetFileSourceType(FileSourceType::MEDIA);
        result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA_HO_LAKE, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        srcObj.SetFileSourceType(FileSourceType::MEDIA_HO_LAKE);
        srcObj.SetStoragePath(srcObj.GetAssetPath());
        result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        srcObj.SetFileSourceType(FileSourceType::FILE_MANAGER);
        srcObj.SetStoragePath(srcObj.GetAssetPath());
        result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::MEDIA, true);
        EXPECT_EQ(result.errCode, E_OK);
        ResetPath(srcObj, destPath, result.errCode);

        MediaFileUtils::DeleteFile(destPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_asset_005, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    std::string uriConflict = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uriConflict, FileSourceType::MEDIA);
    std::string destPath = "/data/local/tmp/file_access_utils_move_asset_dest.txt";
    std::string finalPath = "/data/local/tmp/file_access_utils_move_asset_dest(1).txt";
    MediaFileUtils::DeleteFile(destPath);
    MediaFileUtils::DeleteFile(finalPath);
    std::string ownerAlbumId = "999";
    std::string displayName = "file_access_utils_move_asset_dest.txt";
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
            id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);
        InitTestFileAsset(fileAsset->GetPath(), ownerAlbumId, displayName);
        AssetOperationInfo srcObj = AssetOperationInfo::CreateFromFileId(id);
        EXPECT_EQ(srcObj.GetOwnerAlbumId(), ownerAlbumId);

        id = MediaFileUtils::GetIdFromUri(uriConflict);
        fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID, id,
            OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
        ASSERT_NE(fileAsset, nullptr);
        InitTestFileAsset(fileAsset->GetPath(), ownerAlbumId, displayName);

        MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER, true);
        EXPECT_EQ(result.errCode, E_OK);
        EXPECT_FALSE(result.isExecuteRename);
        EXPECT_EQ(result.newPath, destPath);

        MediaFileUtils::DeleteFile(destPath);
        MediaFileUtils::DeleteFile(finalPath);
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_move_asset_create_parent_dir_fail_001,
    TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_move_asset_src_for_mkdir_fail.txt";
    std::string blockerFile = "/data/local/tmp/file_access_utils_mkdir_blocker";
    std::string destPath = blockerFile + "/child/file_access_utils_move_asset_dest.txt";

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteDir(blockerFile);
    MediaFileUtils::DeleteFile(blockerFile);

    ASSERT_TRUE(CreateFileWithData(srcPath, "move_asset"));
    ASSERT_TRUE(CreateFileWithData(blockerFile, "blocker"));

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(srcPath, AssetPathType::NORMAL_PATH);
    MoveResult result = MediaFileAccessUtils::MoveAsset(srcObj, destPath, FileSourceType::FILE_MANAGER, true);
    EXPECT_EQ(result.errCode, E_ERR);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(srcPath));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(destPath));

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(blockerFile);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_copy_file_001, TestSize.Level0)
{
    std::string srcPath = "/data/local/tmp/file_access_utils_copy_src.txt";
    std::string destPath = "/data/local/tmp/file_access_utils_copy_dest.txt";
    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
    ASSERT_TRUE(CreateFileWithData(srcPath, "copy_data"));

    int32_t ret = MediaFileAccessUtils::CopyFile(srcPath, destPath);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(destPath));

    MediaFileUtils::DeleteFile(srcPath);
    MediaFileUtils::DeleteFile(destPath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_delete_asset_001, TestSize.Level0)
{
    std::string testPath = "/data/local/tmp/file_access_utils_delete.txt";
    MediaFileUtils::DeleteFile(testPath);
    ASSERT_TRUE(CreateFileWithData(testPath, "delete_data"));

    AssetOperationInfo srcObj = AssetOperationInfo::CreateFromPath(testPath, AssetPathType::NORMAL_PATH);
    bool ret = MediaFileAccessUtils::DeleteAsset(srcObj);
    EXPECT_TRUE(ret);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(testPath));
}

void MediaLibraryMediaFileAccessUtilsTest::RunSameNameRenameCase(const std::string &sameNamePath,
    const std::vector<std::string> &existingPaths, const std::string &expectedPath)
{
    std::vector<std::string> cleanupPaths = existingPaths;
    cleanupPaths.push_back(sameNamePath);
    cleanupPaths.push_back(expectedPath);
    for (const auto &path : cleanupPaths) {
        MediaFileUtils::DeleteFile(path);
    }

    ASSERT_TRUE(CreateFileWithData(sameNamePath, "same_name"));
    for (const auto &path : existingPaths) {
        ASSERT_TRUE(CreateFileWithData(path, "same_name"));
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

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_001, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_name.jpg",
        { "/data/local/tmp/file_access_utils_same_name(1).jpg" },
        "/data/local/tmp/file_access_utils_same_name(2).jpg");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_002, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_name",
        { "/data/local/tmp/file_access_utils_same_name(1)" },
        "/data/local/tmp/file_access_utils_same_name(2)");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_003, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_(1)name",
        {},
        "/data/local/tmp/file_access_utils_same_(1)name(1)");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_004, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_name(1).jpg",
        {},
        "/data/local/tmp/file_access_utils_same_name(1)(1).jpg");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_005, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils.same.name.jpg",
        {},
        "/data/local/tmp/file_access_utils.same.name(1).jpg");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_006, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_name(test).jpg",
        {},
        "/data/local/tmp/file_access_utils_same_name(test)(1).jpg");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_007, TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_same_name.jpg",
        {
            "/data/local/tmp/file_access_utils_same_name(1).jpg",
            "/data/local/tmp/file_access_utils_same_name(2).jpg",
        },
        "/data/local/tmp/file_access_utils_same_name(3).jpg");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_empty_title_001,
    TestSize.Level0)
{
    std::string sameNamePath = "/data/local/tmp/.jpg";
    MediaFileUtils::DeleteFile(sameNamePath);
    ASSERT_TRUE(CreateFileWithData(sameNamePath, "same_name"));

    std::string renamePath;
    std::string renameTitle;
    std::string renameDisplayName;
    int32_t ret = MediaFileAccessUtils::HandleSameNameRename(sameNamePath, renamePath, renameTitle,
        renameDisplayName);
    EXPECT_EQ(ret, E_ERR);

    MediaFileUtils::DeleteFile(sameNamePath);
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_handle_same_name_rename_empty_extension_001,
    TestSize.Level0)
{
    RunSameNameRenameCase(
        "/data/local/tmp/file_access_utils_no_ext.",
        {},
        "/data/local/tmp/file_access_utils_no_ext(1).");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_file_asset_from_db_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::MEDIA);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        std::shared_ptr<FileAsset> fileAsset = MediaFileAccessUtils::GetFileAssetFromDb(MediaColumn::MEDIA_ID, id);
        ASSERT_NE(fileAsset, nullptr);
        EXPECT_NE(fileAsset->GetFilePath(), "");
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_check_file_manager_lpath_001, TestSize.Level0)
{
    std::string validLPath = "/FromDocs/TestFileManager/test.jpg";
    std::string invalidPrefix = "FromDocs/TestFileManager/test.jpg";
    std::string excludedLPath = "/FromDocs/.Trash/test.jpg";
    std::string validLPath2 = "/FromDocs/";

    EXPECT_TRUE(PhotoFileUtils::CheckFileManagerLPath(validLPath));
    EXPECT_FALSE(PhotoFileUtils::CheckFileManagerLPath(invalidPrefix));
    EXPECT_FALSE(PhotoFileUtils::CheckFileManagerLPath(excludedLPath));
    EXPECT_TRUE(PhotoFileUtils::CheckFileManagerLPath(validLPath2));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_file_manager_lpath_from_real_path_001,
    TestSize.Level0)
{
    std::string realPath = "/storage/media/local/files/Docs/Download/com.xxx.example/test.jpg";
    std::string realPath2 = "/storage/media/local/files/Docs/";
    std::string realPath3 = "/storage/media/local/files/Docs/test.jpg";
    std::string invalidPath = "/storage/media/local/files/Video/test.jpg";

    std::string lPath = PhotoFileUtils::GetFileManagerLPathFromRealPath(realPath);
    EXPECT_EQ(lPath, "/FromDocs/Download/com.xxx.example");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerLPathFromRealPath(invalidPath), "");
    lPath = PhotoFileUtils::GetFileManagerLPathFromRealPath(realPath2);
    EXPECT_EQ(lPath, "/FromDocs/");
    lPath = PhotoFileUtils::GetFileManagerLPathFromRealPath(realPath3);
    EXPECT_EQ(lPath, "/FromDocs/");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_file_manager_lpath_from_real_path_002,
    TestSize.Level0)
{
    std::string realDirPath = "/storage/media/local/files/Docs/Download/com.xxx.example";
    std::string realDirPathWithSlash = realDirPath + "/";

    EXPECT_TRUE(MediaFileUtils::CreateDirectory(realDirPath) == E_OK || MediaFileUtils::IsDirExists(realDirPath));

    EXPECT_EQ(PhotoFileUtils::GetFileManagerLPathFromRealPath(realDirPath),
        "/FromDocs/Download/com.xxx.example");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerLPathFromRealPath(realDirPathWithSlash),
        "/FromDocs/Download/com.xxx.example");

    EXPECT_TRUE(MediaFileUtils::DeleteDir(realDirPath));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_file_manager_lpath_from_real_path_003,
    TestSize.Level0)
{
    std::string topLevelDirPath = "/storage/media/local/files/Docs/GetLPathTopLevelCase";
    std::string topLevelDirPathWithSlash = topLevelDirPath + "/";

    EXPECT_TRUE(MediaFileUtils::CreateDirectory(topLevelDirPath) == E_OK ||
        MediaFileUtils::IsDirExists(topLevelDirPath));

    EXPECT_EQ(PhotoFileUtils::GetFileManagerLPathFromRealPath(topLevelDirPath),
        "/FromDocs/GetLPathTopLevelCase");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerLPathFromRealPath(topLevelDirPathWithSlash),
        "/FromDocs/GetLPathTopLevelCase");

    EXPECT_TRUE(MediaFileUtils::DeleteDir(topLevelDirPath));
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_get_file_manager_dir_from_lpath_001, TestSize.Level1)
{
    EXPECT_EQ(PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs/Download/com.example.app"),
        "/storage/media/local/files/Docs/Download/com.example.app/");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs"), "");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs/"), "/storage/media/local/files/Docs/");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs/.Trash"), "");
    EXPECT_EQ(PhotoFileUtils::GetFileManagerDirFromLPath("FromDocs/Download/com.example.app"), "");
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, asset_operation_info_getters_setters_001, TestSize.Level0)
{
    AssetOperationInfo info = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    EXPECT_FALSE(info.IsInfoAvailable());
    EXPECT_FALSE(info.IsValid());

    info.SetFileId("100");
    info.SetAssetPath("/data/local/tmp/asset_path.jpg");
    info.SetStoragePath("/data/local/tmp/storage_path.jpg");
    info.SetFileSourceType(FileSourceType::MEDIA);
    info.SetSubType(PhotoSubType::MOVING_PHOTO);
    info.SetOwnerAlbumId("200");
    info.SetBurstCoverLevel(BurstCoverLevelType::COVER);
    info.SetBurstKey("burst_key");

    EXPECT_EQ(info.GetFileId(), "100");
    EXPECT_EQ(info.GetAssetPath(), "/data/local/tmp/asset_path.jpg");
    EXPECT_EQ(info.GetStoragePath(), "/data/local/tmp/storage_path.jpg");
    EXPECT_EQ(info.GetFileSourceType(), FileSourceType::MEDIA);
    EXPECT_EQ(info.GetSubType(), PhotoSubType::MOVING_PHOTO);
    EXPECT_EQ(info.GetOwnerAlbumId(), "200");
    EXPECT_EQ(info.GetBurstCoverLevel(), BurstCoverLevelType::COVER);
    EXPECT_EQ(info.GetBurstKey(), "burst_key");
    EXPECT_TRUE(info.IsValid());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, asset_operation_info_init_001, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);

        AssetOperationInfo info = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
        info.SetFileId(id);
        EXPECT_FALSE(info.Init());
        info.Reset();
        EXPECT_TRUE(info.Init());
        EXPECT_TRUE(info.IsInfoAvailable());
        EXPECT_TRUE(info.IsValid());
        EXPECT_EQ(info.GetFileId(), id);
        EXPECT_NE(info.GetAssetPath(), "");
        EXPECT_NE(info.GetStoragePath(), "");
    }
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, asset_operation_info_is_valid_001, TestSize.Level0)
{
    AssetOperationInfo emptyInfo = AssetOperationInfo::CreateFromPath("", AssetPathType::NORMAL_PATH);
    EXPECT_FALSE(emptyInfo.IsValid());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, asset_operation_info_is_info_available_001, TestSize.Level0)
{
    AssetOperationInfo normalInfo = AssetOperationInfo::CreateFromPath("/data/local/tmp/any.txt",
        AssetPathType::NORMAL_PATH);
    EXPECT_FALSE(normalInfo.IsInfoAvailable());
}

HWTEST_F(MediaLibraryMediaFileAccessUtilsTest, file_access_utils_burst_member_real_path, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryMediaFileAccessUtilsTest::InitAsset(uri, FileSourceType::FILE_MANAGER);
    if (CheckDBIsSupported()) {
        EXPECT_NE(uri, "");
        std::string id = MediaFileUtils::GetIdFromUri(uri);
        AssetOperationInfo info = AssetOperationInfo::CreateFromFileId(id);

        std::string assetPath = info.GetAssetPath();
        std::string storagePath = info.GetStoragePath();
        EXPECT_TRUE(MediaFileUtils::IsFileExists(storagePath));
        EXPECT_FALSE(MediaFileUtils::IsFileExists(assetPath));
        info.SetAssetPath(storagePath);
        info.SetStoragePath(assetPath);
        info.SetSubType(PhotoSubType::BURST);
        info.SetBurstCoverLevel(BurstCoverLevelType::MEMBER);

        std::string realPath = MediaFileAccessUtils::GetAssetRealPath(info);
        EXPECT_EQ(realPath, info.GetAssetPath());

        info.SetBurstCoverLevel(BurstCoverLevelType::COVER);
        std::string realPathCover = MediaFileAccessUtils::GetAssetRealPath(info);
        EXPECT_EQ(realPathCover, info.GetStoragePath());

        info.SetSubType(PhotoSubType::DEFAULT);
        std::string realPathDefault = MediaFileAccessUtils::GetAssetRealPath(info);
        EXPECT_EQ(realPathDefault, info.GetStoragePath());
    }
}
} // namespace Media
} // namespace OHOS