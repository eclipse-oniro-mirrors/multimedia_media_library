/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "media_library_camera_helper_test.h"

#include <string>

#include "base_data_uri.h"
#include "datashare_helper.h"
#include "iservice_registry.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "media_time_utils.h"
#include "media_uri_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "photo_file_utils.h"
#include "photo_proxy_test.h"
#include "result_set_utils.h"
#include "system_ability_definition.h"
#include "userfilemgr_uri.h"

#define private public
#include "media_photo_asset_proxy.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace DataShare;
using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int32_t BYTES_PER_PIXEL = 4;
constexpr int32_t PIXEL_VALUE_MAX = 256;

const int CLEAN_TIME = 5;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const int SCAN_WAIT_TIME = 10;
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const std::string API_VERSION = "api_version";

const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";

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
    "ohos.permission.GET_BUNDLE_INFO",
    "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED"
};

const std::vector<std::string> QUERY_COLUMN = {
    MediaColumn::MEDIA_ID,
    MediaColumn::MEDIA_FILE_PATH,
    MediaColumn::MEDIA_SIZE,
    MediaColumn::MEDIA_TITLE,
    MediaColumn::MEDIA_NAME,
    MediaColumn::MEDIA_TYPE,
    MediaColumn::MEDIA_MIME_TYPE,
    MediaColumn::MEDIA_DATE_ADDED,
    MediaColumn::MEDIA_DATE_MODIFIED,
    MediaColumn::MEDIA_DATE_TAKEN,
    MediaColumn::MEDIA_DURATION,
    MediaColumn::MEDIA_TIME_PENDING,
    PhotoColumn::PHOTO_DIRTY,
    PhotoColumn::PHOTO_META_DATE_MODIFIED,
    PhotoColumn::PHOTO_HEIGHT,
    PhotoColumn::PHOTO_WIDTH,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_ID,
    PhotoColumn::PHOTO_QUALITY,
    PhotoColumn::PHOTO_DEFERRED_PROC_TYPE,
    PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
    PhotoColumn::PHOTO_IS_TEMP,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_BURST_KEY,
    PhotoColumn::PHOTO_CE_AVAILABLE,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID,
    PhotoColumn::STAGE_VIDEO_TASK_STATUS,
    PhotoColumn::PHOTO_IS_AUTO,
    PhotoColumn::PHOTO_MEDIA_SUFFIX,
};

std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
MediaLibraryCameraManager* mediaLibraryManager = MediaLibraryCameraManager::GetMediaLibraryCameraManager();
static MediaLibraryMockNativeToken* mockToken = nullptr;
static uint64_t g_shellToken = 0;
std::shared_ptr<MediaLowQualityMemoryCallback> callback_;

void CreateDataHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    ASSERT_NE(remoteObj, nullptr);

    mediaLibraryManager->InitMediaLibraryCameraManager(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryCameraManager success!");

    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        ASSERT_NE(sDataShareHelper_, nullptr);
    }
}

void ClearAllFile()
{
    system("rm -rf /storage/media/100/local/files/.thumbs/*");
    system("rm -rf /storage/cloud/100/files/Audio/*");
    system("rm -rf /storage/cloud/100/files/Audios/*");
    system("rm -rf /storage/cloud/100/files/Camera/*");
    system("rm -rf /storage/cloud/100/files/Documents/*");
    system("rm -rf /storage/cloud/100/files/Photo/*");
    system("rm -rf /storage/cloud/100/files/Pictures/*");
    system("rm -rf /storage/cloud/100/files/Download/*");
    system("rm -rf /storage/cloud/100/files/Videos/*");
    system("rm -rf /storage/cloud/100/files/.*");
    system("rm -rf /data/app/el2/100/database/com.ohos.medialibrary.medialibrarydata/*");
    system("kill -9 `pidof com.ohos.medialibrary.medialibrarydata`");
    system("scanner");
}

void MediaLibraryCameraHelperTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryCameraHelperTest::SetUpTestCase:: invoked");

    // 保存 shell token
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(sDataShareHelper_, nullptr);

    // make sure board is empty
    ClearAllFile();

    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    sDataShareHelper_->Insert(scanUri, valuesBucket);
    sleep(SCAN_WAIT_TIME);
    MEDIA_INFO_LOG("MediaLibraryCameraHelperTest::SetUpTestCase:: Finish");
}

void MediaLibraryCameraHelperTest::TearDownTestCase(void)
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

void MockNativeToken()
{
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("camera_service");  // 或其他SA进程名

    // 授予 native token 媒体权限
    for (auto perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(g_shellToken, perm, 0);
    }
}

// SetUp:Execute before each test case
void MediaLibraryCameraHelperTest::SetUp(void)
{
    // restore shell token before testcase
    MockNativeToken();
    system("rm -rf /storage/cloud/100/files/Photo/*");
    system("rm -rf /storage/cloud/100/files/.editData/*");
    system("rm -rf /storage/cloud/100/files/.thumbs/*");
}

void MediaLibraryCameraHelperTest::TearDown(void)
{
    // rescovery shell toekn after tesecase
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();
}

static std::shared_ptr<DataShareResultSet> QueryPhotoAsset(int32_t fileId)
{
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::string uriStr = CONST_URI_QUERY_PHOTO;
    MediaUriUtils::AppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri queryFileUri(uriStr);
    std::vector<std::string> columns = QUERY_COLUMN;
    auto resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    return resultSet;
}

static void SetBuffer(sptr<PhotoProxyTest> &photoProxy)
{
    int32_t bytesPerPixel = BYTES_PER_PIXEL;
    int32_t rowDataSize = photoProxy->GetWidth() * bytesPerPixel;
    uint32_t bufferSize = rowDataSize * photoProxy->GetHeight();
    if (bufferSize <= 0) {
        MEDIA_ERR_LOG("failed to SetBuffer.");
        return;
    }
    void *buffer = malloc(bufferSize);
    if (buffer == nullptr) {
        return;
    }
    uint8_t *ch = static_cast<uint8_t *>(buffer);
    for (unsigned int i = 0; i < bufferSize; ++i) {
        *(ch++) = static_cast<uint8_t>(i % PIXEL_VALUE_MAX);
    }
    photoProxy->fileDataAddr_ = buffer;
    photoProxy->fileSize_ = bufferSize;
}
 
static std::string GetPhotoIdForNumber(const std::string &title)
{
    stringstream result;
    for (size_t i = 0; i < title.length(); i++) {
        if (isdigit(title[i])) {
            result << title[i];
        }
    }
    return result.str();
}

/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test01
 * @tc.desc: 获取photoAssetProxy对象, 已知CameraShotType, 可以获取到对应的subtype
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test01");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    callerInfo.ToString();
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::MOVING_PHOTO);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::MOVING_PHOTO);
}

/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test02
 * @tc.desc: 获取photoAssetProxy对象, 未知CameraShotType, subtype_ 默认为 PhotoSubType::CAMERA
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_old_test02");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    const int32_t INVALID_VALUE = 100;
    CameraShotType shottype = static_cast<CameraShotType>(INVALID_VALUE);
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, shottype);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::CAMERA);
}

/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test01
 * @tc.desc: 获取photoAssetProxy对象, 已知CameraShotType, 可以获取到对应的subtype
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test01");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::MOVING_PHOTO,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };

    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::MOVING_PHOTO);
    EXPECT_EQ(photoAssetProxy->saveImageType_, SaveImageType::ONE_IMAGE);
    EXPECT_EQ(photoAssetProxy->saveVideoType_, SaveVideoType::ONE_VIDEO);
}
 
/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test02
 * @tc.desc: 获取photoAssetProxy对象, 未知CameraShotType, subtype_ 默认为 PhotoSubType::CAMERA
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_new_test02");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    const int32_t INVALID_VALUE = 100;
    CameraPresetPara presetPara = {
        .cameraShotType = static_cast<CameraShotType>(INVALID_VALUE),
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };

    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::CAMERA);
    EXPECT_EQ(photoAssetProxy->saveImageType_, SaveImageType::ONE_IMAGE);
    EXPECT_EQ(photoAssetProxy->saveVideoType_, SaveVideoType::ONE_VIDEO);
}

/**
 * @tc.name: MediaLibraryCameraManager_OpenAsset_test01
 * @tc.desc: uri 不能为空
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_OpenAsset_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_OpenAsset_test01");
    std::string uri = "";
    std::string openMode = MEDIA_FILEMODE_READWRITE;
    int32_t fd = mediaLibraryManager->OpenAsset(uri, openMode);
    EXPECT_EQ(fd, E_ERR);
}

/**
 * @tc.name: MediaLibraryCameraManager_OpenAsset_test02
 * @tc.desc: openMode 不能为空
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_OpenAsset_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_OpenAsset_test02");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->cloudImageEnhanceFlag_ = 0;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    std::string uri = photoAssetProxy->GetPhotoAssetUri();
    EXPECT_EQ(uri.empty(), false);

    std::string openMode = "";
    int32_t fd = mediaLibraryManager->OpenAsset(uri, openMode);
    EXPECT_EQ(fd, E_ERR);
}

/**
 * @tc.name: MediaLibraryCameraManager_OpenAsset_test03
 * @tc.desc: openMode 必须为有效值
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_OpenAsset_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_OpenAsset_test03");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->cloudImageEnhanceFlag_ = 0;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    std::string uri = photoAssetProxy->GetPhotoAssetUri();
    EXPECT_EQ(uri.empty(), false);

    std::string openMode = "test_value";
    int32_t fd = mediaLibraryManager->OpenAsset(uri, openMode);
    EXPECT_EQ(fd, E_ERR);
}

/**
 * @tc.name: MediaLibraryCameraManager_OpenAsset_test04
 * @tc.desc: openMode 必须为有效值
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_OpenAsset_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_OpenAsset_test04");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->cloudImageEnhanceFlag_ = 0;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    std::string uri = photoAssetProxy->GetPhotoAssetUri();
    EXPECT_EQ(uri.empty(), false);

    int32_t fd = mediaLibraryManager->OpenAsset(uri, MEDIA_FILEMODE_READWRITE);
    EXPECT_EQ(fd > 0, true);
}

/**
 * @tc.name: MediaLibraryCameraManager_GetDeferredPictureInfo_test01
 * @tc.desc: NEW_IMAGE 场景下才会保存 editData (格式需要正确: helper中不测试 editdata 的校验能力)
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_GetDeferredPictureInfo_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_GetDeferredPictureInfo_test01");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    // 1.构造 NEW_IMAGE 场景
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::JPG;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoId_ = PHOTO_ID_INPUT;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, nullptr, EDIT_DATA_STRING);

    // 2.获取
    DeferredPictureInfo pictureInfo = mediaLibraryManager->GetDeferredPictureInfo(PHOTO_ID_INPUT);

    // editData 符合预期
    std::string editData = pictureInfo.editData;
    bool ret = editData.find(CONST_APP_ID) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_COMPATIBLE_FORMAT) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_FORMAT_VERSION) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_EDIT_DATA) != std::string::npos;
    ASSERT_EQ(ret, true);

    // mimetype 符合预期
    ASSERT_EQ(pictureInfo.mimeType, "image/jpeg");
}

/**
 * @tc.name: MediaLibraryCameraManager_GetDeferredPictureInfo_test02
 * @tc.desc: YUV 场景下不会保存 editData
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_GetDeferredPictureInfo_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_GetDeferredPictureInfo_test02");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    // 1.构造 YUV 场景
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoId_ = PHOTO_ID_INPUT;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, nullptr, EDIT_DATA_STRING);

    // 2.获取
    DeferredPictureInfo pictureInfo = mediaLibraryManager->GetDeferredPictureInfo(PHOTO_ID_INPUT);

    // editData 符合预期
    std::string editData = pictureInfo.editData;
    ASSERT_EQ(editData, "");

    // mimetype 符合预期
    ASSERT_EQ(pictureInfo.mimeType, "image/jpeg");
}

/**
 * @tc.name: MediaLibraryCameraManager_GetDeferredPictureInfo_test03
 * @tc.desc: VIDEO 场景下不会保存 editData
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_GetDeferredPictureInfo_test03, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_GetDeferredPictureInfo_test03");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    // 1.构造 YUV 场景
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::VIDEO,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoId_ = "";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    // 2.获取
    DeferredPictureInfo pictureInfo = mediaLibraryManager->GetDeferredPictureInfo(PHOTO_ID_INPUT);

    // editData 符合预期
    std::string editData = pictureInfo.editData;
    ASSERT_EQ(editData, "");

    // mimetype 符合预期
    ASSERT_EQ(pictureInfo.mimeType, "image/jpeg");
}

/**
 * @tc.name: MediaLibraryCameraManager_GetDeferredPictureInfo_test04
 * @tc.desc: mimeType = heif
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_GetDeferredPictureInfo_test04, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_GetDeferredPictureInfo_test04");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };

    // 1.构造 NEW_IMAGE 场景
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::HEIF;
    photoProxyTest->extension_ = "heic";
    std::string photoId = "1970_000000_0000_002";
    photoProxyTest->photoId_ = photoId;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, nullptr, EDIT_DATA_STRING);

    // 2.获取
    DeferredPictureInfo pictureInfo = mediaLibraryManager->GetDeferredPictureInfo(photoId);

    // editData 符合预期
    std::string editData = pictureInfo.editData;
    bool ret = editData.find(CONST_APP_ID) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_COMPATIBLE_FORMAT) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_FORMAT_VERSION) != std::string::npos;
    ASSERT_EQ(ret, true);
    ret = editData.find(CONST_EDIT_DATA) != std::string::npos;
    ASSERT_EQ(ret, true);

    // mimetype 符合预期
    ASSERT_EQ(pictureInfo.mimeType, "image/heic");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test001
 * @tc.desc: [普通照片: YUV场景(低质量、不支持云增强)]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 0
 *           [12] photo_id 与输入一致
 *           [13] photo_quality 为低质量 -> dirty = -1
 *           [14] deferred_proc_type 与输入一致
 *           [15] is_temp = 1
 *           [16] 不支持云增强 -> ce_available = 0, is_auto = 0
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test001");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->cloudImageEnhanceFlag_ = 0;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test002
 * @tc.desc: [普通照片: YUV场景(高质量、支持手动云增强)]
 *           [1] photo_quality 为高质量 -> dirty = 1
 *           [2] is_temp = 1
 *           [3] 支持手动云增强 -> ce_available = 1, is_auto = 0
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test002");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    constexpr uint32_t MANUAL_ENHANCEMENT = 1;
    photoProxyTest->cloudImageEnhanceFlag_ = MANUAL_ENHANCEMENT;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test003
 * @tc.desc: [普通照片: YUV场景(支持自动云增强)]
 *           [1] 支持自动云增强 -> ce_available = 1, is_auto = 1
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test03");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    constexpr uint32_t AUTO_ENHANCEMENT = 1 << 1;
    photoProxyTest->cloudImageEnhanceFlag_ = AUTO_ENHANCEMENT;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test004
 * @tc.desc: [普通照片: YUV场景(异常场景)]
 *           [1] 如果PhotoProxy中的title为null, 则失败。
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test03");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->title_ = "";
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test001
 * @tc.desc: [普通照片: YUV场景(低质量、不支持云增强)] srcPhotoProxy 和 editData 不影响
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 0
 *           [12] photo_id 与输入一致
 *           [13] photo_quality 为低质量 -> dirty = -1
 *           [14] deferred_proc_type 与输入一致
 *           [15] is_temp = 1
 *           [16] 不支持云增强 -> ce_available = 0, is_auto = 0
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test001");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->cloudImageEnhanceFlag_ = 0;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test002
 * @tc.desc: [普通照片: YUV场景(高质量、支持手动云增强)] srcPhotoProxy 和 editData 不影响
 *           [1] photo_quality 为高质量 -> dirty = 1
 *           [2] is_temp = 1
 *           [3] 支持手动云增强 -> ce_available = 1, is_auto = 0
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test002");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    constexpr uint32_t MANUAL_ENHANCEMENT = 1;
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;
    photoProxyTest->cloudImageEnhanceFlag_ = MANUAL_ENHANCEMENT;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test003
 * @tc.desc: [普通照片: YUV场景(支持自动云增强)] srcPhotoProxy 和 editData 不影响
 *           [1] 支持自动云增强 -> ce_available = 1, is_auto = 1
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test003");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    constexpr uint32_t AUTO_ENHANCEMENT = 1 << 1;
    photoProxyTest->cloudImageEnhanceFlag_ = AUTO_ENHANCEMENT;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test004
 * @tc.desc: [普通照片: YUV场景(异常场景)] srcPhotoProxy 和 editData 不影响
 *           [1] 如果PhotoProxy中的title为null, 则失败。
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test004");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->title_ = "";
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test101
 * @tc.desc: [普通照片: 非YUV场景(不受扫描影响)] buffer 为 null
 *           [1] fileDataAddr_ 为null -> size = 0
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = 0 (openFile -> -1 变为 -2, 但是closeFd存在监控, 又更改为0)
 *           [10] meta_date_modified > 0
 *           [11] height = null, width = null
 *           [12] subtype = 0
 *           [13] photo_id 与输入一致
 *           [14] photo_quality 为低质量 -> dirty = -1
 *           [15] deferred_proc_type 与输入一致
 *           [16] is_temp = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test101, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test101");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::RGBA;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test102
 * @tc.desc: [普通照片: 非YUV场景(受扫描影响)] buffer 不为 null, PhotoFormat::RGBA
 *           [1] 有文件落盘, size > 0
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken > 0
 *           [8] date_modified > 0
 *           [9] time_pending = 0
 *           [10] meta_date_modified > 0
 *           [11] height、width 与输入一致
 *           [12] subtype = 0
 *           [13] photo_id 与输入一致
 *           [14] photo_quality 为低质量 -> dirty = -1
 *           [15] deferred_proc_type 与输入一致
 *           [16] is_temp = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test102, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test102");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::IMAGE);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::RGBA;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    SetBuffer(photoProxyTest);
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), photoProxyTest->GetHeight());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), photoProxyTest->GetWidth());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet),
        static_cast<int32_t>(photoProxyTest->GetPhotoQuality()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test101
 * @tc.desc: [普通照片: 非YUV场景(不受扫描影响)] buffer 为 null
 *           [1] fileDataAddr_ 为null -> size = 0
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = 0 (openFile -> -1 变为 -2, 但是closeFd存在监控, 又更改为0)
 *           [10] meta_date_modified > 0
 *           [11] height = null, width = null
 *           [12] subtype = 0
 *           [13] photo_id 与输入一致
 *           [14] photo_quality 为低质量 -> dirty = -1
 *           [15] deferred_proc_type 与输入一致
 *           [16] is_temp = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test101, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test101");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::RGBA;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test102
 * @tc.desc: [普通照片: 非YUV场景(受扫描影响、单图流)] buffer 不为 null, PhotoFormat::RGBA
 *           [1] 有文件落盘, size > 0
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken > 0
 *           [8] date_modified > 0
 *           [9] time_pending = 0
 *           [10] meta_date_modified > 0
 *           [11] height、width 与输入一致
 *           [12] subtype = 0
 *           [13] photo_id 与输入一致
 *           [14] photo_quality 为低质量 -> dirty = -1
 *           [15] deferred_proc_type 与输入一致
 *           [16] is_temp = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test102, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test102");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::RGBA;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    SetBuffer(photoProxyTest);
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), photoProxyTest->GetHeight());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), photoProxyTest->GetWidth());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet),
        static_cast<int32_t>(photoProxyTest->GetPhotoQuality()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test103
 * @tc.desc: [普通照片: 非YUV场景(受扫描影响、双图流)] buffer 不为 null, PhotoFormat::RGBA
 *           [1] 有文件落盘, size > 0
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken > 0
 *           [8] date_modified > 0
 *           [9] time_pending = 0
 *           [10] meta_date_modified > 0
 *           [11] height、width 与输入一致
 *           [12] subtype = 0
 *           [13] photo_id 与输入一致
 *           [14] photo_quality 为低质量 -> dirty = -1
 *           [15] deferred_proc_type 与输入一致
 *           [16] is_temp = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test103, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test103");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::IMAGE,
        .saveImageType = SaveImageType::TWO_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::RGBA;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    SetBuffer(photoProxyTest);
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), photoProxyTest->GetHeight());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), photoProxyTest->GetWidth());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet),
        static_cast<int32_t>(photoProxyTest->GetPhotoQuality()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet),
        static_cast<int32_t>(photoProxyTest->GetDeferredProcType()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test201
 * @tc.desc: [动态照片: 只有YUV场景]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 3
 *           [12] photo_id 与输入一致
 *           [13] photo_quality 为低质量 -> dirty = -1
 *           [14] is_temp = 1
 *           [15] 需要二阶段视频 -> stage_video_task_status = 1
 *           [16] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test201, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test201");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::MOVING_PHOTO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->stageVideoTaskStatus_ = 1;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::STAGE_VIDEO_TASK_STATUS, resultSet),
        static_cast<int32_t>(StageVideoTaskStatus::NEED_TO_STAGE));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test202
 * @tc.desc: [动态照片: 只有YUV场景] 如果没有调用NotifyVideoSaveFinished, 会降低规格变为普通照片
 *           [1] subtype = 0
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test202, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test202");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::MOVING_PHOTO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->stageVideoTaskStatus_ = static_cast<int32_t>(StageVideoTaskStatus::NO_NEED_TO_STAGE);
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    // 释放 photoAssetProxy
    int32_t fileId = photoAssetProxy->fileId_;
    photoAssetProxy = nullptr;

    auto resultSet = QueryPhotoAsset(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test201
 * @tc.desc: [动态照片: NEW_IMAGE下, YUV场景]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 3
 *           [12] photo_id 与输入一致
 *           [13] photo_quality 为低质量 -> dirty = -1
 *           [14] is_temp = 1
 *           [15] 需要二阶段视频 -> stage_video_task_status = 1
 *           [16] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test201, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test201");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::MOVING_PHOTO,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->stageVideoTaskStatus_ = 1;
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::STAGE_VIDEO_TASK_STATUS, resultSet),
        static_cast<int32_t>(StageVideoTaskStatus::NEED_TO_STAGE));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test202
 * @tc.desc: [动态照片: NEW_IMAGE下, YUV场景] 如果没有调用NotifyVideoSaveFinished, 会降低规格变为普通照片
 *           [1] subtype = 0
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test202, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test202");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::MOVING_PHOTO,
        .saveImageType = SaveImageType::ONE_IMAGE,
        .saveVideoType = SaveVideoType::ONE_VIDEO,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->stageVideoTaskStatus_ = static_cast<int32_t>(StageVideoTaskStatus::NO_NEED_TO_STAGE);
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    // 释放 photoAssetProxy
    int32_t fileId = photoAssetProxy->fileId_;
    photoAssetProxy = nullptr;

    auto resultSet = QueryPhotoAsset(fileId);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test301
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] burst_key 不能为null
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test301, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test301");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::BURST);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test302
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 4
 *           [12] photo_id = title的数字序列
 *           [13] photo_quality 为高质量 -> dirty = -1
 *           [14] is_temp = 1
 *           [15] burst_key 不能为null
 *           [16] 封面 -> burst_cover_level = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test302, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test302");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::BURST);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->title_ += "BURST001_COVER";
    photoProxyTest->photoId_ = "";
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;

    const std::string burstKey = "test_xxxx_xxxx_xxxxxxxx_xxxx";
    photoProxyTest->burstKey_ = burstKey;
    photoProxyTest->isCoverPhoto_ = true;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::BURST));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), GetPhotoIdForNumber(photoProxyTest->GetTitle()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet), burstKey);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::COVER));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test303
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] burst_key 不能为null
 *           [2] 非封面 -> burst_cover_level = 2
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test303, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test303");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::BURST);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    const std::string burstKey = "test_xxxx_xxxx_xxxxxxxx_xxxx";
    photoProxyTest->burstKey_ = burstKey;
    photoProxyTest->isCoverPhoto_ = false;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::MEMBER));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test301
 * @tc.desc: [连拍照片: NEW_IMAGE下, YUV场景]
 *           [1] burst_key 不能为null
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test301, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test301");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::BURST,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test302
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 1
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 4
 *           [12] photo_id = title的数字序列
 *           [13] photo_quality 为高质量 -> dirty = -1
 *           [14] is_temp = 1
 *           [15] burst_key 不能为null
 *           [16] 封面 -> burst_cover_level = 1
 *           [17] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test302, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test302");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::BURST,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->title_ += "BURST001_COVER";
    photoProxyTest->photoId_ = "";
    photoProxyTest->extension_ = "jpg";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;

    const std::string burstKey = "test_xxxx_xxxx_xxxxxxxx_xxxx";
    photoProxyTest->burstKey_ = burstKey;
    photoProxyTest->isCoverPhoto_ = true;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest, (sptr<PhotoProxy>&)photoProxyTest, "");

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".jpg"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "image/jpeg");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::BURST));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), GetPhotoIdForNumber(photoProxyTest->GetTitle()));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet), burstKey);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::COVER));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "jpg");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test303
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] burst_key 不能为null
 *           [2] 非封面 -> burst_cover_level = 2
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test303, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_NEW_IMAGE_test303");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    CameraPresetPara presetPara = {
        .cameraShotType = CameraShotType::BURST,
        .saveImageType = SaveImageType::ONE_IMAGE,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, presetPara);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->photoFormat_ = PhotoFormat::YUV;
    photoProxyTest->extension_ = "jpg";

    const std::string burstKey = "test_xxxx_xxxx_xxxxxxxx_xxxx";
    photoProxyTest->burstKey_ = burstKey;
    photoProxyTest->isCoverPhoto_ = false;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::MEMBER));
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test401
 * @tc.desc: [录像]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 2
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 0
 *           [12] photo_id = null
 *           [13] photo_quality 为高质量 -> dirty = 1
 *           [14] is_temp = 1
 *           [15] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test401, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test401");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoId_ = "";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".mp4"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "video/mp4");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), "");
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "mp4");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test501
 * @tc.desc: [电影模式]
 *           [1] size = null
 *           [2] title 存在
 *           [3] display_name = title + extension
 *           [4] media_type = 2
 *           [5] mime_type 与 extension 对应
 *           [6] date_added > 0
 *           [7] date_taken 与 date_added 相等 (单段式流程中, 需要)
 *           [8] date_modified = 0
 *           [9] time_pending = -1 (仅创建数据)
 *           [10] meta_date_modified = 0
 *           [11] subtype = 5
 *           [12] photo_id = null
 *           [13] photo_quality 为低质量 -> dirty = -1
 *           [14] is_temp = 1
 *           [15] media_suffix = extension
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_AddPhotoProxy_test501, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_AddPhotoProxy_test501");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::CINEMATIC_VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoId_ = "";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_TITLE, resultSet), photoProxyTest->GetTitle());
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), (photoProxyTest->GetTitle() + ".mp4"));
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    EXPECT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), "video/mp4");
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet) > 0, true);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet),
        GetInt64Val(MediaColumn::MEDIA_DATE_ADDED, resultSet));
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), "");
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(photoProxyTest->photoQuality_));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), "mp4");
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_test101
 * @tc.desc: [录像]
 *           [1] time_pending: -1 -> -2
 *           [2] meta_date_modified: null -> 时间戳
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_test101, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_test101");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoId_ = "";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);
    std::string uri = photoAssetProxy->GetPhotoAssetUri();

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // 初始值
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);

    int32_t originFd = photoAssetProxy->GetVideoFd(VideoType::ORIGIN_VIDEO);
    EXPECT_EQ(originFd > 0, true);

    resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // GetVideoFd 符合预期(字段)
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCLOSE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);

    close(originFd);
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_test201
 * @tc.desc: [电影模式]
 *           [1] time_pending: -1 -> -2
 *           [2] meta_date_modified: null -> 时间戳
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_test201, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_test201");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::CINEMATIC_VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // 初始值
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);

    int32_t originFd = photoAssetProxy->GetVideoFd(VideoType::ORIGIN_VIDEO);
    EXPECT_EQ(originFd > 0, true);

    resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // GetVideoFd 符合预期(字段)
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCLOSE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);

    close(originFd);
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_test202
 * @tc.desc: [电影模式]
 *           [1] time_pending: -1 -> -2
 *           [2] meta_date_modified: null -> 时间戳
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_test202, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_test202");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::CINEMATIC_VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // 初始值
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet), 0);

    int32_t effectFd = photoAssetProxy->GetVideoFd(VideoType::EFFECT_VIDEO);
    EXPECT_EQ(effectFd > 0, true);

    resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // GetVideoFd 符合预期(字段)
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCLOSE_FILE_TIMEPENDING);
    EXPECT_EQ(GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet) > 0, true);

    close(effectFd);
}

/**
 * @tc.name: PhotoAssetProxy_NotifyVideoSaveFinished_test202
 * @tc.desc: [电影模式: 源文件(editdata)]
 *           [1] size: null -> >0
 *           [2] date_modified: null -> 时间戳
 *           [3] time_pending: -2 -> 0
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_NotifyVideoSaveFinished_test202, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_NotifyVideoSaveFinished_test202");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::CINEMATIC_VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    // 初始值
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet), 0);
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);

    int32_t fd = photoAssetProxy->GetVideoFd(VideoType::ORIGIN_VIDEO);
    write(fd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    close(fd);
    photoAssetProxy->NotifyVideoSaveFinished(VideoType::ORIGIN_VIDEO);

    resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
}

/**
 * @tc.name: PhotoAssetProxy_UpdatePhotoProxy_test001
 * @tc.desc: [录像]
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_UpdatePhotoProxy_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_UpdatePhotoProxy_test001");

    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::VIDEO);
    ASSERT_NE(photoAssetProxy, nullptr);

    sptr<PhotoProxyTest> photoProxyTest = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest, nullptr);
    photoProxyTest->extension_ = "mp4";
    photoProxyTest->photoQuality_ = PhotoQuality::HIGH;
    photoProxyTest->photoId_ = "";
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), 0);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), "");

    sptr<PhotoProxyTest> photoProxyTest2 = new(std::nothrow) PhotoProxyTest();
    ASSERT_NE(photoProxyTest2, nullptr);
    photoProxyTest2->photoQuality_ = PhotoQuality::LOW;
    std::string photoId = photoProxyTest2->GetPhotoId();
    photoAssetProxy->UpdatePhotoProxy((sptr<PhotoProxy>&)photoProxyTest2);

    auto resultSet2 = QueryPhotoAsset(photoAssetProxy->fileId_);
    ASSERT_NE(resultSet2, nullptr);
    EXPECT_EQ(resultSet2->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet2), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet2), photoId);
}
} // namespace Media
} // namespace OHOS