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

#include "media_cloud_sync_service_vo_test.h"

#include "media_log.h"

#include "media_itypes_utils.h"
#include "cloud_error_detail_vo.h"
#include "cloud_file_data_vo.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "failed_size_resp_vo.h"
#include "get_aging_file_vo.h"
#include "get_check_records_vo.h"
#include "get_cloud_thm_stat_vo.h"
#include "get_dirty_type_stat_vo.h"
#include "get_download_asset_vo.h"
#include "get_download_thm_by_uri_vo.h"
#include "get_download_thm_num_vo.h"
#include "get_download_thm_vo.h"
#include "get_file_pos_stat_vo.h"
#include "get_retey_records_vo.h"
#include "get_video_to_cache_vo.h"
#include "media_operate_result_vo.h"
#include "on_copy_records_photos_vo.h"
#include "on_create_records_album_vo.h"
#include "on_create_records_photos_vo.h"
#include "on_delete_albums_vo.h"
#include "on_delete_records_album_vo.h"
#include "on_delete_records_photos_vo.h"
#include "on_dentry_file_vo.h"
#include "on_download_asset_vo.h"
#include "on_download_thms_vo.h"
#include "on_fetch_photos_vo.h"
#include "on_fetch_records_album_vo.h"
#include "on_fetch_records_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "on_modify_file_dirty_vo.h"
#include "on_modify_records_photos_vo.h"
#include "photo_album_vo.h"
#include "photos_vo.h"
#include "update_dirty_vo.h"
#include "update_local_file_dirty_vo.h"
#include "update_position_vo.h"
#include "update_sync_status_vo.h"
#include "update_thm_status_vo.h"
#define protected public
#define private public
#include "cloud_mdkrecord_photos_vo.h"
#undef protected
#undef private

namespace OHOS::Media::CloudSync {
using namespace testing::ext;
using namespace OHOS::NativeRdb;

void CloudMediaSyncServiceVoTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaSyncServiceVoTest::SetUpTestCase");
}

void CloudMediaSyncServiceVoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaSyncServiceVoTest::TearDownTestCase");
}

// SetUp:Execute before each test case
void CloudMediaSyncServiceVoTest::SetUp()
{
    MEDIA_INFO_LOG("setup");
}

void CloudMediaSyncServiceVoTest::TearDown() {}

void FillContainersPart2(std::vector<std::shared_ptr<IPC::IMediaParcelable>> &containers)
{
    containers.emplace_back(std::make_shared<CloudErrorDetail>());
    containers.emplace_back(std::make_shared<CloudFileDataVo>());
    containers.emplace_back(std::make_shared<CloudMdkRecordPhotoAlbumVo>());
    containers.emplace_back(std::make_shared<CloudMdkRecordPhotoAlbumReqBody>());
    containers.emplace_back(std::make_shared<CloudMdkRecordPhotoAlbumRespBody>());

    containers.emplace_back(std::make_shared<CloudMdkRecordPhotosVo>());
    containers.emplace_back(std::make_shared<CloudMdkRecordPhotosReqBody>());
    containers.emplace_back(std::make_shared<CloudMdkRecordPhotosRespBody>());
    containers.emplace_back(std::make_shared<FailedSizeResp>());
    containers.emplace_back(std::make_shared<GetAgingFileReqBody>());

    containers.emplace_back(std::make_shared<GetAgingFileRespBody>());

    containers.emplace_back(std::make_shared<GetCheckRecordsReqBody>());
    containers.emplace_back(std::make_shared<GetCheckRecordsRespBodyCheckData>());
    containers.emplace_back(std::make_shared<GetCheckRecordsRespBody>());
    containers.emplace_back(std::make_shared<GetCloudThmStatRespBody>());
    containers.emplace_back(std::make_shared<GetDirtyTypeStatRespBody>());

    containers.emplace_back(std::make_shared<GetDownloadAssetReqBody>());
    containers.emplace_back(std::make_shared<GetDownloadAssetRespBody>());
    containers.emplace_back(std::make_shared<GetDownloadThmsByUriReqBody>());
    containers.emplace_back(std::make_shared<GetDownloadThmsByUriRespBody>());
    containers.emplace_back(std::make_shared<GetDownloadThmNumReqBody>());

    containers.emplace_back(std::make_shared<GetDownloadThmNumRespBody>());
    containers.emplace_back(std::make_shared<GetDownloadThmReqBody>());
    containers.emplace_back(std::make_shared<GetDownloadThmRespBody>());
    containers.emplace_back(std::make_shared<GetFilePosStatRespBody>());
    containers.emplace_back(std::make_shared<GetRetryRecordsRespBody>());

    containers.emplace_back(std::make_shared<GetVideoToCacheRespBody>());
    containers.emplace_back(std::make_shared<MediaOperateResultRespBodyResultNode>());
    containers.emplace_back(std::make_shared<MediaOperateResultRespBody>());
    containers.emplace_back(std::make_shared<OnCopyRecord>());
    containers.emplace_back(std::make_shared<OnCopyRecordsPhotosReqBody>());
}

void FillContainersPart1(std::vector<std::shared_ptr<IPC::IMediaParcelable>> &containers)
{
    containers.emplace_back(std::make_shared<OnCreateRecordsAlbumReqBodyAlbumData>());
    containers.emplace_back(std::make_shared<OnCreateRecordsAlbumReqBody>());
    containers.emplace_back(std::make_shared<OnCreateRecord>());
    containers.emplace_back(std::make_shared<OnCreateRecordsPhotosReqBody>());
    containers.emplace_back(std::make_shared<OnDeleteAlbumsRespBody>());

    containers.emplace_back(std::make_shared<OnDeleteAlbumData>());
    containers.emplace_back(std::make_shared<OnDeleteRecordsAlbumReqBody>());
    containers.emplace_back(std::make_shared<OnDeleteRecordsAlbumRespBody>());
    containers.emplace_back(std::make_shared<OnDeleteRecordsPhoto>());
    containers.emplace_back(std::make_shared<OnDeleteRecordsPhotosReqBody>());

    containers.emplace_back(std::make_shared<OnDeleteRecordsPhotosRespBody>());
    containers.emplace_back(std::make_shared<OnDentryFileReqBody>());
    containers.emplace_back(std::make_shared<OnDentryFileRespBody>());
    containers.emplace_back(std::make_shared<OnDownloadAssetReqBody>());
    containers.emplace_back(std::make_shared<OnDownloadThmsReqBody>());

    containers.emplace_back(std::make_shared<OnFetchPhotosVo>());
    containers.emplace_back(std::make_shared<OnFetchRecordsAlbumReqBody>());
    containers.emplace_back(std::make_shared<OnFetchRecordsAlbumRespBody>());
    containers.emplace_back(std::make_shared<OnFetchRecordsReqBody>());
    containers.emplace_back(std::make_shared<OnFetchRecordsRespBody>());

    containers.emplace_back(std::make_shared<OnMdirtyAlbumRecord>());
    containers.emplace_back(std::make_shared<OnMdirtyRecordsAlbumReqBody>());
    containers.emplace_back(std::make_shared<OnMdirtyRecordsAlbumRespBody>());
    containers.emplace_back(std::make_shared<OnFileDirtyRecord>());
    containers.emplace_back(std::make_shared<OnFileDirtyRecordsReqBody>());

    containers.emplace_back(std::make_shared<OnModifyRecord>());
    containers.emplace_back(std::make_shared<OnModifyRecordsPhotosReqBody>());
    containers.emplace_back(std::make_shared<PhotoAlbumVo>());
    containers.emplace_back(std::make_shared<PhotosVo>());
    containers.emplace_back(std::make_shared<UpdateDirtyReqBody>());

    containers.emplace_back(std::make_shared<UpdateLocalFileDirtyReqBody>());
    containers.emplace_back(std::make_shared<UpdatePositionReqBody>());
    containers.emplace_back(std::make_shared<UpdateSyncStatusReqBody>());
    containers.emplace_back(std::make_shared<UpdateThmStatusReqBody>());
}

HWTEST_F(CloudMediaSyncServiceVoTest, GeneralVoBasic_Test, TestSize.Level1)
{
    std::vector<std::shared_ptr<IPC::IMediaParcelable>> containers;
    FillContainersPart1(containers);
    FillContainersPart2(containers);
    for (const auto &iter : containers) {
        MessageParcel parcel;
        ASSERT_TRUE(iter);
        iter->Marshalling(parcel);
        iter->Unmarshalling(parcel);
    }
}

HWTEST_F(CloudMediaSyncServiceVoTest, GeneralToString_Test, TestSize.Level1)
{
    EXPECT_FALSE(PhotoAlbumVo().ToString().empty());
    EXPECT_TRUE(GetDownloadAssetRespBody().ToString().empty());
    EXPECT_TRUE(GetDownloadAssetReqBody().ToString().empty());
    EXPECT_TRUE(GetVideoToCacheRespBody().ToString().empty());
    EXPECT_FALSE(UpdateDirtyReqBody().ToString().empty());

    EXPECT_TRUE(GetDirtyTypeStatRespBody().ToString().empty());
    EXPECT_TRUE(GetDownloadThmsByUriRespBody().ToString().empty());
    EXPECT_TRUE(GetDownloadThmsByUriReqBody().ToString().empty());
    EXPECT_TRUE(GetCloudThmStatRespBody().ToString().empty());
    EXPECT_TRUE(GetRetryRecordsRespBody().ToString().empty());

    EXPECT_FALSE(GetDownloadThmNumRespBody().ToString().empty());
    EXPECT_FALSE(GetDownloadThmNumReqBody().ToString().empty());
    EXPECT_TRUE(GetDownloadThmRespBody().ToString().empty());
    EXPECT_FALSE(GetDownloadThmReqBody().ToString().empty());
    EXPECT_TRUE(GetFilePosStatRespBody().ToString().empty());

    EXPECT_FALSE(CloudErrorDetail().ToString().empty());
    EXPECT_TRUE(CloudMdkRecordPhotoAlbumVo().ToString().empty());
    EXPECT_TRUE(CloudMdkRecordPhotoAlbumReqBody().ToString().empty());
    EXPECT_FALSE(FailedSizeResp().ToString().empty());
    EXPECT_FALSE(UpdateThmStatusReqBody().ToString().empty());
    EXPECT_FALSE(UpdateSyncStatusReqBody().ToString().empty());
    EXPECT_FALSE(PhotosVo().ToString().empty());
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnFetchRecords_Test, TestSize.Level1)
{
    auto respBody = std::make_shared<OnFetchRecordsRespBody>();
    EXPECT_NE(respBody, nullptr);
    PhotosVo newPhoto;
    newPhoto.fileId = 1;
    newPhoto.cloudId = "photo";
    respBody->newDatas.push_back(newPhoto);
    respBody->newDatas.push_back(newPhoto);
    respBody->fdirtyDatas.push_back(newPhoto);
    respBody->fdirtyDatas.push_back(newPhoto);
    respBody->failedRecords.push_back("failed_record_1");
    respBody->failedRecords.push_back("failed_record_2");
    EXPECT_NE(respBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, GetCheckRecords_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<GetCheckRecordsReqBody>();
    EXPECT_NE(reqBody, nullptr);
    reqBody->cloudIds.push_back("id1");
    reqBody->cloudIds.push_back("id2");
    EXPECT_NE(reqBody->ToString(), "");

    auto respBody = std::make_shared<GetCheckRecordsRespBody>();
    EXPECT_NE(respBody, nullptr);
    GetCheckRecordsRespBodyCheckData data;
    data.cloudId = "cloud_id";
    data.size = 1024;
    data.data = "/path/to/file";
    data.displayName = "File";
    respBody->checkDataList["cloud_id_1"] = data;
    respBody->checkDataList["cloud_id_2"] = data;
    EXPECT_NE(respBody->ToString(), "");

    MessageParcel parcel;
    respBody->Marshalling(parcel);
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnModifyFileDirty_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnFileDirtyRecordsReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnFileDirtyRecord record;
    CloudErrorDetail err;
    err.domain = "cloud_sync";
    err.reason = "network_error";
    err.errorCode = "26004977";
    err.description = "Failed to sync data due to network issues";
    err.errorPos = "upload";
    err.errorParam = "file:///path/to/file";
    err.detailCode = 1001;
    record.fileId = 123;
    record.rotation = 456;
    record.fileType = 3;
    record.errorDetails.push_back(err);
    record.errorDetails.push_back(err);
    reqBody->records.push_back(record);
    reqBody->records.push_back(record);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnFetchRecordsAlbum_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnFetchRecordsAlbumReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnFetchRecordsAlbumReqBody::AlbumReqData req;
    req.cloudId = "cloud_id_1";
    req.localPath = "local_path";
    req.albumName = "album_name";
    reqBody->albums.push_back(req);
    reqBody->albums.push_back(req);
    EXPECT_NE(reqBody->ToString(), "");

    auto respBody = std::make_shared<OnFetchRecordsAlbumRespBody>();
    EXPECT_NE(reqBody, nullptr);
    respBody->failedRecords.push_back("failed_record_1");
    respBody->failedRecords.push_back("failed_record_2");
    EXPECT_NE(respBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnFetchPhotos_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnFetchPhotosVo>();
    EXPECT_NE(reqBody, nullptr);
    reqBody->sourceAlbumIds.push_back("source_album_id_1");
    reqBody->sourceAlbumIds.push_back("source_album_id_2");
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnDownloadThms_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnDownloadThmsReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnDownloadThmsReqBody::DownloadThmsData data;
    data.cloudId = "cloud_id_1";
    data.thumbStatus = 1;
    reqBody->downloadThmsDataList.push_back(data);
    reqBody->downloadThmsDataList.push_back(data);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnDownloadAsset_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnDownloadAssetReqBody>();
    EXPECT_NE(reqBody, nullptr);
    reqBody->cloudIds.push_back("cloud_id_1");
    reqBody->cloudIds.push_back("cloud_id_2");
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnDentryFile_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnDentryFileReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnFetchPhotosVo record;
    record.cloudId = "test_cloud_id";
    record.fileName = "test_file_name";
    reqBody->AddOnDentryFileRecord(record);
    reqBody->AddOnDentryFileRecord(record);
    EXPECT_NE(reqBody->ToString(), "");

    auto respBody = std::make_shared<OnDentryFileRespBody>();
    EXPECT_NE(reqBody, nullptr);
    respBody->failedRecords.push_back("failed_record_1");
    respBody->failedRecords.push_back("failed_record_2");
    EXPECT_NE(respBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnDeleteRecordsPhotos_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnDeleteRecordsPhotosReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnDeleteRecordsPhoto record;
    record.dkRecordId = "test_cloud_id";
    record.cloudId = "test_cloud_id";
    record.isSuccess = true;
    reqBody->AddDeleteRecord(record);
    reqBody->AddDeleteRecord(record);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnDeleteRecordsAlbum_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnDeleteRecordsAlbumReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnDeleteAlbumData albumData;
    albumData.cloudId = "test_cloud_id";
    albumData.isSuccess = true;
    reqBody->AddSuccessResult(albumData);
    reqBody->AddSuccessResult(albumData);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnCreateRecordsPhotos_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnCreateRecordsPhotosReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnCreateRecord record;
    record.fileId = 123;
    record.localId = 456;
    CloudErrorDetail err;
    err.domain = "cloud_sync";
    err.reason = "network_error";
    err.errorCode = "26004977";
    err.description = "Failed to sync data due to network issues";
    err.errorPos = "upload";
    err.errorParam = "file:///path/to/file";
    err.detailCode = 1001;
    record.fileId = 123;
    record.rotation = 456;
    record.fileType = 3;
    record.errorDetails.push_back(err);
    record.errorDetails.push_back(err);
    reqBody->AddRecord(record);
    reqBody->AddRecord(record);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, OnCopyRecordsPhotos_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<OnCopyRecordsPhotosReqBody>();
    EXPECT_NE(reqBody, nullptr);
    OnCopyRecord record;
    record.fileId = 123;
    record.rotation = 0;
    record.fileType = 1;
    record.size = 1024;
    record.createTime = 1625097600;
    record.modifyTime = 1625097600;
    record.version = 1;
    record.cloudId = "cloud123";
    record.path = "/path/to/file";
    record.fileName = "test.txt";
    record.sourcePath = "/source/path";
    record.isSuccess = true;
    reqBody->AddCopyRecord(record);
    reqBody->AddCopyRecord(record);
    EXPECT_NE(reqBody->ToString(), "");
}

HWTEST_F(CloudMediaSyncServiceVoTest, MediaOperateResult_Test, TestSize.Level1)
{
    auto respBody = std::make_shared<MediaOperateResultRespBody>();
    EXPECT_NE(respBody, nullptr);

    respBody->result.resize(2);
    respBody->result[0].cloudId = "001";
    respBody->result[0].errorCode = 1;
    respBody->result[0].errorMsg = "success";
    respBody->result[1].cloudId = "002";
    respBody->result[1].errorCode = 2;
    respBody->result[1].errorMsg = "fail";
    respBody->ToString();
}

HWTEST_F(CloudMediaSyncServiceVoTest, UpdatePositionReqBody_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<UpdatePositionReqBody>();
    ASSERT_TRUE(reqBody);
    reqBody->position = 1;
    EXPECT_EQ(reqBody->ToString(), "{\"position\": 1, []}");
    reqBody->cloudIds.emplace_back("001");
    reqBody->cloudIds.emplace_back("002");
    EXPECT_EQ(reqBody->ToString(), "{\"position\": 1, [001,002]}");
}

HWTEST_F(CloudMediaSyncServiceVoTest, UpdateLocalFileDirtyReqBody_Test, TestSize.Level1)
{
    auto reqBody = std::make_shared<UpdateLocalFileDirtyReqBody>();
    ASSERT_TRUE(reqBody);
    EXPECT_EQ(reqBody->ToString(), "[]");
    reqBody->cloudIds.emplace_back("001");
    reqBody->cloudIds.emplace_back("002");
    EXPECT_EQ(reqBody->ToString(), "[\"001\",\"002\"]");
}

HWTEST_F(CloudMediaSyncServiceVoTest, PhotosVo_Test, TestSize.Level1)
{
    auto vo = std::make_shared<PhotosVo>();
    ASSERT_TRUE(vo);
    std::map<std::string, CloudFileDataVo> dataMap;
    EXPECT_FALSE(vo->ToString().empty());

    CloudFileDataVo vo1;
    CloudFileDataVo vo2;
    dataMap["001"] = vo1;
    dataMap["002"] = vo2;
    EXPECT_FALSE(vo->ToString().empty());
}

HWTEST_F(CloudMediaSyncServiceVoTest, CloudFileDataVo_Test, TestSize.Level1)
{
    MessageParcel parcel;
    auto dataVo = std::make_shared<CloudFileDataVo>();
    ASSERT_TRUE(dataVo);
    dataVo->Marshalling(parcel);
    dataVo->Unmarshalling(parcel);
    string result = dataVo->ToString();
    EXPECT_FALSE(result.empty());

    string str1 = "test1";
    string str2 = "test2";
    CloudFileDataVo vo1;
    CloudFileDataVo vo2;
    std::map<std::string, CloudFileDataVo> res;

    EXPECT_TRUE(dataVo->Marshalling(res, parcel));
    EXPECT_TRUE(dataVo->Unmarshalling(res, parcel));
    res.insert(std::make_pair(str1, vo1));
    res.insert(std::make_pair(str2, vo2));
    EXPECT_TRUE(dataVo->Marshalling(res, parcel));
    EXPECT_TRUE(dataVo->Unmarshalling(res, parcel));
}

HWTEST_F(CloudMediaSyncServiceVoTest, CloudMdkRecordPhotoAlbumRespBody_Test, TestSize.Level1)
{
    MessageParcel parcel;
    auto reqBody = std::make_shared<CloudMdkRecordPhotoAlbumRespBody>();
    ASSERT_TRUE(reqBody);
    parcel.WriteInt32(-1);
    EXPECT_FALSE(reqBody->Unmarshalling(parcel));
    EXPECT_FALSE(reqBody->Marshalling(parcel));

    auto reqBody2 = std::make_shared<CloudMdkRecordPhotoAlbumRespBody>();
    MessageParcel parcel2;
    parcel2.WriteInt32(1);
    EXPECT_FALSE(reqBody2->Unmarshalling(parcel2));
    EXPECT_FALSE(reqBody2->Marshalling(parcel2));
}

HWTEST_F(CloudMediaSyncServiceVoTest, CloudMdkRecordPhotosVo_Test, TestSize.Level1)
{
    MessageParcel parcel;
    auto photosVo = std::make_shared<CloudMdkRecordPhotosVo>();
    photosVo->Marshalling(parcel);
    photosVo->Unmarshalling(parcel);
    string result = photosVo->ToString();
    EXPECT_FALSE(result.empty());

    std::stringstream ss;
    photosVo->removeAlbumCloudId.clear();
    photosVo->removeAlbumCloudId.emplace_back("001");
    photosVo->removeAlbumCloudId.emplace_back("002");
    photosVo->GetRemoveAlbumInfo(ss);
    EXPECT_EQ(ss.str(), "[001,002]");
}

HWTEST_F(CloudMediaSyncServiceVoTest, GetAgingFileReqBody_Test, TestSize.Level1)
{
    MessageParcel parcel;
    auto reqBody = std::make_shared<GetAgingFileReqBody>();
    ASSERT_TRUE(reqBody);
    reqBody->Marshalling(parcel);
    reqBody->Unmarshalling(parcel);
    string result = reqBody->ToString();
    EXPECT_FALSE(result.empty());

    auto fileBody = std::make_shared<GetAgingFileRespBody>();
    fileBody->Marshalling(parcel);
    fileBody->Unmarshalling(parcel);
    fileBody->ToString();
}

HWTEST_F(CloudMediaSyncServiceVoTest, CloudMdkRecordPhotosRespBody_TruncateDataBy200K_Test, TestSize.Level1)
{
    CloudMdkRecordPhotosVo photosVo;
    std::vector<CloudMdkRecordPhotosVo> uploadRecords = {photosVo};
    CloudMdkRecordPhotosRespBody respBody{uploadRecords};
    EXPECT_TRUE(respBody.TruncateDataBy200K());
    EXPECT_EQ(1, respBody.GetDataSize());
}
}