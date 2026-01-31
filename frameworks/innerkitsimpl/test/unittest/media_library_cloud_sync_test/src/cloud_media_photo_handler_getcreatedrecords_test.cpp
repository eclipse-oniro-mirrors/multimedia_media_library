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
#include "cloud_media_photo_handler_getcreatedrecords_test.h"

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

using namespace testing::ext;
using namespace testing::internal;
using namespace OHOS::Media::ORM;
namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaPhotoHandlerGetCreatedRecordsTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoHandlerGetCreatedRecordsTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoHandlerGetCreatedRecordsTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest SetUpTestCase ret: " << ret;
}

void CloudMediaPhotoHandlerGetCreatedRecordsTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaPhotoHandlerGetCreatedRecordsTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest SetUp";
}

void CloudMediaPhotoHandlerGetCreatedRecordsTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerGetCreatedRecordsTest TearDown";
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> testRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_created_record_result.json");
    reader.ConvertToMDKRecordVector(testRecords);

    //CASE1 获取本地新增数据，THM和LCD存在，预期返回此集合数据
    //dirty = 1、ThumbnailReady >= 3、 LcdReady >= 2、MEDIA_DATE_TRASHED = "0"、MEDIA_TIME_PENDING = "0"
    std::vector<std::string> case1CloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a1",
                                              "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a3"};

    std::vector<MDKRecord> target1RecordIds;
    for (auto testRecord : testRecords) {
        for (auto case1CloudId : case1CloudIds) {
            if (testRecord.GetRecordId() == case1CloudId) {
                target1RecordIds.emplace_back(testRecord);
            }
        }
    }
    EXPECT_GT(records.size(), 0);
    EXPECT_EQ(target1RecordIds.size(), case1CloudIds.size());
    int32_t case1Count = 0;
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "lcd_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "sourceFileName",
                                            "sourcePath",
                                            "data",
                                            "original_asset_cloud_id",
                                            "cloud_id",
                                            "front_camera",
                                            "shooting_mode_tag",
                                            "shooting_mode",
                                            "date_day",
                                            "date_month",
                                            "date_year",
                                            "burst_key",
                                            "virtual_path",
                                            "relative_path",
                                            "title",
                                            "mimeType",
                                            "description",
                                            "source",
                                            "hashId",
                                            "type",
                                            "fileName",
                                            "rotate",
                                            "width",
                                            "height",
                                            "fix_version",
                                            "owner_album_id"
                                            "strong_association",
                                            "supported_watermark_type",
                                            "moving_photo_effect_mode",
                                            "original_subtype",
                                            "dynamic_range_type",
                                            "burst_cover_level",
                                            "subtype",
                                            "hidden",
                                            "duration",
                                            "media_type",
                                            "fileType"};
    for (auto &record : records) {
        for (auto &target : target1RecordIds) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                case1Count++;
            }
        }
    }
    EXPECT_EQ(case1Count, target1RecordIds.size());
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords_no_thm_lcd, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> testRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_created_record_result.json");
    reader.ConvertToMDKRecordVector(testRecords);

    //CASE2 获取本地新增数据，但是本地LCD和THUM不存在，预期返回的record中不带无缩略图的数据
    //ThumbnailReady < 3、 LcdReady < 2
    std::vector<std::string> case2CloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a2"};
    int32_t case2Count = 0;
    for (auto &record : records) {
        for (auto case2CloudId : case2CloudIds) {
            if (record.GetRecordId() == case2CloudId) {
                case2Count++;
            }
        }
    }
    EXPECT_EQ(case2Count, 0);
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords_no_srcFile, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> testRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_created_record_result.json");
    reader.ConvertToMDKRecordVector(testRecords);

    //CASE3 获取本地新增数据，但是本地LCD和THUM不存在，预期返回的record中不带无原图的数据
    //MEDIA_DATE_TRASHED > "0"
    std::vector<std::string> case3CloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a9"};
    int32_t case3Count = 0;
    for (auto &record : records) {
        for (auto case3CloudId : case3CloudIds) {
            if (record.GetRecordId() == case3CloudId) {
                case3Count++;
            }
        }
    }
    EXPECT_EQ(case3Count, 0);
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords_rollback_File, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> testRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_created_record_result.json");
    reader.ConvertToMDKRecordVector(testRecords);

    //CASE4 获取本地新增数据 新增的是编辑可回退文件 //暂无可回退文件
    std::vector<std::string> case4CloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a4"};

    std::vector<MDKRecord> target4RecordIds;
    for (auto testRecord : testRecords) {
        for (auto case4CloudId : case4CloudIds) {
            if (testRecord.GetRecordId() == case4CloudId) {
                target4RecordIds.emplace_back(testRecord);
            }
        }
    }
    EXPECT_GT(records.size(), 0);
    EXPECT_EQ(target4RecordIds.size(), case4CloudIds.size());
    int32_t case4Count = 0;
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "lcd_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "sourceFileName",
                                            "sourcePath",
                                            "data",
                                            "original_asset_cloud_id",
                                            "cloud_id",
                                            "front_camera",
                                            "shooting_mode_tag",
                                            "shooting_mode",
                                            "date_day",
                                            "date_month",
                                            "date_year",
                                            "burst_key",
                                            "virtual_path",
                                            "relative_path",
                                            "title",
                                            "mimeType",
                                            "description",
                                            "source",
                                            "hashId",
                                            "type",
                                            "fileName",
                                            "rotate",
                                            "width",
                                            "height",
                                            "fix_version",
                                            "owner_album_id"
                                            "strong_association",
                                            "supported_watermark_type",
                                            "moving_photo_effect_mode",
                                            "original_subtype",
                                            "dynamic_range_type",
                                            "burst_cover_level",
                                            "subtype",
                                            "hidden",
                                            "duration",
                                            "media_type",
                                            "fileType"};
    for (auto &record : records) {
        for (auto &target : target4RecordIds) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                case4Count++;
            }
        }
    }
    EXPECT_EQ(case4Count, target4RecordIds.size());
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords_content_File, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> testRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_created_record_result.json");
    reader.ConvertToMDKRecordVector(testRecords);

    //CASE5 获取本地新增数据 新增的是动图
    std::vector<std::string> case5CloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff96a5"};

    std::vector<MDKRecord> target5RecordIds;
    for (auto testRecord : testRecords) {
        for (auto case5CloudId : case5CloudIds) {
            if (testRecord.GetRecordId() == case5CloudId) {
                target5RecordIds.emplace_back(testRecord);
            }
        }
    }
    EXPECT_GT(records.size(), 0);
    EXPECT_EQ(target5RecordIds.size(), case5CloudIds.size());
    int32_t case5Count = 0;
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "lcd_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "sourceFileName",
                                            "sourcePath",
                                            "data",
                                            "original_asset_cloud_id",
                                            "cloud_id",
                                            "front_camera",
                                            "shooting_mode_tag",
                                            "shooting_mode",
                                            "date_day",
                                            "date_month",
                                            "date_year",
                                            "burst_key",
                                            "virtual_path",
                                            "relative_path",
                                            "title",
                                            "mimeType",
                                            "description",
                                            "source",
                                            "hashId",
                                            "type",
                                            "fileName",
                                            "rotate",
                                            "width",
                                            "height",
                                            "fix_version",
                                            "owner_album_id"
                                            "strong_association",
                                            "supported_watermark_type",
                                            "moving_photo_effect_mode",
                                            "original_subtype",
                                            "dynamic_range_type",
                                            "burst_cover_level",
                                            "subtype",
                                            "hidden",
                                            "duration",
                                            "media_type",
                                            "fileType"};
    for (auto &record : records) {
        for (auto &target : target5RecordIds) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                case5Count++;
            }
        }
    }
    EXPECT_EQ(case5Count, target5RecordIds.size());
}

HWTEST_F(CloudMediaPhotoHandlerGetCreatedRecordsTest, GetCreatedRecords_no_created, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;

    //case7 把本地dirty = new 的数据全部变成0
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::map<std::string, int32_t> cloudIdDirtydMap;
    std::vector<std::string> dbCloudIds;

    std::vector<PhotosPo> photosList = photosDao.QueryAllPhotos();
    for (auto photos : photosList) {
        if (photos.cloudId.has_value() && !photos.cloudId.value().empty()) {
            dbCloudIds.emplace_back(photos.cloudId.value_or(""));
        }
    }
    EXPECT_GE(dbCloudIds.size(), 15);  //数据库为15条数据

    for (auto dbCloudId : dbCloudIds) {
        int32_t dbDirty = photosDao.GetPhotoDirtyByCloudId(dbCloudId);
        EXPECT_GT(dbDirty, -1) << "GetPhotoDirtyByCloudId faild:" << dbCloudId;
        cloudIdDirtydMap[dbCloudId] = dbDirty;

        if (dbDirty == 1) {  //把new的数据改为sync
            photosDao.UpdatePhotoDirtyByCloudId(dbCloudId, 0);
        }
    }

    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(records.size(), 0);

    //恢复数据库的dirty内容
    for (auto dbCloudId : dbCloudIds) {
        int32_t dbDirty = photosDao.GetPhotoDirtyByCloudId(dbCloudId);
        EXPECT_GT(dbDirty, -1) << "recoving GetPhotoDirtyByCloudId faild:" << dbCloudId;

        if (cloudIdDirtydMap[dbCloudId] != dbDirty) {
            photosDao.UpdatePhotoDirtyByCloudId(dbCloudId, cloudIdDirtydMap[dbCloudId]);
        }
    }
}
}  // namespace OHOS::Media::CloudSync