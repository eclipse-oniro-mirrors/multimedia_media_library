/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "live_photo_4d_asset_helper_test.h"

#include "live_photo_4d_asset_helper.h"
#include "medialibrary_type_const.h"
#include "photo_asset_change_info.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

void LivePhoto4DAssetHelperTest::SetUpTestCase(void) {}

void LivePhoto4DAssetHelperTest::TearDownTestCase(void) {}

void LivePhoto4DAssetHelperTest::SetUp() {}

void LivePhoto4DAssetHelperTest::TearDown() {}

void SetCommonSystemAsset(PhotoAssetChangeInfo &assetInfo, bool isHiddenAsset)
{
    assetInfo.syncStatus_ = static_cast<int32_t> (SyncStatusType::TYPE_VISIBLE);
    assetInfo.cleanFlag_ = static_cast<int32_t> (CleanType::TYPE_NOT_CLEAN);
    assetInfo.dateTrashedMs_ = 0;
    assetInfo.isHidden_ = isHiddenAsset;
    assetInfo.timePending_ = 0;
    assetInfo.isTemp_ = false;
    assetInfo.burstCoverLevel_ = static_cast<int32_t> (BurstCoverLevelType::COVER);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsAsset_test_001, TestSize.Level0)
{
    // 用例说明：测试IsAsset返回true的场景
    // - 覆盖场景：livephoto4dStatus为TYPE_LIVEPHOTO_4D且为非隐藏系统资产
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回true && IsCommonSystemAsset返回true
    // - 触发条件：设置livephoto4dStatus_为TYPE_LIVEPHOTO_4D，媒体类型为视频
    // - 业务验证：验证IsAsset返回true
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    SetCommonSystemAsset(assetInfo, false);

    bool result = LivePhoto4DAssetHelper::IsAsset(assetInfo);

    EXPECT_TRUE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsAsset_test_002, TestSize.Level0)
{
    // 用例说明：测试IsAsset返回false的场景-livephoto4dStatus不匹配
    // - 覆盖场景：livephoto4dStatus不是TYPE_LIVEPHOTO_4D
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回false
    // - 触发条件：设置livephoto4dStatus_为TYPE_UNIDENTIFIED
    // - 业务验证：验证IsAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    assetInfo.isHidden_ = false;

    bool result = LivePhoto4DAssetHelper::IsAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsAsset_test_003, TestSize.Level0)
{
    // 用例说明：测试IsAsset返回false的场景-不是系统资产
    // - 覆盖场景：livephoto4dStatus为TYPE_LIVEPHOTO_4D但不是系统资产
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回true && IsCommonSystemAsset返回false
    // - 触发条件：设置dateTrashedMs_非0（已删除）
    // - 业务验证：验证IsAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.dateTrashedMs_ = 12345;

    bool result = LivePhoto4DAssetHelper::IsAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsVideoAsset_test_001, TestSize.Level0)
{
    // 用例说明：测试IsVideoAsset返回true的场景
    // - 覆盖场景：LivePhoto4D资产且为视频类型
    // - 覆盖分支点：IsAsset返回true && IsVideoAsset返回true
    // - 触发条件：livephoto4dStatus为TYPE_LIVEPHOTO_4D，媒体类型为视频
    // - 业务验证：验证IsVideoAsset返回true
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    SetCommonSystemAsset(assetInfo, false);

    bool result = LivePhoto4DAssetHelper::IsVideoAsset(assetInfo);

    EXPECT_TRUE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsVideoAsset_test_002, TestSize.Level0)
{
    // 用例说明：测试IsVideoAsset返回false的场景-不是LivePhoto4D资产
    // - 覆盖场景：不是LivePhoto4D资产
    // - 覆盖分支点：IsAsset返回false
    // - 触发条件：livephoto4dStatus为TYPE_UNSUPPORTED
    // - 业务验证：验证IsVideoAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNSUPPORTED);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);

    bool result = LivePhoto4DAssetHelper::IsVideoAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsVideoAsset_test_003, TestSize.Level0)
{
    // 用例说明：测试IsVideoAsset返回false的场景-不是视频类型
    // - 覆盖场景：LivePhoto4D资产但为图片类型
    // - 覆盖分支点：IsAsset返回true && IsVideoAsset返回false
    // - 触发条件：livephoto4dStatus为TYPE_LIVEPHOTO_4D，媒体类型为图片
    // - 业务验证：验证IsVideoAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    assetInfo.isHidden_ = false;
    assetInfo.dateTrashedMs_ = 0;

    bool result = LivePhoto4DAssetHelper::IsVideoAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsHiddenAsset_test_001, TestSize.Level0)
{
    // 用例说明：测试IsHiddenAsset返回true的场景
    // - 覆盖场景：LivePhoto4D资产且为隐藏系统资产
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回true && IsCommonSystemAsset(isHidden=true)返回true
    // - 触发条件：livephoto4dStatus为TYPE_LIVEPHOTO_4D，isHidden为true
    // - 业务验证：验证IsHiddenAsset返回true
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    SetCommonSystemAsset(assetInfo, true);

    bool result = LivePhoto4DAssetHelper::IsHiddenAsset(assetInfo);

    EXPECT_TRUE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsHiddenAsset_test_002, TestSize.Level0)
{
    // 用例说明：测试IsHiddenAsset返回false的场景-livephoto4dStatus不匹配
    // - 覆盖场景：livephoto4dStatus不是TYPE_LIVEPHOTO_4D
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回false
    // - 触发条件：livephoto4dStatus为TYPE_SUPPORTED
    // - 业务验证：验证IsHiddenAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_SUPPORTED);
    assetInfo.isHidden_ = true;

    bool result = LivePhoto4DAssetHelper::IsHiddenAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsHiddenAsset_test_003, TestSize.Level0)
{
    // 用例说明：测试IsHiddenAsset返回false的场景-不是隐藏系统资产
    // - 覆盖场景：livephoto4dStatus为TYPE_LIVEPHOTO_4D但不是隐藏资产
    // - 覆盖分支点：IsTypeStatusLivePhoto4D返回true && IsCommonSystemAsset(isHidden=true)返回false
    // - 触发条件：isHidden为false
    // - 业务验证：验证IsHiddenAsset返回false
    PhotoAssetChangeInfo assetInfo;
    assetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    assetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    assetInfo.isHidden_ = false;
    assetInfo.dateTrashedMs_ = 0;

    bool result = LivePhoto4DAssetHelper::IsHiddenAsset(assetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerAsset_test_001, TestSize.Level0)
{
    // 用例说明：测试IsNewerAsset返回true的场景
    // - 覆盖场景：compareAssetInfo比currentAssetInfo新
    // - 覆盖分支点：两个IsAsset都返回true && IsNewerByDateTaken返回true
    // - 触发条件：两个都是LivePhoto4D资产，compare的dateTaken更大
    // - 业务验证：验证IsNewerAsset返回true
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    compareAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    compareAssetInfo.dateTakenMs_ = 200000;
    SetCommonSystemAsset(compareAssetInfo, false);

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    currentAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    currentAssetInfo.dateTakenMs_ = 100000;
    SetCommonSystemAsset(currentAssetInfo, false);

    bool result = LivePhoto4DAssetHelper::IsNewerAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_TRUE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerAsset_test_002, TestSize.Level0)
{
    // 用例说明：测试IsNewerAsset返回false的场景-compare不是LivePhoto4D资产
    // - 覆盖场景：compareAssetInfo不是LivePhoto4D资产
    // - 覆盖分支点：第一个IsAsset返回false
    // - 触发条件：compare的livephoto4dStatus为TYPE_UNSUPPORTED
    // - 业务验证：验证IsNewerAsset返回false
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNSUPPORTED);

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    currentAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    currentAssetInfo.isHidden_ = false;
    currentAssetInfo.dateTrashedMs_ = 0;

    bool result = LivePhoto4DAssetHelper::IsNewerAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerAsset_test_003, TestSize.Level0)
{
    // 用例说明：测试IsNewerAsset返回false的场景-current不是LivePhoto4D资产
    // - 覆盖场景：currentAssetInfo不是LivePhoto4D资产
    // - 覆盖分支点：第一个IsAsset返回true && 第二个IsAsset返回false
    // - 触发条件：current的livephoto4dStatus为TYPE_USED
    // - 业务验证：验证IsNewerAsset返回false
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    compareAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    compareAssetInfo.isHidden_ = false;
    compareAssetInfo.dateTrashedMs_ = 0;

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_USED);

    bool result = LivePhoto4DAssetHelper::IsNewerAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerHiddenAsset_test_001, TestSize.Level0)
{
    // 用例说明：测试IsNewerHiddenAsset返回true的场景
    // - 覆盖场景：compareHiddenAsset比currentHiddenAsset新
    // - 覆盖分支点：两个IsHiddenAsset都返回true && IsNewerByHiddenTime返回true
    // - 触发条件：两个都是LivePhoto4D隐藏资产，compare的hiddenTime更大
    // - 业务验证：验证IsNewerHiddenAsset返回true
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    compareAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    SetCommonSystemAsset(compareAssetInfo, true);
    compareAssetInfo.hiddenTime_ = 200000;

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    currentAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    SetCommonSystemAsset(currentAssetInfo, true);
    currentAssetInfo.hiddenTime_ = 100000;

    bool result = LivePhoto4DAssetHelper::IsNewerHiddenAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_TRUE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerHiddenAsset_test_002, TestSize.Level0)
{
    // 用例说明：测试IsNewerHiddenAsset返回false的场景-compare不是隐藏资产
    // - 覆盖场景：compareAssetInfo不是隐藏资产
    // - 覆盖分支点：第一个IsHiddenAsset返回false
    // - 触发条件：compare的isHidden为false
    // - 业务验证：验证IsNewerHiddenAsset返回false
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    compareAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    compareAssetInfo.isHidden_ = false;
    compareAssetInfo.dateTrashedMs_ = 0;

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    currentAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    currentAssetInfo.isHidden_ = true;
    currentAssetInfo.dateTrashedMs_ = 0;

    bool result = LivePhoto4DAssetHelper::IsNewerHiddenAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_FALSE(result);
}

HWTEST_F(LivePhoto4DAssetHelperTest, IsNewerHiddenAsset_test_003, TestSize.Level0)
{
    // 用例说明：测试IsNewerHiddenAsset返回false的场景-current不是隐藏资产
    // - 覆盖场景：currentAssetInfo不是隐藏资产
    // - 覆盖分支点：第一个IsHiddenAsset返回true && 第二个IsHiddenAsset返回false
    // - 触发条件：current的isHidden为false
    // - 业务验证：验证IsNewerHiddenAsset返回false
    PhotoAssetChangeInfo compareAssetInfo;
    compareAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    compareAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    compareAssetInfo.isHidden_ = true;
    compareAssetInfo.dateTrashedMs_ = 0;

    PhotoAssetChangeInfo currentAssetInfo;
    currentAssetInfo.livephoto4dStatus_ = static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D);
    currentAssetInfo.mediaType_ = static_cast<int32_t>(MEDIA_TYPE_VIDEO);
    currentAssetInfo.isHidden_ = false;
    currentAssetInfo.dateTrashedMs_ = 0;

    bool result = LivePhoto4DAssetHelper::IsNewerHiddenAsset(compareAssetInfo, currentAssetInfo);

    EXPECT_FALSE(result);
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS