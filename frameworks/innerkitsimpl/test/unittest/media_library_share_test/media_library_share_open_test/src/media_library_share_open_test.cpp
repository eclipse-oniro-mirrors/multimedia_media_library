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
#include "photo_custom_restore_operation.h"
#define protected public
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_photo_operations.h"
#undef protected
#undef private

#include "ability_context_impl.h"
#include "base_data_uri.h"
#include "custom_restore_const.h"
#include "directory_ex.h"
#include "file_asset.h"
#include "get_self_permissions.h"
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
#include "rdb_store.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "unique_fd.h"
#include "tlv_util.h"
#include "photo_file_utils.h"

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
const std::string TLV_FILE_CACHE = TEST_FILE_ROOT + "assets_share/tlv_cache/";
const std::string TLV_FILE_CACHE_FILE = TLV_FILE_CACHE + "tlv_file_cache.tlv";
const std::string OPEN_ORIGINAL_FILE = TLV_FILE_CACHE + "open_origin.jpg";

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
    MediaLibraryShareOpenTest::ProcessPermission();
    MediaLibraryShareOpenTest::InitMediaLibrary();
    MediaLibraryShareOpenTest::CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
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
    std::string editData = PhotoFileUtils::GetEditDataPath(assetPath);
    bool isParentDirExist = CheckAndCreateDir(editData);
    bool copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_PATH, editData);
    EXPECT_TRUE(copyFile);

    std::string sourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(assetPath);
    isParentDirExist = CheckAndCreateDir(sourceBackPath);
    copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_SOURCE_BACK_PATH, sourceBackPath);
    EXPECT_TRUE(copyFile);

    std::string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(assetPath);
    isParentDirExist = CheckAndCreateDir(editDataCameraPath);
    copyFile = MediaFileUtils::CopyFileSafe(EDIT_DATA_CAMERA_PATH, editDataCameraPath);
    EXPECT_TRUE(copyFile);

    std::string sourcePath = PhotoFileUtils::GetEditDataSourcePath(assetPath);
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
    
    ret = dataManager->NotifyAssetSended(uri);
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

    ret = dataManager->NotifyAssetSended(uri);
    EXPECT_EQ(ret, E_OK);
}

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
} // namespace Media
} // namespace OHOS