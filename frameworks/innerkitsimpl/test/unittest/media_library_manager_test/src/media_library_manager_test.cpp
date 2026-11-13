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

#include <fstream>
#include <functional>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <utility>

#include "media_library_manager_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "medialibrary_mock_tocken.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include "media_app_uri_permission_column.h"
#include "ability_context_impl.h"
#include "medialibrary_db_const.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_library_extend_manager.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "system_ability_definition.h"
#include "thumbnail_const.h"
#include "userfilemgr_uri.h"
#include "data_secondary_directory_uri.h"
#include "photo_album_column.h"
#include "medialibrary_unistore_manager.h"
#include "photo_file_utils.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include "medialibrary_unittest_utils.h"
#include "analysis_metadata_query_test_helper.h"
#include "get_assets_analysis_metadata_vo.h"
#include "media_resp_vo.h"
#include "media_req_vo.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

/**
 * @FileName MediaLibraryManagerTest
 * @Desc Media library manager native function test
 *
 */
namespace OHOS {
namespace Media {
const string API_VERSION = "api_version";
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
int g_albumMediaType = MEDIA_TYPE_ALBUM;
const int CLEAN_TIME = 1;
const int SCAN_WAIT_TIME = 10;
constexpr int32_t URI_SIZE = 101;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const std::string FILE_MANAGER_TEST_DIR = "/storage/media/local/files/Docs/Download/ManagerTest/";

static string JoinFormats(const vector<string> &formats)
{
    string result = "";
    for (size_t i = 0; i < formats.size(); i++) {
        result += formats[i];
        if (i + 1 < formats.size()) {
            result += ",";
        }
    }
    return result;
}

static const unsigned char FILE_CONTENT_JPG[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b,
    0x20, 0x20, 0x20,
};

static const unsigned char FILE_CONTENT_MP4[] = {
    0x20, 0x20, 0x20, 0x20, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6f, 0x6d, 0x20, 0x20, 0x02, 0x20, 0x69, 0x73, 0x6f,
    0x6d, 0x69, 0x73, 0x6f, 0x32, 0x61, 0x76, 0x63, 0x31, 0x6d, 0x70, 0x34, 0x31, 0x20, 0x20, 0x20, 0x08, 0x66, 0x72,
    0x65, 0x65, 0x20, 0x49, 0xdd, 0x01, 0x6d, 0x64, 0x61, 0x74, 0x20, 0x20, 0x02, 0xa0, 0x06, 0x05, 0xff, 0xff, 0x9c,
};

static const std::vector<std::string> EXPECTED_IMAGE_FORMATS = {
    "xpm", "xbm", "rgb", "pef", "psd", "jng", "cpt", "pat", "wbmp", "ras", "ief", "crw",
    "djvu", "djv", "tiff", "tif", "pcx", "ipm", "svg", "svgz", "jp2", "jpg2", "raw", "jpg",
    "jpeg", "jpe", "nrw", "jpm", "raf", "cur", "heif", "hif", "orf", "avif", "heic", "nef",
    "gif", "arw", "ico", "cdt", "rw2", "pgm", "heics", "heifs", "cdr", "pbm", "ppm", "png",
    "cr2", "dng", "art", "srw", "xwd", "erf", "jpx", "jpf", "bmp", "bm", "webp", "pnm"
};

static const std::vector<std::string> EXPECTED_VIDEO_FORMATS = {
    "wmx", "wm", "asf", "asx", "mng", "lsf", "lsx", "gl", "dif", "dv", "axv", "avi",
    "flv", "h264", "webm", "mkv", "mpv", "mov", "qt", "wvx", "wrf", "yt", "m4v", "f4v",
    "mp4v", "mpeg4", "mp4", "3gpp", "3gp", "3gpp2", "3gp2", "3g2", "ts", "rmvb", "dl",
    "fli", "mxu", "mpe", "mpeg", "mpeg2", "mpv2", "mp2v", "m2v", "m2t", "mpeg1", "mpv1",
    "mp1v", "m1v", "mpg", "m2ts", "mts", "wmv", "movie", "ogv"
};

static const std::vector<std::string> perms = {
    "ohos.permission.READ_IMAGEVIDEO",
    "ohos.permission.WRITE_IMAGEVIDEO",
    "ohos.permission.READ_AUDIO",
    "ohos.permission.WRITE_AUDIO",
    "ohos.permission.READ_MEDIA",
    "ohos.permission.WRITE_MEDIA",
    "ohos.permission.MEDIA_LOCATION",
    "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED"
};

static std::shared_ptr<MediaLibraryMockHapToken> hapToken = nullptr;

MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();
MediaLibraryExtendManager* mediaLibraryExtendManager = MediaLibraryExtendManager::GetMediaLibraryExtendManager();

void MediaLibraryManagerTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryManagerTest::SetUpTestCase:: invoked");
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    if (sDataShareHelper_ == nullptr) {
        exit(0);
    }

    // make sure board is empty
    ClearAllFile();

    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);
    mediaLibraryExtendManager->InitMediaLibraryExtendManager();
    MEDIA_INFO_LOG("MediaLibraryManagerTest::SetUpTestCase:: Finish");
}

void MediaLibraryManagerTest::TearDownTestCase(void)
{
    MEDIA_ERR_LOG("TearDownTestCase start");
    if (sDataShareHelper_ != nullptr) {
        sDataShareHelper_->Release();
    }
    sleep(CLEAN_TIME);
    ClearAllFile();
    MEDIA_INFO_LOG("TearDownTestCase end");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void mockToken(const std::vector<std::string>& perms, shared_ptr<MediaLibraryMockHapToken>& token)
{
    // mock tokenID
    token = make_shared<MediaLibraryMockHapToken>("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

// SetUp:Execute before each test case
void MediaLibraryManagerTest::SetUp(void)
{
    // restore shell token before testcase
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaLibraryManagerTest::TearDown(void)
{
    // rescovery shell toekn after tesecase
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();
}

void CreateDataHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    ASSERT_NE(remoteObj, nullptr);

    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");

    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        if (sDataShareHelper_ == nullptr) {
            exit(0);
        }
    }
    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
}

void ClearAllFile()
{
    system("rm -rf /storage/media/100/local/files/.thumbs/*");
    system("rm -rf /storage/cloud/100/files/Audio/*");
    system("rm -rf /storage/cloud/100/files/Audios/*");
    system("rm -rf /storage/cloud/100/files/Camera/*");
    system("rm -rf /storage/cloud/100/files/Docs/Documents/*");
    system("rm -rf /storage/cloud/100/files/Photo/*");
    system("rm -rf /storage/cloud/100/files/Pictures/*");
    system("rm -rf /storage/cloud/100/files/Docs/Download/*");
    system("rm -rf /storage/cloud/100/files/Docs/.*");
    system("rm -rf /storage/cloud/100/files/Videos/*");
    system("rm -rf /storage/cloud/100/files/.*");
    system("rm -rf /data/app/el2/100/database/com.ohos.medialibrary.medialibrarydata/*");
    system("kill -9 `pidof com.ohos.medialibrary.medialibrarydata`");
    system("scanner");
}

static int32_t CloseAssetByPhotoAccess(const std::string &assetUri)
{
    CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ != nullptr, E_FAIL,
        "CloseAssetByPhotoAccess failed, dataShareHelper is null");
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_URI, assetUri);
    std::string closeUri = CONST_PAH_CLOSE_PHOTO;
    MediaFileUtils::UriAppendKeyValue(closeUri, URI_PARAM_API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    MediaFileUtils::UriAppendKeyValue(closeUri, MediaColumn::MEDIA_TIME_PENDING, "0");
    Uri closeAssetUri(closeUri);
    return sDataShareHelper_->Insert(closeAssetUri, valuesBucket);
}

static int32_t CloseAssetWithFallback(const std::string &assetUri, int32_t fd)
{
    int32_t closeRet = mediaLibraryManager->CloseAsset(assetUri, fd);
    if (closeRet == E_OK || closeRet == E_SUCCESS || closeRet != E_INVALID_FILEID) {
        return closeRet;
    }
    int32_t pahCloseRet = CloseAssetByPhotoAccess(assetUri);
    if (pahCloseRet == E_OK || pahCloseRet == E_SUCCESS) {
        MEDIA_INFO_LOG("CloseAsset fallback to PAH close success, uri=%{public}s", assetUri.c_str());
        return E_SUCCESS;
    }
    MEDIA_WARN_LOG(
        "CloseAsset fallback to PAH close failed, uri=%{public}s, closeRet=%{public}d, pahCloseRet=%{public}d",
        assetUri.c_str(), closeRet, pahCloseRet);
    return closeRet;
}

static string CreatePhotoAsset(string displayName)
{
    int32_t resWrite = -1;
    auto uri = mediaLibraryManager->CreateAsset(displayName);
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    EXPECT_NE(destFd <= 0, true);
    if (displayName.find(".jpg") != std::string::npos) {
        resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    } else if (displayName.find(".mp4") != std::string::npos) {
        resWrite = write(destFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    }
    (void)CloseAssetWithFallback(uri, destFd);
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    return uri;
}

static std::shared_ptr<MediaLibraryRdbStore> TryGetRdbStoreWithInit()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore != nullptr) {
        return rdbStore;
    }

    MEDIA_WARN_LOG("rdbStore is nullptr, try InitMediaLibraryMgr once");
    MediaLibraryUnitTestUtils::Init();
    return MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

using RdbStorePtr = decltype(TryGetRdbStoreWithInit());

static bool QueryAssetPathFromDb(const RdbStorePtr &rdbStore, const std::string &fileId, std::string &assetPath)
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
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(assetPath), false,
        "create asset file failed, path=%{public}s", assetPath.c_str());
    return true;
}

static bool MoveAssetToFileManagerPath(const std::string &assetPath, std::string &storagePath)
{
    std::string fileName = MediaFileUtils::GetFileName(assetPath);
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), false, "failed to get fileName from assetPath=%{public}s",
        assetPath.c_str());
    storagePath = FILE_MANAGER_TEST_DIR;
    if (storagePath.back() != '/') {
        storagePath += "/";
    }
    storagePath += fileName;
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
    return true;
}

static bool UpdateFileManagerSourceInDb(const RdbStorePtr &rdbStore, const std::string &fileId,
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

static bool ConvertAssetToFileManager(const std::string &uri)
{
    CHECK_AND_RETURN_RET_LOG(!uri.empty() && !FILE_MANAGER_TEST_DIR.empty(), false,
        "invalid input, uri empty: %{public}d, testDir empty: %{public}d", uri.empty(), FILE_MANAGER_TEST_DIR.empty());
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), false, "failed to parse fileId from uri: %{public}s", uri.c_str());
    auto rdbStore = TryGetRdbStoreWithInit();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    std::string assetPath;
    CHECK_AND_RETURN_RET_LOG(QueryAssetPathFromDb(rdbStore, fileId, assetPath), false,
        "query assetPath failed, fileId=%{public}s", fileId.c_str());
    CHECK_AND_RETURN_RET_LOG(EnsureAssetFileExists(assetPath), false,
        "ensure asset file exists failed, path=%{public}s", assetPath.c_str());
    std::string storagePath;
    CHECK_AND_RETURN_RET_LOG(MoveAssetToFileManagerPath(assetPath, storagePath), false,
        "move asset to file manager path failed, fileId=%{public}s", fileId.c_str());
    return UpdateFileManagerSourceInDb(rdbStore, fileId, storagePath);
}

static std::string CreateFileManagerAsset(const std::string &displayName)
{
    std::string uri = CreatePhotoAsset(displayName);
    if (!uri.empty()) {
        ConvertAssetToFileManager(uri);
    }
    return uri;
}

static bool CompareIfArraysEquals(const unsigned char originArray[],
    const unsigned char targetArray[], int32_t size)
{
    for (int i = 0; i < size - 1; i++) {
        if (originArray[i] != targetArray[i]) {
            return false;
        }
    }
    return true;
}

static string GetFileAssetUri(unique_ptr<FileAsset> &fileAsset)
{
    string filePath = fileAsset->GetPath();
    string displayName = fileAsset->GetDisplayName();
    string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    return MediaFileUtils::GetUriByExtrConditions("file://media/Photo/", to_string(fileAsset->GetId()), extrUri);
}
/**
 * @tc.number    : MediaLibraryManager_test_001
 * @tc.name      : create a test.jpg
 * @tc.desc      : create a image asset to see if error occurs
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_test_001::Start");
    string displayName = "test.jpg";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    auto ret = CloseAssetWithFallback(uri, destFd);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", displayName.c_str());
}

/**
 * @tc.number    : MediaLibraryManager_test_002
 * @tc.name      : create image again to see if error occurs
 * @tc.desc      : create same name file to see if error occur and
 *               : read image msg to see if equals
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_test_002::Start");
    string displayName = "test2.jpg";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    ASSERT_NE(uri, "");
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

    int32_t srcFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    int64_t srcLen = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(srcLen));
    ASSERT_NE(buf, nullptr);
    read(srcFd, buf, srcLen);
    EXPECT_EQ(CompareIfArraysEquals(buf, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG)), true);
    free(buf);
    mediaLibraryManager->CloseAsset(uri, srcFd);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", displayName.c_str());
}

/**
 * @tc.number    : MediaLibraryManager_test_003
 * @tc.name      : create video to see if error occurs
 * @tc.desc      : create video file to see if error occur and
 *               : read video msg to see if equals
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_test_003::Start");
    string displayName = "testVideo.mp4";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    ASSERT_NE(uri, "");
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    ASSERT_NE(resWrite, -1);

    mediaLibraryManager->CloseAsset(uri, destFd);
    int32_t srcFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    int64_t srcLen = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(srcLen));
    ASSERT_NE(buf, nullptr);
    read(srcFd, buf, srcLen);
    EXPECT_EQ(CompareIfArraysEquals(buf, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4)), true);
    free(buf);
    mediaLibraryManager->CloseAsset(uri, srcFd);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", displayName.c_str());
}

/**
 * @tc.number    : MediaLibraryManager_test_004
 * @tc.name      : create error type asset testVideo.xxx to see if error occurs
 * @tc.desc      : create error type asset to see if error occurs
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_test_004::Start");
    string displayName = "testVideo.xxx";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    EXPECT_EQ(uri, "");
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", displayName.c_str());
}

/**
 * @tc.number    : MediaLibraryManager_test_005
 * @tc.name      : create png image again to see if error occurs
 * @tc.desc      : create png image to see if error occur and
 *               : read image msg to see if equals
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_test_005::Start");
    string displayName = "testPNG.png";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    ASSERT_NE(uri, "");
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

    int32_t srcFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    int64_t srcLen = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(srcLen));
    ASSERT_NE(buf, nullptr);
    read(srcFd, buf, srcLen);
    free(buf);
    mediaLibraryManager->CloseAsset(uri, srcFd);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", displayName.c_str());
}

/**
 * @tc.number    : MediaLibraryManager_GetBatchAstcs_test_006
 * @tc.name      : Query astc batch to see if error occurs
 * @tc.desc      : Input uri list to obtain astc bacth
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetBatchAstcs_test_006, TestSize.Level1)
{
    vector<string> uriBatch;
    vector<vector<uint8_t>> astcBatch;
    int ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);
    EXPECT_EQ(ret, E_INVALID_URI);

    string beginUri = "file://media/Photo/64/IMG_063/IMG_11311.jpg?oper=astc&width=256&height=256&time_id=00000001";
    uriBatch.push_back("0000000001");
    ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);
    EXPECT_EQ(ret, E_INVALID_URI);

    uriBatch.clear();
    uriBatch.push_back(beginUri);
    ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);
    EXPECT_EQ(ret, E_INVALID_URI);

    uriBatch.clear();
    beginUri = "file://media/Photo/64/IMG_063/IMG_11311.jpg?oper=astc&width=128&height=128&time_id=00000001";
    uriBatch.push_back(beginUri);
    ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);

    uriBatch.clear();
    beginUri = "file://media/Photo/64/IMG_063/IMG_11311.jpg?oper=astc&width=64&height=64&time_id=00000001";
    uriBatch.push_back(beginUri);
    ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);
}

/**
 * @tc.number    : MediaLibraryManager_GetAstc_test_007
 * @tc.name      : Get astc image to see if error occurs
 * @tc.desc      : Input uri to obtain astc
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAstc_test_007, TestSize.Level1)
{
    string uriStr1 = "file://media/Photo/64/test?oper=thumbnail&width=256&height=256&path=test";
    auto pixelmap1 = mediaLibraryManager->GetAstc(Uri(uriStr1));
    EXPECT_EQ(pixelmap1, nullptr);

    string uriStr2 = "file://media/Photo/64/testoper=astc&width=256&height=256&path=test";
    auto pixelmap2 = mediaLibraryManager->GetAstc(Uri(uriStr2));
    EXPECT_EQ(pixelmap2, nullptr);

    string uriStr3 = "file://media/Photo/64/test?oper=astc&width=256&height=256&path=test";
    auto pixelmap3 = mediaLibraryManager->GetAstc(Uri(uriStr3));
    EXPECT_EQ(pixelmap3, nullptr);
}

/**
 * @tc.number    : MediaLibraryManager_test_008
 * @tc.name      : Read video of moving photo to see if error occurs
 * @tc.desc      : Input uri to read video of moving photo
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_008, TestSize.Level1)
{
    // read invalid uri
    EXPECT_EQ(mediaLibraryManager->ReadMovingPhotoVideo(""), -1);
    EXPECT_EQ(mediaLibraryManager->ReadMovingPhotoVideo("file://media/Photo"), -1);
    EXPECT_EQ(mediaLibraryManager->ReadMovingPhotoVideo("file://media/Photo/1"), -1);
    EXPECT_EQ(mediaLibraryManager->ReadMovingPhotoVideo("file://media/Photo/1/IMG_008/IMG_008.jpg"), -1);

    string displayName = "movingPhoto.jpg";
    string uri = mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    ASSERT_NE(uri, "");

    int32_t fd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    EXPECT_GE(fd, 0);
    mediaLibraryManager->CloseAsset(uri, fd);

    int32_t rfd = mediaLibraryManager->ReadMovingPhotoVideo(uri);
    EXPECT_LE(rfd, 0);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_test_009, TestSize.Level1)
{
    // read invalid uri
    EXPECT_EQ(mediaLibraryManager->ReadPrivateMovingPhoto(""), -1);
    EXPECT_EQ(mediaLibraryManager->ReadPrivateMovingPhoto("file://media/Photo"), -1);
    EXPECT_EQ(mediaLibraryManager->ReadPrivateMovingPhoto("file://media/Photo/1"), -1);
    EXPECT_EQ(mediaLibraryManager->ReadPrivateMovingPhoto("file://media/Photo/1/IMG_009/IMG_009.jpg"), -1);

    string displayName = "movingPhoto.jpg";
    string uri = mediaLibraryManager->CreateAsset(displayName);
    MEDIA_INFO_LOG("createFile uri: %{public}s", uri.c_str());
    ASSERT_NE(uri, "");

    int32_t fd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    EXPECT_GE(fd, 0);
    mediaLibraryManager->CloseAsset(uri, fd);
}

// Scenario1: Test when uriBatch is empty then GetBatchAstcs returns E_INVALID_URI.
HWTEST_F(MediaLibraryManagerTest, GetBatchAstcs_ShouldReturnE, TestSize.Level1)
{
    std::vector<std::string> uriBatch;
    std::vector<std::vector<uint8_t>> astcBatch;
    EXPECT_EQ(manager.GetBatchAstcs(uriBatch, astcBatch), E_INVALID_URI);
}

// Scenario2: Test when uriBatch contains CONST_ML_URI_OFFSET then GetBatchAstcs calls GetAstcsByOffset.
HWTEST_F(MediaLibraryManagerTest, GetBatchAstcs_ShouldCallGetAstcsByOffset_001, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"/media/ml/uri/offset/1"};
    std::vector<std::vector<uint8_t>> astcBatch;
    EXPECT_EQ(manager.GetBatchAstcs(uriBatch, astcBatch), E_INVALID_URI);
}

HWTEST_F(MediaLibraryManagerTest, GetBatchAstcs_ShouldCallGetAstcsByOffset_002, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"/media/ml/uri/offset/1/image?size=100x200"};
    std::vector<std::vector<uint8_t>> astcBatch;
    EXPECT_EQ(manager.GetBatchAstcs(uriBatch, astcBatch), E_INVALID_URI);
}

HWTEST_F(MediaLibraryManagerTest, GetBatchAstcs_ShouldCallGetAstcsByOffset_003, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"/media/ml/uri/offset/1/image?size=32x32"};
    std::vector<std::vector<uint8_t>> astcBatch;
    EXPECT_EQ(manager.GetBatchAstcs(uriBatch, astcBatch), E_INVALID_URI);
}

// Scenario3: Test when uriBatch does not contain CONST_ML_URI_OFFSET then GetBatchAstcs calls GetAstcsBatch.
HWTEST_F(MediaLibraryManagerTest, GetBatchAstcs_ShouldCallGetAstcsBatch_004, TestSize.Level1)
{
    std::vector<std::string> uriBatch = {"/media/ml/uri/1"};
    std::vector<std::vector<uint8_t>> astcBatch;
    EXPECT_EQ(manager.GetBatchAstcs(uriBatch, astcBatch), E_INVALID_URI);
}

HWTEST_F(MediaLibraryManagerTest, ReadMovingPhotoVideo_001, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(manager.ReadMovingPhotoVideo(uri), -1);
}

HWTEST_F(MediaLibraryManagerTest, ReadMovingPhotoVideo_002, TestSize.Level1)
{
    string uri = "..;";
    EXPECT_EQ(manager.ReadMovingPhotoVideo(uri), -1);
}

HWTEST_F(MediaLibraryManagerTest, ReadMovingPhoto_001, TestSize.Level1)
{
    string uri = "";
    EXPECT_EQ(manager.ReadPrivateMovingPhoto(uri), -1);
}

HWTEST_F(MediaLibraryManagerTest, ReadMovingPhoto_002, TestSize.Level1)
{
    string uri = "..;";
    EXPECT_EQ(manager.ReadPrivateMovingPhoto(uri), -1);
}

HWTEST_F(MediaLibraryManagerTest, GetMovingPhotoImageUri_001, TestSize.Level1)
{
    std::string uri = "";
    std::string result = manager.GetMovingPhotoImageUri(uri);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaLibraryManagerTest, GetMovingPhotoImageUri_002, TestSize.Level1)
{
    std::string uri = "mediaLibraryUri";
    std::string result = manager.GetMovingPhotoImageUri(uri);
    EXPECT_EQ(result, uri);
}

HWTEST_F(MediaLibraryManagerTest, GetUrisByOldUris_001, TestSize.Level1)
{
    std::vector<std::string> uris;
    // test case 1 empty uris will return emty map
    auto ret = manager.GetUrisByOldUris(uris);
    EXPECT_EQ(ret.empty(), true);

    // test case 2 normal uris
    uris.emplace_back(CONST_UFM_CREATE_PHOTO);
    uris.emplace_back(CONST_UFM_CREATE_AUDIO);
    uris.emplace_back(CONST_UFM_CREATE_PHOTO_ALBUM);
    ret = manager.GetUrisByOldUris(uris);
    EXPECT_EQ(ret.empty(), true);

    // test case 3 cover max uris will reutrn empty map
    uris.clear();
    for (int32_t i = 0; i < URI_SIZE; i++) {
        uris.emplace_back("testuri");
    }
    ret = manager.GetUrisByOldUris(uris);
    EXPECT_EQ(ret.empty(), true);

    // invalid uris
    uris.clear();
    uris.emplace_back("you_look_only_once");
    uris.emplace_back("//media/we_shall_never_surrender/");
    uris.emplace_back("//media/we_shall_never/_surrender");
    uris.emplace_back("/storage/emulated/love_and_peace/");
    uris.emplace_back("12345");
    uris.emplace_back("");
    ret = manager.GetUrisByOldUris(uris);
    EXPECT_EQ(ret.empty(), false);
}

constexpr size_t ANALYSIS_BATCH_MAX_SIZE = 100;

using AnalysisMetadataInvalidInputQuery = std::function<std::pair<int32_t, bool>(const std::vector<std::string> &)>;
static testing::AssertionResult CheckAnalysisMetadataInvalidInputs(const AnalysisMetadataInvalidInputQuery &query);

HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_InvalidInput_001, TestSize.Level1)
{
    EXPECT_TRUE(CheckAnalysisMetadataInvalidInputs([this](const std::vector<std::string> &uris) {
        std::unordered_map<std::string, std::vector<std::string>> result;
        result.emplace("stale", std::vector<std::string> {"stale"});
        const int32_t ret = manager.GetAssetsLabel(uris, result);
        return std::make_pair(ret, result.empty());
    }));
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_RequestBatch_001, TestSize.Level1)
{
    const std::string uriA = "file://media/Photo/2000001001";
    const std::string uriB = "file://media/Photo/2000001002";
    const std::string uriC = "file://media/Photo/2000001003";
    MediaLibraryManager::AssetsAnalysisMetadataQuery query;
    ASSERT_TRUE(manager.BuildAssetsAnalysisMetadataQuery({uriA, uriB, uriC, uriA}, query));
    const std::vector<int32_t> expectedFileIds = {2000001001, 2000001002, 2000001003};
    ASSERT_EQ(query.fileIds, expectedFileIds);
    ASSERT_EQ(query.urisByFileId.size(), 3U);
    ASSERT_EQ(query.urisByFileId.count(2000001001), 1U);
    ASSERT_EQ(query.urisByFileId.count(2000001002), 1U);
    ASSERT_EQ(query.urisByFileId.count(2000001003), 1U);
    EXPECT_EQ(query.urisByFileId.at(2000001001), std::vector<std::string> {uriA});
    EXPECT_EQ(query.urisByFileId.at(2000001002), std::vector<std::string> {uriB});
    EXPECT_EQ(query.urisByFileId.at(2000001003), std::vector<std::string> {uriC});
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_UriVariants_001, TestSize.Level1)
{
    const string uri = "file://media/Photo/2000001001";
    MediaLibraryManager::AssetsAnalysisMetadataQuery duplicated;
    ASSERT_TRUE(manager.BuildAssetsAnalysisMetadataQuery({uri, uri}, duplicated));
    EXPECT_EQ(duplicated.fileIds, vector<int32_t> {2000001001});
    ASSERT_EQ(duplicated.urisByFileId.count(2000001001), 1U);
    EXPECT_EQ(duplicated.urisByFileId.at(2000001001), vector<string> {uri});
    // A-21 / A-22: normalization must not alias a different spelling to an asset ID.
    for (const string &invalidUri : {"file://media/Photo/02000001001", "file://media/Photo/%32" "000001001",
        "file://media/Photo/ 2000001001", "file://media/Photo/2000001001 "}) {
        MediaLibraryManager::AssetsAnalysisMetadataQuery query;
        EXPECT_FALSE(manager.BuildAssetsAnalysisMetadataQuery({invalidUri}, query));
        unordered_map<string, vector<string>> labels;
        labels.emplace("stale", vector<string> {"stale"});
        EXPECT_EQ(manager.GetAssetsLabel({invalidUri}, labels), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(labels.empty());
        unordered_map<string, string> captions;
        captions.emplace("stale", "stale");
        EXPECT_EQ(manager.GetAssetsCaption({invalidUri}, captions), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(captions.empty());
    }
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_BatchLimit_001, TestSize.Level1)
{
    std::vector<std::string> uris;
    for (size_t i = 0; i < ANALYSIS_BATCH_MAX_SIZE; ++i) {
        uris.emplace_back("file://media/Photo/" + std::to_string(2000000000 + i));
    }
    // A valid 100-item request is built locally; query success requires a permitted Native caller.
    MediaLibraryManager::AssetsAnalysisMetadataQuery query;
    ASSERT_TRUE(manager.BuildAssetsAnalysisMetadataQuery(uris, query));
    EXPECT_EQ(query.fileIds.size(), ANALYSIS_BATCH_MAX_SIZE);
    EXPECT_EQ(query.urisByFileId.size(), ANALYSIS_BATCH_MAX_SIZE);

    std::unordered_map<std::string, std::vector<std::string>> result;
    result.emplace("stale", std::vector<std::string> {"stale"});
    uris.emplace_back("file://media/Photo/" + std::to_string(2000000000 + ANALYSIS_BATCH_MAX_SIZE));
    EXPECT_EQ(manager.GetAssetsLabel(uris, result), E_INVALID_ARGUMENTS);
    EXPECT_TRUE(result.empty());

    // The limit applies to input count before URI deduplication.
    result.emplace("stale", std::vector<std::string> {"stale"});
    uris.assign(ANALYSIS_BATCH_MAX_SIZE + 1, "file://media/Photo/2000000000");
    EXPECT_EQ(manager.GetAssetsLabel(uris, result), E_INVALID_ARGUMENTS);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsCaption_InvalidInput_001, TestSize.Level1)
{
    EXPECT_TRUE(CheckAnalysisMetadataInvalidInputs([this](const std::vector<std::string> &uris) {
        std::unordered_map<std::string, std::string> result;
        result.emplace("stale", "stale");
        const int32_t ret = manager.GetAssetsCaption(uris, result);
        return std::make_pair(ret, result.empty());
    }));
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsCaption_NonNativeCallerDenied_001, TestSize.Level1)
{
    ASSERT_EQ(AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID())),
        ATokenTypeEnum::TOKEN_HAP);
    const std::vector<std::string> uris = {"file://media/Photo/2000001001", "file://media/Photo/2000001002",
        "file://media/Photo/2000001003", "file://media/Photo/2000001001"};
    std::unordered_map<std::string, std::string> result;
    result.emplace("stale", "stale");
    // UID 3553 only passes the local UID check; the HAP token still fails SYSTEMINNERAPI_PERM after IPC.
    const int32_t expectedRet = getuid() == 3553 ? -E_CHECK_SYSTEMAPP_FAIL : E_PERMISSION_DENIED;
    EXPECT_EQ(manager.GetAssetsCaption(uris, result), expectedRet);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsCaption_BatchLimit_001, TestSize.Level1)
{
    std::vector<std::string> uris;
    for (size_t i = 0; i < ANALYSIS_BATCH_MAX_SIZE; ++i) {
        uris.emplace_back("file://media/Photo/" + std::to_string(2000000000 + i));
    }
    MediaLibraryManager::AssetsAnalysisMetadataQuery query;
    ASSERT_TRUE(manager.BuildAssetsAnalysisMetadataQuery(uris, query));
    EXPECT_EQ(query.fileIds.size(), ANALYSIS_BATCH_MAX_SIZE);

    std::unordered_map<std::string, std::string> result;
    result.emplace("stale", "stale");
    uris.emplace_back("file://media/Photo/" + std::to_string(2000000000 + ANALYSIS_BATCH_MAX_SIZE));
    EXPECT_EQ(manager.GetAssetsCaption(uris, result), E_INVALID_ARGUMENTS);
    EXPECT_TRUE(result.empty());

    result.emplace("stale", "stale");
    uris.assign(ANALYSIS_BATCH_MAX_SIZE + 1, "file://media/Photo/2000000000");
    EXPECT_EQ(manager.GetAssetsCaption(uris, result), E_INVALID_ARGUMENTS);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_NonNativeCallerDenied_001, TestSize.Level1)
{
    ASSERT_EQ(AccessTokenKit::GetTokenTypeFlag(static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID())),
        ATokenTypeEnum::TOKEN_HAP);
    std::unordered_map<std::string, std::vector<std::string>> result;
    result.emplace("stale", std::vector<std::string> {"stale"});
    // READ_IMAGEVIDEO and a valid URI do not turn the fixture's HAP token into a Native token.
    EXPECT_EQ(manager.GetAssetsLabel({"file://media/Photo/2000001001"}, result), -E_CHECK_SYSTEMAPP_FAIL);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(MediaLibraryManagerTest, GetMovingPhotoDateModified_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetMovingPhotoDateModified_002 enter");
    int64_t dateModified = mediaLibraryManager->GetMovingPhotoDateModified("");
    EXPECT_EQ(dateModified, E_ERR);

    dateModified = mediaLibraryManager->GetMovingPhotoDateModified("file://media/image/1/test/test.jpg");
    EXPECT_EQ(dateModified, E_ERR);

    dateModified = mediaLibraryManager->GetMovingPhotoDateModified("file://media/Photo/4096/IMG_2024_001/test.jpg");
    EXPECT_EQ(dateModified, E_ERR);
    MEDIA_INFO_LOG("GetMovingPhotoDateModified_002 exit");
}

HWTEST_F(MediaLibraryManagerTest, GetUriFromFilePath_001, TestSize.Level1)
{
    string filePath;
    string file = "/path/to/file";
    Uri fileUri(file);
    string userId;
    auto result = manager.GetUriFromFilePath(filePath, fileUri, userId);
    EXPECT_EQ(result, E_INVALID_PATH);
}

HWTEST_F(MediaLibraryManagerTest, GetUriFromFilePath_002, TestSize.Level1)
{
    string filePath = PRE_PATH_VALUES;
    string file = "/path/to/file";
    Uri fileUri(file);
    string userId;
    auto result = manager.GetUriFromFilePath(filePath, fileUri, userId);
    EXPECT_EQ(result, E_CHECK_ROOT_DIR_FAIL);
}

HWTEST_F(MediaLibraryManagerTest, GetSandboxPath_001, TestSize.Level1)
{
    std::string path = "/storage/cloud/";
    Size size;
    bool isAstc = true;
    auto result = manager.GetSandboxPath(path, size, isAstc);
    EXPECT_EQ(result, "");
}

HWTEST_F(MediaLibraryManagerTest, GetSandboxPath_002, TestSize.Level1)
{
    std::string path = ROOT_MEDIA_DIR;
    Size size;
    size.width = DEFAULT_ORIGINAL;
    size.height = DEFAULT_ORIGINAL;
    bool isAstc = false;
    auto result = manager.GetSandboxPath(path, size, isAstc);
    EXPECT_EQ(result, ROOT_SANDBOX_DIR + ".thumbs/" + "/LCD.jpg");
}

HWTEST_F(MediaLibraryManagerTest, GetSandboxPath_003, TestSize.Level1)
{
    std::string path = ROOT_MEDIA_DIR;
    Size size;
    size.width = 256;
    size.height = 768;
    bool isAstc = false;
    auto result = manager.GetSandboxPath(path, size, isAstc);
    EXPECT_NE(result, ROOT_SANDBOX_DIR + ".thumbs/" + "/LCD.jpg");
}

HWTEST_F(MediaLibraryManagerTest, GetSandboxPath_004, TestSize.Level1)
{
    std::string path = ROOT_MEDIA_DIR;
    Size size;
    size.width = 768;
    size.height = 768;
    bool isAstc = false;
    auto result = manager.GetSandboxPath(path, size, isAstc);
    EXPECT_EQ(result, ROOT_SANDBOX_DIR + ".thumbs/" + "/LCD.jpg");
}

HWTEST_F(MediaLibraryManagerTest, GetUriIdPrefix_001, TestSize.Level1)
{
    std::string fileUri = "";
    manager.GetUriIdPrefix(fileUri);
    EXPECT_EQ(fileUri, "");
}

HWTEST_F(MediaLibraryManagerTest, GetUriIdPrefix_002, TestSize.Level1)
{
    std::string fileUri = "/Photo";
    manager.GetUriIdPrefix(fileUri);
    EXPECT_EQ(fileUri, "/Photo");
}

HWTEST_F(MediaLibraryManagerTest, GetUriIdPrefix_003, TestSize.Level1)
{
    std::string fileUri = "a/b/Photo";
    manager.GetUriIdPrefix(fileUri);
    EXPECT_EQ(fileUri, "a");
}

HWTEST_F(MediaLibraryManagerTest, IfSizeEqualsRatio_001, TestSize.Level1)
{
    Size imageSize;
    imageSize.height = 0;
    Size targetSize;
    targetSize.height = 0;
    auto ret = manager.IfSizeEqualsRatio(imageSize, targetSize);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryManagerTest, IfSizeEqualsRatio_002, TestSize.Level1)
{
    Size imageSize;
    imageSize.height = 10;
    imageSize.width = 10;
    Size targetSize;
    targetSize.height = 20;
    targetSize.width = 90000;
    auto ret = manager.IfSizeEqualsRatio(imageSize, targetSize);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryManagerTest, IfSizeEqualsRatio_003, TestSize.Level1)
{
    Size imageSize;
    imageSize.height = 100;
    imageSize.width = 100;
    Size targetSize;
    targetSize.height = 300;
    targetSize.width = 300;
    auto ret = manager.IfSizeEqualsRatio(imageSize, targetSize);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryManagerTest, OpenReadOnlyAppSandboxVideo_001, TestSize.Level1)
{
    string uri;
    auto ret = manager.OpenReadOnlyAppSandboxVideo(uri);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(MediaLibraryManagerTest, GetSandboxMovingPhotoTime_001, TestSize.Level1)
{
    string uri;
    auto ret = manager.GetSandboxMovingPhotoTime(uri);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAstc_emptyUris, TestSize.Level1)
{
    string uriStr;
    auto pixelMap = mediaLibraryManager->GetAstc(Uri(uriStr));
    EXPECT_EQ(pixelMap, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, GetResultSetFromPhotos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetResultSetFromPhotos_002 enter");
    EXPECT_NE(mediaLibraryExtendManager, nullptr);
    string value = "1";
    vector<string> columns;
    EXPECT_EQ(mediaLibraryExtendManager->GetResultSetFromPhotos(value, columns), nullptr);
    MEDIA_INFO_LOG("GetResultSetFromPhotos_002 exit");
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAstcYearAndMonth_test, TestSize.Level1)
{
    vector<string> uris;
    int32_t ret = mediaLibraryManager->GetAstcYearAndMonth(uris);
    EXPECT_EQ(ret, E_ERR);

    for (int i = 0; i < 5; i++) {
        uris.push_back(CreatePhotoAsset("test.mp4"));
    }
    ret = mediaLibraryManager->GetAstcYearAndMonth(uris);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_ReadPrivateMovingPhoto_test, TestSize.Level1)
{
    EXPECT_NE(mediaLibraryExtendManager, nullptr);
    string uri = "test";
    EXPECT_EQ(mediaLibraryExtendManager->ReadPrivateMovingPhoto(uri, HideSensitiveType::ALL_DESENSITIZE), E_ERR);
    uri = "../test/test.txt";
    EXPECT_EQ(mediaLibraryExtendManager->ReadPrivateMovingPhoto(uri, HideSensitiveType::ALL_DESENSITIZE), E_ERR);
    uri = CreatePhotoAsset("test.mp4");
    EXPECT_EQ(mediaLibraryExtendManager->ReadPrivateMovingPhoto(uri, HideSensitiveType::ALL_DESENSITIZE), E_ERR);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetResultSetFromDb_test, TestSize.Level1)
{
    EXPECT_NE(mediaLibraryExtendManager, nullptr);
    string columnName = "file_id";
    string value = "1";
    vector<string> columns;
    EXPECT_EQ(mediaLibraryExtendManager->GetResultSetFromDb(columnName, value, columns), nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_InitMediaLibraryManager_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret = 0;
    string uri = "file://media-test_value";
    string openMode = "test_value";
    mediaLibraryManager->InitMediaLibraryManager();
    ret = mediaLibraryManager->OpenAsset(uri, openMode);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAstcYearAndMonth_test_002, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    std::vector<string> uris;
    int32_t ret;
    for (int i = 0; i < 5; i++) {
        uris.push_back(CreatePhotoAsset("test.mp4"));
    }
    ret = mediaLibraryManager->GetAstcYearAndMonth(uris);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_QueryTotalSize_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret;
    MediaVolume outMediaVolume;
    ret = mediaLibraryManager->QueryTotalSize(outMediaVolume);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetResultSetFromDb_test_002, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    std::string columnName = CONST_MEDIA_DATA_DB_URI;
    std::string value = "test";
    std::vector<string> columns;
    std::shared_ptr<DataShare::DataShareResultSet> res =
        mediaLibraryManager->GetResultSetFromDb(columnName, value, columns);
    EXPECT_EQ(res, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetResultSetFromDb_test_003, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    std::shared_ptr<DataShareResultSet> ptr = nullptr;
    std::string columnName = "file_id_test";
    std::string value = "test";
    std::vector<string> columns;
    ptr = mediaLibraryManager->GetResultSetFromDb(columnName, value, columns);
    EXPECT_EQ(ptr, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CheckResultSet_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    std::shared_ptr<DataShareResultSet> resultSet = nullptr;
    int32_t ret;
    ret = mediaLibraryManager->CheckResultSet(resultSet);
    EXPECT_NE(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CheckResultSet_test_002, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    vector<string> columns;
    DataSharePredicates predicates;
    string prefix = string(CONST_MEDIA_DATA_DB_MEDIA_TYPE) + " <> " + to_string(g_albumMediaType);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    shared_ptr<DataShareResultSet> resultSet = nullptr;
    ASSERT_NE(sDataShareHelper_, nullptr);
    resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t ret;
    ret = mediaLibraryManager->CheckResultSet(resultSet);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetFilePathFromUri_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    string file = "/path/to/file";
    Uri fileUri(file);
    string filePath = "/path/to/testfile";
    string userId = "100";
    int32_t ret;
    ret = mediaLibraryManager->GetFilePathFromUri(fileUri, filePath, userId);
    EXPECT_NE(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetFilePathFromUri_test_002, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    string file = "/path/to/file";
    Uri fileUri(file);
    string filePath = "/path/to/testfile";
    string userId = "100";
    int32_t ret;
#define MEDIALIBRARY_COMPATIBILITY
    ret = mediaLibraryManager->GetFilePathFromUri(fileUri, filePath, userId);
#undef MEDIALIBRARY_COMPATIBILITY
    EXPECT_NE(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_OpenThumbnail_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    string uristr = CONST_URI_QUERY_PHOTO;
    string filePath = "/path/to/testfile";
    Size size;
    bool isAstc = true;
    int ret;
    ret = MediaLibraryManager::OpenThumbnail(uristr, filePath, size, isAstc);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DecodeThumbnail_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    UniqueFd uniqueFd;
    Size size;
    DecodeDynamicRange dynamicRange = DecodeDynamicRange::SDR;
    std::unique_ptr<PixelMap> ret = nullptr;
    ret = MediaLibraryManager::DecodeThumbnail(uniqueFd, size, dynamicRange);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_QueryThumbnail_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    UriParams params;
    params.path = "/path/to/testfile";
    params.fileUri = "/Photo";
    params.isAstc = true;
    std::unique_ptr<PixelMap> ret = nullptr;
    ret = MediaLibraryManager::QueryThumbnail(params);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetThumbnail_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    string file = "/path/to/file";
    Uri fileUri(file);
    std::unique_ptr<PixelMap> ret = nullptr;
    ret = mediaLibraryManager->GetThumbnail(fileUri);
    EXPECT_EQ(ret, nullptr);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetBatchAstcs_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    vector<string> uriBatch;
    vector<vector<uint8_t>> astcBatch;
    string beginUri =
        "file://media/Photo/64/IMG_063/IMG_11311.jpg?oper=astc&width=256&height=256&time_id=00000001";
    uriBatch.push_back(beginUri);
    uriBatch.push_back("/media/ml/uri/offset/1");
    uriBatch.push_back("&offset=");
    int32_t ret = mediaLibraryManager->GetBatchAstcs(uriBatch, astcBatch);
    EXPECT_EQ(ret, E_INVALID_URI);
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DecodeAstc_test, TestSize.Level1)
{
    ASSERT_NE(mediaLibraryManager, nullptr);
    UniqueFd uniqueFd;
    unique_ptr<PixelMap> ret = nullptr;
    ret = MediaLibraryManager::DecodeAstc(uniqueFd);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbumLpath_test_001
 * @tc.name      : Get lpath by albumId
 * @tc.desc      : Get lpath by albumId success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbumLpath_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpath_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    uint32_t ownerAlbumId = 1;
    std::string lpath = "";
    int32_t ret = mediaLibraryManager->GetAlbumLpath(ownerAlbumId, lpath);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpath_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbumLpaths_test_001
 * @tc.name      : Get lpaths by albumType
 * @tc.desc      : Get lpaths by albumType failed
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbumLpaths_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    uint32_t albumType = 1024;
    std::shared_ptr<DataShare::ResultSet> resultSet = nullptr;
    int32_t ret = mediaLibraryManager->GetAlbumLpaths(albumType, resultSet);
    EXPECT_EQ(ret, E_FAIL);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbumLpaths_test_002
 * @tc.name      : Get lpaths by albumType
 * @tc.desc      : Get lpaths by albumType success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbumLpaths_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_002 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    uint32_t albumType = 0;
    std::shared_ptr<DataShare::ResultSet> resultSet = nullptr;
    int32_t ret = mediaLibraryManager->GetAlbumLpaths(albumType, resultSet);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbumLpaths_test_003
 * @tc.name      : Get lpaths by albumType
 * @tc.desc      : Get lpaths by albumType success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbumLpaths_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_003 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    uint32_t albumType = 2048;
    std::shared_ptr<DataShare::ResultSet> resultSet = nullptr;
    int32_t ret = mediaLibraryManager->GetAlbumLpaths(albumType, resultSet);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbumLpaths_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_RetainCloudMediaAsset_test_001
 * @tc.name      : RetainCloudMediaAsset: HDC_RETAIN_FORCE
 * @tc.desc      : RetainCloudMediaAsset failed
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_RetainCloudMediaAsset_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_RetainCloudMediaAsset_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    CloudMediaRetainType retainType = CloudMediaRetainType::HDC_RETAIN_FORCE;
    int32_t ret = mediaLibraryManager->RetainCloudMediaAsset(retainType);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("MediaLibraryManager_RetainCloudMediaAsset_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbums_test_001
 * @tc.name      : Get albums
 * @tc.desc      : Get albums success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbums_test_001 enter");
    DataSharePredicates predicates;
    vector<string> columns;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columns, &predicates);
    int32_t albumCountSrc = albumsFetchResult.GetCount();
    string albumNameTest = "testAlbum";
    int32_t ret = mediaLibraryManager->CreateAlbum(albumNameTest);
    EXPECT_GE(ret, 0);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columns, &predicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), albumCountSrc + 1);
    unique_ptr<PhotoAlbum> lastAlbumPtr = albumsFetchResult.GetLastObject();
    ASSERT_NE(lastAlbumPtr, nullptr);
    vector<unique_ptr<PhotoAlbum>> albumsVector;
    albumsVector.push_back(move(lastAlbumPtr));
    ret = mediaLibraryManager->DeleteAlbums(albumsVector);
    EXPECT_GE(ret, 0);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columns, &predicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), albumCountSrc);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbums_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAlbums_test_002
 * @tc.name      : Get albums
 * @tc.desc      : Get albums by album name success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAlbums_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbums_test_002 enter");
    string albumNameTest = "testAlbum";
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret = mediaLibraryManager->CreateAlbum(albumNameTest);
    EXPECT_GE(ret, 0);
    DataSharePredicates predicates;
    vector<string> columns;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumNameTest);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columns, &predicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), 1);
    auto albumPtr = albumsFetchResult.GetFirstObject();
    ASSERT_NE(albumPtr, nullptr);
    DataSharePredicates predicatesErr;
    predicatesErr.EqualTo(PhotoAlbumColumns::ALBUM_NAME, "errAlbumName");
    albumsFetchResult = mediaLibraryManager->GetAlbums(columns, &predicatesErr);
    EXPECT_EQ(albumsFetchResult.GetCount(), 0);
    vector<unique_ptr<PhotoAlbum>> albumsVector;
    albumsVector.push_back(move(albumPtr));
    ret = mediaLibraryManager->DeleteAlbums(albumsVector);
    EXPECT_GE(ret, albumsVector.size());
    albumsFetchResult = mediaLibraryManager->GetAlbums(columns, nullptr);
    EXPECT_EQ(albumsFetchResult.GetCount(), 0);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAlbums_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_ReadAssets_test_001
 * @tc.name      : Read assets
 * @tc.desc      : Read assets success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_ReadAssets_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_001 enter");
    string displayName = "test.jpg";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    GTEST_LOG_(INFO) << "uri is " << uri;
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);
    DataSharePredicates predicatesAlbum;
    vector<string> columnsAlbum;
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    int32_t albumCountSrc = albumsFetchResult.GetCount();
    EXPECT_GE(albumCountSrc, 0);
    unique_ptr<PhotoAlbum> albumPtr = albumsFetchResult.GetFirstObject();
    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;
    while (albumPtr != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        albumPtr = albumsFetchResult.GetNextObject();
    }
    ASSERT_NE(albumPtr, nullptr);
    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_ReadAssets_test_002
 * @tc.name      : Read assets
 * @tc.desc      : Read assets when columns is not empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_ReadAssets_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_002 enter");
    DataSharePredicates predicatesAlbum;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    int32_t albumCount = albumsFetchResult.GetCount();
    EXPECT_GE(albumCount, 0);
    unique_ptr<PhotoAlbum> albumPtr;
    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset{ MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME };
    FetchResult<FileAsset> assetsFetchResult;
    for (int i = albumCount - 1; i >= 0; i--) {
        albumPtr = albumsFetchResult.GetObjectAtPosition(i);
        ASSERT_NE(albumPtr, nullptr);
        assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
    }
    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    unique_ptr<FileAsset> firstAssetPtr = assetsFetchResult.GetFirstObject();
    ASSERT_NE(firstAssetPtr, nullptr);
    string displayNameRec = firstAssetPtr->GetDisplayName();
    GTEST_LOG_(INFO) << "firstAssetPtr's displayName is " << displayNameRec;
    int32_t mediaId = firstAssetPtr->GetId();
    GTEST_LOG_(INFO) << "firstAssetPtr's mediaId is " << mediaId;
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_ReadAssets_test_003
 * @tc.name      : Read assets
 * @tc.desc      : Read assets when columns and predicates are not empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_ReadAssets_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_003 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCount = albumsFetchResult.GetCount();
    EXPECT_GT(albumCount, 0);
    unique_ptr<PhotoAlbum> srcAlbum;
    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset{ MediaColumn::MEDIA_NAME };
    DataSharePredicates predicatesAsset;
    predicatesAsset.IsNotNull(MediaColumn::MEDIA_NAME);
    for (int32_t i = albumCount - 1; i >= 0; i--) {
        srcAlbum = albumsFetchResult.GetObjectAtPosition(i);
        assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
    }
    unique_ptr<FileAsset> firstAssetPtr = assetsFetchResult.GetFirstObject();
    ASSERT_NE(firstAssetPtr, nullptr);
    string assetDisplayName = firstAssetPtr->GetDisplayName();
    GTEST_LOG_(INFO) << "firstAssetPtr's displayName is " << assetDisplayName;
    string assetUri = GetFileAssetUri(firstAssetPtr);
    GTEST_LOG_(INFO) << "assetUri is " << assetUri;
    MEDIA_INFO_LOG("MediaLibraryManager_ReadAssets_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_CreateAlbum_test_001
 * @tc.name      : Create album
 * @tc.desc      : Create album success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CreateAlbum_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_001 enter");
    DataSharePredicates predicatesAlbum;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    int32_t albumCountBefore = albumsFetchResult.GetCount();
    string albumName = "testAlbum";
    unique_ptr<PhotoAlbum> album = make_unique<PhotoAlbum>();
    for (int i = 0; i < albumCountBefore; i++) {
        album = albumsFetchResult.GetObjectAtPosition(i);
        if (album->GetAlbumName() == albumName) {
            FAIL() << "albumName is " << albumName << ", it already exists";
        }
    }
    int32_t ret = mediaLibraryManager->CreateAlbum(albumName);
    ASSERT_GT(ret, 0);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    int32_t albumCount = albumsFetchResult.GetCount();
    ASSERT_EQ(albumCountBefore + 1, albumCount);
    for (int i = 0; i < albumCount; i++) {
        album = albumsFetchResult.GetObjectAtPosition(i);
        if (album->GetAlbumName() == albumName) {
            break;
        }
    }
    EXPECT_EQ(ret, album->GetAlbumId());
    vector<unique_ptr<PhotoAlbum>> albumsVector;
    albumsVector.push_back(move(album));
    ret = mediaLibraryManager->DeleteAlbums(albumsVector);
    EXPECT_GE(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_CreateAlbum_test_002
 * @tc.name      : Create album
 * @tc.desc      : Create album fail when albumName is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CreateAlbum_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_002 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCountBefore = albumsFetchResult.GetCount();
    string albumName = "test*Album?";
    unique_ptr<PhotoAlbum> album = make_unique<PhotoAlbum>();
    for (int i = 0; i < albumCountBefore; i++) {
        album = albumsFetchResult.GetObjectAtPosition(i);
        if (album->GetAlbumName() == albumName) {
            FAIL() << "albumName is " << albumName << ", it already exists";
        }
    }
    int32_t errCode = mediaLibraryManager->CreateAlbum(albumName);
    ASSERT_EQ(errCode, E_FAIL);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    ASSERT_EQ(albumsFetchResult.GetCount(), albumCountBefore);
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_001
 * @tc.name      : Delete albums
 * @tc.desc      : Delete album fail when album is system album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_001 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    vector<unique_ptr<PhotoAlbum>> albums;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCountBefore = albumsFetchResult.GetCount();
    EXPECT_GE(albumCountBefore, 0);
    unique_ptr<PhotoAlbum> album = albumsFetchResult.GetFirstObject();
    while (album != nullptr) {
        if (album->IsSystemAlbum(album->GetPhotoAlbumType())) {
            albums.push_back(move(album));
            break;
        }
        album = albumsFetchResult.GetNextObject();
    }
    EXPECT_GE(albums.size(), 1);
    int32_t ret = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(ret, E_FAIL);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    ASSERT_EQ(albumsFetchResult.GetCount(), albumCountBefore);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_002
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_002 enter");
    vector<string> albumNames = {"testAlbum1", "testAlbum2", "testAlbum3"};
    ASSERT_NE(mediaLibraryManager, nullptr);
    vector<string> columnsAlbum;
    DataSharePredicates predicates;
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    vector<unique_ptr<PhotoAlbum>> albums;
    int32_t albumCount = albumsFetchResult.GetCount();
    for (auto albumName : albumNames) {
        int id = mediaLibraryManager->CreateAlbum(albumName);
        EXPECT_GT(id, 0);
        DataSharePredicates tempPredicates;
        tempPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, id);
        FetchResult<PhotoAlbum> albumsToDelete= mediaLibraryManager->GetAlbums(columnsAlbum, &tempPredicates);
        EXPECT_EQ(albumsToDelete.GetCount(), 1);
        auto album = albumsToDelete.GetFirstObject();
        ASSERT_NE(album, nullptr);
        albums.push_back(move(album));
    }
    DataSharePredicates emptyPredicates;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), albumCount + albums.size());
    int32_t ret = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(ret, albums.size());
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &emptyPredicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), albumCount);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_003
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums fail with unexistent album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_003 enter");
    vector<string> albumNames = {"testAlbum1", "testAlbum2", "testAlbum3"};
    ASSERT_NE(mediaLibraryManager, nullptr);
    vector<string> columnsAlbum;
    DataSharePredicates predicates, emptyPredicates;
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    vector<unique_ptr<PhotoAlbum>> albums;
    int32_t albumCount = albumsFetchResult.GetCount();
    for (auto albumName : albumNames) {
        int id = mediaLibraryManager->CreateAlbum(albumName);
        EXPECT_GT(id, 0);
        DataSharePredicates tempPredicates;
        tempPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, id);
        FetchResult<PhotoAlbum> albumsToDelete= mediaLibraryManager->GetAlbums(columnsAlbum, &tempPredicates);
        EXPECT_EQ(albumsToDelete.GetCount(), 1);
        auto album = albumsToDelete.GetFirstObject();
        ASSERT_NE(album, nullptr);
        albums.push_back(move(album));
    }
    int32_t albumInvalidId = 100;
    while (albumInvalidId++) {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumInvalidId);
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        if (albumsFetchResult.GetCount() == 0) {
            break;
        }
    }
    unique_ptr<PhotoAlbum> albumInvalid = make_unique<PhotoAlbum>();
    albumInvalid->SetAlbumId(albumInvalidId);
    albums.push_back(move(albumInvalid));
    int32_t ret = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(ret, E_INVALID_URI);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &emptyPredicates);
    EXPECT_EQ(albumsFetchResult.GetCount(), albumCount + albums.size() - 1);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_004
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums fail with system album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_004 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    vector<unique_ptr<PhotoAlbum>> albums;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCountBefore = albumsFetchResult.GetCount();
    EXPECT_GE(albumCountBefore, 0);
    unique_ptr<PhotoAlbum> album = albumsFetchResult.GetFirstObject();
    while (album != nullptr) {
        if (album->IsSystemAlbum(album->GetPhotoAlbumType())) {
            albums.push_back(move(album));
            break;
        }
        album = albumsFetchResult.GetNextObject();
    }
    EXPECT_GE(albums.size(), 1);
    int32_t albumId = mediaLibraryManager->CreateAlbum("testAlbum4");
    EXPECT_GT(albumId, 0);
    DataSharePredicates albumsPredicates;
    albumsPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    auto albumPtr = mediaLibraryManager->GetAlbums(columnsAlbum, &albumsPredicates);
    EXPECT_EQ(albumPtr.GetCount(), 1);
    auto normalAlbum = albumPtr.GetFirstObject();
    ASSERT_NE(normalAlbum, nullptr);
    albums.push_back(move(normalAlbum));
    int32_t ret = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(ret, E_FAIL);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    ASSERT_EQ(albumsFetchResult.GetCount(), albumCountBefore + 1);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_004 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_001
 * @tc.name      : Get assets
 * @tc.desc      : Get assets fail, predicate is nullptr
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_001 enter");
    DataSharePredicates predicatesAlbum;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    EXPECT_GE(albumsFetchResult.GetCount(), 0);
    unique_ptr<PhotoAlbum> albumPtr = albumsFetchResult.GetFirstObject();
    ASSERT_NE(albumPtr, nullptr);
    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;
    while (albumPtr != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        albumPtr = albumsFetchResult.GetNextObject();
    }
    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, nullptr);
    EXPECT_EQ(assetsFetchResult.GetCount(), 0);
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_002
 * @tc.name      : Get assets
 * @tc.desc      : Get assets fail, fetchColumns is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_002 enter");
    DataSharePredicates predicatesAlbum;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicatesAlbum);
    EXPECT_GE(albumsFetchResult.GetCount(), 0);
    unique_ptr<PhotoAlbum> albumPtr = albumsFetchResult.GetFirstObject();
    ASSERT_NE(albumPtr, nullptr);
    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;
    while (albumPtr != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        albumPtr = albumsFetchResult.GetNextObject();
    }
    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    columnsAsset.push_back("xxx");
    assetsFetchResult = mediaLibraryManager->GetAssets(*albumPtr, columnsAsset, &predicatesAsset);
    EXPECT_EQ(assetsFetchResult.GetCount(), 0);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_003
 * @tc.name      : Get assets
 * @tc.desc      : Get assets success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_003 enter");
    string displayName = "test.jpg";
    string uri = mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    GTEST_LOG_(INFO) << "uri is " << uri;
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;

    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);

    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_004
 * @tc.name      : Get assets
 * @tc.desc      : Get assets when columns is not empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_004 enter");

    ASSERT_NE(mediaLibraryManager, nullptr);

    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset{ MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME };
    FetchResult<FileAsset> assetsFetchResult;

    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);

    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    unique_ptr<FileAsset> firstAssetPtr = assetsFetchResult.GetFirstObject();
    ASSERT_NE(firstAssetPtr, nullptr);
    string displayNameRec = firstAssetPtr->GetDisplayName();
    GTEST_LOG_(INFO) << "firstAssetPtr's displayName is " << displayNameRec;
    int32_t mediaId = firstAssetPtr->GetId();
    GTEST_LOG_(INFO) << "firstAssetPtr's mediaId is " << mediaId;
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_004 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_005
 * @tc.name      : Get assets
 * @tc.desc      : Get assets when columns and predicates are not empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_005 enter");
    string displayName = "test3.jpg";
    string uri = mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    GTEST_LOG_(INFO) << "uri is " << uri;
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset{ MediaColumn::MEDIA_NAME };
    DataSharePredicates predicatesAsset;
    predicatesAsset.EqualTo(MediaColumn::MEDIA_NAME, "test3.jpg");

    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);

    unique_ptr<FileAsset> firstAssetPtr = assetsFetchResult.GetFirstObject();
    ASSERT_NE(firstAssetPtr, nullptr);
    string assetDisplayName = firstAssetPtr->GetDisplayName();
    GTEST_LOG_(INFO) << "firstAssetPtr's displayName is " << assetDisplayName;
    EXPECT_EQ(assetDisplayName, displayName);
    string assetUri = GetFileAssetUri(firstAssetPtr);
    GTEST_LOG_(INFO) << "assetUri is " << assetUri;
    int32_t assetFd = mediaLibraryManager->OpenAsset(assetUri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(assetFd, 0);
    mediaLibraryManager->CloseAsset(assetUri, assetFd);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_005 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_006
 * @tc.name      : Get assets
 * @tc.desc      : Get assets fail, fetchColumns is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_006 enter");

    ASSERT_NE(mediaLibraryManager, nullptr);

    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;

    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);

    EXPECT_GT(assetsFetchResult.GetCount(), 0);

    columnsAsset.push_back("xxx");
    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);
    EXPECT_EQ(assetsFetchResult.GetCount(), 0);
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_006 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetAssets_test_007
 * @tc.name      : Get assets
 * @tc.desc      : Get assets fail, predicate is nullptr
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetAssets_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_007 enter");

    ASSERT_NE(mediaLibraryManager, nullptr);

    DataSharePredicates predicatesAsset;
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;

    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, &predicatesAsset);

    EXPECT_GT(assetsFetchResult.GetCount(), 0);
    assetsFetchResult = mediaLibraryManager->GetAssets(columnsAsset, nullptr);
    EXPECT_EQ(assetsFetchResult.GetCount(), 0);

    MEDIA_INFO_LOG("MediaLibraryManager_GetAssets_test_007 exit");
}

void TestMoveAssets()
{
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    EXPECT_GT(albumsFetchResult.GetCount(), 0);
    unique_ptr<PhotoAlbum> srcAlbum = albumsFetchResult.GetFirstObject();
    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset;
    DataSharePredicates predicatesAsset;
    predicatesAsset.EqualTo(MediaColumn::MEDIA_NAME, "testForMove.jpg");
    while (srcAlbum != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        srcAlbum = albumsFetchResult.GetNextObject();
    }
    ASSERT_NE(srcAlbum, nullptr);
    vector<unique_ptr<FileAsset>> assets;
    for (int i = 0; i < assetsFetchResult.GetCount(); i++) {
        auto asset = assetsFetchResult.GetObjectAtPosition(i);
        assets.push_back(move(asset));
    }
    EXPECT_EQ(assets.size(), assetsFetchResult.GetCount());
    string tagetAlbumName = "testAlbumForMove1";
    bool isTargetAlbumExist = false;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    unique_ptr<PhotoAlbum> targetAlbum = albumsFetchResult.GetFirstObject();
    while (targetAlbum != nullptr) {
        if (targetAlbum->GetAlbumName() == tagetAlbumName) {
            isTargetAlbumExist = true;
            break;
        }
        targetAlbum = albumsFetchResult.GetNextObject();
    }
    EXPECT_TRUE(isTargetAlbumExist);
    int32_t targetAlbumAssetCount = targetAlbum->GetCount();
    int32_t ret = mediaLibraryManager->MoveAssets(assets, *srcAlbum, *targetAlbum);
    EXPECT_GE(ret, 0);
    EXPECT_EQ(targetAlbum->GetCount(), targetAlbumAssetCount + assets.size());
}

void TestMoveAssetsFromUserAlbumToUserAlbum()
{
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    EXPECT_GT(albumsFetchResult.GetCount(), 0);
    string srcAlbumName = "testAlbumForMove1";
    bool isSrcAlbumExist = false;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    unique_ptr<PhotoAlbum> srcAlbum = albumsFetchResult.GetFirstObject();
    while (srcAlbum != nullptr) {
        if (srcAlbum->GetAlbumName() == srcAlbumName) {
            isSrcAlbumExist = true;
            break;
        }
        srcAlbum = albumsFetchResult.GetNextObject();
    }
    EXPECT_TRUE(isSrcAlbumExist);
    int32_t srcAlbumAssetCount = srcAlbum->GetCount();
    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset;
    DataSharePredicates predicatesAsset;
    predicatesAsset.EqualTo(MediaColumn::MEDIA_NAME, "testForMove.jpg");
    assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
    ASSERT_NE(srcAlbum, nullptr);
    vector<unique_ptr<FileAsset>> assets;
    for (int i = 0; i < assetsFetchResult.GetCount(); i++) {
        auto asset = assetsFetchResult.GetObjectAtPosition(i);
        assets.push_back(move(asset));
    }
    EXPECT_EQ(assets.size(), assetsFetchResult.GetCount());
    string targetAlbumName = "testAlbumForMove2";
    bool isTargetAlbumExist = false;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    unique_ptr<PhotoAlbum> targetAlbum = albumsFetchResult.GetFirstObject();
    while (targetAlbum != nullptr) {
        if (targetAlbum->GetAlbumName() == targetAlbumName) {
            isTargetAlbumExist = true;
            break;
        }
        targetAlbum = albumsFetchResult.GetNextObject();
    }
    EXPECT_TRUE(isTargetAlbumExist);
    int32_t targetAlbumAssetCount = targetAlbum->GetCount();
    int32_t ret = mediaLibraryManager->MoveAssets(assets, *srcAlbum, *targetAlbum);
    EXPECT_GE(ret, 0);
    EXPECT_EQ(srcAlbum->GetCount(), srcAlbumAssetCount - assets.size());
    EXPECT_EQ(targetAlbum->GetCount(), targetAlbumAssetCount + assets.size());
}

/**
 * @tc.number    : MediaLibraryManager_MoveAssets_test_001
 * @tc.name      : Move assets to other album
 * @tc.desc      : Move assets to other album success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_MoveAssets_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_001 enter");
    string albumNameTest = "testAlbumForMove1";
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret = mediaLibraryManager->CreateAlbum(albumNameTest);
    EXPECT_GE(ret, 0);
    string albumNameTest2 = "testAlbumForMove2";
    ret = mediaLibraryManager->CreateAlbum(albumNameTest2);
    EXPECT_GE(ret, 0);
    string displayName = "testForMove.jpg";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    GTEST_LOG_(INFO) << "uri is " << uri;
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);
    TestMoveAssets();
    TestMoveAssetsFromUserAlbumToUserAlbum();
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_MoveAssets_test_002
 * @tc.name      : Move assets to other album
 * @tc.desc      : Move assets to other album fail, assets is empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_MoveAssets_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_002 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCount = albumsFetchResult.GetCount();
    EXPECT_GT(albumsFetchResult.GetCount(), 0);
    unique_ptr<PhotoAlbum> srcAlbum = albumsFetchResult.GetFirstObject();
    vector<string> columnsAsset;
    FetchResult<FileAsset> assetsFetchResult;
    while (srcAlbum != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicates);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        srcAlbum = albumsFetchResult.GetNextObject();
    }
    ASSERT_NE(srcAlbum, nullptr);
    int32_t srcAlbumId = srcAlbum->GetAlbumId();
    EXPECT_GT(srcAlbumId, 0);
    DataSharePredicates predicatesAsset;
    vector<unique_ptr<FileAsset>> assets;
    string tagetAlbumName = "testAlbum";
    bool isTargetAlbumExist = false;
    unique_ptr<PhotoAlbum> targetAlbum = make_unique<PhotoAlbum>();
    int32_t targetAlbumId = -1;
    int32_t targetAlbumCount = 0;
    while (!isTargetAlbumExist) {
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        albumCount = albumsFetchResult.GetCount();
        for (int i = 0; i < albumCount ; i++) {
            targetAlbum = albumsFetchResult.GetObjectAtPosition(i);
            if (targetAlbum->GetAlbumName() == tagetAlbumName) {
                isTargetAlbumExist = true;
                targetAlbumCount = targetAlbum->GetCount();
                targetAlbumId = targetAlbum->GetAlbumId();
                break;
            }
        }
        if (!isTargetAlbumExist) {
            int32_t id = mediaLibraryManager->CreateAlbum(tagetAlbumName);
            EXPECT_GT(id, 0);
        }
    }
    EXPECT_GT(targetAlbumId, 0);
    int32_t ret = mediaLibraryManager->MoveAssets(assets, *srcAlbum, *targetAlbum);
    EXPECT_EQ(ret, E_FAIL);
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_MoveAssets_test_003
 * @tc.name      : Move assets to other album
 * @tc.desc      : Move assets to other album fail, targetAlbum is not user album or source album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_MoveAssets_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_003 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCount = albumsFetchResult.GetCount();
    EXPECT_GT(albumCount, 0);
    unique_ptr<PhotoAlbum> srcAlbum = albumsFetchResult.GetFirstObject();
    int32_t srcAlbumId = -1;
    int32_t srcAlbumImageCount = 0;
    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset;
    DataSharePredicates predicatesAsset;
    while (srcAlbum != nullptr) {
        assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            break;
        }
        srcAlbum = albumsFetchResult.GetNextObject();
    }
    ASSERT_NE(srcAlbum, nullptr);
    srcAlbumId = srcAlbum->GetAlbumId();
    srcAlbumImageCount = srcAlbum->GetCount();
    EXPECT_GT(srcAlbumId, 0);
    vector<unique_ptr<FileAsset>> assets;
    assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
    EXPECT_EQ(assetsFetchResult.GetCount(), srcAlbumImageCount);
    for (int i = 0; i < srcAlbumImageCount; i++) {
        auto asset = assetsFetchResult.GetObjectAtPosition(i);
        assets.push_back(move(asset));
    }
    EXPECT_EQ(assets.size(), srcAlbumImageCount);

    bool isTargetAlbumExist = false;
    unique_ptr<PhotoAlbum> targetAlbum = nullptr;
    int32_t targetAlbumId = -1;
    int32_t targetAlbumAssetCount = 0;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    albumCount = albumsFetchResult.GetCount();
    for (int i = 0; i < albumCount ; i++) {
        targetAlbum = albumsFetchResult.GetObjectAtPosition(i);
        if (!PhotoAlbum::IsUserPhotoAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType())
            && !PhotoAlbum::IsSourceAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType())) {
            isTargetAlbumExist = true;
            targetAlbumAssetCount = targetAlbum->GetCount();
            targetAlbumId = targetAlbum->GetAlbumId();
            break;
        }
    }
    EXPECT_TRUE(isTargetAlbumExist);
    EXPECT_GT(targetAlbumId, 0);
    int32_t ret = mediaLibraryManager->MoveAssets(assets, *srcAlbum, *targetAlbum);
    EXPECT_EQ(ret, E_FAIL);
    EXPECT_EQ(targetAlbumAssetCount, targetAlbum->GetCount());
    MEDIA_INFO_LOG("MediaLibraryManager_MoveAssets_test_003 exit");
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetSupportedPhotoFormats_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    auto imageFormats = mediaLibraryManager->GetSupportedPhotoFormats(PHOTOTYPE_IMAGE);
    MEDIA_INFO_LOG("image suffix list: %{public}s", JoinFormats(imageFormats).c_str());
    EXPECT_GT(imageFormats.size(), 0);
    auto expectedSorted = EXPECTED_IMAGE_FORMATS;
    auto actualSorted = imageFormats;
    std::sort(expectedSorted.begin(), expectedSorted.end());
    std::sort(actualSorted.begin(), actualSorted.end());

    EXPECT_EQ(actualSorted, expectedSorted);
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_001 end");
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetSupportedPhotoFormats_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_002 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    auto videoFormats = mediaLibraryManager->GetSupportedPhotoFormats(PHOTOTYPE_VIDEO);
    MEDIA_INFO_LOG("video suffix list: %{public}s", JoinFormats(videoFormats).c_str());
    EXPECT_GT(videoFormats.size(), 0);
    auto expectedSorted = EXPECTED_VIDEO_FORMATS;
    auto actualSorted = videoFormats;
    std::sort(expectedSorted.begin(), expectedSorted.end());
    std::sort(actualSorted.begin(), actualSorted.end());

    EXPECT_EQ(actualSorted, expectedSorted);
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_002 end");
}

HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetSupportedPhotoFormats_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_003 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    auto invalidFormats = mediaLibraryManager->GetSupportedPhotoFormats(static_cast<PhotoType>(MEDIA_TYPE_AUDIO));
    MEDIA_INFO_LOG("invalid suffix list: %{public}s", JoinFormats(invalidFormats).c_str());
    EXPECT_TRUE(invalidFormats.empty());
    MEDIA_INFO_LOG("MediaLibraryManager_GetSupportedPhotoFormats_test_003 end");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAssets_test_001
 * @tc.name      : Delete assets
 * @tc.desc      : Delete assets success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAssets_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_001 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    ASSERT_NE(mediaLibraryManager, nullptr);
    FetchResult<PhotoAlbum> albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    int32_t albumCount = albumsFetchResult.GetCount();
    EXPECT_GT(albumCount, 0);
    unique_ptr<PhotoAlbum> srcAlbum;
    int32_t srcAlbumId = -1;
    int32_t srcAlbumImageCount = 0;
    FetchResult<FileAsset> assetsFetchResult;
    vector<string> columnsAsset;
    DataSharePredicates predicatesAsset;
    predicatesAsset.EqualTo(MediaColumn::MEDIA_NAME, "test.jpg");
    for (int32_t i = albumCount - 1; i >= 0; i--) {
        srcAlbum = albumsFetchResult.GetObjectAtPosition(i);
        assetsFetchResult = mediaLibraryManager->GetAssets(*srcAlbum, columnsAsset, &predicatesAsset);
        if (assetsFetchResult.GetCount() > 0) {
            srcAlbumId = srcAlbum->GetAlbumId();
            srcAlbumImageCount = srcAlbum->GetCount();
            break;
        }
    }
    EXPECT_GT(srcAlbumId, 0);
    unique_ptr<FileAsset> firstAssetPtr = assetsFetchResult.GetFirstObject();
    ASSERT_NE(firstAssetPtr, nullptr);
    string assetPath = firstAssetPtr->GetPath();
    GTEST_LOG_(INFO) << "firstAssetPtr's path is " << assetPath;
    string assetDisplayName = firstAssetPtr->GetDisplayName();
    GTEST_LOG_(INFO) << "firstAssetPtr's displayName is " << assetDisplayName;
    vector<unique_ptr<FileAsset>> assets;
    assets.push_back(move(firstAssetPtr));
    int32_t ret =mediaLibraryManager->DeleteAssets(assets);
    EXPECT_EQ(ret, assets.size());
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, srcAlbumId);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    EXPECT_GT(albumsFetchResult.GetCount(), 0);
    srcAlbum = albumsFetchResult.GetFirstObject();
    ASSERT_NE(srcAlbum, nullptr);
    EXPECT_EQ(srcAlbum->GetCount(), srcAlbumImageCount - 1);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAssets_test_002
 * @tc.name      : Delete assets
 * @tc.desc      : Delete assets fail, assets is empty
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAssets_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_002 enter");
    vector<unique_ptr<FileAsset>> assets;
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret = mediaLibraryManager->DeleteAssets(assets);
    EXPECT_EQ(ret, E_FAIL);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAssets_test_003
 * @tc.name      : Delete assets
 * @tc.desc      : Delete assets fail, assets' uri is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAssets_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_003 enter");
    vector<unique_ptr<FileAsset>> assets;
    unique_ptr<FileAsset> asset = make_unique<FileAsset>();
    asset->SetDisplayName("errAsset");
    assets.push_back(move(asset));
    ASSERT_NE(mediaLibraryManager, nullptr);
    int32_t ret = mediaLibraryManager->DeleteAssets(assets);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAssets_test_003 exit");
}

#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_file_manager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_file_manager_test_001 enter");
    string displayName = "test_file_manager.jpg";
    string uri = CreateFileManagerAsset(displayName);
    ASSERT_NE(uri, "");
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

    int32_t srcFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    int64_t srcLen = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(srcLen));
    ASSERT_NE(buf, nullptr);
    read(srcFd, buf, srcLen);
    EXPECT_EQ(CompareIfArraysEquals(buf, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG)), true);
    free(buf);
    mediaLibraryManager->CloseAsset(uri, srcFd);
}
#endif

/**
 * @tc.number    : MediaLibraryManager_BatchUpdateMetaDataModified_test_001
 * @tc.name      : Batch update metadata modified with valid fileIds
 * @tc.desc      : Test BatchUpdateMetaDataModified with valid file ids, metadata_modified should change
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_BatchUpdateMetaDataModified_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_BatchUpdateMetaDataModified_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);
    
    string displayName = "test_batch_valid.jpg";
    string uri = CreatePhotoAsset(displayName);
    ASSERT_NE(uri, "");
    
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    ASSERT_NE(fileId, "");
    
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns;
    columns.push_back(PhotoColumn::PHOTO_META_DATE_MODIFIED);
    FetchResult<FileAsset> assetsFetchResult = mediaLibraryManager->GetAssets(columns, &predicates);
    ASSERT_GT(assetsFetchResult.GetCount(), 0);
    unique_ptr<FileAsset> fileAsset = assetsFetchResult.GetFirstObject();
    ASSERT_NE(fileAsset, nullptr);
    int64_t beforeModified = std::get<int64_t>(fileAsset->GetMemberValue(PhotoColumn::PHOTO_META_DATE_MODIFIED));
    
    std::vector<std::string> fileIds;
    fileIds.push_back(fileId);
    mediaLibraryManager->BatchUpdateMetaDataModified(fileIds);
    
    assetsFetchResult = mediaLibraryManager->GetAssets(columns, &predicates);
    ASSERT_GT(assetsFetchResult.GetCount(), 0);
    fileAsset = assetsFetchResult.GetFirstObject();
    ASSERT_NE(fileAsset, nullptr);
    int64_t afterModified = std::get<int64_t>(fileAsset->GetMemberValue(PhotoColumn::PHOTO_META_DATE_MODIFIED));
    
    EXPECT_NE(beforeModified, afterModified);
    MEDIA_INFO_LOG("MediaLibraryManager_BatchUpdateMetaDataModified_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_GetClonedAssetUris_test_001
 * @tc.name      : GetClonedAssetUris
 * @tc.desc      : empty uri list and invalid uri format return E_INVALID_URI
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_GetClonedAssetUris_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_GetClonedAssetUris_test_001 enter");
    ASSERT_NE(mediaLibraryManager, nullptr);

    std::map<std::string, std::string> uriMap;
    int32_t ret = mediaLibraryManager->GetClonedAssetUris({}, uriMap);
    EXPECT_EQ(ret, E_INVALID_URI);
    EXPECT_TRUE(uriMap.empty());
    
    std::vector<std::string> badUris = {
        "",
        "not_a_uri",
        "/storage/emulated/0/DCIM/invalid.jpg",
        "file:///storage/emulated/0/DCIM/invalid.jpg"
    };
    ret = mediaLibraryManager->GetClonedAssetUris(badUris, uriMap);
    EXPECT_EQ(ret, E_INVALID_URI);
    EXPECT_TRUE(uriMap.empty());
    MEDIA_INFO_LOG("MediaLibraryManager_GetClonedAssetUris_test_001 exit");
}

static void ExpectAnalysisMetadataReply(AnalysisMetadataMockDataShareHelper &helper,
    const vector<int32_t> &fileIds, const vector<string> &values, const vector<int32_t> &expectedRequestFileIds)
{
    GetAssetsAnalysisMetadataRespBody body;
    body.fileIds = fileIds;
    body.values = values;
    EXPECT_CALL(helper, UserDefineFunc(testing::_, testing::_, testing::_))
        .WillOnce([body, expectedRequestFileIds](MessageParcel &data, MessageParcel &reply, MessageOption &) {
            EXPECT_FALSE(data.ReadInterfaceToken().empty());
            IPC::MediaReqVo<GetAssetsAnalysisMetadataReqBody> request;
            EXPECT_TRUE(request.Unmarshalling(data));
            EXPECT_EQ(request.GetBody().fileIds, expectedRequestFileIds);
            IPC::MediaRespVo<GetAssetsAnalysisMetadataRespBody> response;
            response.SetBody(body);
            response.SetErrCode(E_OK);
            response.SetTraceId("analysis-metadata-test");
            response.SetSuccess(true);
            return response.Marshalling(reply) ? E_OK : E_FAIL;
        });
}

// F-38 / F-25 / F-26: shuffled 1/3/0 results keep their original URI keys.
HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_MixedResultsKeepUriKeys, TestSize.Level1)
{
    auto helper = make_shared<AnalysisMetadataMockDataShareHelper>();
    ScopedAnalysisMetadataHelper helperScope(helper);
    const vector<int32_t> responseFileIds = {102, 101, 102, 102};
    const vector<string> responseLabels = {"b1", "a1", "b2", "b3"};
    const vector<int32_t> requestFileIds = {103, 102, 101};
    ExpectAnalysisMetadataReply(*helper, responseFileIds, responseLabels, requestFileIds);
    const string uriA = "file://media/Photo/101";
    const string uriB = "file://media/Photo/102";
    const string uriC = "file://media/Photo/103";
    unordered_map<string, vector<string>> result;
    ASSERT_EQ(manager.GetAssetsLabel({uriC, uriB, uriA}, result), E_OK);
    ASSERT_EQ(result.size(), 3U);
    ASSERT_EQ(result.count(uriA), 1U);
    ASSERT_EQ(result.count(uriB), 1U);
    ASSERT_EQ(result.count(uriC), 1U);
    EXPECT_EQ(result.at(uriA), vector<string> {"a1"});
    const vector<string> expectedB = {"b1", "b2", "b3"};
    EXPECT_EQ(result.at(uriB), expectedB);
    EXPECT_TRUE(result.at(uriC).empty());
}

// C-04 (Label): a late unexpected ID must not expose rows already accumulated locally.
HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_UnknownFileIdReturnsNoPartialResult, TestSize.Level1)
{
    auto helper = make_shared<AnalysisMetadataMockDataShareHelper>();
    ScopedAnalysisMetadataHelper helperScope(helper);
    const vector<int32_t> responseFileIds = {101, 999};
    const vector<string> responseLabels = {"own", "unexpected"};
    const vector<int32_t> requestFileIds = {101};
    ExpectAnalysisMetadataReply(*helper, responseFileIds, responseLabels, requestFileIds);
    unordered_map<string, vector<string>> result;
    result.emplace("stale", vector<string> {"stale"});
    EXPECT_EQ(manager.GetAssetsLabel({"file://media/Photo/101"}, result), E_FAIL);
    EXPECT_TRUE(result.empty());
}

// F-02 (Label): one requested URI produces exactly one result key.
HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_SingleAssetKeepsOneKey, TestSize.Level1)
{
    auto helper = make_shared<AnalysisMetadataMockDataShareHelper>();
    ScopedAnalysisMetadataHelper helperScope(helper);
    ExpectAnalysisMetadataReply(*helper, {101}, {"single-label"}, {101});
    const string uri = "file://media/Photo/101";
    unordered_map<string, vector<string>> result;
    result.emplace("stale", vector<string> {"stale"});
    ASSERT_EQ(manager.GetAssetsLabel({uri}, result), E_OK);
    ASSERT_EQ(result.size(), 1U);
    ASSERT_EQ(result.count(uri), 1U);
    EXPECT_EQ(result.at(uri), vector<string> {"single-label"});
}

// F-03 (Label): 99 distinct URIs remain distinct result keys.
HWTEST_F(MediaLibraryManagerTest, GetAssetsLabel_NinetyNineAssetsKeepAllKeys, TestSize.Level1)
{
    auto helper = make_shared<AnalysisMetadataMockDataShareHelper>();
    ScopedAnalysisMetadataHelper helperScope(helper);
    constexpr int32_t assetCount = 99;
    vector<string> uris;
    vector<int32_t> fileIds;
    vector<string> labels;
    for (int32_t index = 0; index < assetCount; ++index) {
        const int32_t fileId = 101 + index;
        uris.emplace_back("file://media/Photo/" + to_string(fileId));
        fileIds.emplace_back(fileId);
        labels.emplace_back("label-" + to_string(fileId));
    }
    ExpectAnalysisMetadataReply(*helper, fileIds, labels, fileIds);
    unordered_map<string, vector<string>> result;
    result.emplace("stale", vector<string> {"stale"});
    ASSERT_EQ(manager.GetAssetsLabel(uris, result), E_OK);
    ASSERT_EQ(result.size(), 99U);
    for (int32_t index = 0; index < assetCount; ++index) {
        const string uri = "file://media/Photo/" + to_string(101 + index);
        ASSERT_EQ(result.count(uri), 1U) << "asset index: " << index;
        const vector<string> expected = {"label-" + to_string(101 + index)};
        EXPECT_EQ(result.at(uri), expected) << "asset index: " << index;
    }
}

// F-12 / F-17 / F-18: malformed paths are rejected before metadata transport.
HWTEST_F(MediaLibraryManagerTest, GetAssetsMetadata_InvalidUriShapesDoNotSendIpc, TestSize.Level1)
{
    auto helper = make_shared<AnalysisMetadataMockDataShareHelper>();
    ScopedAnalysisMetadataHelper helperScope(helper);
    EXPECT_CALL(*helper, UserDefineFunc(testing::_, testing::_, testing::_)).Times(0);
    for (const string &uri : {"", "file://media/Photo//1234//a.jpg",
        "file://media/Photo/%2e%2e%2f1234/a.jpg"}) {
        unordered_map<string, vector<string>> labels;
        labels.emplace("stale", vector<string> {"stale"});
        EXPECT_EQ(manager.GetAssetsLabel({uri}, labels), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(labels.empty());
        unordered_map<string, string> captions;
        captions.emplace("stale", "stale");
        EXPECT_EQ(manager.GetAssetsCaption({uri}, captions), E_INVALID_ARGUMENTS);
        EXPECT_TRUE(captions.empty());
    }
}

static testing::AssertionResult CheckAnalysisMetadataInvalidInputs(const AnalysisMetadataInvalidInputQuery &query)
{
    // Empty, non-photo, non-positive, overflowing, traversal, remote, cross-user and mixed batches.
    const std::vector<std::vector<std::string>> invalidInputs = {
        {},
        {"file://media/Audio/1"},
        {"file://media/Album/1"},
        {"file://media/Photo/0"},
        {"file://media/Photo/-1"},
        {"file://media/Photo/2147483648"},
        {"file://media/Photo/1/../2"},
        {"file://media/Photo/1?networkid=remote_device"},
        {"file://media/Photo/1?user=101"},
        {"file://media/Photo/1", "file://media/Audio/1"},
    };
    for (size_t index = 0; index < invalidInputs.size(); ++index) {
        const auto [ret, resultEmpty] = query(invalidInputs[index]);
        if (ret != E_INVALID_ARGUMENTS || !resultEmpty) {
            MEDIA_ERR_LOG("Invalid input case %{public}zu failed, ret: %{public}d, result empty: %{public}d",
                index, ret, resultEmpty);
            return testing::AssertionFailure() << "invalid input case: " << index << ", ret: " << ret
                << ", expected: " << E_INVALID_ARGUMENTS << ", result empty: " << resultEmpty;
        }
    }
    return testing::AssertionSuccess();
}

} // namespace Media
} // namespace OHOS
