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

#ifndef MEDIA_NOTIFY_SUPPRESS_MANAGER_H
#define MEDIA_NOTIFY_SUPPRESS_MANAGER_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
/*
 * 通用文件变更通知抑制框架。
 * 仅用于调用方在窗口内自行保证文件与数据库一致性(含失败回滚)的操作
 * (如湖内资产跨挂载copy-and-delete), 禁止用于掩盖扫描判重缺陷;
 * 窗口尽量短、粒度尽量小(当前仅支持精确路径匹配);
 * 不做嵌套引用计数, 重叠路径的嵌套窗口会提前解除。
 */
class MediaNotifySuppressManager {
public:
    EXPORT static MediaNotifySuppressManager &GetInstance();

    /* 声明抑制(湖内路径自动登记逻辑/物理两种形态) */
    EXPORT void Suppress(const std::vector<std::string> &paths);

    EXPORT void Release(const std::vector<std::string> &paths);

    /* file_monitor 消费: 这条通知要不要跳过(任意路径形态入参均可命中) */
    EXPORT bool IsSuppressed(const std::string &path);

    /* 累计跳过的通知数(可维: 配合Release日志判断窗口是否生效, 无需开DEBUG级) */
    EXPORT uint64_t GetSkippedMsgCount() const;

private:
    MediaNotifySuppressManager() = default;
    /* 扩展预留: 匹配方式(当前精确匹配)/释放回调(对账扫描)将来在此结构体添字段 */
    struct Entry {
        int64_t expireAtMs{0};
    };
    mutable std::mutex mtx_;
    std::unordered_map<std::string, Entry> suppressed_;  /* key: 归一化后的路径形态 */
    std::atomic<uint64_t> skippedMsgCount_{0};           /* 累计跳过的通知数 */
};

/* 抑制窗口守卫: 构造登记, 析构自动解除(防失败路径漏解除); 新确定的路径用 AddPaths 补充 */
class MediaNotifySuppressScope {
public:
    EXPORT explicit MediaNotifySuppressScope(std::vector<std::string> paths);
    EXPORT ~MediaNotifySuppressScope();
    EXPORT void AddPaths(const std::vector<std::string> &paths);
    EXPORT void Release();
    /* 仅支持移动(跨函数传递), 禁止拷贝 */
    MediaNotifySuppressScope(const MediaNotifySuppressScope &) = delete;
    MediaNotifySuppressScope &operator=(const MediaNotifySuppressScope &) = delete;
    MediaNotifySuppressScope(MediaNotifySuppressScope &&other) noexcept;
    MediaNotifySuppressScope &operator=(MediaNotifySuppressScope &&) = delete;

private:
    std::vector<std::string> paths_;
    bool released_{false};
};
} // namespace OHOS::Media
#endif // MEDIA_NOTIFY_SUPPRESS_MANAGER_H
