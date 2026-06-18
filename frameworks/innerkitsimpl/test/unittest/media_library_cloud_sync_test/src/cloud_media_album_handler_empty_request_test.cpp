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
#include "cloud_media_album_handler_empty_request_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <fstream>
#include "i_cloud_media_data_handler.h"
#include "cloud_media_data_handler.h"
#include "medialibrary_errno.h"
#include "mdk_record.h"

using namespace testing::ext;
using namespace testing::internal;
namespace OHOS::Media::CloudSync {

void CloudMediaAlbumHandlerEmptyRequestTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerEmptyRequestTest SetUpTestCase";
}

void CloudMediaAlbumHandlerEmptyRequestTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerEmptyRequestTest TearDownTestCase";
}

// SetUp:Execute before each test case
void CloudMediaAlbumHandlerEmptyRequestTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerEmptyRequestTest SetUp";
}

void CloudMediaAlbumHandlerEmptyRequestTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerEmptyRequestTest TearDown";
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnCreateRecords_EMPTY, TestSize.Level1)
{
    std::map<std::string, MDKRecordOperResult> map;
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnFetchRecords_EMPTY, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t ret;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    std::vector<MDKRecord> records;
    std::vector<CloudMetaData> newData;
    std::vector<CloudMetaData> fdirtyData;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats;

    ret = dataHandler->OnFetchRecords(records, newData, fdirtyData, failedRecords, stats);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnMdirtyRecords_EMPTY, TestSize.Level1)
{
    std::map<std::string, MDKRecordOperResult> map;
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    int32_t ret = dataHandler->OnMdirtyRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnFdirtyRecords_EMPTY, TestSize.Level1)
{
    std::map<std::string, MDKRecordOperResult> map;
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    int32_t ret = dataHandler->OnFdirtyRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnDeleteRecords_EMPTY, TestSize.Level1)
{
    std::map<std::string, MDKRecordOperResult> map;
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    int32_t ret = dataHandler->OnDeleteRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaAlbumHandlerEmptyRequestTest, OnCopyRecords_EMPTY, TestSize.Level1)
{
    std::map<std::string, MDKRecordOperResult> map;
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    int32_t ret = dataHandler->OnCopyRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
}
}  // namespace OHOS::Media::CloudSync