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

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "message_parcel.h"

#include "default_cover_order_info.h"
#include "media_albums_controller_service.h"
#include "medialibrary_business_code.h"
#include "modify_album_default_cover_order_vo.h"
#include "user_define_ipc.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::IPC;

namespace OHOS {
namespace Media {
class ModifyAlbumDefaultCoverOrderTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

static const std::string TEST_LPATH = "/storage/media/123.jpg";
static const std::string TEST_ORDER_KEY = "date_added";
static const std::string TEST_ORDER_SUB_KEY = "media_id";

static DefaultCoverOrderInfo BuildInfo(int32_t albumType, int32_t albumSubType, int32_t orderType)
{
    DefaultCoverOrderInfo info;
    info.albumType = albumType;
    info.albumSubType = albumSubType;
    info.lpath = TEST_LPATH;
    info.orderKey = TEST_ORDER_KEY;
    info.orderSubKey = TEST_ORDER_SUB_KEY;
    info.orderType = orderType;
    return info;
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyMarshalUnmarshalSingle, TestSize.Level1)
{
    ModifyAlbumDefaultCoverOrderReqBody reqBody;
    reqBody.coverOrderInfos.push_back(BuildInfo(0, 1, 2));
    reqBody.disable = true;
    reqBody.isAsyncRefreshAlbum = true;

    MessageParcel parcel;
    ASSERT_TRUE(reqBody.Marshalling(parcel));

    ModifyAlbumDefaultCoverOrderReqBody unmarshal;
    ASSERT_TRUE(unmarshal.Unmarshalling(parcel));
    ASSERT_EQ(unmarshal.coverOrderInfos.size(), 1u);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].albumType, 0);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].albumSubType, 1);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].lpath, TEST_LPATH);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].orderKey, TEST_ORDER_KEY);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].orderSubKey, TEST_ORDER_SUB_KEY);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].orderType, 2);
    EXPECT_TRUE(unmarshal.disable);
    EXPECT_TRUE(unmarshal.isAsyncRefreshAlbum);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyMarshalUnmarshalMulti, TestSize.Level1)
{
    ModifyAlbumDefaultCoverOrderReqBody reqBody;
    reqBody.coverOrderInfos.push_back(BuildInfo(0, 1, 2));
    reqBody.coverOrderInfos.push_back(BuildInfo(3, 4, 5));
    reqBody.disable = false;
    reqBody.isAsyncRefreshAlbum = false;

    MessageParcel parcel;
    ASSERT_TRUE(reqBody.Marshalling(parcel));

    ModifyAlbumDefaultCoverOrderReqBody unmarshal;
    ASSERT_TRUE(unmarshal.Unmarshalling(parcel));
    ASSERT_EQ(unmarshal.coverOrderInfos.size(), 2u);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].albumType, 0);
    EXPECT_EQ(unmarshal.coverOrderInfos[0].orderType, 2);
    EXPECT_EQ(unmarshal.coverOrderInfos[1].albumType, 3);
    EXPECT_EQ(unmarshal.coverOrderInfos[1].orderType, 5);
    EXPECT_FALSE(unmarshal.disable);
    EXPECT_FALSE(unmarshal.isAsyncRefreshAlbum);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyMarshalEmptyInfos, TestSize.Level1)
{
    ModifyAlbumDefaultCoverOrderReqBody reqBody;
    MessageParcel parcel;
    EXPECT_FALSE(reqBody.Marshalling(parcel));
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyMarshalOverMaxSize, TestSize.Level1)
{
    ModifyAlbumDefaultCoverOrderReqBody reqBody;
    DefaultCoverOrderInfo info = BuildInfo(0, 1, 2);
    for (size_t i = 0; i <= 1024 * 1024; i++) {
        reqBody.coverOrderInfos.push_back(info);
    }
    MessageParcel parcel;
    EXPECT_FALSE(reqBody.Marshalling(parcel));
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyUnmarshalEmptyParcel, TestSize.Level1)
{
    MessageParcel parcel;
    ModifyAlbumDefaultCoverOrderReqBody unmarshal;
    EXPECT_FALSE(unmarshal.Unmarshalling(parcel));
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyUnmarshalZeroSize, TestSize.Level1)
{
    MessageParcel parcel;
    parcel.WriteInt32(0);
    ModifyAlbumDefaultCoverOrderReqBody unmarshal;
    EXPECT_FALSE(unmarshal.Unmarshalling(parcel));
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, CoverOrderReqBodyUnmarshalTruncated, TestSize.Level1)
{
    MessageParcel parcel;
    parcel.WriteInt32(1);
    ModifyAlbumDefaultCoverOrderReqBody unmarshal;
    EXPECT_FALSE(unmarshal.Unmarshalling(parcel));
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, ControllerModifyCoverOrderReadBodyFail, TestSize.Level1)
{
    auto controller = make_shared<MediaAlbumsControllerService>();
    MessageParcel data;
    MessageParcel reply;
    int32_t ret = controller->ModifyAlbumDefaultCoverOrder(data, reply);
    EXPECT_LT(ret, 0);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, ControllerModifyHiddenCoverOrderReadBodyFail, TestSize.Level1)
{
    auto controller = make_shared<MediaAlbumsControllerService>();
    MessageParcel data;
    MessageParcel reply;
    int32_t ret = controller->ModifyHiddenAlbumDefaultCoverOrder(data, reply);
    EXPECT_LT(ret, 0);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, PermPolicyModifyAlbumDefaultCoverOrder, TestSize.Level1)
{
    auto controller = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = controller->GetPermissionPolicy(
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_MODIFY_ALBUM_DEFAULT_COVER_ORDER),
        policy, isBypass);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_FALSE(isBypass);
    ASSERT_EQ(policy.size(), 1u);
    ASSERT_EQ(policy[0].size(), 2u);
    EXPECT_EQ(policy[0][0], SYSTEMAPI_PERM);
    EXPECT_EQ(policy[0][1], WRITE_PERM);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, PermPolicyModifyHiddenAlbumDefaultCoverOrder, TestSize.Level1)
{
    auto controller = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = controller->GetPermissionPolicy(
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_MODIFY_HIDDEN_ALBUM_DEFAULT_COVER_ORDER),
        policy, isBypass);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_FALSE(isBypass);
    ASSERT_EQ(policy.size(), 1u);
    ASSERT_EQ(policy[0].size(), 3u);
    EXPECT_EQ(policy[0][0], SYSTEMAPI_PERM);
    EXPECT_EQ(policy[0][1], WRITE_PERM);
    EXPECT_EQ(policy[0][2], PRIVATE_PERM);
}

HWTEST_F(ModifyAlbumDefaultCoverOrderTest, PermPolicyUnknownCode, TestSize.Level1)
{
    auto controller = make_shared<MediaAlbumsControllerService>();
    std::vector<std::vector<PermissionType>> policy;
    bool isBypass = false;
    int32_t ret = controller->GetPermissionPolicy(0xFFFFFFFF, policy, isBypass);
    EXPECT_EQ(ret, E_FAIL);
}
} // namespace Media
} // namespace OHOS
