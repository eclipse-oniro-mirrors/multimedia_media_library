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

#ifndef MEDIA_LIBRARY_ENABLE_SHARED_CREATE_H
#define MEDIA_LIBRARY_ENABLE_SHARED_CREATE_H

#include <memory>

#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

template<typename classType, typename... Args>
auto MediaMakeSharedPrivateObj(Args&& ...args)
{
    struct MakeSharedHelper : public classType {
        explicit MakeSharedHelper(Args&& ...args) : classType(std::forward<Args>(args)...) {}
    };
    return std::make_shared<MakeSharedHelper>(std::forward<Args>(args)...);
}

// 解决继承 std::enable_shared_from_this 必须被 public 继承, 否则 shared_from_this 是未定义性行为
// 自己实现: EnableSharedCreate/EnableSharedCreateInit 可以 protected 继承
template<typename Class>
class EnableSharedCreate {
public:
    template<typename... Args>
    static std::shared_ptr<Class> Create(Args&& ...args)
    {
        struct MakeSharedHelper : public Class {
            explicit MakeSharedHelper(Args&& ...a) : Class(std::forward<Args>(a)...) {}
        };

        std::shared_ptr<Class> sharePtr = std::make_shared<MakeSharedHelper>(std::forward<Args>(args)...);
        // If static_cast<EnableSharedCreate*> is used, protected cannot be used to inherit the template.
        // Avoid this line error, not like code: auto base = static_cast<EnableSharedCreate*>(sharePtr.get());
        auto base = (EnableSharedCreate*)(sharePtr.get());
        EnableSharedFromThis(sharePtr, base);
        return sharePtr;
    }

    std::shared_ptr<Class> shared_from_this()
    {
        return std::shared_ptr<Class>(weakThis_);
    }

    std::shared_ptr<const Class> shared_from_this() const
    {
        return std::shared_ptr<const Class>(weakThis_);
    }

    std::weak_ptr<Class> weak_from_this() noexcept
    {
        return this->weakThis_;
    }

    std::weak_ptr<const Class> weak_from_this() const noexcept
    {
        return this->weakThis_;
    }

protected:
    inline void InitWeakThis(const std::shared_ptr<Class>& weak)
    {
        weakThis_ = weak;
    }

private:
    template<typename X, typename Y>
    friend void EnableSharedFromThis(const std::shared_ptr<X>& sp, Y* base);

    mutable std::weak_ptr<Class> weakThis_;
};

template<typename T, typename X>
inline void EnableSharedFromThis(const std::shared_ptr<T>& sp, X* base)
{
    if (base != nullptr) {
        base->InitWeakThis(sp);
    }
}

template<typename Class>
class EnableSharedCreateInit : public EnableSharedCreate<Class> {
public:
    template<typename... Args>
    static std::shared_ptr<Class> Create(Args&& ...args)
    {
        auto sharedObj = EnableSharedCreate<Class>::Create(std::forward<Args>(args)...);
        if (sharedObj && (sharedObj->Initialize() != E_SUCCESS)) {
            sharedObj = nullptr;
        }
        return sharedObj;
    }
};

}
}

#endif // MEDIA_LIBRARY_ENABLE_SHARED_CREATE_H
