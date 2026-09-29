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

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>

#include "cloud_album_data_convert.h"
#include "mdk_record_album_data.h"
#include "on_fetch_records_album_vo.h"
#include "cloud_media_sync_const.h"
#include "photo_album_column.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS::Media::CloudSync {

class CloudAlbumDataConvertTest : public testing::Test {
public:
    void SetUp() override
    {
        convertor_ = std::make_unique<CloudAlbumDataConvert>();
    }

    void TearDown() override {}

    std::unique_ptr<CloudAlbumDataConvert> convertor_;
};

HWTEST_F(CloudAlbumDataConvertTest, TC001_ConvertAttributesHashMap_Empty_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertAttributesHashMap(albumData, reqData);
    
    EXPECT_EQ(reqData.stringfields.size(), 0);
}

HWTEST_F(CloudAlbumDataConvertTest, TC002_ConvertAttributesHashMap_WithFields_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumName("Test Album");
    albumData.SetlPath("/storage/test/album");
    albumData.SetCloudId("cloud_id_convert_test");
    
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertAttributesHashMap(albumData, reqData);
}

HWTEST_F(CloudAlbumDataConvertTest, TC003_ConvertInt64FieldsHashMap_Empty_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertInt64FieldsHashMap(albumData, reqData);
    
    EXPECT_EQ(reqData.int64fields.size(), 0);
}

HWTEST_F(CloudAlbumDataConvertTest, TC004_ConvertInt64FieldsHashMap_WithFields_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetDateAdded(1234567890);
    albumData.SetDateModified(1234567900);
    
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertInt64FieldsHashMap(albumData, reqData);
}

HWTEST_F(CloudAlbumDataConvertTest, TC005_GetAttributeFieldValue_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumName("Field Test Album");
    
    auto result = albumData.GetAttributeFieldValue(PhotoAlbumColumns::ALBUM_NAME);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Field Test Album");
}

HWTEST_F(CloudAlbumDataConvertTest, TC006_GetAttributeFieldValue_NotExist_Empty, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    
    auto result = albumData.GetAttributeFieldValue("non_existent_field");
    
    EXPECT_FALSE(result.has_value());
}

HWTEST_F(CloudAlbumDataConvertTest, TC007_GetAttributeFieldLongValue_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetDateAdded(9876543210);
    
    auto result = albumData.GetAttributeFieldLongValue(PhotoAlbumColumns::ALBUM_DATE_ADDED);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 9876543210);
}

HWTEST_F(CloudAlbumDataConvertTest, TC008_GetAttributeFieldLongValue_NotExist_Empty, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    
    auto result = albumData.GetAttributeFieldLongValue("non_existent_long_field");
    
    EXPECT_FALSE(result.has_value());
}

HWTEST_F(CloudAlbumDataConvertTest, TC009_ConvertAttributesHashMap_MultipleFields_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumName("Multiple Fields Album");
    albumData.SetlPath("/storage/multi/album");
    albumData.SetBundleName("com.test.multiple");
    albumData.SetLocalLanguage("en-US");
    
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertAttributesHashMap(albumData, reqData);
}

HWTEST_F(CloudAlbumDataConvertTest, TC010_ConvertInt64FieldsHashMap_MultipleFields_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetDateAdded(1111111111);
    albumData.SetDateModified(2222222222);
    
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertInt64FieldsHashMap(albumData, reqData);
}

HWTEST_F(CloudAlbumDataConvertTest, TC011_MixedConvert_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumName("Mixed Test Album");
    albumData.SetlPath("/storage/mixed");
    albumData.SetCloudId("mixed_cloud_id");
    albumData.SetDateAdded(3333333333);
    albumData.SetDateModified(4444444444);
    
    OnFetchRecordsAlbumReqBody::AlbumReqData reqData;
    
    convertor_->ConvertAttributesHashMap(albumData, reqData);
    convertor_->ConvertInt64FieldsHashMap(albumData, reqData);
}

HWTEST_F(CloudAlbumDataConvertTest, TC012_SetAndGetAlbumType_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumType(1);
    
    auto result = albumData.GetAlbumType();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

HWTEST_F(CloudAlbumDataConvertTest, TC013_SetAndGetAlbumSubType_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetAlbumSubType(2);
    
    auto result = albumData.GetAlbumSubType();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 2);
}

HWTEST_F(CloudAlbumDataConvertTest, TC014_SetAndGetCloudId_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetCloudId("cloud_id_test_123");
    
    auto result = albumData.GetCloudId();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "cloud_id_test_123");
}

HWTEST_F(CloudAlbumDataConvertTest, TC015_SetAndGetBundleName_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetBundleName("com.test.bundle.name");
    
    auto result = albumData.GetBundleName();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "com.test.bundle.name");
}

HWTEST_F(CloudAlbumDataConvertTest, TC016_SetAndGetLocalLanguage_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetLocalLanguage("zh-CN");
    
    auto result = albumData.GetLocalLanguage();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "zh-CN");
}

HWTEST_F(CloudAlbumDataConvertTest, TC017_SetAndGetUniqueId_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetUniqueId("unique_id_test");
    
    auto result = albumData.GetUniqueId();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "unique_id_test");
}

HWTEST_F(CloudAlbumDataConvertTest, TC018_SetAndGetSceneId_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetSceneId(10);
    
    auto result = albumData.GetSceneId();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 10);
}

HWTEST_F(CloudAlbumDataConvertTest, TC019_SetAndGetShareType_Success, TestSize.Level1)
{
    MDKRecordAlbumData albumData;
    albumData.SetShareType(5);
    
    auto result = albumData.GetShareType();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 5);
}

}  // namespace OHOS::Media::CloudSync