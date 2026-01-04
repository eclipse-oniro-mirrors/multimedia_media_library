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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H

#include "multistages_photo_capture_manager.h"
#include "multistages_video_capture_manager.h"

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MultiStagesCaptureManager {
public:
    EXPORT MultiStagesCaptureManager();
    EXPORT ~MultiStagesCaptureManager();

    EXPORT static void RemovePhotos(const NativeRdb::AbsRdbPredicates &predicates,
        bool isRestorable = true);
    EXPORT static void RemovePhotosWithResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        bool isRestorable = true);
    EXPORT static void RestorePhotos(const NativeRdb::AbsRdbPredicates &predicates);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_MANAGER_H