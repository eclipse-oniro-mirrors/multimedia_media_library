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

#ifndef SCAN_TASK_CONTEXT_H
#define SCAN_TASK_CONTEXT_H

#include "scan_config.h"

namespace OHOS {
namespace Media {
struct ScanTaskContext {
    ScanConfig config;

    explicit ScanTaskContext(const ScanConfig& config) : config(config) {}
};
} // namespace Media
} // namespace OHOS
#endif // SCAN_TASK_CONTEXT_H