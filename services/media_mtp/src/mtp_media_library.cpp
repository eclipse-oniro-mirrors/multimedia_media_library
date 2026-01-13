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
#define MLOG_TAG "MtpMediaLibrary"

#include "mtp_media_library.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <shared_mutex>
#include "mtp_data_utils.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "mtp_error_utils.h"
#include "mtp_file_observer.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
#include "post_proc.h"
#include "image_packer.h"
#include "avmetadatahelper.h"
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
namespace {
using ReadLock = std::shared_lock<std::shared_mutex>;
using WriteLock = std::lock_guard<std::shared_mutex>;
const std::string PUBLIC_REAL_PATH_PRE               = "/storage/media/";
const std::string PUBLIC_REAL_PATH_END               = "/local/files/Docs";
const std::string PUBLIC_DOC                         = "/storage/media/local/files/Docs";
const std::string SD_DOC                             = "/storage/External";
const std::string TRASH_DIR_NAME                     = "/storage/media/local/files/Docs/.Trash";
const std::string RECENT_DIR_NAME                    = "/storage/media/local/files/Docs/.Recent";
const std::string THUMBS_DIR_NAME                    = "/storage/media/local/files/Docs/.thumbs";
const std::string BACKUP_DIR_NAME                    = "/storage/media/local/files/Docs/.backup";
const std::string APPDATA_DIR_NAME                   = "/storage/media/local/files/Docs/appdata";
const std::string DESKTOP_NAME                       = "/storage/media/local/files/Docs/Desktop";
const std::string PATH_SEPARATOR                     = "/";
constexpr int32_t NORMAL_WIDTH                       = 256;
constexpr int32_t NORMAL_HEIGHT                      = 256;
constexpr int32_t COMPRE_SIZE_LEVEL_2                = 204800;
constexpr int32_t PATH_TIMEVAL_MAX                   = 2;
const std::string THUMBNAIL_FORMAT                   = "image/jpeg";
static constexpr uint8_t THUMBNAIL_MID               = 90;
constexpr int32_t SHORT_SIDE_THRESHOLD               = 350;
constexpr int32_t MAXIMUM_SHORT_SIDE_THRESHOLD       = 1050;
constexpr int32_t ASPECT_RATIO_THRESHOLD             = 3;
static std::unordered_map<uint32_t, std::string> handleToPathMap;
static std::unordered_map<std::string, uint32_t> pathToHandleMap;
static std::shared_mutex g_mutex;
static std::unordered_map<uint32_t, std::string> storageIdToPathMap;
} // namespace

std::atomic<uint32_t> MtpMediaLibrary::id_ = 0;
std::shared_ptr<MtpMediaLibrary> MtpMediaLibrary::instance_ = nullptr;
constexpr int32_t ROTATE_ANGLE_90 = 90;
constexpr int32_t ROTATE_ANGLE_270 = 270;

std::shared_ptr<MtpMediaLibrary> MtpMediaLibrary::GetInstance()
{
    static std::once_flag oc;
    std::call_once(oc, []() {
        instance_ = std::make_shared<MtpMediaLibrary>();
        if (instance_ != nullptr) {
            instance_->Init();
        }
    });
    return instance_;
}

void MtpMediaLibrary::Init()
{
    id_ = START_ID;
    {
        WriteLock lock(g_mutex);
        handleToPathMap.clear();
        pathToHandleMap.clear();
        storageIdToPathMap.clear();
        std::unordered_map<uint32_t, std::string>().swap(handleToPathMap);
        std::unordered_map<std::string, uint32_t>().swap(pathToHandleMap);
        std::unordered_map<uint32_t, std::string>().swap(storageIdToPathMap);
    }
    // clear all storages, otherwise it maybe has duty data.
    auto manager = MtpStorageManager::GetInstance();
    if (manager != nullptr) {
        manager->ClearStorages();
    }
}

void MtpMediaLibrary::Clear()
{
    MEDIA_INFO_LOG("MtpMediaLibrary::Clear is called");
    Init();
}

uint32_t MtpMediaLibrary::GetId()
{
    return id_++;
}

static bool IsHiddenDirectory(const std::string &dir)
{
    CHECK_AND_RETURN_RET_LOG(!dir.empty(), false, "dir is empty");
    static const std::unordered_map<std::string, uint8_t> hiddenDirs = {
        {TRASH_DIR_NAME, 0},
        {RECENT_DIR_NAME, 0},
        {THUMBS_DIR_NAME, 0},
        {BACKUP_DIR_NAME, 0},
        {APPDATA_DIR_NAME, 0},
        {DESKTOP_NAME, 0}
    };
    CHECK_AND_RETURN_RET(hiddenDirs.find(dir) != hiddenDirs.end(), false);
    return true;
}

static bool IsRootPath(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), false, "path is empty");
    for (const auto &it : storageIdToPathMap) {
        CHECK_AND_RETURN_RET(path.compare(it.second) != 0, true);
    }
    return false;
}

static void GetStatTime(const std::string &fromPath, const std::string &toPath, bool recursive,
    std::unordered_map<std::string, std::pair<long, long>> &statTimeMap)
{
    std::error_code ec;
    std::vector<std::string> pathList;
    pathList.push_back(fromPath);
    if (recursive && sf::is_directory(fromPath, ec)) {
        for (const auto& entry : sf::recursive_directory_iterator(fromPath, ec)) {
            if (ec.value() == MTP_SUCCESS) {
                pathList.push_back(entry.path().string());
            }
        }
    }

    struct stat statInfo = {};
    for (const auto &path : pathList) {
        if (stat(path.c_str(), &statInfo) != 0) {
            MEDIA_WARN_LOG("stat fromPath:%{public}s failed", path.c_str());
            continue;
        }
        std::string to = path;
        to.replace(0, fromPath.size(), toPath);
        statTimeMap[to].first = statInfo.st_ctime;
        statTimeMap[to].second = statInfo.st_mtime;
    }
}

static void SetStatTime(const std::unordered_map<std::string, std::pair<long, long>> &statTimeMap)
{
    struct timeval times[PATH_TIMEVAL_MAX] = { { 0, 0 }, { 0, 0 } };
    for (auto it = statTimeMap.begin(); it != statTimeMap.end(); it++) {
        times[0].tv_sec = it->second.first;
        times[1].tv_sec = it->second.second;
        if (utimes(it->first.c_str(), times) != 0) {
            MEDIA_WARN_LOG("utimes toPath:%{public}s failed", it->first.c_str());
        }
    }
}

int32_t MtpMediaLibrary::ScanDirNoDepth(const std::string &root, std::shared_ptr<UInt32List> &out)
{
    CHECK_AND_RETURN_RET_LOG(out != nullptr, E_ERR, "out is nullptr");
    CHECK_AND_RETURN_RET_LOG(access(root.c_str(), R_OK) == 0, E_ERR, "access failed root[%{public}s]", root.c_str());
    bool cond = (!sf::exists(root) || !sf::is_directory(root));
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR,
        "MtpMediaLibrary::ScanDirNoDepth root[%{public}s] is not exists", root.c_str());
    std::error_code ec;
    for (const auto& entry : sf::directory_iterator(root, ec)) {
        if (ec.value() != MTP_SUCCESS) {
            continue;
        }
        // show not recycle dir
        if (sf::is_directory(entry.path(), ec) && IsHiddenDirectory(entry.path().string())) {
            continue;
        }
        uint32_t id = AddPathToMap(entry.path().string());
        out->push_back(id);
    }
    return MTP_SUCCESS;
}

void MtpMediaLibrary::AddToHandlePathMap(const std::string &path, const uint32_t id)
{
    if (handleToPathMap.find(id) != handleToPathMap.end()) {
        handleToPathMap.erase(id);
    }
    if (pathToHandleMap.find(path) != pathToHandleMap.end()) {
        pathToHandleMap.erase(path);
    }
    pathToHandleMap.emplace(path, id);
    handleToPathMap.emplace(id, path);
}

void MtpMediaLibrary::ModifyHandlePathMap(const std::string &from, const std::string &to)
{
    auto it = pathToHandleMap.find(from);
    CHECK_AND_RETURN_LOG(it != pathToHandleMap.end(), "MtpMediaLibrary::ModifyHandlePathMap from not found");
    uint32_t id = it->second;
    pathToHandleMap.erase(it);
    pathToHandleMap.emplace(to, id);

    auto iter = handleToPathMap.find(id);
    if (iter != handleToPathMap.end()) {
        handleToPathMap.erase(iter);
        handleToPathMap.emplace(id, to);
    }
}

void MtpMediaLibrary::ModifyPathHandleMap(const std::string &path, const uint32_t id)
{
    auto it = pathToHandleMap.find(path);
    CHECK_AND_RETURN_LOG(it != pathToHandleMap.end(), "MtpMediaLibrary::ModifyPathHandleMap from not found");

    uint32_t originalId = it->second;
    pathToHandleMap.erase(it);
    pathToHandleMap.emplace(path, id);

    auto iter = handleToPathMap.find(originalId);
    if (iter != handleToPathMap.end()) {
        handleToPathMap.erase(iter);
        handleToPathMap.emplace(id, path);
    }
}

bool MtpMediaLibrary::StartsWith(const std::string& str, const std::string& prefix)
{
    if (prefix.size() > str.size() || prefix.empty() || str.empty()) {
        MEDIA_DEBUG_LOG("MtpMediaLibrary::StartsWith prefix size error");
        return false;
    }

    for (size_t i = 0; i < prefix.size(); ++i) {
        if (str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

void MtpMediaLibrary::DeleteHandlePathMap(const std::string &path, const uint32_t id)
{
    WriteLock lock(g_mutex);
    if (pathToHandleMap.find(path) != pathToHandleMap.end()) {
        pathToHandleMap.erase(path);
    }
    if (handleToPathMap.find(id) != handleToPathMap.end()) {
        handleToPathMap.erase(id);
    }
}

uint32_t MtpMediaLibrary::ObserverAddPathToMap(const std::string &path)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::ObserverAddPathToMap path[%{public}s]", path.c_str());
    {
        WriteLock lock(g_mutex);
        return AddPathToMap(path);
    }
}

void MtpMediaLibrary::ObserverDeletePathToMap(const std::string &path)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::ObserverDeletePathToMap path[%{public}s]", path.c_str());
    {
        WriteLock lock(g_mutex);
        auto it = pathToHandleMap.find(path);
        if (it == pathToHandleMap.end()) {
            return;
        }
        ErasePathInfo(it->second, path);
    }
}

void MtpMediaLibrary::MoveHandlePathMap(const std::string &from, const std::string &to)
{
    std::string prefix = from + "/";
    for (auto it = pathToHandleMap.begin(); it != pathToHandleMap.end();) {
        if (StartsWith(it->first, prefix)) {
            uint32_t eachId = it->second;
            std::string eachStr = it->first;
            it = pathToHandleMap.erase(it);

            std::string eachSuffixString = eachStr.substr(prefix.size());
            std::string newPath = to + "/" + eachSuffixString;
            pathToHandleMap.emplace(newPath, eachId);

            auto iter = handleToPathMap.find(eachId);
            if (iter != handleToPathMap.end()) {
                handleToPathMap.erase(iter);
                handleToPathMap.emplace(eachId, newPath);
            }
        } else {
            ++it;
        }
    }
}

void MtpMediaLibrary::MoveRepeatDirHandlePathMap(const std::string &from, const std::string &to)
{
    std::string prefix = from + "/";
    for (auto it = pathToHandleMap.begin(); it != pathToHandleMap.end();) {
        if (StartsWith(it->first, prefix)) {
            uint32_t eachId = it->second;
            std::string eachStr = it->first;
            it = pathToHandleMap.erase(it);

            std::string eachSuffixString = eachStr.substr(prefix.size());
            std::string newPath = to + "/" + eachSuffixString;
            pathToHandleMap.emplace(newPath, eachId);

            auto iter = handleToPathMap.find(eachId);
            if (iter != handleToPathMap.end()) {
                handleToPathMap.erase(iter);
                handleToPathMap.emplace(eachId, newPath);
            }
        } else {
            ++it;
        }
    }
    uint32_t originToId = pathToHandleMap[to];
    auto iterator = pathToHandleMap.find(from);
    if (iterator != pathToHandleMap.end()) {
        uint32_t id = iterator->second;
        pathToHandleMap.erase(iterator);
        pathToHandleMap[to] = id;

        auto iter = handleToPathMap.find(id);
        if (iter != handleToPathMap.end()) {
            handleToPathMap.erase(originToId);
            handleToPathMap[id] = to;
        }
    }
}

int32_t MtpMediaLibrary::GetHandles(int32_t parentId, std::vector<int> &outHandles, MediaType mediaType)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::GetHandles parent[%{public}d]", parentId);
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(parentId, path) == MTP_SUCCESS,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "MtpMediaLibrary::GetHandles parent not found");
    std::shared_ptr<UInt32List> out = std::make_shared<UInt32List>();
    {
        WriteLock lock(g_mutex);
        ScanDirNoDepth(path, out);
    }
    for (const auto &handle : *out) {
        outHandles.push_back(handle);
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::GetHandles(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt32List> &outHandles)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "context is nullptr");
    uint32_t parentId = context->parent;
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathByContextParent(context, path) == MTP_SUCCESS,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::GetHandles parent[%{public}d] not found", parentId);
    MEDIA_DEBUG_LOG("MtpMediaLibrary::GetHandles path[%{public}s]", path.c_str());
    int32_t errCode;
    {
        WriteLock lock(g_mutex);
        errCode = ScanDirNoDepth(path, outHandles);
    }
    return errCode;
}

uint32_t MtpMediaLibrary::GetParentId(const std::string &path)
{
    ReadLock lock(g_mutex);
    auto parentPath = sf::path(path).parent_path().string();
    auto it = pathToHandleMap.find(parentPath);
    CHECK_AND_RETURN_RET(it != pathToHandleMap.end(), 0);
    return it->second;
}

uint32_t MtpMediaLibrary::GetSizeFromOfft(const off_t &size)
{
    return size > std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : size;
}

int32_t MtpMediaLibrary::GetObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<ObjectInfo> &outObjectInfo)
{
    bool cond = (context == nullptr || context->handle <= 0 || outObjectInfo == nullptr);
    CHECK_AND_RETURN_RET_LOG(!cond, MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "handle error");
    MEDIA_DEBUG_LOG("MtpMediaLibrary::GetObjectInfo storageID[%{public}d]", context->storageID);
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, path) == MTP_SUCCESS,
        MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "MtpMediaLibrary::GetObjectInfo handle not found");
    outObjectInfo->handle = context->handle;
    outObjectInfo->name = sf::path(path).filename().string();
    outObjectInfo->parent = GetParentId(path);
    outObjectInfo->storageID = context->storageID;
    MtpDataUtils::GetMtpFormatByPath(path, outObjectInfo->format);
    MediaType mediaType;
    MtpDataUtils::GetMediaTypeByformat(outObjectInfo->format, mediaType);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE || mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_2;
        outObjectInfo->thumbFormat = MTP_FORMAT_EXIF_JPEG_CODE;
        outObjectInfo->thumbPixelHeight = NORMAL_HEIGHT;
        outObjectInfo->thumbPixelWidth = NORMAL_WIDTH;
    }

    std::error_code ec;
    if (sf::is_directory(path, ec)) {
        outObjectInfo->format = MTP_FORMAT_ASSOCIATION_CODE;
    }
    struct stat statInfo;
    CHECK_AND_RETURN_RET_LOG(stat(path.c_str(), &statInfo) == 0,
        MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "MtpMediaLibrary::GetObjectInfo stat failed");
    outObjectInfo->size = GetSizeFromOfft(statInfo.st_size);
    outObjectInfo->dateCreated = statInfo.st_ctime;
    outObjectInfo->dateModified = statInfo.st_mtime;
    return MtpErrorUtils::SolveGetObjectInfoError(E_SUCCESS);
}

bool MtpMediaLibrary::IsExistObject(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, false, "context is nullptr");

    std::string realPath("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, realPath) == MTP_SUCCESS, false,
        "MtpMediaLibrary::IsExistObject handle not found");
    bool ret = sf::exists(realPath);
    if (!ret) {
        DeleteHandlePathMap(realPath, context->handle);
    }
    return ret;
}

int32_t MtpMediaLibrary::GetFd(const std::shared_ptr<MtpOperationContext> &context, int32_t &outFd, bool forWrite)
{
    MEDIA_INFO_LOG("MtpMediaLibrary::GetFd");
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "context is nullptr");
    std::string realPath("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, realPath) == MTP_SUCCESS,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "MtpMediaLibrary::GetFd handle not found");

    std::error_code ec;
    realPath = sf::weakly_canonical(realPath, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS,
        MtpErrorUtils::SolveGetFdError(E_HAS_FS_ERROR), "MtpMediaLibrary::GetFd normalized realPath failed");

    int mode = O_RDONLY;
    if (forWrite) {
        mode = sf::exists(realPath, ec) ? O_RDWR : (O_RDWR | O_CREAT);
    }

    outFd = open(realPath.c_str(), mode);
    MEDIA_INFO_LOG("MTP:file %{public}s fd %{public}d", realPath.c_str(), outFd);
    CHECK_AND_RETURN_RET(outFd <= 0, MtpErrorUtils::SolveGetFdError(E_SUCCESS));
    return MtpErrorUtils::SolveGetFdError(E_HAS_FS_ERROR);
}

bool MtpMediaLibrary::CompressImage(PixelMap &pixelMap, std::vector<uint8_t> &data)
{
    PackOption option = {
        .format = THUMBNAIL_FORMAT,
        .quality = THUMBNAIL_MID,
        .numberHint = 1
    };
    data.resize(pixelMap.GetByteCount());

    ImagePacker imagePacker;
    uint32_t errorCode = imagePacker.StartPacking(data.data(), data.size(), option);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to StartPacking %{public}d", errorCode);

    errorCode = imagePacker.AddImage(pixelMap);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to AddImage %{public}d", errorCode);

    int64_t packedSize = 0;
    errorCode = imagePacker.FinalizePacking(packedSize);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to FinalizePacking %{public}d", errorCode);

    data.resize(packedSize);
    return true;
}

static bool ResizeThumb(int &width, int &height)
{
    int maxLen = max(width, height);
    int minLen = min(width, height);
    if (minLen == 0 || maxLen < 0) {
        MEDIA_ERR_LOG("Divisor minLen is 0");
        return false;
    }
    double ratio = static_cast<double>(maxLen) / minLen;
    if (minLen > SHORT_SIDE_THRESHOLD) {
        minLen = SHORT_SIDE_THRESHOLD;
        maxLen = static_cast<int>(SHORT_SIDE_THRESHOLD * ratio);
        if (maxLen > MAXIMUM_SHORT_SIDE_THRESHOLD) {
            maxLen = MAXIMUM_SHORT_SIDE_THRESHOLD;
        }
        if (height > width) {
            width = minLen;
            height = maxLen;
        } else {
            width = maxLen;
            height = minLen;
        }
    }
    if (minLen <= SHORT_SIDE_THRESHOLD && maxLen > SHORT_SIDE_THRESHOLD && ratio > ASPECT_RATIO_THRESHOLD) {
        int newMaxLen = static_cast<int>(minLen * ASPECT_RATIO_THRESHOLD);
        if (height > width) {
            width = minLen;
            height = newMaxLen;
        } else {
            width = newMaxLen;
            height = minLen;
        }
    }
    return true;
}

int32_t MtpMediaLibrary::GetVideoThumb(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt8List> &outThumb)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_CONTEXT_IS_NULL, "context is nullptr");
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT,
        "avMetadataHelper is nullptr");
    int32_t fd = 0;
    int error = GetFd(context, fd);
    CHECK_AND_RETURN_RET_LOG(error == MTP_SUCCESS, MTP_ERROR_NO_THUMBNAIL_PRESENT, "GetFd failed");
    struct stat64 st;
    int32_t ret = fstat64(fd, &st);
    CondCloseFd(ret != 0, fd);
    CHECK_AND_RETURN_RET_LOG(ret == 0, MTP_ERROR_NO_THUMBNAIL_PRESENT, "Get file state failed, err %{public}d", errno);
    int64_t length = static_cast<int64_t>(st.st_size);
    ret = avMetadataHelper->SetSource(fd, 0, length, AV_META_USAGE_PIXEL_MAP);
    CondCloseFd(ret != 0, fd);
    CHECK_AND_RETURN_RET_LOG(ret == 0, MTP_ERROR_NO_THUMBNAIL_PRESENT, "SetSource failed, ret %{public}d", ret);
    auto resultMap = avMetadataHelper->ResolveMetadata();
    int32_t width = NORMAL_WIDTH;
    int32_t height = NORMAL_HEIGHT;
    HaveRotateToChangeWidthHeight(width, height, resultMap);
    CHECK_AND_PRINT_LOG(ResizeThumb(width, height), "resize thumb fail");
    PixelMapParams param = {
        .dstWidth = width,
        .dstHeight = height,
        .colorFormat = PixelFormat::RGBA_8888
    };
    shared_ptr<PixelMap> sPixelMap = avMetadataHelper->FetchFrameYuv(0,
        AVMetadataQueryOption::AV_META_QUERY_NEXT_SYNC, param);
    CondCloseFd(sPixelMap == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(sPixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "sPixelMap is nullptr");
    sPixelMap->SetAlphaType(AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL);
    CloseFd(context, fd);
    bool isCompressImageSuccess = CompressImage(*sPixelMap.get(), *outThumb);
    CHECK_AND_RETURN_RET_LOG(isCompressImageSuccess == true, MTP_ERROR_NO_THUMBNAIL_PRESENT, "CompressImage is fail");
    return MTP_SUCCESS;
}

void MtpMediaLibrary::HaveRotateToChangeWidthHeight(int32_t &width, int32_t &height,
    std::unordered_map<int32_t, std::string> &resultMap)
{
    if (resultMap.empty()) {
        MEDIA_ERR_LOG("resultMap is empty.");
        return;
    }
    if (resultMap.find(AVMetadataCode::AV_KEY_VIDEO_WIDTH) == resultMap.end() ||
        resultMap.find(AVMetadataCode::AV_KEY_VIDEO_HEIGHT) == resultMap.end()) {
        return;
    }
    std::string widthStr = resultMap.at(AVMetadataCode::AV_KEY_VIDEO_WIDTH);
    std::string heightStr = resultMap.at(AVMetadataCode::AV_KEY_VIDEO_HEIGHT);
    if (MtpDataUtils::IsNumber(widthStr) && MtpDataUtils::IsNumber(heightStr)) {
        width = atoi(widthStr.c_str());
        height = atoi(heightStr.c_str());
    }
    if (resultMap.find(AVMetadataCode::AV_KEY_VIDEO_ORIENTATION) == resultMap.end()) {
        return;
    }
    std::string strTempStr = resultMap.at(AVMetadataCode::AV_KEY_VIDEO_ORIENTATION);
    int32_t strTemp = 0;
    if (MtpDataUtils::IsNumber(strTempStr)) {
        strTemp = atoi(strTempStr.c_str());
    }
    if (strTemp == ROTATE_ANGLE_90 || strTemp == ROTATE_ANGLE_270) {
        std::swap(width, height);
    }
}

int32_t MtpMediaLibrary::GetPictureThumb(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt8List> &outThumb)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_CONTEXT_IS_NULL, "context is nullptr");

    int32_t fd = 0;
    uint32_t errorCode = MTP_SUCCESS;
    errorCode = static_cast<uint32_t>(GetFd(context, fd));
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_NO_THUMBNAIL_PRESENT, "GetFd failed");

    SourceOptions opts;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(fd, opts, errorCode);
    CondCloseFd(imageSource == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "imageSource is nullptr");

    ImageInfo imageInfo;
    imageSource->GetImageInfo(imageInfo);
    int width = imageInfo.size.width;
    int height = imageInfo.size.height;
    CHECK_AND_RETURN_RET_LOG(ResizeThumb(width, height), MTP_ERROR_NO_THUMBNAIL_PRESENT, "resize thumb fail");
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {
        .width = width,
        .height = height,
    };

    std::unique_ptr<PixelMap> cropPixelMap = imageSource->CreatePixelMap(decodeOpts, errorCode);
    CondCloseFd(cropPixelMap == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(cropPixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "cropPixelMap is nullptr");
    int32_t imageOrientation = 0;
    errorCode = imageSource->GetImagePropertyInt(0, PHOTO_DATA_IMAGE_ORIENTATION, imageOrientation);
    if (errorCode == E_OK && imageOrientation != 0) {
        PostProc::RotateInRectangularSteps(*(cropPixelMap.get()), static_cast<float>(imageOrientation), true);
    }
    CloseFd(context, fd);
    bool isCompressImageSuccess = CompressImage(*cropPixelMap, *outThumb);
    CHECK_AND_RETURN_RET_LOG(isCompressImageSuccess == true, MTP_ERROR_NO_THUMBNAIL_PRESENT, "CompressImage is fail");
    return MTP_SUCCESS;
}

void MtpMediaLibrary::CondCloseFd(const bool condition, const int fd)
{
    if (!condition || fd <= 0) {
        return;
    }
    int32_t ret = close(fd);
    CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "DealFd CloseFd fail!");
}


int32_t MtpMediaLibrary::GetThumb(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt8List> &outThumb)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_CONTEXT_IS_NULL, "context is nullptr");
    auto it = handleToPathMap.find(context->handle);
    CHECK_AND_RETURN_RET_LOG(it != handleToPathMap.end(), MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::GetThumb handle not found");

    uint16_t format;
    MtpDataUtils::GetMtpFormatByPath(it->second, format);
    MediaType mediaType;
    MtpDataUtils::GetMediaTypeByformat(format, mediaType);
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        return GetPictureThumb(context, outThumb);
    } else if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        return GetVideoThumb(context, outThumb);
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::SendObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
    uint32_t &outStorageID, uint32_t &outParent, uint32_t &outHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    std::string doc("");
    CHECK_AND_RETURN_RET_LOG(GetPathByContextParent(context, doc) == MTP_SUCCESS,
        MtpErrorUtils::SolveSendObjectInfoError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::SendObjectInfo parent not found");

    std::string path = doc + "/" + context->name;
    if (context->format == MTP_FORMAT_ASSOCIATION_CODE) {
        std::error_code ec;
        bool cond = (!sf::create_directory(path, ec) || ec.value() != MTP_SUCCESS);
        CHECK_AND_RETURN_RET_LOG(!cond, MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::GetThumb handle not found");
    } else {
        std::error_code ec;
        path = sf::weakly_canonical(path, ec);
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveSendObjectInfoError(E_HAS_FS_ERROR),
            "MtpMediaLibrary::SendObjectInfo normalized path failed");
    }
    uint32_t outObjectHandle;
    {
        WriteLock lock(g_mutex);
        outObjectHandle = AddPathToMap(path);
        MEDIA_DEBUG_LOG("SendObjectInfo path[%{public}s], handle[%{public}d]", path.c_str(), outObjectHandle);
    }

    outHandle = outObjectHandle;
    outStorageID = context->storageID;
    outParent = context->parent;
    return MtpErrorUtils::SolveSendObjectInfoError(E_SUCCESS);
}

int32_t MtpMediaLibrary::GetPathById(const int32_t id, std::string &outPath)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::GetPathById id[%{public}d]", id);
    ReadLock lock(g_mutex);
    auto it = handleToPathMap.find(id);
    if (it != handleToPathMap.end()) {
        outPath = it->second;
        return MTP_SUCCESS;
    }
    return E_ERR;
}

int32_t MtpMediaLibrary::GetPathByContextParent(const std::shared_ptr<MtpOperationContext> &context, std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_CONTEXT_IS_NULL, "context is nullptr");
    if (context->parent == 0 || context->parent == MTP_ALL_HANDLE_ID) {
        auto it = storageIdToPathMap.find(context->storageID);
        if (it != storageIdToPathMap.end()) {
            path = it->second;
            return MTP_SUCCESS;
        }
        return E_ERR;
    }
    return GetPathById(context->parent, path);
}

int32_t MtpMediaLibrary::GetIdByPath(const std::string &path, uint32_t &outId)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::GetIdByPath path[%{public}s]", path.c_str());
    ReadLock lock(g_mutex);
    auto it = pathToHandleMap.find(path);
    if (it != pathToHandleMap.end()) {
        outId = it->second;
        return E_SUCCESS;
    }
    for (const auto &it : storageIdToPathMap) {
        if (path.compare(it.second) == 0) {
            outId = it.first;
            return E_SUCCESS;
        }
    }
    return E_NO_SUCH_FILE;
}

int32_t MtpMediaLibrary::GetRealPath(const std::string &path, std::string &outPath)
{
    if (PUBLIC_DOC.compare(path.substr(0, PUBLIC_DOC.size())) == 0) {
        uid_t uid = getuid() / BASE_USER_RANGE;
        std::string realPath = PUBLIC_REAL_PATH_PRE + std::to_string(uid) + PUBLIC_REAL_PATH_END;
        outPath = realPath + path.substr(PUBLIC_DOC.size(), path.size());
        return MTP_SUCCESS;
    }
    if (SD_DOC.compare(path.substr(0, SD_DOC.size())) == 0) {
        outPath = path;
        return MTP_SUCCESS;
    }
    MEDIA_ERR_LOG("MtpMediaLibrary::GetRealPath path[%{public}s] error", path.c_str());
    return E_ERR;
}

uint32_t MtpMediaLibrary::MoveObjectSub(const sf::path &fromPath, const sf::path &toPath, const bool &isDir,
    uint32_t &repeatHandle)
{
    auto it = pathToHandleMap.find(toPath.string());
    if (it == pathToHandleMap.end()) {
        if (isDir) {
            MoveHandlePathMap(fromPath.string(), toPath.string());
        }
        ModifyHandlePathMap(fromPath.string(), toPath.string());
    } else {
        if (isDir) {
            uint32_t toHandle = pathToHandleMap.find(toPath.string())->second;
            MoveRepeatDirHandlePathMap(fromPath, toPath);
            repeatHandle = toHandle;
        } else {
            repeatHandle = pathToHandleMap.find(toPath.string())->second;
            auto ite = pathToHandleMap.find(fromPath.string());
            if (ite != pathToHandleMap.end()) {
                ModifyPathHandleMap(toPath.string(), ite->second);
            }
        }
    }
    return MTP_SUCCESS;
}

void CrossCopyAfter(bool isDir, const std::string &toPath)
{
    CHECK_AND_RETURN_LOG(isDir, "MoveObjectAfter not dir");
    CHECK_AND_RETURN_LOG(!toPath.empty(), "MoveObjectAfter path is empty");
    std::error_code ec;
    CHECK_AND_RETURN_LOG(sf::exists(toPath, ec), "MoveObjectAfter path is not exists");

    MtpFileObserver::GetInstance().AddPathToWatchMap(toPath);
    for (const auto& entry : sf::recursive_directory_iterator(toPath, ec)) {
        if (ec.value() != MTP_SUCCESS) {
            continue;
        }
        if (sf::is_directory(entry.path(), ec)) {
            MtpFileObserver::GetInstance().AddPathToWatchMap(entry.path().string());
        }
    }
}

int32_t MtpMediaLibrary::MoveObject(const std::shared_ptr<MtpOperationContext> &context, uint32_t &repeatHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    std::string from("");
    std::string to("");
    if (GetPathById(context->handle, from) != MTP_SUCCESS || GetPathByContextParent(context, to) != MTP_SUCCESS) {
        MEDIA_ERR_LOG("MtpMediaLibrary::MoveObject from or to not found");
        return MtpErrorUtils::SolveMoveObjectError(E_HAS_DB_ERROR);
    }
    std::error_code ec;
    if (!sf::exists(from, ec) || !sf::exists(to, ec)) {
        MEDIA_ERR_LOG("MtpMediaLibrary::MoveObject from or to path not found");
        return MtpErrorUtils::SolveMoveObjectError(E_HAS_DB_ERROR);
    }

    CHECK_AND_RETURN_RET_LOG(sf::is_directory(to, ec), MtpErrorUtils::SolveMoveObjectError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::MoveObject parent path is not dir");
    auto fromPath = sf::path(from);
    auto toPath = sf::path(to) / sf::path(from).filename();
    bool isDir = sf::is_directory(fromPath, ec);
    // compare the prefix of the two paths
    const auto len = PUBLIC_REAL_PATH_PRE.size();
    bool isSameStorage = from.substr(0, len).compare(to.substr(0, len)) == 0;
    MEDIA_INFO_LOG("from[%{public}s],to[%{public}s] %{public}d", fromPath.c_str(), toPath.c_str(), isSameStorage);
    std::unordered_map<std::string, std::pair<long, long>> statTimeMap;
    GetStatTime(fromPath.string(), toPath.string(), !isSameStorage, statTimeMap);
    {
        WriteLock lock(g_mutex);
        if (isSameStorage) {
            // move in the same storage
            sf::rename(fromPath, toPath, ec);
        } else {
            // move between different storage
            sf::copy(fromPath, toPath, sf::copy_options::recursive | sf::copy_options::overwrite_existing, ec);
            CrossCopyAfter(isDir, toPath);
            isDir ? sf::remove_all(fromPath, ec) : sf::remove(fromPath, ec);
        }
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveMoveObjectError(E_FAIL),
            "MtpMediaLibrary::MoveObject failed");
        MoveObjectSub(fromPath, toPath, isDir, repeatHandle);
    }
    SetStatTime(statTimeMap);
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::CopyObject(const std::shared_ptr<MtpOperationContext> &context,
    uint32_t &outObjectHandle, uint32_t &oldHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    std::string from("");
    std::string to("");
    if (GetPathById(context->handle, from) != MTP_SUCCESS || GetPathByContextParent(context, to) != MTP_SUCCESS) {
        MEDIA_ERR_LOG("MtpMediaLibrary::CopyObject from or to not found");
        return MtpErrorUtils::SolveMoveObjectError(E_HAS_DB_ERROR);
    }

    bool cond = (!sf::exists(from) || !sf::exists(to));
    CHECK_AND_RETURN_RET_LOG(!cond, MtpErrorUtils::SolveCopyObjectError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::CopyObject handle or parent path not found");
    CHECK_AND_RETURN_RET_LOG(sf::is_directory(to), MtpErrorUtils::SolveCopyObjectError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::CopyObject parent path is not dir");
    std::error_code ec;
    auto fromPath = sf::path(from);
    auto toPath = sf::path(to) / sf::path(from).filename();
    CHECK_AND_RETURN_RET_LOG(!sf::exists(toPath, ec), MtpErrorUtils::SolveCopyObjectError(E_FILE_EXIST),
        "MtpMediaLibrary::CopyObject toPath exists");
    MEDIA_INFO_LOG("from[%{public}s],to[%{public}s]", fromPath.c_str(), toPath.c_str());
    std::unordered_map<std::string, std::pair<long, long>> statTimeMap;
    GetStatTime(fromPath.string(), toPath.string(), true, statTimeMap);
    sf::copy(fromPath, toPath, sf::copy_options::recursive | sf::copy_options::overwrite_existing, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveCopyObjectError(E_FAIL),
        "MtpMediaLibrary::CopyObject failed");
    SetStatTime(statTimeMap);
    {
        WriteLock lock(g_mutex);
        outObjectHandle = AddPathToMap(toPath.string());
        MEDIA_INFO_LOG("CopyObject successful to[%{public}s], handle[%{public}d]", toPath.c_str(), outObjectHandle);
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::DeleteObject(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, path) == MTP_SUCCESS,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::DeleteObject handle not found");
    std::error_code ec;
    if (sf::exists(path, ec) == false) {
        DeleteHandlePathMap(path, context->handle);
        return MTP_SUCCESS;
    }
    MEDIA_DEBUG_LOG("MtpMediaLibrary::DeleteObject path[%{public}s]", path.c_str());
    if (sf::is_directory(path, ec)) {
        sf::remove_all(path, ec);
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::DeleteObject remove_all failed");
        {
            WriteLock lock(g_mutex);
            ErasePathInfo(context->handle, path);
        }
    } else {
        sf::remove(path, ec);
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::DeleteObject remove failed");
        DeleteHandlePathMap(path, context->handle);
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::SetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context)
{
    MEDIA_INFO_LOG("MtpMediaLibrary::SetObjectPropValue");
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    std::string colName("");
    variant<int64_t, std::string> colValue;
    int32_t errCode = MtpDataUtils::SolveSetObjectPropValueData(context, colName, colValue);
    CHECK_AND_RETURN_RET_LOG(errCode == 0, errCode, "fail to SolveSetObjectPropValueData");
    CHECK_AND_RETURN_RET(colName.compare(MEDIA_DATA_DB_PARENT_ID) != 0, MTP_SUCCESS);
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, path) == MTP_SUCCESS,
        MtpErrorUtils::SolveObjectPropValueError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::SetObjectPropValue handle not found");

    std::error_code ec;
    string to = sf::path(path).parent_path().string() + "/" + get<std::string>(colValue);
    bool cond = (sf::exists(to, ec) || ec.value() != MTP_SUCCESS);
    CHECK_AND_RETURN_RET_LOG(!cond, MtpErrorUtils::SolveObjectPropValueError(E_HAS_DB_ERROR),
        "MtpMediaLibrary::SetObjectPropValue rename failed, file/doc exists");
    {
        WriteLock lock(g_mutex);
        sf::rename(path, to, ec);
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MtpErrorUtils::SolveObjectPropValueError(E_HAS_DB_ERROR),
            "MtpMediaLibrary::SetObjectPropValue rename failed");
        ModifyHandlePathMap(path, to);
        if (sf::is_directory(to, ec)) {
            MoveHandlePathMap(path, to);
        }
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::CloseFd(const std::shared_ptr<MtpOperationContext> &context, int32_t fd)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::CloseFd fd=[%{public}d]", fd);
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_ERR, "wrong fd");
    int errCode = close(fd);
    return MtpErrorUtils::SolveCloseFdError(errCode);
}

void MtpMediaLibrary::GetHandles(const uint32_t handle, const std::string &root,
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out)
{
    CHECK_AND_RETURN_LOG(out != nullptr, "out is nullptr");
    auto it = handleToPathMap.find(handle);
    if (it == handleToPathMap.end() || access(it->second.c_str(), R_OK) != 0) {
        return;
    }
    out->emplace(handle, it->second);
}

std::shared_ptr<std::unordered_map<uint32_t, std::string>> MtpMediaLibrary::GetHandlesMap(
    const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "context is nullptr");
    auto handlesMap = std::make_shared<std::unordered_map<uint32_t, std::string>>();
    CHECK_AND_RETURN_RET_LOG(handlesMap != nullptr, nullptr, "handlesMap is nullptr");
    auto it = storageIdToPathMap.find(context->storageID);
    const std::string root = (it == storageIdToPathMap.end()) ? PUBLIC_DOC : it->second;
    if (context->depth == MTP_ALL_DEPTH && (context->handle == 0 || context->handle == MTP_ALL_HANDLE_ID)) {
        context->handle = MTP_ALL_HANDLE_ID;
        context->depth = 0;
    }
    if (context->handle != 0) {
        if (context->depth == 0) {
            if (context->handle == MTP_ALL_HANDLE_ID) {
                ScanDirTraverseWithType(root, handlesMap);
            } else {
                GetHandles(context->handle, root, handlesMap);
            }
        }
        if (context->depth == 1) {
            if (context->handle == MTP_ALL_HANDLE_ID) {
                ScanDirWithType(root, handlesMap);
            } else {
                auto it = handleToPathMap.find(context->handle);
                std::string path = (it == handleToPathMap.end()) ? root : it->second;
                ScanDirWithType(path, handlesMap);
            }
        }
    } else {
        ScanDirWithType(root, handlesMap);
    }
    return handlesMap;
}

void MtpMediaLibrary::CorrectStorageId(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    CHECK_AND_RETURN_LOG(context->handle > 0, "no need correct");

    auto it = handleToPathMap.find(context->handle);
    CHECK_AND_RETURN_LOG(it != handleToPathMap.end(), "no find by context->handle");

    for (auto storage = storageIdToPathMap.begin(); storage != storageIdToPathMap.end(); ++storage) {
        if (it->second.compare(0, storage->second.size(), storage->second) == 0) {
            context->storageID = storage->first;
            return;
        }
    }
}

int32_t MtpMediaLibrary::GetObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<std::vector<Property>> &outProps)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    if (context->property == 0) {
        CHECK_AND_RETURN_RET_LOG(context->groupCode != 0, MTP_ERROR_PARAMETER_NOT_SUPPORTED, "groupCode error");
        MEDIA_ERR_LOG("context property = 0");
        return MTP_ERROR_SPECIFICATION_BY_GROUP_UNSUPPORTED;
    }
    if (context->depth == MTP_ALL_DEPTH && (context->handle == 0 || context->handle == MTP_ALL_HANDLE_ID)) {
        context->handle = MTP_ALL_HANDLE_ID;
        context->depth = 0;
    }
    MEDIA_DEBUG_LOG("GetObjectPropList handle[0x%{public}x], depth[0x%{public}x] parent[%{public}d]",
        context->handle, context->depth, context->parent);
    bool cond = (context->depth == 0 || context->depth == 1);
    CHECK_AND_RETURN_RET_LOG(cond, MTP_ERROR_SPECIFICATION_BY_DEPTH_UNSUPPORTED, "depth error");

    MEDIA_DEBUG_LOG("GetObjectPropList storageID[%{public}d],format[%{public}d],property[0x%{public}x]",
        context->storageID, context->format, context->property);
    int32_t errCode = MTP_ERROR_INVALID_OBJECTHANDLE;
    {
        WriteLock lock(g_mutex);
        CorrectStorageId(context);
        auto handlesMap = GetHandlesMap(context);
        bool condition = (handlesMap == nullptr || handlesMap->empty());
        CHECK_AND_RETURN_RET_LOG(!condition, errCode, "MtpMediaLibrary::GetObjectPropList out is empty");
        errCode = MtpDataUtils::GetMtpPropList(handlesMap, pathToHandleMap, context, outProps);
    }
    return errCode;
}

uint32_t MtpMediaLibrary::AddPathToMap(const std::string &path)
{
    uint32_t id;
    auto it = pathToHandleMap.find(path);
    if (it == pathToHandleMap.end()) {
        id = GetId();
        AddToHandlePathMap(path, id);
    } else {
        id = it->second;
    }
    MEDIA_DEBUG_LOG("MtpMediaLibrary::AddPathToMap path[%{public}s] id[%{public}d]", path.c_str(), id);
    return id;
}

uint32_t MtpMediaLibrary::ScanDirWithType(const std::string &root,
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out)
{
    MEDIA_INFO_LOG("MtpMediaLibrary::ScanDirWithType root[%{public}s]", root.c_str());
    CHECK_AND_RETURN_RET_LOG(out != nullptr, E_ERR, "out is nullptr");
    CHECK_AND_RETURN_RET_LOG(access(root.c_str(), R_OK) == 0, E_ERR, "access failed root[%{public}s]", root.c_str());
    std::error_code ec;
    if (sf::exists(root, ec) && sf::is_directory(root, ec)) {
        if (!IsRootPath(root)) {
            out->emplace(AddPathToMap(root), root);
        }
        for (const auto& entry : sf::directory_iterator(root, ec)) {
            if (ec.value() != MTP_SUCCESS) {
                continue;
            }
            if (sf::is_directory(entry.path(), ec) && IsHiddenDirectory(entry.path().string())) {
                continue;
            }
            out->emplace(AddPathToMap(entry.path().string()), entry.path().string());
        }
    } else if (sf::exists(root, ec) && sf::is_regular_file(root, ec)) {
        out->emplace(AddPathToMap(root), root);
    }
    return MTP_SUCCESS;
}

uint32_t MtpMediaLibrary::ScanDirTraverseWithType(const std::string &root,
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out)
{
    MEDIA_INFO_LOG("MtpMediaLibrary::ScanDirTraverseWithType root[%{public}s]", root.c_str());
    CHECK_AND_RETURN_RET_LOG(out != nullptr, E_ERR, "out is nullptr");
    CHECK_AND_RETURN_RET_LOG(access(root.c_str(), R_OK) == 0, E_ERR, "access failed root[%{public}s]", root.c_str());
    std::error_code ec;
    if (sf::exists(root, ec) && sf::is_directory(root, ec)) {
        if (!IsRootPath(root)) {
            out->emplace(AddPathToMap(root), root);
        }
        for (const auto& entry : sf::recursive_directory_iterator(root,
            sf::directory_options::skip_permission_denied)) {
            if (sf::is_directory(entry.path(), ec) && IsHiddenDirectory(entry.path().string())) {
                continue;
            }
            out->emplace(AddPathToMap(entry.path().string()), entry.path().string());
        }
    } else if (sf::exists(root, ec) && sf::is_regular_file(root, ec)) {
        out->emplace(AddPathToMap(root), root);
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::GetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context,
    uint64_t &outIntVal, uint128_t &outLongVal, std::string &outStrVal)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    MEDIA_INFO_LOG("MtpMediaLibrary::GetObjectPropValue handle[%{public}d] property[%{public}d]",
        context->handle, context->property);
    std::string path("");
    CHECK_AND_RETURN_RET_LOG(GetPathById(context->handle, path) == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE,
        "MtpMediaLibrary::GetObjectPropValue handle not found");

    if (MTP_PROPERTY_PARENT_OBJECT_CODE == context->property) {
        outIntVal = GetParentId(path);
        return MTP_SUCCESS;
    }

    PropertyValue propValue;
    int32_t errCode = MtpDataUtils::GetMtpPropValue(path, context->property, 0, propValue);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "fail to get GetMtpPropValue");
    outIntVal = propValue.outIntVal;
    outStrVal = propValue.outStrVal;
    return errCode;
}

bool MtpMediaLibrary::TryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    MEDIA_DEBUG_LOG("TryAddExternalStorage fsUuid[%{public}s]", fsUuid.c_str());
    CHECK_AND_RETURN_RET_LOG(!fsUuid.empty(), false, "fsUuid is empty");
    {
        WriteLock lock(g_mutex);
        return AddExternalStorage(fsUuid, storageId);
    }
}

bool MtpMediaLibrary::TryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    MEDIA_DEBUG_LOG("TryRemoveExternalStorage fsUuid[%{public}s]", fsUuid.c_str());
    CHECK_AND_RETURN_RET_LOG(!fsUuid.empty(), false, "fsUuid is empty");
    const std::string path = GetExternalPathByUuid(fsUuid);
    storageId = 0;
    {
        WriteLock lock(g_mutex);
        for (const auto &it : storageIdToPathMap) {
            if (path.compare(it.second) == 0) {
                storageId = it.first;
                storageIdToPathMap.erase(storageId);
                break;
            }
        }
        CHECK_AND_RETURN_RET_LOG(storageId != 0, false, "external storage is not exists");
        ErasePathInfoSub(path);
    }
    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, false, "MtpStorageManager instance is nullptr");
    auto storage = manager->GetStorage(storageId);
    if (storage != nullptr) {
        manager->RemoveStorage(storage);
    }
    return true;
}

const std::string MtpMediaLibrary::GetExternalPathByUuid(const std::string &fsUuid)
{
    return std::string(SD_DOC + "/" + fsUuid);
}

bool MtpMediaLibrary::AddExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    CHECK_AND_RETURN_RET_LOG(!fsUuid.empty(), false, "fsUuid is empty");
    const std::string path = GetExternalPathByUuid(fsUuid);
    for (const auto &it : storageIdToPathMap) {
        if (path.compare(it.second) == 0) {
            storageId = it.first;
            return true;
        }
    }
    uint32_t id = SD_START_ID;
    for (id = SD_START_ID; id <= SD_END_ID; id++) {
        if (storageIdToPathMap.find(id) == storageIdToPathMap.end()) {
            break;
        }
    }
    CHECK_AND_RETURN_RET_LOG(id <= SD_END_ID, false, "error: too many ext disk");
    MEDIA_INFO_LOG("Mtp AddExternalStorage id[%{public}d] path[%{public}s]", id, path.c_str());

    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, false, "MtpStorageManager instance is nullptr");

    auto storage = make_shared<Storage>();
    CHECK_AND_RETURN_RET_LOG(storage != nullptr, false, "storage is nullptr");
    storage->SetStorageID(id);
    storage->SetStorageType(MTP_STORAGE_REMOVABLERAM);
    storage->SetFilesystemType(MTP_FILESYSTEM_GENERICHIERARCHICAL);
    storage->SetAccessCapability(MTP_ACCESS_READ_WRITE);
    storage->SetMaxCapacity(manager->GetTotalSize(path));
    storage->SetFreeSpaceInBytes(manager->GetFreeSize(path));
    storage->SetFreeSpaceInObjects(0);
    std::string desc = manager->GetStorageDescription(MTP_STORAGE_REMOVABLERAM);
    id > SD_START_ID ? desc.append(" (").append(std::to_string(id - INNER_STORAGE_ID)).append(")") : desc;
    storage->SetStorageDescription(desc);
    storage->SetVolumeIdentifier(fsUuid);
    manager->AddStorage(storage);
    storageIdToPathMap[id] = path;
    storageId = id;
    MEDIA_ERR_LOG("Mtp AddExternalStorage storageId[%{public}d] path[%{public}s]", id, path.c_str());
    return true;
}

int MtpMediaLibrary::GetStorageIds()
{
    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, E_ERR, "MtpStorageManager instance is nullptr");

    auto storage = make_shared<Storage>();
    CHECK_AND_RETURN_RET_LOG(storage != nullptr, E_ERR, "storage is nullptr");
    storage->SetStorageID(INNER_STORAGE_ID);
    storage->SetStorageType(MTP_STORAGE_FIXEDRAM);
    storage->SetFilesystemType(MTP_FILESYSTEM_GENERICHIERARCHICAL);
    storage->SetAccessCapability(MTP_ACCESS_READ_WRITE);
    storage->SetMaxCapacity(manager->GetTotalSize(PUBLIC_DOC));
    storage->SetFreeSpaceInBytes(manager->GetFreeSize(PUBLIC_DOC));
    storage->SetFreeSpaceInObjects(0);
    storage->SetStorageDescription(manager->GetStorageDescription(MTP_STORAGE_FIXEDRAM));
    manager->AddStorage(storage);
    {
        WriteLock lock(g_mutex);
        storageIdToPathMap[INNER_STORAGE_ID] = PUBLIC_DOC;
        GetExternalStorages();
    }
    return MTP_SUCCESS;
}

void MtpMediaLibrary::GetExternalStorages()
{
    CHECK_AND_RETURN_LOG(access(SD_DOC.c_str(), R_OK) == 0, "access failed [%{public}s]", SD_DOC.c_str());
    std::error_code ec;
    CHECK_AND_RETURN_LOG(sf::exists(SD_DOC, ec) && sf::is_directory(SD_DOC, ec), "SD_DOC is not exists");
    for (const auto& entry : sf::directory_iterator(SD_DOC, ec)) {
        if (!sf::is_directory(entry.path(), ec)) {
            continue;
        }
        MEDIA_INFO_LOG("Mtp GetExternalStorages path[%{public}s]", entry.path().c_str());

        uint32_t storageId;
        AddExternalStorage(entry.path().filename().string(), storageId);
    }
}

void MtpMediaLibrary::ErasePathInfo(const uint32_t handle, const std::string &path)
{
    CHECK_AND_RETURN_LOG(!path.empty(), "path is empty");
    if (handleToPathMap.find(handle) != handleToPathMap.end()) {
        handleToPathMap.erase(handle);
    }
    if (pathToHandleMap.find(path) != pathToHandleMap.end()) {
        pathToHandleMap.erase(path);
    }

    ErasePathInfoSub(path);
}

void MtpMediaLibrary::ErasePathInfoSub(const std::string &path)
{
    std::string prefix = path + PATH_SEPARATOR;
    const auto size = prefix.size();
    std::vector<std::string> erasePaths;
    for (const auto &it : pathToHandleMap) {
        if (prefix.compare(it.first.substr(0, size)) != 0) {
            continue;
        }
        erasePaths.push_back(std::move(it.first));
        if (handleToPathMap.find(it.second) != handleToPathMap.end()) {
            handleToPathMap.erase(it.second);
        }
    }
    for (const auto &p : erasePaths) {
        if (pathToHandleMap.find(p) != pathToHandleMap.end()) {
            pathToHandleMap.erase(p);
        }
    }
    std::vector<std::string>().swap(erasePaths);
}

int32_t MtpMediaLibrary::GetGalleryObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<ObjectInfo> &outObjectInfo, const std::string &name)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::%{public}s is called", __func__);
    bool cond = (context == nullptr || context->handle != PTP_IN_MTP_ID || outObjectInfo == nullptr || name.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, MTP_ERROR_INVALID_OBJECTHANDLE, "parameter error");

    outObjectInfo->handle    = PTP_IN_MTP_ID;
    outObjectInfo->name      = name;
    outObjectInfo->parent    = DEFAULT_PARENT_ROOT;
    outObjectInfo->storageID = INNER_STORAGE_ID;
    outObjectInfo->format    = MTP_FORMAT_ASSOCIATION_CODE;

    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::GetGalleryPropValue(const std::shared_ptr<MtpOperationContext> &context,
    uint64_t &outIntVal, uint128_t &outLongVal, std::string &outStrVal, const std::string &name)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::%{public}s is called", __func__);
    bool cond = (context == nullptr || name.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, MTP_ERROR_INVALID_OBJECTHANDLE, "parameter error");
    MEDIA_DEBUG_LOG("handle[%{public}d] property[%{public}d]", context->handle, context->property);

    if (MTP_PROPERTY_PARENT_OBJECT_CODE == context->property) {
        outIntVal = DEFAULT_PARENT_ROOT;
    }
    if (MTP_PROPERTY_DISPLAY_NAME_CODE == context->property) {
        outStrVal = name;
    }
    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::GetGalleryObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<std::vector<Property>> &outProps, const std::string &name)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::%{public}s is called", __func__);
    return MtpDataUtils::GetGalleryPropList(context, outProps, name);
}

int32_t MtpMediaLibrary::CopyGalleryPhoto(const std::shared_ptr<MtpOperationContext> &context,
    const PathMap &paths, uint32_t &outObjectHandle)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    std::string to("");
    CHECK_AND_RETURN_RET_LOG(GetPathByContextParent(context, to) == MTP_SUCCESS,
        MTP_ERROR_INVALID_OBJECTHANDLE, "to not found");
    CHECK_AND_RETURN_RET_LOG(!paths.empty(), MTP_ERROR_INVALID_OBJECTHANDLE, "from is empty");
    CHECK_AND_RETURN_RET_LOG(sf::is_directory(to), MTP_ERROR_INVALID_OBJECTHANDLE, "parent path is not dir");

    std::error_code ec;
    for (const auto &it : paths) {
        CHECK_AND_CONTINUE_INFO_LOG(sf::exists(it.first, ec), "from path not exists err %{public}d", errno);
        auto toPath = sf::path(to) / it.second;
        sf::copy(sf::path(it.first), toPath, sf::copy_options::recursive | sf::copy_options::overwrite_existing, ec);
        CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MTP_ERROR_PARAMETER_NOT_SUPPORTED, "CopyObject failed");

        {
            WriteLock lock(g_mutex);
            outObjectHandle = AddPathToMap(toPath);
            MEDIA_INFO_LOG("CopyPhoto successful to[%{public}s], handle[%{public}d]", toPath.c_str(), outObjectHandle);
        }
    }

    return MTP_SUCCESS;
}

int32_t MtpMediaLibrary::CopyGalleryAlbum(const std::shared_ptr<MtpOperationContext> &context,
    const std::string &albumName, const PathMap &paths, uint32_t &outObjectHandle)
{
    MEDIA_DEBUG_LOG("MtpMediaLibrary::%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    std::string to("");
    CHECK_AND_RETURN_RET_LOG(GetPathByContextParent(context, to) == MTP_SUCCESS,
        MTP_ERROR_INVALID_OBJECTHANDLE, "to not found");
    CHECK_AND_RETURN_RET_LOG(!albumName.empty(), MTP_ERROR_INVALID_OBJECTHANDLE, "albumName is empty");

    std::error_code ec;
    CHECK_AND_RETURN_RET_LOG(sf::is_directory(to, ec), MTP_ERROR_INVALID_OBJECTHANDLE, "parent path is not dir");

    auto toPath = sf::path(to) / sf::path(albumName);
    sf::create_directory(toPath, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == MTP_SUCCESS, MTP_ERROR_PARAMETER_NOT_SUPPORTED, "create folder failed");

    {
        WriteLock lock(g_mutex);
        outObjectHandle = AddPathToMap(toPath);
        MEDIA_INFO_LOG("CopyAlbum successful to[%{public}s], handle[%{public}d]", toPath.c_str(), outObjectHandle);
    }

    CHECK_AND_RETURN_RET_LOG(!paths.empty(), MTP_SUCCESS, "from is empty, empty folder");
    for (auto it = paths.begin(); it != paths.end(); it++) {
        CHECK_AND_CONTINUE_INFO_LOG(sf::exists(it->first, ec), "from path not exists err %{public}d", errno);
        auto toPhoto = toPath / it->second;
        sf::copy(it->first, toPhoto, sf::copy_options::recursive | sf::copy_options::overwrite_existing, ec);
        CHECK_AND_CONTINUE_INFO_LOG(ec.value() == MTP_SUCCESS, "CopyGalleryAlbum err %{public}d", errno);
        MEDIA_DEBUG_LOG("from[%{public}s], to[%{public}s]", it->first.c_str(), toPhoto.c_str());
    }
    return MTP_SUCCESS;
}

} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP