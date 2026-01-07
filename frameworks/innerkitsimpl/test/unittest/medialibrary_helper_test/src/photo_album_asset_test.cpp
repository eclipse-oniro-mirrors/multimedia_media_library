/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "media_column.h"
#include "medialibrary_db_const.h"
#include "photo_album.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
/*
 * Feature : MediaLibraryHelperUnitTest
 * Function : Check set get function
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryHelperUnitTest, PhotoAlbum_SetGet_Test_001, TestSize.Level1)
{
    PhotoAlbum photoAlbum;

    const int32_t TEST_ALBUM_ID = 1;
    photoAlbum.SetAlbumId(TEST_ALBUM_ID);
    EXPECT_EQ(photoAlbum.GetAlbumId(), TEST_ALBUM_ID);

    photoAlbum.SetPhotoAlbumType(USER);
    EXPECT_EQ(photoAlbum.GetPhotoAlbumType(), USER);

    photoAlbum.SetPhotoAlbumSubType(USER_GENERIC);
    EXPECT_EQ(photoAlbum.GetPhotoAlbumSubType(), USER_GENERIC);

    const string TEST_URI = "file://media/album/1";
    photoAlbum.SetAlbumUri(TEST_URI);
    EXPECT_EQ(photoAlbum.GetAlbumUri(), TEST_URI);

    const string TEST_ALBUM_NAME = "test";
    photoAlbum.SetAlbumName(TEST_ALBUM_NAME);
    EXPECT_EQ(photoAlbum.GetAlbumName(), TEST_ALBUM_NAME);

    const string TEST_COVERURI = TEST_URI;
    photoAlbum.SetCoverUri(TEST_COVERURI);
    EXPECT_EQ(photoAlbum.GetCoverUri(), TEST_COVERURI);

    const int32_t TEST_COUNT = 1;
    photoAlbum.SetCount(TEST_COUNT);
    EXPECT_EQ(photoAlbum.GetCount(), TEST_COUNT);

    const string TEST_RELATIVE_PATH = "Camera";
    photoAlbum.SetRelativePath(TEST_RELATIVE_PATH);
    EXPECT_EQ(photoAlbum.GetRelativePath(), TEST_RELATIVE_PATH);

    const int64_t CHANGE_TIME = 1;
    photoAlbum.SetChangeTime(CHANGE_TIME);
    EXPECT_EQ(photoAlbum.GetChangeTime(), CHANGE_TIME);

    photoAlbum.SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    EXPECT_EQ(photoAlbum.GetResultNapiType(), ResultNapiType::TYPE_USERFILE_MGR);

    photoAlbum.SetHidden(TEST_ALBUM_ID);
    EXPECT_EQ(photoAlbum.GetHidden(), TEST_ALBUM_ID);
}
} // namespace Media
} // namespace OHOS