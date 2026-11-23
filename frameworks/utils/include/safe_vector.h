/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_SAFE_VECTOR_H
#define OHOS_MEDIA_SAFE_VECTOR_H

#include <vector>
#include <mutex>
namespace OHOS::Media {

template <typename T>
class SafeVector {
public:
    SafeVector() = default;
    ~SafeVector() = default;

    void PushBack(const T &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.push_back(value);
    }

    T operator[](size_t index) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_[index];
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.empty();
    }

    std::vector<T> ToVector() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }

    void Remove(const T &value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.erase(std::remove(data_.begin(), data_.end(), value), data_.end());
    }

private:
    std::vector<T> data_;
    mutable std::mutex mutex_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_SAFE_VECTOR_H
