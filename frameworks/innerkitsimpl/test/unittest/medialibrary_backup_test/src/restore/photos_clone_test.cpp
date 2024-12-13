/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotosCloneTest"

#include "photos_clone_test.h"

#include <string>

#define private public
#include "photos_clone.h"
#undef private

#include "photo_album_dao.h"
#include "backup_const.h"
#include "userfile_manager_types.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS::Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

void PhotosCloneTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void PhotosCloneTest::TearDownTestCase(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    MEDIA_INFO_LOG("TearDownTestCase");
}

// SetUp:Execute before each test case
void PhotosCloneTest::SetUp()
{}

void PhotosCloneTest::TearDown(void)
{}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_1_lPath_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/Screenrecords");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_1_lPath_Not_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/Screenshots";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/Screenrecords");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_2_lPath_Empty_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_LPATH_EMPTY_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_Hidden_Screenshots_Video_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.hidden = 1;
    fileInfo.sourcePath = "/storage/emulated/0/Pictures/Screenshots/SVID_20240805_113052_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/Pictures/Screenrecords");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_TRASHED_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}

HWTEST_F(PhotosCloneTest, FindAlbumInfo_Scenario_3_TRASHED_LPATH_EMPTY_Test, TestSize.Level0)
{
    FileInfo fileInfo;
    fileInfo.lPath = "/Pictures/其它";
    fileInfo.fileType = MediaType::MEDIA_TYPE_VIDEO;
    fileInfo.recycledTime = 1;
    fileInfo.sourcePath = "/storage/emulated/0/DCIM/Camera/SVID_20241029_225550_1.mp4";
    PhotoAlbumDao::PhotoAlbumRowData albumRowData = PhotosClone().FindAlbumInfo(fileInfo);
    EXPECT_EQ(albumRowData.albumId, 0);
    EXPECT_EQ(albumRowData.lPath, "/DCIM/Camera");
}
}  // namespace OHOS::Media