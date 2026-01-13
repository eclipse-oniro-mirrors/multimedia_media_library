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

#include "medialibrary_ithumbnail_helper_test.h"

#include <thread>

#define private public
#include "thumbnail_service.h"
#include "ithumbnail_helper.h"
#include "thumbnail_generate_helper.h"
#undef private
#include "highlight_column.h"
#include "kvstore.h"
#include "media_file_utils.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_mocksinglekvstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_source_loading.h"
#include "vision_db_sqls.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
static int64_t g_id;
const string KV_STORE_DIR = "/data/medialibrary/database";
const int64_t DATE_TAKEN_TEST_VALUE = 1756111539577;
const int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;
const string TEST_IMAGE_PATH = "/storage/cloud/files/Photo/1/CreateImageThumbnailTest_001.jpg";
const string NO_HDR_NO_ROTATE_IMAGE_PATH = "/storage/cloud/files/Photo/1/NoHdrNoRotate.jpg";
const string HAS_HDR_NO_ROTATE_IMAGE_PATH = "/storage/cloud/files/Photo/1/HasHdrNoRotate.jpg";
const string NO_HDR_HAS_ROTATE_IMAGE_PATH = "/storage/cloud/files/Photo/1/NoHdrHasRotate.jpg";
const string HAS_HDR_HAS_ROTATE_IMAGE_PATH = "/storage/cloud/files/Photo/1/HasHdrHasRotate.jpg";
const string LARGE_IMAGE_PATH = "/data/local/tmp/hdr.jpg";
class TddRdbOpenCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override
    {
        return E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

static void InitRdbStore()
{
    const string dbPath = "/data/test/medialibrary_thumbnail_rdb_utils_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    TddRdbOpenCallback openCallback;

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, 1, openCallback);
    ASSERT_EQ(ret, E_OK);
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);

    ret = g_rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, DATE_TAKEN_TEST_VALUE);
    ret = g_rdbStore->Insert(g_id, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(ret, NativeRdb::E_OK);
}

static void DeleteRdbStore()
{
    string dropSql = "DROP TABLE IF EXIST " + PhotoColumn::PHOTOS_TABLE + ";";
    int32_t ret = g_rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop photos table ret: %{public}d", ret == NativeRdb::E_OK);
    MediaLibraryUnitTestUtils::StopUnistore();
}

void MediaLibraryIthumbnailHelperTest::SetUpTestCase(void)
{
    InitRdbStore();
    if (!MediaFileUtils::IsDirExists(KV_STORE_DIR)) {
        bool ret = MediaFileUtils::CreateDirectory(KV_STORE_DIR);
        ASSERT_EQ(ret, true);
    }
}

void MediaLibraryIthumbnailHelperTest::TearDownTestCase(void)
{
    DeleteRdbStore();
    MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
    if (MediaFileUtils::IsDirExists(KV_STORE_DIR)) {
        bool ret = MediaFileUtils::DeleteDir("/data/medialibrary");
        ASSERT_EQ(ret, true);
    }
}
    
void MediaLibraryIthumbnailHelperTest::SetUp() {}
    
void MediaLibraryIthumbnailHelperTest::TearDown(void) {}

static std::shared_ptr<PixelMap> CreateTestPixelMap(PixelFormat format, bool useDMA)
{
    InitializationOptions opts;
    opts.size.width = TEST_PIXELMAP_WIDTH_AND_HEIGHT;
    opts.size.height = TEST_PIXELMAP_WIDTH_AND_HEIGHT;
    opts.srcPixelFormat = format;
    opts.pixelFormat = format;
    opts.useDMA = useDMA;
    std::shared_ptr<PixelMap> pixelMap = PixelMap::Create(opts);
    return pixelMap;
}

static std::shared_ptr<Picture> CreateTestPicture(std::shared_ptr<PixelMap> pixelMap,
    std::shared_ptr<PixelMap> gainMap)
{
    if (pixelMap == nullptr) {
        return nullptr;
    }

    auto sourcePtr = Picture::Create(pixelMap);
    std::shared_ptr<Picture> picture = std::move(sourcePtr);
    if (gainMap == nullptr) {
        return picture;
    }

    Size gainMapSize = {gainMap->GetWidth(), gainMap->GetHeight()};
    auto auxiliaryPicturePtr = AuxiliaryPicture::Create(gainMap, AuxiliaryPictureType::GAINMAP, gainMapSize);
    std::shared_ptr<AuxiliaryPicture> auxiliaryPicture = std::move(auxiliaryPicturePtr);
    CHECK_AND_RETURN_RET_LOG(auxiliaryPicture != nullptr, nullptr, "Create auxiliaryPicture failed");
    picture->SetAuxiliaryPicture(auxiliaryPicture);
    return picture;
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, TrySaveCurrentPixelMap_test_001, TestSize.Level0)
{
    ThumbnailData data;
    data.id = "test_id";
    data.path = "test_path";
    data.dateModified = "test_date";
    ThumbnailType type = ThumbnailType::LCD;
    ThumbnailWait thumbnailWait(true);
    auto res = thumbnailWait.TrySaveCurrentPixelMap(data, type);
    EXPECT_EQ(res, false);
    type = ThumbnailType::THUMB;
    res = thumbnailWait.TrySaveCurrentPixelMap(data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, TrySaveCurrentPicture_test_001, TestSize.Level0)
{
    ThumbnailData data;
    data.id = "123";
    data.path = "/path/to/image";
    data.dateModified = "2025-04-14";
    bool isSourceEx = false;
    string tempOutputPath = "/path/to/temp";
    ThumbnailWait thumbnailWait(true);
    auto res = thumbnailWait.TrySaveCurrentPicture(data, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, UpdateSavedFileMap_test_001, TestSize.Level0)
{
    ThumbnailSyncStatus syncStatus;
    string id = "testId";
    ThumbnailType type = ThumbnailType::THUMB;
    string dateModified = "2025-04-14";
    bool isSourceEx = false;
    syncStatus.latestSavedFileMap_[id + "THM"] = "2025-04-15";
    auto res = syncStatus.UpdateSavedFileMap(id, type, dateModified);
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, TrySavePixelMap_test_001, TestSize.Level0)
{
    ThumbnailData data;
    ThumbnailType type = ThumbnailType::THUMB;
    auto res = IThumbnailHelper::TrySavePixelMap(data, type);
    EXPECT_EQ(res, false);
    data.needCheckWaitStatus = true;
    res = IThumbnailHelper::TrySavePixelMap(data, type);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, TrySavePicture_test_001, TestSize.Level0)
{
    ThumbnailData data;
    bool isSourceEx = false;
    const string tempOutputPath = "/path/to/temp";
    ThumbnailType type = ThumbnailType::THUMB;
    auto res = IThumbnailHelper::TrySavePicture(data, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
    data.needCheckWaitStatus = true;
    res = IThumbnailHelper::TrySavePicture(data, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, CacheSuccessState_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.id = "";
    opts.row = "";
    auto res = IThumbnailHelper::CacheSuccessState(opts, data);
    EXPECT_EQ(res, false);
    data.id = "validId";
    opts.row = "validRow";
    res = IThumbnailHelper::CacheSuccessState(opts, data);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = IThumbnailHelper::CacheSuccessState(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, StorePicture_test_001, TestSize.Level0)
{
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    bool ret = IThumbnailHelper::StorePicture(data, picture, false);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, StorePictureLowQuality_test_001, TestSize.Level0)
{
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    bool ret = IThumbnailHelper::StorePictureLowQuality(data, picture, false, LCD_UPLOAD_LIMIT_SIZE);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, GetLcdDesiredSize_test_001, TestSize.Level0)
{
    ThumbnailData data;
    data.lcdDesiredSize.height = 1080;
    data.lcdDesiredSize.width = 1920;
    data.orientation = 90;
    Size ret = IThumbnailHelper::GetLcdDesiredSize(data, true);
    bool res = ret.width == data.lcdDesiredSize.width && ret.height == data.lcdDesiredSize.height;
    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, DoCreatetLcdAndThumbnail_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("DoCreatetLcdAndThumbnail_test_001");
    ThumbRdbOpt opts;
    ThumbnailData data;
    opts.store = g_rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    data.id = std::to_string(g_id);
    data.path = TEST_IMAGE_PATH;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    
    IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcd,
        opts, data, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    bool ret = IThumbnailHelper::DoCreateLcdAndThumbnail(opts, data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("DoCreatetLcdAndThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, CreateLowQulityLcd_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_001");
    ThumbRdbOpt opts;
    ThumbnailData data;
    opts.store = g_rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    data.id = std::to_string(g_id);
    data.createLowQulityLcd = true;
    data.path = NO_HDR_NO_ROTATE_IMAGE_PATH;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;

    MediaFileUtils::DeleteDir(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX));
    
    bool ret = IThumbnailHelper::DoCreateLcd(opts, data);
    EXPECT_EQ(ret, true);
    size_t size;
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_001 end");
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, CreateLowQulityLcd_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_002");
    ThumbRdbOpt opts;
    ThumbnailData data;
    opts.store = g_rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    data.id = std::to_string(g_id);
    data.createLowQulityLcd = true;
    data.path = NO_HDR_HAS_ROTATE_IMAGE_PATH;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    data.orientation = 90;
    data.exifRotate = 6;
    data.mediaType = MediaType::MEDIA_TYPE_IMAGE;

    MediaFileUtils::DeleteDir(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbExDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_EX_SUFFIX));
    
    bool ret = IThumbnailHelper::DoCreateLcd(opts, data);
    EXPECT_EQ(ret, true);
    size_t size;
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD_EX, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_002 end");
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, CreateLowQulityLcd_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_003");
    ThumbRdbOpt opts;
    ThumbnailData data;
    opts.store = g_rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    data.id = std::to_string(g_id);
    data.createLowQulityLcd = true;
    data.path = HAS_HDR_NO_ROTATE_IMAGE_PATH;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    data.loaderOpts.isHdr = true;

    MediaFileUtils::DeleteDir(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX));
    
    bool ret = IThumbnailHelper::DoCreateLcd(opts, data);
    EXPECT_EQ(ret, true);
    size_t size;
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_003 end");
}

HWTEST_F(MediaLibraryIthumbnailHelperTest, CreateLowQulityLcd_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_004");
    ThumbRdbOpt opts;
    ThumbnailData data;
    opts.store = g_rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    data.id = std::to_string(g_id);
    data.createLowQulityLcd = true;
    data.path = HAS_HDR_HAS_ROTATE_IMAGE_PATH;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    data.orientation = 90;
    data.exifRotate = 6;
    data.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    data.loaderOpts.isHdr = true;

    MediaFileUtils::DeleteDir(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbnailDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX));
    MediaFileUtils::CreateDirectory(ThumbnailFileUtils::GetThumbExDir(data));
    MediaFileUtils::CopyFileUtil(LARGE_IMAGE_PATH, GetThumbnailPath(data.path, THUMBNAIL_LCD_EX_SUFFIX));
    
    bool ret = IThumbnailHelper::DoCreateLcd(opts, data);
    EXPECT_EQ(ret, true);
    size_t size;
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    ret = ThumbnailFileUtils::GetThumbFileSize(data, ThumbnailType::LCD_EX, size);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(size < LCD_UPLOAD_LIMIT_SIZE, true);
    MEDIA_INFO_LOG("CreateLowQulityLcd_test_004 end");
}

} // namespace Media
} // namespace OHOS