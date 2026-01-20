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

#include "cloud_media_album_handler_onfetchrecords_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <fstream>

#include "album_dao.h"
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
#include "cloud_media_sync_const.h"

using namespace testing::ext;
using namespace testing::internal;
namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaAlbumHandlerOnFetchRecordsTest::dbDataMock_;
int32_t CloudMediaAlbumHandlerOnFetchRecordsTest::otherAlbumDirty_;
int32_t CloudMediaAlbumHandlerOnFetchRecordsTest::hiddenAlbumDirty_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaAlbumHandlerOnFetchRecordsTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerOnFetchRecordsTest SetUpTestCase";
    // mock native Token
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).BackupDatabase();
    EXPECT_EQ(ret, E_OK) << "CloudMediaAlbumHandlerOnFetchRecordsTest BackupDatabase failed";
    ret = dbDataMock_.MockData(CloudMediaAlbumHandlerOnFetchRecordsTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "CloudMediaAlbumHandlerOnFetchRecordsTest SetUpTestCase ret: " << ret;

    // test insert data
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<CloudMetaData> newDatas;
    std::vector<CloudMetaData> FdirtyDatas;
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

    AlbumDao dao;
    otherAlbumDirty_ = dao.GetAlbumDirtyByName("其他");
    hiddenAlbumDirty_ = dao.GetAlbumDirtyByName(".hiddenAlbum");

    // 先更新隐藏、其他两个相册为TYPE_SYNCED
    std::vector<std::string> albumNames = {".hiddenAlbum", "其它"};
    dao.UpdateDirtySyncedByAlbumNames(albumNames);
    std::vector<MDKRecord> records;
    JsonFileReader jsonReader("/data/test/cloudsync/albumhandler/onfetchrecords_album.json");
    jsonReader.ConvertToMDKRecordVector(records);
    int32_t jsonRecordCount = 21;

    // 同时存在于onfetchrecords_album.csv和onfetchrecords_album.json的数据(用于修改相关的测试用例)
    int32_t updateAlbumCount = 3;  // 合并同名相册、修改相册任一字段
    int32_t deleteAlbumCount = 4;  // 隐藏、其他、空相册、非空相册
    EXPECT_EQ(records.size(), jsonRecordCount);
    ret = dataHandler->OnFetchRecords(records, newDatas, FdirtyDatas, failedRecords, stats);
    EXPECT_EQ(ret, 0) << "AlbumHandler OnFetchRecords ret error:" << ret;
    EXPECT_EQ(failedRecords.size(), 0) << "AlbumHandler OnFetchRecords NewData error";
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], records.size() - updateAlbumCount - deleteAlbumCount)
        << "AlbumHandler OnFetchRecords stat[0] error:" << stats[StatsIndex::NEW_RECORDS_COUNT];
    EXPECT_EQ(stats[StatsIndex::MERGE_RECORDS_COUNT], 0)
        << "AlbumHandler OnFetchRecords stat[1] error:" << stats[StatsIndex::MERGE_RECORDS_COUNT];
    EXPECT_EQ(stats[StatsIndex::META_MODIFY_RECORDS_COUNT], updateAlbumCount)
        << "AlbumHandler OnFetchRecords stat[2] error:" << stats[StatsIndex::META_MODIFY_RECORDS_COUNT];
    EXPECT_EQ(stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT], 0)
        << "AlbumHandler OnFetchRecords stat[3] error:" << stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT];
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], deleteAlbumCount)
        << "AlbumHandler OnFetchRecords stat[4] error:" << stats[StatsIndex::DELETE_RECORDS_COUNT];
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest SetUpTestCase end";
}

void CloudMediaAlbumHandlerOnFetchRecordsTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest TearDownTestCase";
    AlbumDao dao;
    dao.UpdateAlbumDirtyByName("其他", otherAlbumDirty_);
    dao.UpdateAlbumDirtyByName(".hiddenAlbum", hiddenAlbumDirty_);
    int32_t ret = dbDataMock_.RestoreDatabase();
    EXPECT_EQ(ret, E_OK) << "CloudMediaAlbumHandlerOnFetchRecordsTest RestoreDatabase failed";

    // revocery shell token
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
void CloudMediaAlbumHandlerOnFetchRecordsTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest SetUp";
}

void CloudMediaAlbumHandlerOnFetchRecordsTest::TearDown(void)
{
    GTEST_LOG_(INFO) << "CloudMediaPhotoHandlerTest TearDown";
}

/**
 * 云上新增5个用户相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Add_User_Album_Case1, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"User000", "User001", "User002", "User003", "User004"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"default-album-9d51ed54ff9000", {"User000", "/Pictures/Users/User000", "", "1"}},
        {"default-album-9d51ed54ff9001", {"User001", "/Pictures/Users/User001", "", "1"}},
        {"default-album-9d51ed54ff9002", {"User002", "/Pictures/Users/User002", "", "1"}},
        {"default-album-9d51ed54ff9003", {"User003", "/Pictures/Users/User003", "", "1"}},
        {"default-album-9d51ed54ff9004", {"User004", "/Pictures/Users/User004", "", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Check_Added_User_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.cloudId.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(-1) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_GT(album.dateModified.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_GT(album.dateAdded.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.bundleName.value_or("") == albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", bundleName:" << album.bundleName.value_or("")
            << ", bundleName2:" << albumInfo[2];
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
    }
}

/**
 * 云上新增2个来源相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Added_Source_Album_Case2, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Source000", "Source001"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"default-album-9d51ed54ff9400",
         {"Source000", "/storage/media/Photo/Source000", "", "1", "1739459613600", "1739459613700"}},
        {"default-album-9d51ed54ff9401",
         {"Source001", "/storage/media/Photo/Source001", "", "1", "1739459613601", "1739459613601"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Check_Added_Source_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.cloudId.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 2048)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 2049)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[5])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.bundleName.value_or("") == albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", bundleName:" << album.bundleName.value_or("")
            << ", bundleName2:" << albumInfo[2];
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
    }
}

/**
 * 云上新增5个来源相册，白名单相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Added_Allow_List_Album_Case3, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"天天快报", "QQ", "屏幕录制", "QQ邮箱", "腾讯新闻"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"/Pictures/TencentReading", {"天天快报", "default-album-9d51ed54ff9100", "", "1"}},
        {"/tencent/QQ_Images", {"QQ", "default-album-101", "com.tencent.mqq", "0"}},
        {"/Pictures/Screenrecords", {"屏幕录制", "default-album-2", "com.huawei.hmos.screenrecorder", "1"}},
        {"/QQMail", {"QQ邮箱", "default-album-9d51ed54ff9103", "com.tencent.qqmail.hmos", "0"}},
        {"/Pictures/TencentNews", {"腾讯新闻", "default-album-9d51ed54ff9104", "com.tencent.hm.news", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Check_Added_Source_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.lpath.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("");
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 2048)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 2049)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_GT(album.dateModified.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_GT(album.dateAdded.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.bundleName.value_or("") == albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", bundleName:" << album.bundleName.value_or("")
            << ", bundleName2:" << albumInfo[2];
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", cloudId1:" << album.cloudId.value_or("")
            << ", cloudId2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
    }
}

/**
 * 云上新增2个逻辑相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Add_Logic_Album_Case4, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Logic200", "Logic201"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"9d51ed54ff9200", {"Logic200", "1739459613600", "1739459613700", "1", "/Pictures/Logic/Logic200"}},
        {"9d51ed54ff9201", {"Logic201", "1739459613601", "1739459613601", "1", "/Pictures/Logic/Logic201"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Check_Added_Logic_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.cloudId.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_TRUE(album.albumType.value_or(0) == 2048)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 2049)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
    }
}

/**
 * 云上新增10个逻辑相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Cloud_Modify_Same_Name_Album_Info_Case5,
         TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Album60"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"Album60",
         {"Album60", "/Pictures/Users/Album60", "1739459628868", "1739459628868", "1",
          "defalut-album-8d51ed54ff80601"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Cloud_Modify_Same_Name_Album_Info: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
        EXPECT_TRUE(album.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_SYNCED))
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.dirty.value_or(0);
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[5])
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
    }
}

/**
 * 云上新增10个本地同名相册,本地相册dirty=new,lpath和云上相册一致(不区分大小写)(云上有lPath,本地有lPath,本地没有cloudId)
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Modify_Album_Info_Case6, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Album59"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"Album59",
         {"defalut-album-8d51ed54ff8059", "/Pictures/Users/Album59", "1739459628890", "1739459628891", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Modify_Album_Info: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == it->first)
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("");
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("");
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.cloudId.value_or("");
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0);
    }
}

/**
 * 云上删除一个空相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Cloud_Delete_Hide_Album_Info_Case7, TestSize.Level1)
{
    std::vector<std::string> albumNames = {".hiddenAlbum", "其它"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {".hiddenAlbum", {"default-album-4", "/Pictures/hiddenAlbum", "1"}},
        {"其它", {"default-album-5", "/Pictures/其它", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Cloud_Modify_Hide_Album_Info: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == it->first)
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("") << ", name2:" << it->first;
        EXPECT_EQ(album.albumType.value_or(0), 2048)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 2049)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_TRUE(album.dateModified.value_or(0) >= 0)
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_GT(album.dateAdded.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[2];
        EXPECT_TRUE(album.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_NEW))
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.dirty.value_or(0);
        EXPECT_TRUE(album.cloudId.value_or("").empty())
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.cloudId.value_or("");
    }
}

/**
 * 云上删除一个空相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Cloud_Delete_Not_Empty_Album_Case8, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Album61"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"Album61",
         {"defalut-album-8d51ed54ff8061", "/Pictures/Users/Album61", "1739459628868", "1739459628868", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Modify_Album_Info: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == it->first)
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("") << ", name2:" << it->first;
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
        EXPECT_TRUE(album.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_SDIRTY))
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.dirty.value_or(0);
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.cloudId.value_or("");
    }
}

/**
 * 云上删除一个空相册
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Cloud_Delete_Empty_Album_Case9, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Album62"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"Album62",
         {"defalut-album-8d51ed54ff8062", "/Pictures/Users/Album62", "1739459628868", "1739459628868", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Cloud_Delete_Empty_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == it->first)
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
        EXPECT_TRUE(album.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_SDIRTY))
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.dirty.value_or(0);
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.cloudId.value_or("");
    }
}

/**
 * 修改云上相册数据库某一字段（云上没有lPath，本地有cloudId）
 */
HWTEST_F(CloudMediaAlbumHandlerOnFetchRecordsTest, OnFetchRecords_Cloud_Modify_Album_Case10, TestSize.Level1)
{
    std::vector<std::string> albumNames = {"Album63"};
    std::map<std::string, std::vector<std::string>> sourceAlbumInfo = {
        {"Album63",
         {"defalut-album-8d51ed54ff8063", "/Pictures/Users/Album63", "1739459628868", "1739459628868", "1"}}};
    AlbumDao dao;
    std::vector<PhotoAlbumPo> albumList = dao.QueryByAlbumNames(albumNames);
    EXPECT_EQ(albumList.size(), albumNames.size());
    for (auto &album : albumList) {
        GTEST_LOG_(INFO) << "OnFetchRecords_Cloud_Delete_Empty_Album: " << album.ToString();
        auto it = sourceAlbumInfo.find(album.albumName.value_or(""));
        EXPECT_TRUE(it != sourceAlbumInfo.end())
            << "id:" << album.albumId.value_or(0) << ", cloudId:" << album.cloudId.value_or("");
        if (it == sourceAlbumInfo.end()) {
            continue;
        }
        std::vector<std::string> albumInfo = it->second;
        EXPECT_TRUE(album.albumName.value_or("") == it->first)
            << "id:" << album.albumId.value_or(0) << "name:" << album.albumName.value_or("")
            << ", name2:" << albumInfo[0];
        EXPECT_EQ(album.albumType.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumType:" << album.albumType.value_or(0);
        EXPECT_EQ(album.albumSubtype.value_or(0), 1)
            << "id:" << album.albumId.value_or(0) << ", albumSubtype:" << album.albumSubtype.value_or(0);
        EXPECT_EQ(std::to_string(album.dateModified.value_or(0)), albumInfo[3])
            << "id:" << album.albumId.value_or(0) << ", dateModified:" << album.dateModified.value_or(0);
        EXPECT_EQ(std::to_string(album.dateAdded.value_or(0)), albumInfo[2])
            << "id:" << album.albumId.value_or(0) << ", dateAdded:" << album.dateAdded.value_or(0);
        EXPECT_GT(album.albumOrder.value_or(0), 0)
            << "id:" << album.albumId.value_or(0) << ", albumOrder:" << album.albumOrder.value_or(0);
        EXPECT_TRUE(album.lpath.value_or("") == albumInfo[1])
            << "id:" << album.albumId.value_or(0) << ", lpath:" << album.lpath.value_or("")
            << ", lpath2:" << albumInfo[1];
        EXPECT_TRUE(std::to_string(album.priority.value_or(0)) == albumInfo[4])
            << "id:" << album.albumId.value_or(0) << ", priority:" << album.priority.value_or(0)
            << ", priority2:" << albumInfo[4];
        EXPECT_TRUE(album.dirty.value_or(0) == static_cast<int32_t>(DirtyType::TYPE_SYNCED))
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.dirty.value_or(0);
        EXPECT_TRUE(album.cloudId.value_or("") == albumInfo[0])
            << "id:" << album.albumId.value_or(0) << ", dirty:" << album.cloudId.value_or("");
    }
}
}  // namespace OHOS::Media::CloudSync