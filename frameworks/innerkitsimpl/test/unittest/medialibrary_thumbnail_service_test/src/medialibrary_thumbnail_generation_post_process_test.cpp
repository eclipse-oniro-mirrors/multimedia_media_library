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

#include "medialibrary_thumbnail_generation_post_process_test.h"

#define private public
#include "thumbnail_generation_post_process.h"
#undef protected
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "vision_db_sqls.h"
#include "media_upgrade.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
shared_ptr<MediaLibraryRdbStore> store = nullptr;
string dataId;

class ConfigOpenCallPostProcessTest : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const string CREATE_TABLE_TEST;
};

const string ConfigOpenCallPostProcessTest::CREATE_TABLE_TEST = string("CREATE TABLE IF NOT EXISTS test ") +
    "(file_id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT NOT NULL, media_type INTEGER," +
    " date_added TEXT, display_name TEXT, thumbnail_ready TEXT, position TEXT)";

const int32_t E_GETROUWCOUNT_ERROR = 27394103;

int ConfigOpenCallPostProcessTest::OnCreate(NativeRdb::RdbStore &store)
{
    return store.ExecuteSql(CREATE_TABLE_TEST);
}

int ConfigOpenCallPostProcessTest::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

void MediaLibraryThumbnailGenerationPostProcessTest::SetUpTestCase()
{
    const string dbPath = "/data/test/medialibrary_thumbnail_generation_post_process_test.db";
    NativeRdb::RdbStoreConfig config(dbPath);
    ConfigOpenCallPostProcessTest helper;
    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, 1, helper);
    EXPECT_EQ(ret, E_OK);
    store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(store, nullptr);

    ret = store->ExecuteSql(PhotoUpgrade::CREATE_PHOTO_TABLE);
    ASSERT_EQ(ret, NativeRdb::E_OK);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 0);
    int64_t outRowId;
    ret = store->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    dataId = to_string(outRowId);
    ASSERT_EQ(ret, NativeRdb::E_OK);
}

void MediaLibraryThumbnailGenerationPostProcessTest::TearDownTestCase()
{
    string dropSql = "DROP TABLE " + PhotoUpgrade::CREATE_PHOTO_TABLE + ";";
    int32_t ret = store->ExecuteSql(dropSql);
    MEDIA_INFO_LOG("Drop photos table success: %{public}d", ret == NativeRdb::E_OK);
    MediaLibraryUnitTestUtils::StopUnistore();
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryThumbnailGenerationPostProcessTest::SetUp() {}

void MediaLibraryThumbnailGenerationPostProcessTest::TearDown(void) {}

HWTEST_F(MediaLibraryThumbnailGenerationPostProcessTest, PostProcess_test_001, TestSize.Level0)
{
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_LATER));
    ThumbnailData data;
    data.rdbUpdateCache = values;
    data.id = dataId;
    ThumbRdbOpt opts;
    opts.store = store;
    opts.table = PhotoColumn::PHOTOS_TABLE;

    int ret = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryThumbnailGenerationPostProcessTest, PostProcess_test_002, TestSize.Level0)
{
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY));
    ThumbnailData data;
    data.rdbUpdateCache = values;
    data.id = dataId;
    ThumbRdbOpt opts;
    opts.store = store;
    opts.table = PhotoColumn::PHOTOS_TABLE;

    int ret = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    EXPECT_EQ(ret, E_OK);
}

HWTEST_F(MediaLibraryThumbnailGenerationPostProcessTest, PostProcess_test_003, TestSize.Level0)
{
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY));
    ThumbnailData data;
    data.rdbUpdateCache = values;
    data.id = "1231";
    ThumbRdbOpt opts;
    opts.store = store;
    opts.table = PhotoColumn::PHOTOS_TABLE;

    int ret = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    EXPECT_EQ(ret, E_RDB_UPDATE_NO_ROWS_CHANGED);
}

HWTEST_F(MediaLibraryThumbnailGenerationPostProcessTest, PostProcess_test_004, TestSize.Level0)
{
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_LATER));
    ThumbnailData data;
    data.rdbUpdateCache = values;
    data.id = "1231";
    ThumbRdbOpt opts;
    opts.store = store;
    opts.table = PhotoColumn::PHOTOS_TABLE;

    int ret = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    EXPECT_EQ(ret, E_RDB_QUERY_NO_RES);
}

} // namespace Media
} // namespace OHOS