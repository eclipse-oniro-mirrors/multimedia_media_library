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

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"

#include "media_assets_recover_service.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::Common;
using namespace OHOS::Media::ORM;
namespace OHOS::Media::CloudSync {

void MediaAssetsRecoverServiceTest::SetUpTestCase() {}
void MediaAssetsRecoverServiceTest::TearDownTestCase() {}
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

}  // namespace OHOS::Media::CloudSync
