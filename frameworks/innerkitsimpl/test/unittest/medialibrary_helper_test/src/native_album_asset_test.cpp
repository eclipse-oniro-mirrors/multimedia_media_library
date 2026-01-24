/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "native_album_asset.h"
#include "medialibrary_db_const.h"
#include "medialibrary_type_const.h"
#include "userfilemgr_uri.h"
#include "media_file_uri.h"

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
HWTEST_F(MediaLibraryHelperUnitTest, NativeAlbumAsset_SetGet_Test_001, TestSize.Level1)
{
    NativeAlbumAsset nativeAlbumAsset;

    const int32_t TEST_ALBUM_ID = 1;
    nativeAlbumAsset.SetAlbumId(TEST_ALBUM_ID);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumId(), TEST_ALBUM_ID);

    const string TEST_ALBUM_NAME = "test";
    nativeAlbumAsset.SetAlbumName(TEST_ALBUM_NAME);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumName(), TEST_ALBUM_NAME);

    const string TEST_URI = MEDIALIBRARY_DATA_URI + CONST_MEDIALIBRARY_TYPE_FILE_URI + "/" + to_string(TEST_ALBUM_ID);
    nativeAlbumAsset.SetAlbumUri(TEST_URI);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumUri(), TEST_URI);

    const int64_t TEST_ALBUM_DATE_MODIFIED = 1;
    nativeAlbumAsset.SetAlbumDateModified(TEST_ALBUM_DATE_MODIFIED);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumDateModified(), TEST_ALBUM_DATE_MODIFIED);

    const int32_t TEST_COUNT = 1;
    nativeAlbumAsset.SetCount(TEST_COUNT);
    EXPECT_EQ(nativeAlbumAsset.GetCount(), TEST_COUNT);

    const string TEST_ALBUM_RELATIVE_PATH = PIC_DIR_VALUES;
    nativeAlbumAsset.SetAlbumRelativePath(TEST_ALBUM_RELATIVE_PATH);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumRelativePath(), TEST_ALBUM_RELATIVE_PATH);

    const string TEST_COVERURI = TEST_URI;
    nativeAlbumAsset.SetCoverUri(TEST_COVERURI);
    EXPECT_EQ(nativeAlbumAsset.GetCoverUri(), TEST_COVERURI);

    const string TEST_ALBUM_PATH = ROOT_MEDIA_DIR + PIC_DIR_VALUES + TEST_ALBUM_NAME;
    nativeAlbumAsset.SetAlbumPath(TEST_ALBUM_PATH);
    EXPECT_EQ(nativeAlbumAsset.GetAlbumPath(), TEST_ALBUM_PATH);
}

/*
 * Feature : MediaLibraryHelperUnitTest
 * Function : Check createalbumasset
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryHelperUnitTest, NativeAlbumAsset_CreateAlbumAsset_Test_001, TestSize.Level1)
{
    NativeAlbumAsset nativeAlbumAsset;
    string albumPath = "/data/test/native_createalbumasset_001";
    nativeAlbumAsset.SetAlbumPath(albumPath);
    EXPECT_EQ(nativeAlbumAsset.CreateAlbumAsset(), true);
    EXPECT_EQ(nativeAlbumAsset.CreateAlbumAsset(), false);
}

/*
 * Feature : MediaLibraryHelperUnitTest
 * Function : Check deletealbumasset
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryHelperUnitTest, NativeAlbumAsset_DeleteAlbumAsset_Test_001, TestSize.Level1)
{
    NativeAlbumAsset nativeAlbumAsset;
    string albumPath = "/data/test/native_deletealbumasset_001";
    nativeAlbumAsset.SetAlbumPath(albumPath);
    EXPECT_EQ(nativeAlbumAsset.CreateAlbumAsset(), true);
    EXPECT_EQ(nativeAlbumAsset.DeleteAlbumAsset(albumPath), true);
}

/*
 * Feature : MediaLibraryHelperUnitTest
 * Function : Check modifyalbumasset
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryHelperUnitTest, NativeAlbumAsset_ModifyAlbumAsset_Test_001, TestSize.Level1)
{
    NativeAlbumAsset nativeAlbumAsset;
    string albumPath = "/data/test/native_deletealbumasset_001";
    nativeAlbumAsset.SetAlbumPath(albumPath);
    EXPECT_EQ(nativeAlbumAsset.CreateAlbumAsset(), true);
    nativeAlbumAsset.SetAlbumName("native_deletealbumasset_001_modify");
    EXPECT_EQ(nativeAlbumAsset.ModifyAlbumAsset(albumPath), true);
}

/*
 * Feature : MediaLibraryHelperUnitTest
 * Function : Check modifyalbumasset
 * SubFunction : NA
 * FunctionPoints : NA
 * EnvContions : NA
 * CaseDescription : NA
 */
HWTEST_F(MediaLibraryHelperUnitTest, NativeAlbumAsset_ModifyAlbumAsset_Test_002, TestSize.Level1)
{
    NativeAlbumAsset nativeAlbumAsset;
    string albumPath = "native_deletealbumasset_002";
    EXPECT_EQ(nativeAlbumAsset.ModifyAlbumAsset(albumPath), false);
}
} // namespace Media
} // namespace OHOS