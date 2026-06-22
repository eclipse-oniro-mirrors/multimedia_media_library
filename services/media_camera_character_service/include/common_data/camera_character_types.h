/*
 * Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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
 
#ifndef OHOS_CAMERA_CHARATER_TYPES_H
#define OHOS_CAMERA_CHARATER_TYPES_H
 
#include <limits>
#include <string>
#include <tuple>
#include <vector>
 
namespace OHOS {
namespace Media {
const std::string CAMERA_PIPELINE_TYPE = "CameraPipelineType";
const std::string EDIT_DATA = "edit_data";

enum class CameraPipelineType : int32_t {
    UNDEFINED = 0,
    NEW_IMAGE,
    IMAGE,
    YUV,
    VIDEO,
};

enum class CameraPathType : int32_t {
    UNDEFINED = 0,
    // image
    EDITED_PATH,          // IMG_xxx.jpg

    // edit_data
    EDIT_DATA_SOURCE_PATH,          // source.jpg
    EDIT_DATA_CAMERA_PATH,      // editdata_camera

    // temp path
    TEMP_LOW_PATH,                      // low_IMG_1773664117_013.jpg
    TEMP_LOW_FILTERS_PATH,              // low_filters_IMG_1773664117_013.jpg
    TEMP_LOW_EDIT_DATA_SOURCE_PATH,     // low_source.jpg
    TEMP_HIGH_PATH,                     // high_IMG_1773664117_013.jpg
    TEMP_HIGH_FILTERS_PATH,             // high_filters_IMG_1773664117_013.jpg
    TEMP_HIGH_EDIT_DATA_SOURCE_PATH,    // high_source.jpg
};

enum class ObserverType : int32_t {
    UNDEFINED = 0,
    REQUEST_IMAGE,
    REQUEST_VIDEO,
    REQUEST_QUICK_IMAGE,
    OBSERVER_END,
};
 
enum class MultistagesCaptureNotifyType : int32_t {
    UNDEFINED = 0,
    ON_PROCESS_IMAGE_DONE,
    ON_ERROR_IMAGE,
    ON_PROCESS_VIDEO_DONE,
    ON_ERROR_VIDEO,
    YUV_READY,
    NOTIFY_END,
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_FILEMANAGEMENT_USERFILEMGR_TYPES_H