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

#define MLOG_TAG "DfxPhotoErrorProcessorTest"

#include <gtest/gtest.h>
#include <string>

#include "dfx_photo_error_dao.h"
#include "dfx_photo_error_helper.h"
#include "dfx_photo_error_processor.h"
#include "dfx_reporter.h"
#include "media_column.h"
#include "media_log.h"
#include "media_time_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "mock_medialibrary_subscriber.h"
#include "result_set_utils.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

using namespace testing::ext;

namespace OHOS::Media {
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static int64_t g_seq = 0;

static int32_t ClearTable(const std::string &table)
{
    NativeRdb::RdbPredicates predicates(table);
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    EXPECT_EQ(err, E_OK);
    return E_OK;
}

static int32_t InsertPhoto(const PhotoErrorRow &row)
{
    EXPECT_NE(g_rdbStore, nullptr);
    int64_t ts = MediaTimeUtils::UTCTimeMilliSeconds();
    std::string title = "DFXPE_" + std::to_string(++g_seq);
    std::string displayName = title + ".jpg";
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, row.data);
    values.PutString(MediaColumn::MEDIA_TITLE, title);
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, row.mediaType);
    values.PutLong(MediaColumn::MEDIA_SIZE, row.size);
    values.PutLong(MediaColumn::MEDIA_DATE_ADDED, ts);
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    values.PutInt(PhotoColumn::PHOTO_POSITION, row.position);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, row.subtype);
    values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, row.storagePath);
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, row.fileSourceType);
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, row.southDeviceType);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, 0);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);
    int64_t rowId = -1;
    int32_t ret = g_rdbStore->Insert(rowId, PhotoColumn::PHOTOS_TABLE, values);
    EXPECT_EQ(ret, E_OK);
    return ret;
}
} // namespace

class DfxPhotoErrorProcessorTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        MediaLibraryUnitTestUtils::Init();
        g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        EXPECT_NE(g_rdbStore, nullptr);
    }
    static void TearDownTestCase(void)
    {
        ClearTable(PhotoColumn::PHOTOS_TABLE);
    }
    void SetUp()
    {
        ClearTable(PhotoColumn::PHOTOS_TABLE);
    }
    void TearDown()
    {
        ClearTable(PhotoColumn::PHOTOS_TABLE);
        test::ResetSubscriberMock();
    }
};

HWTEST_F(DfxPhotoErrorProcessorTest, empty_scan_returns_no_batches, TestSize.Level0)
{
    DfxPhotoErrorProcessor processor;
    auto batches = processor.GetPhotoErrorBatches(500, 5);
    EXPECT_EQ(batches.size(), 0u);
}

HWTEST_F(DfxPhotoErrorProcessorTest, missing_file_counted_as_not_exist, TestSize.Level0)
{
    PhotoErrorRow row{};
    row.data = "/nonexistent/dfx_processor_test/photo.jpg";
    row.mediaType = MEDIA_TYPE_IMAGE;
    row.position = 1;
    row.size = 100;
    InsertPhoto(row);

    DfxPhotoErrorProcessor processor;
    auto batches = processor.GetPhotoErrorBatches(500, 5);
    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].photoErrorTypes.size(), 1u);
    ASSERT_EQ(batches[0].photoErrorCounts.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorCounts[0], 1);
    EXPECT_EQ(batches[0].photoErrorTypes[0] % MEDIA_TYPE_BASE,
        static_cast<int32_t>(PhotoErrorType::FILE_NOT_EXIST));
}

HWTEST_F(DfxPhotoErrorProcessorTest, empty_resolved_path_row_excluded, TestSize.Level0)
{
    PhotoErrorRow row{};
    row.data = "/d/a.jpg";
    row.fileSourceType = 1;
    row.mediaType = MEDIA_TYPE_IMAGE;
    row.position = 1;
    row.size = 100;
    InsertPhoto(row);

    DfxPhotoErrorProcessor processor;
    auto batches = processor.GetPhotoErrorBatches(500, 5);
    EXPECT_EQ(batches.size(), 0u);
}

HWTEST_F(DfxPhotoErrorProcessorTest, same_dimension_rows_accumulate_count, TestSize.Level0)
{
    PhotoErrorRow row1{};
    row1.data = "/nonexistent/dfx_processor_test/a.jpg";
    row1.mediaType = MEDIA_TYPE_IMAGE;
    row1.position = 1;
    row1.size = 100;
    InsertPhoto(row1);

    PhotoErrorRow row2{};
    row2.data = "/nonexistent/dfx_processor_test/b.jpg";
    row2.mediaType = MEDIA_TYPE_IMAGE;
    row2.position = 1;
    row2.size = 200;
    InsertPhoto(row2);

    DfxPhotoErrorProcessor processor;
    auto batches = processor.GetPhotoErrorBatches(500, 5);
    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].photoErrorTypes.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorCounts[0], 2);
}

HWTEST_F(DfxPhotoErrorProcessorTest, interruption_discards_partial_stat, TestSize.Level0)
{
    PhotoErrorRow row1{};
    row1.data = "/nonexistent/dfx_processor_test/interrupt_a.jpg";
    row1.mediaType = MEDIA_TYPE_IMAGE;
    row1.position = 1;
    row1.size = 100;
    InsertPhoto(row1);

    PhotoErrorRow row2{};
    row2.data = "/nonexistent/dfx_processor_test/interrupt_b.jpg";
    row2.mediaType = MEDIA_TYPE_IMAGE;
    row2.position = 1;
    row2.size = 200;
    InsertPhoto(row2);
    test::SetSubscriberTrueLimit(1);

    DfxPhotoErrorProcessor processor;
    auto batches = processor.GetPhotoErrorBatches(500, 5);
    EXPECT_EQ(batches.size(), 0u);

    test::ResetSubscriberMock();
}

} // namespace OHOS::Media
