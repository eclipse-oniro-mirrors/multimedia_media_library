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
#define MLOG_TAG "MediaLibrarySearchTest"

#include "medialibrary_search_test.h"

#include <thread>
#include "datashare_result_set.h"
#include "get_self_permissions.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "search_column.h"
#include "uri.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const std::string URI_SEARCH_INDEX = MEDIALIBRARY_DATA_URI + "/" + SEARCH_TOTAL_TABLE;

void CleanSearchData()
{
    DataShare::DataSharePredicates predicates;
    Uri searchUri(URI_SEARCH_INDEX);
    MediaLibraryCommand searchCmd(searchUri);
    MediaLibraryDataManager::GetInstance()->Delete(searchCmd, predicates);
}

void MediaLibrarySearchTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Search_Test::Start");
    MediaLibraryUnitTestUtils::Init();
    vector<string> perms = { "ohos.permission.MEDIA_LOCATION" };
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibrarySearchTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
}

void MediaLibrarySearchTest::TearDownTestCase(void)
{
    CleanSearchData();
    MEDIA_INFO_LOG("Search_Test::End");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibrarySearchTest::SetUp(void)
{
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
}

void MediaLibrarySearchTest::TearDown(void) {}

HWTEST_F(MediaLibrarySearchTest, Search_InsertSearchIndex_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Search_InsertSearchIndex_Test_001::Start");
    Uri searchUri(URI_SEARCH_INDEX);
    MediaLibraryCommand cmd(searchUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TBL_SEARCH_DATA, "/storage/cloud/files/Photo/16/IMG_1502005964_000.jpg");
    valuesBucket.Put(TBL_SEARCH_DISPLAYNAME, "IMG_201786_155103.jpg");
    valuesBucket.Put(TBL_SEARCH_LATITUDE, 1234);
    valuesBucket.Put(TBL_SEARCH_LONGITUDE, 5678);
    valuesBucket.Put(TBL_SEARCH_DATE_MODIFIED, 12345678);
    valuesBucket.Put(TBL_SEARCH_PHOTO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_CV_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_GEO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Search_InsertSearchIndex_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibrarySearchTest, Search_InsertSearchIndex_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Search_InsertSearchIndex_Test_002::Start");
    Uri searchUri(URI_SEARCH_INDEX);
    MediaLibraryCommand cmd(searchUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TBL_SEARCH_FILE_ID, 11111);
    valuesBucket.Put(TBL_SEARCH_DATA, "/storage/cloud/files/Photo/16/IMG_1502005964_000.jpg");
    valuesBucket.Put(TBL_SEARCH_DISPLAYNAME, "IMG_201786_155103.jpg");
    valuesBucket.Put(TBL_SEARCH_LATITUDE, 1234);
    valuesBucket.Put(TBL_SEARCH_LONGITUDE, 5678);
    valuesBucket.Put(TBL_SEARCH_DATE_MODIFIED, 12345678);
    valuesBucket.Put(TBL_SEARCH_PHOTO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_CV_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_GEO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Search_InsertSearchIndex_Test_002::retVal = %{public}d. retVal2 = %{public}d. End",
        retVal, retVal2);
}

HWTEST_F(MediaLibrarySearchTest, Search_UpdateSearchIndex_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Search_UpdateSearchIndex_Test_001::Start");
    Uri searchUri(URI_SEARCH_INDEX);
    MediaLibraryCommand cmd(searchUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TBL_SEARCH_FILE_ID, 11112);
    valuesBucket.Put(TBL_SEARCH_DATA, "testupdate1");
    valuesBucket.Put(TBL_SEARCH_DISPLAYNAME, "testupdate1");
    valuesBucket.Put(TBL_SEARCH_LATITUDE, 1234);
    valuesBucket.Put(TBL_SEARCH_LONGITUDE, 5678);
    valuesBucket.Put(TBL_SEARCH_DATE_MODIFIED, 12345678);
    valuesBucket.Put(TBL_SEARCH_PHOTO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_CV_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_GEO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(TBL_SEARCH_DATA, "testupdate2");
    updateValues.Put(TBL_SEARCH_DISPLAYNAME, "testupdate2");
    updateValues.Put(TBL_SEARCH_LATITUDE, 1234);
    updateValues.Put(TBL_SEARCH_LONGITUDE, 5678);
    updateValues.Put(TBL_SEARCH_DATE_MODIFIED, 88888888);
    updateValues.Put(TBL_SEARCH_PHOTO_STATUS, 1);
    updateValues.Put(TBL_SEARCH_CV_STATUS, 1);
    updateValues.Put(TBL_SEARCH_GEO_STATUS, 1);
    updateValues.Put(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("11112");
    inValues.push_back("21233");
    predicates.In(TBL_SEARCH_FILE_ID, inValues)->And()->EqualTo(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    vector<string> columns;
    columns.push_back(TBL_SEARCH_DISPLAYNAME);
    columns.push_back(TBL_SEARCH_DATE_MODIFIED);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    string displayName;
    resultSet->GetString(0, displayName);
    MEDIA_INFO_LOG("Search_UpdateSearchIndex_Test_001::displayName = %{public}s. End", displayName.c_str());
}

HWTEST_F(MediaLibrarySearchTest, Search_DeleteSearch_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Search_DeleteSearch_Test_001::Start");
    Uri searchUri(URI_SEARCH_INDEX);
    MediaLibraryCommand cmd(searchUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TBL_SEARCH_FILE_ID, 11113);
    valuesBucket.Put(TBL_SEARCH_DATA, "/storage/cloud/files/Photo/16/IMG_1502005964_000.jpg");
    valuesBucket.Put(TBL_SEARCH_DISPLAYNAME, "IMG_201786_155103.jpg");
    valuesBucket.Put(TBL_SEARCH_LATITUDE, 1234);
    valuesBucket.Put(TBL_SEARCH_LONGITUDE, 5678);
    valuesBucket.Put(TBL_SEARCH_DATE_MODIFIED, 12345678);
    valuesBucket.Put(TBL_SEARCH_PHOTO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_CV_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_GEO_STATUS, 1);
    valuesBucket.Put(TBL_SEARCH_SYSTEM_LANGUAGE, "zh-Hans");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("11113");
    inValues.push_back("21233");
    predicates.In(TBL_SEARCH_FILE_ID, inValues);
    predicates.And();
    predicates.EqualTo(TBL_SEARCH_PHOTO_STATUS, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Search_DeleteSearch_Test_001::retVal = %{public}d. End", retVal);
}
} // namespace Media
} // namespace OHOS