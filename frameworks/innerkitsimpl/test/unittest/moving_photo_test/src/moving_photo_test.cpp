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

#define MLOG_TAG "MovingPhotoTest"

#include <thread>
#include "file_asset.h"
#include "moving_photo_test.h"
#include "moving_photo_capi.h"
#include "moving_photo_impl.h"
#include "media_library_manager.h"
#include "thumbnail_const.h"
#include "medialibrary_db_const.h"
#include "oh_moving_photo.h"
#include "media_file_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;

void MovingPhotoTest::SetUpTestCase(void) {}

void MovingPhotoTest::TearDownTestCase(void) {}

void MovingPhotoTest::SetUp(void) {}

void MovingPhotoTest::TearDown(void) {}

void MovingPhotoTest::FreeCharPointer(const char *freeCharPointer)
{
    if (freeCharPointer != nullptr) {
        freeCharPointer = nullptr;
    }
}

const int SCAN_WAIT_TIME_1S = 1;
MediaLibraryManager* mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();

const std::string ROOT_TEST_MEDIA_DIR =
    "/data/app/el2/100/base/com.ohos.medialibrary.medialibrarydata/haps/";

/**
 * @tc.name: moving_photo_test_001
 * @tc.desc: Test getting the URI from a moving photo
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_001, TestSize.Level0)
{
    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    const char *uri = nullptr;
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_GetUri(movingPhoto, &uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
    FreeCharPointer(uri);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_002
 * @tc.desc: Test getting the URI from a moving photo with various invalid inputs
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_002, TestSize.Level0)
{
    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    const char *uri = nullptr;
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_GetUri(nullptr, &uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_GetUri(movingPhoto, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    movingPhoto->movingPhoto_ = nullptr;
    ret = OH_MovingPhoto_GetUri(movingPhoto, &uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    FreeCharPointer(uri);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_003
 * @tc.desc: Test requesting content URIs from a moving photo
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_003, TestSize.Level0)
{
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + srcDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    string destUri_ = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri_, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri_.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    char* imageUri = strdup(destUri.c_str());
    char* videoUri = strdup(destUri_.c_str());
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithUris(movingPhoto, imageUri, videoUri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_004
 * @tc.desc: Test requesting content URIs from a moving photo with various invalid inputs
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_004, TestSize.Level0)
{
    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    char imageUri[] = "testImageUri";
    char videoUri[] = "testVideoUri";
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithUris(nullptr, imageUri, videoUri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_RequestContentWithUris(movingPhoto, nullptr, videoUri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_RequestContentWithUris(movingPhoto, imageUri, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    movingPhoto->movingPhoto_ = nullptr;
    ret = OH_MovingPhoto_RequestContentWithUris(movingPhoto, imageUri, videoUri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}


/**
 * @tc.name: moving_photo_test_005
 * @tc.desc: Test requesting content URI from a moving photo
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_005, TestSize.Level0)
{
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    MediaLibrary_ResourceType resourceType = MEDIA_LIBRARY_IMAGE_RESOURCE;
    char* uri = strdup(destUri.c_str());
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithUri(movingPhoto, resourceType, uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_006
 * @tc.desc: Test requesting content URI from a moving photo with various invalid inputs
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_006, TestSize.Level0)
{
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    MediaLibrary_ResourceType resourceType = MEDIA_LIBRARY_IMAGE_RESOURCE;
    char* uri = strdup(destUri.c_str());
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithUri(nullptr, resourceType, uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_RequestContentWithUri(movingPhoto, resourceType, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    movingPhoto->movingPhoto_ = nullptr;
    ret = OH_MovingPhoto_RequestContentWithUri(movingPhoto, resourceType, uri);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_007
 * @tc.desc: Test requesting content from a moving photo using a buffer
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_007, TestSize.Level0)
{
    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    MediaLibrary_ResourceType resourceType = MEDIA_LIBRARY_IMAGE_RESOURCE;
    uint32_t size = 10;
    uint8_t* contentBuffer = nullptr;
    contentBuffer = new uint8_t[size];
    const uint8_t** buffer = const_cast<const uint8_t**>(&contentBuffer);
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithBuffer(movingPhoto, resourceType, buffer, &size);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_008
 * @tc.desc: Test requesting content from a moving photo using a buffer with various invalid inputs
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_008, TestSize.Level0)
{
    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    uint32_t size = 10;
    uint8_t* contentBuffer = nullptr;
    contentBuffer = new uint8_t[size];
    const uint8_t** buffer = const_cast<const uint8_t**>(&contentBuffer);
    MediaLibrary_ResourceType resourceType = MEDIA_LIBRARY_IMAGE_RESOURCE;
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_RequestContentWithBuffer(nullptr, resourceType, buffer, &size);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_RequestContentWithBuffer(movingPhoto, resourceType, nullptr, &size);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_RequestContentWithBuffer(movingPhoto, resourceType, buffer, nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    movingPhoto->movingPhoto_ = nullptr;
    ret = OH_MovingPhoto_RequestContentWithBuffer(movingPhoto, resourceType, buffer, &size);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(OH_MovingPhoto_Release(movingPhoto), MEDIA_LIBRARY_OK);
}

/**
 * @tc.name: moving_photo_test_009
 * @tc.desc: Test release moving photo
 * @tc.type: FUNC
 */
HWTEST_F(MovingPhotoTest, moving_photo_test_009, TestSize.Level0)
{
    string srcDisplayName = "request_video_src_4.jpg";
    string destDisplayName = "request_video_dest_4.jpg";
    string srcuri = mediaLibraryManager->CreateAsset(srcDisplayName);
    int32_t srcFd = mediaLibraryManager->OpenAsset(srcuri, MEDIA_FILEMODE_READWRITE);
    mediaLibraryManager->CloseAsset(srcuri, srcFd);

    string destUri = ROOT_TEST_MEDIA_DIR + destDisplayName;
    EXPECT_NE(destUri, "");
    MEDIA_INFO_LOG("createFile uri: %{public}s", destUri.c_str());
    sleep(SCAN_WAIT_TIME_1S);

    std::string uri_ = "ParseThumbnailInfo?" + THUMBNAIL_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        THUMBNAIL_WIDTH + "=1&" + THUMBNAIL_HEIGHT + "=1";
    auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(uri_);
    auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
    MediaLibrary_ErrorCode ret = OH_MovingPhoto_Release(nullptr);
    EXPECT_EQ(ret, MEDIA_LIBRARY_PARAMETER_ERROR);
    ret = OH_MovingPhoto_Release(movingPhoto);
    EXPECT_EQ(ret, MEDIA_LIBRARY_OK);
}
} // namespace Media
} // namespace OHOS