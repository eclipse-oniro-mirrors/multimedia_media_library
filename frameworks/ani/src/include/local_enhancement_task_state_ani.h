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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_ANI_H

#include <ani.h>
#include <memory>
#include "cloud_enhancement_ani.h"

namespace OHOS {
namespace Media {

class LocalEnhancementTaskStateAni {
public:
    LocalEnhancementTaskStateAni() = default;
    ~LocalEnhancementTaskStateAni() = default;
    static ani_object NewLocalEnhancementTaskStateAni(ani_env *env, unique_ptr<CloudEnhancementAniContext> &context);
    static ani_status Init(ani_env *env);

private:
    static ani_object Constructor(ani_env *env, ani_class cls,
        std::unique_ptr<LocalEnhancementTaskStateAni> &nativeHandle);
    static LocalEnhancementTaskStateAni* Unwrap(ani_env *env, ani_object object);

    static ani_int GetProgress(ani_env *env, ani_object object);
    void SetProgress(int32_t progress);

    int32_t progress_ = 0;
};

} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_LOCAL_ENHANCEMENT_TASK_STATE_ANI_H