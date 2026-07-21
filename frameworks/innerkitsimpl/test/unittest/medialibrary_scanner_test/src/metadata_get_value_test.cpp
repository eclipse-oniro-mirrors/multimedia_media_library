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
 
#include "metadata_get_value_test.h"
 
#include "metadata.h"
 
#include "media_column.h"
#include "media_log.h"
#include "photo_album_column.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void MetadataGetValueTest::SetUp() {}
void MetadataGetValueTest::TearDown() {}
 
/**
 * @tc.name: GetValue_UnknownColumn_test01
 * @tc.desc: 不存在的列名返回空ValueObject
 */
HWTEST_F(MetadataGetValueTest, GetValue_UnknownColumn_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_UnknownColumn_test01");
    Metadata data;
    auto val = data.GetValue("nonexistent_column");
    EXPECT_EQ(val.GetType(), NativeRdb::ValueObject::TYPE_NULL);
    MEDIA_INFO_LOG("end GetValue_UnknownColumn_test01");
}
 
/**
 * @tc.name: GetValue_Int32Field_test02
 * @tc.desc: orientation=90 → GetValue(PHOTO_ORIENTATION) 返回 90
 */
HWTEST_F(MetadataGetValueTest, GetValue_Int32Field_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_Int32Field_test02");
    Metadata data;
    data.SetOrientation(90);
    auto val = data.GetValue(PhotoColumn::PHOTO_ORIENTATION);
    int32_t result = 0;
    val.GetInt(result);
    EXPECT_EQ(result, 90);
    MEDIA_INFO_LOG("end GetValue_Int32Field_test02");
}
 
/**
 * @tc.name: GetValue_Int64Field_test03
 * @tc.desc: fileSize=12345678 → GetValue(MEDIA_SIZE)
 */
HWTEST_F(MetadataGetValueTest, GetValue_Int64Field_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_Int64Field_test03");
    Metadata data;
    data.SetFileSize(static_cast<int64_t>(12345678));
    auto val = data.GetValue(MediaColumn::MEDIA_SIZE);
    int64_t result = 0;
    val.GetLong(result);
    EXPECT_EQ(result, 12345678);
    MEDIA_INFO_LOG("end GetValue_Int64Field_test03");
}
 
/**
 * @tc.name: GetValue_StringRefField_test04
 * @tc.desc: mimeType="image/jpeg" → GetValue(MEDIA_MIME_TYPE)
 */
HWTEST_F(MetadataGetValueTest, GetValue_StringRefField_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_StringRefField_test04");
    Metadata data;
    data.SetFileMimeType("image/jpeg");
    auto val = data.GetValue(MediaColumn::MEDIA_MIME_TYPE);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "image/jpeg");
    MEDIA_INFO_LOG("end GetValue_StringRefField_test04");
}
 
/**
 * @tc.name: GetValue_StringValField_test05
 * @tc.desc: frontCamera="front" → GetValue(PHOTO_FRONT_CAMERA)
 */
HWTEST_F(MetadataGetValueTest, GetValue_StringValField_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_StringValField_test05");
    Metadata data;
    data.SetFrontCamera("front");
    auto val = data.GetValue(PhotoColumn::PHOTO_FRONT_CAMERA);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "front");
    MEDIA_INFO_LOG("end GetValue_StringValField_test05");
}
 
/**
 * @tc.name: GetValue_DoubleField_test06
 * @tc.desc: aspectRatio=1.777 → GetValue(PHOTO_ASPECT_RATIO)
 */
HWTEST_F(MetadataGetValueTest, GetValue_DoubleField_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_DoubleField_test06");
    Metadata data;
    data.SetFileAspectRatio(1.777);
    auto val = data.GetValue(PhotoColumn::PHOTO_ASPECT_RATIO);
    double result = 0.0;
    val.GetDouble(result);
    EXPECT_DOUBLE_EQ(result, 1.777);
    MEDIA_INFO_LOG("end GetValue_DoubleField_test06");
}
 
/**
 * @tc.name: GetValue_MediaTypeCastToInt32_test07
 * @tc.desc: mediaType=IMAGE → GetValue 返回 int32_t 而非枚举
 */
HWTEST_F(MetadataGetValueTest, GetValue_MediaTypeCastToInt32_test07, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_MediaTypeCastToInt32_test07");
    Metadata data;
    data.SetFileMediaType(MEDIA_TYPE_IMAGE);
    auto val = data.GetValue(CONST_MEDIA_DATA_DB_MEDIA_TYPE);
    int32_t result = -1;
    val.GetInt(result);
    EXPECT_EQ(result, static_cast<int32_t>(MEDIA_TYPE_IMAGE));
    MEDIA_INFO_LOG("end GetValue_MediaTypeCastToInt32_test07");
}
 
/**
 * @tc.name: GetValue_MediaTypeVideo_test08
 * @tc.desc: mediaType=VIDEO → int32_t 转换正确
 */
HWTEST_F(MetadataGetValueTest, GetValue_MediaTypeVideo_test08, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_MediaTypeVideo_test08");
    Metadata data;
    data.SetFileMediaType(MEDIA_TYPE_VIDEO);
    auto val = data.GetValue(CONST_MEDIA_DATA_DB_MEDIA_TYPE);
    int32_t result = -1;
    val.GetInt(result);
    EXPECT_EQ(result, static_cast<int32_t>(MEDIA_TYPE_VIDEO));
    MEDIA_INFO_LOG("end GetValue_MediaTypeVideo_test08");
}
 
/**
 * @tc.name: GetValue_OwnerPackage_test09
 * @tc.desc: 测试 GetterStringConstVal 分支(ownerPackage)
 */
HWTEST_F(MetadataGetValueTest, GetValue_OwnerPackage_test09, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_OwnerPackage_test09");
    Metadata data;
    data.SetOwnerPackage("com.test.app");
    auto val = data.GetValue(PhotoColumn::MEDIA_OWNER_PACKAGE);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "com.test.app");
    MEDIA_INFO_LOG("end GetValue_OwnerPackage_test09");
}
 
/**
 * @tc.name: GetValue_ExifRotate_test10
 * @tc.desc: exifRotate=270
 */
HWTEST_F(MetadataGetValueTest, GetValue_ExifRotate_test10, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_ExifRotate_test10");
    Metadata data;
    data.SetExifRotate(270);
    auto val = data.GetValue(PhotoColumn::PHOTO_EXIF_ROTATE);
    int32_t result = 0;
    val.GetInt(result);
    EXPECT_EQ(result, 270);
    MEDIA_INFO_LOG("end GetValue_ExifRotate_test10");
}
 
/**
 * @tc.name: GetValue_LocalAssetSize_test11
 * @tc.desc: localAssetSize=999999
 */
HWTEST_F(MetadataGetValueTest, GetValue_LocalAssetSize_test11, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_LocalAssetSize_test11");
    Metadata data;
    data.SetLocalAssetSize(static_cast<int64_t>(999999));
    auto val = data.GetValue(PhotoColumn::LOCAL_ASSET_SIZE);
    int64_t result = 0;
    val.GetLong(result);
    EXPECT_EQ(result, 999999);
    MEDIA_INFO_LOG("end GetValue_LocalAssetSize_test11");
}
 
/**
 * @tc.name: GetValue_LastVisitTime_test12
 * @tc.desc: lastVisitTime=int64_t 大值
 */
HWTEST_F(MetadataGetValueTest, GetValue_LastVisitTime_test12, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_LastVisitTime_test12");
    Metadata data;
    data.SetLastVisitTime(1718400000000LL);
    auto val = data.GetValue(PhotoColumn::PHOTO_LAST_VISIT_TIME);
    int64_t result = 0;
    val.GetLong(result);
    EXPECT_EQ(result, 1718400000000LL);
    MEDIA_INFO_LOG("end GetValue_LastVisitTime_test12");
}
 
/**
 * @tc.name: GetValue_ShootingMode_test13
 * @tc.desc: shootingMode="mode_hdr"
 */
HWTEST_F(MetadataGetValueTest, GetValue_ShootingMode_test13, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_ShootingMode_test13");
    Metadata data;
    data.SetShootingMode("mode_hdr");
    auto val = data.GetValue(PhotoColumn::PHOTO_SHOOTING_MODE);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "mode_hdr");
    MEDIA_INFO_LOG("end GetValue_ShootingMode_test13");
}
 
/**
 * @tc.name: GetValue_UserComment_test14
 * @tc.desc: userComment="test"
 */
HWTEST_F(MetadataGetValueTest, GetValue_UserComment_test14, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_UserComment_test14");
    Metadata data;
    data.SetUserComment("test");
    auto val = data.GetValue(PhotoColumn::PHOTO_USER_COMMENT);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "test");
    MEDIA_INFO_LOG("end GetValue_UserComment_test14");
}
 
/**
 * @tc.name: GetValue_DetailTime_test15
 * @tc.desc: detailTime="2025:06:15 10:30:00"
 */
HWTEST_F(MetadataGetValueTest, GetValue_DetailTime_test15, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetValue_DetailTime_test15");
    Metadata data;
    data.SetDetailTime("2025:06:15 10:30:00");
    auto val = data.GetValue(PhotoColumn::PHOTO_DETAIL_TIME);
    std::string result;
    val.GetString(result);
    EXPECT_EQ(result, "2025:06:15 10:30:00");
    MEDIA_INFO_LOG("end GetValue_DetailTime_test15");
}
 
/**
 * @tc.name: InitValueFuncMap_Size_test01
 * @tc.desc: 验证 valueFuncMap_ 条目数量符合预期
 */
HWTEST_F(MetadataGetValueTest, InitValueFuncMap_Size_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter InitValueFuncMap_Size_test01");
    Metadata data;
    // InitValueFuncMap is called in constructor via Init()
    // Count entries: 22 int32 + 8 int64 + 18 string(ref) + 3 string(val) + 1 string(const val)
    // + 1 string(ref path) + 3 double + 1 mediatype = 57
    EXPECT_GE(data.valueFuncMap_.size(), 50u);
    MEDIA_INFO_LOG("end InitValueFuncMap_Size_test01, size: %{public}zu", data.valueFuncMap_.size());
}
 
} // namespace Media
} // namespace OHOS