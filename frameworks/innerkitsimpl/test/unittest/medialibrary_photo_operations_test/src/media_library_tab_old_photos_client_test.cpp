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
 
#define MLOG_TAG "MediaLibraryTabOldPhotosClientTest"
 
#include "media_library_tab_old_photos_client_test.h"
 
#include <string>
 
#define private public
#include "media_library_tab_old_photos_client.h"
#undef private
 
#include "media_library_manager.h"
#include "media_log.h"
 
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
void TabOldPhotosClientTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}
 
void TabOldPhotosClientTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}
 
// SetUp:Execute before each test case
void TabOldPhotosClientTest::SetUp() {}
 
void TabOldPhotosClientTest::TearDown(void) {}
 
/**
 * @tc.name  : Parse_ShouldReturnIdLinkType_WhenUriContainsGalleryUriPrefix
 * @tc.number: TabOldPhotosClientTest_001
 * @tc.desc  : Test when uri contains gallery uri prefix, the function should return id link type
 */
HWTEST_F(TabOldPhotosClientTest, TabOldPhotosClient_test_001, TestSize.Level1)
{
    std::vector<std::string> queryTabOldPhotosUris =
    {"file://media/storage/emulated/123"};
    std::vector<TabOldPhotosClient::RequestUriObj> result =
    TabOldPhotosClient().Parse(queryTabOldPhotosUris);
    EXPECT_EQ(result[0].type, TabOldPhotosClient::URI_TYPE_ID_LINK);
    EXPECT_EQ(result[0].oldFileId, 123);
}

/**
 * @tc.name  : Parse_ShouldReturnPathType_WhenUriContainsGalleryPath
 * @tc.number: TabOldPhotosClientTest_002
 * @tc.desc  : Test when uri contains gallery path, the function should return path type
 */
HWTEST_F(TabOldPhotosClientTest, TabOldPhotosClient_test_002, TestSize.Level1)
{
    std::vector<std::string> queryTabOldPhotosUris =
    {"/storage/emulated/0/DCIM/Camera/IMG_20240923_121826.jpg"};
    std::vector<TabOldPhotosClient::RequestUriObj> result =
    TabOldPhotosClient().Parse(queryTabOldPhotosUris);
    EXPECT_EQ(result[0].type, TabOldPhotosClient::URI_TYPE_PATH);
    EXPECT_EQ(result[0].oldData, "/storage/emulated/0/DCIM/Camera/IMG_20240923_121826.jpg");
}

/**
 * @tc.name  : Parse_ShouldReturnIdType_WhenUriIsDigit
 * @tc.number: TabOldPhotosClientTest_003
 * @tc.desc  : Test when uri is digit, the function should return id type
 */
HWTEST_F(TabOldPhotosClientTest, TabOldPhotosClient_test_003, TestSize.Level1)
{
    std::vector<std::string> queryTabOldPhotosUris = {"123"};
    std::vector<TabOldPhotosClient::RequestUriObj> result =
    TabOldPhotosClient().Parse(queryTabOldPhotosUris);
    EXPECT_EQ(result[0].type, TabOldPhotosClient::URI_TYPE_ID);
    EXPECT_EQ(result[0].oldFileId, 123);
}
} // namespace Media
} // namespace OHOS