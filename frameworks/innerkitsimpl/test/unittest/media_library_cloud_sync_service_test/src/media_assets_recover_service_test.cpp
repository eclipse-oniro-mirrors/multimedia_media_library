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

#define MLOG_TAG "MediaAssetsRecoverServiceTest"

#include "media_assets_recover_service_test.h"

#include <cstdlib>

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "media_cloud_sync_test_utils.h"
#include "result_set_utils.h"

#include "media_assets_recover_service.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::Common;
using namespace OHOS::Media::ORM;
namespace OHOS::Media::CloudSync {

static shared_ptr<MediaLibraryRdbStore> g_rdbStore = nullptr;

void MediaAssetsRecoverServiceTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
}
void MediaAssetsRecoverServiceTest::TearDownTestCase()
{
    g_rdbStore = nullptr;
}
void MediaAssetsRecoverServiceTest::SetUp() {}
void MediaAssetsRecoverServiceTest::TearDown() {}

// MoveOutTrashAndMergeWithSameAsset: null photoRefresh
HWTEST_F(MediaAssetsRecoverServiceTest, MoveOutTrash_NullPhotoRefresh_ReturnsError, TestSize.Level1)
{
    /**
     * @tc.name: MoveOutTrash_NullPhotoRefresh_ReturnsError
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo photo;
    std::optional<PhotosPo> targetOp;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.MoveOutTrashAndMergeWithSameAsset(nullRefresh, photo, targetOp);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

// RecoverPackageName: null photoRefresh
HWTEST_F(MediaAssetsRecoverServiceTest, RecoverPackageName_NullPhotoRefresh_ReturnsError, TestSize.Level1)
{
    /**
     * @tc.name: RecoverPackageName_NullPhotoRefresh_ReturnsError
     * @tc.desc: Returns E_RDB_STORE_NULL when photoRefresh is nullptr
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> nullRefresh = nullptr;

    int32_t ret = service.RecoverPackageName(source, target, nullRefresh);
    EXPECT_EQ(ret, E_RDB_STORE_NULL);
}

// RecoverPackageName: empty source packageName
HWTEST_F(MediaAssetsRecoverServiceTest, RecoverPackageName_EmptySourcePackage_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: RecoverPackageName_EmptySourcePackage_ReturnsOK
     * @tc.desc: Returns E_OK early when source packageName is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.packageName = ""; // empty
    target.packageName = "com.example.app";
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.RecoverPackageName(source, target, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

// RecoverPackageName: target already has packageName
HWTEST_F(MediaAssetsRecoverServiceTest, RecoverPackageName_TargetAlreadyHasPackage_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: RecoverPackageName_TargetAlreadyHasPackage_ReturnsOK
     * @tc.desc: Returns E_OK early when target already has a non-empty packageName
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.packageName = "com.source.app";
    target.packageName = "com.target.app"; // already has package
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.RecoverPackageName(source, target, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

// RecoverPackageName: both nullopt packageName
HWTEST_F(MediaAssetsRecoverServiceTest, RecoverPackageName_BothNullopt_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: RecoverPackageName_BothNullopt_ReturnsOK
     * @tc.desc: Returns E_OK when both source and target packageName are nullopt (empty via value_or)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target; // packageName nullopt
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.RecoverPackageName(source, target, photoRefresh);
    EXPECT_EQ(ret, E_OK);
}

// StoreThumbnailAndEditSize: invalid fileId
HWTEST_F(MediaAssetsRecoverServiceTest, StoreThumbnailAndEditSize_ZeroFileId, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_ZeroFileId
     * @tc.desc: Returns E_INVALID_VALUES when fileId is 0
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo photo;
    photo.fileId = 0;
    photo.data = "/some/path";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

// StoreThumbnailAndEditSize: empty data
HWTEST_F(MediaAssetsRecoverServiceTest, StoreThumbnailAndEditSize_EmptyData, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_EmptyData
     * @tc.desc: Returns E_INVALID_VALUES when data is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo photo;
    photo.fileId = 10;
    photo.data = "";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

// StoreThumbnailAndEditSize: nullopt fileId
HWTEST_F(MediaAssetsRecoverServiceTest, StoreThumbnailAndEditSize_NulloptFileId, TestSize.Level1)
{
    /**
     * @tc.name: StoreThumbnailAndEditSize_NulloptFileId
     * @tc.desc: Returns E_INVALID_VALUES when fileId is nullopt (default 0)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo photo;
    photo.data = "/some/path";

    int32_t ret = service.StoreThumbnailAndEditSize(photo);
    EXPECT_EQ(ret, E_INVALID_VALUES);
}

// MergeAssetFile: source is CLOUD → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MergeAssetFile_SourceCloud_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MergeAssetFile_SourceCloud_ReturnsOK
     * @tc.desc: Returns E_OK when source position is CLOUD (no file move needed)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.position = static_cast<int32_t>(PhotoPositionType::CLOUD);

    int32_t ret = service.MergeAssetFile(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MergeAssetFile: target is LOCAL → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MergeAssetFile_TargetLocal_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MergeAssetFile_TargetLocal_ReturnsOK
     * @tc.desc: Returns E_OK when target position is LOCAL (no file move needed)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    target.position = static_cast<int32_t>(PhotoPositionType::LOCAL);

    int32_t ret = service.MergeAssetFile(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MergeAssetFile: target is LOCAL_AND_CLOUD → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MergeAssetFile_TargetLocalAndCloud_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MergeAssetFile_TargetLocalAndCloud_ReturnsOK
     * @tc.desc: Returns E_OK when target position is LOCAL_AND_CLOUD (no file move needed)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    target.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);

    int32_t ret = service.MergeAssetFile(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MergeAssetFileFromMediaToLake: source CLOUD → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MergeAssetFileMediaToLake_SourceCloud_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MergeAssetFileMediaToLake_SourceCloud_ReturnsOK
     * @tc.desc: Returns E_OK when source is CLOUD (no file move needed)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.position = static_cast<int32_t>(PhotoPositionType::CLOUD);

    int32_t ret = service.MergeAssetFileFromMediaToLake(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MergeAssetFileFromMediaToLake: target not CLOUD → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MergeAssetFileMediaToLake_TargetNotCloud_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MergeAssetFileMediaToLake_TargetNotCloud_ReturnsOK
     * @tc.desc: Returns E_OK when target is not CLOUD (no file move needed)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    target.position = static_cast<int32_t>(PhotoPositionType::LOCAL);

    int32_t ret = service.MergeAssetFileFromMediaToLake(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MoveAssetFileFromMediaToLake: not media file → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MoveAssetFileMediaToLake_NotMediaFile_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MoveAssetFileMediaToLake_NotMediaFile_ReturnsOK
     * @tc.desc: Returns E_OK when source is not a media file (ShouldHandleAsMediaFile=false)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.fileSourceType = static_cast<int32_t>(FileSourceType::FILE_MANAGER); // not media
    target.storagePath = "/lake/path";

    int32_t ret = service.MoveAssetFileFromMediaToLake(source, target);
    EXPECT_EQ(ret, E_OK);
}

// MoveAssetFileFromMediaToLake: empty lake path → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MoveAssetFileMediaToLake_EmptyLakePath_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: MoveAssetFileMediaToLake_EmptyLakePath_ReturnsOK
     * @tc.desc: Returns E_OK when target storagePath is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    target.storagePath = ""; // empty

    int32_t ret = service.MoveAssetFileFromMediaToLake(source, target);
    EXPECT_EQ(ret, E_OK);
}

// CommonMergeDiffCloudAsset: not cloud assets → skip
HWTEST_F(MediaAssetsRecoverServiceTest, CommonMergeDiffCloudAsset_NotCloudAsset_ReturnsInvalidMode, TestSize.Level1)
{
    /**
     * @tc.name: CommonMergeDiffCloudAsset_NotCloudAsset_ReturnsInvalidMode
     * @tc.desc: Returns E_INVALID_MODE when source is not a cloud asset
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.cloudId = "";
    target.cloudId = "different_cloud";
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CommonMergeDiffCloudAsset(source, target, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CommonMergeSameCloudAsset: not cloud assets → skip
HWTEST_F(MediaAssetsRecoverServiceTest, CommonMergeSameCloudAsset_NotCloudAsset_ReturnsInvalidMode, TestSize.Level1)
{
    /**
     * @tc.name: CommonMergeSameCloudAsset_NotCloudAsset_ReturnsInvalidMode
     * @tc.desc: Returns E_INVALID_MODE when source is not a cloud asset
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.cloudId = "";
    target.cloudId = "";
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CommonMergeSameCloudAsset(source, target, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CommonMergeCloudToLocalAsset: source not cloud → skip
HWTEST_F(MediaAssetsRecoverServiceTest, CommonMergeCloudToLocal_SourceNotCloud_ReturnsInvalidMode, TestSize.Level1)
{
    /**
     * @tc.name: CommonMergeCloudToLocal_SourceNotCloud_ReturnsInvalidMode
     * @tc.desc: Returns E_INVALID_MODE when source is not a cloud asset
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.cloudId = "";
    source.dirty = static_cast<int32_t>(DirtyType::TYPE_NEW); // TYPE_NEW → not cloud
    target.cloudId = "";
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CommonMergeCloudToLocalAsset(source, target, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// CommonMergeLocalToLocalAsset: source is cloud → skip
HWTEST_F(MediaAssetsRecoverServiceTest, CommonMergeLocalToLocal_SourceIsCloud_ReturnsInvalidMode, TestSize.Level1)
{
    /**
     * @tc.name: CommonMergeLocalToLocal_SourceIsCloud_ReturnsInvalidMode
     * @tc.desc: Returns E_INVALID_MODE when source is a cloud asset
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.cloudId = "cloud_123";
    source.dirty = static_cast<int32_t>(DirtyType::TYPE_MDIRTY); // not TYPE_NEW → is cloud
    source.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.CommonMergeLocalToLocalAsset(source, target, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

// MediaAndMediaMergeLocalToCloudAsset: source is cloud → skip
HWTEST_F(MediaAssetsRecoverServiceTest, MediaAndMediaMerge_SourceIsCloud_ReturnsInvalidMode, TestSize.Level1)
{
    /**
     * @tc.name: MediaAndMediaMerge_SourceIsCloud_ReturnsInvalidMode
     * @tc.desc: Returns E_INVALID_MODE when source is a cloud asset (should be local)
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source, target;
    source.cloudId = "cloud_123";
    source.dirty = static_cast<int32_t>(DirtyType::TYPE_MDIRTY);
    source.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    target.cloudId = "cloud_456";
    target.dirty = static_cast<int32_t>(DirtyType::TYPE_MDIRTY);
    target.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    auto photoRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();

    int32_t ret = service.MediaAndMediaMergeLocalToCloudAsset(source, target, photoRefresh);
    EXPECT_EQ(ret, E_INVALID_MODE);
}

HWTEST_F(MediaAssetsRecoverServiceTest, FillRecoverResults_NoRecoveredUri_AllFailed, TestSize.Level1)
{
    /**
     * @tc.name: FillRecoverResults_NoRecoveredUri_AllFailed
     * @tc.desc: Each input asset gets one result, and all are marked as failed when no uri is recovered
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> fileIds = { "1001", "1002" };
    std::vector<AssetUriResult> results;

    int32_t ret = service.FillRecoverResults(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(results.size(), fileIds.size());
    for (size_t i = 0; i < fileIds.size(); i++) {
        EXPECT_EQ(results[i].originalFileId, fileIds[i]);
        EXPECT_FALSE(results[i].isSuccess);
        EXPECT_TRUE(results[i].resultUri.empty());
    }
}

HWTEST_F(MediaAssetsRecoverServiceTest, FillRecoverResults_PartialSuccess_AlignedWithInput, TestSize.Level1)
{
    /**
     * @tc.name: FillRecoverResults_PartialSuccess_AlignedWithInput
     * @tc.desc: The result is aligned with the input order, only the matched asset is successful
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> fileIds = { "2001", "2002" };
    std::unordered_map<std::string, std::string> recoveredUris = {
        { "2002", "file://media/Photo/3002/IMG_002/IMG_002.jpg" } };
    std::vector<AssetUriResult> results;

    int32_t ret = service.FillRecoverResults(fileIds, recoveredUris, results);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].originalFileId, "2001");
    EXPECT_FALSE(results[0].isSuccess);
    EXPECT_TRUE(results[0].resultUri.empty());
    EXPECT_EQ(results[1].originalFileId, "2002");
    EXPECT_TRUE(results[1].isSuccess);
    EXPECT_EQ(results[1].resultUri, "file://media/Photo/3002/IMG_002/IMG_002.jpg");
}

HWTEST_F(MediaAssetsRecoverServiceTest, FillRecoverResults_EmptyInput_EmptyResult, TestSize.Level1)
{
    /**
     * @tc.name: FillRecoverResults_EmptyInput_EmptyResult
     * @tc.desc: Returns E_OK and empty results when the input is empty
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> fileIds;
    std::vector<AssetUriResult> results;
    results.emplace_back(AssetUriResult());

    int32_t ret = service.FillRecoverResults(fileIds, results);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(results.empty());
}

HWTEST_F(MediaAssetsRecoverServiceTest, RemoveDuplicates_KeepsFirstOccurrenceOrder, TestSize.Level1)
{
    /**
     * @tc.name: RemoveDuplicates_KeepsFirstOccurrenceOrder
     * @tc.desc: Duplicated values are removed and the first occurrence order is kept
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> values = { "b", "a", "b", "c", "a" };

    int32_t ret = service.RemoveDuplicates(values);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], "b");
    EXPECT_EQ(values[1], "a");
    EXPECT_EQ(values[2], "c");
}

HWTEST_F(MediaAssetsRecoverServiceTest, RemoveDuplicates_EmptyInput_NoChange, TestSize.Level1)
{
    /**
     * @tc.name: RemoveDuplicates_EmptyInput_NoChange
     * @tc.desc: Empty input stays empty without any crash
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> values;

    int32_t ret = service.RemoveDuplicates(values);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(values.empty());
}

HWTEST_F(MediaAssetsRecoverServiceTest, PrepareForBurstRecovery_EmptyInput_ReturnsOK, TestSize.Level1)
{
    /**
     * @tc.name: PrepareForBurstRecovery_EmptyInput_ReturnsOK
     * @tc.desc: Returns E_OK early when fileIds is empty, and does not query the database
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    std::vector<std::string> fileIds;

    int32_t ret = service.PrepareForBurstRecovery(fileIds);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(fileIds.empty());
}

HWTEST_F(MediaAssetsRecoverServiceTest, RecordRecoveredAsset_Success_RecordsTargetUri, TestSize.Level1)
{
    /**
     * @tc.name: RecordRecoveredAsset_Success_RecordsTargetUri
     * @tc.desc: The uri of the recovered asset is recorded by the original fileId
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source;
    source.fileId = 4001;
    PhotosPo target;
    target.fileId = 4002;
    target.data = "/storage/cloud/files/Photo/16/IMG_4002.jpg";
    target.displayName = "IMG_4002.jpg";
    std::optional<PhotosPo> targetOp = target;
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> recoveredUris;

    int32_t ret = service.RecordRecoveredAsset(source, E_OK, targetOp, targetFileIds, recoveredUris);
    EXPECT_EQ(ret, E_OK);
    ASSERT_EQ(targetFileIds.size(), 1);
    EXPECT_EQ(targetFileIds[0], target.BuildFileUri());
    ASSERT_EQ(recoveredUris.count("4001"), 1);
    EXPECT_EQ(recoveredUris["4001"], target.BuildFileUri());
}

HWTEST_F(MediaAssetsRecoverServiceTest, RecordRecoveredAsset_TargetAbsent_Skipped, TestSize.Level1)
{
    /**
     * @tc.name: RecordRecoveredAsset_TargetAbsent_Skipped
     * @tc.desc: Nothing is recorded when the target asset is absent
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source;
    source.fileId = 4003;
    std::optional<PhotosPo> targetOp;
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> recoveredUris;

    int32_t ret = service.RecordRecoveredAsset(source, E_OK, targetOp, targetFileIds, recoveredUris);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(targetFileIds.empty());
    EXPECT_TRUE(recoveredUris.empty());
}

HWTEST_F(MediaAssetsRecoverServiceTest, RecordRecoveredAsset_FailedRet_Skipped, TestSize.Level1)
{
    /**
     * @tc.name: RecordRecoveredAsset_FailedRet_Skipped
     * @tc.desc: Nothing is recorded when the operation failed, and the error code is returned
     * @tc.type: FUNCTION
     */
    MediaAssetsRecoverService service;
    PhotosPo source;
    source.fileId = 4004;
    std::optional<PhotosPo> targetOp = source;
    std::vector<std::string> targetFileIds;
    std::unordered_map<std::string, std::string> recoveredUris;

    int32_t ret = service.RecordRecoveredAsset(source, E_HAS_DB_ERROR, targetOp, targetFileIds, recoveredUris);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    EXPECT_TRUE(targetFileIds.empty());
    EXPECT_TRUE(recoveredUris.empty());
}

static int32_t InsertTrashedPhotoAsset(const std::string &title)
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
        "1501924205, 0, 0, 1748488395008, 0)";
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

/*
 * @tc.name: BatchMoveOutTrashAndMergeWithSameAsset_TrashedAsset_ReturnsResults
 * @tc.desc: Positive service case, a trashed asset is recovered and a successful result is returned
 * @tc.type: FUNCTION
 */
HWTEST_F(MediaAssetsRecoverServiceTest, BatchMoveOutTrashAndMergeWithSameAsset_TrashedAsset_ReturnsResults,
    TestSize.Level1)
{
    ClearAndRestart(g_rdbStore);
    const std::string title = "svc_recover_pic";
    int32_t assetId = InsertTrashedPhotoAsset(title);
    ASSERT_GT(assetId, 0);
    std::string trashedUri = "file://media/Photo/" + std::to_string(assetId) + "/IMG_" + title + "/" + title + ".jpg";

    MediaAssetsRecoverService service;
    std::vector<std::string> assets = { trashedUri };
    std::vector<std::string> targetFileIds;
    std::vector<AssetUriResult> results;

    int32_t ret = service.BatchMoveOutTrashAndMergeWithSameAsset(assets, targetFileIds, results);
    EXPECT_GE(ret, 0);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].originalFileId, std::to_string(assetId));
    EXPECT_TRUE(results[0].isSuccess);
    EXPECT_EQ(results[0].resultUri, trashedUri);
    ASSERT_EQ(targetFileIds.size(), 1);
    EXPECT_EQ(targetFileIds[0], trashedUri);
}

}  // namespace OHOS::Media::CloudSync
