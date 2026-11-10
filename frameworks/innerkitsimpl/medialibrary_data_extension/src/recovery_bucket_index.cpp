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
#include "recovery_bucket_index.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <mutex>
#include <sys/stat.h>
#include <unordered_map>
#include <unistd.h>
#include <vector>

namespace OHOS::Media {
namespace {
constexpr int64_t MS_PER_SECOND = 1000;
constexpr int64_t NS_PER_MS = 1000000;
constexpr int64_t NS_PER_SECOND = 1000000000;
constexpr char INDEX_SUFFIX[] = ".idx";
constexpr char JSON_SUFFIX[] = ".json";
constexpr size_t INDEX_SUFFIX_SIZE = sizeof(INDEX_SUFFIX) - 1;
constexpr size_t JSON_SUFFIX_SIZE = sizeof(JSON_SUFFIX) - 1;
std::mutex g_mutex;
// Cache only the current marker name; the marker file remains the source of truth.
std::unordered_map<std::string, std::string> g_markers;

bool StartsWith(const std::string &value, const std::string &prefix)
{
    return value.compare(0, prefix.size(), prefix) == 0;
}

bool GetMtime(const struct stat &info, int64_t &mtime)
{
    if (info.st_mtim.tv_sec < 0 || info.st_mtim.tv_nsec < 0 || info.st_mtim.tv_nsec >= NS_PER_SECOND ||
        info.st_mtim.tv_sec > (std::numeric_limits<int64_t>::max() - MS_PER_SECOND) / MS_PER_SECOND) {
        return false;
    }
    mtime = info.st_mtim.tv_sec * MS_PER_SECOND + info.st_mtim.tv_nsec / NS_PER_MS;
    return true;
}

int ListMarkers(const std::string &directory, std::vector<std::string> &markers)
{
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr) {
        return errno;
    }
    errno = 0;
    while (auto *entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (IsRecoveryBucketMarker(name)) {
            markers.emplace_back(name);
        }
        errno = 0;
    }
    int error = errno;
    closedir(dir);
    return error;
}

// A bucket without a trusted marker may contain older, pre-existing JSONs.
// Certify them once before installing the first usable marker.
bool CanCertifyBucket(const std::string &directory, int64_t nowMs)
{
    DIR *dir = opendir(directory.c_str());
    if (dir == nullptr) {
        return false;
    }
    bool valid = true;
    errno = 0;
    while (auto *entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name == "." || name == ".." || IsRecoveryBucketMarker(name)) {
            continue;
        }
        const bool jsonFile = name.size() >= 5 && name.compare(name.size() - 5, 5, ".json") == 0;
        if (!jsonFile && entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) {
            continue;
        }
        struct stat info {};
        int64_t mtime = 0;
        if (lstat((directory + "/" + name).c_str(), &info) != 0 || S_ISDIR(info.st_mode) ||
            (S_ISREG(info.st_mode) && jsonFile &&
            (!GetMtime(info, mtime) || mtime > nowMs))) {
            valid = false;
            break;
        }
        errno = 0;
    }
    const int error = errno;
    closedir(dir);
    return valid && error == 0;
}

int RemoveMarkers(const std::string &directory, const std::vector<std::string> &markers,
    const std::string &keep = "")
{
    int result = 0;
    for (const auto &name : markers) {
        if (name != keep && unlink((directory + "/" + name).c_str()) != 0 && errno != ENOENT) {
            result = errno;
        }
    }
    return result;
}

bool ReadMarkerTime(const std::string &directory, const std::string &marker, int64_t &latestUpperMs)
{
    int64_t recordedMs = 0;
    std::string name;
    if (!ParseRecoveryBucketMarker(marker, recordedMs, name)) {
        return false;
    }
    struct stat markerInfo {};
    int64_t markerMs = 0;
    if (lstat((directory + "/" + marker).c_str(), &markerInfo) != 0 || !S_ISREG(markerInfo.st_mode) ||
        markerInfo.st_size != 0 || !GetMtime(markerInfo, markerMs)) {
        return false;
    }
    struct stat info {};
    int64_t actualMs = 0;
    if (lstat((directory + "/" + name).c_str(), &info) != 0 || !S_ISREG(info.st_mode) ||
        !GetMtime(info, actualMs)) {
        return false;
    }
    const int64_t difference = actualMs >= recordedMs ? actualMs - recordedMs : recordedMs - actualMs;
    const int64_t markerDifference = actualMs >= markerMs ? actualMs - markerMs : markerMs - actualMs;
    if (difference > MS_PER_SECOND || markerDifference > MS_PER_SECOND) {
        return false;
    }
    // The pre-write marker may be up to a second newer than the JSON. Use the
    // latest of the three times so a window boundary cannot hide another file.
    latestUpperMs = std::max({actualMs, recordedMs, markerMs});
    return true;
}

int InstallMarker(const std::string &directory, const std::vector<std::string> &oldMarkers,
    const std::string &marker, int64_t mtimeMs, std::string &installed)
{
    const std::string path = directory + "/" + marker;
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) {
        // A stale verified marker must not survive a write with no replacement.
        return RemoveMarkers(directory, oldMarkers);
    }
    const time_t seconds = static_cast<time_t>(mtimeMs / MS_PER_SECOND);
    const long nanos = static_cast<long>((mtimeMs % MS_PER_SECOND) * NS_PER_MS);
    const struct timespec times[2] {{seconds, nanos}, {seconds, nanos}};
    struct stat info {};
    const bool valid = mtimeMs >= 0 && static_cast<int64_t>(seconds) == mtimeMs / MS_PER_SECOND &&
        fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_size == 0 && futimens(fd, times) == 0;
    close(fd);
    if (!valid) {
        int error = unlink(path.c_str()) == 0 || errno == ENOENT ? 0 : errno;
        int cleanupError = RemoveMarkers(directory, oldMarkers);
        return error != 0 ? error : cleanupError;
    }
    // If cleanup fails, multiple markers make the bucket ineligible for skipping.
    if (RemoveMarkers(directory, oldMarkers, marker) == 0) {
        installed = marker;
    }
    return 0;
}
} // namespace

bool IsRecoveryBucketMarker(const std::string &name)
{
    return StartsWith(name, RECOVERY_BUCKET_MARKER_PREFIX) || StartsWith(name, RECOVERY_BUCKET_UNVERIFIED_PREFIX);
}

bool ParseRecoveryBucketMarker(const std::string &name, int64_t &mtimeMs, std::string &jsonName)
{
    if (!StartsWith(name, RECOVERY_BUCKET_MARKER_PREFIX)) {
        return false;
    }
    const std::string value = name.substr(sizeof(RECOVERY_BUCKET_MARKER_PREFIX) - 1);
    const size_t separator = value.find('_');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= value.size()) {
        return false;
    }
    const std::string time = value.substr(0, separator);
    if (time.find_first_not_of("0123456789") != std::string::npos) {
        return false;
    }
    errno = 0;
    char *end = nullptr;
    const long long parsed = strtoll(time.c_str(), &end, 10);
    jsonName = value.substr(separator + 1);
    if (jsonName.size() < INDEX_SUFFIX_SIZE ||
        jsonName.compare(jsonName.size() - INDEX_SUFFIX_SIZE, INDEX_SUFFIX_SIZE, INDEX_SUFFIX) != 0) {
        return false;
    }
    jsonName.resize(jsonName.size() - INDEX_SUFFIX_SIZE);
    if (errno != 0 || end == nullptr || *end != '\0' || parsed < 0 || jsonName.find('/') != std::string::npos ||
        jsonName == "." || jsonName == ".." || jsonName.size() < JSON_SUFFIX_SIZE ||
        jsonName.compare(jsonName.size() - JSON_SUFFIX_SIZE, JSON_SUFFIX_SIZE, JSON_SUFFIX) != 0 ||
        IsRecoveryBucketMarker(jsonName)) {
        return false;
    }
    mtimeMs = static_cast<int64_t>(parsed);
    return true;
}

bool CanSkipRecoveryBucket(const std::string &directory, int64_t startMs)
{
    if (startMs <= 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<std::string> markers;
    int64_t actualMs = 0;
    return ListMarkers(directory, markers) == 0 && markers.size() == 1 &&
        ReadMarkerTime(directory, markers.front(), actualMs) && actualMs < startMs;
}

int PrepareRecoveryBucketWrite(const std::string &directory, const std::string &name, int64_t nowMs)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<std::string> markers;
    auto cached = g_markers.find(directory);
    int error = 0;
    if (cached == g_markers.end()) {
        error = ListMarkers(directory, markers);
    } else {
        markers.push_back(cached->second);
    }
    if (error != 0) {
        return error;
    }
    int64_t oldMs = 0;
    std::string previousName;
    const bool previousValid = markers.size() == 1 && (cached != g_markers.end() ?
        ParseRecoveryBucketMarker(markers.front(), oldMs, previousName) :
        ReadMarkerTime(directory, markers.front(), oldMs));
    const bool verified = previousValid ? nowMs >= oldMs : CanCertifyBucket(directory, nowMs);
    const std::string prefix = verified ? RECOVERY_BUCKET_MARKER_PREFIX : RECOVERY_BUCKET_UNVERIFIED_PREFIX;
    const std::string marker = prefix + std::to_string(nowMs) + "_" + name + ".idx";
    std::string installed;
    error = InstallMarker(directory, markers, marker, nowMs, installed);
    if (error == 0 && !installed.empty()) {
        g_markers[directory] = installed;
    } else {
        g_markers.erase(directory);
    }
    return error;
}

void InvalidateRecoveryBucketIndex(const std::string &directory)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_markers.erase(directory);
    std::vector<std::string> markers;
    if (ListMarkers(directory, markers) == 0) {
        (void)RemoveMarkers(directory, markers);
    }
}
} // namespace OHOS::Media
