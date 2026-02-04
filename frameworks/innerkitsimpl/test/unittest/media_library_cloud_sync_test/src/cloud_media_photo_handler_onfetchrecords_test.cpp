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

#include "cloud_media_photo_handler_onfetchrecords_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <fstream>
#include <sstream>
#include <map>

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
#include "album_dao.h"
#include "cloud_media_sync_const.h"

using namespace testing::ext;
using namespace testing::internal;
namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaPhotoHandlerOnFetchRecordsTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoHandlerOnFetchRecordsTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoHandlerOnFetchRecordsTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest SetUpTestCase ret: " << ret;
}

void CloudMediaPhotoHandlerOnFetchRecordsTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaPhotoHandlerOnFetchRecordsTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest SetUp";
}

void CloudMediaPhotoHandlerOnFetchRecordsTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnFetchRecordsTest TearDown";
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords, TestSize.Level1)
{
    //test insert data
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE1 新增数据
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_new_data_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    int32_t ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(stats[StatsIndex::NEW_RECORDS_COUNT], 0);
    EXPECT_GT(newDatas.size(), 0);
    std::vector<std::string> checkCloudDataList = {
        "cloudId", "size",         "path",      "fileName",
        "type",    "modifiedTime", "thumbnail", "lcdThumbnail"};  //new data 无 "originalCloudId"
    int32_t checkCount = 0;
    std::vector<CloudMetaData> targetList;
    CloudDataUtils utils;
    jsonReader.ConvertToCloudMetaDataVector(targetList);
    EXPECT_GT(targetList.size(), 0);
    for (auto newData : newDatas) {
        for (auto target : targetList) {
            if (newData.cloudId == target.cloudId) {
                utils.CloudMetaDataEquals(newData, target, checkCloudDataList);
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, newDatas.size());
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_favorite_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE2 云端设置收藏属性favorite
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_cloud_favorite_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    std::vector<PhotosPo> photosList;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIdsFacorite;
    std::vector<std::string> cloudIdsNotFacorite;
    for (auto &record : records) {
        MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
        if (photosData.GetFavorite().value_or(0)) {
            cloudIdsFacorite.emplace_back(record.GetRecordId());  //构造的云端数据为收藏的Id集合
        } else {
            cloudIdsNotFacorite.emplace_back(record.GetRecordId());  //构造的云端数据为未收藏的Id集合
        }
    }
    //云端收藏，本地未收藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsFacorite);
    EXPECT_EQ(photosList.size(), cloudIdsFacorite.size());
    for (auto cloudIdFacorite : cloudIdsFacorite) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdFacorite, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 0);  //构造的本地数据为未收藏
    }
    //云端未收藏，本地已收藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsNotFacorite);
    EXPECT_EQ(photosList.size(), cloudIdsNotFacorite.size());
    for (auto cloudIdNotFacorite : cloudIdsNotFacorite) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdNotFacorite, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 1);  //构造的本地数据为收藏
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], records.size());

    //云端收藏，本地改收藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsFacorite);
    EXPECT_EQ(photosList.size(), cloudIdsFacorite.size());
    for (auto cloudIdFacorite : cloudIdsFacorite) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdFacorite, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 1);  //预期本地数据变成收藏
    }
    //云端未收藏，本地改未收藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsNotFacorite);
    EXPECT_EQ(photosList.size(), cloudIdsNotFacorite.size());
    for (auto cloudIdNotFacorite : cloudIdsNotFacorite) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdNotFacorite, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 0);  //预期本地数据变成收藏
    }
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_hidden_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE3 云端设置隐藏属性hidden
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_cloud_hidden_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    std::vector<PhotosPo> photosList;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIdsHide;
    std::vector<std::string> cloudIdsNotHide;
    for (auto &record : records) {
        MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
        if (photosData.GetHidden().value_or(0)) {
            cloudIdsHide.emplace_back(record.GetRecordId());  //构造的云端数据为隐藏的Id集合
        } else {
            cloudIdsNotHide.emplace_back(record.GetRecordId());  //构造的云端数据为未隐藏的Id集合
        }
    }
    //云端隐藏，本地未隐藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsHide);
    EXPECT_EQ(photosList.size(), cloudIdsHide.size());
    for (auto cloudIdHide : cloudIdsHide) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdHide, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.hidden.value_or(-1), 0);  //构造的本地数据为未隐藏
    }
    //云端未隐藏，本地已隐藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsNotHide);
    EXPECT_EQ(photosList.size(), cloudIdsNotHide.size());
    for (auto cloudIdNotHide : cloudIdsNotHide) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdNotHide, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.hidden.value_or(-1), 1);  //构造的本地数据为隐藏
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], records.size());

    //云端隐藏，本地改隐藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsHide);
    EXPECT_EQ(photosList.size(), cloudIdsHide.size());
    for (auto cloudIdHide : cloudIdsHide) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdHide, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.hidden.value_or(-1), 1);  //预期本地数据变成隐藏
    }
    //云端未隐藏，本地改未隐藏检验
    photosList = photosDao.QueryPhotosByCloudIds(cloudIdsNotHide);
    EXPECT_EQ(photosList.size(), cloudIdsNotHide.size());
    for (auto cloudIdNotHide : cloudIdsNotHide) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudIdNotHide, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.hidden.value_or(-1), 0);  //预期本地数据变成隐藏
    }
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_ReName_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE5 重命名
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_rename_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.displayName.value_or(""), "IMG_20250213_330461.jpg");
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], cloudIds.size());

    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.displayName.value_or(""), "IMG_20250213_123456.jpg");
    }
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_DateTrashed_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE3 回收站
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_date_trashed_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.dateTrashed.value_or(-1), 0);
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], cloudIds.size());

    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_GT(photo.dateTrashed.value_or(-1), 0);
    }
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_file_modify_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE6 CASE7 position为cloud的文件修改，position为both的文件修改且LocalWriteOpen=false
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_file_modify_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }

    std::map<std::string, int64_t> cloudIdDateModifyMap;
    std::map<std::string, string> cloudIdOriginalCloudIdMap;

    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_NE(photo.dateModified.value_or(1800000000000), 1800000000000);  //云端构造 1800000000000
        EXPECT_NE(photo.dateAdded.value_or(1000000000000), 1000000000000);     //云端构造1000000000000
        EXPECT_EQ(photo.dirty.value_or(0), 0);  //dirty为0、5、6才会更新数据库,本地数据库构造dirty为0
        cloudIdDateModifyMap[cloudId] = photo.dateModified.value_or(1800000000000);
        cloudIdOriginalCloudIdMap[cloudId] = photo.originalAssetCloudId.value_or("");
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT], records.size());
    EXPECT_EQ(failedRecords.size(), 0);
    EXPECT_EQ(FdirtyDatas.size(), records.size());

    //数据库更新检查
    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.dateModified.value_or(0), 1800000000000);  //云端构造 1800000000000
        EXPECT_EQ(photo.dateAdded.value_or(0), 1000000000000);     //云端构造1000000000000
        EXPECT_EQ(photo.dirty.value_or(0), 0);  // 云端下行修改 date_modified 更新后，dirty 应该为0
    }

    std::vector<std::string> checkCloudDataList = {
        "cloudId", "size",      "path",        "fileName",
        "type",    "thumbnail", "lcdThumbnail"};  // 澄清fdirty的dateModefied以本地的为主，未被云端覆盖，originalCloudId onfetchRecord不会更新，取本地数据校验
    int32_t checkCount = 0;
    std::vector<CloudMetaData> targetList;
    CloudDataUtils utils;
    jsonReader.ConvertToCloudMetaDataVector(targetList);
    EXPECT_GT(targetList.size(), 0);
    for (auto FdirtyData : FdirtyDatas) {
        for (auto target : targetList) {
            if (FdirtyData.cloudId == target.cloudId) {
                utils.CloudMetaDataEquals(FdirtyData, target, checkCloudDataList);
                EXPECT_EQ(FdirtyData.modifiedTime, cloudIdDateModifyMap[FdirtyData.cloudId]);
                EXPECT_EQ(FdirtyData.originalCloudId, cloudIdOriginalCloudIdMap[FdirtyData.cloudId]);
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, FdirtyDatas.size());
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_No_Change_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE8 本地修改，云端修改，LocalWriteOpen=false //数据不刷新，以本地为主
    int32_t ret;
    std::vector<PhotosPo> photosList;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_no_change_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    EXPECT_GT(cloudIds.size(), 0);
    //默认本地数据已修改，且与云端数据不一致
    int32_t checkCount = 0;
    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        for (auto &record : records) {
            if (cloudId == record.GetRecordId()) {
                MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
                EXPECT_NE(photo.dateModified.value_or(0), photosData.GetEditedTimeMs().value_or(0));
                EXPECT_NE(std::to_string(photo.dateAdded.value_or(0)), photosData.GetFirstUpdateTime().value_or("0"));
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, cloudIds.size());

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT], 0);
    EXPECT_EQ(FdirtyDatas.size(), 0);

    //本地文件内容不跟随云端修改
    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.dateModified.value_or(0), 1744444444444);  //数据库构造为1744444444444
        EXPECT_EQ(photo.dateAdded.value_or(0), 1733333333333);     //数据库构造为1733333333333
    }
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_Delete_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE9 删除、CASE11 删除本地position为cloud
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_delete_data_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
    }
    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], cloudIds.size());
    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), 0);  //被删除预期是找不到
}

static void CheckDeletePositionCloudRecordSuccess(std::vector<std::string> &testPaths)
{
    int32_t errorCode = 0;
    auto rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    EXPECT_TRUE(rdbStore != nullptr);
    std::vector<std::string> columns = {PhotoColumn::PHOTO_CLOUD_ID, PhotoColumn::PHOTO_POSITION,
                                        PhotoColumn::PHOTO_DIRTY, PhotoColumn::PHOTO_CLOUD_VERSION,
                                        PhotoColumn::MEDIA_FILE_PATH};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_FILE_PATH, testPaths);
    auto resultSet = rdbStore->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(rowCount, testPaths.size());
    bool checkFlag = false;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        EXPECT_EQ(position, static_cast<int32_t>(PhotoPositionType::LOCAL));

        std::string cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        EXPECT_EQ(cloudId.empty(), true);

        int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_NEW));

        int64_t cloudVersion = GetInt64Val(PhotoColumn::PHOTO_CLOUD_VERSION, resultSet);
        EXPECT_EQ(cloudVersion, 0);
        checkFlag = true;
    }
    resultSet->Close();
    EXPECT_TRUE(checkFlag);
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_Delete_dirty_test, TestSize.Level1)
{
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE10 删除本地position为both,dirty
    int32_t ret;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_delete_dirty_data_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    std::vector<std::string> testPaths;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.position.value_or(-1), 3);
        EXPECT_EQ(photo.dirty.value_or(-1), 2);
        EXPECT_EQ(photo.cloudVersion.value_or(-1), 1);    //构造为1
        EXPECT_NE(photo.data.value_or(""), "");           //path不为空
        testPaths.emplace_back(photo.data.value_or(""));  //cloudId会被清除，后续以路径作为索引
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], cloudIds.size());

    //cloudId被清除，以路径作为索引，预期position = POSITION_LOCAL，cloudId = empty，dirty = TYPE_NEW，cloudVersion = 0
    CheckDeletePositionCloudRecordSuccess(testPaths);
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnFetchRecords_all_data_test, TestSize.Level1)
{
    //test insert data
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    //CASE13 新增数据,修改数据,删除数据，数据合一
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_all_data_test.json");
    std::vector<MDKRecord> records;
    jsonReader.ConvertToMDKRecordVector(records);

    int32_t ret;
    std::vector<PhotosPo> photosList;
    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> newCloudIds = {"473b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff301"};
    std::vector<std::string> deleteCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c2"};
    std::vector<std::string> dirtyCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97cc"};
    std::vector<std::string> metaDataCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c5"};
    std::vector<std::string> modifiedCloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97d3"};
    std::vector<std::string> mergeCloudIds;
    // std::vector<std::string> mergeCloudIds = { "473b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff311" };

    //delete位置为local执行前查询
    photosList = photosDao.QueryPhotosByCloudIds(deleteCloudIds);
    EXPECT_EQ(photosList.size(), deleteCloudIds.size());
    for (auto deCloudId : deleteCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, deCloudId, photo);
        EXPECT_EQ(ret, 0);
    }
    //delete位置为cloud执行前查询
    std::vector<std::string> testPaths;
    photosList = photosDao.QueryPhotosByCloudIds(dirtyCloudIds);
    EXPECT_EQ(photosList.size(), dirtyCloudIds.size());
    for (auto diCloudId : dirtyCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, diCloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.position.value_or(-1), 3);
        EXPECT_EQ(photo.dirty.value_or(-1), 2);
        EXPECT_EQ(photo.cloudVersion.value_or(-1), 1);  //构造为1
        EXPECT_NE(photo.data.value_or(""), "");
        testPaths.emplace_back(photo.data.value_or(""));  //cloudId会被清除，后续以路径作为索引
    }
    //修改元数据(收藏) 执行前查询
    photosList = photosDao.QueryPhotosByCloudIds(metaDataCloudIds);
    EXPECT_EQ(photosList.size(), metaDataCloudIds.size());
    for (auto meCloudId : metaDataCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, meCloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 0);  //构造的本地数据为未收藏
    }
    std::map<std::string, int64_t> cloudIdDateModifyMap;
    std::map<std::string, string> cloudIdOriginalCloudIdMap;
    //修改文件内容 执行前查询
    photosList = photosDao.QueryPhotosByCloudIds(modifiedCloudIds);
    EXPECT_EQ(photosList.size(), modifiedCloudIds.size());
    for (auto moCloudId : modifiedCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, moCloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_NE(photo.dateModified.value_or(1800000000000), 1800000000000);  //云端构造 1800000000000
        EXPECT_NE(photo.dateAdded.value_or(1000000000000), 1000000000000);     //云端构造1000000000000
        EXPECT_EQ(photo.dirty.value_or(0), 0);  //dirty为0、5、6才会更新数据库,本地数据库构造dirty为0
        cloudIdDateModifyMap[moCloudId] = photo.dateModified.value_or(1800000000000);
        cloudIdOriginalCloudIdMap[moCloudId] = photo.originalAssetCloudId.value_or("");
    }

    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], newCloudIds.size() + mergeCloudIds.size());
    EXPECT_EQ(stats[StatsIndex::MERGE_RECORDS_COUNT], mergeCloudIds.size());
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], metaDataCloudIds.size());
    EXPECT_EQ(stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT], modifiedCloudIds.size());
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], deleteCloudIds.size() + dirtyCloudIds.size());

    EXPECT_EQ(newDatas.size(), newCloudIds.size() + mergeCloudIds.size());
    EXPECT_EQ(FdirtyDatas.size(), modifiedCloudIds.size());

    photosList = photosDao.QueryPhotosByCloudIds(deleteCloudIds);
    EXPECT_EQ(photosList.size(), 0);  //被删除预期是找不到

    //dirtyCloudIds cloudId被清除，以路径作为索引，预期position = POSITION_LOCAL，cloud = empty，dirty = TYPE_NEW，cloudVersion = 0
    CheckDeletePositionCloudRecordSuccess(testPaths);

    //云端收藏，本地改收藏检验
    photosList = photosDao.QueryPhotosByCloudIds(metaDataCloudIds);
    EXPECT_EQ(photosList.size(), metaDataCloudIds.size());
    for (auto meCloudId : metaDataCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, meCloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.isFavorite.value_or(-1), 1);  //预期本地数据变成收藏
    }

    photosList = photosDao.QueryPhotosByCloudIds(modifiedCloudIds);
    EXPECT_EQ(photosList.size(), modifiedCloudIds.size());
    for (auto moCloudId : modifiedCloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, moCloudId, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.dateModified.value_or(0), 1800000000000);  //云端构造1800000000000
        EXPECT_EQ(photo.dateAdded.value_or(0), 1000000000000);     //云端构造1000000000000
    }

    int32_t checkCount = 0;
    std::vector<CloudMetaData> targetList;
    CloudDataUtils utils;
    jsonReader.ConvertToCloudMetaDataVector(targetList);
    EXPECT_GT(targetList.size(), 0);

    // 澄清fdirty的dateModefied以本地的为主，未被云端覆盖，originalCloudId onfetchRecord不会更新，取本地数据校验
    std::vector<std::string> checkfCloudDataList = {
        "cloudId", "size", "path", "fileName",
        "type", "thumbnail", "lcdThumbnail"};
    for (auto FdirtyData : FdirtyDatas) {
        for (auto target : targetList) {
            if (FdirtyData.cloudId == target.cloudId) {
                utils.CloudMetaDataEquals(FdirtyData, target, checkfCloudDataList);
                EXPECT_EQ(FdirtyData.modifiedTime, cloudIdDateModifyMap[FdirtyData.cloudId]);
                EXPECT_EQ(FdirtyData.originalCloudId, cloudIdOriginalCloudIdMap[FdirtyData.cloudId]);
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, FdirtyDatas.size());

    checkCount = 0;
    std::vector<std::string> checknCloudDataList = {
        "cloudId", "size",         "path",      "fileName",
        "type",    "modifiedTime", "thumbnail", "lcdThumbnail"};  //new data 无 "originalCloudId"
    for (auto newData : newDatas) {
        for (auto target : targetList) {
            if (newData.cloudId == target.cloudId) {
                utils.CloudMetaDataEquals(newData, target, checknCloudDataList);
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, newDatas.size());
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnDentryFileInsert, TestSize.Level1)
{
    //test insert data
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<std::string> failedRecords;

    std::vector<MDKRecord> records;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_new_data_test.json");
    jsonReader.ConvertToMDKRecordVector(records);

    TestUtils::AlbumDao albumDao;
    int32_t albumCount = 0;
    std::vector<std::string> albumCloudIds = {"373b364a41e59ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c4"};
    std::vector<PhotoAlbumPo> albumList = albumDao.QueryByCloudIds(albumCloudIds);
    EXPECT_EQ(albumList.size(), 1);
    for (auto album : albumList) {
        albumCount = albumDao.GetAlbumCountByAlbumCloudId(album.cloudId.value_or(""));
        EXPECT_GT(albumCount, 0) << "album cloudId:" << album.cloudId.value_or("");
    }

    int32_t ret = dataHandler->OnDentryFileInsert(records, failedRecords);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failedRecords.size(), 0);

    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<std::string> cloudIds;
    for (auto &record : records) {
        cloudIds.emplace_back(record.GetRecordId());
    }
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());

    int32_t checkCount = 0;
    std::vector<std::string> checkPhotoPoRecordList = {"cloudId",
                                                       "data",
                                                       "size",
                                                       "FileName",
                                                       "title",
                                                       "mediaType",
                                                       "mimeType",
                                                       "deviceName",
                                                       "dateAdded",
                                                       "dateModified",
                                                       "dateTaken",
                                                       "duration",
                                                       "isFavorite",
                                                       "dateTrashed",
                                                       "hidden",
                                                       "relativePath",
                                                       "virtualPath",
                                                       "metaDateModified",
                                                       "orientation",
                                                       "latitude_longitude",
                                                       "height",
                                                       "width",
                                                       "subtype",
                                                       "burstCoverLevel",
                                                       "burstKey",
                                                       "dateYear",
                                                       "dateMonth",
                                                       "dateDay",
                                                       "shootingMode",
                                                       "shootingModeTag",
                                                       "dynamicRangeType",
                                                       "frontCamera",
                                                       "detailTime",
                                                       "editTime",
                                                       "originalSubtype",
                                                       "userComment",
                                                       "dateDay",
                                                       "coverPosition",
                                                       "movingPhotoEffectMode",
                                                       "originalAssetCloudId",
                                                       "sourcePath",
                                                       "supportedWatermarkType",
                                                       "strongAssociation"};
    CloudDataUtils utils;
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0);
        for (auto &record : records) {
            if (cloudId == record.GetRecordId()) {
                utils.CloudPhotoPoAndMDKRecordEquals(photo, record, checkPhotoPoRecordList);
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, records.size());
}

HWTEST_F(CloudMediaPhotoHandlerOnFetchRecordsTest, OnDentryFileInsert_merge, TestSize.Level1)
{
    //test insert data
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<std::string> failedRecords;

    std::vector<MDKRecord> records;
    JsonFileReader jsonReader("/data/test/cloudsync/photo_on_fetch_records_merge_data_test.json");
    jsonReader.ConvertToMDKRecordVector(records);

    //CASE12 数据合一
    std::vector<std::string> cloudIds;
    std::vector<std::string> fileNames;
    for (auto &record : records) {
        MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
        fileNames.emplace_back(photosData.GetFileName().value_or(""));
        cloudIds.emplace_back(record.GetRecordId());
    }
    EXPECT_GT(records.size(), 0);
    EXPECT_EQ(fileNames.size(), records.size());
    EXPECT_EQ(cloudIds.size(), records.size());

    PhotosPo photo;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList;
    int32_t ret;

    //预期本地能找到fileName但是没有cloudId
    photosList = photosDao.QueryPhotosByDisplayNames(fileNames);
    EXPECT_EQ(photosList.size(), fileNames.size());
    for (auto fileName : fileNames) {
        ret = photosDao.GetPhotoByDisplayName(photosList, fileName, photo);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(photo.cloudId.value_or(""), "");
    }

    ret = dataHandler->OnDentryFileInsert(records, failedRecords);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failedRecords.size(), 0);

    //预期数据合一，字段补齐
    int32_t checkCount = 0;
    std::vector<std::string> checkPhotoPoRecordList = {"cloudId",
                                                       "data",
                                                       "size",
                                                       "FileName",
                                                       "title",
                                                       "mediaType",
                                                       "mimeType",
                                                       "deviceName",
                                                       "dateAdded",
                                                       "dateModified",
                                                       "dateTaken",
                                                       "duration",
                                                       "isFavorite",
                                                       "dateTrashed",
                                                       "hidden",
                                                       "relativePath",
                                                       "virtualPath",
                                                       "metaDateModified",
                                                       "orientation",
                                                       "latitude_longitude",
                                                       "height",
                                                       "width",
                                                       "subtype",
                                                       "burstCoverLevel",
                                                       "burstKey",
                                                       "dateYear",
                                                       "dateMonth",
                                                       "dateDay",
                                                       "shootingMode",
                                                       "shootingModeTag",
                                                       "dynamicRangeType",
                                                       "frontCamera",
                                                       "detailTime",
                                                       "editTime",
                                                       "originalSubtype",
                                                       "userComment",
                                                       "dateDay",
                                                       "coverPosition",
                                                       "movingPhotoEffectMode",
                                                       "originalAssetCloudId",
                                                       "sourcePath",
                                                       "supportedWatermarkType",
                                                       "strongAssociation"};
    CloudDataUtils utils;
    photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photosList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
        EXPECT_EQ(ret, 0) << "GetPhotoByCloudId not find cloudId: " << cloudId;
        for (auto &record : records) {
            if (cloudId == record.GetRecordId()) {
                utils.CloudPhotoPoAndMDKRecordEquals(photo, record, checkPhotoPoRecordList);
                EXPECT_EQ(photo.position.value_or(-1), 3);  //预期position为Both
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, records.size());
}
}  // namespace OHOS::Media::CloudSync