/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#define MLOG_TAG "KvStoreSwapOperatorTest"

#include "kvstore_swap_operator_test.h"

#include "backup_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "reverse_clone_restore/kvstore_swap_operator.h"
#include "reverse_clone_restore_marker.h"
#include "settings_data_manager.h"

#include <gtest/hwext/gtest-ext.h>
#include <atomic>
#include <thread>

using namespace testing::ext;

namespace OHOS {
namespace Media {

static const std::string BACKUP_ROOT = "/storage/media/local/files/.backup/restore";
static const std::string TEMP_OLD = BACKUP_ROOT + "/old";
static const std::string TEMP_NEW = BACKUP_ROOT + "/new";
static const std::string TEMP_DIR = BACKUP_ROOT + "/temp";
static const std::string HASH_A = "hashA";
static const std::string HASH_B = "hashB";
static const std::string OLD_YEAR_HASH = "5f796bfa83fcd52d829a2304cb7194119a6e6e84b8efdde115b26fbd424b8541";
static const std::string NEW_YEAR_HASH = "d1fceec62f61bcc5d3b48e9b01c80384ba4ab9c91eb68b13a573ccea03803e43";
static const std::string OLD_MONTH_HASH = "1bd52423fd08d9b97e0c93366e3f9747b17e5dc321a3f0e328273cbd3dca611a";
static const std::string NEW_MONTH_HASH = "3ddb6fb8b2fcb38d2f431e86bfb806dab771637860d6e86bb9430fa15df04248";

void KvStoreSwapOperatorTest::SetUpTestCase()
{
    MediaFileUtils::CreateDirectory(BACKUP_ROOT);
}

void KvStoreSwapOperatorTest::TearDownTestCase()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

void KvStoreSwapOperatorTest::SetUp()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaFileUtils::CreateDirectory(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

void KvStoreSwapOperatorTest::TearDown()
{
    MediaFileUtils::DeleteDir(BACKUP_ROOT);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

static void CreateSingleVer(const std::string &base, const std::string &hash)
{
    std::string dir = base + "/" + hash + "/single_ver/main";
    MediaFileUtils::CreateDirectory(dir);
    FILE *f = fopen((dir + "/gen_natural_store.db").c_str(), "w");
    if (f != nullptr) {
        fputs("test", f);
        fclose(f);
    }
}

static bool FileExists(const std::string &base, const std::string &hash)
{
    return MediaPureFileUtils::IsFileExists(base + "/" + hash + "/single_ver/main/gen_natural_store.db");
}

static void CreateHashDir(const std::string &base, const std::string &hash)
{
    MediaFileUtils::CreateDirectory(base + "/" + hash);
}

HWTEST_F(KvStoreSwapOperatorTest, CleanupTempDir_NotExist_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.CleanupTempDir());
}

HWTEST_F(KvStoreSwapOperatorTest, CleanupTempDir_Exist_001, TestSize.Level1)
{
    CreateSingleVer(REVERSE_KVDB_TEMP_DIR + "/kvdb", HASH_A);
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.CleanupTempDir());
    EXPECT_FALSE(MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_OldDirsEmpty_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_NewDirsEmpty_001, TestSize.Level1)
{
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_A);
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_B);
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_NoMatch_001, TestSize.Level1)
{
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_A);
    CreateHashDir(BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb", HASH_B);
    CreateHashDir(std::string(MEDIA_KVDB_DIR) + "/kvdb", "hashC");
    CreateHashDir(std::string(MEDIA_KVDB_DIR) + "/kvdb", "hashD");
    KvStoreSwapOperator op;
    EXPECT_FALSE(op.Swap(BACKUP_ROOT));
    MediaFileUtils::DeleteDir(std::string(MEDIA_KVDB_DIR) + "/kvdb/hashC");
    MediaFileUtils::DeleteDir(std::string(MEDIA_KVDB_DIR) + "/kvdb/hashD");
}

HWTEST_F(KvStoreSwapOperatorTest, Swap_AllSuccess_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    int32_t ret = SettingsDataManager::SetKvDbCloseStatus("0");
    ASSERT_EQ(ret, E_OK) << "SettingsDataManager unavailable, skip Swap success test";

    std::atomic<bool> stopFlag{false};
    std::thread simThread([&stopFlag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SettingsDataManager::SetKvDbCloseStatus("1");
        while (!stopFlag.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);

    KvStoreSwapOperator op;
    bool result = op.Swap(BACKUP_ROOT);

    stopFlag.store(true, std::memory_order_relaxed);
    simThread.join();

    EXPECT_TRUE(result);
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_TRUE(FileExists(tempBase, OLD_YEAR_HASH));
    EXPECT_FALSE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(ReverseCloneRestoreMarker::Exists(ReverseCloneMarkerType::KVDB));

    ReverseCloneRestoreMarker::Delete(ReverseCloneMarkerType::KVDB);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_MONTH_HASH);
    MediaFileUtils::DeleteDir(oldBase);
    SettingsDataManager::SetKvDbCloseStatus("0");
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_FromNone_Empty_001, TestSize.Level1)
{
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.Rollback());
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_FromNone_WithTemp_001, TestSize.Level1)
{
    CreateSingleVer(REVERSE_KVDB_TEMP_DIR + "/kvdb", HASH_A);
    KvStoreSwapOperator op;
    EXPECT_TRUE(op.Rollback());
    EXPECT_FALSE(MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR));
}

HWTEST_F(KvStoreSwapOperatorTest, Rollback_FromNone_WithFiles_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string masterBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    KvStoreSwapOperator op;
    op.RestoreOriginalPositionsForResume(BACKUP_ROOT);

    CreateSingleVer(masterBase, NEW_YEAR_HASH);
    CreateSingleVer(tempBase, OLD_YEAR_HASH);
    CreateSingleVer(masterBase, NEW_MONTH_HASH);
    CreateSingleVer(tempBase, OLD_MONTH_HASH);

    EXPECT_TRUE(op.Rollback());
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(FileExists(masterBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    MediaFileUtils::DeleteDir(masterBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(masterBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_MONTH_HASH);
}

static bool DoSwapWithSimulatedHandshake()
{
    if (SettingsDataManager::SetKvDbCloseStatus("0") != E_OK) {
        return false;
    }
    std::atomic<bool> stopFlag{false};
    std::thread simThread([&stopFlag]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SettingsDataManager::SetKvDbCloseStatus("1");
        while (!stopFlag.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    KvStoreSwapOperator op;
    bool result = op.Swap(BACKUP_ROOT);
    stopFlag.store(true, std::memory_order_relaxed);
    simThread.join();
    return result;
}

static void CleanupSwapArtifacts()
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(tempBase + "/" + OLD_MONTH_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_YEAR_HASH);
    MediaFileUtils::DeleteDir(oldBase + "/" + OLD_MONTH_HASH);
    ReverseCloneRestoreMarker::Delete(ReverseCloneMarkerType::KVDB);
    SettingsDataManager::SetKvDbCloseStatus("0");
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_FullRestore_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_MasterLost_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";
    const std::string tempBase = std::string(REVERSE_KVDB_TEMP_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_FALSE(FileExists(tempBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_AlreadyRestored_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));
    EXPECT_TRUE(FileExists(newBase, NEW_YEAR_HASH));
    EXPECT_TRUE(FileExists(oldBase, OLD_YEAR_HASH));

    CleanupSwapArtifacts();
}

HWTEST_F(KvStoreSwapOperatorTest, Resume_AfterSwap_AllLost_001, TestSize.Level1)
{
    const std::string oldBase = BACKUP_ROOT + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    const std::string newBase = std::string(MEDIA_KVDB_DIR) + "/kvdb";

    CreateSingleVer(oldBase, OLD_YEAR_HASH);
    CreateSingleVer(oldBase, OLD_MONTH_HASH);
    CreateSingleVer(newBase, NEW_YEAR_HASH);
    CreateSingleVer(newBase, NEW_MONTH_HASH);
    ASSERT_TRUE(DoSwapWithSimulatedHandshake()) << "Swap failed, skip Resume test";

    MediaFileUtils::DeleteDir(newBase + "/" + NEW_YEAR_HASH);
    MediaFileUtils::DeleteDir(newBase + "/" + NEW_MONTH_HASH);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
    KvStoreSwapOperator resumeOp;
    EXPECT_TRUE(resumeOp.RestoreOriginalPositionsForResume(BACKUP_ROOT));

    CleanupSwapArtifacts();
}
} // namespace Media
} // namespace OHOS
