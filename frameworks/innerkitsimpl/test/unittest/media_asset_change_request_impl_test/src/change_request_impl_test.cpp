/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaAssetChangeRequestImplTest"
#include <fcntl.h>
#include "change_request_impl_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "file_uri.h"
#include "get_self_permissions.h"
#include "hilog/log.h"
#include "userfilemgr_uri.h"
#include "iservice_registry.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_asset_change_request_impl.h"
#include "system_ability_definition.h"
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "scanner_utils.h"
#include "media_asset_base_capi.h"
#include "userfile_client.h"
#include "media_asset_impl.h"
#include "directory_ex.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const int SCAN_WAIT_TIME = 10;
const int CLEAN_TIME = 1;
const int DEFAULT_ID = 1;
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

void MediaAssetChangeRequestImplTest::SetUpTestCase(void)
{
    vector<string> perms;
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaAssetChangeRequestImplTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);

    MEDIA_INFO_LOG("MediaAssetChangeRequestImplTest::SetUpTestCase:: invoked");
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    if (sDataShareHelper_ == nullptr) {
        ASSERT_NE(sDataShareHelper_, nullptr);
        return;
    }

    // make sure board is empty
    ClearAllFile();

    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);

    MEDIA_INFO_LOG("MediaAssetChangeRequestImplTest::SetUpTestCase:: Finish");
}

void MediaAssetChangeRequestImplTest::TearDownTestCase(void)
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

void MediaAssetChangeRequestImplTest::SetUp(void)
{
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaAssetChangeRequestImplTest::TearDown(void) {}

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

void SetFileAssetInfo(shared_ptr<FileAsset> fileAsset, int32_t id, int32_t photoSubType, MediaType mediaType)
{
    if (fileAsset != nullptr) {
        fileAsset->SetId(id);
        fileAsset->SetPhotoSubType(photoSubType);
        fileAsset->SetMediaType(mediaType);
    }
}
#ifdef HAS_ACE_ENGINE_PART
HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_001, TestSize.Level0)
{
    std::shared_ptr<MediaAsset> mediaAsset = nullptr;
    MediaAssetChangeRequestImpl impl(mediaAsset);
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    auto result = impl.ContainsResource(resourceType);
    EXPECT_EQ(result, false);
}
#endif

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_002, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    AssetChangeOperation option = AssetChangeOperation::DISCARD_CAMERA_PHOTO;
    auto result = impl.ChangeOperationExecute(option);
    EXPECT_EQ(result, false);
    option = AssetChangeOperation::CREATE_FROM_SCRATCH;
    result = impl.ChangeOperationExecute(option);
    EXPECT_EQ(result, false);
}

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_003, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    auto result = impl.OpenWriteCacheHandler(true);
    EXPECT_EQ(result, E_FAIL);
}

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_004, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    auto fileasset = impl.mediaAsset_->GetFileAssetInstance();
    string assetUri = fileasset->GetUri();
    Uri uri(assetUri);
    OHOS::UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    auto result = impl.CopyDataBufferToMediaLibrary(destFd, true);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_005, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    auto fileasset = impl.mediaAsset_->GetFileAssetInstance();
    string assetUri = fileasset->GetUri();
    Uri uri(assetUri);
    OHOS::UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    string realPath = true ? impl.movingPhotoVideoRealPath_ : impl.realPath_;
    string absFilePath;
    OHOS::PathToRealPath(realPath, absFilePath);
    OHOS::UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    auto result = impl.SendFile(srcFd, destFd);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_006, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    int32_t cacheFd = impl.OpenWriteCacheHandler();
    OHOS::UniqueFd uniqueFd(cacheFd);
    AddResourceMode mode = AddResourceMode::DATA_BUFFER;
    auto result = impl.AddResourceByMode(uniqueFd, mode, true);
    EXPECT_EQ(result, true);
}

HWTEST_F(MediaAssetChangeRequestImplTest, media_library_test_007, TestSize.Level0)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    std::shared_ptr<MediaAsset> mediaAsset = std::make_shared<MediaAssetImpl>(fileAsset);
    MediaAssetChangeRequestImpl impl(mediaAsset);
    auto fileasset = impl.mediaAsset_->GetFileAssetInstance();
    string assetUri = fileasset->GetUri();
    Uri uri(assetUri);
    OHOS::UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    auto result = impl.WriteCacheByArrayBuffer(destFd, true);
    EXPECT_EQ(result, true);
}
} // namespace Media
} // namespace OHOS
