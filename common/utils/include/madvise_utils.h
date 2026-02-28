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

#ifndef COMMON_UTILS_MADVISE_UTILS_H_
#define COMMON_UTILS_MADVISE_UTILS_H_

#include <string>
#include <vector>
#include <unordered_set>
#include <link.h>

namespace OHOS::Media {
/**
 * @brief MadviseUtils 线程安全说明
 *
 * 线程安全策略：
 * - 本工具类【不是可重入的】
 * - 所有对 dl_iterate_phdr 的调用通过全局互斥锁串行化
 *
 * 设计原因：
 * 1. dl_iterate_phdr 内部遍历 dynamic linker 的全局数据结构
 * 2. glibc / linker 并不保证其与 dlopen / dlclose 并发安全
 * 3. 并发调用 dl_iterate_phdr 可能导致未定义行为（UB）
 *
 * 保证：
 * - 任意时刻，进程内最多只有一个线程执行 dl_iterate_phdr
 * - madvise 系统调用本身是线程安全的
 *
 * 使用约束：
 * - 本工具保证内部串行
 * - 但仍建议避免与 dlopen / dlclose 并发调用
 */
class MadviseUtils final {
public:
    MadviseUtils() = delete;
    ~MadviseUtils() = delete;

    /**
     * @brief 对单个已加载 so 应用 madvise（基于 dl_iterate_phdr）
     * @param libName so 名称（支持子串匹配）
     * @return true 至少一个段成功
     */
    static bool MadviseSingleLibrary(const std::string &libName);

    /**
     * @brief 对多个已加载 so 应用 madvise
     * @param libNames so basename 列表（如 libxxx.so）
     * @return 成功优化的库数量
     */
    static int32_t MadviseMultipleLibraries(const std::vector<std::string> &libNames);

private:
    static constexpr const char *UNKNOWN_LIB_NAME = "unknown";
    static constexpr int32_t MADVISE_SUCCESS = 0;

    // ---- ELF 回调数据结构 ----
    struct SingleLibContext {
        int32_t successCount {0};
        int32_t failCount {0};
        std::string targetLib {};
    };

    struct MultiLibContext {
        int32_t successCount {0};
        int32_t failCount {0};
        std::unordered_set<std::string> targetLibs {};
        std::unordered_set<std::string> processedLibs;
    };

    // ---- 回调函数 ----
    static int32_t PhdrCallbackSingle(struct dl_phdr_info *info, size_t size, void *data);
    static int32_t PhdrCallbackMultiple(struct dl_phdr_info *info, size_t size, void *data);

    // ---- 内部工具函数 ----
    static bool ShouldOptimizeSegment(uint32_t phdrFlags);
    static bool ApplyMadviseAligned(void *addr, size_t len);
    static size_t PageSize();
};
} // namespace OHOS::Media
#endif // COMMON_UTILS_MADVISE_UTILS_H_
