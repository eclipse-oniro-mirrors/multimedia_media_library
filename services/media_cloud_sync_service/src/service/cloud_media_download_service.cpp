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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_download_service.h"

#include "background_cloud_batch_selected_file_processor.h"
#include "cover_position_parser.h"
#include "directory_ex.h"
#include "parameters.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_attachment_utils.h"
#include "cloud_media_sync_utils.h"
#include "cloud_sync_notify_handler.h"
#include "thumbnail_const.h"
#include "thumbnail_service.h"
#include "cloud_media_asset_manager.h"
#include "cloud_media_asset_types.h"
#include "cloud_media_dfx_service.h"
#include "cloud_media_scan_service.h"
#include "dfx_const.h"
#include "exif_rotate_utils.h"
#include "media_gallery_sync_notify.h"
#include "dfx_utils.h"
#include "photo_video_mode_operation.h"
#include "metadata_extractor.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"

namespace OHOS::Media::CloudSync {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
int32_t CloudMediaDownloadService::GetDownloadThmNum(const int32_t type, int32_t &totalNum)
{
    return this->dao_.GetDownloadThmNum(type, totalNum);
}

int32_t CloudMediaDownloadService::GetDownloadThms(
    const DownloadThumbnailQueryDto &queryDto, std::vector<PhotosDto> &photosDtos)
{
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dao_.GetDownloadThms(queryDto, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetDownloadThms failed, ret:%{public}d", ret);
    photosDtos = this->processor_.GetPhotosDto(photosPos);
    return ret;
}

std::vector<PhotosDto> CloudMediaDownloadService::GetDownloadThmsByUri(
    const std::vector<int32_t> &fileIds, const int32_t type)
{
    MEDIA_INFO_LOG("enter CloudMediaDownloadService::GetDownloadThmsByUri type:%{public}d", type);
    std::vector<PhotosDto> photosDtoVec;
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dao_.GetDownloadAsset(fileIds, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, photosDtoVec, "GetDownloadAsset failed, ret:%{public}d", ret);
    std::vector<DownloadAssetData> downloadAssetDataVec;
    this->processor_.GetDownloadAssetData(photosPos, downloadAssetDataVec);
    MEDIA_INFO_LOG("GetDownloadThmsByUri size of downloadAssetDataVec: %{public}zu", downloadAssetDataVec.size());
    CHECK_AND_RETURN_RET_LOG(
        !downloadAssetDataVec.empty(), photosDtoVec, "GetDownloadThmsByUri Failed to get downloadAssetDataVec.");
    for (const auto &downloadAssetData : downloadAssetDataVec) {
        std::string filePath;
        std::string fileName;
        CHECK_AND_RETURN_RET_LOG(
            CloudMediaFileUtils::GetParentPathAndFilename(downloadAssetData.path, filePath, fileName),
            photosDtoVec,
            "GetDownloadThmsByUri failed to GetParentPathAndFilename");
        PhotosDto photosDto;
        photosDto.fileId = downloadAssetData.fileId;
        photosDto.cloudId = downloadAssetData.cloudId;
        photosDto.data = filePath;
        photosDto.mediaType = downloadAssetData.mediaType;
        photosDto.size = downloadAssetData.fileSize;
        photosDto.path = downloadAssetData.path;
        photosDto.modifiedTime = downloadAssetData.editTime;
        photosDto.fileName = fileName;
        photosDto.originalCloudId = downloadAssetData.originalCloudId;
        int32_t retThm = E_OK;
        int32_t retLcd = E_OK;
        if (static_cast<uint32_t>(type) & TYPE_THM_MASK) {
            retThm = CloudMediaAttachmentUtils::GetThumbnail("thumbnail", downloadAssetData, photosDto);
            CHECK_AND_PRINT_LOG(retThm == E_OK, "GetDownloadThmsByUri GetAttachment thm fail");
        }
        if (static_cast<uint32_t>(type) & TYPE_LCD_MASK) {
            retLcd = CloudMediaAttachmentUtils::GetLcdThumbnail("lcd", downloadAssetData, photosDto);
            CHECK_AND_PRINT_LOG(retLcd == E_OK, "GetDownloadThmsByUri GetAttachment lcd fail");
        }
        if ((retThm != E_OK) && (retLcd != E_OK)) {
            continue;
        }
        MEDIA_DEBUG_LOG("GetDownloadThmsByUri Photo: %{public}s", photosDto.ToString().c_str());
        photosDtoVec.push_back(photosDto);
    }
    return photosDtoVec;
}

int32_t CloudMediaDownloadService::OnDownloadThm(
    const std::vector<std::string> &thmVector, std::vector<MediaOperateResultDto> &result)
{
    CHECK_AND_RETURN_RET_LOG(!thmVector.empty(), E_OK, "thmVector is empty");
    MEDIA_INFO_LOG("size of thmVector is %{public}zu", thmVector.size());
    int32_t ret = this->dao_.UpdateDownloadThm(thmVector);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to UpdateDownloadThms.");
    if (ret == E_OK && !thmVector.empty()) {
        CloudMediaDfxService::UpdateAttachmentStat(INDEX_THUMB_SUCCESS, thmVector.size());
    }
    /* 通知
     DataSyncNotifier::GetInstance().TryNotify(PHOTO_URI_PREFIX, ChangeType::INSERT, "");
     DataSyncNotifier::GetInstance().FinalNotify();
    */
    MediaGallerySyncNotify::GetInstance().TryNotify(PhotoColumn::PHOTO_CLOUD_URI_PREFIX, ChangeType::INSERT, "");
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    for (auto &thm : thmVector) {  // collect results
        MediaOperateResultDto mediaResult;
        mediaResult.cloudId = thm;
        mediaResult.errorCode = ret;
        result.emplace_back(mediaResult);
    }
    return ret;
}

int32_t CloudMediaDownloadService::OnDownloadLcd(
    const std::vector<std::string> &lcdVector, std::vector<MediaOperateResultDto> &result)
{
    CHECK_AND_RETURN_RET_LOG(!lcdVector.empty(), E_OK, "lcdVector is empty");
    MEDIA_INFO_LOG("size of lcdVector is %{public}zu", lcdVector.size());
    int32_t ret = this->dao_.UpdateDownloadLcd(lcdVector);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to UpdateDownloadLcd.");
    if (ret == E_OK && !lcdVector.empty()) {
        CloudMediaDfxService::UpdateAttachmentStat(INDEX_LCD_SUCCESS, lcdVector.size());
    }
    for (auto &thm : lcdVector) {  // collect results
        MediaOperateResultDto mediaResult;
        mediaResult.cloudId = thm;
        mediaResult.errorCode = ret;
        result.emplace_back(mediaResult);
    }
    return ret;
}

int32_t CloudMediaDownloadService::OnDownloadThmAndLcd(
    const std::vector<std::string> &bothVector, std::vector<MediaOperateResultDto> &result)
{
    CHECK_AND_RETURN_RET_LOG(!bothVector.empty(), E_OK, "bothVector is empty");
    MEDIA_INFO_LOG("size of bothVector is %{public}zu", bothVector.size());
    int32_t ret = this->dao_.UpdateDownloadThmAndLcd(bothVector);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to UpdateDownloadThmAndLcd.");
    if (ret == E_OK && !bothVector.empty()) {
        CloudMediaDfxService::UpdateAttachmentStat(INDEX_THUMB_SUCCESS, bothVector.size());
        CloudMediaDfxService::UpdateAttachmentStat(INDEX_LCD_SUCCESS, bothVector.size());
    }
    for (auto &thm : bothVector) {  // collect results
        MediaOperateResultDto mediaResult;
        mediaResult.cloudId = thm;
        mediaResult.errorCode = ret;
        result.emplace_back(mediaResult);
    }
    return ret;
}

void CloudMediaDownloadService::NotifyDownloadLcd(const std::vector<std::string> &cloudIds)
{
    std::vector<std::string> fileIds;
    this->dao_.GetFileIdFromCloudId(cloudIds, fileIds);
    MEDIA_INFO_LOG("size of fileIds is %{public}zu", fileIds.size());
    for (auto &fileId : fileIds) {
        std::string uri = PhotoColumn::PHOTO_CLOUD_URI_PREFIX + fileId;
        MediaGallerySyncNotify::GetInstance().TryNotify(uri, ChangeType::INSERT, "");
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
}

bool CloudMediaDownloadService::IsCloudInsertTaskPriorityHigh()
{
    int32_t cloudSyncStatus = static_cast<int32_t>(system::GetParameter(CLOUDSYNC_STATUS_KEY, "0").at(0) - '0');
    MEDIA_INFO_LOG("cloudSyncStatus: %{public}d", cloudSyncStatus);
    return cloudSyncStatus == CloudSyncStatus::FIRST_FIVE_HUNDRED ||
           cloudSyncStatus == CloudSyncStatus::INCREMENT_DOWNLOAD;
}

int32_t CloudMediaDownloadService::OnDownloadThms(
    const std::unordered_map<std::string, int32_t> &downloadThumbnailMap, std::vector<MediaOperateResultDto> &result)
{
    MEDIA_INFO_LOG("enter CloudMediaDownloadService::OnDownloadThms");
    std::vector<std::string> thmVector;
    std::vector<std::string> lcdVector;
    std::vector<std::string> bothVector;
    std::vector<std::string> astcVector;
    MEDIA_INFO_LOG("size of downloadThumbnailMap is %{public}zu", downloadThumbnailMap.size());
    for (auto &pair : downloadThumbnailMap) {
        //(key,value) => key : cloudId, value : 001-> thm, 010 -> lcd, 011 -> thm and lcd, 100 -> astc(端云不会下载astc)
        if (pair.second == TYPE_THM) {
            thmVector.emplace_back(pair.first);
        } else if (pair.second == TYPE_LCD) {
            lcdVector.emplace_back(pair.first);
        } else if (pair.second == TYPE_THM_AND_LCD) {
            bothVector.emplace_back(pair.first);
        }
    }

    int32_t ret = E_ERR;
    ret = this->OnDownloadThm(thmVector, result);
    ret = this->OnDownloadLcd(lcdVector, result);
    if (ret == E_OK) {
        astcVector.insert(astcVector.end(), lcdVector.begin(), lcdVector.end());
    }
    ret = this->OnDownloadThmAndLcd(bothVector, result);
    if (ret == E_OK) {
        astcVector.insert(astcVector.end(), bothVector.begin(), bothVector.end());
    }
    MEDIA_INFO_LOG("size of astcVector is %{public}zu", astcVector.size());
    this->NotifyDownloadLcd(astcVector);
    return E_OK;
}

std::vector<PhotosDto> CloudMediaDownloadService::GetDownloadAsset(const std::vector<int32_t> &fileIds)
{
    MEDIA_INFO_LOG("enter CloudMediaDownloadService::GetDownloadAsset");
    std::vector<PhotosDto> photosDtoVec;
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dao_.GetDownloadAsset(fileIds, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, photosDtoVec, "GetDownloadAsset failed, ret:%{public}d", ret);
    std::vector<DownloadAssetData> downloadAssetDataVec;
    this->processor_.GetDownloadAssetData(photosPos, downloadAssetDataVec);
    MEDIA_INFO_LOG("GetDownloadAsset size of downloadAssetDataVec: %{public}zu", downloadAssetDataVec.size());
    CHECK_AND_RETURN_RET_LOG(
        !downloadAssetDataVec.empty(), photosDtoVec, "GetDownloadAsset Failed to get downloadAssetDataVec.");
    for (const auto &downloadAssetData : downloadAssetDataVec) {
        std::string filePath;
        std::string fileName;
        CHECK_AND_RETURN_RET_LOG(
            CloudMediaFileUtils::GetParentPathAndFilename(downloadAssetData.path, filePath, fileName),
            photosDtoVec,
            "GetDownloadAsset failed to GetParentPathAndFilename");
        PhotosDto photosDto;
        photosDto.fileId = downloadAssetData.fileId;
        photosDto.cloudId = downloadAssetData.cloudId;
        photosDto.data = filePath;
        photosDto.mediaType = downloadAssetData.mediaType;
        photosDto.size = downloadAssetData.fileSize;
        photosDto.path = downloadAssetData.path;
        photosDto.modifiedTime = downloadAssetData.editTime;
        photosDto.fileName = fileName;
        photosDto.originalCloudId = downloadAssetData.originalCloudId;
        photosDto.hidden = downloadAssetData.hidden;
        photosDto.dateTrashed = downloadAssetData.dateTrashed;
        photosDto.fileSourceType = downloadAssetData.fileSourceType;
        photosDto.storagePath = downloadAssetData.storagePath;

        CHECK_AND_RETURN_RET_LOG(
            CloudMediaAttachmentUtils::GetAttachment("content", downloadAssetData, photosDto) == E_OK,
            photosDtoVec,
            "failed to GetAttachment");
        MEDIA_DEBUG_LOG("GetDownloadAsset Photo: %{public}s", photosDto.ToString().c_str());

        photosDtoVec.push_back(photosDto);
    }
    return photosDtoVec;
}

OnDownloadAssetData CloudMediaDownloadService::GetOnDownloadAssetData(PhotosPo &photosPo)
{
    OnDownloadAssetData assetData;
    assetData.err = E_OK;
    assetData.errorMsg = "";
    bool isMovingPhoto = CloudMediaSyncUtils::IsMovingPhoto(photosPo);
    bool isGraffiti = CloudMediaSyncUtils::IsGraffiti(photosPo);
    bool isLivePhoto = CloudMediaSyncUtils::IsLivePhoto(photosPo);
    bool isInvalidCover = photosPo.coverPosition.value_or(0) == 0 && photosPo.isRectificationCover.value_or(0) == 0;
    MEDIA_INFO_LOG("GetOnDownloadAssetData %{public}d,%{public}d,%{public}d", isMovingPhoto, isGraffiti, isLivePhoto);
    assetData.fixFileType = isMovingPhoto && !isGraffiti && !isLivePhoto;
    assetData.needSliceContent = (isMovingPhoto && !isGraffiti) && isLivePhoto;
    assetData.needParseCover = isMovingPhoto && isInvalidCover;
    assetData.needSliceRaw = isMovingPhoto;
    assetData.path = photosPo.data.value_or("");
    assetData.localPath = CloudMediaSyncUtils::GetLocalPath(assetData.path);
    assetData.dateModified = photosPo.dateModified.value_or(0);
    std::string extraUri = MediaFileUtils::GetExtraUri(photosPo.displayName.value_or(""), photosPo.data.value_or(""));
    assetData.fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
                                                               std::to_string(photosPo.fileId.value_or(0)), extraUri);
    assetData.needScanShootingMode = (photosPo.shootingModeTag.has_value() && photosPo.shootingModeTag->empty()) ||
        (photosPo.frontCamera.has_value() && photosPo.frontCamera->empty());
    assetData.needScanSubtype = (photosPo.subtype.value_or(0) == 0);
    assetData.mediaType = photosPo.mediaType.value_or(0);
    assetData.exifRotate = CloudMediaSyncUtils::GetExifRotate(assetData.mediaType, assetData.localPath);
    assetData.needScanHdrMode = photosPo.hdrMode.value_or(0) == 0 || photosPo.editTime.value_or(0) > 0 ||
        photosPo.movingPhotoEffectMode.value_or(0) > 0;
    return assetData;
}

OnDownloadAssetData CloudMediaDownloadService::GetOnDownloadLakeAssetData(PhotosPo &photosPo,
    const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos)
{
    OnDownloadAssetData assetData;
    assetData.err = E_OK;
    assetData.errorMsg = "";
    bool isMovingPhoto = CloudMediaSyncUtils::IsMovingPhoto(photosPo);
    bool isGraffiti = CloudMediaSyncUtils::IsGraffiti(photosPo);
    bool isLivePhoto = CloudMediaSyncUtils::IsLivePhoto(photosPo);
    bool isInvalidCover = photosPo.coverPosition.value_or(0) == 0 && photosPo.isRectificationCover.value_or(0) == 0;
    MEDIA_INFO_LOG("GetOnDownloadLakeAssetData %{public}d,%{public}d,%{public}d",
        isMovingPhoto, isGraffiti, isLivePhoto);
    assetData.fixFileType = isMovingPhoto && !isGraffiti && !isLivePhoto;
    assetData.needSliceContent = (isMovingPhoto && !isGraffiti) && isLivePhoto;
    assetData.needParseCover = isMovingPhoto && isInvalidCover;
    assetData.needSliceRaw = isMovingPhoto;
    assetData.path = photosPo.data.value_or("");
    assetData.localPath = CloudMediaSyncUtils::GetLocalPath(assetData.path);
    assetData.dateModified = photosPo.dateModified.value_or(0);
    std::string extraUri = MediaFileUtils::GetExtraUri(photosPo.displayName.value_or(""), photosPo.data.value_or(""));
    assetData.fileUri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
                                                               std::to_string(photosPo.fileId.value_or(0)), extraUri);
    assetData.needScanShootingMode = (photosPo.shootingModeTag.has_value() && photosPo.shootingModeTag->empty()) ||
        (photosPo.frontCamera.has_value() && photosPo.frontCamera->empty());
    assetData.lakeInfo = lakeInfos.at(photosPo.cloudId.value_or(""));
    assetData.needScanHdrMode = photosPo.hdrMode.value_or(0) == 0 || photosPo.editTime.value_or(0) > 0 ||
        photosPo.movingPhotoEffectMode.value_or(0) > 0;
    return assetData;
}

void CloudMediaDownloadService::UnlinkAsset(OnDownloadAssetData &assetData)
{
    int32_t ret = unlink(assetData.localPath.c_str());
    CHECK_AND_RETURN_LOG(
        ret != E_OK, "unlink %{public}s succeeded", MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
    assetData.err = errno;  // err always -1, use errno.
    assetData.errorMsg = "unlink failed";
    MEDIA_WARN_LOG("unlink %{public}s failed", MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
}

void CloudMediaDownloadService::ResetAssetModifyTime(OnDownloadAssetData &assetData)
{
    int32_t ret = access(assetData.localPath.c_str(), F_OK);  // 0 mean file exist.
    CHECK_AND_RETURN_LOG(
        ret == E_OK, "file not exist %{public}s", MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
    int32_t err = CloudMediaSyncUtils::UpdateModifyTime(assetData.localPath, assetData.dateModified);
    CHECK_AND_RETURN_INFO_LOG(err != E_OK,
        "UpdateModifyTime %{public}s succeeded",
        MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
    assetData.err = err;
    assetData.errorMsg = "Update ModifyTime failed";
    MEDIA_WARN_LOG("DownloadAsset UpdateModifyTime %{public}s failed",
        MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
}

int32_t CloudMediaDownloadService::SliceAssetFile(const std::string &originalFile, const std::string &path,
    const std::string &videoPath, const std::string &extraDataPath)
{
    MEDIA_INFO_LOG("SliceAssetFile");
    if (access(originalFile.c_str(), F_OK) != F_OK) {
        MEDIA_ERR_LOG("SliceAssetFile Not exist %{public}s", originalFile.c_str());
        return E_PATH;
    }
    std::string temp = originalFile + ".slicetemp";
    if (rename(originalFile.c_str(), temp.c_str()) == 0) {
        MEDIA_INFO_LOG("SliceAssetFile originalFile:%{public}s, path:%{public}s, videoPath:%{public}s, "
                       "extraDataPath:%{public}s, temp:%{public}s",
            originalFile.c_str(),
            path.c_str(),
            videoPath.c_str(),
            extraDataPath.c_str(),
            temp.c_str());
        if (MovingPhotoFileUtils::ConvertToMovingPhoto(temp, path, videoPath, extraDataPath) != 0) {
            MEDIA_INFO_LOG("SliceAssetFile convert to moving photo fail %{public}s", originalFile.c_str());
            if (unlink(temp.c_str()) != 0) {
                MEDIA_WARN_LOG("SliceAssetFile convert failed delete temp");
            }
            return E_PATH;
        }
        if (unlink(temp.c_str()) != 0) {
            MEDIA_WARN_LOG("SliceAssetFile convert success delete temp");
        }
    } else {
        MEDIA_ERR_LOG("SliceAssetFile rename failed path:%{public}s, to temp:%{public}s", path.c_str(), temp.c_str());
        return E_PATH;
    }
    return E_OK;
}

int32_t CloudMediaDownloadService::SliceAsset(const OnDownloadAssetData &assetData, const PhotosPo &photo)
{
    if (assetData.needSliceRaw) {
        std::string rawFilePath = PhotoFileUtils::GetEditDataSourcePath(assetData.path);
        bool isLivePhoto = MovingPhotoFileUtils::IsLivePhoto(rawFilePath);
        if (isLivePhoto) {
            std::string sourceImage = CloudMediaSyncUtils::GetSourceMovingPhotoImagePath(assetData.path);
            std::string sourceVideo = CloudMediaSyncUtils::GetSourceMovingPhotoVideoPath(assetData.path);
            int32_t ret = SliceAssetFile(rawFilePath, sourceImage, sourceVideo, "");
            CHECK_AND_PRINT_LOG(ret == E_OK,
                "SliceRawFile Failed. rawFilePath: %{public}s, sourceImage: %{public}s, sourceVideo: %{public}s",
                rawFilePath.c_str(), sourceImage.c_str(), sourceVideo.c_str());
        } else {
            MEDIA_WARN_LOG("OnDownloadAsset need slice raw, but file is not live photo");
        }
    }
    int ret = E_OK;
    if (assetData.needSliceContent) {
        bool isGraffiti = CloudMediaSyncUtils::IsGraffiti(photo);
        std::string videoPath = CloudMediaSyncUtils::GetMovingPhotoVideoPath(assetData.path);
        std::string extraDir = CloudMediaSyncUtils::GetMovingPhotoExtraDataDir(assetData.path);
        std::string extraDataPath = isGraffiti ? "" : CloudMediaSyncUtils::GetMovingPhotoExtraDataPath(assetData.path);
        if (!ForceCreateDirectory(extraDir)) {
            MEDIA_ERR_LOG("HandleAssetFile %{public}s error %{public}d", extraDir.c_str(), errno);
            return E_PATH;
        }
        ret = SliceAssetFile(assetData.localPath, assetData.localPath, videoPath, extraDataPath);
        if (ret == E_OK && assetData.needParseCover) {
            MEDIA_DEBUG_LOG("cover position is invalid, parse cover position from file");
            CoverPositionParser::GetInstance().AddTask(assetData.path, assetData.fileUri);
        }
    }
    // for cloud enhancement composite photo
    CloudMediaSyncUtils::SyncDealWithCompositePhoto(assetData.path, photo.fileId.value_or(0));
    MEDIA_INFO_LOG("SliceAsset, assetData: %{public}s", assetData.ToString().c_str());
    return ret;
}

std::string CloudMediaDownloadService::GetDisplayName(const PhotosPo &photosPo)
{
    std::string photoDisplayName = "";
    if (photosPo.displayName.has_value()) {
        photoDisplayName = photosPo.displayName.value();
        MEDIA_INFO_LOG("CloudMediaDownloadService::GetDisplayName photoDisplayName=%{public}s",
                       photoDisplayName.c_str());
    } else {
        MEDIA_INFO_LOG("no CloudMediaDownloadService::GetDisplayName photoDisplayName");
    }
    return photoDisplayName;
}
 
int32_t CloudMediaDownloadService::GetFileId(const PhotosPo &photosPo)
{
    MEDIA_INFO_LOG("CloudMediaDownloadService::GetFileId start");
    int32_t photoFileId = 0;
    if (photosPo.displayName.has_value()) {
        photoFileId = photosPo.fileId.value();
        MEDIA_INFO_LOG("CloudMediaDownloadService::GetFileId photoFileId=%{public}d", photoFileId);
    } else {
        MEDIA_INFO_LOG("no CloudMediaDownloadService::GetFileId photoFileId");
    }
    return photoFileId;
}
 
void CloudMediaDownloadService::UpdateVideoMode(std::vector<PhotosPo> &photosPoVec)
{
    for (auto &photosPo : photosPoVec) {
        int32_t mediaTypePhoto = photosPo.mediaType.value_or(0);
        if (mediaTypePhoto != static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
            MEDIA_INFO_LOG("photosPo is not video");
            continue;
        }
        int32_t fileId = GetFileId(photosPo);
        std::string queryVideoSql = "SELECT video_mode FROM Photos WHERE file_id = " + std::to_string(fileId);
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null!");
        std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryVideoSql);
        CHECK_AND_RETURN_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
            "UpdateVideoMode resultSet is nullptr or empty.");
        int32_t videoMode = GetInt32Val(PhotoColumn::PHOTO_VIDEO_MODE, resultSet);
        if (videoMode != static_cast<int32_t>(VideoMode::DEFAULT)) {
            MEDIA_INFO_LOG("photosPo has scannered");
            continue;
        }
        string logVideoPath = photosPo.data.value_or("");
        unique_ptr<Metadata> videoModeData = make_unique<Metadata>();
        videoModeData->SetFilePath(logVideoPath);
        int32_t err = MetadataExtractor::ExtractAVMetadata(videoModeData);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to extract metadata for photosPo: %{public}s",
                DfxUtils::GetSafePath(logVideoPath).c_str());
            continue;
        }
        int32_t videoModeUpdate = videoModeData->GetVideoMode();
        MEDIA_INFO_LOG("photosPo videoMode=%{public}d", videoModeUpdate);
        auto photoRet = PhotoVideoModeOperation::UpdatePhotosVideoMode(videoModeUpdate, fileId);
        CHECK_AND_RETURN_LOG(photoRet == NativeRdb::E_OK,
            "UpdatePhotosVideoMod photostab failed, error id: %{public}d", photoRet);
    }
}

int32_t CloudMediaDownloadService::OnDownloadAsset(
    const std::vector<std::string> &cloudIds, std::vector<MediaOperateResultDto> &result)
{
    // get downloadAssetDataVec
    std::vector<PhotosPo> photosPoVec;
    int32_t ret = this->dao_.QueryDownloadAssetByCloudIds(cloudIds, photosPoVec);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "QueryDownloadAssetByCloudIds failed, ret:%{public}d", ret);
    // Requirement: If any asset is not in the database, return error. Caller should check the result.
    CHECK_AND_RETURN_RET_LOG(photosPoVec.size() == cloudIds.size(),
        E_CLOUDSYNC_INVAL_ARG,
        "QueryDownloadAssetByCloudIds failed, cloudIds size:%{public}zu, photosPoVec size:%{public}zu",
        cloudIds.size(),
        photosPoVec.size());
    // Update
    UpdateVideoMode(photosPoVec);
    OnDownloadAssetData assetData;
    for (auto &photosPo : photosPoVec) {
        assetData = this->GetOnDownloadAssetData(photosPo);
        MEDIA_DEBUG_LOG(
            "OnDownloadAsset %{public}s, %{public}s", photosPo.ToString().c_str(), assetData.ToString().c_str());
        HandlePhoto(photosPo, assetData);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
        UpdateBatchDownloadTask(photosPo);
#endif
        // record result
        MediaOperateResultDto mediaResult;
        mediaResult.cloudId = photosPo.cloudId.value_or("");
        mediaResult.errorCode = assetData.err;
        mediaResult.errorMsg = assetData.errorMsg;
        result.emplace_back(mediaResult);
    }
    return E_OK;
}

int32_t CloudMediaDownloadService::OnDownloadLakeAsset(
    const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos,
    std::vector<MediaOperateResultDto> &result)
{
    MEDIA_INFO_LOG("enter CloudMediaDownloadService::OnDownloadLakeAsset, %{public}zu", lakeInfos.size());
    // get downloadAssetDataVec
    std::vector<PhotosPo> photosPoVec;
    int32_t ret = this->dao_.QueryDownloadLakeAssetByCloudIds(lakeInfos, photosPoVec);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "QueryDownloadLakeAssetByCloudIds failed, ret:%{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(photosPoVec.size() == lakeInfos.size(),
        E_CLOUDSYNC_INVAL_ARG,
        "QueryDownloadLakeAssetByCloudIds failed, cloudIds size:%{public}u, photosPoVec size:%{public}u",
        lakeInfos.size(),
        photosPoVec.size());
    // Update
    OnDownloadAssetData assetData;
    for (auto &photosPo : photosPoVec) {
        assetData = this->GetOnDownloadLakeAssetData(photosPo, lakeInfos);
        MEDIA_DEBUG_LOG(
            "OnDownloadLakeAsset %{public}s, %{public}s", photosPo.ToString().c_str(), assetData.ToString().c_str());
        HandlePhoto(photosPo, assetData);
        // record result
        MediaOperateResultDto mediaResult;
        mediaResult.cloudId = photosPo.cloudId.value_or("");
        mediaResult.errorCode = assetData.err;
        mediaResult.errorMsg = assetData.errorMsg;
        result.emplace_back(mediaResult);
    }
    return E_OK;
}

void CloudMediaDownloadService::HandlePhoto(const ORM::PhotosPo &photo, OnDownloadAssetData &assetData)
{
    int32_t ret = SliceAsset(assetData, photo);
    if (ret != E_OK) {
        MEDIA_INFO_LOG(
            "HandlePhoto Failed to Slice %{public}s", MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
        assetData.errorMsg = "Slice Moving File Failed";
        assetData.err = ret;
        return;
    }
    CloudMediaScanService::ScanResult scanResult;
    bool isNeedScan = assetData.needScanShootingMode || assetData.needScanSubtype || assetData.needScanHdrMode;
    CHECK_AND_EXECUTE(!isNeedScan, this->scanService_.ScanDownloadedFile(assetData.path, scanResult));
    if (assetData.lakeInfo) {
        // lake file
        MEDIA_INFO_LOG("the file is lake");
        ret = this->dao_.UpdateDownloadLakeAsset(assetData, scanResult);
    } else {
        ret = this->dao_.UpdateDownloadAsset(assetData, scanResult);
    }
    CalEditDataSizeInHandlePhoto(photo);
    if (scanResult.scanSuccess) {
        CloudMediaScanService().UpdateAndNotifyShootingModeAlbumIfNeeded(scanResult);
    }

    if (ret != E_OK) {
        MEDIA_INFO_LOG(
            "Failed to Handle HandlePhoto %{public}s", MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
        assetData.errorMsg = "UpdateDownloadAsset failed";
        assetData.err = ret;
        this->UnlinkAsset(assetData);
        return;
    } else if (assetData.dateModified > 0 && (assetData.needSliceContent || assetData.needSliceRaw)) {
        this->ResetAssetModifyTime(assetData);
    }

    ret = FixDownloadAssetExifRotate(photo, assetData);
    if (ret != E_OK) {
        MEDIA_INFO_LOG("HandlePhoto Failed to fix exif rotate %{public}s",
            MediaFileUtils::DesensitizePath(assetData.localPath).c_str());
        assetData.errorMsg = "Fix Exif Rotate Failed";
        assetData.err = ret;
        return;
    }
    CloudMediaSyncUtils::RemoveTransCodePath(assetData.localPath);
    ret = this->dao_.UpdateTransCodeInfo(assetData.path);
    if (ret != E_OK) {
        assetData.errorMsg = "[OnDownloadAsset] UpdateTransCodeInfo failed";
        assetData.err = ret;
        return;
    }
    MEDIA_INFO_LOG("[OnDownloadAsset] Delete transCode file Success!");
}
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
void CloudMediaDownloadService::UpdateBatchDownloadTask(const ORM::PhotosPo &photo)
{
    int32_t fileId = photo.fileId.value_or(-1);
    CHECK_AND_RETURN(fileId != -1);
    MEDIA_INFO_LOG("Successfully download asset[%{public}d] by single task, need to handle batch task.", fileId);
    BackgroundCloudBatchSelectedFileProcessor::UpdateDBStatusInfoForSingleDownloadCompletely(fileId);
}
#endif
void CloudMediaDownloadService::CalEditDataSizeInHandlePhoto(const ORM::PhotosPo &photo)
{
    int32_t fileId = photo.fileId.value_or(0);
    int32_t ret = MediaLibraryPhotoOperations::CalSingleEditDataSize(to_string(fileId));
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CalSingleEditDataSize failed for ID: %{public}d (ret code: %{public}d)", fileId, ret);
    }
}

int32_t CloudMediaDownloadService::FixDownloadAssetExifRotate(
    const ORM::PhotosPo &photo, OnDownloadAssetData &assetData)
{
    CHECK_AND_RETURN_RET(assetData.exifRotate != photo.exifRotate.value_or(0),
        CheckRegenerateThumbnail(photo, assetData));

    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL,
        "FixDownloadAssetExifRotate Failed to get rdbStore.");

    int32_t oldExifRotate = photo.exifRotate.value_or(0);
    int32_t newExifRotate = assetData.exifRotate;
    int32_t fileId = photo.fileId.value_or(0);
    int32_t ret;
    MEDIA_INFO_LOG("Need FixDownloadAssetExifRotate, id:%{public}d, mediaType:%{public}d, oldExifRotate:%{public}d, "
        "newExifRotate:%{public}d", fileId, assetData.mediaType, oldExifRotate, newExifRotate);
    if (CloudMediaSyncUtils::CanUpdateExifRotateOnly(assetData.mediaType, oldExifRotate, newExifRotate)) {
        ret = this->dao_.UpdateDownloadAssetExifRotateFix(
            photoRefresh, fileId, assetData.exifRotate, DirtyTypes::TYPE_MDIRTY, false);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        photoRefresh->Notify();
        return E_OK;
    }

    DirtyTypes dirtyType;
    if (assetData.mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
        dirtyType = DirtyTypes::TYPE_MDIRTY;
    } else {
        dirtyType = DirtyTypes::TYPE_FDIRTY;
    }
    ret = this->dao_.UpdateDownloadAssetExifRotateFix(photoRefresh, fileId, assetData.exifRotate, dirtyType, true);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    auto thumbnailService = ThumbnailService::GetInstance();
    thumbnailService->DeleteThumbnailDirAndAstc(std::to_string(fileId), PhotoColumn::PHOTOS_TABLE,
        assetData.path, std::to_string(photo.dateTaken.value_or(0)));
    photoRefresh->Notify();
    return thumbnailService->FixThumbnailExifRotateAfterDownloadAsset(std::to_string(fileId));
}

int32_t CloudMediaDownloadService::CheckRegenerateThumbnail(
    const ORM::PhotosPo &photo, OnDownloadAssetData &assetData)
{
    CHECK_AND_RETURN_RET(ExifRotateUtils::IsExifRotateWithFlip(assetData.exifRotate), E_OK);
    int32_t fileId = photo.fileId.value_or(0);
    MEDIA_INFO_LOG("Need regenerate thumbnail, id:%{public}d, exifRotate:%{public}d", fileId, assetData.exifRotate);
    auto thumbnailService = ThumbnailService::GetInstance();
    return thumbnailService->FixThumbnailExifRotateAfterDownloadAsset(std::to_string(fileId), false);
}
}  // namespace OHOS::Media::CloudSync