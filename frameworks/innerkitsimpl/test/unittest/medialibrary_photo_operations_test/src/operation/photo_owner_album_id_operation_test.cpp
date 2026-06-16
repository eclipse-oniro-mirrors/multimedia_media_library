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

#define MLOG_TAG "PhotoOwnerAlbumIdOperationTest"

#define PRIVATE public
#define PROTECTED public
#include "photo_owner_album_id_operation.h"
#undef PRIVATE
#undef PROTECTED

#include <string>

#include "album_plugin_config.h"
#include "media_log.h"
#include "photo_owner_album_id_operation_test.h"

using namespace testing::ext;

namespace OHOS::Media {
void PhotoOwnerAlbumIdOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void PhotoOwnerAlbumIdOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotoOwnerAlbumIdOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoOwnerAlbumIdOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, FixPhotoRelation_NULL_RDB_Test, TestSize.Level1)
{
    std::vector<std::string> fileIds = {"1", "2", "3"};
    std::shared_ptr<MediaLibraryRdbStore> uniStore = nullptr;
    int32_t ret = PhotoOwnerAlbumIdOperation().SetRdbStore(uniStore).SetFileIds(fileIds).FixPhotoRelation();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, ParseSourcePathToLPath_NormalPath_001, TestSize.Level0)
{
    std::string result = operation_.ParseSourcePathToLPath("/storage/emulated/0/DCIM/Camera/IMG.jpg");
    EXPECT_EQ(result, "/DCIM/Camera");
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, ParseSourcePathToLPath_HiddenAlbumToRecover_002, TestSize.Level0)
{
    std::string result = operation_.ParseSourcePathToLPath("/storage/emulated/0/Pictures/hiddenAlbum/secret.jpg");
    EXPECT_EQ(result, AlbumPlugin::LPATH_RECOVER);
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, ParseSourcePathToLPath_NoRootPath_003, TestSize.Level0)
{
    std::string result = operation_.ParseSourcePathToLPath("");
    EXPECT_EQ(result, operation_.LPATH_DEFAULT_OTHER);

    result = operation_.ParseSourcePathToLPath("/data/backup/DCIM/Camera/IMG.jpg");
    EXPECT_EQ(result, operation_.LPATH_DEFAULT_OTHER);
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, ParseSourcePathToLPath_OffsetOverflowOrTooShort_004, TestSize.Level0)
{
    std::string result = operation_.ParseSourcePathToLPath("/storage/emulated/");
    EXPECT_EQ(result, operation_.LPATH_DEFAULT_OTHER);

    result = operation_.ParseSourcePathToLPath("/storage/emulated/0/");
    EXPECT_EQ(result, operation_.LPATH_DEFAULT_OTHER);
}

HWTEST_F(PhotoOwnerAlbumIdOperationTest, ParseSourcePathToLPath_RootLevelFile_005, TestSize.Level0)
{
    std::string result = operation_.ParseSourcePathToLPath("/storage/emulated/0/xxx.jpg");
    EXPECT_EQ(result, operation_.LPATH_DEFAULT_OTHER);
}
}  // namespace OHOS::Media