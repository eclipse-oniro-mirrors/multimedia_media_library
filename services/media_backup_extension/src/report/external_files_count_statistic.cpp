/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#include "external_files_count_statistic.h"

#include "media_log.h"
#include "backup_database_utils.h"
#include "backup_const.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
static const int32_t INIT_FAIL_TOTAL_COUNT = -1;
static const int32_t IMAGE_MEDIA_TYPE = 1;
static const int32_t VIDEO_MEDIA_TYPE = 3;
std::vector<AlbumMediaStatisticInfo> ExternalFilesCountStatistic::Load()
{
    CHECK_AND_RETURN_RET_LOG(this->externalRdb_ != nullptr, {}, "externalRdb_ is nullptr, Maybe init failed.");
    return {this->GetMediaStatInfo(), this->GetAudioStatInfo(), this->GetGalleryNotSyncMediaStatInfo()};
}

AlbumMediaStatisticInfo ExternalFilesCountStatistic::GetMediaStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.albumName = "External_Media";
    auto mediaTypeCountMap = this->QueryExternalImageAndVideoCount();
    GetMediaStatInfoFromMap(mediaTypeCountMap, info);
    return info;
}

AlbumMediaStatisticInfo ExternalFilesCountStatistic::GetAudioStatInfo()
{
    int32_t externalAudioCount = this->QueryExternalAudioCount();
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.albumName = "External_Audio";
    info.totalCount = externalAudioCount;
    return info;
}

AlbumMediaStatisticInfo ExternalFilesCountStatistic::GetGalleryNotSyncMediaStatInfo()
{
    AlbumMediaStatisticInfo info;
    info.sceneCode = this->sceneCode_;
    info.taskId = this->taskId_;
    info.albumName = "Extermal_Restore_Gallery_Not_Sync_Media";
    if (this->galleryRdb_ == nullptr) {
        MEDIA_ERR_LOG("galleryRdb_ is nullptr, Maybe init failed.");
        info.totalCount = INIT_FAIL_TOTAL_COUNT;
        return info;
    }
    int32_t maxId = BackupDatabaseUtils::QueryInt(galleryRdb_, QUERY_MAX_ID_ALL, CUSTOM_MAX_ID);
    auto mediaTypeCountMap = this->QueryGalleryNotSyncMedia(maxId);
    GetMediaStatInfoFromMap(mediaTypeCountMap, info);
    return info;
}

void ExternalFilesCountStatistic::GetMediaStatInfoFromMap(
    const std::unordered_map<int32_t, int32_t>& mediaTypeCountMap, AlbumMediaStatisticInfo& info)
{
    for (auto iter = mediaTypeCountMap.begin(); iter != mediaTypeCountMap.end(); iter++) {
        int32_t mediaType = iter->first;
        int32_t count = iter->second;
        info.totalCount += count;
        switch (mediaType) {
            case IMAGE_MEDIA_TYPE:
                info.imageCount = count;
                break;
            case VIDEO_MEDIA_TYPE:
                info.videoCount = count;
                break;
            default:
                break;
        }
    }
}

int32_t ExternalFilesCountStatistic::QueryExternalAudioCount()
{
    static string QUERY_EXTERNAL_AUDIO_COUNT = "SELECT count(1) as count FROM files WHERE media_type = 2 AND _size > 0 \
        AND _data LIKE '/storage/emulated/0/Music%'";
    return BackupDatabaseUtils::QueryInt(this->externalRdb_, QUERY_EXTERNAL_AUDIO_COUNT, CUSTOM_COUNT);
}

std::unordered_map<int32_t, int32_t> ExternalFilesCountStatistic::QueryExternalImageAndVideoCount()
{
    static string QUERY_EXTERNAL_IMAGE_AND_VIDEO_COUNT = QUERY_MEDIA_TYPE_AND_COUNT_FROM_FILES +
        IMAGE_AND_VIDEO_TYPE + GROUP_BY_MEIDA_TYPE;
    return BackupDatabaseUtils::QueryMediaTypeCount(this->externalRdb_, QUERY_EXTERNAL_IMAGE_AND_VIDEO_COUNT);
}

std::unordered_map<int32_t, int32_t> ExternalFilesCountStatistic::QueryGalleryNotSyncMedia(const int32_t maxId)
{
    std::string queryNotSyncByCount = QUERY_MEDIA_TYPE_AND_COUNT_FROM_FILES + IS_PENDING + " AND " +
        COMPARE_ID + std::to_string(maxId) + " AND " + QUERY_NOT_SYNC + GROUP_BY_MEIDA_TYPE;
    return BackupDatabaseUtils::QueryMediaTypeCount(externalRdb_, queryNotSyncByCount);
}

}  // namespace OHOS::Media