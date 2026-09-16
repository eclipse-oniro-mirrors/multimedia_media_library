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

#define MLOG_TAG "OCRRestoreTest"

#include "ocr_restore_test.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "backup_const.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "gallery_source.h"
#include "ocr_restore.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {

const std::string TEST_OCR_BACKUP_PATH = "/data/test/gallery_ocr.db";
const std::string TEST_TASK_ID = "1234567890";
const int32_t TEST_SCENE_CODE = 1;

// gallery_media rows for OCR join, _id starts above seed data range (1-37)
const int32_t OCR_MEDIA_ID_BASE = 1000;
const int32_t TEST_FILE_ID_OLD = 1001;      // gallery_media._id, local photo
const int32_t TEST_FILE_ID_OLD_THIRD = 1003;
const int32_t TEST_FILE_ID_NEW = 10001;      // destination file_id
const int32_t TEST_FILE_ID_NEW_CLOUD = 10002;

// exit codes defined in ocr_restore.cpp, mirror the values here
const int32_t EXIT_CODE_MIDDLE = 1;
const int32_t EXIT_CODE_BEGIN = 2;
const int32_t EXIT_CODE_INIT = -1;

// time budget constants defined in ocr_restore.cpp, mirror the values here
const int64_t TEST_THRESHOLD_DATA_TIME = 600000;
const int64_t TEST_SINGLE_OVER_THRESHOLD_DATA_TIME = 216000;
const int64_t TEST_THRESHOLD_DATA_SIZE = 30000;

// Fixed backup start time for deterministic time budget tests: 1000000000s = 2001-09-09 UTC, far in the past.
// taskId semantic is the backup start timestamp in seconds. Expected values below derive from
// TEST_BACKUP_START_MS directly (not via std::stoll of the taskId) so the sec-to-ms conversion is verified too.
const int64_t TEST_BACKUP_START_SEC = 1000000000;
const int64_t TEST_BACKUP_START_MS = TEST_BACKUP_START_SEC * 1000;

const std::string TEST_HASH_FIRST = "ocr_hash_first";
const std::string TEST_HASH_SECOND = "ocr_hash_second";
const std::string TEST_HASH_THIRD = "ocr_hash_third";
const std::string TEST_OCR_TEXT = "ocr_text_first";
const std::string TEST_OCR_TEXT_SECOND = "ocr_text_second";
const std::string TEST_OCR_TEXT_THIRD = "ocr_text_third";
const int32_t TEST_OCR_VERSION = 1;
const int32_t TEST_OCR_WIDTH = 100;
const int32_t TEST_OCR_HEIGHT = 200;

// gallery_media column values for OCR test rows, must satisfy the restore where clause
const int32_t TEST_MEDIA_SIZE = 10240;
const int32_t TEST_MEDIA_TYPE_IMAGE = 1;
const int32_t TEST_STORAGE_ID_MAIN = 65537;
const int32_t TEST_MEDIA_WIDTH = 3968;
const int32_t TEST_MEDIA_HEIGHT = 2976;
const int32_t TEST_RECYCLE_FLAG_NORMAL = 0;
const int32_t TEST_LOCAL_MEDIA_ID_FIRST = 101;
const int32_t TEST_LOCAL_MEDIA_ID_THIRD = 103;

// content of pre-inserted tab_analysis_ocr rows; only row existence matters, values are arbitrary
const std::string TEST_PREEXIST_OCR_TEXT = "exist";
const std::string TEST_PREEXIST_OCR_VERSION = "backup1";
const int32_t TEST_PREEXIST_OCR_DIM = 1;

// synthesized millisecond timestamps for CheckBatchTimeout, only relative order matters
const int64_t TEST_SYNC_START_MS = 500;
const int64_t TEST_SYNC_SHOULD_END_MS = 1000;
const int64_t TEST_SYNC_AFTER_END_MS = 2000;
// 60s headroom, far beyond any single-batch test duration
const int64_t TEST_TIMEOUT_HEADROOM_MS = 60000;

// expected budget units for over-threshold cases, ceil(size / 10000)
const int64_t TEST_EXPECTED_UNITS_JUST_OVER = 4;  // ceil(30001 / 10000)
const int64_t TEST_EXPECTED_UNITS_LARGE = 5;      // ceil(50000 / 10000)
const int32_t TEST_LARGE_DATA_SIZE = 50000;

// data profile test constants
const std::string TEST_HASH_ORPHAN = "ocr_hash_orphan";  // no matching gallery_media row
const std::string TEST_HASH_DUP = "ocr_hash_dup";        // duplicated in t_ocr_result
const std::string TEST_OCR_TEXT_DUP = "ocr_text_dup";
const int64_t TEST_PROFILE_DUP_ROWS = 3;                 // rows sharing TEST_HASH_DUP
const int64_t TEST_PROFILE_MAX_DUP = 3;                  // max duplication of TEST_HASH_DUP
const int64_t TEST_PROFILE_TWO_HASHES = 2;               // distinct valid hashes in normal case
const int64_t TEST_PROFILE_EMPTY_HASH_ROWS = 2;          // one NULL hash + one '' hash
const int64_t TEST_PROFILE_ORPHAN_ROWS = 1;

static std::shared_ptr<MediaLibraryRdbStore> g_ocrRdbStore = nullptr;
static std::shared_ptr<NativeRdb::RdbStore> g_ocrGalleryPtr = nullptr;

static void ExecuteSqls(shared_ptr<RdbStore> rdbStore, const vector<string> &sqls)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("ExecuteSqls skipped: rdbStore is nullptr");
        return;
    }
    for (const auto &sql : sqls) {
        int32_t errCode = rdbStore->ExecuteSql(sql);
        if (errCode == E_OK) {
            continue;
        }
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", sql.c_str(), errCode);
    }
}

static void ClearData()
{
    MEDIA_INFO_LOG("Start clear data");
    if (g_ocrRdbStore != nullptr && g_ocrRdbStore->GetRaw() != nullptr) {
        ExecuteSqls(g_ocrRdbStore->GetRaw(), {
            "DELETE FROM tab_analysis_ocr",
            "DELETE FROM tab_analysis_total",
        });
    }
    if (g_ocrGalleryPtr != nullptr) {
        ExecuteSqls(g_ocrGalleryPtr, {
            "DELETE FROM t_ocr_result",
            "DELETE FROM gallery_media WHERE _id >= " + std::to_string(OCR_MEDIA_ID_BASE),
        });
    }
    MEDIA_INFO_LOG("End clear data");
}

// insert one gallery_media row with explicit hash for the OCR join
static void InsertGalleryMediaRow(int32_t id, int32_t localMediaId, const std::string &hash)
{
    std::string sql = "INSERT INTO gallery_media (_id, local_media_id, _data, _size, media_type, storage_id, "
        "width, height, relative_bucket_id, recycleFlag, hash) VALUES (" +
        std::to_string(id) + ", " + std::to_string(localMediaId) +
        ", '/storage/emulated/0/DCIM/Camera/ocr_" + std::to_string(id) + ".jpg', " +
        std::to_string(TEST_MEDIA_SIZE) + ", " + std::to_string(TEST_MEDIA_TYPE_IMAGE) + ", " +
        std::to_string(TEST_STORAGE_ID_MAIN) + ", " + std::to_string(TEST_MEDIA_WIDTH) + ", " +
        std::to_string(TEST_MEDIA_HEIGHT) + ", NULL, " + std::to_string(TEST_RECYCLE_FLAG_NORMAL) +
        ", '" + hash + "')";
    ExecuteSqls(g_ocrGalleryPtr, { sql });
}

static void InsertOcrResult(const std::string &hash, const std::string &ocrText, int32_t version)
{
    std::string sql = "INSERT INTO t_ocr_result (hash, ocr_text, version_ocr, width, height) VALUES ('" +
        hash + "', '" + ocrText + "', " + std::to_string(version) + ", " +
        std::to_string(TEST_OCR_WIDTH) + ", " + std::to_string(TEST_OCR_HEIGHT) + ")";
    ExecuteSqls(g_ocrGalleryPtr, { sql });
}

// insert one t_ocr_result row with raw hash/text SQL expressions, for NULL/'' anomaly rows
static void InsertOcrResultRaw(const std::string &hashExpr, const std::string &textExpr)
{
    std::string sql = "INSERT INTO t_ocr_result (hash, ocr_text, version_ocr, width, height) VALUES (" +
        hashExpr + ", " + textExpr + ", " + std::to_string(TEST_OCR_VERSION) + ", " +
        std::to_string(TEST_OCR_WIDTH) + ", " + std::to_string(TEST_OCR_HEIGHT) + ")";
    ExecuteSqls(g_ocrGalleryPtr, { sql });
}

static void InsertAnalysisOcr(int32_t fileId)
{
    if (g_ocrRdbStore == nullptr || g_ocrRdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("InsertAnalysisOcr skipped: destination rdb is not ready");
        return;
    }
    std::string sql = "INSERT INTO tab_analysis_ocr (file_id, ocr_text, ocr_version, width, height) VALUES (" +
        std::to_string(fileId) + ", '" + TEST_PREEXIST_OCR_TEXT + "', '" + TEST_PREEXIST_OCR_VERSION +
        "', " + std::to_string(TEST_PREEXIST_OCR_DIM) + ", " + std::to_string(TEST_PREEXIST_OCR_DIM) + ")";
    ExecuteSqls(g_ocrRdbStore->GetRaw(), { sql });
}

static int32_t QueryDestinationInt(const std::string &sql, const std::string &column)
{
    if (g_ocrRdbStore == nullptr || g_ocrRdbStore->GetRaw() == nullptr) {
        MEDIA_ERR_LOG("QueryDestinationInt skipped: destination rdb is not ready");
        return -1;
    }
    return BackupDatabaseUtils::QueryInt(g_ocrRdbStore->GetRaw(), sql, column);
}

static std::unordered_map<int32_t, PhotoInfo> BuildPhotoInfoMap(const std::vector<int32_t> &oldIds,
    const std::vector<int32_t> &newIds)
{
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    for (size_t i = 0; i < oldIds.size() && i < newIds.size(); i++) {
        PhotoInfo photoInfo;
        photoInfo.fileIdNew = newIds[i];
        photoInfoMap[oldIds[i]] = photoInfo;
    }
    return photoInfoMap;
}

void OCRRestoreTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    ASSERT_TRUE(MediaLibraryUnitTestUtils::IsValid());
    g_ocrRdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_ocrRdbStore, nullptr);
    ASSERT_NE(g_ocrRdbStore->GetRaw(), nullptr);
    // ensure vision tables exist in destination db, keep same schema as upgrade_vision_sqls.h
    ExecuteSqls(g_ocrRdbStore->GetRaw(), {
        "CREATE TABLE IF NOT EXISTS tab_analysis_ocr (id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "file_id INT UNIQUE, ocr_text TEXT, ocr_version TEXT, ocr_text_msg TEXT, "
        "width INT, height INT, analysis_version TEXT)",
        "CREATE TABLE IF NOT EXISTS tab_analysis_total (id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "file_id INT UNIQUE, status INT, ocr INT, label INT, aesthetics_score INT)",
    });
    // ensure the parent directory exists before creating the gallery source db
    const std::string galleryDir = MediaFileUtils::GetParentPath(TEST_OCR_BACKUP_PATH);
    if (!MediaFileUtils::IsFileExists(galleryDir) &&
        !MediaFileUtils::CreateDirectory(galleryDir)) {
        MEDIA_ERR_LOG("Gallery source dir %{public}s create failed", galleryDir.c_str());
        ASSERT_TRUE(false);
        return;
    }
    GallerySource gallerySource;
    gallerySource.Init(TEST_OCR_BACKUP_PATH);
    g_ocrGalleryPtr = gallerySource.galleryStorePtr_;
    ASSERT_NE(g_ocrGalleryPtr, nullptr);
    // ocr result table in gallery (source) db
    ExecuteSqls(g_ocrGalleryPtr, {
        "CREATE TABLE IF NOT EXISTS t_ocr_result (hash TEXT, ocr_text TEXT, "
        "version_ocr INTEGER, width INTEGER, height INTEGER)",
    });
    ClearData();
}

void OCRRestoreTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void OCRRestoreTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearData();
}

void OCRRestoreTest::TearDown()
{
    MEDIA_INFO_LOG("TearDown");
}

// ---------------- GetShouldEndTime: time budget added by this feature ----------------

// Test GetShouldEndTime with empty taskId
HWTEST_F(OCRRestoreTest, GetShouldEndTime_EmptyTaskId_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_EmptyTaskId_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = "";
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), 0);
    MEDIA_INFO_LOG("GetShouldEndTime_EmptyTaskId_Test end");
}

// Test GetShouldEndTime with non-numeric taskId
HWTEST_F(OCRRestoreTest, GetShouldEndTime_NonNumericTaskId_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_NonNumericTaskId_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = "12a34";
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), 0);
    MEDIA_INFO_LOG("GetShouldEndTime_NonNumericTaskId_Test end");
}

// Test GetShouldEndTime with empty photo info map, baseline budget
HWTEST_F(OCRRestoreTest, GetShouldEndTime_EmptyMap_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_EmptyMap_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = std::to_string(TEST_BACKUP_START_SEC);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    int64_t expected = TEST_BACKUP_START_MS + TEST_THRESHOLD_DATA_TIME;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), expected);
    MEDIA_INFO_LOG("GetShouldEndTime_EmptyMap_Test end");
}

// Test GetShouldEndTime with data size below threshold, baseline budget
HWTEST_F(OCRRestoreTest, GetShouldEndTime_BelowThreshold_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_BelowThreshold_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = std::to_string(TEST_BACKUP_START_SEC);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap = BuildPhotoInfoMap(
        { TEST_FILE_ID_OLD }, { TEST_FILE_ID_NEW });
    int64_t expected = TEST_BACKUP_START_MS + TEST_THRESHOLD_DATA_TIME;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), expected);
    MEDIA_INFO_LOG("GetShouldEndTime_BelowThreshold_Test end");
}

// Test GetShouldEndTime at threshold boundary (30000 is still baseline)
HWTEST_F(OCRRestoreTest, GetShouldEndTime_ThresholdBoundary_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_ThresholdBoundary_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = std::to_string(TEST_BACKUP_START_SEC);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    for (int i = 0; i < TEST_THRESHOLD_DATA_SIZE; i++) {
        PhotoInfo info;
        info.fileIdNew = i;
        photoInfoMap[i] = info;
    }
    int64_t expected = TEST_BACKUP_START_MS + TEST_THRESHOLD_DATA_TIME;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), expected);
    MEDIA_INFO_LOG("GetShouldEndTime_ThresholdBoundary_Test end");
}

// Test GetShouldEndTime just over threshold, budget grows by ceil(size/10000) * 216s
HWTEST_F(OCRRestoreTest, GetShouldEndTime_JustOverThreshold_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_JustOverThreshold_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = std::to_string(TEST_BACKUP_START_SEC);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    for (int i = 0; i < TEST_THRESHOLD_DATA_SIZE + 1; i++) {
        PhotoInfo info;
        info.fileIdNew = i;
        photoInfoMap[i] = info;
    }
    // (30001 + 9999) / 10000 = 4
    int64_t expected = TEST_BACKUP_START_MS + TEST_EXPECTED_UNITS_JUST_OVER *
        TEST_SINGLE_OVER_THRESHOLD_DATA_TIME;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), expected);
    MEDIA_INFO_LOG("GetShouldEndTime_JustOverThreshold_Test end");
}

// Test GetShouldEndTime with large data size
HWTEST_F(OCRRestoreTest, GetShouldEndTime_LargeDataSize_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShouldEndTime_LargeDataSize_Test start");
    OCRRestore ocrRestore;
    ocrRestore.taskId_ = std::to_string(TEST_BACKUP_START_SEC);
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    for (int i = 0; i < TEST_LARGE_DATA_SIZE; i++) {
        PhotoInfo info;
        info.fileIdNew = i;
        photoInfoMap[i] = info;
    }
    // (50000 + 9999) / 10000 = 5
    int64_t expected = TEST_BACKUP_START_MS + TEST_EXPECTED_UNITS_LARGE *
        TEST_SINGLE_OVER_THRESHOLD_DATA_TIME;
    EXPECT_EQ(ocrRestore.GetShouldEndTime(photoInfoMap), expected);
    MEDIA_INFO_LOG("GetShouldEndTime_LargeDataSize_Test end");
}

// ---------------- CheckBatchTimeout: timeout exit added by this feature ----------------

// Test CheckBatchTimeout when current time is within budget
HWTEST_F(OCRRestoreTest, CheckBatchTimeout_NotTimeout_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckBatchTimeout_NotTimeout_Test start");
    OCRRestore ocrRestore;
    ocrRestore.exitCode_ = EXIT_CODE_INIT;
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    EXPECT_TRUE(ocrRestore.CheckBatchTimeout(now, now + TEST_TIMEOUT_HEADROOM_MS, now, true));
    EXPECT_TRUE(ocrRestore.CheckBatchTimeout(now, now + TEST_TIMEOUT_HEADROOM_MS, now, false));
    EXPECT_EQ(ocrRestore.exitCode_, EXIT_CODE_INIT);
    MEDIA_INFO_LOG("CheckBatchTimeout_NotTimeout_Test end");
}

// Test CheckBatchTimeout at exact boundary, equal time is not timeout
HWTEST_F(OCRRestoreTest, CheckBatchTimeout_BoundaryEqual_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckBatchTimeout_BoundaryEqual_Test start");
    OCRRestore ocrRestore;
    ocrRestore.exitCode_ = EXIT_CODE_INIT;
    int64_t now = TEST_SYNC_SHOULD_END_MS;
    EXPECT_TRUE(ocrRestore.CheckBatchTimeout(now, now, TEST_SYNC_START_MS, true));
    EXPECT_EQ(ocrRestore.exitCode_, EXIT_CODE_INIT);
    MEDIA_INFO_LOG("CheckBatchTimeout_BoundaryEqual_Test end");
}

// Test CheckBatchTimeout timeout at first batch, exit code is BEGIN
HWTEST_F(OCRRestoreTest, CheckBatchTimeout_TimeoutFirstBatch_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckBatchTimeout_TimeoutFirstBatch_Test start");
    OCRRestore ocrRestore;
    ocrRestore.exitCode_ = EXIT_CODE_INIT;
    EXPECT_FALSE(ocrRestore.CheckBatchTimeout(TEST_SYNC_AFTER_END_MS, TEST_SYNC_SHOULD_END_MS,
        TEST_SYNC_START_MS, true));
    EXPECT_EQ(ocrRestore.exitCode_, EXIT_CODE_BEGIN);
    MEDIA_INFO_LOG("CheckBatchTimeout_TimeoutFirstBatch_Test end");
}

// Test CheckBatchTimeout timeout at middle batch, exit code is MIDDLE
HWTEST_F(OCRRestoreTest, CheckBatchTimeout_TimeoutMiddle_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckBatchTimeout_TimeoutMiddle_Test start");
    OCRRestore ocrRestore;
    ocrRestore.exitCode_ = EXIT_CODE_INIT;
    EXPECT_FALSE(ocrRestore.CheckBatchTimeout(TEST_SYNC_AFTER_END_MS, TEST_SYNC_SHOULD_END_MS,
        TEST_SYNC_START_MS, false));
    EXPECT_EQ(ocrRestore.exitCode_, EXIT_CODE_MIDDLE);
    MEDIA_INFO_LOG("CheckBatchTimeout_TimeoutMiddle_Test end");
}

// ---------------- Statistics queries added by this feature ----------------

// Test GetTotalGalleryOcrRecords with null gallery rdb
HWTEST_F(OCRRestoreTest, GetTotalGalleryOcrRecords_NullRdb_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_NullRdb_Test start");
    OCRRestore ocrRestore;
    ocrRestore.galleryRdb_ = nullptr;
    EXPECT_EQ(ocrRestore.GetTotalGalleryOcrRecords(), 0);
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_NullRdb_Test end");
}

// Test GetTotalGalleryOcrRecords with empty table
HWTEST_F(OCRRestoreTest, GetTotalGalleryOcrRecords_EmptyTable_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_EmptyTable_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    EXPECT_EQ(ocrRestore.GetTotalGalleryOcrRecords(), 0);
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_EmptyTable_Test end");
}

// Test GetTotalGalleryOcrRecords with records
HWTEST_F(OCRRestoreTest, GetTotalGalleryOcrRecords_WithRecords_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_WithRecords_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    InsertOcrResult(TEST_HASH_FIRST, TEST_OCR_TEXT, TEST_OCR_VERSION);
    InsertOcrResult(TEST_HASH_SECOND, TEST_OCR_TEXT_SECOND, TEST_OCR_VERSION);
    InsertOcrResult(TEST_HASH_THIRD, TEST_OCR_TEXT_THIRD, TEST_OCR_VERSION);
    EXPECT_EQ(ocrRestore.GetTotalGalleryOcrRecords(), 3);
    MEDIA_INFO_LOG("GetTotalGalleryOcrRecords_WithRecords_Test end");
}

// Test GetMaxId returns max rowid of tab_analysis_ocr
HWTEST_F(OCRRestoreTest, GetMaxId_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetMaxId_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    InsertAnalysisOcr(TEST_FILE_ID_NEW);
    InsertAnalysisOcr(TEST_FILE_ID_NEW_CLOUD);
    ocrRestore.GetMaxId();
    EXPECT_GE(ocrRestore.maxId_, 2);
    MEDIA_INFO_LOG("GetMaxId_Test end");
}

// Test BatchInsertAndReport with empty values, counters unchanged
HWTEST_F(OCRRestoreTest, BatchInsertAndReport_EmptyValues_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("BatchInsertAndReport_EmptyValues_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    std::vector<NativeRdb::ValuesBucket> values;
    ocrRestore.BatchInsertAndReport(values);
    EXPECT_EQ(ocrRestore.successCnt_, 0);
    EXPECT_EQ(ocrRestore.failCnt_, 0);
    MEDIA_INFO_LOG("BatchInsertAndReport_EmptyValues_Test end");
}

// Test repeated insert with the same file_id keeps REPLACE idempotency and accumulates counters
HWTEST_F(OCRRestoreTest, BatchInsertAndReport_RepeatedInsert_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("BatchInsertAndReport_RepeatedInsert_Test start");
    // pre-existing row with the same unique file_id, simulates a recovery retry
    InsertAnalysisOcr(TEST_FILE_ID_NEW);
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);

    GalleryOCRInfo ocrInfo;
    ocrInfo.photoInfo.fileIdNew = TEST_FILE_ID_NEW;
    ocrInfo.ocrText = TEST_OCR_TEXT;
    ocrInfo.ocrVersion = TEST_OCR_VERSION;
    ocrInfo.width = TEST_OCR_WIDTH;
    ocrInfo.height = TEST_OCR_HEIGHT;
    std::vector<NativeRdb::ValuesBucket> values;
    ocrRestore.UpdateOcrInsertValues(values, ocrInfo);
    EXPECT_EQ(values.size(), 1);

    // first insert overwrites the pre-existing row via REPLACE conflict resolution
    ocrRestore.BatchInsertAndReport(values);
    EXPECT_EQ(ocrRestore.successCnt_, 1);
    EXPECT_EQ(ocrRestore.failCnt_, 0);
    // second insert with the same file_id replaces again, counters keep accumulating
    ocrRestore.BatchInsertAndReport(values);
    EXPECT_EQ(ocrRestore.successCnt_, 2);
    EXPECT_EQ(ocrRestore.failCnt_, 0);
    // replace semantics keeps exactly one row, no duplicates
    EXPECT_EQ(QueryDestinationInt("SELECT count(1) FROM tab_analysis_ocr", "count(1)"), 1);
    MEDIA_INFO_LOG("BatchInsertAndReport_RepeatedInsert_Test end");
}

// ---------------- RestoreOCR: timeout exit and counter behavior added by this feature ----------------

// Test RestoreOCR exits with BEGIN exit code and zero insert on first-batch timeout
HWTEST_F(OCRRestoreTest, RestoreOCR_Timeout_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("RestoreOCR_Timeout_Test start");
    InsertGalleryMediaRow(TEST_FILE_ID_OLD, TEST_LOCAL_MEDIA_ID_FIRST, TEST_HASH_FIRST);
    InsertOcrResult(TEST_HASH_FIRST, TEST_OCR_TEXT, TEST_OCR_VERSION);
    auto photoInfoMap = BuildPhotoInfoMap({ TEST_FILE_ID_OLD }, { TEST_FILE_ID_NEW });

    OCRRestore ocrRestore;
    // a far-in-the-past backup start time makes shouldEndTime already exceeded
    ocrRestore.Init(TEST_SCENE_CODE, std::to_string(TEST_BACKUP_START_SEC), g_ocrRdbStore->GetRaw(),
        g_ocrGalleryPtr);
    ocrRestore.RestoreOCR(photoInfoMap, false);

    EXPECT_EQ(ocrRestore.successCnt_, 0);
    EXPECT_EQ(ocrRestore.failCnt_, 0);
    EXPECT_EQ(ocrRestore.exitCode_, EXIT_CODE_BEGIN);
    // timeout exit happens before any row is inserted
    EXPECT_EQ(QueryDestinationInt("SELECT count(1) FROM tab_analysis_ocr", "count(1)"), 0);
    MEDIA_INFO_LOG("RestoreOCR_Timeout_Test end");
}

// ---------------- CollectOcrDataProfile: source db data quality profile added by this feature ----------------

// Test profile on an empty t_ocr_result table, all counters stay zero (COALESCE keeps SUM/MAX at 0)
HWTEST_F(OCRRestoreTest, CollectOcrDataProfile_EmptyTable_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CollectOcrDataProfile_EmptyTable_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    ocrRestore.CollectOcrDataProfile();

    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrTotal, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrValidHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.distinctOcrValidHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.matchedHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashEmpty, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.textEmpty, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.duplicateHashCount, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.maxDuplicateTimes, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashNotExist, 0);
    // gallery_media keeps seed rows, only assert it is not below the seed baseline
    EXPECT_GE(ocrRestore.ocrDataProfile_.mediaTotal, 0);
    MEDIA_INFO_LOG("CollectOcrDataProfile_EmptyTable_Test end");
}

// Test profile with two valid hashes matched to gallery_media rows
HWTEST_F(OCRRestoreTest, CollectOcrDataProfile_NormalData_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CollectOcrDataProfile_NormalData_Test start");
    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    // capture the media baseline first, seed rows in gallery_media are out of test control
    ocrRestore.CollectOcrDataProfile();
    int64_t mediaTotalBase = ocrRestore.ocrDataProfile_.mediaTotal;
    int64_t mediaValidHashBase = ocrRestore.ocrDataProfile_.mediaValidHash;
    int64_t distinctMediaValidHashBase = ocrRestore.ocrDataProfile_.distinctMediaValidHash;

    InsertGalleryMediaRow(TEST_FILE_ID_OLD, TEST_LOCAL_MEDIA_ID_FIRST, TEST_HASH_FIRST);
    InsertGalleryMediaRow(TEST_FILE_ID_OLD_THIRD, TEST_LOCAL_MEDIA_ID_THIRD, TEST_HASH_THIRD);
    InsertOcrResult(TEST_HASH_FIRST, TEST_OCR_TEXT, TEST_OCR_VERSION);
    InsertOcrResult(TEST_HASH_THIRD, TEST_OCR_TEXT_THIRD, TEST_OCR_VERSION);
    ocrRestore.CollectOcrDataProfile();

    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrTotal, TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrValidHash, TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.distinctOcrValidHash, TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.matchedHash, TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashEmpty, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.textEmpty, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.duplicateHashCount, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.maxDuplicateTimes, 1);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashNotExist, 0);
    // media counters grow by the two inserted rows, seed rows have NULL hash and stay out
    EXPECT_EQ(ocrRestore.ocrDataProfile_.mediaTotal, mediaTotalBase + TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.mediaValidHash, mediaValidHashBase + TEST_PROFILE_TWO_HASHES);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.distinctMediaValidHash,
        distinctMediaValidHashBase + TEST_PROFILE_TWO_HASHES);
    MEDIA_INFO_LOG("CollectOcrDataProfile_NormalData_Test end");
}

// Test profile counts duplicated hash groups and the max duplication
HWTEST_F(OCRRestoreTest, CollectOcrDataProfile_DuplicateHash_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CollectOcrDataProfile_DuplicateHash_Test start");
    InsertGalleryMediaRow(TEST_FILE_ID_OLD, TEST_LOCAL_MEDIA_ID_FIRST, TEST_HASH_DUP);
    InsertOcrResult(TEST_HASH_DUP, TEST_OCR_TEXT_DUP, TEST_OCR_VERSION);
    InsertOcrResult(TEST_HASH_DUP, TEST_OCR_TEXT_DUP, TEST_OCR_VERSION);
    InsertOcrResult(TEST_HASH_DUP, TEST_OCR_TEXT_DUP, TEST_OCR_VERSION);

    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    ocrRestore.CollectOcrDataProfile();

    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrTotal, TEST_PROFILE_DUP_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrValidHash, TEST_PROFILE_DUP_ROWS);
    // three rows share one hash
    EXPECT_EQ(ocrRestore.ocrDataProfile_.distinctOcrValidHash, 1);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.matchedHash, 1);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.duplicateHashCount, 1);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.maxDuplicateTimes, TEST_PROFILE_MAX_DUP);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashNotExist, 0);
    MEDIA_INFO_LOG("CollectOcrDataProfile_DuplicateHash_Test end");
}

// Test profile counts orphan hashes that have no matching gallery_media row
HWTEST_F(OCRRestoreTest, CollectOcrDataProfile_OrphanHash_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CollectOcrDataProfile_OrphanHash_Test start");
    InsertOcrResult(TEST_HASH_ORPHAN, TEST_OCR_TEXT, TEST_OCR_VERSION);

    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    ocrRestore.CollectOcrDataProfile();

    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrTotal, TEST_PROFILE_ORPHAN_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrValidHash, TEST_PROFILE_ORPHAN_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashNotExist, TEST_PROFILE_ORPHAN_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.matchedHash, 0);
    MEDIA_INFO_LOG("CollectOcrDataProfile_OrphanHash_Test end");
}

// Test profile counts NULL/'' hash rows and empty ocr_text rows, they stay out of valid hash counters
HWTEST_F(OCRRestoreTest, CollectOcrDataProfile_EmptyHash_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("CollectOcrDataProfile_EmptyHash_Test start");
    // one NULL hash row with normal text, one '' hash row with empty text
    InsertOcrResultRaw("NULL", "'" + TEST_OCR_TEXT + "'");
    InsertOcrResultRaw("''", "''");

    OCRRestore ocrRestore;
    ocrRestore.Init(TEST_SCENE_CODE, TEST_TASK_ID, g_ocrRdbStore->GetRaw(), g_ocrGalleryPtr);
    ocrRestore.CollectOcrDataProfile();

    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrTotal, TEST_PROFILE_EMPTY_HASH_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashEmpty, TEST_PROFILE_EMPTY_HASH_ROWS);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.textEmpty, 1);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.ocrValidHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.distinctOcrValidHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.matchedHash, 0);
    EXPECT_EQ(ocrRestore.ocrDataProfile_.hashNotExist, 0);
    MEDIA_INFO_LOG("CollectOcrDataProfile_EmptyHash_Test end");
}
} // namespace OHOS::Media
