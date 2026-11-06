/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H
#define OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H

#include <unordered_map>

#include "nlohmann/json.hpp"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MimeTypeUtils {
public:
    EXPORT MimeTypeUtils(const MimeTypeUtils&) = delete;
    EXPORT MimeTypeUtils(MimeTypeUtils&&) = delete;
    EXPORT MimeTypeUtils& operator=(const MimeTypeUtils&) = delete;
    EXPORT MimeTypeUtils& operator=(MimeTypeUtils&&) = delete;
    EXPORT static bool IsMimeTypeMapEmpty();
    EXPORT static int32_t InitMimeTypeMap();
    EXPORT static std::string GetMimeTypeFromContent(const std::string &filePath);
    EXPORT static std::string GetMimeTypeFromExtension(const std::string &extension);
    EXPORT static std::string GetMimeTypeFromExtension(const std::string &extension,
        const std::unordered_map<std::string, std::vector<std::string>> &mimeTypeMap);
    /**
     * @brief 按 MIME 反查常见后缀（不带点、已转小写）。
     *
     * 同一 MIME 可能对应多个扩展名（如 image/jpeg → jpeg/jpg），优先返回 jpg / mp4 等最常见写法，
     * 否则取映射表中的首个扩展名，保证拆分产物的后缀与内容一致。
     * @return 成功返回 E_OK 并回填 extension；MIME 未知或无对应扩展名返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t GetExtensionFromMimeType(const std::string &mimeType, std::string &extension);
    EXPORT static MediaType GetMediaTypeFromMimeType(const std::string &mimeType);
    EXPORT static MediaType GetMediaType(const std::string &filePath);
    /**
     * @brief 按图片内容识别 MIME 类型（走 ImageSource 解析文件头，不依赖扩展名）。
     * @return 成功返回 E_OK 并回填 mimeType（如 image/jpeg、image/heic），失败返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t GetImageMimetype(const std::string &filePath, std::string &mimeType);
    /**
     * @brief 按 fd 指向的图片内容识别 MIME 类型，不接管 fd 所有权（由调用方关闭）。
     */
    EXPORT static int32_t GetImageMimetype(const int32_t fd, std::string &mimeType);
    /**
     * @brief 按视频内容识别 MIME 类型（走 AVMetadataHelper 解析，不依赖扩展名）。
     * @return 成功返回 E_OK 并回填 mimeType（如 video/mp4），失败返回其余错误码。
     */
    EXPORT static int32_t GetVideoMimetype(const std::string &filePath, std::string &mimeType);
    /**
     * @brief 按 fd 指定区间识别视频 MIME 类型，供合一文件只读取视频段时使用，不接管 fd 所有权。
     * @param offset 视频段起始偏移；@param size 视频段长度。
     */
    EXPORT static int32_t GetVideoMimetype(const int32_t fd, int64_t offset, int64_t size, std::string &mimeType);

private:
    static void CreateMapFromJson();

    static std::mutex lockCreateMap_;

    static std::unordered_map<std::string, std::vector<std::string>> mediaJsonMap_;
};
}
}
#endif //OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H
