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

#include "cloud_mdkrecord_photo_album_vo.h"

#include <gtest/gtest.h>
#include <message_parcel.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS::Media::CloudSync {

class CloudMdkRecordPhotoAlbumVoTest : public testing::Test {};

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC001_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_123";
    original.albumName = "Test Album";
    original.albumType = 1;
    original.lpath = "DCIM/Album";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.albumName, original.albumName);
    EXPECT_EQ(restored.albumType, original.albumType);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC002_Unmarshalling_ReadStringAlbumCloudId_Fail, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumVo反序列化失败路径；
    // 覆盖albumCloudId字段读取失败（触发条件：MessageParcel.ReadString失败）；验证业务状态断言：反序列化返回false

    OHOS::MessageParcel parcel;
    bool ret = parcel.WriteString("album_cloud_123");
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo vo;
    ret = vo.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC003_Unmarshalling_ReadInt32AlbumType_Fail, TestSize.Level1)
{
    // 用例说明：测试错误处理；覆盖错误路径（触发条件：异常输入）；验证业务状态断言：处理失败
    OHOS::MessageParcel parcel;
    parcel.WriteString("album_cloud_123");
    parcel.WriteString("Test Album");

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo vo;
    bool ret = vo.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC004_RespBody_UnMarshallRecords_LenNegative, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody反序列化；覆盖len < 0分支（触发条件：ReadInt32返回负数）；
    // 验证业务状态断言：反序列化返回false
    OHOS::MessageParcel parcel;
    bool ret = parcel.WriteInt32(-1);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody respBody;
    ret = respBody.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC005_RespBody_UnMarshallRecords_SizeExceedReadable, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody反序列化；覆盖size > readAbleSize分支（触发条件：size超过可读字节）；
    // 验证业务状态断言：反序列化返回false
    OHOS::MessageParcel parcel;
    bool ret = parcel.WriteInt32(1000000);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody respBody;
    ret = respBody.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC006_RespBody_UnMarshallRecords_SizeExceedMaxSize, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody反序列化；覆盖size > max_size()分支（触发条件：size超过vector最大容量）；
    // 验证业务状态断言：反序列化返回false
    OHOS::MessageParcel parcel;
    size_t maxSize = std::vector<CloudMdkRecordPhotoAlbumVo>().max_size();
    bool ret = parcel.WriteInt32(static_cast<int32_t>(maxSize) + 1);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody respBody;
    ret = respBody.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC007_RespBody_UnMarshallRecords_ElementUnmarshallingFail, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody反序列化；覆盖循环内Unmarshalling失败分支（触发条件：元素反序列化失败）；
    // 验证业务状态断言：反序列化返回false
    OHOS::MessageParcel parcel;
    bool ret = parcel.WriteInt32(1);
    ASSERT_TRUE(ret);
    ret = parcel.WriteString("incomplete_data");
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody respBody;
    ret = respBody.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC008_RespBody_Marshalling_Unmarshalling_Empty, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody序列化与反序列化；覆盖空记录分支（触发条件：empty vector）；
    // 验证业务状态断言：反序列化后的记录数量为0
    std::vector<CloudMdkRecordPhotoAlbumVo> emptyRecords;
    CloudMdkRecordPhotoAlbumRespBody original(emptyRecords);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    auto records = restored.GetPhotoAlbumRecords();
    EXPECT_EQ(records.size(), 0);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC009_RespBody_Marshalling_Unmarshalling_Single, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody序列化与反序列化；覆盖单条记录分支（触发条件：size=1）；
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    std::vector<CloudMdkRecordPhotoAlbumVo> records;
    CloudMdkRecordPhotoAlbumVo album;
    album.cloudId = "album_cloud_123";
    album.albumName = "Test Album";
    album.albumType = 1;
    records.push_back(album);

    CloudMdkRecordPhotoAlbumRespBody original(records);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    auto restoredRecords = restored.GetPhotoAlbumRecords();
    EXPECT_EQ(restoredRecords.size(), 1);
    EXPECT_EQ(restoredRecords[0].cloudId, album.cloudId);
    EXPECT_EQ(restoredRecords[0].albumName, album.albumName);
    EXPECT_EQ(restoredRecords[0].albumType, album.albumType);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC010_RespBody_Marshalling_Unmarshalling_Multiple, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumRespBody序列化与反序列化；覆盖多条记录分支（触发条件：size>1）；
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    std::vector<CloudMdkRecordPhotoAlbumVo> records;
    for (int i = 0; i < 3; i++) {
        CloudMdkRecordPhotoAlbumVo album;
        album.cloudId = "album_cloud_" + std::to_string(i);
        album.albumName = "Test Album " + std::to_string(i);
        album.albumType = i;
        records.push_back(album);
    }

    CloudMdkRecordPhotoAlbumRespBody original(records);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    auto restoredRecords = restored.GetPhotoAlbumRecords();
    EXPECT_EQ(restoredRecords.size(), 3);
    for (size_t i = 0; i < restoredRecords.size(); i++) {
        EXPECT_EQ(restoredRecords[i].cloudId, records[i].cloudId);
        EXPECT_EQ(restoredRecords[i].albumName, records[i].albumName);
        EXPECT_EQ(restoredRecords[i].albumType, records[i].albumType);
    }
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC011_ReqBody_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumReqBody序列化与反序列化；覆盖正常路径（触发条件：正常数据）；
    // 验证业务状态断言：反序列化后的数据与原始数据一致
    CloudMdkRecordPhotoAlbumReqBody original;
    original.size = 10;
    original.isCloudSpaceFull = true;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.size, original.size);
    EXPECT_EQ(restored.isCloudSpaceFull, original.isCloudSpaceFull);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC012_ReqBody_Unmarshalling_ReadInt32_Fail, TestSize.Level1)
{
    // 用例说明：测试CloudMdkRecordPhotoAlbumReqBody反序列化失败；覆盖ReadInt32失败分支（触发条件：ReadInt32失败）；
    // 验证业务状态断言：反序列化返回false
    OHOS::MessageParcel parcel;
    bool ret = parcel.WriteBool(true);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumReqBody reqBody;
    ret = reqBody.Unmarshalling(parcel);
    EXPECT_FALSE(ret);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC001_Stringfields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_123";
    original.albumName = "Test Album";
    original.albumType = 1;
    original.stringfields["custom_field_1"] = "value_1";
    original.stringfields["custom_field_2"] = "value_2";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.stringfields["custom_field_1"], "value_1");
    EXPECT_EQ(restored.stringfields["custom_field_2"], "value_2");
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC002_Int64fields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_456";
    original.albumName = "Test Album 2";
    original.int64fields["int_field_1"] = 1234567890123;
    original.int64fields["int_field_2"] = -9876543210;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.int64fields["int_field_1"], 1234567890123);
    EXPECT_EQ(restored.int64fields["int_field_2"], -9876543210);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC003_MixedFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_789";
    original.albumName = "Test Album 3";
    original.albumType = 2;
    original.stringfields["field_str_1"] = "string_value";
    original.stringfields["field_str_2"] = "";
    original.int64fields["field_int_1"] = 100;
    original.int64fields["field_int_2"] = 0;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.albumType, original.albumType);
    EXPECT_EQ(restored.stringfields["field_str_1"], "string_value");
    EXPECT_EQ(restored.stringfields["field_str_2"], "");
    EXPECT_EQ(restored.int64fields["field_int_1"], 100);
    EXPECT_EQ(restored.int64fields["field_int_2"], 0);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC004_EmptyFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_empty";
    original.albumName = "Empty Fields Album";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.stringfields.size(), 0);
    EXPECT_EQ(restored.int64fields.size(), 0);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC005_MultipleFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo original;
    original.cloudId = "album_cloud_multi";
    original.albumName = "Multiple Fields Album";
    
    for (int i = 0; i < 10; i++) {
        original.stringfields["str_field_" + std::to_string(i)] = "value_" + std::to_string(i);
        original.int64fields["int_field_" + std::to_string(i)] = i * 1000;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumVo restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.stringfields.size(), 10);
    EXPECT_EQ(restored.int64fields.size(), 10);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(restored.stringfields["str_field_" + std::to_string(i)], "value_" + std::to_string(i));
        EXPECT_EQ(restored.int64fields["int_field_" + std::to_string(i)], i * 1000);
    }
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC006_ToString_WithFields_Success, TestSize.Level1)
{
    CloudMdkRecordPhotoAlbumVo vo;
    vo.cloudId = "album_cloud_tostring";
    vo.albumName = "ToString Test Album";
    vo.albumType = 1;
    vo.stringfields["field_1"] = "value_1";
    vo.int64fields["int_field_1"] = 123;

    std::string result = vo.ToString();
    
    EXPECT_TRUE(result.find("\"albumName\":") != std::string::npos);
    EXPECT_TRUE(result.find("\"cloudId\":") != std::string::npos);
    EXPECT_TRUE(result.find("\"stringfields\":") != std::string::npos);
    EXPECT_TRUE(result.find("\"int64fields\":") != std::string::npos);
}

HWTEST_F(CloudMdkRecordPhotoAlbumVoTest, TC007_RespBody_WithFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    std::vector<CloudMdkRecordPhotoAlbumVo> records;
    CloudMdkRecordPhotoAlbumVo album;
    album.cloudId = "album_cloud_resp";
    album.albumName = "Response Album";
    album.stringfields["resp_field"] = "resp_value";
    album.int64fields["resp_int"] = 999;
    records.push_back(album);

    CloudMdkRecordPhotoAlbumRespBody original(records);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    CloudMdkRecordPhotoAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    auto restoredRecords = restored.GetPhotoAlbumRecords();
    EXPECT_EQ(restoredRecords.size(), 1);
    EXPECT_EQ(restoredRecords[0].stringfields["resp_field"], "resp_value");
    EXPECT_EQ(restoredRecords[0].int64fields["resp_int"], 999);
}
}  // namespace OHOS::Media::CloudSync
