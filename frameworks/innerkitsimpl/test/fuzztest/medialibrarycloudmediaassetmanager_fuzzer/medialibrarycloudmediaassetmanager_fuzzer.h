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

#ifndef NOTIFYCHANGE_FUZZER_H
#define NOTIFYCHANGE_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibrarycloudmediaassetmanager_fuzzer"

#include "userfilemgr_uri.h"
#include "cloud_media_asset_uri.h"

namespace OHOS {
namespace Media {
const std::vector<std::string> CLOUD_MEDIA_ASSET_MANAGER_FUZZER_URI_LISTS = {
    // PhotoAccessHelper cloud media asset manager
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_FORCE,
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_GENTLE,
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_PAUSE,
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_CANCEL,
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE,
    CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY,
};
} // namespace Media
} // namespace OHOS
#endif
