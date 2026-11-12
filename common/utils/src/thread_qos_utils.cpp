/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "ThreadQosUtils"
 
#include "thread_qos_utils.h"
 
#include <string>
#include <unordered_map>
#include <unistd.h>
 
#include "media_log.h"
#include "concurrent_task_client.h"
#include "qos.h"
 
namespace OHOS {
namespace Media {
 
void ThreadQosUtils::SetThreadQos()
{
    // Same as MediaLibraryDataManager::SetThreadQos: request auth then boost
    // current thread to QOS_USER_INTERACTIVE (QS5).
    std::unordered_map<std::string, std::string> payload;
    payload["pid"] = std::to_string(getpid());
    OHOS::ConcurrentTask::ConcurrentTaskClient::GetInstance().RequestAuth(payload);
    int32_t err = OHOS::QOS::SetThreadQos(QOS::QosLevel::QOS_USER_INTERACTIVE);
    MEDIA_INFO_LOG("set qos level result: %{public}d", err);
    OHOS::QOS::QosLevel qosLevel = OHOS::QOS::QosLevel::QOS_BACKGROUND;
    OHOS::QOS::GetThreadQos(qosLevel);
    MEDIA_INFO_LOG("set qos level: %{public}d", static_cast<int>(qosLevel));
}
 
void ThreadQosUtils::ResetThreadQos()
{
    int32_t err = OHOS::QOS::ResetThreadQos();
    MEDIA_INFO_LOG("reset qos level result: %{public}d", err);
    OHOS::QOS::QosLevel qosLevel = OHOS::QOS::QosLevel::QOS_BACKGROUND;
    OHOS::QOS::GetThreadQos(qosLevel);
    MEDIA_INFO_LOG("reset qos level: %{public}d", static_cast<int>(qosLevel));
}
 
} // namespace Media
} // namespace OHOS