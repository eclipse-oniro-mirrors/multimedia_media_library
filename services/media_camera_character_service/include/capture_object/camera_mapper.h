/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_LIBRARY_CAMERA_MAPPER_H
#define OHOS_MEDIA_LIBRARY_CAMERA_MAPPER_H

#include <map>
#include <string>

#include "multistages_capture_dfx_result.h"
#include "picture.h"

namespace OHOS::Media {
#define IMAGE_FILE_SOURCE_TYPE "image_file_source_type"
#define IMAGE_FILE_EDITED_TYPE "image_file_edited_type"

struct ImageFileMapper {
    void* addr {nullptr};
    int64_t bytes {0};

    bool IsValid() const
    {
        return (addr != nullptr) && (bytes > 0);
    }
};

struct OnProcessParamForNewImage {
    std::map<std::string, ImageFileMapper> files;
    std::shared_ptr<Media::Picture> lcdImage = nullptr;

    void Clear()
    {
        lcdImage.reset();
    }

    bool IsValid() const
    {
        for (const auto& file : files) {
            if (!file.second.IsValid()) {
                return false;
            }
        }
        return true;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "imageFd count: " << std::to_string(files.size()) << ", "
           << "lcdImage: " << std::to_string(static_cast<int32_t>(lcdImage != nullptr))
           << "}";
        return ss.str();
    }
};

struct OnProcessParamForImage {
    ImageFileMapper file;

    bool IsValid() const
    {
        return file.IsValid();
    }
};

struct OnProcessParamForYuv {
    std::shared_ptr<Media::Picture> picture = nullptr;

    bool IsValid() const
    {
        return (picture != nullptr) && (picture->GetMainPixel() != nullptr);
    }
};

enum class FirstStageModifyType : int32_t {
    NOT_MODIFIED = 0,
    EDITED,
    TRASHED,
};

struct MediaDpsMetadata {
    bool isReady{false};

    // 相机框架传入的数据
    uint32_t cloudImageEnhanceFlag{0};
    uint32_t captureEnhancementFlag{0};
    std::string editData;

    // 二阶段过程中, 一阶段的影响
    FirstStageModifyType modifyType{FirstStageModifyType::NOT_MODIFIED};
    // dfx的类型
    MultiStagesCaptureMediaType dfxMediaType{MultiStagesCaptureMediaType::IMAGE};
    
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "cloudImageEnhanceFlag: " << std::to_string(cloudImageEnhanceFlag) << ", "
           << "captureEnhancementFlag: " << std::to_string(captureEnhancementFlag) << ", "
           << "modifyType: " << std::to_string(static_cast<int32_t>(modifyType))
           << "}";
        return ss.str();
    }
};

struct OnProcessImageWrapper {
    OnProcessParamForNewImage newImage;
    OnProcessParamForImage image;
    OnProcessParamForYuv yuv;
    MediaDpsMetadata metadata;
};

enum class MediaDpsErrorCode {
    UNDEFINED = -1,

    // image process error code
    MEDIA_ERROR_IMAGE_PROC_INVALID_PHOTO_ID = 2,
    MEDIA_ERROR_IMAGE_PROC_FAILED = 3,
    MEDIA_ERROR_IMAGE_PROC_TIMEOUT = 4,
    MEDIA_ERROR_IMAGE_PROC_ABNORMAL = 5,
    MEDIA_ERROR_IMAGE_PROC_INTERRUPTED = 6,
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LIBRARY_CAMERA_MAPPER_H