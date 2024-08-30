/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <cstdint>

#include "medialibrary_errno.h"
#include "medialibrary_scanner_db_test.h"
#include "media_scanner_db.h"
#include "userfile_manager_types.h"
#define private public
#include "metadata_extractor.h"
#include "meta.h"
#include "meta_key.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryScannerDbTest, medialib_Extract_test_001, TestSize.Level0)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_ALBUM));
    int32_t ret = MetadataExtractor::Extract(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_Extract_test_002, TestSize.Level0)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    int32_t ret = MetadataExtractor::Extract(data);
    EXPECT_EQ(ret, E_AVMETADATA);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_Extract_empty_path, TestSize.Level0)
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

HWTEST_F(MediaLibraryScannerDbTest, medialib_ExtractAVMetadata_test_001, TestSize.Level0)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_EQ(ret, E_AVMETADATA);
    data->SetFilePath(path);
    ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_NE(ret, E_OK);
    data->SetFilePath("ExtractAVMetadata");
    ret = MetadataExtractor::ExtractAVMetadata(data);
    EXPECT_EQ((ret == E_SYSCALL || ret == E_AVMETADATA), true);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ExtractAVMetadata_clone_test, TestSize.Level0)
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

HWTEST_F(MediaLibraryScannerDbTest, medialib_ExtractImageMetadata_test_001, TestSize.Level0)
{
    unique_ptr<Metadata> data = make_unique<Metadata>();
    unique_ptr<MediaScannerDb> mediaScannerDb;
    string path = "/storage/cloud/files/";
    mediaScannerDb->GetFileBasicInfo(path, data);
    data->SetFileMediaType(static_cast<MediaType>(MEDIA_TYPE_DEVICE));
    data->SetFilePath(path);
    int32_t ret = MetadataExtractor::ExtractImageMetadata(data);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_ExtractImageMetadata_test_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryScannerDbTest, medialib_FillExtractedMetadata_test_001, TestSize.Level0)
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
        {AV_KEY_VIDEO_ORIENTATION, ""}, {AV_KEY_VIDEO_IS_HDR_VIVID, ""}, {AV_KEY_TITLE, ""}, {AV_KEY_GENRE, ""}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetAlbum(), "");
    EXPECT_EQ(data->GetLongitude(), 0);
    EXPECT_EQ(data->GetLatitude(), 0);
}

HWTEST_F(MediaLibraryScannerDbTest, medialib_FillExtractedMetadata_test_002, TestSize.Level0)
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
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetAlbum(), "a");
    EXPECT_EQ(data->GetLongitude(), longtitude);
    EXPECT_EQ(data->GetLatitude(), latitude);
}
HWTEST_F(MediaLibraryScannerDbTest, medialib_FillExtractedMetadata_photo, TestSize.Level0)
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
        {AV_KEY_VIDEO_ORIENTATION, "a"}, {AV_KEY_VIDEO_IS_HDR_VIVID, "a"}, {AV_KEY_TITLE, "a"}, {AV_KEY_GENRE, "a"}};
    MetadataExtractor::FillExtractedMetadata(resultMap, meta, data);
    EXPECT_EQ(data->GetAlbum(), "a");
    EXPECT_EQ(data->GetLongitude(), longtitude);
    EXPECT_EQ(data->GetLatitude(), latitude);
}
} // namespace Media
} // namespace OHOS