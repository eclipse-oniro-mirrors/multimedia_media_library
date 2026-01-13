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

#include "medialibrary_thumbnail_rdb_utils_test.h"

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_service.h"
#include "media_upgrade.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static shared_ptr<MediaLibraryRdbStore> rdbStore = nullptr;
static int64_t id;

class TddRdbOpenCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override
    {
        return E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

static void InitRdbStore()
{
    const string dbPath = "/data/test/medialibrary_thumbnail_rdb_utils_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    TddRdbOpenCallback openCallback;

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, 1, openCallback);
    ASSERT_EQ(ret, E_OK);
    rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);

    ret = rdbStore->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    ret = rdbStore->Insert(id, PhotoColumn::PHOTOS_TABLE, values);
    ASSERT_EQ(ret, NativeRdb::E_OK);
}

static void DeleteRdbStore()
{
    string dropSql = "DROP TABLE IF EXIST " + PhotoColumn::PHOTOS_TABLE + ";";
    int32_t ret = rdbStore->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop photos table ret: %{public}d", ret == NativeRdb::E_OK);
    MediaLibraryUnitTestUtils::StopUnistore();
}

void ThumbnailRdbUtilsTest::SetUpTestCase(void)
{
    InitRdbStore();
}
void ThumbnailRdbUtilsTest::TearDownTestCase(void)
{
    DeleteRdbStore();
}
void ThumbnailRdbUtilsTest::SetUp(void) {}
void ThumbnailRdbUtilsTest::TearDown(void) {}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_001");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    ThumbnailData data;
    vector<ThumbnailData> datas { data };
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfos(rdbStore, predicates, columns, datas);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfos_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_002");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    ThumbnailData data;
    vector<ThumbnailData> datas { data };
    int32_t err;
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfos(rdbStore, predicates, columns, datas, err);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_002 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfos_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_003");

    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    ThumbnailData data;
    vector<ThumbnailData> datas { data };
    int32_t err;
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfos(resultSet, columns, datas, err);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfos_Test_003 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfo_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_001");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    ThumbnailData data;
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfo(rdbStore, predicates, columns, data);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfo_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_002");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    ThumbnailData data;
    int32_t err;
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfo(rdbStore, predicates, columns, data, err);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_002 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryThumbnailDataInfo_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_003");
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    ThumbnailData data;
    int32_t err;
    auto ret = ThumbnailRdbUtils::QueryThumbnailDataInfo(resultSet, columns, data, err);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryThumbnailDataInfo_Test_003 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, CheckResultSetCount_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckResultSetCount_Test_001");
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    int32_t err;
    auto ret = ThumbnailRdbUtils::CheckResultSetCount(resultSet, err);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("CheckResultSetCount_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, ParseQueryResult_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseQueryResult_Test_001");
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    int32_t err;
    ThumbnailData data;
    ThumbnailRdbUtils::ParseQueryResult(resultSet, data, err, columns);
    EXPECT_EQ(err, E_OK);
    MEDIA_INFO_LOG("ParseQueryResult_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, ParseStringResult_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseStringResult_Test_001");
    vector<string> columns = { MediaColumn::MEDIA_ID };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    ASSERT_NE(resultSet, nullptr);
    string id;
    ThumbnailRdbUtils::ParseStringResult(resultSet, 0, id);
    MEDIA_INFO_LOG("ParseStringResult_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, ParseInt32Result_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseInt32Result_Test_001");
    vector<string> columns = { PhotoColumn::PHOTO_EXIF_ROTATE };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t exifRotate;
    ThumbnailRdbUtils::ParseInt32Result(resultSet, 0, exifRotate);
    EXPECT_EQ(exifRotate >= 0, true);
    MEDIA_INFO_LOG("ParseInt32Result_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, ParseInt64Result_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ParseInt64Result_Test_001");
    vector<string> columns = { PhotoColumn::PHOTO_META_DATE_MODIFIED };
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int64_t dataModified;
    ThumbnailRdbUtils::ParseInt64Result(resultSet, 0, dataModified);
    EXPECT_EQ(dataModified >= 0, true);
    MEDIA_INFO_LOG("ParseInt64Result_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, QueryLocalNoExifRotateInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryLocalNoExifRotateInfos_Test_001");
    ThumbRdbOpt opts;
    opts.store = rdbStore;
    ThumbnailData data;
    vector<ThumbnailData> datas { data };
    auto ret = ThumbnailRdbUtils::QueryLocalNoExifRotateInfos(opts, datas);
    EXPECT_EQ(ret, true);
    MEDIA_INFO_LOG("QueryLocalNoExifRotateInfos_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, UpdateExifRotateAndDirty_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateExifRotateAndDirty_Test_001");
    ThumbRdbOpt opts;
    opts.store = rdbStore;
    ThumbnailData data;
    data.id = to_string(id);
    data.exifRotate = 1;
    vector<ThumbnailData> datas { data };
    auto ret = ThumbnailRdbUtils::UpdateExifRotateAndDirty(data, DirtyType::TYPE_MDIRTY, true);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("UpdateExifRotateAndDirty_Test_001 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, UpdateExifRotateAndDirty_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateExifRotateAndDirty_Test_002");
    ThumbRdbOpt opts;
    opts.store = rdbStore;
    ThumbnailData data;
    data.id = to_string(id);
    data.exifRotate = 1;
    vector<ThumbnailData> datas { data };
    auto ret = ThumbnailRdbUtils::UpdateExifRotateAndDirty(data, DirtyType::TYPE_MDIRTY, false);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("UpdateExifRotateAndDirty_Test_002 end");
}

HWTEST_F(ThumbnailRdbUtilsTest, UpdateRdbStoreById_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateRdbStoreById_Test_001");
    ThumbRdbOpt opts;
    opts.store = rdbStore;
    opts.table = PhotoColumn::PHOTOS_TABLE;
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    auto ret = ThumbnailRdbUtils::UpdateRdbStoreById(opts, to_string(id), values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("UpdateRdbStoreById_Test_001 end");
}

} // namespace Media
} // namespace OHOS