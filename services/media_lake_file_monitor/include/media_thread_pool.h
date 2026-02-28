/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef MEDIA_LIBRARY_THREAD_POOL_H
#define MEDIA_LIBRARY_THREAD_POOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <algorithm>

#include "media_thread.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

constexpr size_t THREAD_POOL_MAX_THREAD_NUM = 8;
constexpr int64_t TIMEOUT_TO_EXIT_THREAD_DEFAULT = 60 * 1000; // milliseconds

class ThreadPool {
public:
    template<typename NameType>
    explicit ThreadPool(NameType&& threadName, size_t minThreads = 1, size_t maxThreads = 4,
        int64_t timeoutToExitThread = TIMEOUT_TO_EXIT_THREAD_DEFAULT)
        : threadPoolName_(std::forward<NameType>(threadName)), minThreads_(minThreads),
          maxThreads_(maxThreads)
    {
        poolInfo_ = std::make_shared<ThreadPoolInfo>();
        if (poolInfo_ == nullptr) {
            return;
        }
        poolInfo_->timeout = timeoutToExitThread;
        maxThreads_ = std::min(THREAD_POOL_MAX_THREAD_NUM, maxThreads);
        if (minThreads > maxThreads_) {
            minThreads_ = maxThreads_;
        }
        auto& threads = poolInfo_->threads;
        threads.reserve(maxThreads);
        for (size_t i = 0; i < minThreads_; ++i) {
            threads.emplace_back(threadPoolName_ + std::to_string(threadIndex_++),
                [poolInfo = poolInfo_]() { LoopForEver(poolInfo); });
        }
        MEDIA_INFO_LOG("ThreadPool(%{public}s) construct(enter), mix(%{public}u)/max(%{public}u).",
            threadPoolName_.c_str(), minThreads_, maxThreads_);
    }

    ~ThreadPool()
    {
        Stop(true);
        MEDIA_INFO_LOG("ThreadPool(%{public}s) deconstruct(exit).", threadPoolName_.c_str());
    }

    template <typename F, typename... Args>
    auto AddFutureTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        if (task == nullptr) {
            return std::future<return_type>();
        }

        auto ret = AddTaskInner([task]() { (*task)(); });
        if (ret == EXIT_SUCCESS) {
            return task->get_future();
        }

        return std::future<return_type>();
    }

    int32_t AddNormalTask(std::function<void()> func)
    {
        return AddTaskInner(std::move(func));
    }

    int32_t ClearThenAddNormalTask(std::function<void()> func)
    {
        return AddTaskInner(std::move(func), true);
    }

    void Stop(bool waitThreadExit = true)
    {
        if (poolInfo_ == nullptr) {
            return;
        }

        {
            std::unique_lock<std::mutex> lck(poolInfo_->queueMutex);
            poolInfo_->isStop = true;
        }
        poolInfo_->condition.notify_all();
        MEDIA_INFO_LOG("stop all threads (%{public}s), thread num: %{public}u.",
            threadPoolName_.c_str(), poolInfo_->threads.size());
        if (waitThreadExit) {
            Wait<true>();
        } else {
            Wait<false>();
        }
    }

protected:
    // ThreadInfo encapsulates thread-related information for extension.
    struct ThreadInfo {
        template<typename F, typename... Args>
        ThreadInfo(F&& f, Args&& ... args) : thread(std::forward<F>(f), std::forward<Args>(args)...) {}
        Media::thread thread;
    };

    struct ThreadPoolInfo {
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        bool isStop{false};
        int64_t timeout;
        std::vector<ThreadInfo> threads;
    };

    template<typename T>
    void ClearQueue(std::queue<T>& q)
    {
        std::queue<T> empty;
        swap(empty, q);
    }

    using WaitCondSatisfiedFunc = bool (std::unique_lock<std::mutex>& lck, ThreadPoolInfo* poolInfo);

    int32_t AddTaskInner(std::function<void()>&& func, bool clearAllTasks = false)
    {
        if (poolInfo_ == nullptr || func == nullptr) {
            MEDIA_ERR_LOG("input param is null. thread pool name: %{public}s", threadPoolName_.c_str());
            return E_INVALID_ARGUMENTS;
        }

        {
            std::lock_guard<std::mutex> lck(poolInfo_->queueMutex);
            if (!poolInfo_->isStop) {
                if (clearAllTasks) {
                    ClearQueue(poolInfo_->tasks);
                }
                poolInfo_->tasks.emplace(std::move(func));
                auto taskNum = poolInfo_->tasks.size();
                auto threadNum = poolInfo_->threads.size();
                MEDIA_INFO_LOG("ThreadPool(%{public}s) tasks: %{public}u, size: %{public}u, max threads: %{public}u",
                    threadPoolName_.c_str(), taskNum, threadNum, maxThreads_);
                if (taskNum > threadNum && threadNum < maxThreads_) {
                    AddThreadUnlock(threadIndex_++);
                }
            } else {
                MEDIA_ERR_LOG("ThreadPool(%{public}s) has stopped.", threadPoolName_.c_str());
                return E_THREAD_HAS_STOPPED;
            }
        }
        poolInfo_->condition.notify_one();
        return E_SUCCESS;
    }

    void AddThreadUnlock(size_t index)
    {
        auto &threads = poolInfo_->threads;
        threads.emplace_back(threadPoolName_ + std::to_string(index),
            [poolInfo = poolInfo_]() { LoopExitWhenTimeout(poolInfo); });
        if (!threads.empty()) {
            if (threads.back().thread.is_invalid()) {
                threads.pop_back();
                MEDIA_ERR_LOG("thread create failed, pop");
            }
        }
    }

    static void RemoveThreadUnlock(ThreadPoolInfo* poolInfo)
    {
        auto& threads = poolInfo->threads;
        auto id = pthread_self();
        for (auto it = threads.begin(); it != threads.end(); ++it) {
            if (it->thread.get_id() == id) {
                it->thread.detach();
                threads.erase(it);
                return;
            }
        }
    }

    static bool WaitForEver(std::unique_lock<std::mutex>& lck, ThreadPoolInfo* poolInfo)
    {
        poolInfo->condition.wait(lck, [poolInfo] {
            return poolInfo->isStop || !poolInfo->tasks.empty();
        });
        return true;
    }

    static bool WaitForTimeout(std::unique_lock<std::mutex>& lck, ThreadPoolInfo* poolInfo)
    {
        return poolInfo->condition.wait_for(lck, std::chrono::milliseconds(poolInfo->timeout),
            [poolInfo] { return poolInfo->isStop || !poolInfo->tasks.empty(); });
    }

    static void LoopEnterBase(std::shared_ptr<ThreadPoolInfo>& poolInfo, WaitCondSatisfiedFunc waitCondUnlock)
    {
        if (poolInfo == nullptr) {
            MEDIA_ERR_LOG("thread param is null");
            threadIndex_ = 0;
            return;
        }

        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lck(poolInfo->queueMutex);
                auto notTimeout = waitCondUnlock(lck, poolInfo.get());
                if (!notTimeout) {
                    RemoveThreadUnlock(poolInfo.get());
                    MEDIA_INFO_LOG("detach self, task num: %{public}u, threads num: %{public}u",
                        poolInfo->tasks.size(), poolInfo->threads.size());
                    return;
                }
                // note: need execute all task, or Block the thread that invokes std::future::get
                if ((poolInfo->isStop && poolInfo->tasks.empty())) {
                    MEDIA_INFO_LOG("threads, tid: %{public}d. "
                        "stop: %{public}d, tasks: %{public}zu",
                        gettid(), poolInfo->isStop, poolInfo->tasks.size());
                    return;
                }
                task = std::move(poolInfo->tasks.front());
                poolInfo->tasks.pop();
            }
            task();
        }

        MEDIA_INFO_LOG("exit thread loop. task num: %{public}u, threads num: %{public}u",
            poolInfo->tasks.size(), poolInfo->threads.size());
        threadIndex_ = 0;
    }

    static void LoopForEver(std::shared_ptr<ThreadPoolInfo> poolInfo)
    {
        LoopEnterBase(poolInfo, WaitForEver);
    }

    static void LoopExitWhenTimeout(std::shared_ptr<ThreadPoolInfo> poolInfo)
    {
        LoopEnterBase(poolInfo, WaitForTimeout);
    }

    template<bool waitThreadExist>
    void Wait()
    {
        if (poolInfo_ == nullptr) {
            return;
        }
        auto& threads = poolInfo_->threads;
        for (auto &thd : threads) {
            if (thd.thread.joinable()) {
                if constexpr (waitThreadExist) {
                    thd.thread.join();
                } else {
                    thd.thread.detach();
                }
            }
        }
    }

private:
    std::string threadPoolName_;
    size_t minThreads_;
    size_t maxThreads_;
    inline static std::atomic<size_t> threadIndex_{0};
    std::shared_ptr<ThreadPoolInfo> poolInfo_; // After ThreadPool is destructed, the thread can access ThreadPool data.
};

}
}

#endif // MEDIA_LIBRARY_THREAD_POOL_H
