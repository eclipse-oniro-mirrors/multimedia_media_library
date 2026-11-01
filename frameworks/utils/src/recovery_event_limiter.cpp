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
#include "recovery_event_limiter.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <sys/file.h>
#include <unistd.h>
#include <vector>

namespace OHOS::Media {
namespace {
constexpr char EVENT_DIR[] = "/data/storage/el2/base/preferences";
constexpr char EVENT_PREFIX[] = "/recovery_event_limit_";
constexpr int64_t HOUR_MS = 60LL * 60 * 1000;
constexpr size_t EVENT_LIMIT = 2;

int ReserveLocked(const std::string &path, int64_t nowMs)
{
    std::vector<int64_t> recent;
    std::ifstream input(path);
    if (!input && access(path.c_str(), F_OK) == 0) {
        return EIO;
    }
    size_t count = 0;
    int64_t eventMs = 0;
    while (input >> eventMs) {
        if (eventMs < 0 || ++count > EVENT_LIMIT) {
            return EINVAL;
        }
        if (IsRecoveryEventRecent(eventMs, nowMs)) {
            recent.push_back(eventMs);
        }
    }
    if (input.bad() || (input.is_open() && (!input.eof() || count == 0))) {
        return EINVAL;
    }
    if (recent.size() >= EVENT_LIMIT) {
        return EAGAIN;
    }
    recent.push_back(nowMs);
    std::string temp = path + ".tmp";
    std::ofstream output(temp, std::ios::trunc);
    for (int64_t eventMs : recent) {
        output << eventMs << '\n';
    }
    output.close();
    if (!output || rename(temp.c_str(), path.c_str()) != 0) {
        return EIO;
    }
    return 0;
}
} // namespace

const char *RecoveryEventName(RecoveryEventKind kind)
{
    switch (kind) {
        case RecoveryEventKind::DATABASE_CORRUPTION:
            return "DATABASE_CORRUPTION_ERROR";
        case RecoveryEventKind::META_RECOVERY_INFO:
            return "MEDIALIB_META_RECOVERY_INFO";
        case RecoveryEventKind::META_RECOVERY_ERROR:
            return "MEDIALIB_META_RECOVERY_ERROR";
        default:
            return nullptr;
    }
}

bool IsRecoveryEventRecent(int64_t eventMs, int64_t nowMs)
{
    if (eventMs < 0 || nowMs < 0 || nowMs < eventMs) {
        // A clock rollback must not allow extra reports.
        return true;
    }
    return nowMs - eventMs < HOUR_MS;
}

int ReserveRecoveryEvent(const std::string &directory, RecoveryEventKind kind, int64_t nowMs)
{
    const char *name = RecoveryEventName(kind);
    if (name == nullptr || nowMs < 0) {
        return EINVAL;
    }
    int fd = open((directory + "/recovery_event_limit.lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        return errno;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        int error = errno == EWOULDBLOCK ? EBUSY : errno;
        close(fd);
        return error;
    }
    int result = ReserveLocked(directory + EVENT_PREFIX + name, nowMs);
    flock(fd, LOCK_UN);
    close(fd);
    return result;
}

int ReserveRecoveryEvent(RecoveryEventKind kind)
{
    const int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return ReserveRecoveryEvent(EVENT_DIR, kind, nowMs);
}
} // namespace OHOS::Media
