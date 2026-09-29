/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef PHOTO_MAP_CODE_OPERATIOIN_H
#define PHOTO_MAP_CODE_OPERATIOIN_H

#include "values_bucket.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"

#include "media_column.h"
#include "directory_ex.h"
#include "media_log.h"
#include "abs_rdb_predicates.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_sync_utils.h"
#include "cloud_media_operation_code.h"
#include "medialibrary_unistore_manager.h"
#include "moving_photo_file_utils.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "cloud_sync_convert.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "scanner_utils.h"
#include "cloud_media_dao_const.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_dao_utils.h"

#include <string>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class PhotoMapType {
    QUERY_AND_INSERT = 0,
    UPDATE_AND_INSERT,
};

const std::string MAP_CODE_IS_NOT_READY = "0";
const std::string MAP_CODE_IS_READY = "1";
const std::string MAP_CODE_READY_STATUS_DEFAULT = "0";

class PhotoMapData {
public:
    int32_t fileId{-1};
    double latitude{0.0};
    double longitude{0.0};

    PhotoMapData(int32_t fileIdIn, double latitudeIn, double longitudeIn) : fileId(fileIdIn), latitude(latitudeIn),
        longitude(longitudeIn) {}
    PhotoMapData() : fileId(-1), latitude(0.0),
        longitude(0.0) {}
};

class PhotoMapCodeOperation {
public:
    static int32_t InsertPhotosMapCodes(const std::vector<PhotoMapData> &photoMapDatas,
        const std::shared_ptr<NativeRdb::RdbStore> cloneLibraryRdb);
    static int32_t GetPhotosMapCodesMRS(const std::vector<PhotoMapData> &photoMapDatas,
        const std::shared_ptr<MediaLibraryRdbStore> store);
    static int32_t GetPhotoMapCode(const PhotoMapData &photoMapData, const PhotoMapType &photoMapType);
    static int32_t UpgradePendingPhotoMapCodes(const std::shared_ptr<MediaLibraryRdbStore> store, int32_t batchSize);
    static int32_t RemovePhotosMapCodes(const std::vector<std::string> &fileIds);
    static int32_t GetPhotosPoByInputValues(const std::vector<std::string> &inputValues,
        std::vector<Media::ORM::PhotosPo> &photosPos, const std::vector<std::string> &getValues);
    static int64_t GetMapHilbertCode(double lat, double lon, int level);
    static void SetMapCodeReadyStatus(std::string status);
    static std::string GetMapCodeReadyStatus();

private:
    static vector<std::string> FilterFileIds(const vector<std::string> &fileIds);
    static void GetPhotoMapCode(NativeRdb::ValuesBucket &mapValue, double lat, double lon);
    static std::vector<double> SetPoint(double lat, double lon);
    static int64_t GetMapCode(std::vector<double> &latAndLon, int level);
    static int32_t BuildMapValues(const std::vector<PhotoMapData> &photoMapDatas,
        vector<NativeRdb::ValuesBucket> &mapValues);
    static int32_t BatchInsertMapCodes(const vector<NativeRdb::ValuesBucket> &mapValues,
        const std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static int32_t BatchInsertMapCodes(const vector<NativeRdb::ValuesBucket> &mapValues,
        const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    static int32_t ExecSqlWithRetry(std::function<int32_t()> execSql);
    // 计算工具函数，内部可见
    static std::string Int64ToBinaryWithPadding(int64_t num, int width);
    static void UpdateZoomLevelStep(int64_t zoomLevel, std::vector<int64_t> &point);
    static int64_t DistanceFromPoint(int64_t xPosition, int64_t yPosition, int level);

    static int32_t QueryPhotosPoByStepLevel(const std::shared_ptr<MediaLibraryRdbStore> store,
        int32_t startId, int32_t stepLevel, std::vector<Media::ORM::PhotosPo> &photosPos);
    static int32_t ConvertToPhotoMapData(const std::vector<Media::ORM::PhotosPo> &photosPos,
        int32_t &i, std::vector<PhotoMapData> &photoMapDatas, int32_t &detailCount);
    static int32_t ConvertToPhotoMapData(const std::vector<Media::ORM::PhotosPo> &photosPos,
        std::vector<PhotoMapData> &photoMapDatas);
private:
    static const int LEVEL_START;
    static const int LEVEL_COUNT;
    static const int STEP_COUNT;
    static const int STEP_LEVEL;
};
} // namespace Media
} // namespace OHOS

#endif // PHOTO_MAP_CODE_OPERATIOIN_H
