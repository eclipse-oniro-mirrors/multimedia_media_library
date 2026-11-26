/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "medialibrary_thumbnail_utils_test.h"

#include <thread>

#include "medialibrary_mocksinglekvstore.h"
#include "highlight_column.h"
#include "kvstore.h"
#include "vision_db_sqls.h"

#include "exif_rotate_utils.h"
#define private public
#include "thumbnail_service.h"
#include "ithumbnail_helper.h"
#include "thumbnail_generate_helper.h"
#undef private
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"

  
using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
 
namespace OHOS {
namespace Media {
 
void MediaLibraryThumbnailUtilsTest::SetUpTestCase(void) {}
 
void MediaLibraryThumbnailUtilsTest::TearDownTestCase(void) {}
     
void MediaLibraryThumbnailUtilsTest::SetUp() {}
     
void MediaLibraryThumbnailUtilsTest::TearDown(void) {}
 
HWTEST_F(MediaLibraryThumbnailUtilsTest, LoadAudioFileInfo_test_001, TestSize.Level0)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = nullptr;
    ThumbnailData data;
    Size desiredsize;
    uint32_t errCode;
    auto res = ThumbnailUtils::LoadAudioFileInfo(avMetadataHelper, data, desiredsize, errCode);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, LoadVideoFrame_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size desiredsize;
    int64_t timeStamp = 0;
    data.path = "test_path";
    auto res = ThumbnailUtils::LoadVideoFrame(data, desiredsize, timeStamp);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, GenTargetPixelmap_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size desiredsize;
    auto res = ThumbnailUtils::GenTargetPixelmap(data, desiredsize);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, SaveAfterPacking_test_001, TestSize.Level0)
{
    ThumbnailData data;
    data.path = "test_path";
    bool isSourceEx = false;
    string tempOutputPath = "temp_outout_path";
    Size desiredsize;
    auto res = ThumbnailUtils::SaveAfterPacking(data, isSourceEx, tempOutputPath);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, QueryLcdCount_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    int outLcdCount = 0;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, QueryLcdCountByTime_test_001, TestSize.Level0)
{
    int64_t time = 12345467890;
    bool before = true;
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    int outLcdCount = 0;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, QueryLocalNoLcdInfos_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    std::vector<ThumbnailData> infos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryLocalNoLcdInfos(opts, infos, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::QueryLocalNoLcdInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, QueryNoHighlightPath_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    ThumbnailData data;
    data.id = "123";
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryNoHighlightPath(opts, data, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::QueryNoHighlightPath(opts, data, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, QueryNoHighlightInfos_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    std::vector<ThumbnailData> infos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, GetHighlightTracks_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    opts.row = "123";
    std::vector<int> trackInfos;
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::GetHighlightTracks(opts, trackInfos, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::GetHighlightTracks(opts, trackInfos, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, UpdateHighlightInfo_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    ThumbnailData data;
    data.id = "12345";
    data.tracks = "track_data";
    int err = 0;
    opts.store = nullptr;
    auto res = ThumbnailUtils::UpdateHighlightInfo(opts, data, err);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::UpdateHighlightInfo(opts, data, err);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, CheckDateTaken_test_001, TestSize.Level0)
{
    ThumbRdbOpt opts;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    ThumbnailData data;
    data.dateTaken = "";
    data.id = "123";
    opts.store = nullptr;
    auto res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
    opts.store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    res = ThumbnailUtils::CheckDateTaken(opts, data);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, GetLocalThumbSize_test_001, TestSize.Level0)
{
    ThumbnailData data;
    Size size;
    ThumbnailType type = ThumbnailType::THUMB;
    data.path = "/path/to/image.jpg";
    auto res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
    type = ThumbnailType::THUMB_ASTC;
    res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
    type = ThumbnailType::LCD;
    res = ThumbnailUtils::GetLocalThumbSize(data, type, size);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, ConvertStrToInt32_test_001, TestSize.Level0)
{
    string str = "123";
    int32_t ret;
    auto res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, true);
    str = "test";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
    str = "-100000000000000";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
    str = "100000000000000";
    res = ThumbnailUtils::ConvertStrToInt32(str, ret);
    EXPECT_EQ(res, false);
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, CreateOutputPath_test_001, TestSize.Level0)
{
    ThumbnailData data;
    auto res = ThumbnailUtils::CreateOutputPath(data, THUMBNAIL_LCD_SUFFIX);
    EXPECT_EQ(res, "");
    data.tracks = "tracks";
    res = ThumbnailUtils::CreateOutputPath(data, THUMBNAIL_LCD_SUFFIX);
    EXPECT_EQ(res, "");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsExCloudThumbnail_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001");
    ThumbnailData data;
    data.orientation = 1;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsExCloudThumbnail_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_002");
    ThumbnailData data;
    data.exifRotate = 1;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsExCloudThumbnail_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_003");
    ThumbnailData data;
    data.exifRotate = 2;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsExCloudThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsExCloudThumbnail_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, NeedRotateThumbnail_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_001");
    ThumbnailData data;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, NeedRotateThumbnail_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_002");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    data.lastLoadSource = SourceState::LOCAL_THUMB;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, NeedRotateThumbnail_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_003");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    data.lastLoadSource = SourceState::LOCAL_ORIGIN;
    auto ret = ThumbnailUtils::NeedRotateThumbnail(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("NeedRotateThumbnail_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_001");
    ThumbnailData data;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_001 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_002");
    ThumbnailData data;
    data.mediaType = 2;
    data.exifRotate = 2;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_002 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_003");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 1;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_003 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_004");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 0;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("IsImageWithRotate_test_004 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_005");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 2;
    data.orientation = 0;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsImageWithRotate_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsImageWithRotate_test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsImageWithRotate_test_005");
    ThumbnailData data;
    data.mediaType = 1;
    data.exifRotate = 0;
    data.orientation = 90;
    auto ret = ThumbnailUtils::IsImageWithRotate(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsImageWithRotate_test_005 end");
}

HWTEST_F(MediaLibraryThumbnailUtilsTest, IsUseRotatedSource_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("IsUseRotatedSource_test_001");
    ThumbnailData data;
    data.lastLoadSource = SourceState::LOCAL_THUMB;
    auto ret = ThumbnailUtils::IsUseRotatedSource(data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("IsUseRotatedSource_test_001 end");
}

} // namespace Media
} // namespace OHOS