/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_CHECKER_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_CHECKER_H

namespace OHOS {
namespace Media {

class CloudEnhancementChecker {
public:
    static void AddPermissionForCloudEnhancement();
    // 息屏充电任务，将名字带'_enhanced'后缀的非云增强图片识别为云增强图片
    static void RecognizeCloudEnhancementPhotosByDisplayName();
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_CLOUD_ENHANCEMENT_CHECKER_H
