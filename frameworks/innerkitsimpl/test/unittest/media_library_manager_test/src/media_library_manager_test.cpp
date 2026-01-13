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
#include <iostream>

#include "media_library_manager_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "medialibrary_mock_tocken.h"
#include "hilog/log.h"
#include "iservice_registry.h"
#include "media_app_uri_permission_column.h"
#include "medialibrary_db_const.h"
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

struct UriParams {
    string path;
    string fileUri;
    Size size;
    bool isAstc;
    DecodeDynamicRange dynamicRange;
    string user;
};

static const unsigned char FILE_CONTENT_JPG[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b,
    0x20, 0x20, 0x20,
};

static const unsigned char FILE_CONTENT_MP4[] = {
    0x20, 0x20, 0x20, 0x20, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6f, 0x6d, 0x20, 0x20, 0x02, 0x20, 0x69, 0x73, 0x6f,
    0x6d, 0x69, 0x73, 0x6f, 0x32, 0x61, 0x76, 0x63, 0x31, 0x6d, 0x70, 0x34, 0x31, 0x20, 0x20, 0x20, 0x08, 0x66, 0x72,
    0x65, 0x65, 0x20, 0x49, 0xdd, 0x01, 0x6d, 0x64, 0x61, 0x74, 0x20, 0x20, 0x02, 0xa0, 0x06, 0x05, 0xff, 0xff, 0x9c,
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

    Uri scanUri(URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
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
    mediaLibraryManager->CloseAsset(uri, destFd);
    if (resWrite == -1) {
        EXPECT_EQ(false, true);
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
    auto ret = mediaLibraryManager->CloseAsset(uri, destFd);
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

// Scenario2: Test when uriBatch contains ML_URI_OFFSET then GetBatchAstcs calls GetAstcsByOffset.
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

// Scenario3: Test when uriBatch does not contain ML_URI_OFFSET then GetBatchAstcs calls GetAstcsBatch.
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
    uris.emplace_back(UFM_CREATE_PHOTO);
    uris.emplace_back(UFM_CREATE_AUDIO);
    uris.emplace_back(UFM_CREATE_PHOTO_ALBUM);
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
    EXPECT_NE(mediaLibraryExtendManager->GetResultSetFromDb(columnName, value, columns), nullptr);
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
    std::string columnName = MEDIA_DATA_DB_URI;
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
    EXPECT_NE(ptr, nullptr);
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
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> " + to_string(g_albumMediaType);
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
    string uristr = URI_QUERY_PHOTO;
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
    string displayName = "test1.jpg";
    string uri =  mediaLibraryManager->CreateAsset(displayName);
    ASSERT_NE(uri, "");
    GTEST_LOG_(INFO) << "uri is " << uri;
    int32_t destFd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);
    int32_t resWrite = write(destFd, FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    ASSERT_NE(resWrite, -1);
    mediaLibraryManager->CloseAsset(uri, destFd);

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
    predicatesAsset.EqualTo(MediaColumn::MEDIA_NAME, "test1.jpg");
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
    int32_t assetFd = mediaLibraryManager->OpenAsset(assetUri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(assetFd, 0);
    mediaLibraryManager->CloseAsset(assetUri, assetFd);
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
 * @tc.desc      : Create album fail when album is already exists
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CreateAlbum_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_002 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    string albumName = "testAlbum";
    int32_t albumId = -1;
    bool isAlbumExist = false;
    unique_ptr<PhotoAlbum> album = make_unique<PhotoAlbum>();
    int32_t albumCountBefore = 0;
    FetchResult<PhotoAlbum> albumsFetchResult;
    ASSERT_NE(mediaLibraryManager, nullptr);
    while (!isAlbumExist) {
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        albumCountBefore = albumsFetchResult.GetCount();
        for (int i = 0; i < albumCountBefore; i++) {
            album = albumsFetchResult.GetObjectAtPosition(i);
            if (album->GetAlbumName() == albumName) {
                isAlbumExist = true;
                albumId = album->GetAlbumId();
                break;
            }
        }
        if (!isAlbumExist) {
            int32_t id = mediaLibraryManager->CreateAlbum(albumName);
            EXPECT_GT(id, 0);
        }
    }
    int32_t errCode = mediaLibraryManager->CreateAlbum(albumName);
    ASSERT_EQ(errCode, -1);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    vector<unique_ptr<PhotoAlbum>> albumsVector;
    albumsVector.push_back(move(album));
    int32_t ret = mediaLibraryManager->DeleteAlbums(albumsVector);
    EXPECT_GE(ret, 0);
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_CreateAlbum_test_003
 * @tc.name      : Create album
 * @tc.desc      : Create album fail when albumName is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_CreateAlbum_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_003 enter");
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
    MEDIA_INFO_LOG("MediaLibraryManager_CreateAlbum_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_001
 * @tc.name      : Delete albums
 * @tc.desc      : Delete one album success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_001 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    vector<unique_ptr<PhotoAlbum>> albums;
    FetchResult<PhotoAlbum> albumsFetchResult;
    int32_t albumCountBefore = 0;
    string albumName = "testAlbum";
    bool isAlbumExist = false;
    unique_ptr<PhotoAlbum> album = make_unique<PhotoAlbum>();
    int32_t albumId = -1;
    ASSERT_NE(mediaLibraryManager, nullptr);
    while (!isAlbumExist) {
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        albumCountBefore = albumsFetchResult.GetCount();
        for (int i = 0; i < albumsFetchResult.GetCount(); i++) {
            album = albumsFetchResult.GetObjectAtPosition(i);
            ASSERT_NE(album, nullptr);
            if (album->GetAlbumName() == albumName) {
                isAlbumExist = true;
                albumId = album->GetAlbumId();
                albums.push_back(move(album));
                break;
            }
        }
        if (!isAlbumExist) {
            int32_t id = mediaLibraryManager->CreateAlbum(albumName);
            EXPECT_GT(id, 0);
        }
    }
    int32_t ret = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(ret, 1);
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
    ASSERT_EQ(albumsFetchResult.GetCount(), albumCountBefore - 1);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbum_test_001 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_002
 * @tc.name      : Delete albums
 * @tc.desc      : Delete album fail when albumId is invalid
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_002 enter");
    DataSharePredicates predicates;
    vector<string> columnsAlbum;
    vector<unique_ptr<PhotoAlbum>> albums;
    FetchResult<PhotoAlbum> albumsFetchResult;
    int32_t albumCountBefore = 0;
    string albumName = "testAlbum";
    bool isAlbumExist = false;
    unique_ptr<PhotoAlbum> album = make_unique<PhotoAlbum>();
    int32_t albumId = -1;
    ASSERT_NE(mediaLibraryManager, nullptr);
    while (!isAlbumExist) {
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        albumCountBefore = albumsFetchResult.GetCount();
        for (int i = 0; i < albumsFetchResult.GetCount(); i++) {
            album = albumsFetchResult.GetObjectAtPosition(i);
            if (album->GetAlbumName() == albumName) {
                isAlbumExist = true;
                albumId = album->GetAlbumId();
                break;
            }
        }
        if (!isAlbumExist) {
            int32_t id = mediaLibraryManager->CreateAlbum(albumName);
            EXPECT_GT(id, 0);
        }
    }
    while (albumId++) {
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
        albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &predicates);
        if (albumsFetchResult.GetCount() == 0) {
            break;
        }
    }
    album->SetAlbumId(albumId);
    albums.push_back(move(album));
    int32_t errCode = mediaLibraryManager->DeleteAlbums(albums);
    ASSERT_EQ(errCode, E_INVALID_URI);
    DataSharePredicates emptyPredicate;
    albumsFetchResult = mediaLibraryManager->GetAlbums(columnsAlbum, &emptyPredicate);
    ASSERT_EQ(albumsFetchResult.GetCount(), albumCountBefore);
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_002 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_003
 * @tc.name      : Delete albums
 * @tc.desc      : Delete album fail when album is system album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_003 enter");
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
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_003 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_004
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums success
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_004 enter");
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
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_004 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_005
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums fail with unexistent album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_005 enter");
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
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_005 exit");
}

/**
 * @tc.number    : MediaLibraryManager_DeleteAlbums_test_006
 * @tc.name      : Delete albums
 * @tc.desc      : Delete albums fail with system album
 */
HWTEST_F(MediaLibraryManagerTest, MediaLibraryManager_DeleteAlbums_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_006 enter");
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
    MEDIA_INFO_LOG("MediaLibraryManager_DeleteAlbums_test_006 exit");
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
} // namespace Media
} // namespace OHOS
