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

#define MLOG_TAG "SingleFileMovingPhotoUtil"

#include "single_file_moving_photo_util_napi.h"

#include <fcntl.h>
#include <memory>
#include <unistd.h>

#include "file_uri.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_library_error_code.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_utils.h"
#include "single_file_moving_photo_utils.h"
#include "unique_fd.h"
#include "uri.h"

namespace OHOS::Media {
static const std::string SINGLE_FILE_MOVING_PHOTO_UTIL = "SingleFileMovingPhotoUtil";
thread_local napi_ref SingleFileMovingPhotoUtil::constructor_ = nullptr;

napi_value SingleFileMovingPhotoUtil::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = SINGLE_FILE_MOVING_PHOTO_UTIL,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("isSingleFileMovingPhoto", JSIsSingleFileMovingPhoto),
            DECLARE_NAPI_STATIC_FUNCTION("unpackingSingleFileMovingPhoto", JSUnpackingSingleFileMovingPhoto),
            DECLARE_NAPI_STATIC_FUNCTION("packingSingleFileMovingPhoto", JSPackingSingleFileMovingPhoto),
            DECLARE_NAPI_STATIC_FUNCTION("getVideoOffset", JSGetVideoOffset),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value SingleFileMovingPhotoUtil::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    napi_status status = napi_get_new_target(env, info, &newTarget);
    if (status != napi_ok || newTarget == nullptr) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
            "SingleFileMovingPhotoUtil must be called with new");
        return nullptr;
    }
    size_t argc = ARGS_ZERO;
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr);
    return thisVar;
}

void SingleFileMovingPhotoUtil::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    // 本类无 napi_wrap 原生实例，无资源需释放。
}

// 入参既允许 file:// uri，也允许应用沙箱内的绝对路径；其余形式解析为空路径。
static std::string RealPathOf(const std::string& pathOrUri)
{
    if (pathOrUri.rfind("file://", 0) == 0) {
        AppFileService::ModuleFileUri::FileUri fileUri(pathOrUri);
        return fileUri.GetRealPath();
    }
    return pathOrUri;
}

// 打开沙箱文件：入参为 file:// uri 或应用沙箱绝对路径。
// 不做 uri 形态的主动拦截：媒体库 uri 解析不出真实路径时自然打开失败，统一归 23800151。
// 但含 ../ 的路径一律拒绝，避免借相对路径跳出沙箱。
static int32_t OpenByUri(const std::string& pathOrUri, int flags)
{
    if (pathOrUri.find("../") != std::string::npos) {
        MEDIA_ERR_LOG("OpenByUri rejected, path traversal detected");
        return -1;
    }
    std::string path = RealPathOf(pathOrUri);
    if (path.empty()) {
        MEDIA_ERR_LOG("OpenByUri failed, real path is empty");
        return -1;
    }
    int32_t fd = open(path.c_str(), flags);
    MEDIA_INFO_LOG("OpenByUri sandbox file, fd:%{public}d", fd);
    return fd;
}

static bool ParseString(napi_env env, napi_value value, std::string& out)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok || type != napi_string) {
        return false;
    }
    size_t len = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &len) != napi_ok) {
        return false;
    }
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(len + 1);
    if (napi_get_value_string_utf8(env, value, buf.get(), len + 1, &len) != napi_ok) {
        return false;
    }
    out = buf.get();
    return true;
}

// 从 uri 中提取源文件名：媒体库 URI 取路径最后一段，沙箱 URI 取真实路径文件名。
// 含 ../ 的入参返回空串，由调用方的空名校验拦下。
static std::string GetFileNameFromUri(const std::string& uri)
{
    if (uri.empty() || uri.find("../") != std::string::npos) {
        MEDIA_ERR_LOG("GetFileNameFromUri rejected, path traversal detected");
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
static void SplitFileName(const std::string& fileName, std::string& title, std::string& extension)
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

// helper 层内部错误码 -> 对外业务错误码（纯客户端接口，不使用 23800301）
//  - metadata 无法解析 / 空文件 / 图片段或视频段长度非正：23800109（文件格式不正确）
//  - 其余（参数、目录、后缀、重名冲突、读写失败）：23800151（场景参数校验不通过）
static int32_t ToJsErrorCode(int32_t ret)
{
    if (ret == E_OK) {
        return E_OK;
    }
    if (ret == E_INVALID_LIVE_PHOTO) {
        return JS_E_FILE_FORMAT_ERROR;
    }
    return JS_E_PARAM_INVALID;
}

// 抛出 code 为 int 的 BusinessError，并返回 nullptr，供同步接口统一使用
static napi_value ThrowJsError(napi_env env, int32_t code, const char *message)
{
    NapiError::ThrowErrorWithIntCode(env, code, message);
    return nullptr;
}

// 同步接口：判定单文件动图。参数非法或打开失败抛 23800151，媒体库 URI 直接返回 false。
napi_value SingleFileMovingPhotoUtil::JSIsSingleFileMovingPhoto(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { nullptr };
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    std::string uri;
    if (argc < ARGS_ONE || !ParseString(env, argv[PARAM0], uri) || uri.empty()) {
        NapiError::ThrowErrorWithIntCode(env, JS_E_PARAM_INVALID, "The movingPhotoFileUri is invalid");
        return nullptr;
    }
    MEDIA_DEBUG_LOG("isSingleFileMovingPhoto enter, argc:%{public}zu", argc);
    // 媒体库 URI 需走数据库与服务端鉴权，本接口按约定直接返回 false
    if (MediaUriUtils::IsLibraryUri(uri)) {
        MEDIA_DEBUG_LOG("isSingleFileMovingPhoto skip library uri");
        napi_value result = nullptr;
        napi_get_boolean(env, false, &result);
        return result;
    }

    UniqueFd uniqueFd(OpenByUri(uri, O_RDONLY));
    if (uniqueFd.Get() < 0) {
        NapiError::ThrowErrorWithIntCode(env, JS_E_PARAM_INVALID, "Failed to open moving photo file");
        return nullptr;
    }
    bool isLive = SingleFileMovingPhotoUtils::IsSingleFileMovingPhoto(uniqueFd.Get());
    MEDIA_DEBUG_LOG("isSingleFileMovingPhoto exit, isLive:%{public}d", static_cast<int32_t>(isLive));

    napi_value result = nullptr;
    napi_get_boolean(env, isLive, &result);
    return result;
}

// 归一化图片/视频输出目录，任一非法即返回 false（不允许路径穿越、必须是已存在目录）
static bool NormalizeDecodeDirs(const std::string &outImageFolder, const std::string &outVideoFolder,
    std::string &imageDir, std::string &videoDir)
{
    if (SingleFileMovingPhotoUtils::NormalizeOutputDir(RealPathOf(outImageFolder), imageDir) != E_OK ||
        SingleFileMovingPhotoUtils::NormalizeOutputDir(RealPathOf(outVideoFolder), videoDir) != E_OK) {
        MEDIA_ERR_LOG("unpackingSingleFileMovingPhoto failed, output folder is invalid");
        return false;
    }
    return true;
}

// 构造 [图片路径, 视频路径] 的 JS 字符串数组
static napi_value BuildTwoStringArray(napi_env env, const std::string &imagePath, const std::string &videoPath)
{
    napi_value result = nullptr;
    napi_value imageValue = nullptr;
    napi_value videoValue = nullptr;
    if (napi_create_array_with_length(env, ARGS_TWO, &result) != napi_ok ||
        napi_create_string_utf8(env, imagePath.c_str(), NAPI_AUTO_LENGTH, &imageValue) != napi_ok ||
        napi_create_string_utf8(env, videoPath.c_str(), NAPI_AUTO_LENGTH, &videoValue) != napi_ok ||
        napi_set_element(env, result, PARAM0, imageValue) != napi_ok ||
        napi_set_element(env, result, PARAM1, videoValue) != napi_ok) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to build the unpacking result");
    }
    return result;
}

// 校验入参合一文件后缀并拆分出主名：只允许 jpeg/jpg/heif/heic（大小写不敏感），其余后缀按参数错误拒绝。
static bool CheckSingleFileSourceName(napi_env env, const std::string& srcUri, std::string& title)
{
    std::string sourceExtension;
    SplitFileName(GetFileNameFromUri(srcUri), title, sourceExtension);
    if (!SingleFileMovingPhotoUtils::IsSupportedImageExtension(sourceExtension)) {
        MEDIA_ERR_LOG("Unsupported single file moving photo extension:%{public}s", sourceExtension.c_str());
        ThrowJsError(env, JS_E_PARAM_INVALID, "The single-file moving photo must be .jpeg/.jpg/.heif/.heic");
        return false;
    }
    return true;
}

// 同步接口：拆分合一文件为图片 + 视频，返回 [图片路径, 视频路径]
napi_value SingleFileMovingPhotoUtil::JSUnpackingSingleFileMovingPhoto(napi_env env, napi_callback_info info)
{
    constexpr size_t argCount = ARGS_THREE;
    size_t argc = argCount;
    napi_value argv[argCount] = { nullptr };
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    std::string srcUri;
    std::string outImageFolder;
    std::string outVideoFolder;
    if (argc != argCount || !ParseString(env, argv[PARAM0], srcUri) ||
        !ParseString(env, argv[PARAM1], outImageFolder) || !ParseString(env, argv[PARAM2], outVideoFolder) ||
        srcUri.empty() || outImageFolder.empty() || outVideoFolder.empty()) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The parameters of unpackingSingleFileMovingPhoto are invalid");
    }
    UniqueFd srcFd(OpenByUri(srcUri, O_RDONLY));
    if (srcFd.Get() < 0) {
        // 源文件找不到或无法打开属于场景参数校验失败
        MEDIA_ERR_LOG("unpackingSingleFileMovingPhoto failed to open src uri");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to open the single-file moving photo");
    }
    std::string imageDir;
    std::string videoDir;
    if (!NormalizeDecodeDirs(outImageFolder, outVideoFolder, imageDir, videoDir)) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The output folder is invalid");
    }

    std::string title;
    if (!CheckSingleFileSourceName(env, srcUri, title)) {
        return nullptr;
    }
    SingleFileMovingPhotoInfo movingPhotoInfo;
    int32_t ret = SingleFileMovingPhotoUtils::ParseSingleFileMovingPhoto(srcFd.Get(), movingPhotoInfo);
    if (ret != E_OK) {
        return ThrowJsError(env, ToJsErrorCode(ret), "Failed to parse the single-file moving photo");
    }
    // 图片与视频后缀由拆分内部按内容决定
    SingleFileMovingPhotoUtils::DecodeToFolderRequest request = { srcFd.Get(), title, imageDir, videoDir };
    SingleFileMovingPhotoUtils::DecodeToFolderResult result;
    ret = SingleFileMovingPhotoUtils::DecodeSingleFileMovingPhotoToFolder(request, result);
    if (ret != E_OK) {
        return ThrowJsError(env, ToJsErrorCode(ret), "Failed to unpack the single-file moving photo");
    }
    MEDIA_DEBUG_LOG("unpackingSingleFileMovingPhoto exit, argc:%{public}zu", argc);
    return BuildTwoStringArray(env, result.imagePath, result.videoPath);
}

// 同步接口：打包图片 + 视频为合一文件，返回生成的合一文件路径
// packing 的严格格式校验：图片后缀需为 jpeg/jpg/heif/heic 且内容为 JPEG/HEIF，
// 视频后缀需为 mp4 且内容为 video/mp4。
// 失败时已抛出参数错误；成功时回填由内容决定的产物图片后缀（只有 jpg / heic 两种）。
static bool CheckPackingSourceFormat(napi_env env, const std::string &imageUri, const std::string &videoUri,
    std::string &imageExtension)
{
    if (SingleFileMovingPhotoUtils::ResolvePackingImageExtension(RealPathOf(imageUri), imageExtension) != E_OK) {
        MEDIA_ERR_LOG("packingSingleFileMovingPhoto failed, image is not JPEG or HEIF");
        ThrowJsError(env, JS_E_PARAM_INVALID, "The image file must be JPEG or HEIF");
        return false;
    }
    if (SingleFileMovingPhotoUtils::CheckPackingVideoMimetype(RealPathOf(videoUri)) != E_OK) {
        MEDIA_ERR_LOG("packingSingleFileMovingPhoto failed, video is not MP4");
        ThrowJsError(env, JS_E_PARAM_INVALID, "The video file must be MP4");
        return false;
    }
    return true;
}

// 解析 packing 的三个字符串入参（图片 / 视频 / 输出目录），任一为空或缺失即报参数错误。
static bool ParsePackingStrings(napi_env env, napi_callback_info info, std::string &imageUri,
    std::string &videoUri, std::string &outFolder)
{
    constexpr size_t argCount = ARGS_THREE;
    size_t argc = argCount;
    napi_value argv[argCount] = { nullptr };
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (argc != argCount || !ParseString(env, argv[PARAM0], imageUri) ||
        !ParseString(env, argv[PARAM1], videoUri) || !ParseString(env, argv[PARAM2], outFolder) ||
        imageUri.empty() || videoUri.empty() || outFolder.empty()) {
        ThrowJsError(env, JS_E_PARAM_INVALID, "The parameters of packingSingleFileMovingPhoto are invalid");
        return false;
    }
    return true;
}

napi_value SingleFileMovingPhotoUtil::JSPackingSingleFileMovingPhoto(napi_env env, napi_callback_info info)
{
    std::string imageUri;
    std::string videoUri;
    std::string outFolder;
    if (!ParsePackingStrings(env, info, imageUri, videoUri, outFolder)) {
        return nullptr;
    }
    MEDIA_DEBUG_LOG("packingSingleFileMovingPhoto enter");

    UniqueFd imageFd(OpenByUri(imageUri, O_RDONLY));
    if (imageFd.Get() < 0) {
        MEDIA_ERR_LOG("packingSingleFileMovingPhoto failed to open image uri");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to open the image file");
    }
    UniqueFd videoFd(OpenByUri(videoUri, O_RDONLY));
    if (videoFd.Get() < 0) {
        MEDIA_ERR_LOG("packingSingleFileMovingPhoto failed to open video uri");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to open the video file");
    }
    std::string outDir;
    if (SingleFileMovingPhotoUtils::NormalizeOutputDir(RealPathOf(outFolder), outDir) != E_OK) {
        MEDIA_ERR_LOG("packingSingleFileMovingPhoto failed, output folder is invalid");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The output folder is invalid");
    }

    std::string imageTitle;
    std::string sourceImageExtension;  // 仅用于拆分文件名，产物后缀改由内容 MIME 决定
    SplitFileName(GetFileNameFromUri(imageUri), imageTitle, sourceImageExtension);
    std::string videoTitle;
    std::string videoExtension;
    SplitFileName(GetFileNameFromUri(videoUri), videoTitle, videoExtension);
    // 严格格式校验：图片后缀 jpeg/jpg/heif/heic + 内容 JPEG/HEIF，视频后缀 mp4 + 内容 video/mp4
    std::string imageExtension;
    if (!CheckPackingSourceFormat(env, imageUri, videoUri, imageExtension)) {
        return nullptr;
    }
    std::string outPath;
    SingleFileMovingPhotoUtils::PackToFolderRequest request = {
        imageFd.Get(), videoFd.Get(), outDir, imageTitle, imageExtension, videoExtension };
    int32_t ret = SingleFileMovingPhotoUtils::PackingSingleFileMovingPhotoToFolder(request, outPath);
    if (ret != E_OK) {
        return ThrowJsError(env, ToJsErrorCode(ret), "Failed to pack the single-file moving photo");
    }

    napi_value result = nullptr;
    if (napi_create_string_utf8(env, outPath.c_str(), NAPI_AUTO_LENGTH, &result) != napi_ok) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to build the packing result");
    }
    MEDIA_DEBUG_LOG("packingSingleFileMovingPhoto exit, path:%{private}s", outPath.c_str());
    return result;
}

// 同步接口：获取合一文件中视频的起始偏移；媒体库 uri 直接判为参数错误
napi_value SingleFileMovingPhotoUtil::JSGetVideoOffset(napi_env env, napi_callback_info info)
{
    constexpr size_t argCount = ARGS_ONE;
    size_t argc = argCount;
    napi_value argv[argCount] = { nullptr };
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    std::string srcUri;
    if (argc != argCount || !ParseString(env, argv[PARAM0], srcUri) || srcUri.empty()) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The movingPhotoFileUri is invalid");
    }
    // 本接口只处理沙箱文件，媒体库 URI 直接判为参数错误
    if (MediaUriUtils::IsLibraryUri(srcUri)) {
        MEDIA_ERR_LOG("getVideoOffset does not support library uri");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "getVideoOffset does not support media library uri");
    }
    UniqueFd srcFd(OpenByUri(srcUri, O_RDONLY));
    if (srcFd.Get() < 0) {
        MEDIA_ERR_LOG("getVideoOffset failed to open src uri");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to open the single-file moving photo");
    }
    // 入参合一文件后缀只允许 jpeg/jpg/heif/heic，其它后缀按参数错误拒绝
    if (SingleFileMovingPhotoUtils::CheckImageSourceExtension(RealPathOf(srcUri)) != E_OK) {
        MEDIA_ERR_LOG("getVideoOffset failed, unsupported source extension");
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The single-file moving photo must be .jpeg/.jpg/.heif/.heic");
    }
    int64_t offset = 0;
    int32_t ret = SingleFileMovingPhotoUtils::GetVideoOffset(srcFd.Get(), offset);
    if (ret != E_OK) {
        return ThrowJsError(env, ToJsErrorCode(ret), "Failed to get the video offset");
    }

    napi_value result = nullptr;
    constexpr int32_t MAX_INT32 = 0x7FFFFFFF;
    if (offset > static_cast<int64_t>(MAX_INT32)) {
        MEDIA_ERR_LOG("The video offset exceeds int32 range: %{public}lld", static_cast<long long>(offset));
        return ThrowJsError(env, JS_E_PARAM_INVALID, "The video offset is out of range");
    }
    if (napi_create_int32(env, static_cast<int32_t>(offset), &result) != napi_ok) {
        return ThrowJsError(env, JS_E_PARAM_INVALID, "Failed to build the video offset result");
    }
    return result;
}
} // namespace OHOS::Media
