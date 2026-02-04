/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "medialibrary_helper_test.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_file_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasSource_001, TestSize.Level1)
{
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 10), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1732767140111, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 1732767140222, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1732767140333, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10, 5), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailExists_001, TestSize.Level1)
{
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123456123_001.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg/LCD.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456123_001.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailExists(photoPath), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_IsThumbnailLatest_001, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/1/"), true);
    string photoPath = "/storage/cloud/files/Photo/1/IMG_123456789_123.jpg";
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateFile(photoPath), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);

    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/THM.jpg"), true);
    EXPECT_EQ(PhotoFileUtils::IsThumbnailLatest(photoPath), false);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg/LCD.jpg"), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/Photo/1/"), true);
    EXPECT_EQ(MediaFileUtils::DeleteDir("/storage/cloud/files/.thumbs/Photo/1/IMG_123456789_123.jpg"), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_HasSource_002, TestSize.Level1)
{
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 10), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 0), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 2), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 1), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 3), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 4), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 5), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 6), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 7), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 8), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 9), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 0, 10), false);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(true, 1, 0), true);
    EXPECT_EQ(PhotoFileUtils::HasSource(false, 1, 2), true);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetMetaPathFromOrignalPath_001, TestSize.Level1)
{
    string photoPath = "";
    string metaPath = "";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), -209);

    photoPath = "/123/456/789";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), -209);

    photoPath = "/data/media/Recovery/Photo/IMG_123.jpg";
    EXPECT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(photoPath, metaPath), 0);
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetMetaDataRealPath_001, TestSize.Level1)
{
    string photoPath = "";
    int32_t userId = 0;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId), "");

    photoPath = "/123/456/789";
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId), "");

    photoPath = "/storage/cloud/files/Photo/1/IMG_123435213_124.jpg";
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/0/files/.meta/Photo/1/IMG_123435213_124.jpg.json");

    userId = -1;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/files/.meta/Photo/1/IMG_123435213_124.jpg.json");
    userId = 1;
    EXPECT_EQ(PhotoFileUtils::GetMetaDataRealPath(photoPath, userId),
        "/storage/cloud/1/files/.meta/Photo/1/IMG_123435213_124.jpg.json");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetThumbDir_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    string res = PhotoFileUtils::GetThumbDir(photoPath);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetLCDPath_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    int32_t userId = 0;
    string res = PhotoFileUtils::GetLCDPath(photoPath, userId);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetTHMPath_001, TestSize.Level1)
{
    std::string photoPath = "/Picture/";
    int32_t userId = 0;
    string res = PhotoFileUtils::GetTHMPath(photoPath, userId);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_ConstructDateAddedDateParts, TestSize.Level1)
{
    MEDIA_INFO_LOG("PhotoFileUtils_ConstructDateAddedDateParts Start");
    int64_t dateAdded = 1732767140000; // 2024-11-28 02:25:40
    auto parts = PhotoFileUtils::ConstructDateAddedDateParts(dateAdded);
    EXPECT_EQ(parts.year, "2024");
    EXPECT_EQ(parts.month, "202411");
    EXPECT_EQ(parts.day, "20241128");
    parts = PhotoFileUtils::ConstructDateAddedDateParts(0);
    EXPECT_NE(parts.year, "");
    EXPECT_NE(parts.month, "");
    EXPECT_NE(parts.day, "");
    MEDIA_INFO_LOG("PhotoFileUtils_ConstructDateAddedDateParts End");
}
} // namespace Media
} // namespace OHOS