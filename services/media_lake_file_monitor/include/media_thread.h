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
#ifndef MEDIA_LIBRARY_THREAD_H
#define MEDIA_LIBRARY_THREAD_H

#include <iostream>
#include <atomic>
#include <functional>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>

#include "media_log.h"

namespace OHOS {
namespace Media {

class thread {
public:
    thread()
    {
        MEDIA_WARN_LOG("default thread: %{public}s", name_.c_str());
    }

    ~thread()
    {
        if (joinable()) {
            MEDIA_WARN_LOG("thread: %{public}s,  no join or detach", name_.c_str());
            // maybe better call pthread_detach(threadId_); it's prone to thread leaks.
            std::terminate(); // If called, the implementation is the same as the standard.
        }
    }

    template<typename T>
    struct _not : public std::bool_constant<!bool(T::value)> {};

    template<typename... Cond>
    using _require = typename std::enable_if<std::conjunction<Cond...>::value, void>::type;

    template<class T>
    struct _remove_cvref {
        using type = std::remove_cv_t<std::remove_reference_t<T>>;
    };

    template<typename Tp>
    using not_same = _not<std::is_same<_remove_cvref<Tp>, thread>>;

    // same std::thread
    template<typename F, typename... Args, typename = _require<not_same<F>>>
    explicit thread(F&& f, Args&& ... args)
    {
        MEDIA_WARN_LOG("create thread: %{public}s  ", name_.c_str());
        start(f, std::forward<Args>(args)...);
    }

    // add std::thread name
    template<typename NameType, typename F, typename... Args, typename = _require<not_same<F>>>
    thread(NameType&& name, F&& f, Args&& ... args) : name_(std::forward<NameType>(name))
    {
        MEDIA_WARN_LOG("create name thread: %{public}s  ", name_.c_str());
        start(f, std::forward<Args>(args)...);
    }

    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;
    thread(thread&& other) noexcept
        : joinable_(other.joinable_.load()), threadId_(other.threadId_)
    {
        MEDIA_WARN_LOG("move constructor thread: %{public}s  ", other.name_.c_str());
        name_ = std::move(other.name_);
        other.joinable_ = false;
        other.threadId_ = 0;
    }

    thread& operator=(thread&& other) noexcept
    {
        if (this != &other) {
            if (joinable_) {
                std::terminate();
            }
            name_ = std::move(other.name_);
            joinable_ = other.joinable_.load();
            threadId_ = other.threadId_;
            other.joinable_ = false;
            other.threadId_ = 0;
            MEDIA_WARN_LOG("move assigned thread: %{public}s  ", name_.c_str());
        }
        return *this;
    }

    // same std::thread function name
    void join()
    {
        if (!joinable()) {
            MEDIA_ERR_LOG("thread(%{public}s), not join able.", name_.c_str());
            std::terminate();
            return;
        }

        MEDIA_INFO_LOG("thread(%{public}s) create_thread_self: %{public}lu, curr_thread_self: %{public}lu",
            name_.c_str(), threadId_, pthread_self());
        int ret;
        if (threadId_ != pthread_self()) {
            ret = pthread_join(threadId_, nullptr);
        } else {
            MEDIA_WARN_LOG("thread(%{public}s) join in self thread", name_.c_str());
            ret = pthread_detach(threadId_);
        }
        if (ret != 0) {
            MEDIA_ERR_LOG("thread(%{public}s) join err:%{public}d(%{public}s)", name_.c_str(), ret, strerror(ret));
        }
        joinable_ = false;
    }

    void detach()
    {
        if (!joinable()) {
            MEDIA_ERR_LOG("thread(%{public}s), not join able", name_.c_str());
            std::terminate();
            return;
        }

        int ret = pthread_detach(threadId_);
        if (ret != 0) {
            MEDIA_ERR_LOG("thread(%{public}s) detach err:%{public}d(%{public}s)", name_.c_str(), ret, strerror(ret));
        }

        joinable_ = false;
    }

    bool joinable() const noexcept
    {
        return joinable_;
    }

    pthread_t get_id() const noexcept
    {
        return threadId_;
    }

    template<typename NameType>
    void set_thread_name(NameType&& name)
    {
        name_ = std::forward<NameType>(name);
        if (joinable()) {
            active_thread_name(name_);
        }
        return;
    }

    bool is_invalid() const noexcept
    {
        return threadId_ == 0;
    }

protected:
    class ThreadCallback {
    public:
        ThreadCallback(std::string name, const std::function<void()>& func)
            : name_(std::move(name)), func_(func) {}
        void operator()()
        {
            auto threadNum = ++threadNum_;
            auto pthread_slf = pthread_self();
            auto tid = gettid();
            MEDIA_INFO_LOG("tid: %{public}d, thread(%{public}s-%{public}lu) enter. thread num:%{public}llu",
                tid, name_.c_str(), pthread_self(), threadNum);
            func_();
            threadNum = --threadNum_;
            MEDIA_INFO_LOG("tid: %{public}d, thread(%{public}s-%{public}lu): exit. thread num :%{public}llu",
                tid, name_.c_str(), pthread_slf, threadNum);
        }
    private:
        std::string name_;
        std::function<void()> func_;
    };

    template<typename F, typename... Args, typename = _require<not_same<F>>>
    void start(F&& f, Args&& ... args)
    {
        static_assert(std::is_invocable<typename std::decay<F>::type,
                typename std::decay<Args>::type...>::value,
            "thread arguments must be invocable after conversion to rvalues");
        auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto callback = new ThreadCallback(name_, func);
        int ret = pthread_create(&threadId_, nullptr, [](void* arg) -> void* {
            auto threadCallback = static_cast<ThreadCallback*>(arg);
            (*threadCallback)();
            delete threadCallback;
            return nullptr;
        }, callback);
        if (ret != 0) {
            MEDIA_ERR_LOG("thread(%{public}s): create thread fail. err:%{public}d(%{public}s)",
                name_.c_str(), ret, strerror(errno));
            delete callback;
            threadId_ = 0;
            return;
        }
        joinable_ = true;
        active_thread_name(name_);
        MEDIA_INFO_LOG("thread(%{public}s-%{public}lu): create thread success.", name_.c_str(), threadId_);
    }

    void active_thread_name(const std::string& name)
    {
        constexpr int threadNameMaxLen = 15;
        if (name.length() <= threadNameMaxLen) {
            pthread_setname_np(threadId_, name.c_str());
        } else {
            const char* nameTmp = name.c_str() + name.length() - threadNameMaxLen;
            pthread_setname_np(threadId_, nameTmp);
        }
    }

private:
    std::string name_{"unknown"};
    std::atomic<bool> joinable_{false};
    pthread_t threadId_{0};
    inline static std::atomic<uint64_t> threadNum_{0};
};

}
}

#endif // MEDIA_LIBRARY_THREAD_H
