/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_COMMON_UTILS_FDSAN_FD_H
#define OHOS_MEDIA_COMMON_UTILS_FDSAN_FD_H

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

#ifndef MEDIALIBRARY_SDK_FDSAN_TAG
#define MEDIALIBRARY_SDK_FDSAN_TAG 0xD002B71
#endif

#ifndef MEDIALIBRARY_FDSAN_TAG
#define MEDIALIBRARY_FDSAN_TAG 0xD002B700
#endif

#ifndef MEDIALIBRARY_FUSE_FDSAN_TAG
#define MEDIALIBRARY_FUSE_FDSAN_TAG 0xD002B701
#endif

#ifndef MEDIALIBRARY_BACKUP_FDSAN_TAG
#define MEDIALIBRARY_BACKUP_FDSAN_TAG 0xD002B702
#endif

#ifndef MEDIALIBRARY_FDSAN_CLIENT_TAG
#define MEDIALIBRARY_FDSAN_CLIENT_TAG 0xD002B703
#endif

namespace OHOS::Media {

/**
 * @class FdsanFd
 * @brief RAII 封装文件描述符，使用 fdsan (file descriptor sanitizer) 检测 double-close/use-after-close
 *
 * 使用方式:
 * - 构造时传入有效 fd，fd 所有权转移给 FdsanFd
 * - 析构时自动关闭 fd
 * - 使用 Get() 获取 fd 副本（fd 所有权仍属于 FdsanFd）
 * - 使用 Close() 或 CloseAndReset() 显式关闭
 *
 * 无效 fd 约定:
 * - INVALID_FD = -1: 表示未持有有效 fd
 * - ZERO_FD = 0: 标准输入 fd，过滤此值及更小的值
 * - 有效 fd 范围: fd > ZERO_FD (即 fd >= 1)
 */
class FdsanFd {
private:
    /** @brief 交换 fd 的 fdsan 标签 */
    static void ExchangeTag(int32_t fd, uint64_t oldTag, uint64_t newTag);

public:
    /** @brief 为 fd 添加 fdsan 标签 */
    static void AddTag(int32_t fd, uint64_t newTag);
    /** @brief 使用 fdsan 关闭 fd */
    static int32_t Close(int32_t fd, uint64_t tag);
    static void RemoveTag(int32_t fd, uint64_t oldTag);

public:
    FdsanFd() = default;

    /** @brief 构造并接管 fd 所有权，fd 必须 > ZERO_FD */
    explicit FdsanFd(int32_t fd);

    ~FdsanFd();

    FdsanFd(const FdsanFd &copy) = delete;
    FdsanFd(FdsanFd &&move);
    FdsanFd &operator=(const FdsanFd &copy) = delete;
    FdsanFd &operator=(FdsanFd &&move);

    /** @brief 返回 fd 副本（fd 所有权仍属于 FdsanFd，析构时会关闭） */
    int32_t Get() const;

    /** @brief 关闭当前持有的 fd，设置 fd_ = INVALID_FD */
    int32_t Close();

    /**
     * @brief 关闭当前 fd 并设置新 fd
     * @param newFd 新 fd，必须 > ZERO_FD 才有效
     */
    void CloseAndReset(int32_t newFd);

    /**
     * @brief 释放当前持有的 fd 的所有权并返回
     * @return 释放前持有的 fd，若无有效 fd 则返回 INVALID_FD
     * @note 移除原 fd 的 fdsan tag，fd_ 设为 INVALID_FD，返回原 fd（未关闭）
     * @note 调用方需自行负责关闭返回的 fd（因 tag 已清除，close 不会触发 fdsan 检测）
     * @note 释放后内部 fd_ 设置为 INVALID_FD，析构时不会重复关闭
     */
    int32_t Release();

private:
    /** @brief 生成唯一的 fdsan tag（使用 this 指针地址） */
    uint64_t Tag();

    int32_t fd_ = INVALID_FD;
    /** @brief 无效 fd 值，表示未持有有效 fd */
    static constexpr int32_t INVALID_FD = -1;
    /** @brief 最小有效 fd 值（标准输入 fd = 0 被视为无效） */
    static constexpr int32_t ZERO_FD = 0;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_COMMON_UTILS_FDSAN_FD_H
