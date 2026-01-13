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
#ifndef MEDIA_LIBRARY_MEDIA_PROCESSOR_REGISTER_H
#define MEDIA_LIBRARY_MEDIA_PROCESSOR_REGISTER_H

#include "i_processor.h"
#include "media_composite_processor.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace OHOS {
namespace Media {
using ProcessorFactory = std::function<std::unique_ptr<IProcessor>()>;
using ProcessorKey = std::pair<FileNotifyObjectType, FileNotifyOperationType>;

struct ProcessorKeyHash {
    std::size_t operator()(const ProcessorKey& key) const
    {
        return std::hash<int>{}(static_cast<int>(key.first)) ^ (std::hash<int>{}(static_cast<int>(key.second)) << 1);
    }
};

class MediaProcessorRegistry {
public:
    static MediaProcessorRegistry& GetInstance()
    {
        static MediaProcessorRegistry instance;
        return instance;
    }

    void Register(const ProcessorKey& key, ProcessorFactory factory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        registry_[key] = std::move(factory);
    }

    // 多个Processor按序复合处理（只在传入工厂数量 > 1 时启用）
    template <typename... Factories, typename = std::enable_if_t<(sizeof...(Factories) > 1)>>
    void Register(const ProcessorKey& key, Factories&&... factories)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bool needPostProcess = (key.second == FileNotifyOperationType::MOD);
        registry_[key] = [f = std::make_tuple(std::forward<Factories>(factories)...), needPostProcess]() {
            std::vector<std::unique_ptr<IProcessor>> processors;
            processors.reserve(sizeof...(factories));

            std::apply([&](auto&&... fac) {
                (processors.push_back(fac()), ...);
            }, f);
            return std::make_unique<MediaCompositeProcessor>(std::move(processors), needPostProcess);
        };
    }

    std::unique_ptr<IProcessor> CreateProcessor(const ProcessorKey& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = registry_.find(key);
        if (it != registry_.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    MediaProcessorRegistry() = default;
    ~MediaProcessorRegistry() = default;
    MediaProcessorRegistry(const MediaProcessorRegistry&) = delete;
    MediaProcessorRegistry& operator=(const MediaProcessorRegistry&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<ProcessorKey, ProcessorFactory, ProcessorKeyHash> registry_;
};
}
}

#endif // MEDIA_LIBRARY_MEDIA_PROCESSOR_REGISTER_H