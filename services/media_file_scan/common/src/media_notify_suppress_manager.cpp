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

#define MLOG_TAG "NotifySuppressManager"

#include "media_notify_suppress_manager.h"

#include <cinttypes>

#include "dfx_utils.h"
#include "file_scan_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
namespace {
// 漏解除兜底超时: 覆盖GB级视频跨挂载copy时长
constexpr int64_t DEFAULT_SUPPRESS_TIMEOUT_MS = 10 * 60 * 1000; // 10 min

std::string NormalizePath(const std::string &path)
{
    std::string normalized = path;
    while (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}
} // namespace

MediaNotifySuppressManager &MediaNotifySuppressManager::GetInstance()
{
    static MediaNotifySuppressManager instance;
    return instance;
}

void MediaNotifySuppressManager::Suppress(const std::vector<std::string> &paths)
{
    CHECK_AND_RETURN_LOG(!paths.empty(), "Suppress paths is empty");
    int64_t expireAtMs = MediaFileUtils::UTCTimeMilliSeconds() + DEFAULT_SUPPRESS_TIMEOUT_MS;
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto &path : paths) {
        for (const auto &form : FileScanUtils::ExpandPathForms(path)) {
            if (form.empty()) {
                continue;
            }
            suppressed_[form] = Entry{expireAtMs};
        }
    }
    MEDIA_INFO_LOG("Suppress paths: %{public}zu, active entries: %{public}zu",
        paths.size(), suppressed_.size());
}

void MediaNotifySuppressManager::Release(const std::vector<std::string> &paths)
{
    CHECK_AND_RETURN_LOG(!paths.empty(), "Release paths is empty");
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto &path : paths) {
        for (const auto &form : FileScanUtils::ExpandPathForms(path)) {
            suppressed_.erase(form);
        }
    }
    MEDIA_INFO_LOG("Release paths: %{public}zu, active entries: %{public}zu, total skipped msgs: %{public}" PRIu64,
        paths.size(), suppressed_.size(), skippedMsgCount_.load(std::memory_order_relaxed));
}

bool MediaNotifySuppressManager::IsSuppressed(const std::string &path)
{
    std::string normalized = NormalizePath(path);
    if (normalized.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    for (const auto &form : FileScanUtils::ExpandPathForms(normalized)) {
        auto it = suppressed_.find(form);
        if (it == suppressed_.end()) {
            continue;
        }
        if (now >= it->second.expireAtMs) {
            MEDIA_WARN_LOG("suppressed entry expired, path: %{public}s", DfxUtils::GetSafePath(form).c_str());
            suppressed_.erase(it); // 惰性清理过期条目
            continue;
        }
        skippedMsgCount_.fetch_add(1, std::memory_order_relaxed);
        return true; // 当前为精确匹配, 匹配方式扩展点
    }
    return false;
}

uint64_t MediaNotifySuppressManager::GetSkippedMsgCount() const
{
    return skippedMsgCount_.load(std::memory_order_relaxed);
}

MediaNotifySuppressScope::MediaNotifySuppressScope(std::vector<std::string> paths)
    : paths_(std::move(paths))
{
    MediaNotifySuppressManager::GetInstance().Suppress(paths_);
}

MediaNotifySuppressScope::~MediaNotifySuppressScope()
{
    if (!released_) {
        Release();
    }
}

MediaNotifySuppressScope::MediaNotifySuppressScope(MediaNotifySuppressScope &&other) noexcept
    : paths_(std::move(other.paths_)), released_(other.released_)
{
    other.released_ = true; // 源对象已交出登记内容, 其析构不得再解除
}

void MediaNotifySuppressScope::AddPaths(const std::vector<std::string> &paths)
{
    CHECK_AND_RETURN_LOG(!paths.empty(), "AddPaths paths is empty");
    if (released_) {
        MEDIA_WARN_LOG("AddPaths after Release is invalid, paths: %{public}zu", paths.size());
        return;
    }
    paths_.insert(paths_.end(), paths.begin(), paths.end());
    MediaNotifySuppressManager::GetInstance().Suppress(paths);
}

void MediaNotifySuppressScope::Release()
{
    if (released_) {
        return;
    }
    released_ = true;
    MediaNotifySuppressManager::GetInstance().Release(paths_);
}
} // namespace OHOS::Media
