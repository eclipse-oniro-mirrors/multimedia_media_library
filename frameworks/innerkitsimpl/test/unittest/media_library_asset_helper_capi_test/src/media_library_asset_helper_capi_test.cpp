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
#define MLOG_TAG "MediaLibraryAssetHelperCapiTest"

#define private public
#include "media_library_asset_helper_capi_test.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "file_uri.h"
#include "medialibrary_mock_tocken.h"
#include "hilog/log.h"
#include "userfilemgr_uri.h"
#include "iservice_registry.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "media_asset_base_capi.h"
#include "media_access_helper_capi.h"
#include "media_asset_manager_capi.h"
#include "media_asset_types.h"
#include "oh_media_asset_change_request.h"
#include "media_asset_change_request_capi.h"
#include "media_asset_change_request_impl.h"
#include "media_asset_capi.h"
#include "system_ability_definition.h"
#include "oh_media_asset.h"
#include "media_asset.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_library_manager.h"
#include "medialibrary_data_manager.h"
#include "media_log.h"
#include "media_volume.h"
#include "medialibrary_unistore_manager.h"
#include "photo_file_utils.h"
#include "scanner_utils.h"
#include "userfile_manager_types.h"
#include "ability_context_impl.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace Media {
std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
void ClearAllFile();
void CreateDataHelper(int32_t systemAbilityId);

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const int SCAN_WAIT_TIME_1S = 1;
const int SCAN_WAIT_TIME = 10;
const int CLEAN_TIME = 1;
const int DEFAULT_ID = 1;
const std::string ROOT_TEST_MEDIA_DIR =
    "/data/app/el2/100/base/com.ohos.medialibrary.medialibrarydata/haps/";
const std::string FILE_MANAGER_TEST_DIR = "/storage/media/local/files/Docs/Download/AssetHelperCapiTest/";
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

void MediaLibraryAssetHelperCapiTest::SetUpTestCase(void)
{
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.MEDIA_LOCATION");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");

    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    MEDIA_INFO_LOG("MediaLibraryAssetHelperCapiTest::SetUpTestCase:: invoked");
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(sDataShareHelper_, nullptr);
    // make sure board is empty
    ClearAllFile();

    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);

    MEDIA_INFO_LOG("MediaLibraryAssetHelperCapiTest::SetUpTestCase:: Finish");
}

void MediaLibraryAssetHelperCapiTest::TearDownTestCase(void)
{
    MEDIA_ERR_LOG("TearDownTestCase start");
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());

    if (sDataShareHelper_ != nullptr) {
        sDataShareHelper_->Release();
    }
    sleep(CLEAN_TIME);
    ClearAllFile();
    MEDIA_INFO_LOG("TearDownTestCase end");
}

void MediaLibraryAssetHelperCapiTest::SetUp(void)
{
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaLibraryAssetHelperCapiTest::TearDown(void) {}

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

void CallbackFunc(int32_t result, MediaLibrary_RequestId requestId)
{
    MEDIA_INFO_LOG("CallbackFunc::result: %{public}d", result);
    MEDIA_INFO_LOG("CallbackFunc::requestId: %{public}s", requestId.requestId);
}

void SetFileAssetInfo(shared_ptr<FileAsset> fileAsset, int32_t id, int32_t photoSubType, MediaType mediaType)
{
    if (fileAsset != nullptr) {
        fileAsset->SetId(id);
        fileAsset->SetPhotoSubType(photoSubType);
        fileAsset->SetMediaType(mediaType);
    }
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
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(assetPath), false,
        "create asset file failed, path=%{public}s", assetPath.c_str());
    return true;
}

static bool BuildFileManagerStoragePath(const std::string &assetPath, std::string &storagePath)
{
    std::string fileName = MediaFileUtils::GetFileName(assetPath);
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), false, "failed to get fileName from assetPath=%{public}s",
        assetPath.c_str());
    storagePath = FILE_MANAGER_TEST_DIR;
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

static bool ConvertAssetToFileManager(const std::string &uri)
{
    CHECK_AND_RETURN_RET_LOG(!uri.empty() && !FILE_MANAGER_TEST_DIR.empty(), false,
        "invalid input, uri empty: %{public}d, testDir empty: %{public}d", uri.empty(), FILE_MANAGER_TEST_DIR.empty());
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), false, "failed to parse fileId from uri: %{public}s", uri.c_str());
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

static std::string CreateFileManagerAsset(const std::string &displayName)
{
    std::string uri = mediaLibraryManager->CreateAsset(displayName);
    if (!uri.empty()) {
        EXPECT_TRUE(ConvertAssetToFileManager(uri));
    }
    return uri;
}

/**
 * @tc.name: media_library_capi_test_001
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is MEDIA_TYPE_IMAGE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_001, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_002
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is MEDIA_TYPE_VIDEO
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_002, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_003
 * @tc.desc: OH_MediaAssetChangeRequest_Create entry parameter is TYPE_MEDIALIBRARY
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_003, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_004
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer resourceType is MEDIA_LIBRARY_IMAGE_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_004, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_005
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer length is zero
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_005, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = 0;
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_006
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer GetPhotoSubType is CAMERA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_006, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = 0;
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_007
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithBuffer resourceType is MEDIA_LIBRARY_VIDEO_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_007, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    OH_MediaAccessHelper_ApplyChanges(changeRequest);
    uint32_t resultChange = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer,
        length);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_008
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_008, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_009
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is ADD_FILTERS
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_009, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_FILTERS;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_010
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_010, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_011
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_011, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_012
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_012, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_IMAGE_JPEG;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_013
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_013, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_014
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is ADD_FILTERS
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_014, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_FILTERS;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_015
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_015, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_016
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_016, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_017
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_017, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_018
 * @tc.desc: OH_MediaAssetChangeRequest_DiscardCameraPhoto changeRequest is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_018, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    uint32_t result = OH_MediaAssetChangeRequest_DiscardCameraPhoto(changeRequest);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_019
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeRequest is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_019, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_020
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_020, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_021
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_021, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_022
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is CREATE_FROM_URI
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_022, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_URI;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_023
 * @tc.desc: OH_MediaAssetChangeRequest_GetWriteCacheHandler changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_023, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    int32_t fd = 0;
    uint32_t resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(changeRequest, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(nullptr, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);
    changeRequest->request_ = nullptr;
    resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(changeRequest, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_file_manager_test_001, TestSize.Level1)
{
    string srcDisplayName = "request_image_src_fm.jpg";
    string destDisplayName = "request_image_dest_fm.jpg";
    string srcuri = CreateFileManagerAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    changeRequest->request_->RecordChangeOperation(changeOperation);
    ASSERT_NE(changeRequest, nullptr);

    int32_t fd = 0;
    uint32_t resultChange = OH_MediaAssetChangeRequest_GetWriteCacheHandler(changeRequest, &fd);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}
#endif


/**
 * @tc.name: media_library_capi_test_024
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri resourceType is MEDIA_LIBRARY_VIDEO_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_024, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_025
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri resourceType is MEDIA_LIBRARY_IMAGE_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_025, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_026
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri invalid file type
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_026, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(nullptr, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, nullptr);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
    changeRequest->request_ = nullptr;
    result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_027
 * @tc.desc: OH_MediaAssetChangeRequest_AddResourceWithUri GetPhotoSubType is CAMERA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_027, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    string destDisplayName = "request_image_dest.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    char* fileUri = strdup(destUri.c_str());
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithUri(changeRequest, resourceType, fileUri);
    EXPECT_EQ(result, MEDIA_LIBRARY_NO_SUCH_FILE);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_028
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_028, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_029
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_029, TestSize.Level1)
{
    string displayName = "image_test.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_030
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is ADD_RESOURCE and SAVE_CAMERA_PHOTO
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_030, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::SAVE_CAMERA_PHOTO;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    AssetChangeOperation changeOperation_ = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation_);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_031
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is ADD_RESOURCE return MEDIA_LIBRARY_PARAMETER_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_031, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_032
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is CAMERA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_032, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_033
 * @tc.desc: OH_MediaAccessHelper_ApplyChanges changeOperation is ADD_RESOURCE and CREATE_FROM_URI
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_033, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetId(DEFAULT_ID);
    fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    AssetChangeOperation changeOperation_ = AssetChangeOperation::CREATE_FROM_URI;
    changeRequest->request_->RecordChangeOperation(changeOperation_);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_034
 * @tc.desc: test OH_MediaAssetChangeRequest_Release when movingPhotoVideoDataBuffer_ is not empty
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_034, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);
    std::shared_ptr<MediaAssetChangeRequestImpl> impl = std::static_pointer_cast<MediaAssetChangeRequestImpl>
        (changeRequest->request_);

    impl->movingPhotoVideoDataBuffer_ = new uint8_t[10];
    EXPECT_EQ(OH_MediaAsset_Release(mediaAsset), MEDIA_LIBRARY_OK);
    EXPECT_EQ(OH_MediaAssetChangeRequest_Release(changeRequest), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: media_library_capi_test_035
 * @tc.desc: after inserting ADD_RESOURCE and GET_WRITE_CACHE_HANDLER changeOperation, test
 *           OH_MediaAssetChangeRequest_AddResourceWithBuffer, return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_035, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    EXPECT_EQ(OH_MediaAsset_Release(mediaAsset), MEDIA_LIBRARY_OK);
    EXPECT_EQ(OH_MediaAssetChangeRequest_Release(changeRequest), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: media_library_capi_test_036
 * @tc.desc: after inserting ADD_RESOURCE changeOperation, test OH_MediaAssetChangeRequest_AddResourceWithBuffer,
 *           return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_036, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    EXPECT_EQ(OH_MediaAsset_Release(mediaAsset), MEDIA_LIBRARY_OK);
    EXPECT_EQ(OH_MediaAssetChangeRequest_Release(changeRequest), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: media_library_capi_test_037
 * @tc.desc: after inserting GET_WRITE_CACHE_HANDLER changeOperation, test
 *           OH_MediaAssetChangeRequest_AddResourceWithBuffer, return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_037, TestSize.Level1)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t length = sizeof(buffer);
    MediaLibrary_ResourceType resourceType = MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE;
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_AddResourceWithBuffer(changeRequest, resourceType, buffer, length);
    EXPECT_EQ(result, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    EXPECT_EQ(OH_MediaAsset_Release(mediaAsset), MEDIA_LIBRARY_OK);
    EXPECT_EQ(OH_MediaAssetChangeRequest_Release(changeRequest), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: media_library_capi_test_038
 * @tc.desc: after inserting GET_WRITE_CACHE_HANDLER changeOperation, if cacheFileName_ and cacheMovingPhotoVideoName_
 *           is not empty, but sDataShareHelper_ is nullptr, test OH_MediaAccessHelper_ApplyChanges, return
 *           MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_038, TestSize.Level1)
{
    string srcDisplayName = "request_image_src.jpg";
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    SetFileAssetInfo(fileAsset, DEFAULT_ID, static_cast<int32_t>(PhotoSubType::CAMERA), OHOS::Media::MEDIA_TYPE_IMAGE);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);
    std::shared_ptr<MediaAssetChangeRequestImpl> impl = std::static_pointer_cast<MediaAssetChangeRequestImpl>
        (changeRequest->request_);

    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    impl->RecordChangeOperation(changeOperation);
    impl->cacheFileName_ = srcDisplayName;
    impl->cacheMovingPhotoVideoName_ = srcDisplayName;
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);

    EXPECT_EQ(OH_MediaAsset_Release(mediaAsset), MEDIA_LIBRARY_OK);
    EXPECT_EQ(OH_MediaAssetChangeRequest_Release(changeRequest), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: media_library_capi_test_039
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is MEDIA_LIBRARY_FILE_VIDEO,
 *           changeOperation is SET_EDIT_DATA
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_039, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_FILE_VIDEO;
    AssetChangeOperation changeOperation = AssetChangeOperation::SET_EDIT_DATA;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_040
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is MEDIA_LIBRARY_FILE_VIDEO,
 *           changeOperation is ADD_FILTERS
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_040, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_FILE_VIDEO;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_FILTERS;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_041
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is MEDIA_LIBRARY_FILE_VIDEO,
 *           changeOperation is CREATE_FROM_SCRATCH
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_041, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_FILE_VIDEO;
    AssetChangeOperation changeOperation = AssetChangeOperation::CREATE_FROM_SCRATCH;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_042
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is MEDIA_LIBRARY_FILE_VIDEO,
 *           changeOperation is GET_WRITE_CACHE_HANDLER
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_042, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_FILE_VIDEO;
    AssetChangeOperation changeOperation = AssetChangeOperation::GET_WRITE_CACHE_HANDLER;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_test_043
 * @tc.desc: OH_MediaAssetChangeRequest_SaveCameraPhoto imageFileType is MEDIA_LIBRARY_FILE_VIDEO,
 *           changeOperation is ADD_RESOURCE
 * @tc.type: FUNC
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_test_043, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_VIDEO);
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    ASSERT_NE(changeRequest, nullptr);

    MediaLibrary_ImageFileType imageFileType = MEDIA_LIBRARY_FILE_VIDEO;
    AssetChangeOperation changeOperation = AssetChangeOperation::ADD_RESOURCE;
    changeRequest->request_->RecordChangeOperation(changeOperation);
    uint32_t result = OH_MediaAssetChangeRequest_SaveCameraPhoto(changeRequest, imageFileType);
    EXPECT_EQ(result, MEDIA_LIBRARY_OK);
    uint32_t resultChange = OH_MediaAccessHelper_ApplyChanges(changeRequest);
    EXPECT_EQ(resultChange, MEDIA_LIBRARY_PARAMETER_ERROR);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

static std::shared_ptr<MediaAsset> CreateSharedMediaAsset(MediaType mediaType)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(mediaType);
    fileAsset->SetIsShared(static_cast<int32_t>(PhotoSharedType::SHARED));
    return MediaAssetFactory::CreateMediaAsset(fileAsset);
}

/**
 * @tc.name: media_library_capi_shared_001
 * @tc.desc: OH_MediaAssetChangeRequest_Create rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_001, TestSize.Level0)
{
    auto mediaAssetImpl = CreateSharedMediaAsset(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto* mediaAsset = new OH_MediaAsset(mediaAssetImpl);
    auto changeRequest = OH_MediaAssetChangeRequest_Create(mediaAsset);
    EXPECT_EQ(changeRequest, nullptr);

    OH_MediaAsset_Release(mediaAsset);
    OH_MediaAssetChangeRequest_Release(changeRequest);
}

/**
 * @tc.name: media_library_capi_shared_002
 * @tc.desc: AddResourceWithBuffer rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_002, TestSize.Level0)
{
    auto mediaAssetImpl = CreateSharedMediaAsset(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto request = std::make_shared<MediaAssetChangeRequestImpl>(mediaAssetImpl);

    uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t result = request->AddResourceWithBuffer(MediaLibrary_ResourceType::MEDIA_LIBRARY_IMAGE_RESOURCE,
        buffer, sizeof(buffer));
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
}

/**
 * @tc.name: media_library_capi_shared_003
 * @tc.desc: GetWriteCacheHandler rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_003, TestSize.Level0)
{
    auto mediaAssetImpl = CreateSharedMediaAsset(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto request = std::make_shared<MediaAssetChangeRequestImpl>(mediaAssetImpl);

    int32_t fd = 0;
    uint32_t result = request->GetWriteCacheHandler(&fd);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
}

/**
 * @tc.name: media_library_capi_shared_004
 * @tc.desc: SaveCameraPhoto rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_004, TestSize.Level0)
{
    auto mediaAssetImpl = CreateSharedMediaAsset(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto request = std::make_shared<MediaAssetChangeRequestImpl>(mediaAssetImpl);

    uint32_t result = request->SaveCameraPhoto(MEDIA_LIBRARY_IMAGE_JPEG);
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
}

/**
 * @tc.name: media_library_capi_shared_005
 * @tc.desc: DiscardCameraPhoto rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_005, TestSize.Level0)
{
    auto mediaAssetImpl = CreateSharedMediaAsset(OHOS::Media::MEDIA_TYPE_IMAGE);
    auto request = std::make_shared<MediaAssetChangeRequestImpl>(mediaAssetImpl);

    uint32_t result = request->DiscardCameraPhoto();
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
}

/**
 * @tc.name: media_library_capi_shared_006
 * @tc.desc: ApplyChanges rejects shared asset
 */
HWTEST_F(MediaLibraryAssetHelperCapiTest, media_library_capi_shared_006, TestSize.Level0)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetResultNapiType(OHOS::Media::ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetMediaType(OHOS::Media::MEDIA_TYPE_IMAGE);
    fileAsset->SetId(1);
    fileAsset->SetIsShared(static_cast<int32_t>(PhotoSharedType::SHARED));
    auto mediaAssetImpl = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto request = std::make_shared<MediaAssetChangeRequestImpl>(mediaAssetImpl);
    request->RecordChangeOperation(AssetChangeOperation::SAVE_CAMERA_PHOTO);

    uint32_t result = request->ApplyChanges();
    EXPECT_EQ(result, MEDIA_LIBRARY_PARAMETER_ERROR);
}
} // namespace Media
} // namespace OHOS
