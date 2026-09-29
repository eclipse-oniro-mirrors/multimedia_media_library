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
#define MLOG_TAG "DirScanAnomalyHelper"

#include "dir_scan_anomaly_helper.h"

#include <filesystem>

#include "file_scan_utils.h"
#include "file_manager_scan_rule_config.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string DirScanAnomalyHelper::XML_PATH =
    "/data/storage/el2/base/preferences/dir_scan_anomaly.xml";
const std::string DirScanAnomalyHelper::KEY_ANOMALY_DIRS = "anomaly_dirs";
std::mutex DirScanAnomalyHelper::mutex_;
json DirScanAnomalyHelper::cachedDirs_ = json::array();
bool DirScanAnomalyHelper::cacheLoaded_ = false;

// ── 路径工具 ──

static std::string NormalizeDirPath(const std::string &dirPath)
{
    std::error_code ec;
    std::string normalized = fs::absolute(fs::path(dirPath), ec).lexically_normal().string();
    if (ec) {
        MEDIA_ERR_LOG("NormalizeDirPath failed: %{public}s, path: %{public}s",
            ec.message().c_str(), MediaFileUtils::DesensitizePath(dirPath).c_str());
        return "";
    }
    return normalized;
}

static bool IsFileManagerDir(const std::string &dirPath)
{
    return FileScanUtils::IsPathUnderRoot(dirPath, std::string(FILE_MANAGER_SCAN_PATH));
}

// ── Preferences ──

std::shared_ptr<NativePreferences::Preferences> DirScanAnomalyHelper::GetPreferences()
{
    int32_t errCode;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(XML_PATH, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, nullptr,
        "GetPreferences failed, err: %{public}d", errCode);
    return prefs;
}

// ── 缓存读写（调用方已持锁）──

json DirScanAnomalyHelper::GetDirsLocked()
{
    CHECK_AND_RETURN_RET(!cacheLoaded_, cachedDirs_);
    MEDIA_INFO_LOG("Get prefs");
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_RET(prefs != nullptr, json::array());
    std::string rawJson = prefs->GetString(KEY_ANOMALY_DIRS, "[]");
    json dirs = json::parse(rawJson, nullptr, false);
    CHECK_AND_EXECUTE(dirs.is_discarded() || !dirs.is_array(), dirs = json::array());
    cachedDirs_ = dirs;
    cacheLoaded_ = true;
    return cachedDirs_;
}

void DirScanAnomalyHelper::SaveDirsLocked(const json &dirs)
{
    MEDIA_INFO_LOG("Save prefs");
    auto prefs = GetPreferences();
    CHECK_AND_RETURN_LOG(prefs != nullptr, "GetPreferences failed");
    cachedDirs_ = dirs;
    prefs->PutString(KEY_ANOMALY_DIRS,
        dirs.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    prefs->FlushSync();
}

// ── 公共接口 ──

int32_t DirScanAnomalyHelper::AddAnomalyDir(const std::string &dirPath)
{
    std::string normalized = NormalizeDirPath(dirPath);
    CHECK_AND_RETURN_RET_LOG(!normalized.empty() && IsFileManagerDir(normalized), E_ERR,
        "Invalid dir: %{public}s", MediaFileUtils::DesensitizePath(dirPath).c_str());

    std::lock_guard<std::mutex> lock(mutex_);
    json dirs = GetDirsLocked();

    for (const auto &item : dirs) {
        if (item.is_string() && item.get<std::string>() == normalized) {
            MEDIA_DEBUG_LOG("Dir already in anomaly set: %{public}s",
                MediaFileUtils::DesensitizePath(normalized).c_str());
            return E_OK;
        }
    }

    dirs.push_back(normalized);
    SaveDirsLocked(dirs);
    MEDIA_INFO_LOG("AddAnomalyDir: %{public}s", MediaFileUtils::DesensitizePath(normalized).c_str());
    return E_OK;
}

bool DirScanAnomalyHelper::MatchesAnomalyDir(const std::string &dirPath)
{
    std::string normalized = NormalizeDirPath(dirPath);
    CHECK_AND_RETURN_RET_LOG(IsFileManagerDir(normalized), false,
        "Not file manager dir: %{public}s", MediaFileUtils::DesensitizePath(normalized).c_str());

    std::lock_guard<std::mutex> lock(mutex_);
    json dirs = GetDirsLocked();

    std::string prefix = normalized + "/";
    for (const auto &item : dirs) {
        if (!item.is_string()) {
            continue;
        }
        const std::string path = item.get<std::string>();
        if (path == normalized || path.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

int32_t DirScanAnomalyHelper::RemoveAnomalyDir(const std::string &dirPath)
{
    // 快速路径：缓存为空直接返回，避免路径规范化与遍历开销（删除操作频繁但绝大多数无异常）
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (GetDirsLocked().empty()) {
            return E_OK;
        }
    }

    std::string normalized = NormalizeDirPath(dirPath);
    CHECK_AND_RETURN_RET_LOG(!normalized.empty(), E_ERR,
        "NormalizeDirPath failed: %{public}s", MediaFileUtils::DesensitizePath(dirPath).c_str());

    std::lock_guard<std::mutex> lock(mutex_);
    json dirs = GetDirsLocked();

    // 与 MatchesAnomalyDir 对称：删除精确匹配项及其子目录记录
    // 重命名/删除 a 时，anomaly 中 a 自身及 a/b 等子目录异常记录都应一并清理
    std::string prefix = normalized + "/";
    bool removed = false;
    for (auto it = dirs.begin(); it != dirs.end();) {
        if (it->is_string()) {
            const std::string path = it->get<std::string>();
            if (path == normalized || path.compare(0, prefix.size(), prefix) == 0) {
                it = dirs.erase(it);
                removed = true;
                continue;
            }
        }
        ++it;
    }
    if (!removed) {
        MEDIA_DEBUG_LOG("Dir not in anomaly set: %{public}s",
            MediaFileUtils::DesensitizePath(normalized).c_str());
        return E_OK;
    }

    SaveDirsLocked(dirs);
    MEDIA_INFO_LOG("RemoveAnomalyDir: %{public}s", MediaFileUtils::DesensitizePath(normalized).c_str());
    return E_OK;
}

void DirScanAnomalyHelper::ClearAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    SaveDirsLocked(json::array());
    MEDIA_INFO_LOG("ClearAll anomaly_dirs");
}

} // namespace OHOS::Media
