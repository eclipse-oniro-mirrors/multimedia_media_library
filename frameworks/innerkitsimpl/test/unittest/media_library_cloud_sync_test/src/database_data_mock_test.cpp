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

#define MLOG_TAG "MediaCloudSync"

#include "database_data_mock_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "media_log.h"
#include "database_data_mock.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {
void DatabaseDataMockTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void DatabaseDataMockTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

// SetUp:Execute before each test case
void DatabaseDataMockTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void DatabaseDataMockTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(DatabaseDataMockTest, CSVFileReader, TestSize.Level1)
{
    std::string csvFileFolder = "/data/test/cloudsync/";
    std::string csvFileName = "database_mock-photos.csv";
    CSVFileReader csvFileReader(csvFileFolder + csvFileName);
    int32_t ret = csvFileReader.ReadCSVFile();
    EXPECT_EQ(ret, CSVFileReader::E_OK);
    std::vector<std::string> headers = csvFileReader.GetHeaderNames();
    bool hasColumn = false;
    for (auto &header : headers) {
        if (header == "data") {
            hasColumn = true;
            break;
        }
    }
    EXPECT_TRUE(hasColumn);
    std::vector<CSVRowData> rows = csvFileReader.GetRows();
    EXPECT_GT(rows.size(), 0);
    bool hasValue = false;
    for (auto &row : rows) {
        std::string columnValue = csvFileReader.GetString(row, "data");
        if (columnValue.size() > 0) {
            hasValue = true;
            break;
        }
    }
    EXPECT_TRUE(hasValue);
}

HWTEST_F(DatabaseDataMockTest, DATABASE, TestSize.Level1)
{
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    EXPECT_EQ(errorCode, 0);
    ASSERT_NE(rdbStore, nullptr);
    auto resultSet = rdbStore->QuerySql("SELECT 1");
    ASSERT_NE(resultSet, nullptr);
}

HWTEST_F(DatabaseDataMockTest, DATABASE_DATA_MOCK, TestSize.Level1)
{
    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    EXPECT_EQ(errorCode, 0);
    ASSERT_NE(rdbStore, nullptr);
    // Test CheckPoint
    DatabaseDataMock dbDataMock;
    int32_t ret = dbDataMock.SetRdbStore(rdbStore).CheckPoint();
    EXPECT_EQ(ret, DatabaseDataMock::E_OK);
        int64_t maxFileId = dbDataMock.GetMaxFileId();
    int64_t maxAlbumId = dbDataMock.GetMaxAlbumId();
    ASSERT_GE(maxFileId, 0);
    ASSERT_GE(maxAlbumId, 0);
    auto photoResultSet = rdbStore->QuerySql("SELECT COUNT(*) as cnt FROM Photos");
    ASSERT_NE(photoResultSet, nullptr);
    // Test Mock Data
    ret = dbDataMock.MockData(DatabaseDataMockTest::GetTableMockInfoList());
    EXPECT_EQ(ret, DatabaseDataMock::E_OK);
    auto mockResultSet = rdbStore->QuerySql("SELECT COUNT(*) as cnt FROM Photos");
    ASSERT_NE(mockResultSet, nullptr);
    // Test Rollback
    ret = dbDataMock.Rollback();
    EXPECT_EQ(ret, DatabaseDataMock::E_OK);
}
}  // namespace OHOS::Media::CloudSync