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
#include "media_string_utils.h"
#include "userfile_manager_types.h"

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
    string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, sql);

    sql = "SELECT name, score FROM Stu WHERE age >= {0} AND age <= {1};";
    result = MediaStringUtils::FillParams(sql, bindArgs);
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
    string result = "";
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath(""), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath("test"), "");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath(".test"), ".mp4");
    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoVideoPath("what/can/i/say.avi"), "what/can/i/say.mp4");

    EXPECT_EQ(CloudMediaSyncUtils::GetMovingPhotoTmpPath(""), "");
    result = CloudMediaSyncUtils::GetMovingPhotoTmpPath("/storage/cloud/files/user/test");
    EXPECT_EQ(result, "/mnt/hmdfs/account/device_view/local/files/.cloud_cache/download_cache/user/test");
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

HWTEST_F(CloudMediaSyncServiceUtilsTest, IsFileManagerAlbumPath_Test, TestSize.Level1)
{
    string path;
    auto ret = CloudMediaSyncUtils::IsFileManagerAlbumPath(path);
    EXPECT_EQ(ret, false);
    path = "/pictures/users/test/car";
    ret = CloudMediaSyncUtils::IsFileManagerAlbumPath(path);
    EXPECT_EQ(ret, false);
    path = "/FromDocs/Documents";
    ret = CloudMediaSyncUtils::IsFileManagerAlbumPath(path);
    EXPECT_EQ(ret, true);
    path = "/fromdocs/Documents";
    ret = CloudMediaSyncUtils::IsFileManagerAlbumPath(path);
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
    NativeRdb::ValuesBucket values;

    auto ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, path, orientation, exifRotate, thumbState);
    EXPECT_EQ(ret, E_OK);
    ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, data, values);
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

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensateData_Test_001, TestSize.Level1)
{
    ValuesBucket values;
    CloudMediaPullDataDto data;
    data.attributesUniqueId = "test_inner_unique_id";
    data.attributesPackageName = "test_inner_package_name";
    CloudSyncConvert::CompensateUniqueId(data, values);
    CloudSyncConvert::CompensatePackageName(data, values);
    std::string uniqueId1;
    ValueObject valueObject1;
    values.GetObject(PhotoColumn::UNIQUE_ID, valueObject1);
    valueObject1.GetString(uniqueId1);
    EXPECT_EQ(uniqueId1, "test_inner_unique_id");

    std::string packageName1;
    ValueObject valueObject2;
    values.GetObject(MediaColumn::MEDIA_PACKAGE_NAME, valueObject2);
    valueObject2.GetString(packageName1);
    EXPECT_EQ(packageName1, "test_inner_package_name");
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
    EXPECT_EQ(ret, E_INVALID_VALUES);
    ret = CloudMediaUriUtils::GetFileIdFromUri("file://media/Photo/testcar", fileId);
    EXPECT_EQ(ret, E_INVALID_VALUES);
    ret = CloudMediaUriUtils::GetFileIdFromUri("file://media/Photo/test/car", fileId);
    EXPECT_EQ(ret, E_INVALID_VALUES);
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
    EXPECT_EQ(ret, E_INVALID_VALUES);

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

HWTEST_F(CloudMediaSyncServiceUtilsTest, CompensateDateAddedYearMonthDay_Test, TestSize.Level1)
{
    CloudMediaPullDataDto data;
    ValuesBucket values;
    int64_t timeStamp = 1700000000;
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, timeStamp);
    auto ret = CloudSyncConvert::CompensateDateAddedYearMonthDay(data, values);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, HandleDateAddedYearMonthDay_Test, TestSize.Level1)
{
    CloudMediaPullDataDto data;
    ValuesBucket values;
    int64_t oldDateAdded = 1700000000;
    int64_t newDateAdded = 1700000001;
    CloudSyncConvert::HandleDateAddedYearMonthDay(oldDateAdded, newDateAdded, values);
    EXPECT_EQ(values.HasColumn(PhotoColumn::PHOTO_DATE_ADDED_YEAR), true);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_SingleValue, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖单值分支（触发条件：vector.size()==1）；验证返回值等于单引号包裹的字符串
    std::vector<std::string> values = {"test1"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'test1'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_MultipleValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖多值分支（触发条件：vector.size()>1）；验证返回值正确用逗号分隔并加引号
    std::vector<std::string> values = {"test1", "test2", "test3"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'test1','test2','test3'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_EmptyVector, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖空向量分支（触发条件：vector.size()==0）；验证返回值为空字符串
    std::vector<std::string> values = {};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_TwoValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖双值分支（触发条件：vector.size()==2）；验证返回值正确格式化
    std::vector<std::string> values = {"value1", "value2"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'value1','value2'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_SpecialCharacters, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖特殊字符分支（触发条件：值包含@、_、/等特殊字符）；验证特殊字符正确处理
    std::vector<std::string> values = {"test@123", "file_name", "path/to/file"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'test@123','file_name','path/to/file'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_LongString, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖长字符串分支（触发条件：字符串长度超过正常范围）；验证长字符串正确处理
    std::vector<std::string> values = {"very_long_string_value_that_exceeds_normal_length"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'very_long_string_value_that_exceeds_normal_length'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_TenValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖多值分支（触发条件：vector.size()==10）；验证10个值正确格式化
    std::vector<std::string> values = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'1','2','3','4','5','6','7','8','9','10'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_SingleValue, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖单值分支（触发条件：vector.size()==1）；验证返回值等于原始字符串
    std::vector<std::string> fileIds = {"file001"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "file001");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_MultipleValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖多值分支（触发条件：vector.size()>1）；验证返回值用逗号正确分隔
    std::vector<std::string> fileIds = {"file001", "file002", "file003"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "file001,file002,file003");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_EmptyVector, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖空向量分支（触发条件：vector.size()==0）；验证返回值为空字符串
    std::vector<std::string> fileIds = {};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_TwoValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖双值分支（触发条件：vector.size()==2）；验证两个值用逗号正确分隔
    std::vector<std::string> fileIds = {"id1", "id2"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "id1,id2");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_NumericStrings, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖数字字符串分支（触发条件：值为纯数字字符串）；验证数字字符串正确处理
    std::vector<std::string> fileIds = {"123", "456", "789"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "123,456,789");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_UUIDLikeStrings, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖UUID格式字符串分支（触发条件：值包含UUID格式）；验证UUID格式字符串正确处理
    std::vector<std::string> fileIds = {"550e8400-e29b-41d4-a716-446655440000", "6ba7b810-9dad-11d1-80b4-00c04fd430c8"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "550e8400-e29b-41d4-a716-446655440000,6ba7b810-9dad-11d1-80b4-00c04fd430c8");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_FiveValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖多值分支（触发条件：vector.size()==5）；验证5个值正确用逗号分隔
    std::vector<std::string> fileIds = {"a", "b", "c", "d", "e"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "a,b,c,d,e");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_SingleParam, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖单参数分支（触发条件：bindArgs.size()==1）；验证SQL中{0}被正确替换
    std::string sql = "SELECT * FROM table WHERE id = {0}";
    std::vector<std::string> bindArgs = {"123"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE id = 123");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_MultipleParams, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖多参数分支（触发条件：bindArgs.size()>1）；验证SQL中{0}和{1}被正确替换
    std::string sql = "SELECT * FROM table WHERE id = {0} AND name = {1}";
    std::vector<std::string> bindArgs = {"123", "test"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE id = 123 AND name = test");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_NoParams, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖无参数分支（触发条件：bindArgs.size()==0）；验证SQL原样返回
    std::string sql = "SELECT * FROM table";
    std::vector<std::string> bindArgs = {};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_EmptySql, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖空SQL分支（触发条件：sql为空字符串）；验证返回空字符串
    std::string sql = "";
    std::vector<std::string> bindArgs = {"value"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_ThreeParams, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖三参数分支（触发条件：bindArgs.size()==3）；验证SQL中{0}、{1}、{2}被正确替换
    std::string sql = "INSERT INTO table VALUES ({0}, {1}, {2})";
    std::vector<std::string> bindArgs = {"1", "2", "3"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "INSERT INTO table VALUES (1, 2, 3)");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_RepeatedParam, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖重复占位符分支（触发条件：SQL中{0}出现多次）；验证所有{0}都被替换为同一值
    std::string sql = "SELECT * FROM table WHERE id = {0} OR parent_id = {0}";
    std::vector<std::string> bindArgs = {"123"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE id = 123 OR parent_id = 123");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_StringParam, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖字符串参数分支（触发条件：参数为字符串类型）；验证字符串参数正确替换
    std::string sql = "SELECT * FROM table WHERE name = '{0}'";
    std::vector<std::string> bindArgs = {"testname"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE name = 'testname'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_FiveParams, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖五参数分支（触发条件：bindArgs.size()==5）；验证5个占位符都被正确替换
    std::string sql = "SELECT * FROM table WHERE a={0} AND b={1} AND c={2} AND d={3} AND e={4}";
    std::vector<std::string> bindArgs = {"1", "2", "3", "4", "5"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE a=1 AND b=2 AND c=3 AND d=4 AND e=5");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_LongStringValue, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖长参数值分支（触发条件：参数值长度很长）；验证长参数值正确替换
    std::string sql = "SELECT * FROM table WHERE description = {0}";
    std::vector<std::string> bindArgs = {"This is a long description that contains many words"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE description = This is a long description that contains many words");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillParams_ValueContainsFlag, TestSize.Level1)
{
    // 用例说明：测试FillParams功能；覆盖CHECK_AND_CONTINUE异常分支（触发条件：val包含flag）；验证函数能正确处理并继续替换下一个占位符
    std::string sql = "SELECT * FROM table WHERE a = {0} AND b = {1}";
    std::vector<std::string> bindArgs = {"{0}-value1", "value2"};
    std::string result = MediaStringUtils::FillParams(sql, bindArgs);
    EXPECT_EQ(result, "SELECT * FROM table WHERE a = {0} AND b = value2");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_AllNumbers, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖全数字分支（触发条件：所有值都是数字字符串）；验证返回所有数字字符串
    std::vector<std::string> albumIds = {"1", "2", "3", "4", "5"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[4], "5");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_MixedValues, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖混合值分支（触发条件：值包含数字和非数字字符串）；验证只返回数字字符串
    std::vector<std::string> albumIds = {"1", "abc", "2", "def", "3"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[1], "2");
    EXPECT_EQ(result[2], "3");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_NoNumbers, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖无非数字分支（触发条件：所有值都不是数字字符串）；验证返回空向量
    std::vector<std::string> albumIds = {"abc", "def", "ghi"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_EmptyVector, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖空向量分支（触发条件：输入向量size==0）；验证返回空向量
    std::vector<std::string> albumIds = {};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_LargeNumbers, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖大数字分支（触发条件：数字字符串表示大数值）；验证大数字字符串正确返回
    std::vector<std::string> albumIds = {"999999", "1000000", "123456789"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "999999");
    EXPECT_EQ(result[1], "1000000");
    EXPECT_EQ(result[2], "123456789");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_ZeroValue, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖零值分支（触发条件：值为"0"字符串）；验证零值被识别为数字
    std::vector<std::string> albumIds = {"0", "abc", "1"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "0");
    EXPECT_EQ(result[1], "1");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetNumbers_DecimalNumbers, TestSize.Level1)
{
    // 用例说明：测试GetNumbers功能；覆盖小数分支（触发条件：值为小数字符串）；验证小数字符串不被识别为数字
    std::vector<std::string> albumIds = {"1.5", "2.5", "3"};
    std::vector<std::string> result = CloudMediaDaoUtils::GetNumbers(albumIds);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "3");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_ValidNumber, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖有效数字分支（触发条件：字符串为有效数字）；验证返回正确的int32_t值
    std::string str = "123";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 123);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_Zero, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖零值分支（触发条件：字符串为"0"）；验证返回0
    std::string str = "0";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_NegativeNumber, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖负数分支（触发条件：字符串为负数）；验证返回正确的负数int32_t值
    std::string str = "-456";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, -456);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_InvalidString, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖无效字符串分支（触发条件：字符串包含非数字字符）；验证返回0
    std::string str = "abc";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_EmptyString, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖空字符串分支（触发条件：字符串为空）；验证返回0
    std::string str = "";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_PartialInvalid, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖部分无效分支（触发条件：字符串包含数字后跟非数字字符）；验证返回0
    std::string str = "123abc";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_MaxInt32, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖边界值分支（触发条件：字符串为INT32_MAX）；验证返回2147483647
    std::string str = "2147483647";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 2147483647);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_MinInt32, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖边界值分支（触发条件：字符串为INT32_MIN）；验证返回-2147483648
    std::string str = "-2147483648";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, -2147483648);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_OverflowPositive, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖溢出分支（触发条件：字符串值>INT32_MAX）；验证返回0
    std::string str = "2147483648";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_OverflowNegative, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖溢出分支（触发条件：字符串值<INT32_MIN）；验证返回0
    std::string str = "-2147483649";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToInt32_LargeNumber, TestSize.Level1)
{
    // 用例说明：测试ToInt32功能；覆盖大数溢出分支（触发条件：字符串值远超INT32范围）；验证返回0
    std::string str = "999999999999";
    int32_t result = CloudMediaDaoUtils::ToInt32(str);
    EXPECT_EQ(result, 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_SingleValue, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖单值分支（触发条件：intVals.size()==1）；验证返回包含一个字符串的向量
    std::vector<int32_t> intVals = {100};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "100");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_MultipleValues, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖多值分支（触发条件：intVals.size()>1）；验证所有int32_t转换为字符串
    std::vector<int32_t> intVals = {1, 2, 3, 4, 5};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[4], "5");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_EmptyVector, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖空向量分支（触发条件：intVals.size()==0）；验证返回空向量
    std::vector<int32_t> intVals = {};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 0);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_NegativeValues, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖负值分支（触发条件：包含负数int32_t）；验证负数正确转换为字符串
    std::vector<int32_t> intVals = {-1, -2, -3};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "-1");
    EXPECT_EQ(result[1], "-2");
    EXPECT_EQ(result[2], "-3");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_ZeroValues, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖零值分支（触发条件：包含0值）；验证0正确转换为"0"字符串
    std::vector<int32_t> intVals = {0, 0, 0};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "0");
    EXPECT_EQ(result[1], "0");
    EXPECT_EQ(result[2], "0");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_LargeValues, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖大值分支（触发条件：包含大数值int32_t）；验证大数值正确转换为字符串
    std::vector<int32_t> intVals = {1000000, 2000000, 3000000};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "1000000");
    EXPECT_EQ(result[1], "2000000");
    EXPECT_EQ(result[2], "3000000");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_TenValues, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖多值分支（触发条件：intVals.size()==10）；验证10个值全部正确转换
    std::vector<int32_t> intVals = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 10);
    EXPECT_EQ(result[0], "1");
    EXPECT_EQ(result[9], "10");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetStringVector_MaxMinInt32, TestSize.Level1)
{
    // 用例说明：测试GetStringVector功能；覆盖边界值分支（触发条件：包含INT32_MAX和INT32_MIN）；验证边界值正确转换
    std::vector<int32_t> intVals = {2147483647, -2147483648};
    std::vector<std::string> result = CloudMediaDaoUtils::GetStringVector(intVals);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "2147483647");
    EXPECT_EQ(result[1], "-2147483648");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_SingleValue, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖单值分支（触发条件：vec.size()==1）；验证返回格式为"[value]"
    std::vector<uint64_t> vec = {100};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[100]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_MultipleValues, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖多值分支（触发条件：vec.size()>1）；验证返回值用指定分隔符正确分隔
    std::vector<uint64_t> vec = {1, 2, 3, 4, 5};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[1, 2, 3, 4, 5]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_EmptyVector, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖空向量分支（触发条件：vec.size()==0）；验证返回"[]"
    std::vector<uint64_t> vec = {};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_CustomSeparator, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖自定义分隔符分支（触发条件：使用非默认分隔符）；验证使用自定义分隔符
    std::vector<uint64_t> vec = {1, 2, 3};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, "|");
    EXPECT_EQ(result, "[1|2|3]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_DefaultSeparator, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖默认分隔符分支（触发条件：不指定分隔符参数）；验证使用默认分隔符", "
    std::vector<uint64_t> vec = {10, 20, 30};
    std::string result = CloudMediaDaoUtils::VectorToString(vec);
    EXPECT_EQ(result, "[10, 20, 30]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_LargeValues, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖大值分支（触发条件：包含大uint64_t值）；验证大值正确格式化
    std::vector<uint64_t> vec = {1000000000, 2000000000, 3000000000};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[1000000000, 2000000000, 3000000000]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_TenValues, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖多值分支（触发条件：vec.size()==10）；验证10个值正确格式化
    std::vector<uint64_t> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_ZeroValues, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖零值分支（触发条件：包含0值）；验证零值正确格式化
    std::vector<uint64_t> vec = {0, 0, 0};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ", ");
    EXPECT_EQ(result, "[0, 0, 0]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, VectorToString_SingleSeparator, TestSize.Level1)
{
    // 用例说明：测试VectorToString功能；覆盖单字符分隔符分支（触发条件：分隔符为单个字符","）；验证无空格分隔
    std::vector<uint64_t> vec = {1, 2, 3};
    std::string result = CloudMediaDaoUtils::VectorToString(vec, ",");
    EXPECT_EQ(result, "[1,2,3]");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_ThreeValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖三值分支（触发条件：vector.size()==3）；验证三个值正确格式化
    std::vector<std::string> values = {"a", "b", "c"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'a','b','c'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_FourValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖四值分支（触发条件：vector.size()==4）；验证四个值正确格式化
    std::vector<std::string> values = {"1", "2", "3", "4"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'1','2','3','4'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithCommaAndQuote_SixValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithCommaAndQuote功能；覆盖六值分支（触发条件：vector.size()==6）；验证六个值正确格式化
    std::vector<std::string> values = {"a", "b", "c", "d", "e", "f"};
    std::string result = CloudMediaDaoUtils::ToStringWithCommaAndQuote(values);
    EXPECT_EQ(result, "'a','b','c','d','e','f'");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_ThreeValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖三值分支（触发条件：vector.size()==3）；验证三个值用逗号正确分隔

    std::vector<std::string> fileIds = {"a", "b", "c"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "a,b,c");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_FourValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖四值分支（触发条件：vector.size()==4）；验证四个值用逗号正确分隔
    std::vector<std::string> fileIds = {"1", "2", "3", "4"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "1,2,3,4");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, ToStringWithComma_SixValues, TestSize.Level1)
{
    // 用例说明：测试ToStringWithComma功能；覆盖六值分支（触发条件：vector.size()==6）；验证六个值用逗号正确分隔
    std::vector<std::string> fileIds = {"a", "b", "c", "d", "e", "f"};
    std::string result = CloudMediaDaoUtils::ToStringWithComma(fileIds);
    EXPECT_EQ(result, "a,b,c,d,e,f");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillPhotosDto_Test_002, TestSize.Level1)
{
    int32_t thumbState = 0;
    int32_t orientation = 0;
    int32_t exifRotate = 0;
    std::string path = "test";
    CloudSync::PhotosDto photosDto;
    CloudSync::CloudMediaPullDataDto data;
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::UNIQUE_ID, "test_unique_id_inner");
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "test_package_name_inner");
    values.PutInt(PhotoColumn::PHOTO_RISK_STATUS, 1);

    auto ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, path, orientation, exifRotate, thumbState);
    EXPECT_EQ(ret, E_OK);
    ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, data, values);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photosDto.uniqueId, "test_unique_id_inner");
    EXPECT_EQ(photosDto.packageName, "test_package_name_inner");
    EXPECT_EQ(photosDto.photoRiskStatus, 1);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FillPhotosDto_Test_003, TestSize.Level1)
{
    int32_t thumbState = 0;
    int32_t orientation = 0;
    int32_t exifRotate = 0;
    std::string path = "test";
    CloudSync::PhotosDto photosDto;
    CloudSync::CloudMediaPullDataDto data;
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::UNIQUE_ID, "test_unique_id_inner");
    values.PutString(MediaColumn::MEDIA_PACKAGE_NAME, "test_package_name_inner");
    values.PutInt(PhotoColumn::PHOTO_RISK_STATUS, 3);

    auto ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, path, orientation, exifRotate, thumbState);
    EXPECT_EQ(ret, E_OK);
    ret = CloudMediaSyncUtils::FillPhotosDto(photosDto, data, values);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(photosDto.uniqueId, "test_unique_id_inner");
    EXPECT_EQ(photosDto.packageName, "test_package_name_inner");
    EXPECT_EQ(photosDto.photoRiskStatus, 1);
    EXPECT_EQ(photosDto.isCritical, 1);
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpathWithoutDocPrefix_NormalPath_Test, TestSize.Level1)
{
    string lPath = "/FromDocs/Documents";
    string result = CloudMediaSyncUtils::GetLpathWithoutDocPrefix(lPath);
    EXPECT_EQ(result, "Documents");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpathWithoutDocPrefix_RootPath_Test, TestSize.Level1)
{
    string lPath = "/FromDocs/";
    string result = CloudMediaSyncUtils::GetLpathWithoutDocPrefix(lPath);
    EXPECT_TRUE(result.empty());
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpathWithoutDocPrefix_NonFileManager_Test, TestSize.Level1)
{
    string lPath = "/Pictures/Camera";
    string result = CloudMediaSyncUtils::GetLpathWithoutDocPrefix(lPath);
    EXPECT_EQ(result, "/Pictures/Camera");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, GetLpathWithoutDocPrefix_LowerCase_Test, TestSize.Level1)
{
    string lPath = "/fromdocs/documents";
    string result = CloudMediaSyncUtils::GetLpathWithoutDocPrefix(lPath);
    EXPECT_EQ(result, "documents");
}

// FindStoragePath 函数测试

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_MediaAsset_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_MediaAsset_Hidden_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_MediaAsset_Trashed_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 123456789;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DocsAsset_Normal_ReturnsStoragePath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 1;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/Docs/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/Docs/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DocsAsset_Hidden_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 1;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DocsAsset_Trashed_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 1;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 123456789;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DocsAsset_HiddenAndTrashed_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 1;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 123456789;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_LakeAsset_Normal_ReturnsStoragePath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 3;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/HO_DATA_EXT_MISC/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_LakeAsset_Hidden_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 3;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_LakeAsset_Trashed_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 3;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 123456789;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_LakeAsset_HiddenAndTrashed_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 3;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/HO_DATA_EXT_MISC/test.jpg";
    photoInfo.hidden = 1;
    photoInfo.dateTrashed = 123456789;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_EmptyData_ReturnsEmpty, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "";
    photoInfo.storagePath = "";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_EmptyStoragePath_ReturnsEmpty, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 1;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_AllFieldsDefault_ReturnsDataPath, TestSize.Level1)
{
    PhotosPo photoInfo;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_UnknownSourceType_ReturnsStoragePath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 99;
    photoInfo.data = "/storage/cloud/files/Photo/16/test.jpg";
    photoInfo.storagePath = "/storage/media/unknown/test.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DataWithChineseChars, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/测试文件.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/测试文件.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_DataWithSpecialChars, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/test-file_name.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result, "/storage/media/local/files/Photo/16/test-file_name.jpg");
}

HWTEST_F(CloudMediaSyncServiceUtilsTest, FindStoragePath_LongPath, TestSize.Level1)
{
    PhotosPo photoInfo;
    photoInfo.fileSourceType = 0;
    photoInfo.data = "/storage/cloud/files/Photo/16/very_long_directory_name/very_long_file_name.jpg";
    photoInfo.hidden = 0;
    photoInfo.dateTrashed = 0;

    std::string result = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    EXPECT_EQ(result.find("/storage/media/local/files"), 0);
}
}  // namespace OHOS::Media::CloudSync
