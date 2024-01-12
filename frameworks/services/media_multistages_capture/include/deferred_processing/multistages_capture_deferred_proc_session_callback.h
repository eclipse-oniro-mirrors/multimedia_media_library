/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DEFERRED_PROC_SESSION_CALLBACK_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DEFERRED_PROC_SESSION_CALLBACK_H

#include <memory>
#include <string>

#include "deferred_photo_proc_session.h"

namespace OHOS {
namespace Media {
class MultiStagesCaptureDeferredProcSessionCallback : public CameraStandard::IDeferredPhotoProcSessionCallback {
public:
    void OnProcessImageDone(const std::string &imageId, const uint8_t *addr, const long bytes) override;
    void OnError(const std::string &imageId, const CameraStandard::DpsErrorCode error) override;
    void OnStateChanged(const CameraStandard::DpsStatusCode state) override;
};

} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DEFERRED_PROC_SESSION_CALLBACK_H