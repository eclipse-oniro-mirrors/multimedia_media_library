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

#include "medialibrary_thumb_test.h"
#include "medialibrary_thumbnail_image_framework_test.h"

#include "image_source.h"

#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include <thread>

#include "foundation/ability/form_fwk/test/mock/include/mock_single_kv_store.h"
#include "highlight_column.h"
#include "kvstore.h"
#include "vision_db_sqls.h"

#include "exif_rotate_utils.h"
#define private public
#include "thumbnail_service.h"
#include "ithumbnail_helper.h"
#include "thumbnail_generate_helper.h"
#include "thumbnail_image_framework_utils.h"
#undef private
#include "thumbnail_highlight_utils.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
  
using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
namespace OHOS {
namespace Media {

const int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;
const string TEST_HDR_JPG = "/data/local/tmp/hdr.jpg";

void MediaLibraryThumbTest::SetUpTestCase(void) {}
 
void MediaLibraryThumbTest::TearDownTestCase(void) {}
     
void MediaLibraryThumbTest::SetUp() {}
     
void MediaLibraryThumbTest::TearDown(void) {}

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

static std::shared_ptr<Picture> CreateTestPicture(std::shared_ptr<PixelMap> pixelMap, std::shared_ptr<PixelMap> gainMap)
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
 
HWTEST_F(MediaLibraryThumbTest, LoadAudioFileInfo_test_001, TestSize.Level0)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = nullptr;
    ThumbnailData data;
    Size desiredsize;
    uint32_t errCode;
    auto res = ThumbnailUtils::LoadAudioFileInfo(avMetadataHelper, data, desiredsize, errCode);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, LoadVideoFrame_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size desiredsize;
    int64_t timeStamp = 0;
    data.path = "test_path";
    auto res = ThumbnailUtils::LoadVideoFrame(data, desiredsize, timeStamp);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, GenTargetPixelmap_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size desiredsize;
    auto res = ThumbnailUtils::GenTargetPixelmap(data, desiredsize);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, SaveAfterPacking_test_001, TestSize.Level0)
{
    ThumbnailData data;
    data.path = "test_path";
    bool isSourceEx = false;
    string tempOutputPath = "temp_outout_path";
    Size desiredsize;
    auto res = ThumbnailUtils::SaveAfterPacking(data, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, QueryLcdCount_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    int outLcdCount = 0;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, QueryLcdCountByTime_test_001, TestSize.Level0)
{
    int64_t time = 12345467890;
    bool before = true;
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    int outLcdCount = 0;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, QueryLocalNoLcdInfos_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    std::vector<ThumbnailData> infos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLocalNoLcdInfos(opts, infos, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailUtils::QueryLocalNoLcdInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, QueryNoHighlightPath_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.id = "123";
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailHighlightUtils::QueryNoHighlightPath(opts, data, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailHighlightUtils::QueryNoHighlightPath(opts, data, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, QueryNoHighlightInfos_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    std::vector<ThumbnailData> infos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailHighlightUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailHighlightUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, GetHighlightTracks_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    opts.row = "123";
    std::vector<int> trackInfos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailHighlightUtils::GetHighlightTracks(opts, trackInfos, err);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailHighlightUtils::GetHighlightTracks(opts, trackInfos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, UpdateHighlightInfo_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    ThumbnailData data;
    data.id = "12345";
    data.tracks = "track_data";
    opts.store = nullptr;
    auto res = ThumbnailHighlightUtils::UpdateHighlightInfo(opts, data);
    EXPECT_EQ(res, E_ERR);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailHighlightUtils::UpdateHighlightInfo(opts, data);
    EXPECT_EQ(res, E_ERR);
}

HWTEST_F(MediaLibraryThumbTest, CheckDateTaken_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    ThumbnailData data;
    data.dateTaken = "";
    data.id = "123";
    opts.store = nullptr;
    auto res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
    opts.store = ThumbnailService::GetInstance()->rdbStorePtr_;
    res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, GetLocalThumbSize_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size size;
    ThumbnailType type = ThumbnailType::THUMB;
    data.path = "/path/to/image.jpg";
    auto res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
    type = ThumbnailType::THUMB_ASTC;
    res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
    type = ThumbnailType::LCD;
    res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, ConvertStrToInt32_test_001, TestSize.Level0)
{
    string str = "123";
    int32_t ret;
    auto res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, true);
    str = "test";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
    str = "-100000000000000";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
    str = "100000000000000";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, CreateOutputPath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    auto res = ThumbnailUtils::CreateOutputPath(data, THUMBNAIL_LCD_SUFFIX);
    EXPECT_EQ(res, "");
    data.tracks = "tracks";
    res = ThumbnailUtils::CreateOutputPath(data, THUMBNAIL_LCD_SUFFIX);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbTest, IsExCloudThumbnail_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001");
    ThumbnailData data;
    data.orientation = 1;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbTest, IsExCloudThumbnail_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_002");
    ThumbnailData data;
    data.exifRotate = 1;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbTest, IsExCloudThumbnail_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_003");
    ThumbnailData data;
    data.exifRotate = 2;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_003 end");
}

HWTEST_F(MediaLibraryThumbTest, NeedRotateThumbnail_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_001");
    ThumbnailData data;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbTest, NeedRotateThumbnail_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_002");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    data.lastLoadSource = SourceState::LOCAL_THUMB;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_002 end");
}

HWTEST_F(MediaLibraryThumbTest, NeedRotateThumbnail_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_003");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    data.lastLoadSource = SourceState::LOCAL_ORIGIN;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_003 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_001");
    ThumbnailData data;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_001 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_002");
    ThumbnailData data;
    data.mediaType = 2;
    data.exifRotate = 2;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_002 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_003");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 1;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_003 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_004");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 0;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_004 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_005");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsImageWithRotate_test_005 end");
}

HWTEST_F(MediaLibraryThumbTest, IsImageWithRotate_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_005");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 0;
    data.orientation = 90;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsImageWithRotate_test_005 end");
}

HWTEST_F(MediaLibraryThumbTest, IsUseRotatedSource_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsUseRotatedSource_test_001");
    ThumbnailData data;
    data.lastLoadSource = SourceState::LOCAL_THUMB;
    auto ret = ThumbnailUtils::IsUseRotatedSource(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsUseRotatedSource_test_001 end");
}

static const string HDR_PICTURE_PATH = "/storage/media/local/files/Photo/1/HDR_picture.jpg";

static unique_ptr<ImageSource> CreateTestImageSource(const std::string &path)
{
    MEDIA_INFO_LOG("file: %{public}s exist: %{public}d", path.c_str(), MediaFileUtils::IsFileExists(path));
    uint32_t err;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, err);
    CHECK_AND_PRINT_LOG(err == E_OK, "ImageSource::CreateImageSource err: %{public}d", err);
    return imageSource;
}

static shared_ptr<Picture> CreateTestPicture()
{
    unique_ptr<ImageSource> imageSource = CreateTestImageSource(HDR_PICTURE_PATH);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "ImageSource::CreateImageSource failed");

    DecodingOptionsForPicture pictureOpts;
    pictureOpts.desireAuxiliaryPictures = {AuxiliaryPictureType::GAINMAP};
    uint32_t err;
    auto picture = imageSource->CreatePicture(pictureOpts, err);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr, "ImageSource->CreatePicture err: %{public}d", err);
    return picture;
}

static std::shared_ptr<PixelMap> CreateTestPixelMap()
{
    auto picture = CreateTestPicture();
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr, "CreateTestPicture() failed");

    return picture->GetMainPixel();
}

static std::shared_ptr<PixelMap> CreateTestHdrPixelMap(bool isYuv)
{
    unique_ptr<ImageSource> imageSource = CreateTestImageSource(TEST_HDR_JPG);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "ImageSource::CreateImageSource failed");

    DecodeOptions decodeOpts;
    decodeOpts.desiredDynamicRange = DecodeDynamicRange::AUTO;
    if (isYuv) {
        decodeOpts.photoDesiredPixelFormat = PixelFormat::YCBCR_P010;
    }
    uint32_t errorCode = 0;
    unique_ptr<PixelMap> pixelMapPtr = imageSource->CreatePixelMap(decodeOpts, errorCode);
    CHECK_AND_RETURN_RET_LOG(errorCode == 0 && pixelMapPtr != nullptr, nullptr,
        "Failed to create pixelMap, err:%{public}d", errorCode);

    std::shared_ptr<PixelMap> pixelMap = std::move(pixelMapPtr);
    return pixelMap;
}

HWTEST_F(MediaLibraryThumbTest, IsYuvPixelMap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = nullptr;
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, false);
}

HWTEST_F(MediaLibraryThumbTest, IsYuvPixelMap_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, true);
}

HWTEST_F(MediaLibraryThumbTest, IsYuvPixelMap_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, false);
}

HWTEST_F(MediaLibraryThumbTest, CopyPictureSource_test_001, TestSize.Level0)
{
    std::shared_ptr<Picture> picture = nullptr;
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_EQ(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPictureSource_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    std::shared_ptr<PixelMap> gainMap = nullptr;
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_EQ(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPictureSource_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_NE(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPixelMapSource_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = nullptr;
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_EQ(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPixelMapSource_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPixelMapSource_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyNormalPixelmap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyNormalPixelmap(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyYuvPixelmap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(pixelMap->GetAllocatorType() == AllocatorType::DMA_ALLOC, false);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyYuvPixelmap(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyPixelMapSource_test_004, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::YCRCB_P010, true);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_EQ(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, IsSupportCopyPixelMap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = nullptr;
    EXPECT_EQ(ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap), false);

    pixelMap = CreateTestPixelMap(PixelFormat::YCRCB_P010, true);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap), false);

    pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, true);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap), true);

    pixelMap = CreateTestPixelMap(PixelFormat::NV21, true);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(pixelMap), true);
}

HWTEST_F(MediaLibraryThumbTest, CopySurfaceBufferInfo_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> source = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> dst = SurfaceBuffer::Create();
    ThumbnailImageFrameWorkUtils::CopySurfaceBufferInfo(source, dst);
    EXPECT_NE(source, nullptr);
    EXPECT_NE(dst, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, GetSbStaticMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 1;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::GetSbStaticMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, GetSbDynamicMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 2;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::GetSbDynamicMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, SetSbStaticMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 3;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::SetSbStaticMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, SetSbDynamicMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 4;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::SetSbDynamicMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, SetPixelMapYuvInfo_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV12, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, SetPixelMapYuvInfo_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, SetPixelMapYuvInfo_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::BGRA_8888, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, IsPictureValid_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    bool ret = ThumbnailImageFrameWorkUtils::IsPictureValid(picture);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, IsPixelMapValid_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> PixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    bool ret = ThumbnailImageFrameWorkUtils::IsPixelMapValid(PixelMap);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbTest, CopyAndScalePicture_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    Size desiredSize = { TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2, TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2 };
    auto ret = ThumbnailImageFrameWorkUtils::CopyAndScalePicture(picture, desiredSize);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, CopyAndScalePixelMap_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    Size desiredSize = { TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2, TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2 };
    auto ret = ThumbnailImageFrameWorkUtils::CopyAndScalePixelMap(pixelMap, desiredSize);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryThumbTest, FlipAndRotatePicture_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePicture_001");
    auto picture = CreateTestPicture();
    int32_t exifRotate = 2;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(picture, exifRotate);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePicture_001 end");
}

HWTEST_F(MediaLibraryThumbTest, FlipAndRotatePicture_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePicture_002");
    auto picture = CreateTestPicture();
    FlipAndRotateInfo info;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(picture, info);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePicture_002 end");
}

HWTEST_F(MediaLibraryThumbTest, FlipAndRotatePixelMap_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_001");
    auto pixelMap = CreateTestPixelMap();
    int32_t exifRotate = 2;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, exifRotate);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_001 end");
}

HWTEST_F(MediaLibraryThumbTest, FlipAndRotatePixelMap_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_002");
    auto pixelMap = CreateTestPixelMap();
    FlipAndRotateInfo info;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, info);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_002 end");
}

HWTEST_F(MediaLibraryThumbTest, ConvertPixelMapToSdrAndFormatRGBA8888_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_001");
    auto pixelMap = CreateTestHdrPixelMap(true);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(pixelMap->IsHdr(), true);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->IsHdr(), false);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_001 end");
}

HWTEST_F(MediaLibraryThumbTest, ConvertPixelMapToSdrAndFormatRGBA8888_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_002");
    auto pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_002 end");
}

HWTEST_F(MediaLibraryThumbTest, ConvertPixelMapToSdrAndFormatRGBA8888_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_003");
    auto pixelMap = CreateTestPixelMap(PixelFormat::NV12, true);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_003 end");
}

HWTEST_F(MediaLibraryThumbTest, ConvertPixelMapToSdrAndFormatRGBA8888_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_004");
    auto pixelMap = CreateTestHdrPixelMap(false);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_004 end");
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_GetLocalKeyFrame_test_002, TestSize.Level0)
{
    std::string path = ROOT_MEDIA_DIR + "test";
    std::string key = "";
    std::string timeStamp = "";

    std::string res = OHOS::Media::GetLocalKeyFrameThumbnailPath(path, key, timeStamp);

    std::string suffix = (key == "") ? "" : "/" + key + ".jpg";
    std::string expectRes = LOCAL_MEDIA_PATH + ((key == "") ? "" : ".thumbs/") +
        path.substr(ROOT_MEDIA_DIR.length()) + "/beginTimeStamp" + timeStamp + "/" + suffix;

    EXPECT_EQ(res, expectRes);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_NeedAutoResize_test_001, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD + 1;
    size.height = SHORT_SIDE_THRESHOLD + 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_NeedAutoResize_test_002, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD - 1;
    size.height = SHORT_SIDE_THRESHOLD + 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_NeedAutoResize_test_003, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD + 1;
    size.height = SHORT_SIDE_THRESHOLD - 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_GenDecodeOpts_test_001, TestSize.Level0)
{
    Size sourceSize;
    Size targetSize;
    DecodeOptions decodeOptions;
    sourceSize.width = 0;

    bool res = OHOS::Media::GenDecodeOpts(sourceSize, targetSize, decodeOptions);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_GenDecodeOpts_test_002, TestSize.Level0)
{
    Size sourceSize;
    Size targetSize;
    DecodeOptions decodeOptions;
    sourceSize.width = 1;
    targetSize.width = SHORT_SIDE_THRESHOLD - 1;
    targetSize.height = SHORT_SIDE_THRESHOLD - 1;

    bool res = OHOS::Media::GenDecodeOpts(sourceSize, targetSize, decodeOptions);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_GenDecodeOpts_test_003, TestSize.Level0)
{
    Size sourceSize;
    Size targetSize;
    DecodeOptions decodeOptions;
    sourceSize.width = 1;
    targetSize.width = SHORT_SIDE_THRESHOLD + 1;
    targetSize.height = SHORT_SIDE_THRESHOLD + 1;

    bool res = OHOS::Media::GenDecodeOpts(sourceSize, targetSize, decodeOptions);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_ConvertDecodeSize_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size sourceSize;
    Size desiredSize;
    sourceSize.width = 0;
    sourceSize.height = 0;

    Size res = OHOS::Media::ConvertDecodeSize(data, sourceSize, desiredSize);

    bool cond = res.width == 0 && res.height == 0;
    EXPECT_EQ(cond, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_ConvertDecodeSize_test_002, TestSize.Level0)
{
    ThumbnailData data;
    Size sourceSize;
    Size desiredSize;
    sourceSize.width = 10;
    sourceSize.height = 10;
    desiredSize = sourceSize;
    data.loaderOpts.decodeInThumbSize = true;

    Size res = OHOS::Media::ConvertDecodeSize(data, sourceSize, desiredSize);

    bool cond = res.width != 0 || res.height != 0;
    EXPECT_EQ(cond, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_ConvertDecodeSize_test_003, TestSize.Level0)
{
    ThumbnailData data;
    Size sourceSize;
    Size desiredSize;
    sourceSize.width = 10;
    sourceSize.height = 10;
    desiredSize.width = 10;
    desiredSize.height = 20;
    data.loaderOpts.decodeInThumbSize = false;
    data.needResizeLcd = true;

    Size res = OHOS::Media::ConvertDecodeSize(data, sourceSize, desiredSize);

    bool cond = res.width != 0 || res.height != 0;
    EXPECT_EQ(cond, true);
}

HWTEST_F(MediaLibraryThumbTest, ThumbnailSourceLoading_ParseDesiredMinSide_test_001, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::THUMB;

    int32_t res = OHOS::Media::ParseDesiredMinSide(type);

    EXPECT_EQ(res, SHORT_SIDE_THRESHOLD);
}
} // namespace Media
} // namespace OHOS