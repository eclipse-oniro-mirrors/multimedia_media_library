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

#define MLOG_TAG "CloneToAlbumServiceTest"

#include "clone_to_album_service_test.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"

#include "clone_to_album_service.h"
#undef private

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
namespace OHOS::Media::CloudSync {

void CloneToAlbumServiceTest::SetUpTestCase() {}
void CloneToAlbumServiceTest::TearDownTestCase() {}
void CloneToAlbumServiceTest::SetUp() {}
void CloneToAlbumServiceTest::TearDown() {}

// ===== CloneTaskInfo copy constructor =====
HWTEST_F(CloneToAlbumServiceTest, CloneTaskInfo_CopyConstructor_CopiesFields, TestSize.Level1)
{
    /**
     * @tc.name: CloneTaskInfo_CopyConstructor_CopiesFields
     * @tc.desc: CloneTaskInfo copy constructor correctly copies all fields
     * @tc.type: FUNCTION
     */
    CloneTaskInfo original;
    original.processedSize.store(100);
    original.processedCount.store(5);
    original.albumType = 1;
    original.albumSubType = 2;
    original.requestId = 42;
    original.targetDir = "/test/dir";
    original.cloneCallbackType = CloneCallbackType::FILEPATH;

    CloneAssetInfo assetInfo;
    assetInfo.fileId = 10;
    assetInfo.displayName = "test.jpg";
    original.cloneAssetInfo.push_back(assetInfo);

    CloneTaskInfo copy(original);
    EXPECT_EQ(copy.processedSize.load(), 100u);
    EXPECT_EQ(copy.processedCount.load(), 5u);
    EXPECT_EQ(copy.albumType, 1);
    EXPECT_EQ(copy.albumSubType, 2);
    EXPECT_EQ(copy.requestId, 0);
    EXPECT_EQ(copy.targetDir, "/test/dir");
    EXPECT_EQ(copy.cloneCallbackType, CloneCallbackType::FILEPATH);
    EXPECT_EQ(copy.cloneAssetInfo.size(), 1u);
    EXPECT_EQ(copy.cloneAssetInfo[0].fileId, 10);
    EXPECT_EQ(copy.cloneAssetInfo[0].displayName, "test.jpg");
}

// ===== CloneAssetInfo default values =====
HWTEST_F(CloneToAlbumServiceTest, CloneAssetInfo_DefaultValues, TestSize.Level1)
{
    /**
     * @tc.name: CloneAssetInfo_DefaultValues
     * @tc.desc: CloneAssetInfo struct has correct default values
     * @tc.type: FUNCTION
     */
    CloneAssetInfo info;
    EXPECT_EQ(info.fileId, -1);
    EXPECT_EQ(info.filePath, "");
    EXPECT_EQ(info.displayName, "");
    EXPECT_EQ(info.mediaType, -1);
    EXPECT_EQ(info.size, 0);
    EXPECT_EQ(info.hidden, 0);
    EXPECT_EQ(info.dateTrashed, -1);
    EXPECT_EQ(info.position, -1);
    EXPECT_EQ(info.storagePath, "");
    EXPECT_EQ(info.sourcePath, "");
    EXPECT_EQ(info.burstKey, "");
    EXPECT_EQ(info.mode, 0);
    EXPECT_EQ(info.albumLpath, "");
    EXPECT_EQ(info.albumId, -1);
    EXPECT_EQ(info.albumSubType, -1);
    EXPECT_EQ(info.albumType, -1);
    EXPECT_EQ(info.requestId, 0);
    EXPECT_EQ(info.targetFilePath, "");
    EXPECT_EQ(info.targetFileTitle, "");
    EXPECT_EQ(info.targetDisplayName, "");
    EXPECT_EQ(info.photoSubType, 0);
    EXPECT_EQ(info.movingPhotoEffectMode, 0);
    EXPECT_EQ(info.fileSourceType, -1);
    EXPECT_TRUE(info.burstCloneAssetList.empty());
}

// ===== CloneCallbackType enum =====
HWTEST_F(CloneToAlbumServiceTest, CloneCallbackType_EnumValues, TestSize.Level1)
{
    /**
     * @tc.name: CloneCallbackType_EnumValues
     * @tc.desc: CloneCallbackType enum has correct values
     * @tc.type: FUNCTION
     */
    EXPECT_EQ(static_cast<int>(CloneCallbackType::URI), 0);
    EXPECT_EQ(static_cast<int>(CloneCallbackType::FILEPATH), 1);
    EXPECT_EQ(static_cast<int>(CloneCallbackType::PHOTOASSET), 2);
}

}  // namespace OHOS::Media::CloudSync
