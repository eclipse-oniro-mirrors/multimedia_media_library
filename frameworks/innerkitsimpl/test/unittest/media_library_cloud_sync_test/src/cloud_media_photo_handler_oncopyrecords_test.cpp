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
#include "cloud_media_photo_handler_oncopyrecords_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <fstream>

#include "cloud_check_data.h"
#include "cloud_file_data.h"
#include "cloud_media_data_client.h"
#include "cloud_media_data_handler.h"
#include "cloud_meta_data.h"
#include "i_cloud_media_data_handler.h"
#include "json/json.h"
#include "json_file_reader.h"
#include "media_log.h"
#include "mdk_asset.h"
#include "mdk_database.h"
#include "mdk_error.h"
#include "mdk_record_field.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_record_photos_data.h"
#include "mdk_record_utils.h"
#include "cloud_data_utils.h"
#include "photos_dao.h"
#include "cloud_file_error.h"

using namespace testing::ext;
using namespace testing::internal;
using namespace OHOS::Media::ORM;
namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaPhotoHandlerOnCopyRecordsTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoHandlerOnCopyRecordsTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoHandlerOnCopyRecordsTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest SetUpTestCase ret: " << ret;
}

void CloudMediaPhotoHandlerOnCopyRecordsTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaPhotoHandlerOnCopyRecordsTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest SetUp";
}

void CloudMediaPhotoHandlerOnCopyRecordsTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCopyRecordsTest TearDown";
}

/**
 * 预设条件：
 * 期望结果:
 * 1.
 */
HWTEST_F(CloudMediaPhotoHandlerOnCopyRecordsTest, OnCopyRecords, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_copy_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize = 0;
    int32_t ret;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);

    ret = dataHandler->OnCopyRecords(map, failSize);
    EXPECT_EQ(ret, FileManagement::E_STOP);
    std::map<std::string, int32_t> successCloudIdFileIdMap = {
        {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d5", 233},
    };
    std::map<std::string, int32_t> failedCloudIdFileIdMap = {
        {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d6", 234},
    };
    std::vector<std::string> recordIdSuccess;
    std::vector<int32_t> fileIdfailed;
    EXPECT_EQ(failSize, failedCloudIdFileIdMap.size());

    for (auto pair : successCloudIdFileIdMap) {
        recordIdSuccess.emplace_back(pair.first);
    }
    for (auto pair : failedCloudIdFileIdMap) {
        fileIdfailed.emplace_back(pair.second);
    }

    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photosList;
    photosList = dao.QueryPhotosByCloudIds(recordIdSuccess);
    EXPECT_EQ(photosList.size(), successCloudIdFileIdMap.size());
    for (auto photo : photosList) {
        EXPECT_TRUE(photo.cloudId.has_value());
        EXPECT_EQ(photo.fileId.value_or(-1), successCloudIdFileIdMap[photo.cloudId.value_or("")]);
        EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        EXPECT_EQ(photo.cloudVersion.value_or(-1), 1);
        EXPECT_EQ(photo.syncStatus.value_or(-1), static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
        EXPECT_EQ(photo.originalAssetCloudId.value_or(""), "");
    }

    for (auto failedId : fileIdfailed) {
        photosList = dao.QueryPhotosByFileId(failedId);
        EXPECT_EQ(photosList.size(), 1);
        for (auto photo : photosList) {
            EXPECT_EQ(photo.cloudId.value_or(""), "");
            EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_COPY));
            EXPECT_EQ(photo.cloudVersion.value_or(-1), 0);
            EXPECT_EQ(photo.syncStatus.value_or(-1), static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
            EXPECT_EQ(photo.originalAssetCloudId.value_or(""), "Test_originalAssetCloudId");
        }
    }
}
}  // namespace OHOS::Media::CloudSync