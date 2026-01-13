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
const int CLEAN_TIME = 5;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const int SCAN_WAIT_TIME = 10;
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
const std::string API_VERSION = "api_version";

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
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_ID,
    PhotoColumn::PHOTO_QUALITY,
    PhotoColumn::PHOTO_DIRTY,
    PhotoColumn::PHOTO_IS_TEMP,
    PhotoColumn::PHOTO_IS_AUTO,
    PhotoColumn::PHOTO_CE_AVAILABLE,
    PhotoColumn::STAGE_VIDEO_TASK_STATUS,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    PhotoColumn::PHOTO_BURST_KEY,
};

std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
MediaLibraryCameraManager* mediaLibraryManager = MediaLibraryCameraManager::GetMediaLibraryCameraManager();
static std::shared_ptr<MediaLibraryMockHapToken> hapToken = nullptr;

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
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(sDataShareHelper_, nullptr);

    // make sure board is empty
    ClearAllFile();

    Uri scanUri(URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
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

void mockToken(const std::vector<std::string>& perms, shared_ptr<MediaLibraryMockHapToken>& token)
{
    // mock tokenID
    token = make_shared<MediaLibraryMockHapToken>("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

// SetUp:Execute before each test case
void MediaLibraryCameraHelperTest::SetUp(void)
{
    // restore shell token before testcase
    uint64_t shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(shellToken);
    mockToken(perms, hapToken);
    system("rm -rf /storage/cloud/100/files/Photo/*");
}

void MediaLibraryCameraHelperTest::TearDown(void)
{
    // rescovery shell toekn after tesecase
    hapToken = nullptr;
    SetSelfTokenID(MediaLibraryMockTokenUtils::GetShellToken());
    MediaLibraryMockTokenUtils::ResetToken();
}

static std::shared_ptr<DataShareResultSet> QueryPhotoAsset(const std::shared_ptr<PhotoAssetProxy> &photoAssetProxy)
{
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoAssetProxy->fileId_);
    std::string uriStr = URI_QUERY_PHOTO;
    MediaUriUtils::AppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri queryFileUri(uriStr);
    std::vector<std::string> columns = QUERY_COLUMN;
    auto resultSet = sDataShareHelper_->Query(queryFileUri, predicates, columns);
    return resultSet;
}

static void SetBuffer(sptr<PhotoProxyTest> &photoProxy)
{
    int32_t rowDataSize = photoProxy->GetWidth();
    uint32_t bufferSize = rowDataSize * photoProxy->GetHeight();
    void *buffer = malloc(bufferSize);
    char *ch = static_cast<char *>(buffer);
    for (unsigned int i = 0; i < bufferSize; i++) {
        *(ch++) = (char)i;
    }
    photoProxy->fileDataAddr_ = buffer;
    photoProxy->fileSize_ = bufferSize;
}

/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_test01
 * @tc.desc: 获取photoAssetProxy对象, 已知CameraShotType, 可以获取到对应的subtype
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_test01");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, CameraShotType::MOVING_PHOTO);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::MOVING_PHOTO);
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_CreatePhotoAssetProxy_test01");
}

/**
 * @tc.name: MediaLibraryCameraManager_CreatePhotoAssetProxy_test02
 * @tc.desc: 获取photoAssetProxy对象, 未知CameraShotType, subtype_ 默认为 PhotoSubType::CAMERA
 */
HWTEST_F(MediaLibraryCameraHelperTest, MediaLibraryCameraManager_CreatePhotoAssetProxy_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter MediaLibraryCameraManager_CreatePhotoAssetProxy_test02");
    PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = 0,
        .userId = 0,
    };
    const int32_t INVALID_VALUE = 100;
    CameraShotType shottype = static_cast<CameraShotType>(INVALID_VALUE);
    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(callerInfo, shottype);
    ASSERT_NE(photoAssetProxy, nullptr);

    EXPECT_EQ(photoAssetProxy->subType_, PhotoSubType::CAMERA);
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_CreatePhotoAssetProxy_test02");
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
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_OpenAsset_test01");
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
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_OpenAsset_test02");
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
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_OpenAsset_test03");
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
    MEDIA_INFO_LOG("end MediaLibraryCameraManager_OpenAsset_test04");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test001
 * @tc.desc: [普通照片: YUV场景]
 *           [1] media_type = 1
 *           [2] subtype = 0
 *           [3] photo_id不为空 -> photo_id = xxx
 *           [4] 不支持云增强 -> ce_available = 0, is_auto = 0
 *           [5] photo_quality 为低质量 -> dirty = -1
 *           [6] is_temp = 1
 *           [7] 没有文件落盘
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test001");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test002
 * @tc.desc: [普通照片: YUV场景]
 *           [1] 支持手动云增强 -> ce_available = 1, is_auto = 0
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
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::NOT_AUTO));

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test002");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test003
 * @tc.desc: [普通照片: YUV场景]
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet),
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet),
        static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test003");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test101
 * @tc.desc: [普通照片: 非YUV场景]
 *           [1] fileDataAddr_ 为null -> size = 0
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test101");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test102
 * @tc.desc: [普通照片: 非YUV场景]
 *           [1] media_type = 1
 *           [2] subtype = 0
 *           [3] photo_quality 为低质量 -> dirty = -1
 *           [4] is_temp = 1
 *           [5] 有文件落盘(待处理)
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet), photoProxyTest->GetPhotoId());
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet), 1);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test102");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test201
 * @tc.desc: [动态照片: 只有YUV场景]
 *           [1] media_type = 1
 *           [2] subtype = 3
 *           [3] 需要二阶段视频 -> stage_video_task_status = 1
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    EXPECT_EQ(GetInt32Val(PhotoColumn::STAGE_VIDEO_TASK_STATUS, resultSet),
        static_cast<int32_t>(StageVideoTaskStatus::NEED_TO_STAGE));

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test201");
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_NE(resultSet->GoToNextRow(), NativeRdb::E_OK);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test301");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test302
 * @tc.desc: [连拍照片: 只有YUV场景]
 *           [1] media_type = 1
 *           [2] subtype = 4
 *           [3] burst_key 不能为null
 *           [4] 封面 -> burst_cover_level = 1
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
    photoProxyTest->extension_ = "jpg";

    const std::string burstKey = "test_xxxx_xxxx_xxxxxxxx_xxxx";
    photoProxyTest->burstKey_ = burstKey;
    photoProxyTest->isCoverPhoto_ = true;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::BURST));
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet), burstKey);
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::COVER));

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test302");
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet),
        static_cast<int32_t>(BurstCoverLevelType::MEMBER));

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test303");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test401
 * @tc.desc: [视频]
 *           [1] media_type = 2
 *           [2] subtype = 2
 *           [3] photo_quality 为低质量 -> dirty = -1
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
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::DEFAULT));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test401");
}

/**
 * @tc.name: PhotoAssetProxy_AddPhotoProxy_test501
 * @tc.desc: [电影模式]
 *           [1] media_type = 2
 *           [2] subtype = 5
 *           [3] photo_quality 一阶段是低质量
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
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet), static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet), static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO));
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);

    MEDIA_INFO_LOG("end PhotoAssetProxy_AddPhotoProxy_test501");
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test001
 * @tc.desc: [动态照片视频]
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test001");

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
    photoProxyTest->photoQuality_ = PhotoQuality::LOW;
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)photoProxyTest);

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    int32_t fd = photoAssetProxy->GetVideoFd(VideoType::ORIGIN_VIDEO);
    // value 符合预期
    EXPECT_EQ(fd > 0, true);

    write(fd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    close(fd);
    photoAssetProxy->NotifyVideoSaveFinished(VideoType::ORIGIN_VIDEO);

    MEDIA_INFO_LOG("end PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test001");
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test101
 * @tc.desc: [电影模式]
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test101, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test101");

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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    int32_t originFd = photoAssetProxy->GetVideoFd(VideoType::ORIGIN_VIDEO);
    // value 符合预期
    EXPECT_EQ(originFd > 0, true);

    write(originFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    close(originFd);
    photoAssetProxy->NotifyVideoSaveFinished(VideoType::ORIGIN_VIDEO);

    MEDIA_INFO_LOG("end PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test101");
}

/**
 * @tc.name: PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test102
 * @tc.desc: [电影模式]
 */
HWTEST_F(MediaLibraryCameraHelperTest, PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test102, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test102");

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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);

    int32_t effectFd = photoAssetProxy->GetVideoFd(VideoType::EFFECT_VIDEO);
    // value 符合预期
    EXPECT_EQ(effectFd > 0, true);

    write(effectFd, FILE_CONTENT_MP4, sizeof(FILE_CONTENT_MP4));
    close(effectFd);
    photoAssetProxy->NotifyVideoSaveFinished(VideoType::EFFECT_VIDEO);

    MEDIA_INFO_LOG("end PhotoAssetProxy_GetVideoFd_NotifyVideoSaveFinished_test102");
}

/**
 * @tc.name: PhotoAssetProxy_UpdatePhotoProxy_test001
 * @tc.desc: [先录后编]
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

    auto resultSet = QueryPhotoAsset(photoAssetProxy);
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

    auto resultSet2 = QueryPhotoAsset(photoAssetProxy);
    ASSERT_NE(resultSet2, nullptr);
    EXPECT_EQ(resultSet2->GoToNextRow(), NativeRdb::E_OK);

    // value符合预期
    EXPECT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet2), 1);
    EXPECT_EQ(GetStringVal(PhotoColumn::PHOTO_ID, resultSet2), photoId);

    MEDIA_INFO_LOG("end PhotoAssetProxy_UpdatePhotoProxy_test001");
}
} // namespace Media
} // namespace OHOS