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
 
#include "image_pipeline_test.h"
 
#include "camera_path_utils.h"
#include "camera_path_utils_test.h"
#include "camera_test_utils.h"
#include "media_assets_service.h"
#include "media_camera_character_service.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "media_upgrade.h"
#include "media_uri_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#define private public
#define protected public
#include "multistages_camera_capture_manager.h"
#include "multistages_capture_deferred_photo_proc_session_callback.h"
#include "picture_adapter.h"
#undef private
#undef protected
#include "multistages_capture_request_task_manager.h"
#include "multistages_photo_capture_manager.h"
#include "save_camera_photo_dto.h"
#include "userfilemgr_uri.h"
 
namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

static const int32_t FILE_ID_INPUT = 1;
static const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";

static const std::string JPEG_MIME_TYPE = "image/jpeg";
static const std::string JPEG_DISPLAY_NAME = "IMG_20260313_211551818.jpg";
static const std::string JPEG_PATH = "/storage/cloud/files/Photo/16/IMG_1773407852_010.jpg";
static const std::string JPEG_EDIT_DATA_PATH = "/storage/cloud/100/files/.editData/Photo/16/IMG_1773407852_010.jpg/";
static const std::string JPEG_EDIT_DATA_SOURCE_PATH =
    "/storage/cloud/100/files/.editData/Photo/16/IMG_1773407852_010.jpg/source.jpg";
 
static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}
 
static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}
 
static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
 
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        std::string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}
 
void ImagePipelineTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}
 
void ImagePipelineTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
 
    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}
 
// SetUp:Execute before each test case
void ImagePipelineTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();

    // 文件
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/16"), true);

    // 水印
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/"), true);

    // 临时文件
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.cache/"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.cache/Photo/"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.cache/Photo/16/"), true);
}
 
void ImagePipelineTest::TearDown(void)
{
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.clear();
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.clear();
}

static int32_t CreateAssetForImagePipelineTest(const PhotoSubType& subtype, const MultiStagesPhotoQuality& quality,
    bool isEdited = false)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);

    // common data
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_FILE_PATH, JPEG_PATH);
    valuesBucket.Put(MediaColumn::MEDIA_NAME, JPEG_DISPLAY_NAME);
    valuesBucket.Put(MediaColumn::MEDIA_MIME_TYPE, JPEG_MIME_TYPE);
    valuesBucket.Put(MediaColumn::MEDIA_TYPE, to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)));
    valuesBucket.Put(PhotoColumn::PHOTO_ID, PHOTO_ID_INPUT);
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, 1);
    valuesBucket.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(quality));
    valuesBucket.Put(PhotoColumn::PHOTO_DIRTY, -1);
    valuesBucket.Put(MediaColumn::MEDIA_TIME_PENDING, UNCREATE_FILE_TIMEPENDING);
    valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(subtype));
    valuesBucket.Put(PhotoColumn::PHOTO_HEIGHT, 0);
    valuesBucket.Put(PhotoColumn::PHOTO_WIDTH, 0);
    if (isEdited) {
        valuesBucket.Put(PhotoColumn::PHOTO_EDIT_TIME, MediaTimeUtils::UTCTimeSeconds());
    }

    int64_t fileId = -1;
    int32_t ret = rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CreateAssetForYuvPipelineTest, fileId: %{public}d.", static_cast<int32_t>(fileId));
    return static_cast<int32_t>(fileId);
}

static bool CreateImagePipeline(int32_t fileId, CameraPipelineType pipelineType)
{
    FileAssetInfo assetInfo = {
        .fileId = fileId,
        .photoId = PHOTO_ID_INPUT,
        .path = JPEG_PATH,
    };
    std::string pipelineTypeStr = std::to_string(static_cast<int32_t>(pipelineType));
    return CameraTestUtils::InsertPipelineForConfirmType(pipelineTypeStr, assetInfo);
}

/**
 * @tc.name: [(旧)非YUV场景: 一阶段上报] ImagePipelineTest_CreateCameraFileFd_test001
 * @tc.desc: 成功 -> 如果缓存中存在 pipeline, 则可以在 cache 中创建路径
 *           [1] TEMP_LOW_PATH 对应的路径, 存在文件
 */
HWTEST_F(ImagePipelineTest, ImagePipelineTest_CreateCameraFileFd_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter ImagePipelineTest_CreateCameraFileFd_test001");

    // 1.创建一个NewImagePipeline
    int32_t fileId = FILE_ID_INPUT;
    EXPECT_EQ(CreateImagePipeline(fileId, CameraPipelineType::IMAGE), true);

    std::string tempPath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_LOW_PATH, JPEG_PATH, tempPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(tempPath), false);

    // 2.执行
    CreateCameraFileFdDto dto = {
        .fileId = fileId,
        .mode = MEDIA_FILEMODE_READWRITE,
        .pathType = static_cast<int32_t>(CameraPathType::TEMP_LOW_PATH),
    };

    CreateCameraFileFdRespBody respBody;
    int32_t ret = MediaCameraCharacterService::GetInstance().CreateCameraFileFd(dto, respBody);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(respBody.fd >= 0, true);

    // 3.value符合预期
    EXPECT_EQ(MediaFileUtils::IsFileExists(tempPath), true);

    close(respBody.fd);
}

/**
 * @tc.name: [(旧)非YUV场景: 一阶段上报] ImagePipelineTest_CloseCameraFileFd_test001
 * @tc.desc: 成功 -> 如果缓存中存在 pipeline, 效果图文件转正
 */
HWTEST_F(ImagePipelineTest, ImagePipelineTest_CloseCameraFileFd_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter ImagePipelineTest_CloseCameraFileFd_test001");

    // 1.1 创建一个NewImagePipeline
    int32_t fileId = FILE_ID_INPUT;
    EXPECT_EQ(CreateImagePipeline(fileId, CameraPipelineType::IMAGE), true);

    std::string tempPath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_LOW_PATH, JPEG_PATH, tempPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(tempPath), false);

    // 1.2 执行 CreateCameraFileFd
    CreateCameraFileFdDto dto = {
        .fileId = fileId,
        .mode = MEDIA_FILEMODE_READWRITE,
        .pathType = static_cast<int32_t>(CameraPathType::TEMP_LOW_PATH),
    };

    // 1.3 预期符合
    CreateCameraFileFdRespBody respBody;
    int32_t ret = MediaCameraCharacterService::GetInstance().CreateCameraFileFd(dto, respBody);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(respBody.fd >= 0, true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(tempPath), true);

    close(respBody.fd);

    // 2.执行
    ScanCameraFileDto closeDto = {
        .fileId = fileId,
        .pathType = static_cast<int32_t>(CameraPathType::TEMP_LOW_PATH),
    };
    ret = MediaCameraCharacterService::GetInstance().ScanCameraFile(closeDto);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(MediaFileUtils::IsFileExists(tempPath), false);

    std::string realPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDITED_PATH, JPEG_PATH, realPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(realPath), true);
}

/**
 * @tc.name: [(旧)非YUV场景: 一阶段落盘] ImagePipelineTest_SaveCameraPhoto_test001
 * @tc.desc: 如果没有水印文件, 则不需要执行落盘
 */
HWTEST_F(ImagePipelineTest, ImagePipelineTest_SaveCameraPhoto_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter ImagePipelineTest_SaveCameraPhoto_test001");

    // 1.1 数据库中预置一条数据(普通照片, 低质量)
    int32_t fileId = CreateAssetForImagePipelineTest(PhotoSubType::DEFAULT, MultiStagesPhotoQuality::LOW);
    EXPECT_EQ(fileId > 0, true);
    MEDIA_INFO_LOG("fileId: %{public}d.", fileId);

    // 1.2 创建一个NewImagePipeline
    EXPECT_EQ(CreateImagePipeline(fileId, CameraPipelineType::IMAGE), true);

    // 1.3 预置文件
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/Photo/16"), true);
    system("cp /data/local/tmp/test_jpeg.jpg /storage/cloud/files/Photo/16/IMG_1773407852_010.jpg");
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);

    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, JPEG_PATH, editDataSourcePath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataSourcePath), false);
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, JPEG_PATH, editDataCameraPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .photoSubType = static_cast<int32_t>(PhotoSubType::DEFAULT),
        .containsAddResource = true,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret, 1);

    // 4.value符合预期
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataSourcePath), false);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), false);
}

/**
 * @tc.name: [(旧)非YUV场景: 一阶段落盘] ImagePipelineTest_SaveCameraPhoto_test002
 * @tc.desc: 如果有水印文件, 则效果图落盘
 */
HWTEST_F(ImagePipelineTest, ImagePipelineTest_SaveCameraPhoto_test002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter ImagePipelineTest_SaveCameraPhoto_test002");

    // 1.1 数据库中预置一条数据(普通照片, 低质量)
    int32_t fileId = CreateAssetForImagePipelineTest(PhotoSubType::DEFAULT, MultiStagesPhotoQuality::LOW);
    EXPECT_EQ(fileId > 0, true);
    MEDIA_INFO_LOG("fileId: %{public}d.", fileId);

    // 1.2 创建一个NewImagePipeline
    EXPECT_EQ(CreateImagePipeline(fileId, CameraPipelineType::IMAGE), true);

    // 1.3 预置文件
    ASSERT_EQ(MediaFileUtils::IsFileExists("/storage/cloud/files/Photo/16"), true);
    system("cp /data/local/tmp/test_jpeg.jpg /storage/cloud/files/Photo/16/IMG_1773407852_010.jpg");
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);

    // 1.4 添加水印
    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(fileId, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    EXPECT_EQ(pipeline->GetAssetInfo().GetTakeEffectStatus(), TakeEffectStatus::UNDEFINED);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket& values = cmd.GetValueBucket();
    values.Put(CONST_COMPATIBLE_FORMAT, COMPATIBLE_FORMAT);
    values.Put(CONST_FORMAT_VERSION, FORMAT_VERSION);
    values.Put(CONST_EDIT_DATA, EDIT_DATA_FOR_TEST);
    pipeline->SaveEditDataCamera(cmd, BUNDLE_NAME_CAMERA, "");

    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, JPEG_PATH, editDataSourcePath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataSourcePath), false);
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, JPEG_PATH, editDataCameraPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), true);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .photoSubType = static_cast<int32_t>(PhotoSubType::DEFAULT),
        .containsAddResource = true,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret, 1);

    // 4.value符合预期
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataSourcePath), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), true);
}
} // namespace Media
} // namespace OHOS