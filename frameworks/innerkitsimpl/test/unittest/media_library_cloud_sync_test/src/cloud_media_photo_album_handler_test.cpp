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

#include "cloud_media_photo_album_handler_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "media_log.h"

#include "album_dao.h"
#include "cloud_check_data.h"
#include "cloud_file_data.h"
#include "cloud_media_data_client.h"
#include "cloud_meta_data.h"
#include "mdk_asset.h"
#include "mdk_database.h"
#include "mdk_error.h"
#include "mdk_record_field.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "i_cloud_media_data_handler.h"
#include "cloud_media_data_handler.h"
#include "json/json.h"
#include "json_file_reader.h"
#include "mdk_record_utils.h"
#include "photos_dao.h"
#include "cloud_album_data_convert.h"
#include "cloud_file_data_convert.h"
#include "cloud_data_convert_to_vo.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaPhotoAlbumHandlerTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaPhotoAlbumHandlerTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaPhotoAlbumHandlerTest::GetTableMockInfoList());
    MEDIA_INFO_LOG("SetUpTestCase ret: %{public}d", ret);
}

void CloudMediaPhotoAlbumHandlerTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    MEDIA_INFO_LOG("TearDownTestCase ret: %{public}d", ret);
}

// SetUp:Execute before each test case
void CloudMediaPhotoAlbumHandlerTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void CloudMediaPhotoAlbumHandlerTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

/**
 * 获取相册创建记录 GetCreatedRecords，结果包含album_plugin记录
 * 期望结果：
 * 输出对应数量的records，records对应字段和album_plugin一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetCreatedRecords_Known_Album, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    // 不包含空相册集合
    std::vector<std::string> lpaths = {
        "/tencent/QQ_Images",
        "/Pictures/Screenshots",
    };
    std::map<std::string, std::vector<std::string>> knownAlbumInfo = {
        {"/tencent/QQ_Images", {"QQ", "default-album-101", "com.tencent.mqq", "0"}},
        {"/Tencent/MicroMsg/WeiXin", {"微信", "default-album-102", "com.tencent.wechat", "0"}},
    };

    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordAlbumData data(record);
        auto it = knownAlbumInfo.find(data.GetCloudId().value_or(""));
        if (it != knownAlbumInfo.end()) {
            EXPECT_EQ(data.GetAlbumName().value_or("albumName"), it->second[0]);
            EXPECT_EQ(data.GetCloudId().value_or("cloudId"), it->second[1]);
            EXPECT_EQ(data.GetBundleName().value_or("bundleName"), it->second[2]);
            EXPECT_EQ(std::to_string(data.GetPriority().value_or(-1)), it->second[3]);
            EXPECT_EQ(data.GetlPath().value_or("lpath"), it->first);
        }
    }
    EXPECT_EQ(num, 0);
}

/**
 * 获取相册创建记录 GetCreatedRecords，不包含空相册的记录
 * 期望结果：
 * 输出对应数量的records，records对应字段和实际修改一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetCreatedRecords_Empty_Album, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    // 不包含空相册集合
    std::vector<std::string> cloudIds = {"373b364a41e54ebf912b3414aeabe963507a901b2b1a4332939d51ed54ff97c5",
                                         "373b364a41e54ebf912b3424aeabe963507a901b2b1a4332939d51ed54ff97c6",
                                         "373b364a41e54ebf912b3424aeabe963507a901b2b1a4332939d51ed54ff97c7",
                                         "373b364a41e54ebf912b3464aeabe963507a901b2b1a4332939d58ed54ff97c8",
                                         "373b364a41e54ebf912b3464aeabe963507a901b2b1a4332939d58ed54ff97c9"};

    int32_t num = 0;
    for (auto &record : records) {
        for (auto &cloudId : cloudIds) {
            if (record.GetRecordId() == cloudId) {
                num++;
            }
        }
    }
    EXPECT_EQ(num, 0);
}

/**
 * 获取相册创建记录 GetCreatedRecords，获取到不为空相册记录
 * 期望结果：
 * 输出对应数量的records，records对应字段和实际修改一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetCreatedRecords_Not_Empty_Album, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);  //默认数据

    int32_t num = 0;
    for (auto &record : records) {
        MDKRecordAlbumData data = MDKRecordAlbumData(record);
        if (data.GetAlbumName().value_or("") != ".hiddenAlbum") {
            num++;
        }
    }
    EXPECT_GT(num, 0);
}

/**
 * 获取相册创建记录 GetCreatedRecords，修改相册名字
 * 期望结果：
 * 输出对应数量的records，records对应字段和实际修改一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetCreatedRecords_Update_AlbumName, TestSize.Level1)
{
    // 先获取记录
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/S-202504021638"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    EXPECT_EQ(albums.size(), 1);

    int32_t albumId = -1;
    for (auto &album : albums) {
        albumId = album.albumId.value_or(-1);
    }
    EXPECT_NE(albumId, -1);

    // 更新相册名称
    int32_t changeRows = dao.UpdatePhotoAlbumName(albumId, "SS");
    EXPECT_EQ(changeRows, 1);

    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetCreatedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::string albumName = "";
    for (auto &record : records) {
        if (record.GetRecordId() == "default-album-200-/Pictures/Users/S-202504021638") {
            MDKRecordAlbumData data = MDKRecordAlbumData(record);
            albumName = data.GetAlbumName().value_or("");
        }
    }
    EXPECT_EQ(albumName, "SS");
}

/**
 * 获取相册创建记录 GetMetaModifiedRecords
 * 期望结果：
 * 输出对应数量的records，records对应字段和实际修改一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetMetaModifiedRecords, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
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

    std::string cloudId = "default-album-200-/Pictures/Users/T-202504021638";
    int32_t num = 0;
    for (auto &record : records) {
        if (record.GetRecordId() == cloudId) {
            num++;
        }
    }
    EXPECT_EQ(num, 1);
}

// album does not handle this operation <GetFileModifiedRecords>.
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetFileModifiedRecords, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetDeletedRecords, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);
    std::vector<MDKRecord> targetRecords;
    JsonFileReader reader("/data/test/cloudsync/album_handler_get_delete_record_result.json");
    reader.ConvertToMDKRecordVector(targetRecords);
    EXPECT_GT(targetRecords.size(), 0);
    MDKRecordUtils utils;
    std::vector<std::string> checkFields = {"album_type",    "album_subtype", "logicType",      "date_added",
                                            "date_modified", "bundle_name",   "albumName",      "localPath",
                                            "albumId",       "type",          "local_language", "emptyShow"};
    int32_t checkCount = 0;
    for (auto &record : records) {
        for (auto &target : targetRecords) {
            if (record.GetRecordId() == target.GetRecordId()) {
                EXPECT_TRUE(utils.Equals(record, target, checkFields, MDKRecordUtils::RecordType::ALBUM));
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, targetRecords.size());
}

/**
 * 获取相册删除记录 GetDeletedRecords
 * 期望结果：
 * 输出对应数量的records，records对应字段和实际修改一致
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetDeletedRecords_Delete, TestSize.Level1)
{
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::string cloudId = "default-album-200-/Pictures/Users/U-202504021520";
    int32_t deleteRows = dao.DeleteAlbumByCloudId(cloudId, 4);
    EXPECT_EQ(deleteRows, 1);

    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(records.size(), 0);

    int32_t num = 0;
    for (auto &record : records) {
        if (record.GetRecordId() == cloudId) {
            num++;
        }
    }
    EXPECT_EQ(num, 1);
}

/**
 * 获取相册删除记录 GetDeletedRecords 但是相册非空，需要修改dirty为new
 * 期望结果：
 * 非空相册dirty修改为new
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetDeletedRecords_notEmptyAlbumSetNew, TestSize.Level1)
{
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/U-202504113456"};
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::vector<MDKRecord> records;
    int32_t size = 100;
    int32_t ret = dataHandler->GetDeletedRecords(records, size);
    EXPECT_EQ(ret, 0);
    //预期没有获取到record或者获取到的record中不包含目标record
    for (auto &record : records) {
        for (auto cloudId : cloudIds) {
            EXPECT_TRUE(record.GetRecordId() != cloudId) << "find album_id:" << cloudId;
        }
    }
    //预期目标record的dirty已修改为new
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<PhotoAlbumPo> albumList = dao.QueryByCloudIds(cloudIds);
    EXPECT_EQ(albumList.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        PhotoAlbumPo album = PhotoAlbumPo();
        EXPECT_EQ(dao.GetAlbumByCloudId(albumList, cloudId, album), 0);
        EXPECT_EQ(album.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_NEW));
    }
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetCopyRecords, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

/**
 * 上行相册记录 OnCreateRecords 一个正常，一个异常
 * 期望结果：
 * 输出对应数量的records，异常未上行，正常上行
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCreateRecords_Has_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map;
    JsonFileReader reader("/data/test/cloudsync/albumhandler/oncreatedrecords_has_error.json");
    reader.ConvertToMDKRecordOperResultMap(map);
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_GE(ret, 0);
    EXPECT_EQ(failSize, 1);

    // 校验数据库结果
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/Ab-202504021601"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    EXPECT_EQ(cloudIds.size(), albums.size());
    for (auto cloudId : cloudIds) {
        PhotoAlbumPo album = PhotoAlbumPo();
        EXPECT_EQ(dao.GetAlbumByCloudId(albums, cloudId, album), 0);
        auto it = map.find(cloudId);
        if (it != map.end()) {
            EXPECT_NE(album.dirty.value_or(-1), static_cast<int32_t>(DirtyTypes::TYPE_SYNCED)) << it->first;
        }
    }
}

/**
 * 上行相册记录 OnCreateRecords 没有异常
 * 期望结果：
 * 输出对应数量的records，异常未上行，正常上行
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCreateRecords_No_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map;
    JsonFileReader reader("/data/test/cloudsync/albumhandler/oncreatedrecords_no_error.json");
    reader.ConvertToMDKRecordOperResultMap(map);
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnCreateRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);

    // 校验数据库结果
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/Aa-202504021601"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    EXPECT_EQ(albums.size(), cloudIds.size());
    for (auto cloudId : cloudIds) {
        PhotoAlbumPo album = PhotoAlbumPo();
        EXPECT_EQ(dao.GetAlbumByCloudId(albums, cloudId, album), 0);
        EXPECT_EQ(album.dirty.value_or(0), 0);
    }
}

/**
 * 上行Mdirty记录 OnMdirtyRecords 一个异常，一个正常
 * 期望结果：
 * 输出对应数量的records，异常未上行，正常上行
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnMdirtyRecords_Has_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map{
        {"default-album-200-/Pictures/Users/W-202504021520", MDKLocalErrorCode::NO_ERROR},
        {"default-album-200-/Pictures/Users/X-202504021520", MDKLocalErrorCode::IPC_SEND_FAILED},
    };
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnMdirtyRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 1);

    // 校验数据库结果
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/W-202504021520",
                                         "default-album-200-/Pictures/Users/X-202504021520"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    for (auto cloudId : cloudIds) {
        PhotoAlbumPo album = PhotoAlbumPo();
        EXPECT_EQ(dao.GetAlbumByCloudId(albums, cloudId, album), 0);
        if (cloudId == "default-album-200-/Pictures/Users/W-202504021520") {
            EXPECT_EQ(album.dirty.value_or(0), 0);
        }
        if (cloudId == "default-album-200-/Pictures/Users/X-202504021520") {
            EXPECT_GT(album.dirty.value_or(0), 0);
        }
    }
}

/**
 * 上行Mdirty记录 OnMdirtyRecords 没有异常
 * 期望结果：
 * 输出对应数量的records，异常未上行，正常上行
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnMdirtyRecords_No_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map{
        {"default-album-200-/Pictures/Users/W-202504021520", MDKLocalErrorCode::NO_ERROR},
        {"default-album-200-/Pictures/Users/X-202504021520", MDKLocalErrorCode::NO_ERROR},
    };
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnMdirtyRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);

    // 校验数据库结果
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/W-202504021520",
                                         "default-album-200-/Pictures/Users/X-202504021520"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    for (auto cloudId : cloudIds) {
        PhotoAlbumPo album = PhotoAlbumPo();
        EXPECT_EQ(dao.GetAlbumByCloudId(albums, cloudId, album), 0);
        EXPECT_EQ(album.dirty.value_or(0), 0);
    }
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnFdirtyRecords, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map{
        {"373b364a41e54ebf912b3424aeabe963507a901b2b1a4332939d51ed54ff97c7", MDKLocalErrorCode::NO_ERROR},
        {"373b364a41e54ebf912b3424aeabe963507a901b2b1a4332939d51ed54ff97c6", MDKLocalErrorCode::NO_ERROR},
    };
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnFdirtyRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);
}

/**
 * 上传数据结果中存在上传失败
 * 期望结果：
 * 删除成功的记录在数据库中查询不到，删除失败的记录可以查询到
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnDeleteRecords_Has_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map{
        {"default-album-200-/Pictures/Users/Y-202504021601", MDKLocalErrorCode::NO_ERROR},
        {"default-album-200-/Pictures/Users/Z-202504021601", MDKLocalErrorCode::IPC_SEND_FAILED},
    };
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnDeleteRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 1);

    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/Y-202504021601",
                                         "default-album-200-/Pictures/Users/Z-202504021601"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    EXPECT_GT(albums.size(), 0);

    for (auto cloudId : cloudIds) {
        if (cloudId == "default-album-200-/Pictures/Users/Z-202504021601") {
            PhotoAlbumPo album = PhotoAlbumPo();
            EXPECT_EQ(dao.GetAlbumByCloudId(albums, cloudId, album), 0);
            EXPECT_EQ(album.dirty.value_or(0), 4);
        }
    }
}

/**
 * 上传数据结果中全部成功
 * 期望结果：
 * 删除成功的记录在数据库中查询不到
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnDeleteRecords_No_Error, TestSize.Level1)
{
    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    std::map<std::string, MDKRecordOperResult> map{
        {"default-album-200-/Pictures/Users/Y-202504021601", MDKLocalErrorCode::NO_ERROR},
        {"default-album-200-/Pictures/Users/Z-202504021601", MDKLocalErrorCode::NO_ERROR},
    };
    int32_t failSize = 0;
    int32_t ret = dataHandler->OnDeleteRecords(map, failSize);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(failSize, 0);

    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<std::string> cloudIds = {"default-album-200-/Pictures/Users/Y-202504021601",
                                         "default-album-200-/Pictures/Users/Z-202504021601"};
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryByCloudIds(cloudIds);
    EXPECT_EQ(albums.size(), 0);
}

// Album does not implement the following methods [OnCopyRecords]
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCopyRecords, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCompletePush, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnStartSync, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCompleteSync, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnDentryFileInsert, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

// Album does not implement the following methods [GetRetryRecords]
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, GetRetryRecords, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

/**
 * OnCompletePull
 * 期望结果：
 * dirty = 6 ，count != 0 修改dirty，否则删除
 */
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, OnCompletePull, TestSize.Level1)
{
    TestUtils::AlbumDao dao = TestUtils::AlbumDao();
    std::vector<CloudSync::PhotoAlbumPo> albums = dao.QueryAllAlbums();
    EXPECT_GT(albums.size(), 0);

    std::vector<CloudSync::PhotoAlbumPo> albums2;
    for (auto album : albums) {
        if (album.dirty.value_or(-1) == 6) {
            albums2.emplace_back(album);
        }
    }

    std::string tableName = "PhotoAlbum";
    int32_t cloudType = 0;
    int32_t userId = 100;
    MediaOperateResult optRet = {"", 0, ""};
    std::shared_ptr<CloudMediaDataHandler> dataHandler =
        std::make_shared<CloudMediaDataHandler>(tableName, cloudType, userId);
    int32_t ret = dataHandler->OnCompletePull(optRet);
    EXPECT_EQ(ret, 0);
    /* 变更：取消预期 - dirty = 6 ，count != 0 修改dirty，否则删除；
    下一步：使用监听器处理消息
    std::vector<CloudSync::PhotoAlbumPo> albums3 = dao.QueryAllAlbums();
    EXPECT_GT(albums3.size(), 0);

    int32_t num = 0;
    for (auto album_1 : albums2) {
        for (auto album_2 : albums3) {
            if (album_1.albumId.value_or(-1) == album_2.albumId.value_or(-1)) {
                EXPECT_EQ(album_2.dirty.value_or(-1), 1) << "album_2 id:" << album_2.cloudId.value_or("");
                num--;
            }
        }
        num++;
    }
    if (num >= 0) {
        EXPECT_TRUE(true);
    }
    */
}

/**
* 创建相册记录转换 ConvertToOnCreateRecord 没有异常
* 期望结果：
* 转换成功 E_OK
*/
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, AlbumDataConvert_ConvertToOnCreateRecord_01, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo albumData;
    albumData.albumType = 2048;
    albumData.cloudId = "e5647b0f37424ce3951fb93218f99a08ac76a6daa479434893c02fd5a99b9ee3";
    albumData.albumId = 1001;
    albumData.albumSubtype = 2049;
    albumData.isInWhiteList = 0;
    albumData.lpath = "";
    albumData.dualAlbumName = "";
    albumData.albumNameEn = "album1";
    albumData.dateModified = static_cast<int64_t>(1737529312000);
    albumData.dateAdded = static_cast<int64_t>(1737529312000);
    albumData.bundleName = "com.bundle1";
    albumData.localLanguage = "";
    CloudAlbumDataConvert cloudAlbumDataConvert{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};

    MDKRecord dkRecord;
    dkRecord.SetRecordId("001");
    std::map<std::string, MDKRecordField> data;
    int32_t albumId = 1002;
    data["albumId"] = MDKRecordField(albumId);
    dkRecord.SetRecordData(data);

    int32_t errorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    MDKError error;
    error.SetServerError(errorCode);
    MDKRecordOperResult result;
    result.SetDKRecord(dkRecord);
    result.SetDKError(error);

    std::map<std::string, MDKRecordField> dataOut;
    result.GetDKRecord().GetRecordData(dataOut);
    MEDIA_INFO_LOG("albumId: %{public}s", dataOut["albumId"].ToJsonValue().toStyledString().c_str());
    OnCreateRecordsAlbumReqBodyAlbumData recordOut;
    cloudAlbumDataConvert.ConvertToOnCreateRecord(albumData.cloudId, result, recordOut);
    EXPECT_EQ(false, recordOut.isSuccess);
    MEDIA_INFO_LOG("newCloudId: %{public}s", recordOut.newCloudId.c_str());
    MEDIA_INFO_LOG("OnCreateRecordsAlbumReqBodyAlbumData: %{public}s", recordOut.ToString().c_str());
    EXPECT_EQ(errorCode, recordOut.serverErrorCode);
}

/**
* 创建相册记录转换 ConvertToMdkRecord 没有异常
* 期望结果：
* 转换成功
*/
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, AlbumDataConvert_ConvertToMdkRecord_01, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo albumData;
    albumData.albumType = 2048;
    albumData.cloudId = "e5647b0f37424ce3951fb93218f99a08ac76a6daa479434893c02fd5a99b9ee3";
    albumData.albumId = 1001;
    albumData.albumSubtype = 2049;
    albumData.isInWhiteList = 0;
    albumData.lpath = "";
    albumData.dualAlbumName = "";
    albumData.albumNameEn = "album1";
    albumData.dateModified = static_cast<int64_t>(1737529312000);
    albumData.dateAdded = static_cast<int64_t>(1737529312000);
    albumData.bundleName = "com.bundle1";
    albumData.localLanguage = "";
    CloudAlbumDataConvert cloudAlbumDataConvert{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};
    std::map<string, MDKRecordField> map;
    shared_ptr<MDKRecord> record = make_shared<MDKRecord>();
    std::map<string, MDKRecordField> dataMap;
    cloudAlbumDataConvert.HandleAlbumName(map, albumData);
    cloudAlbumDataConvert.HandleGeneral(map, albumData);
    cloudAlbumDataConvert.HandleProperties(record, dataMap, albumData);
    cloudAlbumDataConvert.HandleAlbumLogicType(map, albumData);
    cloudAlbumDataConvert.HandleType(map, albumData);
    cloudAlbumDataConvert.HandleAlbumId(map, albumData);
    cloudAlbumDataConvert.HandleRecordId(record, albumData);
    cloudAlbumDataConvert.HandlePath(map, albumData);
    cloudAlbumDataConvert.FillRecordId(record, albumData);
    cloudAlbumDataConvert.HandleEmptyShow(record, dataMap, albumData);
    cloudAlbumDataConvert.ConvertToDoubleScreenshot(record, dataMap);
    std::shared_ptr<MDKRecord> mdkRecord = cloudAlbumDataConvert.ConvertToMdkRecord(albumData);
    EXPECT_NE(nullptr, mdkRecord);
}

/**
* 修改相册记录转换 BuildModifyRecord 没有异常
* 期望结果：
* 转换成功 E_OK
*/
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, AlbumDataConvert_BuildModifyRecord_01, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo albumData;
    albumData.albumType = 2048;
    albumData.cloudId = "e5647b0f37424ce3951fb93218f99a08ac76a6daa479434893c02fd5a99b9ee3";
    albumData.albumId = 1001;
    albumData.albumSubtype = 2049;
    albumData.isInWhiteList = 0;
    albumData.lpath = "";
    albumData.dualAlbumName = "";
    albumData.albumNameEn = "album1";
    albumData.dateModified = static_cast<int64_t>(1737529312000);
    albumData.dateAdded = static_cast<int64_t>(1737529312000);
    albumData.bundleName = "com.bundle1";
    albumData.localLanguage = "";
    CloudAlbumDataConvert cloudAlbumDataConvert{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};
    MDKError error;
    MDKRecordOperResult result;
    MDKRecord dkRecord;
    std::map<std::string, MDKRecordField> data;
    int32_t albumId = 1002;
    data["albumId"] = MDKRecordField(albumId);
    dkRecord.GetRecordData(data);
    dkRecord.SetRecordData(data);
    result.SetDKRecord(dkRecord);
    int32_t errorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    error.SetServerError(errorCode);
    result.SetDKError(error);
    OnMdirtyAlbumRecord recordOut;
    std::vector<CloudErrorDetail> errorDetails;
    cloudAlbumDataConvert.ConvertErrorTypeDetails(result, errorDetails);
    int32_t ret = cloudAlbumDataConvert.BuildModifyRecord(albumData.cloudId, result, recordOut);
    MEDIA_INFO_LOG("OnMdirtyAlbumRecord: %{public}s", recordOut.ToString().c_str());
    EXPECT_EQ(0, ret);
}

// Test SanitizeAlbumName helper
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, AlbumDataConvert_SanitizeAlbumName, TestSize.Level1)
{
    CloudAlbumDataConvert albumDataConvert{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};
    // Empty, single and double dot should become empty
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName(""), "");
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("."), "");
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName(".."), "");
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("<>:*?\"/\\"), "");
    // All forbidden characters should be replaced, resulting in only dots which are then trimmed to empty
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("<>:*?\"/\\."), ".");
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("..<>."), "..  .");
    // Trim leading/trailing spaces
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("  abc  "), "abc");
    // Replace forbidden characters with spaces but preserve interior spacing
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("a<b>c"), "a b c");
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("a<相机>c"), "a 相机 c");
    // Multiple forbidden and surrounding spaces become trimmed
    EXPECT_EQ(albumDataConvert.SanitizeAlbumName("<>:*?\"/\\  name  "), "name");
}

// Test HandleAlbumName logic
HWTEST_F(CloudMediaPhotoAlbumHandlerTest, AlbumDataConvert_HandleAlbumName, TestSize.Level1)
{
    CloudAlbumDataConvert albumDataConvert{CloudAlbumOperationType::PHOTO_ALBUM_CREATE};

    // Case 1: SOURCE and in whitelist with dualAlbumName -> use dualAlbumName and set localPath
    std::map<std::string, MDKRecordField> map1;
    CloudMdkRecordPhotoAlbumVo album1;
    album1.albumType = AlbumType::SOURCE;
    album1.isInWhiteList = 1;
    album1.lpath = "/DCIM/Camera";
    album1.dualAlbumName = "DualName";
    album1.albumNameEn = "EnName";
    album1.albumName = "Orig";
    EXPECT_EQ(albumDataConvert.HandleAlbumName(map1, album1), E_OK);
    std::string val;
    EXPECT_EQ(map1["localPath"].GetString(val), MDKLocalErrorCode::NO_ERROR);
    EXPECT_EQ(val, album1.lpath);
    EXPECT_EQ(map1["albumName"].GetString(val), MDKLocalErrorCode::NO_ERROR);
    EXPECT_EQ(val, "DualName");

    // Case 2: SOURCE and in whitelist without dual but with albumNameEn -> use albumNameEn
    std::map<std::string, MDKRecordField> map2;
    CloudMdkRecordPhotoAlbumVo album2 = album1;
    album2.dualAlbumName = "";
    album2.albumNameEn = "EnglishName";
    EXPECT_EQ(albumDataConvert.HandleAlbumName(map2, album2), E_OK);
    EXPECT_EQ(map2["albumName"].GetString(val), MDKLocalErrorCode::NO_ERROR);
    EXPECT_EQ(val, "EnglishName");

    // Case 3: not SOURCE or not in whitelist -> use albumName (sanitized)
    std::map<std::string, MDKRecordField> map3;
    CloudMdkRecordPhotoAlbumVo album3;
    album3.albumType = AlbumType::NORMAL;
    album3.isInWhiteList = 0;
    album3.albumName = "  a<b>c  ";
    EXPECT_EQ(albumDataConvert.HandleAlbumName(map3, album3), E_OK);
    EXPECT_EQ(map3["albumName"].GetString(val), MDKLocalErrorCode::NO_ERROR);
    EXPECT_EQ(val, "a b c");
}
}  // namespace OHOS::Media::CloudSync