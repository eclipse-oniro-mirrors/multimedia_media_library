/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "medialibrary_thumbnail_image_framework_test.h"

#include "image_source.h"

#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"

#define private public
#define protected public
#include "thumbnail_image_framework_utils.h"
#undef private
#undef protected

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {
const int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;
const string TEST_HDR_JPG = "/data/local/tmp/hdr.jpg";

void MediaLibraryThumbnailImageFrameworkTest::SetUpTestCase(void) {}

void MediaLibraryThumbnailImageFrameworkTest::TearDownTestCase(void) {}

void MediaLibraryThumbnailImageFrameworkTest::SetUp() {}

void MediaLibraryThumbnailImageFrameworkTest::TearDown(void) {}

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

static const string HDR_PICTURE_PATH = "/storage/media/local/files/Photo/1/HDR_picture.jpg";

static unique_ptr<ImageSource> CreateTestImageSource()
{
    std::string path = HDR_PICTURE_PATH;
    MEDIA_INFO_LOG("file: %{public}s exist: %{public}d", path.c_str(), MediaFileUtils::IsFileExists(path));
    uint32_t err;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, err);
    CHECK_AND_PRINT_LOG(err == E_OK, "ImageSource::CreateImageSource err: %{public}d", err);
    return imageSource;
}

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

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsYuvPixelMap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = nullptr;
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, false);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsYuvPixelMap_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsYuvPixelMap_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    bool isYuv = ThumbnailImageFrameWorkUtils::IsYuvPixelMap(pixelMap);
    EXPECT_EQ(isYuv, false);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPictureSource_test_001, TestSize.Level0)
{
    std::shared_ptr<Picture> picture = nullptr;
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_EQ(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPictureSource_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    std::shared_ptr<PixelMap> gainMap = nullptr;
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_EQ(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPictureSource_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    std::shared_ptr<Picture> copyPicture = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
    EXPECT_NE(copyPicture, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPixelMapSource_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = nullptr;
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_EQ(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPixelMapSource_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPixelMapSource_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyNormalPixelmap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyNormalPixelmap(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyYuvPixelmap_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, false);
    ASSERT_NE(pixelMap, nullptr);
    EXPECT_EQ(pixelMap->GetAllocatorType() == AllocatorType::DMA_ALLOC, false);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyYuvPixelmap(pixelMap);
    EXPECT_NE(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyPixelMapSource_test_004, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::YCRCB_P010, true);
    ASSERT_NE(pixelMap, nullptr);
    std::shared_ptr<PixelMap> copyPixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
    EXPECT_EQ(copyPixelMap, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsSupportCopyPixelMap_test_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopySurfaceBufferInfo_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> source = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> dst = SurfaceBuffer::Create();
    ThumbnailImageFrameWorkUtils::CopySurfaceBufferInfo(source, dst);
    EXPECT_NE(source, nullptr);
    EXPECT_NE(dst, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, GetSbStaticMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 1;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::GetSbStaticMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, GetSbDynamicMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 2;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::GetSbDynamicMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, SetSbStaticMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 3;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::SetSbStaticMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, SetSbDynamicMetadata_test_001, TestSize.Level0)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    std::vector<uint8_t> staticMetadata;
    uint8_t testvalue = 4;
    staticMetadata.push_back(testvalue);
    bool ret = ThumbnailImageFrameWorkUtils::SetSbDynamicMetadata(buffer, staticMetadata);
    EXPECT_NE(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, SetPixelMapYuvInfo_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV12, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, SetPixelMapYuvInfo_test_002, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::NV21, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, SetPixelMapYuvInfo_test_003, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::BGRA_8888, true);
    ASSERT_NE(pixelMap, nullptr);
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    bool isHdr = true;
    bool ret = ThumbnailImageFrameWorkUtils::SetPixelMapYuvInfo(surfaceBuffer, pixelMap, isHdr);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsPictureValid_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    bool ret = ThumbnailImageFrameWorkUtils::IsPictureValid(picture);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsPixelMapValid_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> PixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    bool ret = ThumbnailImageFrameWorkUtils::IsPixelMapValid(PixelMap);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyAndScalePicture_test_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    Size desiredSize = { TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2, TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2 };
    auto ret = ThumbnailImageFrameWorkUtils::CopyAndScalePicture(picture, desiredSize);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CopyAndScalePixelMap_001, TestSize.Level0)
{
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    Size desiredSize = { TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2, TEST_PIXELMAP_WIDTH_AND_HEIGHT / 2 };
    auto ret = ThumbnailImageFrameWorkUtils::CopyAndScalePixelMap(pixelMap, desiredSize);
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, FlipAndRotatePicture_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePicture_001");
    auto picture = CreateTestPicture();
    int32_t exifRotate = 2;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(picture, exifRotate);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePicture_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, FlipAndRotatePicture_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePicture_002");
    auto picture = CreateTestPicture();
    FlipAndRotateInfo info;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(picture, info);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePicture_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, FlipAndRotatePixelMap_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_001");
    auto pixelMap = CreateTestPixelMap();
    int32_t exifRotate = 2;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, exifRotate);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, FlipAndRotatePixelMap_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_002");
    auto pixelMap = CreateTestPixelMap();
    FlipAndRotateInfo info;
    auto ret = ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, info);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ConvertPixelMapToSdrAndFormatRGBA8888_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ConvertPixelMapToSdrAndFormatRGBA8888_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_002");
    auto pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ConvertPixelMapToSdrAndFormatRGBA8888_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_003");
    auto pixelMap = CreateTestPixelMap(PixelFormat::NV12, true);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_003 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ConvertPixelMapToSdrAndFormatRGBA8888_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_004");
    auto pixelMap = CreateTestHdrPixelMap(false);
    ASSERT_NE(pixelMap, nullptr);
    auto ret = ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(pixelMap);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(pixelMap->GetPixelFormat(), PixelFormat::RGBA_8888);
    MEDIA_INFO_LOG("ConvertPixelMapToSdrAndFormatRGBA8888_004 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePictureTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePictureTest_001");
    constexpr int32_t TEST_TARGET_WIDHT = 350;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), size);
    EXPECT_NE(picture, nullptr);
    MEDIA_INFO_LOG("CreatePictureTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePictureTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePictureTest_002");
    constexpr int32_t TEST_TARGET_WIDHT = 350;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(nullptr, size);
    EXPECT_EQ(picture, nullptr);
    MEDIA_INFO_LOG("CreatePictureTest_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePictureTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePictureTest_003");
    constexpr int32_t TEST_TARGET_WIDHT = 0;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), size);
    EXPECT_EQ(picture, nullptr);
    MEDIA_INFO_LOG("CreatePictureTest_003 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePictureTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePictureTest_004");
    constexpr int32_t TEST_TARGET_WIDHT = -1;
    constexpr int32_t TEST_TARGET_HEIGHT = -1;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), size);
    EXPECT_EQ(picture, nullptr);
    MEDIA_INFO_LOG("CreatePictureTest_004 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePixelMapTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePixelMapTest_001");
    uint32_t err = E_OK;
    constexpr int32_t TEST_TARGET_WIDHT = 350;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };
    DecodeOptions decodeOpts;
    decodeOpts.desiredDynamicRange = DecodeDynamicRange::SDR;
    decodeOpts.desiredSize = size;
    decodeOpts.desiredPixelFormat = PixelFormat::RGBA_8888;
    auto pixelMap = ThumbnailImageFrameWorkUtils::CreatePixelMap(
        CreateTestImageSource(), decodeOpts, err);
    EXPECT_NE(pixelMap, nullptr);
    MEDIA_INFO_LOG("CreatePixelMapTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePixelMapTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePixelMapTest_002");
    uint32_t err = E_OK;
    constexpr int32_t TEST_TARGET_WIDHT = 350;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };


    DecodeOptions decodeOpts;
    decodeOpts.desiredDynamicRange = DecodeDynamicRange::HDR;
    decodeOpts.desiredSize = size;
    decodeOpts.desiredPixelFormat = PixelFormat::RGBA_1010102;
    auto pixelMap = ThumbnailImageFrameWorkUtils::CreatePixelMap(
        CreateTestImageSource(), decodeOpts, err);
    
    EXPECT_NE(pixelMap, nullptr);
    MEDIA_INFO_LOG("CreatePixelMapTest_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, CreatePixelMapTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("CreatePixelMapTest_003");
    uint32_t err = E_OK;
    constexpr int32_t TEST_TARGET_WIDHT = 350;
    constexpr int32_t TEST_TARGET_HEIGHT = 1050;
    Size size = { TEST_TARGET_WIDHT, TEST_TARGET_HEIGHT };


    DecodeOptions decodeOpts;
    decodeOpts.desiredDynamicRange = DecodeDynamicRange::SDR;
    decodeOpts.desiredSize = size;
    decodeOpts.desiredPixelFormat = PixelFormat::RGBA_8888;
    auto pixelMap = ThumbnailImageFrameWorkUtils::CreatePixelMap(
        nullptr, decodeOpts, err);

    EXPECT_EQ(pixelMap, nullptr);
    MEDIA_INFO_LOG("CreatePixelMapTest_003 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ResizePictureTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("ResizePictureTest_001: Normal case with valid picture and size");
    Size sourceSize = {1920, 1080};
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), sourceSize);
    ASSERT_NE(picture, nullptr);
    
    Size targetSize = {800, 600};
    bool ret = ThumbnailImageFrameWorkUtils::ResizePicture(picture, targetSize);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("ResizePictureTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ResizePictureTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("ResizePictureTest_002: Null picture pointer");
    
    PicturePtr picture = nullptr;
    Size targetSize = {800, 600};
    bool ret = ThumbnailImageFrameWorkUtils::ResizePicture(picture, targetSize);
    
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ResizePictureTest_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ResizePictureTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("ResizePictureTest_003: Zero target size");
    
    Size sourceSize = {1920, 1080};
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), sourceSize);
    ASSERT_NE(picture, nullptr);
    
    Size targetSize = {0, 0};
    bool ret = ThumbnailImageFrameWorkUtils::ResizePicture(picture, targetSize);
    
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ResizePictureTest_003 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, ResizePictureTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("ResizePictureTest_004: Negative target size");
    
    Size sourceSize = {1920, 1080};
    auto picture = ThumbnailImageFrameWorkUtils::CreatePicture(CreateTestImageSource(), sourceSize);
    ASSERT_NE(picture, nullptr);
    
    Size targetSize = {-100, -200};
    bool ret = ThumbnailImageFrameWorkUtils::ResizePicture(picture, targetSize);
    
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("ResizePictureTest_004 end");
}

// IsAdaptedHdrType函数测试
HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsAdaptedHdrTypeTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAdaptedHdrTypeTest_001: Test all adapted HDR types");
    
    // 测试所有适配的HDR类型
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_ISO_DUAL));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_CUVA));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_VIVID_DUAL));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_LOG_DUAL));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_VIVID_SINGLE));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_ISO_SINGLE));
    
    MEDIA_INFO_LOG("IsAdaptedHdrTypeTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsAdaptedHdrTypeTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsAdaptedHdrTypeTest_002: Test non-adapted HDR types");
    
    // 测试非适配的HDR类型
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::SDR));
    
    // 测试超出枚举范围的值
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(static_cast<ImageHdrType>(100)));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(static_cast<ImageHdrType>(-1)));
    
    MEDIA_INFO_LOG("IsAdaptedHdrTypeTest_002 end");
}

// IsSinglePixelMapHdrType函数测试
HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsSinglePixelMapHdrTypeTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_001: Test single pixel map HDR types");
    
    // 测试单像素图的HDR类型
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_VIVID_SINGLE));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_ISO_SINGLE));
    
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsSinglePixelMapHdrTypeTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_002: Test non-single pixel map HDR types");
    
    // 测试非单像素图的HDR类型（但仍然是适配的HDR类型）
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_ISO_DUAL));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_CUVA));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_VIVID_DUAL));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_LOG_DUAL));
    
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_002 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsSinglePixelMapHdrTypeTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_003: Test edge cases");
    
    // 测试未适配的HDR类型
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::SDR));
    
    // 测试无效枚举值
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(static_cast<ImageHdrType>(99)));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(static_cast<ImageHdrType>(-5)));
    
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_003 end");
}

HWTEST_F(MediaLibraryThumbnailImageFrameworkTest, IsSinglePixelMapHdrTypeTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_004: Relationship test between two functions");
    
    // 验证两个函数的关系：所有单像素图HDR类型都应该是适配的HDR类型
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_VIVID_SINGLE));
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_ISO_SINGLE));
    
    // 验证逆命题不一定成立：适配的HDR类型不一定是单像素图类型
    EXPECT_TRUE(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(ImageHdrType::HDR_ISO_DUAL));
    EXPECT_FALSE(ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(ImageHdrType::HDR_ISO_DUAL));
    
    MEDIA_INFO_LOG("IsSinglePixelMapHdrTypeTest_004 end");
}

} // namespace Media
} // namespace OHOS