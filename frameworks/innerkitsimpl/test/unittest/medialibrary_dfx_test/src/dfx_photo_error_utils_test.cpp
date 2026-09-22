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

#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <vector>

#include "dfx_photo_error_utils.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
// 纯函数 seam 的独立 fixture：不依赖 rdbStore / 文件系统，仅验证纯逻辑 seam。
// 期望值全部由 ADR 0003 公式手算得出（独立真值源，非按代码反推）。
class DfxPhotoErrorUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

// ---------- Normalize ----------

HWTEST_F(DfxPhotoErrorUtilsTest, normalize_south_device_type_visit_to_null, TestSize.Level0)
{
    // VISIT(-1) 归一为 0（本地原生）
    EXPECT_EQ(NormalizeSouthDeviceType(-1), 0);
    EXPECT_EQ(NormalizeSouthDeviceType(0), 0);
    EXPECT_EQ(NormalizeSouthDeviceType(1), 1);
    EXPECT_EQ(NormalizeSouthDeviceType(2), 2);
}

HWTEST_F(DfxPhotoErrorUtilsTest, normalize_other_dims_passthrough, TestSize.Level0)
{
    // 其余维度当前 passthrough（预留扩展点）
    EXPECT_EQ(NormalizeFileSourceType(0), 0);
    EXPECT_EQ(NormalizeFileSourceType(1), 1);
    EXPECT_EQ(NormalizeFileSourceType(3), 3);
    EXPECT_EQ(NormalizeMediaType(1), 1);
    EXPECT_EQ(NormalizeMediaType(2), 2);
    EXPECT_EQ(NormalizePosition(1), 1);
    EXPECT_EQ(NormalizePosition(3), 3);
}

// ---------- EncodePhotoErrorType ----------
// 公式：fst×100000 + norm(sd)×10000 + pos×1000 + mt×100 + err×1

HWTEST_F(DfxPhotoErrorUtilsTest, encode_lake_image_local_missing, TestSize.Level0)
{
    // ADR 0003 示例场景：lake(3) + image(1) + sd=0(本地原生) + pos=1(本地) + 文件不存在(1)
    PhotoErrorDimension d{3, 0, 1, 1, PhotoErrorType::FILE_NOT_EXIST};
    // 3×100000 + 0×10000 + 1×1000 + 1×100 + 1×1 = 301101
    EXPECT_EQ(EncodePhotoErrorType(d), 301101);
}

HWTEST_F(DfxPhotoErrorUtilsTest, encode_min_real_code, TestSize.Level0)
{
    // 最小上报 type 码：fst=0/sd=0/pos=1/mt=1/err=1 = 1101（CONSISTENT=0 不上报，故 err 从 1 起）
    PhotoErrorDimension d{0, 0, 1, 1, PhotoErrorType::FILE_NOT_EXIST};
    EXPECT_EQ(EncodePhotoErrorType(d), 1101);
}

HWTEST_F(DfxPhotoErrorUtilsTest, encode_max_code, TestSize.Level0)
{
    // 最大 type 码：fst=3/sd=2/pos=3/mt=2/err=4 = 323204（int32 装得下）
    PhotoErrorDimension d{3, 2, 3, 2, PhotoErrorType::FILE_LARGER_THAN_DB};
    EXPECT_EQ(EncodePhotoErrorType(d), 323204);
}

HWTEST_F(DfxPhotoErrorUtilsTest, encode_normalizes_south_device_type, TestSize.Level0)
{
    // sd=-1 应在编码内归一为 0：fst=1/sd=-1/pos=1/mt=2/err=1 = 100000+0+1000+200+1 = 101201
    PhotoErrorDimension d{1, -1, 1, 2, PhotoErrorType::FILE_NOT_EXIST};
    EXPECT_EQ(EncodePhotoErrorType(d), 101201);
}

HWTEST_F(DfxPhotoErrorUtilsTest, encode_smaller_than_db_subtype, TestSize.Level0)
{
    // fst=0/sd=1/pos=3/mt=1/err=3(文件小于DB) = 0+10000+3000+100+3 = 13103
    PhotoErrorDimension d{0, 1, 3, 1, PhotoErrorType::FILE_SMALLER_THAN_DB};
    EXPECT_EQ(EncodePhotoErrorType(d), 13103);
}

HWTEST_F(DfxPhotoErrorUtilsTest, encode_no_collision_across_domain, TestSize.Level0)
{
    // 值域 {fst in 0,1,3} x {sd in 0,1,2} x {pos in 1,3} x {mt in 1,2} x {err in 0,1,2,3,4}
    // 全展开 3*3*2*2*5=180 组合，码两两不同（place-value 无碰撞：各维均单位数 <10）。
    std::vector<int32_t> codes;
    for (int32_t fst : {0, 1, 3}) {
        for (int32_t sd : {0, 1, 2}) {
            for (int32_t pos : {1, 3}) {
                for (int32_t mt : {1, 2}) {
                    for (int32_t err : {0, 1, 2, 3, 4}) {
                        codes.push_back(EncodePhotoErrorType(
                            {fst, sd, pos, mt, static_cast<PhotoErrorType>(err)}));
                    }
                }
            }
        }
    }
    std::sort(codes.begin(), codes.end());
    auto last = std::unique(codes.begin(), codes.end());
    EXPECT_EQ(last, codes.end()); // 全部唯一
    EXPECT_EQ(codes.size(), 180u);
}

// ---------- ResolveRealPath ----------
// 规则：fst in {1,3} -> storage_path（空则返回空串）；其他 -> data

HWTEST_F(DfxPhotoErrorUtilsTest, resolve_media_uses_data, TestSize.Level0)
{
    EXPECT_EQ(ResolveRealPath(0, "/d/a.jpg", ""), "/d/a.jpg");
    EXPECT_EQ(ResolveRealPath(0, "/d/a.jpg", "/s/a.jpg"), "/d/a.jpg"); // fst=0 忽略 storage_path
}

HWTEST_F(DfxPhotoErrorUtilsTest, resolve_file_manager_uses_storage_path, TestSize.Level0)
{
    EXPECT_EQ(ResolveRealPath(1, "/d/a.jpg", "/s/a.jpg"), "/s/a.jpg");
}

HWTEST_F(DfxPhotoErrorUtilsTest, resolve_lake_uses_storage_path, TestSize.Level0)
{
    EXPECT_EQ(ResolveRealPath(3, "/d/a.jpg", "/s/a.jpg"), "/s/a.jpg");
}

HWTEST_F(DfxPhotoErrorUtilsTest, resolve_file_manager_empty_storage_returns_empty, TestSize.Level0)
{
    // fst=1 但 storage_path 空 -> 返回空串，调用方应跳过（避免假阳性"文件不存在"）
    EXPECT_EQ(ResolveRealPath(1, "/d/a.jpg", ""), "");
}

HWTEST_F(DfxPhotoErrorUtilsTest, resolve_peripheral_falls_back_to_data, TestSize.Level0)
{
    // 其他 fst（如 2 PERIPHERAL）走 data 兜底
    EXPECT_EQ(ResolveRealPath(2, "/d/a.jpg", ""), "/d/a.jpg");
}

// ---------- ClassifyPhotoError ----------
// 1=missing, 2=size0, 3=disk<db(文件小于DB), 4=disk>db(文件大于DB), 0=consistent

HWTEST_F(DfxPhotoErrorUtilsTest, classify_missing_file, TestSize.Level0)
{
    EXPECT_EQ(ClassifyPhotoError(false, 0, 100), PhotoErrorType::FILE_NOT_EXIST);
}

HWTEST_F(DfxPhotoErrorUtilsTest, classify_size_zero, TestSize.Level0)
{
    // 文件存在但 0 字节 -> FILE_SIZE_ZERO（即使 db 也是 0 也先判 size 0）
    EXPECT_EQ(ClassifyPhotoError(true, 0, 100), PhotoErrorType::FILE_SIZE_ZERO);
    EXPECT_EQ(ClassifyPhotoError(true, 0, 0), PhotoErrorType::FILE_SIZE_ZERO);
}

HWTEST_F(DfxPhotoErrorUtilsTest, classify_file_smaller_than_db, TestSize.Level0)
{
    // disk < db -> 文件小于 DB
    EXPECT_EQ(ClassifyPhotoError(true, 50, 100), PhotoErrorType::FILE_SMALLER_THAN_DB);
}

HWTEST_F(DfxPhotoErrorUtilsTest, classify_file_larger_than_db, TestSize.Level0)
{
    // disk > db -> 文件大于 DB
    EXPECT_EQ(ClassifyPhotoError(true, 150, 100), PhotoErrorType::FILE_LARGER_THAN_DB);
}

HWTEST_F(DfxPhotoErrorUtilsTest, classify_consistent, TestSize.Level0)
{
    EXPECT_EQ(ClassifyPhotoError(true, 100, 100), PhotoErrorType::CONSISTENT);
}

HWTEST_F(DfxPhotoErrorUtilsTest, classify_missing_takes_precedence_over_size_zero, TestSize.Level0)
{
    // 短路顺序：missing 优先于 size0（GetFileSize 对缺失文件返回 0，须先判 missing）
    EXPECT_EQ(ClassifyPhotoError(false, 0, 0), PhotoErrorType::FILE_NOT_EXIST);
}

// ---------- PackPhotoErrors ----------
// std::map 已按 key 升序；每 packBatchSize 个一事件；空映射 -> 空 vector（静默）

HWTEST_F(DfxPhotoErrorUtilsTest, pack_empty_is_silent, TestSize.Level0)
{
    std::map<int32_t, int32_t> empty;
    auto batches = PackPhotoErrors(empty, 5);
    EXPECT_EQ(batches.size(), 0u); // 0 非零 -> 0 事件
}

HWTEST_F(DfxPhotoErrorUtilsTest, pack_single_entry_one_event, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{1101, 5}};
    auto batches = PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].photoErrorTypes.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes[0], 1101);
    EXPECT_EQ(batches[0].photoErrorCounts[0], 5);
}

HWTEST_F(DfxPhotoErrorUtilsTest, pack_five_entries_one_event, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{1101, 1}, {1201, 2}, {1301, 3}, {1401, 4}, {1501, 5}};
    auto batches = PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 5u);
    // 升序：1101..1501
    EXPECT_EQ(batches[0].photoErrorTypes[0], 1101);
    EXPECT_EQ(batches[0].photoErrorTypes[4], 1501);
}

HWTEST_F(DfxPhotoErrorUtilsTest, pack_six_entries_two_events, TestSize.Level0)
{
    std::map<int32_t, int32_t> m{{1101, 1}, {1201, 2}, {1301, 3}, {1401, 4}, {1501, 5}, {1601, 6}};
    auto batches = PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 2u); // ceil(6/5)=2
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 5u); // 前 5
    EXPECT_EQ(batches[1].photoErrorTypes.size(), 1u);  // 余 1
    EXPECT_EQ(batches[1].photoErrorTypes[0], 1601);
    EXPECT_EQ(batches[1].photoErrorCounts[0], 6);
}

HWTEST_F(DfxPhotoErrorUtilsTest, pack_preserves_ascending_order, TestSize.Level0)
{
    // 即便插入顺序乱，map 已排序；打包后整体升序、跨事件不回退
    std::map<int32_t, int32_t> m{{301101, 7}, {1101, 1}, {101201, 3}};
    auto batches = PackPhotoErrors(m, 5);
    ASSERT_EQ(batches.size(), 1u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 3u);
    EXPECT_EQ(batches[0].photoErrorTypes[0], 1101);
    EXPECT_EQ(batches[0].photoErrorTypes[1], 101201);
    EXPECT_EQ(batches[0].photoErrorTypes[2], 301101);
}

HWTEST_F(DfxPhotoErrorUtilsTest, pack_respects_custom_batch_size, TestSize.Level0)
{
    // packBatchSize=2 -> 3 条入 2 事件（2+1）
    std::map<int32_t, int32_t> m{{1101, 1}, {1201, 2}, {1301, 3}};
    auto batches = PackPhotoErrors(m, 2);
    ASSERT_EQ(batches.size(), 2u);
    EXPECT_EQ(batches[0].photoErrorTypes.size(), 2u);
    EXPECT_EQ(batches[1].photoErrorTypes.size(), 1u);
}
} // namespace Media
} // namespace OHOS
