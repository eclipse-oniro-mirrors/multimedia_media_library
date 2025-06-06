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

#include "sys_utils.h"

#include <cerrno>
#include <sched.h>
#include <unistd.h>

#include "media_log.h"

namespace OHOS::Media::CloudSync {
void SysUtils::SlowDown()
{
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    for (int cpu = CPU_IDX_0; cpu <= CPU_IDX_6; cpu++) {
        CPU_SET(cpu, &cpuSet);
    }
    if (sched_setaffinity(0, sizeof(cpuSet), &cpuSet) < 0) {
        MEDIA_ERR_LOG("set thread affinity failed, errno %{public}d", errno);
    }
}

void SysUtils::ResetCpu()
{
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);

    if (sched_setaffinity(0, sizeof(cpuSet), &cpuSet) < 0) {
        MEDIA_ERR_LOG("set thread affinity failed, errno %{public}d", errno);
        return;
    }
}
}  // namespace OHOS::Media::CloudSync