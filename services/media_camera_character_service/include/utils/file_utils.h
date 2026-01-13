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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_FILE_UTILS_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_FILE_UTILS_H

#include <string>
#include "picture.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class FileUtils {
public:
    FileUtils();
    ~FileUtils();
    EXPORT static int32_t SaveImage(const std::string &filePath, void *output, size_t writeSize);
    EXPORT static int32_t SavePicture(const std::string &imageId,
        std::shared_ptr<Media::Picture> &picture, bool isEdited, bool isLowQualityPicure = false);
    EXPORT static int32_t SavePicture(const std::string &path, std::shared_ptr<Media::Picture> &picture,
        const std::string &mime_type, bool isEdited = false);
    EXPORT static int DealPicture(const std::string &mime_type, const std::string &path,
        std::shared_ptr<Media::Picture> &picture, bool isHighQualityPicture);
    EXPORT static int32_t SaveVideo(const std::string &filePath, bool isEdited = false);
    EXPORT static int32_t SaveMovingPhotoVideo(const std::string &filePath, bool isEdited,
        bool isMovingPhotoEffectMode);
    EXPORT static int32_t DeleteTempVideoFile(const std::string &filePath);
    EXPORT static int DeleteFile(const std::string &fileName);
    EXPORT static bool IsFileExist(const std::string &fileName);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_FILE_UTILS_H