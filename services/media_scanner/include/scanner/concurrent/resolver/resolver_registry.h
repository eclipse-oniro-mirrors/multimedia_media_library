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

#ifndef RESOLVER_REGISTRY_H
#define RESOLVER_REGISTRY_H

#include <map>
#include <memory>

#include "i_conflict_resolver.h"
#include "scan_config.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ResolverRegistry {
public:
    ResolverRegistry();
    ~ResolverRegistry();

    void RegisterResolver(ConflictPolicy policy, std::shared_ptr<IConflictResolver> resolver);
    std::shared_ptr<IConflictResolver> GetResolver(ConflictPolicy policy) const;

    void ClearAllResolvers();

private:
    std::map<ConflictPolicy, std::shared_ptr<IConflictResolver>> resolvers_;
};
} // namespace Media
} // namespace OHOS
#endif // RESOLVER_REGISTRY_H