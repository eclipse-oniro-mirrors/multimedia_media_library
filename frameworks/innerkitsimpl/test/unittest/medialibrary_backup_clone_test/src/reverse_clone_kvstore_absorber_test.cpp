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

#define MLOG_TAG "ReverseCloneKvStoreAbsorberTest"

#include "reverse_clone_kvstore_absorber_test.h"

#include "backup_const.h"
#include "backup_database_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "rdb_helper.h"
#define private public
#define protected public
#include "reverse_clone_restore/reverse_clone_kvstore_absorber.h"
#undef private
#undef protected
#include "reverse_clone_restore/reverse_clone_resource_plan.h"
#include "result_set_utils.h"
#include "values_bucket.h"

#include <gtest/hwext/gtest-ext.h>
#include <memory>

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static const std::string TEST_DB_PATH = "/data/local/tmp/absorber_test.db";
static std::shared_ptr<NativeRdb::RdbStore> g_rdbStore = nullptr;

class AbsorberTestRdbCallback : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        store.ExecuteSql("CREATE TABLE IF NOT EXISTS Photos (file_id INTEGER PRIMARY KEY, "
            "date_taken INTEGER, time_pending INTEGER, is_temp INTEGER, date_trashed INTEGER, "
            "thumbnail_ready INTEGER DEFAULT 1, thumbnail_visible INTEGER DEFAULT 1)");
        store.ExecuteSql("CREATE TABLE IF NOT EXISTS tab_cloned_old_photos ("
            "file_id INTEGER, old_file_id INTEGER)");
        return NativeRdb::E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore &, int, int) override
    {
        return NativeRdb::E_OK;
    }
};

static void InsertTestPhoto(int32_t fileId, int64_t dateTaken, int32_t oldFileId)
{
    std::string insertPhoto = "INSERT OR REPLACE INTO Photos (file_id, date_taken, time_pending, "
        "is_temp, date_trashed) VALUES (" + std::to_string(fileId) + ", " +
        std::to_string(dateTaken) + ", 0, 0, 0)";
    g_rdbStore->ExecuteSql(insertPhoto);
    if (oldFileId > 0 && oldFileId != fileId) {
        std::string insertMap = "INSERT OR REPLACE INTO tab_cloned_old_photos (file_id, old_file_id) "
            "VALUES (" + std::to_string(fileId) + ", " + std::to_string(oldFileId) + ")";
        g_rdbStore->ExecuteSql(insertMap);
    }
}

static void ClearTestPhotos()
{
    g_rdbStore->ExecuteSql("DELETE FROM Photos");
    g_rdbStore->ExecuteSql("DELETE FROM tab_cloned_old_photos");
}

void ReverseCloneKvStoreAbsorberTest::SetUpTestCase()
{
    MediaPureFileUtils::CreateDirectory(MEDIA_KVDB_DIR);
    MediaPureFileUtils::CreateDirectory(REVERSE_KVDB_TEMP_DIR);

    int errCode = NativeRdb::E_OK;
    AbsorberTestRdbCallback callback;
    g_rdbStore = NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(TEST_DB_PATH), 1, callback, errCode);
    ASSERT_NE(g_rdbStore, nullptr);
}

void ReverseCloneKvStoreAbsorberTest::TearDownTestCase()
{
    g_rdbStore = nullptr;
    MediaFileUtils::DeleteFile(TEST_DB_PATH);
    MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR);
}

void ReverseCloneKvStoreAbsorberTest::SetUp()
{
    ClearTestPhotos();
}

void ReverseCloneKvStoreAbsorberTest::TearDown()
{
    ClearTestPhotos();
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_NullRdb_001, TestSize.Level1)
{
    ReverseCloneKvStoreAbsorber absorber;
    std::vector<ReverseCloneResourcePlan> plans;
    EXPECT_FALSE(absorber.Absorb(nullptr, plans));
}

static void ForceNullStores(ReverseCloneKvStoreAbsorber &absorber)
{
    absorber.monthStores_.master = nullptr;
    absorber.monthStores_.temp = nullptr;
    absorber.yearStores_.master = nullptr;
    absorber.yearStores_.temp = nullptr;
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_ValidRdb_NullStores_001, TestSize.Level1)
{
    InsertTestPhoto(1, 20240101120000, 100);

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans;
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_EmptyRdb_NullStores_001, TestSize.Level1)
{
    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans;
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_SameFileId_Skipped_001, TestSize.Level1)
{
    InsertTestPhoto(1, 20240101120000, 0);

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans;
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_DuplicatePlans_SameCloudConflict_001, TestSize.Level1)
{
    ReverseCloneResourcePlan plan;
    plan.matchType = ReverseCloneMatchType::SAME_CLOUD_CONFLICT;
    plan.donor.fileId = 200;
    plan.donor.dateTaken = 20240101120000;
    plan.absorbed.fileId = 1;

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans = { plan };
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_DuplicatePlans_InvalidDonor_001, TestSize.Level1)
{
    ReverseCloneResourcePlan plan1;
    plan1.matchType = ReverseCloneMatchType::NORMAL_SIGNATURE;
    plan1.donor.fileId = 0;
    plan1.donor.dateTaken = 20240101120000;

    ReverseCloneResourcePlan plan2;
    plan2.matchType = ReverseCloneMatchType::NORMAL_SIGNATURE;
    plan2.donor.fileId = 200;
    plan2.donor.dateTaken = 0;

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans = { plan1, plan2 };
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_DuplicatePlans_ValidDonor_NullStores_001, TestSize.Level1)
{
    ReverseCloneResourcePlan plan;
    plan.matchType = ReverseCloneMatchType::NORMAL_SIGNATURE;
    plan.donor.fileId = 200;
    plan.donor.dateTaken = 20240101120000;
    plan.donor.originalFileId = 150;
    plan.absorbed.fileId = 1;

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans = { plan };
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

HWTEST_F(ReverseCloneKvStoreAbsorberTest, Absorb_MultiplePhotos_NullStores_001, TestSize.Level1)
{
    InsertTestPhoto(1, 20240101120000, 100);
    InsertTestPhoto(2, 20240102120000, 200);
    InsertTestPhoto(3, 20240103120000, 300);

    ReverseCloneKvStoreAbsorber absorber;
    absorber.Init();
    ForceNullStores(absorber);

    std::vector<ReverseCloneResourcePlan> plans;
    EXPECT_FALSE(absorber.Absorb(g_rdbStore, plans));
}

} // namespace Media
} // namespace OHOS
