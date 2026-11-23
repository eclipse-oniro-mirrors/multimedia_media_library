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

#define MLOG_TAG "MediaLibraryShareOpenUnitTest"
#include "media_library_share_open_test.h"

#include <fcntl.h>
#include <fstream>
#include <unistd.h>

#define private public
#ifdef MEDIALIBRARY_FEATURE_CUSTOM_RESTORE
#include "photo_custom_restore_operation.h"
#endif
#define protected public
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_photo_operations.h"
#undef protected
#undef private

#include "ability_context_impl.h"
#include "base_data_uri.h"
#ifdef MEDIALIBRARY_FEATURE_CUSTOM_RESTORE
#include "custom_restore_const.h"
#endif
#include "directory_ex.h"
#include "file_asset.h"
#include "get_self_permissions.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_operation.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_library_extend_manager.h"
#include "photo_album_column.h"

#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "media_library_manager.h"
#include "medialibrary_object_utils.h"
#include "moving_photo_file_utils.h"
#include "rdb_store.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "unique_fd.h"
#include "tlv_util.h"
#include "media_edit_utils.h"
#include "media_library_handler.h"

#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
#include "media_file_access_utils.h"
#endif

using namespace testing;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
mutex MediaLibraryShareOpenTest::Mutex_;
static uint64_t g_shellToken = 0;
static std::shared_ptr<MediaLibraryMockHapToken> mockToken = nullptr;
const std::string TEST_FILE_ROOT = "/data/local/tmp/assets_share_resources/";
const std::string EFFECT_FILE_PATH = TEST_FILE_ROOT + "assets_share/CreateImageLcdTest_001.jpg";
const std::string EDIT_DATA_PATH = TEST_FILE_ROOT + "assets_share/editdata";
const std::string EDIT_DATA_CAMERA_PATH = TEST_FILE_ROOT + "assets_share/editdata_camera";
const std::string EDIT_DATA_SOURCE_BACK_PATH = TEST_FILE_ROOT + "assets_share/HasHdrNoRotate.jpg";
const std::string EDIT_DATA_SOURCE_PATH = TEST_FILE_ROOT + "assets_share/HasHdrHasRotate.jpg";
const std::string TLV_FILE_CACHE = TEST_FILE_ROOT + "assets_share/tlv_cache";
const std::string TLV_FILE_CACHE_FILE = TLV_FILE_CACHE + "/tlv_file_cache.tlv";
const std::string OPEN_ORIGINAL_FILE = TLV_FILE_CACHE + "/open_origin.jpg";
const std::string OPEN_COMPRESS_AFTER_DELETE_FILE = TLV_FILE_CACHE + "/open_compress_after_delete.tlv";
const std::string OPEN_LIVE_PHOTO_AFTER_DELETE_FILE = TLV_FILE_CACHE + "/open_live_photo_after_delete.jpg";
const std::string VIDEO_TEST_FILE_PATH = TEST_FILE_ROOT + "assets_share/CreateVideoThumbnailTest_001.mp4";

const std::string FILE_MANAGER_SHARE_DIR = "/storage/media/local/files/Docs/Download/MediaShareOpen/";
const std::string ROOT_TEST_MEDIA_DIR =
    "/data/app/el2/100/base/com.ohos.medialibrary.medialibrarydata/haps/";
const std::string TEST_DISPLAY_NAME = "test_image.png";
constexpr int32_t RETRY_MAX = 5;
constexpr int32_t RETRY_TIME = 5;

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

const static vector<string> PHOTO_COLUMN_VECTOR = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::MEDIA_TYPE,
    PhotoColumn::MEDIA_TIME_PENDING,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_POSITION,
    MediaColumn::MEDIA_HIDDEN,
    MediaColumn::MEDIA_DATE_TRASHED,
    MediaColumn::MEDIA_SIZE,
    MediaColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
};

void MediaLibraryShareOpenTest::CreateDataHelper(int32_t systemAbilityId)
{
    MEDIA_INFO_LOG("CreateDataHelper start");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    ASSERT_NE(remoteObj, nullptr);

    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");
}

void MediaLibraryShareOpenTest::ProcessPermission()
{
    MEDIA_INFO_LOG("MediaLibraryShareOpenTest ProcessPermission");
    vector<string> perms;

    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    mockToken = make_shared<MediaLibraryMockHapToken>("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
    uint64_t tokenId = -1;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryShareOpenTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
}

int32_t CreateFileWithData(const std::string &filePath, const std::string &data)
{
    std::string parentPath = MediaFileUtils::GetParentPath(filePath);
    if (!MediaFileUtils::IsDirExists(parentPath)) {
        std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
    }
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.is_open()) {
        MEDIA_ERR_LOG("Failed to open file: %{public}s", filePath.c_str());
        return E_ERR;
    }
    ofs << data;
    ofs.close();
    return E_OK;
}

void MediaLibraryShareOpenTest::InitEditedFiles()
{
    std::string data1 = "This is edited data.";
    std::string data2 = "This is edited data camera.";
    int32_t ret = CreateFileWithData(EDIT_DATA_PATH, data1);
    EXPECT_EQ(ret, E_OK);
    ret = CreateFileWithData(EDIT_DATA_CAMERA_PATH, data2);
    EXPECT_EQ(ret, E_OK);
}

void MediaLibraryShareOpenTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryShareOpenTest SetUpTestCase");
    MediaLibraryShareOpenTest::InitMediaLibrary();
    MediaLibraryShareOpenTest::CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    MediaLibraryShareOpenTest::ProcessPermission();
    InitEditedFiles();
    CheckTestFileExistence();
}

void MediaLibraryShareOpenTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryShareOpenTest TearDownTestCase");

    if (mockToken != nullptr) {
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    MEDIA_INFO_LOG("TearDownTestCase end");
}

void MediaLibraryShareOpenTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryShareOpenTest SetUp");
}

void MediaLibraryShareOpenTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryShareOpenTest TearDown");
}

bool MediaLibraryShareOpenTest::IsValid()
{
    return isValid_;
}

void MediaLibraryShareOpenTest::InitMediaLibrary()
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

int32_t MediaLibraryShareOpenTest::CreatePhotoApi10(int mediaType, const string &displayName, bool isPhotoEdited,
    bool isMovingPhoto)
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

void MediaLibraryShareOpenTest::CheckTestFileExistence()
{
    bool isFileExist = MediaFileUtils::IsFileExists(EFFECT_FILE_PATH);
    EXPECT_TRUE(isFileExist);
    isFileExist = MediaFileUtils::IsFileExists(EDIT_DATA_PATH);
    EXPECT_TRUE(isFileExist);
    isFileExist = MediaFileUtils::IsFileExists(EDIT_DATA_CAMERA_PATH);
    EXPECT_TRUE(isFileExist);
    isFileExist = MediaFileUtils::IsFileExists(EDIT_DATA_SOURCE_BACK_PATH);
    EXPECT_TRUE(isFileExist);
    isFileExist = MediaFileUtils::IsFileExists(EDIT_DATA_SOURCE_PATH);
    EXPECT_TRUE(isFileExist);
    isFileExist = MediaFileUtils::IsFileExists(VIDEO_TEST_FILE_PATH);
    EXPECT_TRUE(isFileExist);
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

static auto TryGetRdbStoreWithInit()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore != nullptr) {
        return rdbStore;
    }

    MEDIA_WARN_LOG("rdbStore is nullptr, try InitMediaLibraryMgr once");
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    int32_t initRet = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    MEDIA_INFO_LOG("InitMediaLibraryMgr for rdb retry ret: %{public}d", initRet);
    return MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

static bool QueryAssetPathByFileId(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::string &fileId, std::string &assetPath)
{
    std::string sql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + " AS path FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_ID + " = ? LIMIT 1;";
    std::vector<std::string> whereArgs = { fileId };
    auto resultSet = rdbStore->QuerySql(sql, whereArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query photo path failed, fileId=%{public}s",
        fileId.c_str());
    if (resultSet->GoToFirstRow() != E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("resultSet GoToFirstRow failed, fileId=%{public}s", fileId.c_str());
        return false;
    }
    if (resultSet->GetString(0, assetPath) != E_OK || assetPath.empty()) {
        resultSet->Close();
        MEDIA_ERR_LOG("query result invalid, fileId=%{public}s", fileId.c_str());
        return false;
    }
    resultSet->Close();
    return true;
}

static bool EnsureAssetFileExists(const std::string &assetPath)
{
    if (MediaFileUtils::IsFileExists(assetPath)) {
        return true;
    }
    std::string assetParentDir = MediaFileUtils::GetParentPath(assetPath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsDirExists(assetParentDir) ||
        MediaFileUtils::CreateDirectory(assetParentDir), false,
        "create asset parent dir failed: %{public}s", assetParentDir.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(EFFECT_FILE_PATH), false,
        "effect source file not exist: %{public}s", EFFECT_FILE_PATH.c_str());
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(EFFECT_FILE_PATH, assetPath), false,
        "copy effect file to assetPath failed, src=%{public}s, dst=%{public}s",
        EFFECT_FILE_PATH.c_str(), assetPath.c_str());
    return true;
}

static bool BuildFileManagerStoragePath(const std::string &assetPath, std::string &storagePath)
{
    std::string fileName = MediaFileUtils::GetFileName(assetPath);
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), false, "failed to get fileName from assetPath=%{public}s",
        assetPath.c_str());
    storagePath = FILE_MANAGER_SHARE_DIR;
    if (storagePath.back() != '/') {
        storagePath += "/";
    }
    storagePath += fileName;
    return true;
}

static bool MarkAsFileManagerSource(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, const std::string &fileId,
    const std::string &storagePath)
{
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::FILE_MANAGER));
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = 0;
    int32_t updateRet = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(updateRet == E_OK && changedRows > 0, false,
        "update db failed, ret=%{public}d, changedRows=%{public}d, fileId=%{public}s",
        updateRet, changedRows, fileId.c_str());
    return true;
}

static bool ConvertAssetToFileManager(const std::string &assetUri)
{
    MEDIA_INFO_LOG("ConvertAssetToFileManager start");
    CHECK_AND_RETURN_RET_LOG(!assetUri.empty() && !FILE_MANAGER_SHARE_DIR.empty(), false,
        "invalid input, assetUri empty: %{public}d, shareDir empty: %{public}d",
        assetUri.empty(), FILE_MANAGER_SHARE_DIR.empty());
    std::string fileId = MediaFileUtils::GetIdFromUri(assetUri);
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), false, "failed to parse fileId from assetUri: %{public}s",
        assetUri.c_str());
    auto rdbStore = TryGetRdbStoreWithInit();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    std::string assetPath;
    CHECK_AND_RETURN_RET_LOG(QueryAssetPathByFileId(rdbStore, fileId, assetPath), false,
        "query asset path by fileId failed");
    CHECK_AND_RETURN_RET_LOG(EnsureAssetFileExists(assetPath), false, "ensure asset file exists failed");
    std::string storagePath;
    CHECK_AND_RETURN_RET_LOG(BuildFileManagerStoragePath(assetPath, storagePath), false,
        "build storage path failed");
    std::string parentDir = MediaFileUtils::GetParentPath(storagePath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsDirExists(parentDir) || MediaFileUtils::CreateDirectory(parentDir),
        false, "create parentDir failed: %{public}s", parentDir.c_str());
    CHECK_AND_RETURN_RET_LOG(PhotoFileUtils::CheckFileManagerRealPath(storagePath), false,
        "invalid file manager real path: %{public}s", storagePath.c_str());
    if (MediaFileUtils::IsFileExists(storagePath)) {
        MediaFileUtils::DeleteFile(storagePath);
    }
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileAndDelSrc(assetPath, storagePath), false,
        "copy and delete source failed, src=%{public}s, dst=%{public}s", assetPath.c_str(), storagePath.c_str());
    return MarkAsFileManagerSource(rdbStore, fileId, storagePath);
}

int32_t CreatePhotoApi10FileMgr(string &uri, int mediaType, const string &displayName, bool isPhotoEdited)
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
    MEDIA_INFO_LOG("CreatePhotoApi10 ret = %{public}d", ret);
    uri = cmd.GetResult();
    return ret;
}

void InitAsset(std::string &dataFileUri, const string &displayName, MediaType type = MediaType::MEDIA_TYPE_IMAGE)
{
    MEDIA_INFO_LOG("InitAsset start");
    int32_t dataFileId = 0;
    dataFileId = CreatePhotoApi10FileMgr(dataFileUri, type, displayName, false);
    MEDIA_INFO_LOG("InitAsset dataFileId: %{public}d", dataFileId);
    MEDIA_INFO_LOG("InitAsset uri: %{public}s", dataFileUri.c_str());
    string id = MediaFileUtils::GetIdFromUri(dataFileUri);
    MediaLibraryShareOpenTest::InitTestFileAsset(id, false);
    std::vector<std::string> queryColumns = PHOTO_COLUMN_VECTOR;
    queryColumns.push_back(PhotoColumn::PHOTO_EDIT_TIME);
    std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, queryColumns);
    EXPECT_NE(fileAsset, nullptr);
    std::string assetPath = fileAsset->GetPath();
    EXPECT_NE(assetPath, "");
    bool isParentDirExist = CheckAndCreateDir(assetPath);
    
    MediaFileUtils::CopyFileSafe(EFFECT_FILE_PATH, assetPath);
    bool isfileExist = MediaFileUtils::IsFileExists(assetPath);
    EXPECT_TRUE(isfileExist);
    MEDIA_INFO_LOG("InitAsset end");
}

static std::string CreateFileManagerAsset(const std::string &displayName, MediaType type = MediaType::MEDIA_TYPE_IMAGE)
{
    MEDIA_INFO_LOG("CreateFileManagerAsset start");
    std::string uri;
    InitAsset(uri, displayName, type);
    if (!uri.empty()) {
        int32_t retry = 0;
        bool ret = ConvertAssetToFileManager(uri);
        while (!ret && retry < RETRY_MAX) {
            retry++;
            sleep(RETRY_TIME);
            ret = ConvertAssetToFileManager(uri);
        }
        MEDIA_INFO_LOG("CreateFileManagerAsset ret: %{public}d, retry time: %{public}d", ret, retry);
        EXPECT_TRUE(ret);
    }
    MEDIA_INFO_LOG("CreateFileManagerAsset uri : %{public}s", uri.c_str());
    return uri;
}

void MediaLibraryShareOpenTest::InitTestFileAsset(const std::string &id, bool isEdited)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);
    NativeRdb::ValuesBucket values;
    if (isEdited) {
        values.PutLong(PhotoColumn::PHOTO_EDIT_TIME, static_cast<int32_t>(MediaFileUtils::UTCTimeSeconds()));
    }
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, id);
    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("InitTestFileAsset update failed, file_id: %{public}s", id.c_str());
    } else {
        MEDIA_INFO_LOG("InitTestFileAsset update success, file_id: %{public}s, changedRows: %{public}d",
            id.c_str(), changedRows);
    }
}

void MediaLibraryShareOpenTest::InitEditedFiles(const std::string &assetPath)
{
    std::string editData = MediaEditUtils::GetEditDataPath(assetPath);
    bool isParentDirExist = CheckAndCreateDir(editData);
    bool copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_PATH, editData);
    EXPECT_TRUE(copyFile);

    std::string sourceBackPath = MediaEditUtils::GetEditDataSourceBackPath(assetPath);
    isParentDirExist = CheckAndCreateDir(sourceBackPath);
    copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_SOURCE_BACK_PATH, sourceBackPath);
    EXPECT_TRUE(copyFile);

    std::string editDataCameraPath = MediaEditUtils::GetEditDataCameraPath(assetPath);
    isParentDirExist = CheckAndCreateDir(editDataCameraPath);
    copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_CAMERA_PATH, editDataCameraPath);
    EXPECT_TRUE(copyFile);

    std::string sourcePath = MediaEditUtils::GetEditDataSourcePath(assetPath);
    isParentDirExist = CheckAndCreateDir(sourcePath);
    copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_SOURCE_PATH, sourcePath);
    EXPECT_TRUE(copyFile);
}

void MediaLibraryShareOpenTest::InitPhotoAsset(std::string &dataFileUri, bool isEdited)
{
    std::string effectFileName = MediaFileUtils::GetFileName(EFFECT_FILE_PATH);
    int32_t dataFileId = 0;
    
    std::string uriReal = mediaLibraryManager->CreateAsset(effectFileName);
    MEDIA_INFO_LOG("InitPhotoAsset uriReal: %{public}s", uriReal.c_str());
    if (isEdited) {
        dataFileId = MediaLibraryShareOpenTest::CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, effectFileName, true);
    } else {
        dataFileId = MediaLibraryShareOpenTest::CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, effectFileName, false);
    }
    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(dataFileId), "", MEDIA_API_VERSION_V10);
    dataFileUri = fileUri.ToString();
    string id = MediaFileUtils::GetIdFromUri(dataFileUri);
    MediaLibraryShareOpenTest::InitTestFileAsset(id, isEdited);
    std::vector<std::string> queryColumns = PHOTO_COLUMN_VECTOR;
    queryColumns.push_back(PhotoColumn::PHOTO_EDIT_TIME);
    std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, queryColumns);
    EXPECT_NE(fileAsset, nullptr);
    std::string assetPath = fileAsset->GetPath();
    bool isParentDirExist = CheckAndCreateDir(assetPath);

    MediaFileUtils::CopyFileSafe(EFFECT_FILE_PATH, assetPath);
    bool isfileExist = MediaFileUtils::IsFileExists(assetPath);
    EXPECT_NE(assetPath, "");

    if (isEdited) {
        MediaLibraryShareOpenTest::InitEditedFiles(assetPath);
    }
}

static bool QueryAssetPathByUri(const std::string &assetUri, std::string &assetPath)
{
    std::string id = MediaFileUtils::GetIdFromUri(assetUri);
    std::shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        id, OperationObject::FILESYSTEM_PHOTO, PHOTO_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("QueryAssetPathByUri failed, id: %{public}s", id.c_str());
        return false;
    }
    assetPath = fileAsset->GetPath();
    return !assetPath.empty();
}

static bool PrepareMovingPhotoFiles(const std::string &assetUri, std::string &assetPath, std::string &cachePath)
{
    MEDIA_INFO_LOG("PrepareMovingPhotoFiles start");
    if (!QueryAssetPathByUri(assetUri, assetPath)) {
        return false;
    }
    if (!CheckAndCreateDir(assetPath) || !MediaFileUtils::CopyFileSafe(EFFECT_FILE_PATH, assetPath)) {
        MEDIA_ERR_LOG("Prepare moving photo image failed, path: %{public}s", assetPath.c_str());
        return false;
    }

    std::string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(assetPath);
    if (!CheckAndCreateDir(videoPath) || !MediaFileUtils::CopyFileSafe(VIDEO_TEST_FILE_PATH, videoPath)) {
        MEDIA_ERR_LOG("Prepare moving photo video failed, path: %{public}s", videoPath.c_str());
        return false;
    }

    off_t extraDataSize = 0;
    int32_t ret = MovingPhotoFileUtils::GetExtraDataLen(assetPath, videoPath, 0, 0, extraDataSize);
    if (ret != E_OK || extraDataSize <= 0) {
        MEDIA_ERR_LOG("Prepare moving photo extraData failed, ret: %{public}d", ret);
        return false;
    }

    cachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(assetPath);
    if (MediaFileUtils::IsFileExists(cachePath)) {
        MediaFileUtils::DeleteFile(cachePath);
    }
    MEDIA_INFO_LOG("PrepareMovingPhotoFiles end");
    return true;
}

static bool GenerateLivePhotoCache(const std::string &assetUri)
{
    MEDIA_INFO_LOG("GenerateLivePhotoCache start");
    std::string livePhotoUri = assetUri;
    MediaFileUtils::UriAppendKeyValue(livePhotoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
        CONST_OPEN_PRIVATE_LIVE_PHOTO);
    Uri uri(livePhotoUri);
    MediaLibraryCommand cmd(uri, Media::OperationType::OPEN);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "rw");
    if (fd < 0) {
        MEDIA_ERR_LOG("GenerateLivePhotoCache failed, uri: %{public}s", livePhotoUri.c_str());
        return false;
    }
    close(fd);
    MEDIA_INFO_LOG("GenerateLivePhotoCache end");
    return true;
}

static int32_t OpenLivePhotoCacheFd(const std::string &assetUri)
{
    MEDIA_INFO_LOG("OpenLivePhotoCacheFd start");
    std::string livePhotoUri = assetUri;
    MediaFileUtils::UriAppendKeyValue(livePhotoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
        CONST_OPEN_PRIVATE_LIVE_PHOTO);
    Uri uri(livePhotoUri);
    MediaLibraryCommand cmd(uri, Media::OperationType::OPEN);
    cmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    int32_t fd = MediaLibraryDataManager::GetInstance()->OpenFile(cmd, "rw");
    MEDIA_INFO_LOG("OpenLivePhotoCacheFd end, fd: %{public}d", fd);
    return fd;
}

static bool GenerateCompressCache(const std::string &assetUri, std::string &cachePath)
{
    MEDIA_INFO_LOG("GenerateCompressCache start, assetUri: %{public}s", assetUri.c_str());
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        MEDIA_ERR_LOG("GenerateCompressCache failed, dataManager is null");
        return false;
    }

    std::string id = MediaFileUtils::GetIdFromUri(assetUri);
    cachePath = MediaLibraryAssetOperations::GetAssetCompressCachePath(id);
    if (cachePath.empty()) {
        MEDIA_ERR_LOG("GenerateCompressCache failed, cachePath is empty");
        return false;
    }
    if (MediaFileUtils::IsFileExists(cachePath)) {
        MediaFileUtils::DeleteFile(cachePath);
    }

    std::string compressUri = assetUri;
    int32_t type = static_cast<int32_t>(HideSensitiveType::ALL_DESENSITIZE);
    MediaFileUtils::UriAppendKeyValue(compressUri, "type", to_string(type));
    Uri openUri(compressUri);
    MediaLibraryCommand cmd(openUri, Media::OperationType::OPEN);
    int32_t version = dataManager->GetAssetCompressVersion();
    int32_t fd = -1;
    std::string tlvPath;
    int32_t ret = MediaLibraryPhotoOperations::OpenAssetCompress(cmd, tlvPath, version, fd);
    if (ret != E_OK || fd < 0) {
        MEDIA_ERR_LOG("GenerateCompressCache failed, ret: %{public}d, fd: %{public}d", ret, fd);
        return false;
    }
    close(fd);
    cachePath = tlvPath;
    MEDIA_INFO_LOG("GenerateCompressCache end, cachePath: %{public}s", cachePath.c_str());
    return MediaFileUtils::IsFileExists(cachePath);
}

static int32_t OpenCompressCacheFd(const std::string &assetUri, std::string &cachePath)
{
    MEDIA_INFO_LOG("OpenCompressCacheFd start, assetUri: %{public}s", assetUri.c_str());
    auto dataManager = MediaLibraryDataManager::GetInstance();
    if (dataManager == nullptr) {
        MEDIA_ERR_LOG("OpenCompressCacheFd failed, dataManager is null");
        return E_ERR;
    }

    std::string compressUri = assetUri;
    int32_t type = static_cast<int32_t>(HideSensitiveType::ALL_DESENSITIZE);
    MediaFileUtils::UriAppendKeyValue(compressUri, "type", to_string(type));
    Uri openUri(compressUri);
    MediaLibraryCommand cmd(openUri, Media::OperationType::OPEN);
    int32_t version = dataManager->GetAssetCompressVersion();
    int32_t fd = -1;
    std::string tlvPath;
    int32_t ret = MediaLibraryPhotoOperations::OpenAssetCompress(cmd, tlvPath, version, fd);
    if (ret != E_OK || fd < 0) {
        MEDIA_ERR_LOG("OpenCompressCacheFd failed, ret: %{public}d, fd: %{public}d", ret, fd);
        return E_ERR;
    }
    cachePath = tlvPath;
    MEDIA_INFO_LOG("OpenCompressCacheFd end, cachePath: %{public}s, fd: %{public}d", cachePath.c_str(), fd);
    return fd;
}

static void CleanupTempFiles(const std::initializer_list<std::string> &paths)
{
    for (const auto &path : paths) {
        if (!path.empty() && MediaFileUtils::IsFileExists(path)) {
            EXPECT_TRUE(MediaFileUtils::DeleteFile(path));
        }
    }
}

struct FileDigestInfo {
    uint64_t hash = 14695981039346656037ULL;
    uint64_t size = 0;
    bool valid = false;
};

static FileDigestInfo CalculateFileDigest(const std::string &path)
{
    constexpr uint64_t fnvPrime = 1099511628211ULL;
    constexpr size_t bufferSize = 4096;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return {};
    }

    FileDigestInfo digestInfo;
    char buffer[bufferSize];
    while (ifs.good()) {
        ifs.read(buffer, bufferSize);
        std::streamsize bytesRead = ifs.gcount();
        for (std::streamsize index = 0; index < bytesRead; ++index) {
            digestInfo.hash ^= static_cast<unsigned char>(buffer[index]);
            digestInfo.hash *= fnvPrime;
        }
        digestInfo.size += static_cast<uint64_t>(bytesRead);
    }
    digestInfo.valid = !ifs.bad();
    return digestInfo;
}

static void InitMovingPhotoAsset(std::string &assetUri)
{
    MEDIA_INFO_LOG("InitMovingPhotoAsset start");
    std::string effectFileName = MediaFileUtils::GetFileName(EFFECT_FILE_PATH);
    int32_t dataFileId = MediaLibraryShareOpenTest::CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, effectFileName,
        false, true);
    ASSERT_GT(dataFileId, 0);

    MediaFileUri fileUri(MediaType::MEDIA_TYPE_IMAGE, to_string(dataFileId), "", MEDIA_API_VERSION_V10);
    assetUri = fileUri.ToString();
    std::string id = MediaFileUtils::GetIdFromUri(assetUri);
    MediaLibraryShareOpenTest::InitTestFileAsset(id, false);
    MEDIA_INFO_LOG("InitMovingPhotoAsset assetUri: %{public}s", assetUri.c_str());
}

void MediaLibraryShareOpenTest::CopyToDestPath(int32_t srcFd, const std::string &destPath)
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

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_001, TestSize.Level0)
{
    ASSERT_TRUE(MediaLibraryShareOpenTest::IsValid());
    std::string dataPath = "/data/local/tmp/assets_share_resources/assets_share/CreateImageLcdTest_001.jpg";
    bool isFileExist = MediaFileUtils::IsFileExists(dataPath);
    MEDIA_INFO_LOG("media_library_share_test_001 file exist: %{public}d", isFileExist);
    EXPECT_TRUE(isFileExist);
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_002, TestSize.Level0)
{
    std::string uri = "";
    bool isEdited = true;
    MediaLibraryShareOpenTest::InitPhotoAsset(uri, isEdited);

    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);

    int32_t type = static_cast<int32_t>(HideSensitiveType::ALL_DESENSITIZE);
    int32_t version = dataManager->GetAssetCompressVersion();
    int32_t fd = -1;
    MediaFileUtils::UriAppendKeyValue(uri, "type", to_string(static_cast<int32_t>(type)));
    MEDIA_DEBUG_LOG("merged uri = %{public}s", uri.c_str());
    Uri openUri(uri);
    MediaLibraryCommand cmd(openUri, Media::OperationType::OPEN);
    std::string tlvPath = "";
    int32_t ret =
        MediaLibraryPhotoOperations::OpenAssetCompress(cmd, tlvPath, version, fd);
    EXPECT_EQ(ret, E_OK);
    MediaLibraryShareOpenTest::CopyToDestPath(fd, TLV_FILE_CACHE_FILE);
    UniqueFd srcFdGuard(fd);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(TLV_FILE_CACHE_FILE));

    std::unordered_map<TlvTag, std::string> extractedFiles = {};
    ret = TlvUtil::ExtractTlv(TLV_FILE_CACHE_FILE, TLV_FILE_CACHE, extractedFiles);
    for (const auto &item : extractedFiles) {
        GTEST_LOG_(INFO)<< "media_library_share_test_002 ExtractedFiles tag" << item.first
            << " path " << item.second;
    }
    
    ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_003, TestSize.Level0)
{
    std::string uri = "";
    MediaLibraryShareOpenTest::InitPhotoAsset(uri);

    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);

    int32_t type = static_cast<int32_t>(HideSensitiveType::ALL_DESENSITIZE);
    int32_t version = dataManager->GetAssetCompressVersion();
    int32_t fd = -1;
    MediaFileUtils::UriAppendKeyValue(uri, "type", to_string(static_cast<int32_t>(type)));
    Uri openUri(uri);
    MediaLibraryCommand cmd(openUri, Media::OperationType::OPEN);
    std::string tlvPath = "";
    int32_t ret =
        MediaLibraryPhotoOperations::OpenAssetCompress(cmd, tlvPath, version, fd);
    MediaLibraryShareOpenTest::CopyToDestPath(fd, OPEN_ORIGINAL_FILE);
    UniqueFd srcFdGuard(fd);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(OPEN_ORIGINAL_FILE));

    ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri);
    EXPECT_EQ(ret, E_OK);
}

#ifdef MEDIALIBRARY_FEATURE_CUSTOM_RESTORE
HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_004 Start");
    PhotoCustomRestoreOperation &operatorObj = PhotoCustomRestoreOperation::GetInstance();

    string tlvPathAccept = TLV_FILE_CACHE_FILE;
    if (!MediaFileUtils::IsFileExists(tlvPathAccept)) {
        MEDIA_ERR_LOG("TLV file does not exist: %{public}s", tlvPathAccept.c_str());
    }
    RestoreTaskInfo restoreTaskInfo;
    restoreTaskInfo.keyPath = tlvPathAccept;
    const unordered_map<string, TimeInfo> timeInfoMap = operatorObj.GetTimeInfoMap(restoreTaskInfo);
    const vector<string> filePathVector = {tlvPathAccept};
    bool isFirst = true;
    UniqueNumber uniqueNumber{0, 0, 0, 0};

    auto ret = operatorObj.HandleTlvRestore(timeInfoMap, restoreTaskInfo, filePathVector, isFirst, uniqueNumber);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("media_library_share_test_004 End");
}
#endif

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_005 Start");
    std::string uri = "";
    MediaLibraryShareOpenTest::InitPhotoAsset(uri);
    const vector<string> uris = {uri};
    int64_t totalSize = 0;
    int32_t ret = E_OK;

    string id = MediaFileUtils::GetIdFromUri(uri);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore != nullptr) {
        NativeRdb::AbsRdbPredicates predicates(PhotoExtColumn::PHOTOS_EXT_TABLE);
        predicates.EqualTo(PhotoExtColumn::PHOTO_ID, id);
        std::vector<std::string> columns = { PhotoExtColumn::EDITDATA_SIZE };
        auto resultSet = rdbStore->Query(predicates, columns);
        if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
            ret = MediaLibraryPhotoOperations::GetCompressAssetSize(uris, totalSize);
        }
    }
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("GetCompressAssetSize totalSize: %{public}" PRId64, totalSize);
    MEDIA_INFO_LOG("media_library_share_test_005 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_006 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);

    std::string assetPath;
    std::string cachePath;
    std::string compressCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, cachePath));
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    ASSERT_TRUE(GenerateLivePhotoCache(uri));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_006 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_007 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);

    std::string assetPath;
    std::string cachePath;
    std::string compressCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, cachePath));
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    ASSERT_FALSE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_007 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_008 Start");
    std::string uri;
    InitPhotoAsset(uri, false);
    std::string compressCachePath;
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_008 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_009 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);
    std::string assetPath;
    std::string cachePath;
    std::string compressCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, cachePath));
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    ASSERT_TRUE(GenerateLivePhotoCache(uri));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended("", ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_ERR);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({cachePath, compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_009 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_010 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);

    std::string assetPath;
    std::string cachePath;
    std::string compressCachePath;
    ASSERT_TRUE(QueryAssetPathByUri(uri, assetPath));
    ASSERT_TRUE(CheckAndCreateDir(assetPath));
    ASSERT_TRUE(MediaFileUtils::CopyFileSafe(EFFECT_FILE_PATH, assetPath));
    cachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(assetPath);
    if (MediaFileUtils::IsFileExists(cachePath)) {
        EXPECT_TRUE(MediaFileUtils::DeleteFile(cachePath));
    }
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    std::string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(assetPath);
    if (MediaFileUtils::IsFileExists(videoPath)) {
        EXPECT_TRUE(MediaFileUtils::DeleteFile(videoPath));
    }
    EXPECT_FALSE(GenerateLivePhotoCache(uri));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(cachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_010 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_011 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);
    std::string assetPath;
    std::string cachePath;
    std::string compressCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, cachePath));
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    ASSERT_TRUE(GenerateLivePhotoCache(uri));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, static_cast<ServiceShareType>(-1));
    EXPECT_EQ(ret, E_ERR);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(cachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    CleanupTempFiles({cachePath, compressCachePath});
    MEDIA_INFO_LOG("media_library_share_test_011 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_012 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);

    std::string assetPath;
    std::string livePhotoCachePath;
    std::string compressCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, livePhotoCachePath));
    ASSERT_TRUE(GenerateCompressCache(uri, compressCachePath));
    ASSERT_TRUE(GenerateLivePhotoCache(uri));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(livePhotoCachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(compressCachePath));
    EXPECT_TRUE(MediaFileUtils::IsFileExists(livePhotoCachePath));
    CleanupTempFiles({livePhotoCachePath});
    MEDIA_INFO_LOG("media_library_share_test_012 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_013 Start");
    std::string uri;
    InitPhotoAsset(uri, false);

    std::string compressCachePath;
    int32_t fd = OpenCompressCacheFd(uri, compressCachePath);
    ASSERT_GE(fd, 0);
    UniqueFd srcFdGuard(fd);
    ASSERT_TRUE(MediaFileUtils::IsFileExists(compressCachePath));
    FileDigestInfo expectedDigest = CalculateFileDigest(compressCachePath);
    ASSERT_TRUE(expectedDigest.valid);
    ASSERT_GT(expectedDigest.size, 0);

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(compressCachePath));

    ASSERT_NE(lseek(fd, 0, SEEK_SET), -1);
    MediaLibraryShareOpenTest::CopyToDestPath(fd, OPEN_COMPRESS_AFTER_DELETE_FILE);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(OPEN_COMPRESS_AFTER_DELETE_FILE));
    FileDigestInfo actualDigest = CalculateFileDigest(OPEN_COMPRESS_AFTER_DELETE_FILE);
    EXPECT_TRUE(actualDigest.valid);
    EXPECT_EQ(actualDigest.size, expectedDigest.size);
    EXPECT_EQ(actualDigest.hash, expectedDigest.hash);
    CleanupTempFiles({OPEN_COMPRESS_AFTER_DELETE_FILE});
    MEDIA_INFO_LOG("media_library_share_test_013 End");
}

HWTEST_F(MediaLibraryShareOpenTest, media_library_share_test_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("media_library_share_test_014 Start");
    std::string uri;
    InitMovingPhotoAsset(uri);

    std::string assetPath;
    std::string livePhotoCachePath;
    ASSERT_TRUE(PrepareMovingPhotoFiles(uri, assetPath, livePhotoCachePath));

    int32_t fd = OpenLivePhotoCacheFd(uri);
    ASSERT_GE(fd, 0);
    UniqueFd srcFdGuard(fd);
    ASSERT_TRUE(MediaFileUtils::IsFileExists(livePhotoCachePath));
    FileDigestInfo expectedDigest = CalculateFileDigest(livePhotoCachePath);
    ASSERT_TRUE(expectedDigest.valid);
    ASSERT_GT(expectedDigest.size, 0);

    int32_t ret = MediaLibraryPhotoOperations::NotifyAssetSended(uri, ServiceShareType::NON_ASSET_LEVEL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(MediaFileUtils::IsFileExists(livePhotoCachePath));

    ASSERT_NE(lseek(fd, 0, SEEK_SET), -1);
    MediaLibraryShareOpenTest::CopyToDestPath(fd, OPEN_LIVE_PHOTO_AFTER_DELETE_FILE);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(OPEN_LIVE_PHOTO_AFTER_DELETE_FILE));
    FileDigestInfo actualDigest = CalculateFileDigest(OPEN_LIVE_PHOTO_AFTER_DELETE_FILE);
    EXPECT_TRUE(actualDigest.valid);
    EXPECT_EQ(actualDigest.size, expectedDigest.size);
    EXPECT_EQ(actualDigest.hash, expectedDigest.hash);
    CleanupTempFiles({OPEN_LIVE_PHOTO_AFTER_DELETE_FILE});
    MEDIA_INFO_LOG("media_library_share_test_014 End");
}
} // namespace Media
} // namespace OHOS