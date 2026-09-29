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

#include "file_asset.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_IsFileHidden_Default_001, TestSize.Level1)
{
    FileAsset fileAsset;
    EXPECT_FALSE(fileAsset.IsFileHidden());
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetFileHidden_True_002, TestSize.Level1)
{
    FileAsset fileAsset;
    fileAsset.SetFileHidden(true);
    EXPECT_TRUE(fileAsset.IsFileHidden());
}

HWTEST_F(MediaLibraryHelperUnitTest, FileAsset_SetFileHidden_False_003, TestSize.Level1)
{
    FileAsset fileAsset;
    fileAsset.SetFileHidden(true);
    ASSERT_TRUE(fileAsset.IsFileHidden());
    fileAsset.SetFileHidden(false);
    EXPECT_FALSE(fileAsset.IsFileHidden());
}
} // namespace Media
} // namespace OHOS
