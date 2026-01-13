/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaPrivacyManager"

#include "media_privacy_manager.h"

#include <algorithm>
#include <cerrno>
#include <mntent.h>
#include <securec.h>
#include <unistd.h>
#include <unordered_map>
#include <fcntl.h>

#include "epfs.h"
#include "image_source.h"
#include "media_container_types.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "permission_utils.h"
#include "media_exif.h"
#include "medialibrary_bundle_manager.h"
#include "media_app_uri_sensitive_column.h"
#include "medialibrary_urisensitive_operations.h"
#include "medialibrary_tracer.h"
#include "parameters.h"
#include "lake_file_utils.h"
#include "directory_ex.h"

using namespace std;
using PrivacyRanges = vector<pair<uint32_t, uint32_t>>;

namespace OHOS {
namespace Media {
#define HMDFS_IOC_SAVE_APPID _IOW(0xF2, 10, struct HmdfsSaveAppId)
constexpr uint32_t APPID_MAX_LEN = 256;
struct HmdfsSaveAppId {
    char appId[APPID_MAX_LEN] = {0};
};
constexpr uint32_t E_NO_EXIF = 1;
constexpr uint32_t E_NO_PRIVACY_EXIF_TAG = 2;
constexpr int32_t DEFAULT_TYPE = -1;
const std::vector<std::string> ALL_SENSITIVE_EXIF = {
    PHOTO_DATA_IMAGE_GPS_LATITUDE,
    PHOTO_DATA_IMAGE_GPS_LONGITUDE,
    PHOTO_DATA_IMAGE_GPS_TIME_STAMP,
    PHOTO_DATA_IMAGE_GPS_DATE_STAMP,
    PHOTO_DATA_IMAGE_GPS_ALTITUDE,
    PHOTO_DATA_IMAGE_GPS_VERSION_ID,
    PHOTO_DATA_IMAGE_MAKE,
    PHOTO_DATA_IMAGE_MODEL,
    PHOTO_DATA_IMAGE_SOFTWARE,
    PHOTO_DATA_IMAGE_DATE_TIME,
    PHOTO_DATA_IMAGE_EXPOSURE_TIME,
    PHOTO_DATA_IMAGE_F_NUMBER,
    PHOTO_DATA_IMAGE_EXPOSURE_PROGRAM,
    PHOTO_DATA_IMAGE_STANDARD_OUTPUT_SENSITIVITY,
    PHOTO_DATA_IMAGE_PHOTOGRAPHIC_SENSITIVITY,
    PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL,
    PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL_FOR_MEDIA,
    PHOTO_DATA_IMAGE_DATE_TIME_DIGITIZED,
    PHOTO_DATA_IMAGE_EXPOSURE_BIAS_VALUE,
    PHOTO_DATA_IMAGE_METERING_MODE,
    PHOTO_DATA_IMAGE_LIGHT_SOURCE,
    PHOTO_DATA_IMAGE_FLASH,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH,
    PHOTO_DATA_IMAGE_EXPOSURE_MODE,
    PHOTO_DATA_IMAGE_WHITE_BALANCE,
    PHOTO_DATA_IMAGE_DIGITAL_ZOOM_RATIO,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH_IN_35_MM_FILM
};
const std::vector<std::string> GEOGRAPHIC_LOCATION_EXIF = {
    PHOTO_DATA_IMAGE_GPS_LATITUDE,
    PHOTO_DATA_IMAGE_GPS_LONGITUDE,
    PHOTO_DATA_IMAGE_GPS_TIME_STAMP,
    PHOTO_DATA_IMAGE_GPS_DATE_STAMP,
    PHOTO_DATA_IMAGE_GPS_ALTITUDE,
    PHOTO_DATA_IMAGE_GPS_VERSION_ID
};
const std::vector<std::string> SHOOTING_PARAM_EXIF = {
    PHOTO_DATA_IMAGE_MAKE,
    PHOTO_DATA_IMAGE_MODEL,
    PHOTO_DATA_IMAGE_SOFTWARE,
    PHOTO_DATA_IMAGE_DATE_TIME,
    PHOTO_DATA_IMAGE_EXPOSURE_TIME,
    PHOTO_DATA_IMAGE_F_NUMBER,
    PHOTO_DATA_IMAGE_EXPOSURE_PROGRAM,
    PHOTO_DATA_IMAGE_PHOTOGRAPHIC_SENSITIVITY,
    PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL,
    PHOTO_DATA_IMAGE_DATE_TIME_DIGITIZED,
    PHOTO_DATA_IMAGE_EXPOSURE_BIAS_VALUE,
    PHOTO_DATA_IMAGE_METERING_MODE,
    PHOTO_DATA_IMAGE_LIGHT_SOURCE,
    PHOTO_DATA_IMAGE_FLASH,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH,
    PHOTO_DATA_IMAGE_EXPOSURE_MODE,
    PHOTO_DATA_IMAGE_WHITE_BALANCE,
    PHOTO_DATA_IMAGE_DIGITAL_ZOOM_RATIO,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH_IN_35_MM_FILM
};

MediaPrivacyManager::MediaPrivacyManager(const string &path, const string &mode, const string &fileId,
    const int32_t type)
    : path_(path), mode_(mode), fileId_(fileId), type_(type), uid_(0), tokenId_(0), fuseFlag_(false)
{}

MediaPrivacyManager::MediaPrivacyManager(const string &path, const string &mode, const string &fileId,
    const string &appId, const string &clientBundle, const int32_t &uid, const uint32_t &tokenId)
    : path_(path), mode_(mode), fileId_(fileId), type_(DEFAULT_TYPE),
    appId_(appId), clientBundle_(clientBundle), uid_(uid), tokenId_(tokenId), fuseFlag_(true)
{}

MediaPrivacyManager::~MediaPrivacyManager()
{}

const unordered_map<PrivacyType, string> PRIVACY_PERMISSION_MAP = {
    { PrivacyType::PRIVACY_LOCATION, PERMISSION_NAME_MEDIA_LOCATION },
};

const vector<string> EXIF_SUPPORTED_EXTENSION = {
    IMAGE_CONTAINER_TYPE_JPG,
    IMAGE_CONTAINER_TYPE_JPEG,
    IMAGE_CONTAINER_TYPE_JPE,
    IMAGE_CONTAINER_TYPE_PNG,
    IMAGE_CONTAINER_TYPE_WEBP,
    IMAGE_CONTAINER_TYPE_DNG,
    IMAGE_CONTAINER_TYPE_HEIC,
};

static bool IsTargetExtension(const string &path)
{
    const string ext = MediaFileUtils::GetExtensionFromPath(path);
    return find(EXIF_SUPPORTED_EXTENSION.begin(), EXIF_SUPPORTED_EXTENSION.end(), ext) !=
        EXIF_SUPPORTED_EXTENSION.end();
}

static bool IsWriteMode(const string &mode)
{
    return mode.find(MEDIA_FILEMODE_WRITEONLY) != string::npos;
}

static bool CheckFsMounted(const string &fsType, const string &mountPoint)
{
    struct mntent mountEntry;
    constexpr uint32_t mntEntrySize = 1024;
    char entryStr[mntEntrySize] = {0};
    FILE *mountTable = setmntent("/proc/mounts", "r");
    CHECK_AND_RETURN_RET_LOG(mountTable != nullptr, false, "Failed to get mount table, errno:%{public}d", errno);

    do {
        struct mntent *mnt = getmntent_r(mountTable, &mountEntry, entryStr, sizeof(entryStr));
        if (mnt == nullptr) {
            endmntent(mountTable);
            break;
        }
        if ((mountEntry.mnt_type != nullptr) &&
            (mountEntry.mnt_dir != nullptr) &&
            (strcmp(mountEntry.mnt_type, fsType.c_str()) == 0) &&
            (strcmp(mountEntry.mnt_dir, mountPoint.c_str()) == 0)) {
            endmntent(mountTable);
            return true;
        }
    } while (true);
    return false;
}

static int32_t BindFilterProxyFdToOrigin(const int32_t originFd, int32_t &proxyFd)
{
    int ret = ioctl(proxyFd, IOC_SET_ORIGIN_FD, &originFd);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, ret,
        "Failed to set origin fd: %{public}d to filter proxy fd: %{public}d, error: %{public}d",
        originFd, proxyFd, errno);
    return ret;
}

static int32_t SendRangesToIoctl(const int32_t originFd, const int32_t proxyFd, const PrivacyRanges &rans)
{
    FilterProxyRanges *ranges = (FilterProxyRanges *)malloc(sizeof(*ranges) + sizeof(ranges->range[0]) * rans.size());
    CHECK_AND_RETURN_RET_LOG(ranges != nullptr, -ENOMEM, "Failed to malloc ranges, errno: %{public}d", errno);
    ranges->size = static_cast<uint64_t>(rans.size());
    ranges->reserved = 0;
    for (size_t i = 0; i < rans.size(); i++) {
        // first: offset, second: end
        ranges->range[i].begin = static_cast<uint64_t>(rans[i].first);
        ranges->range[i].end = static_cast<uint64_t>(rans[i].second);
    }
    int err = ioctl(proxyFd, IOC_SET_FILTER_PROXY_RANGE, ranges);
    CHECK_AND_PRINT_LOG(err >= 0, "Failed to set ranges to fd: %{public}d, error: %{public}d", proxyFd, errno);
    free(ranges);
    return err;
}

static void SendAppidToIoctl(int32_t fd)
{
    if (fd < 0) {
        MEDIA_ERR_LOG("fd = %{public}d is invalid", fd);
        return;
    }

    std::string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    std::string appId = PermissionUtils::GetAppIdByBundleName(bundleName);
    MEDIA_DEBUG_LOG("bundleName: %{public}s, appId: %{public}s", bundleName.c_str(), appId.c_str());
    struct HmdfsSaveAppId iocAppId;
    uint32_t len = appId.size() < APPID_MAX_LEN ? appId.size() : APPID_MAX_LEN - 1;
    if (strncpy_s(iocAppId.appId, APPID_MAX_LEN, appId.c_str(), len) != E_OK) {
        MEDIA_ERR_LOG("strncpy_s appid failed, appid len: %{public}d", static_cast<int32_t>(appId.size()));
        return;
    }

    int32_t ret = ioctl(fd, HMDFS_IOC_SAVE_APPID, &iocAppId);
    if (ret < 0) {
        MEDIA_ERR_LOG("failed to send appid, fd: %{public}d, ret: %{public}d, appid: %{private}s, erron: %{public}d",
            fd, ret, appId.c_str(), errno);
    } else {
        MEDIA_DEBUG_LOG("appid send success, fd: %{public}d, appid: %{private}s", fd, appId.c_str());
    }
}

/* Caller is responsible to close the returned fd */
static int32_t OpenOriginFd(const string &path, const string &mode, string &clientBundle, const bool fuseFlag)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPrivacyManager::OpenOriginFd");
    if (fuseFlag == false) {
        clientBundle = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    }
    if (clientBundle.empty()) {
        MEDIA_DEBUG_LOG("clientBundleName is empty");
    }
    int32_t fd = MediaFileUtils::OpenFile(path, mode, clientBundle);
    SendAppidToIoctl(fd);
    return fd;
}

/*
 * Read to the returned @filterProxyFd will redirect to the file specified by @path, but the privacy ranges(@ranges) in
 * read buffer will be filtered out and filled with 0.
 *
 * Caller is responsible to close the returned @filterProxyFd.
 */
static int32_t OpenFilterProxyFd(const string &path, const string &mode, const PrivacyRanges &ranges,
    string &clientBundle, const bool &fuseFlag)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPrivacyManager::OpenFilterProxyFd");
    CHECK_AND_RETURN_RET_LOG(CheckFsMounted(FS_TYPE_EPFS, EPFS_MOUNT_POINT),
        OpenOriginFd(path, mode, clientBundle, fuseFlag),
        "Epfs is currently not supported yet");
    int32_t originFd = open(path.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(originFd >= 0, originFd,
        "Failed to open file, errno: %{public}d, path: %{private}s", errno, path.c_str());
    SendAppidToIoctl(originFd);

    constexpr mode_t epfsFileMode = 0400;
    // filterProxyFd_ will be returned to user, so there is no need to close it here.
    int32_t filterProxyFd = open(EPFS_MOUNT_POINT.c_str(), O_TMPFILE | O_RDWR, epfsFileMode);
    if (filterProxyFd < 0) {
        MEDIA_ERR_LOG("Failed to open epfs, error: %{public}d", errno);
        close(originFd);
        return filterProxyFd;
    }
    int32_t ret = BindFilterProxyFdToOrigin(originFd, filterProxyFd);
    if (ret < 0) {
        close(originFd);
        close(filterProxyFd);
        return ret;
    }
    ret = SendRangesToIoctl(originFd, filterProxyFd, ranges);
    if (ret < 0) {
        close(originFd);
        close(filterProxyFd);
        return ret;
    }
    close(originFd);
    MEDIA_INFO_LOG("FilterProxyFd will be returned: %{private}d", filterProxyFd);
    return filterProxyFd;
}

static void ShowRanges(const PrivacyRanges &ranges)
{
    for (auto range : ranges) {
        MEDIA_DEBUG_LOG("Range: [%{public}u, %{public}u)", range.first, range.second);
    }
}

static bool CmpMode(pair<uint32_t, uint32_t> pairA, pair<uint32_t, uint32_t> pairB)
{
    return pairA.first < pairB.first;
}

static int32_t SortRangesAndCheck(PrivacyRanges &ranges)
{
    if (ranges.empty()) {
        return E_SUCCESS;
    }
    uint32_t size = ranges.size();
    if (size > PRIVACY_MAX_RANGES) {
        MEDIA_ERR_LOG("Privacy ranges size invalid: %{public}d", size);
        return -EINVAL;
    }
    sort(ranges.begin(), ranges.end(), CmpMode);
    const auto u_idx = unique(ranges.begin(), ranges.end());
    ranges.erase(u_idx, ranges.end());
    size = ranges.size();

    if (ranges[0].first >= ranges[0].second) {
        MEDIA_ERR_LOG("Incorrect fileter ranges: begin(%{public}u) is not less than end(%{public}u)",
                      ranges[0].first, ranges[0].second);
        return -EINVAL;
    }

    for (uint32_t i = 1; i < size; i++) {
        if ((ranges[i].first >= ranges[i].second) || (ranges[i].first < ranges[i - 1].second)) {
            MEDIA_ERR_LOG("Invalid ranges: [%{public}u, %{public}u), last range is [%{public}u, %{public}u)",
                          ranges[i].first, ranges[i].second, ranges[i - 1].first, ranges[i - 1].second);
            return -EINVAL;
        }
    }
    ShowRanges(ranges);
    return E_SUCCESS;
}

static int32_t CollectRanges(const string &path, const HideSensitiveType &sensitiveType, PrivacyRanges &ranges,
    bool checkResult)
{
    if (sensitiveType == HideSensitiveType::NO_DESENSITIZE) {
        return E_SUCCESS;
    }

    SourceOptions opts;
    opts.formatHint = "image/jpeg";
    uint32_t err = -1;
    auto imageSource = ImageSource::CreateImageSource(path, opts, err);
    if (imageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to create image source, err: %{public}u", err);
        return -ENOMEM;
    }

    PrivacyRanges areas;
    std::vector<std::string> exifKeys;
    switch (sensitiveType) {
        case HideSensitiveType::ALL_DESENSITIZE:
            err = imageSource->GetFilterArea(ALL_SENSITIVE_EXIF, areas);
            break;
        case HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE:
            err = imageSource->GetFilterArea(GEOGRAPHIC_LOCATION_EXIF, areas);
            break;
        case HideSensitiveType::SHOOTING_PARAM_DESENSITIZE:
            err = imageSource->GetFilterArea(SHOOTING_PARAM_EXIF, areas);
            break;
        case HideSensitiveType::DEFAULT:
            CHECK_AND_RETURN_RET_LOG(!checkResult, E_SUCCESS, "Has MEDIA_LOCATION, no need hide sensitive.");
            err = imageSource->GetFilterArea(ALL_SENSITIVE_EXIF, areas);
            break;
        default:
            MEDIA_ERR_LOG("Invaild hide sensitive type %{public}d", sensitiveType);
            return E_SUCCESS;
    }
    CHECK_AND_WARN_LOG(err == E_SUCCESS,
        "Failed to get privacy area with type %{public}d, err: %{public}u", sensitiveType, err);
    for (auto &range : areas) {
        ranges.insert(ranges.end(), std::make_pair(range.first, range.first + range.second));
    }
    return E_SUCCESS;
}

/*
 * @path: [Input], the real path of the target file
 * @mode: [Input], the mode specified by user
 * @ranges: [Output], the privacy ranges of the target file
 *
 * The return value is listed below:
 * o Not a jpeg file: return success with empty ranges
 * o Write jpeg with no MEDIA_LOCATION: return permission denied
 * o Write jpeg with MEDIA_LOCATION: return success with empty ranges
 * o Read jpeg with no MEDIA_LOCATION: return success with privacy ranges if have any
 * o Read jpeg with MEDIA_LOCATION: return success with empty ranges
 * o Other cases: return negative error code.
 */
int32_t MediaPrivacyManager::GetPrivacyRanges()
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaPrivacyManager::GetPrivacyRanges");
    if (!IsTargetExtension(path_)) {
        return E_SUCCESS;
    }
    if (fileId_.empty()) {
        return E_SUCCESS;
    }
    if (mode_.find('w') != string::npos) {
        return E_SUCCESS;
    }
    bool result;
    if (fuseFlag_ == false) {
        string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
        appId_ = PermissionUtils::GetAppIdByBundleName(bundleName);
        tokenId_ = PermissionUtils::GetTokenId();
        result = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
    } else {
        result = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_MEDIA_LOCATION, uid_);
    }
    if ((result == false) && IsWriteMode(mode_)) {
        return E_PERMISSION_DENIED;
    }
    int32_t err = -1;
    if (type_ != DEFAULT_TYPE) {
        MEDIA_DEBUG_LOG("force type");
        err = CollectRanges(path_, (HideSensitiveType)type_, ranges_, result);
    } else {
        //collect ranges by hideSensitiveType
        HideSensitiveType sensitiveType =
            static_cast<HideSensitiveType>(UriSensitiveOperations::QuerySensitiveType(tokenId_, fileId_));
        MEDIA_DEBUG_LOG("GetPrivacyRanges::sensitiveType: %{public}d", static_cast<int32_t>(sensitiveType));
        err = CollectRanges(path_, sensitiveType, ranges_, result);
    }
    if (err < 0) {
        return err;
    }
    return SortRangesAndCheck(ranges_);
}

static bool IsDeveloperMediaTool()
{
    if (!PermissionUtils::IsRootShell() && !PermissionUtils::IsHdcShell()) {
        MEDIA_DEBUG_LOG("Mediatool permission check failed: target is not root");
        return false;
    }
    CHECK_AND_RETURN_RET_LOG(OHOS::system::GetBoolParameter("const.security.developermode.state", true),
        false, "Mediatool permission check failed: target is not in developer mode");
    return true;
}

int32_t MediaPrivacyManager::Open()
{
    path_ = LakeFileUtils::GetAssetRealPath(path_);
    int err = GetPrivacyRanges();
    if (err < 0) {
        MEDIA_ERR_LOG("GetPrivacyRanges failed");
        return err;
    }
    string absFilePath;
    if (!PathToRealPath(path_, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", path_.c_str());
        return -1;
    }
    if (ranges_.size() > 0 && !IsDeveloperMediaTool()) {
        return OpenFilterProxyFd(absFilePath, mode_, ranges_, clientBundle_, fuseFlag_);
    } else {
        return OpenOriginFd(absFilePath, mode_, clientBundle_, fuseFlag_);
    }
}
} // namespace Media
} // namespace OHOS
