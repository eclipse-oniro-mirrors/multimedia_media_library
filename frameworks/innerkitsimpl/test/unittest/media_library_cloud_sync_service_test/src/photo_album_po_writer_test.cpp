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
#include <variant>

#include "photo_album_po_writer.h"
#include "photo_album_column.h"
#include "medialibrary_errno.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::ORM;

namespace OHOS::Media::ORM {

class PhotoAlbumPoWriterTest : public testing::Test {
public:
    void SetUp() override
    {
        albumPo_ = PhotoAlbumPo();
    }

    void TearDown() override {}

    PhotoAlbumPo albumPo_;
};

HWTEST_F(PhotoAlbumPoWriterTest, TC001_SetMemberVariable_AlbumId_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 123;
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_ID, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.albumId.has_value());
    EXPECT_EQ(albumPo_.albumId.value(), 123);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC002_SetMemberVariable_AlbumName_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("Test Album Name");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_NAME, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.albumName.has_value());
    EXPECT_EQ(albumPo_.albumName.value(), "Test Album Name");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC003_SetMemberVariable_DateAdded_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = static_cast<int64_t>(1234567890);
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_DATE_ADDED, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.dateAdded.has_value());
    EXPECT_EQ(albumPo_.dateAdded.value(), 1234567890);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC004_SetMemberVariable_CloudId_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("cloud_id_12345");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_CLOUD_ID, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.cloudId.has_value());
    EXPECT_EQ(albumPo_.cloudId.value(), "cloud_id_12345");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC005_SetMemberVariable_NonIdentifyField_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("custom_field_value");
    
    int32_t ret = writer.SetMemberVariable("custom_field", val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(albumPo_.attributes["custom_field"], "custom_field_value");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC006_SetMemberVariable_Int64Attribute_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = static_cast<int64_t>(999888777);
    
    int32_t ret = writer.SetMemberVariable("int64_attribute", val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(albumPo_.attributes["int64_attribute"], "999888777");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC007_SetMemberVariable_Int32Attribute_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 42;
    
    int32_t ret = writer.SetMemberVariable("int32_attribute", val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(albumPo_.attributes["int32_attribute"], "42");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC008_SetMemberVariable_DoubleAttribute_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 3.14159;
    
    int32_t ret = writer.SetMemberVariable("double_attribute", val);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC009_ToMap_WithIdentifyOnly_Success, TestSize.Level1)
{
    albumPo_.albumId = 100;
    albumPo_.albumName = "Test Album";
    albumPo_.cloudId = "test_cloud_id";
    
    PhotoAlbumPoWriter writer(albumPo_);
    auto result = writer.ToMap(true);
    
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_ID) != result.end());
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_NAME) != result.end());
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_CLOUD_ID) != result.end());
}

HWTEST_F(PhotoAlbumPoWriterTest, TC010_ToMap_WithAllFields_Success, TestSize.Level1)
{
    albumPo_.albumId = 200;
    albumPo_.albumName = "All Fields Album";
    albumPo_.attributes["custom_field_1"] = "value_1";
    albumPo_.attributes["custom_field_2"] = "value_2";
    
    PhotoAlbumPoWriter writer(albumPo_);
    auto result = writer.ToMap(false);
    
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_ID) != result.end());
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_NAME) != result.end());
    EXPECT_TRUE(result.find("custom_field_1") != result.end());
    EXPECT_TRUE(result.find("custom_field_2") != result.end());
    EXPECT_EQ(result["custom_field_1"], "value_1");
    EXPECT_EQ(result["custom_field_2"], "value_2");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC011_GetAlbumId_Success, TestSize.Level1)
{
    albumPo_.albumId = 999;
    PhotoAlbumPoWriter writer(albumPo_);
    
    std::string val;
    bool ret = writer.GetAlbumId(val);
    EXPECT_TRUE(ret);
    EXPECT_EQ(val, "999");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC012_GetAlbumId_NoValue_Fail, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    
    std::string val;
    bool ret = writer.GetAlbumId(val);
    EXPECT_FALSE(ret);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC013_GetAlbumName_Success, TestSize.Level1)
{
    albumPo_.albumName = "Get Name Test";
    PhotoAlbumPoWriter writer(albumPo_);
    
    std::string val;
    bool ret = writer.GetAlbumName(val);
    EXPECT_TRUE(ret);
    EXPECT_EQ(val, "Get Name Test");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC014_GetAlbumCloudId_Success, TestSize.Level1)
{
    albumPo_.cloudId = "cloud_id_get_test";
    PhotoAlbumPoWriter writer(albumPo_);
    
    std::string val;
    bool ret = writer.GetAlbumCloudId(val);
    EXPECT_TRUE(ret);
    EXPECT_EQ(val, "cloud_id_get_test");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC015_SetMemberVariable_AlbumType_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 1;
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_TYPE, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.albumType.has_value());
    EXPECT_EQ(albumPo_.albumType.value(), 1);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC016_SetMemberVariable_AlbumSubtype_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 2;
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_SUBTYPE, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.albumSubtype.has_value());
    EXPECT_EQ(albumPo_.albumSubtype.value(), 2);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC017_SetMemberVariable_DateModified_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = static_cast<int64_t>(9876543210);
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.dateModified.has_value());
    EXPECT_EQ(albumPo_.dateModified.value(), 9876543210);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC018_SetMemberVariable_BundleName_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("com.test.bundle");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.bundleName.has_value());
    EXPECT_EQ(albumPo_.bundleName.value(), "com.test.bundle");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC019_SetMemberVariable_LocalLanguage_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("zh-CN");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.localLanguage.has_value());
    EXPECT_EQ(albumPo_.localLanguage.value(), "zh-CN");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC020_SetMemberVariable_CoverUriSource_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 5;
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::COVER_URI_SOURCE, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.coverUriSource.has_value());
    EXPECT_EQ(albumPo_.coverUriSource.value(), 5);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC021_SetMemberVariable_CoverCloudId_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("cover_cloud_id_test");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::COVER_CLOUD_ID, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.coverCloudId.has_value());
    EXPECT_EQ(albumPo_.coverCloudId.value(), "cover_cloud_id_test");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC022_SetMemberVariable_UniqueId_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("unique_id_test_123");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::UNIQUE_ID, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.uniqueId.has_value());
    EXPECT_EQ(albumPo_.uniqueId.value(), "unique_id_test_123");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC023_SetMemberVariable_UploadStatus_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = 1;
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::UPLOAD_STATUS, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(albumPo_.uploadStatus.has_value());
    EXPECT_EQ(albumPo_.uploadStatus.value(), 1);
}

HWTEST_F(PhotoAlbumPoWriterTest, TC024_MultipleSetAndToMap_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    
    std::variant<int32_t, int64_t, double, std::string> val1 = 1;
    std::variant<int32_t, int64_t, double, std::string> val2 = std::string("Multi Test Album");
    std::variant<int32_t, int64_t, double, std::string> val3 = std::string("multi_cloud_id");
    
    writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_TYPE, val1);
    writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_NAME, val2);
    writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_CLOUD_ID, val3);
    
    auto result = writer.ToMap(false);
    
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_TYPE) != result.end());
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_NAME) != result.end());
    EXPECT_TRUE(result.find(PhotoAlbumColumns::ALBUM_CLOUD_ID) != result.end());
    EXPECT_EQ(result[PhotoAlbumColumns::ALBUM_TYPE], "1");
    EXPECT_EQ(result[PhotoAlbumColumns::ALBUM_NAME], "Multi Test Album");
    EXPECT_EQ(result[PhotoAlbumColumns::ALBUM_CLOUD_ID], "multi_cloud_id");
}

HWTEST_F(PhotoAlbumPoWriterTest, TC025_WrongTypeVariant_Success, TestSize.Level1)
{
    PhotoAlbumPoWriter writer(albumPo_);
    std::variant<int32_t, int64_t, double, std::string> val = std::string("wrong_type");
    
    int32_t ret = writer.SetMemberVariable(PhotoAlbumColumns::ALBUM_ID, val);
    EXPECT_EQ(ret, E_OK);
    EXPECT_FALSE(albumPo_.albumId.has_value());
}

}  // namespace OHOS::Media::ORM