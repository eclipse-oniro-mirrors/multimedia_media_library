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
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "cloud_media_album_controller_processor.h"
#include "photo_album_po.h"
#include "cloud_mdkrecord_photo_album_vo.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::CloudSync;

namespace OHOS::Media::CloudSync {

class CloudAlbumProcessorExtTest : public testing::Test {
public:
    void SetUp() override
    {
        processor_ = std::make_unique<CloudMediaAlbumControllerProcessor>();
    }

    void TearDown() override {}

    std::unique_ptr<CloudMediaAlbumControllerProcessor> processor_;
};

HWTEST_F(CloudAlbumProcessorExtTest, TC001_GetAttributesHashMap_Empty_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetAttributesHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
    EXPECT_EQ(albumVo.stringfields.size(), 0);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC002_GetAttributesHashMap_WithFields_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 1;
    albumPo.albumName = "Test Album";
    albumPo.cloudId = "cloud_id_processor";
    albumPo.lpath = "/storage/processor/test";
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetAttributesHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC003_GetAttributesHashMap_WithAttributes_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 2;
    albumPo.albumName = "Attributes Test Album";
    albumPo.attributes["custom_attr_1"] = "attr_value_1";
    albumPo.attributes["custom_attr_2"] = "attr_value_2";
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetAttributesHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC004_GetInt64FieldsHashMap_Empty_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
    EXPECT_EQ(albumVo.int64fields.size(), 0);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC005_GetInt64FieldsHashMap_WithFields_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 3;
    albumPo.dateAdded = 1234567890;
    albumPo.dateModified = 1234567900;
    albumPo.attributes["int_field_1"] = "111";
    albumPo.attributes["int_field_2"] = "222";
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC006_GetInt64FieldsHashMap_InvalidValue_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 4;
    albumPo.attributes["invalid_int"] = "not_a_number";
    albumPo.attributes["valid_int"] = "999";
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC007_ConvertRecordPoToVo_WithFields_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 5;
    albumPo.albumName = "Convert Test Album";
    albumPo.albumType = 1;
    albumPo.cloudId = "convert_cloud_id";
    albumPo.dateAdded = 1111111111;
    albumPo.dateModified = 2222222222;
    albumPo.attributes["convert_attr"] = "convert_value";
    
    CloudMdkRecordPhotoAlbumVo albumVo = processor_->ConvertRecordPoToVo(albumPo);
    
    EXPECT_EQ(albumVo.albumId, 5);
    EXPECT_EQ(albumVo.albumName, "Convert Test Album");
}

HWTEST_F(CloudAlbumProcessorExtTest, TC008_MultipleAttributes_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 6;
    albumPo.albumName = "Multiple Attributes Album";
    
    for (int i = 0; i < 5; i++) {
        albumPo.attributes["attr_" + std::to_string(i)] = std::to_string(i * 100);
    }
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetAttributesHashMap(albumPo, albumVo);
    EXPECT_TRUE(ret);
    
    ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC009_LargeInt64Values_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 7;
    albumPo.attributes["large_int"] = std::to_string(INT64_MAX);
    albumPo.attributes["negative_int"] = std::to_string(INT64_MIN);
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    
    EXPECT_TRUE(ret);
}

HWTEST_F(CloudAlbumProcessorExtTest, TC010_EmptyAttributeValues_Success, TestSize.Level1)
{
    OHOS::Media::ORM::PhotoAlbumPo albumPo;
    albumPo.albumId = 8;
    albumPo.attributes["empty_attr"] = "";
    
    CloudMdkRecordPhotoAlbumVo albumVo;
    
    bool ret = processor_->GetAttributesHashMap(albumPo, albumVo);
    EXPECT_TRUE(ret);
    
    ret = processor_->GetInt64FieldsHashMap(albumPo, albumVo);
    EXPECT_TRUE(ret);
}

}  // namespace OHOS::Media::CloudSync