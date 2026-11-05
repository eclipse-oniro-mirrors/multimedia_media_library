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

#include "media_assets_delete_service_test.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"

#include <cstdlib>

#include "media_albums_controller_service.h"
#include "create_album_vo.h"

#define private public
#define protected public
#include "media_assets_delete_service.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::Common;
using namespace OHOS::Media::ORM;
namespace OHOS::Media::CloudSync {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;
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

void CloudMediaAssetsDeleteTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    bool ret = MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    ASSERT_TRUE(ret);
}

void CloudMediaAssetsDeleteTest::TearDownTestCase()
{
    system("rm -rf /storage/cloud/files/*");
    // drop table
    bool ret = MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    ASSERT_TRUE(ret);
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("CloudMediaPhotoDeleteTest is finish");
}

void CloudMediaAssetsDeleteTest::SetUp()
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

void CloudMediaAssetsDeleteTest::TearDown() {}

HWTEST_F(CloudMediaAssetsDeleteTest, CloudMediaAssetsDeleteLocalAssets_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("start CloudMediaAssetsDeleteLocalAssets_Test");
    std::vector<std::string> fileIds;
    auto service = make_shared<MediaAssetsDeleteService>();
    ASSERT_NE(service, nullptr);

    int32_t ret = service->DeleteLocalAssets(fileIds);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    int32_t albumId = CreateAlbum("summer");
    ASSERT_GT(albumId, 0);

    int32_t albumId1 = CreateAlbum("winter");
    ASSERT_GT(albumId1, 0);
    MEDIA_INFO_LOG("end CloudMediaAssetsDeleteLocalAssets_Test");
}

// EraseCloudInfo
HWTEST_F(CloudMediaAssetsDeleteTest, EraseCloudInfo_ResetsAllCloudFields, TestSize.Level1)
{
    /**
     * @tc.name: EraseCloudInfo_ResetsAllCloudFields
     * @tc.desc: EraseCloudInfo resets cloudId, dirty to TYPE_NEW, position to LOCAL, cloudVersion
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.cloudId = "cloud_123";
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_MDIRTY);
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photo.cloudVersion = 100;

    int32_t ret = service.EraseCloudInfo(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.cloudId.has_value());
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_NEW));
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL));
    EXPECT_FALSE(photo.cloudVersion.has_value());
}

// ResetFileId
HWTEST_F(CloudMediaAssetsDeleteTest, ResetFileId_ClearsFileId, TestSize.Level1)
{
    /**
     * @tc.name: ResetFileId_ClearsFileId
     * @tc.desc: ResetFileId resets the optional fileId to nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 42;

    int32_t ret = service.ResetFileId(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.fileId.has_value());
}

// ResetVirtualPath
HWTEST_F(CloudMediaAssetsDeleteTest, ResetVirtualPath_ClearsVirtualPath, TestSize.Level1)
{
    /**
     * @tc.name: ResetVirtualPath_ClearsVirtualPath
     * @tc.desc: ResetVirtualPath resets the optional virtualPath to nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.virtualPath = "/some/virtual/path";

    int32_t ret = service.ResetVirtualPath(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.virtualPath.has_value());
}

HWTEST_F(CloudMediaAssetsDeleteTest, ResetVirtualPath_AlreadyEmpty, TestSize.Level1)
{
    /**
     * @tc.name: ResetVirtualPath_AlreadyEmpty
     * @tc.desc: ResetVirtualPath on already-empty virtualPath still succeeds
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo; // virtualPath is nullopt by default

    int32_t ret = service.ResetVirtualPath(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.virtualPath.has_value());
}

// SetDateTrashed
HWTEST_F(CloudMediaAssetsDeleteTest, SetDateTrashed_SetsCorrectValue, TestSize.Level1)
{
    /**
     * @tc.name: SetDateTrashed_SetsCorrectValue
     * @tc.desc: SetDateTrashed sets dateTrashed to the given timestamp
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dateTrashed = 0;

    int32_t ret = service.SetDateTrashed(photo, 1234567890LL);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.dateTrashed.value_or(0), 1234567890LL);
}

// SetPosition
HWTEST_F(CloudMediaAssetsDeleteTest, SetPosition_SetsCloud, TestSize.Level1)
{
    /**
     * @tc.name: SetPosition_SetsCloud
     * @tc.desc: SetPosition sets position to CLOUD value
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);

    int32_t ret = service.SetPosition(photo, static_cast<int32_t>(PhotoPositionType::CLOUD));
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.position.value_or(0), static_cast<int32_t>(PhotoPositionType::CLOUD));
}

HWTEST_F(CloudMediaAssetsDeleteTest, SetPosition_SetsLocal, TestSize.Level1)
{
    /**
     * @tc.name: SetPosition_SetsLocal
     * @tc.desc: SetPosition sets position to LOCAL value
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);

    int32_t ret = service.SetPosition(photo, static_cast<int32_t>(PhotoPositionType::LOCAL));
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.position.value_or(0), static_cast<int32_t>(PhotoPositionType::LOCAL));
}

// SetFilePath
HWTEST_F(CloudMediaAssetsDeleteTest, SetFilePath_SetsDataField, TestSize.Level1)
{
    /**
     * @tc.name: SetFilePath_SetsDataField
     * @tc.desc: SetFilePath sets the data field to the given path
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;

    int32_t ret = service.SetFilePath(photo, "/storage/media/local/files/Photo/test.jpg");
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.data.value_or(""), "/storage/media/local/files/Photo/test.jpg");
}

HWTEST_F(CloudMediaAssetsDeleteTest, SetFilePath_EmptyPath, TestSize.Level1)
{
    /**
     * @tc.name: SetFilePath_EmptyPath
     * @tc.desc: SetFilePath sets data field to empty string
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.data = "/original/path";

    int32_t ret = service.SetFilePath(photo, "");
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.data.value_or("x"), "");
}

// SetFileId
HWTEST_F(CloudMediaAssetsDeleteTest, SetFileId_SetsCorrectId, TestSize.Level1)
{
    /**
     * @tc.name: SetFileId_SetsCorrectId
     * @tc.desc: SetFileId sets fileId to the given integer value
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;

    int32_t ret = service.SetFileId(photo, 99);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.fileId.value_or(0), 99);
}

// ClearCloudInfo
HWTEST_F(CloudMediaAssetsDeleteTest, ClearCloudInfo_ResetsCloudFields, TestSize.Level1)
{
    /**
     * @tc.name: ClearCloudInfo_ResetsCloudFields
     * @tc.desc: ClearCloudInfo resets cloudId, dirty to TYPE_NEW, position to LOCAL, cloudVersion
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.cloudId = "abc123";
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_SDIRTY);
    photo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    photo.cloudVersion = 42;

    int32_t ret = service.ClearCloudInfo(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.cloudId.has_value());
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_NEW));
    EXPECT_EQ(photo.position.value_or(-1), static_cast<int32_t>(PhotoPositionType::LOCAL));
    EXPECT_FALSE(photo.cloudVersion.has_value());
}

// ResetNullableFields
HWTEST_F(CloudMediaAssetsDeleteTest, ResetNullableFields_EmptyStringsResetToNullopt, TestSize.Level1)
{
    /**
     * @tc.name: ResetNullableFields_EmptyStringsResetToNullopt
     * @tc.desc: Empty burstKey, originalAssetCloudId, relativePath, userComment are reset to nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.burstKey = "";
    photo.originalAssetCloudId = "";
    photo.relativePath = "";
    photo.userComment = "";
    photo.latitude = 0.0;
    photo.longitude = 0.0;

    int32_t ret = service.ResetNullableFields(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.burstKey.has_value());
    EXPECT_FALSE(photo.originalAssetCloudId.has_value());
    EXPECT_FALSE(photo.relativePath.has_value());
    EXPECT_FALSE(photo.userComment.has_value());
    EXPECT_FALSE(photo.latitude.has_value());
    EXPECT_FALSE(photo.longitude.has_value());
}

HWTEST_F(CloudMediaAssetsDeleteTest, ResetNullableFields_NonEmptyPreserved, TestSize.Level1)
{
    /**
     * @tc.name: ResetNullableFields_NonEmptyPreserved
     * @tc.desc: Non-empty burstKey, relativePath, userComment are preserved (not reset)
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.burstKey = "burst_abc";
    photo.originalAssetCloudId = "cloud_xyz";
    photo.relativePath = "/some/path";
    photo.userComment = "hello";
    photo.latitude = 39.9;
    photo.longitude = 116.4;

    int32_t ret = service.ResetNullableFields(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(photo.burstKey.has_value());
    EXPECT_EQ(photo.burstKey.value(), "burst_abc");
    EXPECT_TRUE(photo.originalAssetCloudId.has_value());
    EXPECT_EQ(photo.originalAssetCloudId.value(), "cloud_xyz");
    EXPECT_TRUE(photo.relativePath.has_value());
    EXPECT_EQ(photo.relativePath.value(), "/some/path");
    EXPECT_TRUE(photo.userComment.has_value());
    EXPECT_EQ(photo.userComment.value(), "hello");
    EXPECT_TRUE(photo.latitude.has_value());
    EXPECT_EQ(photo.latitude.value(), 39.9);
    EXPECT_TRUE(photo.longitude.has_value());
    EXPECT_EQ(photo.longitude.value(), 116.4);
}

HWTEST_F(CloudMediaAssetsDeleteTest, ResetNullableFields_MixedLatLon, TestSize.Level1)
{
    /**
     * @tc.name: ResetNullableFields_MixedLatLon
     * @tc.desc: latitude=0 with longitude=non-zero keeps both; latitude=0 with longitude=0 resets both
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.latitude = 0.0;
    photo.longitude = 116.4; // non-zero

    int32_t ret = service.ResetNullableFields(photo);
    EXPECT_EQ(ret, E_OK);
    // latitude=0 but longitude!=0, condition not met, so not reset
    EXPECT_TRUE(photo.latitude.has_value());
    EXPECT_TRUE(photo.longitude.has_value());
}

// ResetFileSourceType
HWTEST_F(CloudMediaAssetsDeleteTest, ResetFileSourceType_SetsToMedia, TestSize.Level1)
{
    /**
     * @tc.name: ResetFileSourceType_SetsToMedia
     * @tc.desc: ResetFileSourceType sets fileSourceType to MEDIA (0)
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);

    int32_t ret = service.ResetFileSourceType(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.fileSourceType.value_or(-1), static_cast<int32_t>(FileSourceType::MEDIA));
}

// SetMdirty
HWTEST_F(CloudMediaAssetsDeleteTest, SetMdirty_SyncedBecomesMdirty, TestSize.Level1)
{
    /**
     * @tc.name: SetMdirty_SyncedBecomesMdirty
     * @tc.desc: When dirty=TYPE_SYNCED, SetMdirty sets dirty to TYPE_MDIRTY
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_SYNCED);

    int32_t ret = service.SetMdirty(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
}

HWTEST_F(CloudMediaAssetsDeleteTest, SetMdirty_SdirtyBecomesMdirty, TestSize.Level1)
{
    /**
     * @tc.name: SetMdirty_SdirtyBecomesMdirty
     * @tc.desc: When dirty=TYPE_SDIRTY, SetMdirty sets dirty to TYPE_MDIRTY
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_SDIRTY);

    int32_t ret = service.SetMdirty(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
}

HWTEST_F(CloudMediaAssetsDeleteTest, SetMdirty_TdirtyBecomesMdirty, TestSize.Level1)
{
    /**
     * @tc.name: SetMdirty_TdirtyBecomesMdirty
     * @tc.desc: When dirty=TYPE_TDIRTY, SetMdirty sets dirty to TYPE_MDIRTY
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_TDIRTY);

    int32_t ret = service.SetMdirty(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
}

HWTEST_F(CloudMediaAssetsDeleteTest, SetMdirty_NewStaysAsNew, TestSize.Level1)
{
    /**
     * @tc.name: SetMdirty_NewStaysAsNew
     * @tc.desc: When dirty=TYPE_NEW, TryGetMdirty returns TYPE_NEW (no conversion), dirty stays
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);

    int32_t ret = service.SetMdirty(photo);
    EXPECT_EQ(ret, E_OK);
    // TryGetMdirty returns TYPE_NEW (1) when dirty=TYPE_NEW, not converting to MDIRTY
    EXPECT_EQ(photo.dirty.value_or(-1), static_cast<int32_t>(DirtyType::TYPE_NEW));
}

// GenerateUuid
HWTEST_F(CloudMediaAssetsDeleteTest, GenerateUuid_ReturnsValidFormat, TestSize.Level1)
{
    /**
     * @tc.name: GenerateUuid_ReturnsValidFormat
     * @tc.desc: GenerateUuid returns a 36-char UUID string with dashes at positions 8,13,18,23
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    std::string uuid = service.GenerateUuid();
    EXPECT_EQ(uuid.length(), 36u);
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
}

HWTEST_F(CloudMediaAssetsDeleteTest, GenerateUuid_TwoCallsAreDifferent, TestSize.Level1)
{
    /**
     * @tc.name: GenerateUuid_TwoCallsAreDifferent
     * @tc.desc: Two consecutive GenerateUuid calls produce different UUIDs
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    std::string uuid1 = service.GenerateUuid();
    std::string uuid2 = service.GenerateUuid();
    EXPECT_NE(uuid1, uuid2);
}

// ResetSouthDeviceType
HWTEST_F(CloudMediaAssetsDeleteTest, ResetSouthDeviceType_ClearsField, TestSize.Level1)
{
    /**
     * @tc.name: ResetSouthDeviceType_ClearsField
     * @tc.desc: ResetSouthDeviceType resets southDeviceType to nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.southDeviceType = 5;

    int32_t ret = service.ResetSouthDeviceType(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(photo.southDeviceType.has_value());
}

// ResetUniqueId
HWTEST_F(CloudMediaAssetsDeleteTest, ResetUniqueId_SetsToMinusOne, TestSize.Level1)
{
    /**
     * @tc.name: ResetUniqueId_SetsToMinusOne
     * @tc.desc: ResetUniqueId sets uniqueId to "-1"
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.uniqueId = "original_id";

    int32_t ret = service.ResetUniqueId(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.uniqueId.value_or(""), "-1");
}

HWTEST_F(CloudMediaAssetsDeleteTest, ResetUniqueId_FromNullopt, TestSize.Level1)
{
    /**
     * @tc.name: ResetUniqueId_FromNullopt
     * @tc.desc: ResetUniqueId sets uniqueId to "-1" even when previously nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;

    int32_t ret = service.ResetUniqueId(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photo.uniqueId.value_or(""), "-1");
}

// ResetTransCode
HWTEST_F(CloudMediaAssetsDeleteTest, ResetTransCode_RemovesAllTranscodeKeys, TestSize.Level1)
{
    /**
     * @tc.name: ResetTransCode_RemovesAllTranscodeKeys
     * @tc.desc: ResetTransCode removes PHOTO_TRANSCODE_TIME, PHOTO_TRANS_CODE_FILE_SIZE,
     *           PHOTO_EXIST_COMPATIBLE_DUPLICATE from attributes map
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.attributes[PhotoColumn::PHOTO_TRANSCODE_TIME] = "1000";
    photo.attributes[PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE] = "2048";
    photo.attributes[PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE] = "1";
    photo.attributes["other_key"] = "keep";

    int32_t ret = service.ResetTransCode(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(photo.attributes.find(PhotoColumn::PHOTO_TRANSCODE_TIME) == photo.attributes.end());
    EXPECT_TRUE(photo.attributes.find(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE) == photo.attributes.end());
    EXPECT_TRUE(photo.attributes.find(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE) == photo.attributes.end());
    // other_key should be preserved
    EXPECT_TRUE(photo.attributes.find("other_key") != photo.attributes.end());
}

HWTEST_F(CloudMediaAssetsDeleteTest, ResetTransCode_EmptyAttributesNoOp, TestSize.Level1)
{
    /**
     * @tc.name: ResetTransCode_EmptyAttributesNoOp
     * @tc.desc: ResetTransCode on empty attributes map succeeds without error
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo; // empty attributes

    int32_t ret = service.ResetTransCode(photo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(photo.attributes.empty());
}

// StoreThumbnailAndEditSize (single PhotosPo)
HWTEST_F(CloudMediaAssetsDeleteTest, StoreThumbnailAndEditSize_InvalidFileId, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_InvalidFileId
     * @tc.desc: Returns E_INVALID_VALUES when fileId is 0 or data is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 0;
    photo.data = "/some/path";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaAssetsDeleteTest, StoreThumbnailAndEditSize_EmptyData, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_EmptyData
     * @tc.desc: Returns E_INVALID_VALUES when fileId is valid but data is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 10;
    photo.data = "";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

HWTEST_F(CloudMediaAssetsDeleteTest, StoreThumbnailAndEditSize_NoFileId, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_NoFileId
     * @tc.desc: Returns E_INVALID_VALUES when fileId is nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.data = "/some/path";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

// StoreThumbnailAndEditSize (with optional)
HWTEST_F(CloudMediaAssetsDeleteTest, StoreThumbnailAndEditSize_OptionalEmpty, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_OptionalEmpty
     * @tc.desc: Returns E_OK immediately when targetPhotoInfoOp is nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;

    int32_t ret = service.StoreThumbnailAndEditSize(photo, targetOp);
    EXPECT_EQ(ret, E_OK);
}

// BuildMediaFilePath
HWTEST_F(CloudMediaAssetsDeleteTest, BuildMediaFilePath_EmptyDisplayName, TestSize.Level1)
{
    /**
     * @tc.name: BuildMediaFilePath_EmptyDisplayName
     * @tc.desc: Returns E_FILE_NAME_INVALID when displayName is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.displayName = "";
    photo.mediaType = 1;
    std::string targetPath;

    int32_t ret = service.BuildMediaFilePath(photo, targetPath);
    EXPECT_EQ(ret, E_FILE_NAME_INVALID);
}

HWTEST_F(CloudMediaAssetsDeleteTest, BuildMediaFilePath_NoDisplayName, TestSize.Level1)
{
    /**
     * @tc.name: BuildMediaFilePath_NoDisplayName
     * @tc.desc: Returns E_FILE_NAME_INVALID when displayName is nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.mediaType = 1;
    std::string targetPath;

    int32_t ret = service.BuildMediaFilePath(photo, targetPath);
    EXPECT_EQ(ret, E_FILE_NAME_INVALID);
}

// Constructor with isCloudPullData
HWTEST_F(CloudMediaAssetsDeleteTest, Constructor_DefaultIsCloudPullDataFalse, TestSize.Level1)
{
    /**
     * @tc.name: Constructor_DefaultIsCloudPullDataFalse
     * @tc.desc: Default constructor sets isCloudPullData_ to false
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    EXPECT_FALSE(service.isCloudPullData_);
}

HWTEST_F(CloudMediaAssetsDeleteTest, Constructor_ExplicitIsCloudPullDataTrue, TestSize.Level1)
{
    /**
     * @tc.name: Constructor_ExplicitIsCloudPullDataTrue
     * @tc.desc: Explicit constructor sets isCloudPullData_ to true
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service(true);
    EXPECT_TRUE(service.isCloudPullData_);
}

// CopyAndMoveLocalAssetToTrash precondition checks
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveLocalAssetToTrash_NullPhotoRefresh, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveLocalAssetToTrash_NullPhotoRefresh
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.CopyAndMoveLocalAssetToTrash(photo, targetOp, nullRefresh);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveLocalAssetToTrash_NotLocalAndCloud, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveLocalAssetToTrash_NotLocalAndCloud
     * @tc.desc: Returns E_OK early when position is not LOCAL_AND_CLOUD
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveLocalAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

// CopyAndMoveCloudAssetToTrash precondition checks
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveCloudAssetToTrash_NullPhotoRefresh, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveCloudAssetToTrash_NullPhotoRefresh
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.CopyAndMoveCloudAssetToTrash(photo, targetOp, nullRefresh);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveCloudAssetToTrash_NotLocalAndCloud, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveCloudAssetToTrash_NotLocalAndCloud
     * @tc.desc: Returns E_OK early when position is not LOCAL_AND_CLOUD
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveCloudAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

// CheckAndFindBurstAssets
HWTEST_F(CloudMediaAssetsDeleteTest, CheckAndFindBurstAssets_NotInTrash, TestSize.Level1)
{
    /**
     * @tc.name: CheckAndFindBurstAssets_NotInTrash
     * @tc.desc: Returns E_INVALID_MODE when dateTrashed != 0
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dateTrashed = 100; // already trashed
    photo.burstKey = "some_key";
    std::optional<PhotosPo> coverOp;
    std::vector<PhotosPo> burstAssets;

    int32_t ret = service.CheckAndFindBurstAssets(photo, coverOp, burstAssets);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

HWTEST_F(CloudMediaAssetsDeleteTest, CheckAndFindBurstAssets_EmptyBurstKey, TestSize.Level1)
{
    /**
     * @tc.name: CheckAndFindBurstAssets_EmptyBurstKey
     * @tc.desc: Returns E_INVALID_MODE when burstKey is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dateTrashed = 0;
    photo.burstKey = "";
    std::optional<PhotosPo> coverOp;
    std::vector<PhotosPo> burstAssets;

    int32_t ret = service.CheckAndFindBurstAssets(photo, coverOp, burstAssets);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

HWTEST_F(CloudMediaAssetsDeleteTest, CheckAndFindBurstAssets_NoBurstKey, TestSize.Level1)
{
    /**
     * @tc.name: CheckAndFindBurstAssets_NoBurstKey
     * @tc.desc: Returns E_INVALID_MODE when burstKey is nullopt
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.dateTrashed = 0;
    std::optional<PhotosPo> coverOp;
    std::vector<PhotosPo> burstAssets;

    int32_t ret = service.CheckAndFindBurstAssets(photo, coverOp, burstAssets);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// DeleteLocalAssetSingle / DeleteCloudAssetSingle precondition
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetSingle_NullPhotoRefresh, TestSize.Level1)
{
    /**
     * @tc.name: DeleteLocalAssetSingle_NullPhotoRefresh
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.DeleteLocalAssetSingle(photo, targetOp, nullRefresh);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

HWTEST_F(CloudMediaAssetsDeleteTest, DeleteCloudAssetSingle_NullPhotoRefresh, TestSize.Level1)
{
    /**
     * @tc.name: DeleteCloudAssetSingle_NullPhotoRefresh
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.DeleteCloudAssetSingle(photo, targetOp, nullRefresh);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

// CopyAndMoveMediaLocalAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveMediaLocalAssetToTrash_NotMediaFile, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveMediaLocalAssetToTrash_NotMediaFile
     * @tc.desc: Returns E_INVALID_MODE when fileSourceType is not MEDIA
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveMediaLocalAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveMediaLocalAssetToTrash_AlreadyTrashed, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveMediaLocalAssetToTrash_AlreadyTrashed
     * @tc.desc: Returns E_INVALID_MODE when dateTrashed != 0
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photo.dateTrashed = 100; // already trashed
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveMediaLocalAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CopyAndMoveLakeLocalAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveLakeLocalAssetToTrash_IsMediaFile, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveLakeLocalAssetToTrash_IsMediaFile
     * @tc.desc: Returns E_INVALID_MODE when asset is media file (not lake)
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA); // media, not lake
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveLakeLocalAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CopyAndMoveMediaCloudAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveMediaCloudAssetToTrash_NotMediaFile, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveMediaCloudAssetToTrash_NotMediaFile
     * @tc.desc: Returns E_INVALID_MODE when fileSourceType is not MEDIA
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER);
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveMediaCloudAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CopyAndMoveLakeCloudAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveLakeCloudAssetToTrash_IsMediaFile, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveLakeCloudAssetToTrash_IsMediaFile
     * @tc.desc: Returns E_INVALID_MODE when asset is media file (not lake)
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveLakeCloudAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CopyAndMoveFileManagerCloudAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveFileManagerCloudAssetToTrash_NotFileManager, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveFileManagerCloudAssetToTrash_NotFileManager
     * @tc.desc: Returns E_INVALID_MODE when fileSourceType is not FILE_MANAGER
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveFileManagerCloudAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CopyAndMoveFileManagerLocalAssetToTrash precondition
HWTEST_F(CloudMediaAssetsDeleteTest, CopyAndMoveFileManagerLocalAssetToTrash_NotFileManager, TestSize.Level1)
{
    /**
     * @tc.name: CopyAndMoveFileManagerLocalAssetToTrash_NotFileManager
     * @tc.desc: Returns E_INVALID_MODE when fileSourceType is not FILE_MANAGER
     * @tc.type: FUNCTION
     */
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    photo.dateTrashed = 0;
    std::optional<PhotosPo> targetOp;
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CopyAndMoveFileManagerLocalAssetToTrash(photo, targetOp, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_EmptyFileIds
 * @tc.desc: Empty input produces an empty result array without out-of-range access
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetsAndGetUris_EmptyFileIds, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    std::vector<std::string> fileIds;
    std::vector<AssetUriResult> results;

    int32_t ret = service.DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(results.empty());
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_AssetsNotExist
 * @tc.desc: Returns E_OK with one failed result per input when the assets do not exist
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetsAndGetUris_AssetsNotExist, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    std::vector<std::string> fileIds = { "1001", "1002", "1003" };
    std::vector<AssetUriResult> results;

    int32_t ret = service.DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(results.size(), fileIds.size());
    for (size_t i = 0; i < fileIds.size(); i++) {
        EXPECT_EQ(results[i].originalFileId, fileIds[i]);
        EXPECT_FALSE(results[i].isSuccess);
        EXPECT_TRUE(results[i].resultUri.empty());
    }
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_DuplicateFileIds
 * @tc.desc: Duplicated fileIds still produce one result per input without merging or dropping items
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetsAndGetUris_DuplicateFileIds, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    std::vector<std::string> fileIds = { "1004", "1004" };
    std::vector<AssetUriResult> results;

    int32_t ret = service.DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].originalFileId, "1004");
    EXPECT_EQ(results[1].originalFileId, "1004");
}

/*
 * @tc.name: BatchCopyAndMoveLocalAssetToTrash_LocalAsset
 * @tc.desc: A local-only asset is not copied, the original uri is recorded
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, BatchCopyAndMoveLocalAssetToTrash_LocalAsset, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 100;
    photo.data = "/storage/cloud/files/Photo/16/IMG_100.jpg";
    photo.displayName = "IMG_100.jpg";
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
    photo.burstCoverLevel = 1;
    photo.dateTrashed = 0;
    std::vector<PhotosPo> photosList = { photo };
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> targetUriMap;

    int32_t ret = service.BatchCopyAndMoveLocalAssetToTrash(photosList, targetFileIds, targetUriMap);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(targetFileIds.size(), 1);
    EXPECT_EQ(targetFileIds[0], photo.BuildFileUri());
    ASSERT_EQ(targetUriMap.count("100"), 1);
    EXPECT_EQ(targetUriMap["100"], photo.BuildFileUri());
}

/*
 * @tc.name: BatchCopyAndMoveLocalAssetToTrash_CloudOnlyAsset
 * @tc.desc: A cloud-only asset is skipped and produces no result and no trash target
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, BatchCopyAndMoveLocalAssetToTrash_CloudOnlyAsset, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 101;
    photo.cloudId = "cloud_101";
    photo.data = "/storage/cloud/files/Photo/16/IMG_101.jpg";
    photo.displayName = "IMG_101.jpg";
    photo.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW);
    photo.burstCoverLevel = 1;
    photo.dateTrashed = 0;
    std::vector<PhotosPo> photosList = { photo };
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> targetUriMap;

    int32_t ret = service.BatchCopyAndMoveLocalAssetToTrash(photosList, targetFileIds, targetUriMap);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(targetFileIds.empty());
    EXPECT_TRUE(targetUriMap.empty());
}

/*
 * @tc.name: BatchCopyAndMoveLocalAssetToTrash_BurstMember
 * @tc.desc: A non-cover burst member is skipped, only the cover is processed
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, BatchCopyAndMoveLocalAssetToTrash_BurstMember, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 102;
    photo.data = "/storage/cloud/files/Photo/16/IMG_102.jpg";
    photo.displayName = "IMG_102.jpg";
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    photo.burstCoverLevel = 2;  // member
    photo.burstKey = "burst_key_102";
    photo.dateTrashed = 0;
    std::vector<PhotosPo> photosList = { photo };
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> targetUriMap;

    int32_t ret = service.BatchCopyAndMoveLocalAssetToTrash(photosList, targetFileIds, targetUriMap);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(targetFileIds.empty());
    EXPECT_TRUE(targetUriMap.empty());
}

/*
 * @tc.name: BatchCopyAndMoveLocalAssetToTrash_FileDirtyAsset
 * @tc.desc: A FDIRTY asset goes to trash with its original uri, no local copy is made
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, BatchCopyAndMoveLocalAssetToTrash_FileDirtyAsset, TestSize.Level1)
{
    MediaAssetsDeleteService service;
    PhotosPo photo;
    photo.fileId = 103;
    photo.data = "/storage/cloud/files/Photo/16/IMG_103.jpg";
    photo.displayName = "IMG_103.jpg";
    photo.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    photo.dirty = static_cast<int32_t>(DirtyType::TYPE_FDIRTY);
    photo.burstCoverLevel = 1;
    photo.dateTrashed = 0;
    std::vector<PhotosPo> photosList = { photo };
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> targetUriMap;

    int32_t ret = service.BatchCopyAndMoveLocalAssetToTrash(photosList, targetFileIds, targetUriMap);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(targetFileIds.size(), 1);
    EXPECT_EQ(targetFileIds[0], photo.BuildFileUri());
    ASSERT_EQ(targetUriMap.count("103"), 1);
    EXPECT_EQ(targetUriMap["103"], photo.BuildFileUri());
}

static int32_t InsertPhotoAssetForGetUrisCase(const std::string &title)
{
    const std::string data = "/storage/cloud/files/Photo/16/IMG_" + title + ".jpg";
    system("mkdir -p /storage/cloud/files/Photo/16");
    system(("touch " + data).c_str());
    std::string sql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " +
        MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " +
        MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME +
        ", " + MediaColumn::MEDIA_DATE_ADDED + ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " +
        MediaColumn::MEDIA_DATE_TAKEN + ", " + MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV +
        ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " + MediaColumn::MEDIA_HIDDEN + ") VALUES('" + data +
        "', 175258, '" + title + "', '" + title + ".jpg', 1, 'com.ohos.camera', '相机', 1501924205218, 0, "
        "1501924205, 0, 0, 0, 0)";
    int32_t ret = g_rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, 0, "insert photo failed, ret:%{public}d", ret);

    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_TITLE, title);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query asset failed, title:%{public}s", title.c_str());
        return 0;
    }
    int32_t fileId = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    }
    resultSet->Close();
    return fileId;
}

static int64_t QueryDateTrashedForGetUrisCase(int32_t fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns = { MediaColumn::MEDIA_DATE_TRASHED };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query date trashed failed, fileId:%{public}d", fileId);
        return -1;
    }
    int64_t dateTrashed = -1;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    }
    resultSet->Close();
    return dateTrashed;
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_LocalAsset_ReturnsMap
 * @tc.desc: Positive service case, a local-only asset is deleted locally and the map holds its uri
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetsAndGetUris_LocalAsset_ReturnsMap, TestSize.Level1)
{
    const std::string title = "svc_map_pic";
    int32_t assetId = InsertPhotoAssetForGetUrisCase(title);
    ASSERT_GT(assetId, 0);
    std::string expectedUri = "file://media/Photo/" + std::to_string(assetId) + "/IMG_" + title + "/" + title + ".jpg";

    auto service = make_shared<MediaAssetsDeleteService>();
    std::vector<std::string> fileIds = { std::to_string(assetId) };
    std::unordered_map<std::string, std::string> targetUriMap;

    int32_t ret = service->DeleteLocalAssetsInternal(fileIds, targetUriMap);
    EXPECT_GE(ret, 0);
    ASSERT_EQ(targetUriMap.count(std::to_string(assetId)), 1);
    EXPECT_EQ(targetUriMap[std::to_string(assetId)], expectedUri);
    EXPECT_GT(QueryDateTrashedForGetUrisCase(assetId), 0);
}

/*
 * @tc.name: DeleteLocalAssetsAndGetUris_LocalAsset_ReturnsResults
 * @tc.desc: Positive service case, a local-only asset is deleted locally and a successful result is returned
 * @tc.type: FUNCTION
 */
HWTEST_F(CloudMediaAssetsDeleteTest, DeleteLocalAssetsAndGetUris_LocalAsset_ReturnsResults, TestSize.Level1)
{
    const std::string title = "svc_delete_pic";
    int32_t assetId = InsertPhotoAssetForGetUrisCase(title);
    ASSERT_GT(assetId, 0);
    std::string expectedUri = "file://media/Photo/" + std::to_string(assetId) + "/IMG_" + title + "/" + title + ".jpg";

    auto service = make_shared<MediaAssetsDeleteService>();
    std::vector<std::string> fileIds = { std::to_string(assetId) };
    std::vector<AssetUriResult> results;

    int32_t ret = service->DeleteLocalAssetsAndGetUris(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].originalFileId, std::to_string(assetId));
    EXPECT_TRUE(results[0].isSuccess);
    EXPECT_EQ(results[0].resultUri, expectedUri);
    EXPECT_GT(QueryDateTrashedForGetUrisCase(assetId), 0);
}

}  // namespace OHOS::Media::CloudSync
