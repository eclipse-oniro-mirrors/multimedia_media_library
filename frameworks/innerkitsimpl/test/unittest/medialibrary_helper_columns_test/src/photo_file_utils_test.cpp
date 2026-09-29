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

#include "medialibrary_helper_test.h"

#include "photo_file_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_CheckSubDir_Empty_001, TestSize.Level1)
{
    EXPECT_TRUE(PhotoFileUtils::CheckSubDirForFileManager("/storage/media/local/files/", 27));
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_CheckSubDir_Excluded_002, TestSize.Level1)
{
    EXPECT_FALSE(PhotoFileUtils::CheckSubDirForFileManager(
        "/storage/media/local/files/.thumbs/abc", 27));
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_CheckSubDir_Normal_003, TestSize.Level1)
{
    EXPECT_TRUE(PhotoFileUtils::CheckSubDirForFileManager(
        "/storage/media/local/files/Documents/abc", 27));
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_CheckRealPath_NotDocs_004, TestSize.Level1)
{
    EXPECT_FALSE(PhotoFileUtils::CheckFileManagerRealPath("/data/local/tmp/abc"));
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetDirFromLPath_BadPrefix_005, TestSize.Level1)
{
    EXPECT_TRUE(PhotoFileUtils::GetFileManagerDirFromLPath("/storage/media/local/files/Documents").empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetDirFromLPath_Root_006, TestSize.Level1)
{
    string dir = PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs/");
    EXPECT_FALSE(dir.empty());
}

HWTEST_F(MediaLibraryHelperUnitTest, PhotoFileUtils_GetDirFromLPath_Normal_007, TestSize.Level1)
{
    string dir = PhotoFileUtils::GetFileManagerDirFromLPath("/FromDocs/Documents");
    EXPECT_FALSE(dir.empty());
    EXPECT_NE(dir.find("Documents"), string::npos);
}
} // namespace Media
} // namespace OHOS
