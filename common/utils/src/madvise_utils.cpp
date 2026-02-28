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
#define MLOG_TAG "Madvise_Utils"

#include "madvise_utils.h"

#include <cstring>
#include <mutex>
#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include "media_log.h"

namespace OHOS::Media {
namespace {
/**
 * @brief 全局互斥锁
 *
 * 用于保护 dl_iterate_phdr 的调用，防止：
 * - 多线程并发遍历 linker 内部结构
 * - 与潜在的 dlopen / dlclose 并发引发 UB
 */
std::mutex gDlIteratePhdrMutex;
} // anonymous namespace
/**
 * @brief 获取系统页大小（Page Size）
 *
 * 原理说明：
 * - Linux 虚拟内存管理以“页”为最小管理单位
 * - madvise / mmap / munmap 等系统调用均要求页粒度对齐
 * - getpagesize() 是一个 syscall，理论上有开销
 *
 * 设计要点：
 * - 使用 static 局部变量缓存页大小
 * - 确保全进程生命周期内只调用一次 syscall
 *
 * @return 系统页大小（通常为 4096 字节）
 */
size_t MadviseUtils::PageSize()
{
    static const size_t pageSize = static_cast<size_t>(::getpagesize());
    return pageSize;
}

/**
 * @brief 判断 ELF LOAD 段是否适合做 madvise 回收
 *
 * 原理说明：
 * - 只对「可读 + 不可写」段做 madvise
 * - 典型包括：
 *   - .text  (r-x)  可执行代码段
 *   - .rodata (r--) 只读常量段
 *
 * 不能回收的段：
 * - 可写段（RW）：
 *   - 可能包含 heap / GOT / relocation
 *   - madvise(DONTNEED) 会导致数据丢失或重建成本极高
 *
 * 关键原则：
 * - 宁可少回收，也不能破坏进程语义
 *
 * @param flags ELF Program Header 的 p_flags
 * @return true 表示该段可安全优化
 */
bool MadviseUtils::ShouldOptimizeSegment(uint32_t flags)
{
    const bool readable = (flags & PF_R) != 0;
    const bool writable = (flags & PF_W) != 0;
    return readable && !writable;
}

/**
 * @brief 对一段虚拟地址区间执行 madvise（自动页对齐）
 *
 * 【工具核心函数】
 *
 * -------------------- 原理说明 --------------------
 *
 * 1️.为什么必须页对齐？
 * - Linux 内核的 VMA（虚拟内存区域）管理以 page 为单位
 * - madvise 的实际作用对象是：
 *   [page_start, page_end)
 *
 * - 若 addr / len 未对齐：
 *   - 内核要么直接 EINVAL
 *   - 要么隐式向下 / 向上取整，行为不可控
 *
 * 2️.对齐策略（这是正确做法）
 *
 *   原始区间：
 *     [addr, addr + len)
 *
 *   对齐后区间：
 *     [floor(addr / page) * page,
 *      ceil((addr + len) / page) * page)
 *
 *   这样可以确保：
 *   - 完整覆盖原始区间
 *   - 不遗漏任何一个可能被访问的 page
 *
 * 3️.为什么允许“多 madvise 一点”？
 * - 多出来的页一定属于：
 *   - 同一个 VMA
 *   - 同一个 ELF LOAD segment
 * - 不会越界到不相关内存
 *
 * 4️.MADV_DONTNEED 的真实语义
 * - 并不是立即释放
 * - 而是：
 *   - 丢弃当前物理页
 *   - 下次访问触发 page fault
 *   - 从文件重新映射（只读段）或 zero-fill
 *
 * -------------------- 设计保证 --------------------
 * - 不破坏程序语义
 * - 可重复调用（幂等）
 * - 在低内存场景可显著降低 RSS / PSS
 *
 * @param addr 段起始虚拟地址（不要求对齐）
 * @param len  段长度（字节）
 * @return true 表示 madvise 成功
 */
bool MadviseUtils::ApplyMadviseAligned(void *addr, size_t len)
{
    if (addr == nullptr || len == 0) {
        return false;
    }

    const size_t pageSize = PageSize();
    auto start = reinterpret_cast<uintptr_t>(addr);
    auto alignedStart = start & ~(pageSize - 1);

    auto end = start + len;
    auto alignedEnd = (end + pageSize - 1) & ~(pageSize - 1);

    const size_t alignedLen = alignedEnd - alignedStart;

    const int32_t ret = ::madvise(reinterpret_cast<void *>(alignedStart), alignedLen, MADV_DONTNEED);
    if (ret == MADVISE_SUCCESS) {
        MEDIA_INFO_LOG("madvise success: addr=%{public}p len=%{public}zu", reinterpret_cast<void *>(alignedStart),
                       alignedLen);
        return true;
    }

    MEDIA_ERR_LOG("madvise failed: addr=%{public}p len=%{public}zu errno=%{public}d",
                  reinterpret_cast<void *>(alignedStart), alignedLen, errno);
    return false;
}

/**
 * @brief dl_iterate_phdr 回调（单库模式）
 *
 * 原理说明：
 * - dl_iterate_phdr 会遍历当前进程中
 *   所有已加载 ELF 对象（主程序 + so）
 *
 * - 每个 dl_phdr_info 对应一个 ELF 映像
 * - info->dlpi_phdr 描述的是「Program Header」
 *
 * 为什么用 PHDR 而不是 /proc/self/maps？
 * - PHDR 是 loader 级信息
 * - 精确对应 ELF LOAD segment
 * - 无需字符串解析，性能极高（~0.1ms）
 *
 * 回调流程：
 * 1. 匹配目标 so
 * 2. 遍历所有 PT_LOAD 段
 * 3. 对只读段做 madvise
 *
 * @return 永远返回 0（继续遍历）
 */
int32_t MadviseUtils::PhdrCallbackSingle(struct dl_phdr_info *info, size_t, void *data)
{
    auto *ctx = static_cast<SingleLibContext *>(data);

    CHECK_AND_RETURN_RET_LOG(info != nullptr && ctx != nullptr, 0, "info is nullptr or ctx is nullptr");
    const char *libName = info->dlpi_name;

    if (libName == nullptr || std::strlen(libName) == 0) {
        libName = UNKNOWN_LIB_NAME;
    }

    if (libName == nullptr || ctx->targetLib.empty() ||
        std::strstr(libName, ctx->targetLib.c_str()) == nullptr) {
        return 0;
    }

    MEDIA_INFO_LOG("Processing library: %{public}s", libName);

    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr) &phdr = info->dlpi_phdr[i];

        if (phdr.p_type != PT_LOAD || !ShouldOptimizeSegment(phdr.p_flags)) {
            continue;
        }

        void *segmentAddr =
            reinterpret_cast<void *>(info->dlpi_addr + phdr.p_vaddr);

        if (ApplyMadviseAligned(segmentAddr, phdr.p_memsz)) {
            ++ctx->successCount;
        } else {
            ++ctx->failCount;
        }
    }

    return 0;
}

/**
 * @brief 对单个已加载 so 执行 madvise 优化
 *
 * 行为说明：
 * - 基于 dl_iterate_phdr
 * - 支持 so 名称子串匹配
 * - 至少一个段成功即返回 true
 *
 * 典型使用场景：
 * - so 初始化完成后
 * - 冷路径代码 / 常量段长期不再访问
 * - 希望尽快降低 PSS / RSS
 *
 * 注意：
 * - 不会卸载 so
 * - 不影响 dlopen / dlsym
 */
bool MadviseUtils::MadviseSingleLibrary(const std::string &libName)
{
    if (libName.empty()) {
        MEDIA_ERR_LOG("Empty library name");
        return false;
    }

    std::lock_guard<std::mutex> lock(gDlIteratePhdrMutex);

    SingleLibContext ctx;
    ctx.targetLib.assign(libName);

    ::dl_iterate_phdr(PhdrCallbackSingle, &ctx);

    MEDIA_INFO_LOG("madvise_single_library done: success=%{public}d fail=%{public}d",
                   ctx.successCount, ctx.failCount);

    return ctx.successCount > 0;
}

/**
 * @brief dl_iterate_phdr 回调（多库模式）
 *
 * 设计目的：
 * - 只遍历一次 dl_iterate_phdr
 * - 同时处理多个目标 so
 *
 * 关键细节：
 * - 使用 basename 匹配（libxxx.so）
 * - processedLibs 防止：
 *   - 同一个 so 被多次统计
 *   - 多个映射路径重复计算
 *
 * 成功判定标准：
 * - 只要一个 LOAD 段 madvise 成功
 * - 即认为该 so 优化成功
 */
int32_t MadviseUtils::PhdrCallbackMultiple(struct dl_phdr_info *info, size_t, void *data)
{
    auto *ctx = static_cast<MultiLibContext *>(data);
    CHECK_AND_RETURN_RET_LOG(info != nullptr && ctx != nullptr, 0, "info is nullptr or ctx is nullptr");

    if (info->dlpi_name == nullptr || std::strlen(info->dlpi_name) == 0) {
        return 0;
    }

    const char *baseName = std::strrchr(info->dlpi_name, '/');
    baseName = (baseName == nullptr) ? info->dlpi_name : baseName + 1;

    if (ctx->targetLibs.count(baseName) == 0 ||
        ctx->processedLibs.count(baseName) != 0) {
        MEDIA_DEBUG_LOG("cannot find lib in targetlibs or processedLibs, basename:%{public}s", baseName);
        return 0;
    }

    ctx->processedLibs.insert(baseName);

    int32_t segmentSuccess = 0;

    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr) &phdr = info->dlpi_phdr[i];

        if (phdr.p_type != PT_LOAD || !ShouldOptimizeSegment(phdr.p_flags)) {
            MEDIA_DEBUG_LOG("skip phdr");
            continue;
        }

        void *segmentAddr =
            reinterpret_cast<void *>(info->dlpi_addr + phdr.p_vaddr);

        if (ApplyMadviseAligned(segmentAddr, phdr.p_memsz)) {
            ++segmentSuccess;
            MEDIA_INFO_LOG("madvise %{public}s success", baseName);
        }
    }

    if (segmentSuccess > 0) {
        ++ctx->successCount;
    } else {
        ++ctx->failCount;
    }

    return 0;
}

/**
 * @brief 对多个已加载 so 执行 madvise
 *
 * 相比单库模式的优势：
 * - dl_iterate_phdr 只调用一次
 * - 避免 O(N * so_count) 的遍历复杂度
 *
 * 返回值语义：
 * - 返回成功优化的 so 数量
 * - 不是段数量
 *
 * 适合场景：
 * - 冷启动后批量优化
 * - 模块化组件卸载前
 */
int32_t MadviseUtils::MadviseMultipleLibraries(const std::vector<std::string> &libNames)
{
    if (libNames.empty()) {
        MEDIA_ERR_LOG("Library list is empty");
        return 0;
    }

    std::lock_guard<std::mutex> lock(gDlIteratePhdrMutex);

    MultiLibContext ctx;
    ctx.targetLibs = std::unordered_set<std::string>(libNames.begin(), libNames.end());

    ::dl_iterate_phdr(PhdrCallbackMultiple, &ctx);

    MEDIA_INFO_LOG("madvise_multiple_libraries done: success=%{public}d fail=%{public}d",
                   ctx.successCount, ctx.failCount);

    return ctx.successCount;
}
} // namespace OHOS::Media