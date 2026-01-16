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

#include "cloud_media_data_service.h"

#include <fcntl.h>
#include <string>
#include <vector>

#include "cloud_media_operation_code.h"
#include "cloud_sync_notify_handler.h"
#include "cloud_media_sync_utils.h"
#include "directory_ex.h"
#include "cloud_media_file_utils.h"
#include "media_file_utils.h"
#include "cloud_media_attachment_utils.h"
#include "media_itypes_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "moving_photo_file_utils.h"
#include "parameters.h"
#include "result_set_utils.h"
#include "query_data_vo.h"
#include "photos_po.h"
#include "photos_po_writer.h"
#include "photo_album_po.h"
#include "photo_album_po_writer.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaDataService::UpdateDirty(const std::string &cloudId, const int32_t dirtyType)
{
    MEDIA_INFO_LOG("UpdateDirty begin, cloudId: %{public}s, dirtyType: %{public}d", cloudId.c_str(), dirtyType);
    int32_t ret = this->dataDao_.UpdateDirty(cloudId, dirtyType);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateDirty.");
    return ret;
}

int32_t CloudMediaDataService::UpdatePosition(const std::vector<std::string> &cloudIds, const int32_t position)
{
    MEDIA_INFO_LOG("UpdatePosition begin, cloudIds size: %{public}zu, position: %{public}d", cloudIds.size(), position);
    int32_t ret = this->dataDao_.UpdatePosition(cloudIds, position);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdatePosition.");
    return ret;
}

int32_t CloudMediaDataService::UpdatePosWithType(const std::vector<std::string> &cloudIds,
    const int32_t position, const int32_t fileSourceType)
{
    MEDIA_INFO_LOG("UpdatePosWithType begin, cloudIds size: %{public}zu, position: %{public}d,"
        "fileSourceType: %{public}d",
        cloudIds.size(), position, fileSourceType);
    int32_t ret = this->dataDao_.UpdatePosWithType(cloudIds, position, fileSourceType);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdatePosWithType.");
    return ret;
}

int32_t CloudMediaDataService::UpdateFileSourceType(const std::vector<std::string> &cloudIds,
    int32_t fileSourceType)
{
    MEDIA_INFO_LOG("UpdateFileSourceType begin, cloudIds size: %{public}zu, fileSourceType: %{public}d",
        cloudIds.size(), fileSourceType);
    int32_t ret = this->dataDao_.UpdateFileSourceType(cloudIds, fileSourceType);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateFileSourceType.");
    return ret;
}


int32_t CloudMediaDataService::UpdateSyncStatus(const std::string &cloudId, const int32_t syncStatus)
{
    MEDIA_INFO_LOG("UpdateSyncStatus, cloudId: %{public}s, syncStatus: %{public}d", cloudId.c_str(), syncStatus);
    int32_t ret = this->dataDao_.UpdateSyncStatus(cloudId, syncStatus);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateSyncStatus.");
    return ret;
}

int32_t CloudMediaDataService::UpdateThmStatus(const std::string &cloudId, const int32_t thmStatus)
{
    MEDIA_INFO_LOG("UpdateThmStatus, cloudId: %{public}s, thmStatus: %{public}d", cloudId.c_str(), thmStatus);
    int32_t ret = this->dataDao_.UpdateThmStatus(cloudId, thmStatus);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateThmStatus.");
    return ret;
}

int32_t CloudMediaDataService::GetAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosDto> &photosDtos)
{
    MEDIA_INFO_LOG("GetAgingFile, queryDto: %{public}s", queryDto.ToString().c_str());
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dataDao_.GetAgingFile(queryDto, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetAgingFile, ret: %{public}d", ret);
    this->processor_.GetPhotosDto(photosPos, photosDtos);
    return E_OK;
}

int32_t CloudMediaDataService::GetActiveAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosDto> &photosDtos)
{
    MEDIA_INFO_LOG("GetActiveAgingFile, queryDto: %{public}s", queryDto.ToString().c_str());
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dataDao_.GetActiveAgingFile(queryDto, photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetActiveAgingFile, ret: %{public}d", ret);
    this->processor_.GetPhotosDto(photosPos, photosDtos);
    return E_OK;
}

int32_t CloudMediaDataService::GetVideoToCache(std::vector<PhotosDto> &photosDtos)
{
    MEDIA_INFO_LOG("GetVideoToCache, photosDtos size: %{public}zu", photosDtos.size());
    std::vector<PhotosPo> photosPos;
    int32_t ret = this->dataDao_.GetVideoToCache(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetVideoToCache, ret = %{public}d", ret);
    this->processor_.GetPhotosDtoOfVideoCache(photosPos, photosDtos);
    return E_OK;
}

std::vector<uint64_t> CloudMediaDataService::GetFilePosStat()
{
    MEDIA_INFO_LOG("GetFilePosStat");
    std::vector<uint64_t> filePosStat = {0, 0, 0};

    /* get local file status */
    int32_t ret;
    int32_t num;
    ret = this->dataDao_.QueryFilePosStat(static_cast<int32_t>(PhotoPositionType::LOCAL), num);
    filePosStat[INDEX_LOCAL] = static_cast<uint64_t>(num);

    /* get cloud file status */
    ret = this->dataDao_.QueryFilePosStat(static_cast<int32_t>(PhotoPositionType::CLOUD), num);
    filePosStat[INDEX_CLOUD] = static_cast<uint64_t>(num);

    /* get both file status */
    ret = this->dataDao_.QueryFilePosStat(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD), num);
    filePosStat[INDEX_LOCAL_AND_CLOUD] = static_cast<uint64_t>(num);
    return filePosStat;
}

std::vector<uint64_t> CloudMediaDataService::GetCloudThmStat()
{
    MEDIA_INFO_LOG("GetCloudThmStat");
    std::vector<uint64_t> cloudThmStat{0, 0, 0, 0};
    /* get DOWNLOADED thm status */
    int num = 0;
    this->dataDao_.QueryCloudThmStat(static_cast<int32_t>(ThumbState::DOWNLOADED), num);
    cloudThmStat[INDEX_DOWNLOADED] = static_cast<uint64_t>(num);
    MEDIA_INFO_LOG("ThumbState[0]: %{public}d", num);

    /* get LCD_TO_DOWNLOAD thm status */
    this->dataDao_.QueryCloudThmStat(static_cast<int32_t>(ThumbState::LCD_TO_DOWNLOAD), num);
    cloudThmStat[INDEX_LCD_TO_DOWNLOAD] = static_cast<uint64_t>(num);
    MEDIA_INFO_LOG("ThumbState[1]: %{public}d", num);

    /* get THM_TO_DOWNLOAD thm status */
    this->dataDao_.QueryCloudThmStat(static_cast<int32_t>(ThumbState::THM_TO_DOWNLOAD), num);
    cloudThmStat[INDEX_THM_TO_DOWNLOAD] = static_cast<uint64_t>(num);
    MEDIA_INFO_LOG("ThumbState[2]: %{public}d", num);

    /* get TO_DOWNLOAD thm status */
    this->dataDao_.QueryCloudThmStat(static_cast<int32_t>(ThumbState::TO_DOWNLOAD), num);
    cloudThmStat[INDEX_TO_DOWNLOAD] = static_cast<uint64_t>(num);
    MEDIA_INFO_LOG("ThumbState[3]: %{public}d", num);

    return cloudThmStat;
}

int32_t CloudMediaDataService::GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat)
{
    return this->dataDao_.GetDirtyTypeStat(dirtyTypeStat);
}

static bool IsLocalDirty(int32_t dirty, bool isDelete)
{
    bool localDirty = (dirty == static_cast<int32_t>(DirtyType::TYPE_MDIRTY)) ||
                      (dirty == static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    if (isDelete) {
        return localDirty;
    } else {
        return localDirty || (dirty == static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
}

int32_t CloudMediaDataService::UpdateLocalFileDirty(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("UpdateLocalFileDirty enter");
    std::vector<std::string> queryColums = {
        PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_CLOUD_ID, PhotoColumn::PHOTO_DIRTY};
    std::vector<PhotosPo> photos;
    int32_t ret = this->commonDao_.QueryLocalByCloudId(cloudIds, queryColums, photos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "UpdateLocalFileDirty Query Error");

    for (auto &photo : photos) {
        std::string cloudId = photo.cloudId.value_or("");
        MEDIA_INFO_LOG("UpdateLocalFileDirty Query CloudId: %{public}s", cloudId.c_str());
        if (cloudId.empty() || std::find(cloudIds.begin(), cloudIds.end(), cloudId) == cloudIds.end()) {
            continue;
        }
        if (IsLocalDirty(photo.dirty.value_or(-1), false)) {
            MEDIA_ERR_LOG("UpdateLocalFileDirty record dirty: %{public}s", cloudId.c_str());
            continue;
        }
        std::string path = photo.data.value_or("");
        if (path.empty()) {
            MEDIA_ERR_LOG("UpdateLocalFileDirty path empty: %{public}s", cloudId.c_str());
            continue;
        }
        std::string filePath = CloudMediaSyncUtils::GetLocalPath(path);
        if (access(filePath.c_str(), F_OK) != E_OK) {
            MEDIA_ERR_LOG("UpdateLocalFileDirty no original file, cloudId: %{public}s", cloudId.c_str());
            continue;
        }
        ret = this->dataDao_.UpdateLocalFileDirty(cloudId);
        MEDIA_INFO_LOG("UpdateLocalFileDirty update cloudId: %{public}s, ret: %{public}d", cloudId.c_str(), ret);
    }
    return ret;
}

int32_t CloudMediaDataService::CheckAndFixAlbum()
{
    int32_t ret = this->dataDao_.CheckAndDeleteAlbum();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to CheckAndDeleteAlbum, ret = %{public}d", ret);
    ret = this->dataDao_.CheckAndUpdateAlbum();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to CheckAndUpdateAlbum, ret = %{public}d", ret);
    return E_OK;
}

int32_t CloudMediaDataService::QueryData(const DataShare::DataSharePredicates &predicates,
    const std::vector<std::string> &columnNames, const std::string &tableName,
    std::vector<std::unordered_map<std::string, std::string>> &results)
{
    int32_t ret;
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        std::vector<PhotosPo> photoInfos;
        ret = this->dataDao_.QueryDataFromPhotos(predicates, columnNames, photoInfos);
        for (PhotosPo &photoPo : photoInfos) {
            results.emplace_back(PhotosPoWriter(photoPo).ToMap(false));
        }
    } else if (tableName == PhotoAlbumColumns::TABLE) {
        std::vector<PhotoAlbumPo> photoAlbumInfos;
        ret = this->dataDao_.QueryDataFromPhotoAlbums(predicates, columnNames, photoAlbumInfos);
        for (PhotoAlbumPo &photoAlbumPo : photoAlbumInfos) {
            results.emplace_back(PhotoAlbumPoWriter(photoAlbumPo).ToMap(false));
        }
    } else {
        MEDIA_INFO_LOG("Invalid tableName in QueryData");
        return E_ERR;
    }
    return ret;
}

int32_t CloudMediaDataService::UpdateData(const std::string &tableName,
    const DataShare::DataSharePredicates &predicates, const DataShare::DataShareValuesBucket &value,
    const std::string &operateName)
{
    int32_t ret;
    if (tableName == PhotoColumn::PHOTOS_TABLE) {
        ret = this->dataDao_.UpdateDataInPhotos(predicates, value);
    } else {
        MEDIA_INFO_LOG(
            "Invalid tableName in UpdateData, tableName = %{public}s", tableName.c_str());
        return E_ERR;
    }
    return ret;
}
}  // namespace OHOS::Media::CloudSync