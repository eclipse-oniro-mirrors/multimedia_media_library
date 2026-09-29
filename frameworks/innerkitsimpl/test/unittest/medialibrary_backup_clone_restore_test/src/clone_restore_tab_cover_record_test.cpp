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

#define MLOG_TAG "CloneRestoreTabCoverRecordTest"

#include "clone_restore_tab_cover_record_test.h"

#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>

#include "clone_restore.h"

#include "backup_database_utils.h"
#include "cover_record_columns.h"
#include "photo_album_column.h"
#include "rdb_helper.h"

using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
const std::string SRC_DB_PATH = "/data/test/backup/clone_restore_cover_record_src.db";
const std::string DST_DB_PATH = "/data/test/backup/clone_restore_cover_record_dst.db";

constexpr int32_t SLEEP_FIVE_SECONDS = 5;

std::shared_ptr<NativeRdb::RdbStore> g_srcDb = nullptr;
std::shared_ptr<NativeRdb::RdbStore> g_dstDb = nullptr;

const std::string CREATE_COVER_RECORD_SQL =
    "CREATE TABLE IF NOT EXISTS tab_cover_record ("
    "album_type INT NOT NULL DEFAULT 0, "
    "album_subtype INT NOT NULL DEFAULT 0, "
    "lpath TEXT DEFAULT NULL, "
    "cover_order_key TEXT DEFAULT NULL, "
    "cover_order_subkey TEXT DEFAULT NULL, "
    "cover_order_type INT NOT NULL DEFAULT 0, "
    "hidden_cover_order_key TEXT DEFAULT NULL, "
    "hidden_cover_order_subkey TEXT DEFAULT NULL, "
    "hidden_cover_order_type INT NOT NULL DEFAULT 0);";

const std::string CREATE_PHOTO_ALBUM_SQL =
    "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
    "album_id INTEGER PRIMARY KEY, "
    "album_type INT DEFAULT 0, "
    "album_subtype INT DEFAULT 0, "
    "album_name TEXT, "
    "album_bundle_name TEXT DEFAULT '', "
    "lpath TEXT DEFAULT '', "
    "date_modified BIGINT DEFAULT 0);";

int ExecSqlList(RdbStore &store, const std::vector<std::string> &sqls)
{
    for (const auto &sql : sqls) {
        int ret = store.ExecuteSql(sql);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

class CoverRecordSchemaCallback final : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &store) override
    {
        return ExecSqlList(store, {CREATE_COVER_RECORD_SQL, CREATE_PHOTO_ALBUM_SQL});
    }
    int OnUpgrade(RdbStore &store, int, int) override
    {
        return E_OK;
    }
};

void ClearCoverRecordDbs(const std::shared_ptr<RdbStore> &srcStore, const std::shared_ptr<RdbStore> &dstStore)
{
    if (srcStore == nullptr || dstStore == nullptr) {
        return;
    }
    (void)srcStore->ExecuteSql("DELETE FROM tab_cover_record;");
    (void)srcStore->ExecuteSql("DELETE FROM PhotoAlbum;");
    (void)dstStore->ExecuteSql("DELETE FROM tab_cover_record;");
    (void)dstStore->ExecuteSql("DELETE FROM PhotoAlbum;");
}

struct CoverRecordRow {
    int32_t albumType = 0;
    int32_t albumSubtype = 0;
    std::string lpath;
    std::string orderKey;
    int32_t orderType = 0;
};

void InsertCoverRecord(RdbStore &store, const CoverRecordRow &row)
{
    std::string sql = "INSERT INTO tab_cover_record (album_type, album_subtype, lpath, "
        "cover_order_key, cover_order_type) VALUES (" +
        std::to_string(row.albumType) + ", " + std::to_string(row.albumSubtype) + ", '" + row.lpath + "', '" +
        row.orderKey + "', " + std::to_string(row.orderType) + ");";
    (void)store.ExecuteSql(sql);
}

void InsertPhotoAlbum(RdbStore &store, int32_t albumId, int32_t albumType, int32_t albumSubtype,
    const std::string &lpath)
{
    std::string sql = "INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, lpath) VALUES (" +
        std::to_string(albumId) + ", " + std::to_string(albumType) + ", " + std::to_string(albumSubtype) +
        ", '" + lpath + "');";
    (void)store.ExecuteSql(sql);
}

void InsertCoverRecord(const std::shared_ptr<RdbStore> &store, const CoverRecordRow &row)
{
    if (store == nullptr) {
        return;
    }
    InsertCoverRecord(*store, row);
}

void InsertPhotoAlbum(const std::shared_ptr<RdbStore> &store, int32_t albumId, int32_t albumType,
    int32_t albumSubtype, const std::string &lpath)
{
    if (store == nullptr) {
        return;
    }
    InsertPhotoAlbum(*store, albumId, albumType, albumSubtype, lpath);
}
} // namespace

void CloneRestoreTabCoverRecordTest::SetUpTestCase(void)
{
    std::error_code ec;
    (void)std::filesystem::create_directories("/data/test/backup", ec);

    (void)RdbHelper::DeleteRdbStore(SRC_DB_PATH);
    (void)RdbHelper::DeleteRdbStore(DST_DB_PATH);

    int32_t srcErrCode = E_OK;
    int32_t dstErrCode = E_OK;
    CoverRecordSchemaCallback cb;
    g_srcDb = RdbHelper::GetRdbStore(RdbStoreConfig(SRC_DB_PATH), 1, cb, srcErrCode);
    g_dstDb = RdbHelper::GetRdbStore(RdbStoreConfig(DST_DB_PATH), 1, cb, dstErrCode);
    ASSERT_EQ(srcErrCode, E_OK);
    ASSERT_EQ(dstErrCode, E_OK);
    ASSERT_NE(g_srcDb, nullptr);
    ASSERT_NE(g_dstDb, nullptr);
}

void CloneRestoreTabCoverRecordTest::TearDownTestCase(void)
{
    g_srcDb = nullptr;
    g_dstDb = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void CloneRestoreTabCoverRecordTest::SetUp()
{
    ClearCoverRecordDbs(g_srcDb, g_dstDb);
}

void CloneRestoreTabCoverRecordTest::TearDown()
{
    ClearCoverRecordDbs(g_srcDb, g_dstDb);
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_NullRdb_EarlyReturn, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = nullptr;
    restore.mediaLibraryRdb_ = nullptr;
    restore.RestoreTabCoverRecord();
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_EmptySource_EarlyReturn, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    restore.RestoreTabCoverRecord();
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb, "SELECT count(1) AS c FROM tab_cover_record;", "c"), 0);
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_InsertNonSystem, TestSize.Level1)
{
    InsertCoverRecord(*g_srcDb, CoverRecordRow{0, 1, "/data/a.jpg", "date_added", 2});
    InsertPhotoAlbum(*g_dstDb, 100, 0, 1, "/data/a.jpg");

    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    restore.RestoreTabCoverRecord();

    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb, "SELECT count(1) AS c FROM tab_cover_record;", "c"), 1);
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb,
        "SELECT cover_order_type FROM tab_cover_record WHERE album_type = 0 AND album_subtype = 1;",
        "cover_order_type"), 2);
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb,
        "SELECT count(1) AS c FROM PhotoAlbum WHERE album_type = 0 AND album_subtype = 1 AND "
        "LOWER(lpath) = LOWER('/data/a.jpg');", "c"), 1);
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_InsertSystemNoLpath, TestSize.Level1)
{
    InsertCoverRecord(*g_srcDb, CoverRecordRow{1024, 4104, "", "date_modified", 1});
    InsertPhotoAlbum(*g_dstDb, 200, 1024, 4104, "");

    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    restore.RestoreTabCoverRecord();

    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb, "SELECT count(1) AS c FROM tab_cover_record;", "c"), 1);
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb,
        "SELECT cover_order_type FROM tab_cover_record WHERE album_type = 1024 AND album_subtype = 4104;",
        "cover_order_type"), 1);
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_UpdateWhenExist, TestSize.Level1)
{
    InsertCoverRecord(*g_srcDb, CoverRecordRow{0, 1, "/data/a.jpg", "date_added", 3});
    InsertCoverRecord(*g_dstDb, CoverRecordRow{0, 1, "/data/a.jpg", "old_key", 0});
    InsertPhotoAlbum(*g_dstDb, 100, 0, 1, "/data/a.jpg");

    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    restore.RestoreTabCoverRecord();

    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb, "SELECT count(1) AS c FROM tab_cover_record;", "c"), 1);
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb,
        "SELECT cover_order_type FROM tab_cover_record WHERE album_type = 0 AND album_subtype = 1;",
        "cover_order_type"), 3);
}

HWTEST_F(CloneRestoreTabCoverRecordTest, RestoreTabCoverRecord_SystemAlbumUpdateWhenExist,
    TestSize.Level1)
{
    InsertCoverRecord(*g_srcDb, CoverRecordRow{1024, 4104, "", "date_modified", 5});
    InsertCoverRecord(*g_dstDb, CoverRecordRow{1024, 4104, "", "old_key", 0});
    InsertPhotoAlbum(*g_dstDb, 200, 1024, 4104, "");

    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    restore.RestoreTabCoverRecord();

    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb, "SELECT count(1) AS c FROM tab_cover_record;", "c"), 1);
    EXPECT_EQ(BackupDatabaseUtils::QueryInt(g_dstDb,
        "SELECT cover_order_type FROM tab_cover_record WHERE album_type = 1024 AND album_subtype = 4104;",
        "cover_order_type"), 5);
}
} // namespace Media
} // namespace OHOS
