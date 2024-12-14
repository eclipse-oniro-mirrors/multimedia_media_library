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

#define MLOG_TAG "PhotoAlbumLPathOperationTest"

#include "photo_album_lpath_operation_test.h"

#include <string>
#include <vector>

#include "photo_album_lpath_operation.h"
#include "media_log.h"

using namespace testing::ext;

namespace OHOS::Media {
void PhotoAlbumLPathOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
}

void PhotoAlbumLPathOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotoAlbumLPathOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoAlbumLPathOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanInvalidPhotoAlbums_Test, TestSize.Level0)
{
    int32_t affectedCount =
        PhotoAlbumLPathOperation().SetRdbStore(nullptr).CleanInvalidPhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanDuplicatePhotoAlbums_Test, TestSize.Level0)
{
    int32_t affectedCount =
        PhotoAlbumLPathOperation().SetRdbStore(nullptr).CleanDuplicatePhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}

HWTEST_F(PhotoAlbumLPathOperationTest, CleanEmptylPathPhotoAlbums_Test, TestSize.Level0)
{
    int32_t affectedCount =
        PhotoAlbumLPathOperation().SetRdbStore(nullptr).CleanEmptylPathPhotoAlbums().GetAlbumAffectedCount();
    EXPECT_EQ(affectedCount, 0);
}
}  // namespace OHOS::Media