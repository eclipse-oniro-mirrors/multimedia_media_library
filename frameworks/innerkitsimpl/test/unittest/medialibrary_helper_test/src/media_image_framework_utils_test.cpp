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

#include "media_image_framework_utils_test.h"
#include "media_pure_file_utils.h"

#include "image_source.h"
#include "media_file_utils.h"
#include "media_image_framework_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaImageFrameWorkUtilsTest::SetUpTestCase(void) {}
void MediaImageFrameWorkUtilsTest::TearDownTestCase(void) {}
void MediaImageFrameWorkUtilsTest::SetUp(void) {}
void MediaImageFrameWorkUtilsTest::TearDown(void) {}

static const string HDR_PICTURE_PATH = "/data/local/tmp/HDR_picture.jpg";

static unique_ptr<ImageSource> CreateTestImageSource()
{
    string path = HDR_PICTURE_PATH;
    MEDIA_INFO_LOG("file: %{public}s exist: %{public}d", path.c_str(), MediaPureFileUtils::IsFileExists(path));
    uint32_t err;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, err);
    CHECK_AND_PRINT_LOG(err == E_OK, "ImageSource::CreateImageSource err: %{public}d", err);
    return imageSource;
}

static shared_ptr<Picture> CreateTestPicture()
{
    unique_ptr<ImageSource> imageSource = CreateTestImageSource();
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "ImageSource::CreateImageSource failed");

    DecodingOptionsForPicture pictureOpts;
    pictureOpts.desireAuxiliaryPictures = {AuxiliaryPictureType::GAINMAP};
    uint32_t err;
    auto picture = imageSource->CreatePicture(pictureOpts, err);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr, "ImageSource->CreatePicture err: %{public}d", err);
    return picture;
}

HWTEST_F(MediaImageFrameWorkUtilsTest, GetExifRotate_by_image_source_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetExifRotate_by_image_source_Test_001");
    int32_t exifRotate = 0;
    unique_ptr<ImageSource> imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    auto ret = MediaImageFrameWorkUtils::GetExifRotate(imageSource, exifRotate);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("GetExifRotate_by_image_source_Test_001 end");
}

HWTEST_F(MediaImageFrameWorkUtilsTest, GetExifRotate_by_picture_Test_001, TestSize.Level1)
{
    int32_t exifRotate;
    auto picture = CreateTestPicture();
    ASSERT_NE(picture, nullptr);
    // picture->GetExifMetadata() failed
    auto ret = MediaImageFrameWorkUtils::GetExifRotate(picture, exifRotate);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaImageFrameWorkUtilsTest, GetExifRotate_by_path_Test_001, TestSize.Level1)
{
    int32_t exifRotate;
    string path = HDR_PICTURE_PATH;
    auto ret = MediaImageFrameWorkUtils::GetExifRotate(path, exifRotate);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaImageFrameWorkUtilsTest, GetOrientationKey_by_image_source_Test_001, TestSize.Level1)
{
    string orientationKey;
    unique_ptr<ImageSource> imageSource = CreateTestImageSource();
    ASSERT_NE(imageSource, nullptr);
    auto ret = MediaImageFrameWorkUtils::GetOrientationKey(imageSource, orientationKey);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaImageFrameWorkUtilsTest, GetOrientationKey_by_picture_Test_001, TestSize.Level1)
{
    string orientationKey;
    auto picture = CreateTestPicture();
    ASSERT_NE(picture, nullptr);
    auto ret = MediaImageFrameWorkUtils::GetOrientationKey(picture, orientationKey);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaImageFrameWorkUtilsTest, FlipAndRotatePixelMap_by_exifRotate_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_by_exifRotate_Test_001");
    int32_t exifRotate = 1;
    auto picture = CreateTestPicture();
    ASSERT_NE(picture, nullptr);
    auto pixelMap = picture->GetMainPixel();
    ASSERT_NE(pixelMap, nullptr);
    auto ret = MediaImageFrameWorkUtils::FlipAndRotatePixelMap((*pixelMap), exifRotate);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_by_exifRotate_Test_001 end");
}

HWTEST_F(MediaImageFrameWorkUtilsTest, FlipAndRotatePixelMap_by_FlipAndRotateInfo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_by_FlipAndRotateInfo_Test_001");
    FlipAndRotateInfo info;
    auto picture = CreateTestPicture();
    ASSERT_NE(picture, nullptr);
    auto pixelMap = picture->GetMainPixel();
    ASSERT_NE(pixelMap, nullptr);
    auto ret = MediaImageFrameWorkUtils::FlipAndRotatePixelMap((*pixelMap), info);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("FlipAndRotatePixelMap_by_FlipAndRotateInfo_Test_001");
}

} // namespace Media
} // namespace OHOS