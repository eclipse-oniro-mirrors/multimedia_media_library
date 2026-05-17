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

#ifndef QUALITY_CONFLICT_RESOLVER_TEST_H
#define QUALITY_CONFLICT_RESOLVER_TEST_H

#include <gtest/gtest.h>
#include <memory>

#include "quality_conflict_resolver.h"
#include "scan_config_builder.h"
#include "scan_task_context.h"

namespace OHOS {
namespace Media {

class QualityConflictResolverTest : public testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

    std::shared_ptr<ScanTaskContext> CreateTask(ScanQuality quality);

    std::shared_ptr<QualityConflictResolver> resolver_ = nullptr;
};

} // namespace Media
} // namespace OHOS

#endif // QUALITY_CONFLICT_RESOLVER_TEST_H