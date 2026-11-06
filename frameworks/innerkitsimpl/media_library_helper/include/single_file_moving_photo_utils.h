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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTILS_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTILS_H

#include <cstdint>
#include <string>

#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * @brief 单文件动态照片的布局信息，由 ParseSingleFileMovingPhoto 一次性解析得到，
 * 供识别、拆分、取偏移量三个能力共享，避免重复 IO 与重复格式判定。
 */
struct SingleFileMovingPhotoInfo {
    int64_t totalSize = 0;
    int64_t imageSize = 0;       // 静态图大小，即视频相对文件头的偏移量
    int64_t videoSize = 0;
    int64_t extraDataSize = 0;   // 尾部元数据大小，仅私有格式有效
    int64_t videoOffset = 0;     // 视频段起始偏移（字节）

    bool IsSingleFileMovingPhoto() const  // 判断是否为有效的单文件动态照片
    {
        return imageSize > 0 && videoSize > 0;  // 静态图与视频大小均有效即为合一文件
    }
};

/**
 * @brief 动图合一工具类（Native 层）。所有接口均为静态方法，只做无状态的文件处理，
 * URI 语义、权限、异步调度由上层（NAPI/ANI）负责。
 */
class SingleFileMovingPhotoUtils {
public:
    /**
     * @brief 「合一文件 → 图片 + 视频」按目录拆分的入参。
     * srcFd 由调用方持有，本类不接管所有权；imageExt / videoExt 不带点。
     */
    struct DecodeToFolderRequest {
        int32_t srcFd = -1;
        std::string title;         // 输出文件名主名（源合一文件主名）
        std::string outImageDir;   // 图片输出目录（未归一化，内部会校验）
        std::string outVideoDir;   // 视频输出目录（未归一化，内部会校验）
    };

    /** @brief 「合一文件 → 图片 + 视频」按目录拆分的结果，仅成功时有效。 */
    struct DecodeToFolderResult {
        std::string imagePath;
        std::string videoPath;
    };

    /**
     * @brief 「图片 + 视频 → 合一文件」按目录打包的入参。
     * imageFd / videoFd 由调用方持有，本类不接管所有权；imageExt / videoExt 不带点。
     */
    struct PackToFolderRequest {
        int32_t imageFd = -1;
        int32_t videoFd = -1;
        std::string outDir;        // 输出目录（未归一化，内部会校验）
        std::string imageTitle;    // 产物文件名主名
        std::string imageExt;      // 图片后缀，最终归一化为 jpg / heic
        std::string videoExt;      // 视频后缀，固定 mp4
    };
    /**
     * @brief 解析单文件动态照片的布局。
     * 解析成功返回 E_OK；metadata 不规范返回 E_INVALID_LIVE_PHOTO；文件 IO 异常返回 E_HAS_FS_ERROR。
     */
    EXPORT static int32_t ParseSingleFileMovingPhoto(const std::string &path, SingleFileMovingPhotoInfo &info);
    EXPORT static int32_t ParseSingleFileMovingPhoto(const int32_t fd, SingleFileMovingPhotoInfo &info);

    /** 识别文件是否为单文件动态照片，非单文件返回 false。 */
    EXPORT static bool IsSingleFileMovingPhoto(const std::string &path);
    EXPORT static bool IsSingleFileMovingPhoto(const int32_t fd);

    /** 获取视频段在合一文件中的起始偏移（字节）。 */
    EXPORT static int32_t GetVideoOffset(const std::string &path, int64_t &videoOffset);
    EXPORT static int32_t GetVideoOffset(const int32_t fd, int64_t &videoOffset);

    /** 将合一文件拆分为独立的静态图文件与视频文件。 */
    EXPORT static int32_t DecodeSingleFileMovingPhoto(const std::string &srcPath, const std::string &outImagePath,
        const std::string &outVideoPath);
    EXPORT static int32_t DecodeSingleFileMovingPhoto(const int32_t srcFd, const std::string &outImagePath,
        const std::string &outVideoPath);

    /** 将独立的静态图与视频合并为合一文件。 */
    EXPORT static int32_t PackingSingleFileMovingPhoto(const std::string &imagePath, const std::string &videoPath,
        const std::string &outSingleFilePath);
    EXPORT static int32_t PackingSingleFileMovingPhoto(const int32_t imageFd, const int32_t videoFd,
        const std::string &outSingleFilePath);

    /**
     * @brief 复核拆分产物：图片与视频都必须已落盘且大小大于 0。
     *        拆分链路在写盘失败时可能只留下空文件，不复核会把空文件当成功结果返回。
     * @return 两个产物均存在且非空返回 true，其余返回 false。
     */
    EXPORT static bool CheckDecodedResourceFiles(const std::string &imagePath, const std::string &videoPath);

    /**
     * @brief 按 fd 指向的图片内容识别拆分产物图片后缀，识别出什么就写什么（如 image/jpeg → "jpg"、
     *        image/heic → "heic"、image/png → "png"），不再限制在固定白名单内。
     * @return 成功返回 E_OK 并回填不带点的小写后缀；内容无法识别或 MIME 无对应扩展名时返回
     *         E_INVALID_VALUES，不降级为默认后缀。
     */
    EXPORT static int32_t ResolveOutputImageExtension(const int32_t fd, std::string &extension);

    /**
     * @brief 按 fd 指定区间识别拆分产物视频后缀，识别出什么容器就写什么后缀（如 video/mp4 → "mp4"、
     *        video/quicktime → "mov"），不再固定为 mp4。
     * @return 成功返回 E_OK 并回填后缀；内容无法识别或 MIME 无对应扩展名时返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t ResolveOutputVideoExtension(const int32_t fd, int64_t videoOffset, int64_t videoSize,
        std::string &extension);

    /**
     * @brief 图片后缀白名单：jpeg / jpg / heif / heic 四类，大小写不敏感且允许带点。
     * @return 命中返回 true，无后缀或其它后缀返回 false。
     */
    EXPORT static bool IsSupportedImageExtension(const std::string &extension);

    /**
     * @brief 按路径后缀校验图片白名单（jpeg / jpg / heif / heic）。
     * @return 命中返回 E_OK，无后缀或其它后缀返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t CheckImageSourceExtension(const std::string &path);

    /**
     * @brief packing 的严格图片格式校验：后缀必须是 jpeg/jpg/heif/heic，内容必须是 JPEG 或 HEIF。
     * @return 成功返回 E_OK 并回填归一化产物后缀（由内容决定：jpeg/jpg → "jpg"，heic/heif → "heic"）；
     *         不满足任一条件时返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t ResolvePackingImageExtension(const std::string &imagePath, std::string &extension);

    /**
     * @brief packing 的严格视频格式校验：后缀必须是 mp4，内容必须是 MP4。
     * @return 满足条件返回 E_OK，其余（含识别失败）返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t CheckPackingVideoMimetype(const std::string &videoPath);

    /**
     * @brief 校验并归一化输出目录：拒绝 ../、要求路径存在且为目录。
     * @return 成功返回 E_OK 并回填 canonical 路径，否则返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t NormalizeOutputDir(const std::string &path, std::string &outDir);

    /**
     * @brief 校验并归一化已存在的文件路径：拒绝 ../，解析真实路径，并要求必须是普通文件。
     * @return 成功返回 E_OK 并回填绝对路径，路径非法或不是普通文件返回 E_INVALID_VALUES。
     */
    EXPORT static int32_t NormalizeExistingFile(const std::string &path, std::string &outPath);

    /**
     * @brief 将合一文件拆分到指定目录，命名为 title_decode.<imageExt>/<videoExt>，同名时追加 _(1)~(10)。
     * 失败时只清理本次调用创建的文件，不删除目录。
     */
    EXPORT static int32_t DecodeSingleFileMovingPhotoToFolder(const DecodeToFolderRequest &request,
        DecodeToFolderResult &result);

    /**
     * @brief 将图片和视频打包到指定目录，命名为图片原名，同名时追加 _(1)~(10)。
     * imageExt 仅支持 jpg/jpeg/heic/heif，videoExt 仅支持 mp4；imageFd 本身是合一文件时返回参数错误。
     * 失败时只清理本次调用创建的文件，不删除目录。
     */
    EXPORT static int32_t PackingSingleFileMovingPhotoToFolder(const PackToFolderRequest &request,
        std::string &outPath);
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTILS_H
