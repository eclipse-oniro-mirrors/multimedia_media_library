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

#ifndef CLOUD_MEDIA_ASSET_URI_H
#define CLOUD_MEDIA_ASSET_URI_H
#include "base_data_uri.h"

namespace OHOS {
namespace Media {
#define CONST_CLOUD_MEDIA_ASSET_OPERATE "cloud_media_asset_operate"

#define CONST_CLOUD_MEDIA_ASSET_TASK_START_FORCE "cloud_media_asset_task_start_force"
#define CONST_CLOUD_MEDIA_ASSET_TASK_START_GENTLE "cloud_media_asset_task_start_gentle"
#define CONST_CLOUD_MEDIA_ASSET_TASK_PAUSE "cloud_media_asset_task_pause"
#define CONST_CLOUD_MEDIA_ASSET_TASK_CANCEL "cloud_media_asset_task_cancel"
#define CONST_CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE "cloud_media_asset_task_retain_force"
#define CONST_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY "cloud_media_asset_task_status_query"

#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_FORCE \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_start_force"
#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_START_GENTLE \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_start_gentle"
#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_PAUSE \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_pause"
#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_CANCEL \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_cancel"
#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_retain_force"
#define CONST_CMAM_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY \
    "datashare:///media/cloud_media_asset_operate/cloud_media_asset_task_status_query"
} // namespace Media
} // namespace OHOS

#endif // CLOUD_MEDIA_ASSET_URI_H