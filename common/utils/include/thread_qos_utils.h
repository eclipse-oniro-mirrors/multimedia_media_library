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
 
#ifndef MEDIA_THREAD_QOS_UTILS_H
#define MEDIA_THREAD_QOS_UTILS_H
 
namespace OHOS {
namespace Media {
 
class ThreadQosUtils {
public:
    // Boost current thread qos to QOS_USER_INTERACTIVE (QS5) via qos_manager.
    static void SetThreadQos();
 
    // Reset current thread qos to default via qos_manager.
    static void ResetThreadQos();
};
 
} // namespace Media
} // namespace OHOS
 
#endif // MEDIA_THREAD_QOS_UTILS_H