/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#include "media_space_statistics_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "get_self_permissions.h"
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
#include "userfilemgr_uri.h"
#include "media_file_uri.h"
#include "data_secondary_directory_uri.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

/**
 * @FileName MediaSpaceStatisticsTest
 * @Desc Media space statistics function test
 *
 */
namespace OHOS {
namespace Media {
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
void CreateFile(std::string baseURI, std::string targetPath, std::string newName, MediaType mediaType,
    const unsigned char fileContent[], const int len);
std::unique_ptr<FileAsset> GetFile(int mediaTypeId);
void ClearFile();
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
int g_albumMediaType = MEDIA_TYPE_ALBUM;
const int SCAN_WAIT_TIME = 10;
int64_t g_oneImageSize = 0;
int64_t g_oneVideoSize = 0;
int64_t g_oneAudioSize = 0;
int64_t g_oneFileSize = 0;


static const unsigned char FILE_CONTENT_TXT[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24
};
static const unsigned char FILE_CONTENT_JPG[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b,
    0x20, 0x20, 0x20, 0x50
};
static const unsigned char FILE_CONTENT_MP3[] = {
    0x49, 0x44, 0x33, 0x03, 0x20, 0x20, 0x20, 0x0c, 0x24, 0x5d, 0x54, 0x45, 0x4e, 0x43, 0x20, 0x20, 0x20, 0x0b, 0x20,
    0x20, 0x20, 0x50, 0x72, 0x6f, 0x20, 0x54, 0x6f, 0x6f, 0x6c, 0x73, 0x20, 0x54, 0x58, 0x58, 0x58, 0x20, 0x20, 0x20,
    0x27, 0x20, 0x20, 0x20, 0x6f, 0x72, 0x69, 0x67, 0x69, 0x6e, 0x61, 0x74, 0x6f, 0x72, 0x5f, 0x72, 0x65, 0x66, 0x65,
    0x72, 0x65, 0x6e, 0x63, 0x65, 0x20, 0x21, 0x46, 0x6c, 0x4c, 0x55, 0x6b, 0x6e, 0x45, 0x6d, 0x52, 0x62, 0x61, 0x61,
    0x61, 0x47, 0x6b, 0x20, 0x54, 0x59, 0x45, 0x52, 0x20, 0x20, 0x20, 0x06, 0x20, 0x20, 0x20, 0x32, 0x30, 0x31, 0x35,
    0x20, 0x54, 0x44, 0x41, 0x54, 0x20, 0x20, 0x20, 0x06, 0x20, 0x20, 0x20, 0x32, 0x33, 0x31, 0x31, 0x20, 0x54, 0x58,
    0x58, 0x58, 0x20, 0x20, 0x20, 0x17, 0x20, 0x20, 0x20, 0x74, 0x69, 0x6d, 0x65, 0x5f, 0x72, 0x65, 0x66, 0x65, 0x72,
    0x65, 0x6e, 0x63, 0x65, 0x20, 0x31, 0x36, 0x36, 0x31, 0x31, 0x39, 0x20, 0x54, 0x43, 0x4f, 0x4d, 0x20, 0x20, 0x20,
    0x09, 0x20, 0x20, 0x01, 0xff, 0xfe, 0x4b, 0x6d, 0xd5, 0x8b, 0x20, 0x20, 0x54, 0x50, 0x45, 0x31, 0x20, 0x20, 0x20,
    0x0f, 0x20, 0x20, 0x01, 0xff, 0xfe, 0x43, 0x51, 0x70, 0x65, 0x6e, 0x63, 0x4b, 0x6d, 0xd5, 0x8b, 0x20, 0x20, 0x54,
    0x41, 0x4c, 0x42, 0x20, 0x20, 0x20, 0x07, 0x20, 0x20, 0x20, 0x6d, 0x65, 0x64, 0x69, 0x61, 0x20, 0x54, 0x49, 0x54,
    0x32, 0x20, 0x20, 0x20, 0x06, 0x20, 0x20, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x54, 0x50, 0x45, 0x32, 0x20, 0x20,
    0x20, 0x0c, 0x20, 0x20, 0x20, 0x6d, 0x65, 0x64, 0x69, 0x61, 0x5f, 0x74, 0x65, 0x73, 0x74, 0x20, 0x54, 0x58, 0x58,
    0x58, 0x20, 0x20, 0x20, 0x0e, 0x20, 0x20, 0x20, 0x61, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x20, 0x6d, 0x65, 0x64, 0x69,
    0x61, 0x20, 0x54, 0x43, 0x4f, 0x4e, 0x20, 0x20, 0x20, 0x09, 0x20, 0x20, 0x20, 0x4c, 0x79, 0x72, 0x69, 0x63, 0x61,
    0x6c, 0x20, 0x54, 0x53, 0x53, 0x45, 0x20, 0x20, 0x20, 0x0f, 0x20, 0x20, 0x20, 0x4c, 0x61
};
static const unsigned char FILE_CONTENT_MP4[] = {
    0x20, 0x20, 0x20, 0x20, 0x66, 0x74, 0x79, 0x70, 0x69, 0x73, 0x6f, 0x6d, 0x20, 0x20, 0x02, 0x20, 0x69, 0x73, 0x6f,
    0x6d, 0x69, 0x73, 0x6f, 0x32, 0x61, 0x76, 0x63, 0x31, 0x6d, 0x70, 0x34, 0x31, 0x20, 0x20, 0x20, 0x08, 0x66, 0x72,
    0x65, 0x65, 0x20, 0x49, 0xdd, 0x01, 0x6d, 0x64, 0x61, 0x74, 0x20, 0x20, 0x02, 0xa0, 0x06, 0x05, 0xff, 0xff, 0x9c,
    0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7, 0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef, 0x78, 0x32, 0x36,
    0x34, 0x20, 0x2d, 0x20, 0x63, 0x6f, 0x72, 0x65, 0x20, 0x31, 0x35, 0x39, 0x20, 0x2d, 0x20, 0x48, 0x2e, 0x32, 0x36,
    0x34, 0x2f, 0x4d, 0x50, 0x45, 0x47, 0x2d, 0x34, 0x20, 0x41, 0x56, 0x43, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x63, 0x20,
    0x2d, 0x20, 0x43, 0x6f, 0x70, 0x79, 0x6c, 0x65, 0x66, 0x74, 0x20, 0x32, 0x30, 0x30, 0x33, 0x2d, 0x32, 0x30, 0x31,
    0x39, 0x20, 0x2d, 0x20, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x76, 0x69, 0x64, 0x65,
    0x6f, 0x6c, 0x61, 0x6e, 0x2e, 0x6f, 0x72, 0x67, 0x2f, 0x78, 0x32, 0x36, 0x34, 0x2e, 0x68, 0x74, 0x6d, 0x6c, 0x20,
    0x2d, 0x20, 0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x3a, 0x20, 0x63, 0x61, 0x62, 0x61, 0x63, 0x3d, 0x31, 0x20,
    0x72, 0x65, 0x66, 0x3d, 0x33, 0x20, 0x64, 0x65, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x3d, 0x31, 0x3a, 0x30, 0x3a, 0x30,
    0x20, 0x61, 0x6e, 0x61, 0x6c, 0x79, 0x73, 0x65, 0x3d, 0x30, 0x78, 0x33, 0x3a, 0x30, 0x78, 0x31, 0x31, 0x33, 0x20,
    0x6d, 0x65, 0x3d, 0x68, 0x65, 0x78, 0x20, 0x73, 0x75, 0x62, 0x6d, 0x65, 0x3d, 0x37, 0x20, 0x70, 0x73, 0x79, 0x3d,
    0x31, 0x20, 0x70, 0x73, 0x79, 0x5f, 0x72, 0x64, 0x3d, 0x31, 0x2e, 0x30, 0x30, 0x3a, 0x30, 0x2e, 0x30, 0x30, 0x20,
    0x6d, 0x69, 0x78, 0x65, 0x64, 0x5f, 0x72, 0x65, 0x66, 0x3d, 0x31, 0x20, 0x6d, 0x65, 0x5f, 0x72, 0x61, 0x6e, 0x67,
    0x65, 0x3d, 0x31, 0x36, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61, 0x5f, 0x6d, 0x65, 0x3d, 0x31, 0x20, 0x74, 0x72,
    0x65, 0x6c, 0x6c, 0x69, 0x73, 0x3d, 0x31, 0x20, 0x38, 0x78, 0x38, 0x64, 0x63, 0x74, 0x3d, 0x31, 0x20, 0x63, 0x71,
    0x6d, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x61, 0x64, 0x7a, 0x6f, 0x6e, 0x65, 0x3d, 0x32, 0x31, 0x2c, 0x31, 0x31, 0x20,
    0x66, 0x61, 0x73, 0x74, 0x5f, 0x70, 0x73, 0x6b, 0x69, 0x70, 0x3d, 0x31, 0x20, 0x63, 0x68, 0x72, 0x6f, 0x6d, 0x61,
    0x5f, 0x71, 0x70, 0x5f, 0x6f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x3d, 0x2d, 0x32, 0x20, 0x74, 0x68, 0x72, 0x65, 0x61,
    0x64, 0x73, 0x3d, 0x36, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x61, 0x68, 0x65, 0x61, 0x64, 0x5f, 0x74, 0x68, 0x72, 0x65,
    0x61, 0x64, 0x73, 0x3d, 0x31, 0x20, 0x73, 0x6c, 0x69, 0x63, 0x65, 0x64, 0x5f, 0x74, 0x68, 0x72, 0x65, 0x61, 0x64,
    0x73, 0x3d, 0x30, 0x20, 0x6e, 0x72, 0x3d, 0x30, 0x20, 0x64, 0x65, 0x63, 0x69, 0x6d, 0x61, 0x74, 0x65, 0x3d, 0x31,
    0x20, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6c, 0x61, 0x63, 0x65, 0x64, 0x3d, 0x30, 0x20, 0x62, 0x6c, 0x75, 0x72, 0x61,
    0x79, 0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x74, 0x3d, 0x30, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x72, 0x61, 0x69,
    0x6e, 0x65, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x72, 0x61, 0x3d, 0x30, 0x20, 0x62, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x73,
    0x3d, 0x33, 0x20, 0x62, 0x5f, 0x70, 0x79, 0x72, 0x61, 0x6d, 0x69, 0x64, 0x3d, 0x32, 0x20, 0x62, 0x5f, 0x61, 0x64,
    0x61, 0x70, 0x74, 0x3d, 0x31, 0x20, 0x62, 0x5f, 0x62, 0x69, 0x61, 0x73, 0x3d, 0x30, 0x20, 0x64, 0x69, 0x72, 0x65,
    0x63, 0x74, 0x3d, 0x31, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x62, 0x3d, 0x31, 0x20, 0x6f, 0x70, 0x65, 0x6e,
    0x5f, 0x67, 0x6f, 0x70, 0x3d, 0x30, 0x20, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x70, 0x3d, 0x32, 0x20, 0x6b, 0x65,
    0x79, 0x69, 0x6e, 0x74, 0x3d, 0x32, 0x35, 0x30, 0x20, 0x6b, 0x65, 0x79, 0x69, 0x6e, 0x74, 0x5f, 0x6d, 0x69, 0x6e,
    0x3d, 0x32, 0x35, 0x20, 0x73, 0x63, 0x65, 0x6e, 0x65, 0x63, 0x75
};

MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

void MediaSpaceStatisticsTest::SetUpTestCase(void)
{
    // test QueryTotalSize when sDataShareHelper_ is nullptr
    MediaVolume mediaVolume;
    mediaLibraryManager->QueryTotalSize(mediaVolume);
    mediaLibraryManager->CloseAsset("", 0);

    vector<string> perms;
    perms.push_back("ohos.permission.READ_MEDIA");
    perms.push_back("ohos.permission.WRITE_MEDIA");
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.FILE_ACCESS_MANAGER");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaSpaceStatisticsUnitTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);

    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: invoked");
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(sDataShareHelper_, nullptr);

    // make sure board is empty
    ClearAllFile();

    // create base file
    CreateFile(CONST_MEDIALIBRARY_IMAGE_URI, "Pictures/", "MediaSpaceStatisticsTest.jpg", MEDIA_TYPE_IMAGE,
        FILE_CONTENT_JPG, sizeof(FILE_CONTENT_JPG));
    CreateFile(CONST_MEDIALIBRARY_VIDEO_URI, "Videos/", "MediaSpaceStatisticsTest.mp4", MEDIA_TYPE_VIDEO,
        FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    CreateFile(CONST_MEDIALIBRARY_AUDIO_URI, "Audios/", "MediaSpaceStatisticsTest.mp3", MEDIA_TYPE_AUDIO,
        FILE_CONTENT_MP3, sizeof(FILE_CONTENT_MP3));
    CreateFile(CONST_MEDIALIBRARY_FILE_URI, "Docs/Documents/", "MediaSpaceStatisticsTest.txt", MEDIA_TYPE_FILE,
        FILE_CONTENT_TXT, sizeof(FILE_CONTENT_TXT));

    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    auto ret = sDataShareHelper_->Insert(scanUri, valuesBucket);
    EXPECT_EQ(ret, 0);
    sleep(SCAN_WAIT_TIME);

    // get base size
    auto image = GetFile(MEDIA_TYPE_IMAGE);
    auto video = GetFile(MEDIA_TYPE_VIDEO);
    auto audio = GetFile(MEDIA_TYPE_AUDIO);
    ASSERT_TRUE(image) << "Failed to get image file";
    ASSERT_TRUE(video) << "Failed to get video file";
    ASSERT_TRUE(audio) << "Failed to get audio file";
    g_oneImageSize = image->GetSize();
    g_oneVideoSize = video->GetSize();
    g_oneAudioSize = audio->GetSize();
    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: g_oneImageSize = %{public}lld",
        (long long)g_oneImageSize);
    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: g_oneVideoSize = %{public}lld",
        (long long)g_oneVideoSize);
    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: g_oneAudioSize = %{public}lld",
        (long long)g_oneAudioSize);
    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: g_oneFileSize = %{public}lld",
        (long long)g_oneFileSize);
    MEDIA_INFO_LOG("MediaSpaceStatisticsTest::SetUpTestCase:: Finish");
}

void MediaSpaceStatisticsTest::TearDownTestCase(void)
{
    MEDIA_ERR_LOG("TearDownTestCase start");
    if (sDataShareHelper_ != nullptr) {
        sDataShareHelper_->Release();
    }
    ClearAllFile();
    MEDIA_INFO_LOG("TearDownTestCase end");
}
// SetUp:Execute before each test case
void MediaSpaceStatisticsTest::SetUp(void) {}

void MediaSpaceStatisticsTest::TearDown(void) {}

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
    MEDIA_INFO_LOG("InitMediaLibraryManager success~!");

    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    }

    mediaLibraryManager->InitMediaLibraryManager(remoteObj);
}

std::unique_ptr<FileAsset> GetFile(int mediaTypeId)
{
    if (sDataShareHelper_ == nullptr) {
        return nullptr;
    }
    vector<string> columns;
    DataSharePredicates predicates;
    string prefix = std::string(CONST_MEDIA_DATA_DB_MEDIA_TYPE) + " = " + std::to_string(mediaTypeId);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    shared_ptr<DataShareResultSet> resultSet = nullptr;
    resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }

    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    if (fetchFileResult->GetCount() <= 0) {
        return nullptr;
    }

    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetLastObject();
    EXPECT_NE(fileAsset, nullptr);
    return fileAsset;
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
    Uri deleteAssetUri(MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_FILEOPRN);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(CONST_MEDIA_DATA_DB_ID, MediaFileUtils::GetIdFromUri(fileUri));
    int retVal = sDataShareHelper_->Delete(deleteAssetUri, predicates);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test DeleteFile::uri :%{private}s", deleteAssetUri.ToString().c_str());
    EXPECT_NE(retVal, E_ERR);
}

void ClearFile()
{
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    vector<string> columns;
    DataSharePredicates predicates;
    string prefix = string(CONST_MEDIA_DATA_DB_MEDIA_TYPE) + " <> " + to_string(g_albumMediaType);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(MEDIALIBRARY_DATA_URI);
    shared_ptr<DataShareResultSet> resultSet = nullptr;
    resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    ASSERT_NE(resultSet, nullptr);

    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    ASSERT_GT(fetchFileResult->GetCount(), 0);
    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
    while (fileAsset != nullptr) {
        DeleteFile(fileAsset->GetUri());
        fileAsset = fetchFileResult->GetNextObject();
    }
}

void CreateFile(std::string baseURI, std::string targetPath, std::string newName, MediaType mediaType,
    const unsigned char fileContent[], const int len)
{
    MEDIA_INFO_LOG("CreateFile:: start Create file: %s", newName.c_str());
    if (sDataShareHelper_ == nullptr) {
        return;
    }

    string abilityUri = Media::MEDIALIBRARY_DATA_URI;
    if (MediaFileUtils::StartsWith(targetPath, "Pictures/") ||
        MediaFileUtils::StartsWith(targetPath, "Videos/")) {
        abilityUri += string(CONST_MEDIA_PHOTOOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    } else if (MediaFileUtils::StartsWith(targetPath, "Audios/")) {
        abilityUri += string(CONST_MEDIA_AUDIOOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    } else {
        abilityUri += string(CONST_MEDIA_FILEOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    }
    Uri createAssetUri(abilityUri);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, newName);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, targetPath);

    int32_t index = sDataShareHelper_->Insert(createAssetUri, valuesBucket);
    int64_t virtualIndex = MediaFileUtils::GetVirtualIdByType(index, mediaType);
    string destUri = baseURI + "/" + std::to_string(virtualIndex);

    Uri openFileUriDest(destUri);
    int32_t destFd = sDataShareHelper_->OpenFile(openFileUriDest, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);

    int32_t resWrite = write(destFd, fileContent, len);
    if (resWrite == -1) {
        ASSERT_EQ(false, true);
    }

    mediaLibraryManager->CloseAsset(destUri, destFd);
    MEDIA_INFO_LOG("CreateFile:: end Create file: %s", newName.c_str());
}

void CopyFile(std::string srcUri, std::string baseURI, std::string targetPath, std::string newName,
    MediaType mediaType, int sleepSecond)
{
    MEDIA_INFO_LOG("CopyFile:: start Copy sleepSecond[%d] file: %s", sleepSecond, newName.c_str());
    if (sDataShareHelper_ == nullptr) {
        return;
    }
    Uri openFileUri(srcUri);
    int32_t srcFd = sDataShareHelper_->OpenFile(openFileUri, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(srcFd, 0);

    string abilityUri = Media::MEDIALIBRARY_DATA_URI;
    if (MediaFileUtils::StartsWith(targetPath, "Pictures/") ||
        MediaFileUtils::StartsWith(targetPath, "Videos/")) {
        abilityUri += string(CONST_MEDIA_PHOTOOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    } else if (MediaFileUtils::StartsWith(targetPath, "Audios/")) {
        abilityUri += string(CONST_MEDIA_AUDIOOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    } else {
        abilityUri += string(CONST_MEDIA_FILEOPRN) + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
    }
    Uri createAssetUri(abilityUri);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, newName);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, targetPath);
    int32_t index = sDataShareHelper_->Insert(createAssetUri, valuesBucket);
    int64_t virtualIndex = MediaFileUtils::GetVirtualIdByType(index, mediaType);
    string destUri = baseURI + "/" + std::to_string(virtualIndex);
    Uri openFileUriDest(destUri);
    int32_t destFd = sDataShareHelper_->OpenFile(openFileUriDest, MEDIA_FILEMODE_READWRITE);
    ASSERT_GT(destFd, 0);

    int64_t srcLen = lseek(srcFd, 0, SEEK_END);
    lseek(srcFd, 0, SEEK_SET);
    char buf[srcLen];
    int32_t readRet = read(srcFd, buf, srcLen);
    int32_t resWrite = write(destFd, buf, readRet);
    if (resWrite == -1) {
        ASSERT_EQ(false, true);
    }

    mediaLibraryManager->CloseAsset(srcUri, srcFd);
    mediaLibraryManager->CloseAsset(destUri, destFd);
    sleep(sleepSecond);
    MEDIA_INFO_LOG("CopyFile:: end Copy file: %s", newName.c_str());
}

void CheckQuerySize(std::string testNo, int mediaTypeId, int targetFileNumber)
{
    MediaVolume mediaVolume;
    mediaLibraryManager->QueryTotalSize(mediaVolume);
    int64_t querySize = 0;
    int64_t targetSize = 0;
    if (mediaTypeId == MEDIA_TYPE_IMAGE) {
        querySize = mediaVolume.GetImagesSize();
        targetSize = targetFileNumber * g_oneImageSize;
    } else if (mediaTypeId == MEDIA_TYPE_VIDEO) {
        querySize = mediaVolume.GetVideosSize();
        targetSize = targetFileNumber * g_oneVideoSize;
    } else if (mediaTypeId == MEDIA_TYPE_AUDIO) {
        querySize = mediaVolume.GetAudiosSize();
        targetSize = targetFileNumber * g_oneAudioSize;
    } else if (mediaTypeId == MEDIA_TYPE_FILE) {
        querySize = mediaVolume.GetFilesSize();
        targetSize = targetFileNumber * g_oneFileSize;
    }
    MEDIA_INFO_LOG("%s QueryTotalSize querySize = %{public}lld", testNo.c_str(), (long long)querySize);
    MEDIA_INFO_LOG("%s QueryTotalSize targetSize = %{public}lld", testNo.c_str(), (long long)targetSize);
    if (mediaTypeId == MEDIA_TYPE_IMAGE) {
        EXPECT_TRUE(querySize >= targetSize);
    } else {
        EXPECT_EQ(querySize, targetSize);
    }
}

int32_t CreateTestFile(MediaType MediaType, string fileName, string relativePath)
{
    string abilityUri = CONST_ML_FILE_URI_PREFIX;
    Uri createAssetUri(abilityUri + "/" + CONST_MEDIA_FILEOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, MediaType);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, fileName);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    return sDataShareHelper_->Insert(createAssetUri, valuesBucket);
}

string ReturnUri(string type)
{
    return (string(CONST_ML_FILE_URI_PREFIX) + '/' + "file" + "/" + type);
}

/**
 * @tc.number    : MediaSpaceStatistics_test_001
 * @tc.name      : get Media image size
 * @tc.desc      : 1.push 01.jpg into the device and make sure there is only one image
 *                 2.call the method to get media size
 *                 3.compare the size
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_001::Start");
    ASSERT_NE(sDataShareHelper_, nullptr);

    CheckQuerySize("MediaSpaceStatistics_test_001", MEDIA_TYPE_FILE, 0);
    CheckQuerySize("MediaSpaceStatistics_test_001", MEDIA_TYPE_IMAGE, 1);
    CheckQuerySize("MediaSpaceStatistics_test_001", MEDIA_TYPE_VIDEO, 1);
    CheckQuerySize("MediaSpaceStatistics_test_001", MEDIA_TYPE_AUDIO, 0);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_001::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_002
 * @tc.name      : get Media video size
 * @tc.desc      : 1.push 01.mp4 into the device and make sure there is only one video
 *                 2.call the method to get media size
 *                 3.compare the size
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_002::Start");
    ASSERT_NE(sDataShareHelper_, nullptr);
    CheckQuerySize("MediaSpaceStatistics_test_002", MEDIA_TYPE_VIDEO, 1);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_002::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_003
 * @tc.name      : get Media audio size
 * @tc.desc      : 1.push 01.mp3 into the device and make sure there is only one audio
 *                 2.call the method to get media size
 *                 3.compare the size
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_003::Start");
    ASSERT_NE(sDataShareHelper_, nullptr);
    CheckQuerySize("MediaSpaceStatistics_test_003", MEDIA_TYPE_AUDIO, 0);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_003::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_004
 * @tc.name      : get Media file size
 * @tc.desc      : 1.push 01.txt into the device and make sure there is only one file
 *                 2.call the method to get media size
 *                 3.compare the size
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_004::Start");
    ASSERT_NE(sDataShareHelper_, nullptr);
    CheckQuerySize("MediaSpaceStatistics_test_004", MEDIA_TYPE_FILE, 0);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_004::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_006
 * @tc.name      : get Media(image,video,audio,file) size
 * @tc.desc      : 1.delete all media
 *                 2.query media size
 *                 3.make sure size is 0
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_006::Start");
    ClearFile();
    MediaVolume mediaVolume;
    mediaLibraryManager->QueryTotalSize(mediaVolume);
    EXPECT_EQ(mediaVolume.GetImagesSize(), 0);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_006::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_007
 * @tc.name      : test for get uri or path by default relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_007::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_IMAGE, "MediaSpaceStatistics_test_007.jpg", "Pictures/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Pictures/MediaSpaceStatistics_test_007.jpg";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    int32_t ret = mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    ret = mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_007::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_008
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_008::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_FILE, "MediaSpaceStatistics_test_008.txt", "Docs/Documents/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Documents/MediaSpaceStatistics_test_008.txt";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_008::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");
    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_008::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_008::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_009
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_009::Start");

    int32_t index = CreateTestFile(MEDIA_TYPE_FILE, "MediaSpaceStatistics_test_009.txt", "Docs/Download/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Download/MediaSpaceStatistics_test_009.txt";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_009::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_009::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_009::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_010
 * @tc.name      : test for get uri or path there is not file in database
 * @tc.desc      : 1.get uri from path
 *                 2.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_010::Start");
    const Uri realUri(ReturnUri("1"));
    const string realPath = "/storage/cloud/100/files/Docs/Documents/MediaSpaceStatistics_test_010.txt";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    int32_t ret = mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    ret = mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_010::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_011
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_011::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_FILE, "MediaSpaceStatistics_test_011.txt", "Docs/Documents/test1/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Documents/test1/MediaSpaceStatistics_test_011.txt";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_011::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_011::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_011::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_012
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_012::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_FILE, "test.txt", "Docs/Documents/weixin/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Documents/weixin/test.txt";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_012::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_012::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_012::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_013
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_013::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_AUDIO, "MediaSpaceStatistics_test_013.mp3", "Docs/Download/weixin/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Download/weixin/MediaSpaceStatistics_test_013.mp3";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_013::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_013::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_013::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_014
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_014::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_IMAGE, "MediaSpaceStatistics_test_014.jpg", "Camera/weixin/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Camera/weixin/MediaSpaceStatistics_test_014.jpg";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    int32_t ret = mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    ret = mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_014::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_015
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_015::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_IMAGE, "MediaSpaceStatistics_test_015.jpg", "Docs/Download/weixin/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Download/weixin/MediaSpaceStatistics_test_015.jpg";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_015::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_015::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_015::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_016
 * @tc.name      : test for get uri or path by true relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_016::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_FILE, "MediaSpaceStatistics_test_016.xxx", "Docs/Download/weixin/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Docs/Download/weixin/MediaSpaceStatistics_test_016.xxx";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ(fileUri.ToString(), realUri.ToString());
    printf("MediaSpaceStatistics_test_016::fileUri is : %s\n", fileUri.ToString().c_str());
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ(filePath, realPath);
    printf("MediaSpaceStatistics_test_016::filePath is : %s\n", filePath.c_str());
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_016::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_017
 * @tc.name      : test for get uri or path by default relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_017, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_017::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_VIDEO, "MediaSpaceStatistics_test_017.mp4", "Videos/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Videos/MediaSpaceStatistics_test_017.mp4";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    int32_t ret = mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    ret = mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_017::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_018
 * @tc.name      : test for get uri or path by default relative path
 * @tc.desc      : 1.creat a file in Picture
 *                 2.get uri from path
 *                 3.get path from uri
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_018, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_018::Start");
    int32_t index = CreateTestFile(MEDIA_TYPE_AUDIO, "MediaSpaceStatistics_test_018.mp3", "Audios/");
    EXPECT_EQ((index > 0), true);
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(index, MediaType::MEDIA_TYPE_FILE);
    const Uri realUri(ReturnUri(to_string(virtualId)));
    const string realPath = "/storage/cloud/100/files/Audios/MediaSpaceStatistics_test_018.mp3";
    Uri fileUri(CONST_ML_FILE_URI_PREFIX);
    string filePath;
    string userId;
    int32_t ret = mediaLibraryManager->GetUriFromFilePath(realPath, fileUri, userId);
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetUriFromFilePath::End");

    ret = mediaLibraryManager->GetFilePathFromUri(realUri, filePath, "100");
    EXPECT_EQ((ret < 0), true);
    MEDIA_INFO_LOG("GetFilePathFromUri::End");

    MEDIA_INFO_LOG("MediaSpaceStatistics_test_018::End");
}

HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_019, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_019::Start");
    string columnName;
    string value;
    vector<string> columns;
    auto ret = mediaLibraryManager->GetResultSetFromDb(columnName, value, columns);
    EXPECT_EQ(ret, nullptr);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_019::End");
}

/**
 * @tc.number    : MediaSpaceStatistics_test_020
 * @tc.name      : Get thumbnail by medialibraryManager
 * @tc.desc      : Get thumbnail by medialibraryManager
 */
HWTEST_F(MediaSpaceStatisticsTest, MediaSpaceStatistics_test_020, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_020::Start");
    Uri uri("file://media/Photo/10");
    auto ret = mediaLibraryManager->GetThumbnail(uri);
    EXPECT_EQ((ret != nullptr), false);
    MEDIA_INFO_LOG("MediaSpaceStatistics_test_020::End");
}
} // namespace Media
} // namespace OHOS
