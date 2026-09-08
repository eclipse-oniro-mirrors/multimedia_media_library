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

#define MLOG_TAG "Media_Service"

#include "cloud_media_file_manager_service.h"

#include "media_log.h"
#include "cloud_media_sync_utils.h"
#include "medialibrary_db_const.h"
#include "media_string_utils.h"
#include "cloud_media_sync_const.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"

namespace OHOS::Media::CloudSync {
void CloudMediaFileManagerService::FixFileInfo(CloudMediaPullDataDto &pullData)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloudMediaFileManagerService::FixFileInfo");

    this->fileInfoService_.FixFileInfo(pullData);
}

/**
 * 检查是否需要移动文件，满足以下条件才移动文件：
 * 1. 关联到了本地资产（HasLocalAsset返回true）
 * 2. 本地不是纯云资产（IsLocalFile返回true）
 * 3. 本地和云端不是都是媒体资产（IsNotBothMedia返回true）
 * 4. 文件路径发生变化（IsPathChanged返回true）
 * 5. 目标路径如果是媒体桶路径，则目标路径不存在文件（IsTargetMediaFileNotExists返回true）
 * @return true 需要移动文件；false 不需要移动文件
 */
bool CloudMediaFileManagerService::Accept(const CloudMediaPullDataDto &pullData)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloudMediaFileManagerService::Accept");

    for (const auto &validator : validators_) {
        if (!(this->*validator)(pullData)) {
            return false;
        }
    }
    MEDIA_INFO_LOG("Relocate Accept, cloud: %{public}s", pullData.cloudId.c_str());
    return true;
}

/**
 * 关联不到本地资产，无需移动文件
 * 场景：云端是媒体资产，设备上没有关联的本地资产，此时无需移动文件
 * @return true 关联到了本地资产，需要移动文件；false 没有关联到本地资产，无需移动文件
 */
bool CloudMediaFileManagerService::HasLocalAsset(const CloudMediaPullDataDto &pullData) const
{
    const bool hasLocalAsset = pullData.localPhotosPoOp.has_value();
    MEDIA_DEBUG_LOG("HasLocalAsset checked, cloudId: %{public}s, hasLocalAsset: %{public}d",
                    pullData.cloudId.c_str(),
                    hasLocalAsset);
    return hasLocalAsset;
}

/**
 * 本地是纯云资产，无需移动文件
 * 场景：本地是纯云资产，设备没有文件，此时无需移动文件
 * @return true 本地不是纯云资产，需要移动文件；false 本地是纯云资产，无需移动文件
 */
bool CloudMediaFileManagerService::IsLocalFile(const CloudMediaPullDataDto &pullData) const
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const bool isLocal = CloudMediaSyncUtils::FileIsLocal(photoInfo.position.value_or(0));
    MEDIA_DEBUG_LOG(
        "IsLocalFile checked, cloudId: %{public}s, isLocal: %{public}d", pullData.cloudId.c_str(), isLocal);
    return isLocal;
}

/**
 * 本地和云端都是媒体资产，无需移动文件
 * 场景：本地和云端都是媒体资产（fileSourceType都是MEDIA），文件路径相同，此时无需再次移动文件
 * @return true 本地和云端不是都是媒体资产，需要移动文件；false 本地和云端都是媒体资产，无需移动文件
 */
bool CloudMediaFileManagerService::IsNotBothMedia(const CloudMediaPullDataDto &pullData) const
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const int32_t deviceFileSourceType = photoInfo.fileSourceType.value_or(0);
    const int32_t cloudFileSourceType = pullData.attributesFileSourceType;
    const bool isBothMedia = deviceFileSourceType == static_cast<int32_t>(FileSourceType::MEDIA) &&
                             cloudFileSourceType == static_cast<int32_t>(FileSourceType::MEDIA);
    MEDIA_DEBUG_LOG(
        "IsNotBothMedia checked, cloudId: %{public}s, isBothMedia: %{public}d", pullData.cloudId.c_str(), isBothMedia);
    return !isBothMedia;
}

/**
 * 文件路径未发生变化，无需移动文件
 * @return true 文件路径发生变化，需要移动文件；false 文件路径未发生变化，无需移动文件
 */
bool CloudMediaFileManagerService::IsPathChanged(const CloudMediaPullDataDto &pullData) const
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const std::string deviceFileStoragePath = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    const std::string cloudFileStoragePath = CloudMediaSyncUtils::FindFileStoragePathWithPullData(pullData);
    const bool isPathChanged = deviceFileStoragePath != cloudFileStoragePath;
    MEDIA_DEBUG_LOG("IsPathChanged checked, cloudId: %{public}s, isPathChanged: %{public}d",
                    pullData.cloudId.c_str(),
                    isPathChanged);
    return isPathChanged;
}

/**
 * 场景：目标路径是媒体桶路径（/storage/media/local/files/Photo），并且已存在文件
 * 处理：不移动文件，保持原路径不变，避免覆盖目标路径的文件
 * 原因：媒体桶路径是文件名称唯一，存在文件并移动文件，会导致新文件重命名，且无法删除重命名的文件，造成存储空间浪费和用户困扰
 * @return true 目标路径不存在，可以移动文件；false 目标路径已存在，不需要移动文件
 */
bool CloudMediaFileManagerService::IsTargetMediaFileNotExists(const CloudMediaPullDataDto &pullData) const
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);

    const std::string cloudFileStoragePath = CloudMediaSyncUtils::FindFileStoragePathWithPullData(pullData);
    const bool isMediaFile = CloudMediaSyncUtils::IsMediaFile(cloudFileStoragePath);
    const bool isTargetMediaExists = isMediaFile && MediaFileUtils::IsFileExists(cloudFileStoragePath);
    MEDIA_DEBUG_LOG("IsTargetMediaFileNotExists checked, "
                    "cloudId: %{public}s, isMediaFile: %{public}d, isTargetMediaExists: %{public}d",
                    pullData.cloudId.c_str(),
                    isMediaFile,
                    isTargetMediaExists);
    return !isTargetMediaExists;
}

/**
 * 场景：设备资产，已存在文件
 * @return true 设备资产存在文件，可以移动文件；false 设备资产不存在文件，不需要移动文件
 */
bool CloudMediaFileManagerService::IsSrcFileExists(const CloudMediaPullDataDto &pullData) const
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const std::string deviceFileStoragePath = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
    const bool isSrcFileExists = MediaFileUtils::IsFileExists(deviceFileStoragePath);
    MEDIA_DEBUG_LOG("IsSrcFileExists checked, cloudId: %{public}s, isSrcFileExists: %{public}d",
                    pullData.cloudId.c_str(),
                    isSrcFileExists);
    CHECK_AND_PRINT_LOG(
        isSrcFileExists,
        "Source file does not exist, cannot relocate file, cloudId: %{public}s, deviceFileStoragePath: %{public}s",
        pullData.cloudId.c_str(),
        MediaFileUtils::DesensitizePath(deviceFileStoragePath).c_str());
    return isSrcFileExists;
}

int32_t CloudMediaFileManagerService::FindUniqueFilePathAndUpdateDB(
    const std::string &destPath, const int32_t fileId, std::string &finalDestPath)
{
    int32_t ret = CloudMediaSyncUtils::FindUniqueFilePath(destPath, finalDestPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK,
                             ret,
                             "FindUniqueFilePath failed, ret: %{public}d, fileId: %{public}d, destPath: %{public}s",
                             ret,
                             fileId,
                             MediaFileUtils::DesensitizePath(destPath).c_str());
    if (finalDestPath != destPath) {
        ret = this->fileManagerDao_.UpdateStoragePath(fileId, finalDestPath);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK,
                                 ret,
                                 "UpdateStoragePath failed, ret: %{public}d, fileId: %{public}d, destPath: "
                                 "%{public}s, finalDestPath: %{public}s",
                                 ret,
                                 fileId,
                                 MediaFileUtils::DesensitizePath(destPath).c_str(),
                                 MediaFileUtils::DesensitizePath(finalDestPath).c_str());
    }
    return E_OK;
}

int32_t CloudMediaFileManagerService::RelocateFileInner(const PhotosPo &photoInfo, std::string &destPath)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloudMediaFileManagerService::RelocateFileInner");

    const int32_t fileId = photoInfo.fileId.value_or(0);
    const std::string deviceFileStoragePath = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);

    // 参数有效性检查
    bool isValid = !deviceFileStoragePath.empty() && !destPath.empty();
    CHECK_AND_RETURN_RET(isValid, E_INVALID_ARGUMENTS);

    isValid = deviceFileStoragePath != destPath;
    isValid = isValid && !(CloudMediaSyncUtils::IsMediaFile(destPath) &&
                            CloudMediaSyncUtils::IsMediaFile(deviceFileStoragePath));
    CHECK_AND_RETURN_RET(isValid, E_OK);

    std::string finalDestPath;

    // 先找目标唯一路径 -> 更新数据库中的路径 -> 再移动文件
    int32_t ret = this->FindUniqueFilePathAndUpdateDB(destPath, fileId, finalDestPath);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    destPath = finalDestPath;

    const bool isLivePhoto = CloudMediaSyncUtils::IsLivePhotoWithMetaData(photoInfo);
    const bool isMediaFile = CloudMediaSyncUtils::IsMediaFile(destPath) ||
                             CloudMediaSyncUtils::IsMediaFile(deviceFileStoragePath);
    if (isLivePhoto && isMediaFile) {  // 跨媒体库桶路径处理动图文件
        ret = CloudMediaSyncUtils::MoveLivePhoto(deviceFileStoragePath, destPath, finalDestPath);
        ret = (finalDestPath == destPath) ? ret : E_FILE_EXIST;
    } else {  // 如果是普通图片，则直接移动文件；
        ret = CloudMediaSyncUtils::MoveFileWithConflictResolution(deviceFileStoragePath, destPath, finalDestPath);
        ret = (finalDestPath == destPath) ? ret : E_FILE_EXIST;
    }
    MEDIA_INFO_LOG("RelocateFileInner completed, ret: %{public}d, fileId: %{public}d, "
                   "isLivePhoto: %{public}d, isMediaFile: %{public}d, "
                   "deviceFileStoragePath: %{public}s, destPath: %{public}s, finalDestPath: %{public}s",
                   ret,
                   fileId,
                   isLivePhoto,
                   isMediaFile,
                   MediaFileUtils::DesensitizePath(deviceFileStoragePath).c_str(),
                   MediaFileUtils::DesensitizePath(destPath).c_str(),
                   MediaFileUtils::DesensitizePath(finalDestPath).c_str());
    return ret;
}

int32_t CloudMediaFileManagerService::RelocateFileInner(const CloudMediaPullDataDto &pullData)
{
    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), false);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    std::string cloudFileStoragePath = CloudMediaSyncUtils::FindFileStoragePathWithPullData(pullData);
    int32_t ret = this->RelocateFileInner(photoInfo, cloudFileStoragePath);
    MEDIA_INFO_LOG("RelocateFileInner with pullData completed, "
                   "ret: %{public}d, cloudId: %{public}s, cloudFileStoragePath: %{public}s",
                   ret,
                   pullData.cloudId.c_str(),
                   MediaFileUtils::DesensitizePath(cloudFileStoragePath).c_str());
    return ret;
}

int32_t CloudMediaFileManagerService::ResetPositionToCloudOnly(const CloudMediaPullDataDto &pullData)
{
    // pullData.attributesFileSourceType直接入库，可以使用该值表示当前资产的fileSourceType，避免再次查询数据库获取fileSourceType
    int32_t fileSourceType = pullData.attributesFileSourceType;
    // 特殊：纯云湖内资产，fileSourceType=MEDIA(0).
    fileSourceType = (fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE))
                         ? static_cast<int32_t>(FileSourceType::MEDIA)
                         : fileSourceType;
    return this->dataDao_.UpdatePosWithType(
        {pullData.cloudId}, static_cast<int32_t>(PhotoPositionType::CLOUD), fileSourceType);
}

int32_t CloudMediaFileManagerService::RelocateFile(const CloudMediaPullDataDto &pullData)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloudMediaFileManagerService::RelocateFile");

    // 关联不到本地资产，无需移动文件
    CHECK_AND_RETURN_RET(pullData.localPhotosPoOp.has_value(), E_OK);
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    // 不满足移动文件的条件，无需移动文件
    CHECK_AND_RETURN_RET(this->Accept(pullData), E_OK);

    // 满足移动文件的条件，执行移动文件逻辑
    const int32_t relocateRet = this->RelocateFileInner(pullData);

    // 如果文件移动失败，将资产重置为纯云资产，避免文件路径不一致导致的各种问题
    int32_t resetRet = E_OK;
    if (relocateRet != E_OK) {
        resetRet = this->ResetPositionToCloudOnly(pullData);
    }

    MEDIA_INFO_LOG("RelocateFile completed, cloudId: %{public}s, relocateRet: %{public}d, resetRet: %{public}d.",
                   pullData.cloudId.c_str(),
                   relocateRet,
                   resetRet);
    return relocateRet;
}
}  // namespace OHOS::Media::CloudSync