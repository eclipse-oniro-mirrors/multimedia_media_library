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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_EDIT_UTILS_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_EDIT_UTILS_H

#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaEditUtils {
public:
    EXPORT static std::string GetEditDataDir(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataCameraPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetTransCodePath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourcePath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourceBackPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataTempPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static std::string GetEditDataSourceTempPath(const std::string &photoPath, int32_t userId = -1);
    EXPORT static bool IsEditDataSourceBackExists(const std::string &photoPath, int32_t userId = -1);
    EXPORT static bool HasEditData(int64_t editTime);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_MEDIA_EDIT_UTILS_H