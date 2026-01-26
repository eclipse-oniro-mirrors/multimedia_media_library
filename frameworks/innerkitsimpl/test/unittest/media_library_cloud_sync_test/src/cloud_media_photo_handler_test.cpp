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

#include "cloud_media_photo_handler_test.h"

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
namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaPhotoHandlerTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoHandlerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest SetUpTestCase";
    // Get RdbStore
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoHandlerTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest SetUpTestCase ret: " << ret;
}

void CloudMediaPhotoHandlerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaPhotoHandlerTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest SetUp";
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    (void)dataHandler->OnCompletePush();
}

void CloudMediaPhotoHandlerTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest TearDown";
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_Mdirty, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t mdirty = 2;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targets;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_meta_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targets);
    EXPECT_EQ(targets.size(), 1);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite", "recycled", "thumb_size", "lcd_size", "cover_position", "size",
        "editDataCamera", "file_position", "sourceFileName", "sourcePath", "data", "original_asset_cloud_id",
        "cloud_id", "front_camera", "shooting_mode_tag", "shooting_mode", "date_day", "date_month", "date_year",
        "burst_key", "virtual_path", "relative_path", "title", "mimeType", "description", "source", "hashId", "type",
        "fileName", "rotate", "width", "height", "fix_version", "owner_album_id" "strong_association",
        "supported_watermark_type", "moving_photo_effect_mode", "original_subtype", "dynamic_range_type",
        "burst_cover_level", "subtype", "hidden", "duration", "media_type", "fileType"};
    for (auto &record : records) {
        for (auto &target : targets) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
            }
        }
    }
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_Sdirty, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t sdirty = 6;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, sdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targets;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_meta_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targets);
    EXPECT_EQ(targets.size(), 1);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite", "recycled", "thumb_size", "lcd_size", "cover_position", "size",
        "editDataCamera", "file_position", "sourceFileName", "sourcePath", "data", "original_asset_cloud_id",
        "cloud_id", "front_camera", "shooting_mode_tag", "shooting_mode", "date_day", "date_month", "date_year",
        "burst_key", "virtual_path", "relative_path", "title", "mimeType", "description", "source", "hashId", "type",
        "fileName", "rotate", "width", "height", "fix_version", "owner_album_id" "strong_association",
        "supported_watermark_type", "moving_photo_effect_mode", "original_subtype", "dynamic_range_type",
        "burst_cover_level", "subtype", "hidden", "duration", "media_type", "fileType"};
    for (auto &record : records) {
        for (auto &target : targets) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
            }
        }
    }
}

/**
 * 获取MetaModifiedRecords， 更新收藏字段
 * 期望结果：
 * 输出对应数量的records，且records的信息与实际修改一致
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_Update_Favorite, TestSize.Level1)
{
    // 更新数据库收藏字段
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds = {"374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d9"};

    int32_t changeNum = dao.UpdatePhotoFavorite(cloudIds, 1);
    EXPECT_EQ(changeNum, cloudIds.size());

    // 查询数据
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t mdirty = 2;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    // 检验字段
    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        MEDIA_INFO_LOG("GetMetaModifiedRecords_Update_Favorite: data is %{public}s, %{public}d",
                       record.GetRecordId().c_str(), data.GetFavorite().value());
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && data.GetFavorite().value_or(false)) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());

    records = {};
    changeNum = dao.UpdatePhotoFavorite(cloudIds, 0);
    EXPECT_EQ(changeNum, cloudIds.size());
    ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        MEDIA_INFO_LOG("GetMetaModifiedRecords_Update_Favorite: data is %{public}s, %{public}d",
                       record.GetRecordId().c_str(), data.GetFavorite().value());
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && !data.GetFavorite().value_or(false)) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());
}

/**
 * 获取MetaModifiedRecords， 更新隐藏字段
 * 期望结果：
 * 输出对应数量的records，且records的信息与实际修改一致
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_Update_Hidden, TestSize.Level1)
{
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds = {"374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d9"};
    int32_t changeNum = dao.UpdatePhotoHidden(cloudIds, 1);
    EXPECT_EQ(changeNum, cloudIds.size());

    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t mdirty = 2;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        MEDIA_INFO_LOG("GetMetaModifiedRecords_Update_Hidden: data is %{public}s, %{public}d",
                       record.GetRecordId().c_str(), data.GetHidden().value());
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && data.GetHidden().value_or(-1) == 1) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());

    records = {};
    changeNum = dao.UpdatePhotoHidden(cloudIds, 0);
    EXPECT_EQ(changeNum, cloudIds.size());
    ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        MEDIA_INFO_LOG("GetMetaModifiedRecords_Update_Hidden: data is %{public}s, %{public}d",
                       record.GetRecordId().c_str(), data.GetHidden().value());
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && data.GetHidden().value_or(-1) == 0) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());
}

/**
 * 获取MetaModifiedRecords， 更新图片名称
 * 期望结果：
 * 输出对应数量的records，且records的信息与实际修改一致
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_Update_PhotoName, TestSize.Level1)
{
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds = {"374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d9"};
    std::string updateName = "GetMetaModifiedRecords_update_name.jpg";
    int32_t changeNum = dao.UpdatePhotoName(cloudIds, updateName);
    EXPECT_EQ(changeNum, cloudIds.size());

    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t mdirty = 2;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && data.GetFileName().value_or("") == updateName) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());
}

/**
 * 获取MetaModifiedRecords， 删除图片
 * 期望结果：
 * 输出对应数量的records，且records的信息与实际修改一致
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetMetaModifiedRecords_To_Trash, TestSize.Level1)
{
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds = {"374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d9"};

    int32_t changeNum = dao.UpdatePhotoDateTrashed(cloudIds, 2);
    EXPECT_EQ(changeNum, cloudIds.size());

    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t mdirty = 2;
    int32_t ret = dataHandler->GetMetaModifiedRecords(records, size, mdirty);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordPhotosData data = MDKRecordPhotosData(record);
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId && data.GetRecycled().value_or(false)) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());
}

/**
 * GetFileModifiedRecords基础用例
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetFileModifiedRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetFileModifiedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_file_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "file_position",
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
    int32_t checkCount = 0;
    Json::FastWriter writer;
    for (auto record : records) {
        std::string json = writer.write(record.ToJsonValue());
        GTEST_LOG_(INFO) << "record:" << json;
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, targetRecords.size());
}

/**
 * 本地静态图片进行裁剪/旋转/添加滤镜并覆盖原图
 * 期望结果:
 * 输出的records字段完整，符合云端上传字段要求，云端图片和本地一致
 * 错误码为期望值E_OK
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetFileModifiedRecords_case001, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetFileModifiedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_file_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "file_position",
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
    std::string cloudId = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d2-20250511203739";
    int32_t checkCount = 0;
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    bool found = false;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
            if (record.GetRecordId() == cloudId) {
                found = true;
                MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
                std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudId(cloudId);
                EXPECT_GT(photos.size(), 0);
                ORM::PhotosPo photo;
                int32_t retInside = dao.GetPhotoByCloudId(photos, cloudId, photo);
                EXPECT_EQ(retInside, 0);
                EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
                EXPECT_GT(photo.dateModified.value_or(-1), 0);
            }
        }
    }
    EXPECT_TRUE(found);
    EXPECT_EQ(checkCount, targetRecords.size());
}

/**
 * 本地动态图片进行裁剪/旋转/添加滤镜并覆盖原图
 * 期望结果:
 * records字段附件信息都有，附件均能上传成功，云端呈现动图
 * 错误码为期望值E_OK
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetFileModifiedRecords_case002, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetFileModifiedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_file_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "file_position",
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
    std::string cloudId = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d3-20250511220158";
    int32_t checkCount = 0;
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    bool found = false;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
            if (record.GetRecordId() == cloudId) {
                found = true;
                MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
                std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudId(cloudId);
                EXPECT_GT(photos.size(), 0);
                ORM::PhotosPo photo;
                int32_t retInside = dao.GetPhotoByCloudId(photos, cloudId, photo);
                EXPECT_EQ(retInside, 0);
                EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
                EXPECT_GT(photo.dateModified.value_or(-1), 0);
            }
        }
    }
    EXPECT_TRUE(found);
    EXPECT_EQ(checkCount, targetRecords.size());
}

/**
 * 本地动态图片进行涂鸦并覆盖原图
 * 期望结果:
 * 1. 云端呈现静态图片
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetFileModifiedRecords_case003, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetFileModifiedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_file_modified_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "thumb_size",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "file_position",
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
    std::string cloudId = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d4";
    int32_t checkCount = 0;
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    bool found = false;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
            if (record.GetRecordId() == cloudId) {
                found = true;
                MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
                std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudId(cloudId);
                EXPECT_GT(photos.size(), 0);
                ORM::PhotosPo photo;
                int32_t retInside = dao.GetPhotoByCloudId(photos, cloudId, photo);
                EXPECT_EQ(retInside, 0);
                EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
                EXPECT_GT(photo.dateModified.value_or(-1), 0);
            }
        }
    }
    EXPECT_TRUE(found);
    EXPECT_EQ(checkCount, targetRecords.size());
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetDeletedRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_delete_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"cloud_id"};
    int32_t checkCount = 0;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, targetRecords.size());
}

/**
 * 本地删除10张图片进入回收站
 * 期望结果:
 * 1. 本地删除进入回收站的图片的date_trashed字段不为0
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetDeletedRecords_001, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::string cloudId = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d0";
    Json::FastWriter writer;
    for (auto record : records) {
        std::string json = writer.write(record.ToJsonValue());
        GTEST_LOG_(INFO) << "GetDeletedRecords_001:" << json;
        EXPECT_TRUE(!record.GetRecordId().empty());
        EXPECT_TRUE(record.GetRecordId() != cloudId);
    }
    std::string data = "/storage/cloud/files/Photo/13/IMG_1739459145_10026.jpg";
    std::vector<std::string> datas = {data};
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    ASSERT_EQ(photos.size(), datas.size());
    ORM::PhotosPo photo = photos[0];
    EXPECT_TRUE(photo.dateTrashed.value_or(0) > 0);
    EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
}

/**
 * 本地彻底删除10张图片
 * 期望结果:
 * 1. 本地彻底删除的图片的date_trashed字段不为0且dirty为TYPE_DELETED
 */
HWTEST_F(CloudMediaPhotoHandlerTest, GetDeletedRecords_002, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    bool found = false;
    std::string cloudId = "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d1";
    for (auto &record : records) {
        EXPECT_TRUE(!record.GetRecordId().empty());
        if (record.GetRecordId() == cloudId) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetCopyRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 20;
    int32_t ret = dataHandler->GetCopyRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/photo_handler_get_copy_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"favorite",
                                            "recycled",
                                            "cover_position",
                                            "size",
                                            "editDataCamera",
                                            "file_position",
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
                                            "media_type"};
    int32_t checkCount = 0;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::PHOTO));
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, targetRecords.size());
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetCheckRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<std::string> cloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97cc"};
    std::string fileName = "IMG_1739459148_022.jpg";
    std::string path = "/storage/cloud/files/Photo/2/";
    std::unordered_map<std::string, CloudCheckData> checkRecords;
    int32_t ret = dataHandler->GetCheckRecords(cloudIds, checkRecords);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(checkRecords.size(), 1);
    auto record = checkRecords.find("373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97cc");
    ASSERT_TRUE(record != checkRecords.end());
    EXPECT_TRUE(std::find(cloudIds.begin(), cloudIds.end(), cloudIds[0]) != cloudIds.end());
    EXPECT_EQ(path, record->second.path);
    EXPECT_EQ(fileName, record->second.fileName);
    EXPECT_EQ(record->second.attachment.size(), 2);
    PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, record->second.cloudId, photo);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << photo.ToString();
    EXPECT_EQ(photo.dirty.value_or(-1), record->second.dirtyType);
    EXPECT_EQ(photo.thumbStatus.value_or(-1), record->second.thmStatus);
    EXPECT_EQ(photo.syncStatus.value_or(-1), record->second.syncStatus);
    for (auto [key, value] : record->second.attachment) {
        EXPECT_TRUE(!value.fileName.empty());
        EXPECT_TRUE(!value.filePath.empty());
        EXPECT_TRUE(value.fileName.find('/') == std::string::npos);
        EXPECT_TRUE(value.filePath.rfind('/') == value.filePath.length() - 1);
        EXPECT_GT(value.size, 0);
    }
}

HWTEST_F(CloudMediaPhotoHandlerTest, OnCreateRecords, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_create_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    int32_t size = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    for (auto &record : records) {
        GTEST_LOG_(INFO) << "query record id:" << record.GetRecordId();
        for (auto &entry : map) {
            if (record.GetRecordId() != entry.first) {
                continue;
            }
            GTEST_LOG_(INFO) << "find record id:" << entry.first;
            std::map<std::string, MDKRecordField> recordData;
            record.GetRecordData(recordData);
            auto it = recordData.find("local_id");
            if (it == recordData.end()) {
                continue;
            }
            int64_t localId = -1;
            MDKLocalErrorCode errorCode = it->second.GetLong(localId);
            if (errorCode != MDKLocalErrorCode::NO_ERROR) {
                continue;
            }
            MDKRecord mdkRecord = entry.second.GetDKRecord();
            GTEST_LOG_(INFO) << "find local id:" << localId;
            std::map<std::string, MDKRecordField> data;
            mdkRecord.GetRecordData(data);
            data["local_id"] = MDKRecordField(static_cast<int32_t>(localId));
            std::map<std::string, MDKRecordField> attributes = data["attributes"];
            attributes["file_id"] = MDKRecordField(static_cast<int32_t>(localId));
            data["attributes"] = MDKRecordField(attributes);
            mdkRecord.SetRecordData(data);
            entry.second.SetDKRecord(mdkRecord);
        }
    }
    ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);
}

/**
 * 元数据修改结果处理上行
 * 期望结果:
 * 1.数据处理成功 返回ret = E_OK，failSize = 0，成功的数据刷新dirty = TYPE_SYNCED、version字段
 * 2.上传数据存在失败 map数据设置对应的失败数据 输出结果:同新增上行结果处理失败处理
 */
HWTEST_F(CloudMediaPhotoHandlerTest, OnMdirtyRecords, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_modify_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnMdirtyRecords(map, failSize);
    EXPECT_EQ(ret, FileManagement::E_STOP);
    std::vector<std::string> successCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c1"};
    std::vector<std::string> failedCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97ca"};
    EXPECT_EQ(failSize, failedCloudIds.size());

    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos;
    photos = dao.QueryPhotosByCloudIds(successCloudIds);
    EXPECT_EQ(photos.size(), successCloudIds.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnMdirtyRecords Photo:" << photo.ToString();

        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 1);
    }

    photos = dao.QueryPhotosByCloudIds(failedCloudIds);
    EXPECT_EQ(photos.size(), failedCloudIds.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnMdirtyRecords Photo:" << photo.ToString();

        EXPECT_TRUE(photo.dirty.value_or(-1) ==
                    static_cast<int32_t>(DirtyType::TYPE_DELETED));  //本地数据不变为TYPE_SYNCED
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);           //本地数据没变
    }
}

/**
 * 文件修改结果处理上行
 * 期望结果:
 * 1.数据处理成功 返回ret = E_OK，failSize = 0，成功的数据刷新dirty = TYPE_SYNCED、version字段
 * 2.上传数据存在失败 map数据设置对应的失败数据 输出结果:同新增上行结果处理失败处理
 */
HWTEST_F(CloudMediaPhotoHandlerTest, OnFdirtyRecords, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fdirty_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnFdirtyRecords(map, failSize);
    EXPECT_EQ(ret, FileManagement::E_STOP);
    std::vector<std::string> recordIdSuccess = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97cd"};
    std::vector<std::string> recordIdFailed = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97cc"};
    EXPECT_EQ(failSize, recordIdFailed.size());

    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos;
    photos = dao.QueryPhotosByCloudIds(recordIdSuccess);
    EXPECT_EQ(photos.size(), recordIdSuccess.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnFdirtyRecords Photo:" << photo.ToString();

        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 1);
    }

    photos = dao.QueryPhotosByCloudIds(recordIdFailed);
    EXPECT_EQ(photos.size(), recordIdFailed.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnFdirtyRecords Photo:" << photo.ToString();

        EXPECT_TRUE(photo.dirty.value_or(-1) ==
                    static_cast<int32_t>(DirtyType::TYPE_COPY));  //本地数据不变为TYPE_SYNCED
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);        //本地数据没变
    }
}

/**
 * 上传数据结果中存在上传失败
 * 期望结果：
 * 删除成功的记录在数据库中查询不到，删除失败的记录可以查询到
 */
HWTEST_F(CloudMediaPhotoHandlerTest, OnDeleteRecords_Has_Error, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_delete_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);

    std::string recordIdSuccess = "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d2";
    std::string recordIdFailed = "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d3";

    for (auto it = map.begin(); it != map.end(); ++it) {
        MDKError error;
        if (it->first == recordIdSuccess) {
            error.SetLocalError(MDKLocalErrorCode::NO_ERROR);
            it->second.SetDKError(error);
        }
        if (it->first == recordIdFailed) {
            error.SetLocalError(MDKLocalErrorCode::IPC_SEND_FAILED);
            it->second.SetDKError(error);
        }
    }

    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t failSize;
    int32_t ret = dataHandler->OnDeleteRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 1);

    // 校验结果
    std::vector<std::string> cloudIds = {recordIdSuccess, recordIdFailed};
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_GT(photos.size(), 0);

    int32_t num = 0;
    for (auto cloudId : cloudIds) {
        PhotosPo photo = PhotosPo();
        if (cloudId == recordIdSuccess) {
            EXPECT_EQ(dao.GetPhotoByCloudId(photos, cloudId, photo), -1);
        }
        if (cloudId == recordIdFailed) {
            EXPECT_EQ(dao.GetPhotoByCloudId(photos, cloudId, photo), 0);
            num++;
        }
    }
    EXPECT_EQ(num, 1);
}

/**
 * 上传数据结果中全部成功
 * 期望结果：
 * 删除成功的记录在数据库中查询不到
 */
HWTEST_F(CloudMediaPhotoHandlerTest, OnDeleteRecords_No_Error, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_delete_records_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);

    std::string recordIdSuccess = "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d2";
    std::string recordIdFailed = "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d3";

    for (auto it = map.begin(); it != map.end(); ++it) {
        MDKError error;
        if (it->first == recordIdSuccess) {
            error.SetLocalError(MDKLocalErrorCode::NO_ERROR);
            it->second.SetDKError(error);
        }
        if (it->first == recordIdFailed) {
            error.SetLocalError(MDKLocalErrorCode::NO_ERROR);
            it->second.SetDKError(error);
        }
    }
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t failSize;
    int32_t ret = dataHandler->OnDeleteRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);

    // 校验结果
    std::vector<std::string> cloudIds = {recordIdSuccess, recordIdFailed};
    TestUtils::PhotosDao dao = TestUtils::PhotosDao();
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), 0);
}

HWTEST_F(CloudMediaPhotoHandlerTest, OnCompletePush, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoHandlerTest, OnStartSync, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoHandlerTest, OnCompleteSync, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

// Photos does not implement the following methods [OnDeleteAlbums]
HWTEST_F(CloudMediaPhotoHandlerTest, OnDeleteAlbums, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoHandlerTest, GetRetryRecords, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<std::string> records;
    int32_t ret = dataHandler->GetRetryRecords(records);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::string targeRecordId;
    for (auto &record : records) {
        if (record == "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c2") {
            targeRecordId = record;
            break;
        }
    }
    EXPECT_EQ(targeRecordId, "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c2");
}

/**
 * OnCompletePull
 * 期望结果：
 * 预置cloudId图片的资源PHOTO_SYNC_STATUS 置为0
 */
HWTEST_F(CloudMediaPhotoHandlerTest, OnCompletePull, TestSize.Level1)
{
    std::vector<std::string> cloudIds = {"374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d4",
                                         "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d5",
                                         "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d6",
                                         "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d7",
                                         "374b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d8"};

    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    MediaOperateResult optRet = {"", 0, ""};
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCompletePull(optRet);
    EXPECT_EQ(ret, 0);

    PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());

    int32_t num = 0;
    for (auto &photo : photos) {
        if (photo.syncStatus.value_or(-2) == 0) {
            num++;
        }
    }
    EXPECT_EQ(num, photos.size());
}
}  // namespace OHOS::Media::CloudSync