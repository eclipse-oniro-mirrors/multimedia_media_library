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

#include "resolver_registry_test.h"

#include "media_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void ResolverRegistryTest::SetUp()
{
    registry_ = std::make_shared<ResolverRegistry>();
}

void ResolverRegistryTest::TearDown()
{
    registry_ = nullptr;
}

/**
 * @tc.name: ResolverRegistry_RegisterAndGet_test01
 * @tc.desc: 注册和获取resolver测试
 */
HWTEST_F(ResolverRegistryTest, ResolverRegistry_RegisterAndGet_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ResolverRegistry_RegisterAndGet_test01");
    auto retrieved = registry_->GetResolver(ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(retrieved, nullptr);

    auto resolver = std::make_shared<QualityConflictResolver>();
    registry_->RegisterResolver(ConflictPolicy::QUALITY_PRIORITY, resolver);
    retrieved = registry_->GetResolver(ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(retrieved, resolver);

    registry_->RegisterResolver(ConflictPolicy::QUALITY_PRIORITY, nullptr);
    retrieved = registry_->GetResolver(ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_NE(retrieved, nullptr);

    auto resolver2 = std::make_shared<QualityConflictResolver>();
    registry_->RegisterResolver(ConflictPolicy::QUALITY_PRIORITY, resolver2);
    retrieved = registry_->GetResolver(ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(retrieved, resolver2);
    MEDIA_INFO_LOG("end ResolverRegistry_RegisterAndGet_test01");
}

/**
 * @tc.name: ResolverRegistry_ClearAllResolvers_test01
 * @tc.desc: ClearAllResolvers清除所有resolver
 */
HWTEST_F(ResolverRegistryTest, ResolverRegistry_ClearAllResolvers_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter ResolverRegistry_ClearAllResolvers_test01");
    auto resolver = std::make_shared<QualityConflictResolver>();
    registry_->RegisterResolver(ConflictPolicy::QUALITY_PRIORITY, resolver);

    registry_->ClearAllResolvers();

    auto retrieved = registry_->GetResolver(ConflictPolicy::QUALITY_PRIORITY);
    EXPECT_EQ(retrieved, nullptr);
    MEDIA_INFO_LOG("end ResolverRegistry_ClearAllResolvers_test01");
}

} // namespace Media
} // namespace OHOS