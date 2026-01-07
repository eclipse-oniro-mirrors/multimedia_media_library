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

#include "medialibrary_thumbnail_source_loading_test.h"
#include "medialibrary_errno.h"
#include "thumbnail_source_loading.h"
#include "dfx_manager.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "image_source.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "post_proc.h"
#include "thumbnail_image_framework_utils.h"
#include "thumbnail_utils.h"
#include "thumbnail_const.h"

#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <thread>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {

static const std::string LOCAL_MEDIA_PATH = "/storage/media/local/files/";
static const string HDR_PICTURE_PATH = "/storage/media/local/files/Photo/1/HDR_picture.jpg";
static constexpr int32_t TEST_PIXELMAP_WIDTH_AND_HEIGHT = 100;

static unique_ptr<ImageSource> CreateTestImageSource()
{
    string path = HDR_PICTURE_PATH;
    MEDIA_INFO_LOG("file: %{public}s exist: %{public}d", path.c_str(), MediaFileUtils::IsFileExists(path));
    uint32_t err;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, err);
    CHECK_AND_PRINT_LOG(err == E_OK, "ImageSource::CreateImageSource err: %{public}d", err);
    return imageSource;
}

static std::shared_ptr<PixelMap> CreateTestPixelMap(PixelFormat format, bool useDMA,
    int32_t width = TEST_PIXELMAP_WIDTH_AND_HEIGHT, int32_t height = TEST_PIXELMAP_WIDTH_AND_HEIGHT)
{
    InitializationOptions opts;
    opts.size.width = width;
    opts.size.height = height;
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
 
void MediaLibraryThumbnailSourceLoadingTest::SetUpTestCase(void) {}

void MediaLibraryThumbnailSourceLoadingTest::TearDownTestCase(void) {}

void MediaLibraryThumbnailSourceLoadingTest::SetUp() {}

void MediaLibraryThumbnailSourceLoadingTest::TearDown(void) {}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_GetLocalKeyFrameThumbnailPath_test_001, TestSize.Level0)
{
    std::string path = "";
    std::string key = "";
    std::string timeStamp = "";

    std::string res = OHOS::Media::GetLocalKeyFrameThumbnailPath(path, key, timeStamp);
    
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_GetLocalKeyFrameThumbnailPath_test_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_NeedAutoResize_test_001, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD + 1;
    size.height = SHORT_SIDE_THRESHOLD + 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_NeedAutoResize_test_002, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD - 1;
    size.height = SHORT_SIDE_THRESHOLD + 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_NeedAutoResize_test_003, TestSize.Level0)
{
    Size size;
    size.width = SHORT_SIDE_THRESHOLD + 1;
    size.height = SHORT_SIDE_THRESHOLD - 1;

    bool res = OHOS::Media::NeedAutoResize(size);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_GenDecodeOpts_test_001, TestSize.Level0)
{
    Size sourceSize;
    Size targetSize;
    DecodeOptions decodeOptions;
    sourceSize.width = 0;

    bool res = OHOS::Media::GenDecodeOpts(sourceSize, targetSize, decodeOptions);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_GenDecodeOpts_test_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_GenDecodeOpts_test_003, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ConvertDecodeSize_test_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ConvertDecodeSize_test_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ConvertDecodeSize_test_003, TestSize.Level0)
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

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ConvertDecodeSize_test_004, TestSize.Level0)
{
    ThumbnailData data;
    Size sourceSize;
    Size desiredSize;
    sourceSize.width = 10;
    sourceSize.height = 10;
    desiredSize.width = 10;
    desiredSize.height = 20;
    data.loaderOpts.decodeInThumbSize = false;
    data.needResizeLcd = false;

    Size res = OHOS::Media::ConvertDecodeSize(data, sourceSize, desiredSize);

    bool cond = res.width != 0 || res.height != 0;
    EXPECT_EQ(cond, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ParseDesiredMinSide_test_001, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::THUMB;

    int32_t res = OHOS::Media::ParseDesiredMinSide(type);

    EXPECT_EQ(res, SHORT_SIDE_THRESHOLD);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ParseDesiredMinSide_test_002, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::MTH_ASTC;

    int32_t res = OHOS::Media::ParseDesiredMinSide(type);

    EXPECT_EQ(res, MONTH_SHORT_SIDE_THRESHOLD);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_ParseDesiredMinSide_test_003, TestSize.Level0)
{
    ThumbnailType type = ThumbnailType::YEAR_ASTC;

    int32_t res = OHOS::Media::ParseDesiredMinSide(type);

    EXPECT_EQ(res, std::numeric_limits<int32_t>::max());
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_LoadImageSource_test_001, TestSize.Level0)
{
    std::string path = "";
    uint32_t err;

    auto res = OHOS::Media::LoadImageSource(path, err);

    EXPECT_EQ(res, nullptr);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_001, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    data.originalPhotoPicture = picture;
    data.loaderOpts.decodeInThumbSize = true;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_002, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::YCRCB_P010, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    data.originalPhotoPicture = picture;
    data.loaderOpts.decodeInThumbSize = false;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();
    
    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_003, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = nullptr;
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    data.originalPhotoPicture = picture;
    data.loaderOpts.decodeInThumbSize = false;
    data.orientation = 0;
    data.mediaType = MEDIA_TYPE_VIDEO;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, true);
}


HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_004, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<PixelMap> gainMap = CreateTestPixelMap(PixelFormat::RGBA_8888, false);
    std::shared_ptr<Picture> picture = CreateTestPicture(pixelMap, gainMap);
    ASSERT_NE(picture, nullptr);
    data.originalPhotoPicture = picture;
    data.loaderOpts.decodeInThumbSize = false;
    data.orientation = 0;
    data.mediaType = MEDIA_TYPE_AUDIO;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, true);
}


HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_005, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    data.originalPhotoPicture = nullptr;
    data.loaderOpts.decodeInThumbSize = false;
    data.orientation = 0;
    data.mediaType = MEDIA_TYPE_VIDEO;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();
    data.loaderOpts.loadingStates.emplace(SourceState::BEGIN, SourceState::FINISH);

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_006, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    data.originalPhotoPicture = nullptr;
    data.loaderOpts.decodeInThumbSize = false;
    data.orientation = 0;
    data.mediaType = MEDIA_TYPE_VIDEO;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();
    data.loaderOpts.loadingStates.emplace(SourceState::BEGIN, SourceState::LOCAL_THUMB);
    data.loaderOpts.loadingStates.emplace(SourceState::LOCAL_THUMB, SourceState::FINISH);

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    ThumbnailSourceLoading_RunLoading_test_007, TestSize.Level0)
{
    Size desiredSize = {64, 64};
    ThumbnailData data;
    data.originalPhotoPicture = nullptr;
    data.loaderOpts.decodeInThumbSize = false;
    data.orientation = 0;
    data.mediaType = MEDIA_TYPE_IMAGE;
    data.loaderOpts.loadingStates = std::unordered_map<SourceState, SourceState>();
    data.loaderOpts.loadingStates.emplace(SourceState::BEGIN, SourceState::CLOUD_THUMB);
    data.loaderOpts.loadingStates.emplace(SourceState::CLOUD_THUMB, SourceState::FINISH);

    SourceLoader sourceLoader(desiredSize, data);

    bool res = sourceLoader.RunLoading();

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalThumbSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;

    std::string res = LocalThumbSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalThumbSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;

    bool res = LocalThumbSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalThumbSource_IsSizeLargeEnough_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;

    bool res = LocalThumbSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalLcdSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;

    std::string res = LocalLcdSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalLcdSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;

    bool res = LocalLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalLcdSource_IsSizeLargeEnough_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;

    bool res = LocalLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalOriginSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;

    std::string res = LocalOriginSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    LocalOriginSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;

    bool res = LocalOriginSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudThumbSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;

    std::string res = CloudThumbSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}


HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudThumbSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;
    data.loaderOpts.desiredType = ThumbnailType::THUMB;

    bool res = CloudThumbSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudThumbSource_IsSizeLargeEnough_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;
    data.loaderOpts.desiredType = ThumbnailType::THUMB;

    bool res = CloudThumbSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;
    data.orientation = 1;

    std::string res = CloudLcdSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_GetSourcePath_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;
    data.orientation = 0;

    std::string res = CloudLcdSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;
    data.mediaType = MEDIA_TYPE_VIDEO;

    bool res = CloudLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_IsSizeLargeEnough_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;
    data.mediaType = MEDIA_TYPE_IMAGE;
    data.photoWidth = SHORT_SIDE_THRESHOLD - 1;
    data.photoHeight = SHORT_SIDE_THRESHOLD - 1;

    bool res = CloudLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_IsSizeLargeEnough_test_003, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;
    data.mediaType = MEDIA_TYPE_IMAGE;
    data.photoWidth = 0;
    data.photoHeight = 0;

    bool res = CloudLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_IsSizeLargeEnough_test_004, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD - 1;
    data.mediaType = MEDIA_TYPE_IMAGE;
    data.photoWidth = SHORT_SIDE_THRESHOLD;
    data.photoHeight = SHORT_SIDE_THRESHOLD;

    bool res = CloudLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudLcdSource_IsSizeLargeEnough_test_005, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;
    data.mediaType = MEDIA_TYPE_IMAGE;
    data.photoWidth = SHORT_SIDE_THRESHOLD;
    data.photoHeight = SHORT_SIDE_THRESHOLD;

    bool res = CloudLcdSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudOriginSource_GetSourcePath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;
    data.mediaType = MEDIA_TYPE_VIDEO;

    std::string res = CloudOriginSource::GetSourcePath(data, err);

    EXPECT_EQ(res, data.path);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudOriginSource_GetSourcePath_test_002, TestSize.Level0)
{
    ThumbnailData data;
    int32_t err;
    data.mediaType = MEDIA_TYPE_IMAGE;

    std::string res = CloudOriginSource::GetSourcePath(data, err);

    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest,
    CloudOriginSource_IsSizeLargeEnough_test_001, TestSize.Level0)
{
    ThumbnailData data;
    int32_t minSize = SHORT_SIDE_THRESHOLD;

    bool res = CloudOriginSource::IsSizeLargeEnough(data, minSize);

    EXPECT_EQ(res, true);
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_001: Test with valid dual HDR source and valid size");
    
    ThumbnailData data;
    Size targetSize = {800, 600};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    
    EXPECT_TRUE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_001 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_002: Test with valid dual HDR source and valid size");
    
    ThumbnailData data;
    Size targetSize = {800, 600};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    
    EXPECT_TRUE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_002 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_003: Test with null image source");
    
    ThumbnailData data;
    Size targetSize = {800, 600};
    unique_ptr<ImageSource> imageSource = nullptr;
    
    SourceLoader loader(targetSize, data);
    
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    EXPECT_FALSE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_003 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_004: Test with invalid target size (zero width)");
    
    ThumbnailData data;
    Size targetSize = {0, 600};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    EXPECT_FALSE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_004 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_005: Test with invalid target size (negative height)");
    
    ThumbnailData data;
    Size targetSize = {800, -100};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    EXPECT_FALSE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_005 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_006: Test with both dimensions invalid");
    
    ThumbnailData data;
    Size targetSize = {0, 0};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    EXPECT_FALSE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_006 end");
}

HWTEST_F(MediaLibraryThumbnailSourceLoadingTest, LoadHdrSourceTest_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("LoadHdrSourceTest_007: Test with minimal valid size");
    
    ThumbnailData data;
    Size targetSize = {1, 1};
    auto imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    
    SourceLoader loader(targetSize, data);
    
    bool ret = loader.LoadHdrSource(imageSource, targetSize);
    EXPECT_TRUE(ret);
    
    MEDIA_INFO_LOG("LoadHdrSourceTest_007 end");
}

} // namespace Media
} // namespace OHOS