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

#define MLOG_TAG "ResolverRegistry"

#include "resolver_registry.h"

#include "media_log.h"

namespace OHOS {
namespace Media {

ResolverRegistry::ResolverRegistry()
{
    MEDIA_DEBUG_LOG("ResolverRegistry created");
}

ResolverRegistry::~ResolverRegistry()
{
    ClearAllResolvers();
    MEDIA_DEBUG_LOG("ResolverRegistry destroyed");
}

void ResolverRegistry::RegisterResolver(ConflictPolicy policy, std::shared_ptr<IConflictResolver> resolver)
{
    if (resolver == nullptr) {
        MEDIA_ERR_LOG("RegisterResolver: resolver is nullptr");
        return;
    }

    resolvers_[policy] = resolver;
    MEDIA_INFO_LOG("resolver registered (policy %{public}d)", static_cast<int>(policy));
}

std::shared_ptr<IConflictResolver> ResolverRegistry::GetResolver(ConflictPolicy policy) const
{
    auto it = resolvers_.find(policy);
    if (it == resolvers_.end()) {
        MEDIA_WARN_LOG("GetResolver: resolver not found (policy %{public}d)",
            static_cast<int>(policy));
        return nullptr;
    }

    return it->second;
}

void ResolverRegistry::ClearAllResolvers()
{
    resolvers_.clear();
    MEDIA_INFO_LOG("all resolvers cleared");
}

} // namespace Media
} // namespace OHOS