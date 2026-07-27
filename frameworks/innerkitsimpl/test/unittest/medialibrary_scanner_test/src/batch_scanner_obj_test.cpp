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
#include "batch_scanner_obj_test.h"

#include "batch_scanner_obj.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void BatchScannerObjTest::SetUp() {}
void BatchScannerObjTest::TearDown() {}

// ==================== Constructor tests ====================

/**
 * @tc.name: Constructor_ValidCustomRestoreInfo_test01
 * @tc.desc: items_ 为空，customRestoreInfo_ 引用匹配
 */
HWTEST_F(BatchScannerObjTest, Constructor_ValidCustomRestoreInfo_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Constructor_ValidCustomRestoreInfo_test01");
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/test/file.jpg"});
    BatchScannerObj obj(customInfo);
    EXPECT_EQ(&obj.customRestoreInfo_, &customInfo);
    EXPECT_TRUE(obj.items_.empty());
    MEDIA_INFO_LOG("end Constructor_ValidCustomRestoreInfo_test01");
}

// ==================== PostProcess tests (directly fill items_) ====================

/**
 * @tc.name: PostProcess_AllSuccess_test01
 * @tc.desc: 3个非重复项 → outFileInfos=3, sameNum=0, successNum=3
 */
HWTEST_F(BatchScannerObjTest, PostProcess_AllSuccess_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_AllSuccess_test01");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    for (int i = 0; i < 3; i++) {
        BatchScannerObj::BatchScanItem item;
        item.isDuplicate = false;
        item.metadata = std::make_unique<Metadata>();
        item.fileInfo.fileName = "file" + to_string(i) + ".jpg";
        obj.items_.push_back(std::move(item));
    }

    obj.PostProcess();
    EXPECT_EQ(customInfo.GetOutFileInfos().size(), 3u);
    EXPECT_EQ(customInfo.GetOutSameFileNum(), 0);
    EXPECT_EQ(customInfo.GetOutSuccessFileNum(), 3);
    MEDIA_INFO_LOG("end PostProcess_AllSuccess_test01");
}

/**
 * @tc.name: PostProcess_AllDuplicate_test02
 * @tc.desc: 3个重复项 → outFileInfos=0, sameNum=3, successNum=0
 */
HWTEST_F(BatchScannerObjTest, PostProcess_AllDuplicate_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_AllDuplicate_test02");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    for (int i = 0; i < 3; i++) {
        BatchScannerObj::BatchScanItem item;
        item.isDuplicate = true;
        obj.items_.push_back(std::move(item));
    }

    obj.PostProcess();
    EXPECT_EQ(customInfo.GetOutFileInfos().size(), 0u);
    EXPECT_EQ(customInfo.GetOutSameFileNum(), 3);
    EXPECT_EQ(customInfo.GetOutSuccessFileNum(), 0);
    MEDIA_INFO_LOG("end PostProcess_AllDuplicate_test02");
}

/**
 * @tc.name: PostProcess_Mixed_test03
 * @tc.desc: 2成功+1重复 → outFileInfos=2
 */
HWTEST_F(BatchScannerObjTest, PostProcess_Mixed_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_Mixed_test03");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item1;
    item1.isDuplicate = false;
    item1.metadata = std::make_unique<Metadata>();
    item1.fileInfo.fileName = "file1.jpg";
    obj.items_.push_back(std::move(item1));

    BatchScannerObj::BatchScanItem item2;
    item2.isDuplicate = true;
    obj.items_.push_back(std::move(item2));

    BatchScannerObj::BatchScanItem item3;
    item3.isDuplicate = false;
    item3.metadata = std::make_unique<Metadata>();
    item3.fileInfo.fileName = "file3.jpg";
    obj.items_.push_back(std::move(item3));

    obj.PostProcess();
    EXPECT_EQ(customInfo.GetOutFileInfos().size(), 2u);
    EXPECT_EQ(customInfo.GetOutSameFileNum(), 1);
    EXPECT_EQ(customInfo.GetOutSuccessFileNum(), 2);
    MEDIA_INFO_LOG("end PostProcess_Mixed_test03");
}

/**
 * @tc.name: PostProcess_BackFillsMimeType_test04
 * @tc.desc: metadata mimeType 回填到 fileInfo
 */
HWTEST_F(BatchScannerObjTest, PostProcess_BackFillsMimeType_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_BackFillsMimeType_test04");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMimeType("image/jpeg");
    item.fileInfo.fileName = "test.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_EQ(customInfo.GetOutFileInfos()[0].mimeType, "image/jpeg");
    MEDIA_INFO_LOG("end PostProcess_BackFillsMimeType_test04");
}

/**
 * @tc.name: PostProcess_BackFillsShootingMode_test05
 * @tc.desc: shootingMode 回填
 */
HWTEST_F(BatchScannerObjTest, PostProcess_BackFillsShootingMode_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_BackFillsShootingMode_test05");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetShootingMode("mode_hdr");
    item.fileInfo.fileName = "test.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_EQ(customInfo.GetOutFileInfos()[0].shootingMode, "mode_hdr");
    MEDIA_INFO_LOG("end PostProcess_BackFillsShootingMode_test05");
}

/**
 * @tc.name: PostProcess_BackFillsFrontCamera_test06
 * @tc.desc: frontCamera 回填
 */
HWTEST_F(BatchScannerObjTest, PostProcess_BackFillsFrontCamera_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_BackFillsFrontCamera_test06");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFrontCamera("front");
    item.fileInfo.fileName = "test.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_EQ(customInfo.GetOutFileInfos()[0].frontCamera, "front");
    MEDIA_INFO_LOG("end PostProcess_BackFillsFrontCamera_test06");
}

/**
 * @tc.name: PostProcess_LivePhotoSubtype_test07
 * @tc.desc: isLivePhoto → subtype=MOVING_PHOTO
 */
HWTEST_F(BatchScannerObjTest, PostProcess_LivePhotoSubtype_test07, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_LivePhotoSubtype_test07");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.fileInfo.isLivePhoto = true;
    item.fileInfo.fileName = "live.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_EQ(customInfo.GetOutFileInfos()[0].subtype, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    MEDIA_INFO_LOG("end PostProcess_LivePhotoSubtype_test07");
}

/**
 * @tc.name: PostProcess_Spatial3DGSOverride_test08
 * @tc.desc: PhotoSubType==SPATIAL_3DGS 覆盖
 */
HWTEST_F(BatchScannerObjTest, PostProcess_Spatial3DGSOverride_test08, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_Spatial3DGSOverride_test08");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
    item.fileInfo.fileName = "spatial.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_EQ(customInfo.GetOutFileInfos()[0].subtype, static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
    MEDIA_INFO_LOG("end PostProcess_Spatial3DGSOverride_test08");
}

/**
 * @tc.name: PostProcess_NullMetadataNotBackFilled_test09
 * @tc.desc: metadata=null 但 isDuplicate=false → mimeType 保持默认空
 */
HWTEST_F(BatchScannerObjTest, PostProcess_NullMetadataNotBackFilled_test09, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter PostProcess_NullMetadataNotBackFilled_test09");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = nullptr;
    item.fileInfo.fileName = "test.jpg";
    obj.items_.push_back(std::move(item));

    obj.PostProcess();
    ASSERT_EQ(customInfo.GetOutFileInfos().size(), 1u);
    EXPECT_TRUE(customInfo.GetOutFileInfos()[0].mimeType.empty());
    MEDIA_INFO_LOG("end PostProcess_NullMetadataNotBackFilled_test09");
}

// ==================== ConvertToValues tests (directly fill items_ metadata) ====================

/**
 * @tc.name: ConvertToValues_ImageFile_test01
 * @tc.desc: 验证 MEDIA_FILE_PATH, MEDIA_TYPE, UNIQUE_ID
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_ImageFile_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_ImageFile_test01");
    CustomRestoreInfo customInfo;
    customInfo.SetPackageName("com.test");
    customInfo.SetBundleName("com.test.bundle");
    customInfo.SetAppId("appId123");
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(1024));
    item.metadata->SetOrientation(0);
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/test.jpg";
    item.fileInfo.title = "test";
    item.fileInfo.displayName = "test.jpg";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    obj.items_.push_back(std::move(item));

    int32_t result = obj.ConvertToValues();
    EXPECT_EQ(result, E_OK);
    EXPECT_FALSE(obj.items_[0].values.IsEmpty());
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_FILE_PATH));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_TYPE));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(PhotoColumn::UNIQUE_ID));
    MEDIA_INFO_LOG("end ConvertToValues_ImageFile_test01");
}

/**
 * @tc.name: ConvertToValues_VideoFile_test02
 * @tc.desc: VIDEO 类型也生成 UNIQUE_ID
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_VideoFile_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_VideoFile_test02");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_VIDEO);
    item.metadata->SetFileSize(static_cast<int64_t>(2048));
    item.metadata->SetOrientation(0);
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/test.mp4";
    item.fileInfo.title = "test";
    item.fileInfo.displayName = "test.mp4";
    item.fileInfo.mediaType = MEDIA_TYPE_VIDEO;
    obj.items_.push_back(std::move(item));

    int32_t result = obj.ConvertToValues();
    EXPECT_EQ(result, E_OK);
    EXPECT_TRUE(obj.items_[0].values.HasColumn(PhotoColumn::UNIQUE_ID));
    MEDIA_INFO_LOG("end ConvertToValues_VideoFile_test02");
}

/**
 * @tc.name: ConvertToValues_LivePhotoSubtype_test03
 * @tc.desc: isLivePhoto → PHOTO_SUBTYPE=MOVING_PHOTO
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_LivePhotoSubtype_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_LivePhotoSubtype_test03");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(1024));
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/live.jpg";
    item.fileInfo.title = "live";
    item.fileInfo.displayName = "live.jpg";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    item.fileInfo.isLivePhoto = true;
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    EXPECT_TRUE(obj.items_[0].values.HasColumn(PhotoColumn::PHOTO_SUBTYPE));
    NativeRdb::ValueObject subtypeObj;
    obj.items_[0].values.GetObject(PhotoColumn::PHOTO_SUBTYPE, subtypeObj);
    int32_t subtype = 0;
    subtypeObj.GetInt(subtype);
    EXPECT_EQ(subtype, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    MEDIA_INFO_LOG("end ConvertToValues_LivePhotoSubtype_test03");
}

/**
 * @tc.name: ConvertToValues_Spatial3DGSSubtype_test04
 * @tc.desc: SPATIAL_3DGS → PHOTO_SUBTYPE 覆盖
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_Spatial3DGSSubtype_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_Spatial3DGSSubtype_test04");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(1024));
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.metadata->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
    item.fileInfo.filePath = "/data/spatial.jpg";
    item.fileInfo.title = "spatial";
    item.fileInfo.displayName = "spatial.jpg";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    NativeRdb::ValueObject subtypeObj;
    obj.items_[0].values.GetObject(PhotoColumn::PHOTO_SUBTYPE, subtypeObj);
    int32_t subtype = 0;
    subtypeObj.GetInt(subtype);
    EXPECT_EQ(subtype, static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
    MEDIA_INFO_LOG("end ConvertToValues_Spatial3DGSSubtype_test04");
}

/**
 * @tc.name: ConvertToValues_DirectFields_test05
 * @tc.desc: 验证 directFields 循环填充了 ORIENTATION, MIME_TYPE, SIZE
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_DirectFields_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_DirectFields_test05");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(4096));
    item.metadata->SetOrientation(90);
    item.metadata->SetFileMimeType("image/png");
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/test.png";
    item.fileInfo.title = "test";
    item.fileInfo.displayName = "test.png";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    // Check direct fields were written
    EXPECT_TRUE(obj.items_[0].values.HasColumn(PhotoColumn::PHOTO_ORIENTATION));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_MIME_TYPE));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_SIZE));
    MEDIA_INFO_LOG("end ConvertToValues_DirectFields_test05");
}

/**
 * @tc.name: ConvertToValues_SkipsDuplicate_test06
 * @tc.desc: 重复项不生成 values
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_SkipsDuplicate_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_SkipsDuplicate_test06");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = true;
    item.metadata = std::make_unique<Metadata>();
    item.fileInfo.filePath = "/data/dup.jpg";
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    EXPECT_TRUE(obj.items_[0].values.IsEmpty());
    MEDIA_INFO_LOG("end ConvertToValues_SkipsDuplicate_test06");
}

/**
 * @tc.name: ConvertToValues_NullMetadataMarkedDuplicate_test07
 * @tc.desc: metadata=null → 标记 isDuplicate=true
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_NullMetadataMarkedDuplicate_test07, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_NullMetadataMarkedDuplicate_test07");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = nullptr;
    item.fileInfo.filePath = "/data/test.jpg";
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    EXPECT_TRUE(obj.items_[0].isDuplicate);
    MEDIA_INFO_LOG("end ConvertToValues_NullMetadataMarkedDuplicate_test07");
}

/**
 * @tc.name: ConvertToValues_PackageFields_test08
 * @tc.desc: packageName/bundleName/appId 写入 values
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_PackageFields_test08, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_PackageFields_test08");
    CustomRestoreInfo customInfo;
    customInfo.SetPackageName("com.test.pkg");
    customInfo.SetBundleName("com.test.bundle");
    customInfo.SetAppId("appId456");
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(1024));
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/test.jpg";
    item.fileInfo.title = "test";
    item.fileInfo.displayName = "test.jpg";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_PACKAGE_NAME));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_OWNER_PACKAGE));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_OWNER_APPID));
    MEDIA_INFO_LOG("end ConvertToValues_PackageFields_test08");
}

/**
 * @tc.name: ConvertToValues_TimePendingAndQuality_test09
 * @tc.desc: TIME_PENDING=-1, PHOTO_QUALITY=0
 */
HWTEST_F(BatchScannerObjTest, ConvertToValues_TimePendingAndQuality_test09, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ConvertToValues_TimePendingAndQuality_test09");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    item.metadata->SetFileSize(static_cast<int64_t>(1024));
    item.metadata->SetFileDateModified(1718400000000LL);
    item.metadata->SetFileDateAdded(1718400001000LL);
    item.metadata->SetDateTaken(1718400002000LL);
    item.fileInfo.filePath = "/data/test.jpg";
    item.fileInfo.title = "test";
    item.fileInfo.displayName = "test.jpg";
    item.fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    obj.items_.push_back(std::move(item));

    obj.ConvertToValues();
    EXPECT_TRUE(obj.items_[0].values.HasColumn(MediaColumn::MEDIA_TIME_PENDING));
    EXPECT_TRUE(obj.items_[0].values.HasColumn(PhotoColumn::PHOTO_QUALITY));
    NativeRdb::ValueObject tpObj;
    obj.items_[0].values.GetObject(MediaColumn::MEDIA_TIME_PENDING, tpObj);
    int64_t timePending = 0;
    tpObj.GetLong(timePending);
    EXPECT_EQ(timePending, -1);
    NativeRdb::ValueObject qObj;
    obj.items_[0].values.GetObject(PhotoColumn::PHOTO_QUALITY, qObj);
    int32_t quality = -1;
    qObj.GetInt(quality);
    EXPECT_EQ(quality, 0);
    MEDIA_INFO_LOG("end ConvertToValues_TimePendingAndQuality_test09");
}

// ==================== Integration tests ====================

/**
 * @tc.name: Execute_EmptyFileInfos_test01
 * @tc.desc: 空列表 → E_OK, outFileInfos=0
 */
HWTEST_F(BatchScannerObjTest, Execute_EmptyFileInfos_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Execute_EmptyFileInfos_test01");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);
    int32_t result = obj.Execute();
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(customInfo.GetOutFileInfos().size(), 0u);
    MEDIA_INFO_LOG("end Execute_EmptyFileInfos_test01");
}

/**
 * @tc.name: ResolveMetadata_NonExistentFile_test01
 * @tc.desc: 不存在的文件 → metadata=null, isDuplicate=true
 */
HWTEST_F(BatchScannerObjTest, ResolveMetadata_NonExistentFile_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ResolveMetadata_NonExistentFile_test01");
    CustomRestoreInfo customInfo;
    RestoreFileInfo fileInfo;
    fileInfo.originFilePath = "/nonexistent/file.jpg";
    fileInfo.fileName = "file.jpg";
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    customInfo.SetFileInfos({fileInfo});
    BatchScannerObj obj(customInfo);

    int32_t result = obj.ResolveMetadata();
    EXPECT_EQ(result, E_OK);
    // FillMetadata fails for nonexistent file → metadata=null, isDuplicate=true
    EXPECT_EQ(obj.items_.size(), 1u);
    EXPECT_TRUE(obj.items_[0].isDuplicate);
    MEDIA_INFO_LOG("end ResolveMetadata_NonExistentFile_test01");
}

/**
 * @tc.name: Deduplicate_AlreadyDuplicateSkipped_test01
 * @tc.desc: 已标记重复的项不进入去重逻辑
 */
HWTEST_F(BatchScannerObjTest, Deduplicate_AlreadyDuplicateSkipped_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Deduplicate_AlreadyDuplicateSkipped_test01");
    CustomRestoreInfo customInfo;
    customInfo.SetIsDeduplication(true);
    customInfo.SetAlbumId(1);
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = true;
    item.metadata = std::make_unique<Metadata>();
    item.fileInfo.fileName = "dup.jpg";
    obj.items_.push_back(std::move(item));

    int32_t result = obj.Deduplicate();
    EXPECT_EQ(result, E_OK);
    // Already duplicate remains duplicate
    EXPECT_TRUE(obj.items_[0].isDuplicate);
    MEDIA_INFO_LOG("end Deduplicate_AlreadyDuplicateSkipped_test01");
}

/**
 * @tc.name: Deduplicate_BackFillsSizeOrientation_test02
 * @tc.desc: metadata 的 size/orientation 回填到 fileInfo
 */
HWTEST_F(BatchScannerObjTest, Deduplicate_BackFillsSizeOrientation_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Deduplicate_BackFillsSizeOrientation_test02");
    CustomRestoreInfo customInfo;
    customInfo.SetIsDeduplication(false);
    customInfo.SetAlbumId(0);
    BatchScannerObj obj(customInfo);

    BatchScannerObj::BatchScanItem item;
    item.isDuplicate = false;
    item.metadata = std::make_unique<Metadata>();
    item.metadata->SetFileSize(static_cast<int64_t>(4096));
    item.metadata->SetOrientation(90);
    item.fileInfo.fileName = "test.jpg";
    item.fileInfo.size = 0; // not set
    item.fileInfo.orientation = 0;
    obj.items_.push_back(std::move(item));

    int32_t result = obj.Deduplicate();
    EXPECT_EQ(result, E_OK);
    // Size and orientation should be back-filled from metadata
    EXPECT_EQ(obj.items_[0].fileInfo.size, 4096);
    EXPECT_EQ(obj.items_[0].fileInfo.orientation, 90);
    MEDIA_INFO_LOG("end Deduplicate_BackFillsSizeOrientation_test02");
}

/**
 * @tc.name: Insert_AllDuplicate_test01
 * @tc.desc: 全部重复 → 返回 E_OK，无插入
 */
HWTEST_F(BatchScannerObjTest, Insert_AllDuplicate_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Insert_AllDuplicate_test01");
    CustomRestoreInfo customInfo;
    BatchScannerObj obj(customInfo);

    for (int i = 0; i < 3; i++) {
        BatchScannerObj::BatchScanItem item;
        item.isDuplicate = true;
        obj.items_.push_back(std::move(item));
    }

    int32_t result = obj.Insert();
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end Insert_AllDuplicate_test01");
}

} // namespace Media
} // namespace OHOS
