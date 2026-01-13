/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef GEO_KNOWLEDGE_RESTORE_H
#define GEO_KNOWLEDGE_RESTORE_H

#include <string>

#include "backup_const.h"
#include "nlohmann/json.hpp"
#include "rdb_store.h"

namespace OHOS::Media {
class GeoKnowledgeRestore {
public:
    void Init(int32_t sceneCode, std::string taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    void RestoreGeo(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

private:
    struct GeoKnowledgeInfo {
        int64_t locationKey;
        std::string language;
        std::string country;
        std::string adminArea;
        std::string subAdminArea;
        std::string locality;
        std::string subLocality;
        std::string thoroughfare;
        std::string subThoroughfare;
        std::string featureName;
        double latitude;
        double longitude;
    };

    struct GeoMapInfo {
        int32_t fileIdOld {-1};
        PhotoInfo photoInfo;
        double latitude {0};
        double longitude {0};
    };

    void GetGeoKnowledgeInfos();
    void RestoreMaps(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void ReportGeoRestoreTask();
    NativeRdb::ValuesBucket GetMapInsertValue(std::vector<GeoKnowledgeInfo>::iterator it, int32_t fileId);
    int32_t BatchUpdate(const std::string &tableName, std::vector<std::string> &fileIds);
    int32_t BatchInsertWithRetry(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);
    void UpdateMaps(std::vector<NativeRdb::ValuesBucket> &values, std::vector<std::string> &fileIds);
    std::string UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values, const GeoMapInfo &geoMapInfo);
    std::string UpdateByGeoLocation(std::vector<NativeRdb::ValuesBucket> &values, const GeoMapInfo &geoMapInfo);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
    std::string systemLanguage_{"zh-Hans"};
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::vector<GeoKnowledgeInfo> albumInfos_;
    std::atomic<int32_t> batchCnt_{0};
    std::atomic<int32_t> successInsertCnt_{0};
    std::atomic<int32_t> successUpdateCnt_{0};
    std::atomic<int32_t> failInsertCnt_{0};
    std::atomic<int32_t> failUpdateCnt_{0};
};
} // namespace OHOS::Media

#endif // GEO_KNOWLEDGE_RESTORE_H