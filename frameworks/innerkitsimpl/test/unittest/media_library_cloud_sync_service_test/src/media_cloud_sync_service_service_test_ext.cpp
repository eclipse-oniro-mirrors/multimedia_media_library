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

#include "media_cloud_sync_service_service_test_ext.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#define private public
#include "cloud_media_album_service.h"
#include "cloud_media_photos_service.h"
#undef private

using namespace testing::ext;
using namespace OHOS::AAFwk;

namespace OHOS::Media::CloudSync {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoAlbumColumns::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void CloudMediaSyncServiceTestExt::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start CloudMediaSyncServiceTestExt failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void CloudMediaSyncServiceTestExt::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("CloudMediaSyncServiceTestExt  finish");
}

void CloudMediaSyncServiceTestExt::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start CloudMediaSyncServiceTestExt failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void CloudMediaSyncServiceTestExt::TearDown() {}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_HandleFetchOldRecord_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    bool bContinue = true;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_OnFetchOldRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    std::vector<PhotoAlbumDto> records = {record};
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.OnFetchOldRecords(records, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_HandleLPathRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleLPathRecords(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changeType, ChangeInfo::ChangeType::INSERT);
    EXPECT_EQ(resp.stats[StatsIndex::NEW_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_HandleLPathRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = true;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleLPathRecords(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_OnMdirtyRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = true;
    std::vector<PhotoAlbumDto> albumDtoList = {dto1};
    int32_t failSize = 0;
    int32_t ret = service.OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 0);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_OnDeleteRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = true;
    std::vector<PhotoAlbumDto> albumDtoList = {dto1};
    int32_t failSize = 0;
    int32_t ret = service.OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 0);
}

HWTEST_F(CloudMediaSyncServiceTestExt, AlbumService_OnFetchRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats;
    std::vector<std::string> failedRecords;
    int32_t ret = service.OnFetchRecords(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_HandleRecord_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds = {"cloud_id1", "cloud_id2", "cloud_id3"};
    CloudMediaPullDataDto dto1;
    dto1.localPath = "/xx/filename1.txt";
    dto1.basicIsDelete = true;
    dto1.localFileId = 1;
    CloudMediaPullDataDto dto2;
    dto2.localPath = "/xx/filename2.txt";
    dto2.basicIsDelete = false;
    dto2.localFileId = 1;
    CloudMediaPullDataDto dto3;
    dto3.localPath = "";
    dto3.basicIsDelete = false;
    dto3.localFileId = 1;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap = {
        {"cloud_id1", dto1},
        {"cloud_id2", dto2},
        {"cloud_id3", dto3},
    };
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats(StatsIndex::DELETE_RECORDS_COUNT + 1);
    std::vector<std::string> failedRecords;

    int32_t ret = service.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], 1);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_PullDelete_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localPosition = 1;
    data.localDirty = 2;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.PullDelete(data, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_PullDelete_Test_002, TestSize.Level1)
{
    std::string fileName = "/data/testtdd.txt";
    char buffer[] = "xxxxLIVE_xxxx";
    int32_t fd = open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0x777);
    write(fd, buffer, strlen(buffer));
    close(fd);

    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localPosition = 1;
    data.localDirty = 1;
    data.localPath = fileName;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullDelete(data, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    system("rm -rf /data/testtdd.txt");
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_PullUpdate_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.localDirty = 0;
    pullData.localDateModified = "";
    pullData.attributesEditedTimeMs = 1;
    pullData.localDateAdded = "";

    std::set<std::string> refreshAlbums;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats(5);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullUpdate(pullData, refreshAlbums, fdirtyData, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_PullUpdate_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.localDirty = 0;
    pullData.localDateModified = "11";
    pullData.attributesEditedTimeMs = 1;
    pullData.localPosition = 1;
    pullData.localPath = "/data/local/tmp";

    std::set<std::string> refreshAlbums;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats(5);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullUpdate(pullData, refreshAlbums, fdirtyData, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_DoDataMerge_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    KeyData localKeyData;
    localKeyData.modifyTime = 1;
    KeyData cloudKeyData;
    cloudKeyData.modifyTime = 2;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.DoDataMerge(pullData, localKeyData, cloudKeyData, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_PullInsert_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data1;
    data1.basicFileName = "name1";
    data1.basicFileType = -1;
    data1.hasProperties = true;
    data1.propertiesSourceFileName = "";
    data1.hasAttributes = false;
    std::vector<CloudMediaPullDataDto> pullDatas = {data1};
    std::vector<std::string> failedRecords;

    int32_t ret = service.PullInsert(pullDatas, failedRecords);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failedRecords.size(), 1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_CreateEntry_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data1;
    data1.basicFileType = 2;
    std::vector<CloudMediaPullDataDto> pullDatas = {data1};
    std::set<std::string> refreshAlbums;
    std::vector<PhotosDto> newData;
    std::vector<int32_t> stats(5);
    std::vector<std::string> failedRecords;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CreateEntry(pullDatas, refreshAlbums, newData, stats, failedRecords, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_GetDeletedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    std::vector<PhotosPo> cloudRecordPoList = service.GetDeletedRecords(size);
    EXPECT_EQ(cloudRecordPoList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnCreateRecordSuccess_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = -1;
    std::unordered_map<std::string, LocalInfo> localMap;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, localMap, photoRefresh);
    EXPECT_EQ(ret, E_INVAL_ARG);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnCreateRecordSuccess_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = 0;
    std::unordered_map<std::string, LocalInfo> localMap;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, localMap, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnCreateRecordSuccess_Test_003, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = 0;
    LocalInfo info;
    std::unordered_map<std::string, LocalInfo> localMap = {
        {"0", info},
    };
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, localMap, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnFdirtyRecordSuccess_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    std::unordered_map<std::string, LocalInfo> localMap;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnFdirtyRecordSuccess(record, localMap, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_HandleNoContentUploadFail_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.path = "/data/local/tmp";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.HandleNoContentUploadFail(photo, photoRefresh);
    EXPECT_EQ(ret, E_RDB);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_HandleNoContentUploadFail_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.path = "/data/tddxxx.txt";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.HandleNoContentUploadFail(photo, photoRefresh);
    EXPECT_EQ(ret, E_RDB);
}
}