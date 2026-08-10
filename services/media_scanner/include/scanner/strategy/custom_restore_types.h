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
 
#ifndef CUSTOM_RESTORE_TYPES_H
#define CUSTOM_RESTORE_TYPES_H
 
#include <string>
#include <vector>
 
#include "userfile_manager_types.h"
 
namespace OHOS {
namespace Media {
 
struct RestoreFileInfo {
    std::string originFilePath;
    std::string filePath;
    std::string fileName;
    std::string displayName;
    std::string title;
    std::string extension;
    MediaType mediaType{MEDIA_TYPE_FILE};
    int32_t size{0};
    int32_t orientation{0};
    bool isLivePhoto{false};
    int32_t fileId{0};
    std::string mimeType;
    int32_t subtype{0};
    int32_t movingPhotoEffectMode{0};
    std::string frontCamera;
    std::string shootingMode;
    int32_t albumId{0};
};
 
struct UniqueNumber {
    int32_t imageTotalNumber = 0;
    int32_t videoTotalNumber = 0;
    int32_t imageCurrentNumber = 0;
    int32_t videoCurrentNumber = 0;
 
    UniqueNumber operator+(const UniqueNumber &other) const
    {
        UniqueNumber result;
        result.imageTotalNumber = this->imageTotalNumber + other.imageTotalNumber;
        result.videoTotalNumber = this->videoTotalNumber + other.videoTotalNumber;
        result.imageCurrentNumber = this->imageCurrentNumber + other.imageCurrentNumber;
        result.videoCurrentNumber = this->videoCurrentNumber + other.videoCurrentNumber;
        return result;
    }
 
    void clear()
    {
        imageTotalNumber = 0;
        videoTotalNumber = 0;
        imageCurrentNumber = 0;
        videoCurrentNumber = 0;
    }
};
 
struct TimeInfo {
    int64_t dateAdded{0};
    int64_t dateTaken{0};
    std::string detailTime;
};
 
} // namespace Media
} // namespace OHOS
 
#endif // CUSTOM_RESTORE_TYPES_H