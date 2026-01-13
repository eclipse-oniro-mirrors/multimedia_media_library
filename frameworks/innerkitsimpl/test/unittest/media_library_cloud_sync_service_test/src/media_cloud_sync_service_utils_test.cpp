/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCloudSync"

#include "media_cloud_sync_service_utils_test.h"

#include "media_log.h"

#include "cloud_media_dao_utils.h"
#include "cloud_media_sync_utils.h"
#include "cloud_media_dao_utils.h"
#include "cloud_sync_convert.h"
#include "exif_rotate_utils.h"
#define protected public
#define private public
#include "cloud_media_uri_utils.h"
#undef protected
#undef private
#include "media_itypes_utils.h"
#include "safe_vector.h"
#include "medialibrary_unittest_utils.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
using namespace testing::ext;
using namespace OHOS::NativeRdb;

void CloudMediaSyncServiceUtilsTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaSyncServiceUtilsTest::SetUpTestCase");
}

void CloudMediaSyncServiceUtilsTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("start CloudMediaSyncServiceUtilsTest::TearDownTestCase");
}

// SetUp:Execute before each test case
void CloudMediaSyncServiceUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("setup");
}

void CloudMediaSyncServiceUtilsTest::TearDown(void) {}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_Test, TestSize.Level1)
{
    vector<string> values;
    values.emplace_back("hello");
    values.emplace_back("world");
    string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'hello','world'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_Test, TestSize.Level1)
{
    vector<string> fileIds;
    fileIds.emplace_back("1");
    fileIds.emplace_back("2");
    string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "1,2");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_Test, TestSize.Level1)
{
    string sql = "SELECT name, score FROM Stu WHERE age >= ? AND age <= ?;";
    vector<string> bindArgs = {"18", "45"};
    string result = CloudMediaDaoUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, sql);

    sql = "SELECT name, score FROM Stu WHERE age >= {0} AND age <= {1};";
    result = CloudMediaDaoUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT name, score FROM Stu WHERE age >= 18 AND age <= 45;");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_Test, TestSize.Level1)
{
    vector<string> albumIds;
    albumIds.emplace_back("hello");
    albumIds.emplace_back("1");
    albumIds.emplace_back("world");
    auto result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 1);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_Test, TestSize.Level1)
{
    std::vector<uint64_t> albumIds = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    auto result = CloudMediaDaoUtils::VectorToString(albumIds);
    EXPECT_GT(result.size(), 9);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLocalPath_Test, TestSize.Level1)
{
    string res = CloudMediaSyncUtils::GetLocalPath("");
    EXPECT_EQ(res, "");

    res = CloudMediaSyncUtils::GetLocalPath("/storage/cloud");
    EXPECT_EQ(res, "/storage/cloud");

    res = CloudMediaSyncUtils::GetLocalPath("/storage/cloud/files/test/cxx/html");
    EXPECT_EQ(res, "/storage/media/local/files/test/cxx/html");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, UpdateModifyTime_Test, TestSize.Level1)
{
    string path = "/test/GYH";
    int64_t localMtime = 0;
    int32_t ret = CloudMediaSyncUtils::UpdateModifyTime(path, localMtime);
    printf("UpdateModifyTime ret = %d\n", ret);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, IsLocalDirty_Test, TestSize.Level1)
{
    EXPECT_EQ(CloudMediaSyncUtils::IsLocalDirty(0, true), false);
    EXPECT_EQ(CloudMediaSyncUtils::IsLocalDirty(0, false), false);
    EXPECT_EQ(CloudMediaSyncUtils::IsLocalDirty(2, false), true);
    EXPECT_EQ(CloudMediaSyncUtils::IsLocalDirty(2, true), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FileIsLocal_Test, TestSize.Level1)
{
    EXPECT_EQ(CloudMediaSyncUtils::FileIsLocal(0), false);
    EXPECT_EQ(CloudMediaSyncUtils::FileIsLocal(1), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetCloudPath_Test, TestSize.Level1)
{
    EXPECT_EQ(CloudMediaSyncUtils::GetCloudPath("", ""), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetCloudPath("hello", "world"), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetCloudPath("/user/local/test", "/user"), "/user/local/test");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetThumbParentPath_Test, TestSize.Level1)
{
    string result = CloudMediaSyncUtils::GetThumbParentPath("", "");
    EXPECT_EQ(result, "/storage/cloud/files/.thumbs");

    result = CloudMediaSyncUtils::GetThumbParentPath("/user/local/test.jpg", "/user/local");
    EXPECT_EQ(result, "/storage/cloud/files/.thumbs/test.jpg");

    result = CloudMediaSyncUtils::GetThumbParentPath("/user/local/test", "prefix");
    EXPECT_EQ(result, "");

    CloudMediaSyncUtils::RemoveThmParentPath("", "");
    CloudMediaSyncUtils::RemoveThmParentPath("/user/local/test.jpg", "/user/local");
    CloudMediaSyncUtils::RemoveEditDataParentPath("");
    CloudMediaSyncUtils::RemoveEditDataParentPath("/user/local/test.jpg");
    CloudMediaSyncUtils::RemoveMetaDataPath("");
    CloudMediaSyncUtils::RemoveEditDataParentPath("/user/local/test.jpg");
    CloudMediaSyncUtils::InvalidVideoCache("");
    CloudMediaSyncUtils::InvalidVideoCache("/storage/cloud/test/car");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, RemoveEditDataSourcePath_Test, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/16/"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);

    string path = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
    CloudMediaSyncUtils::RemoveEditDataSourcePath(path);

    EXPECT_EQ(MediaFileUtils::DeleteFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, BackUpEditDataSourcePath_Test01, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/16/"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile(
        "/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg"), true);

    string path = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
    CloudMediaSyncUtils::BackUpEditDataSourcePath(path);

    EXPECT_EQ(MediaFileUtils::DeleteFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(
        "/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg"), false);
    EXPECT_EQ(MediaFileUtils::DeleteFile(
        "/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/photo_temp.jpg"), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, BackUpEditDataSourcePath_Test02, TestSize.Level1)
{
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/Photo/16/"), true);
    EXPECT_EQ(MediaFileUtils::CreateFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::CreateDirectory("/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg"), true);

    string path = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
    CloudMediaSyncUtils::BackUpEditDataSourcePath(path);

    EXPECT_EQ(MediaFileUtils::DeleteFile("/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg"), true);
    EXPECT_EQ(MediaFileUtils::DeleteFile(
        "/storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/photo_temp.jpg"), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CommonPath_Test, TestSize.Level1)
{
    EXPECT_EQ(CloudMediaSyncUtils::GetEditDataDir(""), "");
    string result = CloudMediaSyncUtils::GetEditDataDir("/storage/cloud/files/user/test");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/user/test");

    EXPECT_EQ(CloudMediaSyncUtils::GetEditDataPath(""), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetEditDataPath("/storage/cloud/files/user/test"),
              "/storage/cloud/files/.editData/user/test/editdata");

    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath(""), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath("test"), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath(".test"), ".mp4");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath("what/can/i/say.avi"), "what/can/i/say.mp4");

    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoTmpPath(""), ""),
        result = CloudMediaSyncUtils::GetMovingPhotoTmpPath("/storage/cloud/files/user/test");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/user/test");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GenerateCloudIdWithHash_Test, TestSize.Level1)
{
    CloudSync::PhotoAlbumPo record;
    record.cloudId = "test";
    uint32_t ret = CloudMediaSyncUtils::GenerateCloudIdWithHash(record);
    EXPECT_EQ(ret, E_CLOUDID_IS_NOT_NULL);

    record.cloudId = "";
    ret = CloudMediaSyncUtils::GenerateCloudIdWithHash(record);
    EXPECT_EQ(ret, E_OK);

    record.dateAdded = 1;
    CloudMediaSyncUtils::GenerateCloudIdWithHash(record);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetMovingPhotoExtraDataDir_Test, TestSize.Level1)
{
    auto result = CloudMediaSyncUtils::GetMovingPhotoExtraDataDir("");
    EXPECT_EQ(result, "");
    result = CloudMediaSyncUtils::GetMovingPhotoExtraDataDir("/user/local/bin/fc/real/marid/hal/marid");
    EXPECT_EQ(result, "");
    result = CloudMediaSyncUtils::GetMovingPhotoExtraDataDir("/storage/cloud/files/test.avi");
    EXPECT_EQ(result, "/storage/cloud/files/.editData/test.avi");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetMovingPhotoExtraDataPath_Test, TestSize.Level1)
{
    auto result = CloudMediaSyncUtils::GetMovingPhotoExtraDataPath("");
    EXPECT_EQ(result, "");
    CloudMediaSyncUtils::GetMovingPhotoExtraDataPath("/storage/cloud/files/test.avi");
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpathFromSourcePath_Test, TestSize.Level1)
{
    string path;
    auto result = CloudMediaSyncUtils::GetLpathFromSourcePath(path);
    EXPECT_EQ(result, "");
    path = "/storage/emulated/0/testcar.avi";
    result = CloudMediaSyncUtils::GetLpathFromSourcePath(path);
    EXPECT_EQ(result, "");
    path = "/storage/emulated/0/test/car/mercedes/porsher.jpg";
    result = CloudMediaSyncUtils::GetLpathFromSourcePath(path);
    EXPECT_EQ(result, "/test/car/mercedes");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpath_Test, TestSize.Level1)
{
    CloudMediaPullDataDto pullData;
    pullData.propertiesSourcePath = "";
    auto result = CloudMediaSyncUtils::GetLpath(pullData);
    EXPECT_EQ(result, "");

    pullData.propertiesSourcePath = "/storage/emulated/0/Pictures/Screenshots/test/local/car/";
    pullData.basicFileType = -1;
    result = CloudMediaSyncUtils::GetLpath(pullData);
    EXPECT_EQ(result, "/Pictures/Screenshots/test/local/car");

    pullData.basicFileType = CloudSync::FILE_TYPE_VIDEO;
    pullData.basicFileName = "porsher.jpg";
    result = CloudMediaSyncUtils::GetLpath(pullData);
    EXPECT_EQ(result, "/Pictures/Screenrecords");

    pullData.propertiesSourcePath = "/storage/emulated/0/test/car/mercedes/porsher.jpg";
    result = CloudMediaSyncUtils::GetLpath(pullData);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, IsUserAlbumPath_Test, TestSize.Level1)
{
    string path;
    auto ret = CloudMediaSyncUtils::IsUserAlbumPath(path);
    EXPECT_EQ(ret, false);
    path = "/user/local/test/pictures/";
    ret = CloudMediaSyncUtils::IsUserAlbumPath(path);
    EXPECT_EQ(ret, false);
    path = "/pictures/users/test/car";
    ret = CloudMediaSyncUtils::IsUserAlbumPath(path);
    EXPECT_EQ(ret, true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillPhotosDto_Test, TestSize.Level1)
{
    int32_t thumbState = 0;
    int32_t orientation = 0;
    int32_t exifRotate = 0;
    std::string path = "test";
    CloudSync::PhotosDto photosDto;
    CloudSync::CloudMediaPullDataDto data;

    auto ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, path, orientation, exifRotate, thumbState);
    EXPECT_EQ(ret, E_OK);
    ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, data);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CloudSyncConvert_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.hasAttributes = true;
    data.attributesTitle = "test";             /* title */
    data.attributesMediaType = 0;              /* media_type */
    data.attributesHidden = 0;                 /* hidden */
    data.attributesHiddenTime = 0;             /* hidden_time */
    data.attributesRelativePath = "test";      /* relative_path */
    data.attributesVirtualPath = "test";       /* virtual_path */
    data.attributesPath = "test";              /* data */
    data.attributesMetaDateModified = 0;       /* meta_date_modified */
    data.attributesSubtype = 0;                /* subtype */
    data.attributesBurstCoverLevel = 0;        /* burst_cover_level */
    data.attributesBurstKey = "test";          /* burst_key */
    data.attributesDateYear = "test";          /* date_year */
    data.attributesDateMonth = "test";         /* date_month */
    data.attributesDateDay = "test";           /* date_day */
    data.attributesShootingMode = "test";      /* shooting_mode */
    data.attributesShootingModeTag = "test";   /* shooting_mode_tag */
    data.attributesDynamicRangeType = 0;       /* dynamic_range_type */
    data.attributesFrontCamera = "test";       /* front_camera */
    data.attributesEditTime = 0;               /* edit_time */
    data.attributesOriginalSubtype = 0;        /* original_subtype */
    data.attributesCoverPosition = 0;          /* cover_position */
    data.attributesMovingPhotoEffectMode = 0;  /* moving_photo_effect_mode */
    data.attributesSupportedWatermarkType = 0; /* supported_watermark_type */
    data.attributesStrongAssociation = 0;      /* strong_association */
    auto ret = CloudSyncConvert::ExtractAttributeValue(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropOrientation_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    vector<int32_t> rotates = {0, 1, 6, 3, 8};
    for (auto &rotate : rotates) {
        data.propertiesRotate = rotate;
        EXPECT_EQ(CloudSyncConvert::CompensatePropOrientation(data, values), E_OK);
    }
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ExtractCompatibleValue_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.cloudId = "test";
    data.basicIsDelete = true;
    data.basicSize = 1;
    data.basicDisplayName = "test";
    data.basicMimeType = "test";
    data.basicDeviceName = "test";
    data.modifiedTime = 1;
    data.basicEditedTime = 1;
    data.basicCreatedTime = 1;
    data.basicIsFavorite = 1;
    data.basicIsRecycle = 1;
    data.basicRecycledTime = 1;
    data.basicDescription = 1;
    data.basicFileType = 1;
    data.basicFileName = "test";
    data.basicCloudVersion = 1;
    data.duration = 0; /* duration */
    auto ret = CloudSyncConvert::ExtractCompatibleValue(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropHeightAndHeight_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesHeight = 1;
    data.propertiesWidth = 1;

    EXPECT_EQ(CloudSyncConvert::CompensatePropHeight(data, values), E_OK);
    EXPECT_EQ(CloudSyncConvert::CompensatePropWidth(data, values), E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDataAdded_Test_001, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesFirstUpdateTime = "";
    data.basicCreatedTime = 0;
    CloudSyncConvert::CompensateTimeInfo(data, values);
    int64_t dateAdded1 = 0;
    ValueObject valueObject1;
    values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject1);
    valueObject1.GetLong(dateAdded1);
    EXPECT_GT(dateAdded1, 0);

    data.propertiesFirstUpdateTime = "20111111";
    CloudSyncConvert::CompensateTimeInfo(data, values);
    int64_t dateAdded2 = 0;
    ValueObject valueObject2;
    values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject2);
    valueObject2.GetLong(dateAdded2);
    EXPECT_GT(dateAdded2, 0);

    data.propertiesFirstUpdateTime = "2011111d1abc";
    CloudSyncConvert::CompensateTimeInfo(data, values);
    int64_t dateAdded3 = 0;
    ValueObject valueObject3;
    values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject3);
    valueObject3.GetLong(dateAdded3);
    EXPECT_GT(dateAdded3, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDataAdded_Test_002, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesFirstUpdateTime = "1752233169000";  // 2025:07:11
    data.basicCreatedTime = 1751544669000;             // 2025:07:03
    CloudSyncConvert::CompensateTimeInfo(data, values);

    int64_t dateAdded = 0;
    ValueObject valueObject;
    values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject);
    valueObject.GetLong(dateAdded);
    EXPECT_EQ(dateAdded, 1752233169000);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDataAdded_Test_003, TestSize.Level1)
{
    vector<string> illegalFirstUpdateTimes{"illegal_time", "", "0", "-1"};
    for (const auto &illegalFirstUpdateTime : illegalFirstUpdateTimes) {
        ValuesBucket values;
        CloudMediaPullDataDto data;
        data.propertiesFirstUpdateTime = illegalFirstUpdateTime;
        data.basicCreatedTime = 1751544669000;  // 2025-07-03 20:11:09
        CloudSyncConvert::CompensateTimeInfo(data, values);

        int64_t dateAdded = 0;
        ValueObject valueObject;
        values.GetObject(PhotoColumn::MEDIA_DATE_ADDED, valueObject);
        valueObject.GetLong(dateAdded);
        EXPECT_EQ(dateAdded, 1751544669000);
    }
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDetailTime_Test_001, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesDetailTime = "2025:07:03 20:11:09";
    data.propertiesFirstUpdateTime = "1752233169000";  // 2025:07:11
    data.basicCreatedTime = 1751544669000;             // 2025:07:03
    CloudSyncConvert::CompensateTimeInfo(data, values);

    ValueObject valueObject;
    std::string detailTime;
    values.GetObject(PhotoColumn::PHOTO_DETAIL_TIME, valueObject);
    valueObject.GetString(detailTime);
    EXPECT_EQ(detailTime, "2025:07:03 20:11:09");

    std::string dateDay;
    values.GetObject(PhotoColumn::PHOTO_DATE_DAY, valueObject);
    valueObject.GetString(dateDay);
    EXPECT_EQ(dateDay, "20250703");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDetailTime_Test_002, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesDetailTime = "1970:01:01 08:00:00";
    data.propertiesFirstUpdateTime = "1752233169000";  // 2025:07:11
    data.basicCreatedTime = 1751544669000;             // 2025:07:03
    CloudSyncConvert::CompensateTimeInfo(data, values);

    ValueObject valueObject;
    std::string detailTime;
    values.GetObject(PhotoColumn::PHOTO_DETAIL_TIME, valueObject);
    valueObject.GetString(detailTime);
    EXPECT_NE(detailTime, "1970:01:01 08:00:00");

    std::string dateDay;
    values.GetObject(PhotoColumn::PHOTO_DATE_DAY, valueObject);
    valueObject.GetString(dateDay);
    EXPECT_EQ(dateDay, "20250703");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropDetailTime_Test_003, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesDetailTime = "1970:01:01 08:00:00";
    data.propertiesFirstUpdateTime = "1752233169000";  // 2025:07:11
    data.basicCreatedTime = 0;
    CloudSyncConvert::CompensateTimeInfo(data, values);

    ValueObject valueObject;
    std::string detailTime;
    values.GetObject(PhotoColumn::PHOTO_DETAIL_TIME, valueObject);
    valueObject.GetString(detailTime);
    EXPECT_NE(detailTime, "1970:01:01 08:00:00");

    std::string dateDay;
    values.GetObject(PhotoColumn::PHOTO_DATE_DAY, valueObject);
    valueObject.GetString(dateDay);
    EXPECT_EQ(dateDay, "20250711");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropSourcePath_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.propertiesSourcePath = "test";
    auto ret = CloudSyncConvert::CompensatePropSourcePath(data, values);
    EXPECT_EQ(ret, E_OK);

    data.propertiesSourcePath = "/storage/emulated/0/Pictures/Screenshots/test/";
    data.basicFileName = "car.avi";
    ret = CloudSyncConvert::CompensatePropSourcePath(data, values);
    EXPECT_EQ(ret, E_OK);

    data.propertiesSourcePath = "/storage/emulated/0/Pictures/Screenshots/test/";
    data.basicFileName = "car.avi";
    data.basicFileType = 4;
    ret = CloudSyncConvert::CompensatePropSourcePath(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensateBasicDateModified_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    CloudSyncConvert::CompensateTimeInfo(data, values);
    int64_t dateModified1 = 0;
    ValueObject valueObject1;
    values.GetObject(PhotoColumn::MEDIA_DATE_MODIFIED, valueObject1);
    valueObject1.GetLong(dateModified1);
    EXPECT_GT(dateModified1, 0);

    data.attributesEditedTimeMs = 1;
    CloudSyncConvert::CompensateTimeInfo(data, values);
    int64_t dateModified2 = 0;
    ValueObject valueObject2;
    values.GetObject(PhotoColumn::MEDIA_DATE_MODIFIED, valueObject2);
    valueObject2.GetLong(dateModified2);
    EXPECT_GT(dateModified2, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensateBasicDateTrashed_Test, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.basicIsRecycle = 1;
    data.basicRecycledTime = 121;
    auto ret = CloudSyncConvert::CompensateBasicDateTrashed(data, values);
    EXPECT_EQ(ret, E_OK);

    data.basicIsRecycle = 0;
    ret = CloudSyncConvert::CompensateBasicDateTrashed(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensatePropTitle_Test, TestSize.Level1)
{
    CloudMediaPullDataDto data;
    ValuesBucket values;
    data.propertiesSourceFileName = "Mercedes";
    auto ret = CloudSyncConvert::CompensatePropTitle(data, values);
    EXPECT_EQ(ret, E_OK);

    data.propertiesSourceFileName = "Mercedes.txt";
    values.PutString(PhotoColumn::MEDIA_TITLE, "test");
    ret = CloudSyncConvert::CompensatePropTitle(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensateBasicSubtype_Test, TestSize.Level1)
{
    CloudMediaPullDataDto data;
    ValuesBucket values;
    data.basicFileType = 9;
    auto ret = CloudSyncConvert::CompensateBasicSubtype(data, values);
    EXPECT_EQ(ret, E_OK);

    data.basicFileType = 1;
    data.propertiesSourcePath = "";
    ret = CloudSyncConvert::CompensateBasicSubtype(data, values);
    EXPECT_EQ(ret, E_OK);

    std::vector<string> sourcePath;
    sourcePath.emplace_back("DCIM__XXCamera");
    sourcePath.emplace_back("DCIM__XXCamera");
    sourcePath.emplace_back("Screenshots/Test");
    sourcePath.emplace_back("Test");
    for (auto &path : sourcePath) {
        data.propertiesSourcePath = path;
        ret = CloudSyncConvert::CompensateBasicSubtype(data, values);
        EXPECT_EQ(ret, E_OK);
    }
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CloudMediaSyncServiceUtilsSafeVector_Test, TestSize.Level1)
{
    vector<int> vec1;
    SafeVector<int> safeVec;
    for (int i = 0; i < 10; i++) {
        safeVec.PushBack(i);
        vec1.push_back(i);
    }

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(vec1[i], safeVec[i]);
    }
    EXPECT_EQ(vec1.size(), safeVec.Size());
    EXPECT_EQ(vec1.empty(), safeVec.Empty());
    safeVec.Clear();
    EXPECT_EQ(safeVec.Empty(), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetFileIdFromUri_Test, TestSize.Level1)
{
    int32_t fileId = 0;
    int32_t ret = CloudMediaUriUtils::GetFileIdFromUri("/test/file://media/Photo/", fileId);
    EXPECT_EQ(ret, E_INVAL_ARG);
    ret = CloudMediaUriUtils::GetFileIdFromUri("file://media/Photo/testcar", fileId);
    EXPECT_EQ(ret, E_INVAL_ARG);
    ret = CloudMediaUriUtils::GetFileIdFromUri("file://media/Photo/test/car", fileId);
    EXPECT_EQ(ret, E_INVAL_ARG);
    ret = CloudMediaUriUtils::GetFileIdFromUri("file://media/Photo/01/car", fileId);
    EXPECT_EQ(ret, E_OK);
}
HWTEST_F(CloudMediaSyncServiceUtilsTest, GetFileIds_Test, TestSize.Level1)
{
    std::vector<string> uris;
    std::vector<int32_t> fileIds;
    uris.emplace_back("/test/file://media/Photo/");
    uris.emplace_back("file://media/Photo/testcar");
    uris.emplace_back("file://media/Photo/test/car");
    uris.emplace_back("file://media/Photo/01/car");
    int32_t ret = CloudMediaUriUtils::GetFileIds(uris, fileIds);
    EXPECT_EQ(ret, E_INVAL_ARG);

    uris.clear();
    fileIds.clear();
    uris.emplace_back("file://media/Photo/01/car");
    ret = CloudMediaUriUtils::GetFileIds(uris, fileIds);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, CanUpdateExifRotateOnly_Test, TestSize.Level1)
{
    int32_t mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    int32_t oldExifRotate = 0;
    int32_t newExifRotate = static_cast<int32_t>(ExifRotateType::TOP_RIGHT);
    int32_t ret = CloudMediaSyncUtils::CanUpdateExifRotateOnly(mediaType, oldExifRotate, newExifRotate);
    EXPECT_EQ(ret, false);
}
}
