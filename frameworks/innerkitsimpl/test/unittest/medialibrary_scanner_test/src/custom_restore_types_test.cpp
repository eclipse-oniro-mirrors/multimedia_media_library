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
 
#include "custom_restore_types_test.h"
 
#include "media_log.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void CustomRestoreTypesTest::SetUp() {}
void CustomRestoreTypesTest::TearDown() {}
 
/**
 * @tc.name: RestoreFileInfo_DefaultValues_test01
 * @tc.desc: 默认构造验证所有字段默认值
 */
HWTEST_F(CustomRestoreTypesTest, RestoreFileInfo_DefaultValues_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter RestoreFileInfo_DefaultValues_test01");
    RestoreFileInfo info;
    EXPECT_EQ(info.mediaType, MEDIA_TYPE_FILE);
    EXPECT_EQ(info.size, 0);
    EXPECT_EQ(info.orientation, 0);
    EXPECT_FALSE(info.isLivePhoto);
    EXPECT_EQ(info.fileId, 0);
    EXPECT_TRUE(info.mimeType.empty());
    EXPECT_EQ(info.subtype, 0);
    EXPECT_EQ(info.movingPhotoEffectMode, 0);
    EXPECT_TRUE(info.frontCamera.empty());
    EXPECT_TRUE(info.shootingMode.empty());
    EXPECT_EQ(info.albumId, 0);
    EXPECT_TRUE(info.originFilePath.empty());
    EXPECT_TRUE(info.filePath.empty());
    EXPECT_TRUE(info.fileName.empty());
    EXPECT_TRUE(info.displayName.empty());
    EXPECT_TRUE(info.title.empty());
    EXPECT_TRUE(info.extension.empty());
    MEDIA_INFO_LOG("end RestoreFileInfo_DefaultValues_test01");
}
 
/**
 * @tc.name: RestoreFileInfo_AllFieldsSet_test02
 * @tc.desc: 设置所有字段为非默认值并验证回读
 */
HWTEST_F(CustomRestoreTypesTest, RestoreFileInfo_AllFieldsSet_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter RestoreFileInfo_AllFieldsSet_test02");
    RestoreFileInfo info;
    info.originFilePath = "/data/origin.jpg";
    info.filePath = "/data/dest.jpg";
    info.fileName = "dest.jpg";
    info.displayName = "dest";
    info.title = "dest";
    info.extension = "jpg";
    info.mediaType = MEDIA_TYPE_IMAGE;
    info.size = 1024;
    info.orientation = 90;
    info.isLivePhoto = true;
    info.fileId = 42;
    info.mimeType = "image/jpeg";
    info.subtype = 1;
    info.movingPhotoEffectMode = 2;
    info.frontCamera = "front";
    info.shootingMode = "mode_hdr";
    info.albumId = 5;
 
    EXPECT_EQ(info.originFilePath, "/data/origin.jpg");
    EXPECT_EQ(info.filePath, "/data/dest.jpg");
    EXPECT_EQ(info.fileName, "dest.jpg");
    EXPECT_EQ(info.displayName, "dest");
    EXPECT_EQ(info.title, "dest");
    EXPECT_EQ(info.extension, "jpg");
    EXPECT_EQ(info.mediaType, MEDIA_TYPE_IMAGE);
    EXPECT_EQ(info.size, 1024);
    EXPECT_EQ(info.orientation, 90);
    EXPECT_TRUE(info.isLivePhoto);
    EXPECT_EQ(info.fileId, 42);
    EXPECT_EQ(info.mimeType, "image/jpeg");
    EXPECT_EQ(info.subtype, 1);
    EXPECT_EQ(info.movingPhotoEffectMode, 2);
    EXPECT_EQ(info.frontCamera, "front");
    EXPECT_EQ(info.shootingMode, "mode_hdr");
    EXPECT_EQ(info.albumId, 5);
    MEDIA_INFO_LOG("end RestoreFileInfo_AllFieldsSet_test02");
}
 
/**
 * @tc.name: UniqueNumber_DefaultValues_test01
 * @tc.desc: 默认构造验证4个int32_t全为0
 */
HWTEST_F(CustomRestoreTypesTest, UniqueNumber_DefaultValues_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter UniqueNumber_DefaultValues_test01");
    UniqueNumber num;
    EXPECT_EQ(num.imageTotalNumber, 0);
    EXPECT_EQ(num.videoTotalNumber, 0);
    EXPECT_EQ(num.imageCurrentNumber, 0);
    EXPECT_EQ(num.videoCurrentNumber, 0);
    MEDIA_INFO_LOG("end UniqueNumber_DefaultValues_test01");
}
 
/**
 * @tc.name: UniqueNumber_OperatorPlus_test02
 * @tc.desc: 两个不同值相加，各字段独立求和
 */
HWTEST_F(CustomRestoreTypesTest, UniqueNumber_OperatorPlus_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter UniqueNumber_OperatorPlus_test02");
    UniqueNumber a;
    a.imageTotalNumber = 10;
    a.videoTotalNumber = 20;
    a.imageCurrentNumber = 5;
    a.videoCurrentNumber = 15;
 
    UniqueNumber b;
    b.imageTotalNumber = 1;
    b.videoTotalNumber = 2;
    b.imageCurrentNumber = 3;
    b.videoCurrentNumber = 4;
 
    UniqueNumber c = a + b;
    EXPECT_EQ(c.imageTotalNumber, 11);
    EXPECT_EQ(c.videoTotalNumber, 22);
    EXPECT_EQ(c.imageCurrentNumber, 8);
    EXPECT_EQ(c.videoCurrentNumber, 19);
    MEDIA_INFO_LOG("end UniqueNumber_OperatorPlus_test02");
}
 
/**
 * @tc.name: UniqueNumber_OperatorPlusZero_test03
 * @tc.desc: 加全零对象，值不变
 */
HWTEST_F(CustomRestoreTypesTest, UniqueNumber_OperatorPlusZero_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter UniqueNumber_OperatorPlusZero_test03");
    UniqueNumber a;
    a.imageTotalNumber = 100;
    a.videoTotalNumber = 200;
    a.imageCurrentNumber = 50;
    a.videoCurrentNumber = 150;
 
    UniqueNumber zero;
    UniqueNumber result = a + zero;
    EXPECT_EQ(result.imageTotalNumber, 100);
    EXPECT_EQ(result.videoTotalNumber, 200);
    EXPECT_EQ(result.imageCurrentNumber, 50);
    EXPECT_EQ(result.videoCurrentNumber, 150);
    MEDIA_INFO_LOG("end UniqueNumber_OperatorPlusZero_test03");
}
 
/**
 * @tc.name: UniqueNumber_Clear_test04
 * @tc.desc: 填入值后clear()，全部归零
 */
HWTEST_F(CustomRestoreTypesTest, UniqueNumber_Clear_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter UniqueNumber_Clear_test04");
    UniqueNumber num;
    num.imageTotalNumber = 100;
    num.videoTotalNumber = 200;
    num.imageCurrentNumber = 50;
    num.videoCurrentNumber = 150;
    num.clear();
    EXPECT_EQ(num.imageTotalNumber, 0);
    EXPECT_EQ(num.videoTotalNumber, 0);
    EXPECT_EQ(num.imageCurrentNumber, 0);
    EXPECT_EQ(num.videoCurrentNumber, 0);
    MEDIA_INFO_LOG("end UniqueNumber_Clear_test04");
}
 
/**
 * @tc.name: UniqueNumber_ClearIdempotent_test05
 * @tc.desc: 对已清零对象再clear()，仍全零
 */
HWTEST_F(CustomRestoreTypesTest, UniqueNumber_ClearIdempotent_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter UniqueNumber_ClearIdempotent_test05");
    UniqueNumber num;
    num.imageTotalNumber = 100;
    num.clear();
    num.clear();
    EXPECT_EQ(num.imageTotalNumber, 0);
    EXPECT_EQ(num.videoTotalNumber, 0);
    EXPECT_EQ(num.imageCurrentNumber, 0);
    EXPECT_EQ(num.videoCurrentNumber, 0);
    MEDIA_INFO_LOG("end UniqueNumber_ClearIdempotent_test05");
}
 
/**
 * @tc.name: TimeInfo_DefaultValues_test01
 * @tc.desc: 默认构造验证dateAdded=0, dateTaken=0, detailTime空
 */
HWTEST_F(CustomRestoreTypesTest, TimeInfo_DefaultValues_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter TimeInfo_DefaultValues_test01");
    TimeInfo info;
    EXPECT_EQ(info.dateAdded, 0);
    EXPECT_EQ(info.dateTaken, 0);
    EXPECT_TRUE(info.detailTime.empty());
    MEDIA_INFO_LOG("end TimeInfo_DefaultValues_test01");
}
 
/**
 * @tc.name: TimeInfo_SetValues_test02
 * @tc.desc: 设置已知值并验证回读
 */
HWTEST_F(CustomRestoreTypesTest, TimeInfo_SetValues_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter TimeInfo_SetValues_test02");
    TimeInfo info;
    info.dateAdded = 1718400000000LL;
    info.dateTaken = 1718400010000LL;
    info.detailTime = "2025:06:15 10:30:00";
    EXPECT_EQ(info.dateAdded, 1718400000000LL);
    EXPECT_EQ(info.dateTaken, 1718400010000LL);
    EXPECT_EQ(info.detailTime, "2025:06:15 10:30:00");
    MEDIA_INFO_LOG("end TimeInfo_SetValues_test02");
}
 
/**
 * @tc.name: TimeInfo_CopySemantics_test03
 * @tc.desc: 拷贝构造验证字段匹配
 */
HWTEST_F(CustomRestoreTypesTest, TimeInfo_CopySemantics_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter TimeInfo_CopySemantics_test03");
    TimeInfo original;
    original.dateAdded = 1718400000000LL;
    original.dateTaken = 1718400010000LL;
    original.detailTime = "2025:06:15 10:30:00";
 
    TimeInfo copy(original);
    EXPECT_EQ(copy.dateAdded, original.dateAdded);
    EXPECT_EQ(copy.dateTaken, original.dateTaken);
    EXPECT_EQ(copy.detailTime, original.detailTime);
    MEDIA_INFO_LOG("end TimeInfo_CopySemantics_test03");
}
 
/**
 * @tc.name: RestoreFileInfo_CopySemantics_test04
 * @tc.desc: 拷贝构造RestoreFileInfo验证所有字段匹配
 */
HWTEST_F(CustomRestoreTypesTest, RestoreFileInfo_CopySemantics_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter RestoreFileInfo_CopySemantics_test04");
    RestoreFileInfo original;
    original.originFilePath = "/orig";
    original.filePath = "/dest";
    original.fileName = "file.jpg";
    original.displayName = "file";
    original.title = "file";
    original.extension = "jpg";
    original.mediaType = MEDIA_TYPE_IMAGE;
    original.size = 2048;
    original.orientation = 270;
    original.isLivePhoto = true;
    original.fileId = 99;
    original.mimeType = "image/png";
    original.subtype = 3;
    original.movingPhotoEffectMode = 7;
    original.frontCamera = "back";
    original.shootingMode = "mode_night";
    original.albumId = 10;
 
    RestoreFileInfo copy(original);
    EXPECT_EQ(copy.originFilePath, original.originFilePath);
    EXPECT_EQ(copy.filePath, original.filePath);
    EXPECT_EQ(copy.fileName, original.fileName);
    EXPECT_EQ(copy.displayName, original.displayName);
    EXPECT_EQ(copy.title, original.title);
    EXPECT_EQ(copy.extension, original.extension);
    EXPECT_EQ(copy.mediaType, original.mediaType);
    EXPECT_EQ(copy.size, original.size);
    EXPECT_EQ(copy.orientation, original.orientation);
    EXPECT_EQ(copy.isLivePhoto, original.isLivePhoto);
    EXPECT_EQ(copy.fileId, original.fileId);
    EXPECT_EQ(copy.mimeType, original.mimeType);
    EXPECT_EQ(copy.subtype, original.subtype);
    EXPECT_EQ(copy.movingPhotoEffectMode, original.movingPhotoEffectMode);
    EXPECT_EQ(copy.frontCamera, original.frontCamera);
    EXPECT_EQ(copy.shootingMode, original.shootingMode);
    EXPECT_EQ(copy.albumId, original.albumId);
    MEDIA_INFO_LOG("end RestoreFileInfo_CopySemantics_test04");
}
 
} // namespace Media
} // namespace OHOS