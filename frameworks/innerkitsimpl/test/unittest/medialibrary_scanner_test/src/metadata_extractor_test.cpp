/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "metadata_extractor_test.h"

#define private public
#include "metadata_extractor.h"
#include "meta.h"
#include "meta_key.h"
#undef private

#include <cstdint>

#include "image_source.h"
#include "media_exif.h"
#include "media_scanner_db.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaLibraryMetadataExtractorTest::SetUpTestCase(void) {}

void MediaLibraryMetadataExtractorTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void MediaLibraryMetadataExtractorTest::SetUp() {}

void MediaLibraryMetadataExtractorTest::TearDown(void) {}

static const char* TEST_IMAGE_PATH = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
static const char* TEST_VIDEO_PATH = "/storage/cloud/100/files/Documents/CreateVideoThumbnailTest_001.mp4";

HWTEST_F(MediaLibraryMetadataExtractorTest, Extract_test_001, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    int32_t ret = MetadataExtractor::Extract(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, Extract_test_002, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    int32_t ret = MetadataExtractor::Extract(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, Extract_empty_path, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path;
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_IMAGE));
    data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    // empty path Extract will return E_IMAGE
    int32_t ret = MetadataExtractor::Extract(data);
    EXPECT_EQ(ret, E_IMAGE);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractAVMetadata_empty_path, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath("");
    // empty path ExtractAVMetadata will return E_AVMETADATA
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractAVMetadata_normal_path, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExtractAVMetadata_normal_path");
    unique_ptr<Metadata> data = make_unique<Metadata>();
    string path = TEST_VIDEO_PATH;
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_VIDEO));
    data->SetFilePath(path);
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ExtractAVMetadata_normal_path end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractAVMetadata_nonexistent_path, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath("ExtractAVMetadata");
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractAVMetadata_clone_test, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path;
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    // empty path ExtractAVMetadata will return E_AVMETADATA
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data, Scene::AV_META_SCENE_CLONE);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageMetadata_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ExtractImageMetadata_test_001");
    unique_ptr<Metadata> data = make_unique<Metadata>();
    string path = TEST_IMAGE_PATH;
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_IMAGE));
    data->SetFilePath(path);
    int32_t ret = MetadataExtractor::ExtractImageMetadata(data);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ExtractImageMetadata_test_001 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageMetadata_test_002, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_IMAGE));
    data->SetFilePath(path);
    int32_t ret = MetadataExtractor::ExtractImageMetadata(data);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageMetadata_test_003, TestSize.Level1)
{
    unique_ptr<Metadata> metadata = make_unique<Metadata>();
    string path = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    metadata->SetFilePath(path);
    metadata->SetForAdd(false);

    int64_t dateTaken = 1753081131111L;
    metadata->SetDateTaken(dateTaken);
    int64_t dateModified = 1753081131222L;
    metadata->SetFileDateModified(dateModified);
    int32_t ret = MetadataExtractor::ExtractImageMetadata(metadata);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(metadata->GetDateTaken(), dateTaken);

    dateTaken = -1753081131333L;
    metadata->SetDateTaken(dateTaken);
    dateModified = 1753081131444L;
    metadata->SetFileDateModified(dateModified);
    ret = MetadataExtractor::ExtractImageMetadata(metadata);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(metadata->GetDateTaken(), dateModified);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageMetadata_test_004, TestSize.Level1)
{
    unique_ptr<Metadata> metadata = make_unique<Metadata>();
    string path = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    metadata->SetFileMediaType(MEDIA_TYPE_IMAGE);
    metadata->SetFilePath(path);
    metadata->SetForAdd(true);

    int64_t dateTaken = 1753081131555L;
    metadata->SetDateTaken(dateTaken);
    int64_t dateModified = 1753081131666L;
    metadata->SetFileDateModified(dateModified);
    int32_t ret = MetadataExtractor::ExtractImageMetadata(metadata);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(metadata->GetDateTaken(), dateTaken);

    dateTaken = -1753081131777L;
    metadata->SetDateTaken(dateTaken);
    dateModified = 1753081131888L;
    metadata->SetFileDateModified(dateModified);
    ret = MetadataExtractor::ExtractImageMetadata(metadata);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(metadata->GetDateTaken(), dateModified);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, FillExtractedMetadata_test_001, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    data->SetFileDateModified(static_cast<int64_t>(11));
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    unordered_map<int32_t, std::string> resultMap;
    resultMap = {{AV_KEY_ALBUM, ""}, {AV_KEY_ARTIST, ""}, {AV_KEY_DURATION, ""}, {AV_KEY_DATE_TIME_FORMAT, ""},
        {AV_KEY_VIDEO_HEIGHT, ""}, {AV_KEY_VIDEO_WIDTH, ""}, {AV_KEY_MIME_TYPE, ""}, {AV_KEY_MIME_TYPE, ""},
        {AV_KEY_VIDEO_ORIENTATION, ""}, {AV_KEY_VIDEO_IS_HDR_VIVID, ""}, {AV_KEY_TITLE, ""}, {AV_KEY_GENRE, ""},
        {AV_KEY_DATE_TIME_ISO8601, ""}, {AV_KEY_GLTF_OFFSET, ""}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetAlbum(), "");
    EXPECT_EQ(data->GetLongitude(), 0);
    EXPECT_EQ(data->GetLatitude(), 0);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, FillExtractedMetadata_test_002, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    data->SetFileDateModified(static_cast<int64_t>(11));
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    float longtitude = 1.2;
    float latitude = 138.2;
    meta->SetData(Tag::MEDIA_LONGITUDE, longtitude);
    meta->SetData(Tag::MEDIA_LATITUDE, latitude);
    unordered_map<int32_t, std::string> resultMap;
    resultMap = {{AV_KEY_ALBUM, "a"}, {AV_KEY_ARTIST, "a"}, {AV_KEY_DURATION, "a"}, {AV_KEY_DATE_TIME_FORMAT, "a"},
        {AV_KEY_VIDEO_HEIGHT, "a"}, {AV_KEY_VIDEO_WIDTH, "a"}, {AV_KEY_MIME_TYPE, "a"}, {AV_KEY_MIME_TYPE, "a"},
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"},
        {AV_KEY_DATE_TIME_ISO8601, "2025-06-11T18:00:00.000000Z"}, {AV_KEY_GLTF_OFFSET, "110"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetDateTaken(), 1749664800000);
    EXPECT_EQ(data->GetAlbum(), "a");
    EXPECT_EQ(data->GetLongitude(), longtitude);
    EXPECT_EQ(data->GetLatitude(), latitude);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, FillExtractedMetadata_test_003, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    int64_t dateModify = 1749632132523;
    data->SetFileDateModified(dateModify);
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    unordered_map<int32_t, std::string> resultMap;
    resultMap = {{AV_KEY_ALBUM, "a"}, {AV_KEY_ARTIST, "a"}, {AV_KEY_DURATION, "a"}, {AV_KEY_DATE_TIME_FORMAT, "a"},
        {AV_KEY_VIDEO_HEIGHT, "a"}, {AV_KEY_VIDEO_WIDTH, "a"}, {AV_KEY_MIME_TYPE, "a"}, {AV_KEY_MIME_TYPE, "a"},
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"},
        {AV_KEY_DATE_TIME_ISO8601, "11112"}, {AV_KEY_GLTF_OFFSET, "-1"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetDateTaken(), dateModify);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, FillExtractedMetadata_test_004, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    int64_t dateModify = 1749632132523;
    data->SetFileDateModified(dateModify);
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    unordered_map<int32_t, std::string> resultMap;
    resultMap = {{AV_KEY_ALBUM, "a"}, {AV_KEY_ARTIST, "a"}, {AV_KEY_DURATION, "a"}, {AV_KEY_DATE_TIME_FORMAT, "a"},
        {AV_KEY_VIDEO_HEIGHT, "a"}, {AV_KEY_VIDEO_WIDTH, "a"}, {AV_KEY_MIME_TYPE, "a"}, {AV_KEY_MIME_TYPE, "a"},
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"},
        {AV_KEY_DATE_TIME_ISO8601, "2025/06/11 18:00:00Z"}, {AV_KEY_GLTF_OFFSET, "10001"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetDateTaken(), dateModify);
}

static unique_ptr<Metadata> CreateImageMetadata(const string &path)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_IMAGE));
    data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::DEFAULT));
    data->SetFilePath(path);
    int64_t dateModified = 1763369312532;
    data->SetFileDateModified(dateModified);
    return data;
}

static std::unique_ptr<ImageSource> CreateImageSource(const string &path)
{
    uint32_t errorCode = 0;
    SourceOptions opts;
    opts.formatHint = "image/jpeg";
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, errorCode);
    EXPECT_NE(imageSource, nullptr);
    return imageSource;
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageTimeInfo_test_001, TestSize.Level1)
{
    const string srcPath = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    const string destPath = "/storage/cloud/100/files/Documents/medialib_ExtractImageTimeInfo_test_001.jpg";
    const string cmd = "cp " + srcPath + " " + destPath;
    std::system(cmd.c_str());
    std::unique_ptr<ImageSource> imageSource = CreateImageSource(destPath);

    uint32_t index = 0;
    std::string valueModify = "2018:11:24 15:49:41";
    uint32_t retModify =
        imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL, valueModify, destPath);
    ASSERT_EQ(retModify, E_OK);

    unique_ptr<Metadata> data = CreateImageMetadata(destPath);

    MetadataExtractor::ExtractImageTimeInfo(imageSource, data);
    EXPECT_EQ(data->GetDetailTime(), "2018:11:24 15:49:41");
    EXPECT_EQ(data->GetDateDay(), "20181124");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageTimeInfo_test_002, TestSize.Level1)
{
    const string srcPath = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    const string destPath = "/storage/cloud/100/files/Documents/medialib_ExtractImageTimeInfo_test_002.jpg";
    const string cmd = "cp " + srcPath + " " + destPath;
    std::system(cmd.c_str());
    std::unique_ptr<ImageSource> imageSource = CreateImageSource(destPath);
    uint32_t index = 0;
    std::string gpsDateStamp = "2018:09:07";
    uint32_t retModify =
        imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_DATE_STAMP, gpsDateStamp, destPath);
    ASSERT_EQ(retModify, E_OK);
    std::string gpsTimeStamp = "08:58:21.00";
    retModify = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_TIME_STAMP, gpsTimeStamp, destPath);
    ASSERT_EQ(retModify, E_OK);

    unique_ptr<Metadata> data = CreateImageMetadata(destPath);

    MetadataExtractor::ExtractImageTimeInfo(imageSource, data);
    EXPECT_EQ(data->GetDetailTime(), "2018:09:07 08:58:21");
    EXPECT_EQ(data->GetDateDay(), "20180907");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageTimeInfo_test_003, TestSize.Level1)
{
    const string srcPath = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    const string destPath = "/storage/cloud/100/files/Documents/medialib_ExtractImageTimeInfo_test_003.jpg";
    const string cmd = "cp " + srcPath + " " + destPath;
    std::system(cmd.c_str());
    std::unique_ptr<ImageSource> imageSource = CreateImageSource(destPath);
    uint32_t index = 0;
    std::string valueModify = "2018:11:27 15:49:41";
    uint32_t retModify = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_DATE_TIME, valueModify, destPath);
    ASSERT_EQ(retModify, E_OK);

    unique_ptr<Metadata> data = CreateImageMetadata(destPath);

    MetadataExtractor::ExtractImageTimeInfo(imageSource, data);
    EXPECT_EQ(data->GetDetailTime(), "2018:11:27 15:49:41");
    EXPECT_EQ(data->GetDateDay(), "20181127");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, ExtractImageTimeInfo_test_004, TestSize.Level1)
{
    const string srcPath = "/storage/cloud/100/files/Documents/CreateImageLcdTest_001.jpg";
    const string destPath = "/storage/cloud/100/files/Documents/medialib_ExtractImageTimeInfo_test_004.jpg";
    const string cmd = "cp " + srcPath + " " + destPath;
    std::system(cmd.c_str());
    std::unique_ptr<ImageSource> imageSource = CreateImageSource(destPath);

    unique_ptr<Metadata> data = CreateImageMetadata(destPath);
    data->SetFileDateModified(1541868049000);  // 2018-11-11 00:40:49

    MetadataExtractor::ExtractImageTimeInfo(imageSource, data);
    EXPECT_EQ(data->GetDateDay(), "20181111");
}

static unique_ptr<Metadata> CreateVideoMetadata()
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/100/files/Documents/CreateVideoThumbnailTest_001.mp4";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_VIDEO));
    data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::DEFAULT));
    data->SetFilePath(path);
    int64_t dateModified = 1763369312167;
    data->SetFileDateModified(dateModified);
    return data;
}

static unordered_map<int32_t, std::string> GetResultMap()
{
    return {{AV_KEY_ALBUM, ""},
        {AV_KEY_ARTIST, ""},
        {AV_KEY_DURATION, ""},
        {AV_KEY_DATE_TIME_FORMAT, "2025-10-30 10:10:39"},
        {AV_KEY_VIDEO_HEIGHT, ""},
        {AV_KEY_VIDEO_WIDTH, ""},
        {AV_KEY_MIME_TYPE, ""},
        {AV_KEY_MIME_TYPE, ""},
        {AV_KEY_VIDEO_ORIENTATION, ""},
        {AV_KEY_VIDEO_IS_HDR_VIVID, ""},
        {AV_KEY_TITLE, ""},
        {AV_KEY_GENRE, ""},
        {AV_KEY_DATE_TIME_ISO8601, "2025-10-30T02:10:39.000000Z"},
        {AV_KEY_GLTF_OFFSET, ""}};
}

HWTEST_F(MediaLibraryMetadataExtractorTest, PopulateVideoTimeInfo_test_001, TestSize.Level1)
{
    unique_ptr<Metadata> data = CreateVideoMetadata();
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    meta->SetData(PHOTO_DATA_VIDEO_IOS_CREATION_DATE, "2025-10-30T10:05:39+0800");
    unordered_map<int32_t, std::string> resultMap = GetResultMap();
    MetadataExtractor::PopulateVideoTimeInfo(meta, resultMap, data);
    EXPECT_EQ(data->GetDetailTime(), "2025:10:30 10:05:39");
    EXPECT_EQ(data->GetDateDay(), "20251030");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, PopulateVideoTimeInfo_test_002, TestSize.Level1)
{
    unique_ptr<Metadata> data = CreateVideoMetadata();
    unordered_map<int32_t, std::string> resultMap = GetResultMap();
    MetadataExtractor::PopulateVideoTimeInfo(nullptr, resultMap, data);
    EXPECT_EQ(data->GetDetailTime(), "2025:10:30 10:10:39");
    EXPECT_EQ(data->GetDateDay(), "20251030");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, PopulateVideoTimeInfo_test_003, TestSize.Level1)
{
    unique_ptr<Metadata> data = CreateVideoMetadata();
    data->SetFileDateModified(1541878049000);  // 2018-11-11 03:27:29
    unordered_map<int32_t, std::string> resultMap;
    MetadataExtractor::PopulateVideoTimeInfo(nullptr, resultMap, data);
    EXPECT_EQ(data->GetDateDay(), "20181111");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, FillExtractedMetadata_photo, TestSize.Level1)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_IMAGE));
    // subtype = PhotoSubType::MOVING_PHOTO
    data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    data->SetFilePath(path);
    data->SetFileDateModified(static_cast<int64_t>(11));
    std::shared_ptr<Media::Meta> meta = std::make_shared<Media::Meta>();
    float longtitude = 1.2;
    float latitude = 138.2;
    meta->SetData(Tag::MEDIA_LONGITUDE, longtitude);
    meta->SetData(Tag::MEDIA_LATITUDE, latitude);
    unordered_map<int32_t, std::string> resultMap;
    resultMap = {{AV_KEY_ALBUM, "a"}, {AV_KEY_ARTIST, "a"}, {AV_KEY_DURATION, "a"}, {AV_KEY_DATE_TIME_FORMAT, "a"},
        {AV_KEY_VIDEO_HEIGHT, "a"}, {AV_KEY_VIDEO_WIDTH, "a"}, {AV_KEY_MIME_TYPE, "a"}, {AV_KEY_MIME_TYPE, "a"},
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"},
        {AV_KEY_DATE_TIME_ISO8601, "a"}, {AV_KEY_GLTF_OFFSET, "a"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetAlbum(), "a");
    EXPECT_EQ(data->GetLongitude(), longtitude);
    EXPECT_EQ(data->GetLatitude(), latitude);
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_001");
    std::string userComment = "normal user comment";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "normal user comment";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_001 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_002");
    // 正常情况
    std::string userComment =
        "<mgzn-title><mgzn-cpname><mgzn-download><mgzn-contenturi><mgzn-pkgname><mgzn-content>"
        "This is real content"
        "<mgzn-worksdes><mgzn-appver><mgzn-contentid>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "This is real content";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_002 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_003");
    // 内容中包含换行和特殊字符
    std::string userComment =
        "<mgzn-title>Title</mgzn-title>"
        "<mgzn-cpname>Company</mgzn-cpname>"
        "<mgzn-download>true</mgzn-download>"
        "<mgzn-contenturi>http://example.com</mgzn-contenturi>"
        "<mgzn-pkgname>com.example.app</mgzn-pkgname>"
        "<mgzn-content>"
        "这是真实内容\n包含换行和\t制表符\n还有特殊字符：@#$%"
        "<mgzn-worksdes>"
        "<mgzn-appver>1.0.0</mgzn-appver>"
        "<mgzn-contentid>123456</mgzn-contentid>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes =
        "这是真实内容\n包含换行和\t制表符\n还有特殊字符：@#$%";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_003 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_004");
    // 标签顺序错误（开始标签在结束标签之后）
    std::string userComment = "<mgzn-worksdes>end first<mgzn-content>start later";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    // 由于长度小于140，应返回原字符串
    std::string expectRes = "<mgzn-worksdes>end first<mgzn-content>start later";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_004 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_005");
    // 只有开始标签没有结束标签
    std::string userComment = "<mgzn-content>no end tag";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "<mgzn-content>no end tag";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_005 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_006");
    // 长度刚好140
    std::string userComment(140, 'a'); // 创建140个'a'的字符串
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes(140, 'a');
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_006 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_007");
    // 长度141，应该返回空字符串
    std::string userComment(141, 'b'); // 创建141个'b'的字符串
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_007 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_008");
    // 测试空字符串
    std::string userComment = "";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_008 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_009");
    // 内容为空
    std::string userComment = "<mgzn-content><mgzn-worksdes>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_009 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_010");
    // 包含特殊字符和换行
    std::string userComment = "<mgzn-content>Line1\nLine2\tTab<mgzn-worksdes>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "Line1\nLine2\tTab";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_010 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_011");
    // 只有结束标签没有开始标签
    std::string userComment = "no start tag<mgzn-worksdes>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "no start tag<mgzn-worksdes>";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_011 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_012");
    // 测试边界情况 - 长度为139的备注
    std::string userComment(139, 'c');
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes(139, 'c');
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_012 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_013");
    // 在完整格式中提取内容
    std::string userComment =
        "<mgzn-title>Test Title<mgzn-cpname>Test Company<mgzn-download>yes"
        "<mgzn-contenturi>uri<mgzn-pkgname>pkg<mgzn-content>"
        "Actual user comment text here"
        "<mgzn-worksdes><mgzn-appver>1.0<mgzn-contentid>12345";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "Actual user comment text here";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_013 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_014");
    // 内容为空的情况
    std::string userComment =
        "<mgzn-title><mgzn-cpname><mgzn-download><mgzn-contenturi><mgzn-pkgname><mgzn-content>"
        ""
        "<mgzn-worksdes><mgzn-appver><mgzn-contentid>";
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_014 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_015, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_015");
    // 只有部分标签（缺少开始或结束标签）
    std::string userComment =
        "<mgzn-title><mgzn-cpname><mgzn-download><mgzn-contenturi><mgzn-pkgname><mgzn-content>"
        "Partial format content";
    // 缺少<mgzn-worksdes>标签，应该按普通备注处理
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = "<mgzn-title><mgzn-cpname><mgzn-download><mgzn-contenturi><mgzn-pkgname><mgzn-content>"
                            "Partial format content";
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_015 end");
}

HWTEST_F(MediaLibraryMetadataExtractorTest, GetCompatibleUserComment_test_016, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_016");
    // 标签顺序正确但内容很长（超过140字符）
    std::string longContent(150, 'x'); // 150个'x'
    std::string userComment =
        "<mgzn-title><mgzn-cpname><mgzn-download><mgzn-contenturi><mgzn-pkgname><mgzn-content>"
        + longContent +
        "<mgzn-worksdes><mgzn-appver><mgzn-contentid>";
    // 提取的内容长度为150，应该正常返回
    std::string ret = MetadataExtractor::GetCompatibleUserComment(userComment);
    std::string expectRes = longContent;
    EXPECT_EQ(ret, expectRes);
    MEDIA_INFO_LOG("medialib_GetCompatibleUserComment_test_016 end");
}

} // namespace Media
} // namespace OHOS