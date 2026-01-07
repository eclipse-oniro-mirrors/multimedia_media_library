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

#include "media_cloud_sync_service_service_test.h"

#include <memory>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include "media_cloud_sync_test_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "cloud_media_operation_code.h"
#include "cloud_media_sync_const.h"
#include "exif_rotate_utils.h"
#include "photo_album_column.h"

#include "cloud_media_dfx_service.h"
#define private public
#include "cloud_media_album_service.h"
#include "cloud_media_data_service.h"
#include "cloud_media_download_service.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_scan_service.h"
#undef private

using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::AAFwk;

namespace OHOS::Media::CloudSync {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void CloudMediaSyncServiceTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "init g_rdbStore failed";
        exit(1);
    }
    InitTestTables(g_rdbStore);
}

void CloudMediaSyncServiceTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
}

void CloudMediaSyncServiceTest::SetUp() {}

void CloudMediaSyncServiceTest::TearDown() {}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetCreatedRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    int32_t ret = 0;
    std::vector<PhotoAlbumPo> photoAlbumList;
    bool isCloudSpaceFull = true;
    ret = service.GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetCreatedRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    int32_t ret = 0;
    std::vector<PhotoAlbumPo> photoAlbumList;
    bool isCloudSpaceFull = false;
    ret = service.GetCreatedRecords(size, isCloudSpaceFull, photoAlbumList);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetAlbumMetaModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    std::vector<PhotoAlbumPo> photoAlbumList = service.GetAlbumMetaModifiedRecords(size);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetAlbumFileModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    std::vector<PhotoAlbumPo> photoAlbumList = service.GetAlbumFileModifiedRecords(size);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetAlbumDeletedRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    std::vector<PhotoAlbumPo> photoAlbumList = service.GetAlbumDeletedRecords(size);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_GetAlbumCopyRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t size = 0;
    std::vector<PhotoAlbumPo> photoAlbumList = service.GetAlbumCopyRecords(size);
    EXPECT_EQ(photoAlbumList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCreateRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    std::vector<PhotoAlbumDto> albumDtoList;
    int32_t failSize = -1;
    int32_t ret = service.OnCreateRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCreateRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    PhotoAlbumDto dto2;
    dto2.cloudId = "id2";
    dto2.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnCreateRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCreateRecords_Test_003, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    dto1.errorType = ErrorType::TYPE_UNKNOWN;
    dto1.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    dto1.errorDetails = errorDetails;
    PhotoAlbumDto dto2;
    dto2.cloudId = "id2";
    dto2.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnCreateRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnMdirtyRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    std::vector<PhotoAlbumDto> albumDtoList;
    int32_t failSize = -1;
    int32_t ret = service.OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnMdirtyRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    PhotoAlbumDto dto2;
    dto2.cloudId = "id2";
    dto2.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnMdirtyRecords_Test_003, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    dto1.errorType = ErrorType::TYPE_UNKNOWN;
    dto1.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    dto1.errorDetails = errorDetails;
    PhotoAlbumDto dto2;
    dto2.cloudId = "id2";
    dto2.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnMdirtyRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFdirtyRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnFdirtyRecords();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnDeleteRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    std::vector<PhotoAlbumDto> albumDtoList;
    int32_t failSize = -1;
    int32_t ret = service.OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnDeleteRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    PhotoAlbumDto dto2;
    dto1.cloudId = "id2";
    dto1.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCopyRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnCopyRecords();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    PhotoAlbumDto dto2;
    dto2.lPath = DEFAULT_SCREENSHOT_LPATH_EN;
    dto2.cloudId = "id2";
    dto2.isSuccess = true;
    PhotoAlbumDto dto3;
    dto3.cloudId = DEFAULT_SCREENSHOT_CLOUDID;
    dto3.isSuccess = false;
    PhotoAlbumDto dto4;
    dto4.lPath = "/xxxx";
    dto4.cloudId = "id1";
    dto4.isSuccess = false;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2, dto3, dto4};
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = service.OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchRecords_Test_002, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.cloudId = "id1";
    dto1.isSuccess = false;
    PhotoAlbumDto dto2;
    dto1.cloudId = "id2";
    dto1.isSuccess = true;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1, dto2};
    int32_t failSize = 0;
    int32_t ret = service.OnDeleteRecords(albumDtoList, failSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchOldRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.isDelete = false;
    dto1.cloudId = "cloud_id1";

    std::vector<PhotoAlbumDto> albumDtoList = {dto1};
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = service.OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchOldRecords_Test_002, TestSize.Level1)
{
    std::string cloudId = "cloud_id1";

    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    ValuesBucket values2;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values2);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values2);

    CloudMediaAlbumService service;
    PhotoAlbumDto dto1;
    dto1.isDelete = false;
    dto1.cloudId = cloudId;

    std::vector<PhotoAlbumDto> albumDtoList = {dto1};
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = service.OnFetchRecords(albumDtoList, resp);
    EXPECT_EQ(ret, E_OK);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_HandleFetchOldRecord_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = false;
    bool bContinue = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changeType, ChangeInfo::ChangeType::INSERT);
    EXPECT_EQ(resp.stats[StatsIndex::NEW_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_HandleFetchOldRecord_Test_002, TestSize.Level1)
{
    std::string cloudId = "cloud_id1";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);
    ValuesBucket values2;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values2);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values2);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = false;
    record.cloudId = cloudId;
    bool bContinue = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_DATA);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_HandleFetchOldRecord_Test_003, TestSize.Level1)
{
    std::string cloudId = "cloud_id1";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values1);
    SetValuesBucketInPhotoAlbumTable("dirty", "1", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = false;
    record.cloudId = cloudId;
    bool bContinue = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(bContinue, true);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_HandleFetchOldRecord_Test_004, TestSize.Level1)
{
    std::string cloudId = "cloud_id1";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values1);
    SetValuesBucketInPhotoAlbumTable("dirty", "0", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = true;
    record.cloudId = cloudId;
    bool bContinue = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.stats[StatsIndex::DELETE_RECORDS_COUNT], 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_HandleFetchOldRecord_Test_005, TestSize.Level1)
{
    std::string cloudId = "cloud_id1";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("cloud_id", cloudId, values1);
    SetValuesBucketInPhotoAlbumTable("dirty", "0", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.isDelete = false;
    record.cloudId = cloudId;
    bool bContinue = false;
    ChangeInfo::ChangeType changeType = ChangeInfo::ChangeType::INVAILD;
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.HandleFetchOldRecord(record, bContinue, changeType, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT], 1);
    EXPECT_EQ(changeType, ChangeInfo::ChangeType::UPDATE);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchLPathRecords_Test_001, TestSize.Level1)
{
    std::string lpath = "/picture/temp";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_LPATH, lpath, values1);
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_DIRTY, "-1", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.lPath = lpath;
    std::vector<PhotoAlbumDto> records = {record};
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.OnFetchLPathRecords(records, resp);
    EXPECT_EQ(ret, E_OK);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchLPathRecords_Test_002, TestSize.Level1)
{
    std::string lpath = "/picture/temp";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_LPATH, lpath, values1);
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_DIRTY, "0", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.lPath = lpath;
    record.isDelete = true;
    std::vector<PhotoAlbumDto> records = {record};
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.OnFetchLPathRecords(records, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.stats[StatsIndex::DELETE_RECORDS_COUNT], 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchLPathRecords_Test_003, TestSize.Level1)
{
    std::string lpath = "/Pictures/hiddenAlbum";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_LPATH, lpath, values1);
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_DIRTY, "0", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.lPath = lpath;
    record.isDelete = true;
    std::vector<PhotoAlbumDto> records = {record};
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.OnFetchLPathRecords(records, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.stats[StatsIndex::DELETE_RECORDS_COUNT], 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnFetchLPathRecords_Test_004, TestSize.Level1)
{
    std::string lpath = "/picture/temp";
    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_LPATH, lpath, values1);
    SetValuesBucketInPhotoAlbumTable(PhotoAlbumColumns::ALBUM_DIRTY, "0", values1);
    InsertTable(g_rdbStore, PhotoAlbumColumns::TABLE, values1);

    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    record.lPath = lpath;
    record.isDelete = false;
    std::vector<PhotoAlbumDto> records = {record};
    OnFetchRecordsAlbumRespBody resp;

    int32_t ret = service.OnFetchLPathRecords(records, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.stats[StatsIndex::META_MODIFY_RECORDS_COUNT], 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnDentryFileInsert_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnDentryFileInsert();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnStartSync_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnStartSync();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCompleteSync_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnCompleteSync();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCompletePull_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    MediaOperateResult optRet = {"", 0, ""};
    int32_t ret = service.OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCompletePush_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnCompletePush();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, AlbumService_OnCompleteCheck_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    int32_t ret = service.OnCompleteCheck();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_OnCompleteCheck_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::string cloudId = "id1";
    int32_t dirtyType = 0;
    int32_t ret = service.UpdateDirty(cloudId, dirtyType);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_UpdatePosition_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<std::string> cloudIds = {"id1"};
    int32_t position = 0;
    int32_t ret = service.UpdatePosition(cloudIds, position);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_UpdateSyncStatus_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::string cloudId = "id1";
    int32_t syncStatus = 1;
    int32_t ret = service.UpdateSyncStatus(cloudId, syncStatus);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_UpdateThmStatus_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::string cloudId = "id1";
    int32_t thmStatus = 1;
    int32_t ret = service.UpdateThmStatus(cloudId, thmStatus);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetAgingFile_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    AgingFileQueryDto queryDto;
    std::vector<PhotosDto> photosDtos;
    int32_t ret = service.GetAgingFile(queryDto, photosDtos);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetActiveAgingFile_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    AgingFileQueryDto queryDto;
    std::vector<PhotosDto> photosDtos;
    int32_t ret = service.GetActiveAgingFile(queryDto, photosDtos);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetVideoToCache_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<PhotosDto> photosDtos;
    int32_t ret = service.GetVideoToCache(photosDtos);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetFilePosStat_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<PhotosDto> photosDtos;
    std::vector<uint64_t> filePosStat = service.GetFilePosStat();
    EXPECT_EQ(filePosStat.size(), 3);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetCloudThmStat_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<PhotosDto> photosDtos;
    std::vector<uint64_t> filePosStat = service.GetCloudThmStat();
    EXPECT_EQ(filePosStat.size(), 4);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_GetDirtyTypeStat_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<uint64_t> dirtyTypeStat;
    int32_t ret = service.GetDirtyTypeStat(dirtyTypeStat);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(dirtyTypeStat.size(), 9);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_UpdateLocalFileDirty_Test_001, TestSize.Level1)
{
    CloudMediaDataService service;
    std::vector<std::string> cloudIds = {"id1"};
    int32_t ret = service.UpdateLocalFileDirty(cloudIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataService_UpdateLocalFileDirty_Test_002, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/data", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);
    ValuesBucket values2;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id1", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "0", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/data", values2);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values2);
    ValuesBucket values3;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id2", values3);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "2", values3);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "", values3);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values3);
    ValuesBucket values4;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id3", values4);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "2", values4);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/xxx", values4);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values4);
    ValuesBucket values5;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id4", values5);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "2", values5);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/data", values5);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values5);

    CloudMediaDataService service;
    std::vector<std::string> cloudIds = {"", "cloud_id1", "cloud_id2", "cloud_id3", "cloud_id4"};
    int32_t ret = service.UpdateLocalFileDirty(cloudIds);
    EXPECT_EQ(ret, E_OK);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_SyncStart_Test_001, TestSize.Level1)
{
    CloudMediaDfxService service;
    std::string taskId = "id1";
    service.SyncStart(taskId);
    EXPECT_EQ(taskId, "id1");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_UpdateMetaStat_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    uint32_t index = 1;
    uint64_t diff = 10;
    service->UpdateMetaStat(index, diff);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_UpdateAttachmentStat_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    uint32_t index = 1;
    uint64_t diff = 10;
    service->UpdateAttachmentStat(index, diff);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_UpdateAlbumStat_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    uint32_t index = 1;
    uint64_t diff = 10;
    service->UpdateAlbumStat(index, diff);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_UpdateUploadMetaStat_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    uint32_t index = 1;
    uint64_t diff = 10;
    service->UpdateUploadMetaStat(index, diff);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_UpdateUploadDetailError_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    int32_t error = -1;
    service->UpdateUploadDetailError(error);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_SyncEnd_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    int32_t stopReason = 1;
    service->SyncEnd(stopReason);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDfxService_ReportSyncFault_Test_001, TestSize.Level1)
{
    auto service = std::make_shared<CloudMediaDfxService>();
    ASSERT_NE(service, nullptr);
    std::string funcName = "test";
    int32_t lineNum = 20;
    SyncFaultEvent event;
    service->ReportSyncFault(funcName, lineNum, event);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadThmNum_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    int32_t type = 0;
    int32_t totalNum;
    int32_t ret = service.GetDownloadThmNum(type, totalNum);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(totalNum, 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadThms_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    DownloadThumbnailQueryDto queryDto;
    std::vector<PhotosDto> photosDtos;
    int32_t ret = service.GetDownloadThms(queryDto, photosDtos);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photosDtos.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadThmsByUri_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::vector<int32_t> fileIds = {1, 2, 3};
    int32_t type = 1;
    std::vector<PhotosDto> ret = service.GetDownloadThmsByUri(fileIds, type);
    EXPECT_EQ(ret.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadThmsByUri_Test_002, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_ID, "1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_SYNC_STATUS, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLEAN_FLAG, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/storage1/cloud1/files1/filename1.txt", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);
    ValuesBucket values2;
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_ID, "2", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_SYNC_STATUS, "0", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLEAN_FLAG, "0", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/p/f.txt", values2);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values2);

    CloudMediaDownloadService service;
    std::vector<int32_t> fileIds = {1, 2, 3};
    int32_t type = 3;
    std::vector<PhotosDto> ret = service.GetDownloadThmsByUri(fileIds, type);
    EXPECT_EQ(ret.size(), 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_OnDownloadThms_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::unordered_map<std::string, int32_t> downloadThumbnailMap = {
        {"key1", 1},
        {"key2", 2},
        {"key3", 3},
        {"key4", 4},
    };
    std::vector<MediaOperateResultDto> result;
    int32_t ret = service.OnDownloadThms(downloadThumbnailMap, result);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadAsset_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::vector<int32_t> fileIds = {1, 2, 3, 4, 5, 6, 7};
    std::vector<PhotosDto> ret = service.GetDownloadAsset(fileIds);
    EXPECT_EQ(ret.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_GetDownloadAsset_Test_002, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_ID, "1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_SYNC_STATUS, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLEAN_FLAG, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/storage1/cloud1/files1/filename1.txt", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);
    ValuesBucket values2;
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_ID, "2", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_SYNC_STATUS, "0", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLEAN_FLAG, "0", values2);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_FILE_PATH, "/p/f.txt", values2);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values2);

    CloudMediaDownloadService service;
    std::vector<int32_t> fileIds = {1, 2, 3};
    std::vector<PhotosDto> ret = service.GetDownloadAsset(fileIds);
    EXPECT_EQ(ret.size(), 2);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_OnDownloadAsset_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::vector<std::string> fileIds = {"id1", "id2", "id3", "id4"};
    std::vector<MediaOperateResultDto> result;
    int32_t ret = service.OnDownloadAsset(fileIds, result);
    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_OnDownloadAsset_Test_002, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::vector<std::string> fileIds;
    std::vector<MediaOperateResultDto> result;
    int32_t ret = service.OnDownloadAsset(fileIds, result);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_OnDownloadAsset_Test_003, TestSize.Level1)
{
    std::string fileName = "/data/filename1.txt";
    char buffer[] = "xxLIVE_123456789123456";
    int32_t fd = open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0x777);
    write(fd, buffer, strlen(buffer));
    close(fd);

    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_SUBTYPE, "3", values1);
    SetValuesBucketInPhotosTable(MediaColumn::MEDIA_FILE_PATH, fileName, values1);
    SetValuesBucketInPhotosTable(MediaColumn::MEDIA_DATE_MODIFIED, "100", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);

    CloudMediaDownloadService service;
    std::vector<std::string> fileIds = {"cloud_id1"};
    std::vector<MediaOperateResultDto> result;
    int32_t ret = service.OnDownloadAsset(fileIds, result);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(result.size(), 1);

    InitTestTables(g_rdbStore);
    system("rm -rf /data/filename1.txt");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_UnlinkAsset_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    OnDownloadAssetData assetData;
    assetData.localPath = "/data/abcedf.txt";
    service.UnlinkAsset(assetData);
    EXPECT_EQ(assetData.errorMsg, "unlink failed");

    int fd = creat(assetData.localPath.c_str(), S_IRUSR | S_IWUSR);
    ASSERT_GT(fd, 0);
    service.UnlinkAsset(assetData);
    EXPECT_EQ(access(assetData.localPath.c_str(), F_OK), -1);
    close(fd);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_ResetAssetModifyTime_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    OnDownloadAssetData assetData;
    assetData.localPath = "/xxx/xxx";
    EXPECT_EQ(access(assetData.localPath.c_str(), F_OK), -1);
    service.ResetAssetModifyTime(assetData);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_ResetAssetModifyTime_Test_002, TestSize.Level1)
{
    std::string filename = "/data/filetdd.txt";
    int fd = creat(filename.c_str(), S_IRUSR | S_IWUSR);
    ASSERT_GT(fd, 0);
    EXPECT_EQ(access(filename.c_str(), F_OK), E_OK);

    CloudMediaDownloadService service;
    OnDownloadAssetData assetData;
    assetData.localPath = filename;
    service.ResetAssetModifyTime(assetData);
    system("rm -rf /data/filetdd.txt");
    close(fd);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_SliceAssetFile_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    std::string originalFile = "/xxx/xxx";
    std::string path = "";
    std::string videoPath = "";
    std::string extraDataPat = "";
    int32_t ret = service.SliceAssetFile(originalFile, path, videoPath, extraDataPat);
    EXPECT_EQ(ret, E_PATH);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_SliceAssetFile_Test_002, TestSize.Level1)
{
    std::string filename = "/data/filetdd.txt";
    int32_t fd = open(filename.c_str(), O_CREAT | O_RDONLY, S_IRUSR);
    close(fd);

    CloudMediaDownloadService service;
    std::string originalFile = filename;
    std::string path = "";
    std::string videoPath = "";
    std::string extraDataPat = "";
    int32_t ret = service.SliceAssetFile(originalFile, path, videoPath, extraDataPat);
    EXPECT_EQ(ret, E_PATH);

    system("rm -rf /data/filetdd.txt");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataServiceProcessor_GetPhotosDto_Test_001, TestSize.Level1)
{
    CloudMediaDataServiceProcessor processor;
    PhotosPo photosPo1;
    photosPo1.data = "/filepath/filename.txt";
    PhotosPo photosPo2;
    std::vector<PhotosPo> photosPos = {photosPo1, photosPo2};
    std::vector<PhotosDto> photosDtos;

    processor.GetPhotosDto(photosPos, photosDtos);
    EXPECT_EQ(photosPos.size(), photosDtos.size() + 1);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDataServiceProcessor_GetPhotosDtoOfVideoCache_Test_001, TestSize.Level1)
{
    CloudMediaDataServiceProcessor processor;
    PhotosPo photosPo1;
    photosPo1.data = "/filepath/filename.txt";
    PhotosPo photosPo2;
    photosPo2.data = "/filepath/filename2.txt";
    std::vector<PhotosPo> photosPos = {photosPo1, photosPo2};
    std::vector<PhotosDto> photosDtos;

    processor.GetPhotosDto(photosPos, photosDtos);
    EXPECT_EQ(photosPos.size(), photosDtos.size());
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadServiceProcessor_GetPhotosDto_Test_001, TestSize.Level1)
{
    CloudMediaDownloadServiceProcessor processor;
    PhotosPo photosPo;
    photosPo.data = "/filepath/filename.txt";
    photosPo.fileId = 1;
    photosPo.size = 2;
    photosPo.mediaType = 3;
    photosPo.cloudId = "id1";
    photosPo.thumbStatus = 4;
    photosPo.orientation = 5;
    std::vector<PhotosPo> photosPos = {photosPo};
    std::vector<PhotosDto> photosDtos = processor.GetPhotosDto(photosPos);
    EXPECT_EQ(photosPos.size(), photosDtos.size());
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadServiceProcessor_GetDownloadAssetData_Test_001, TestSize.Level1)
{
    CloudMediaDownloadServiceProcessor processor;
    PhotosPo photosPo;
    photosPo.data = "/filepath/filename.txt";
    photosPo.fileId = 1;
    photosPo.size = 2;
    photosPo.mediaType = 3;
    photosPo.cloudId = "id1";
    photosPo.thumbStatus = 4;
    photosPo.orientation = 5;
    std::vector<PhotosPo> photosPos = {photosPo};
    std::vector<DownloadAssetData> downloadAssetDatas;
    processor.GetDownloadAssetData(photosPos, downloadAssetDatas);
    EXPECT_EQ(photosPos.size(), downloadAssetDatas.size());
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotoServiceProcessor_GetPhotosDtos_Test_001, TestSize.Level1)
{
    CloudMediaPhotoServiceProcessor processor;
    PhotosPo photosPo;
    photosPo.data = "/filepath/filename.txt";
    photosPo.fileId = 1;
    photosPo.size = 2;
    photosPo.mediaType = 3;
    photosPo.cloudId = "id1";
    photosPo.thumbStatus = 4;
    photosPo.orientation = 5;
    std::vector<PhotosPo> photosPos = {photosPo};
    std::vector<PhotosDto> photosDtoList = processor.GetPhotosDtos(photosPos);
    EXPECT_EQ(photosPos.size(), photosDtoList.size());
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetCheckRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds;
    std::vector<PhotosDto> result = service.GetCheckRecords(cloudIds);
    EXPECT_EQ(cloudIds.size(), result.size());
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetCreatedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    std::vector<PhotosPo> createdRecords;
    int32_t ret = service.GetCreatedRecords(size, createdRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetMetaModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    int32_t mdirty = 2;
    std::vector<PhotosPo> modifiedRecords;
    int32_t ret = service.GetMetaModifiedRecords(size, modifiedRecords, mdirty);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetMetaModifiedRecords_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    int32_t sdirty = 6;
    std::vector<PhotosPo> modifiedRecords;
    int32_t ret = service.GetMetaModifiedRecords(size, modifiedRecords, sdirty);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetFileModifiedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    std::vector<PhotosPo> modifiedRecords;
    int32_t ret = service.GetFileModifiedRecords(size, modifiedRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetDeletedRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    std::vector<PhotosPo> cloudRecordPoList = service.GetDeletedRecords(size);
    EXPECT_EQ(cloudRecordPoList.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetCopyRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t size = 0;
    std::vector<PhotosPo> copyRecords;
    int32_t ret = service.GetCopyRecords(size, copyRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetRetryRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds;
    int32_t ret = service.GetRetryRecords(cloudIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnStartSync_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t ret = service.OnStartSync();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCompleteSync_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t ret = service.OnCompleteSync();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCompletePull_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    MediaOperateResult optRet = {"", 0, ""};
    int32_t ret = service.OnCompletePull(optRet);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCompletePush_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t ret = service.OnCompletePush();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCompleteCheck_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    int32_t ret = service.OnCompleteCheck();
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ReportFailure_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS);
    failureDto.errorCode = E_THM_SOURCE_BASIC + ENOENT;
    failureDto.fileId = 1;
    int32_t ret = service.ReportFailure(failureDto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ReportFailure_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS);
    failureDto.errorCode = E_LCD_SOURCE_BASIC + ENOENT;
    failureDto.fileId = 1;
    int32_t ret = service.ReportFailure(failureDto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ReportFailure_Test_003, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS);
    failureDto.errorCode = E_DB_SIZE_IS_ZERO;
    failureDto.fileId = 1;
    int32_t ret = service.ReportFailure(failureDto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ReportFailure_Test_004, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS);
    failureDto.errorCode = E_LCD_IS_TOO_LARGE;
    failureDto.fileId = 1;
    int32_t ret = service.ReportFailure(failureDto);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ReportFailure_Test_005, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ReportFailureDto failureDto;
    failureDto.apiCode = static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS);
    failureDto.errorCode = 0;
    failureDto.fileId = 1;
    int32_t ret = service.ReportFailure(failureDto);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCreateRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnCreateRecords(records, failedSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    EXPECT_EQ(failedSize, 2);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCreateRecords_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    dto3.errorType = ErrorType::TYPE_UNKNOWN;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    dto3.errorDetails = errorDetails;

    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnCreateRecords(records, failedSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failedSize, 2);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnMdirtyRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    dto1.cloudId = "id1";
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnMdirtyRecords(records, failedSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    EXPECT_EQ(failedSize, 2);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnMdirtyRecords_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    dto1.cloudId = "id1";
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    dto3.errorType = ErrorType::TYPE_UNKNOWN;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    dto3.errorDetails = errorDetails;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnMdirtyRecords(records, failedSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(failedSize, 2);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnFdirtyRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    dto1.cloudId = "id1";
    dto1.metaDateModified = 10;
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    dto2.metaDateModified = -1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnFdirtyRecords(records, failedSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    EXPECT_EQ(failedSize, 2);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnDeleteRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    dto1.cloudId = "id1";
    dto1.metaDateModified = 10;
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    dto2.metaDateModified = -1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnDeleteRecords(records, failedSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    EXPECT_EQ(failedSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCopyRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto dto1;
    dto1.isSuccess = true;
    dto1.localId = -1;
    dto1.cloudId = "id1";
    dto1.metaDateModified = 10;
    PhotosDto dto2;
    dto2.isSuccess = true;
    dto2.localId = 1;
    dto2.metaDateModified = -1;
    PhotosDto dto3;
    dto3.isSuccess = false;
    dto3.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    dto3.fileId = 0;
    std::vector<PhotosDto> records = {dto1, dto2, dto3};
    int32_t failedSize = 0;
    int32_t ret = service.OnCopyRecords(records, failedSize);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);
    EXPECT_EQ(failedSize, 1);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnFetchRecords_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds = {"id1", "id2", "id3"};
    CloudMediaPullDataDto dto1;
    dto1.localPath = "/xx/filename1.txt";
    dto1.basicIsDelete = true;
    dto1.localFileId = 1;
    dto1.cloudId = "cloud1";
    CloudMediaPullDataDto dto2;
    dto2.localPath = "/xx/filename2.txt";
    dto2.basicIsDelete = false;
    dto2.localFileId = 1;
    dto2.cloudId = "cloud2";
    CloudMediaPullDataDto dto3;
    dto3.localPath = "";
    dto3.basicIsDelete = false;
    dto3.localFileId = 1;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap = {
        {"id1", dto1},
        {"id2", dto2},
        {"id3", dto3},
    };
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats;
    stats.resize(StatsIndex::DELETE_RECORDS_COUNT + 1);
    std::vector<std::string> failedRecords;

    int32_t ret = service.OnFetchRecords(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnFetchRecords_Test_002, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id1", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);
    ValuesBucket values2;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "", values2);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values2);

    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds = {"", "cloud_id1", "cloud_id2", "cloud_id3"};
    CloudMediaPullDataDto dto1;
    dto1.localPath = "/xx/filename1.txt";
    dto1.basicIsDelete = true;
    dto1.localFileId = 1;
    dto1.cloudId = "cloud1";
    CloudMediaPullDataDto dto2;
    dto2.localPath = "/xx/filename2.txt";
    dto2.basicIsDelete = false;
    dto2.localFileId = 1;
    dto2.cloudId = "cloud2";
    CloudMediaPullDataDto dto3;
    dto3.localPath = "";
    dto3.basicIsDelete = false;
    dto3.localFileId = 1;
    CloudMediaPullDataDto dto4;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap = {
        {"cloud_id1", dto1},
        {"cloud_id2", dto2},
        {"cloud_id3", dto3},
        {"", dto4},
    };
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats;
    stats.resize(StatsIndex::DELETE_RECORDS_COUNT + 1);
    std::vector<std::string> failedRecords;

    int32_t ret = service.OnFetchRecords(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(ret, E_OK);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_HandleRecord_Test_001, TestSize.Level1)
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

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ConvertPullDataToPhotosDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.basicFileType = FILE_TYPE_VIDEO;
    data.localPath = "";
    PhotosDto dto;

    service.ConvertPullDataToPhotosDto(data, dto);
    EXPECT_EQ(dto.size, data.localSize);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_PullDelete_Test_001, TestSize.Level1)
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

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_PullDelete_Test_002, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);

    std::string fileName = "/data/testtdd.txt";
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localPosition = 1;
    data.localDirty = 1;
    data.localPath = fileName;
    data.cloudId = "cloud_id";
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullDelete(data, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(refreshAlbums.size(), 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_IsMtimeChanged_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localDateModified = "11";
    data.attributesEditedTimeMs = 1;
    bool changed = false;

    int32_t ret = service.IsMtimeChanged(data, changed);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changed, true);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_IsMtimeChanged_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localDateModified = "";
    data.attributesEditedTimeMs = 1;
    bool changed = false;

    int32_t ret = service.IsMtimeChanged(data, changed);
    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
    EXPECT_EQ(changed, false);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_IsMtimeChanged_Test_003, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.localDateModified = "";
    data.attributesEditedTimeMs = -1;
    data.basicCreatedTime = 11;
    data.localDateAdded = "11";
    bool changed = true;

    int32_t ret = service.IsMtimeChanged(data, changed);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(changed, false);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_ExtractEditDataCamera_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data;
    data.attributesEditDataCamera = "11";
    data.localPath = "/data/local/tmp/testtdd.txt";
    service.ExtractEditDataCamera(data);
    EXPECT_EQ(data.localPath, "/data/local/tmp/testtdd.txt");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_PullUpdate_Test_001, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_CLOUD_ID, "cloud_id1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "0", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);

    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.localDirty = 0;
    pullData.localDateModified = "11";
    pullData.attributesEditedTimeMs = 1;
    pullData.localPosition = 1;
    pullData.localPath = "";
    pullData.attributesMediaType = 2;
    pullData.cloudId = "cloud_id1";

    std::set<std::string> refreshAlbums;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats(5);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullUpdate(pullData, refreshAlbums, fdirtyData, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(refreshAlbums.size(), 2);
    EXPECT_EQ(stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT], 1);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_GetMergeDataMap_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto data1;
    data1.hasAttributes = false;
    CloudMediaPullDataDto data2;
    data2.hasAttributes = true;
    data2.basicFileName = "xx";
    data2.basicSize = 0;
    data2.propertiesRotate = 0;
    data2.basicFileType = 0;
    std::vector<CloudMediaPullDataDto> pullDatas = {data1, data2};
    std::map<std::string, KeyData> mergeDataMap;
    service.GetMergeDataMap(pullDatas, mergeDataMap);
    EXPECT_EQ(mergeDataMap.size(), 1);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_DoDataMerge_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    KeyData localKeyData;
    localKeyData.modifyTime = 1;
    localKeyData.createTime = 1;
    KeyData cloudKeyData;
    cloudKeyData.modifyTime = 2;
    cloudKeyData.createTime = 2;
    std::set<std::string> refreshAlbums;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.DoDataMerge(pullData, localKeyData, cloudKeyData, refreshAlbums, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_PullRecordsConflictProc_Test_001, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_POSITION, "1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_NAME, "name3", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_SIZE, "0", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_ORIENTATION, "1", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);

    CloudMediaPhotosService service;
    CloudMediaPullDataDto data1;
    data1.basicFileName = "name1";
    data1.cloudId = "cloud_id1";
    data1.hasAttributes = false;
    data1.basicSize = 0;
    data1.basicFileType = 0;
    CloudMediaPullDataDto data2;
    data2.basicFileName = "name2";
    data2.cloudId = "cloud_id2";
    data2.hasAttributes = true;
    data2.basicSize = 0;
    data2.basicFileType = 0;
    CloudMediaPullDataDto data3;
    data3.basicFileName = "name3";
    data3.cloudId = "cloud_id3";
    data3.hasAttributes = true;
    data3.basicSize = 0;
    data3.basicFileType = 2;
    data3.propertiesRotate = 1;
    std::vector<CloudMediaPullDataDto> pullDatas = {data1, data2, data3};
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats(5);
    std::vector<std::string> failedRecords;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.PullRecordsConflictProc(pullDatas, refreshAlbums, stats, failedRecords, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::MERGE_RECORDS_COUNT], 0);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_NotifyPhotoInserted_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    ValuesBucket values1;
    values1.PutString(Media::PhotoColumn::PHOTO_CLOUD_ID, "id1");
    ValuesBucket values2;
    values2.PutInt(Media::PhotoColumn::PHOTO_CLOUD_ID, 1);
    ValuesBucket values3;
    std::vector<NativeRdb::ValuesBucket> insertFiles = {values1, values2, values3};

    service.NotifyPhotoInserted(insertFiles);
    EXPECT_EQ(insertFiles.size(), 3);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnCreateRecordSuccess_Test_001, TestSize.Level1)
{
    ValuesBucket values1;
    SetValuesBucketInPhotosTable(PhotoColumn::MEDIA_ID, "1", values1);
    SetValuesBucketInPhotosTable(PhotoColumn::PHOTO_DIRTY, "1", values1);
    InsertTable(g_rdbStore, PhotoColumn::PHOTOS_TABLE, values1);

    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = 0;
    record.fileId = 1;
    record.cloudId = "cloud_id1";
    record.version = 10;
    LocalInfo info;
    std::unordered_map<std::string, LocalInfo> localMap = {
        {"0", info},
    };
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, localMap, photoRefresh);
    EXPECT_EQ(ret, E_OK);

    InitTestTables(g_rdbStore);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnDentryFileInsert_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto dto1;
    CloudMediaPullDataDto dto2;
    std::vector<CloudMediaPullDataDto> pullDatas = {dto1, dto2};
    std::vector<std::string> failedRecords;

    int32_t ret = service.OnDentryFileInsert(pullDatas, failedRecords);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnRecordFailed_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.serverErrorCode = ServerErrorCode::NETWORK_ERROR;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_SYNC_FAILED_NETWORK_NOT_AVAILABLE);

    photo.serverErrorCode = ServerErrorCode::UID_EMPTY;
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_STOP);

    photo.serverErrorCode = ServerErrorCode::SWITCH_OFF;
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_STOP);

    photo.serverErrorCode = ServerErrorCode::INVALID_LOCK_PARAM;
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_STOP);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_INVAL_ARG);

    CloudErrorDetail detailError;

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    detailError.detailCode = ErrorDetailCode::SPACE_FULL;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_CLOUD_STORAGE_FULL);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    detailError.detailCode = ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_BUSINESS_MODE_CHANGED);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaPhotosService_OnRecordFailed_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto photo;
    int32_t ret;
    CloudErrorDetail detailError;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    detailError.detailCode = ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_DATA);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    detailError.detailCode = ErrorDetailCode::CONTENT_NOT_FIND;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_OK);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_UNKNOWN;
    detailError.detailCode = ErrorDetailCode::LACK_OF_PARAM;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_UNKNOWN);

    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_NOT_NEED_RETRY;
    detailError.detailCode = ErrorDetailCode::LACK_OF_PARAM;
    photo.errorDetails.clear();
    photo.errorDetails.emplace_back(detailError);
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_STOP);

    photo.errorDetails.clear();
    photo.serverErrorCode = ServerErrorCode::RESPONSE_TIME_OUT;
    photo.errorType = ErrorType::TYPE_NOT_NEED_RETRY;
    ret = service.OnRecordFailed(photo, photoRefresh);
    EXPECT_EQ(ret, E_STOP);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_FillMetadata_Test_001, TestSize.Level1)
{
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data = nullptr;
    int32_t ret = service.FillMetadata(data);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_FillMetadata_Test_002, TestSize.Level1)
{
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath("/Invalid/Path/filename.txt");
    int32_t ret = service.FillMetadata(data);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_ScanMetaData_Test_001, TestSize.Level1)
{
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = nullptr;
    std::string filePath = "/data/filename1.jpg";
    int32_t ret = service.ScanMetaData(filePath, data);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_ScanShootingMode_Test_001, TestSize.Level1)
{
    CloudMediaScanService service;
    CloudMediaScanService::ScanResult scanResult;
    std::string filePath = "/Invalid/Path/filename.txt";
    int32_t ret = service.ScanDownloadedFile(filePath, scanResult);
    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_ScanShootingMode_Test_002, TestSize.Level1)
{
    CloudMediaScanService service;
    CloudMediaScanService::ScanResult scanResult;
    std::string filePath = "/data/filename1.jpg";
    char buffer[] = "xxLIVE_123456789123456";
    int32_t fd = open(filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0x777);
    write(fd, buffer, strlen(buffer));
    close(fd);
    int32_t ret = service.ScanDownloadedFile(filePath, scanResult);
    EXPECT_EQ(ret, E_OK);
    system("rm -rf /data/filename1.jpg");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_UpdateAndNotifyShootingModeAlbumIfNeeded_Test_002,
    TestSize.Level1)
{
    CloudMediaScanService service;
    CloudMediaScanService::ScanResult scanResult;
    scanResult.shootingMode = "1";
    scanResult.frontCamera = "1";
    service.UpdateAndNotifyShootingModeAlbumIfNeeded(scanResult);
    EXPECT_EQ(scanResult.shootingMode, "1");
    EXPECT_EQ(scanResult.frontCamera, "1");
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_001, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_002, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.fileId = 1;
    photo.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_RIGHT);
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_RIGHT);
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_003, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.fileId = 1;
    photo.exifRotate = 0;
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    assetData.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_004, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.fileId = 1;
    photo.exifRotate = 0;
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::RIGHT_TOP);
    assetData.mediaType = MediaType::MEDIA_TYPE_VIDEO;
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_005, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.fileId = 1;
    photo.exifRotate = 0;
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_RIGHT);
    assetData.mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaDownloadService_FixDownloadAssetExifRotate_Test_006, TestSize.Level1)
{
    CloudMediaDownloadService service;
    ORM::PhotosPo photo;
    OnDownloadAssetData assetData;
    photo.fileId = 1;
    photo.exifRotate = 0;
    assetData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_RIGHT);
    assetData.mediaType = MediaType::MEDIA_TYPE_VIDEO;
    int32_t ret = service.FixDownloadAssetExifRotate(photo, assetData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_RemoveLocalFile_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto dto;
    dto.localPath = "";
    dto.attributesSubtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    dto.attributesMovingPhotoEffectMode = 0;
    dto.attributesOriginalSubtype = 0;
    int32_t ret = service.RemoveLocalFile(dto);
    EXPECT_NE(ret, E_OK); // 暂未配置能删除的资源，先预期not ok
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaScanService_ClearLocalData_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    CloudMediaPullDataDto dto;
    std::vector<PhotosDto> fdirtyData;
    dto.localPath = "";
    dto.localPosition = 1;
    dto.attributesSubtype = static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
    dto.attributesMovingPhotoEffectMode = 0;
    dto.attributesOriginalSubtype = 0;
    int32_t ret = service.ClearLocalData(dto, fdirtyData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaAlbumService_HandleLPathRecords_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    ChangeType changeType;
    OnFetchRecordsAlbumRespBody resp;
    record.isDelete = true;
    int32_t ret = service.HandleLPathRecords(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaAlbumService_PullInsert_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    ChangeType changeType;
    OnFetchRecordsAlbumRespBody resp;
    record.isDelete = true;
    int32_t ret = service.PullInsert(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaAlbumService_PullUpdate_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    ChangeType changeType;
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = service.PullUpdate(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTest, CloudMediaAlbumService_PullDelete_Test_001, TestSize.Level1)
{
    CloudMediaAlbumService service;
    PhotoAlbumDto record;
    ChangeType changeType;
    OnFetchRecordsAlbumRespBody resp;
    int32_t ret = service.PullDelete(record, changeType, resp);
    EXPECT_EQ(ret, E_OK);
}
}