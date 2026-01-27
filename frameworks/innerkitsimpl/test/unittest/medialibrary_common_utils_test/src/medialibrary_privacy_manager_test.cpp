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
#include "medialibrary_common_utils_test.h"
#include "medialibrary_errno.h"
#define private public
#include "media_privacy_manager.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

const int32_t ALL_DESENSITIZE = 0;
const int32_t GEOGRAPHIC_LOCATION_DESENSITIZE = 1;
const int32_t SHOOTING_PARAM_DESENSITIZE = 2;
const int32_t NO_DESENSITIZE = 3;
const int32_t DEFAULT = 4;

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_001, TestSize.Level1)
{
    string path = "";
    string mode = "";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_002, TestSize.Level1)
{
    string path = ".Open";
    string mode = "Z";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_003, TestSize.Level1)
{
    string path = ".jpeg";
    string mode = "";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_004, TestSize.Level1)
{
    string path = ".Open";
    string mode = "w";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_005, TestSize.Level1)
{
    string path(4096, 'a');
    string mode = "w";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_006, TestSize.Level1)
{
    string path = "data:image/;base64,Open.jpeg";
    string mode = "a";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_007, TestSize.Level1)
{
    string path = "data:image/;base64,Open.jpeg";
    string mode = "rwrwrw";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_008, TestSize.Level1)
{
    string path = "/storage/cloud/files/open.jpeg";
    string mode = "w";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_009, TestSize.Level1)
{
    string path = "";
    string mode = "w";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_010, TestSize.Level1)
{
    string path = "/storage/cloud/files/";
    string mode = "w";
    string fileId = "";
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, E_ERR);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_011, TestSize.Level1)
{
    string path = "/storage/cloud/100/files/1.jpg";
    string mode = "r";
    string fileId = "1";
    int32_t type = ALL_DESENSITIZE;
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId, type);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, -ENOMEM);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_012, TestSize.Level1)
{
    string path = "/storage/cloud/100/files/1.jpg";
    string mode = "r";
    string fileId = "1";
    int32_t type = GEOGRAPHIC_LOCATION_DESENSITIZE;
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId, type);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, -ENOMEM);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_013, TestSize.Level1)
{
    string path = "/storage/cloud/100/files/1.jpg";
    string mode = "r";
    string fileId = "1";
    int32_t type = SHOOTING_PARAM_DESENSITIZE;
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId, type);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, -ENOMEM);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_014, TestSize.Level1)
{
    string path = "/storage/cloud/100/files/1.jpg";
    string mode = "r";
    string fileId = "1";
    int32_t type = NO_DESENSITIZE;
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId, type);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_NE(ret, -ENOMEM);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_Open_test_015, TestSize.Level1)
{
    string path = "/storage/cloud/100/files/1.jpg";
    string mode = "r";
    string fileId = "1";
    int32_t type = DEFAULT;
    MediaPrivacyManager mediaPrivacyManager(path, mode, fileId, type);
    int32_t ret = mediaPrivacyManager.Open();
    EXPECT_EQ(ret, -ENOMEM);
}
} // namespace Media
} // namespace OHOS