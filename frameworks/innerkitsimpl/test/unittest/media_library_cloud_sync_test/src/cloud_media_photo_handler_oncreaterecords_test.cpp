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
#include "cloud_media_photo_handler_oncreaterecords_test.h"

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
DatabaseDataMock CloudMediaPhotoHandlerOnCreateRecordsTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoHandlerOnCreateRecordsTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoHandlerOnCreateRecordsTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest SetUpTestCase ret: " << ret;
}

void CloudMediaPhotoHandlerOnCreateRecordsTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaPhotoHandlerOnCreateRecordsTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest SetUp";
}

void CloudMediaPhotoHandlerOnCreateRecordsTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerOnCreateRecordsTest TearDown";
}

/**
 * 预设条件：上传数据处理结果均为处理成功，本地数据在上传阶段没有再变化
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version字段
 * 2. dirty字段更新为synced
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case001, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_normal_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/9/IMG_1739459141_20011.jpg",
                                      "/storage/cloud/files/Photo/10/IMG_1739459142_20012.jpg"};
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case001 Photo:" << photo.ToString();
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) > 0);
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
}

/**
 * 预设条件：上传数据处理结果均为处理成功，本地数据在上传阶段发生元数据或文件内容更改
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version字段
 * 2. dirty字段更新为mdirty或fdirty
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case002, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_upload_metadata_changed_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/11/IMG_1739459144_20013.jpg",
                                      "/storage/cloud/files/Photo/12/IMG_1739459145_20014.jpg"};
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case002 Photo:" << photo.ToString();
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        if (photo.data.value_or("") == "/storage/cloud/files/Photo/11/IMG_1739459144_20013.jpg") {
            EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20013);
            EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
        } else {
            EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20014);
            EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
        }
    }
}

/**
 * 上传10个数据，部分失败，五个上传数据失败原因为云空间不足
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version、dirty字段
 * 2. 失败的数据刷新cloudid字段（断点续传），错误码为期望值
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case003, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_cloud_full_space_error_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_CLOUD_STORAGE_FULL);
    std::vector<std::string> successDatas = {"/storage/cloud/files/Photo/13/IMG_1739459146_20015.jpg"};
    std::vector<std::string> failedDatas = {"/storage/cloud/files/Photo/13/IMG_1739459146_20016.jpg",
                                            "/storage/cloud/files/Photo/15/IMG_1739459146_20017.jpg"};
    EXPECT_EQ(failSize, failedDatas.size());
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> successPhotos = dao.QueryPhotosByFilePaths(successDatas);
    std::vector<ORM::PhotosPo> failedPhotos = dao.QueryPhotosByFilePaths(failedDatas);

    EXPECT_EQ(successPhotos.size(), successDatas.size());
    EXPECT_EQ(failedPhotos.size(), failedDatas.size());
    for (auto &photo : successPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case003 Success Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20015);
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    for (auto &photo : failedPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case003 Failed Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_NEW));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);
    }
}

/**
 * 上传10个数据，部分失败，五个上传数据失败原因为为网络问题
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version、dirty字段
 * 2. 失败的数据刷新cloudid字段（断点续传），错误码为期望值
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case004, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_cloud_network_error_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    std::vector<std::string> successDatas = {"/storage/cloud/files/Photo/16/IMG_1739459146_20018.jpg"};
    std::vector<std::string> failedDatas = {"/storage/cloud/files/Photo/1/IMG_1739459147_20019.jpg"};
    EXPECT_EQ(failSize, failedDatas.size());
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> successPhotos = dao.QueryPhotosByFilePaths(successDatas);
    std::vector<ORM::PhotosPo> failedPhotos = dao.QueryPhotosByFilePaths(failedDatas);

    EXPECT_EQ(successPhotos.size(), successDatas.size());
    EXPECT_EQ(failedPhotos.size(), failedDatas.size());
    for (auto &photo : successPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case004 Success Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20018);
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    for (auto &photo : failedPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case004 Failed Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_NEW));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);
    }
}

/**
 * 上传10个数据，数据失败原因为普通用户上传禁止
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version、dirty字段
 * 2. 失败的数据刷新cloudid字段（断点续传），错误码为期望值
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case005, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_user_upload_unpermited_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_BUSINESS_MODE_CHANGED);
    std::vector<std::string> successDatas = {"/storage/cloud/files/Photo/2/IMG_1739459147_20020.jpg"};
    std::vector<std::string> failedDatas = {"/storage/cloud/files/Photo/2/IMG_1739459148_20021.jpg"};
    EXPECT_EQ(failSize, failedDatas.size());
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> successPhotos = dao.QueryPhotosByFilePaths(successDatas);
    std::vector<ORM::PhotosPo> failedPhotos = dao.QueryPhotosByFilePaths(failedDatas);

    EXPECT_EQ(successPhotos.size(), successDatas.size());
    EXPECT_EQ(failedPhotos.size(), failedDatas.size());
    for (auto &photo : successPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case005 Success Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20020);
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    for (auto &photo : failedPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case005 Failed Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL));
        EXPECT_TRUE(!photo.cloudId.value_or("").empty());
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_NEW));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);
    }
}

/**
 * 上传10个数据，上传过程中关闭开关或退出账号，数据失败为原因同步停止
 * 期望结果:
 * 1. map中带cloudid的数据回填线数据库cloudid字段，错误码为期望值
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case006, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_stop_sync_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_STOP);
    std::vector<std::string> successDatas = {"/storage/cloud/files/Photo/2/IMG_1739459148_20022.jpg"};
    std::vector<std::string> failedDatas = {"/storage/cloud/files/Photo/2/IMG_1739459148_20023.jpg"};
    EXPECT_EQ(failSize, failedDatas.size());
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> successPhotos = dao.QueryPhotosByFilePaths(successDatas);
    std::vector<ORM::PhotosPo> failedPhotos = dao.QueryPhotosByFilePaths(failedDatas);

    std::vector<std::string> cloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20022",
                                         "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20023"};
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "QueryByCloudId:" << photo.ToString();
    }
    EXPECT_EQ(successPhotos.size(), successDatas.size());
    EXPECT_EQ(failedPhotos.size(), failedDatas.size());
    for (auto &photo : successPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case006 Success Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
        EXPECT_TRUE(photo.cloudId.value_or("") == "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20022");
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 20022);
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    for (auto &photo : failedPhotos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case006 Failed Photo:" << photo.ToString();
        EXPECT_TRUE(photo.position.value_or(-1) == static_cast<int32_t>(PhotoPositionType::LOCAL));
        EXPECT_TRUE(photo.cloudId.value_or("") == "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20023");
        EXPECT_TRUE(photo.dirty.value_or(-1) == static_cast<int32_t>(DirtyType::TYPE_NEW));
        EXPECT_TRUE(photo.cloudVersion.value_or(-1) == 0);
    }
}

/**
 * 上传10个数据，部分失败，五个数据失败原因为文件名重复
 * 期望结果:
 * 1. 成功的数据刷新postiion、cloudid、version、dirty字段
 * 2. 失败的数据cloudid字段刷新（断点续传），文件名修改为原名_1
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case007, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_same_filename_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/2/IMG_1739459148_20024.jpg",
                                      "/storage/cloud/files/Photo/13/IMG_1739459145_20025.jpg"};
    EXPECT_EQ(failSize, 1);
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    std::map<std::string, std::vector<std::string>> resultMap = {
        {"/storage/cloud/files/Photo/2/IMG_1739459148_20024.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)), "20024",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)), "IMG_1739459148_20024.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20024"}},
        {"/storage/cloud/files/Photo/13/IMG_1739459145_20025.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459145_20025_1.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20025"}},
    };
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case007 Photo:" << photo.ToString();
        auto it = resultMap.find(photo.data.value_or(""));
        EXPECT_TRUE(it != resultMap.end());
        if (it == resultMap.end()) {
            continue;
        }
        std::vector<std::string> expected = it->second;

        EXPECT_TRUE(std::to_string(photo.position.value_or(-1)) == expected[0]);
        EXPECT_TRUE(std::to_string(photo.cloudVersion.value_or(-1)) == expected[1]);
        EXPECT_TRUE(std::to_string(photo.dirty.value_or(-1)) == expected[2]) << "dirty:" << photo.dirty.value_or(-1);
        EXPECT_TRUE(photo.displayName.value_or("") == expected[3]);
        EXPECT_TRUE(photo.cloudId.value_or("") == expected[4]);
    }
}

/**
 * 上传10个数据，部分失败，失败文件为原图上行失败，本地无原图
 * 期望结果:
 * 1. 成功的数据刷新postiion、cloudid、version、dirty字段
 * 2. 失败的数据从数据库删除
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case008, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_file_not_exist_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/8/IMG_1739459149_20026.jpg",
                                      "/storage/cloud/files/Photo/8/IMG_1739459150_20027.jpg"};
    EXPECT_EQ(failSize, 1);
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    std::map<std::string, std::vector<std::string>> resultMap = {
        {"/storage/cloud/files/Photo/8/IMG_1739459149_20026.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)), "20026",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)), "IMG_1739459149_20026.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20026"}},
    };
    EXPECT_EQ(photos.size(), datas.size() - 1);
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case008 Photo:" << photo.ToString();
        auto it = resultMap.find(photo.data.value_or(""));
        EXPECT_TRUE(it != resultMap.end());
        if (it == resultMap.end()) {
            continue;
        }
        std::vector<std::string> expected = it->second;

        EXPECT_TRUE(std::to_string(photo.position.value_or(-1)) == expected[0]);
        EXPECT_TRUE(std::to_string(photo.cloudVersion.value_or(-1)) == expected[1]);
        EXPECT_TRUE(std::to_string(photo.dirty.value_or(-1)) == expected[2]);
        EXPECT_TRUE(photo.displayName.value_or("") == expected[3]);
        EXPECT_TRUE(photo.cloudId.value_or("") == expected[4]);
    }
}

/**
 * 上传10个数据，部分失败，失败文件为原图上行失败，本地有原图
 * 期望结果:
 * 1. 成功的数据刷新postiion、cloudid、version、dirty字段
 * 2. 失败的数据等下次上行
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case009, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_file_exist_upload_failed_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/8/IMG_1739459151_20028.jpg",
                                      "/storage/cloud/files/Photo/8/IMG_1739459151_20029.jpg"};
    EXPECT_EQ(failSize, 1);
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    std::map<std::string, std::vector<std::string>> resultMap = {
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20028.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)), "20028",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)), "IMG_1739459151_20028.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20028"}},
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20029.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459151_20029.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20029"}},
    };
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case009 Photo:" << photo.ToString();
        auto it = resultMap.find(photo.data.value_or(""));
        EXPECT_TRUE(it != resultMap.end());
        if (it == resultMap.end()) {
            continue;
        }
        std::vector<std::string> expected = it->second;

        EXPECT_TRUE(std::to_string(photo.position.value_or(-1)) == expected[0]);
        EXPECT_TRUE(std::to_string(photo.cloudVersion.value_or(-1)) == expected[1]);
        EXPECT_TRUE(std::to_string(photo.dirty.value_or(-1)) == expected[2]);
        EXPECT_TRUE(photo.displayName.value_or("") == expected[3]);
        EXPECT_TRUE(photo.cloudId.value_or("") == expected[4]);
    }
}

/**
 * 上传10个数据，失败原因为设备未持锁
 * 期望结果:
 * 1. map中带cloudid的数据回填线数据库cloudid字段，错误码为期望值E_STOP
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case010, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_invalid_lock_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_STOP);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/8/IMG_1739459151_20030.jpg",
                                      "/storage/cloud/files/Photo/8/IMG_1739459151_20031.jpg"};
    EXPECT_EQ(failSize, 2);
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    std::map<std::string, std::vector<std::string>> resultMap = {
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20030.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459151_20030.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20030"}},
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20031.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459151_20031.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20031"}},
    };
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case007 Photo:" << photo.ToString();
        auto it = resultMap.find(photo.data.value_or(""));
        EXPECT_TRUE(it != resultMap.end()) << "can not find data in map";
        if (it == resultMap.end()) {
            continue;
        }
        std::vector<std::string> expected = it->second;
        EXPECT_TRUE(std::to_string(photo.position.value_or(-1)) == expected[0])
            << "local position is:" << photo.position.value_or(-1);
        EXPECT_TRUE(std::to_string(photo.cloudVersion.value_or(-1)) == expected[1])
            << "local cloud version is:" << photo.cloudVersion.value_or(-1);
        EXPECT_TRUE(std::to_string(photo.dirty.value_or(-1)) == expected[2])
            << "local dirty is:" << photo.dirty.value_or(-1);
        EXPECT_TRUE(photo.displayName.value_or("") == expected[3])
            << "local displayName is:" << photo.displayName.value_or("");
        EXPECT_TRUE(photo.cloudId.value_or("") == expected[4]) << "local cloudId is:" << photo.cloudId.value_or("");
    }
}

/**
 * 上传10个数据，部分失败，失败原因设置其他错误
 * 期望结果:
 * 1. 成功的数据刷新position、cloudid、version、dirty字段
 * 2. 失败的数据刷新cloudid，本次同步不再遍历，等待下次上行
 * 错误码为期望值E_OK
 */
HWTEST_F(CloudMediaPhotoHandlerOnCreateRecordsTest, OnCreateRecords_case011, TestSize.Level1)
{
    JsonFileReader jsonReader("/data/test/cloudsync/photohandler/oncreaterecords_other_error_test.json");
    std::map<std::string, MDKRecordOperResult> map;
    jsonReader.ConvertToMDKRecordOperResultMap(map);
    std::string tableName = "Photos";
    int32_t cloudType = 0;
    int32_t userId = 100;
    int32_t failSize;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, E_OK);
    std::vector<std::string> datas = {"/storage/cloud/files/Photo/8/IMG_1739459151_20032.jpg",
                                      "/storage/cloud/files/Photo/8/IMG_1739459151_20033.jpg",
                                      "/storage/cloud/files/Photo/8/IMG_1739459151_20034.jpg"};
    EXPECT_EQ(failSize, 2);
    TestUtils::PhotosDao dao;
    std::vector<ORM::PhotosPo> photos = dao.QueryPhotosByFilePaths(datas);
    std::map<std::string, std::vector<std::string>> resultMap = {
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20032.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)), "20032",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_SYNCED)), "IMG_1739459151_20032.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20032"}},
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20033.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459151_20033.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20033"}},
        {"/storage/cloud/files/Photo/8/IMG_1739459151_20034.jpg",
         {std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)), "0",
          std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), "IMG_1739459151_20034.jpg",
          "373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff20034"}},
    };
    EXPECT_EQ(photos.size(), datas.size());
    for (auto &photo : photos) {
        GTEST_LOG_(INFO) << "OnCreateRecords_case0011 Photo:" << photo.ToString();
        auto it = resultMap.find(photo.data.value_or(""));
        EXPECT_TRUE(it != resultMap.end());
        if (it == resultMap.end()) {
            continue;
        }
        std::vector<std::string> expected = it->second;

        EXPECT_TRUE(std::to_string(photo.position.value_or(-1)) == expected[0]);
        EXPECT_TRUE(std::to_string(photo.cloudVersion.value_or(-1)) == expected[1]);
        EXPECT_TRUE(std::to_string(photo.dirty.value_or(-1)) == expected[2]);
        EXPECT_TRUE(photo.displayName.value_or("") == expected[3]);
        EXPECT_TRUE(photo.cloudId.value_or("") == expected[4]);
    }
}
}  // namespace OHOS::Media::CloudSync