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

#define MLOG_TAG "SingleFileMovingPhotoUtilAni"

#include "single_file_moving_photo_util_ani.h"

#include <array>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "ani_class_name.h"
#include "file_uri.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_uri_utils.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "single_file_moving_photo_utils.h"
#include "unique_fd.h"

namespace OHOS {
namespace Media {
namespace {
// 入参既允许 file:// uri，也允许应用沙箱内的绝对路径；其余形式解析为空路径。
std::string RealPathOf(const std::string &pathOrUri)
{
    if (pathOrUri.rfind("file://", 0) == 0) {
        AppFileService::ModuleFileUri::FileUri fileUri(pathOrUri);
        return fileUri.GetRealPath();
    }
    return pathOrUri;
}

// 打开沙箱文件：入参为 file:// uri 或应用沙箱绝对路径。
// 但含 ../ 的路径一律拒绝，避免借相对路径跳出沙箱。
int32_t OpenByUri(const std::string &pathOrUri, int flags)
{
    if (pathOrUri.find("../") != std::string::npos) {
        ANI_ERR_LOG("OpenByUri rejected, path traversal detected");
        return -1;
    }
    std::string path = RealPathOf(pathOrUri);
    if (path.empty()) {
        ANI_ERR_LOG("OpenByUri failed, real path is empty");
        return -1;
    }
    int32_t fd = open(path.c_str(), flags);
    ANI_INFO_LOG("OpenByUri sandbox uri, fd:%{public}d", fd);
    return fd;
}

// 输出目录必须是已存在目录，且不允许路径穿越
int32_t ResolveOutputDir(const std::string &uri, std::string &outDir)
{
    return SingleFileMovingPhotoUtils::NormalizeOutputDir(RealPathOf(uri), outDir) == E_OK ?
        E_OK : JS_E_PARAM_INVALID;
}

// 从 uri 中提取源文件名：媒体库 URI 取路径最后一段，沙箱 URI 取真实路径文件名。
// 含 ../ 的入参返回空串，由调用方的空名校验拦下。
std::string GetFileNameFromUri(const std::string &uri)
{
    if (uri.empty() || uri.find("../") != std::string::npos) {
        ANI_ERR_LOG("GetFileNameFromUri rejected, path traversal detected");
        return "";
    }
    if (MediaUriUtils::IsLibraryUri(uri)) {
        OHOS::Uri parsedUri(uri);
        const std::string path = parsedUri.GetPath();
        size_t index = path.rfind('/');
        return (index == std::string::npos) ? "" : path.substr(index + 1);
    }
    std::string realPath = RealPathOf(uri);
    if (!realPath.empty()) {
        return MediaFileUtils::GetFileName(realPath);
    }
    return MediaFileUtils::GetFileName(uri);
}

// 拆分文件名中的主名与后缀，后缀保持原始大小写。
void SplitFileName(const std::string &fileName, std::string &title, std::string &extension)
{
    size_t index = fileName.rfind('.');
    if (index == std::string::npos || index == 0 || index == fileName.size() - 1) {
        title = fileName;
        extension.clear();
        return;
    }
    title = fileName.substr(0, index);
    extension = fileName.substr(index + 1);
}
// packing 的严格格式校验：图片后缀需为 jpeg/jpg/heif/heic 且内容为 JPEG/HEIF，
// 视频后缀需为 mp4 且内容为 video/mp4。
// 失败时已抛出参数错误；成功时回填由内容决定的产物图片后缀（只有 jpg / heic 两种）。
bool CheckPackingSourceFormat(ani_env *env, const std::string &imageUri, const std::string &videoUri,
    std::string &imageExtension)
{
    if (SingleFileMovingPhotoUtils::ResolvePackingImageExtension(RealPathOf(imageUri), imageExtension) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__,
            "The image file must be JPEG or HEIF");
        return false;
    }
    if (SingleFileMovingPhotoUtils::CheckPackingVideoMimetype(RealPathOf(videoUri)) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "The video file must be MP4");
        return false;
    }
    return true;
}

// packing 的三个字符串入参
struct PackingStrings {
    std::string imageUri;
    std::string videoUri;
    std::string outFolder;
};

// 解析 packing 的三个字符串入参，任一为空或缺失即报参数错误。
bool ParsePackingStrings(ani_env *env, ani_string imageUriAni, ani_string videoUriAni, ani_string outFolderAni,
    PackingStrings &out)
{
    if (MediaLibraryAniUtils::GetString(env, imageUriAni, out.imageUri) != ANI_OK ||
        MediaLibraryAniUtils::GetString(env, videoUriAni, out.videoUri) != ANI_OK ||
        MediaLibraryAniUtils::GetString(env, outFolderAni, out.outFolder) != ANI_OK ||
        out.imageUri.empty() || out.videoUri.empty() || out.outFolder.empty()) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to get params");
        return false;
    }
    return true;
}

} // namespace

ani_status SingleFileMovingPhotoUtilAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_SINGLE_FILE_MOVING_PHOTO_UTIL.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }
    std::array staticMethods = {
        ani_native_function {"isSingleFileMovingPhotoInner", nullptr,
            reinterpret_cast<void *>(SingleFileMovingPhotoUtilAni::IsSingleFileMovingPhotoInner)},
        ani_native_function {"unpackingSingleFileMovingPhotoInner", nullptr,
            reinterpret_cast<void *>(SingleFileMovingPhotoUtilAni::UnpackingSingleFileMovingPhotoInner)},
        ani_native_function {"packingSingleFileMovingPhotoInner", nullptr,
            reinterpret_cast<void *>(SingleFileMovingPhotoUtilAni::PackingSingleFileMovingPhotoInner)},
        ani_native_function {"getVideoOffsetInner", nullptr,
            reinterpret_cast<void *>(SingleFileMovingPhotoUtilAni::GetVideoOffsetInner)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

// helper 层内部错误码 -> 对外业务错误码，与 NAPI 侧 ToJsErrorCode 保持一致
int32_t SingleFileMovingPhotoUtilAni::ConvertErrCode(int32_t err)
{
    if (err == E_OK) {
        return E_OK;
    }
    if (err == E_INVALID_LIVE_PHOTO) {
        return JS_E_FILE_FORMAT_ERROR;
    }
    return JS_E_PARAM_INVALID;
}

ani_boolean SingleFileMovingPhotoUtilAni::IsSingleFileMovingPhotoInner(ani_env *env,
    [[maybe_unused]] ani_class clazz, ani_string movingPhotoFileUriAni)
{
    std::string uri;
    if (MediaLibraryAniUtils::GetString(env, movingPhotoFileUriAni, uri) != ANI_OK || uri.empty()) {
        ANI_ERR_LOG("isSingleFileMovingPhoto failed to get uri");
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Invalid movingPhotoFileUri");
        return ANI_FALSE;
    }
    // 媒体库 URI 需走数据库与服务端鉴权，本接口按约定直接返回 false
    if (MediaUriUtils::IsLibraryUri(uri)) {
        ANI_DEBUG_LOG("isSingleFileMovingPhoto skip library uri");
        return ANI_FALSE;
    }
    UniqueFd srcFd(OpenByUri(uri, O_RDONLY));
    if (srcFd.Get() < 0) {
        ANI_ERR_LOG("isSingleFileMovingPhoto cannot open uri: %{private}s", uri.c_str());
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to open moving photo file");
        return ANI_FALSE;
    }
    return SingleFileMovingPhotoUtils::IsSingleFileMovingPhoto(srcFd.Get()) ? ANI_TRUE : ANI_FALSE;
}

// 校验入参合一文件后缀并拆分出主名：只允许 jpeg/jpg/heif/heic（大小写不敏感），其余后缀按参数错误拒绝。
bool CheckSingleFileSourceName(ani_env *env, const std::string &srcUri, std::string &title)
{
    std::string sourceExtension;
    SplitFileName(GetFileNameFromUri(srcUri), title, sourceExtension);
    if (!SingleFileMovingPhotoUtils::IsSupportedImageExtension(sourceExtension)) {
        ANI_ERR_LOG("Unsupported single file moving photo extension:%{public}s", sourceExtension.c_str());
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__,
            "The single-file moving photo must be .jpeg/.jpg/.heif/.heic");
        return false;
    }
    return true;
}

// 同步接口：拆分合一文件为图片 + 视频，返回 [图片路径, 视频路径]
ani_object SingleFileMovingPhotoUtilAni::UnpackingSingleFileMovingPhotoInner(ani_env *env,
    [[maybe_unused]] ani_class clazz, ani_string movingPhotoFileUriAni, ani_string outImageFolderAni,
    ani_string outVideoFolderAni)
{
    std::string srcUri;
    std::string outImageFolder;
    std::string outVideoFolder;
    if (MediaLibraryAniUtils::GetString(env, movingPhotoFileUriAni, srcUri) != ANI_OK ||
        MediaLibraryAniUtils::GetString(env, outImageFolderAni, outImageFolder) != ANI_OK ||
        MediaLibraryAniUtils::GetString(env, outVideoFolderAni, outVideoFolder) != ANI_OK ||
        srcUri.empty() || outImageFolder.empty() || outVideoFolder.empty()) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to get params");
        return nullptr;
    }
    UniqueFd srcFd(OpenByUri(srcUri, O_RDONLY));
    if (srcFd.Get() < 0) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to open source uri");
        return nullptr;
    }
    std::string imageDir;
    std::string videoDir;
    if (ResolveOutputDir(outImageFolder, imageDir) != E_OK || ResolveOutputDir(outVideoFolder, videoDir) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Invalid output folder");
        return nullptr;
    }

    std::string title;
    if (!CheckSingleFileSourceName(env, srcUri, title)) {
        return nullptr;
    }
    SingleFileMovingPhotoInfo movingPhotoInfo;
    int32_t err = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(srcFd.Get(), movingPhotoInfo);
    if (err != E_OK) {
        AniError::ThrowError(env, ConvertErrCode(err), __FUNCTION__, __LINE__, "Invalid source file format");
        return nullptr;
    }
    // 图片与视频后缀由拆分内部按内容决定
    SingleFileMovingPhotoUtils::DecodeToFolderRequest request = { srcFd.Get(), title, imageDir, videoDir };
    SingleFileMovingPhotoUtils::DecodeToFolderResult result;
    err = SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhotoToFolder(request, result);
    if (err != E_OK) {
        AniError::ThrowError(env, ConvertErrCode(err), __FUNCTION__, __LINE__,
            "Unpack single file moving photo failed");
        return nullptr;
    }
    ani_object aniResult {};
    if (MediaLibraryAniUtils::ToAniStringArray(env, { result.imagePath, result.videoPath }, aniResult) != ANI_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to build result array");
        return nullptr;
    }
    return aniResult;
}

ani_object SingleFileMovingPhotoUtilAni::PackingSingleFileMovingPhotoInner(ani_env *env,
    [[maybe_unused]] ani_class clazz, ani_string imageUriAni, ani_string videoUriAni,
    ani_string outFolderAni)
{
    PackingStrings args;
    if (!ParsePackingStrings(env, imageUriAni, videoUriAni, outFolderAni, args)) {
        return nullptr;
    }
    UniqueFd imageFd(OpenByUri(args.imageUri, O_RDONLY));
    if (imageFd.Get() < 0) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to open image uri");
        return nullptr;
    }
    UniqueFd videoFd(OpenByUri(args.videoUri, O_RDONLY));
    if (videoFd.Get() < 0) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to open video uri");
        return nullptr;
    }
    std::string outDir;
    if (ResolveOutputDir(args.outFolder, outDir) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Invalid output folder");
        return nullptr;
    }

    std::string imageTitle;
    std::string sourceImageExtension;  // 仅用于拆分文件名，产物后缀改由内容 MIME 决定
    SplitFileName(GetFileNameFromUri(args.imageUri), imageTitle, sourceImageExtension);
    std::string videoTitle;
    std::string videoExtension;
    SplitFileName(GetFileNameFromUri(args.videoUri), videoTitle, videoExtension);
    // 严格格式校验：图片后缀 jpeg/jpg/heif/heic + 内容 JPEG/HEIF，视频后缀 mp4 + 内容 video/mp4
    std::string imageExtension;
    if (!CheckPackingSourceFormat(env, args.imageUri, args.videoUri, imageExtension)) {
        return nullptr;
    }
    std::string outPath;
    SingleFileMovingPhotoUtils::PackToFolderRequest request = {
        imageFd.Get(), videoFd.Get(), outDir, imageTitle, imageExtension, videoExtension };
    int32_t err = SingleFileMovingPhotoUtils::PackingSingleFileMovingPhotoToFolder(request, outPath);
    if (err != E_OK) {
        AniError::ThrowError(env, ConvertErrCode(err), __FUNCTION__, __LINE__,
            "Pack single file moving photo failed");
        return nullptr;
    }
    ani_string aniOutPath {};
    if (MediaLibraryAniUtils::ToAniString(env, outPath, aniOutPath) != ANI_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to build result string");
        return nullptr;
    }
    return aniOutPath;
}

ani_long SingleFileMovingPhotoUtilAni::GetVideoOffsetInner(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_string movingPhotoFileUriAni)
{
    std::string uri;
    if (MediaLibraryAniUtils::GetString(env, movingPhotoFileUriAni, uri) != ANI_OK || uri.empty()) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to get uri");
        return 0;
    }
    // 本接口只处理沙箱文件，媒体库 URI 直接判为参数错误
    if (MediaUriUtils::IsLibraryUri(uri)) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Library uri is not supported");
        return 0;
    }
    UniqueFd srcFd(OpenByUri(uri, O_RDONLY));
    if (srcFd.Get() < 0) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "Failed to open source uri");
        return 0;
    }
    // 入参合一文件后缀只允许 jpeg/jpg/heif/heic，其它后缀按参数错误拒绝
    if (SingleFileMovingPhotoUtils::CheckImageSourceExtension(RealPathOf(uri)) != E_OK) {
        ANI_ERR_LOG("getVideoOffset failed, unsupported source extension");
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__,
            "The single-file moving photo must be .jpeg/.jpg/.heif/.heic");
        return 0;
    }
    int64_t videoOffset = 0;
    int32_t err = SingleFileMovingPhotoUtils::GetVideoOffset(srcFd.Get(), videoOffset);
    if (err != E_OK) {
        AniError::ThrowError(env, ConvertErrCode(err), __FUNCTION__, __LINE__, "Get video offset failed");
        return 0;
    }
    ani_long ret = 0;
    MediaLibraryAniUtils::ToAniLong(env, videoOffset, ret);
    return ret;
}
} // namespace Media
} // namespace OHOS
