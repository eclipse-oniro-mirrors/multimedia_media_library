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
#define MLOG_TAG "Media_Operation"

#include "photo_file_operation.h"

#include <sstream>

#include "cloud_media_sync_utils.h"
#include "dfx_manager.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_utils.h"
#include "medialibrary_tracer.h"
#include "userfile_manager_types.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "media_column.h"
#include "result_set_utils.h"
#include "lake_file_utils.h"
#include "thumbnail_service.h"

namespace OHOS::Media {
// LCOV_EXCL_START
std::string PhotoFileOperation::ToString(const PhotoAssetInfo &photoInfo)
{
    std::stringstream ss;
    ss << "PhotoAssetInfo[displayName: " << photoInfo.displayName << ", filePath: " << photoInfo.filePath
       << ", dateModified: " << photoInfo.dateModified << ", subtype: " << photoInfo.subtype
       << ", videoFilePath: " << photoInfo.videoFilePath << ", editDataFolder: " << photoInfo.editDataFolder
       << ", thumbnailFolder: " << photoInfo.thumbnailFolder << ", isMovingPhoto: " << photoInfo.isMovingPhoto << "]";
    return ss.str();
}

/**
 * @brief Copy Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::CopyPhoto(
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::string &targetPath)
{
    bool cond = (resultSet == nullptr || targetPath.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL,
        "Media_Operation: CopyPhoto failed, resultSet is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    sourcePhotoInfo.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    sourcePhotoInfo.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t originalSubtype = GetInt32Val(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    sourcePhotoInfo.isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(
        sourcePhotoInfo.subtype, effectMode, originalSubtype);
    sourcePhotoInfo.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    sourcePhotoInfo.videoFilePath = this->FindVideoFilePath(sourcePhotoInfo);
    sourcePhotoInfo.editDataFolder = this->FindEditDataFolder(sourcePhotoInfo);
    // Build the Target Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.displayName = sourcePhotoInfo.displayName;
    targetPhotoInfo.filePath = targetPath;
    targetPhotoInfo.subtype = sourcePhotoInfo.subtype;
    targetPhotoInfo.dateModified = sourcePhotoInfo.dateModified;
    // No need to copy video file if the Original Photo is not a moving photo.
    if (!sourcePhotoInfo.videoFilePath.empty()) {
        targetPhotoInfo.videoFilePath = this->GetVideoFilePath(targetPhotoInfo);
    }
    // No need to copy edit data folder if the Original Photo is not edited.
    if (!sourcePhotoInfo.editDataFolder.empty()) {
        targetPhotoInfo.editDataFolder = this->BuildEditDataFolder(targetPhotoInfo);
    }
    MEDIA_INFO_LOG("Media_Operation: sourcePhotoInfo: %{public}s, targetPhotoInfo: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    return this->CopyPhoto(sourcePhotoInfo, targetPhotoInfo);
}

/**
 * @brief Copy thumbnail, include folder and astc data.
 */
int32_t PhotoFileOperation::CopyThumbnail(
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::string &targetPath, int64_t &newAssetId)
{
    bool cond = (resultSet == nullptr || targetPath.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL,
        "Media_Operation: CopyPhoto failed, resultSet is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    sourcePhotoInfo.thumbnailFolder = this->FindThumbnailFolder(sourcePhotoInfo);
    if (sourcePhotoInfo.thumbnailFolder.empty()) {
        MEDIA_INFO_LOG("Source thumbnail is empty, skip copy. thumbnailFolder:%{public}s",
            sourcePhotoInfo.thumbnailFolder.c_str());
        return E_FAIL;
    }

    // copy folder
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.filePath = targetPath;
    targetPhotoInfo.thumbnailFolder = this->BuildThumbnailFolder(targetPhotoInfo);
    int32_t opRet = this->CopyPhotoRelatedThumbnail(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);

    std::string dateTaken = to_string(GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet));
    std::string oldAssetId = to_string(GetInt64Val(MediaColumn::MEDIA_ID, resultSet));
    return HandleThumbnailAstcData(dateTaken, oldAssetId, to_string(newAssetId));
}

int32_t PhotoFileOperation::CopyThumbnail(
    const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo, bool withAstcData)
{
    bool isValid = sourcePhotosPo.data.has_value();
    isValid = isValid && targetPhotosPo.data.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_ARGUMENTS, "Invalid arguments");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.filePath = sourcePhotosPo.data.value_or("");
    sourcePhotoInfo.thumbnailFolder = this->FindThumbnailFolder(sourcePhotoInfo);
    CHECK_AND_RETURN_RET_LOG(!sourcePhotoInfo.thumbnailFolder.empty(),
        E_FAIL,
        "Source thumbnail is empty, skip copy. thumbnailFolder:%{public}s",
        sourcePhotoInfo.thumbnailFolder.c_str());

    // copy folder
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.filePath = targetPhotosPo.data.value_or("");
    targetPhotoInfo.thumbnailFolder = this->BuildThumbnailFolder(targetPhotoInfo);
    int32_t opRet = this->CopyPhotoRelatedThumbnail(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);

    CHECK_AND_RETURN_RET(withAstcData, E_OK);
    isValid = sourcePhotosPo.fileId.has_value();
    isValid = isValid && targetPhotosPo.fileId.has_value();
    isValid = isValid && sourcePhotosPo.dateTaken.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_ARGUMENTS, "Invalid arguments, fileId is empty");
    std::string dateTaken = std::to_string(sourcePhotosPo.dateTaken.value_or(0));
    std::string oldAssetId = std::to_string(sourcePhotosPo.fileId.value_or(0));
    std::string targetAssetId = std::to_string(targetPhotosPo.fileId.value_or(0));
    return HandleThumbnailAstcData(dateTaken, oldAssetId, targetAssetId);
}

int32_t PhotoFileOperation::HandleThumbnailAstcData(const std::string &dateTaken, const std::string &oldAssetId,
    const std::string &newAssetId)
{
    // 取旧key拿value，然后在put新key和value（和旧的value一样）到kvstore中
    string oldKey;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::GenerateKvStoreKey(oldAssetId, dateTaken, oldKey), E_ERR,
        "GenerateKvStoreKey failed");

    string newKey;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::GenerateKvStoreKey(newAssetId, dateTaken, newKey), E_ERR,
        "GenerateKvStoreKey failed");

    int32_t err = MediaLibraryKvStoreUtils::CopyAstcDataToKvStoreByType(KvStoreValueType::MONTH_ASTC, oldKey, newKey);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "CopyAstcDataToKvStoreByType failed, err: %{public}d", err);

    err = MediaLibraryKvStoreUtils::CopyAstcDataToKvStoreByType(KvStoreValueType::YEAR_ASTC, oldKey, newKey);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "CopyAstcDataToKvStoreByType failed, err: %{public}d", err);
    MEDIA_INFO_LOG("Success to copy thumbnail. oldKey:%{public}s, newKey:%{public}s", oldKey.c_str(), newKey.c_str());
    return E_OK;
}

/**
 * @brief Copy Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::CopyPhoto(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    int32_t opRet = this->CopyPhotoFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);

    opRet = this->CopyPhotoRelatedVideoFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);

    return this->CopyPhotoRelatedExtraData(sourcePhotoInfo, targetPhotoInfo);
}

/**
 * @brief Get the video file path of the photo, without any check.
 * @return the video file path of the photo. Only replace the suffix of file extension, such as .jpg to .mp4.
 * @example if filePath is /xxx/.../xxx/123456789.jpg, then return /xxx/.../xxx/123456789.mp4.
 */
std::string PhotoFileOperation::GetVideoFilePath(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    return MediaFileUtils::GetMovingPhotoVideoPath(photoInfo.filePath);
}

/**
 * @brief Get the video file path of the photo, without any check.
 * @return the video file path of the photo. Only replace the suffix of file extension, such as .jpg to .mp4.
 *         If the photo is not a moving photo, return an empty string.
 *         If the photo's file path is empty, return an empty string.
 *         If the photo is a moving photo, return the video file path of the photo.
 */
std::string PhotoFileOperation::FindVideoFilePath(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    if (!photoInfo.isMovingPhoto) {
        return "";
    }
    // If file path is empty, return empty string. Trace log will be printed in CopyPhotoFile.
    CHECK_AND_RETURN_RET(!photoInfo.filePath.empty(), "");

    std::string videoFilePath = this->GetVideoFilePath(photoInfo);
    if (!MediaFileUtils::IsFileExists(videoFilePath)) {
        MEDIA_WARN_LOG("Media_Operation: videoFilePath not exists, videoFilePath: %{public}s, Object: %{public}s.",
            videoFilePath.c_str(),
            this->ToString(photoInfo).c_str());
        return "";
    }
    return videoFilePath;
}

/**
 * @brief Find the Relative Path of filePath.
 * @return Relative Path.
 *         If prefix of filePath is "/storage/cloud/files", return the relative path of cloud prefix.
 *         If prefix of filePath is "/storage/media/local/files", return the relative path of local prefix.
 *         Otherwise, return empty string.
 */
std::string PhotoFileOperation::FindRelativePath(const std::string &filePath)
{
    std::string prefix = "/storage/cloud/files";
    size_t pos = filePath.find(prefix);
    CHECK_AND_RETURN_RET(pos == std::string::npos, filePath.substr(pos + prefix.size()));

    prefix = "/storage/media/local/files";
    pos = filePath.find(prefix);
    CHECK_AND_RETURN_RET(pos == std::string::npos, filePath.substr(pos + prefix.size()));
    MEDIA_ERR_LOG("Media_Operation: Find RelativePath failed, filePath: %{public}s", filePath.c_str());
    return "";
}

/**
 * @brief Find the prefix of photo Data Folder.
 * @return Return the prefix of photo Data Folder.
 *         There are two prefixes, one is local, the other is cloud.
 *         If the prefix of filePath is cloud, return "/storage/cloud/files/" + dataName.
 *         If the prefix of filePath is local, return "/storage/media/local/files/" + dataName;
 *         If the prefix of filePath is not found, return "".
 */
std::string PhotoFileOperation::FindPrefixDataFolder(const std::string &filePath, const std::string &dataName)
{
    std::string prefix = "/storage/cloud/files";
    size_t pos = filePath.find(prefix);
    if (pos != std::string::npos) {
        return "/storage/cloud/files/" + dataName;
    }
    prefix = "/storage/media/local/files";
    pos = filePath.find(prefix);
    if (pos != std::string::npos) {
        return "/storage/media/local/files/" + dataName;
    }
    MEDIA_WARN_LOG("Media_Operation: Get Prefix failed, filePath: %{public}s", filePath.c_str());
    return "";
}

/**
 * @brief Find the prefix of Edit Data Folder.
 * @return Return the prefix of Edit Data Folder.
 *         There are two prefixes, one is local, the other is cloud.
 *         If the prefix of filePath is cloud, return "/storage/cloud/files/.editData".
 *         If the prefix of filePath is local, return "/storage/media/local/files/.editData";
 *         If the prefix of filePath is not found, return "".
 */
std::string PhotoFileOperation::FindPrefixOfEditDataFolder(const std::string &filePath)
{
    return FindPrefixDataFolder(filePath, ".editData");
}

/**
 * @brief Find the prefix of thumbs Data Folder.
 * @return Return the prefix of thumbs Data Folder.
 *         There are two prefixes, one is local, the other is cloud.
 *         If the prefix of filePath is cloud, return "/storage/cloud/files/.thumbs".
 *         If the prefix of filePath is local, return "/storage/media/local/files/.thumbs";
 *         If the prefix of filePath is not found, return "".
 */
std::string PhotoFileOperation::FindPrefixOfThumbnailFolder(const std::string &filePath)
{
    return FindPrefixDataFolder(filePath, ".thumbs");
}

/**
 * @brief Build the Edit Data Folder for the Photo file, without any check.
 * @return Return the Edit Data Folder for the Photo file.
 *         If the filePath is cloud, return "/storage/cloud/files/.editData/" + the relativePath of Photo file.
 *         If the filePath is local, return "/storage/media/local/files/.editData/" + the relativePath of Photo file.
 *         If the filePath is not identified, return "".
 */
std::string PhotoFileOperation::BuildEditDataFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    std::string prefix = this->FindPrefixOfEditDataFolder(photoInfo.filePath);
    std::string relativePath = this->FindRelativePath(photoInfo.filePath);
    if (prefix.empty() || relativePath.empty()) {
        return "";
    }
    return prefix + relativePath;
}

/**
 * @brief Build the thumbs Data Folder for the Photo file, without any check.
 * @return Return the thumbs Data Folder for the Photo file.
 *         If the filePath is cloud, return "/storage/cloud/files/.thumbs/" + the relativePath of Photo file.
 *         If the filePath is local, return "/storage/media/local/files/.thumbs/" + the relativePath of Photo file.
 *         If the filePath is not identified, return "".
 */
std::string PhotoFileOperation::BuildThumbnailFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    std::string prefix = this->FindPrefixOfThumbnailFolder(photoInfo.filePath);
    std::string relativePath = this->FindRelativePath(photoInfo.filePath);
    if (prefix.empty() || relativePath.empty()) {
        return "";
    }
    return prefix + relativePath;
}

/**
 * @brief Find the thumbs Data Folder for the Photo file.
 * @return Return the thumbs Data Folder path. If the thumbs Data Folder is invalid, return empty string.
 *         If the thumbs Data Folder is not exist, has 3 scenario:
 *         1. The photo file is not thumbs. Normal scenario.
 *         2. The photo file is not MOVING_PHOTO. Noraml scenario.
 *         3. The photo file is thumbs or MOVING_PHOTO, but the thumbs Data Folder is not exist. Exceptional scenario.
 */
std::string PhotoFileOperation::FindThumbnailFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    std::string thumbnailFolderPath = this->BuildThumbnailFolder(photoInfo);
    if (!thumbnailFolderPath.empty() && !MediaFileUtils::IsFileExists(thumbnailFolderPath)) {
        MEDIA_INFO_LOG("Media_Operation: thumbnailFolder: %{public}s, not exists, Object: %{public}s.",
            thumbnailFolderPath.c_str(),
            this->ToString(photoInfo).c_str());
        return "";
    }
    return thumbnailFolderPath;
}

/**
 * @brief Find the Edit Data Folder for the Photo file.
 * @return Return the Edit Data Folder path. If the Edit Data Folder is invalid, return empty string.
 *         If the Edit Data Folder is not exist, has 3 scenario:
 *         1. The photo file is not edited. Normal scenario.
 *         2. The photo file is not MOVING_PHOTO. Noraml scenario.
 *         3. The photo file is edited or MOVING_PHOTO, but the Edit Data Folder is not exist. Exceptional scenario.
 */
std::string PhotoFileOperation::FindEditDataFolder(const PhotoFileOperation::PhotoAssetInfo &photoInfo)
{
    std::string editDataFolderPath = this->BuildEditDataFolder(photoInfo);
    if (!editDataFolderPath.empty() && !MediaFileUtils::IsFileExists(editDataFolderPath)) {
        MEDIA_INFO_LOG("Media_Operation: EditDataFolder not exists, It may not be edited photo or moving photo. "
                       "Object: %{public}s.",
            this->ToString(photoInfo).c_str());
        return "";
    }
    return editDataFolderPath;
}

/**
 * @brief Copy Photo File, only include the photo file defined in the Photos table.
 * @return E_OK if success,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::CopyPhotoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    std::string srcPath = sourcePhotoInfo.filePath;
    std::string targetPath = targetPhotoInfo.filePath;
    int64_t dateModified = targetPhotoInfo.dateModified;
    // If File Path is empty, return E_INVALID_PATH.
    if (srcPath.empty() || targetPath.empty()) {
        MEDIA_ERR_LOG("Media_Operation: CopyPhotoFile failed, srcPath or targetPath is empty. "
                      "Source Object: %{public}s, Target Object: %{public}s",
            this->ToString(sourcePhotoInfo).c_str(),
            this->ToString(targetPhotoInfo).c_str());
        return E_INVALID_PATH;
    }
    int32_t opRet = this->CopyFile(srcPath, targetPath);
    if (opRet != E_OK) {
        MEDIA_ERR_LOG("Media_Operation: CopyPhoto failed, srcPath: %{public}s, targetPath: %{public}s",
            srcPath.c_str(),
            targetPath.c_str());
        return opRet;
    }
    MediaFileUtils::ModifyFile(targetPath, dateModified / MSEC_TO_SEC);
    MEDIA_INFO_LOG("Media_Operation: CopyPhotoFile success, srcPath: %{public}s, targetPath: %{public}s",
        srcPath.c_str(),
        targetPath.c_str());
    return E_OK;
}

/**
 * @brief Copy Photo File, only include the vide file related to the photo file defined in the Photos table.
 * @return E_OK if success or not MOVING_PHOTO or video file empty,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::CopyPhotoRelatedVideoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    // If photoSubtype is MOVING_PHOTO, copy video file.
    CHECK_AND_RETURN_RET(sourcePhotoInfo.isMovingPhoto, E_OK);
    std::string srcVideoPath = sourcePhotoInfo.videoFilePath;
    std::string targetVideoPath = targetPhotoInfo.videoFilePath;
    int64_t dateModified = targetPhotoInfo.dateModified;
    // If video file is empty, return E_OK. Trace log will be printed in FindVideoFilePath.
    bool cond = (srcVideoPath.empty() || targetVideoPath.empty());
    CHECK_AND_RETURN_RET(!cond, E_OK);

    int32_t opRet = this->CopyFile(srcVideoPath, targetVideoPath);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet,
        "Media_Operation: CopyPhoto Video failed, srcPath: %{public}s, targetPath: %{public}s", srcVideoPath.c_str(),
        targetVideoPath.c_str());
    MediaFileUtils::ModifyFile(targetVideoPath, dateModified / MSEC_TO_SEC);
    MEDIA_INFO_LOG("Media_Operation: CopyPhotoRelatedVideoFile success, srcPath: %{public}s, targetPath: %{public}s",
        srcVideoPath.c_str(),
        targetVideoPath.c_str());
    return E_OK;
}

int32_t PhotoFileOperation::CopyPhotoRelatedData(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo,
    const std::string& srcFolder, const std::string& targetFolder)
{
    bool cond = (srcFolder.empty() || targetFolder.empty());
    CHECK_AND_RETURN_RET(!cond, E_OK);

    if (!MediaFileUtils::IsFileExists(srcFolder)) {
        MEDIA_ERR_LOG("Media_Operation: %{public}s doesn't exist. %{public}s",
            srcFolder.c_str(), this->ToString(sourcePhotoInfo).c_str());
        return E_NO_SUCH_FILE;
    }
    int32_t opRet = MediaFileUtils::CopyDirectory(srcFolder, targetFolder);
    this->AuditLog(srcFolder, TAG_COPY_SOURCE, opRet);
    this->AuditLog(targetFolder, TAG_COPY_TARGET, opRet);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet,
        "Media_Operation: CopyPhoto extraData failed, sourceInfo: %{public}s, targetInfo: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(), this->ToString(targetPhotoInfo).c_str());

    MEDIA_INFO_LOG("Media_Operation: CopyPhotoRelatedExtraData success, sourceInfo:%{public}s, targetInfo:%{public}s",
        this->ToString(sourcePhotoInfo).c_str(), this->ToString(targetPhotoInfo).c_str());
    return E_OK;
}

/**
 * @brief Copy the Edit Data.
 * @return E_OK if success or not edited photo.
 *         E_NO_SUCH_FILE if the source Edit Data Folder not exits.
 *         E_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::CopyPhotoRelatedExtraData(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    std::string srcEditDataFolder = sourcePhotoInfo.editDataFolder;
    std::string targetEditDataFolder = targetPhotoInfo.editDataFolder;
    return CopyPhotoRelatedData(sourcePhotoInfo, targetPhotoInfo, srcEditDataFolder, targetEditDataFolder);
}

/**
 * @brief Copy the thumbnail Data.
 * @return E_OK if success or not thumbnail folder.
 *         E_NO_SUCH_FILE if the source Edit Data Folder not exits.
 *         E_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::CopyPhotoRelatedThumbnail(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    std::string srcThumbnailFolder = sourcePhotoInfo.thumbnailFolder;
    std::string targetThumbnailFolder = targetPhotoInfo.thumbnailFolder;
    int32_t opRet = CopyPhotoRelatedData(sourcePhotoInfo, targetPhotoInfo, srcThumbnailFolder, targetThumbnailFolder);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet,
        "Media_Operation: CopyPhoto thumbnail failed, srcPath: %{public}s, targetPath: %{public}s",
        srcThumbnailFolder.c_str(), targetThumbnailFolder.c_str());
    MEDIA_INFO_LOG("Media_Operation: CopyPhotoThumbnail success, srcThumbDir: %{public}s, targetThumbDir: %{public}s",
        srcThumbnailFolder.c_str(), targetThumbnailFolder.c_str());

    return E_OK;
}

/**
 * @brief Copy File.
 * @return E_OK if success,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::CopyFile(const std::string &srcPath, std::string &targetPath)
{
    bool opRet = LakeFileUtils::CopyFile(srcPath, targetPath);
    opRet = opRet && MediaFileUtils::IsFileExists(targetPath);
    this->AuditLog(srcPath, TAG_COPY_SOURCE, opRet ? E_OK : E_ERR);
    this->AuditLog(targetPath, TAG_COPY_TARGET, opRet ? E_OK : E_ERR);
    if (!opRet) {
        MEDIA_ERR_LOG("Media_Operation: CopyFile failed, filePath: %{public}s, errno: %{public}d, errmsg: %{public}s",
            srcPath.c_str(),
            errno,
            strerror(errno));
        return E_FILE_OPER_FAIL;
    }
    return E_OK;
}

/**
 * @brief ConvertFormat Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::ConvertFormatPhoto(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    const std::string &targetPath, const std::string &extension)
{
    bool cond = (resultSet == nullptr || targetPath.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, E_FAIL,
        "Media_Operation: ConvertFormatPhoto failed, resultSet is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    sourcePhotoInfo.filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    sourcePhotoInfo.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    sourcePhotoInfo.isMovingPhoto = ((sourcePhotoInfo.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) ||
        (effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)));
    sourcePhotoInfo.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    sourcePhotoInfo.videoFilePath = this->FindVideoFilePath(sourcePhotoInfo);
    sourcePhotoInfo.editDataFolder = this->FindEditDataFolder(sourcePhotoInfo);

    // Build the Target Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.displayName = sourcePhotoInfo.displayName;
    targetPhotoInfo.filePath = targetPath;
    targetPhotoInfo.subtype = sourcePhotoInfo.subtype;
    targetPhotoInfo.dateModified = sourcePhotoInfo.dateModified;
    // No need to copy video file if the Original Photo is not a moving photo.
    if (!sourcePhotoInfo.videoFilePath.empty()) {
        targetPhotoInfo.videoFilePath = this->GetVideoFilePath(targetPhotoInfo);
    }
    // No need to copy edit data folder if the Original Photo is not edited.
    if (!sourcePhotoInfo.editDataFolder.empty()) {
        targetPhotoInfo.editDataFolder = this->BuildEditDataFolder(targetPhotoInfo);
    }
    MEDIA_INFO_LOG("Media_Operation: sourcePhotoInfo: %{public}s, targetPhotoInfo: %{public}s, extension: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(), this->ToString(targetPhotoInfo).c_str(), extension.c_str());
    return this->ConvertFormatPhoto(sourcePhotoInfo, targetPhotoInfo, extension);
}

int32_t PhotoFileOperation::ConvertFormatPhoto(const PhotoAssetInfo &sourcePhotoInfo,
    const PhotoAssetInfo &targetPhotoInfo, const std::string &extension)
{
    // copy renderings, need convert format
    int32_t opRet = this->ConvertFormatFile(sourcePhotoInfo.filePath, targetPhotoInfo.filePath,
        targetPhotoInfo.dateModified, extension);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet, "ConvertFormat File failed");
    MEDIA_INFO_LOG("ConvertFormat File success, srcFile: %{public}s, dstFile: %{public}s, extension: %{public}s",
        sourcePhotoInfo.filePath.c_str(), targetPhotoInfo.filePath.c_str(), extension.c_str());

    // copy moving photo
    if (sourcePhotoInfo.isMovingPhoto) {
        opRet = this->ConvertFormatFile(sourcePhotoInfo.videoFilePath, targetPhotoInfo.videoFilePath,
            targetPhotoInfo.dateModified, "");
        CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet, "ConvertFormat Video failed");
        MEDIA_INFO_LOG("ConvertFormat Video success, srcVideo: %{public}s, dstVideo: %{public}s",
            sourcePhotoInfo.videoFilePath.c_str(), targetPhotoInfo.videoFilePath.c_str());
    }

    // copy editData folder, source.heic need convert format, other copy
    opRet = this->ConvertFormatPhotoExtraData(sourcePhotoInfo.editDataFolder, targetPhotoInfo.editDataFolder,
        extension);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK, opRet, "ConvertFormat PhotoExtraData failed");
    MEDIA_INFO_LOG("ConvertFormat ExtraData success, srcExtraData: %{public}s, dstExtraData: %{public}s, "
        "extension: %{public}s", sourcePhotoInfo.editDataFolder.c_str(), targetPhotoInfo.editDataFolder.c_str(),
        extension.c_str());

    return E_OK;
}

int32_t PhotoFileOperation::ConvertFormatFile(const std::string &srcFilePath, const std::string &dstFilePath,
    const int64_t dateModified, const std::string &extension)
{
    // If File Path is empty, return E_INVALID_PATH.
    std::string tmpPath = LakeFileUtils::GetAssetRealPath(srcFilePath);
    if (tmpPath.empty() || dstFilePath.empty() || !MediaFileUtils::IsFileExists(tmpPath) ||
        !MediaFileUtils::IsFileValid(tmpPath)) {
        MEDIA_ERR_LOG("Media_Operation: check srcPath or targetPath failed");
        return E_INVALID_PATH;
    }

    bool ret = false;
    if (!extension.empty()) {
        ret = MediaFileUtils::ConvertFormatCopy(tmpPath, dstFilePath, extension);
    } else {
        ret = MediaFileUtils::CopyFileUtil(tmpPath, dstFilePath);
    }
    if (!ret || !MediaFileUtils::IsFileExists(dstFilePath)) {
        MEDIA_INFO_LOG("Media_Operation: ConvertFormatFile failed, tmpPath: %{public}s, dstFilePath: %{public}s, "
            "extension: %{public}s", tmpPath.c_str(), dstFilePath.c_str(), extension.c_str());
        return E_FILE_OPER_FAIL;
    }

    MediaFileUtils::ModifyFile(dstFilePath, dateModified / MSEC_TO_SEC);
    return E_OK;
}

int32_t PhotoFileOperation::ConvertFormatPhotoExtraData(const std::string &srcPath, const std::string &dstPath,
    const std::string &extension)
{
    if (srcPath.empty() || dstPath.empty()) {
        return E_OK;
    }
    if (!MediaFileUtils::IsFileExists(srcPath)) {
        MEDIA_ERR_LOG("Media_Operation: %{public}s doesn't exist", srcPath.c_str());
        return E_NO_SUCH_FILE;
    }

    int32_t ret = MediaFileUtils::ConvertFormatExtraDataDirectory(srcPath, dstPath, extension);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ConvertFormatExtraDataDirectory failed, srcPath: %{public}s, dstPath: %{public}s, "
            "extension: %{public}s", srcPath.c_str(), dstPath.c_str(), extension.c_str());
        return ret;
    }
    return E_OK;
}

static int32_t DoTranscodeFailedDfx(const std::string &errMsg, const TranscodeErrorType &type, int32_t ret,
    const std::string &filePath = "")
{
    MEDIA_ERR_LOG("%{public}s", errMsg.c_str());
    CHECK_AND_EXECUTE(filePath.empty(), MediaFileUtils::DeleteFile(filePath));
    auto dfxManager = DfxManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dfxManager != nullptr, ret, "DfxManager::GetInstance() returned nullptr");
    dfxManager->HandleTranscodeFailed(type);
    return ret;
}

int32_t PhotoFileOperation::CreateTmpCompatibleDup(const std::string &srcPath, size_t &size)
{
    CHECK_AND_RETURN_RET(MediaFileUtils::IsFileExists(srcPath),
        DoTranscodeFailedDfx("Origin file is not exists.", INNER_FAILED, E_PARAM_CONVERT_FORMAT));
    const std::string extension = "jpg";
    const std::string duplicate = "/transcode.jpg";
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.filePath = std::move(srcPath);
    auto editDataFolder = BuildEditDataFolder(sourcePhotoInfo);
    CHECK_AND_RETURN_RET(!editDataFolder.empty(), DoTranscodeFailedDfx(
        "CreateTmpCompatibleDup editDataFolder is empty", INNER_FAILED, E_INNER_FAIL));
    if (!MediaFileUtils::IsDirExists(editDataFolder)) {
        CHECK_AND_RETURN_RET(MediaFileUtils::CreateDirectory(editDataFolder),
            DoTranscodeFailedDfx("Create editDataFolder fail", INNER_FAILED, E_INNER_FAIL));
    }

    MediaLibraryTracer tracer;
    tracer.Start("CreateTmpCompatibleDup");

    auto targetPath = std::move(editDataFolder) + duplicate;
    CHECK_AND_RETURN_RET(MediaFileUtils::ConvertFormatCopy(sourcePhotoInfo.filePath, targetPath, extension),
        DoTranscodeFailedDfx("ConvertFormatCopy fail", CODEC_FAILED, E_INNER_FAIL, targetPath));

    tracer.Finish();
    CHECK_AND_RETURN_RET(MediaFileUtils::GetFileSize(targetPath, size),
        DoTranscodeFailedDfx("GetFileSize fail", INNER_FAILED, E_INNER_FAIL));
    return E_OK;
}

/**
 * @brief Move File.
 * @return E_OK if success,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::MoveFile(const std::string &srcPath, std::string &targetPath)
{
    if (srcPath.empty() || !MediaFileUtils::IsFileExists((srcPath)) || !MediaFileUtils::IsFileValid(srcPath)) {
        MEDIA_ERR_LOG("Media_Operation: source file invalid! srcPath: %{public}s", srcPath.c_str());
        this->AuditLog(srcPath, TAG_MOVE_OUT, E_INVALID_PATH);
        return E_INVALID_PATH;
    }
    if (targetPath.empty()) {
        MEDIA_ERR_LOG("Media_Operation: target file invalid! targetPath: %{public}s", targetPath.c_str());
        this->AuditLog(targetPath, TAG_MOVE_IN, E_INVALID_PATH);
        return E_INVALID_PATH;
    }
    bool opRet = MediaFileUtils::MoveFile(srcPath, targetPath);
    if (!opRet) {
        MEDIA_WARN_LOG("Media_Operation: MoveFile failed, try CrossPolicy mode.");
        opRet = MediaFileUtils::MoveFile(srcPath, targetPath, true);
    }
    opRet = opRet && MediaFileUtils::IsFileExists(targetPath);
    this->AuditLog(srcPath, TAG_MOVE_OUT, opRet ? E_OK : E_ERR);
    this->AuditLog(targetPath, TAG_MOVE_IN, opRet ? E_OK : E_ERR);
    if (!opRet) {
        MEDIA_ERR_LOG("Media_Operation: MoveFile failed, filePath: %{public}s, errno: %{public}d, errmsg: %{public}s",
            srcPath.c_str(),
            errno,
            strerror(errno));
        return E_FILE_OPER_FAIL;
    }
    return E_OK;
}

/**
 * @brief Copy Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::CopyPhoto(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo)
{
    bool isValid = sourcePhotosPo.data.has_value();
    isValid = isValid && sourcePhotosPo.displayName.has_value();
    isValid = isValid && sourcePhotosPo.subtype.has_value();
    isValid = isValid && sourcePhotosPo.originalSubtype.has_value();
    isValid = isValid && sourcePhotosPo.movingPhotoEffectMode.has_value();
    isValid = isValid && sourcePhotosPo.dateModified.has_value();
    isValid = isValid && targetPhotosPo.data.has_value();
    isValid = isValid && targetPhotosPo.displayName.has_value();
    isValid = isValid && targetPhotosPo.subtype.has_value();
    isValid = isValid && targetPhotosPo.dateModified.has_value();
    isValid = isValid && !targetPhotosPo.data.value_or("").empty();
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_INVALID_ARGUMENTS,
        "Media_Operation: CopyPhoto failed, sourcePhotosPo is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.displayName = sourcePhotosPo.displayName.value_or("");
    sourcePhotoInfo.filePath = sourcePhotosPo.data.value_or("");
    sourcePhotoInfo.subtype = sourcePhotosPo.subtype.value_or(0);
    int32_t originalSubtype = sourcePhotosPo.originalSubtype.value_or(0);
    int32_t effectMode = sourcePhotosPo.movingPhotoEffectMode.value_or(0);
    sourcePhotoInfo.isMovingPhoto =
        MovingPhotoFileUtils::IsMovingPhoto(sourcePhotoInfo.subtype, effectMode, originalSubtype);
    sourcePhotoInfo.dateModified = sourcePhotosPo.dateModified.value_or(0);
    sourcePhotoInfo.videoFilePath = this->FindVideoFilePath(sourcePhotoInfo);
    sourcePhotoInfo.editDataFolder = this->FindEditDataFolder(sourcePhotoInfo);
    // Build the Target Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.displayName = targetPhotosPo.displayName.value_or("");
    targetPhotoInfo.filePath = targetPhotosPo.data.value_or("");
    targetPhotoInfo.subtype = targetPhotosPo.subtype.value_or(0);
    targetPhotoInfo.dateModified = targetPhotosPo.dateModified.value_or(0);
    // No need to copy video file if the Original Photo is not a moving photo.
    if (!sourcePhotoInfo.videoFilePath.empty()) {
        targetPhotoInfo.videoFilePath = this->GetVideoFilePath(targetPhotoInfo);
    }
    // No need to copy edit data folder if the Original Photo is not edited.
    if (!sourcePhotoInfo.editDataFolder.empty()) {
        targetPhotoInfo.editDataFolder = this->BuildEditDataFolder(targetPhotoInfo);
    }
    MEDIA_INFO_LOG("Media_Operation: sourcePhotoInfo: %{public}s, targetPhotoInfo: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    return this->CopyPhoto(sourcePhotoInfo, targetPhotoInfo);
}

/**
 * @brief Copy Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::MovePhoto(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo)
{
    bool isValid = sourcePhotosPo.data.has_value();
    isValid = isValid && sourcePhotosPo.displayName.has_value();
    isValid = isValid && sourcePhotosPo.subtype.has_value();
    isValid = isValid && sourcePhotosPo.originalSubtype.has_value();
    isValid = isValid && sourcePhotosPo.movingPhotoEffectMode.has_value();
    isValid = isValid && sourcePhotosPo.dateModified.has_value();
    isValid = isValid && targetPhotosPo.data.has_value();
    isValid = isValid && targetPhotosPo.displayName.has_value();
    isValid = isValid && targetPhotosPo.subtype.has_value();
    isValid = isValid && targetPhotosPo.dateModified.has_value();
    isValid = isValid && !targetPhotosPo.data.value_or("").empty();
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_INVALID_ARGUMENTS,
        "Media_Operation: MovePhoto failed, sourcePhotosPo is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.displayName = sourcePhotosPo.displayName.value_or("");
    sourcePhotoInfo.filePath = sourcePhotosPo.data.value_or("");
    sourcePhotoInfo.subtype = sourcePhotosPo.subtype.value_or(0);
    int32_t originalSubtype = sourcePhotosPo.originalSubtype.value_or(0);
    int32_t effectMode = sourcePhotosPo.movingPhotoEffectMode.value_or(0);
    sourcePhotoInfo.isMovingPhoto =
        MovingPhotoFileUtils::IsMovingPhoto(sourcePhotoInfo.subtype, effectMode, originalSubtype);
    sourcePhotoInfo.dateModified = sourcePhotosPo.dateModified.value_or(0);
    sourcePhotoInfo.videoFilePath = this->FindVideoFilePath(sourcePhotoInfo);
    sourcePhotoInfo.editDataFolder = this->FindEditDataFolder(sourcePhotoInfo);
    // Build the Target Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.displayName = targetPhotosPo.displayName.value_or("");
    targetPhotoInfo.filePath = targetPhotosPo.data.value_or("");
    targetPhotoInfo.subtype = targetPhotosPo.subtype.value_or(0);
    targetPhotoInfo.dateModified = targetPhotosPo.dateModified.value_or(0);
    // No need to copy video file if the Original Photo is not a moving photo.
    if (!sourcePhotoInfo.videoFilePath.empty()) {
        targetPhotoInfo.videoFilePath = this->GetVideoFilePath(targetPhotoInfo);
    }
    // No need to copy edit data folder if the Original Photo is not edited.
    if (!sourcePhotoInfo.editDataFolder.empty()) {
        targetPhotoInfo.editDataFolder = this->BuildEditDataFolder(targetPhotoInfo);
    }
    MEDIA_INFO_LOG("Media_Operation: sourcePhotoInfo: %{public}s, targetPhotoInfo: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    return this->MovePhoto(sourcePhotoInfo, targetPhotoInfo);
}

/**
 * @brief Move Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::MovePhoto(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    int32_t opRet = this->MovePhotoFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);
    opRet = this->MovePhotoRelatedVideoFile(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);
    opRet = this->MovePhotoRelatedExtraData(sourcePhotoInfo, targetPhotoInfo);
    CHECK_AND_RETURN_RET(opRet == E_OK, opRet);
    return opRet;
}

/**
 * @brief Move Photo File, only include the photo file defined in the Photos table.
 * @return E_OK if success,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::MovePhotoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    std::string srcPath = sourcePhotoInfo.filePath;
    std::string targetPath = targetPhotoInfo.filePath;
    int64_t dateModified = targetPhotoInfo.dateModified;
    // If File Path is empty, return E_INVALID_PATH.
    CHECK_AND_RETURN_RET_LOG(!srcPath.empty() && !targetPath.empty(),
        E_INVALID_PATH,
        "Media_Operation: MovePhotoFile failed, srcPath or targetPath is empty. "
        "Source Object: %{public}s, Target Object: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    int32_t opRet = this->MoveFile(srcPath, targetPath);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK,
        opRet,
        "Media_Operation: MovePhotoFile failed, srcPath: %{public}s, targetPath: %{public}s",
        srcPath.c_str(),
        targetPath.c_str());
    MediaFileUtils::ModifyFile(targetPath, dateModified / MSEC_TO_SEC);
    MEDIA_INFO_LOG("Media_Operation: MovePhotoFile success, srcPath: %{public}s, targetPath: %{public}s",
        srcPath.c_str(),
        targetPath.c_str());
    return E_OK;
}

/**
 * @brief Move Photo File, only include the vide file related to the photo file defined in the Photos table.
 * @return E_OK if success or not MOVING_PHOTO or video file empty,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::MovePhotoRelatedVideoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    // If photoSubtype is MOVING_PHOTO, copy video file.
    CHECK_AND_RETURN_RET(sourcePhotoInfo.isMovingPhoto, E_OK);
    std::string srcVideoPath = sourcePhotoInfo.videoFilePath;
    std::string targetVideoPath = targetPhotoInfo.videoFilePath;
    int64_t dateModified = targetPhotoInfo.dateModified;
    // If video file is empty, return E_OK. Trace log will be printed in FindVideoFilePath.
    bool cond = (srcVideoPath.empty() || targetVideoPath.empty());
    CHECK_AND_RETURN_RET(!cond, E_OK);

    int32_t opRet = this->MoveFile(srcVideoPath, targetVideoPath);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK,
        opRet,
        "Media_Operation: MovePhoto Video failed, srcPath: %{public}s, targetPath: %{public}s",
        srcVideoPath.c_str(),
        targetVideoPath.c_str());
    MediaFileUtils::ModifyFile(targetVideoPath, dateModified / MSEC_TO_SEC);
    MEDIA_INFO_LOG("Media_Operation: MovePhotoRelatedVideoFile success, srcPath: %{public}s, targetPath: %{public}s",
        srcVideoPath.c_str(),
        targetVideoPath.c_str());
    return E_OK;
}

int32_t PhotoFileOperation::MovePhotoRelatedData(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo, const std::string &srcFolder,
    const std::string &targetFolder)
{
    bool cond = (srcFolder.empty() || targetFolder.empty());
    CHECK_AND_RETURN_RET(!cond, E_OK);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(srcFolder),
        E_NO_SUCH_FILE,
        "Media_Operation: %{public}s doesn't exist. %{public}s",
        srcFolder.c_str(),
        this->ToString(sourcePhotoInfo).c_str());
    int32_t opRet = MediaFileUtils::MoveDirectory(srcFolder, targetFolder);
    this->AuditLog(srcFolder, TAG_MOVE_OUT, opRet);
    this->AuditLog(targetFolder, TAG_MOVE_IN, opRet);
    CHECK_AND_RETURN_RET_LOG(opRet == E_OK,
        opRet,
        "Media_Operation: MovePhoto extraData failed, sourceInfo: %{public}s, targetInfo: %{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    MEDIA_INFO_LOG("Media_Operation: MovePhotoRelatedData success, sourceInfo:%{public}s, targetInfo:%{public}s",
        this->ToString(sourcePhotoInfo).c_str(),
        this->ToString(targetPhotoInfo).c_str());
    return E_OK;
}

/**
 * @brief Move the Edit Data.
 * @return E_OK if success or not edited photo.
 *         E_NO_SUCH_FILE if the source Edit Data Folder not exits.
 *         E_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::MovePhotoRelatedExtraData(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo,
    const PhotoFileOperation::PhotoAssetInfo &targetPhotoInfo)
{
    std::string srcEditDataFolder = sourcePhotoInfo.editDataFolder;
    std::string targetEditDataFolder = targetPhotoInfo.editDataFolder;
    return MovePhotoRelatedData(sourcePhotoInfo, targetPhotoInfo, srcEditDataFolder, targetEditDataFolder);
}

int32_t PhotoFileOperation::DeletePhoto(const PhotosPo &sourcePhotosPo)
{
    bool isValid = sourcePhotosPo.data.has_value();
    isValid = isValid && sourcePhotosPo.displayName.has_value();
    isValid = isValid && sourcePhotosPo.subtype.has_value();
    isValid = isValid && sourcePhotosPo.originalSubtype.has_value();
    isValid = isValid && sourcePhotosPo.movingPhotoEffectMode.has_value();
    isValid = isValid && sourcePhotosPo.dateModified.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_INVALID_ARGUMENTS,
        "Media_Operation: DeletePhoto failed, sourcePhotosPo is null or targetPath is empty");

    // Build the Original Photo Asset Info
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.displayName = sourcePhotosPo.displayName.value_or("");
    sourcePhotoInfo.filePath = sourcePhotosPo.data.value_or("");
    sourcePhotoInfo.subtype = sourcePhotosPo.subtype.value_or(0);
    int32_t originalSubtype = sourcePhotosPo.originalSubtype.value_or(0);
    int32_t effectMode = sourcePhotosPo.movingPhotoEffectMode.value_or(0);
    sourcePhotoInfo.isMovingPhoto =
        MovingPhotoFileUtils::IsMovingPhoto(sourcePhotoInfo.subtype, effectMode, originalSubtype);
    sourcePhotoInfo.dateModified = sourcePhotosPo.dateModified.value_or(0);
    sourcePhotoInfo.videoFilePath = this->FindVideoFilePath(sourcePhotoInfo);
    sourcePhotoInfo.editDataFolder = this->FindEditDataFolder(sourcePhotoInfo);
    MEDIA_INFO_LOG("Media_Operation: sourcePhotoInfo: %{public}s", this->ToString(sourcePhotoInfo).c_str());
    return this->DeletePhoto(sourcePhotoInfo);
}

/**
 * @brief Delete Photo File, include photo file, video file and edit data folder.
 */
int32_t PhotoFileOperation::DeletePhoto(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo)
{
    int32_t opRet = this->DeletePhotoFile(sourcePhotoInfo);
    opRet = this->DeletePhotoRelatedVideoFile(sourcePhotoInfo);
    opRet = this->DeletePhotoRelatedExtraData(sourcePhotoInfo);
    return opRet;
}

/**
 * @brief Delete Photo File, only include the photo file defined in the Photos table.
 * @return E_OK if success,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::DeletePhotoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo)
{
    std::string srcPath = sourcePhotoInfo.filePath;
    // If File Path is empty, return E_INVALID_PATH.
    CHECK_AND_RETURN_RET_LOG(!srcPath.empty(),
        E_INVALID_PATH,
        "Media_Operation: DeletePhotoFile failed, srcPath is empty. Source Object: %{public}s",
        this->ToString(sourcePhotoInfo).c_str());
    bool opRet = MediaFileUtils::DeleteFile(srcPath);
    this->AuditLog(srcPath, TAG_DELETE, opRet ? E_OK : E_ERR);
    MEDIA_INFO_LOG(
        "Media_Operation: DeletePhotoFile success, opRet: %{public}d, srcPath: %{public}s", opRet, srcPath.c_str());
    return E_OK;
}

/**
 * @brief Move Photo File, only include the vide file related to the photo file defined in the Photos table.
 * @return E_OK if success or not MOVING_PHOTO or video file empty,
 *         E_INVALID_PATH if the source file or target file is invalid,
 *         E_FILE_OPER_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::DeletePhotoRelatedVideoFile(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo)
{
    // If photoSubtype is MOVING_PHOTO, copy video file.
    CHECK_AND_RETURN_RET(sourcePhotoInfo.isMovingPhoto, E_OK);
    std::string srcVideoPath = sourcePhotoInfo.videoFilePath;
    // If video file is empty, return E_OK. Trace log will be printed in FindVideoFilePath.
    bool cond = srcVideoPath.empty();
    CHECK_AND_RETURN_RET(!cond, E_OK);
    bool opRet = MediaFileUtils::DeleteFile(srcVideoPath);
    this->AuditLog(srcVideoPath, TAG_DELETE, opRet ? E_OK : E_ERR);
    MEDIA_INFO_LOG("Media_Operation: MovePhotoRelatedVideoFile success, opRet: %{public}d, srcPath: %{public}s",
        opRet,
        srcVideoPath.c_str());
    return E_OK;
}

int32_t PhotoFileOperation::DeletePhotoRelatedData(
    const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo, const std::string &srcFolder)
{
    bool isValid = !srcFolder.empty() && MediaFileUtils::IsFileExists(srcFolder);
    CHECK_AND_RETURN_RET(isValid, E_OK);
    bool opRet = MediaFileUtils::DeleteDir(srcFolder);
    this->AuditLog(srcFolder, TAG_DELETE, opRet ? E_OK : E_ERR);
    MEDIA_INFO_LOG("Media_Operation: DeletePhotoRelatedData success, opRet: %{public}d, sourceInfo: %{public}s",
        opRet,
        this->ToString(sourcePhotoInfo).c_str());
    return E_OK;
}

/**
 * @brief Delete the Edit Data.
 * @return E_OK if success or not edited photo.
 *         E_NO_SUCH_FILE if the source Edit Data Folder not exits.
 *         E_FAIL if the copy operation failed.
 */
int32_t PhotoFileOperation::DeletePhotoRelatedExtraData(const PhotoFileOperation::PhotoAssetInfo &sourcePhotoInfo)
{
    return DeletePhotoRelatedData(sourcePhotoInfo, sourcePhotoInfo.editDataFolder);
}

/**
 * @brief Delete thumbnail, include folder and astc data.
 */
int32_t PhotoFileOperation::DeleteThumbnail(const PhotosPo &photoInfo)
{
    bool isValid = photoInfo.data.has_value();
    isValid = isValid && photoInfo.dateTaken.has_value();
    isValid = isValid && photoInfo.fileId.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_ARGUMENTS, "DeleteThumbnail failed, photoInfo is invalid");
    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.filePath = photoInfo.data.value_or("");
    sourcePhotoInfo.thumbnailFolder = this->FindThumbnailFolder(sourcePhotoInfo);
    CHECK_AND_RETURN_RET_INFO_LOG(!sourcePhotoInfo.thumbnailFolder.empty(), E_OK, "No need to delete thumbnail folder");
    // Use ThumbnailService to delete thumbnail.
    auto thumbnailService = ThumbnailService::GetInstance();
    CHECK_AND_RETURN_RET_LOG(thumbnailService != nullptr, E_OK, "DeleteThumbnail failed, thumbnailService is null");
    bool ret = thumbnailService->DeleteThumbnailDirAndAstc(std::to_string(photoInfo.fileId.value_or(0)),
        PhotoColumn::PHOTOS_TABLE,
        photoInfo.data.value_or(""),
        std::to_string(photoInfo.dateTaken.value_or(0)));
    this->AuditLog(sourcePhotoInfo.thumbnailFolder, TAG_DELETE, ret ? E_OK : E_ERR);
    return ret ? E_OK : E_ERR;
}

void PhotoFileOperation::AuditLog(const std::string &path, const std::string &action, const int32_t result)
{
    std::string resultMsg = (result == E_OK) ? "SUCCESS" : "FAILED";
    std::string logInfo =
        std::to_string(MediaFileUtils::UTCTimeMilliSeconds()) + " # " + action + " # " + resultMsg + " # " + path;
    this->opStats_.emplace_back(logInfo);
}

std::string PhotoFileOperation::GetAuditLog() const
{
    std::stringstream ss;
    for (const auto &item : this->opStats_) {
        ss << item << ", ";
    }
    return ss.str();
}

/**
 * @brief Copy thumbnail in "/storage/media/local/files/.thumbs/".
 */
int32_t PhotoFileOperation::CopyLocalThumbnail(const PhotosPo &sourcePhotosPo, const PhotosPo &targetPhotosPo)
{
    bool isValid = sourcePhotosPo.data.has_value();
    isValid = isValid && targetPhotosPo.data.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_ARGUMENTS, "Invalid arguments");

    PhotoFileOperation::PhotoAssetInfo sourcePhotoInfo;
    sourcePhotoInfo.filePath = sourcePhotosPo.data.value_or("");
    sourcePhotoInfo.filePath = CloudSync::CloudMediaSyncUtils::GetLocalPath(sourcePhotoInfo.filePath);
    sourcePhotoInfo.thumbnailFolder = this->FindThumbnailFolder(sourcePhotoInfo);
    isValid = !sourcePhotoInfo.thumbnailFolder.empty() && !MediaFileUtils::IsDirEmpty(sourcePhotoInfo.thumbnailFolder);
    CHECK_AND_RETURN_RET_LOG(isValid,
        E_FAIL,
        "Source thumbnail is empty, skip copy. filePath:%{public}s", sourcePhotoInfo.filePath.c_str());

    PhotoFileOperation::PhotoAssetInfo targetPhotoInfo;
    targetPhotoInfo.filePath = targetPhotosPo.data.value_or("");
    targetPhotoInfo.filePath = CloudSync::CloudMediaSyncUtils::GetLocalPath(targetPhotoInfo.filePath);
    targetPhotoInfo.thumbnailFolder = this->BuildThumbnailFolder(targetPhotoInfo);

    return this->CopyPhotoRelatedThumbnail(sourcePhotoInfo, targetPhotoInfo);
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media