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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_CAMERA_PATH_UTILS_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_CAMERA_PATH_UTILS_H

#include <cstdint>
#include <string>

#include "camera_character_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class CameraPathUtils {
public:
    // 获取path
    EXPORT static void GetCameraPath(const CameraPathType& type, const std::string& inputPath, std::string& outputPath);
    static std::string GetEditDataDir(const std::string& originPath, int32_t userId = -1);
    static std::string GetCacheDir(const std::string& originPath, int32_t userId = -1);

    EXPORT static std::string GetRealPathFromTempPath(const std::string& path, const CameraPathType& tempPathType,
        CameraPathType& realPathType);
    EXPORT static bool SaveTemporaryImage(const std::string& realPath, const std::string& tempPath);
    // 水印信息的保存与读取
    EXPORT static int32_t SaveEditDataCameraByString(const std::string& path, const std::string& editdata,
        const std::string& bundleName);
    EXPORT static int32_t SaveEditDataCameraByStruct(const std::string& path, const std::string& compatibleFormat,
        const std::string& formatVersion, const std::string& editdata, const std::string& bundleName);
    EXPORT static int32_t ReadEditdataCameraFromFile(const std::string& path, bool onlyForEditdata,
        std::string& editdata);

private:
    CameraPathUtils();
    ~CameraPathUtils();

    // 实际目录
    static void GetEditedPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetEditDataSourcePath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetEditDataCameraPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);

    // 临时目录
    static void GetTempLowPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetTempLowFiltersPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetTempLowEditDataSourcePath(const std::string& inputPath, std::string& outputPath,
        int32_t userId = -1);
    static void GetTempHighPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetTempHighFiltersPath(const std::string& inputPath, std::string& outputPath, int32_t userId = -1);
    static void GetTempHighEditDataSourcePath(const std::string& inputPath, std::string& outputPath,
        int32_t userId = -1);
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_CAMERA_PATH_UTILS_H