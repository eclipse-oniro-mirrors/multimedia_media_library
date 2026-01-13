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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_COMM_NAPI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_COMM_NAPI_H

#include <ani.h>
#include <string>
//LCOV_EXCL_START
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaLibraryCommAni {
public:
    MediaLibraryCommAni();
    ~MediaLibraryCommAni();
    EXPORT static ani_object CreatePhotoAssetAni(ani_env *env, const std::string &uri,
        int32_t cameraShotType, const std::string &burstKey = "");
    EXPORT static ani_object CreatePhotoAssetAni(ani_env *env, const std::string &uri,
        int32_t cameraShotType, int32_t captureId, const std::string &burstKey = "");
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_COMM_NAPI_H
//LCOV_EXCL_STOP
