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

#define MLOG_TAG "NotifySuppressTest"

#include "media_notify_suppress_test.h"

#include <thread>

#include "dfx_utils.h"
#include "file_monitor_interface.h"
#include "file_scan_utils.h"
#include "media_file_change_processor.h"
#include "media_file_notify_info.h"
#include "media_log.h"
#include "media_notify_suppress_manager.h"

namespace OHOS {
namespace Media {
namespace {
using namespace testing::ext;

const std::string LAKE_PHYSICAL_PREFIX = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
const std::string FILE_MANAGER_PHYSICAL_PREFIX = "/storage/media/local/files/Docs/";
const std::string NORMAL_PATH_PREFIX = "/data/local/tmp/";

std::string LakeLogicPrefix()
{
    return FileScanUtils::GetCurrentInLakeLogicPrefix();
}

std::string FileManagerLogicPrefix()
{
    return "/data/service/el2/" + std::to_string(FileScanUtils::GetCurrentAccountId()) +
        "/hmdfs/account/files/Docs/";
}

std::string LakePhysicalPath(const std::string &relative)
{
    return LAKE_PHYSICAL_PREFIX + relative;
}

std::string LakeLogicPath(const std::string &relative)
{
    return LakeLogicPrefix() + relative;
}
} // namespace

void MediaNotifySuppressTest::SetUpTestCase() {}

void MediaNotifySuppressTest::TearDownTestCase() {}

void MediaNotifySuppressTest::SetUp()
{
    // 每个用例独立于全局单例状态, 用例间不互相污染
    MediaNotifySuppressManager::GetInstance().suppressed_.clear();
}

void MediaNotifySuppressTest::TearDown()
{
    MediaNotifySuppressManager::GetInstance().suppressed_.clear();
}

/* ==================== A. 基础API: Suppress / Release / IsSuppressed ==================== */

/**
 * @tc.number    : media_notify_suppress_basic_001
 * @tc.name      : Suppress physical lake path, same path is suppressed
 * @tc.desc      : 登记湖内物理路径后, 同一路径查询应命中
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/a.mp4");
    inst.Suppress({ path });
    EXPECT_TRUE(inst.IsSuppressed(path));
    inst.Release({ path });
    MEDIA_INFO_LOG("media_notify_suppress_basic_001 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_002
 * @tc.name      : Suppress physical path, logic path can match (dual form)
 * @tc.desc      : 核心场景: 登记物理路径(storage_path形态), file_monitor上报的逻辑路径应命中,
 *                 否则屏蔽对monitor侧完全失效
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_002 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string physical = LakePhysicalPath("Pictures/a.mp4");
    std::string logic = LakeLogicPath("Pictures/a.mp4");
    inst.Suppress({ physical });
    EXPECT_TRUE(inst.IsSuppressed(physical));
    EXPECT_TRUE(inst.IsSuppressed(logic));
    inst.Release({ physical });
    MEDIA_INFO_LOG("media_notify_suppress_basic_002 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_003
 * @tc.name      : Suppress logic path, physical path can match (reverse dual form)
 * @tc.desc      : 反向: 登记逻辑路径, 物理路径查询应命中
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_003 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string physical = LakePhysicalPath("Pictures/b.mp4");
    std::string logic = LakeLogicPath("Pictures/b.mp4");
    inst.Suppress({ logic });
    EXPECT_TRUE(inst.IsSuppressed(logic));
    EXPECT_TRUE(inst.IsSuppressed(physical));
    inst.Release({ logic });
    MEDIA_INFO_LOG("media_notify_suppress_basic_003 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_004
 * @tc.name      : Release restores normal processing
 * @tc.desc      : 解除后查询不再命中
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_004 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/c.mp4");
    inst.Suppress({ path });
    EXPECT_TRUE(inst.IsSuppressed(path));
    inst.Release({ path });
    EXPECT_FALSE(inst.IsSuppressed(path));
    EXPECT_FALSE(inst.IsSuppressed(LakeLogicPath("Pictures/c.mp4")));
    MEDIA_INFO_LOG("media_notify_suppress_basic_004 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_005
 * @tc.name      : Release by logic form removes physical form entry
 * @tc.desc      : 登记物理路径, 用逻辑路径Release也能解除(双向展开)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_005 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string physical = LakePhysicalPath("Pictures/d.mp4");
    std::string logic = LakeLogicPath("Pictures/d.mp4");
    inst.Suppress({ physical });
    inst.Release({ logic });
    EXPECT_FALSE(inst.IsSuppressed(physical));
    EXPECT_FALSE(inst.IsSuppressed(logic));
    MEDIA_INFO_LOG("media_notify_suppress_basic_005 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_006
 * @tc.name      : Unregistered path is not suppressed
 * @tc.desc      : 未登记路径查询不命中, 不能误伤其他文件的通知
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_006 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    inst.Suppress({ LakePhysicalPath("Pictures/e.mp4") });
    EXPECT_FALSE(inst.IsSuppressed(LakePhysicalPath("Pictures/e_other.mp4")));
    EXPECT_FALSE(inst.IsSuppressed(NORMAL_PATH_PREFIX + "x.mp4"));
    EXPECT_FALSE(inst.IsSuppressed(""));
    MEDIA_INFO_LOG("media_notify_suppress_basic_006 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_007
 * @tc.name      : Trailing slash is normalized
 * @tc.desc      : 尾斜杠归一化: 登记"path/", 查询"path"应命中; 反向亦然
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_007 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/f.mp4");
    inst.Suppress({ path + "/" });
    EXPECT_TRUE(inst.IsSuppressed(path));
    EXPECT_TRUE(inst.IsSuppressed(path + "/"));
    inst.Release({ path + "/" });
    EXPECT_FALSE(inst.IsSuppressed(path));
    MEDIA_INFO_LOG("media_notify_suppress_basic_007 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_008
 * @tc.name      : Suppress multiple paths at once
 * @tc.desc      : 一次登记多条路径(src+dst), 各自独立命中
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_008 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string src = NORMAL_PATH_PREFIX + "trash/g.mp4";       // 湖外回收站src
    std::string dst = LakePhysicalPath("Pictures/g.mp4");       // 湖内dst
    inst.Suppress({ src, dst });
    EXPECT_TRUE(inst.IsSuppressed(src));
    EXPECT_TRUE(inst.IsSuppressed(dst));
    EXPECT_TRUE(inst.IsSuppressed(LakeLogicPath("Pictures/g.mp4")));
    inst.Release({ src, dst });
    EXPECT_FALSE(inst.IsSuppressed(src));
    EXPECT_FALSE(inst.IsSuppressed(dst));
    MEDIA_INFO_LOG("media_notify_suppress_basic_008 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_009
 * @tc.name      : Duplicate Suppress on same path is idempotent
 * @tc.desc      : 同一路径重复登记为覆盖语义, 不产生重复条目, Release一次即可解除
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_009 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/h.mp4");
    inst.Suppress({ path });
    inst.Suppress({ path });
    EXPECT_EQ(inst.suppressed_.size(), 2u); // 物理+逻辑两种形态各一条
    inst.Release({ path });
    EXPECT_FALSE(inst.IsSuppressed(path));
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_basic_009 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_010
 * @tc.name      : Release unregistered path is a no-op
 * @tc.desc      : 对未登记路径/空串/空列表的Release不影响已有登记，正常Release后完全解除
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_010 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string registered = LakePhysicalPath("Pictures/registered.mp4");
    inst.Suppress({ registered });
    inst.Release({ LakePhysicalPath("not_registered.mp4") });
    inst.Release({ "" });
    inst.Release({});
    EXPECT_TRUE(inst.IsSuppressed(registered));
    EXPECT_EQ(inst.suppressed_.size(), 2u);
    inst.Release({ registered });
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_basic_010 end");
}

/**
 * @tc.number    : media_notify_suppress_basic_011
 * @tc.name      : Suppress empty path list is a no-op
 * @tc.desc      : 空路径列表/空字符串登记不崩溃且不产生条目
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_basic_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_basic_011 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    inst.Suppress({});
    inst.Suppress({ "" });
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_basic_011 end");
}

/**
 * @tc.number    : media_notify_suppress_stats_001
 * @tc.name      : Skipped msg count accumulates on hit only
 * @tc.desc      : 跳过计数: IsSuppressed命中时累计, 未命中/超时不累加(可维性: Release日志带累计值)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_stats_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_stats_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/count.mp4");
    uint64_t before = inst.GetSkippedMsgCount();
    inst.Suppress({ path });
    // 命中2次(物理/逻辑形态各计1次)
    EXPECT_TRUE(inst.IsSuppressed(path));
    EXPECT_TRUE(inst.IsSuppressed(LakeLogicPath("Pictures/count.mp4")));
    EXPECT_EQ(inst.GetSkippedMsgCount(), before + 2);
    // 未命中不计数
    EXPECT_FALSE(inst.IsSuppressed(LakePhysicalPath("Pictures/other.mp4")));
    EXPECT_FALSE(inst.IsSuppressed(""));
    EXPECT_EQ(inst.GetSkippedMsgCount(), before + 2);
    // 解除后不再命中也不计数
    inst.Release({ path });
    EXPECT_FALSE(inst.IsSuppressed(path));
    EXPECT_EQ(inst.GetSkippedMsgCount(), before + 2);
    MEDIA_INFO_LOG("media_notify_suppress_stats_001 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_008
 * @tc.name      : AddPaths after Release is rejected
 * @tc.desc      : 误用防护: Release后调用AddPaths不再登记(防止污染paths_且泄漏到超时兜底)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_008 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/late.mp4");
    std::string latePath = LakePhysicalPath("Pictures/late_add.mp4");
    MediaNotifySuppressScope scope({ path });
    scope.Release();
    EXPECT_FALSE(inst.IsSuppressed(path));
    scope.AddPaths({ latePath });                 // Release后误用
    EXPECT_FALSE(inst.IsSuppressed(latePath));    // 不得重新登记
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_scope_008 end");
}

/* ==================== B. 超时兜底(租约机制) ==================== */

/**
 * @tc.number    : media_notify_suppress_timeout_001
 * @tc.name      : Expired entry fails safe and is lazily cleaned
 * @tc.desc      : 注入过期时间: 漏Release的登记条目到期后查询失效(fail-safe方向为恢复通知处理),
 *                 且查询侧惰性清理条目
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_timeout_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_timeout_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/i.mp4");
    inst.Suppress({ path });
    // 直接注入过期时间(模拟超过10分钟未Release);
    // 湖内物理路径Suppress会登记物理+逻辑两种形态，需对全部展开形态注入过期
    for (const auto &form : FileScanUtils::ExpandPathForms(path)) {
        inst.suppressed_[form].expireAtMs = 1;
    }
    inst.suppressed_[path].expireAtMs = 1;
    EXPECT_FALSE(inst.IsSuppressed(path));
    // 过期条目被查询侧惰性清理
    EXPECT_EQ(inst.suppressed_.count(path), 0u);
    MEDIA_INFO_LOG("media_notify_suppress_timeout_001 end");
}

/**
 * @tc.number    : media_notify_suppress_timeout_002
 * @tc.name      : Re-suppress after expiration takes effect again
 * @tc.desc      : 过期失效后重新登记应重新生效
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_timeout_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_timeout_002 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/j.mp4");
    inst.Suppress({ path });
    // 湖内物理路径Suppress登记物理+逻辑两种形态，需对全部展开形态注入过期
    for (const auto &form : FileScanUtils::ExpandPathForms(path)) {
        inst.suppressed_[form].expireAtMs = 1;
    }
    inst.suppressed_[path].expireAtMs = 1;
    EXPECT_FALSE(inst.IsSuppressed(path));
    inst.Suppress({ path });
    EXPECT_TRUE(inst.IsSuppressed(path));
    inst.Release({ path });
    MEDIA_INFO_LOG("media_notify_suppress_timeout_002 end");
}

/* ==================== C. Scope (RAII) ==================== */

/**
 * @tc.number    : media_notify_suppress_scope_001
 * @tc.name      : Scope registers on construction and releases explicitly
 * @tc.desc      : Scope构造登记, 显式Release解除
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    {
        MediaNotifySuppressScope scope({ LakePhysicalPath("Pictures/k.mp4") });
        EXPECT_TRUE(inst.IsSuppressed(LakePhysicalPath("Pictures/k.mp4")));
        scope.Release();
        EXPECT_FALSE(inst.IsSuppressed(LakePhysicalPath("Pictures/k.mp4")));
    }
    MEDIA_INFO_LOG("media_notify_suppress_scope_001 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_002
 * @tc.name      : Scope destructor releases when Release is not called
 * @tc.desc      : 失败路径兜底: 业务中途return未调Release, Scope析构自动解除(防止漏摘牌)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_002 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string src = NORMAL_PATH_PREFIX + "trash/l.mp4";
    std::string dst = LakePhysicalPath("Pictures/l.mp4");
    // 模拟业务失败路径: 函数中途return, scope由析构兜底
    auto mockBusiness = [&src, &dst]() {
        MediaNotifySuppressScope scope({ src, dst });
        return -1; // 提前返回, 未调Release
    };
    EXPECT_EQ(mockBusiness(), -1);
    EXPECT_FALSE(inst.IsSuppressed(src));
    EXPECT_FALSE(inst.IsSuppressed(dst));
    EXPECT_FALSE(inst.IsSuppressed(LakeLogicPath("Pictures/l.mp4")));
    MEDIA_INFO_LOG("media_notify_suppress_scope_002 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_003
 * @tc.name      : Double Release is idempotent
 * @tc.desc      : Release调用两次不产生副作用, 析构时不再重复解除
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_003 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/m.mp4");
    {
        MediaNotifySuppressScope scope({ path });
        scope.Release();
        scope.Release(); // 幂等
        EXPECT_FALSE(inst.IsSuppressed(path));
    } // 析构: 已released_, 不再重复Release
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_scope_003 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_004
 * @tc.name      : AddPaths registers extra paths into the window
 * @tc.desc      : 窗口创建后新增确定的路径(重命名后dst/动态照片源路径)补充登记并命中
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_004 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string src = NORMAL_PATH_PREFIX + "trash/n.mp4";
    std::string renamedDst = LakePhysicalPath("Pictures/n(1).mp4"); // 同名冲突重命名后的最终dst
    std::string movingPhotoPath = NORMAL_PATH_PREFIX + "trash/n.mp4.mp4"; // 动态照片视频部分
    {
        MediaNotifySuppressScope scope({ src });
        EXPECT_FALSE(inst.IsSuppressed(renamedDst));
        scope.AddPaths({ renamedDst, movingPhotoPath });
        EXPECT_TRUE(inst.IsSuppressed(renamedDst));
        EXPECT_TRUE(inst.IsSuppressed(movingPhotoPath));
        EXPECT_TRUE(inst.IsSuppressed(src));
        scope.Release();
    }
    EXPECT_FALSE(inst.IsSuppressed(src));
    EXPECT_FALSE(inst.IsSuppressed(renamedDst));
    EXPECT_FALSE(inst.IsSuppressed(movingPhotoPath));
    MEDIA_INFO_LOG("media_notify_suppress_scope_004 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_005
 * @tc.name      : Scope with empty paths is safe
 * @tc.desc      : 空路径列表的Scope构造/析构不崩溃
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_005 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    {
        MediaNotifySuppressScope scope({});
        scope.AddPaths({});
        EXPECT_TRUE(inst.suppressed_.empty());
    }
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_scope_005 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_006
 * @tc.name      : Scope covers both dual forms of lake path
 * @tc.desc      : Scope登记湖内路径后, 逻辑/物理两种形态均处于抑制窗口内
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_006 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    {
        MediaNotifySuppressScope scope({ LakePhysicalPath("Videos/o.mp4") });
        EXPECT_TRUE(inst.IsSuppressed(LakePhysicalPath("Videos/o.mp4")));
        EXPECT_TRUE(inst.IsSuppressed(LakeLogicPath("Videos/o.mp4")));
    }
    EXPECT_FALSE(inst.IsSuppressed(LakePhysicalPath("Videos/o.mp4")));
    EXPECT_FALSE(inst.IsSuppressed(LakeLogicPath("Videos/o.mp4")));
    MEDIA_INFO_LOG("media_notify_suppress_scope_006 end");
}

/**
 * @tc.number    : media_notify_suppress_scope_007
 * @tc.name      : Moved-from scope does not release on destruction
 * @tc.desc      : 移动构造后: 源对象析构不得误解除登记(源对象released_置位),
 *                 目标对象Release时才真正解除
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_scope_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_scope_007 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string path = LakePhysicalPath("Pictures/z.mp4");
    {
        MediaNotifySuppressScope first({ path });
        EXPECT_TRUE(inst.IsSuppressed(path));
        MediaNotifySuppressScope second(std::move(first)); // 移动: 登记内容转移
        // 源对象析构(块结束时)不得解除登记, second 仍持有窗口
        EXPECT_TRUE(inst.IsSuppressed(path));
        EXPECT_TRUE(inst.IsSuppressed(LakeLogicPath("Pictures/z.mp4")));
        second.Release(); // 目标对象显式解除
    }
    EXPECT_FALSE(inst.IsSuppressed(path));
    EXPECT_FALSE(inst.IsSuppressed(LakeLogicPath("Pictures/z.mp4")));
    MEDIA_INFO_LOG("media_notify_suppress_scope_007 end");
}

/* ==================== D. FileScanUtils::ExpandPathForms (路径形态) ==================== */

/**
 * @tc.number    : expand_path_forms_lake_physical_001
 * @tc.name      : Lake physical path expands to logic form
 * @tc.desc      : 湖内物理路径展开为[自身, 逻辑形态]
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_lake_physical_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_lake_physical_001 start");
    std::string physical = LakePhysicalPath("Pictures/p.mp4");
    auto forms = FileScanUtils::ExpandPathForms(physical);
    EXPECT_EQ(forms.size(), 2u);
    EXPECT_EQ(forms[0], physical);
    EXPECT_EQ(forms[1], LakeLogicPath("Pictures/p.mp4"));
    MEDIA_INFO_LOG("expand_path_forms_lake_physical_001 end");
}

/**
 * @tc.number    : expand_path_forms_lake_logic_002
 * @tc.name      : Lake logic path expands to physical form
 * @tc.desc      : 湖内逻辑路径展开为[自身, 物理形态]
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_lake_logic_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_lake_logic_002 start");
    std::string logic = LakeLogicPath("Pictures/q.mp4");
    auto forms = FileScanUtils::ExpandPathForms(logic);
    EXPECT_EQ(forms.size(), 2u);
    EXPECT_EQ(forms[0], logic);
    EXPECT_EQ(forms[1], LakePhysicalPath("Pictures/q.mp4"));
    MEDIA_INFO_LOG("expand_path_forms_lake_logic_002 end");
}

/**
 * @tc.number    : expand_path_forms_file_manager_physical_003
 * @tc.name      : FileManager physical path returns as-is (not intercepted yet)
 * @tc.desc      : 文管Docs物理路径当前不做互转(预留扩展), 仅返回归一化自身
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_file_manager_physical_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_file_manager_physical_003 start");
    std::string physical = FILE_MANAGER_PHYSICAL_PREFIX + "doc.pdf";
    auto forms = FileScanUtils::ExpandPathForms(physical);
    EXPECT_EQ(forms.size(), 1u);
    EXPECT_EQ(forms[0], physical);
    MEDIA_INFO_LOG("expand_path_forms_file_manager_physical_003 end");
}

/**
 * @tc.number    : expand_path_forms_file_manager_logic_004
 * @tc.name      : FileManager logic path returns as-is (not intercepted yet)
 * @tc.desc      : 文管Docs逻辑路径当前不做互转(预留扩展), 仅返回归一化自身
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_file_manager_logic_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_file_manager_logic_004 start");
    std::string logic = FileManagerLogicPrefix() + "doc.pdf";
    auto forms = FileScanUtils::ExpandPathForms(logic);
    EXPECT_EQ(forms.size(), 1u);
    EXPECT_EQ(forms[0], logic);
    MEDIA_INFO_LOG("expand_path_forms_file_manager_logic_004 end");
}

/**
 * @tc.number    : expand_path_forms_normal_005
 * @tc.name      : Normal path expands to itself only
 * @tc.desc      : 非湖内/非Docs路径仅返回归一化自身, 不做转换
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_normal_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_normal_005 start");
    auto forms = FileScanUtils::ExpandPathForms(NORMAL_PATH_PREFIX + "r.mp4");
    EXPECT_EQ(forms.size(), 1u);
    EXPECT_EQ(forms[0], NORMAL_PATH_PREFIX + "r.mp4");
    MEDIA_INFO_LOG("expand_path_forms_normal_005 end");
}

/**
 * @tc.number    : expand_path_forms_empty_006
 * @tc.name      : Empty path expands to empty vector
 * @tc.desc      : 空串/纯斜杠输入返回空结果
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_empty_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_empty_006 start");
    EXPECT_TRUE(FileScanUtils::ExpandPathForms("").empty());
    EXPECT_TRUE(FileScanUtils::ExpandPathForms("/").empty());
    EXPECT_TRUE(FileScanUtils::ExpandPathForms("///").empty());
    MEDIA_INFO_LOG("expand_path_forms_empty_006 end");
}

/**
 * @tc.number    : expand_path_forms_slash_007
 * @tc.name      : Trailing slash is stripped in forms
 * @tc.desc      : 展开结果统一去尾斜杠
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_slash_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_slash_007 start");
    auto forms = FileScanUtils::ExpandPathForms(LakePhysicalPath("Pictures/s.mp4") + "/");
    EXPECT_EQ(forms.size(), 2u);
    EXPECT_EQ(forms[0], LakePhysicalPath("Pictures/s.mp4"));
    EXPECT_EQ(forms[1], LakeLogicPath("Pictures/s.mp4"));
    MEDIA_INFO_LOG("expand_path_forms_slash_007 end");
}

/**
 * @tc.number    : expand_path_forms_lake_priority_008
 * @tc.name      : Lake prefix has priority over FileManager Docs prefix
 * @tc.desc      : 湖内路径(HO_DATA_EXT_MISC)不被误判为FileManager Docs路径:
 *                 湖内物理前缀本身包含Docs前缀, 必须优先命中湖内映射
 */
HWTEST_F(MediaNotifySuppressTest, expand_path_forms_lake_priority_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("expand_path_forms_lake_priority_008 start");
    std::string lakePhysical = LakePhysicalPath("Pictures/t.mp4");
    auto forms = FileScanUtils::ExpandPathForms(lakePhysical);
    // 应产生湖内逻辑形态.正确的湖内逻辑前缀本身字面上包含文管Docs逻辑前缀，
    // 因此正确的forms[1]与"Docs逻辑前缀+HO_DATA_EXT_MISC"形态在字符串层面完全不可区分，
    // 负向断言不可用，防护由正向断言承担；若湖内前缀映射退化为不匹配，forms.size()变为1即失败
    EXPECT_EQ(forms.size(), 2u);
    EXPECT_EQ(forms[0], lakePhysical);
    EXPECT_EQ(forms[1], LakeLogicPath("Pictures/t.mp4"));
    MEDIA_INFO_LOG("expand_path_forms_lake_priority_008 end");
}

/* ==================== E. monitor集成: ProcessSingleFileChange跳过 ==================== */

/**
 * @tc.number    : media_notify_suppress_processor_001
 * @tc.name      : Suppressed fileUri is skipped in ProcessSingleFileChange
 * @tc.desc      : 抑制窗口内的通知(fileUri命中湖内逻辑路径)被跳过, 处理器早退不崩溃
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_processor_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_processor_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    processor->StartProcessMsgs();

    std::string logicUri = LakeLogicPath("Pictures/v.mp4");
    inst.Suppress({ LakePhysicalPath("Pictures/v.mp4") }); // 业务侧登记物理形态
    FileMonitorService::FileMsgModel model;
    model.isFile = true;
    model.isMediaData = FileMonitorService::BIN_10;
    model.opType = static_cast<int32_t>(FileNotifyOperationType::ADD);
    model.fileUri = logicUri; // monitor上报逻辑形态
    uint64_t before = inst.GetSkippedMsgCount();
    processor->ProcessSingleFileChange(model); // 应跳过, 不进入扫描处理链
    EXPECT_EQ(inst.GetSkippedMsgCount(), before + 1);
    inst.Release({ LakePhysicalPath("Pictures/v.mp4") });
    EXPECT_FALSE(inst.IsSuppressed(logicUri));
    MEDIA_INFO_LOG("media_notify_suppress_processor_001 end");
}

/**
 * @tc.number    : media_notify_suppress_processor_002
 * @tc.name      : Suppressed oldFileUri is skipped in ProcessSingleFileChange
 * @tc.desc      : MOD类通知的oldFileUri命中抑制路径时同样跳过(rename场景的beforePath)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_processor_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_processor_002 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    processor->StartProcessMsgs();

    std::string logicUri = LakeLogicPath("Pictures/w.mp4");
    inst.Suppress({ LakePhysicalPath("Pictures/w.mp4") });
    FileMonitorService::FileMsgModel model;
    model.isFile = true;
    model.isMediaData = FileMonitorService::BIN_10;
    model.opType = static_cast<int32_t>(FileNotifyOperationType::MOD);
    model.fileUri = LakeLogicPath("Pictures/w_renamed.mp4");
    model.oldFileUri = logicUri; // beforePath命中抑制
    uint64_t before = inst.GetSkippedMsgCount();
    processor->ProcessSingleFileChange(model); // 应跳过
    EXPECT_EQ(inst.GetSkippedMsgCount(), before + 1);
    inst.Release({ LakePhysicalPath("Pictures/w.mp4") });
    EXPECT_FALSE(inst.IsSuppressed(logicUri));
    MEDIA_INFO_LOG("media_notify_suppress_processor_002 end");
}

/**
 * @tc.number    : media_notify_suppress_processor_003
 * @tc.name      : Unsuppressed notify goes through original logic
 * @tc.desc      : 未抑制的通知正常进入原有处理链(非湖内路径早退), 不受框架影响
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_processor_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_processor_003 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    auto processor = MediaFileChangeProcessor::GetInstance();
    processor->StartProcessMsgs();

    inst.Suppress({ LakePhysicalPath("Pictures/x.mp4") });
    FileMonitorService::FileMsgModel model;
    model.isFile = true;
    model.isMediaData = FileMonitorService::BIN_10;
    model.opType = static_cast<int32_t>(FileNotifyOperationType::ADD);
    model.fileUri = NORMAL_PATH_PREFIX + "y.mp4"; // 非抑制路径, 正常处理(非湖内早退)
    // 非抑制路径不命中跳过计数
    uint64_t before = inst.GetSkippedMsgCount();
    processor->ProcessSingleFileChange(model);
    uint64_t afterNormal = inst.GetSkippedMsgCount();
    EXPECT_EQ(afterNormal, before);

    // 空fileUri同样不受影响(IsSuppressed对空串早退，不计数)
    model.fileUri = "";
    processor->ProcessSingleFileChange(model);
    EXPECT_EQ(inst.GetSkippedMsgCount(), afterNormal);
    inst.Release({ LakePhysicalPath("Pictures/x.mp4") });
    EXPECT_FALSE(inst.IsSuppressed(LakeLogicPath("Pictures/x.mp4")));
    MEDIA_INFO_LOG("media_notify_suppress_processor_003 end");
}

/* ==================== F. 并发 ==================== */

/**
 * @tc.number    : media_notify_suppress_concurrent_001
 * @tc.name      : Concurrent Suppress/IsSuppressed/Release is thread safe
 * @tc.desc      : 多线程并发登记/查询/解除无数据竞争, 结束后状态一致
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_concurrent_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_concurrent_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    constexpr int32_t THREAD_NUM = 4;
    constexpr int32_t LOOP_TIMES = 100;
    std::vector<std::thread> workers;
    for (int32_t t = 0; t < THREAD_NUM; t++) {
        workers.emplace_back([&inst, t]() {
            std::string path = LakePhysicalPath("Concurrent/p" + std::to_string(t) + ".mp4");
            for (int32_t i = 0; i < LOOP_TIMES; i++) {
                inst.Suppress({ path });
                (void)inst.IsSuppressed(path);
                (void)inst.IsSuppressed(LakeLogicPath("Concurrent/p" + std::to_string(t) + ".mp4"));
                inst.Release({ path });
            }
        });
    }
    // 交叉的只读线程
    std::thread reader([&inst]() {
        for (int32_t i = 0; i < LOOP_TIMES; i++) {
            (void)inst.IsSuppressed(LakePhysicalPath("Concurrent/other.mp4"));
        }
    });
    for (auto &worker : workers) {
        worker.join();
    }
    reader.join();
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_concurrent_001 end");
}

/* ==================== G. 边界与压力 ==================== */

/**
 * @tc.number    : media_notify_suppress_long_path_001
 * @tc.name      : Very long path is handled safely
 * @tc.desc      : 超长路径(8KB)登记/查询/解除不崩溃
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_long_path_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_long_path_001 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string longName(8 * 1024, 'a');
    std::string longPath = LakePhysicalPath(longName + ".mp4");
    inst.Suppress({ longPath });
    EXPECT_TRUE(inst.IsSuppressed(longPath));
    EXPECT_TRUE(inst.IsSuppressed(LakeLogicPrefix() + longName + ".mp4"));
    inst.Release({ longPath });
    EXPECT_FALSE(inst.IsSuppressed(longPath));
    MEDIA_INFO_LOG("media_notify_suppress_long_path_001 end");
}

/**
 * @tc.number    : media_notify_suppress_special_chars_002
 * @tc.name      : Path with special characters is handled safely
 * @tc.desc      : 含中文/空格/特殊字符的路径正常工作
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_special_chars_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_special_chars_002 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string specialPath = LakePhysicalPath("Pictures/照片 (1)#&%.mp4");
    inst.Suppress({ specialPath });
    EXPECT_TRUE(inst.IsSuppressed(specialPath));
    EXPECT_TRUE(inst.IsSuppressed(LakeLogicPrefix() + "Pictures/照片 (1)#&%.mp4"));
    inst.Release({ specialPath });
    EXPECT_FALSE(inst.IsSuppressed(specialPath));
    MEDIA_INFO_LOG("media_notify_suppress_special_chars_002 end");
}

/**
 * @tc.number    : media_notify_suppress_many_paths_003
 * @tc.name      : Bulk register and release paths
 * @tc.desc      : 批量登记/解除1000条路径正确且高效(批量恢复场景)
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_many_paths_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_many_paths_003 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    constexpr int32_t PATH_COUNT = 1000;
    std::vector<std::string> paths;
    paths.reserve(PATH_COUNT);
    for (int32_t i = 0; i < PATH_COUNT; i++) {
        paths.push_back(LakePhysicalPath("Bulk/p" + std::to_string(i) + ".mp4"));
    }
    inst.Suppress(paths);
    for (int32_t i = 0; i < PATH_COUNT; i += 100) { // 抽查
        EXPECT_TRUE(inst.IsSuppressed(paths[i]));
    }
    inst.Release(paths);
    for (int32_t i = 0; i < PATH_COUNT; i += 100) {
        EXPECT_FALSE(inst.IsSuppressed(paths[i]));
    }
    EXPECT_TRUE(inst.suppressed_.empty());
    MEDIA_INFO_LOG("media_notify_suppress_many_paths_003 end");
}

/**
 * @tc.number    : media_notify_suppress_mixed_scenarios_004
 * @tc.name      : Mixed lake and normal paths coexist
 * @tc.desc      : 混合场景: 湖内src/dst与普通路径并存, 仅命中登记路径, 互不误伤
 */
HWTEST_F(MediaNotifySuppressTest, media_notify_suppress_mixed_scenarios_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("media_notify_suppress_mixed_scenarios_004 start");
    auto &inst = MediaNotifySuppressManager::GetInstance();
    std::string lakeDst = LakePhysicalPath("Pictures/mix.mp4");
    std::string trashSrc = NORMAL_PATH_PREFIX + "trash/mix.mp4";
    std::string otherLake = LakePhysicalPath("Pictures/mix_other.mp4");
    {
        MediaNotifySuppressScope scope({ lakeDst, trashSrc });
        EXPECT_TRUE(inst.IsSuppressed(lakeDst));
        EXPECT_TRUE(inst.IsSuppressed(LakeLogicPath("Pictures/mix.mp4")));
        EXPECT_TRUE(inst.IsSuppressed(trashSrc));
        EXPECT_FALSE(inst.IsSuppressed(otherLake));
        EXPECT_FALSE(inst.IsSuppressed(NORMAL_PATH_PREFIX + "other.mp4"));
        scope.Release();
    }
    EXPECT_FALSE(inst.IsSuppressed(lakeDst));
    EXPECT_FALSE(inst.IsSuppressed(trashSrc));
    MEDIA_INFO_LOG("media_notify_suppress_mixed_scenarios_004 end");
}
} // namespace Media
} // namespace OHOS
