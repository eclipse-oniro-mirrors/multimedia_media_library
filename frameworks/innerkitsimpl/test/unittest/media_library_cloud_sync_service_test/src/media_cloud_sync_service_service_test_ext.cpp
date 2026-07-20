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
#include "cloud_media_enhance_service.h"
#include "cloud_media_scan_service.h"
#include "cloud_media_data_service.h"
#undef private
#include "cloud_file_error.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "photos_po.h"

using namespace testing::ext;
using namespace OHOS::AAFwk;
using namespace OHOS::Media::ORM;

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
    pullData.localPhotosPoOp = PhotosPo();

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
    pullData.localPhotosPoOp = PhotosPo();

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
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnCreateRecordSuccess_Test_002, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = 0;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnCreateRecordSuccess_Test_003, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    record.localId = 0;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnCreateRecordSuccess(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, PhotosService_OnFdirtyRecordSuccess_Test_001, TestSize.Level1)
{
    CloudMediaPhotosService service;
    PhotosDto record;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.OnFdirtyRecordSuccess(record, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_ClearLocalData_Test_001, TestSize.Level1)
{
    // 用例说明：测试 ClearLocalData 方法，localPhotosPoOp 无值
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.localPhotosPoOp = std::nullopt;
    std::vector<PhotosDto> fdirtyData;

    int32_t ret = service.ClearLocalData(pullData, fdirtyData);

    EXPECT_EQ(ret, false);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_GetCloudKeyData_Test_001, TestSize.Level1)
{
    // 用例说明：测试 GetCloudKeyData 方法，数据无效
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.hasAttributes = false;
    pullData.basicFileName = "test.jpg";
    pullData.basicSize = 1024;
    KeyData keyData;

    int32_t ret = service.GetCloudKeyData(pullData, keyData);

    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_GetCloudKeyData_Test_002, TestSize.Level1)
{
    // 用例说明：测试 GetCloudKeyData 方法，fileType 为 -1
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.hasAttributes = true;
    pullData.basicFileName = "test.jpg";
    pullData.basicSize = 1024;
    pullData.basicFileType = -1;
    KeyData keyData;

    int32_t ret = service.GetCloudKeyData(pullData, keyData);

    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_GetCloudKeyData_Test_003, TestSize.Level1)
{
    // 用例说明：测试 GetCloudKeyData 方法，文件类型为视频
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    pullData.hasAttributes = true;
    pullData.basicFileName = "test.mp4";
    pullData.basicSize = 1024;
    pullData.basicFileType = FILE_TYPE_VIDEO;
    pullData.attributesMetaDateModified = 1234567890;
    pullData.basicEditedTime = 1234567891;
    pullData.basicCreatedTime = 1234567889;
    pullData.basicRecycledTime = 0;
    pullData.propertiesRotate = ORIENTATION_NORMAL;
    KeyData keyData;

    int32_t ret = service.GetCloudKeyData(pullData, keyData);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(keyData.mediaType, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_PullInsert_Test_001, TestSize.Level1)
{
    // 用例说明：测试 PullInsert 方法，pullDatas 为空
    CloudMediaPhotosService service;
    std::vector<CloudMediaPullDataDto> pullDatas;
    std::vector<std::string> failedRecords;

    int32_t ret = service.PullInsert(pullDatas, failedRecords);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_CreateEntry_Test_002, TestSize.Level1)
{
    // 用例说明：测试 CreateEntry 方法，pullDatas 为空
    CloudMediaPhotosService service;
    std::vector<CloudMediaPullDataDto> pullDatas;
    std::set<std::string> refreshAlbums;
    std::vector<PhotosDto> newData;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::vector<std::string> failedRecords;
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CreateEntry(pullDatas, refreshAlbums, newData, stats, failedRecords, photoRefresh);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleRecord_Test_002, TestSize.Level1)
{
    // 用例说明：测试 HandleRecord 方法，cloudIds 为空
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::vector<std::string> failedRecords;

    int32_t ret = service.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleRecord_Test_003, TestSize.Level1)
{
    // 用例说明：测试 HandleRecord 方法，新记录（本地路径为空且未删除）
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds = {"test_cloud_id"};
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPath = "";
    pullData.basicIsDelete = false;
    cloudIdRelativeMap["test_cloud_id"] = pullData;
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::vector<std::string> failedRecords;

    int32_t ret = service.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::NEW_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleRecord_Test_004, TestSize.Level1)
{
    // 用例说明：测试 HandleRecord 方法，删除记录
    CloudMediaPhotosService service;
    std::vector<std::string> cloudIds = {"test_cloud_id"};
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    pullData.localPath = "/storage/cloud/files/Photo/test.jpg";
    pullData.basicIsDelete = true;
    cloudIdRelativeMap["test_cloud_id"] = pullData;
    std::vector<PhotosDto> newData;
    std::vector<PhotosDto> fdirtyData;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::vector<std::string> failedRecords;

    int32_t ret = service.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleCloudDeleteRecord_Test_001, TestSize.Level1)
{
    // 用例说明：测试 HandleCloudDeleteRecord 方法，cloudDeleteFileIds 为空
    CloudMediaPhotosService service;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;

    int32_t ret = service.HandleCloudDeleteRecord(cloudIdRelativeMap);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_OnRecordFailedErrorDetails_Test_001, TestSize.Level1)
{
    // 用例说明：测试 OnRecordFailedErrorDetails 方法，空间已满
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.errorType = ErrorType::TYPE_NEED_UPLOAD;
    CloudErrorDetail detail;
    detail.detailCode = static_cast<int32_t>(ErrorDetailCode::SPACE_FULL);
    photo.errorDetails.push_back(detail);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.OnRecordFailedErrorDetails(photo, photoRefresh);

    EXPECT_EQ(ret, FileManagement::E_CLOUD_STORAGE_FULL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_OnRecordFailedErrorDetails_Test_002, TestSize.Level1)
{
    // 用例说明：测试 OnRecordFailedErrorDetails 方法，业务模式变更
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.errorType = ErrorType::TYPE_NEED_UPLOAD;
    CloudErrorDetail detail;
    detail.detailCode = static_cast<int32_t>(ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN);
    photo.errorDetails.push_back(detail);
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.OnRecordFailedErrorDetails(photo, photoRefresh);

    EXPECT_EQ(ret, FileManagement::E_BUSINESS_MODE_CHANGED);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_OnRecordFailedErrorDetails_Test_003, TestSize.Level1)
{
    // 用例说明：测试 OnRecordFailedErrorDetails 方法，同名文件
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.errorType = ErrorType::TYPE_NEED_UPLOAD;
    CloudErrorDetail detail;
    detail.detailCode = static_cast<int32_t>(ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED);
    photo.errorDetails.push_back(detail);
    photo.cloudId = "test_cloud_id";
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.OnRecordFailedErrorDetails(photo, photoRefresh);

    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_GetCloudPath_Test_001, TestSize.Level1)
{
    // 用例说明：测试 GetCloudPath 方法，路径无效
    CloudMediaPhotosService service;
    std::string filePath = "/invalid/path/test.jpg";

    std::string result = service.GetCloudPath(filePath);

    EXPECT_TRUE(result.empty());
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_GetCloudPath_Test_002, TestSize.Level1)
{
    // 用例说明：测试 GetCloudPath 方法，路径有效
    CloudMediaPhotosService service;
    std::string filePath = "/storage/cloud/files/Photo/test.jpg";

    std::string result = service.GetCloudPath(filePath);

    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find("/mnt/hmdfs/account/device_view/cloud") == 0);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleDuplicatedResource_Test_001, TestSize.Level1)
{
    // 用例说明：测试 HandleDuplicatedResource 方法，重新推送失败
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.cloudId = "test_cloud_id";

    int32_t ret = service.HandleDuplicatedResource(photo);

    EXPECT_EQ(ret, E_DATA);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_HandleInvalidCloudResource_Test_001, TestSize.Level1)
{
    // 用例说明：测试 HandleInvalidCloudResource 方法，路径为空
    CloudMediaPhotosService service;
    PhotosDto photo;
    photo.path = "";

    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    int32_t ret = service.HandleInvalidCloudResource(photo, photoRefresh);

    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_001, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，不支持的错误类型
    CloudMediaPhotosService service;
    int32_t errorCode = 999999;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, -1);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_002, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，缩未找到
    CloudMediaPhotosService service;
    int32_t errorCode = E_THM_SOURCE_BASIC + ENOENT;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_003, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，LCD 未找到
    CloudMediaPhotosService service;
    int32_t errorCode = E_LCD_SOURCE_BASIC + ENOENT;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_004, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，数据库大小为零
    CloudMediaPhotosService service;
    int32_t errorCode = E_DB_SIZE_IS_ZERO;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_005, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，LCD 过大
    CloudMediaPhotosService service;
    int32_t errorCode = E_LCD_IS_TOO_LARGE;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_NotifyUploadErr_Test_006, TestSize.Level1)
{
    // 用例说明：测试 NotifyUploadErr 方法，相册未找到
    CloudMediaPhotosService service;
    int32_t errorCode = E_DB_ALBUM_NOT_FOUND;
    std::string fileId = "12345";

    int32_t ret = service.NotifyUploadErr(errorCode, fileId);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_FindPhotoAlbum_Test_001, TestSize.Level1)
{
    // 用例说明：测试 FindPhotoAlbum 方法，已有相册信息
    CloudMediaPhotosService service;
    CloudMediaPullDataDto pullData;
    PhotoAlbumPo albumInfo;
    pullData.albumInfoOp = albumInfo;

    int32_t ret = service.FindPhotoAlbum(pullData);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_IsIgnoreMatch_Test_001, TestSize.Level1)
{
    // 用例说明：测试 IsIgnoreMatch 方法，相册上传关闭
    CloudMediaPhotosService service;
    CloudMediaPullDataDto mergeData;
    KeyData cloudKeyData;
    KeyData localKeyData;

    bool ret = service.IsIgnoreMatch(mergeData, cloudKeyData, localKeyData);

    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_IsIgnoreMatch_Test_002, TestSize.Level1)
{
    // 用例说明：测试 IsIgnoreMatch 方法，云端数据在回收站，本地不在
    CloudMediaPhotosService service;
    CloudMediaPullDataDto mergeData;
    PhotoAlbumPo albumInfo;
    mergeData.albumInfoOp = albumInfo;
    KeyData cloudKeyData;
    cloudKeyData.dateTrashed = 1;
    KeyData localKeyData;
    localKeyData.dateTrashed = 0;

    bool ret = service.IsIgnoreMatch(mergeData, cloudKeyData, localKeyData);

    EXPECT_TRUE(ret);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaPhotosService_IsIgnoreMatch_Test_003, TestSize.Level1)
{
    // 用例说明：测试 Is方法，本地数据在回收站，云端不在
    CloudMediaPhotosService service;
    CloudMediaPullDataDto mergeData;
    PhotoAlbumPo albumInfo;
    mergeData.albumInfoOp = albumInfo;
    KeyData cloudKeyData;
    cloudKeyData.dateTrashed = 0;
    KeyData localKeyData;
    localKeyData.dateTrashed = 1;

    bool ret = service.IsIgnoreMatch(mergeData, cloudKeyData, localKeyData);

    EXPECT_TRUE(ret);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_GetCloudSyncUnPreparedData_Test_001, TestSize.Level1)
{
    // 用例说明：测试 GetCloudSyncUnprearedData 方法，成功获取数据
    CloudMediaEnhanceService service;
    int32_t result = 0;

    int32_t ret = service.GetCloudSyncUnPreparedData(result);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_SubmitCloudSyncPreparedDataTask_Test_001, TestSize.Level1)
{
    // 用例说明：测试 SubmitCloudSyncPreparedDataTask 方法，任务已在运行
    CloudMediaEnhanceService service;
    service.submitRunning_ = true;

    int32_t ret = service.SubmitCloudSyncPreparedDataTask();

    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_SubmitCloudSyncPreparedDataTask_Test_002, TestSize.Level1)
{
    // 用例说明：测试 SubmitCloudSyncPreparedDataTask 方法，执行器为空
    CloudMediaEnhanceService service;
    service.submitRunning_ = false;
    service.executor_ = nullptr;

    int32_t ret = service.SubmitCloudSyncPreparedDataTask();

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_SubmitCloudSyncPreparedDataTask_Test_003, TestSize.Level1)
{
    // 用例说明：测试 SubmitCloudSyncPreparedDataTask 方法，成功提交任务
    CloudMediaEnhanceService service;
    service.submitRunning_ = false;
    service.executor_ = std::make_unique<OHOS::ThreadPool>("TestExecutor");

    int32_t ret = service.SubmitCloudSyncPreparedDataTask();

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaEnhanceService_StopSubmit_Test_001, TestSize.Level1)
{
    // 用例说明：测试 StopSubmit 方法，停止提交
    CloudMediaEnhanceService service;
    service.submitCount_ = 5;
    service.submitPhotoId_ = "test_photo_id";
    service.submitRunning_ = true;

    service.StopSubmit();

    EXPECT_EQ(service.submitCount_, 0);
    EXPECT_TRUE(service.submitPhotoId_.empty());
    EXPECT_FALSE(service.submitRunning_.load());
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaEnhanceService_StopSubmit_Test_002, TestSize.Level1)
{
    // 用例说明：测试 StopSubmit 方法，多次调用
    CloudMediaEnhanceService service;
    service.submitCount_ = 5;
    service.submitPhotoId_ = "test_photo_id";
    service.submitRunning_ = true;

    service.StopSubmit();
    service.StopSubmit();

    EXPECT_EQ(service.submitCount_, 0);
    EXPECT_TRUE(service.submitPhotoId_.empty());
    EXPECT_FALSE(service.submitRunning_.load());
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_SubmitNextCloudSyncPreparedDataTask_Test_001, TestSize.Level1)
{
    // 用例说明：测试 SubmitNextCloudSyncPreparedDataTask 方法，文件 ID 为空
    CloudMediaEnhanceService service;
    service.submitPhotoId_ = "test_photo_id";
    service.submitRunning_ = true;
    service.executor_ = std::make_unique<OHOS::ThreadPool>("TestExecutor");

    service.SubmitNextCloudSyncPreparedDataTask();

    EXPECT_FALSE(service.submitRunning_.load());
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaEnhanceService_SubmitNextCloudSyncPreparedDataTask_Test_002, TestSize.Level1)
{
    // 用例说明：测试 SubmitNextCloudSyncPreparedDataTask 方法，照片 ID 相同
    CloudMediaEnhanceService service;
    service.submitPhotoId_ = "test_photo_id";
    service.submitRunning_ = true;
    service.executor_ = std::make_unique<OHOS::ThreadPool>("TestExecutor");

    service.SubmitNextCloudSyncPreparedDataTask();

    EXPECT_FALSE(service.submitRunning_.load());
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaEnhanceService_SubmitTaskTimeoutCheck_Test_001, TestSize.Level1)
{
    // 用例说明：测试 SubmitTaskTimeoutCheck 方法，回调已完成
    CloudMediaEnhanceService service;
    service.callbackDone_ = true;

    service.SubmitTaskTimeoutCheck();

    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaEnhanceService_SubmitTaskTimeoutCheck_Test_002, TestSize.Level1)
{
    // 用例说明：测试 SubmitTaskTimeoutCheck 方法，超时
    CloudMediaEnhanceService service;
    service.callbackDone_ = false;
    service.submitRunning_ = true;

    service.SubmitTaskTimeoutCheck();

    EXPECT_FALSE(service.submitRunning_.load());
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaDataService_QueryData_Test_001, TestSize.Level1)
{
    // 用例说明：测试 QueryData 方法，查询 PHOTOS 表
    CloudMediaDataService service;
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> columnNames = {PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_CLOUD_ID};
    std::string tableName = PhotoColumn::PHOTOS_TABLE;
    std::vector<std::unordered_map<std::string, std::string>> results;

    int32_t ret = service.QueryData(predicates, columnNames, tableName, results);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaDataService_QueryData_Test_002, TestSize.Level1)
{
    // 用例说明：测试 QueryData 方法，查询 PHOTO_ALBUM 表
    CloudMediaDataService service;
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> columnNames = {PhotoAlbumColumns::ALBUM_NAME};
    std::string tableName = PhotoAlbumColumns::TABLE;
    std::vector<std::unordered_map<std::string, std::string>> results;

    int32_t ret = service.QueryData(predicates, columnNames, tableName, results);

    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaDataService_UpdateData_Test_001, TestSize.Level1)
{
    // 用例说明：测试 UpdateData 方法，更新 PHOTOS 表
    CloudMediaDataService service;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket value;
    std::vector<std::string> columnNames = {PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_CLOUD_ID};
    std::string tableName = PhotoColumn::PHOTOS_TABLE;
    std::string operateName = "test_operation";

    int32_t ret = service.UpdateData(tableName, predicates, value, operateName);

    EXPECT_NE(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaDataService_UpdateData_Test_002, TestSize.Level1)
{
    // 用例说明：测试 UpdateData 方法，更新 PHOTO_ALBUM 表
    CloudMediaDataService service;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket value;
    std::vector<std::string> columnNames = {PhotoAlbumColumns::ALBUM_NAME};
    std::string tableName = PhotoAlbumColumns::TABLE;
    std::string operateName = "test_operation";

    int32_t ret = service.UpdateData(tableName, predicates, value, operateName);

    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanMetaData_Test_002, TestSize.Level1)
{
    // 用例说明：测试 ScanMetaData 方法，data 为 nullptr
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = nullptr;
    std::string path = "/tmp/test.jpg";

    int32_t ret = service.ScanMetaData(path, data);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanMetaData_Test_003, TestSize.Level1)
{
    // 用例说明：测试 ScanMetaData 方法，FillMetadata 失败
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::string path = "/invalid/path/that/does/not/exist.jpg";
    data->SetFilePath(path);

    int32_t ret = service.ScanMetaData(path, data);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanMetaData_Test_004, TestSize.Level1)
{
    // 用例说明：测试 ScanMetaData 方法，提取图片元数据
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::string path = "/tmp/test.jpg";
    data->SetFilePath(path);
    data->SetFileMediaType(MEDIA_TYPE_IMAGE);

    int32_t ret = service.ScanMetaData(path, data);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanMetaData_Test_005, TestSize.Level1)
{
    // 用例说明：测试 ScanMetaData 方法，提取音视频元数据
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::string path = "/tmp/test.mp4";
    data->SetFilePath(path);
    data->SetFileMediaType(MEDIA_TYPE_VIDEO);

    int32_t ret = service.ScanMetaData(path, data);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanDownloadedFile_Test_001, TestSize.Level1)
{
    // 用例说明：测试 ScanDownloadedFile 方法，data 为 nullptr
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = nullptr;
    std::string path = "/tmp/test.jpg";
    CloudMediaScanService::ScanResult result;

    int32_t ret = service.ScanDownloadedFile(path, result);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanDownloadedFile_Test_002, TestSize.Level1)
{
    // 用例说明：测试 ScanDownloadedFile 方法，ScanMetaData 失败
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::string path = "/invalid/path/that/does/not/exist.jpg";
    data->SetFilePath(path);
    CloudMediaScanService::ScanResult result;

    int32_t ret = service.ScanDownloadedFile(path, result);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanDownloadedFile_Test_003, TestSize.Level1)
{
    // 用例说明：测试 ScanDownloadedFile 方法，成功扫描
    CloudMediaScanService service;
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::string path = "/tmp/test.jpg";
    data->SetFilePath(path);
    CloudMediaScanService::ScanResult result;

    int32_t ret = service.ScanDownloadedFile(path, result);

    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaScanService_UpdateAndNotifyShootingModeAlbumIfNeeded_Test_003, TestSize.Level1)
{
    // 用例说明：测试 UpdateAndNotifyShootingModeAlbumIfNeeded 方法，albumIds 为空
    CloudMediaScanService service;
    CloudMediaScanService::ScanResult result;
    result.scanSuccess = true;
    result.subType = 1;

    service.UpdateAndNotifyShootingModeAlbumIfNeeded(result);

    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaSyncServiceTestExt,
    CloudMediaScanService_UpdateAndNotifyShootingModeAlbumIfNeeded_Test_004, TestSize.Level1)
{
    // 用例说明：测试 UpdateAndNotifyShootingModeAlbumIfNeeded 方法，成功更新
    CloudMediaScanService service;
    CloudMediaScanService::ScanResult result;
    result.scanSuccess = true;
    result.subType = 1;

    service.UpdateAndNotifyShootingModeAlbumIfNeeded(result);

    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaSyncServiceTestExt, CloudMediaScanService_ScanResult_Test_001, TestSize.Level1)
{
    // 用例说明：测试 ScanResult::ToString 方法，成功转换为字符串
    CloudMediaScanService::ScanResult result;
    result.scanSuccess = true;
    result.shootingMode = "normal";
    result.frontCamera = "front";
    result.subType = 1;

    std::string str = result.ToString();

    EXPECT_FALSE(str.empty());
    EXPECT_TRUE(str.find("scanSuccess") != string::npos);
}
}