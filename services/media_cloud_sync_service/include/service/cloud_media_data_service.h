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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_SERVICE_H

#include <string>
#include <vector>

#include "message_parcel.h"
#include "photos_dto.h"
#include "photos_vo.h"
#include "rdb_store.h"
#include "check_file_data_dto.h"
#include "media_operate_result_dto.h"
#include "aging_file_query_dto.h"
#include "cloud_media_data_dao.h"
#include "cloud_media_data_service_processor.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_define.h"
#include "query_data_vo.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDataService {
public:
    // 核查相关
    int32_t UpdateDirty(const std::string &cloudId, const int32_t dirtyType);
    int32_t UpdatePosition(const std::vector<std::string> &cloudIds, const int32_t position);
    int32_t UpdatePosWithType(const std::vector<std::string> &cloudIds,
        const int32_t position, const int32_t fileSourceType);
    int32_t UpdateFileSourceType(const std::vector<std::string> &cloudIds,
        const int32_t fileSourceType);
    int32_t UpdateSyncStatus(const std::string &cloudId, const int32_t syncStatus);
    int32_t UpdateThmStatus(const std::string &cloudId, const int32_t thmStatus);
    int32_t GetAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosDto> &photosDtos);
    int32_t GetActiveAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosDto> &photosDtos);
    int32_t UpdateLocalFileDirty(const std::vector<std::string> &cloudIdList);

    // 缓存视频
    int32_t GetVideoToCache(std::vector<PhotosDto> &photosDtos);

    // 大数据
    std::vector<uint64_t> GetFilePosStat();
    std::vector<uint64_t> GetCloudThmStat();
    int32_t GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat);

    int32_t CheckAndFixAlbum();
    int32_t QueryData(const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &columnNames,
                      const std::string &tableName, std::vector<std::unordered_map<std::string, std::string>> &results);
    int32_t UpdateData(const std::string &tableName, const DataShare::DataSharePredicates &predicates,
                        const DataShare::DataShareValuesBucket &value, const std::string &operateName);

private:
    enum {
        // Index of File Position Statistic Info.
        INDEX_LOCAL = 0,
        INDEX_CLOUD = 1,
        INDEX_LOCAL_AND_CLOUD = 2,
        // Index of Cloud Thumbnail Statistic Info.
        INDEX_DOWNLOADED = 0,
        INDEX_LCD_TO_DOWNLOAD = 1,
        INDEX_THM_TO_DOWNLOAD = 2,
        INDEX_TO_DOWNLOAD = 3,
    };

private:
    CloudMediaDataDao dataDao_;
    CloudMediaDataServiceProcessor processor_;
    CloudMediaCommonDao commonDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_SERVICE_H