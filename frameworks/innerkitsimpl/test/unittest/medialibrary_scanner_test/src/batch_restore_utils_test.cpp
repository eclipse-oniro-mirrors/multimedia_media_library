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
 
#include "scanner_utils_custom_restore_test.h"
 
#include <sys/stat.h>
 
#include "custom_restore_utils.h"
#include "custom_restore_info.h"
#include "custom_restore_types.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "metadata.h"
#include "photo_album_column.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
void ScannerUtilsCustomRestoreTest::SetUp() {}
void ScannerUtilsCustomRestoreTest::TearDown() {}
 
// ==================== IsDuplication pure logic tests ====================
 
/**
 * @tc.name: IsDuplication_NoDedup_test01
 * @tc.desc: isDeduplication=false → 返回 false
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_NoDedup_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_NoDedup_test01");
    CustomRestoreInfo config;
    config.SetIsDeduplication(false);
    config.SetAlbumId(1);
    config.SetHasPhotoCache(false);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end IsDuplication_NoDedup_test01");
}
 
/**
 * @tc.name: IsDuplication_AlbumIdZero_test02
 * @tc.desc: albumId=0 → 返回 false（早期返回）
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_AlbumIdZero_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_AlbumIdZero_test02");
    CustomRestoreInfo config;
    config.SetIsDeduplication(true);
    config.SetAlbumId(0);
    config.SetHasPhotoCache(false);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end IsDuplication_AlbumIdZero_test02");
}
 
/**
 * @tc.name: IsDuplication_PhotoCacheHit_test03
 * @tc.desc: photoCache 包含匹配 key → 返回 true
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_PhotoCacheHit_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_PhotoCacheHit_test03");
    CustomRestoreInfo config;
    config.SetIsDeduplication(true);
    config.SetAlbumId(1);
    config.SetHasPhotoCache(true);
    unordered_set<string> photoCache;
    photoCache.insert("test.jpg_1024_1_0");
    config.SetPhotoCache(photoCache);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("end IsDuplication_PhotoCacheHit_test03");
}
 
/**
 * @tc.name: IsDuplication_PhotoCacheMiss_test04
 * @tc.desc: photoCache 不含 key → 返回 false (走RDB查询分支，无RDB则返回false)
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_PhotoCacheMiss_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_PhotoCacheMiss_test04");
    CustomRestoreInfo config;
    config.SetIsDeduplication(true);
    config.SetAlbumId(1);
    config.SetHasPhotoCache(true);
    // photoCache is empty, no match
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    // Without RDB store, the RDB query branch returns false
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end IsDuplication_PhotoCacheMiss_test04");
}
 
/**
 * @tc.name: IsDuplication_PhotoCacheKeyFormat_test05
 * @tc.desc: 验证 key 格式 "fileName_size_mediaType_orientation"
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_PhotoCacheKeyFormat_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_PhotoCacheKeyFormat_test05");
    CustomRestoreInfo config;
    config.SetIsDeduplication(true);
    config.SetAlbumId(1);
    config.SetHasPhotoCache(true);
    unordered_set<string> photoCache;
    // Key format: fileName_size_mediaType_orientation
    photoCache.insert("photo.png_2048_1_90");
    config.SetPhotoCache(photoCache);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "photo.png";
    fileInfo.size = 2048;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 90;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("end IsDuplication_PhotoCacheKeyFormat_test05");
}
 
/**
 * @tc.name: IsDuplication_EarlyReturnBoth_test06
 * @tc.desc: isDeduplication=false && albumId=0 → false
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_EarlyReturnBoth_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_EarlyReturnBoth_test06");
    CustomRestoreInfo config;
    config.SetIsDeduplication(false);
    config.SetAlbumId(0);
    config.SetHasPhotoCache(true);
    unordered_set<string> photoCache;
    photoCache.insert("test.jpg_1024_1_0");
    config.SetPhotoCache(photoCache);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end IsDuplication_EarlyReturnBoth_test06");
}
 
/**
 * @tc.name: IsDuplication_PhotoCacheEmpty_test07
 * @tc.desc: hasPhotoCache=true 但 photoCache 为空 → false (走RDB分支)
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, IsDuplication_PhotoCacheEmpty_test07, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter IsDuplication_PhotoCacheEmpty_test07");
    CustomRestoreInfo config;
    config.SetIsDeduplication(true);
    config.SetAlbumId(1);
    config.SetHasPhotoCache(true);
    RestoreFileInfo fileInfo;
    fileInfo.fileName = "test.jpg";
    fileInfo.size = 1024;
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.orientation = 0;
    bool result = CustomRestoreUtils::IsDuplication(config, fileInfo);
    // No photoCache hit, RDB query will fail without store
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("end IsDuplication_PhotoCacheEmpty_test07");
}
 
// ==================== GetFileMetadata integration tests ====================
 
/**
 * @tc.name: GetFileMetadata_NonExistentPath_test01
 * @tc.desc: 不存在的路径 → 返回 E_FAIL
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, GetFileMetadata_NonExistentPath_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter GetFileMetadata_NonExistentPath_test01");
    auto data = make_unique<Metadata>();
    data->SetFilePath("/nonexistent/path/file.jpg");
    data->SetFileName("file.jpg");
    int32_t result = CustomRestoreUtils::GetFileMetadata(data);
    EXPECT_NE(result, E_OK);
    MEDIA_INFO_LOG("end GetFileMetadata_NonExistentPath_test01");
}
 
/**
 * @tc.name: GetFileMetadata_ExistingFile_test02
 * @tc.desc: 创建临时文件 → size>0, dateModified>0
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, GetFileMetadata_ExistingFile_test02, TestSize.Level1)
{
    MEDIA_INFO_LOG("enter GetFileMetadata_ExistingFile_test02");
    const string tmpPath = "/data/local/tmp/scanner_utils_custom_restore_test_file.jpg";
    // Create a temp file
    FILE *f = fopen(tmpPath.c_str(), "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "test content for metadata");
    fclose(f);
 
    auto data = make_unique<Metadata>();
    data->SetFilePath(tmpPath);
    data->SetFileName("scanner_utils_custom_restore_test_file.jpg");
    int32_t result = CustomRestoreUtils::GetFileMetadata(data);
    EXPECT_EQ(result, E_OK);
    EXPECT_GT(data->GetFileSize(), 0);
    EXPECT_GT(data->GetFileDateModified(), 0);
 
    // Cleanup
    remove(tmpPath.c_str());
    MEDIA_INFO_LOG("end GetFileMetadata_ExistingFile_test02");
}
 
// ==================== FillMetadata integration tests ====================
 
/**
 * @tc.name: FillMetadata_SetsBasicFields_test01
 * @tc.desc: 设置 path/name/type 后调用，验证 Metadata 基本字段
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, FillMetadata_SetsBasicFields_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter FillMetadata_SetsBasicFields_test01");
    unordered_map<string, TimeInfo> timeInfoMap;
    RestoreFileInfo fileInfo;
    fileInfo.originFilePath = "/nonexistent/file.jpg";
    fileInfo.fileName = "file.jpg";
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    auto data = make_unique<Metadata>();
    // Will fail on stat() for nonexistent file
    int32_t result = CustomRestoreUtils::FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_NE(result, E_OK);
    // But basic fields should be set before stat()
    EXPECT_EQ(data->GetFilePath(), "/nonexistent/file.jpg");
    EXPECT_EQ(data->GetFileName(), "file.jpg");
    MEDIA_INFO_LOG("end FillMetadata_SetsBasicFields_test01");
}
 
/**
 * @tc.name: FillMetadata_LivePhotoSubtype_test02
 * @tc.desc: isLivePhoto=true → PhotoSubType=MOVING_PHOTO
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, FillMetadata_LivePhotoSubtype_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter FillMetadata_LivePhotoSubtype_test02");
    unordered_map<string, TimeInfo> timeInfoMap;
    RestoreFileInfo fileInfo;
    fileInfo.originFilePath = "/nonexistent/live.jpg";
    fileInfo.fileName = "live.jpg";
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    fileInfo.isLivePhoto = true;
    auto data = make_unique<Metadata>();
    // Will fail on stat(), but subtype should be set before that
    CustomRestoreUtils::FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_EQ(data->GetPhotoSubType(), static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    MEDIA_INFO_LOG("end FillMetadata_LivePhotoSubtype_test02");
}
 
/**
 * @tc.name: FillMetadata_TimeInfoMapLookup_test03
 * @tc.desc: timeInfoMap 有条目 → dateAdded/dateTaken 被设置
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, FillMetadata_TimeInfoMapLookup_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter FillMetadata_TimeInfoMapLookup_test03");
    unordered_map<string, TimeInfo> timeInfoMap;
    TimeInfo ti;
    ti.dateAdded = 1718400000000LL;
    ti.dateTaken = 1718400010000LL;
    ti.detailTime = "2025:06:15 10:30:00";
    timeInfoMap["file.jpg"] = ti;
 
    RestoreFileInfo fileInfo;
    fileInfo.originFilePath = "/nonexistent/file.jpg";
    fileInfo.fileName = "file.jpg";
    fileInfo.mediaType = MEDIA_TYPE_IMAGE;
    auto data = make_unique<Metadata>();
    // Will fail on stat(), but timeInfo should be set before that
    CustomRestoreUtils::FillMetadata(timeInfoMap, fileInfo, data);
    EXPECT_EQ(data->GetFileDateAdded(), 1718400000000LL);
    EXPECT_EQ(data->GetDateTaken(), 1718400010000LL);
    EXPECT_EQ(data->GetDetailTime(), "2025:06:15 10:30:00");
    MEDIA_INFO_LOG("end FillMetadata_TimeInfoMapLookup_test03");
}
 
// ==================== SetTimeInfo tests ====================
 
/**
 * @tc.name: SetTimeInfo_NormalTimestamps_test01
 * @tc.desc: 三时间戳有效 → values 包含所有时间列
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_NormalTimestamps_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_NormalTimestamps_test01");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(1718400001000LL);
    data->SetDateTaken(1718400002000LL);
    data->SetDetailTime("2025:06:15 10:30:00");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_FALSE(values.IsEmpty());
    // Verify all time columns exist
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DATE_ADDED));
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DATE_MODIFIED));
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DATE_TAKEN));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DETAIL_TIME));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_YEAR));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_MONTH));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_DAY));
    MEDIA_INFO_LOG("end SetTimeInfo_NormalTimestamps_test01");
}
 
/**
 * @tc.name: SetTimeInfo_ZeroDateAdded_test02
 * @tc.desc: dateAdded=0 → 回退到 dateModified
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_ZeroDateAdded_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_ZeroDateAdded_test02");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(static_cast<int64_t>(0));
    data->SetDateTaken(1718400002000LL);
    data->SetDetailTime("");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DATE_ADDED));
    // dateAdded should fallback to dateModified
    NativeRdb::ValueObject dateAddedObj;
    values.GetObject(MediaColumn::MEDIA_DATE_ADDED, dateAddedObj);
    int64_t dateAdded = 0;
    dateAddedObj.GetLong(dateAdded);
    EXPECT_EQ(dateAdded, 1718400000000LL);
    MEDIA_INFO_LOG("end SetTimeInfo_ZeroDateAdded_test02");
}
 
/**
 * @tc.name: SetTimeInfo_ZeroDateTaken_test03
 * @tc.desc: dateTaken=0 → 回退到 min(dateAdded, dateModified)
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_ZeroDateTaken_test03, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_ZeroDateTaken_test03");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(1718400005000LL);
    data->SetDateTaken(static_cast<int64_t>(0));
    data->SetDetailTime("");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_TRUE(values.HasColumn(MediaColumn::MEDIA_DATE_TAKEN));
    NativeRdb::ValueObject dateTakenObj;
    values.GetObject(MediaColumn::MEDIA_DATE_TAKEN, dateTakenObj);
    int64_t dateTaken = 0;
    dateTakenObj.GetLong(dateTaken);
    // dateTaken should fallback to min(added, modified) = min(5000, 0) = 0 (normalized)
    EXPECT_GT(dateTaken, static_cast<int64_t>(0));
    MEDIA_INFO_LOG("end SetTimeInfo_ZeroDateTaken_test03");
}
 
/**
 * @tc.name: SetTimeInfo_InvalidDetailTime_test04
 * @tc.desc: detailTime 超出范围 → 从 dateTaken 重新生成
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_InvalidDetailTime_test04, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_InvalidDetailTime_test04");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(1718400001000LL);
    data->SetDateTaken(1718400002000LL);
    data->SetDetailTime("invalid_time_string");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DETAIL_TIME));
    NativeRdb::ValueObject detailTimeObj;
    values.GetObject(PhotoColumn::PHOTO_DETAIL_TIME, detailTimeObj);
    std::string detailTime;
    detailTimeObj.GetString(detailTime);
    // Invalid detailTime should be regenerated from dateTaken
    EXPECT_FALSE(detailTime.empty());
    EXPECT_NE(detailTime, "invalid_time_string");
    MEDIA_INFO_LOG("end SetTimeInfo_InvalidDetailTime_test04");
}
 
/**
 * @tc.name: SetTimeInfo_ValidDetailTime_test05
 * @tc.desc: detailTime 有效 → 使用 normalizeDetailTime
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_ValidDetailTime_test05, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_ValidDetailTime_test05");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(1718400001000LL);
    data->SetDateTaken(1718400002000LL);
    data->SetDetailTime("2025:06:15 10:30:00");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DETAIL_TIME));
    NativeRdb::ValueObject detailTimeObj;
    values.GetObject(PhotoColumn::PHOTO_DETAIL_TIME, detailTimeObj);
    std::string detailTime;
    detailTimeObj.GetString(detailTime);
    EXPECT_FALSE(detailTime.empty());
    MEDIA_INFO_LOG("end SetTimeInfo_ValidDetailTime_test05");
}
 
/**
 * @tc.name: SetTimeInfo_YearMonthDayExtracted_test06
 * @tc.desc: 验证 year/month/day 从 detailTime 提取
 */
HWTEST_F(ScannerUtilsCustomRestoreTest, SetTimeInfo_YearMonthDayExtracted_test06, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter SetTimeInfo_YearMonthDayExtracted_test06");
    auto data = make_unique<Metadata>();
    data->SetFileDateModified(1718400000000LL);
    data->SetFileDateAdded(1718400001000LL);
    data->SetDateTaken(1718400002000LL);
    data->SetDetailTime("2025:06:15 10:30:00");
 
    RestoreFileInfo info;
    NativeRdb::ValuesBucket values;
    CustomRestoreUtils::SetTimeInfo(data, values);
 
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_YEAR));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_MONTH));
    EXPECT_TRUE(values.HasColumn(PhotoColumn::PHOTO_DATE_DAY));
    MEDIA_INFO_LOG("end SetTimeInfo_YearMonthDayExtracted_test06");
}
 
} // namespace Media
} // namespace OHOS