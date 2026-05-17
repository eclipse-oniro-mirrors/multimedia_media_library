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

#ifndef RESOLVER_REGISTRY_TEST_H
#define RESOLVER_REGISTRY_TEST_H

#include <gtest/gtest.h>
#include <memory>

#include "resolver_registry.h"
#include "quality_conflict_resolver.h"
#include "i_conflict_resolver.h"

namespace OHOS {
namespace Media {

class ResolverRegistryTest : public testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    std::shared_ptr<ResolverRegistry> registry_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // RESOLVER_REGISTRY_TEST_H