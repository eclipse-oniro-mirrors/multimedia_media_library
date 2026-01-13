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
#include <fstream>
#include <iostream>

#include "media_permission_helper_test.h"

#include "media_library_manager.h"
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
#include "userfilemgr_uri.h"
#include "data_secondary_directory_uri.h"
#include "photo_album_column.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

/**
 * @FileName MediaPermissionHelperTest
 * @Desc Media permission helper native function test
 *
 */
namespace OHOS {
namespace Media {
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const int CLEAN_TIME = 1;
const int SCAN_WAIT_TIME = 10;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

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
MediaPermissionHelper* mediaPermissionHelper = MediaPermissionHelper::GetMediaPermissionHelper();

void mockToken(const std::vector<std::string>& perms, shared_ptr<MediaLibraryMockHapToken>& token)
{
    // mock tokenID
    token = make_shared<MediaLibraryMockHapToken>("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

void MediaPermissionHelperTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaPermissionHelperTest::SetUpTestCase:: invoked");
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(sDataShareHelper_, nullptr);
    // make sure board is empty
    ClearAllFile();

    Uri scanUri(URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);
    mediaPermissionHelper->InitMediaPermissionHelper();
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();
    MEDIA_INFO_LOG("MediaPermissionHelperTest::SetUpTestCase:: Finish");
}

void MediaPermissionHelperTest::TearDownTestCase(void)
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

// SetUp:Execute before each test case
void MediaPermissionHelperTest::SetUp(void)
{
    // restore shell token before testcase
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaPermissionHelperTest::TearDown(void)
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

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_001
 * @tc.name      : 仅授权临时读权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_001 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{true, false, false, true, false, false};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_002
 * @tc.name      : 仅授权临时写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_002 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_002 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_003
 * @tc.name      : 仅授权临时读写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_003 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_003 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_004
 * @tc.name      : 仅授权持久读权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_004 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{true, false, false, true, false, false};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_004 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_005
 * @tc.name      : 仅授权持久写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_005 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{false, true, false, false, true, false};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_005 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_006
 * @tc.name      : 仅授权持久读写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_006 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_006 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_007
 * @tc.name      : 不同uri授权不同的持久权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_007 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 2);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{false, true, true, false, true, true};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_007 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_008
 * @tc.name      : 异常场景：uri列表中存储重复uri，授权不同的权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_008 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{3, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 1};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{false, false, true, true, true, true, false, false, true, true, true, true};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_008 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_009
 * @tc.name      : 异常场景：uri列表中存储重复uri，授权相同的权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_009 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{3, 2, 1, 3, 2, 3, 3, 2, 1, 3, 2, 1};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{false, false, true, false, true, true, true, true, false, false, true, true};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_009 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_010
 * @tc.name      :异常场景：授权PERSIST_READWRITE_IMAGEVIDEO权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_010 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_ERR);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 1);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);

    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_010 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_011
 * @tc.name      :异常场景：授权uri列表和权限列表长度不等
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_011 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_ERR);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 1);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);

    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_011 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_012
 * @tc.name      :异常场景：授权uri列表中存在格式错误的uri
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_012 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<string> checkUris = uris;
    uris.push_back("test_with_bad_uri.jpg");

    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_ERR);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(5, 1);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, checkUris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(5, false);
    EXPECT_EQ(resultSet, expectSet);

    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_012 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GrantPhotoUriPermission_test_013
 * @tc.name      :异常场景：授权uri列表的长度超过限制
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GrantPhotoUriPermission_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_013 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 1001; i++) {
        string uri = "test_uris_for_exceeds_limit_size.jpg";
        uris.push_back(uri);
    }

    vector<PhotoPermissionType> permissionTypes(1001, PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO);
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, permissionTypes, SensitiveType);
    EXPECT_EQ(ret, E_ERR);

    MEDIA_INFO_LOG("MediaPermissionHelper_GrantPhotoUriPermission_test_013 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_001
 * @tc.name      : 仅查询读权限，应用无读权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_001 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 1);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_002
 * @tc.name      : 仅查询写权限，应用无写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_002 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 2);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_002 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_003
 * @tc.name      : 仅查询读写权限，应用无读写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_003 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);
    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 3);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_003 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_004
 * @tc.name      : 仅查询读权限，应用有读权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_004 enter");
    // erase mock token
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = {
        "ohos.permission.WRITE_MEDIA",
        "ohos.permission.READ_IMAGEVIDEO",
        "ohos.permission.WRITE_IMAGEVIDEO",
        "ohos.permission.SHORT_TERM_WRITE_IMAGEVIDEO",
    };

    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 1);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_004 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_005
 * @tc.name      : 仅查询写权限，应用有写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_005 enter");
    // erase mock token
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = { "ohos.permission.WRITE_IMAGEVIDEO" };
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 2);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_005 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_006
 * @tc.name      : 仅查询读写权限，应用有读写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_006 enter");
    // erase mock token
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = {
        "ohos.permission.READ_IMAGEVIDEO",
        "ohos.permission.WRITE_IMAGEVIDEO",
    };

    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 3);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_006 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_007
 * @tc.name      : 混合查询，uri列表存在读，写，读写权限查询，应用无权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_007 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_007 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_008
 * @tc.name      : 混合查询，uri列表存在读，写，读写权限查询，应用写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_008 enter");
    // erase mock token
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = { "ohos.permission.WRITE_IMAGEVIDEO" };
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_008 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_009
 * @tc.name      : 混合查询，uri列表存在读，写，读写权限查询，应用读写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_009 enter");
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = {
        "ohos.permission.READ_IMAGEVIDEO",
        "ohos.permission.WRITE_IMAGEVIDEO",
    };

    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, true);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_009 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_010
 * @tc.name      : 异常场景，uri列表存在重复的uri，uri列表存在读，写，读写权限查询，应用无权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_010 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> grantUris;
    vector<string> checkUris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        grantUris.insert(grantUris.end(), 2, uri);
        checkUris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, tokenId, grantUris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{3, 2, 1, 3, 2, 1};
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, checkUris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet{false, true, true, false, true, true};
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_010 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_011
 * @tc.name      : 异常场景，uri列表的长度超过限制，读权限查询，应用有读权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_011 enter");
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();

    std::vector<std::string> perms = { "ohos.permission.READ_IMAGEVIDEO" };
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);

    vector<string> uris;
    for (int i = 0; i < 1001; i++) {
        string uri = "test_uris_for_exceeds_limit_size.jpg";
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(1001, 1);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(IPCSkeleton::GetSelfTokenID(),
        uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_ERR);

    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_011 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_012
 * @tc.name      : 异常场景：查询uri列表和权限列表长度不等
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_012 enter");
    uint64_t tokenId = 1;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(7, 1);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_ERR);
    EXPECT_EQ(resultSet.size(), 0);

    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_012 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckPhotoUriPermission_test_013
 * @tc.name      : 异常场景：查询uri列表中存在格式错误的uri
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckPhotoUriPermission_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_013 enter");
    uint64_t tokenId = 1;
    vector<string> uris;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    uris.push_back("test_with_bad_uri.jpg");

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 1);
    auto ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_ERR);
    EXPECT_EQ(resultSet.size(), 0);

    MEDIA_INFO_LOG("MediaPermissionHelper_CheckPhotoUriPermission_test_013 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_001
 * @tc.name      : 正常cancel临时读权限
 * @tc.desc      : cancel permission success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_001 enter");
    int src_tokenId = 1;
    int target_tokenId = 2;
    vector<string> uris;
    vector<PhotoPermissionType> permissionTypes;
    vector<OperationMode> modes;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
        permissionTypes.push_back(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO);
        modes.push_back(OperationMode::READ_MODE);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    int32_t ret = mediaPermissionHelper->GrantPhotoUriPermission(
        src_tokenId, target_tokenId, uris, permissionTypes, SensitiveType);
    EXPECT_EQ(ret, 0);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris, false, modes);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_002
 * @tc.name      : 正常cancel永久读权限
 * @tc.desc      : cancel permission success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_002 enter");
    int src_tokenId = 1;
    int target_tokenId = 2;
    vector<string> uris;
    vector<PhotoPermissionType> permissionTypes;
    vector<OperationMode> modes;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
        permissionTypes.push_back(PhotoPermissionType::PERSIST_READ_IMAGEVIDEO);
        modes.push_back(OperationMode::READ_MODE);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    int32_t ret = mediaPermissionHelper->GrantPhotoUriPermission(
        src_tokenId, target_tokenId, uris, permissionTypes, SensitiveType);
    EXPECT_EQ(ret, 0);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris, true, modes);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_002 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_003
 * @tc.name      : 正常cancel永久写权限
 * @tc.desc      : cancel permission
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_003 enter");
    int src_tokenId = 3;
    int target_tokenId = 2;
    vector<string> uris;
    vector<string> inColumn;
    vector<OperationMode> modes;
    vector<PhotoPermissionType> permissionTypes;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
        inColumn.push_back(MediaFileUtils::GetIdFromUri(uri));
        modes.push_back(OperationMode::WRITE_MODE);
        permissionTypes.push_back(PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    int32_t ret = mediaPermissionHelper->GrantPhotoUriPermission(src_tokenId, target_tokenId, uris,
        permissionTypes, SensitiveType);
    EXPECT_EQ(ret, 0);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris,
        true, modes);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_003 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_004
 * @tc.name      : 正常cancel临时写权限
 * @tc.desc      : 1.授权一批uri临时读权限 2.cancel永久写权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_004 enter");
    int src_tokenId = 3;
    int target_tokenId = 2;
    vector<string> uris;
    vector<string> inColumn;
    vector<OperationMode> modes;
    vector<PhotoPermissionType> permissionTypes;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
        inColumn.push_back(MediaFileUtils::GetIdFromUri(uri));
        modes.push_back(OperationMode::WRITE_MODE);
        permissionTypes.push_back(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    int32_t ret = mediaPermissionHelper->GrantPhotoUriPermission(src_tokenId, target_tokenId, uris,
        permissionTypes, SensitiveType);
    EXPECT_EQ(ret, 0);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris,
        false, modes);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_004 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_005
 * @tc.name      : 读权限取消
 * @tc.desc      : 1.授权一批uri永久读权限，授权临时读权限
                   2.cancel永久写权限
                   3.check写权限，预期无权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_005 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    vector<PhotoPermissionType> persistRead(6, PhotoPermissionType::PERSIST_READ_IMAGEVIDEO);
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, persistRead, SensitiveType);
    ASSERT_EQ(ret, E_SUCCESS);

    vector<PhotoPermissionType> tempRead(6, PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO);
    ret = mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, tempRead, SensitiveType);
    ASSERT_EQ(ret, E_SUCCESS);

    vector<OperationMode> modes(6, OperationMode::WRITE_MODE);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(srcTokenId, tokenId, uris, true, modes);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 2);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_005 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_006
 * @tc.name      : 写权限取消
 * @tc.desc      : 1.授权一批uri永久写权限，授权临时写权限
                   2.cancel永久写权限
                   3.check写权限，预期无权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_006 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    vector<PhotoPermissionType> persistRead(6, PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO);
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, persistRead, SensitiveType);
    ASSERT_EQ(ret, E_SUCCESS);

    vector<PhotoPermissionType> tempRead(6, PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO);
    ret = mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, tempRead, SensitiveType);
    ASSERT_EQ(ret, E_SUCCESS);

    vector<OperationMode> modes(6, OperationMode::WRITE_MODE);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(srcTokenId, tokenId, uris, true, modes);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> resultSet;
    vector<uint32_t> permissionFlags(6, 2);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet(6, false);
    EXPECT_EQ(resultSet, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_006 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_007
 * @tc.name      : 读写权限取消
 * @tc.desc      : 	1.授权一批uri永久读写权限
                    2.cancel永久读权限
                    3.check读权限，预期无权限
                    4.cancel永久写权限
                    5.check写权限，预期无权限
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_007 enter");
    uint64_t tokenId = 1;
    uint64_t srcTokenId = 2;

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
    }
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    vector<PhotoPermissionType> persistRead(6, PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO);
    auto ret =
        mediaPermissionHelper->GrantPhotoUriPermission(srcTokenId, tokenId, uris, persistRead, SensitiveType);
    ASSERT_EQ(ret, E_SUCCESS);

    vector<OperationMode> readModes(6, OperationMode::READ_MODE);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(srcTokenId, tokenId, uris, true, readModes);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> resultSet1;
    vector<uint32_t> permissionFlags1(6, 1);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet1, permissionFlags1);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet1(6, false);
    EXPECT_EQ(resultSet1, expectSet1);

    vector<OperationMode> writeModes(6, OperationMode::WRITE_MODE);
    ret = mediaPermissionHelper->CancelPhotoUriPermission(srcTokenId, tokenId, uris, true, writeModes);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> resultSet2;
    vector<uint32_t> permissionFlags2(6, 2);
    ret = mediaPermissionHelper->CheckPhotoUriPermission(tokenId, uris, resultSet2, permissionFlags2);
    EXPECT_EQ(ret, E_SUCCESS);

    vector<bool> expectSet2(6, false);
    EXPECT_EQ(resultSet2, expectSet2);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_007 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_008
 * @tc.name      : 异常场景，uris传入空列表
 * @tc.desc      : cancel permission fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_008 enter");
    int src_tokenId = 3;
    int target_tokenId = 2;
    vector<string> uris;
    int32_t ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_008 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CancelPhotoUriPermission_test_009
 * @tc.name      : 异常场景，uris传入长度超过限制
 * @tc.desc      : cancel permission fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CancelPhotoUriPermission_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_009 enter");
    int src_tokenId = 3;
    int target_tokenId = 2;
    vector<string> uris;
    for (int i = 0; i < 1001; i++) {
        string uri = "test_uris_for_exceeds_limit_size.mp4";
        uris.push_back(uri);
    }
    int32_t ret = mediaPermissionHelper->CancelPhotoUriPermission(src_tokenId, target_tokenId, uris);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MediaPermissionHelper_CancelPhotoUriPermission_test_009 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetPhotoUrisPermission_test_001
 * @tc.name      : Get 不支持的权限类型
 * @tc.desc      : Get permission fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetPhotoUrisPermission_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_001 enter");
    int target_tokenId = 2;
    vector<string> uris;
    vector<bool> result;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
    }

    vector<PhotoPermissionType> permissionTypes1(5, PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO);
    int32_t ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes1, result);
    EXPECT_EQ(ret, E_ERR);

    vector<PhotoPermissionType> permissionTypes2(5, PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO);
    ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes2, result);
    EXPECT_EQ(ret, E_ERR);

    vector<PhotoPermissionType> permissionTypes3(5, PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO);
    ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes3, result);
    EXPECT_EQ(ret, E_ERR);

    vector<PhotoPermissionType> permissionTypes4(5, PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO);
    ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes4, result);
    EXPECT_EQ(ret, E_ERR);

    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetPhotoUrisPermission_test_002
 * @tc.name      : uris列表长度不合法
 * @tc.desc      : Get permission fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetPhotoUrisPermission_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_002 enter");
    int target_tokenId = 3;
    vector<bool> result;
    vector<string> uris;
    vector<PhotoPermissionType> permissionTypes;
    int32_t ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes, result);
    EXPECT_EQ(ret, E_ERR);

    uris.resize(5);
    permissionTypes.resize(8);
    ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes, result);
    EXPECT_EQ(ret, E_ERR);

    uris.resize(1001);
    permissionTypes.resize(1001);
    ret = mediaPermissionHelper->GetPhotoUrisPermission(target_tokenId, uris, permissionTypes, result);
    EXPECT_EQ(ret, E_ERR);

    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_002 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetPhotoUrisPermission_test_003
 * @tc.name      : 单一权限类型
 * @tc.desc      : Get permission success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetPhotoUrisPermission_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_003 enter");
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    vector<bool> result1;
    vector<PhotoPermissionType> permissionTypes;
    for (int i = 0; i < 5; i++) {
        auto uri = CreatePhotoAsset("test.mp4");
        uris.push_back(uri);
        permissionTypes.push_back(PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO);
    }
    int32_t ret = mediaPermissionHelper->GetPhotoUrisPermission(targetTokenId, uris, permissionTypes, result1);
    EXPECT_EQ(ret, 0);
    vector<bool> expectSet1{false, false, false, false, false};
    EXPECT_EQ(result1, expectSet1);
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);
    vector<bool> result2;
    vector<PhotoPermissionType> getPermissionTypes{
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    ret = mediaPermissionHelper->GetPhotoUrisPermission(targetTokenId, uris, getPermissionTypes, result2);
    EXPECT_EQ(ret, E_OK);
    vector<bool> expectSet2{false, true, false, false, true};
    EXPECT_EQ(result2, expectSet2);
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_003 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetPhotoUrisPermission_test_004
 * @tc.name      : 混合权限类型：Get uris列表存在永久读，写，读写权限
 * @tc.desc      : Get permission success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetPhotoUrisPermission_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_004 enter");
    uint64_t srcTokenId = 2;
    uint64_t targetTokenId = 3;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);
    vector<bool> result;
    vector<PhotoPermissionType> getPermissionTypes{
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };
    ret = mediaPermissionHelper->GetPhotoUrisPermission(targetTokenId, uris, getPermissionTypes, result);
    EXPECT_EQ(ret, E_OK);
    vector<bool> expectSet{false, true, false, false, true, true};
    EXPECT_EQ(result, expectSet);
    MEDIA_INFO_LOG("MediaPermissionHelper_GetPhotoUrisPermission_test_004 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetUrisFromFusePaths_test_001
 * @tc.name      : Get uri
 * @tc.desc      : Get uri success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetUrisFromFusePaths_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetUrisFromFusePaths_test_001 enter");
    vector<string> paths = {"/data/storage/el2/media/test.mp4"};
    vector<string> uris;
    int32_t ret = mediaPermissionHelper->GetUrisFromFusePaths(paths, uris);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MediaPermissionHelper_GetUrisFromFusePaths_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_GetUrisFromFusePaths_test_002
 * @tc.name      : Get uri
 * @tc.desc      : Get uri fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_GetUrisFromFusePaths_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_GetUrisFromFusePaths_test_002 enter");
    vector<string> paths = {"/data/storage/el2/test.mp4"};
    vector<string> uris;
    int32_t ret = mediaPermissionHelper->GetUrisFromFusePaths(paths, uris);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MediaPermissionHelper_GetUrisFromFusePaths_test_002 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckCloudDownloadPermission_test_001
 * @tc.name      : Get uri
 * @tc.desc      : CheckPermission fail
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckCloudDownloadPermission_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckCloudDownloadPermission_test_001 enter");
    uint32_t tokenId = 1;
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    SetSelfTokenID(tokenId);
    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 2, 3, 1, 2, 3};
    auto ret = mediaPermissionHelper->CheckCloudDownloadPermission(tokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckCloudDownloadPermission_test_001 exit");
}

/**
 * @tc.number    : MediaPermissionHelper_CheckCloudDownloadPermission_test_002
 * @tc.name      : Get uri
 * @tc.desc      : CheckPermission success
 */
HWTEST_F(MediaPermissionHelperTest, MediaPermissionHelper_CheckCloudDownloadPermission_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckCloudDownloadPermission_test_002 enter");
    uint32_t srcTokenId = 1;
    uint32_t targetTokenId = IPCSkeleton::GetSelfTokenID();
    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        uris.push_back(uri);
    }
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
    };
    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    auto ret = mediaPermissionHelper->GrantPhotoUriPermission(
        srcTokenId, targetTokenId, uris, permissionTypes, SensitiveType);
    ASSERT_EQ(ret, E_OK);
    
    vector<bool> resultSet;
    vector<uint32_t> permissionFlags{1, 1, 1, 1, 1, 1};
    ret = mediaPermissionHelper->CheckCloudDownloadPermission(targetTokenId, uris, resultSet, permissionFlags);
    EXPECT_EQ(ret, E_SUCCESS);
    MEDIA_INFO_LOG("MediaPermissionHelper_CheckCloudDownloadPermission_test_002 exit");
}
} // namespace Media
} // namespace OHOS
