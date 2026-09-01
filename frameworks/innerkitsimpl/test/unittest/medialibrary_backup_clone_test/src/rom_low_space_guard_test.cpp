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

#define MLOG_TAG "RomLowSpaceGuardTest"

#include "rom_low_space_guard_test.h"

#include <sys/statvfs.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "backup_const.h"
#include "medialibrary_errno.h"
#include "rdb_helper.h"
#include "rom_low_space_guard.h"

using namespace testing::ext;

// 覆盖目标：services/media_backup_extension/src/rom_low_space_guard.cpp 全部函数
//   RomLowSpaceGuard::Reset / GetAvailableBytes(private) / EvaluateCheckpoint / GetMode
//   AnalysisDataDropper::IsDropTable / BuildFileIdClause(private) / DropBatchRows /
//   DropRowsBuffered / FlushRows / ResetBuffers
//
// 说明：avail < 3G、恰好 3G、监控中跌破、锁存后不再 statvfs 等分支需要可控的
// 文件系统可用空间，真实测试环境难以稳定构造（且 G.EXP.01-CPP 禁止使用 --wrap
// 保留标识符 __wrap_statvfs 打桩），此类难以进入的分支按指导忽略；
// 保留可确定性构造的分支：空路径、statvfs 失败（不存在路径）、成功路径
// （与测试内直查 statvfs 的结果比对，环境自适应 oracle）。

namespace OHOS {
namespace Media {
namespace {
const std::string TEST_DB_PATH = "/data/test/backup/rom_low_space_guard_test.db";

std::shared_ptr<NativeRdb::RdbStore> g_db = nullptr;

class GuardTestOpenCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        int ret = store.ExecuteSql(
            "CREATE TABLE IF NOT EXISTS tab_analysis_segmentation ("
            "file_id INTEGER PRIMARY KEY, feature BLOB);");
        ret |= store.ExecuteSql(
            "CREATE TABLE IF NOT EXISTS tab_analysis_image_face ("
            "file_id INTEGER PRIMARY KEY, feature BLOB);");
        return ret;
    }

    int OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};

int QueryCount(const std::string &table)
{
    if (g_db == nullptr) {
        return -1;
    }
    auto rs = g_db->QuerySql("SELECT COUNT(*) FROM " + table + ";");
    if (rs == nullptr || rs->GoToFirstRow() != E_OK) {
        return -1;
    }
    int count = 0;
    (void)rs->GetInt(0, count);
    rs->Close();
    return count;
}

void InsertRows(const std::string &table, int32_t begin, int32_t end)
{
    if (g_db == nullptr) {
        return;
    }
    for (int32_t i = begin; i < end; i++) {
        (void)g_db->ExecuteSql("INSERT OR IGNORE INTO " + table + " (file_id) VALUES (" +
            std::to_string(i) + ");");
    }
}

std::vector<int32_t> MakeIds(int32_t n)
{
    std::vector<int32_t> ids;
    ids.reserve(n);
    for (int32_t i = 1; i <= n; i++) {
        ids.push_back(i);
    }
    return ids;
}
} // namespace

void RomLowSpaceGuardTest::SetUpTestCase(void)
{
    int32_t errCode = E_OK;
    (void)NativeRdb::RdbHelper::DeleteRdbStore(TEST_DB_PATH);
    GuardTestOpenCallback openCallback;
    g_db = NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(TEST_DB_PATH), 1,
        openCallback, errCode);
}

void RomLowSpaceGuardTest::TearDownTestCase(void)
{
    g_db = nullptr;
    (void)NativeRdb::RdbHelper::DeleteRdbStore(TEST_DB_PATH);
}

void RomLowSpaceGuardTest::SetUp(void)
{
    RomLowSpaceGuard::Reset();
    AnalysisDataDropper::ResetBuffers();
    if (g_db != nullptr) {
        (void)g_db->ExecuteSql("DELETE FROM tab_analysis_segmentation;");
        (void)g_db->ExecuteSql("DELETE FROM tab_analysis_image_face;");
    }
}

void RomLowSpaceGuardTest::TearDown(void)
{
    RomLowSpaceGuard::Reset();
    AnalysisDataDropper::ResetBuffers();
}

// =========================================================================
// RomLowSpaceGuard::Reset / GetMode —— 初始态与复位
// =========================================================================

// 场景：初始态为 NORMAL；连续多次 Reset 幂等
HWTEST_F(RomLowSpaceGuardTest, Reset_InitialAndIdempotent_001, TestSize.Level1)
{
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);

    RomLowSpaceGuard::Reset();
    RomLowSpaceGuard::Reset();
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// 场景：真实检查点判定后 Reset 复位回 NORMAL（oracle：与测试内直查 statvfs 的换算结果
// 比对，环境自适应：空间充足走 MONITORING 分支，紧张走 DROP_LATCHED 分支）
HWTEST_F(RomLowSpaceGuardTest, Reset_AfterRealCheckpoint_002, TestSize.Level1)
{
    struct statvfs st = {};
    ASSERT_EQ(statvfs("/data", &st), 0);
    int64_t avail = static_cast<int64_t>(st.f_bavail) * static_cast<int64_t>(st.f_frsize);
    GTEST_LOG_(INFO) << "statvfs /data: avail=" << avail;

    RomCheckMode expected = (avail < ROM_LOW_SPACE_THRESHOLD) ? RomCheckMode::DROP_LATCHED
                                                              : RomCheckMode::MONITORING;
    EXPECT_EQ(RomLowSpaceGuard::EvaluateCheckpoint("/data"), expected);

    RomLowSpaceGuard::Reset();
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// =========================================================================
// RomLowSpaceGuard::GetAvailableBytes（private，经 EvaluateCheckpoint 间接覆盖）
//   可构造分支：path 为空 -> -1；statvfs 失败（不存在路径）-> -1；
//   成功 -> f_bavail * f_frsize（经真实路径 oracle 验证）。
//   avail < 3G / 恰好 3G / 监控中跌破等需受控磁盘空间的分支难以构造，按指导忽略。
// =========================================================================

// 场景：GetAvailableBytes 空路径分支（提前返回 -1，EvaluateCheckpoint 保持 NORMAL）
HWTEST_F(RomLowSpaceGuardTest, GetAvailableBytes_EmptyPath_001, TestSize.Level1)
{
    EXPECT_EQ(RomLowSpaceGuard::EvaluateCheckpoint(""), RomCheckMode::NORMAL);
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// 场景：GetAvailableBytes statvfs 失败分支（不存在路径 -> -1，保持当前模式 NORMAL）
HWTEST_F(RomLowSpaceGuardTest, GetAvailableBytes_StatvfsFail_002, TestSize.Level1)
{
    EXPECT_EQ(RomLowSpaceGuard::EvaluateCheckpoint("/data/test/rom_guard_path_not_exist"),
        RomCheckMode::NORMAL);
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// 场景：statvfs 成功分支——真实路径 oracle 比对（f_bavail * f_frsize 与阈值比较，
// 结果与测试内直查换算一致，环境自适应覆盖两个比较分支之一；重复评估结果稳定）
HWTEST_F(RomLowSpaceGuardTest, EvaluateCheckpoint_RealPathOracle_003, TestSize.Level1)
{
    const std::string realPath = "/data";
    struct statvfs st = {};
    ASSERT_EQ(statvfs(realPath.c_str(), &st), 0);
    int64_t avail = static_cast<int64_t>(st.f_bavail) * static_cast<int64_t>(st.f_frsize);
    RomCheckMode expected = (avail < ROM_LOW_SPACE_THRESHOLD) ? RomCheckMode::DROP_LATCHED
                                                              : RomCheckMode::MONITORING;

    auto startTime = std::chrono::steady_clock::now();
    RomCheckMode mode = RomLowSpaceGuard::EvaluateCheckpoint(realPath);
    auto endTime = std::chrono::steady_clock::now();
    int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    GTEST_LOG_(INFO) << "EvaluateCheckpoint(" << realPath << ") cost: " << elapsedUs << " us";

    EXPECT_EQ(mode, expected);
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), expected);
    // 未锁存时重复评估结果稳定（MONITORING 下重复 statvfs 仍 MONITORING）
    if (expected == RomCheckMode::MONITORING) {
        EXPECT_EQ(RomLowSpaceGuard::EvaluateCheckpoint(realPath), RomCheckMode::MONITORING);
    }
}

// 场景：连续多次 statvfs 失败均保持当前模式，不误切 DROP_LATCHED（保守策略贯穿）
HWTEST_F(RomLowSpaceGuardTest, EvaluateCheckpoint_StatvfsFailNeverDrops_004, TestSize.Level1)
{
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(RomLowSpaceGuard::EvaluateCheckpoint("/data/test/rom_guard_path_not_exist"),
            RomCheckMode::NORMAL);
    }
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// =========================================================================
// AnalysisDataDropper::IsDropTable —— 白名单分支
// =========================================================================

// 场景：5 张白名单表逐一命中
HWTEST_F(RomLowSpaceGuardTest, IsDropTable_Whitelist_001, TestSize.Level1)
{
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_segmentation"));
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_label"));
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_ocr"));
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_pose"));
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_image_face"));
}

// 场景：非白名单表返回 false（循环走完未命中分支）
HWTEST_F(RomLowSpaceGuardTest, IsDropTable_NotInWhitelist_002, TestSize.Level1)
{
    EXPECT_FALSE(AnalysisDataDropper::IsDropTable("tab_analysis_head"));
    EXPECT_FALSE(AnalysisDataDropper::IsDropTable("tab_analysis_caption"));
    EXPECT_FALSE(AnalysisDataDropper::IsDropTable("Photos"));
    EXPECT_FALSE(AnalysisDataDropper::IsDropTable(""));
}

// =========================================================================
// AnalysisDataDropper::DropBatchRows —— 立即删除（含参数校验与 SQL 执行分支）
// =========================================================================

// 场景：正常删除源表行
HWTEST_F(RomLowSpaceGuardTest, DropBatchRows_Normal_001, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 11); // 10 行
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 10);

    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", MakeIds(10)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 0);
}

// 场景：传入部分不存在的 fileId（幂等删除无副作用）
HWTEST_F(RomLowSpaceGuardTest, DropBatchRows_PartialExisting_002, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 6); // id 1-5
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", MakeIds(10)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 0);
}

// 场景：rdb 句柄为空分支
HWTEST_F(RomLowSpaceGuardTest, DropBatchRows_NullRdb_003, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(nullptr, "tab_analysis_segmentation", MakeIds(2)), E_ERR);
}

// 场景：表名为空分支
HWTEST_F(RomLowSpaceGuardTest, DropBatchRows_EmptyTable_004, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "", MakeIds(2)), E_ERR);
}

// 场景：fileIds 为空分支
HWTEST_F(RomLowSpaceGuardTest, DropBatchRows_EmptyIds_005, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", {}), E_ERR);
}

// =========================================================================
// AnalysisDataDropper::DropRowsBuffered —— 聚合删除（含 1000 条阈值分支）
// =========================================================================

// 场景：不足 1000 条入缓冲，不触发 DELETE
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_BelowThreshold_001, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 101); // id 1-100
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(100)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 100); // 未删除
}

// 场景：恰好 1000 条（>= ROM_DROP_ANALYSIS_BATCH_ROWS）触发一次 DELETE
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_ExactThreshold_002, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 1002); // id 1-1001
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(1000)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // 剩 id=1001
}

// 场景：超阈值单次提交 1500 条：整桶交换触发一次 DELETE 1500 条，缓冲清空
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_OverThresholdSingleCall_003, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 1502); // id 1-1501
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(1500)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // 1-1500 整桶删除，剩 1501

    // 缓冲已被 swap 清空，flush 走 empty 分支
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1);
}

// 场景：跨多次调用累计满 1000 条触发（融合批 200 条 x5 的真实形态）
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_AccumulateAcrossCalls_004, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 1002); // id 1-1001
    for (int round = 0; round < 5; round++) {
        std::vector<int32_t> ids;
        for (int32_t i = round * 200 + 1; i <= (round + 1) * 200; i++) {
            ids.push_back(i);
        }
        EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", ids), E_OK);
    }
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // 第 5 轮恰好触发，仅剩 1001
}

// 场景：fileIds 为空（合法入参，插入 0 条，不触发删除）
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_EmptyIds_005, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 7); // id 1-6
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", {}), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 6);
}

// 场景：rdb 句柄为空分支
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_NullRdb_006, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(nullptr, "tab_analysis_segmentation", MakeIds(2)), E_ERR);
}

// 场景：表名为空分支
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_EmptyTable_007, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "", MakeIds(2)), E_ERR);
}

// 场景：不同表缓冲相互隔离（map 按 table key 分桶）
HWTEST_F(RomLowSpaceGuardTest, DropRowsBuffered_PerTableIsolation_008, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 5); // id 1-4
    InsertRows("tab_analysis_image_face", 1, 5);
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(4)), E_OK);
    // image_face 无缓冲：FlushRows 走 find 未命中分支，不做删除
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_image_face"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 4); // 4 条在缓冲未 flush
    EXPECT_EQ(QueryCount("tab_analysis_image_face"), 4);
}

// =========================================================================
// AnalysisDataDropper::FlushRows —— 尾批清空（含 find 未命中 / 空缓冲分支）
// =========================================================================

// 场景：正常 flush 尾批（<1000 条）
HWTEST_F(RomLowSpaceGuardTest, FlushRows_Normal_001, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 7); // id 1-6
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(5)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 6);

    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // id=6 残留
}

// 场景：无该表缓冲（map::find 未命中分支）
HWTEST_F(RomLowSpaceGuardTest, FlushRows_NoBuffer_002, TestSize.Level1)
{
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
}

// 场景：缓冲为空（empty 分支）——满批触发后 map entry 仍在，随后 FlushRows 走 empty 分支
HWTEST_F(RomLowSpaceGuardTest, FlushRows_EmptyBufferAfterSwap_003, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 1002); // id 1-1001
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(1000)), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // 满批已删 1-1000

    // map entry 存在但 buffer 已被 swap 清空 -> empty 分支，不执行 DELETE
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1);
}

// 场景：FlushRows 后桶复用正常（flush 删除 map entry 后再入缓冲）
HWTEST_F(RomLowSpaceGuardTest, FlushRows_BufferReuseAfterFlush_004, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 7); // id 1-6
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(5)), E_OK);
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1);

    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", { 6 }), E_OK);
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 0);
}

// =========================================================================
// AnalysisDataDropper::ResetBuffers —— 任务级复位
// =========================================================================

// 场景：缓冲未 flush 时 ResetBuffers 清空，后续 flush 不再删除（跨任务防误删）
HWTEST_F(RomLowSpaceGuardTest, ResetBuffers_ClearsPending_001, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 7); // id 1-6
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(5)), E_OK);
    AnalysisDataDropper::ResetBuffers();
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 6); // 未被删除
}

// 场景：ResetBuffers 幂等（无缓冲 / 多表缓冲全清）
HWTEST_F(RomLowSpaceGuardTest, ResetBuffers_Idempotent_002, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 7); // id 1-6
    InsertRows("tab_analysis_image_face", 1, 7);
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(5)), E_OK);
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_image_face", MakeIds(5)), E_OK);
    AnalysisDataDropper::ResetBuffers();
    AnalysisDataDropper::ResetBuffers();
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_image_face"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 6);
    EXPECT_EQ(QueryCount("tab_analysis_image_face"), 6);
}

// =========================================================================
// AnalysisDataDropper::BuildFileIdClause（private，经 DropBatchRows SQL 间接验证）
// =========================================================================

// 场景：单 id 与多 id 的 IN 子句拼接正确性（通过删除结果验证）
HWTEST_F(RomLowSpaceGuardTest, BuildFileIdClause_SingleAndMulti_001, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 4); // id 1-3
    // 单 id（isFirst 分支：无前导逗号）
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", { 2 }), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 2);
    // 多 id（逗号分隔分支）
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", { 1, 3, 4 }), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 0);
}

// 场景：负数 id 拼接（std::to_string 负值，SQL 语义合法不匹配任何行）
HWTEST_F(RomLowSpaceGuardTest, BuildFileIdClause_NegativeIds_002, TestSize.Level1)
{
    InsertRows("tab_analysis_segmentation", 1, 4); // id 1-3
    EXPECT_EQ(AnalysisDataDropper::DropBatchRows(g_db, "tab_analysis_segmentation", { -1, -2 }), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 3); // 负 id 不匹配任何行
}

// =========================================================================
// 并发分支（设计书 2.2.2 / 2.3.3：原子模式位 + 缓冲互斥）
// =========================================================================

// 场景：多线程并发 EvaluateCheckpoint（不存在路径，statvfs 失败分支）——
// 原子模式位无数据竞争，所有线程均观察到保持 NORMAL
HWTEST_F(RomLowSpaceGuardTest, EvaluateCheckpoint_ConcurrentNoRace_001, TestSize.Level1)
{
    constexpr int THREAD_NUM = 8;
    std::atomic<int> normalCount(0);
    std::vector<std::thread> threads;
    for (int t = 0; t < THREAD_NUM; t++) {
        threads.emplace_back([&]() {
            if (RomLowSpaceGuard::EvaluateCheckpoint("/data/test/rom_guard_path_not_exist") ==
                RomCheckMode::NORMAL) {
                normalCount++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(normalCount.load(), THREAD_NUM);
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}

// =========================================================================
// 端到端小场景：任务入口复位 + 白名单 + 聚合删除协同（设计书 2.1.2 / 2.3.1 / 2.7）
// 模式判定的空间阈值分支难以构造，此处验证任务流程中确定性的
// Reset / IsDropTable / DropRowsBuffered / FlushRows / ResetBuffers 协同链路
// =========================================================================

// 场景：任务入口复位 + 白名单表批缓冲删源 + 表末 flush + 新任务复位
HWTEST_F(RomLowSpaceGuardTest, EndToEnd_TaskLifecycleDrop_001, TestSize.Level1)
{
    // 任务入口：模式与缓冲复位（新任务从 NORMAL 重新判定）
    RomLowSpaceGuard::Reset();
    AnalysisDataDropper::ResetBuffers();
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);

    // 白名单表批进入删源缓冲（<1000 条），表末 flush
    InsertRows("tab_analysis_segmentation", 1, 202); // id 1-201
    EXPECT_TRUE(AnalysisDataDropper::IsDropTable("tab_analysis_segmentation"));
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_segmentation", MakeIds(200)), E_OK);
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_segmentation"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_segmentation"), 1); // id=201 残留

    // 新任务入口复位：缓冲清空，flush 不再误删
    InsertRows("tab_analysis_image_face", 1, 7); // id 1-6
    EXPECT_EQ(AnalysisDataDropper::DropRowsBuffered(g_db, "tab_analysis_image_face", MakeIds(6)), E_OK);
    AnalysisDataDropper::ResetBuffers();
    EXPECT_EQ(AnalysisDataDropper::FlushRows(g_db, "tab_analysis_image_face"), E_OK);
    EXPECT_EQ(QueryCount("tab_analysis_image_face"), 6); // 缓冲已清，未误删
    EXPECT_EQ(RomLowSpaceGuard::GetMode(), RomCheckMode::NORMAL);
}
} // namespace Media
} // namespace OHOS
