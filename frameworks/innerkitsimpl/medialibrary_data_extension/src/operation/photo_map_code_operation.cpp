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
#define MLOG_TAG "PhotoMapCodeOperation"

#include "parameters.h"
#include "photo_map_code_column.h"
#include "photo_map_code_operation.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdbstore.h"
#include "directory_ex.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
#include "abs_rdb_predicates.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_sync_utils.h"
#include "cloud_media_operation_code.h"
#include "moving_photo_file_utils.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "cloud_sync_convert.h"
#include "photo_map_column.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "scanner_utils.h"
#include "cloud_media_dao_const.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_dao_utils.h"
#include "base_column.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_tracer.h"
#include "rdb_store.h"

#include "cpu_utils.h"
#include <cerrno>
#include <fstream>
#include <iostream>
#include <bitset>
#include <string>
#include <cmath>

namespace OHOS {
namespace Media {
using namespace std;
const int32_t COUNT_INDEX = 0;
const int32_t MIN_INDEX = 1;
const int32_t MAX_INDEX = 2;
const int32_t POINT_SIZE = 2;
const int FAST_COUNT = 30000;

const int PhotoMapCodeOperation::LEVEL_START = 20;
const int PhotoMapCodeOperation::LEVEL_COUNT = 21;
const int PhotoMapCodeOperation::STEP_COUNT = 10;
const int PhotoMapCodeOperation::STEP_LEVEL = 20;

static constexpr double DOUBLE_EPSILON = 1e-15;
static constexpr double MAX_LATITUDE_EPSILON = 1e-15 + 90.0;
static constexpr double MAX_LONGITUDE_EPSILON = 1e-15 + 180.0;

const std::string MAP_CODE_READY_STATUS_KEY = "persist.multimedia.media_analysis_service.map_code_ready_status";

void PhotoMapCodeOperation::SetMapCodeReadyStatus(std::string status)
{
    int ret = system::SetParameter(MAP_CODE_READY_STATUS_KEY, status);
    CHECK_AND_RETURN_LOG(ret == E_OK, "SetMapCodeReadyStatus failed, ret: %{public}d", ret);
    MEDIA_INFO_LOG("SetMapCodeReadyStatus: %{public}s", status.c_str());
}

std::string PhotoMapCodeOperation::GetMapCodeReadyStatus()
{
    std::string status = OHOS::system::GetParameter(MAP_CODE_READY_STATUS_KEY, MAP_CODE_READY_STATUS_DEFAULT);
    return status;
}

int32_t PhotoMapCodeOperation::ExecSqlWithRetry(std::function<int32_t()> execSql)
{
    int32_t currentTime{0};
    int32_t err = NativeRdb::E_OK;
    while (currentTime < MAX_TRY_TIMES) {
        err = execSql();
        if (err == NativeRdb::E_OK) {
            break;
        } else if (err == NativeRdb::E_SQLITE_LOCKED || err == NativeRdb::E_DATABASE_BUSY ||
            err == NativeRdb::E_SQLITE_BUSY) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TRANSACTION_WAIT_INTERVAL));
            currentTime++;
            MEDIA_ERR_LOG("PhotoMapCodeOperation::ExecSqlWithRetry execSql busy, err: %{public}d, \
                currentTime: %{public}d", err, currentTime);
        } else {
            MEDIA_ERR_LOG("PhotoMapCodeOperation::ExecSqlWithRetry execSql failed, err: %{public}d, \
                currentTime: %{public}d", err, currentTime);
            break;
        }
    }
    return err;
}

int32_t PhotoMapCodeOperation::BuildMapValues(const std::vector<PhotoMapData> &photoMapDatas,
    vector<NativeRdb::ValuesBucket> &mapValues)
{
    for (const auto &photoMapData : photoMapDatas) {
        double longitude = photoMapData.longitude;
        double latitude = photoMapData.latitude;
        int32_t fileId = photoMapData.fileId;
        NativeRdb::ValuesBucket mapValue;
        if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
            fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
            mapValue.PutInt(PhotoMapCodeColumn::MAPCODE_FILE_ID, fileId);
            PhotoMapCodeOperation::GetPhotoMapCode(mapValue, latitude, longitude);
            mapValues.emplace_back(mapValue);
        }
    }
    return E_OK;
}

int32_t PhotoMapCodeOperation::BatchInsertMapCodes(const vector<NativeRdb::ValuesBucket> &mapValues,
    const std::shared_ptr<NativeRdb::RdbStore> rdbStore)
{
    if (mapValues.empty()) {
        return E_OK;
    }
    int64_t rowNum{0};
    int32_t ret = ExecSqlWithRetry([&]() {
        return rdbStore->BatchInsert(rowNum, PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE, mapValues);
    });
    return ret;
}

int32_t PhotoMapCodeOperation::BatchInsertMapCodes(const vector<NativeRdb::ValuesBucket> &mapValues,
    const std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (mapValues.empty()) {
        return E_OK;
    }
    int64_t rowNum{0};
    int32_t ret = rdbStore->BatchInsert(rowNum, PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE,
        const_cast<vector<NativeRdb::ValuesBucket>&>(mapValues));
    return ret;
}

int32_t PhotoMapCodeOperation::InsertPhotosMapCodes(const std::vector<PhotoMapData> &photoMapDatas,
    const std::shared_ptr<NativeRdb::RdbStore> cloneLibraryRdb)
{
    vector<NativeRdb::ValuesBucket> mapValues;
    BuildMapValues(photoMapDatas, mapValues);

    MEDIA_DEBUG_LOG("RestoreMapCodeUtils::InsertPhotosMapCodes mapValues size %{public}zu \
        photoMapDatas size %{public}zu", mapValues.size(), photoMapDatas.size());
    if (mapValues.empty()) {
        MEDIA_WARN_LOG("RestoreMapCodeUtils::InsertPhotosMapCodes mapValues.empty");
        return E_OK;
    }

    int32_t ret = E_RDB;
    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore) {
        ret = BatchInsertMapCodes(mapValues, rdbStore);
        MEDIA_DEBUG_LOG("RestoreMapCodeUtils::InsertPhotosMapCodes BatchInsert rdbStore ret %{public}d", ret);
    } else if (cloneLibraryRdb) {
        ret = BatchInsertMapCodes(mapValues, cloneLibraryRdb);
        MEDIA_DEBUG_LOG("RestoreMapCodeUtils::InsertPhotosMapCodes BatchInsert cloneLibraryRdb ret %{public}d", ret);
    } else {
        MEDIA_ERR_LOG("RestoreMapCodeUtils::InsertPhotosMapCodes BatchInsert rdbStore & cloneLibraryRdb both \
            null ret %{public}d", ret);
    }

    return ret;
}

int32_t PhotoMapCodeOperation::GetPhotosMapCodesMRS(const std::vector<PhotoMapData> &photoMapDatas,
    const std::shared_ptr<MediaLibraryRdbStore> store)
{
    vector<NativeRdb::ValuesBucket> mapValues;
    BuildMapValues(photoMapDatas, mapValues);

    MEDIA_DEBUG_LOG("RestoreMapCodeUtils::GetPhotosMapCodesMRS mapValues size %{public}zu \
        photoMapDatas size %{public}zu", mapValues.size(), photoMapDatas.size());
    if (mapValues.empty()) {
        MEDIA_WARN_LOG("RestoreMapCodeUtils::GetPhotosMapCodesMRS mapValues.empty");
        return E_OK;
    }

    int32_t ret = E_RDB;
    if (store) {
        ret = BatchInsertMapCodes(mapValues, store);
        MEDIA_DEBUG_LOG("RestoreMapCodeUtils::GetPhotosMapCodesMRS BatchInsert rdbStore ret %{public}d", ret);
    } else {
        MEDIA_ERR_LOG("RestoreMapCodeUtils::GetPhotosMapCodesMRS BatchInsert rdbStore both \
            null ret %{public}d", ret);
    }

    return ret;
}

int32_t PhotoMapCodeOperation::GetPhotoMapCode(const PhotoMapData &photoMapData, const PhotoMapType &photoMapType)
{
    if (photoMapData.fileId <= 0) {
        MEDIA_ERR_LOG("PhotoMapCodeOperation::GetPhotoMapCode fileId <= 0");
        return E_ERR;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (!rdbStore) {
        MEDIA_ERR_LOG("PhotoMapCodeOperation::GetPhotoMapCode failed rdbStore is null");
        return E_ERR;
    }

    int32_t fileId = photoMapData.fileId;
    if (photoMapType == PhotoMapType::QUERY_AND_INSERT) {
        const std::string QUERY_MAP_CODE_INFO = "SELECT file_id, cell_20 FROM tab_map_photo_map WHERE "
            + PhotoMapCodeColumn::MAPCODE_FILE_ID + " = " + to_string(fileId);
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MAP_CODE_INFO);
        int rowCount{-1};
        if (resultSet && resultSet->GetRowCount(rowCount) == NativeRdb::E_OK && rowCount > 0 &&
            resultSet->GoToFirstRow() == NativeRdb::E_OK) {
            MEDIA_DEBUG_LOG("GetPhotoMapCode success. Query MapCode info in table");
            resultSet->Close();
            return E_OK;
        }
        if (resultSet) {
            resultSet->Close();
        }
    }
    NativeRdb::ValuesBucket mapValue;
    PhotoMapCodeOperation::GetPhotoMapCode(mapValue, photoMapData.latitude, photoMapData.longitude);
    int32_t updateMapCount{0};
    std::string whereMapClause = PhotoMapCodeColumn::MAPCODE_FILE_ID + " = ?";
    std::vector<std::string> whereMapArgs = { to_string(fileId) };
    std::string mapTableName = PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE;
    int32_t result{-1};
    if (photoMapType == PhotoMapType::UPDATE_AND_INSERT) {
        result = rdbStore->Update(updateMapCount, mapTableName, mapValue, whereMapClause, whereMapArgs);
        MEDIA_DEBUG_LOG("GetPhotoMapCode Update result %{public}d updateMapCount %{public}d", result, updateMapCount);
    }
    if (result != NativeRdb::E_OK || updateMapCount == 0) {
        int64_t insertMapCount{-1};
        mapValue.PutInt(PhotoMapCodeColumn::MAPCODE_FILE_ID, fileId);
        result = rdbStore->Insert(insertMapCount, mapTableName,  mapValue);
        if (result < 0 || insertMapCount <= 0) {
            MEDIA_ERR_LOG("GetPhotoMapCode Ineset failed");
            return E_ERR;
        }
    }
    return result;
}

void PhotoMapCodeOperation::GetPhotoMapCode(NativeRdb::ValuesBucket &mapValue, double lat, double lon)
{
    std::vector<double> latAndlon = SetPoint(lat, lon);
    for (int level = LEVEL_START; level < LEVEL_COUNT; level++) {
        int64_t mapCode = GetMapCode(latAndlon, level);
        mapValue.PutLong("cell_" + std::to_string(level) + "int", mapCode);
    }
}

int32_t PhotoMapCodeOperation::UpgradePendingPhotoMapCodes(const std::shared_ptr<MediaLibraryRdbStore> store,
    int32_t batchSize)
{
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::UpgradePendingPhotoMapCodes start, batchSize: %{public}d", batchSize);
    if (!store) {
        MEDIA_ERR_LOG("UpgradePendingPhotoMapCodes failed. store is nullptr");
        return E_ERR;
    }

    const std::string sqlPendingData =
        "SELECT p.file_id, p.latitude, p.longitude FROM " + PhotoColumn::PHOTOS_TABLE + " p " +
        "LEFT JOIN " + PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE + " m ON p.file_id = m.file_id " +
        "WHERE m.file_id IS NULL AND p.latitude <> 0 AND p.longitude <> 0 " +
        "ORDER BY p.file_id ASC LIMIT " + to_string(batchSize);

    shared_ptr<NativeRdb::ResultSet> resultSet = store->QuerySql(sqlPendingData);
    int rowCount{-1};
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is nullptr!");
        return E_ERR;
    }
    if (resultSet->GetRowCount(rowCount) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpgradePendingPhotoMapCodes query data failed!");
        resultSet->Close();
        return E_ERR;
    }
    if (rowCount == 0) {
        MEDIA_INFO_LOG("UpgradePendingPhotoMapCodes no pending data.");
        resultSet->Close();
        return E_OK;
    }

    std::vector<Media::ORM::PhotosPo> photosPos;
    Media::ORM::ResultSetReader<Media::ORM::PhotosPoWriter, Media::ORM::PhotosPo>(resultSet).ReadRecords(photosPos);
    resultSet->Close();

    if (photosPos.empty()) {
        return E_OK;
    }

    std::vector<PhotoMapData> photoMapDatas;
    ConvertToPhotoMapData(photosPos, photoMapDatas);

    MEDIA_DEBUG_LOG("UpgradePendingPhotoMapCodes photoMapDatas size %{public}zu", photoMapDatas.size());
    if (!photoMapDatas.empty()) {
        GetPhotosMapCodesMRS(photoMapDatas, store);
    }

    return photoMapDatas.size();
}


vector<std::string> PhotoMapCodeOperation::FilterFileIds(const vector<std::string> &fileIds)
{
    MEDIA_INFO_LOG("PhotoMapCodeOperation::FilterFileIds fileIds size %{public}zu", fileIds.size());
    // 过滤出所有符合fileId格式的数据
    vector<std::string> filterFiles;
    for (auto it = fileIds.begin(); it != fileIds.end();) {
        std::string fileId = (*it);
        if (MediaLibraryDataManagerUtils::IsNumber(fileId)) {
            filterFiles.push_back(fileId);
        }
        ++it;
    }

    return filterFiles;
}

int32_t PhotoMapCodeOperation::RemovePhotosMapCodes(const std::vector<string> &fileIds)
{
    vector<std::string> filterFiles = FilterFileIds(fileIds);
    if (filterFiles.empty()) {
        MEDIA_ERR_LOG("PhotoMapCodeOperation::RemovePhotosMapCodes filterFiles is empty");
        return E_OK;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (!rdbStore) {
        MEDIA_ERR_LOG("PhotoMapCodeOperation::RemovePhotosMapCodes rdbStore is null");
        return E_ERR;
    }

    std::string mapTableName = PhotoMapCodeColumn::PHOTOS_MAP_CODE_TABLE;
    NativeRdb::RdbPredicates rdbPredicate(mapTableName);
    rdbPredicate.In(PhotoMapCodeColumn::MAPCODE_FILE_ID, filterFiles);
    int32_t rows = 0;
    int32_t ret = rdbStore->Delete(rows, rdbPredicate);
    return ret >= 0 ? E_OK : E_ERR;
}

int32_t PhotoMapCodeOperation::ConvertToPhotoMapData(const std::vector<Media::ORM::PhotosPo> &photosPos,
    int32_t &i, std::vector<PhotoMapData> &photoMapDatas, int32_t &detailCount)
{
    for (const auto &photoPo : photosPos) {
        if (!(photoPo.longitude.has_value() && photoPo.latitude.has_value() && photoPo.fileId.has_value())) {
            i++;
            continue;
        }
        double longitude = photoPo.longitude.value();
        double latitude = photoPo.latitude.value();
        int32_t fileId = photoPo.fileId.value();
        if (fileId > i) {
            i = fileId;
            detailCount++;
        }
        if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
            fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
            PhotoMapData photoMapData(fileId, latitude, longitude);
            photoMapDatas.emplace_back(photoMapData);
        }
    }
    return E_OK;
}

int32_t PhotoMapCodeOperation::ConvertToPhotoMapData(const std::vector<Media::ORM::PhotosPo> &photosPos,
    std::vector<PhotoMapData> &photoMapDatas)
{
    for (const auto &photoPo : photosPos) {
        if (!(photoPo.longitude.has_value() && photoPo.latitude.has_value() && photoPo.fileId.has_value())) {
            continue;
        }
        double longitude = photoPo.longitude.value();
        double latitude = photoPo.latitude.value();
        int32_t fileId = photoPo.fileId.value();
        if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
            fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
            PhotoMapData photoMapData(fileId, latitude, longitude);
            photoMapDatas.emplace_back(photoMapData);
        }
    }
    return E_OK;
}

std::vector<double> PhotoMapCodeOperation::SetPoint(double lat, double lon)
{
    std::vector<double> latAndlon;
    latAndlon.push_back(lat);
    latAndlon.push_back(lon);
    return latAndlon;
}

int64_t PhotoMapCodeOperation::GetMapCode(std::vector<double> &latAndLon, int level)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetMapCode");
    if (latAndLon.size() < POINT_SIZE) {
        return 0;
    }
    double lat = latAndLon[0];
    double lon = latAndLon[1];
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::GetMapCode level %{public}d", level);
    int64_t result = PhotoMapCodeOperation::GetMapHilbertCode(lat, lon, level);
    tracer.Finish();
    return result;
}

int64_t PhotoMapCodeOperation::GetMapHilbertCode(double lat, double lon, int level)
{
    double latPercent = (lat + 90) / 180.0;
    double lonPercent = (lon + 180) / 360.0;
    int64_t maxCoord = std::pow(2, std::floor(level));
    int64_t latPosition  = std::floor(latPercent * maxCoord) == maxCoord ?
        std::floor(latPercent * maxCoord) - 1 : std::floor(latPercent * maxCoord);
    int64_t lonPosition = std::floor(lonPercent * maxCoord) == maxCoord ?
        std::floor(lonPercent * maxCoord) - 1 : std::floor(lonPercent * maxCoord);

    return PhotoMapCodeOperation::DistanceFromPoint(latPosition, lonPosition, std::floor(level));
}

std::string PhotoMapCodeOperation::Int64ToBinaryWithPadding(int64_t num, int width)
{
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::Int64ToBinaryWithPadding num %{public}" PRId64
        ", width %{public}d", num, width);
    std::string binary = std::bitset<64>(num).to_string();
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::Int64ToBinaryWithPadding binary %{public}s", binary.c_str());

    binary.erase(0, binary.find_first_not_of('0'));
    if (binary.empty()) {
        binary = "0";
    }

    if (binary.length() < width) {
        binary = std::string(width - static_cast<int>(binary.length()), '0') + binary;
    }
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::Int64ToBinaryWithPadding binary %{public}s", binary.c_str());
    return binary;
}

void PhotoMapCodeOperation::UpdateZoomLevelStep(int64_t zoomLevel, std::vector<int64_t> &point)
{
    int64_t zoomLevelStep = zoomLevel;
    while (zoomLevelStep > 1) {
        int64_t zoomLevelCurrentValue = zoomLevelStep - 1;
        for (size_t i = 0; i < point.size(); i++) {
            if (point[i] & zoomLevelStep) {
                point[0] ^= zoomLevelCurrentValue;
            } else {
                int64_t flagValue = (point[0] ^ point[i]) & zoomLevelCurrentValue;
                point[0] ^= flagValue;
                point[i] ^= flagValue;
            }
        }
        zoomLevelStep >>= 1;
    }
    for (size_t i = 1; i < point.size(); i++) {
        point[i] ^= point[i - 1];
    }

    int64_t valueStep = 0;
    zoomLevelStep = zoomLevel;
    while (zoomLevelStep > 1) {
        if (point[point.size() - 1] & zoomLevelStep) {
            valueStep ^= zoomLevelStep - 1;
        }
        zoomLevelStep >>= 1;
    }

    for (size_t i = 0; i < point.size(); i++) {
        point[i] ^= valueStep;
    }
}

int64_t PhotoMapCodeOperation::DistanceFromPoint(int64_t latPosition, int64_t lonPosition, int level)
{
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::DistanceFromPoint latPosition %{public}" PRId64
        ", lonPosition %{public}" PRId64 " level %{public}d", latPosition, lonPosition, level);
    int64_t zoomLevel = 1 << (level - 1);
    std::vector<int64_t> point = {latPosition, lonPosition};

    UpdateZoomLevelStep(zoomLevel, point);

    std::vector<std::string> xBitStr;
    for (size_t i = 0; i < point.size(); i++) {
        int64_t value = point[i];
        std::string binaryStr = Int64ToBinaryWithPadding(value, level);
        xBitStr.push_back(binaryStr);
    }
    std::string codeStrCode = "";
    for (int i = 0; i < level; i++) {
        for (size_t y = 0; y < xBitStr.size(); y++) {
            codeStrCode += xBitStr[y][i];
        }
    }
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::DistanceFromPoint codeStrCode is %{public}s", codeStrCode.c_str());

    int64_t hilbertCode = std::bitset<64>(codeStrCode).to_ullong();
    MEDIA_DEBUG_LOG("PhotoMapCodeOperation::DistanceFromPoint hilbertCode is %{public}" PRId64, hilbertCode);

    return hilbertCode;
}
} // namespace Media
} // namespace OHOS
