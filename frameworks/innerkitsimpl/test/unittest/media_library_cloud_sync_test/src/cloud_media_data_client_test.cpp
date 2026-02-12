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

#include "cloud_media_data_client_test.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>
#include <fcntl.h>

#include "media_log.h"
#include "database_data_mock.h"
#include "cloud_media_data_client.h"
#include "cloud_media_sync_const.h"
#include "photos_dao.h"
#include "cloud_media_operation_code.h"
#include "json_file_reader.h"
#include "cloud_data_utils.h"
#include "media_operate_result.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {
enum ThmsType {
    TYPE_THM = 1,
    TYPE_LCD = 2,
    TYPE_THM_AND_LCD = 3,
    TYPE_ASTC = 4,
};
DatabaseDataMock CloudMediaDataClientTest::dbDataMock_;
std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;

void CloudMediaDataClientTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    // Get RdbStore
    int32_t errorCode = 0;
    rdbStore_ = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore_).CheckPoint();
    ret = dbDataMock_.MockData(CloudMediaDataClientTest::GetTableMockInfoList());
    GTEST_LOG_(INFO) << "SetUpTestCase ret: " << ret;
}

void CloudMediaDataClientTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "TearDownTestCase ret: " << ret;
}

// SetUp:Execute before each test case
void CloudMediaDataClientTest::SetUp() {}

void CloudMediaDataClientTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}
HWTEST_F(CloudMediaDataClientTest, UpdateDirty, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b348e";
    DirtyTypes type = DirtyTypes::TYPE_SYNCED;
    int32_t ret = cloudMediaDataClient.UpdateDirty(cloudId, type);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::vector<std::string> columns = {PhotoColumn::PHOTO_DIRTY};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    auto resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(rowCount, 1);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, UpdatePosition, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b348e";
    PhotoPositionType position = PhotoPositionType::LOCAL_AND_CLOUD;
    std::vector<std::string> cloudIds = {cloudId};
    int32_t ret = cloudMediaDataClient.UpdatePosition(cloudIds, static_cast<int32_t>(position));
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::vector<std::string> columns = {PhotoColumn::PHOTO_POSITION};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    auto resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(rowCount, 1);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t pos = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        EXPECT_EQ(pos, static_cast<int32_t>(position));
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, UpdateSyncStatus, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b348e";
    int32_t syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD);
    int32_t ret = cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);
    EXPECT_EQ(ret, 0);
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    PhotosPo photo;
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.syncStatus.value_or(-1), syncStatus);

    syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_BACKUP);
    ret = cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);
    EXPECT_EQ(ret, 0);
    photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.syncStatus.value_or(-1), syncStatus);

    syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE);
    ret = cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);
    EXPECT_EQ(ret, 0);
    photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.syncStatus.value_or(-1), syncStatus);

    syncStatus = static_cast<int32_t>(SyncStatusType::TYPE_DOWNLOAD);
    ret = cloudMediaDataClient.UpdateSyncStatus(cloudId, syncStatus);
    EXPECT_EQ(ret, 0);
    photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.syncStatus.value_or(-1), syncStatus);
}

// UPDATE Photos SET thumb_status = ? WHERE cloud_id = ?
HWTEST_F(CloudMediaDataClientTest, UpdateThmStatus, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b348e";
    PhotosPo photo;
    std::vector<PhotosPo> photosList;
    int32_t thumbStatus;
    int32_t ret;
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    // CASE 1
    thumbStatus = 0;
    ret = cloudMediaDataClient.UpdateThmStatus(cloudId, thumbStatus);
    EXPECT_EQ(ret, 0);
    photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.thumbStatus.value_or(-1), thumbStatus);
    // CASE 2
    thumbStatus = 1;
    ret = cloudMediaDataClient.UpdateThmStatus(cloudId, thumbStatus);
    EXPECT_EQ(ret, 0);
    photosList = photosDao.QueryPhotosByCloudId(cloudId);
    EXPECT_GT(photosList.size(), 0);
    ret = photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.thumbStatus.value_or(-1), thumbStatus);
}

HWTEST_F(CloudMediaDataClientTest, OnCompleteCheck, TestSize.Level1)
{
    EXPECT_TRUE(true);
}

HWTEST_F(CloudMediaDataClientTest, GetAgingFile, TestSize.Level1)
{
    int64_t time = 1;
    int32_t mediaType = 2;
    int32_t sizeLimit = 10;
    int32_t offset = 0;
    std::vector<CloudMetaData> metaDataList;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.GetAgingFile(time, mediaType, sizeLimit, offset, metaDataList);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(metaDataList.size(), 0);
    // Check the expected element in the list
    CloudMetaData targetData;
    for (auto &metaData : metaDataList) {
        if (metaData.path == "/storage/cloud/files/Photo/15/VID_1735607604_10015.mp4") {
            targetData = metaData;
            break;
        }
    }
    EXPECT_EQ(targetData.path, "/storage/cloud/files/Photo/15/VID_1735607604_10015.mp4");
}

HWTEST_F(CloudMediaDataClientTest, GetActiveAgingFile, TestSize.Level1)
{
    int64_t time = 1;
    int32_t mediaType = 2;
    int32_t sizeLimit = 10;
    int32_t offset = 0;
    std::vector<CloudMetaData> metaDataList;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.GetActiveAgingFile(time, mediaType, sizeLimit, offset, metaDataList);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(metaDataList.size(), 0);
    // Check the expected element in the list
    CloudMetaData targetData;
    for (auto &metaData : metaDataList) {
        if (metaData.path == "/storage/cloud/files/Photo/15/VID_1735607604_10015.mp4") {
            targetData = metaData;
            break;
        }
    }
    EXPECT_EQ(targetData.path, "/storage/cloud/files/Photo/15/VID_1735607604_10015.mp4");
}

/**
 * 下载，输入图片uri
 * 期望结果：
 * 获取到的metaData.size 与传入uri数量一致，且cloudId匹配
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadAsset, TestSize.Level1)
{
    // 构造数据
    std::vector<std::string> cloudIds = {"f98ac2cb8a3e4951a485857a2391a31a6a5cb6d78eb64be8b9c28721cf901509",
                                         "f98ac2cb8a3e4951a485857a2391a31a6a5cb6d78eb64be8b9c28721cf901510"};
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_GT(photosList.size(), 0);

    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<std::string> uris;
    std::vector<std::string> fileKeyVec;
    std::vector<CloudMetaData> cloudMetaDataVec;

    for (auto &photo : photosList) {
        MEDIA_INFO_LOG("GetDownloadAsset %{public}s", photo.ToString().c_str());
        std::string uri = photosDao.BuildUriByPhoto(photo);
        uris.emplace_back(uri);
    }

    int32_t ret = cloudMediaDataClient.GetDownloadAsset(uris, cloudMetaDataVec);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(cloudMetaDataVec.size(), uris.size());

    int32_t num = 0;
    for (auto const &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadAsset %{public}s", data.ToString().c_str());
        for (auto &photo : photosList) {
            if (photo.cloudId == data.cloudId) {
                EXPECT_EQ(photo.fileId.value_or(0), data.fileId);
                EXPECT_EQ(photo.size.value_or(0), data.size);
                EXPECT_EQ(photo.data.value_or(""), data.path);
                EXPECT_EQ(photo.mediaType.value_or(0), data.type);
                EXPECT_EQ(photo.editTime.value_or(0), data.modifiedTime);
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudMetaDataVec.size());
}

/**
 * 下载，输入视频uri
 * 期望结果：
 * 获取到的metaData.size 与传入uri数量一致，且cloudId匹配
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadAsset_Video, TestSize.Level1)
{
    std::vector<std::string> cloudIds = {"f98ac2cb8a3e4951a485857a2391a31a6a5cb6d78eb64be8b9c28721cf901511"};
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_GT(photosList.size(), 0);

    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<std::string> uris;
    std::vector<std::string> fileKeyVec;
    std::vector<CloudMetaData> cloudMetaDataVec;

    for (auto &photo : photosList) {
        std::string uri = photosDao.BuildUriByPhoto(photo);
        uris.emplace_back(uri);
    }

    int32_t ret = cloudMediaDataClient.GetDownloadAsset(uris, cloudMetaDataVec);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(cloudMetaDataVec.size(), uris.size());

    int32_t num = 0;
    for (auto const &data : cloudMetaDataVec) {
        for (auto &photo : photosList) {
            if (photo.cloudId == data.cloudId) {
                EXPECT_EQ(photo.fileId.value_or(0), data.fileId);
                EXPECT_EQ(photo.size.value_or(0), data.size);
                EXPECT_EQ(photo.data.value_or(""), data.path);
                EXPECT_EQ(photo.mediaType.value_or(0), data.type);
                EXPECT_EQ(photo.editTime.value_or(0), data.modifiedTime);
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudMetaDataVec.size());
}

HWTEST_F(CloudMediaDataClientTest, GetDownloadThmsByUri, TestSize.Level1)
{
    // 构造数据
    std::vector<std::string> cloudIds = {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b358e",
                                         "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b368e"};
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_GT(photosList.size(), 0);

    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<std::string> uris;
    int32_t thmType = TYPE_THM_AND_LCD;
    std::vector<CloudMetaData> cloudMetaDataVec;

    for (auto &photo : photosList) {
        std::string uri = photosDao.BuildUriByPhoto(photo);
        uris.emplace_back(uri);
    }

    int32_t ret = cloudMediaDataClient.GetDownloadThmsByUri(uris, thmType, cloudMetaDataVec);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(cloudMetaDataVec.size(), uris.size());
    int32_t num = 0;
    for (auto const &data : cloudMetaDataVec) {
        for (auto const &photo : photosList) {
            if (photo.cloudId == data.cloudId) {
                EXPECT_EQ(photo.fileId.value_or(0), data.fileId);
                EXPECT_EQ(photo.size.value_or(0), data.size);
                EXPECT_EQ(photo.data.value_or(""), data.path);
                EXPECT_EQ(photo.mediaType.value_or(0), data.type);
                EXPECT_EQ(photo.editTime.value_or(0), data.modifiedTime);
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudMetaDataVec.size());
}

HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset, TestSize.Level1)
{
    std::vector<std::string> cloudIds;
    cloudIds.push_back("3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b338e");
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    EXPECT_EQ(ret, 0);
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上静态图片，本地无原图，下载图片成功
 * 期望结果:修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case001, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210036";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上编辑可回退图片，本地无原图，下载图片成功
 * 期望结果:修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case002, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210037";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上动图，本地无原图，下载图片成功
 * 期望结果:原图拆分为图片和视频，修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case003, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210038";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上编辑可回退动图，本地无原图，下载图片成功
 * 期望结果:原图和raw文件拆分为图片和视频，修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case004, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210039";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上连拍，本地无原图，下载图片成功
 * 期望结果:修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case005, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210040";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上视频，本地无原图，下载图片成功
 * 期望结果:修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case006, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210041";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上涂鸦动图，本地无原图，下载图片成功
 * 期望结果:原图不需要拆分，修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case007, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210042";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 预设条件:本地同步云上编辑可回退涂鸦动图，本地无原图，下载图片成功
 * 期望结果:raw文件拆分为图片和视频，修改position字段
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadAsset_case008, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec7442210043";
    std::vector<std::string> cloudIds = {cloudId};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<MediaOperateResult> result;
    int32_t ret = cloudMediaDataClient.OnDownloadAsset(cloudIds, result);
    TestUtils::PhotosDao dao;
    std::vector<PhotosPo> photos = dao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_EQ(photos.size(), cloudIds.size());
    EXPECT_EQ(photos.size(), result.size());
    ORM::PhotosPo photo;
    ret = dao.GetPhotoByCloudId(photos, cloudId, photo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    for (auto &item : result) {
        EXPECT_EQ(item.errorCode, 0);
        EXPECT_EQ(item.errorMsg, "");
    }
}

/**
 * 下载缩略图，ThumAndLcd
 * 期望结果：
 * 按date_taken顺序输出需要下载lcd和thum的图片信息
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadThms_ThumAndLcd_Default, TestSize.Level1)
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t size = 10;
    int32_t offset = 0;
    int32_t type = 3;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    DownloadThumPara param;
    param.size = size;
    param.offset = offset;
    param.type = type;
    int32_t ret = cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataVec.size(), 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosDownloadThms();
    EXPECT_GT(photosList.size(), 0);

    for (auto &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadThms_ThumAndLcd_Default %{public}s", data.ToString().c_str());
    }

    // 检验结果
    int32_t num = 0;
    for (auto &photo : photosList) {
        for (auto &data : cloudMetaDataVec) {
            if (photo.cloudId.value_or("") == data.cloudId) {
                num++;
            }
        }
    }
    EXPECT_GT(num, 0);

    // 校验附件信息
    bool res = true;
    for (auto &data : cloudMetaDataVec) {
        std::map<std::string, CloudFileData> attachment = data.attachment;
        if (attachment.find("lcd") == attachment.end() || attachment.find("thumbnail") == attachment.end()) {
            res = false;
        }
    }
    EXPECT_TRUE(res);
}

/**
 * 下载缩略图，OnlyThum
 * 期望结果：
 * 按date_taken顺序输出需要下载thum的图片信息
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadThms_OnlyThum, TestSize.Level1)
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t size = 10;
    int32_t offset = 0;
    int32_t type = 1;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    DownloadThumPara param;
    param.size = size;
    param.offset = offset;
    param.type = type;
    int32_t ret = cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataVec.size(), 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosDownloadThms();
    EXPECT_GT(photosList.size(), 0);

    for (auto &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadThms_ThumAndLcd_Default %{public}s", data.ToString().c_str());
    }

    // 检验结果
    int32_t num = 0;
    for (auto &photo : photosList) {
        for (auto &data : cloudMetaDataVec) {
            if (photo.cloudId.value_or("") == data.cloudId) {
                num++;
            }
        }
    }
    EXPECT_GT(num, 0);

    // 校验附件信息
    bool res = true;
    for (auto &data : cloudMetaDataVec) {
        std::map<std::string, CloudFileData> attachment = data.attachment;
        if (attachment.find("thumbnail") == attachment.end()) {
            res = false;
        }
    }
    EXPECT_TRUE(res);
}

/**
 * 下载缩略图，OnlyLcd
 * 期望结果：
 * 按date_taken顺序输出需要下载lcd和thms的图片信息
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadThms_OnlyLcd, TestSize.Level1)
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t size = 10;
    int32_t offset = 0;
    int32_t type = 2;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    DownloadThumPara param;
    param.size = size;
    param.offset = offset;
    param.type = type;
    int32_t ret = cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataVec.size(), 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosDownloadThms();
    EXPECT_GT(photosList.size(), 0);

    for (auto &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadThms_ThumAndLcd_Default %{public}s", data.ToString().c_str());
    }

    // 检验结果
    int32_t num = 0;
    for (auto &photo : photosList) {
        for (auto &data : cloudMetaDataVec) {
            if (photo.cloudId.value_or("") == data.cloudId) {
                num++;
            }
        }
    }
    EXPECT_GT(num, 0);

    // 校验附件信息
    bool res = true;
    for (auto &data : cloudMetaDataVec) {
        std::map<std::string, CloudFileData> attachment = data.attachment;
        if (attachment.find("lcd") == attachment.end()) {
            res = false;
        }
    }
    EXPECT_TRUE(res);
}

/**
 * 下载缩略图，Display_True
 * 期望结果：
 * 按date_taken顺序输出需要下载lcd和thms的图片信息
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadThms_ThumAndLcd_Display_True, TestSize.Level1)
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t size = 10;
    int32_t offset = 0;
    int32_t type = 3;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    DownloadThumPara param;
    param.size = size;
    param.offset = offset;
    param.type = type;
    param.isDownloadDisplayFirst = true;
    int32_t ret = cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataVec.size(), 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosDownloadThms();
    EXPECT_GT(photosList.size(), 0);

    for (auto &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadThms_ThumAndLcd_Default %{public}s", data.ToString().c_str());
    }

    int32_t num = 0;
    for (auto &photo : photosList) {
        for (auto &data : cloudMetaDataVec) {
            if (photo.cloudId.value_or("") == data.cloudId) {
                num++;
            }
        }
    }
    EXPECT_GT(num, 0);

    // 校验附件信息
    bool res = true;
    for (auto &data : cloudMetaDataVec) {
        std::map<std::string, CloudFileData> attachment = data.attachment;
        if (attachment.find("lcd") == attachment.end() || attachment.find("thumbnail") == attachment.end()) {
            res = false;
        }
    }
    EXPECT_TRUE(res);
}

/**
 * 下载缩略图，Offset_100
 * 期望结果：
 * 按date_taken顺序输出需要下载lcd和thms的图片信息，并且符合总数和分开查询的要求，比如总数10条，offset为5，最终结果为5条
 */
HWTEST_F(CloudMediaDataClientTest, GetDownloadThms_ThumAndLcd_Offset_100, TestSize.Level1)
{
    std::vector<CloudMetaData> cloudMetaDataVec;
    int32_t size = 10;
    int32_t offset = 5;
    int32_t type = 3;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    DownloadThumPara param;
    param.size = size;
    param.offset = offset;
    param.type = type;
    int32_t ret = cloudMediaDataClient.GetDownloadThms(cloudMetaDataVec, param);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataVec.size(), 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosDownloadThms();
    EXPECT_GT(photosList.size(), 0);

    for (auto &data : cloudMetaDataVec) {
        MEDIA_INFO_LOG("GetDownloadThms_ThumAndLcd_Default %{public}s", data.ToString().c_str());
    }

    int32_t num = 0;
    for (auto &photo : photosList) {
        for (auto &data : cloudMetaDataVec) {
            if (photo.cloudId.value_or("") == data.cloudId) {
                num++;
            }
        }
    }
    EXPECT_GT(num, 0);

    // 校验附件信息
    bool res = true;
    for (auto &data : cloudMetaDataVec) {
        std::map<std::string, CloudFileData> attachment = data.attachment;
        if (attachment.find("lcd") == attachment.end() || attachment.find("thumbnail") == attachment.end()) {
            res = false;
        }
    }
    EXPECT_TRUE(res);
}

/**
 * 预设条件：输入上传下载THM、LCD、THM和LCD结果
 * 期望结果:
 * 1. 数据库刷新PHOTO_SYNC_STATUS = TYPE_VISIBLE，PHOTO_THUMB_STATUS & 对应statusMask = 0
 */
HWTEST_F(CloudMediaDataClientTest, OnDownloadThms, TestSize.Level1)
{
    std::unordered_map<std::string, int32_t> cloudIdThmStatusMap = {
        {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b508e", TYPE_THM},
        {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b518e", TYPE_LCD},
        {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b528e", TYPE_THM_AND_LCD},
    };
    const std::map<int32_t, int32_t> downLoadMaskMap = {
        {0x2, TYPE_THM},
        {0x1, TYPE_LCD},
        {0x3, TYPE_THM_AND_LCD},
    };
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t failSize;
    int32_t ret = cloudMediaDataClient.OnDownloadThms(cloudIdThmStatusMap, failSize);
    EXPECT_EQ(ret, 0);

    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList;
    PhotosPo photo;
    int32_t checkCount = 0;

    for (auto pair : cloudIdThmStatusMap) {
        photosList = photosDao.QueryPhotosByCloudId(pair.first);
        EXPECT_GT(photosList.size(), 0);
        ret = photosDao.GetPhotoByCloudId(photosList, pair.first, photo);
        EXPECT_EQ(ret, 0);
        if (pair.second != TYPE_LCD) {  //TYPE_THM、TYPE_THM_AND_LCD才更新PHOTO_SYNC_STATUS = TYPE_VISIBLE
            EXPECT_EQ(photo.syncStatus.value_or(-1), static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE))
                << "check syncSt not Expect val:" << pair.first;
        }
        for (auto mask : downLoadMaskMap) {
            if (pair.second == mask.second) {
                EXPECT_TRUE((photo.thumbStatus.value_or(0x3) & (static_cast<int32_t>(mask.first))) == 0)
                    << "check thmSt not Expect val:" << pair.first;
                checkCount++;
            }
        }
    }
    EXPECT_EQ(checkCount, cloudIdThmStatusMap.size());
}

HWTEST_F(CloudMediaDataClientTest, GetVideoToCache, TestSize.Level1)
{
    std::vector<std::string> cloudIds = {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b318f"};
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudIds(cloudIds);
    EXPECT_GT(photosList.size(), 0);

    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<CloudMetaData> cloudMetaDataList;
    int32_t size = 10;
    int32_t ret = cloudMediaDataClient.GetVideoToCache(cloudMetaDataList, size);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(cloudMetaDataList.size(), 0);

    int32_t num = 0;
    for (auto const &data : cloudMetaDataList) {
        for (auto const &cloudId : cloudIds) {
            PhotosPo photo;
            photosDao.GetPhotoByCloudId(photosList, cloudId, photo);
            if (photo.cloudId == data.cloudId) {
                EXPECT_EQ(photo.fileId.value_or(0), data.fileId);
                EXPECT_EQ(photo.size.value_or(0), data.size);
                EXPECT_EQ(photo.data.value_or(""), data.path);
                EXPECT_EQ(photo.mediaType.value_or(0), data.type);
                EXPECT_EQ(photo.editTime.value_or(0), data.modifiedTime);
                num++;
            }
        }
    }
    EXPECT_EQ(num, cloudIds.size());
}

/**
 * 获取视频，空对象
 * 期望结果：
 * 预置cloudId资源获取不到
 */
HWTEST_F(CloudMediaDataClientTest, GetVideoToCache_Empty, TestSize.Level1)
{
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b318e";
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    photosDao.UpdatePhotoPositionByCloudId(cloudId, 1);

    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<CloudMetaData> cloudMetaDataList;
    int32_t size = 10;
    int32_t ret = cloudMediaDataClient.GetVideoToCache(cloudMetaDataList, size);
    EXPECT_EQ(ret, 0);

    if (cloudMetaDataList.size() > 0) {
        int32_t num = 0;
        for (auto const &data : cloudMetaDataList) {
            MEDIA_INFO_LOG("GetVideoToCache_Empty data is %{public}s", data.ToString().c_str());
            if (cloudId == data.cloudId) {
                num++;
            }
        }
        EXPECT_EQ(num, 0);
    } else {
        EXPECT_EQ(cloudMetaDataList.size(), 0);
    }
}

HWTEST_F(CloudMediaDataClientTest, GetDownloadThmNum_test_01, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<CloudMetaData> cloudMetaDataList;
    int32_t totalNum = 0;
    int32_t type = static_cast<int32_t>(ThmLcdState::THMLCD);
    int32_t ret = cloudMediaDataClient.GetDownloadThmNum(totalNum, type);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(totalNum, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::vector<std::string> cloudIds = {"3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b398e",
                                         "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b408e",
                                         "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b418e"};
    std::vector<std::string> columns = {PhotoColumn::PHOTO_CLOUD_ID};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS,
                       std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, std::to_string(static_cast<int32_t>(Clean::NOT_NEED_CLEAN)));
    predicates
        .NotEqualTo(PhotoColumn::PHOTO_POSITION, std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)))
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, std::to_string(static_cast<int32_t>(ThumbState::TO_DOWNLOAD)))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, std::to_string(static_cast<int32_t>(ThumbState::THM_TO_DOWNLOAD)))
        ->EndWrap();
    auto resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 0);
    int32_t expectedCounts = 3;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        if (std::find(cloudIds.begin(), cloudIds.end(), cloudId) != cloudIds.end()) {
            expectedCounts--;
        }
    }
    EXPECT_EQ(expectedCounts, 0);
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, GetFilePosStat, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    std::vector<uint64_t> filePosStat = {0, 0, 0};
    int32_t position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    int32_t ret = cloudMediaDataClient.GetFilePosStat(filePosStat);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, std::to_string(position));
    std::vector<std::string> columns = {PhotoColumn::PHOTO_POSITION};
    auto resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    EXPECT_GT(filePosStat[2], 0);
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(filePosStat[2], rowCount);
    resultSet->Close();

    resultSet = nullptr;
    ret = -1;
    predicates.Clear();
    position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, std::to_string(position));
    resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    rowCount = 0;
    EXPECT_GT(filePosStat[0], 0);
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(filePosStat[0], rowCount);

    resultSet = nullptr;
    ret = -1;
    predicates.Clear();
    position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, std::to_string(position));
    resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    rowCount = 0;
    EXPECT_GT(filePosStat[1], 0);
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(filePosStat[1], rowCount);
}

HWTEST_F(CloudMediaDataClientTest, GetCloudThmStat, TestSize.Level1)
{
    std::vector<uint64_t> cloudThmStat{0, 0, 0, 0};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.GetCloudThmStat(cloudThmStat);
    EXPECT_EQ(ret, 0);
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    EXPECT_EQ(cloudThmStat[0], photosDao.GetCloudThmStatNum(0));  //INDEX_DOWNLOADED
    EXPECT_EQ(cloudThmStat[1], photosDao.GetCloudThmStatNum(1));  //INDEX_LCD_TO_DOWNLOAD
    EXPECT_EQ(cloudThmStat[2], photosDao.GetCloudThmStatNum(2));  //INDEX_THM_TO_DOWNLOAD
    EXPECT_EQ(cloudThmStat[3], photosDao.GetCloudThmStatNum(3));  //INDEX_TO_DOWNLOAD
}

HWTEST_F(CloudMediaDataClientTest, GetDirtyTypeStat, TestSize.Level1)
{
    std::vector<uint64_t> dirtyTypeStat{0, 0, 0, 0, 0};
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.GetDirtyTypeStat(dirtyTypeStat);
    EXPECT_EQ(ret, 0);
    TestUtils::PhotosDao photosDao = TestUtils::PhotosDao();
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_SYNCED)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_SYNCED)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_NEW)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_NEW)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_MDIRTY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_MDIRTY)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_FDIRTY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_FDIRTY)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_DELETED)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_DELETED)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_RETRY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_RETRY)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_SDIRTY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_SDIRTY)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_COPY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_COPY)));
    EXPECT_EQ(dirtyTypeStat[static_cast<int32_t>(DirtyType::TYPE_TDIRTY)],
        photosDao.GetDirtyTypeNum(static_cast<int32_t>(DirtyType::TYPE_TDIRTY)));
}

HWTEST_F(CloudMediaDataClientTest, UpdateLocalFileDirty, TestSize.Level1)
{
    JsonFileReader reader("/data/test/cloudsync/data_client_update_local_file_dirty_test.json");
    std::vector<MDKRecord> records;
    reader.ConvertToMDKRecordVector(records);
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.UpdateLocalFileDirty(records);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::vector<std::string> columns = {PhotoColumn::PHOTO_DIRTY};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> cloudIds;
    for (auto const &record : records) {
        EXPECT_TRUE(!record.GetRecordId().empty());
        cloudIds.emplace_back(record.GetRecordId());
    }
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    auto resultSet = rdbStore_->Query(predicates, columns);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 0);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        EXPECT_EQ(dirty, static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, CheckAndFixAlbum_01, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.CheckAndFixAlbum();
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::string cloud_id = "";
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_DIRTY};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_CLOUD_ID, cloud_id);
    auto resultSet = rdbStore_->Query(predicates, columns);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, CheckAndFixAlbum_02, TestSize.Level1)
{
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.CheckAndFixAlbum();
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(rdbStore_ != nullptr);
    std::string cloud_id = "";
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_DIRTY};
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_CLOUD_ID, cloud_id);
    auto resultSet = rdbStore_->Query(predicates, columns);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t dirty = GetInt32Val(PhotoAlbumColumns::ALBUM_DIRTY, resultSet);
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
        EXPECT_NE(dirty, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, QueryDataInPhotos, TestSize.Level1)
{
    DataShare::DataSharePredicates predicates;
    std::string cloudId = "3d4970270f8d4b15b4ced48bd7f25dd44c7ad693ae57426d863fec74422b388e";
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    const std::vector<std::string> columnNames = {PhotoColumn::MEDIA_TITLE};
    const std::string tableName = Media::PhotoColumn::PHOTOS_TABLE;
    std::vector<std::unordered_map<std::string, std::string>> results;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.QueryData(predicates, columnNames, tableName, results);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(results.size(), 0);
    NativeRdb::AbsRdbPredicates rdbpredicates = NativeRdb::AbsRdbPredicates(Media::PhotoColumn::PHOTOS_TABLE);
    rdbpredicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    auto resultSet = rdbStore_->Query(rdbpredicates, columnNames);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 0);
    int32_t pos = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string media_title = GetStringVal(PhotoColumn::MEDIA_TITLE, resultSet);
        EXPECT_LT(pos, results.size());
        EXPECT_EQ(media_title, results[pos][PhotoColumn::MEDIA_TITLE]);
        pos++;
    }
    resultSet->Close();
}

HWTEST_F(CloudMediaDataClientTest, QueryDataInPhotoAlbum, TestSize.Level1)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, 2049);
    const std::vector<std::string> columnNames = {PhotoAlbumColumns::ALBUM_CLOUD_ID, PhotoAlbumColumns::ALBUM_NAME};
    const std::string tableName = Media::PhotoAlbumColumns::TABLE;
    std::vector<std::unordered_map<std::string, std::string>> results;
    CloudMediaDataClient cloudMediaDataClient(1, 100);
    int32_t ret = cloudMediaDataClient.QueryData(predicates, columnNames, tableName, results);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(results.size(), 0);
    NativeRdb::AbsRdbPredicates rdbpredicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    rdbpredicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, 2049);
    auto resultSet = rdbStore_->Query(rdbpredicates, columnNames);
    EXPECT_TRUE(resultSet != nullptr);
    int32_t rowCount = 0;
    ret = resultSet->GetRowCount(rowCount);
    EXPECT_EQ(ret, 0);
    EXPECT_GT(rowCount, 0);
    int32_t pos = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string album_cloud_id = GetStringVal(PhotoAlbumColumns::ALBUM_CLOUD_ID, resultSet);
        std::string album_name = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
        EXPECT_LT(pos, results.size());
        EXPECT_EQ(album_cloud_id, results[pos][PhotoAlbumColumns::ALBUM_CLOUD_ID]);
        EXPECT_EQ(album_name, results[pos][PhotoAlbumColumns::ALBUM_NAME]);
        pos++;
    }
    resultSet->Close();
}
}  // namespace OHOS::Media::CloudSync