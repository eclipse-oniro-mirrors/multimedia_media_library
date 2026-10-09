/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "media_share_photo_data_client_test.h"

#include <memory>
#include <string>
#include <vector>

#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "media_library_database.h"
#include "csv_file_reader.h"
#include "media_share_photo_data_client.h"

using namespace std;
using namespace testing::ext;
using namespace testing::internal;

namespace OHOS::Media::ShareAlbum {
using namespace OHOS::Media::ORM;
using namespace OHOS::Media::TestUtils;

namespace {
// Service side MediaSharePhotoDataService::GetShareAlbumOwnerId return codes.
constexpr int32_t ERR_NOT_FOUND = -1;
constexpr int32_t ERR_RESULT_NOT_SHARED = -3;
// Data rows from share_photo_handler-photos.csv (query by cloud_id, not data path):
// index 10011: is_shared = 0, cloud_id = 373b364a...54ff97c1
// index 10012: is_shared = 1, cloud_id = 374b364a...54ff97d9
const std::string SHARED_PHOTO_CLOUD_ID = "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d9";
const std::string NOT_SHARED_PHOTO_CLOUD_ID = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c1";
const std::string NOT_EXIST_PHOTO_CLOUD_ID = "no_exist_cloud_id";
const std::string EXPECTED_SHARE_ALBUM_OWNER = "shareAlbumOwner002";
}  // namespace

DatabaseDataMock MediaSharePhotoDataClientTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken *mockToken = nullptr;

void MediaSharePhotoDataClientTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest SetUpTestCase";
    // Get RdbStore
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(MediaSharePhotoDataClientTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest SetUpTestCase ret: " << ret;
}

void MediaSharePhotoDataClientTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void MediaSharePhotoDataClientTest::SetUp()
{
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest SetUp";
}

void MediaSharePhotoDataClientTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "MediaSharePhotoDataClientTest TearDown";
}

/**
 * 查询共享相册图片（is_shared = 1），获取共享相册所有者
 * 期望结果：
 * 返回ret为E_OK，ownerId与预期值一致
 */
HWTEST_F(MediaSharePhotoDataClientTest, GetShareAlbumOwnerId_IsSharedTrue_001, TestSize.Level1)
{
    std::shared_ptr<MediaSharePhotoDataClient> photoDataClient = std::make_shared<MediaSharePhotoDataClient>(1, 100);
    string ownerId;
    int32_t ret = photoDataClient->GetShareAlbumOwnerId(SHARED_PHOTO_CLOUD_ID, ownerId);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(ownerId, EXPECTED_SHARE_ALBUM_OWNER);
}

/**
 * 查询非共享相册图片（is_shared = 0），获取共享相册所有者
 * 期望结果：
 * 返回ret为ERR_RESULT_NOT_SHARED（-3），ownerId为空
 */
HWTEST_F(MediaSharePhotoDataClientTest, GetShareAlbumOwnerId_IsSharedFalse_001, TestSize.Level1)
{
    std::shared_ptr<MediaSharePhotoDataClient> photoDataClient = std::make_shared<MediaSharePhotoDataClient>(1, 100);
    string ownerId;
    int32_t ret = photoDataClient->GetShareAlbumOwnerId(NOT_SHARED_PHOTO_CLOUD_ID, ownerId);
    EXPECT_EQ(ret, ERR_RESULT_NOT_SHARED);
    EXPECT_TRUE(ownerId.empty());
}

/**
 * 查询不存在的图片路径，获取共享相册所有者
 * 期望结果：
 * 返回ret为ERR_NOT_FOUND（-1），ownerId为空
 */
HWTEST_F(MediaSharePhotoDataClientTest, GetShareAlbumOwnerId_NotExist_001, TestSize.Level1)
{
    std::shared_ptr<MediaSharePhotoDataClient> photoDataClient = std::make_shared<MediaSharePhotoDataClient>(1, 100);
    string ownerId;
    int32_t ret = photoDataClient->GetShareAlbumOwnerId(NOT_EXIST_PHOTO_CLOUD_ID, ownerId);
    EXPECT_EQ(ret, ERR_NOT_FOUND);
    EXPECT_TRUE(ownerId.empty());
}

/**
 * 传入空字符串查询，获取共享相册所有者
 * 期望结果：
 * 返回ret为ERR_NOT_FOUND（-1），ownerId为空
 */
HWTEST_F(MediaSharePhotoDataClientTest, GetShareAlbumOwnerId_EmptyData_001, TestSize.Level1)
{
    std::shared_ptr<MediaSharePhotoDataClient> photoDataClient = std::make_shared<MediaSharePhotoDataClient>(1, 100);
    string ownerId;
    int32_t ret = photoDataClient->GetShareAlbumOwnerId("", ownerId);
    EXPECT_EQ(ret, ERR_NOT_FOUND);
    EXPECT_TRUE(ownerId.empty());
}

}  // namespace OHOS::Media::ShareAlbum
