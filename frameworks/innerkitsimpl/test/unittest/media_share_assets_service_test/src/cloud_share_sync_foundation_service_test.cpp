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

#define MLOG_TAG "CloudShareSyncFoundationServiceTest"

#include "media_share_assets_test_utils.h"

#include <memory>
#include <string>

#include "cloud_share_sync_foundation_service.h"
#include "media_assets_controller_service.h"
#include "media_empty_obj_vo.h"
#include "media_log.h"
#include "media_resp_vo.h"
#include "medialibrary_errno.h"
#include "mock_cloud_sync_utils.h"
#include "retain_cloud_media_asset_vo.h"

using namespace std;
using namespace testing::ext;

namespace OHOS::Media {

static constexpr int32_t SHARE_RETAIN_FORCE_TYPE = 2;
static constexpr int32_t INVALID_RETAIN_FORCE_TYPE = 99;

/**
 * @tc.name: StopSync_ReturnsOk
 * @tc.desc: 账号退出场景主动停止共享相册端云同步
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, StopSync_ReturnsOk, TestSize.Level1)
{
    CloudShareSyncFoundationService service;
    EXPECT_EQ(service.StopSync(), E_OK);
}

/**
 * @tc.name: TryToStartSync_SwitchOff_ReturnsOk
 * @tc.desc: 共享相册同步开关关闭时不启动同步(覆盖开关为关闭的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryToStartSync_SwitchOff_ReturnsOk, TestSize.Level1)
{
    MockShareAlbumSyncSwitch::SetSwitchOn(false);
    CloudShareSyncFoundationService service;
    EXPECT_EQ(service.TryToStartSync(), E_OK);
}

/**
 * @tc.name: TryToStartSync_SwitchOn_ReturnsOk
 * @tc.desc: 共享相册同步开关打开时尝试启动同步(覆盖开关为打开的分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, TryToStartSync_SwitchOn_ReturnsOk, TestSize.Level1)
{
    MockShareAlbumSyncSwitch::SetSwitchOn(true);
    CloudShareSyncFoundationService service;
    EXPECT_EQ(service.TryToStartSync(), E_OK);
    MockShareAlbumSyncSwitch::SetSwitchOn(false);
}

/**
 * @tc.name: ResetCursor_SwitchOn_ReturnsOk
 * @tc.desc: 开关打开时重置端云同步水位(覆盖重置分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, ResetCursor_SwitchOn_ReturnsOk, TestSize.Level1)
{
    MockShareAlbumSyncSwitch::SetSwitchOn(true);
    CloudShareSyncFoundationService service;
    EXPECT_EQ(service.ResetCursor(), E_OK);
    MockShareAlbumSyncSwitch::SetSwitchOn(false);
}

/**
 * @tc.name: ResetCursor_SwitchOff_ReturnsOk
 * @tc.desc: 开关关闭时不重置水位(覆盖跳过分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, ResetCursor_SwitchOff_ReturnsOk, TestSize.Level1)
{
    MockShareAlbumSyncSwitch::SetSwitchOn(false);
    CloudShareSyncFoundationService service;
    EXPECT_EQ(service.ResetCursor(), E_OK);
}

/**
 * @tc.name: RetainCloudMediaAsset_ShareRetainType_ReturnsOk
 * @tc.desc: 共享相册退出云空间: 通过 SHARE_RETAIN_FORCE 类型触发共享相册与资产清理(覆盖新增 else if 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RetainCloudMediaAsset_ShareRetainType_ReturnsOk, TestSize.Level1)
{
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareAlbum(1, "share_album_1"), E_OK);
    ASSERT_EQ(MediaShareAssetsTestUtils::InsertShareMember(1, "member_1"), E_OK);

    MessageParcel data;
    MessageParcel reply;
    RetainCloudMediaAssetReqBody reqBody;
    reqBody.cloudMediaRetainType = SHARE_RETAIN_FORCE_TYPE;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAssetsControllerService>();
    ASSERT_NE(service, nullptr);
    service->RetainCloudMediaAsset(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_OK);
}

/**
 * @tc.name: RetainCloudMediaAsset_InvalidRetainType_ReturnsInvalidValues
 * @tc.desc: 非法的保留类型返回参数错误(覆盖最后一个 else 分支)
 * @tc.type: FUNC
 */
HWTEST_F(MediaShareAssetsServiceTest, RetainCloudMediaAsset_InvalidRetainType_ReturnsInvalidValues, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    RetainCloudMediaAssetReqBody reqBody;
    reqBody.cloudMediaRetainType = INVALID_RETAIN_FORCE_TYPE;
    ASSERT_TRUE(reqBody.Marshalling(data));

    auto service = make_shared<MediaAssetsControllerService>();
    ASSERT_NE(service, nullptr);
    service->RetainCloudMediaAsset(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_TRUE(respVo.Unmarshalling(reply));
    EXPECT_EQ(respVo.GetErrCode(), E_INVALID_VALUES);
}

} // namespace OHOS::Media