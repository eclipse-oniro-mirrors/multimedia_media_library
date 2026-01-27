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

#include <thread>
#include "avmetadatahelper.h"
#include "kvstore.h"
#include "media_remote_thumbnail_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#define private public
#include "medialibrary_utils_test.h"
#include "thumbnail_service.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_utils.h"
#include "post_event_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"


using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
class ConfigTestOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const string CREATE_TABLE_TEST;
};

const string ConfigTestOpenCall::CREATE_TABLE_TEST = string("CREATE TABLE IF NOT EXISTS test_table ") +
    "(file_id INTEGER PRIMARY KEY AUTOINCREMENT, media_type TEXT NOT NULL, age INTEGER, salary REAL, blobType BLOB)";

int ConfigTestOpenCall::OnCreate(RdbStore &store)
{
    return store.ExecuteSql(CREATE_TABLE_TEST);
}

int ConfigTestOpenCall::OnUpgrade(RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

shared_ptr<MediaLibraryRdbStore> storePtr = nullptr;

void MediaLibraryUtilsTest::SetUpTestCase(void)
{
    const string dbPath = "/data/test/medialibrary_utils_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    ConfigTestOpenCall helper;
    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, 1, helper);
    EXPECT_EQ(ret, E_OK);
    storePtr = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(storePtr, nullptr);
}

void MediaLibraryUtilsTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::StopUnistore();
}

// SetUp:Execute before each test case
void MediaLibraryUtilsTest::SetUp() {}

void MediaLibraryUtilsTest::TearDown(void) {}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryThumbnailSet_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string row = "medialib_QueryThumbnailSet_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = CONST_MEDIALIBRARY_TABLE,
        .row = row
    };
    auto resultSetPtr = ThumbnailUtils::QueryThumbnailSet(opts);
    EXPECT_NE(resultSetPtr, nullptr);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryThumbnailInfo_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string row = "medialib_QueryThumbnailInfo_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = CONST_MEDIALIBRARY_TABLE,
        .row = row
    };
    ThumbnailData data;
    int err = 0;
    auto resultSetPtr = ThumbnailUtils::QueryThumbnailInfo(opts, data, err);
    EXPECT_EQ(resultSetPtr, nullptr);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryLcdCount_test_table, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = "test_table"
    };
    int outLcdCount = 0;
    int err = 0;
    bool ret = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryLcdCount_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = CONST_MEDIALIBRARY_TABLE
    };
    int outLcdCount = 0;
    int err = 0;
    bool ret = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryLcdCount_test_002, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "Photos";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    int outLcdCount = 0;
    int err = 0;
    bool ret = ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryDistributeLcdCount_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbRdbOpt opts = {
        .store = storePtr,
    };
    int outLcdCount = 0;
    int err = 0;
    bool ret = ThumbnailUtils::QueryDistributeLcdCount(opts, outLcdCount, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryAgingLcdInfos_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "medialib_QueryAgingLcdInfos_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    int LcdLimit = 0;
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryAgingLcdInfos(opts, LcdLimit, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoLcdInfos_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "medialib_QueryNoLcdInfos_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoLcdInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoLcdInfos_test_002, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "Photos";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoLcdInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoThumbnailInfos_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "medialib_QueryNoThumbnailInfos_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoThumbnailInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoThumbnailInfos_test_002, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "Photos";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoThumbnailInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoHighlightInfos_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "medialib_QueryNoHighlightInfos_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoHighlightInfos_test_002, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "tab_analysis_video_label";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<ThumbnailData> infos;
    int err = 0;
    bool ret = ThumbnailUtils::QueryNoHighlightInfos(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_GetHighlightValue_test_001, TestSize.Level1)
{
    string str = "{name:value}";
    string key = "nonexistent";
    EXPECT_EQ(ThumbnailUtils::GetHighlightValue(str, key), "");
    str = "{name value}";
    key = "name";
    EXPECT_EQ(ThumbnailUtils::GetHighlightValue(str, key), "");
    str = "{name:value";
    key = "name";
    EXPECT_EQ(ThumbnailUtils::GetHighlightValue(str, key), "");
    str = "{name:value, type:1}";
    key = "name";
    EXPECT_EQ(ThumbnailUtils::GetHighlightValue(str, key), "value");
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryHighlightTriggerPath_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "Photos";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    ThumbnailData data;
    int err = 0;
    bool ret = ThumbnailUtils::QueryHighlightTriggerPath(opts, data, err);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_GetHighlightTracks_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string table = "tab_analysis_video_label";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    vector<int> infos;
    int err = 0;
    bool ret = ThumbnailUtils::GetHighlightTracks(opts, infos, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_QueryNoHighlightPath_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbnailData data;
    ThumbRdbOpt opts = {
        .store = storePtr,
    };
    int err = 0;
    data.id = "1";
    bool ret = ThumbnailUtils::QueryNoHighlightPath(opts, data, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_CacheLcdInfo_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    string row = "medialib_CacheLcdInfo_test_001";
    string table = "medialib_CacheLcdInfo_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table,
        .row = row
    };
    ThumbnailData data;
    bool ret = ThumbnailUtils::CacheLcdInfo(opts, data);
    EXPECT_EQ(ret, false);
    ret = ThumbnailUtils::CacheLcdInfo(opts, data);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_CleanThumbnailInfo_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbRdbOpt opts = {
    .store = storePtr,
    };
    bool withThumb = true;
    bool withLcd = true;
    bool ret = ThumbnailUtils::CleanThumbnailInfo(opts, withThumb, withLcd);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_ResizeImage_test_001, TestSize.Level1)
{
    vector<uint8_t> data;
    Size size;
    unique_ptr<PixelMap> pixelMap = nullptr;
    bool ret = ThumbnailUtils::ResizeImage(data, size, pixelMap);
    EXPECT_EQ(ret, false);
    data.push_back(8);
    ret = ThumbnailUtils::ResizeImage(data, size, pixelMap);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_DeleteAllThumbFilesAndAstc_test_001, TestSize.Level1)
{
    ASSERT_NE(storePtr, nullptr);
    ThumbnailData thumbnailData;
    string row = "medialib_DeleteAllThumbFilesAndAstc_test_001";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = CONST_MEDIALIBRARY_TABLE,
        .row = row
    };
    ThumbnailData data;
    data.id = row;
    bool ret = ThumbnailUtils::DeleteAllThumbFilesAndAstc(opts, data);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_UpdateAstcDateTakenFromKvStore_test_001, TestSize.Level1)
{
    ThumbnailData data;
    const string testStr = "medialib_UpdateAstcDateTakenFromKvStore_test_001";
    data.dateTaken = testStr;
    ThumbRdbOpt opts = {
        .dateTaken = testStr,
        .row = testStr,
    };
    bool ret = ThumbnailUtils::UpdateAstcDateTakenFromKvStore(opts, data);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_UpdateAstcDateTakenFromKvStore_test_002, TestSize.Level0)
{
    ThumbnailData data;
    const string testStr = "test";
    data.dateTaken = testStr;
    ThumbRdbOpt opts = {
        .dateTaken = testStr,
        .row = testStr,
    };
    bool ret = ThumbnailUtils::UpdateAstcDateTakenFromKvStore(opts, data);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_compressImage_test_001, TestSize.Level1)
{
    ThumbnailData data;
    data.thumbnail.push_back(0);
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    bool ret = ThumbnailUtils::CompressImage(pixelMap, data.thumbnail);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_LoadSourceImage_test_001, TestSize.Level1)
{
    ThumbnailData data;
    data.mediaType = MEDIA_TYPE_VIDEO;
    data.loaderOpts.decodeInThumbSize = true;
    data.path = "";
    bool ret = ThumbnailUtils::LoadSourceImage(data);
    EXPECT_EQ(ret, false);
    data.mediaType = MEDIA_TYPE_AUDIO;
    ret = ThumbnailUtils::LoadSourceImage(data);
    EXPECT_EQ(ret, false);
    data.mediaType = MEDIA_TYPE_MEDIA;
    data.path = "Documents/";
    ret = ThumbnailUtils::LoadSourceImage(data);
    EXPECT_EQ(ret, false);
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    ret = ThumbnailUtils::LoadSourceImage(data);
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_uTCTimeSeconds_test_001, TestSize.Level1)
{
    int64_t ret = ThumbnailUtils::UTCTimeMilliSeconds();
    EXPECT_GT(ret, 0);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_parseQueryResult_test_001, TestSize.Level1)
{
    vector<string> column = {
        REMOTE_THUMBNAIL_DB_FILE_ID,
        CONST_MEDIA_DATA_DB_LCD
    };
    ThumbnailData data;
    data.id = "0";
    data.path = "/storage/cloud/files";
    data.mediaType = 0;
    int err = 0;
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = REMOTE_THUMBNAIL_TABLE,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.IsNotNull(CONST_MEDIA_DATA_DB_LCD);
    rdbPredicates.EqualTo(REMOTE_THUMBNAIL_DB_UDID, opts.udid);
    rdbPredicates.Limit(0);
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    ThumbnailRdbUtils::ParseQueryResult(resultSet, data, err, column);
    EXPECT_NE(err, 0);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_parseHighlightQueryResult_test_001, TestSize.Level0)
{
    vector<string> column = {
        REMOTE_THUMBNAIL_DB_FILE_ID,
        CONST_MEDIA_DATA_DB_LCD
    };
    ThumbnailData data;
    data.id = "0";
    data.path = "/storage/cloud/files";
    data.mediaType = 0;
    int err = 0;
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = REMOTE_THUMBNAIL_TABLE,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.IsNotNull(CONST_MEDIA_DATA_DB_LCD);
    rdbPredicates.EqualTo(REMOTE_THUMBNAIL_DB_UDID, opts.udid);
    rdbPredicates.Limit(0);
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    ThumbnailUtils::ParseHighlightQueryResult(resultSet, data, err);
    EXPECT_NE(err, 0);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_parseStringResult_test_001, TestSize.Level1)
{
    vector<string> column = {
        REMOTE_THUMBNAIL_DB_FILE_ID,
        CONST_MEDIA_DATA_DB_LCD
    };
    ThumbRdbOpt opts = {
        .store = storePtr,
    };
    int index = 0;
    string data = "ParseStringResult";
    RdbPredicates rdbPredicates(REMOTE_THUMBNAIL_TABLE);
    rdbPredicates.IsNotNull(CONST_MEDIA_DATA_DB_LCD);
    rdbPredicates.EqualTo(REMOTE_THUMBNAIL_DB_UDID, opts.udid);
    rdbPredicates.Limit(0);
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    string dataTest;
    ThumbnailRdbUtils::ParseStringResult(resultSet, -1, dataTest);
    EXPECT_EQ(dataTest, "");
    ThumbnailRdbUtils::ParseStringResult(resultSet, index, dataTest);
    EXPECT_EQ(dataTest, "");
}

HWTEST_F(MediaLibraryUtilsTest, medialib_checkResultSetCount_test_001, TestSize.Level1)
{
    vector<string> column = {
        CONST_MEDIA_DATA_DB_ID,
        CONST_MEDIA_DATA_DB_FILE_PATH,
        CONST_MEDIA_DATA_DB_THUMBNAIL,
        CONST_MEDIA_DATA_DB_LCD,
        CONST_MEDIA_DATA_DB_MEDIA_TYPE,
        CONST_MEDIA_DATA_DB_DATE_MODIFIED
    };
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = CONST_MEDIALIBRARY_TABLE,
        .row = "CheckResultSetCount",
    };
    int err = 0;
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.IsNotNull(CONST_MEDIA_DATA_DB_LCD);
    rdbPredicates.EqualTo(REMOTE_THUMBNAIL_DB_UDID, opts.udid);
    rdbPredicates.Limit(0);
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    bool ret = ThumbnailRdbUtils::CheckResultSetCount(nullptr, err);
    EXPECT_EQ(ret, false);
    ret = ThumbnailRdbUtils::CheckResultSetCount(resultSet, err);
    EXPECT_EQ(ret, false);
    auto resultSetTest = ThumbnailUtils::QueryThumbnailSet(opts);
    ret = ThumbnailRdbUtils::CheckResultSetCount(resultSetTest, err);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_queryThumbDataFromFileId_test_001, TestSize.Level1)
{
    string table = "Photos";
    ThumbRdbOpt opts = {
        .store = storePtr,
        .table = table
    };
    const string id = "1";
    ThumbnailData data;
    int err = 0;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, id, data, err);

    // expect err
    EXPECT_NE(err, 0);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_scaleTargetImage_test_001, TestSize.Level1)
{
    Size targetSize;
    targetSize.width = 20;
    targetSize.height = 20;
    ThumbnailData data;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    bool ret = ThumbnailUtils::ScaleTargetPixelMap(pixelMap, targetSize, Media::AntiAliasingOption::HIGH);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_loadImageFile_test_001, TestSize.Level1)
{
    ThumbnailData data;
    Size desiredSize;
    desiredSize.width = 20;
    desiredSize.height = 20;
    data.path = "/storage/cloud/files";
    data.loaderOpts.decodeInThumbSize  = false;
    std::shared_ptr<PixelMap> pixelMap = make_shared<PixelMap>();
    data.source.SetPixelMap(pixelMap);
    std::string sourcePath = "";
    bool ret = ThumbnailUtils::LoadImageFile(data, desiredSize);
    EXPECT_EQ(ret, false);
    ret = ThumbnailUtils::LoadVideoFrame(data, desiredSize, 0);
    EXPECT_EQ(ret, false);
    ret = ThumbnailUtils::LoadAudioFile(data, desiredSize);
    EXPECT_EQ(ret, false);
    data.tracks = "tracks";
    data.timeStamp = "0";
    ret = ThumbnailUtils::LoadAudioFile(data, desiredSize);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_ParseVideoSize_test_001, TestSize.Level1)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    bool ret = ThumbnailUtils::ParseVideoSize(avMetadataHelper, videoWidth, videoHeight);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_ConvertStrToInt32_test_001, TestSize.Level1)
{
    std::string testStr = "";
    int32_t result = 0;
    bool ret = ThumbnailUtils::ConvertStrToInt32(testStr, result);
    EXPECT_EQ(ret, false);
    testStr = "not number string";
    ret = ThumbnailUtils::ConvertStrToInt32(testStr, result);
    EXPECT_EQ(ret, false);
    testStr = "10000000000";
    ret = ThumbnailUtils::ConvertStrToInt32(testStr, result);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_resizeThumb_test_001, TestSize.Level1)
{
    int width, height;
    width = 512;
    height = 768;
    bool result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 350);
    EXPECT_EQ(height, 525);

    width = 512;
    height = 2560;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 350);
    EXPECT_EQ(height, 1050);

    width = 200;
    height = 200;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 200);
    EXPECT_EQ(height, 200);

    width = 128;
    height = 300;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 128);
    EXPECT_EQ(height, 300);

    width = 128;
    height = 1000;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 128);
    EXPECT_EQ(height, 384);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_resizeThumb_test_002, TestSize.Level0)
{
    int width, height;
    width = 768;
    height = 512;
    bool result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 525);
    EXPECT_EQ(height, 350);

    width = 2560;
    height = 512;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 1050);
    EXPECT_EQ(height, 350);

    width = 300;
    height = 128;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 300);
    EXPECT_EQ(height, 128);

    width = 1000;
    height = 128;
    result = ThumbnailUtils::ResizeThumb(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 384);
    EXPECT_EQ(height, 128);
}

HWTEST_F(MediaLibraryUtilsTest, medialib_resizeLcd_test_001, TestSize.Level1)
{
    int width, height;
    width = 1000;
    height = 1000;
    bool result = ThumbnailUtils::ResizeLcd(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 1000);
    EXPECT_EQ(height, 1000);

    width = 3840;
    height = 5760;
    result = ThumbnailUtils::ResizeLcd(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 1280);
    EXPECT_EQ(height, 1920);

    width = 3840;
    height = 57600;
    result = ThumbnailUtils::ResizeLcd(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 274);
    EXPECT_EQ(height, 4096);

    width = 3840;
    height = 28800;
    result = ThumbnailUtils::ResizeLcd(width, height);
    EXPECT_TRUE(result);
    EXPECT_EQ(width, 512);
    EXPECT_EQ(height, 3840);
}

HWTEST_F(MediaLibraryUtilsTest, PostErrorProcess_test_001, TestSize.Level1)
{
    auto postEventUtils = make_shared<PostEventUtils>();
    ASSERT_TRUE(postEventUtils);
    uint32_t errType = ErrType::DEFAULT_ERR;
    VariantMap error;
    postEventUtils->PostErrorProcess(errType, error);
}

HWTEST_F(MediaLibraryUtilsTest, PostStatProcess_test_002, TestSize.Level1)
{
    auto postEventUtils = make_shared<PostEventUtils>();
    ASSERT_TRUE(postEventUtils);
    uint32_t statType = StatType::DEFAULT_STAT;
    VariantMap stat;
    postEventUtils->PostStatProcess(statType, stat);
}
} // namespace Media
} // namespace OHOS