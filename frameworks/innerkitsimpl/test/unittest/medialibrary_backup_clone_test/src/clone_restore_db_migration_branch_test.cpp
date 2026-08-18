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

#define MLOG_TAG "CloneRestoreDbMigrationBranchTest"

#include "clone_restore_db_migration_branch_test.h"

#include <optional>
#include <string>
#include <vector>

#include "clone_restore.h"
#include "clone_restore_highlight.h"
#include "photo_album_column.h"
#include "rdb_helper.h"

using namespace OHOS::NativeRdb;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
const std::string SRC_DB_PATH = "/data/test/backup/clone_restore_db_migration_src.db";
const std::string SRC_NORISK_DB_PATH = "/data/test/backup/clone_restore_db_migration_src_no_risk.db";
const std::string DST_DB_PATH = "/data/test/backup/clone_restore_db_migration_dst.db";
std::shared_ptr<RdbStore> g_srcDb = nullptr;
std::shared_ptr<RdbStore> g_srcNoRiskDb = nullptr;
std::shared_ptr<RdbStore> g_dstDb = nullptr;

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

const std::vector<std::string> DB_MIGRATION_SCHEMA_SQLS = {
    "CREATE TABLE IF NOT EXISTS Photos ("
        "file_id INTEGER PRIMARY KEY, "
        "photo_risk_status INT DEFAULT 0, "
        "is_critical INT DEFAULT 0, "
        "package_name TEXT DEFAULT '', "
        "position INT DEFAULT 1);",
    "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
        "album_id INTEGER PRIMARY KEY, "
        "album_type INT DEFAULT 0, "
        "album_subtype INT DEFAULT 0, "
        "album_name TEXT, "
        "album_lpath TEXT DEFAULT '');",
    "CREATE TABLE IF NOT EXISTS AnalysisAlbum ("
        "album_id INTEGER PRIMARY KEY, "
        "album_type INT DEFAULT 0, "
        "album_subtype INT DEFAULT 0, "
        "album_name TEXT, "
        "tag_id TEXT DEFAULT '');",
};

const std::vector<std::string> DB_MIGRATION_NO_RISK_SCHEMA_SQLS = {
    "CREATE TABLE IF NOT EXISTS Photos ("
        "file_id INTEGER PRIMARY KEY, "
        "package_name TEXT DEFAULT '', "
        "position INT DEFAULT 1);",
    "CREATE TABLE IF NOT EXISTS PhotoAlbum ("
        "album_id INTEGER PRIMARY KEY, "
        "album_type INT DEFAULT 0, "
        "album_subtype INT DEFAULT 0, "
        "album_name TEXT, "
        "album_lpath TEXT DEFAULT '');",
    "CREATE TABLE IF NOT EXISTS AnalysisAlbum ("
        "album_id INTEGER PRIMARY KEY, "
        "album_type INT DEFAULT 0, "
        "album_subtype INT DEFAULT 0, "
        "album_name TEXT, "
        "tag_id TEXT DEFAULT '');",
};

class DbMigrationOpenCallback final : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &store) override
    {
        return ExecSqlList(store, DB_MIGRATION_SCHEMA_SQLS);
    }

    int OnUpgrade(RdbStore &store, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

class NoRiskOpenCallback final : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &store) override
    {
        return ExecSqlList(store, DB_MIGRATION_NO_RISK_SCHEMA_SQLS);
    }

    int OnUpgrade(RdbStore &store, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

void ClearDb(const std::shared_ptr<RdbStore> &db)
{
    if (db == nullptr) {
        return;
    }
    (void)db->ExecuteSql("DELETE FROM Photos;");
    (void)db->ExecuteSql("DELETE FROM PhotoAlbum;");
    (void)db->ExecuteSql("DELETE FROM AnalysisAlbum;");
}

int32_t QueryInt(const std::shared_ptr<RdbStore> &db, const std::string &sql, const std::string &column)
{
    auto rs = db->QuerySql(sql);
    if (rs == nullptr || rs->GoToFirstRow() != E_OK) {
        return 0;
    }
    int32_t value = GetInt32Val(column, rs);
    rs->Close();
    return value;
}

std::string QueryString(const std::shared_ptr<RdbStore> &db, const std::string &sql, const std::string &column)
{
    auto rs = db->QuerySql(sql);
    if (rs == nullptr || rs->GoToFirstRow() != E_OK) {
        return "";
    }
    std::string value = GetStringVal(column, rs);
    rs->Close();
    return value;
}
} // namespace

void CloneRestoreDbMigrationBranchTest::SetUpTestCase(void)
{
    int32_t errCode = E_OK;
    DbMigrationOpenCallback callback;
    NoRiskOpenCallback noRiskCallback;
    (void)RdbHelper::DeleteRdbStore(SRC_DB_PATH);
    (void)RdbHelper::DeleteRdbStore(SRC_NORISK_DB_PATH);
    (void)RdbHelper::DeleteRdbStore(DST_DB_PATH);
    g_srcDb = RdbHelper::GetRdbStore(RdbStoreConfig(SRC_DB_PATH), 1, callback, errCode);
    ASSERT_NE(g_srcDb, nullptr);
    g_srcNoRiskDb = RdbHelper::GetRdbStore(RdbStoreConfig(SRC_NORISK_DB_PATH), 1, noRiskCallback, errCode);
    ASSERT_NE(g_srcNoRiskDb, nullptr);
    g_dstDb = RdbHelper::GetRdbStore(RdbStoreConfig(DST_DB_PATH), 1, callback, errCode);
    ASSERT_NE(g_dstDb, nullptr);
}

void CloneRestoreDbMigrationBranchTest::TearDownTestCase(void)
{
    g_srcDb = nullptr;
    g_srcNoRiskDb = nullptr;
    g_dstDb = nullptr;
    (void)RdbHelper::DeleteRdbStore(SRC_DB_PATH);
    (void)RdbHelper::DeleteRdbStore(SRC_NORISK_DB_PATH);
    (void)RdbHelper::DeleteRdbStore(DST_DB_PATH);
}

void CloneRestoreDbMigrationBranchTest::SetUp()
{
    ClearDb(g_srcDb);
    ClearDb(g_srcNoRiskDb);
    ClearDb(g_dstDb);
}

void CloneRestoreDbMigrationBranchTest::TearDown()
{
    ClearDb(g_srcDb);
    ClearDb(g_srcNoRiskDb);
    ClearDb(g_dstDb);
}

// 场景：检查风险列状态，源端与目标端都含风险列。结果应满足条件并成功命中目标分支。
HWTEST_F(CloneRestoreDbMigrationBranchTest, CheckRiskColumnStatus_BothSides_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    EXPECT_TRUE(restore.CheckSrcDbHasRiskStatusColumn());
    EXPECT_TRUE(restore.CheckDestDbHasRiskStatusColumn());
}

// 场景：检查风险列状态，源端缺少风险列。结果为不满足条件或不进行映射。
HWTEST_F(CloneRestoreDbMigrationBranchTest, CheckRiskColumnStatus_SrcNoRisk_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcNoRiskDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    EXPECT_FALSE(restore.CheckSrcDbHasRiskStatusColumn());
    EXPECT_TRUE(restore.CheckDestDbHasRiskStatusColumn());
}

// 场景：更新同照片风险状态，输入中存在无效行。结果为不满足条件或不进行映射。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_SkipInvalidRows_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (1, 0, 0);"),
        E_OK);

    std::vector<FileInfo> infos;
    FileInfo a;
    a.fileIdNew = -1;
    a.isNew = false;
    a.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS);
    infos.push_back(a);

    FileInfo b;
    b.fileIdNew = 1;
    b.isNew = true;
    b.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::REJECTED);
    infos.push_back(b);

    restore.UpdatePreStatusForSamePhotos(infos);
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 1", "photo_risk_status"), 0);
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT is_critical FROM Photos WHERE file_id = 1", "is_critical"), 0);
}

// 场景：更新同照片风险状态，源端没有风险列。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_SrcNoRiskColumn_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcNoRiskDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (2, 0, 0);"),
        E_OK);

    FileInfo info;
    info.fileIdNew = 2;
    info.isNew = false;
    info.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS);
    std::vector<FileInfo> infos = {info};

    restore.UpdatePreStatusForSamePhotos(infos);
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 2", "photo_risk_status"), 0);
}

// 场景：更新同照片风险状态，风险状态为未识别。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_Unidentified_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (3, 0, 0);"),
        E_OK);

    FileInfo info;
    info.fileIdNew = 3;
    info.isNew = false;
    info.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::UNIDENTIFIED);
    std::vector<FileInfo> infos = {info};
    restore.UpdatePreStatusForSamePhotos(infos);
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 3", "photo_risk_status"), 0);
}

// 场景：更新同照片风险状态，风险可疑且关键。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_SuspiciousCritical_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (4, 0, 0);"),
        E_OK);

    FileInfo info;
    info.fileIdNew = 4;
    info.isNew = false;
    info.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS);
    std::vector<FileInfo> infos = {info};
    restore.UpdatePreStatusForSamePhotos(infos);

    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 4", "photo_risk_status"),
        static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS));
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT is_critical FROM Photos WHERE file_id = 4", "is_critical"), 1);
}

// 场景：更新同照片风险状态，风险拒绝且关键。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_RejectedCritical_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (5, 0, 0);"),
        E_OK);

    FileInfo info;
    info.fileIdNew = 5;
    info.isNew = false;
    info.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::REJECTED);
    std::vector<FileInfo> infos = {info};
    restore.UpdatePreStatusForSamePhotos(infos);

    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 5", "photo_risk_status"),
        static_cast<int32_t>(PhotoRiskStatus::REJECTED));
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT is_critical FROM Photos WHERE file_id = 5", "is_critical"), 1);
}

// 场景：更新同照片风险状态，风险非关键。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdateRiskStatusForSamePhotos_NonCritical_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, photo_risk_status, is_critical) VALUES (6, 0, 1);"),
        E_OK);

    FileInfo info;
    info.fileIdNew = 6;
    info.isNew = false;
    info.photoRiskStatus = static_cast<int32_t>(PhotoRiskStatus::UNIDENTIFIED) + 1;
    std::vector<FileInfo> infos = {info};
    restore.UpdatePreStatusForSamePhotos(infos);

    EXPECT_EQ(QueryInt(g_dstDb, "SELECT photo_risk_status FROM Photos WHERE file_id = 6", "photo_risk_status"),
        static_cast<int32_t>(PhotoRiskStatus::UNIDENTIFIED) + 1);
    EXPECT_EQ(QueryInt(g_dstDb, "SELECT is_critical FROM Photos WHERE file_id = 6", "is_critical"), 0);
}

// 场景：更新同照片包名，输入中存在无效行。结果为不满足条件或不进行映射。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdatePackageNameForSamePhotos_SkipInvalidRows_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, package_name) VALUES (11, '');"), E_OK);
    std::vector<FileInfo> infos;
    FileInfo a;
    a.fileIdNew = -1;
    a.isNew = false;
    a.originalPackageName = "pkg.a";
    infos.push_back(a);
    FileInfo b;
    b.fileIdNew = 11;
    b.isNew = true;
    b.originalPackageName = "pkg.b";
    infos.push_back(b);
    FileInfo c;
    c.fileIdNew = 11;
    c.isNew = false;
    c.originalPackageName = "";
    infos.push_back(c);
    restore.UpdatePreStatusForSamePhotos(infos);
    EXPECT_EQ(QueryString(g_dstDb, "SELECT package_name FROM Photos WHERE file_id = 11", "package_name"), "");
}

// 场景：更新同照片包名，仅更新空包名。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, UpdatePackageNameForSamePhotos_UpdateOnlyEmpty_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaLibraryRdb_ = g_dstDb;
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, package_name) VALUES (12, '');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO Photos (file_id, package_name) VALUES (13, 'keep.me');"), E_OK);

    FileInfo infoA;
    infoA.fileIdNew = 12;
    infoA.isNew = false;
    infoA.originalPackageName = "pkg.new";
    FileInfo infoB;
    infoB.fileIdNew = 13;
    infoB.isNew = false;
    infoB.originalPackageName = "pkg.override";
    infoB.newPackageName = "keep.me";
    std::vector<FileInfo> infos = {infoA, infoB};
    restore.UpdatePreStatusForSamePhotos(infos);

    EXPECT_EQ(QueryString(g_dstDb, "SELECT package_name FROM Photos WHERE file_id = 12", "package_name"), "pkg.new");
    EXPECT_EQ(QueryString(g_dstDb, "SELECT package_name FROM Photos WHERE file_id = 13", "package_name"), "keep.me");
}

// 场景：构建系统相册 ID 映射，正常映射流程。结果应满足条件并成功命中目标分支。
HWTEST_F(CloneRestoreDbMigrationBranchTest, PopulateSystemAlbumIdMap_NormalMapping_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;

    ASSERT_EQ(g_srcDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (1001, 1024, 1, 'src_sys_1');"), E_OK);
    ASSERT_EQ(g_srcDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (1002, 1024, 2, 'src_sys_2');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (2001, 1024, 1, 'dst_sys_1');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (2002, 1024, 2, 'dst_sys_2');"), E_OK);

    restore.PopulateSystemAlbumIdMap();

    ASSERT_TRUE(restore.tableAlbumIdMap_.count(PhotoAlbumColumns::TABLE) > 0);
    auto &idMap = restore.tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    EXPECT_EQ(idMap[1001], 2001);
    EXPECT_EQ(idMap[1002], 2002);
}

// 场景：构建系统相册 ID 映射，重复键保留首个。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, PopulateSystemAlbumIdMap_DuplicateKeyKeepFirst_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;

    ASSERT_EQ(g_srcDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (1101, 1024, 7, 'src_sys');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (2101, 1024, 7, 'dst_sys_first');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (2102, 1024, 7, 'dst_sys_second');"), E_OK);

    restore.PopulateSystemAlbumIdMap();

    auto &idMap = restore.tableAlbumIdMap_[PhotoAlbumColumns::TABLE];
    EXPECT_EQ(idMap[1101], 2101);
}

// 场景：构建系统相册 ID 映射，已映射项应跳过。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, PopulateSystemAlbumIdMap_SkipAlreadyMapped_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;

    ASSERT_EQ(g_srcDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (1201, 1024, 3, 'src_sys');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO PhotoAlbum (album_id, album_type, album_subtype, album_name) "
        "VALUES (2201, 1024, 3, 'dst_sys');"), E_OK);

    restore.tableAlbumIdMap_[PhotoAlbumColumns::TABLE][1201] = 9999;
    restore.PopulateSystemAlbumIdMap();
    EXPECT_EQ(restore.tableAlbumIdMap_[PhotoAlbumColumns::TABLE][1201], 9999);
}

// 场景：构建分析相册 ID 映射，已映射项应跳过。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, PopulateAnalysisAlbumIdMap_SkipAlreadyMapped_001, TestSize.Level1)
{
    CloneRestore restore;
    restore.mediaRdb_ = g_srcDb;
    restore.mediaLibraryRdb_ = g_dstDb;

    ASSERT_EQ(g_srcDb->ExecuteSql("INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name, tag_id) "
        "VALUES (3101, 2048, 4104, 'x', 'y');"), E_OK);
    ASSERT_EQ(g_dstDb->ExecuteSql("INSERT INTO AnalysisAlbum (album_id, album_type, album_subtype, album_name, tag_id) "
        "VALUES (4101, 2048, 4104, 'x', 'y');"), E_OK);

    restore.tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE][3101] = 123456;
    restore.PopulateAnalysisAlbumIdMap();
    EXPECT_EQ(restore.tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE][3101], 123456);
}

// 场景：写入高光相册映射关系，合法数据插入，异常数据跳过。结果应与该场景的分支设计保持一致。
HWTEST_F(CloneRestoreDbMigrationBranchTest, StoreHighlightAlbumMappings_InsertAndSkip_001, TestSize.Level1)
{
    CloneRestore restore;

    CloneRestoreHighlight highlight;
    CloneRestoreHighlight::AnalysisAlbumInfo infoA;
    infoA.albumIdOld = 5001;
    infoA.albumIdNew = 6001;
    highlight.analysisInfos_.push_back(infoA);

    CloneRestoreHighlight::AnalysisAlbumInfo infoB;
    infoB.albumIdOld = 5002;
    infoB.albumIdNew = 6002;
    highlight.analysisInfos_.push_back(infoB);

    CloneRestoreHighlight::AnalysisAlbumInfo infoMissing;
    infoMissing.albumIdOld = 5003;
    highlight.analysisInfos_.push_back(infoMissing);

    restore.tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE][5002] = 8888;
    restore.StoreHighlightAlbumMappings(highlight);

    auto &idMap = restore.tableAlbumIdMap_[ANALYSIS_ALBUM_TABLE];
    EXPECT_EQ(idMap[5001], 6001);
    EXPECT_EQ(idMap[5002], 8888);
    EXPECT_TRUE(idMap.count(5003) == 0);
}
} // namespace Media
} // namespace OHOS
