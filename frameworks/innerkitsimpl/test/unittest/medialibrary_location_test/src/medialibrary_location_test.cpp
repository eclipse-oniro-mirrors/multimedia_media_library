/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryLocationTest"

#include "medialibrary_location_test.h"

#include "datashare_result_set.h"
#include "get_self_permissions.h"
#include "locale_config.h"
#include "location_column.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "location_db_sqls.h"
#include "uri.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static std::atomic<int> num{ 0 };
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static std::vector<std::string> createTableSqlLists = {
    CREATE_GEO_KNOWLEDGE_TABLE,
    CREATE_GEO_DICTIONARY_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
};

static std::vector<std::string> testTables = {
    GEO_KNOWLEDGE_TABLE,
    GEO_DICTIONARY_TABLE,
    PhotoColumn::PHOTOS_TABLE,
};

void MediaLibraryLocationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Location_Test::Start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
}

void MediaLibraryLocationTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Location_Test::End");
}

void MediaLibraryLocationTest::SetUp(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
}

void MediaLibraryLocationTest::TearDown(void) {}

HWTEST_F(MediaLibraryLocationTest, Location_InsertGeoKnowledge_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_InsertGeoKnowledge_Test_001::Start");
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand cmd(geoKnowledgeUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(LATITUDE, 31.3107738494873);
    valuesBucket.Put(LONGITUDE, 120.6175308227539);
    valuesBucket.Put(LOCATION_KEY, 141189990037);
    valuesBucket.Put(LANGUAGE, "zh");
    valuesBucket.Put(COUNTRY, "中国");
    valuesBucket.Put(CITY_ID, "123456789101232");
    valuesBucket.Put(ADMIN_AREA, "江苏省");
    valuesBucket.Put(LOCALITY, "苏州市");
    valuesBucket.Put(SUB_LOCALITY, "姑苏区");
    valuesBucket.Put(THOROUGHFARE, "人民路");
    valuesBucket.Put(SUB_THOROUGHFARE, "1285号");
    valuesBucket.Put(CITY_NAME, "苏州市");
    valuesBucket.Put(ADDRESS_DESCRIPTION, "中国江苏省苏州市姑苏区人民路辅路苏州国美电器人名路旗舰店");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Location_InsertGeoKnowledge_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryLocationTest, Location_InsertGeoKnowledge_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_InsertGeoKnowledge_Test_002::Start");
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand cmd(geoKnowledgeUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(LATITUDE, 31.3211254555);
    valuesBucket.Put(LONGITUDE, 120.12356458855);
    valuesBucket.Put(LOCATION_KEY, 141189990037);
    valuesBucket.Put(LANGUAGE, "zh");
    valuesBucket.Put(COUNTRY, "中国");
    valuesBucket.Put(ADMIN_AREA, "江苏省");
    valuesBucket.Put(CITY_ID, "12115546546521");
    valuesBucket.Put(LOCALITY, "淮安市");
    valuesBucket.Put(SUB_LOCALITY, "姑苏区");
    valuesBucket.Put(THOROUGHFARE, "人民路");
    valuesBucket.Put(SUB_THOROUGHFARE, "1285号");
    valuesBucket.Put(FEATURE_NAME, "人民路辅路苏州国美电器人名路旗舰店");
    valuesBucket.Put(CITY_NAME, "淮安市");
    valuesBucket.Put(ADDRESS_DESCRIPTION, "中国江苏省淮安市姑苏区人民路辅路苏州国美电器人名路旗舰店");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    EXPECT_GT(retVal1, 0);
    EXPECT_GT(retVal2, 0);
    MEDIA_INFO_LOG("Location_InsertGeoKnowledge_Test_002::retVal = %{public}d. retVal2 = %{public}d. End",
        retVal1, retVal2);
}

HWTEST_F(MediaLibraryLocationTest, Location_UpdateGeoKnowledge_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_UpdateGeoKnowledge_Test_001::Start");
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand cmd(geoKnowledgeUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(LATITUDE, 31.3107738494873);
    valuesBucket.Put(LONGITUDE, 120.6175308227539);
    valuesBucket.Put(LOCATION_KEY, 141189990037);
    valuesBucket.Put(LANGUAGE, "en");
    valuesBucket.Put(COUNTRY, "China");
    valuesBucket.Put(ADMIN_AREA, "Jiangsu");
    valuesBucket.Put(CITY_ID, "123456789101232");
    valuesBucket.Put(LOCALITY, "Suzhou");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(ADMIN_AREA, "Shanghai");
    updateValues.Put(LOCALITY, "shanghai");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("141189990037");
    inValues.push_back("140877099576");
    predicates.In(LOCATION_KEY, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    vector<string> columns;
    columns.push_back(ADMIN_AREA);
    columns.push_back(LOCALITY);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    string adminArea;
    string locality;
    resultSet->GetString(0, adminArea);
    resultSet->GetString(1, locality);
    MEDIA_INFO_LOG("Location_UpdateGeoKnowledge_Test_001::adminArea = %{public}s locality = %{public}s. End",
        adminArea.c_str(), locality.c_str());
}

HWTEST_F(MediaLibraryLocationTest, Location_DeleteGeoKnowledge_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_DeleteGeoKnowledge_Test_001::Start");
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand cmd(geoKnowledgeUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(LATITUDE, 31.123255565444);
    valuesBucket.Put(LONGITUDE, 120.123454565544);
    valuesBucket.Put(LOCATION_KEY, 12354648855);
    valuesBucket.Put(LANGUAGE, "en");
    valuesBucket.Put(COUNTRY, "China");
    valuesBucket.Put(ADMIN_AREA, "Jiangsu");
    valuesBucket.Put(LOCALITY, "Suzhou");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("12354648855");
    inValues.push_back("140877099576");
    predicates.In(LOCATION_KEY, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Location_DeleteOcr_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryLocationTest, Location_InsertGeoDictionary_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_InsertGeoDictionary_Test_001::Start");
    Uri geoKnowDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand cmd(geoKnowDictionaryUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CITY_ID, "123456789123456789");
    valuesBucket.Put(LANGUAGE, "zh");
    valuesBucket.Put(CITY_NAME, "江苏");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Location_InsertGeoDictionary_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryLocationTest, Location_InsertGeoDictionary_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_InsertGeoDictionary_Test_002::Start");
    Uri geoKnowDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand cmd(geoKnowDictionaryUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CITY_ID, "123456789123456789");
    valuesBucket.Put(LANGUAGE, "zh");
    valuesBucket.Put(CITY_NAME, "江苏");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal1, 0);
    EXPECT_EQ(retVal2, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("Location_InsertGeoDictionary_Test_002::retVal = %{public}d. retVal2 = %{public}d. End",
        retVal1, retVal2);
}

HWTEST_F(MediaLibraryLocationTest, Location_UpdateGeoDictionary_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_UpdateGeoDictionary_Test_001::Start");
    Uri geoKnowDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand cmd(geoKnowDictionaryUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CITY_ID, "123456789123456789");
    valuesBucket.Put(LANGUAGE, "en");
    valuesBucket.Put(CITY_NAME, "Jiangsu");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(CITY_NAME, "Shanghai");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("123456789123456789");
    inValues.push_back("987654321987654321");
    predicates.In(CITY_ID, inValues);
    predicates.And();
    predicates.EqualTo(LANGUAGE, "en");
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    vector<string> columns;
    columns.push_back(CITY_NAME);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    string cityName;
    resultSet->GetString(0, cityName);
    MEDIA_INFO_LOG("Location_UpdateGeoDictionary_Test_001::cityName = %{public}s. End", cityName.c_str());
}

HWTEST_F(MediaLibraryLocationTest, Location_DeleteGeoDictionary_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_DeleteGeoDictionary_Test_001::Start");
    Uri geoKnowDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand cmd(geoKnowDictionaryUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CITY_ID, "123456789123456789");
    valuesBucket.Put(LANGUAGE, "en");
    valuesBucket.Put(CITY_NAME, "Jiangsu");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("123456789123456789");
    inValues.push_back("987654321987654321");
    predicates.In(CITY_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Location_DeleteGeoDictionary_Test_001::retVal = %{public}d. End", retVal);
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    ++num;
    return seconds.count() + num.load();
}

string GetTitle(int64_t &timestamp)
{
    ++num;
    return "IMG_" + to_string(timestamp) + "_" + to_string(num.load());
}

int64_t InsertPhoto(double_t latitude, double_t longitude)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE(rdbStore, nullptr);
    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string path = "/storage/cloud/files/photo/1/" + displayName;
    int32_t position = 2;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t imageDuration = 2560;
    int32_t imageWidth = 1920;
    int32_t imageHeight = 1080;
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, imageSize);
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, imageDuration);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestamp);
    valuesBucket.PutDouble(LATITUDE, latitude);
    valuesBucket.PutDouble(LONGITUDE, longitude);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    int32_t ret = rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    return fileId;
}

int32_t InsertGeoKnowledge(double_t latitude, double_t longitude, const string &language)
{
    MEDIA_INFO_LOG("InsertGeoKnowledge::Start");
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand cmd(geoKnowledgeUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(LATITUDE, latitude);
    valuesBucket.Put(LONGITUDE, longitude);
    const int64_t locationKey = 131048514448;
    valuesBucket.Put(LOCATION_KEY, locationKey);
    valuesBucket.Put(LANGUAGE, language);
    valuesBucket.Put(COUNTRY, "中国");
    valuesBucket.Put(CITY_ID, "1064019431304993816");
    valuesBucket.Put(ADMIN_AREA, "广东省");
    valuesBucket.Put(LOCALITY, "深圳市");
    valuesBucket.Put(SUB_LOCALITY, "南山区");
    valuesBucket.Put(THOROUGHFARE, "科苑南路");
    valuesBucket.Put(SUB_THOROUGHFARE, "2600号");
    valuesBucket.Put(CITY_NAME, "深圳市");
    valuesBucket.Put(ADDRESS_DESCRIPTION, "广东省深圳市南山区粤海街道深圳人才公园");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    int32_t retVal = dataManager->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("InsertGeoKnowledge::End, retVal = %{public}d", retVal);
    return retVal;
}

HWTEST_F(MediaLibraryLocationTest, Location_QueryGeo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Location_QueryGeo_Test_001::Start");
    double_t latitude = 22.5142917630556;
    double_t longitude = 113.946701049722;
    int64_t fileId = InsertPhoto(latitude, longitude);
    EXPECT_GT(fileId, 0);
    string language = Global::I18n::LocaleConfig::GetSystemLanguage();
    int32_t retVal = InsertGeoKnowledge(latitude, longitude, language);
    EXPECT_GT(retVal, 0);

    Uri cmdUri(PAH_QUERY_ANA_ADDRESS);
    MediaLibraryCommand cmd(cmdUri);
    vector<string> columns{ PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE,
        ADDRESS_DESCRIPTION };
    DataShare::DataSharePredicates predicates;
    vector<string> clause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE + " = " +
        GEO_KNOWLEDGE_TABLE + "." + LATITUDE + " AND " + PhotoColumn::PHOTOS_TABLE + "." +
        PhotoColumn::PHOTO_LONGITUDE + " = " + GEO_KNOWLEDGE_TABLE + "." + LONGITUDE + " AND " + GEO_KNOWLEDGE_TABLE +
        "." + LANGUAGE + " = \'" + language + "\'" };
    predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(clause);
    predicates.EqualTo(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, to_string(fileId));
    int errCode = 0;
    auto dataManager = MediaLibraryDataManager::GetInstance();
    EXPECT_NE(dataManager, nullptr);
    auto queryResultSet = dataManager->Query(cmd, columns, predicates, errCode);
    EXPECT_NE(queryResultSet, nullptr);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToNextRow(), NativeRdb::E_OK);
    double_t queryLatitude = GetDoubleVal(PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, resultSet);
    EXPECT_EQ(queryLatitude, latitude);
    double_t queryLongitude = GetDoubleVal(PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, resultSet);
    EXPECT_EQ(queryLongitude, longitude);
    string addressDescription = GetStringVal(ADDRESS_DESCRIPTION, resultSet);
    EXPECT_EQ(addressDescription.empty(), false);
    MEDIA_INFO_LOG("Location_QueryGeo_Test_001::End");
}
} // namespace OHOS
} // namespace Media