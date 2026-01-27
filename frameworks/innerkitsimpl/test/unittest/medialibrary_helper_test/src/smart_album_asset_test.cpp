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

#include "smart_album_asset.h"
#include "medialibrary_db_const.h"
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
HWTEST_F(MediaLibraryHelperUnitTest, SmartAlbumAsset_SetGet_Test_001, TestSize.Level1)
{
    SmartAlbumAsset smartAlbumAsset;

    const int32_t TEST_ALBUM_ID = 1;
    smartAlbumAsset.SetAlbumId(TEST_ALBUM_ID);
    EXPECT_EQ(smartAlbumAsset.GetAlbumId(), TEST_ALBUM_ID);

    const string TEST_ALBUM_NAME = "test";
    smartAlbumAsset.SetAlbumName(TEST_ALBUM_NAME);
    EXPECT_EQ(smartAlbumAsset.GetAlbumName(), TEST_ALBUM_NAME);

    const string TEST_URI = MEDIALIBRARY_DATA_URI + CONST_MEDIALIBRARY_TYPE_FILE_URI + "/" + to_string(TEST_ALBUM_ID);
    smartAlbumAsset.SetAlbumUri(TEST_URI);
    EXPECT_EQ(smartAlbumAsset.GetAlbumUri(), TEST_URI);

    const string TEST_ALBUM_TAG = "test";
    smartAlbumAsset.SetAlbumTag(TEST_ALBUM_TAG);
    EXPECT_EQ(smartAlbumAsset.GetAlbumTag(), TEST_ALBUM_TAG);

    const int32_t TEST_ALBUM_CAPACITY = 1;
    smartAlbumAsset.SetAlbumCapacity(TEST_ALBUM_CAPACITY);
    EXPECT_EQ(smartAlbumAsset.GetAlbumCapacity(), TEST_ALBUM_CAPACITY);

    const int64_t TEST_ALBUM_DATE_MODIFIED = 1;
    smartAlbumAsset.SetAlbumDateModified(TEST_ALBUM_DATE_MODIFIED);
    EXPECT_EQ(smartAlbumAsset.GetAlbumDateModified(), TEST_ALBUM_DATE_MODIFIED);

    const int32_t TEST_CATEGORY_ID = 1;
    smartAlbumAsset.SetCategoryId(TEST_CATEGORY_ID);
    EXPECT_EQ(smartAlbumAsset.GetCategoryId(), TEST_CATEGORY_ID);

    const string TEST_CATEGORY_NAME = "test";
    smartAlbumAsset.SetCategoryName(TEST_CATEGORY_NAME);
    EXPECT_EQ(smartAlbumAsset.GetCategoryName(), TEST_CATEGORY_NAME);

    const string TEST_COVERURI = TEST_URI;
    smartAlbumAsset.SetCoverUri(TEST_COVERURI);
    EXPECT_EQ(smartAlbumAsset.GetCoverUri(), TEST_COVERURI);

    smartAlbumAsset.SetAlbumPrivateType(TYPE_TRASH);
    EXPECT_EQ(smartAlbumAsset.GetAlbumPrivateType(), TYPE_TRASH);

    smartAlbumAsset.SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    EXPECT_EQ(smartAlbumAsset.GetResultNapiType(), ResultNapiType::TYPE_USERFILE_MGR);
}
} // namespace Media
} // namespace OHOS