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

#define MLOG_TAG "AncoReverseCloneAdapterTest"

#include "anco_reverse_clone_adapter.h"

#include "gtest/gtest.h"
#include <gtest/hwext/gtest-ext.h>
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "rdb_helper.h"
#include "result_set_utils.h"

#include <fstream>
#include <vector>

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
const std::string TEST_ROOT = "/data/test/backup/anco_reverse_clone_adapter";
const std::string FINAL_DB_PATH = TEST_ROOT + "/final.db";
const std::string CLONE_INFO_DB_PATH = TEST_ROOT + "/clone_file_info_restore.db";
const std::string OLD_ANCO_PATH = TEST_ROOT + "/old.jpg";
const std::string NEW_ANCO_PATH = TEST_ROOT + "/new.jpg";
const std::string EXISTING_ANCO_PATH = TEST_ROOT + "/existing.jpg";

class AncoFinalDbCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        return store.ExecuteSql("CREATE TABLE IF NOT EXISTS Photos ("
            "file_id INTEGER PRIMARY KEY, "
            "storage_path TEXT, "
            "display_name TEXT, "
            "title TEXT, "
            "size BIGINT DEFAULT 0, "
            "date_modified BIGINT DEFAULT 0, "
            "file_source_type INT DEFAULT 0, "
            "inode TEXT);");
    }

    int OnUpgrade(NativeRdb::RdbStore &, int, int) override
    {
        return NativeRdb::E_OK;
    }
};

class AncoCloneInfoDbCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        int32_t ret = store.ExecuteSql("CREATE TABLE IF NOT EXISTS " + LAKE_FILE_INFO_FAIL_TABLE + " ("
            + PhotoColumn::CLONE_FILE_INFO_PATH + " TEXT);");
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        return store.ExecuteSql("CREATE TABLE IF NOT EXISTS " + LAKE_FILE_INFO_DEDUPLICATION_TABLE + " ("
            + PhotoColumn::CLONE_FILE_INFO_PATH + " TEXT, "
            + PhotoColumn::CLONE_FILE_INFO_NEW_PATH + " TEXT);");
    }

    int OnUpgrade(NativeRdb::RdbStore &, int, int) override
    {
        return NativeRdb::E_OK;
    }
};

std::shared_ptr<NativeRdb::RdbStore> CreateFinalDb()
{
    int32_t errCode = NativeRdb::E_OK;
    AncoFinalDbCallback callback;
    NativeRdb::RdbHelper::DeleteRdbStore(FINAL_DB_PATH);
    return NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(FINAL_DB_PATH), 1, callback, errCode);
}

std::shared_ptr<NativeRdb::RdbStore> CreateCloneInfoDb()
{
    int32_t errCode = NativeRdb::E_OK;
    AncoCloneInfoDbCallback callback;
    NativeRdb::RdbHelper::DeleteRdbStore(CLONE_INFO_DB_PATH);
    return NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(CLONE_INFO_DB_PATH), 1, callback, errCode);
}

void WriteAncoTestFile(const std::string &path, const std::string &content)
{
    ASSERT_TRUE(MediaFileUtils::CreateDirectory(TEST_ROOT));
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << content;
    out.close();
}

std::string QueryStoragePath(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId)
{
    std::vector<NativeRdb::ValueObject> args = { fileId };
    auto resultSet = store->QuerySql("SELECT storage_path FROM Photos WHERE file_id = ?", args);
    CHECK_AND_RETURN_RET(resultSet != nullptr, "");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return "";
    }
    std::string storagePath = GetStringVal("storage_path", resultSet);
    resultSet->Close();
    return storagePath;
}

int32_t QueryPhotoCount(const std::shared_ptr<NativeRdb::RdbStore> &store)
{
    auto resultSet = store->QuerySql("SELECT COUNT(1) AS count FROM Photos");
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

void InsertAncoPhoto(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId,
    const std::string &storagePath, const std::string &displayName, int64_t size)
{
    std::vector<NativeRdb::ValueObject> photoArgs = {
        fileId,
        storagePath,
        displayName,
        MediaFileUtils::GetTitleFromDisplayName(displayName),
        size,
        static_cast<int64_t>(100),
        static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE),
    };
    ASSERT_EQ(store->ExecuteSql("INSERT INTO Photos (file_id, storage_path, display_name, title, size, "
        "date_modified, file_source_type) VALUES (?, ?, ?, ?, ?, ?, ?)",
        photoArgs), NativeRdb::E_OK);
}
} // namespace

class AncoReverseCloneAdapterTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
        ASSERT_TRUE(MediaFileUtils::CreateDirectory(TEST_ROOT));
    }
    void TearDown() override
    {
        NativeRdb::RdbHelper::DeleteRdbStore(FINAL_DB_PATH);
        NativeRdb::RdbHelper::DeleteRdbStore(CLONE_INFO_DB_PATH);
        MediaFileUtils::DeleteDir(TEST_ROOT);
    }
};

HWTEST_F(AncoReverseCloneAdapterTest, DecidePhase_PhaseTwoWhenDestinationSupportsTransfer, TestSize.Level0)
{
    AncoReverseCloneContext context;
    context.dstConfig.ancoFileTransfer = AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED;

    EXPECT_EQ(AncoReverseCloneAdapter::DecidePhase(context), AncoReverseClonePhase::PHASE_TWO);
}

HWTEST_F(AncoReverseCloneAdapterTest, DecidePhase_PhaseOneWhenDestinationDoesNotSupport, TestSize.Level0)
{
    AncoReverseCloneContext context;
    context.dstConfig.ancoFileTransfer = AncoFileTransfer::ANCO_FILE_TRANSFER_NONE;

    EXPECT_EQ(AncoReverseCloneAdapter::DecidePhase(context), AncoReverseClonePhase::PHASE_ONE);
}

HWTEST_F(AncoReverseCloneAdapterTest, GetDefaultDeduplicationDbPath_ReturnsRestoreCloneInfoDb, TestSize.Level0)
{
    EXPECT_EQ(AncoReverseCloneAdapter::GetDefaultDeduplicationDbPath(),
        OTHER_CLONE_PATH + CLONE_FILE_INFO_RESTORE_DB);
}

HWTEST_F(AncoReverseCloneAdapterTest, RepairFinalDb_PhaseTwoValidNewPathUpdatesStoragePath_001, TestSize.Level1)
{
    auto finalDb = CreateFinalDb();
    ASSERT_NE(finalDb, nullptr);
    auto cloneInfoDb = CreateCloneInfoDb();
    ASSERT_NE(cloneInfoDb, nullptr);
    WriteAncoTestFile(NEW_ANCO_PATH, "anco");
    InsertAncoPhoto(finalDb, 1, OLD_ANCO_PATH, "old.jpg", 4);
    std::vector<NativeRdb::ValueObject> cloneInfoArgs = { OLD_ANCO_PATH, NEW_ANCO_PATH };
    ASSERT_EQ(cloneInfoDb->ExecuteSql("INSERT INTO " + LAKE_FILE_INFO_DEDUPLICATION_TABLE + " ("
        + PhotoColumn::CLONE_FILE_INFO_PATH + ", " + PhotoColumn::CLONE_FILE_INFO_NEW_PATH + ") VALUES (?, ?)",
        cloneInfoArgs), NativeRdb::E_OK);

    AncoReverseCloneContext context;
    context.dstConfig.ancoFileTransfer = AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED;
    context.deduplicationDbPath = CLONE_INFO_DB_PATH;
    AncoReverseCloneAdapter adapter;

    EXPECT_EQ(adapter.RepairFinalDb(finalDb, context), E_OK);
    EXPECT_EQ(QueryStoragePath(finalDb, 1), NEW_ANCO_PATH);
    EXPECT_EQ(adapter.GetStats().phaseTwoRowsMatched, 1);
    EXPECT_EQ(adapter.GetStats().invalidNewPathRows, 0);
}

HWTEST_F(AncoReverseCloneAdapterTest, RepairFinalDb_PhaseTwoInfoUnavailableFallsBackToPhaseOne_001, TestSize.Level1)
{
    auto finalDb = CreateFinalDb();
    ASSERT_NE(finalDb, nullptr);
    WriteAncoTestFile(EXISTING_ANCO_PATH, "keep");
    InsertAncoPhoto(finalDb, 1, EXISTING_ANCO_PATH, "existing.jpg", 4);

    AncoReverseCloneContext context;
    context.dstConfig.ancoFileTransfer = AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED;
    context.deduplicationDbPath = CLONE_INFO_DB_PATH;
    AncoReverseCloneAdapter adapter;

    EXPECT_EQ(adapter.RepairFinalDb(finalDb, context), E_OK);
    EXPECT_EQ(adapter.GetStats().totalRows, 1);
    EXPECT_EQ(adapter.GetStats().phaseOneRowsKept, 1);
    EXPECT_EQ(adapter.GetStats().deletedRows, 0);
}

HWTEST_F(AncoReverseCloneAdapterTest, RepairFinalDb_PhaseTwoDuplicateOldPathKeepsSingleMatchedRow_001, TestSize.Level1)
{
    auto finalDb = CreateFinalDb();
    ASSERT_NE(finalDb, nullptr);
    auto cloneInfoDb = CreateCloneInfoDb();
    ASSERT_NE(cloneInfoDb, nullptr);
    WriteAncoTestFile(NEW_ANCO_PATH, "anco");
    InsertAncoPhoto(finalDb, 1, OLD_ANCO_PATH, "old.jpg", 4);
    InsertAncoPhoto(finalDb, 2, OLD_ANCO_PATH, "old_dup.jpg", 4);
    std::vector<NativeRdb::ValueObject> cloneInfoArgs = { OLD_ANCO_PATH, NEW_ANCO_PATH };
    ASSERT_EQ(cloneInfoDb->ExecuteSql("INSERT INTO " + LAKE_FILE_INFO_DEDUPLICATION_TABLE + " ("
        + PhotoColumn::CLONE_FILE_INFO_PATH + ", " + PhotoColumn::CLONE_FILE_INFO_NEW_PATH + ") VALUES (?, ?)",
        cloneInfoArgs), NativeRdb::E_OK);

    AncoReverseCloneContext context;
    context.dstConfig.ancoFileTransfer = AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED;
    context.deduplicationDbPath = CLONE_INFO_DB_PATH;
    AncoReverseCloneAdapter adapter;

    EXPECT_EQ(adapter.RepairFinalDb(finalDb, context), E_OK);
    EXPECT_EQ(adapter.GetStats().phaseTwoRowsMatched, 1);
    EXPECT_EQ(adapter.GetStats().phaseTwoDuplicateOldPathRows, 1);
    EXPECT_EQ(QueryStoragePath(finalDb, 1), NEW_ANCO_PATH);
    EXPECT_EQ(QueryStoragePath(finalDb, 2), OLD_ANCO_PATH);
    EXPECT_EQ(QueryPhotoCount(finalDb), 2);
}
} // namespace Media
} // namespace OHOS
