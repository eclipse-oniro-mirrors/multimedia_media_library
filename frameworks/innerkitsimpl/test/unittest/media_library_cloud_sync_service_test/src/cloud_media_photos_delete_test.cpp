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

#include "cloud_media_photos_delete_test.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "media_file_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "media_albums_controller_service.h"
#include "photos_po.h"
#include "create_album_vo.h"
#include "asset_accurate_refresh.h"

#define private public
#define protected public
#include "photos_po_writer.h"
#include "photo_file_operation.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_photos_delete_service.h"
#undef private

using namespace std;
using namespace testing::ext;

namespace OHOS::Media::CloudSync {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    // add more phots ,audios if necessary
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
};

static int32_t CreateAlbum(const std::string &albumName)
{
    CreateAlbumReqBody reqBody;
    reqBody.albumName = albumName;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->CreatePhotoAlbum(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    return respVo.GetErrCode();
}

void CloudMediaPhotoDeleteTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    bool ret = MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ASSERT_TRUE(ret);
}

void CloudMediaPhotoDeleteTest::TearDownTestCase()
{
    system("rm -rf /storage/cloud/files/*");
    bool ret = MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    ASSERT_TRUE(ret);
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();

    MEDIA_INFO_LOG("CloudMediaPhotoDeleteTest is finish");
}

void CloudMediaPhotoDeleteTest::SetUp()
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
    bool ret = MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    ASSERT_TRUE(ret);
}

void CloudMediaPhotoDeleteTest::TearDown() {}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoFindAlbumUploadStatus, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudMediaPhotoFindAlbumStatus");
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    EXPECT_EQ(service->FindAlbumUploadStatus(pullData), false);

    PhotosPo photoPo;
    PhotoAlbumPo albumInfoOp;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;
    EXPECT_EQ(service->FindAlbumUploadStatus(pullData), false);

    albumInfoOp.uploadStatus = 1;
    pullData.albumInfoOp = albumInfoOp;
    EXPECT_EQ(service->FindAlbumUploadStatus(pullData), true);
    MEDIA_INFO_LOG("end CloudMediaPhotoFindAlbumStatus");
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoIsClearCloudInfo, TestSize.Level1)
{
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    EXPECT_EQ(service->IsClearCloudInfoOnly(pullData), false);

    PhotosPo photosPo;
    photosPo.hidden = 1;
    pullData.basicIsDelete = true;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    EXPECT_EQ(service->IsClearCloudInfoOnly(pullData), true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoFindPhotoAlbum, TestSize.Level1)
{
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    CloudMediaPullDataDto pullData;
    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_ERR);

    PhotosPo photoOp;
    pullData.localPhotosPoOp = photoOp;
    int32_t albumId = CreateAlbum("summer");
    ASSERT_GT(albumId, 0);
    pullData.localOwnerAlbumId = albumId;
    ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoPullClearCloudInfo, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudMediaPhotoPullClearCloudInfo");
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    CloudMediaPullDataDto pullData;
    pullData.cloudId = -1;
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service->PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);

    int32_t albumId = CreateAlbum("winter");
    ASSERT_GT(albumId, 0);
    pullData.cloudId = albumId;
    ret = service->PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoIsMoveOnlyCloudAssetIntoTrash, TestSize.Level1)
{
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    bool ret = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(ret, false);

    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 1;
    pullData.basicIsDelete = false;

    ret = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoIsMoveOutFromTrash, TestSize.Level1)
{
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    CloudMediaPullDataDto pullData;
    bool ret = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(ret, false);

    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.basicRecycledTime = 0;
    pullData.localPhotosPoOp = photoPo;
    ret = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoCopyAndMoveCloudAssetToTrash, TestSize.Level1)
{
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    int32_t ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);

    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;
    ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaCopyThumbnail, TestSize.Level1)
{
    auto photoOperation = make_shared<PhotoFileOperation>();
    ASSERT_NE(photoOperation, nullptr);
    PhotosPo sourcePhotosPo;
    PhotosPo targetPhotosPo;
    int32_t ret = photoOperation->CopyThumbnail(sourcePhotosPo, targetPhotosPo);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);

    sourcePhotosPo.data = "10001";
    sourcePhotosPo.dateTaken = 10001l;
    sourcePhotosPo.fileId = 1;
    targetPhotosPo.fileId = 2;
    targetPhotosPo.data = "10002";
    ret = photoOperation->CopyThumbnail(sourcePhotosPo, targetPhotosPo);
    EXPECT_EQ(ret, E_FAIL);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotosPoTest, TestSize.Level1)
{
    PhotosPo photoPo;
    string result = photoPo.ToString();
    std::regex pattern("\\{.*?\\}");
    EXPECT_EQ(std::regex_search(result, pattern), true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoServicesHandleRecordTest, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudMediaPhotoServicesHandleRecordTest");
    CloudMediaPhotosService services;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::vector<PhotosDto> newData;
    std::vector<std::string> cloudIds;
    std::vector<PhotosDto> fdirtyData;
    std::vector<std::string> failedRecords;
    std::map<std::string, CloudMediaPullDataDto> cloudIdRelativeMap;
    int32_t ret = services.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(ret, E_OK);

    cloudIds.emplace_back("001");
    CloudMediaPullDataDto dataDto;
    dataDto.basicIsDelete = false;
    cloudIdRelativeMap["001"] = dataDto;
    services.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(stats[0], 1);

    cloudIds.emplace_back("002");
    CloudMediaPullDataDto dataDto1;
    dataDto1.localPath = "/test";
    dataDto1.basicIsDelete = true;
    cloudIdRelativeMap["002"] = dataDto1;
    services.HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    EXPECT_EQ(stats[4], 1);
    MEDIA_INFO_LOG("end CloudMediaPhotoServicesHandleRecordTest");
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMedia_PhotosPo_BuildFileUri_Test, TestSize.Level1)
{
    PhotosPo photoPo;
    photoPo.data = "/storage/cloud/files/Photo/8/IMG_1761641838_408.jpg";
    photoPo.displayName = "IMG_20250912_225900.jpg";
    photoPo.fileId = 409;
    std::string fileUri = photoPo.BuildFileUri();
    // file://media/Photo/${file_id}/${data_prefix}/%{display_name}
    const std::string expectFileUri = "file://media/Photo/409/IMG_1761641838_408/IMG_20250912_225900.jpg";
    EXPECT_EQ(fileUri, expectFileUri);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CloudMediaPhotoUpdatePullDataLocalInfo, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudMediaPhotoUpdatePullDataLocalInfo");
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);

    targetPhotoInfoOp.reset();
    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);

    MEDIA_INFO_LOG("end CloudMediaPhotoUpdatePullDataLocalInfo");
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_HiddenPhoto, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖hidden=1分支（触发条件：localPhotosPoOp存在且hidden=1）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_NoAlbumInfo, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖albumInfoOp不存在分支（触发条件：localPhotosPoOp存在但albumInfoOp不存在）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_UploadStatusZero, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖uploadStatus=0分支（触发条件：非camera album且uploadStatus=0）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "TestAlbum";
    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_UploadStatusOne, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖uploadStatus=1分支（触发条件：非camera album且uploadStatus=1）；验证返回true业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "TestAlbum";
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumNowUploadStatus_UploadStatusDefault, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖uploadStatus默认值分支（触发条件：uploadStatus无值默认为0）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "TestAlbum";
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖localPhotosPoOp不存在分支（触发条件：localPhotosPoOp无值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.basicIsDelete = true;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_NotDelete, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖basicIsDelete=false分支（触发条件：basicIsDelete=false）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = false;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_InTrash, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖dateTrashed>0分支（触发条件：dateTrashed>0表示在回收站）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 1;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_NotLocalAndCloud, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖position!=LOCAL_AND_CLOUD分支（触发条件：position不是LOCAL_AND_CLOUD）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_UploadStatusTrue, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖FindAlbumUploadStatus返回true分支（触发条件：相册上传状态为true）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindPhotoAlbum_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试FindPhotoAlbum功能；覆盖localPhotosPoOp不存在分支（触发条件：localPhotosPoOp无值）；验证返回E_ERR业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindPhotoAlbum_InvalidAlbumId, TestSize.Level1)
{
    // 用例说明：测试FindPhotoAlbum功能；覆盖无效albumId分支（触发条件：localOwnerAlbumId无效导致查询失败）；验证返回E_ERR业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.sourcePath = "/test/path";
    pullData.localPhotosPoOp = photoPo;
    pullData.localOwnerAlbumId = -1;

    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindPhotoAlbum_Success, TestSize.Level1)
{
    // 用例说明：测试FindPhotoAlbum功能；覆盖成功查询分支（触发条件：albumId有效且相册存在）；验证返回E_OK和albumInfoOp有值业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    int32_t albumId = CreateAlbum("TestAlbum");
    ASSERT_GT(albumId, 0);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.sourcePath = "/test/path";
    pullData.localPhotosPoOp = photoPo;
    pullData.localOwnerAlbumId = albumId;

    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(pullData.albumInfoOp.has_value());
}

HWTEST_F(CloudMediaPhotoDeleteTest, PullClearCloudInfo_Success, TestSize.Level1)
{
    // 用例说明：测试PullClearCloudInfo功能；覆盖成功清除分支（触发条件：cloudId有效且清除成功）；验证返回E_OK和stats计数增加业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service->PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖localPhotosPoOp不存在分支（触发条件：localPhotosPoOp无值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_BasicIsDelete, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖basicIsDelete=true分支（触发条件：basicIsDelete=true表示从云端删除）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = true;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_NotLocalAndCloud, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖position!=LOCAL_AND_CLOUD分支，验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_NoRecycledTime, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖basicRecycledTime<=0分支（触发条件：basicRecycledTime<=0）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 0;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_AlreadyInTrash, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖dateTrashed>0分支（触发条件：dateTrashed>0表示已在回收站）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_UploadStatusTrue, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖FindAlbumUploadStatus返回true分支（触发条件：相册上传状态为true）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖localPhotosPoOp不存在分支（触发条件：localPhotosPoOp无值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_RecycledTimeNotZero, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖basicRecycledTime>0分支（触发条件：basicRecycledTime>0）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 1;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_DateTrashedZero, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖dateTrashed<=0分支（触发条件：dateTrashed<=0表示不在回收站）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CopyAndMoveCloudAssetToTrash_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试CopyAndMoveCloudAssetToTrash功能；覆盖localPhotosPoOp不存在分支，验证返回E_CLOUDSYNC_INVAL_ARG业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    int32_t ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CopyAndMoveCloudAssetToTrash_Success, TestSize.Level1)
{
    // 用例说明：测试CopyAndMoveCloudAssetToTrash功能；覆盖成功移动分支（触发条件：localPhotosPoOp存在且移动成功）；验证返回E_OK业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.fileId = 100;
    photoPo.data = "/test/path/image.jpg";
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, MoveOutTrashAndMergeWithSameAsset_NoLocalPhotos, TestSize.Level1)
{
    // 用例说明：测试MoveOutTrashAndMergeWithSameAsset功能；覆盖localPhotosPoOp不存在分支，验证返回E_CLOUDSYNC_INVAL_ARG业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    int32_t ret = service->MoveOutTrashAndMergeWithSameAsset(photoRefresh, pullData);
    EXPECT_EQ(ret, E_CLOUDSYNC_INVAL_ARG);
}

HWTEST_F(CloudMediaPhotoDeleteTest, MoveOutTrashAndMergeWithSameAsset_Success, TestSize.Level1)
{
    // 用例说明：测试MoveOutTrashAndMergeWithSameAsset功能；覆盖成功恢复分支（触发条件：localPhotosPoOp存在且恢复成功）；验证返回E_OK业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.fileId = 100;
    photoPo.data = "/test/path/image.jpg";
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->MoveOutTrashAndMergeWithSameAsset(photoRefresh, pullData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_NoTargetPhoto, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖targetPhotoInfoOp不存在分支（触发条件：targetPhotoInfoOp无值）；验证不更新pullData业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_NoFileId, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖fileId不存在分支（触发条件：targetPhotoInfo.fileId无值）；验证不更新pullData业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_NoData, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖data不存在分支（触发条件：targetPhotoInfo.data无值）；验证不更新pullData业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_NoPosition, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖position不存在分支（触发条件：targetPhotoInfo.position无值）；验证不更新pullData业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_SameFileId, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖fileId相同分支（触发条件：localFileId==targetPhotoInfo.fileId）；验证不更新pullData业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 100;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_AllFieldsUpdate, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖所有字段更新分支（触发条件：所有必需字段存在且fileId不同）；验证所有字段正确更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/storage/cloud/files/Photo/1/old_image.jpg";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/storage/cloud/files/Photo/2/new_image.jpg";
    targetPhotoInfo.position = 3;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/storage/cloud/files/Photo/2/new_image.jpg");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_ComplexScenario1, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖hidden=0且非camera album且uploadStatus=0组合分支，验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "Vacation";
    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_ComplexScenario2, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖hidden=0且非camera album且uploadStatus=1组合分支,验证返回true业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "Vacation";
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_ComplexScenario1, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖所有条件满足且FindAlbumUploadStatus返回false组合分支,验证返回true业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_ComplexScenario, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖所有条件满足且FindAlbumUploadStatus返回false组合分支,验证返回true业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1000000000;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_ComplexScenario, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖所有条件满足组合分支（触发条件：basicRecycledTime=0, dateTrashed>0）；验证返回true业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1000000000;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, EdgeCase_FindAlbumUploadStatus_HiddenEdge, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖hidden边界值分支（触发条件：hidden=2非0/1）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 2;
    pullData.localPhotosPoOp = photoPo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, EdgeCase_IsClearCloudInfoOnly_PositionEdge, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖position边界值分支（触发条件：position=3非标准值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 0;
    photosPo.position = 3;
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, EdgeCase_IsMoveOnlyCloudAssetIntoTrash_RecycledTimeEdge, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖basicRecycledTime边界值分支（触发条件：basicRecycledTime=-1负值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = -1;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, EdgeCase_UpdatePullDataLocalInfo_FileIdEdge, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖fileId边界值分支（触发条件：fileId=0或负值）；验证不更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 0;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 0;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 0);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, EdgeCase_UpdatePullDataLocalInfo_PositionEdge, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖position边界值分支（触发条件：position=0或负值）；验证正常更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 0;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 0);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_EmptyPathUpdate, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖空路径更新分支（触发条件：data为空字符串）；验证更新为空路径业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_MaxFileIdUpdate, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖最大fileId更新分支（触发条件：fileId=INT_MAX）；验证正常更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = INT32_MAX;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, INT32_MAX);
    EXPECT_EQ(pullData.localPosition, 2);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_MaxPositionUpdate, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖最大position更新分支（触发条件：position=INT_MAX）；验证正常更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = INT32_MAX;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, INT32_MAX);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_SpecialCharactersPath, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖特殊字符路径更新分支（触发条件：data包含特殊字符）；验证正常更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    std::string specialPath = "/storage/cloud/files/Photo/test@#$%^&*().jpg";
    targetPhotoInfo.data = specialPath;
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, specialPath);
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_MultipleCalls, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖多次调用状态一致性分支（触发条件：连续多次调用相同参数）；验证返回结果一致业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumName = "TestAlbum";
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result1 = service->FindAlbumUploadStatus(pullData);
    bool result2 = service->FindAlbumUploadStatus(pullData);
    bool result3 = service->FindAlbumUploadStatus(pullData);

    EXPECT_EQ(result1, true);
    EXPECT_EQ(result2, true);
    EXPECT_EQ(result3, true);
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_MultipleCalls, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖多次调用状态一致性分支（触发条件：连续多次调用相同参数）；验证返回结果一致业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 1;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result1 = service->IsClearCloudInfoOnly(pullData);
    bool result2 = service->IsClearCloudInfoOnly(pullData);
    bool result3 = service->IsClearCloudInfoOnly(pullData);

    EXPECT_EQ(result1, true);
    EXPECT_EQ(result2, true);
    EXPECT_EQ(result3, true);
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_MultipleCalls, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖多次调用状态一致性分支（触发条件：连续多次调用相同参数）；验证返回结果一致业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    bool result1 = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    bool result2 = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    bool result3 = service->IsMoveOnlyCloudAssetIntoTrash(pullData);

    EXPECT_EQ(result1, true);
    EXPECT_EQ(result2, true);
    EXPECT_EQ(result3, true);
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_MultipleCalls, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖多次调用状态一致性分支（触发条件：连续多次调用相同参数）；验证返回结果一致业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result1 = service->IsMoveOutFromTrash(pullData);
    bool result2 = service->IsMoveOutFromTrash(pullData);
    bool result3 = service->IsMoveOutFromTrash(pullData);

    EXPECT_EQ(result1, true);
    EXPECT_EQ(result2, true);
    EXPECT_EQ(result3, true);
    EXPECT_EQ(result1, result2);
    EXPECT_EQ(result2, result3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_MultipleCalls, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖多次调用状态一致性分支（触发条件：连续多次调用相同参数）；验证状态正确更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);

    std::optional<PhotosPo> targetPhotoInfoOp2;
    PhotosPo targetPhotoInfo2;
    targetPhotoInfo2.fileId = 300;
    targetPhotoInfo2.data = "/newer/path";
    targetPhotoInfo2.position = 3;
    targetPhotoInfoOp2 = targetPhotoInfo2;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp2);
    EXPECT_EQ(pullData.localPath, "/newer/path");
    EXPECT_EQ(pullData.localFileId, 300);
    EXPECT_EQ(pullData.localPosition, 3);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_ChangingHidden, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖hidden值动态变化分支（触发条件：先hidden=0后hidden=1）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result1 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result1, true);

    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;
    bool result2 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_ChangingUploadStatus, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖uploadStatus值动态变化分支（触发条件：先uploadStatus=1后uploadStatus=0）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result1 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result1, true);

    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;
    bool result2 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_ChangingBasicIsDelete, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖basicIsDelete值动态变化分支（触发条件：先true后false）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 1;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result1 = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result1, true);

    pullData.basicIsDelete = false;
    bool result2 = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_ChangingDateTrashed, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖dateTrashed值动态变化分支（触发条件：先0后>0）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 1;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result1 = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result1, true);

    photosPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photosPo;
    bool result2 = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_ChangingBasicRecycledTime, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖basicRecycledTime值动态变化分支（触发条件：先>0后=0）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    bool result1 = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result1, true);

    pullData.basicRecycledTime = 0;
    bool result2 = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_ChangingDateTrashed, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖dateTrashed值动态变化分支（触发条件：先>0后=0）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result1 = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result1, true);

    photoPo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoPo;
    bool result2 = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_ChangingBasicRecycledTime, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖basicRecycledTime值动态变化分支（触发条件：先=0后>0）；验证返回结果相应变化业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result1 = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result1, true);

    pullData.basicRecycledTime = 1;
    bool result2 = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_ChangingFileId, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖fileId值动态变化分支（触发条件：连续更新不同fileId）；验证状态正确更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localFileId, 200);

    targetPhotoInfo.fileId = 300;
    targetPhotoInfoOp = targetPhotoInfo;
    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localFileId, 300);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_ChangingPath, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖path值动态变化分支（触发条件：连续更新不同path）；验证状态正确更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");

    targetPhotoInfo.data = "/newer/path";
    targetPhotoInfoOp = targetPhotoInfo;
    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_ChangingPosition, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖position值动态变化分支（触发条件：连续更新不同position）；验证状态正确更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPosition, 2);

    targetPhotoInfo.position = 3;
    targetPhotoInfoOp = targetPhotoInfo;
    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPosition, 2);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_AllConditionsFalse, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖所有条件为false组合分支,验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 0;
    photosPo.dateTrashed = 1;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = false;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_AllConditionsFalse, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖所有条件为false组合分支,验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = true;
    pullData.basicRecycledTime = 0;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_AllConditionsFalse, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖所有条件为false组合分支（触发条件：basicRecycledTime>0, dateTrashed=0）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 1;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_PartialUpdate, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖部分字段更新分支（触发条件：只有部分字段有值）；验证只更新有值的字段业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖默认值处理分支（触发条件：hidden和uploadStatus都无值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖默认值处理分支（触发条件：所有字段都为默认值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = false;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖默认值处理分支（触发条件：所有字段都为默认值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = -1;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖默认值处理分支（触发条件：所有字段都为默认值）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = -1;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖默认值处理分支（触发条件：targetPhotoInfo所有字段都无值）；验证不更新业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);

    EXPECT_EQ(pullData.localPath, "/old/path");
    EXPECT_EQ(pullData.localFileId, 100);
    EXPECT_EQ(pullData.localPosition, 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, PullClearCloudInfo_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试PullClearCloudInfo功能；覆盖默认值处理分支（触发条件：cloudId为空字符串）；验证返回E_OK业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    pullData.cloudId = "";
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service->PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CopyAndMoveCloudAssetToTrash_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试CopyAndMoveCloudAssetToTrash功能；覆盖默认值处理分支（触发条件：localPhotosPoOp存在但所有字段为默认值）；验证返回E_OK业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, MoveOutTrashAndMergeWithSameAsset_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试MoveOutTrashAndMergeWithSameAsset功能；覆盖默认值处理分支（触发条件：localPhotosPoOp存在但所有字段为默认值）；验证返回E_OK业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->MoveOutTrashAndMergeWithSameAsset(photoRefresh, pullData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindPhotoAlbum_DefaultValues, TestSize.Level1)
{
    // 用例说明：测试FindPhotoAlbum功能；覆盖默认值处理分支（触发条件：localPhotosPoOp存在但sourcePath为空）；验证返回E_ERR业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    pullData.localPhotosPoOp = photoPo;
    pullData.localOwnerAlbumId = 1;

    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_UserAlbumTypes, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖不同user album类型分支（触发条件：albumType为不同USER类型值）；验证返回结果基于uploadStatus业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = 2;
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result1 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result1, true);

    albumInfo.albumType = 3;
    albumInfo.uploadStatus = 0;
    pullData.albumInfoOp = albumInfo;
    bool result2 = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_DifferentPositions, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；；覆盖不同position值分支（触发条件：position为CLOUD_ONLY或LOCAL_ONLY）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData1;
    PhotosPo photosPo1;
    photosPo1.hidden = 0;
    photosPo1.dateTrashed = 0;
    photosPo1.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    pullData1.localPhotosPoOp = photosPo1;
    pullData1.basicIsDelete = true;

    bool result1 = service->IsClearCloudInfoOnly(pullData1);
    EXPECT_EQ(result1, false);

    CloudMediaPullDataDto pullData2;
    PhotosPo photosPo2;
    photosPo2.hidden = 0;
    photosPo2.dateTrashed = 0;
    photosPo2.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData2.localPhotosPoOp = photosPo2;
    pullData2.basicIsDelete = true;

    bool result2 = service->IsClearCloudInfoOnly(pullData2);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_DifferentPositions, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖不同position值分支（触发条件：position为CLOUD_ONLY或LOCAL_ONLY）；验证返回false业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);

    CloudMediaPullDataDto pullData1;
    PhotosPo photoPo1;
    photoPo1.dateTrashed = 0;
    photoPo1.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    pullData1.localPhotosPoOp = photoPo1;
    pullData1.basicIsDelete = false;
    pullData1.basicRecycledTime = 1;

    bool result1 = service->IsMoveOnlyCloudAssetIntoTrash(pullData1);
    EXPECT_EQ(result1, false);

    CloudMediaPullDataDto pullData2;
    PhotosPo photoPo2;
    photoPo2.dateTrashed = 0;
    photoPo2.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    pullData2.localPhotosPoOp = photoPo2;
    pullData2.basicIsDelete = false;
    pullData2.basicRecycledTime = 1;

    bool result2 = service->IsMoveOnlyCloudAssetIntoTrash(pullData2);
    EXPECT_EQ(result2, false);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindAlbumUploadStatus_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试FindAlbumUploadStatus功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.hidden = 0;
    pullData.localPhotosPoOp = photoPo;

    PhotoAlbumPo albumInfo;
    albumInfo.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.uploadStatus = 1;
    pullData.albumInfoOp = albumInfo;

    bool result = service->FindAlbumUploadStatus(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsClearCloudInfoOnly_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试IsClearCloudInfoOnly功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photosPo;
    photosPo.hidden = 1;
    photosPo.dateTrashed = 0;
    photosPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    pullData.localPhotosPoOp = photosPo;
    pullData.basicIsDelete = true;

    bool result = service->IsClearCloudInfoOnly(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOnlyCloudAssetIntoTrash_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试IsMoveOnlyCloudAssetIntoTrash功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 0;
    photoPo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photoPo.hidden = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicIsDelete = false;
    pullData.basicRecycledTime = 1;

    bool result = service->IsMoveOnlyCloudAssetIntoTrash(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, IsMoveOutFromTrash_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试IsMoveOutFromTrash功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.dateTrashed = 1;
    pullData.localPhotosPoOp = photoPo;
    pullData.basicRecycledTime = 0;

    bool result = service->IsMoveOutFromTrash(pullData);
    EXPECT_EQ(result, true);
}

HWTEST_F(CloudMediaPhotoDeleteTest, UpdatePullDataLocalInfo_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试UpdatePullDataLocalInfo功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    pullData.localPath = "/old/path";
    pullData.localFileId = 100;
    pullData.localPosition = 1;

    std::optional<PhotosPo> targetPhotoInfoOp;
    PhotosPo targetPhotoInfo;
    targetPhotoInfo.fileId = 200;
    targetPhotoInfo.data = "/new/path";
    targetPhotoInfo.position = 2;
    targetPhotoInfoOp = targetPhotoInfo;

    service->UpdatePullDataLocalInfo(pullData, targetPhotoInfoOp);
    EXPECT_EQ(pullData.localPath, "/new/path");
    EXPECT_EQ(pullData.localFileId, 200);
    EXPECT_EQ(pullData.localPosition, 2);
}

HWTEST_F(CloudMediaPhotoDeleteTest, PullClearCloudInfo_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试PullClearCloudInfo功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    pullData.cloudId = "test_cloud_id";
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service->PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(stats[StatsIndex::DELETE_RECORDS_COUNT], 1);
}

HWTEST_F(CloudMediaPhotoDeleteTest, CopyAndMoveCloudAssetToTrash_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试CopyAndMoveCloudAssetToTrash功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.fileId = 100;
    photoPo.data = "/test/path/image.jpg";
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, MoveOutTrashAndMergeWithSameAsset_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试MoveOutTrashAndMergeWithSameAsset功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto photoRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>();
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.fileId = 100;
    photoPo.data = "/test/path/image.jpg";
    pullData.localPhotosPoOp = photoPo;

    int32_t ret = service->MoveOutTrashAndMergeWithSameAsset(photoRefresh, pullData);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaPhotoDeleteTest, FindPhotoAlbum_NullptrCheck, TestSize.Level1)
{
    // 用例说明：测试FindPhotoAlbum功能；覆盖空指针检查分支（触发条件：service对象有效）；验证不崩溃业务断言
    auto service = make_shared<CloudMediaPhotosDeleteService>();
    ASSERT_NE(service, nullptr);
    ASSERT_NE(service.get(), nullptr);

    int32_t albumId = CreateAlbum("TestAlbum");
    ASSERT_GT(albumId, 0);

    CloudMediaPullDataDto pullData;
    PhotosPo photoPo;
    photoPo.sourcePath = "/test/path";
    pullData.localPhotosPoOp = photoPo;
    pullData.localOwnerAlbumId = albumId;

    int32_t ret = service->FindPhotoAlbum(pullData);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(pullData.albumInfoOp.has_value());
}
}  // namespace OHOS::Media::CloudSync
