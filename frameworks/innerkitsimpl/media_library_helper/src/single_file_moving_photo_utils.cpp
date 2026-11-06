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

#define MLOG_TAG "SingleFileMovingPhotoUtils"

#include "single_file_moving_photo_utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cinttypes>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "media_string_utils.h"
#include "media_time_utils.h"
#include "medialibrary_errno.h"
#include "mimetype_utils.h"
#include "moving_photo_file_utils.h"
#include "unique_fd.h"

namespace OHOS::Media {
using namespace std;

namespace {
// 重名冲突：原名失败后追加 _<时间戳(ms)>，最多再尝试 10 次，超出即判定失败
constexpr int32_t MAX_CONFLICT_INDEX = 10;
constexpr const char *DEFAULT_IMAGE_EXTENSION = "jpg";
constexpr const char *HEIC_IMAGE_EXTENSION = "heic";
constexpr const char *DEFAULT_VIDEO_EXTENSION = "mp4";
constexpr const char *DECODE_FILE_SUFFIX = "_decode";
constexpr const char *JPEG_IMAGE_EXTENSION = "jpeg";
constexpr const char *HEIF_IMAGE_EXTENSION = "heif";

void ResetFdOffset(const int32_t fd)
{
    if (fd >= 0 && lseek(fd, 0, SEEK_SET) == E_ERR) {  // 复位偏移失败不阻断主流程，仅记录日志
        MEDIA_WARN_LOG("Failed to reset fd offset, errno:%{public}d", errno);
    }
}

int32_t GetFileSize(const int32_t fd, int64_t &size)
{
    off_t endOffset = lseek(fd, 0, SEEK_END);  // 文件末尾位置即文件大小
    CHECK_AND_RETURN_RET_LOG(endOffset != static_cast<off_t>(E_ERR), E_HAS_FS_ERROR,
        "Failed to get file size, errno:%{public}d", errno);
    ResetFdOffset(fd);  // 复位偏移，避免影响后续读取
    size = static_cast<int64_t>(endOffset);  // 取出文件总大小
    CHECK_AND_RETURN_RET_LOG(size > 0, E_INVALID_VALUES, "File size is invalid:%{public}" PRId64, size);
    return E_OK;  // 获取成功
}

int32_t OpenReadOnly(const string &path, UniqueFd &fd)
{
    fd = UniqueFd(MediaFileUtils::OpenFile(path, MEDIA_FILEMODE_READONLY));  // 内部已解析真实路径并校验长度
    CHECK_AND_RETURN_RET_LOG(fd.Get() >= 0, E_HAS_FS_ERROR, "Failed to open file, errno:%{public}d", errno);
    return E_OK;
}

int32_t NormalizeParseError(const int32_t err)
{
    // 文件读写异常归为文件系统错误；tag/长度/metadata 异常统一归为格式错误
    return err == E_HAS_FS_ERROR ? E_HAS_FS_ERROR : E_INVALID_LIVE_PHOTO;
}

string NormalizeExtension(const string &ext)
{
    string result = ext;
    if (!result.empty() && result.front() == '.') {  // 兼容带点后缀
        result.erase(result.begin());
    }
    return MediaStringUtils::ToLower(result);
}

// 图片入参后缀白名单：jpeg / jpg / heif / heic，入参必须是这四类之一
bool IsSupportedImageExtensionName(const string &normalizedExt)
{
    return normalizedExt == "jpg" || normalizedExt == JPEG_IMAGE_EXTENSION ||
        normalizedExt == HEIC_IMAGE_EXTENSION || normalizedExt == HEIF_IMAGE_EXTENSION;
}

// 对外产物图片后缀只有 jpg / heic 两种：jpeg / jpg → jpg，heic / heif → heic，其余 → jpg
string NormalizeOutputImageExtensionName(const string &normalizedExt)
{
    if (normalizedExt == HEIC_IMAGE_EXTENSION || normalizedExt == HEIF_IMAGE_EXTENSION) {  // heif 统一为 heic
        return HEIC_IMAGE_EXTENSION;
    }
    return DEFAULT_IMAGE_EXTENSION;  // jpeg / jpg 及其它一律归一化为 jpg
}

// 取路径中的后缀（不含点、已转小写）：无后缀或点落在目录段时返回空串
string GetImageExtensionFromPath(const string &path)
{
    const size_t slashIndex = path.rfind('/');
    const size_t dotIndex = path.rfind('.');
    if (dotIndex == string::npos || (slashIndex != string::npos && dotIndex < slashIndex)) {
        return "";
    }
    return MediaFileUtils::GetExtensionFromPath(path);  // 统一后缀提取与大小写归一化
}

string JoinPath(const string &dir, const string &name)
{
    return dir + "/" + name;
}

// attempt == 0：使用原名；attempt >= 1：追加 _<baseTs + attempt>（毫秒时间戳）
// 用 baseTs + attempt 而不是每次重新取系统时间，避免同一毫秒内多次重试生成同名文件
string BuildFileName(const string &title, const string &ext, const int64_t baseTs, const int32_t attempt)
{
    string name = title;
    if (attempt > 0) {
        name += "_" + to_string(baseTs + attempt);
    }
    if (!ext.empty()) {
        name += "." + ext;
    }
    return name;
}

// 「按目录拆分」过程中反复用到的命名上下文，聚成一个结构体以满足参数个数约束
struct DecodeNamingContext {
    string imageDir;
    string videoDir;
    string title;
    string imageExt;
    string videoExt;
};

int32_t ResolveDecodeOutputPaths(const DecodeNamingContext &ctx, string &outImagePath, string &outVideoPath)
{
    string imageTitle = ctx.title + DECODE_FILE_SUFFIX;  // 图片名称为源 title + _decode
    string videoTitle = imageTitle;
    if (ctx.imageDir == ctx.videoDir && ctx.imageExt == ctx.videoExt) {  // 同目录同后缀时避免图片视频互相覆盖
        imageTitle += "_image";
        videoTitle += "_video";
    }
    // 图片与视频共用同一个时间戳，保证拆分产物成对；用 baseTs + attempt 避免同毫秒内重试重名
    const int64_t baseTs = MediaTimeUtils::UTCTimeMilliSeconds();
    int32_t attempt = 0;  // 0 表示原名，1~10 表示冲突后追加 _<时间戳>
    while (attempt <= MAX_CONFLICT_INDEX) {
        outImagePath = JoinPath(ctx.imageDir, BuildFileName(imageTitle, ctx.imageExt, baseTs, attempt));
        outVideoPath = JoinPath(ctx.videoDir, BuildFileName(videoTitle, ctx.videoExt, baseTs, attempt));
        if (!MediaPureFileUtils::IsFileExists(outImagePath) &&
            !MediaPureFileUtils::IsFileExists(outVideoPath)) {  // 图片与视频同一组名字均未被占用
            return E_OK;
        }
        attempt++;
    }
    outImagePath.clear();
    outVideoPath.clear();
    return E_INVALID_VALUES;
}

} // namespace

int32_t SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(const std::string &path,
    SingleFileMovingPhotoInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_VALUES, "Path is empty");  // 路径非空校验
    UniqueFd uniqueFd;  // 持有打开的 fd（RAII，自动关闭）
    int32_t err = OpenReadOnly(path, uniqueFd);  // 以只读方式打开文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to open single file moving photo");  // 打开失败透传错误
    return SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(uniqueFd.Get(), info);  // 转调 fd 重载进行解析
}

int32_t SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(const int32_t fd, SingleFileMovingPhotoInfo &info)
{
    info = SingleFileMovingPhotoInfo();  // 先清空，避免失败时向上层残留脏数据
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_INVALID_VALUES, "Invalid fd");  // fd 非法属于参数错误

    int64_t totalSize = 0;  // 文件总大小
    int32_t err = GetFileSize(fd, totalSize);  // 读取文件总大小
    if (err != E_OK) {
        ResetFdOffset(fd);
        CHECK_AND_RETURN_RET_LOG(err == E_HAS_FS_ERROR, E_INVALID_LIVE_PHOTO,  // 空文件按格式错误处理
            "Failed to get file size, err:%{public}d", err);
        return E_HAS_FS_ERROR;
    }

    int64_t imageSize = 0;  // 静态图大小
    int64_t videoSize = 0;  // 视频大小
    int64_t extraDataSize = 0;  // 尾部元数据大小
    err = MovingPhotoFileUtils::GetMovingPhotoDetailedSize(fd, imageSize, videoSize, extraDataSize);  // 复用既有解析
    ResetFdOffset(fd);  // 复用 fd 时不受内部读取位置影响
    if (err != E_OK) {
        MEDIA_DEBUG_LOG("File is not a valid single file moving photo, err:%{public}d", err);
        return NormalizeParseError(err);  // 统一归一化，便于上层映射错误码
    }
    // 只校验各段长度有效，不校验 metadata 中记录的视频长度与解析出的视频段长度一致
    CHECK_AND_RETURN_RET_LOG(imageSize > 0 && videoSize > 0 && extraDataSize > 0, E_INVALID_LIVE_PHOTO,
        "Invalid segments, imageSize:%{public}" PRId64 ", videoSize:%{public}" PRId64
        ", extraDataSize:%{public}" PRId64, imageSize, videoSize, extraDataSize);

    info.totalSize = totalSize;  // 记录总大小
    info.imageSize = imageSize;  // 记录静态图大小
    info.videoSize = videoSize;  // 记录视频大小
    info.extraDataSize = extraDataSize;  // 记录尾部元数据大小
    info.videoOffset = imageSize;  // 视频段偏移等于静态图大小
    return E_OK;  // 解析成功
}

bool SingleFileMovingPhotoUtils::IsSingleFileMovingPhoto(const std::string &path)
{
    SingleFileMovingPhotoInfo info;  // 局部布局信息
    int32_t err = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(path, info);  // 解析文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false,  // 解析失败视为非单文件
        "Failed to parse single file moving photo, err:%{public}d", err);  // 记录失败日志
    return info.IsSingleFileMovingPhoto();  // 返回识别结果
}

bool SingleFileMovingPhotoUtils::IsSingleFileMovingPhoto(const int32_t fd)
{
    SingleFileMovingPhotoInfo info;  // 局部布局信息
    return SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(fd, info) == E_OK;  // 解析成功即为合一文件
}

int32_t SingleFileMovingPhotoUtils::GetVideoOffset(const std::string &path, int64_t &videoOffset)
{
    SingleFileMovingPhotoInfo info;  // 局部布局信息
    int32_t err = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(path, info);  // 解析文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to parse single file moving photo");  // 解析失败透传
    CHECK_AND_RETURN_RET_LOG(info.IsSingleFileMovingPhoto(), E_INVALID_LIVE_PHOTO,  // 非单文件按格式错误处理
        "File is not a single file moving photo");  // 记录失败日志
    videoOffset = info.videoOffset;  // 输出视频偏移
    return E_OK;  // 成功
}

int32_t SingleFileMovingPhotoUtils::GetVideoOffset(const int32_t fd, int64_t &videoOffset)
{
    SingleFileMovingPhotoInfo info;  // 局部布局信息
    int32_t err = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(fd, info);  // 统一走解析入口
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to parse single file moving photo");  // 解析失败透传
    CHECK_AND_RETURN_RET_LOG(info.IsSingleFileMovingPhoto(), E_INVALID_LIVE_PHOTO,  // 非单文件按格式错误处理
        "File is not a single file moving photo");  // 记录失败日志
    videoOffset = info.videoOffset;  // 视频段起始偏移等于静态图大小
    return E_OK;  // 成功
}

int32_t SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhoto(const std::string &srcPath,
    const std::string &outImagePath, const std::string &outVideoPath)
{
    CHECK_AND_RETURN_RET_LOG(!srcPath.empty() && !outImagePath.empty() && !outVideoPath.empty(),
        E_INVALID_VALUES, "Path is empty");  // 路径非空校验

    SingleFileMovingPhotoInfo info;  // 局部布局信息
    int32_t err = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(srcPath, info);  // 解析源文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to parse single file moving photo");  // 解析失败透传
    CHECK_AND_RETURN_RET_LOG(info.IsSingleFileMovingPhoto(), E_INVALID_LIVE_PHOTO,  // 非单文件按格式错误处理
        "File is not a single file moving photo");  // 记录失败日志

    UniqueFd srcFd;  // 源文件 fd（RAII）
    err = OpenReadOnly(srcPath, srcFd);  // 只读打开源文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to open single file moving photo");  // 打开失败透传
    // extraDataPath 为空表示不拆分尾部元数据，保持静态图与视频两份输出
    return SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhoto(srcFd.Get(), outImagePath,
        outVideoPath);  // 转调 fd 重载
}

int32_t SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhoto(const int32_t srcFd,
    const std::string &outImagePath, const std::string &outVideoPath)
{
    CHECK_AND_RETURN_RET_LOG(srcFd >= 0 && !outImagePath.empty() && !outVideoPath.empty(), E_INVALID_VALUES,  // 入参校验
        "Invalid fd or empty path");  // 记录失败日志
    // extraDataPath 为空表示不拆分尾部元数据，保持静态图与视频两份输出
    int32_t err = MovingPhotoFileUtils::ConvertToMovingPhoto(srcFd, outImagePath, outVideoPath, "");  // 复用既有转换拆分
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to decode single file moving photo");  // 拆分失败透传
    // 写盘失败时可能只留下空文件，这里复核产物，避免上层把空文件当成功结果继续处理
    CHECK_AND_RETURN_RET_LOG(CheckDecodedResourceFiles(outImagePath, outVideoPath), E_INVALID_LIVE_PHOTO,
        "Decoded resource of single file moving photo is empty");  // 空产物按格式错误处理
    return E_OK;  // 成功
}

bool SingleFileMovingPhotoUtils::CheckDecodedResourceFiles(const std::string &imagePath,
    const std::string &videoPath)
{
    CHECK_AND_RETURN_RET_LOG(!imagePath.empty() && !videoPath.empty(), false, "Path is empty");  // 路径非空校验
    struct stat imageStat {};
    struct stat videoStat {};
    CHECK_AND_RETURN_RET_LOG(stat(imagePath.c_str(), &imageStat) == 0, false,
        "Failed to stat decoded image, errno:%{public}d", errno);  // 图片产物必须已落盘
    CHECK_AND_RETURN_RET_LOG(stat(videoPath.c_str(), &videoStat) == 0, false,
        "Failed to stat decoded video, errno:%{public}d", errno);  // 视频产物必须已落盘
    CHECK_AND_RETURN_RET_LOG(imageStat.st_size > 0 && videoStat.st_size > 0, false,
        "Decoded resource of single file moving photo is empty, imageSize:%{public}" PRId64
        ", videoSize:%{public}" PRId64, static_cast<int64_t>(imageStat.st_size),
        static_cast<int64_t>(videoStat.st_size));  // 空文件不算拆分成功
    return true;  // 两个产物均有效
}

int32_t SingleFileMovingPhotoUtils::PackingSingleFileMovingPhoto(const std::string &imagePath,
    const std::string &videoPath, const std::string &outSingleFilePath)
{
    CHECK_AND_RETURN_RET_LOG(!imagePath.empty() && !videoPath.empty() && !outSingleFilePath.empty(),
        E_INVALID_VALUES, "Path is empty");  // 路径非空校验

    UniqueFd imageFd;  // 图输入 fd（RAII）
    UniqueFd videoFd;  // 视频输入 fd（RAII）
    int32_t err = OpenReadOnly(imagePath, imageFd);  // 只读打开图片
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to open image file");  // 打开失败透传
    err = OpenReadOnly(videoPath, videoFd);  // 只读打开视频
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to open video file");  // 打开失败透传
    return SingleFileMovingPhotoUtils::PackingSingleFileMovingPhoto(imageFd.Get(), videoFd.Get(),
        outSingleFilePath);  // 转调 fd 重载
}

int32_t SingleFileMovingPhotoUtils::PackingSingleFileMovingPhoto(const int32_t imageFd, const int32_t videoFd,
    const std::string &outSingleFilePath)
{
    CHECK_AND_RETURN_RET_LOG(imageFd >= 0 && videoFd >= 0 && !outSingleFilePath.empty(), E_INVALID_VALUES,  // 入参校验
        "Invalid fd or empty path");  // 记录失败日志
    CHECK_AND_RETURN_RET_LOG(!IsSingleFileMovingPhoto(imageFd), E_INVALID_VALUES,  // 图片入参本身已是合一文件
        "Image fd is already a single file moving photo");
    // 复用端云合并实现：内部写 image + video + 60B 尾部三段标签（v3_f0 / 0:0 / LIVE_<videoSize+20>），
    // 与既有手写尾部字节等价，且格式演进自动跟随端云实现
    return MovingPhotoFileUtils::ConvertToLivePhoto(imageFd, videoFd, outSingleFilePath, 0);  // coverPosition 固定 0
}

int32_t SingleFileMovingPhotoUtils::ResolveOutputImageExtension(const int32_t fd, std::string &extension)
{
    extension.clear();
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_INVALID_VALUES, "Invalid fd");  // fd 非法属于参数错误
    // 按图片内容识别 MIME：走 ImageSource，不受扩展名影响
    string mimeType;
    int32_t err = MimeTypeUtils::GetImageMimetype(fd, mimeType);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES, "Failed to get image mimetype, err:%{public}d", err);
    // 识别出什么后缀就写什么，不再限制在固定白名单内
    err = MimeTypeUtils::GetExtensionFromMimeType(mimeType, extension);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES,
        "Failed to resolve extension of image mimetype:%{public}s", mimeType.c_str());
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::ResolveOutputVideoExtension(const int32_t fd, int64_t videoOffset,
    int64_t videoSize, std::string &extension)
{
    extension.clear();
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_INVALID_VALUES, "Invalid fd");  // fd 非法属于参数错误
    // 只解析视频段区间，合并容器中的图片段与尾部标签不参与识别
    string mimeType;
    int32_t err = MimeTypeUtils::GetVideoMimetype(fd, videoOffset, videoSize, mimeType);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES, "Failed to get video mimetype, err:%{public}d", err);
    // 识别出什么容器就写什么后缀，不再固定为 mp4
    err = MimeTypeUtils::GetExtensionFromMimeType(mimeType, extension);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES,
        "Failed to resolve extension of video mimetype:%{public}s", mimeType.c_str());
    return E_OK;
}

bool SingleFileMovingPhotoUtils::IsSupportedImageExtension(const std::string &extension)
{
    return IsSupportedImageExtensionName(NormalizeExtension(extension));  // 大小写不敏感，兼容带点后缀
}

int32_t SingleFileMovingPhotoUtils::CheckImageSourceExtension(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_VALUES, "Image path is empty");
    const string extension = GetImageExtensionFromPath(path);  // 后缀白名单：jpeg / jpg / heif / heic
    CHECK_AND_RETURN_RET_LOG(IsSupportedImageExtensionName(extension), E_INVALID_VALUES,
        "Unsupported image extension:%{public}s, path:%{private}s", extension.c_str(), path.c_str());
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::ResolvePackingImageExtension(const std::string &imagePath,
    std::string &extension)
{
    extension.clear();
    // 后缀白名单：只允许 jpeg / jpg / heif / heic（大小写不敏感），其它后缀直接拒绝
    CHECK_AND_RETURN_RET_LOG(CheckImageSourceExtension(imagePath) == E_OK, E_INVALID_VALUES,
        "Unsupported packing image extension, path:%{private}s", imagePath.c_str());
    // 内容校验：只接受 JPEG 与 HEIF 两类图片，其余（PNG/GIF/BMP/WebP/TIFF/AVIF 等）一律拒绝
    string mimeType;
    CHECK_AND_RETURN_RET_LOG(MimeTypeUtils::GetImageMimetype(imagePath, mimeType) == E_OK, E_INVALID_VALUES,
        "Failed to get image mimetype, path:%{private}s", imagePath.c_str());
    const bool isHeifContent = (mimeType == "image/heic" || mimeType == "image/heif");
    CHECK_AND_RETURN_RET_LOG(isHeifContent || mimeType == "image/jpeg", E_INVALID_VALUES,
        "Unsupported packing image mimetype:%{public}s", mimeType.c_str());
    // 产物后缀只有 jpg / heic 两种，由图片内容决定：heic / heif → heic，其余（jpeg / jpg）→ jpg
    extension = isHeifContent ? HEIC_IMAGE_EXTENSION : DEFAULT_IMAGE_EXTENSION;
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::CheckPackingVideoMimetype(const std::string &videoPath)
{
    CHECK_AND_RETURN_RET_LOG(!videoPath.empty(), E_INVALID_VALUES, "Video path is empty");
    // 视频后缀只允许 mp4，与图片四种后缀组合成 jpeg/jpg/heif/heic + mp4
    CHECK_AND_RETURN_RET_LOG(GetImageExtensionFromPath(videoPath) == DEFAULT_VIDEO_EXTENSION, E_INVALID_VALUES,
        "Unsupported packing video extension, path:%{private}s", videoPath.c_str());
    // 严格按内容识别：不依赖扩展名，避免把非 MP4 文件改个名字就放行
    string mimeType;
    CHECK_AND_RETURN_RET_LOG(MimeTypeUtils::GetVideoMimetype(videoPath, mimeType) == E_OK, E_INVALID_VALUES,
        "Failed to get video mimetype, path:%{private}s", videoPath.c_str());
    CHECK_AND_RETURN_RET_LOG(mimeType == "video/mp4", E_INVALID_VALUES,
        "Unsupported packing video mimetype:%{public}s", mimeType.c_str());
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::NormalizeOutputDir(const std::string &path, std::string &outDir)
{
    outDir.clear();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_VALUES, "Output dir is empty");  // 空目录视为参数错误
    CHECK_AND_RETURN_RET_LOG(path.find("../") == string::npos, E_INVALID_VALUES,  // 拒绝路径穿越
        "Output dir contains path traversal");

    std::error_code errCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(path, errCode);  // 归一化并消解符号链接
    CHECK_AND_RETURN_RET_LOG(errCode.value() == 0, E_INVALID_VALUES,
        "Failed to canonicalize output dir, err:%{public}d", errCode.value());
    errCode.clear();
    CHECK_AND_RETURN_RET_LOG(std::filesystem::is_directory(canonicalPath, errCode) && errCode.value() == 0,
        E_INVALID_VALUES, "Output dir is not a directory");  // 必须是已存在目录
    outDir = canonicalPath.string();
    CHECK_AND_RETURN_RET_LOG(!outDir.empty(), E_INVALID_VALUES, "Canonical output dir is empty");
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::NormalizeExistingFile(const std::string &path, std::string &outPath)
{
    outPath.clear();
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVALID_VALUES, "Path is empty");  // 空路径视为参数错误
    CHECK_AND_RETURN_RET_LOG(path.find("../") == string::npos, E_INVALID_VALUES,  // 拒绝路径穿越
        "Path contains path traversal, path:%{private}s", path.c_str());

    std::string absPath;  // 解析后的绝对路径
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(path, absPath) && !absPath.empty(), E_INVALID_VALUES,
        "Failed to resolve real path, errno:%{public}d, path:%{private}s", errno, path.c_str());

    std::error_code errCode;
    CHECK_AND_RETURN_RET_LOG(std::filesystem::is_regular_file(absPath, errCode) && errCode.value() == 0,
        E_INVALID_VALUES, "Path is not a regular file, path:%{private}s", absPath.c_str());  // 必须是普通文件
    outPath = absPath;
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhotoToFolder(const DecodeToFolderRequest &request,
    DecodeToFolderResult &result)
{
    result.imagePath.clear();
    result.videoPath.clear();
    CHECK_AND_RETURN_RET_LOG(request.srcFd >= 0, E_INVALID_VALUES, "Invalid fd");  // fd 非法属于参数错误
    CHECK_AND_RETURN_RET_LOG(!request.title.empty(), E_INVALID_VALUES, "Title is empty");  // 源文件名不能为空

    DecodeNamingContext ctx;
    CHECK_AND_RETURN_RET_LOG(NormalizeOutputDir(request.outImageDir, ctx.imageDir) == E_OK, E_INVALID_VALUES,
        "Invalid image output dir");
    CHECK_AND_RETURN_RET_LOG(NormalizeOutputDir(request.outVideoDir, ctx.videoDir) == E_OK, E_INVALID_VALUES,
        "Invalid video output dir");

    SingleFileMovingPhotoInfo info;  // 局部布局信息
    int32_t err = ParseSingleFileMovingPhoto(request.srcFd, info);  // 先校验源文件是合一文件
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to parse single file moving photo");
    // 图片与视频后缀均由实际内容决定，识别失败即判定为非法合一文件，不再降级为默认后缀
    CHECK_AND_RETURN_RET_LOG(ResolveOutputImageExtension(request.srcFd, ctx.imageExt) == E_OK, E_INVALID_VALUES,
        "Failed to resolve image extension of single file moving photo");
    CHECK_AND_RETURN_RET_LOG(ResolveOutputVideoExtension(request.srcFd, info.videoOffset, info.videoSize,
        ctx.videoExt) == E_OK, E_INVALID_VALUES, "Failed to resolve video extension of single file moving photo");
    ctx.title = request.title;

    err = ResolveDecodeOutputPaths(ctx, result.imagePath, result.videoPath);  // 解析不冲突的输出文件名
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err,
        "Failed to resolve output name, retry over %{public}d times", MAX_CONFLICT_INDEX);

    err = MovingPhotoFileUtils::ConvertToMovingPhoto(request.srcFd, result.imagePath, result.videoPath, "");
    // 拆分可能写出空产物，需按落盘结果复核，不能只看返回值
    if (err != E_OK || !CheckDecodedResourceFiles(result.imagePath, result.videoPath)) {
        MEDIA_ERR_LOG("Failed to decode single file moving photo, err:%{public}d", err);
        MediaFileUtils::DeleteFile(result.imagePath);  // 只清理本次创建的产物
        MediaFileUtils::DeleteFile(result.videoPath);
        result.imagePath.clear();
        result.videoPath.clear();
        return err != E_OK ? err : E_INVALID_LIVE_PHOTO;
    }
    // metadata 记录的 videoSize 与实际视频段不一致时，拆出的视频可能损坏，落盘前需按内容复核；
    // 产物后缀已按真实容器决定，这里只校验视频可解析且时长有效，不再按扩展名白名单限制
    if (MovingPhotoFileUtils::GetMovingPhotoVideoDuration(result.videoPath) <= 0) {
        MEDIA_ERR_LOG("The decoded video is invalid, videoPath:%{private}s", result.videoPath.c_str());
        MediaFileUtils::DeleteFile(result.imagePath);
        MediaFileUtils::DeleteFile(result.videoPath);
        result.imagePath.clear();
        result.videoPath.clear();
        return E_INVALID_LIVE_PHOTO;
    }
    return E_OK;
}

int32_t SingleFileMovingPhotoUtils::PackingSingleFileMovingPhotoToFolder(const PackToFolderRequest &request,
    std::string &outPath)
{
    outPath.clear();
    CHECK_AND_RETURN_RET_LOG(request.imageFd >= 0 && request.videoFd >= 0, E_INVALID_VALUES,
        "Invalid fd");  // fd 非法属于参数错误
    CHECK_AND_RETURN_RET_LOG(!request.imageTitle.empty(), E_INVALID_VALUES, "Image title is empty");  // 图片名不能为空

    string dir;  // 归一化后的输出目录
    CHECK_AND_RETURN_RET_LOG(NormalizeOutputDir(request.outDir, dir) == E_OK, E_INVALID_VALUES,
        "Invalid output dir");

    string imageExtension = NormalizeExtension(request.imageExt);  // 图片后缀大小写不敏感
    CHECK_AND_RETURN_RET_LOG(IsSupportedImageExtensionName(imageExtension), E_INVALID_VALUES,
        "Unsupported image extension:%{public}s", imageExtension.c_str());  // 仅支持 jpg/jpeg/heic/heif
    CHECK_AND_RETURN_RET_LOG(NormalizeExtension(request.videoExt) == DEFAULT_VIDEO_EXTENSION, E_INVALID_VALUES,
        "Unsupported video extension:%{public}s", request.videoExt.c_str());  // 仅支持 mp4
    CHECK_AND_RETURN_RET_LOG(!IsSingleFileMovingPhoto(request.imageFd), E_INVALID_VALUES,
        "Image fd is already a single file moving photo");  // 图片入参本身已是合一文件

    // 产物后缀只有 jpg / heic 两种：jpeg / jpg → jpg，heif / heic → heic，不保留入参原后缀
    const string outputExtension = NormalizeOutputImageExtensionName(imageExtension);
    const int64_t baseTs = MediaTimeUtils::UTCTimeMilliSeconds();
    int32_t attempt = 0;  // 0 表示原名，1~10 表示冲突后追加 _<时间戳>
    while (attempt <= MAX_CONFLICT_INDEX) {
        outPath = JoinPath(dir, BuildFileName(request.imageTitle, outputExtension, baseTs, attempt));
        if (!MediaPureFileUtils::IsFileExists(outPath)) {
            break;
        }
        attempt++;
    }
    CHECK_AND_RETURN_RET_LOG(attempt <= MAX_CONFLICT_INDEX, E_INVALID_VALUES,
        "Failed to resolve output name, retry over %{public}d times", MAX_CONFLICT_INDEX);

    int32_t err = MovingPhotoFileUtils::ConvertToLivePhoto(request.imageFd, request.videoFd, outPath, 0);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to pack single file moving photo, err:%{public}d", err);
        MediaFileUtils::DeleteFile(outPath);  // 只清理本次创建的产物
        outPath.clear();
        return err;
    }
    return E_OK;
}
} // namespace OHOS::Media
