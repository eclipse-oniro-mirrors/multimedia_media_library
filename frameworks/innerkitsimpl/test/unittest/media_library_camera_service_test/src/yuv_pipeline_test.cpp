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

#include "yuv_pipeline_test.h"

#include "camera_path_utils.h"
#include "camera_path_utils_test.h"
#include "camera_test_utils.h"
#include "media_assets_service.h"
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
#include "multistages_photo_capture_manager.h"
#include "save_camera_photo_dto.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const std::string HDR_PICTURE_PATH = "/data/local/tmp/test_jpeg.jpg";
static const int32_t FILE_ID_INPUT = 1;
static const std::string PHOTO_ID_INPUT = "1970_000000_0000_001";

const int32_t JPEG_IMAGE_FILE_TYPE = 1;
static const std::string JPEG_PATH = "/storage/cloud/files/Photo/16/IMG_1773407852_010.jpg";
static const std::string JPEG_EDIT_DATA_PATH = "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.jpg";
static const std::string JPEG_EDIT_DATA_CAMERA_PATH =
    "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.jpg/editdata_camera";
static const std::string JPEG_EDIT_DATA_SOURCE_PATH =
    "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.jpg/source.jpg";
static const std::string JPEG_MIME_TYPE = "image/jpeg";
static const std::string JPEG_DISPLAY_NAME = "IMG_20260313_211551818.jpg";
static const std::string JPEG_SUFFIX = "jpg";

static const std::string HEIF_PATH = "/storage/cloud/files/Photo/16/IMG_1773407852_010.heic";
static const std::string HEIF_EDIT_DATA_PATH = "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.heic";
static const std::string HEIF_EDIT_DATA_CAMERA_PATH =
    "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.heic/editdata_camera";
static const std::string HEIF_EDIT_DATA_SOURCE_PATH =
    "/storage/cloud/files/.editData/Photo/16/IMG_1773407852_010.heic/source.heic";
static const std::string HEIF_MIME_TYPE = "image/heic";
static const std::string HEIF_DISPLAY_NAME = "IMG_20260313_211551818.heic";
static const std::string HEIF_SUFFIX = "heic";

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
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        std::string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void YuvPipelineTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void YuvPipelineTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    // 关掉线程
    auto pictureManager = PictureManagerThread::GetInstance();
    EXPECT_NE(pictureManager, nullptr);
    PictureManagerThread::GetInstance()->Stop();

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("Clean is finish");
}

// SetUp:Execute before each test case
void YuvPipelineTest::SetUp()
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
}

void YuvPipelineTest::TearDown(void)
{
    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");

    auto pictureManager = PictureManagerThread::GetInstance();
    EXPECT_NE(pictureManager, nullptr);
    pictureManager->DeleteDataWithImageId(PHOTO_ID_INPUT, PictureType::LOW_QUALITY_PICTURE);
    pictureManager->DeleteDataWithImageId(PHOTO_ID_INPUT, PictureType::HIGH_QUALITY_PICTURE);

    MultistagesCameraCaptureManager::GetInstance().fileId2PhotoId_.clear();
    MultistagesCameraCaptureManager::GetInstance().pipeLinesMap_.clear();
}

static int32_t CreateAssetForYuvPipelineTest(const std::string& mimeType, bool isMovingPhoto, bool needIsTemp = false,
    bool isEdited = false)
{
    std::string path;
    std::string displayName;
    std::string suffix;
    if (mimeType == JPEG_MIME_TYPE) {
        path = JPEG_PATH;
        displayName = JPEG_DISPLAY_NAME;
        suffix = JPEG_SUFFIX;
    } else if (mimeType == HEIF_MIME_TYPE) {
        path = HEIF_PATH;
        displayName = HEIF_DISPLAY_NAME;
        suffix = HEIF_SUFFIX;
    }

    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.Put(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.Put(MediaColumn::MEDIA_MIME_TYPE, mimeType);
    valuesBucket.Put(PhotoColumn::PHOTO_MEDIA_SUFFIX, suffix);

    // common data(yuv场景下, 这些数据是必然的)
    valuesBucket.Put(PhotoColumn::PHOTO_ID, PHOTO_ID_INPUT);
    valuesBucket.Put(PhotoColumn::PHOTO_DIRTY, -1);
    valuesBucket.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    valuesBucket.Put(PhotoColumn::PHOTO_HEIGHT, 0);
    valuesBucket.Put(PhotoColumn::PHOTO_WIDTH, 0);
    valuesBucket.Put(MediaColumn::MEDIA_TIME_PENDING, UNCREATE_FILE_TIMEPENDING);

    int32_t subtype = isMovingPhoto ? static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)
                                    : static_cast<int32_t>(PhotoSubType::DEFAULT);
    valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, subtype);
    if (needIsTemp) {
        valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, 1);
    }
    if (isEdited) {
        valuesBucket.Put(PhotoColumn::PHOTO_EDIT_TIME, MediaTimeUtils::UTCTimeSeconds());
    }

    int64_t fileId = -1;
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CreateAssetForYuvPipelineTest, fileId: %{public}d.", static_cast<int32_t>(fileId));
    return static_cast<int32_t>(fileId);
}

static void InsertPictureToMap(int32_t fileId, bool isHighQualityPicture)
{
    auto picture = CameraTestUtils::CreatePictureByPixelMap("image/jpeg", HDR_PICTURE_PATH);
    ASSERT_NE(picture, nullptr);
    ASSERT_NE(picture->GetMainPixel(), nullptr);

    if (!isHighQualityPicture) {
        MultiStagesPhotoCaptureManager::GetInstance().DealLowQualityPicture(
            PHOTO_ID_INPUT, fileId, std::move(picture));
    } else {
        MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(
            PHOTO_ID_INPUT, fileId, std::move(picture));
    }

    auto threadManager = PictureManagerThread::GetInstance();
    EXPECT_NE(threadManager, nullptr);
    EXPECT_EQ(threadManager->IsExsitPictureByImageId(PHOTO_ID_INPUT), true);
}

static std::shared_ptr<NativeRdb::ResultSet> QueryForTestResult(int32_t fileId)
{
    MEDIA_INFO_LOG("QueryForTestResult, fileId: %{public}d.", fileId);
    static const std::vector<std::string> COLUMNS = {
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_SIZE,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_MIME_TYPE,
        MediaColumn::MEDIA_TIME_PENDING,
        PhotoColumn::PHOTO_DIRTY,
        PhotoColumn::PHOTO_HEIGHT,
        PhotoColumn::PHOTO_WIDTH,
        PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_QUALITY,
        PhotoColumn::PHOTO_MEDIA_SUFFIX,
    };

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    EXPECT_NE(g_rdbStore, nullptr);
    return g_rdbStore->Query(predicates, COLUMNS);
}

static bool CreateYuvPipeline(int32_t fileId, const std::string& mimeType)
{
    std::string path;
    std::string displayName;
    if (mimeType == JPEG_MIME_TYPE) {
        path = JPEG_PATH;
        displayName = JPEG_DISPLAY_NAME;
    } else if (mimeType == HEIF_MIME_TYPE) {
        path = HEIF_PATH;
        displayName = HEIF_DISPLAY_NAME;
    }

    FileAssetInfo assetInfo = {
        .fileId = fileId,
        .photoId = PHOTO_ID_INPUT,
        .path = path,
        .mimeType = mimeType,
        .displayName = displayName,
    };
    std::string pipelineType = std::to_string(static_cast<int32_t>(CameraPipelineType::YUV));
    return CameraTestUtils::InsertPipelineForConfirmType(pipelineType, assetInfo);
}

/**
 * @tc.name: [YUV场景: 一阶段添加水印] YuvPipeline_SaveEditDataCamera_test01
 * @tc.desc: 成功
 *           [1] 有水印文件
 *           [2] TakeEffectStatus = NEED_TAKE_EFFECT
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveEditDataCamera_test01, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveEditDataCamera_test01");

    // 1.创建一个YuvPipeline
    int32_t fileId = FILE_ID_INPUT;
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(fileId, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    EXPECT_EQ(pipeline->GetAssetInfo().GetTakeEffectStatus(), TakeEffectStatus::UNDEFINED);

    // 2.初始化相关入参
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket& values = cmd.GetValueBucket();
    values.Put(CONST_COMPATIBLE_FORMAT, COMPATIBLE_FORMAT);
    values.Put(CONST_FORMAT_VERSION, FORMAT_VERSION);
    values.Put(CONST_EDIT_DATA, EDIT_DATA_FOR_TEST);

    pipeline->SaveEditDataCamera(cmd, BUNDLE_NAME_CAMERA, "");

    // 3.value符合预期
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, JPEG_PATH, editDataCameraPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), true);
    EXPECT_EQ(pipeline->GetAssetInfo().GetTakeEffectStatus(), TakeEffectStatus::NEED_TAKE_EFFECT);
}

/**
 * @tc.name: [YUV场景: 一阶段添加水印] YuvPipeline_SaveEditDataCamera_test02
 * @tc.desc: 失败 -> editdata信息不符合规格
 *           [1] 没有水印文件
 *           [2] TakeEffectStatus = NO_NEED_TAKE_EFFECT
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveEditDataCamera_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveEditDataCamera_test02");

    // 1.创建一个YuvPipeline
    int32_t fileId = FILE_ID_INPUT;
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    CameraPipelineType typeByFileId = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(fileId, typeByFileId);
    ASSERT_NE(pipeline, nullptr);
    EXPECT_EQ(pipeline->GetAssetInfo().GetTakeEffectStatus(), TakeEffectStatus::UNDEFINED);

    // 2.初始化相关入参（editdata为空）
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket& values = cmd.GetValueBucket();
    values.Put(CONST_COMPATIBLE_FORMAT, COMPATIBLE_FORMAT);
    values.Put(CONST_FORMAT_VERSION, FORMAT_VERSION);
    values.Put(CONST_EDIT_DATA, "");

    pipeline->SaveEditDataCamera(cmd, BUNDLE_NAME_CAMERA, "");

    // 3.value符合预期
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, JPEG_PATH, editDataCameraPath);
    EXPECT_EQ(MediaFileUtils::IsFileExists(editDataCameraPath), false);
    EXPECT_EQ(pipeline->GetAssetInfo().GetTakeEffectStatus(), TakeEffectStatus::NO_NEED_TAKE_EFFECT);
}

/**
 * @tc.name: [JPG(无水印)场景: 正常落盘, 保存低质量picture] YuvPipeline_SaveCameraPhoto_test001（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = JPEG_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 jpg 相关
 *           [3] dirty = -1, quality = 1
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test001, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test001");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(低)
    InsertPictureToMap(fileId, false);

    // 1.4 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

/**
 * @tc.name: [JPG(无水印)场景: 正常落盘, 保存高质量picture] YuvPipeline_SaveCameraPhoto_test002（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = JPEG_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 jpg 相关
 *           [3] dirty = 1, quality = 0
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test002, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test002");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(高)
    InsertPictureToMap(fileId, true);

    // 1.4 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), static_cast<int32_t>(DirtyType::TYPE_NEW));
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

/**
 * @tc.name: [JPG(有水印)场景: 正常落盘, 保存低质量picture] YuvPipeline_SaveCameraPhoto_test003（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = JPEG_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 jpg 相关
 *           [3] dirty = -1, quality = 1
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test003, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test003");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(低)
    InsertPictureToMap(fileId, false);

    // 1.4 预置水印文件
    ASSERT_EQ(MediaFileUtils::CreateDirectory(JPEG_EDIT_DATA_PATH), true);
    CameraPathUtils::SaveEditDataCameraByString(JPEG_PATH, EDIT_DATA_FOR_TEST, BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_CAMERA_PATH), true);

    // 1.5 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_SOURCE_PATH), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_SOURCE_PATH), false);

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

/**
 * @tc.name: [JPG(有水印)场景: 正常落盘, 保存高质量picture] YuvPipeline_SaveCameraPhoto_test004（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = JPEG_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 jpg 相关
 *           [3] dirty = 1, quality = 0
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test004, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test004");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(高)
    InsertPictureToMap(fileId, true);

    // 1.4 预置水印文件
    ASSERT_EQ(MediaFileUtils::CreateDirectory(JPEG_EDIT_DATA_PATH), true);
    CameraPathUtils::SaveEditDataCameraByString(JPEG_PATH, EDIT_DATA_FOR_TEST, BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_CAMERA_PATH), true);

    // 1.5 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_SOURCE_PATH), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_SOURCE_PATH), false);

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), static_cast<int32_t>(DirtyType::TYPE_NEW));
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

/**
 * @tc.name: [HEIF(无水印)场景: 正常落盘, 保存低质量picture] YuvPipeline_SaveCameraPhoto_test101（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = HEIF_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 heif 相关
 *           [3] dirty = -1, quality = 1
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test101, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test101");

    // 1.1 数据库中预置一条数据(HEIF)
    int32_t fileId = CreateAssetForYuvPipelineTest(HEIF_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, HEIF_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(低)
    InsertPictureToMap(fileId, false);

    // 1.4 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 1.5 构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 2.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 3.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);   // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);    // heif 不存在

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

/**
 * @tc.name: [HEIF(无水印)场景: 正常落盘, 保存高质量picture] YuvPipeline_SaveCameraPhoto_test102（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = HEIF_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 heif 相关
 *           [3] dirty = 1, quality = 0
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test102, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test102");

    // 1.1 数据库中预置一条数据(HEIF)
    int32_t fileId = CreateAssetForYuvPipelineTest(HEIF_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, HEIF_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(高)
    InsertPictureToMap(fileId, true);

    // 1.4 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);   // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);    // heif 不存在

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), static_cast<int32_t>(DirtyType::TYPE_NEW));
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

/**
 * @tc.name: [HEIF(有水印)场景: 正常落盘, 保存低质量picture] YuvPipeline_SaveCameraPhoto_test103（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = HEIF_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 heif 相关
 *           [3] dirty = -1, quality = 1
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test103, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test103");

    // 1.1 数据库中预置一条数据(HEIF)
    int32_t fileId = CreateAssetForYuvPipelineTest(HEIF_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, HEIF_MIME_TYPE), true);

    // 1.3 在缓存中插入picture
    InsertPictureToMap(fileId, false);

    // 1.4 预置水印文件
    ASSERT_EQ(MediaFileUtils::CreateDirectory(HEIF_EDIT_DATA_PATH), true);
    CameraPathUtils::SaveEditDataCameraByString(HEIF_PATH, EDIT_DATA_FOR_TEST, BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_CAMERA_PATH), true);

    // 1.5 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_SOURCE_PATH), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_PATH), false);

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

/**
 * @tc.name: [HEIF(有水印)场景: 正常落盘, 保存高质量picture] YuvPipeline_SaveCameraPhoto_test104（该用例仅看护YUV落盘）
 * @tc.desc: imageFileType = HEIF_IMAGE_FILE_TYPE
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 heif 相关
 *           [3] dirty = 1, quality = 0
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test104, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test104");

    // 1.1 数据库中预置一条数据(HEIF)
    int32_t fileId = CreateAssetForYuvPipelineTest(HEIF_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, HEIF_MIME_TYPE), true);

    // 1.3 在缓存中插入picture(高)
    InsertPictureToMap(fileId, true);

    // 1.4 预置水印文件
    ASSERT_EQ(MediaFileUtils::CreateDirectory(HEIF_EDIT_DATA_PATH), true);
    CameraPathUtils::SaveEditDataCameraByString(HEIF_PATH, EDIT_DATA_FOR_TEST, BUNDLE_NAME_CAMERA);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_CAMERA_PATH), true);

    // 1.5 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_EDIT_DATA_SOURCE_PATH), true);
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_EDIT_DATA_PATH), false);

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), static_cast<int32_t>(DirtyType::TYPE_NEW));
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

/**
 * @tc.name: [异常流程] YuvPipeline_SaveCameraPhoto_test201
 * @tc.desc: imageFileType 不为空, 为异常值, 按照默认格式落盘(jpeg)。
 *           [1] 会有文件落盘: size > 0
 *           [2] path、display_name、mime_type、suffix 与 jpg 相关
 *           [3] dirty = -1, quality = 1
 *           [4] time_pending = 0
 *           [5] height、width 会重新刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test201, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test201");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 在缓存中插入picture
    InsertPictureToMap(fileId, false);

    // 1.4 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = 0,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), true);      // 保存为 jpg
    ASSERT_EQ(MediaFileUtils::IsFileExists(HEIF_PATH), false);     // heif 不存在

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet) > 0, true);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet), JPEG_PATH);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_NAME, resultSet), JPEG_DISPLAY_NAME);
    ASSERT_EQ(GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet), JPEG_MIME_TYPE);
    ASSERT_EQ(GetStringVal(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet), JPEG_SUFFIX);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet) > 0, true);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet), -1);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

/**
 * @tc.name: [异常流程] YuvPipeline_SaveCameraPhoto_test202
 * @tc.desc: 没有 picture, 则不会落盘。
 *           [1] 此时数据库保持原始状态, size = 0
 *           [2] SaveCameraPhoto 接口本身不知道是什么场景落盘, 接口本身返回成功
 *           [3] time_pending、height、width 不会刷新
 */
HWTEST_F(YuvPipelineTest, YuvPipeline_SaveCameraPhoto_test202, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter YuvPipeline_SaveCameraPhoto_test202");

    // 1.1 数据库中预置一条数据(JPG)
    int32_t fileId = CreateAssetForYuvPipelineTest(JPEG_MIME_TYPE, false);
    EXPECT_EQ(fileId > 0, true);

    // 1.2 创建一个YuvPipeline
    EXPECT_EQ(CreateYuvPipeline(fileId, JPEG_MIME_TYPE), true);

    // 1.3 预期校验
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);

    // 2.构造 SaveCameraPhotoDto 数据
    SaveCameraPhotoDto dto = {
        .fileId = fileId,
        .imageFileType = JPEG_IMAGE_FILE_TYPE,
    };

    // 3.执行
    int32_t ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    ASSERT_EQ(ret > 0, true);

    // 4.value符合预期
    ASSERT_EQ(MediaFileUtils::IsFileExists(JPEG_PATH), false);      // 没有文件落盘

    auto resultSet = QueryForTestResult(fileId);
    EXPECT_NE(resultSet, nullptr);
    int32_t err = resultSet->GoToFirstRow();
    EXPECT_EQ(err, NativeRdb::E_OK);

    ASSERT_EQ(GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet), 0);
    ASSERT_EQ(GetInt32Val(MediaColumn::MEDIA_TIME_PENDING, resultSet), UNCREATE_FILE_TIMEPENDING);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet), 0);
    ASSERT_EQ(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet), 0);
}
} // namespace Media
} // namespace OHOS