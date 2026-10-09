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

#include "get_retey_records_vo.h"

#include <gtest/gtest.h>
#include <message_parcel.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS::Media::CloudSync {

class GetRetryRecordsVoTest : public testing::Test {};

HWTEST_F(GetRetryRecordsVoTest, TC001_Marshalling_Unmarshalling_Empty_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：空数据）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    GetRetryRecordsRespBody original;
    original.retryDataList.clear();

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_TRUE(restored.retryDataList.empty());
}

HWTEST_F(GetRetryRecordsVoTest, TC002_Marshalling_Unmarshalling_Single_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：单条数据）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    GetRetryRecordsRespBody original;
    GetRetryRecordsDataVo retryData;
    retryData.cloudId = "cloud_id_001";
    retryData.shareAlbumOwner = "owner_001";
    original.retryDataList[retryData.cloudId] = retryData;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    ASSERT_EQ(restored.retryDataList.size(), 1);
    auto it = restored.retryDataList.find("cloud_id_001");
    ASSERT_NE(it, restored.retryDataList.end());
    EXPECT_EQ(it->second.cloudId, "cloud_id_001");
    EXPECT_EQ(it->second.shareAlbumOwner, "owner_001");
}

HWTEST_F(GetRetryRecordsVoTest, TC003_Marshalling_Unmarshalling_Multiple_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：多条数据）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    GetRetryRecordsRespBody original;
    for (int i = 1; i <= 3; i++) {
        GetRetryRecordsDataVo retryData;
        retryData.cloudId = "cloud_id_00" + std::to_string(i);
        retryData.shareAlbumOwner = "owner_00" + std::to_string(i);
        original.retryDataList[retryData.cloudId] = retryData;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    ASSERT_EQ(restored.retryDataList.size(), 3);
    for (int i = 1; i <= 3; i++) {
        std::string cloudId = "cloud_id_00" + std::to_string(i);
        auto it = restored.retryDataList.find(cloudId);
        ASSERT_NE(it, restored.retryDataList.end());
        EXPECT_EQ(it->second.cloudId, cloudId);
        EXPECT_EQ(it->second.shareAlbumOwner, "owner_00" + std::to_string(i));
    }
}

HWTEST_F(GetRetryRecordsVoTest, TC004_Marshalling_Unmarshalling_LongString_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖边界路径（触发条件：超长字符串）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    GetRetryRecordsRespBody original;
    std::string longCloudId(1000, 'A');
    std::string longOwner(1000, 'B');
    GetRetryRecordsDataVo retryData;
    retryData.cloudId = longCloudId;
    retryData.shareAlbumOwner = longOwner;
    original.retryDataList[retryData.cloudId] = retryData;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    ASSERT_EQ(restored.retryDataList.size(), 1);
    auto it = restored.retryDataList.find(longCloudId);
    ASSERT_NE(it, restored.retryDataList.end());
    EXPECT_EQ(it->second.cloudId, longCloudId);
    EXPECT_EQ(it->second.shareAlbumOwner, longOwner);
}

HWTEST_F(GetRetryRecordsVoTest, TC005_Marshalling_Unmarshalling_SpecialString_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖边界路径（触发条件：空串与特殊字符）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    std::vector<std::string> cloudIds = {
        "cloud_id_empty", "cloud_id_with_中文", "cloud_id_with_special!@#$%^&*()"};
    std::vector<std::string> owners = {"", "owner_中文", "owner_special!@#$%^&*()"};
    GetRetryRecordsRespBody original;
    for (size_t i = 0; i < cloudIds.size(); i++) {
        GetRetryRecordsDataVo retryData;
        retryData.cloudId = cloudIds[i];
        retryData.shareAlbumOwner = owners[i];
        original.retryDataList[retryData.cloudId] = retryData;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    ASSERT_EQ(restored.retryDataList.size(), cloudIds.size());
    for (size_t i = 0; i < cloudIds.size(); i++) {
        auto it = restored.retryDataList.find(cloudIds[i]);
        ASSERT_NE(it, restored.retryDataList.end());
        EXPECT_EQ(it->second.cloudId, cloudIds[i]);
        EXPECT_EQ(it->second.shareAlbumOwner, owners[i]);
    }
}

HWTEST_F(GetRetryRecordsVoTest, TC006_Marshalling_Unmarshalling_LargeMap_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖边界路径（触发条件：大批量数据）
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    const int32_t dataSize = 100;
    GetRetryRecordsRespBody original;
    for (int32_t i = 0; i < dataSize; i++) {
        GetRetryRecordsDataVo retryData;
        retryData.cloudId = "cloud_id_" + std::to_string(i);
        retryData.shareAlbumOwner = "owner_" + std::to_string(i);
        original.retryDataList[retryData.cloudId] = retryData;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    GetRetryRecordsRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    ASSERT_EQ(restored.retryDataList.size(), dataSize);
    for (int32_t i = 0; i < dataSize; i++) {
        std::string cloudId = "cloud_id_" + std::to_string(i);
        auto it = restored.retryDataList.find(cloudId);
        ASSERT_NE(it, restored.retryDataList.end());
        EXPECT_EQ(it->second.cloudId, cloudId);
        EXPECT_EQ(it->second.shareAlbumOwner, "owner_" + std::to_string(i));
    }
}
}  // namespace OHOS::Media::CloudSync
