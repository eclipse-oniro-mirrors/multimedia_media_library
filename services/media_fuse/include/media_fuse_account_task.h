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

#ifndef OHOS_MEDIA_FUSE_ACCOUNT_TASK_H
#define OHOS_MEDIA_FUSE_ACCOUNT_TASK_H

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "account_enable_checker.h"
#include "account_manager.h"
#include "account_scope.h"

namespace OHOS {
namespace Media {

// Callbacks only set cancellation; they never close a file or touch a database.
class FuseLogoutObserver final : public AccountObserver {
public:
    explicit FuseLogoutObserver(int32_t accountId) : accountId_(accountId) {}

    void OnAccountLogout(int32_t accountId) override
    {
        if (accountId == accountId_) {
            cancelled_.store(true);
        }
    }

    bool IsCancelled() const
    {
        return cancelled_.load();
    }

private:
    const int32_t accountId_;
    std::atomic<bool> cancelled_ {false};
};

class FuseAccountTask final {
public:
    // Public for make_shared; business callers must use Begin() for validation and observer registration.
    explicit FuseAccountTask(int32_t accountId)
        : accountId_(accountId), observer_(std::make_shared<FuseLogoutObserver>(accountId)) {}

    static std::shared_ptr<FuseAccountTask> Begin(int &error)
    {
        error = 0;
        if (!AccountEnableChecker::IsMultiAccountEnabled()) {
            return nullptr;
        }
        auto &manager = AccountManager::GetInstance();
        int32_t accountId = INVALID_ACCOUNT_ID;
        if (!manager.GetForegroundAccountId(accountId) || accountId <= 0) {
            error = -EIO;
            return nullptr;
        }
        auto task = std::make_shared<FuseAccountTask>(accountId);
        manager.RegisterObserver(task->observer_);
        int32_t currentId = INVALID_ACCOUNT_ID;
        if (!manager.GetForegroundAccountId(currentId) || currentId != accountId || task->IsCancelled()) {
            error = -ECANCELED;
            return nullptr;
        }
        return task;
    }

    ~FuseAccountTask()
    {
        // A copied notification snapshot may still retain observer_; it owns no task pointer.
        AccountManager::GetInstance().UnregisterObserver(observer_);
    }

    FuseAccountTask(const FuseAccountTask &) = delete;
    FuseAccountTask &operator=(const FuseAccountTask &) = delete;

    int32_t GetAccountId() const
    {
        return accountId_;
    }

    bool IsCancelled() const
    {
        return observer_->IsCancelled();
    }

private:
    const int32_t accountId_;
    std::shared_ptr<FuseLogoutObserver> observer_;
};

// Keep cancellation visible to synchronous manager calls on this callback thread.
// Scanner worker threads must carry their own task context.
class FuseAccountTaskScope final {
public:
    explicit FuseAccountTaskScope(const std::shared_ptr<FuseAccountTask> &task)
        : previous_(Current()), accountScope_(task->GetAccountId())
    {
        Current() = task;
    }

    ~FuseAccountTaskScope()
    {
        Current() = previous_;
    }

    FuseAccountTaskScope(const FuseAccountTaskScope &) = delete;
    FuseAccountTaskScope &operator=(const FuseAccountTaskScope &) = delete;

    static bool IsCancelled()
    {
        const auto &task = Current();
        return task && task->IsCancelled();
    }

private:
    static std::shared_ptr<FuseAccountTask> &Current()
    {
        thread_local std::shared_ptr<FuseAccountTask> task;
        return task;
    }

    std::shared_ptr<FuseAccountTask> previous_;
    AccountScope accountScope_;
};

// Shared callback entry: cancellation is cooperative, not rollback of completed I/O.
template<typename Operation>
int RunFuseAccountTask(const std::shared_ptr<FuseAccountTask> &task, Operation operation)
{
    if (!task) {
        return operation();
    }
    if (task->IsCancelled()) {
        return -ECANCELED;
    }
    FuseAccountTaskScope scope(task);
    return operation();
}

// Remove state before close(fd): the OS may reuse the descriptor immediately.
class FuseCreatedFiles final {
public:
    static FuseCreatedFiles &GetInstance()
    {
        static FuseCreatedFiles files;
        return files;
    }

    bool Insert(int fd, const std::string &path)
    {
        if (fd < 0 || path.empty()) {
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        return paths_.emplace(fd, path).second;
    }

    bool Take(int fd, std::string &path)
    {
        path.clear();
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = paths_.find(fd);
        if (it == paths_.end()) {
            return false;
        }
        path = it->second;
        paths_.erase(it);
        return true;
    }

private:
    FuseCreatedFiles() = default;
    std::mutex mutex_;
    std::unordered_map<int, std::string> paths_;
};

struct FuseAccountFile {
    std::shared_ptr<FuseAccountTask> task;
    std::string path;
};

// fi->fh remains the real fd. No account ID or pointer is exposed through the path.
class FuseAccountFiles final {
public:
    static FuseAccountFiles &GetInstance()
    {
        static FuseAccountFiles files;
        return files;
    }

    bool Insert(int fd, const std::shared_ptr<FuseAccountTask> &task, const std::string &path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return files_.emplace(fd, std::make_shared<FuseAccountFile>(FuseAccountFile {task, path})).second;
    }

    std::shared_ptr<FuseAccountFile> Find(int fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(fd);
        return it == files_.end() ? nullptr : it->second;
    }

    std::shared_ptr<FuseAccountFile> Take(int fd)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = files_.find(fd);
        if (it == files_.end()) {
            return nullptr;
        }
        auto file = it->second;
        files_.erase(it);
        return file;
    }

private:
    FuseAccountFiles() = default;
    std::mutex mutex_;
    std::unordered_map<int, std::shared_ptr<FuseAccountFile>> files_;
};

struct FuseAccountDirectory {
    std::shared_ptr<FuseAccountTask> task;
    std::string path;
};

// Directory IDs are opaque, never real fds, and never reused during this process.
class FuseAccountDirectories final {
public:
    static FuseAccountDirectories &GetInstance()
    {
        static FuseAccountDirectories directories;
        return directories;
    }

    static bool IsDirectoryHandle(uint64_t handle)
    {
        return (handle & DIRECTORY_HANDLE_FLAG) != 0;
    }

    uint64_t Insert(const std::shared_ptr<FuseAccountTask> &task, const std::string &path)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!task || !IsDirectoryHandle(nextHandle_)) {
            return 0;
        }
        uint64_t handle = nextHandle_++;
        directories_.emplace(handle, std::make_shared<FuseAccountDirectory>(FuseAccountDirectory {task, path}));
        return handle;
    }

    std::shared_ptr<FuseAccountDirectory> Find(uint64_t handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = directories_.find(handle);
        return it == directories_.end() ? nullptr : it->second;
    }

    std::shared_ptr<FuseAccountDirectory> Take(uint64_t handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = directories_.find(handle);
        if (it == directories_.end()) {
            return nullptr;
        }
        auto directory = it->second;
        directories_.erase(it);
        return directory;
    }

private:
    FuseAccountDirectories() = default;
    static constexpr uint64_t DIRECTORY_HANDLE_FLAG = uint64_t {1} << 63;
    uint64_t nextHandle_ = DIRECTORY_HANDLE_FLAG;
    std::mutex mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<FuseAccountDirectory>> directories_;
};

// Release cancelled files without entering query/scan/create business code.
int32_t CloseCancelledFuseFile(const char *path, int fd);

} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
#endif // OHOS_MEDIA_FUSE_ACCOUNT_TASK_H
