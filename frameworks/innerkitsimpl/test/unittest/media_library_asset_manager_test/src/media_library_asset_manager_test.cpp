/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "media_library_asset_manager_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "get_self_permissions.h"
#include "medialibrary_mock_tocken.h"
#include "file_uri.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "scanner_utils.h"
#include "system_ability_definition.h"
#include "media_asset_base_capi.h"
#include "media_asset_manager_capi.h"
#include "oh_media_asset.h"
#include "media_asset.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

/**
 * @FileName MediaLibraryAssetManagerTest
 * @Desc Media library asset manager native function test
 *
 */
namespace OHOS {
namespace Media {
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
std::unique_ptr<FileAsset> GetFile(int mediaTypeId);
void ClearFile();
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
int g_albumMediaType = MEDIA_TYPE_ALBUM;
int64_t g_oneImageSize = 0;
const int CLEAN_TIME = 1;
const int SCAN_WAIT_TIME = 10;
const int SCAN_WAIT_TIME_1S = 1;
const char ERROR_REQUEST_ID[UUID_STR_MAX_LENGTH] = "00000000-0000-0000-0000-000000000000";
const std::string ROOT_TEST_MEDIA_DIR =
    "/data/app/el2/100/base/com.ohos.medialibrary.medialibrarydata/haps/";
const std::string TEST_DISPLAY_NAME = "test_image.png";
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;
static const unsigned char FILE_CONTENT_JPG[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b,
    0x20, 0x20, 0x20,
};

static const unsigned char FILE_CONTENT_MP4[] = {
    0x20, 0x20, 0x20, 0x20, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6f, 0x6d, 0x20, 0x20, 0x02, 0x20, 0x69, 0x73, 0x6f,
    0x6d, 0x69, 0x73, 0x6f, 0x32, 0x61, 0x76, 0x63, 0x31, 0x6d, 0x70, 0x34, 0x31, 0x20, 0x20, 0x20, 0x08, 0x66, 0x72,
    0x65, 0x65, 0x20, 0x49, 0xdd, 0x01, 0x6d, 0x64, 0x61, 0x74, 0x20, 0x20, 0x02, 0xa0, 0x06, 0x05, 0xff, 0xff, 0x9c,
};

MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

void MediaLibraryAssetManagerTest::SetUpTestCase(void)
{
    vector<string> perms;
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    MEDIA_INFO_LOG("MediaLibraryAssetManagerTest::SetUpTestCase:: invoked");
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    if (sDataShareHelper_ == nullptr) {
        exit(0);
    }

    // make sure board is empty
    ClearAllFile();

    Uri scanUri(URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);

    MEDIA_INFO_LOG("MediaLibraryAssetManagerTest::SetUpTestCase:: Finish");
}

void MediaLibraryAssetManagerTest::TearDownTestCase(void)
{
    MEDIA_ERR_LOG("TearDownTestCase start");
    if (sDataShareHelper_ != nullptr) {
        sDataShareHelper_->Release();
    }
    sleep(CLEAN_TIME);
    ClearAllFile();

    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    MEDIA_INFO_LOG("TearDownTestCase end");
}
// SetUp:Execute before each test case
void MediaLibraryAssetManagerTest::SetUp(void)
{
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaLibraryAssetManagerTest::TearDown(void) {}

void CreateDataHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("Get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return;
    }
    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");

    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
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

void DeleteFile(std::string fileUri)
{
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    Uri deleteAssetUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, MediaFileUtils::GetIdFromUri(fileUri));
    int retVal = sDataShareHelper_->Delete(deleteAssetUri, predicates);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test DeleteFile::uri :%{private}s", deleteAssetUri.ToString().c_str());
    ASSERT_NE(retVal, E_ERR);
}

void ClearFile()
{
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    vector<string> columns;
    DataSharePredicates predicates;
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(g_albumMediaType);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    shared_ptr<DataShareResultSet> resultSet = nullptr;
    resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    ASSERT_NE((resultSet == nullptr), true);

    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    ASSERT_NE((fetchFileResult->GetCount() < 0), true);
    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
    while (fileAsset != nullptr) {
        DeleteFile(fileAsset->GetUri());
        fileAsset = fetchFileResult->GetNextObject();
    }
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

void CallbackFunciton(int32_t result, MediaLibrary_RequestId requestId)
{
    EXPECT_EQ(result, E_SUCCESS);
    MEDIA_INFO_LOG("CallbackFunciton::result: %{public}d", result);
    MEDIA_INFO_LOG("CallbackFunciton::requestId: %{public}s", requestId.requestId);
}

void CallbackFuncitonOnImageDataPrepared(MediaLibrary_ErrorCode result,
    MediaLibrary_RequestId requestId, MediaLibrary_MediaQuality mediaQuality,
    MediaLibrary_MediaContentType type, OH_ImageSourceNative* imageSourceNative)
{
    MEDIA_INFO_LOG("CallbackFuncitonOnImageDataPrepared::result: %{public}d", result);
    MEDIA_INFO_LOG("CallbackFuncitonOnImageDataPrepared::requestId: %{public}s", requestId.requestId);
}

void CallbackFuncitonOnMovingPhotoDataPrepared(MediaLibrary_ErrorCode result, MediaLibrary_RequestId requestId,
    MediaLibrary_MediaQuality mediaQuality, MediaLibrary_MediaContentType type, OH_MovingPhoto* movingPhoto)
{
    MEDIA_INFO_LOG("CallbackFuncitonOnMovingPhotoDataPrepared::result: %{public}d", result);
    MEDIA_INFO_LOG("CallbackFuncitonOnMovingPhotoDataPrepared::requestId: %{public}s", requestId.requestId);
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_001
 * @tc.name      : copy src image to dest image to see if error occurs
 * @tc.desc      : compare with src image to see if equals
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_001::Start");
    string srcDisplayName = "request_image_src_1.jpg";
    string destDisplayName = "request_image_dest_1.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    sleep(SCAN_WAIT_TIME_1S);
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_FAST_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    ASSERT_NE(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
    AppFileService::ModuleFileUri::FileUri destFileUri(destUri);
    string destPath = destFileUri.GetRealPath();
    int destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    int64_t destLen = lseek(destFd, 0, SEEK_END);
    lseek(destFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(destLen));
    ASSERT_NE((buf == nullptr), true);
    read(destFd, buf, destLen);
    bool result = CompareIfArraysEquals(buf, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    free(buf);
    close(destFd);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", destDisplayName.c_str());
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_002
 * @tc.name      : copy src image to dest image to see if error occurs
 * @tc.desc      : compare with src image to see if equals
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_002::Start");
    string srcDisplayName = "request_image_src_2.jpg";
    string destDisplayName = "request_image_dest_2.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_HIGH_QUALITY_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    ASSERT_NE(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
    AppFileService::ModuleFileUri::FileUri destFileUri(destUri);
    string destPath = destFileUri.GetRealPath();
    int destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    int64_t destLen = lseek(destFd, 0, SEEK_END);
    lseek(destFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(destLen));
    ASSERT_NE((buf == nullptr), true);
    read(destFd, buf, destLen);
    bool result = CompareIfArraysEquals(buf, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    free(buf);
    close(destFd);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", destDisplayName.c_str());
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_003
 * @tc.name      : copy src video to dest video to see if error occurs
 * @tc.desc      : compare with src video to see if equals
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_003::Start");
    string srcDisplayName = "request_video_src_1.mp4";
    string destDisplayName = "request_video_dest_1.mp4";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_BALANCED_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestVideoForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    ASSERT_NE(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
    AppFileService::ModuleFileUri::FileUri destFileUri(destUri);
    string destPath = destFileUri.GetRealPath();
    int destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    int64_t destLen = lseek(destFd, 0, SEEK_END);
    lseek(destFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(destLen));
    ASSERT_NE((buf == nullptr), true);
    read(destFd, buf, destLen);
    bool result = CompareIfArraysEquals(buf, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    free(buf);
    close(destFd);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", destDisplayName.c_str());
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_004
 * @tc.name      : input uri is null to see if error occurs and
 * @tc.desc      : compare requestId with null to see if equals
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_004::Start");
    string destDisplayName = "request_image_dest_3.jpg";
    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_FAST_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, nullptr,
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    EXPECT_EQ(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_005
 * @tc.name      : create video again to see if error occurs
 * @tc.desc      : compare with src image to see if equals
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_005::Start");
    string srcDisplayName = "request_video_src_2.mp4";
    string destDisplayName = "request_video_dest_2.mp4";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_BALANCED_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestVideoForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    ASSERT_NE(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
    AppFileService::ModuleFileUri::FileUri destFileUri(destUri);
    string destPath = destFileUri.GetRealPath();
    int destFd = MediaFileUtils::OpenFile(destPath, MEDIA_FILEMODE_READWRITE);
    int64_t destLen = lseek(destFd, 0, SEEK_END);
    lseek(destFd, 0, SEEK_SET);
    unsigned char *buf = static_cast<unsigned char*>(malloc(destLen));
    ASSERT_NE((buf == nullptr), true);
    read(destFd, buf, destLen);
    bool result = CompareIfArraysEquals(buf, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    free(buf);
    close(destFd);
    ASSERT_NE(result, true);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %{public}s", destDisplayName.c_str());
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_006
 * @tc.name      : create video again to see if error occurs
 * @tc.desc      : call request image function see if requestId = NULL
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_006::Start");
    string srcDisplayName = "request_video_src_3.mp4";
    string destDisplayName = "request_video_dest_3.mp4";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_FAST_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    MEDIA_INFO_LOG("requestId: %{public}s", requestID.requestId);
    EXPECT_EQ(strcmp(requestID.requestId, ERROR_REQUEST_ID), 0);
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_007
 * @tc.name      : request image by ML_HIGH_QUALITY_MODE, then cancel request
 * @tc.desc      : call request image function see if requestId = NULL
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_007::Start");
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
    }
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_HIGH_QUALITY_MODE;
    static OH_MediaLibrary_OnDataPrepared callback = CallbackFunciton;
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback);
    bool ret = OH_MediaAssetManager_CancelRequest(manager, requestID);
    ASSERT_EQ(ret, false);
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_008
 * @tc.name      : request image by ML_HIGH_QUALITY_MODE, then request image
 * @tc.desc      : call request image function and verify the correct return code and image source
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_008, TestSize.Level1)
{
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_HIGH_QUALITY_MODE;
    static OH_MediaLibrary_OnDataPrepared callback_ = CallbackFunciton;
    static OH_MediaLibrary_OnImageDataPrepared callback = CallbackFuncitonOnImageDataPrepared;
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetDisplayName(TEST_DISPLAY_NAME);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    ASSERT_NE(mediaAsset, nullptr);
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback_);
    MediaLibrary_ErrorCode ret = OH_MediaAssetManager_RequestImage(manager, mediaAsset, requestOptions,
        &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
    ret = OH_MediaAssetManager_RequestImage(nullptr, mediaAsset, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_RequestImage(manager, nullptr, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_RequestImage(manager, mediaAsset, requestOptions, nullptr, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_RequestImage(manager, mediaAsset, requestOptions, &requestID, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    std::shared_ptr<FileAsset> fileAsset_ = mediaAsset->mediaAsset_->GetFileAssetInstance();
    const string displayName = "";
    fileAsset_->SetDisplayName(displayName);
    ret = OH_MediaAssetManager_RequestImage(manager, mediaAsset, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_Release(manager);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
}

/**
 * @tc.number    : MediaLibraryAssetManager_test_009
 * @tc.name      : request moving photo by ML_HIGH_QUALITY_MODE, then request moving photo
 * @tc.desc      : call request moving photo function and verify the correct return code and moving photo source
 */
HWTEST_F(MediaLibraryAssetManagerTest, MediaLibraryAssetManager_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryAssetManager_test_009::Start");
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    ASSERT_NE(srcuri, "");
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    ASSERT_NE(srcFd <= 0, true);
    int32_t resWrite = write(srcFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    ASSERT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);
    OH_MediaAssetManager *manager = OH_MediaAssetManager_Create();
    ASSERT_NE(manager, nullptr);
    MediaLibrary_RequestOptions requestOptions;
    requestOptions.deliveryMode = MediaLibrary_DeliveryMode::MEDIA_LIBRARY_HIGH_QUALITY_MODE;
    static OH_MediaLibrary_OnDataPrepared callback_ = CallbackFunciton;
    static OH_MediaLibrary_OnMovingPhotoDataPrepared callback = CallbackFuncitonOnMovingPhotoDataPrepared;
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetDisplayName(TEST_DISPLAY_NAME);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    ASSERT_NE(mediaAsset, nullptr);
    MediaLibrary_RequestId requestID = OH_MediaAssetManager_RequestImageForPath(manager, srcuri.c_str(),
        requestOptions, destUri.c_str(), callback_);
    MediaLibrary_ErrorCode ret = OH_MediaAssetManager_RequestMovingPhoto(manager, mediaAsset, requestOptions,
        &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
    ret = OH_MediaAssetManager_RequestMovingPhoto(manager, mediaAsset, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
    ret = OH_MediaAssetManager_RequestMovingPhoto(manager, nullptr, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_RequestMovingPhoto(manager, mediaAsset, requestOptions, nullptr, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MediaAssetManager_RequestMovingPhoto(manager, mediaAsset, requestOptions, &requestID, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    std::shared_ptr<FileAsset> fileAsset_ = mediaAsset->mediaAsset_->GetFileAssetInstance();
    const string displayName = "";
    fileAsset_->SetDisplayName(displayName);
    ret = OH_MediaAssetManager_RequestMovingPhoto(manager, mediaAsset, requestOptions, &requestID, callback);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    EXPECT_EQ(OH_MediaAssetManager_Release(manager), MEDIA_LIBRARY_OK);
}
} // namespace Media
} // namespace OHOS
