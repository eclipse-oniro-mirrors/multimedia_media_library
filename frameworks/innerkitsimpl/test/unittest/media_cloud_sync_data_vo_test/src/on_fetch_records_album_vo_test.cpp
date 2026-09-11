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

#include "on_fetch_records_album_vo.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <message_parcel.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS::Media::CloudSync {

class OnFetchRecordsAlbumVoTest : public testing::Test {};

HWTEST_F(OnFetchRecordsAlbumVoTest, TC001_AlbumReqData_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_001";
    original.localPath = "/storage/test/album";
    original.albumName = "Test Album";
    original.albumBundleName = "com.test.app";
    original.localLanguage = "zh-CN";
    original.albumId = 123;
    original.priority = 1;
    original.albumType = 1;
    original.albumSubType = 0;
    original.albumDateCreated = 1234567890;
    original.albumDateAdded = 1234567891;
    original.albumDateModified = 1234567892;
    original.isDelete = false;
    original.coverUriSource = 0;
    original.coverCloudId = "cover_cloud_id";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.localPath, original.localPath);
    EXPECT_EQ(restored.albumName, original.albumName);
    EXPECT_EQ(restored.albumBundleName, original.albumBundleName);
    EXPECT_EQ(restored.localLanguage, original.localLanguage);
    EXPECT_EQ(restored.albumId, original.albumId);
    EXPECT_EQ(restored.priority, original.priority);
    EXPECT_EQ(restored.albumType, original.albumType);
    EXPECT_EQ(restored.albumSubType, original.albumSubType);
    EXPECT_EQ(restored.albumDateCreated, original.albumDateCreated);
    EXPECT_EQ(restored.albumDateAdded, original.albumDateAdded);
    EXPECT_EQ(restored.albumDateModified, original.albumDateModified);
    EXPECT_EQ(restored.isDelete, original.isDelete);
    EXPECT_EQ(restored.coverUriSource, original.coverUriSource);
    EXPECT_EQ(restored.coverCloudId, original.coverCloudId);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC002_AlbumReqData_Marshalling_Unmarshalling_IsDeleted_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_002";
    original.localPath = "/storage/test/album2";
    original.albumName = "Test Album 2";
    original.albumBundleName = "com.test.app2";
    original.localLanguage = "en-US";
    original.albumId = 456;
    original.priority = 2;
    original.albumType = 2;
    original.albumSubType = 1;
    original.albumDateCreated = 2234567890;
    original.albumDateAdded = 2234567891;
    original.albumDateModified = 2234567892;
    original.isDelete = true;
    original.coverUriSource = 1;
    original.coverCloudId = "cover_cloud_id_2";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.isDelete, true);
    EXPECT_EQ(restored.coverUriSource, 1);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC003_AlbumReqData_Marshalling_Unmarshalling_EmptyStrings_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "";
    original.localPath = "";
    original.albumName = "";
    original.albumBundleName = "";
    original.localLanguage = "";
    original.albumId = 0;
    original.priority = 0;
    original.albumType = 0;
    original.albumSubType = 0;
    original.albumDateCreated = 0;
    original.albumDateAdded = 0;
    original.albumDateModified = 0;
    original.isDelete = false;
    original.coverUriSource = 0;
    original.coverCloudId = "";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, "");
    EXPECT_EQ(restored.albumId, 0);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC005_ReqBody_Marshalling_Unmarshalling_Empty_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody original;
    original.albums.clear();

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.albums.size(), 0);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC006_ReqBody_Marshalling_Unmarshalling_Single_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody original;
    OnFetchRecordsAlbumReqBody::AlbumReqData album;
    album.cloudId = "cloud_id_001";
    album.albumName = "Test Album";
    album.albumId = 123;
    original.albums.push_back(album);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.albums.size(), 1);
    EXPECT_EQ(restored.albums[0].cloudId, "cloud_id_001");
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC007_ReqBody_Marshalling_Unmarshalling_Multiple_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody original;

    OnFetchRecordsAlbumReqBody::AlbumReqData album1;
    album1.cloudId = "cloud_id_001";
    album1.albumId = 123;
    original.albums.push_back(album1);

    OnFetchRecordsAlbumReqBody::AlbumReqData album2;
    album2.cloudId = "cloud_id_002";
    album2.albumId = 456;
    original.albums.push_back(album2);

    OnFetchRecordsAlbumReqBody::AlbumReqData album3;
    album3.cloudId = "cloud_id_003";
    album3.albumId = 789;
    original.albums.push_back(album3);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.albums.size(), 3);
    EXPECT_EQ(restored.albums[0].cloudId, "cloud_id_001");
    EXPECT_EQ(restored.albums[1].cloudId, "cloud_id_002");
    EXPECT_EQ(restored.albums[2].cloudId, "cloud_id_003");
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC011_RespBody_Marshalling_Unmarshalling_Empty_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumRespBody original;
    original.failedRecords.clear();
    original.stats = {0, 0, 0, 0, 0};

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.failedRecords.size(), 0);
    EXPECT_EQ(restored.stats.size(), 5);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC012_RespBody_Marshalling_Unmarshalling_WithData_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumRespBody original;
    original.failedRecords.push_back("cloud_id_001");
    original.failedRecords.push_back("cloud_id_002");
    original.stats = {1, 2, 3, 4, 5};

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumRespBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.failedRecords.size(), 2);
    EXPECT_EQ(restored.failedRecords[0], "cloud_id_001");
    EXPECT_EQ(restored.failedRecords[1], "cloud_id_002");
    EXPECT_EQ(restored.stats.size(), 5);
    EXPECT_EQ(restored.stats[0], 1);
    EXPECT_EQ(restored.stats[4], 5);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC015_ReqBody_Marshalling_Unmarshalling_LargeVector_Success, TestSize.Level1)
{
    // 用例说明：测试序列化与反序列化；覆盖正常路径（触发条件：正常数据）；验证业务状态断言：反序列化后的数据与原始数据一致
    OnFetchRecordsAlbumReqBody original;
    for (int i = 0; i < 100; i++) {
        OnFetchRecordsAlbumReqBody::AlbumReqData album;
        album.cloudId = "cloud_id_" + std::to_string(i);
        album.albumId = i;
        original.albums.push_back(album);
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.albums.size(), 100);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(restored.albums[i].cloudId, "cloud_id_" + std::to_string(i));
    }
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC001_AlbumReqData_Stringfields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_extension_001";
    original.localPath = "/storage/test/extension";
    original.albumName = "Extension Test Album";
    original.stringfields["extension_field_1"] = "extension_value_1";
    original.stringfields["extension_field_2"] = "extension_value_2";

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.stringfields["extension_field_1"], "extension_value_1");
    EXPECT_EQ(restored.stringfields["extension_field_2"], "extension_value_2");
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC002_AlbumReqData_Int64fields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_extension_002";
    original.localPath = "/storage/test/int64";
    original.albumName = "Int64 Test Album";
    original.int64fields["int_extension_1"] = 111222333444;
    original.int64fields["int_extension_2"] = -555666777888;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.int64fields["int_extension_1"], 111222333444);
    EXPECT_EQ(restored.int64fields["int_extension_2"], -555666777888);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC003_AlbumReqData_MixedFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_mixed";
    original.albumName = "Mixed Fields Album";
    original.albumId = 999;
    original.sceneId = 1;
    original.shareType = 2;
    original.stringfields["mixed_str_1"] = "mixed_value_1";
    original.stringfields["mixed_str_2"] = "";
    original.int64fields["mixed_int_1"] = 12345;
    original.int64fields["mixed_int_2"] = 0;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.albumId, 999);
    EXPECT_EQ(restored.sceneId, 1);
    EXPECT_EQ(restored.shareType, 2);
    EXPECT_EQ(restored.stringfields["mixed_str_1"], "mixed_value_1");
    EXPECT_EQ(restored.stringfields["mixed_str_2"], "");
    EXPECT_EQ(restored.int64fields["mixed_int_1"], 12345);
    EXPECT_EQ(restored.int64fields["mixed_int_2"], 0);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC004_AlbumReqData_EmptyFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_empty_fields";
    original.albumName = "Empty Fields Album";
    original.albumId = 0;

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.cloudId, original.cloudId);
    EXPECT_EQ(restored.stringfields.size(), 0);
    EXPECT_EQ(restored.int64fields.size(), 0);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC005_AlbumReqData_MultiFields_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_id_multiple";
    original.albumName = "Multiple Fields Album";
    
    for (int i = 0; i < 15; i++) {
        original.stringfields["multi_str_" + std::to_string(i)] = "multi_value_" + std::to_string(i);
        original.int64fields["multi_int_" + std::to_string(i)] = i * 10000;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.stringfields.size(), 15);
    EXPECT_EQ(restored.int64fields.size(), 15);
    for (int i = 0; i < 15; i++) {
        EXPECT_EQ(restored.stringfields["multi_str_" + std::to_string(i)], "multi_value_" + std::to_string(i));
        EXPECT_EQ(restored.int64fields["multi_int_" + std::to_string(i)], i * 10000);
    }
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC006_AlbumReqData_ToString_WithFields_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData data;
    data.cloudId = "cloud_id_tostring";
    data.albumName = "ToString Album";
    data.stringfields["tostring_field"] = "tostring_value";
    data.int64fields["tostring_int"] = 888;

    std::string result = data.ToString();
    
    EXPECT_TRUE(result.find("\"cloudId\":") != std::string::npos);
    EXPECT_TRUE(result.find("\"stringfields\":") != std::string::npos);
    EXPECT_TRUE(result.find("\"int64fields\":") != std::string::npos);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC007_ReqBody_WithExtensionFields_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody original;
    
    OnFetchRecordsAlbumReqBody::AlbumReqData album;
    album.cloudId = "cloud_req_body_ext";
    album.albumName = "Request Body Extension";
    album.stringfields["req_field"] = "req_value";
    album.int64fields["req_int"] = 777;
    original.albums.push_back(album);

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    EXPECT_EQ(restored.albums.size(), 1);
    EXPECT_EQ(restored.albums[0].stringfields["req_field"], "req_value");
    EXPECT_EQ(restored.albums[0].int64fields["req_int"], 777);
}

HWTEST_F(OnFetchRecordsAlbumVoTest, TC008_AlbumReqData_LargeData_Marshalling_Unmarshalling_Success, TestSize.Level1)
{
    OnFetchRecordsAlbumReqBody::AlbumReqData original;
    original.cloudId = "cloud_large_data";
    original.albumName = "Large Data Album";
    
    std::string largeString(1000, 'x');
    for (int i = 0; i < 5; i++) {
        original.stringfields["large_str_" + std::to_string(i)] = largeString;
        original.int64fields["large_int_" + std::to_string(i)] = INT64_MAX - i;
    }

    OHOS::MessageParcel parcel;
    bool ret = original.Marshalling(parcel);
    ASSERT_TRUE(ret);

    parcel.RewindRead(0);
    OnFetchRecordsAlbumReqBody::AlbumReqData restored;
    ret = restored.Unmarshalling(parcel);
    ASSERT_TRUE(ret);

    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(restored.stringfields["large_str_" + std::to_string(i)], largeString);
        EXPECT_EQ(restored.int64fields["large_int_" + std::to_string(i)], INT64_MAX - i);
    }
}
}  // namespace OHOS::Media::CloudSync
