/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Media_Client"

#include "cloud_file_data_convert.h"

#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "directory_ex.h"
#include "exif_rotate_utils.h"
#include "file_ex.h"
#include "media_log.h"
#include "cloud_media_client_utils.h"
#include "cloud_media_sync_const.h"
#include "mdk_record_photos_data.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "media_edit_utils.h"
#include "cloud_report_utils.h"
#include "nlohmann/json.hpp"
#include "mdk_record_photos_data.h"

namespace OHOS::Media::CloudSync {

/* path */
std::string CloudFileDataConvert::prefixLCD_ = "/mnt/hmdfs/";
std::string CloudFileDataConvert::sandboxPrefix_ = "/storage/cloud/files";
std::string CloudFileDataConvert::prefix_ = "/data/service/el2/";
std::string CloudFileDataConvert::suffixLCD_ = "/account/device_view/local/files";
std::string CloudFileDataConvert::suffix_ = "/hmdfs/account/files";
const std::string CloudFileDataConvert::recordType_ = "media";
constexpr off_t THUMB_LIMIT_SIZE = 2 * 1024 * 1024;
constexpr int32_t USER_COMMENT_LIMIT_SIZE = 1024;

CloudFileDataConvert::CloudFileDataConvert(CloudOperationType type, int32_t userId) : userId_(userId), type_(type)
{}

static inline std::string GetThumbnailPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "THM_ASTC") ? ".astc" : ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) + "/" + key + suffix;
}

std::string CloudFileDataConvert::GetThumbPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    /* transform sandbox path */
    return prefixLCD_ + std::to_string(userId_) + suffixLCD_ + "/" +
           GetThumbnailPath(path, key).substr(ROOT_MEDIA_DIR.length());
}

int32_t CloudFileDataConvert::CheckFileSize(const std::string &thumbSuffix, const int64_t &fileSize)
{
    if (fileSize <= 0) {
        MEDIA_ERR_LOG("ReportFailure: get size err, size is %{public}" PRId64, fileSize);
        return (thumbSuffix == THUMB_SUFFIX) ? E_THM_SIZE_IS_ZERO : E_LCD_SIZE_IS_ZERO;
    }
    if (fileSize >= THUMB_LIMIT_SIZE) {
        MEDIA_ERR_LOG("ReportFailure: size is too large, size is %{public}" PRId64, fileSize);
        return (thumbSuffix == THUMB_SUFFIX) ? E_THM_IS_TOO_LARGE : E_LCD_IS_TOO_LARGE;
    }
    return E_OK;
}

int32_t CloudFileDataConvert::GetFileSize(const std::string &path, const std::string &thumbSuffix, int64_t &fileSize)
{
    std::string thumbExSuffix = (thumbSuffix == THUMB_SUFFIX) ? THUMB_EX_SUFFIX : LCD_EX_SUFFIX;
    /* try get file size on xxxjpg/THM_EX/THM.jpg */
    std::string thumbnailPath = GetThumbPath(path, thumbExSuffix);
    struct stat fileStat;
    MEDIA_INFO_LOG("GetFileSize stat %{public}s", thumbnailPath.c_str());
    int32_t err = stat(thumbnailPath.c_str(), &fileStat);
    int32_t ret = E_OK;
    if (err < 0) {
        UTIL_SYNC_FAULT_REPORT({bundleName_,
            UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
            UtilCloud::FaultType::FILE,
            err,
            "get thumb size failed errno : " + std::to_string(errno) + ", path " +
                ReportUtils::GetAnonyString(thumbnailPath)});
        MEDIA_ERR_LOG("get thumb size failed errno :%{public}d, %{public}s", errno, thumbnailPath.c_str());
    } else {
        fileSize = fileStat.st_size;
        ret = this->CheckFileSize(thumbSuffix, fileSize);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CheckFileSize ret, %{public}d", ret);
        return E_OK;
    }
    /* try get file size on xxxjpg/THM.jpg */
    thumbnailPath = GetThumbPath(path, thumbSuffix);
    err = stat(thumbnailPath.c_str(), &fileStat);
    if (err < 0) {
        int32_t errNum = errno;
        UTIL_SYNC_FAULT_REPORT({bundleName_,
            UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
            UtilCloud::FaultType::FILE,
            err,
            "get thumb size failed errno : " + std::to_string(errno) + ", path " +
                ReportUtils::GetAnonyString(thumbnailPath)});
        MEDIA_ERR_LOG("get thumb size failed errno :%{public}d, %{public}s", errno, thumbnailPath.c_str());
        return ((thumbSuffix == THUMB_SUFFIX) ? E_THM_SOURCE_BASIC : E_LCD_SOURCE_BASIC) + errNum;
    }
    fileSize = fileStat.st_size;
    ret = this->CheckFileSize(thumbSuffix, fileSize);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CheckFileSize err, %{public}d", ret);
    MEDIA_INFO_LOG("GetFileSize stat end thumbnailPath: %{public}s, err: %{public}d, size: %{public}" PRId64,
        thumbnailPath.c_str(),
        err,
        fileSize);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleThumbSize(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    if (type_ != FILE_CREATE && type_ != FILE_DATA_MODIFY) {
        return E_OK;
    }
    std::string path = upLoadRecord.data;
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_QUERY_CONTENT_IS_EMPTY, "HandleThumbSize failed to get filepath");
    int64_t fileSize;
    int32_t ret = GetFileSize(path, THUMB_SUFFIX, fileSize);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetFileSize err: %{public}d", ret);
    map["thumb_size"] = MDKRecordField(fileSize);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleLcdSize(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    if (type_ != FILE_CREATE && type_ != FILE_DATA_MODIFY) {
        return E_OK;
    }
    std::string path = upLoadRecord.data;
    int64_t fileSize;
    int32_t ret = GetFileSize(path, LCD_SUFFIX, fileSize);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetFileSize err: %{public}d", ret);
    map["lcd_size"] = MDKRecordField(fileSize);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleFormattedDate(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    std::string year = upLoadRecord.dateYear;
    std::string month = upLoadRecord.dateMonth;
    std::string day = upLoadRecord.dateDay;
    if (year.empty() || month.empty() || day.empty()) {
        MEDIA_INFO_LOG("HandleFormattedDate year month day is empty");
        int64_t createTime = upLoadRecord.dateTaken;
        createTime = createTime / MILLISECOND_TO_SECOND;
        year = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DATE_YEAR_FORMAT, createTime);
        month = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DATE_MONTH_FORMAT, createTime);
        day = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DATE_DAY_FORMAT, createTime);
    }
    map[PhotoColumn::PHOTO_DATE_YEAR] = MDKRecordField(year);
    map[PhotoColumn::PHOTO_DATE_MONTH] = MDKRecordField(month);
    map[PhotoColumn::PHOTO_DATE_DAY] = MDKRecordField(day);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleExifRotate(std::map<std::string, MDKRecordField> &map,
    const CloudMdkRecordPhotosVo &upLoadRecord)
{
    if (upLoadRecord.exifRotate == 0) {
        MEDIA_INFO_LOG("Upload meta data, do not upload exif rotate uncertain");
        return E_OK;
    }
    map[PhotoColumn::PHOTO_EXIF_ROTATE] = MDKRecordField(upLoadRecord.exifRotate);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleUniqueFileds(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    std::map<std::string, MDKRecordField> map;
    /* store media unique fileds in attributes */
    map[PhotoColumn::MEDIA_TITLE] = MDKRecordField(upLoadRecord.title);
    map[PhotoColumn::MEDIA_TYPE] = MDKRecordField(upLoadRecord.mediaType);
    map[PhotoColumn::MEDIA_DURATION] = MDKRecordField(upLoadRecord.duration);
    map[PhotoColumn::MEDIA_HIDDEN] = MDKRecordField(upLoadRecord.hidden);
    map[PhotoColumn::PHOTO_HIDDEN_TIME] = MDKRecordField(upLoadRecord.hiddenTime);
    map[PhotoColumn::MEDIA_RELATIVE_PATH] = MDKRecordField(upLoadRecord.relativePath);
    map[PhotoColumn::MEDIA_VIRTUAL_PATH] = MDKRecordField(upLoadRecord.virtualPath);
    map[PhotoColumn::PHOTO_META_DATE_MODIFIED] = MDKRecordField(upLoadRecord.metaDateModified);
    map[PhotoColumn::PHOTO_SUBTYPE] = MDKRecordField(upLoadRecord.subtype);
    map[PhotoColumn::PHOTO_BURST_COVER_LEVEL] = MDKRecordField(upLoadRecord.burstCoverLevel);
    map[PhotoColumn::PHOTO_BURST_KEY] = MDKRecordField(upLoadRecord.burstKey);
    map[PhotoColumn::PHOTO_DATE_YEAR] = MDKRecordField(upLoadRecord.dateYear);
    map[PhotoColumn::PHOTO_DATE_MONTH] = MDKRecordField(upLoadRecord.dateMonth);
    map[PhotoColumn::PHOTO_DATE_DAY] = MDKRecordField(upLoadRecord.dateDay);
    map[PhotoColumn::PHOTO_SHOOTING_MODE] = MDKRecordField(upLoadRecord.shootingMode);
    map[PhotoColumn::PHOTO_SHOOTING_MODE_TAG] = MDKRecordField(upLoadRecord.shootingModeTag);
    map[PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE] = MDKRecordField(upLoadRecord.dynamicRangeType);
    map[PhotoColumn::PHOTO_HDR_MODE] = MDKRecordField(upLoadRecord.hdrMode);
    map[PhotoColumn::PHOTO_VIDEO_MODE] = MDKRecordField(upLoadRecord.videoMode);
    map[PhotoColumn::PHOTO_FRONT_CAMERA] = MDKRecordField(upLoadRecord.frontCamera);
    map[PhotoColumn::PHOTO_EDIT_TIME] = MDKRecordField(upLoadRecord.editTime);
    map[PhotoColumn::PHOTO_ORIGINAL_SUBTYPE] = MDKRecordField(upLoadRecord.originalSubtype);
    map[PhotoColumn::PHOTO_COVER_POSITION] = MDKRecordField(upLoadRecord.coverPosition);
    map[PhotoColumn::PHOTO_IS_RECTIFICATION_COVER] = MDKRecordField(upLoadRecord.isRectificationCover);
    map[PhotoColumn::MOVING_PHOTO_EFFECT_MODE] = MDKRecordField(upLoadRecord.movingPhotoEffectMode);
    map[PhotoColumn::SUPPORTED_WATERMARK_TYPE] = MDKRecordField(upLoadRecord.supportedWatermarkType);
    map[PhotoColumn::PHOTO_STRONG_ASSOCIATION] = MDKRecordField(upLoadRecord.strongAssociation);
    map[MediaColumn::MEDIA_ID] = MDKRecordField(upLoadRecord.fileId);
    map[PhotoColumn::MEDIA_FILE_PATH] = MDKRecordField(upLoadRecord.data);
    map[PhotoColumn::MEDIA_DATE_ADDED] = MDKRecordField((upLoadRecord.dateAdded) / MILLISECOND_TO_SECOND);
    map[PhotoColumn::MEDIA_DATE_MODIFIED] = MDKRecordField((upLoadRecord.dateModified) / MILLISECOND_TO_SECOND);
    map[PhotoColumn::PHOTO_SUBTYPE] = MDKRecordField(upLoadRecord.subtype);
    map[PhotoColumn::PHOTO_BURST_COVER_LEVEL] = MDKRecordField(upLoadRecord.burstCoverLevel);
    map[PhotoColumn::PHOTO_BURST_KEY] = MDKRecordField(upLoadRecord.burstKey);
    map[PhotoColumn::PHOTO_OWNER_ALBUM_ID] = MDKRecordField(upLoadRecord.ownerAlbumId);
    map[FILE_FIX_VERSION] = MDKRecordField(0);
    map[KEY_EDITED_TIME_MS] = MDKRecordField(upLoadRecord.dateModified);
    map[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = MDKRecordField(upLoadRecord.fileSourceType);
    map[PhotoColumn::PHOTO_STORAGE_PATH] = MDKRecordField(upLoadRecord.storagePath);
    HandleAttributesHashMap(map, upLoadRecord);
    int32_t ret = HandleThumbSize(map, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleThumbSize err: %{public}d", ret);
    ret = HandleLcdSize(map, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleLcdSize err: %{public}d", ret);
    ret = HandleFormattedDate(map, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleFormattedDate err: %{public}d", ret);
    ret = HandleExifRotate(map, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleExifRotate err: %{public}d", ret);
    data[FILE_ATTRIBUTES] = MDKRecordField(map);
    data[FILE_LOCAL_ID] = MDKRecordField(upLoadRecord.fileId);
    return ret;
}

int32_t CloudFileDataConvert::HandleFileType(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    MEDIA_DEBUG_LOG("enter HandleFileType %{public}d, %{public}d", upLoadRecord.subtype, upLoadRecord.dirty);
    if (upLoadRecord.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        data["fileType"] = MDKRecordField(FILE_TYPE_LIVEPHOTO);
        return E_OK;
    }
    // 2. fill fileType with Image/Video when create file
    if (upLoadRecord.dirty == static_cast<int32_t>(DirtyType::TYPE_NEW)) {
        data["fileType"] =
            MDKRecordField(upLoadRecord.mediaType == Media::MEDIA_TYPE_VIDEO ? FILE_TYPE_VIDEO : FILE_TYPE_IMAGE);
        return E_OK;
    }
    // 3. fill fileType with Image when it's a Graffiti
    if (upLoadRecord.subtype == static_cast<int32_t>(PhotoSubType::DEFAULT) &&
        upLoadRecord.originalSubtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        data["fileType"] = MDKRecordField(FILE_TYPE_IMAGE);
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandlePosition(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    CHECK_AND_RETURN_RET(!(upLoadRecord.latitude == 0 && upLoadRecord.longitude == 0), E_OK);
    std::stringstream latitudestream;
    std::stringstream longitudestream;
    latitudestream.precision(15);   // 15:precision
    longitudestream.precision(15);  // 15:precision
    latitudestream << upLoadRecord.latitude;
    longitudestream << upLoadRecord.longitude;
    std::string position = "{\"x\":\"" + latitudestream.str() + "\",\"y\":\"" + longitudestream.str() + "\"}";
    map["position"] = MDKRecordField(position);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleRotate(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    MEDIA_DEBUG_LOG("enter HandleRotate orientation: %{public}d", upLoadRecord.orientation);
    int32_t val;
    switch (upLoadRecord.orientation) {
        case ROTATE_ANGLE_0:
            val = ORIENTATION_NORMAL;
            break;
        case ROTATE_ANGLE_90:
            val = ORIENTATION_ROTATE_90;
            break;
        case ROTATE_ANGLE_180:
            val = ORIENTATION_ROTATE_180;
            break;
        case ROTATE_ANGLE_270:
            val = ORIENTATION_ROTATE_270;
            break;
        default:
            val = ORIENTATION_NORMAL;
            break;
    }
    map["rotate"] = MDKRecordField(val);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleSourcePath(
    std::map<std::string, MDKRecordField> &properties, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    if (!upLoadRecord.sourcePath.empty()) {
        properties["sourcePath"] = MDKRecordField(upLoadRecord.sourcePath);
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleProperties(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    std::map<std::string, MDKRecordField> properties;
    this->HandleSourcePath(properties, upLoadRecord);
    properties["sourceFileName"] = MDKRecordField(upLoadRecord.displayName);
    properties["first_update_time"] = MDKRecordField(std::to_string(upLoadRecord.dateAdded));
    properties["fileCreateTime"] = MDKRecordField(std::to_string(upLoadRecord.dateTaken));
    properties["detail_time"] = MDKRecordField(upLoadRecord.detailTime);
    properties["duration"] = MDKRecordField(upLoadRecord.duration);
    this->HandleWidthAndHeight(properties, upLoadRecord);
    HandlePosition(properties, upLoadRecord);
    HandleRotate(properties, upLoadRecord);
    data[FILE_PROPERTIES] = MDKRecordField(properties);
    return E_OK;
}

std::string CloudFileDataConvert::GetLowerPath(const std::string &path)
{
    size_t pos = path.find(sandboxPrefix_);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path");
        return "";
    }
    return prefix_ + std::to_string(userId_) + suffix_ + path.substr(pos + sandboxPrefix_.size());
}

static void DeleteTmpFile(bool needDelete, const std::string &path)
{
    CHECK_AND_RETURN(needDelete);
    bool isValid = unlink(path.c_str()) >= 0;
    CHECK_AND_PRINT_LOG(isValid, "unlink temp file fail, err: %{public}d, path: %{public}s", errno, path.c_str());
}

int32_t CloudFileDataConvert::HandleRawFile(
    std::map<std::string, MDKRecordField> &data, std::string &path, bool isMovingPhoto)
{
    std::string rawFilePath = MediaEditUtils::GetEditDataSourcePath(path, userId_);
    MEDIA_INFO_LOG("HandleEditData rawFilePath %{public}s", rawFilePath.c_str());
    if (rawFilePath.empty()) {
        return E_OK;
    }
    struct stat fileStat;
    int err = stat(rawFilePath.c_str(), &fileStat);
    if (err < 0 && errno == ENOENT) {
        return E_OK;
    }
    if (err < 0 && errno != ENOENT) {
        int errNum = errno;
        MEDIA_ERR_LOG("get raw size failed errno: %{public}d", errNum);
        return E_CONTENT_SOURCE_BASIC + errNum;
    }

    if (fileStat.st_size <= 0) {
        MEDIA_ERR_LOG("raw size err");
        return E_CONTENT_RAW_SIZE_IS_ZERO;
    }
    if (isMovingPhoto) {
        if (MovingPhotoFileUtils::ConvertToSourceLivePhoto(path, rawFilePath, userId_) != E_OK) {
            MEDIA_ERR_LOG("ConvertToSourceLivePhoto failed %{public}s", path.c_str());
            return E_NO_SUCH_FILE;
        }
    }
    MDKAsset content;
    content.uri = move(rawFilePath);
    content.assetName = FILE_RAW;
    content.operationType = MDKAssetOperType::DK_ASSET_ADD;
    data[FILE_RAW] = MDKRecordField(content);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleEditData(std::map<std::string, MDKRecordField> &data, std::string &path)
{
    MEDIA_INFO_LOG("enter HandleEditData editDataPath %{public}s", path.c_str());
    std::string editDataPath = MediaEditUtils::GetEditDataPath(path, userId_);
    MEDIA_INFO_LOG("HandleEditData editDataPath %{public}s", editDataPath.c_str());
    if (!editDataPath.empty()) {
        MDKAsset content;
        struct stat fileStat;
        if (stat(editDataPath.c_str(), &fileStat) == 0 && fileStat.st_size > 0) {
            MEDIA_INFO_LOG("HandleEditData editDataPath is not empty and stat success");
            content.uri = move(editDataPath);
            content.assetName = FILE_EDIT_DATA;
            content.operationType = MDKAssetOperType::DK_ASSET_ADD;
            data[FILE_EDIT_DATA] = MDKRecordField(content);
        }
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleEditDataCamera(std::map<std::string, MDKRecordField> &data, std::string &path)
{
    MEDIA_INFO_LOG("enter HandleEditData editDataPath %{public}s", path.c_str());
    std::string editDataCameraPath = MediaEditUtils::GetEditDataCameraPath(path, userId_);

    // -- editDataCamera as properties, append to FILE_ATTRIBUTES --
    MEDIA_INFO_LOG("HandleEditData editDataCameraPath %{public}s", editDataCameraPath.c_str());
    if (!editDataCameraPath.empty() && access(editDataCameraPath.c_str(), F_OK) == 0) {
        MEDIA_INFO_LOG("HandleEditData editDataCameraPath is not empty and access success");
        if (data.find(FILE_ATTRIBUTES) == data.end()) {
            MEDIA_ERR_LOG("Cannot add edit_data_camera to properties.");
            return E_INVALID_ARGUMENTS;
        }
        std::string buf;
        if (!LoadStringFromFile(editDataCameraPath, buf)) {
            MEDIA_ERR_LOG("editDataCameraPath read from file failed.");
            return E_INVALID_ARGUMENTS;
        }
        std::map<std::string, MDKRecordField> map;
        data[FILE_ATTRIBUTES].GetRecordMap(map);
        map[FILE_EDIT_DATA_CAMERA] = MDKRecordField(buf);
        data[FILE_ATTRIBUTES] = MDKRecordField(map);
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleEditData(
    std::map<std::string, MDKRecordField> &data, std::string &path, bool isMovingPhoto)
{
    MEDIA_INFO_LOG("enter HandleEditData editDataPath %{public}s, %{public}d", path.c_str(), isMovingPhoto);
    int32_t ret = this->HandleRawFile(data, path, isMovingPhoto);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleEditData HandleRawFile err: %{public}d", ret);
    this->HandleEditData(data, path);
    this->HandleEditDataCamera(data, path);
    return E_OK;
}

int32_t CloudFileDataConvert::CheckContentLivePhoto(const CloudMdkRecordPhotosVo &upLoadRecord, std::string &lowerPath)
{
    std::string path = upLoadRecord.data;
    int64_t coverPosition = upLoadRecord.coverPosition;
    bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(
        upLoadRecord.subtype, upLoadRecord.movingPhotoEffectMode, upLoadRecord.originalSubtype);
    bool isGraffiti = MovingPhotoFileUtils::IsGraffiti(upLoadRecord.subtype, upLoadRecord.originalSubtype);
    CHECK_AND_PRINT_LOG(!isMovingPhoto && !isGraffiti,
        "HandleContent isMovingPhoto: %{public}d, isGraffiti: %{public}d", isMovingPhoto, isGraffiti);
    if (isMovingPhoto && !isGraffiti) {
        std::string localPath = CloudMediaClientUtils::FindLocalPathFromCloudPath(path, userId_);
        bool isValid = MovingPhotoFileUtils::IsExistsLivePhotoFiles(localPath);
        isValid = isValid && MovingPhotoFileUtils::ConvertToLivePhoto(path, coverPosition, lowerPath, userId_) == E_OK;
        CHECK_AND_RETURN_RET_LOG(isValid, E_CONTENT_COVERT_LIVE_PHOTO, "convert to live photo fail");
    } else {
        int32_t ret = CloudMediaClientUtils::GetLocalPathByPhotosVo(upLoadRecord, lowerPath, userId_);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("GetLocalPathByPhotosVo fail");
            return ret;
        }
    }
    return E_OK;
}

int32_t CloudFileDataConvert::CheckContentFile(const CloudMdkRecordPhotosVo &upLoadRecord, const std::string &lowerPath)
{
    bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(
        upLoadRecord.subtype, upLoadRecord.movingPhotoEffectMode, upLoadRecord.originalSubtype);
    bool isGraffiti = MovingPhotoFileUtils::IsGraffiti(upLoadRecord.subtype, upLoadRecord.originalSubtype);
    struct stat fileStat;

    int err = stat(lowerPath.c_str(), &fileStat);
    if (err < 0) {
        int32_t errNum = errno;
        UTIL_SYNC_FAULT_REPORT({bundleName_,
            UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
            UtilCloud::FaultType::FILE,
            errNum,
            "get context size failed errno : " + std::to_string(errNum)});
        MEDIA_ERR_LOG("HandleContent errno : %{public}d, path : %{public}s, %{public}d, %{public}d",
            errno,
            lowerPath.c_str(),
            isMovingPhoto,
            isGraffiti);
        DeleteTmpFile(isMovingPhoto && !isGraffiti, lowerPath);
        return E_CONTENT_SOURCE_BASIC + errNum;
    }
    if (fileStat.st_size <= 0) {
        MEDIA_ERR_LOG("HandleContent content size err");
        DeleteTmpFile(isMovingPhoto && !isGraffiti, lowerPath);
        return E_CONTENT_SIZE_IS_ZERO;
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleContent(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    std::string path = upLoadRecord.data;
    std::string lowerPath = "";
    bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(
        upLoadRecord.subtype, upLoadRecord.movingPhotoEffectMode, upLoadRecord.originalSubtype);
    bool isGraffiti = MovingPhotoFileUtils::IsGraffiti(upLoadRecord.subtype, upLoadRecord.originalSubtype);
    CHECK_AND_PRINT_LOG(!isMovingPhoto && !isGraffiti,
        "HandleContent isMovingPhoto: %{public}d, isGraffiti: %{public}d", isMovingPhoto, isGraffiti);
    int32_t ret = this->CheckContentLivePhoto(upLoadRecord, lowerPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleContent CheckContentLivePhoto err: %{public}d", ret);
    ret = this->CheckContentFile(upLoadRecord, lowerPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleContent err: %{public}d", ret);
    /* asset */
    MDKAsset content;
    content.uri = move(lowerPath);
    content.assetName = FILE_CONTENT;
    content.operationType = MDKAssetOperType::DK_ASSET_ADD;
    data[FILE_CONTENT] = MDKRecordField(content);
    ret = HandleEditData(data, path, isMovingPhoto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HandleContent handle EditData err %{public}d", ret);
        DeleteTmpFile(isMovingPhoto && !isGraffiti, lowerPath);
        return ret;
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleThumbnail(
    std::map<std::string, MDKRecordField> &recordData, std::string &path, bool needUseExDir)
{
    std::string thumbnailUploadPath;
    std::string thumbnailExPath = GetThumbPath(path, THUMB_EX_SUFFIX);
    if (!needUseExDir) {
        std::string thumbnailPath = GetThumbPath(path, THUMB_SUFFIX);
        if (access(thumbnailPath.c_str(), F_OK)) {
            UTIL_SYNC_FAULT_REPORT({bundleName_,
                UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
                UtilCloud::FaultType::FILE,
                F_OK,
                "thumbnailPath " + ReportUtils::GetAnonyString(thumbnailPath) + " doesn't exist"});
            MEDIA_ERR_LOG("ReportFailure: thumnail doesn't exist %{public}s", thumbnailPath.c_str());
            return E_THM_SOURCE_BASIC + ENOENT;
        }
        thumbnailUploadPath = thumbnailPath;
    } else {
        if (access(thumbnailExPath.c_str(), F_OK)) {
            UTIL_SYNC_FAULT_REPORT({bundleName_,
                UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
                UtilCloud::FaultType::FILE,
                F_OK,
                "thumbnailExPath " + ReportUtils::GetAnonyString(thumbnailExPath) + " doesn't exist"});
            MEDIA_ERR_LOG("ReportFailure: thumbnailEx doesn't exist %{public}s", thumbnailExPath.c_str());
            return E_THM_SOURCE_BASIC + ENOENT;
        }
        thumbnailUploadPath = thumbnailExPath;
    }
    /* asset */
    MDKAsset content;
    content.uri = move(thumbnailUploadPath);
    content.assetName = FILE_THUMBNAIL;
    content.operationType = MDKAssetOperType::DK_ASSET_ADD;
    recordData["thumbnail"] = MDKRecordField(content);
    return E_OK;
}

std::string CloudFileDataConvert::GetParentPath(const std::string &path)
{
    std::string name;
    size_t slashIndex = path.rfind("/");
    if (slashIndex != std::string::npos) {
        name = path.substr(0, slashIndex);
    }
    return name;
}

int32_t CloudFileDataConvert::HandleLcd(
    std::map<std::string, MDKRecordField> &recordData, std::string &path, bool needUseExDir)
{
    std::string lcdUploadPath;
    std::string lcdExPath = GetThumbPath(path, LCD_EX_SUFFIX);
    std::string lcdExDir = GetParentPath(lcdExPath);
    if (!needUseExDir) {
        std::string lcdPath = GetThumbPath(path, LCD_SUFFIX);
        if (access(lcdPath.c_str(), F_OK)) {
            UTIL_SYNC_FAULT_REPORT({bundleName_,
                UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
                UtilCloud::FaultType::FILE,
                F_OK,
                "lcdPath " + ReportUtils::GetAnonyString(lcdPath) + " doesn't exist"});
            MEDIA_ERR_LOG("ReportFailure: HandleLcd lcd path doesn't exist %{public}s", lcdPath.c_str());
            return E_LCD_SOURCE_BASIC + ENOENT;
        }
        lcdUploadPath = lcdPath;
    } else {
        if (access(lcdExPath.c_str(), F_OK)) {
            UTIL_SYNC_FAULT_REPORT({bundleName_,
                UtilCloud::FaultScenarioCode::CLOUD_FULL_SYNC,
                UtilCloud::FaultType::FILE,
                F_OK,
                "lcdExPath " + ReportUtils::GetAnonyString(lcdExPath) + " doesn't exist"});
            MEDIA_ERR_LOG("ReportFailure: HandleLcd lcdEx path doesn't exist %{public}s", lcdExPath.c_str());
            return E_LCD_SOURCE_BASIC + ENOENT;
        }
        lcdUploadPath = lcdExPath;
    }
    /* asset */
    MDKAsset content;
    content.uri = move(lcdUploadPath);
    content.assetName = FILE_LCD;
    content.operationType = MDKAssetOperType::DK_ASSET_ADD;
    recordData[FILE_LCD] = MDKRecordField(content);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleAttachments(
    std::map<std::string, MDKRecordField> &recordData, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    bool needUseExDir;
    if (upLoadRecord.mediaType == MEDIA_TYPE_IMAGE) {
        needUseExDir = upLoadRecord.exifRotate == 0 ?
            upLoadRecord.orientation != 0 : upLoadRecord.exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    } else {
        needUseExDir = false;
    }
    /* content */
    int32_t ret = E_OK;
    if (upLoadRecord.dirty == -1 || upLoadRecord.dirty != static_cast<int32_t>(DirtyType::TYPE_TDIRTY)) {
        MEDIA_DEBUG_LOG("handle content when not TDIRTY");
        ret = HandleContent(recordData, upLoadRecord);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to handle content");
    }

    /* thumb */
    std::string path = upLoadRecord.data;
    ret = HandleThumbnail(recordData, path, needUseExDir);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleThumbnail failed. ret: %{public}d.", ret);
    /* lcd */
    ret = HandleLcd(recordData, path, needUseExDir);
    return ret;
}

int32_t CloudFileDataConvert::HandleSize(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    if (upLoadRecord.size <= 0) {
        MEDIA_ERR_LOG("ReportFailure: size is invalid");
        return E_DB_SIZE_IS_ZERO;
    }
    data["size"] = MDKRecordField(upLoadRecord.size);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleWidthAndHeight(
    std::map<std::string, MDKRecordField> &properties, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    bool isValid = upLoadRecord.height != 0 && upLoadRecord.width != 0;
    CHECK_AND_PRINT_LOG(isValid, "Get local height or local width is 0 ");
    /* Resolution is combined by cloud sdk, just upload height and width */
    if (upLoadRecord.height != 0) {
        properties["height"] = MDKRecordField(upLoadRecord.height);
    }
    if (upLoadRecord.width != 0) {
        properties["width"] = MDKRecordField(upLoadRecord.width);
    }
    return E_OK;
}

int32_t CloudFileDataConvert::HandleCompatibleFileds(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    /* gallery-specific or shared fileds */
    data["fileName"] = MDKRecordField(upLoadRecord.displayName);
    data["createdTime"] = MDKRecordField(upLoadRecord.dateTaken);
    data["hashId"] = MDKRecordField("Md5_default_hash");
    int32_t ret = this->HandleSize(data, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleSize failed, ret: %{public}d", ret);
    data["source"] = MDKRecordField(upLoadRecord.deviceName);
    data["recycled"] = MDKRecordField(!!(upLoadRecord.dateTrashed));
    data["recycledTime"] = MDKRecordField(std::to_string(upLoadRecord.dateTrashed));
    data["favorite"] = MDKRecordField(!!(upLoadRecord.isFavorite));
    if (upLoadRecord.userComment.size() <= USER_COMMENT_LIMIT_SIZE) {
        data["description"] = MDKRecordField(upLoadRecord.userComment);
    }
    HandleFileType(data, upLoadRecord);

    /* gallery expand fields */
    HandleProperties(data, upLoadRecord);

    /* cloud sdk extra feature */
    if (type_ == CloudOperationType::FILE_CREATE || type_ == CloudOperationType::FILE_DATA_MODIFY) {
        ret = HandleAttachments(data, upLoadRecord);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleAttachments failed, ret: %{public}d.", ret);
    }

    /* cloudsync-specific fields */
    data["mimeType"] = MDKRecordField(upLoadRecord.mimeType);
    data["editedTime"] = MDKRecordField(std::to_string(upLoadRecord.dateModified));
    return E_OK;
}

int32_t CloudFileDataConvert::SetSourceAlbum(MDKRecord &record, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    std::map<std::string, MDKRecordField> data;
    record.GetRecordData(data);
    std::string displayName = upLoadRecord.displayName;
    int32_t hidden = upLoadRecord.hidden;
    std::string albumCloudId = upLoadRecord.albumCloudId;
    std::string albumLPath = upLoadRecord.albumLPath;
    if (hidden == 1) {
        data["albumId"] = MDKRecordField("default-album-4");
        // Upload: hidden === 0, Download: hidden = 1 when albumId default-album-4
        if (data.find(FILE_ATTRIBUTES) != data.end()) {
            std::map<std::string, MDKRecordField> attrs = data[FILE_ATTRIBUTES];
            attrs[MediaColumn::MEDIA_HIDDEN] = MDKRecordField(0);
            data[FILE_ATTRIBUTES] = MDKRecordField(attrs);
        }
    } else if (!albumCloudId.empty()) {
        data["albumId"] = MDKRecordField(albumCloudId);
    }
    MEDIA_DEBUG_LOG("SetSourceAlbum Hidden:%{public}d, albumCloudId:%{public}s, albumLPath::%{public}s",
        hidden,
        albumCloudId.c_str(),
        albumLPath.c_str());
    data["isLogic"] = MDKRecordField(false);
    // pictures should be found an albumid except for hidden and recycle
    bool isRecycle = upLoadRecord.dateTrashed != 0;
    if (!isRecycle && !hidden && (albumLPath.empty() || albumCloudId.empty())) {
        record.SetRecordData(data);
        MEDIA_ERR_LOG("visible media, but albumid is empty");
        return albumLPath.empty() ? E_DB_ALBUM_NOT_FOUND : E_DB_ALBUM_CLOUD_ID_IS_EMPTY;
    }
    if (!albumLPath.empty()) {
        if (data.find(FILE_PROPERTIES) == data.end()) {
            MEDIA_ERR_LOG("record data donnot have properties set");
            record.SetRecordData(data);
            return E_CLOUD_SYNC_DATA;
        }
        std::map<std::string, MDKRecordField> properties = data[FILE_PROPERTIES];
        std::string sourcePath;
        if (albumCloudId == "default-album-2") {
            sourcePath = SCREENSHOT_ALBUM_PATH + displayName;
        } else {
            sourcePath = "/storage/emulated/0" + albumLPath + "/" + displayName;
        }
        properties.erase("sourcePath");
        properties["sourcePath"] = MDKRecordField(sourcePath);
        data[FILE_PROPERTIES] = MDKRecordField(properties);
    }
    record.SetRecordData(data);
    return E_OK;
}

int32_t CloudFileDataConvert::InsertAlbumIdChanges(
    MDKRecord &record, std::vector<MDKRecord> &records, const CloudMdkRecordPhotosVo &upLoadRecord)
{
    MDKRecord tmp = record;
    std::map<std::string, MDKRecordField> data;
    tmp.GetRecordData(data);
    std::string albumCloudId;
    if (data.find("albumId") == data.end() || data["albumId"].GetString(albumCloudId) != MDKLocalErrorCode::NO_ERROR) {
        MEDIA_ERR_LOG("get albumId failed");
    }
    std::vector<MDKRecordField> rmList;
    /* remove */
    std::vector<std::string> removeId = upLoadRecord.removeAlbumCloudId;
    for (const auto &id : removeId) {
        rmList.push_back(MDKRecordField(MDKReference{id, "album"}));
    }
    if (!rmList.empty()) {
        data.erase("albumId");
        data["isLogic"] = MDKRecordField(true);
        data["FILE_RM_LOGIC_ALBUM_IDS"] = MDKRecordField(rmList);
        tmp.SetRecordData(data);
        records.push_back(tmp);
    }
    return E_OK;
}

int32_t CloudFileDataConvert::ConvertToMdkRecord(const CloudMdkRecordPhotosVo &upLoadRecord, MDKRecord &record)
{
    MEDIA_INFO_LOG("CloudFileDataConvert::ConvertToMdkRecord type:%{public}d, cloudId: %{public}s",
        static_cast<int32_t>(type_),
        upLoadRecord.cloudId.c_str());
    // 0 空
    MEDIA_INFO_LOG("ConvertToMdkRecord fileSourceType is %{public}d, storagePath is %{public}s",
        upLoadRecord.fileSourceType, upLoadRecord.storagePath.c_str());
    record.SetRecordType(recordType_);
    if (type_ == CloudOperationType::FILE_CREATE) {
        record.SetNewCreate(true);
    }
    record.SetRecordId(upLoadRecord.cloudId);
    if (type_ == CloudOperationType::FILE_DELETE) {
        return E_OK;
    }
    std::map<std::string, MDKRecordField> data;
    int32_t ret = HandleUniqueFileds(data, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleUniqueFileds failed, ret: %{public}d", ret);
    ret = HandleCompatibleFileds(data, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "HandleCompatibleFileds failed, ret: %{public}d", ret);
    record.SetRecordData(data);
    ret = SetSourceAlbum(record, upLoadRecord);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "SetSourceAlbum failed, ret: %{public}d", ret);
    return E_OK;
}

int32_t CloudFileDataConvert::BuildCopyRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnCopyRecord &record)
{
    MDKRecordPhotosData photosData = MDKRecordPhotosData(result.GetDKRecord());
    std::optional<std::string> optionalCloudId = photosData.GetCloudId();
    record.cloudId = cloudId;
    std::optional<std::string> optPath = photosData.GetFilePath();
    record.path = optPath.value_or("");
    std::optional<int32_t> optFileId = photosData.GetFileId();
    record.fileId = optFileId.value_or(-1);
    std::optional<std::string> optFileName = photosData.GetFileName();
    record.fileName = optFileName.value_or("");
    std::optional<int64_t> optSize = photosData.GetSize();
    record.size = optSize.value_or(-1);
    std::optional<int64_t> optSingleEditTime = photosData.GetDateModified().value_or(0);
    int64_t singleEditTime = optSingleEditTime.value();
    int64_t dualEditTime = static_cast<int64_t>(result.GetDKRecord().GetEditedTime());
    record.modifyTime = dualEditTime > singleEditTime ? dualEditTime : singleEditTime;
    record.createTime = static_cast<int64_t>(result.GetDKRecord().GetCreateTime());
    HandleRotation(record.rotation, photosData);
    std::optional<int32_t> optFileType = photosData.GetFileType();
    record.fileType = optFileType.value_or(-1);
    std::optional<std::string> optSourcePath = photosData.GetSourcePath();
    record.sourcePath = optSourcePath.value_or("");
    record.version = result.GetDKRecord().GetVersion();
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    record.errorType = static_cast<ErrorType>(static_cast<int32_t>(result.GetDKError().errorType));
    record.isSuccess = result.IsSuccess();
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

void CloudFileDataConvert::ConvertErrorTypeDetails(
    const MDKRecordOperResult &result, std::vector<CloudErrorDetail> &errorDetails)
{
    auto errorType = result.GetDKError();
    if (errorType.errorDetails.empty()) {
        return;
    }
    for (const auto &element : errorType.errorDetails) {
        CloudErrorDetail detail;
        detail.domain = element.domain;
        detail.reason = element.reason;
        detail.errorCode = element.errorCode;
        detail.description = element.description;
        detail.errorPos = element.errorPos;
        detail.errorParam = element.errorParam;
        detail.detailCode = element.detailCode;
        errorDetails.push_back(detail);
    }
}

int32_t CloudFileDataConvert::BuildModifyRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnModifyRecord &record)
{
    MDKRecordPhotosData photosData = MDKRecordPhotosData(result.GetDKRecord());
    std::optional<std::string> optionalCloudId = photosData.GetCloudId();
    record.cloudId = cloudId;
    record.path = photosData.GetFilePath().value_or("");
    record.fileName = photosData.GetFileName().value_or("");
    record.fileId = photosData.GetFileId().value_or(-1);
    int64_t singleEditTime = photosData.GetDateModified().value_or(0);
    int64_t dualEditTime = static_cast<int64_t>(result.GetDKRecord().GetEditedTime());
    record.modifyTime = dualEditTime > singleEditTime ? dualEditTime : singleEditTime;
    record.metaDateModified = photosData.GetPhotoMetaDateModified().value_or(-1);
    record.version = result.GetDKRecord().GetVersion();
    record.isSuccess = result.IsSuccess();
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

int32_t CloudFileDataConvert::ConvertFdirtyRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnFileDirtyRecord &record)
{
    MDKRecordPhotosData photosData = MDKRecordPhotosData(result.GetDKRecord());
    record.cloudId = cloudId;
    record.fileId = photosData.GetFileId().value_or(-1);
    auto metaDateModifiedOpt = photosData.GetPhotoMetaDateModified();
    if (metaDateModifiedOpt.has_value()) {
        record.metaDateModified = metaDateModifiedOpt.value();
    } else {
        record.metaDateModified = -1;
    }
    record.path = photosData.GetFilePath().value_or("");
    record.livePhotoCachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(record.path, userId_);
    record.sourceLivePhoto = MovingPhotoFileUtils::GetSourceLivePhotoCachePath(record.path, userId_);
    record.isSuccess = result.IsSuccess();
    record.version = result.GetDKRecord().GetVersion();
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

int32_t CloudFileDataConvert::ConvertToOnCreateRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnCreateRecord &record)
{
    MDKRecordPhotosData photosData = MDKRecordPhotosData(result.GetDKRecord());
    record.cloudId = cloudId;
    record.fileId = photosData.GetFileId().value_or(-1);
    record.path = photosData.GetFilePath().value_or("");
    record.fileName = photosData.GetFileName().value_or("");
    record.localId = photosData.GetLocalId().value_or(-1);
    record.fileType = photosData.GetFileType().value_or(-1);
    record.size = photosData.GetSize().value_or(-1);
    record.createTime = photosData.GetCreatedTime().value_or(-1);
    record.editedTimeMs = photosData.GetEditedTimeMs().value_or(-1);
    record.metaDateModified = photosData.GetPhotoMetaDateModified().value_or(-1);
    record.version = result.GetDKRecord().GetVersion();
    record.isSuccess = result.IsSuccess();
    record.livePhotoCachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(record.path, userId_);
    record.sourceLivePhoto = MovingPhotoFileUtils::GetSourceLivePhotoCachePath(record.path, userId_);
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    record.fileSourceType = photosData.GetFileSourceType().value_or(0);
    record.storagePath = photosData.GetStoragePath().value_or("");
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

int32_t CloudFileDataConvert::ExtractPosition(const std::string &position, double &latitude, double &longitude)
{
    CHECK_AND_RETURN_RET_LOG(!position.empty(), E_INVALID_VALUES, "position is empty.");
    auto json = nlohmann::json::parse(position, nullptr, false);
    bool isValid = !json.is_discarded();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_VALUES, "position json parse error, %{private}s", position.c_str());
    isValid = json.contains("x") && json.contains("y");
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_VALUES, "position miss x or y fields, %{private}s", position.c_str());
    isValid = json["x"].is_string() && json["y"].is_string();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_VALUES, "position x or y is not string, %{private}s", position.c_str());
    std::string latitudeStr = json["x"].get<std::string>();
    std::string longitudeStr = json["y"].get<std::string>();
    std::stringstream latitudestream(latitudeStr);
    std::stringstream longitudestream(longitudeStr);
    latitudestream.precision(15);   // 15:precision
    longitudestream.precision(15);  // 15:precision
    latitude = 0.0;
    longitude = 0.0;
    latitudestream >> latitude;
    longitudestream >> longitude;
    return E_OK;
}

int32_t CloudFileDataConvert::ExtractPosition(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo)
{
    std::string position = data.GetPosition().value_or("");
    return ExtractPosition(position, onFetchPhotoVo.latitude, onFetchPhotoVo.longitude);
}

void CloudFileDataConvert::ConvertProperties(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo)
{
    onFetchPhotoVo.hasproperties = data.hasProperties();
    HandleRotation(onFetchPhotoVo.rotation, data);
    onFetchPhotoVo.fileSourcePath = data.GetSourcePath().value_or("");
    onFetchPhotoVo.firstVisitTime = data.GetFirstUpdateTime().value_or("");
    onFetchPhotoVo.photoHeight = data.GetHeight().value_or(0);
    onFetchPhotoVo.photoWidth = data.GetWidth().value_or(0);
    onFetchPhotoVo.detailTime = data.GetDetailTime().value_or("");
    HandlePropertyExifRotate(data, onFetchPhotoVo);
    ExtractPosition(data, onFetchPhotoVo);
}

void CloudFileDataConvert::ConvertAttributes(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo)
{
    onFetchPhotoVo.hasAttributes = data.hasAttributes();
    onFetchPhotoVo.fileId = data.GetCloudFileId().value_or(0L);
    onFetchPhotoVo.localPath = data.GetFilePath().value_or("");
    onFetchPhotoVo.lcdSize = data.GetLcdSize().value_or(0L);
    onFetchPhotoVo.thmSize = data.GetThmSize().value_or(0L);
    onFetchPhotoVo.metaDateModified = data.GetPhotoMetaDateModified().value_or(0L);
    onFetchPhotoVo.editedTimeMs = data.GetEditedTimeMs().value_or(0L);
    onFetchPhotoVo.fixVersion = data.GetFixVersion().value_or(-1);
    onFetchPhotoVo.frontCamera = data.GetFrontCamera().value_or("");
    onFetchPhotoVo.editDataCamera = data.GetEditDataCamera().value_or("");
    onFetchPhotoVo.title = data.GetTitle().value_or("");
    onFetchPhotoVo.mediaType = data.GetMediaType().value_or(-1);
    onFetchPhotoVo.hidden = data.GetHidden().value_or(0);
    onFetchPhotoVo.hiddenTime = data.GetHiddenTime().value_or(0L);
    onFetchPhotoVo.relativePath = data.GetRelativePath().value_or("");
    onFetchPhotoVo.virtualPath = data.GetVirtualPath().value_or("");
    onFetchPhotoVo.dateYear = data.GetDateYear().value_or("");
    onFetchPhotoVo.dateMonth = data.GetDateMonth().value_or("");
    onFetchPhotoVo.dateDay = data.GetDateDay().value_or("");
    onFetchPhotoVo.shootingMode = data.GetShootingMode().value_or("");
    onFetchPhotoVo.shootingModeTag = data.GetShootingModeTag().value_or("");
    onFetchPhotoVo.burstKey = data.GetBurstKey().value_or("");
    onFetchPhotoVo.burstCoverLevel = data.GetBurstCoverLevel().value_or(1);
    onFetchPhotoVo.subtype = data.GetSubType().value_or(0);
    onFetchPhotoVo.originalSubtype = data.GetOriginalSubType().value_or(0);
    onFetchPhotoVo.dynamicRangeType = data.GetDynamicRangeType().value_or(0);
    onFetchPhotoVo.hdrMode = data.GetHdrMode().value_or(0);
    onFetchPhotoVo.videoMode = data.GetVideoMode().value_or(-1);
    onFetchPhotoVo.movingPhotoEffectMode = data.GetMovingPhotoEffectMode().value_or(0);
    onFetchPhotoVo.editTime = data.GetEditTime().value_or(0);
    onFetchPhotoVo.coverPosition = data.GetCoverPosition().value_or(0);
    onFetchPhotoVo.isRectificationCover = data.GetIsRectificationCover().value_or(0);
    onFetchPhotoVo.exifRotate = data.GetExifRotate().value_or(0);
    onFetchPhotoVo.supportedWatermarkType = data.GetSupportedWatermarkType().value_or(0);
    onFetchPhotoVo.strongAssociation = data.GetStrongAssociation().value_or(0);
    onFetchPhotoVo.fileSourceType = data.GetFileSourceType().value_or(0);
    onFetchPhotoVo.storagePath = data.GetStoragePath().value_or("");
}

void CloudFileDataConvert::ConvertSourceAlbumIds(const MDKRecord &mdkRecord, OnFetchPhotosVo &onFetchPhotoVo)
{
    std::map<std::string, MDKRecordField> data;
    std::vector<MDKRecordField> list;
    mdkRecord.GetRecordData(data);
    if (data.find("albumIds") != data.end()) {
        if (data["albumIds"].GetRecordList(list) != MDKLocalErrorCode::NO_ERROR) {
            MEDIA_WARN_LOG("cannot get album ids from record");
        }
    } else {
        MEDIA_WARN_LOG("not find albumIds from record");
    }
    if (list.size() > 0) {
        for (const auto &it : list) {
            MDKReference ref;
            if (it.GetReference(ref) != MDKLocalErrorCode::NO_ERROR) {
                continue;
            }
            onFetchPhotoVo.sourceAlbumIds.emplace_back(ref.recordId);
        }
    } else {
        MEDIA_WARN_LOG("albumIds list size is 0");
    }
}

int32_t CloudFileDataConvert::ConverMDKRecordToOnFetchPhotosVo(
    const MDKRecord &mdkRecord, OnFetchPhotosVo &onFetchPhotoVo)
{
    MDKRecordPhotosData photosData = MDKRecordPhotosData(mdkRecord);
    onFetchPhotoVo.cloudId = mdkRecord.GetRecordId();
    onFetchPhotoVo.fileName = photosData.GetFileName().value_or("");
    onFetchPhotoVo.size = photosData.GetSize().value_or(0L);
    onFetchPhotoVo.dualEditTime = static_cast<int64_t>(mdkRecord.GetEditedTime());
    onFetchPhotoVo.createTime = static_cast<int64_t>(mdkRecord.GetCreateTime());
    onFetchPhotoVo.fileType = photosData.GetFileType().value_or(0L);
    onFetchPhotoVo.version = mdkRecord.GetVersion();
    onFetchPhotoVo.isDelete = mdkRecord.GetIsDelete();
    onFetchPhotoVo.mimeType = photosData.GetMimeType().value_or("");
    onFetchPhotoVo.isFavorite = photosData.GetFavorite().value_or(false);
    onFetchPhotoVo.isRecycle = photosData.GetRecycled().value_or(false);
    if (onFetchPhotoVo.isRecycle) {
        onFetchPhotoVo.recycledTime = photosData.GetRecycledTime().value_or(0L);
    }
    onFetchPhotoVo.description = photosData.GetDescription().value_or("");
    onFetchPhotoVo.source = photosData.GetSource().value_or("");
    onFetchPhotoVo.duration = photosData.GetDuration().value_or(0);
    if (onFetchPhotoVo.duration == 0) {
        onFetchPhotoVo.duration = photosData.GetPropertiesDuration().value_or(0);
    }
    ConvertSourceAlbumIds(mdkRecord, onFetchPhotoVo);
    ConvertAttributes(photosData, onFetchPhotoVo);
    ConvertProperties(photosData, onFetchPhotoVo);
    ConvertAttributesHashMap(photosData, onFetchPhotoVo);
    return E_OK;
}

int32_t CloudFileDataConvert::HandleAttributesHashMap(
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &uploadRecord)
{
    for (const auto &node : uploadRecord.stringfields) {
        data[node.first] = MDKRecordField(node.second);
    }
    return E_OK;
}

void CloudFileDataConvert::ConvertAttributesHashMap(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo)
{
    std::optional<std::string> valueStrOp;
    for (const auto &fieldName : PHOTOS_SYNC_COLUMN_STRING) {
        valueStrOp = data.GetAttributeFieldValue(fieldName);
        CHECK_AND_CONTINUE(valueStrOp.has_value());
        onFetchPhotoVo.stringfields[fieldName] = valueStrOp.value();
    }
    return;
}

void CloudFileDataConvert::HandleRotation(int32_t &rotation, MDKRecordPhotosData &data)
{
    int32_t rotate = data.GetRotate().value_or(ORIENTATION_NORMAL);
    rotation = ROTATE_ANGLE_0;
    if (FILE_ROTATIONS.find(rotate) != FILE_ROTATIONS.end()) {
        rotation = FILE_ROTATIONS.find(rotate)->second;
        return;
    }
    std::string cloudId = data.GetCloudId().value_or("");
    MEDIA_WARN_LOG("Rotate is filpped, cloudId = %{public}s, rotate = %{public}d",
        cloudId.c_str(), rotate);
}

void CloudFileDataConvert::HandlePropertyExifRotate(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo)
{
    bool isValid = onFetchPhotoVo.exifRotate == 0 && data.GetPropertyExifRotate().has_value();
    CHECK_AND_RETURN(isValid);
    onFetchPhotoVo.exifRotate = data.GetPropertyExifRotate().value_or(0);
    MEDIA_INFO_LOG("Has propertyExifRotate, cloudId = %{public}s, exifRotate = %{public}d",
        onFetchPhotoVo.cloudId.c_str(), onFetchPhotoVo.exifRotate);
}
} // namespace OHOS::Media::CloudSync