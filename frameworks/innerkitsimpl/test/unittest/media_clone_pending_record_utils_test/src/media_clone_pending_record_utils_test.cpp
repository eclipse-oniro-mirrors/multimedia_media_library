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

#include "media_clone_pending_record_utils_test.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

#include "media_clone_pending_record_utils.h"
#include "media_clone_pending_task.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_fusion_utils.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"

using namespace testing::ext;

namespace OHOS::Media::Background {
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::string TEST_ROOT_DIR = "/data/test/clone_pending_record_utils_test";
const std::string CLONE_PENDING_EVENT = "/data/storage/el2/base/preferences/clone_pending_events.xml";
const std::string CLONE_PENDING_BUCKET_PREFIX = "clone_pending_bucket_";
constexpr int32_t CLONE_PENDING_MULTI_BUCKET_COUNT = 16;
constexpr int64_t TEST_MEDIA_DATE_TAKEN_MS = 1000;

class MockAlwaysAcceptClonePendingTask : public MediaClonePendingTask {
public:
    bool Accept() override
    {
        return true;
    }
};

std::string BuildBucketKeyForTest(int32_t bucketIndex)
{
    return CLONE_PENDING_BUCKET_PREFIX + std::to_string(bucketIndex);
}

int32_t GetBucketIndexForTest(int32_t fileId)
{
    if (fileId <= 0) {
        return 0;
    }
    return fileId % CLONE_PENDING_MULTI_BUCKET_COUNT;
}

using BucketPendingEntries = std::map<int32_t, int64_t>;

bool ParseInt32ForTest(const std::string &text, int32_t &value)
{
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

bool ParseInt64ForTest(const std::string &text, int64_t &value)
{
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

BucketPendingEntries ParsePendingEntriesForTest(const std::string &csv)
{
    BucketPendingEntries entries;
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            continue;
        }
        int32_t fileId = -1;
        int64_t touchTs = 0;
        size_t pos = token.find('_');
        if (pos == std::string::npos) {
            if (ParseInt32ForTest(token, fileId) && fileId > 0) {
                entries[fileId] = 0;
            }
            continue;
        }
        std::string fileIdText = token.substr(0, pos);
        std::string touchTsText = token.substr(pos + 1);
        if (!ParseInt32ForTest(fileIdText, fileId) || fileId <= 0 || !ParseInt64ForTest(touchTsText, touchTs)) {
            continue;
        }
        entries[fileId] = touchTs;
    }
    return entries;
}

std::string SerializePendingEntriesForTest(const BucketPendingEntries &entries)
{
    std::string csv;
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it != entries.begin()) {
            csv += ",";
        }
        csv += std::to_string(it->first) + "_" + std::to_string(it->second);
    }
    return csv;
}

bool RetryDeletePath(const std::string &path)
{
    if (path.empty()) {
        return true;
    }
    if (!MediaFileUtils::IsFileExists(path)) {
        return true;
    }
    if (MediaFileUtils::DeleteFile(path)) {
        return true;
    }
    return !MediaFileUtils::IsFileExists(path) || MediaFileUtils::DeleteFile(path);
}

bool RetryDeleteAssetRow(int32_t fileId)
{
    if (g_rdbStore == nullptr || fileId <= 0) {
        return false;
    }
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    int32_t deleteRows = 0;
    int32_t ret = g_rdbStore->Delete(deleteRows, predicates);
    if (ret == E_OK) {
        return true;
    }
    ret = g_rdbStore->Delete(deleteRows, predicates);
    return ret == E_OK;
}

void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
    };
    for (const auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        CHECK_AND_CONTINUE(ret == E_OK);
    }
}

void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
    };
    for (const auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        CHECK_AND_CONTINUE(ret == E_OK);
    }
}

bool IsDbValid()
{
    CHECK_AND_RETURN_RET_LOG(g_rdbStore != nullptr, false, "rdbStore is nullptr");
    auto resultSet = g_rdbStore->QuerySql("SELECT 1");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "DB precheck query failed");
    bool isValid = (resultSet->GoToFirstRow() == E_OK);
    resultSet->Close();
    return isValid;
}

int32_t InsertPendingPhoto(const std::string &path, int64_t timePending)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, TEST_MEDIA_DATE_TAKEN_MS);
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, timePending);
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);

    int64_t outRowId = -1;
    int32_t ret = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    if (ret != E_OK || outRowId <= 0 || outRowId > std::numeric_limits<int32_t>::max()) {
        return -1;
    }
    return static_cast<int32_t>(outRowId);
}

int64_t InsertCloneSourceAssetForRollback(const std::string &displayName, const std::string &filePath)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutString(MediaColumn::MEDIA_TITLE, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_IMAGE));
    values.PutString(MediaColumn::MEDIA_FILE_PATH, filePath);
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    values.PutInt(PhotoColumn::PHOTO_IS_TEMP, 0);
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutLong(MediaColumn::MEDIA_DATE_DELETED, 0);
    values.PutLong(MediaColumn::MEDIA_SIZE, 1);

    int64_t outRowId = -1;
    int32_t ret = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    return ret == E_OK ? outRowId : -1;
}

int32_t QueryCountBySql(const std::string &sql)
{
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    int32_t count = 0;
    resultSet->GetInt(0, count);
    resultSet->Close();
    return count;
}

bool ExistsPhotoById(int32_t fileId)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, std::to_string(fileId));
    auto resultSet = g_rdbStore->Query(predicates, { MediaColumn::MEDIA_ID });
    if (resultSet == nullptr) {
        return false;
    }
    bool exists = (resultSet->GoToFirstRow() == E_OK);
    resultSet->Close();
    return exists;
}

std::vector<int32_t> GetAllPendingFileIds()
{
    std::vector<int32_t> ids;
    int32_t bucketCount = ClonePendingRecordUtils::GetPendingBucketCount();
    for (int32_t bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex) {
        std::vector<int32_t> bucketIds = ClonePendingRecordUtils::GetPendingFileIdsByBucket(bucketIndex);
        ids.insert(ids.end(), bucketIds.begin(), bucketIds.end());
    }
    return ids;
}

bool HasPendingFileId(int32_t fileId)
{
    std::vector<int32_t> ids = GetAllPendingFileIds();
    return std::find(ids.begin(), ids.end(), fileId) != ids.end();
}

bool DeletePendingTouchForTest(int32_t fileId)
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(CLONE_PENDING_EVENT, errCode);
    if (prefs == nullptr || fileId <= 0) {
        return false;
    }

    int32_t bucketIndex = GetBucketIndexForTest(fileId);
    std::string bucketKey = BuildBucketKeyForTest(bucketIndex);
    auto entries = ParsePendingEntriesForTest(prefs->GetString(bucketKey, ""));
    auto it = entries.find(fileId);
    if (it == entries.end()) {
        return true;
    }
    it->second = 0;
    prefs->PutString(bucketKey, SerializePendingEntriesForTest(entries));
    return prefs->FlushSync();
}

bool SetPendingTouchForTest(int32_t fileId, int64_t ts)
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(CLONE_PENDING_EVENT, errCode);
    if (prefs == nullptr || fileId <= 0) {
        return false;
    }

    int32_t bucketIndex = GetBucketIndexForTest(fileId);
    std::string bucketKey = BuildBucketKeyForTest(bucketIndex);
    auto entries = ParsePendingEntriesForTest(prefs->GetString(bucketKey, ""));
    entries[fileId] = ts;
    prefs->PutString(bucketKey, SerializePendingEntriesForTest(entries));
    return prefs->FlushSync();
}

bool HasPendingTouchForTest(int32_t fileId)
{
    int32_t errCode = E_OK;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(CLONE_PENDING_EVENT, errCode);
    if (prefs == nullptr || fileId <= 0) {
        return false;
    }

    int32_t bucketIndex = GetBucketIndexForTest(fileId);
    std::string bucketKey = BuildBucketKeyForTest(bucketIndex);
    auto entries = ParsePendingEntriesForTest(prefs->GetString(bucketKey, ""));
    auto it = entries.find(fileId);
    return it != entries.end() && it->second > 0;
}

void CleanupTestAssets()
{
    CHECK_AND_RETURN(g_rdbStore != nullptr);
    auto resultSet = g_rdbStore->QuerySql(
        "SELECT file_id, data FROM Photos WHERE data LIKE '" + TEST_ROOT_DIR + "%'");
    std::vector<int32_t> fileIds;
    if (resultSet != nullptr) {
        while (resultSet->GoToNextRow() == E_OK) {
            int32_t fileId = MediaLibraryRdbStore::GetInt(resultSet, MediaColumn::MEDIA_ID);
            std::string path = MediaLibraryRdbStore::GetString(resultSet, MediaColumn::MEDIA_FILE_PATH);
            fileIds.push_back(fileId);
            RetryDeletePath(path);
            RetryDeleteAssetRow(fileId);
        }
        resultSet->Close();
    }

    if (!fileIds.empty()) {
        ClonePendingRecordUtils::RemovePendingFileIds(fileIds);
    }

    std::vector<int32_t> allPendingIds = GetAllPendingFileIds();
    if (!allPendingIds.empty()) {
        ClonePendingRecordUtils::RemovePendingFileIds(allPendingIds);
    }

    std::string clearCmd = "rm -rf " + TEST_ROOT_DIR;
    (void)system(clearCmd.c_str());
}

void CreateTestAssetFile(const std::string &path)
{
    std::string parent = MediaFileUtils::GetParentPath(path);
    CHECK_AND_RETURN(!parent.empty());
    CHECK_AND_RETURN(MediaFileUtils::CreateDirectory(parent));
    std::ofstream out(path);
    out << "clone_pending_record_utils_test";
    out.close();
}
} // namespace

void MediaClonePendingRecordUtilsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    CleanTestTables();
    SetTables();
    ASSERT_TRUE(IsDbValid());
}

void MediaClonePendingRecordUtilsTest::TearDownTestCase(void)
{
    CleanupTestAssets();
    CleanTestTables();
    SetTables();
}

void MediaClonePendingRecordUtilsTest::SetUp(void)
{
    ASSERT_TRUE(IsDbValid());
    CleanupTestAssets();
}

void MediaClonePendingRecordUtilsTest::TearDown(void)
{
    CleanupTestAssets();
}

HWTEST_F(MediaClonePendingRecordUtilsTest, AddAndGetPendingFileId_Deduplicate_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AddAndGetPendingFileId_Deduplicate_001 start");
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(1001));
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(1001));

    std::vector<int32_t> ids = GetAllPendingFileIds();
    size_t count = std::count(ids.begin(), ids.end(), 1001);
    EXPECT_EQ(count, 1);
    EXPECT_TRUE(ClonePendingRecordUtils::RemovePendingFileId(1001));
    MEDIA_INFO_LOG("AddAndGetPendingFileId_Deduplicate_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, RemovePendingFileIds_BatchDelete_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemovePendingFileIds_BatchDelete_001 start");
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(2001));
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(2002));
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(2003));

    ASSERT_TRUE(ClonePendingRecordUtils::RemovePendingFileIds({ 2001, 2003 }));
    std::vector<int32_t> ids = GetAllPendingFileIds();
    EXPECT_EQ(std::count(ids.begin(), ids.end(), 2001), 0);
    EXPECT_EQ(std::count(ids.begin(), ids.end(), 2003), 0);
    EXPECT_EQ(std::count(ids.begin(), ids.end(), 2002), 1);

    EXPECT_TRUE(ClonePendingRecordUtils::RemovePendingFileIds({ 2002 }));
    MEDIA_INFO_LOG("RemovePendingFileIds_BatchDelete_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, CleanupPendingAssetByFileId_DeletePendingRecord_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CleanupPendingAssetByFileId_DeletePendingRecord_001 start");
    std::string path = TEST_ROOT_DIR + "/pending_delete.jpg";
    CreateTestAssetFile(path);
    int32_t fileId = InsertPendingPhoto(path, -1);
    ASSERT_GT(fileId, 0);

    ASSERT_TRUE(ClonePendingRecordUtils::CleanupPendingAssetByFileId(g_rdbStore, fileId));
    EXPECT_FALSE(ExistsPhotoById(fileId));
    MEDIA_INFO_LOG("CleanupPendingAssetByFileId_DeletePendingRecord_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, CleanupPendingAssetByFileId_NotFoundAndSkipNonPending_001,
    TestSize.Level1)
{
    MEDIA_INFO_LOG("CleanupPendingAssetByFileId_NotFoundAndSkipNonPending_001 start");
    EXPECT_TRUE(ClonePendingRecordUtils::CleanupPendingAssetByFileId(g_rdbStore, 999999));

    std::string path = TEST_ROOT_DIR + "/non_pending_keep.jpg";
    CreateTestAssetFile(path);
    int32_t fileId = InsertPendingPhoto(path, 0);
    ASSERT_GT(fileId, 0);

    ASSERT_TRUE(ClonePendingRecordUtils::CleanupPendingAssetByFileId(g_rdbStore, fileId));
    EXPECT_TRUE(ExistsPhotoById(fileId));
    MEDIA_INFO_LOG("CleanupPendingAssetByFileId_NotFoundAndSkipNonPending_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, MediaClonePendingTask_ExecuteCleanupPendingAsset_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaClonePendingTask_ExecuteCleanupPendingAsset_001 start");

    std::string path = TEST_ROOT_DIR + "/task_pending_cleanup.jpg";
    CreateTestAssetFile(path);
    int32_t fileId = InsertPendingPhoto(path, -1);
    ASSERT_GT(fileId, 0);
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(fileId));
    ASSERT_TRUE(DeletePendingTouchForTest(fileId));

    MockAlwaysAcceptClonePendingTask task;
    task.Execute();

    EXPECT_FALSE(ExistsPhotoById(fileId));
    EXPECT_FALSE(HasPendingFileId(fileId));
    MEDIA_INFO_LOG("MediaClonePendingTask_ExecuteCleanupPendingAsset_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, MediaClonePendingTask_SkipActivePendingAsset_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaClonePendingTask_SkipActivePendingAsset_001 start");

    std::string path = TEST_ROOT_DIR + "/task_pending_active.jpg";
    CreateTestAssetFile(path);
    int32_t fileId = InsertPendingPhoto(path, -1);
    ASSERT_GT(fileId, 0);
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(fileId));
    ASSERT_TRUE(ClonePendingRecordUtils::UpdatePendingFileTouch(fileId, true));

    MockAlwaysAcceptClonePendingTask task;
    task.Execute();

    EXPECT_TRUE(ExistsPhotoById(fileId));
    EXPECT_TRUE(HasPendingFileId(fileId));
    MEDIA_INFO_LOG("MediaClonePendingTask_SkipActivePendingAsset_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, CopyLocalSingleFileSync_RollbackCleanup_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("CopyLocalSingleFileSync_RollbackCleanup_001 start");
    std::string sourceName = "rollback_source.jpg";
    std::string sourcePath = TEST_ROOT_DIR + "/rollback_source_missing.jpg";
    int64_t sourceAssetId = InsertCloneSourceAssetForRollback(sourceName, sourcePath);
    ASSERT_GT(sourceAssetId, 0);

    OHOS::Media::CloneAssetInfo cloneAssetInfo;
    cloneAssetInfo.fileId = sourceAssetId;
    cloneAssetInfo.requestId = 1001;
    cloneAssetInfo.targetDisplayName = "rollback_target.jpg";
    cloneAssetInfo.targetFilePath = TEST_ROOT_DIR + "/rollback_target.jpg";

    int32_t beforePendingCount = QueryCountBySql(
        "SELECT count(*) FROM Photos WHERE time_pending = -1 AND data = '" + cloneAssetInfo.targetFilePath + "'");
    std::vector<int32_t> pendingIdsBefore = GetAllPendingFileIds();

    std::string newAssetIds;
    int32_t ret = OHOS::Media::MediaLibraryAlbumFusionUtils::CloneProgressAsset(cloneAssetInfo, 1,
        newAssetIds, [](uint64_t) {});
    EXPECT_NE(ret, E_OK);

    int32_t afterPendingCount = QueryCountBySql(
        "SELECT count(*) FROM Photos WHERE time_pending = -1 AND data = '" + cloneAssetInfo.targetFilePath + "'");
    std::vector<int32_t> pendingIdsAfter = GetAllPendingFileIds();
    EXPECT_EQ(afterPendingCount, beforePendingCount);
    EXPECT_EQ(pendingIdsAfter.size(), pendingIdsBefore.size());
    MEDIA_INFO_LOG("CopyLocalSingleFileSync_RollbackCleanup_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, RemovePendingFileId_RemoveEmbeddedTimestamp_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("RemovePendingFileId_RemoveEmbeddedTimestamp_001 start");
    constexpr int32_t fileId = 987654;
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(fileId));
    ASSERT_TRUE(HasPendingFileId(fileId));
    ASSERT_TRUE(HasPendingTouchForTest(fileId));

    ASSERT_TRUE(ClonePendingRecordUtils::RemovePendingFileId(fileId));
    EXPECT_FALSE(HasPendingFileId(fileId));
    EXPECT_FALSE(HasPendingTouchForTest(fileId));
    MEDIA_INFO_LOG("RemovePendingFileId_RemoveEmbeddedTimestamp_001 end");
}

HWTEST_F(MediaClonePendingRecordUtilsTest, MediaClonePendingTask_CleanupStaleEmbeddedTimestamp_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaClonePendingTask_CleanupStaleEmbeddedTimestamp_001 start");

    std::string path = TEST_ROOT_DIR + "/task_stale_embedded_touch_cleanup.jpg";
    CreateTestAssetFile(path);
    int32_t fileId = InsertPendingPhoto(path, -1);
    ASSERT_GT(fileId, 0);
    ASSERT_TRUE(ClonePendingRecordUtils::AddPendingFileId(fileId));
    ASSERT_TRUE(SetPendingTouchForTest(fileId,
        MediaFileUtils::UTCTimeMilliSeconds() - 5 * 60 * 1000));

    MockAlwaysAcceptClonePendingTask task;
    task.Execute();

    EXPECT_FALSE(ExistsPhotoById(fileId));
    EXPECT_FALSE(HasPendingFileId(fileId));
    MEDIA_INFO_LOG("MediaClonePendingTask_CleanupStaleEmbeddedTimestamp_001 end");
}
} // namespace OHOS::Media::Background
