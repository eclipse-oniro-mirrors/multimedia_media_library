/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "EventCreateTest"

#include "event_create_test.h"

#include "ability_context_impl.h"
#include "get_self_permissions.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_uripermission_operations.h"
#include "userfilemgr_uri.h"

using  namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void EventCreateTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    vector<string> perms = { "ohos.permission.MEDIA_LOCATION" };
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("EventCreateTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
}

void EventCreateTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void EventCreateTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
}

void EventCreateTest::TearDown(void) {}

string ReturnUri(string UriType, string MainUri, string SubUri = "")
{
    if (SubUri.empty()) {
        return (UriType + "/" + MainUri);
    } else {
        return (UriType + "/" + MainUri + "/" + SubUri);
    }
}

int32_t CreateFileAsset(const string &relativePath, const string &displayName, const MediaType &mediaType)
{
    Uri createAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, CONST_MEDIA_FILEOPRN, CONST_MEDIA_FILEOPRN_CREATEASSET));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    MediaLibraryCommand cmd(createAssetUri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t CreateAlbum(const string &albumName, const string &dirPath)
{
    Uri createAlbumUri(ReturnUri(MEDIALIBRARY_DATA_URI, CONST_MEDIA_ALBUMOPRN, CONST_MEDIA_ALBUMOPRN_CREATEALBUM));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, albumName);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, dirPath);
    MediaLibraryCommand cmd(createAlbumUri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t DeleteAsset(const int &id)
{
    Uri deleteAssetUri(ReturnUri(MEDIALIBRARY_DATA_URI, CONST_MEDIA_FILEOPRN, CONST_MEDIA_FILEOPRN_DELETEASSET));
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(CONST_MEDIA_DATA_DB_ID, to_string(id));
    MediaLibraryCommand cmd(deleteAssetUri, Media::OperationType::DELETE);
    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}


HWTEST_F(EventCreateTest, medialib_event_CreateFileAsset_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_001::Start");
    string relativePath = "Pictures/";
    string displayName = "CreateAsset_Test_001.jpg";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    int rowId = CreateFileAsset(relativePath, displayName, mediaType);
    EXPECT_GT(rowId, 0);

    auto retVal = CreateFileAsset(relativePath, displayName, mediaType);
    EXPECT_LT(retVal, 0);

    retVal = DeleteAsset(rowId);
    EXPECT_EQ(retVal, 0);
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(EventCreateTest, medialib_event_CreateFileAsset_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_002::Start");
    string relativePath = "";
    string displayName = "CreateAsset_Test_001.jpg";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    auto retVal = CreateFileAsset(relativePath, displayName, mediaType);
    EXPECT_LT(retVal, 0);
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_002::retVal = %{public}d. End", retVal);
}

HWTEST_F(EventCreateTest, medialib_event_CreateFileAsset_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_003::Start");
    string relativePath = "Pictures/";
    string displayName = "";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    auto retVal = CreateFileAsset(relativePath, displayName, mediaType);
    EXPECT_LT(retVal, 0);
    MEDIA_INFO_LOG("medialib_event_CreateFileAsset_test_003::retVal = %{public}d. End", retVal);
}

HWTEST_F(EventCreateTest, medialib_event_CreateAlbum_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_CreateAlbum_test_001::Start");
    string path = "";
    string displayName = "CreateAlbum_test_001";
    auto retVal = CreateAlbum(displayName, path);
    EXPECT_LT(retVal, 0);
    MEDIA_INFO_LOG("medialib_event_CreateAlbum_test_001::retVal = %{public}d. End", retVal);
}
}
}