/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "cloud_media_share_photo_handler_test.h"

#include <string>
#include <vector>

#include "cloud_check_data.h"
#include "cloud_media_data_client.h"
#include "cloud_media_data_handler.h"
#include "cloud_meta_data.h"
#include "cloud_media_sync_const.h"
#include "i_cloud_media_data_handler.h"
#include "json_file_reader.h"
#include "mdk_record.h"
#include "mdk_record_photos_data.h"
#include "media_library_database.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "photos_dao.h"
#include "share_album_cloud_sync_test_utils.h"

using namespace testing::ext;
using namespace OHOS::Media::ORM;
using namespace OHOS::Media::TestUtils;

namespace OHOS::Media::CloudSync {
DatabaseDataMock CloudMediaSharePhotoHandlerTest::dbDataMock_;
static uint64_t g_shellToken = 0;
static MediaLibraryMockNativeToken* mockToken = nullptr;
// 一次下行的共享相册个数(真实数据为 1 个共享相册)
static constexpr int32_t SHARE_ALBUM_COUNT = 1;
// 一次上行查询的记录条数上限
static constexpr int32_t CREATED_RECORDS_QUERY_SIZE = 20;

void CloudMediaSharePhotoHandlerTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest SetUpTestCase";
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    mockToken = new MediaLibraryMockNativeToken("cloudfileservice");

    int32_t errorCode = 0;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore(errorCode);
    int32_t ret = dbDataMock_.SetRdbStore(rdbStore).CheckPoint();
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest CheckPoint ret: " << ret;
    // 先清一次历史运行可能残留的本套数据(含不带 cloud_id 的源相册), 避免污染后续相册归属
    ShareAlbumCloudSyncTestUtils::CleanShareTddData();

    // 共享资产下行前需要先有共享相册, 这里同样通过 inner 层接口下行
    std::vector<int32_t> albumStats{0, 0, 0, 0, 0};
    ret = ShareAlbumCloudSyncTestUtils::DownLinkShareAlbums(albumStats);
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest DownLinkShareAlbums ret:" << ret
                     << ", newAlbum:" << albumStats[StatsIndex::NEW_RECORDS_COUNT];
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(albumStats[StatsIndex::NEW_RECORDS_COUNT], SHARE_ALBUM_COUNT);
    // 共享资产通过 dentry 通路写入媒体库, 与既有用例(OnDentryFileInsert)的约定一致
    ret = ShareAlbumCloudSyncTestUtils::InsertSharePhotosByDentry();
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest InsertSharePhotosByDentry ret:" << ret;
    EXPECT_EQ(ret, E_OK) << "共享资产 dentry 下行失败, ret: " << ret;
    // dentry 通路的相册归属受生产侧相册映射缓存影响, 这里显式建立资产到共享相册的归属
    ret = ShareAlbumCloudSyncTestUtils::BindSharePhotosToShareAlbums();
    EXPECT_EQ(ret, E_OK) << "共享资产绑定共享相册失败, ret: " << ret;
    PhotosPo insertedPhoto;
    EXPECT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, insertedPhoto)) << "共享资产未写入媒体库";
}

void CloudMediaSharePhotoHandlerTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest TearDownTestCase";
    ShareAlbumCloudSyncTestUtils::CleanShareTddData();
    bool ret = dbDataMock_.Rollback();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest TearDownTestCase ret: " << ret;
}

void CloudMediaSharePhotoHandlerTest::SetUp()
{
    GTEST_LOG_(INFO) << "CloudMediaSharePhotoHandlerTest SetUp";
}

void CloudMediaSharePhotoHandlerTest::TearDown()
{}

/**
 * 测试功能: 共享资产下行能否成功落库, 并带上共享标记/属主/共享字段/文件来源.
 * 期望结果: Photos 中存在该 cloudId 的记录, is_shared=1, 共享字段与云侧一致
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhoto_SavedToMediaLibrary, TestSize.Level1)
{
    PhotosPo photo;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, photo))
        << "共享资产未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID;
    EXPECT_EQ(photo.cloudId.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID);
    EXPECT_EQ(photo.isShared.value_or(0), ShareAlbumCloudSyncTestUtils::IS_SHARED_TRUE);
    EXPECT_EQ(photo.shareAlbumOwner.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER);
    EXPECT_EQ(photo.shareOwnerInfo.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_OWNER_INFO);
    EXPECT_EQ(photo.shareDateDay.value_or(0), ShareAlbumCloudSyncTestUtils::SHARE_DATE_DAY);
    EXPECT_EQ(photo.shareGroup.value_or(0), ShareAlbumCloudSyncTestUtils::SHARE_GROUP);
    EXPECT_EQ(photo.fileSourceType.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::FILE_SOURCE_TYPE_MEDIA_SHARE_ALBUM);
    EXPECT_FALSE(photo.data.value_or("").empty()) << "共享资产未落库文件路径";
}

/**
 * 测试功能: 共享资产下行后的归属相册是否为共享相册.
 * 期望结果: 资产的 owner_album_id 指向共享相册(cloud_id/lpath 与云侧一致)
 * 备注: dentry 通路的相册归属受生产侧进程内相册映射缓存影响, 用例在 SetUpTestCase 里已显式绑定,
 *       这里验证绑定后的归属关系符合共享相册的期望.
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhoto_OwnerAlbumIdBoundToShareAlbum, TestSize.Level1)
{
    int32_t albumId = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumId, 0) << "共享相册未下行到媒体库";

    PhotosPo photo;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, photo))
        << "共享资产未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID;
    int32_t boundAlbumId = photo.ownerAlbumId.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX);
    ASSERT_GT(boundAlbumId, 0) << "共享资产未绑定到任何相册";

    PhotoAlbumPo boundAlbum;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumById(boundAlbumId, boundAlbum))
        << "共享资产归属的相册不存在: " << boundAlbumId;
    EXPECT_EQ(boundAlbum.cloudId.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID)
        << "共享资产未绑定到共享相册";
    EXPECT_EQ(boundAlbum.lpath.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_LPATH);
}

/**
 * 测试功能: 共享相册下行后是否处于可上行状态.
 * 期望结果: 绑定共享相册后 dirty 保持同步态、upload_status 已开启, 保证资产能进入上行查询
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhoto_ShareAlbumNotNewDirty, TestSize.Level1)
{
    int32_t albumId = ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID);
    ASSERT_GT(albumId, 0) << "共享相册未下行到媒体库";
    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumById(albumId, album)) << "共享相册不存在";
    // GetCreatedRecords 的 SQL 会过滤掉 dirty=TYPE_NEW(1) 的相册, 否则共享资产永远上不了行
    EXPECT_NE(album.dirty.value_or(0), ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_NEW)
        << "共享相册处于 TYPE_NEW 状态, 名下资产无法上行";
    EXPECT_EQ(album.uploadStatus.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::UPLOAD_STATUS_OPEN) << "共享相册 upload_status 未开启, 名下资产无法上行";
}

/**
 * 测试功能: 共享资产元数据下行(OnFetchRecords)能否把云侧新增资产上报给端云框架.
 * 期望结果: 返回成功, newData 上报新增资产且带共享标记与相册属主
 * 说明: 本用例先于自动归属用例执行(该资产尚未落库, 才能走新增上报分支).
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhotoMeta_ReportsNewAsset, TestSize.Level1)
{
    std::vector<CloudMetaData> newData;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkSharePhotoMeta(newData, stats);
    EXPECT_EQ(ret, E_OK) << "共享资产元数据下行失败, ret: " << ret;
    // 共享资产通路(CloudMediaSharePhotosService)不统计 NEW_RECORDS_COUNT, 因此以 newData 判定新增
    EXPECT_FALSE(newData.empty()) << "共享资产元数据下行未上报新增数据";

    bool found = false;
    for (const auto &metaData : newData) {
        if (metaData.cloudId != ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_CLOUD_ID) {
            continue;
        }
        found = true;
        EXPECT_EQ(metaData.isShared, ShareAlbumCloudSyncTestUtils::IS_SHARED_TRUE);
        EXPECT_EQ(metaData.shareAlbumOwner, ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER);
    }
    EXPECT_TRUE(found) << "元数据下行结果中没有找到共享资产: "
                       << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_CLOUD_ID;
}

/**
 * 测试功能: 共享资产下行后能否【自动】归属到对应的共享相册(覆盖相册映射把 SHARE 相册纳入的新行为).
 * 期望结果: 资产 owner_album_id 指向该共享相册, 且归属相册的 cloud_id/类型都符合共享相册
 * 说明: 按生产顺序先走一次元数据下行(内部会刷新服务端相册映射), 再 dentry 落盘一条新资产,
 *       因此本用例验证的是自动归属, 不依赖 SetUpTestCase 里的显式绑定.
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhoto_AutoBoundToShareAlbum, TestSize.Level1)
{
    // 与生产顺序一致: 先 OnFetchRecords, 再 OnDentryFileInsert
    std::vector<CloudMetaData> newData;
    std::vector<int32_t> metaStats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkSharePhotoMeta(newData, metaStats);
    EXPECT_EQ(ret, E_OK) << "共享资产元数据下行失败, ret: " << ret;

    ret = ShareAlbumCloudSyncTestUtils::InsertSharePhotosByDentryFile(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_JSON);
    EXPECT_EQ(ret, E_OK) << "共享资产 dentry 落盘失败, ret: " << ret;

    PhotosPo photo;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_META_CLOUD_ID, photo)) << "共享资产未写入媒体库";
    int32_t boundAlbumId = photo.ownerAlbumId.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX);
    ASSERT_GT(boundAlbumId, 0) << "共享资产未自动归属到任何相册";

    PhotoAlbumPo album;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetAlbumById(boundAlbumId, album)) << "共享资产归属的相册不存在";
    EXPECT_EQ(album.cloudId.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID)
        << "共享资产未自动归属到共享相册";
    EXPECT_EQ(album.albumType.value_or(0), ShareAlbumCloudSyncTestUtils::ALBUM_TYPE_SHARE)
        << "归属相册不是共享相册";
    EXPECT_EQ(album.lpath.value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_LPATH);
}

/**
 * 测试功能: 共享资产落盘接口(OnDentryFileInsert)的空入参健壮性.
 * 期望结果: 直接返回成功, 不产生任何数据
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_DentryFileInsert_EmptyRecords_ReturnsOk, TestSize.Level1)
{
    std::vector<MDKRecord> records;
    std::vector<std::string> failedRecords;
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeSharePhotoHandler()->OnDentryFileInsert(records, failedRecords);
    EXPECT_EQ(ret, E_OK) << "空入参 OnDentryFileInsert 返回值不符合预期, ret: " << ret;
    EXPECT_EQ(failedRecords.size(), 0);
}

/**
 * 测试功能: 共享资产上行(元数据通路)能否产出带共享字段与相册属主的记录.
 * 期望结果: 上行记录的属性里带 is_shared/share_album_owner/share_owner_info/share_date_day/share_group,
 *           且记录属主(ownerId)为共享相册属主
 * 备注: 这里走元数据上行通路(GetMetaModifiedRecords, dirty=SDIRTY).
 *       文件新建通路(GetCreatedRecords, dirty=TYPE_NEW)在生成上行记录时还会 stat 真实原图与 THM/LCD
 *       缩略图(CloudFileDataConvert::HandleAttachments), 单测环境不具备这些物理文件, 记录会被丢弃;
 *       该通路的文件依赖由既有用例(依赖 ohos_test.xml 推送资源)覆盖.
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, UpLink_GetMetaModifiedRecords_ContainsShareFieldsAndOwnerId,
    TestSize.Level1)
{
    // 前置状态: 资产需已落库且绑定到共享相册, 否则上行查询(JOIN 相册)会漏掉它
    PhotosPo boundPhoto;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID, boundPhoto))
        << "待上行共享资产未写入媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID;
    ASSERT_EQ(boundPhoto.ownerAlbumId.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::GetAlbumIdByCloudId(
            ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_CLOUD_ID)) << "待上行共享资产未绑定到共享相册";

    ShareAlbumCloudSyncTestUtils::PrepareSharePhotoForUpload(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID, ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SDIRTY);
    PhotosPo preparedPhoto;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID, preparedPhoto)) << "待上行共享资产查询失败";
    EXPECT_EQ(preparedPhoto.dirty.value_or(ShareAlbumCloudSyncTestUtils::INVALID_INDEX),
        ShareAlbumCloudSyncTestUtils::DIRTY_TYPE_SDIRTY) << "共享资产未置为待上行状态";

    std::vector<MDKRecord> records;
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeSharePhotoHandler()->GetMetaModifiedRecords(records,
        CREATED_RECORDS_QUERY_SIZE);
    EXPECT_EQ(ret, E_OK) << "GetMetaModifiedRecords 失败, ret: " << ret;

    bool found = false;
    for (const auto &record : records) {
        if (record.GetRecordId() != ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID) {
            continue;
        }
        found = true;
        MDKRecordPhotosData photosData = MDKRecordPhotosData(record);
        EXPECT_EQ(photosData.GetPhotoIsShared().value_or(0), ShareAlbumCloudSyncTestUtils::IS_SHARED_TRUE);
        EXPECT_EQ(photosData.GetShareAlbumOwner().value_or(""), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER);
        EXPECT_EQ(photosData.GetPhotoShareOwnerInfo().value_or(""),
            ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_OWNER_INFO);
        EXPECT_EQ(photosData.GetPhotoShareDateDay().value_or(0), ShareAlbumCloudSyncTestUtils::SHARE_DATE_DAY);
        EXPECT_EQ(photosData.GetPhotoShareGroup().value_or(0), ShareAlbumCloudSyncTestUtils::SHARE_GROUP);
        EXPECT_EQ(record.GetOwnerId(), ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER)
            << "上行记录的相册属主不正确";
    }
    EXPECT_TRUE(found) << "上行记录中没有找到共享资产: "
                       << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_UPLINK_CLOUD_ID;
}

/**
 * 测试功能: 共享资产上行对账(GetCheckRecords)能否把本地共享资产信息回带给云侧.
 * 期望结果: 能查到该共享资产, 且路径/文件名等基础字段与本地一致
 * 备注: 服务端 GetCheckRecords 的投影(PULL_QUERY_COLUMNS)未包含 share_album_owner,
 *       因此本用例不断言 shareAlbumOwner; 若后续补齐投影, 可在此追加该断言.
 *       文件名由生产按本地文件路径(data)取名(CloudMediaPhotoServiceProcessor::GetPhotosDtos 调用
 *       GetParentPathAndFilename(photosDto.data, ...)), 真实数据下与展示名(display_name)不同,
 *       因此这里断言 data 路径中的文件名.
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, UpLink_GetCheckRecords_ShareAssetReconciled, TestSize.Level1)
{
    std::vector<std::string> cloudIds = {ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID};
    std::unordered_map<std::string, CloudCheckData> checkRecords;
    int32_t ret = ShareAlbumCloudSyncTestUtils::MakeSharePhotoHandler()->GetCheckRecords(cloudIds, checkRecords);
    EXPECT_EQ(ret, E_OK) << "GetCheckRecords 失败, ret: " << ret;
    ASSERT_TRUE(checkRecords.count(ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID) > 0)
        << "对账结果中没有找到共享资产";

    PhotosPo photo;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, photo)) << "共享资产未下行到媒体库";
    const CloudCheckData &checkData = checkRecords[ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID];
    EXPECT_EQ(checkData.cloudId, ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID);
    EXPECT_EQ(checkData.fileName, ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_DATA_FILE_NAME)
        << "对账文件名应为本地文件路径中的文件名";
    EXPECT_EQ(checkData.size, photo.size.value_or(0));
}

/**
 * 测试功能: 共享资产下载元数据能否带回共享相册的属主.
 * 期望结果: 能查到该共享资产, 且 CloudMetaData.shareAlbumOwner 与云侧一致
 * 备注: 下载通路的投影未包含 is_shared, 因此不断言 CloudMetaData.isShared.
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, UpLink_GetDownloadAsset_ReturnsShareAlbumOwner, TestSize.Level1)
{
    PhotosDao photosDao;
    std::vector<PhotosPo> photosList = photosDao.QueryPhotosByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID);
    ASSERT_FALSE(photosList.empty()) << "共享资产未下行到媒体库";
    std::vector<std::string> uris;
    for (auto &photo : photosList) {
        uris.emplace_back(photosDao.BuildUriByPhoto(photo));
    }

    CloudMediaDataClient client(ShareAlbumCloudSyncTestUtils::CLOUD_TYPE, ShareAlbumCloudSyncTestUtils::USER_ID,
        ShareAlbumCloudSyncTestUtils::SCENE_TYPE_SHARE);
    std::vector<CloudMetaData> metaDataVec;
    int32_t ret = client.GetDownloadAsset(uris, metaDataVec);
    EXPECT_EQ(ret, E_OK) << "GetDownloadAsset 失败, ret: " << ret;
    ASSERT_FALSE(metaDataVec.empty()) << "下载资产元数据为空";

    bool found = false;
    for (const auto &metaData : metaDataVec) {
        if (metaData.cloudId != ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID) {
            continue;
        }
        found = true;
        EXPECT_EQ(metaData.shareAlbumOwner, ShareAlbumCloudSyncTestUtils::SHARE_ALBUM_OWNER);
    }
    EXPECT_TRUE(found) << "下载资产元数据中没有找到共享资产";
}

/**
 * 测试功能: 云侧删除共享资产后, 本地媒体库记录是否被清理.
 * 期望结果: Photos 中不再存在该 cloudId 的记录
 */
HWTEST_F(CloudMediaSharePhotoHandlerTest, DownLink_SharePhoto_Deleted_RemovesLocalAsset, TestSize.Level1)
{
    PhotosPo photo;
    ASSERT_TRUE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, photo))
        << "共享资产未下行到媒体库: " << ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID;

    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    int32_t ret = ShareAlbumCloudSyncTestUtils::DownLinkDeletedSharePhoto(stats);
    EXPECT_EQ(ret, E_OK) << "云侧删除下行失败, ret: " << ret;
    EXPECT_FALSE(ShareAlbumCloudSyncTestUtils::GetPhotoByCloudId(
        ShareAlbumCloudSyncTestUtils::SHARE_PHOTO_CLOUD_ID, photo)) << "云侧已删除的共享资产未从媒体库移除";
}
}  // namespace OHOS::Media::CloudSync
