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

#define MLOG_TAG "CoverOrderChangeTest"

#include <gtest/gtest.h>
#include <functional>

#include "album_asset_helper.h"
#include "owner_album_info_calculation.h"
#include "system_album_info_calculation.h"
#include "photo_asset_change_info.h"
#include "accurate_common_data.h"

namespace OHOS {
namespace Media {

using namespace std;
using namespace testing::ext;
using namespace AccurateRefresh;

class CoverOrderChangeTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

static void FillAssetChangeData(PhotoAssetChangeData &data, const string &beforeName, int64_t beforeSize,
    const string &afterName, int64_t afterSize)
{
    data.infoBeforeChange_.displayName_ = beforeName;
    data.infoBeforeChange_.size_ = beforeSize;
    data.infoAfterChange_.displayName_ = afterName;
    data.infoAfterChange_.size_ = afterSize;
}

HWTEST_F(CoverOrderChangeTest, AlbumAssetHelper_IsNameChange_SameName, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    EXPECT_FALSE(AlbumAssetHelper::IsNameChange(data));
}

HWTEST_F(CoverOrderChangeTest, AlbumAssetHelper_IsNameChange_DiffName, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    EXPECT_TRUE(AlbumAssetHelper::IsNameChange(data));
}

HWTEST_F(CoverOrderChangeTest, AlbumAssetHelper_IsSizeChange_SameSize, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    EXPECT_FALSE(AlbumAssetHelper::IsSizeChange(data));
}

HWTEST_F(CoverOrderChangeTest, AlbumAssetHelper_IsSizeChange_DiffSize, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    EXPECT_TRUE(AlbumAssetHelper::IsSizeChange(data));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsNameChange_BothInAlbumAndNameDiff, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &, int32_t) -> bool { return true; };
    EXPECT_TRUE(OwnerAlbumInfoCalculation::IsNameChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsNameChange_BeforeNotInAlbum, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &info, int32_t) -> bool {
            return info.displayName_ != "a.jpg";
        };
    EXPECT_FALSE(OwnerAlbumInfoCalculation::IsNameChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsNameChange_AfterNotInAlbum, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &info, int32_t) -> bool {
            return info.displayName_ != "b.jpg";
        };
    EXPECT_FALSE(OwnerAlbumInfoCalculation::IsNameChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsNameChange_NameSame, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &, int32_t) -> bool { return true; };
    EXPECT_FALSE(OwnerAlbumInfoCalculation::IsNameChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsSizeChange_BothInAlbumAndSizeDiff, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &, int32_t) -> bool { return true; };
    EXPECT_TRUE(OwnerAlbumInfoCalculation::IsSizeChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsSizeChange_BeforeNotInAlbum, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &info, int32_t) -> bool {
            return info.size_ != 100;
        };
    EXPECT_FALSE(OwnerAlbumInfoCalculation::IsSizeChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, Owner_IsSizeChange_SizeSame, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo, int32_t)> isAlbumAsset =
        [] (const PhotoAssetChangeInfo &, int32_t) -> bool { return true; };
    EXPECT_FALSE(OwnerAlbumInfoCalculation::IsSizeChange(data, 1, isAlbumAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsNameChange_BothSystemAndNameDiff, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &) -> bool { return true; };
    EXPECT_TRUE(SystemAlbumInfoCalculation::IsNameChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsNameChange_BeforeNotSystem, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &info) -> bool {
            return info.displayName_ != "a.jpg";
        };
    EXPECT_FALSE(SystemAlbumInfoCalculation::IsNameChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsNameChange_AfterNotSystem, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &info) -> bool {
            return info.displayName_ != "b.jpg";
        };
    EXPECT_FALSE(SystemAlbumInfoCalculation::IsNameChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsNameChange_NameSame, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &) -> bool { return true; };
    EXPECT_FALSE(SystemAlbumInfoCalculation::IsNameChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsSizeChange_BothSystemAndSizeDiff, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &) -> bool { return true; };
    EXPECT_TRUE(SystemAlbumInfoCalculation::IsSizeChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsSizeChange_BeforeNotSystem, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "a.jpg", 200);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &info) -> bool {
            return info.size_ != 100;
        };
    EXPECT_FALSE(SystemAlbumInfoCalculation::IsSizeChange(data, isSystemAsset));
}

HWTEST_F(CoverOrderChangeTest, System_IsSizeChange_SizeSame, TestSize.Level1)
{
    PhotoAssetChangeData data;
    FillAssetChangeData(data, "a.jpg", 100, "b.jpg", 100);
    function<bool(PhotoAssetChangeInfo)> isSystemAsset =
        [] (const PhotoAssetChangeInfo &) -> bool { return true; };
    EXPECT_FALSE(SystemAlbumInfoCalculation::IsSizeChange(data, isSystemAsset));
}

} // namespace Media
} // namespace OHOS
