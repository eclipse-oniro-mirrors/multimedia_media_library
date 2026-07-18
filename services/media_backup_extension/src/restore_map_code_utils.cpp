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
#define RESTORE_MAP_LOG_TAG "RestoreMapCode"

#include "restore_map_code_utils.h"
#include "media_column.h"
#include "photo_map_code_operation.h"

#include "medialibrary_rdbstore.h"
#include "media_log.h"
#include "medialibrary_type_const.h"

#include "application_context.h"
#include "backup_database_utils.h"
#include "backup_dfx_utils.h"
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#include "cloud_sync_manager.h"
#include "cloud_sync_utils.h"
#include "directory_ex.h"
#include "extension_context.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "media_scanner_manager.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_type_const.h"
#include "medialibrary_errno.h"
#include "moving_photo_file_utils.h"
#include <nlohmann/json.hpp>
#include "parameters.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "userfilemgr_uri.h"
#include "medialibrary_notify.h"
#include "upgrade_restore_task_report.h"
#include "medialibrary_rdb_transaction.h"
#include "database_report.h"
#include "ohos_account_kits.h"

#include <cerrno>
#include <fstream>
#include <iostream>
#include <bitset>
#include <string>
#include <cmath>

namespace OHOS {
namespace Media {
using namespace std;
static constexpr double DOUBLE_EPSILON = 1e-15;
static constexpr double MAX_LATITUDE_EPSILON = 1e-15 + 90.0;
static constexpr double MAX_LONGITUDE_EPSILON = 1e-15 + 180.0;

#define SQL_GET_UNMATCH_MAP_CODE_FILES \
    "SELECT p.file_id FROM photos p" \
    " LEFT JOIN tab_map_photo_map m ON p.file_id = m.file_id" \
    " WHERE p.latitude IS NOT NULL AND p.longitude IS NOT NULL" \
    " AND p.latitude <> 0 AND p.longitude <> 0 AND m.file_id IS NULL;"

int32_t RestoreMapCodeUtils::GetNotReadyPhotoCount(const std::shared_ptr<NativeRdb::RdbStore> store)
{
    CHECK_AND_RETURN_RET_LOG(store != nullptr, E_ERR, "store is nullptr");
    shared_ptr<NativeRdb::ResultSet> resultSet = store->QuerySql(SQL_GET_UNMATCH_MAP_CODE_FILES);
    int rowCount{-1};
    if (resultSet == nullptr)
    {
        MEDIA_ERR_LOG("resultSet is nullptr!");
        return E_ERR;
    }
    if (resultSet->GetRowCount(rowCount) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetNotReadyPhotoCount query data failed!");
        resultSet->Close();
        return E_ERR;
    }
    resultSet->Close();
    MEDIA_INFO_LOG("Phot Map Code Not Ready Count: %{public}d", rowCount);
    return rowCount;
}

int32_t RestoreMapCodeUtils::ReverseFileInfosToMapCode(const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const vector<FileInfo> &fileInfos)
{
    MEDIA_DEBUG_LOG("RestoreMapCodeUtils::ReverseFileInfosToMapCode fileInfos size %{public}zu", fileInfos.size());
    vector<PhotoMapData> photoMapDatas;
    int64_t rowNum = 0;
    for (const auto &fileInfo : fileInfos) {
        double longitude = fileInfo.longitude;
        double latitude = fileInfo.latitude;
        int32_t fileId = fileInfo.fileIdOld;
        if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
            fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
            PhotoMapData photoMapData(fileId, latitude, longitude);
            photoMapDatas.emplace_back(photoMapData);
        }
    }
 
    return PhotoMapCodeOperation::InsertPhotosMapCodes(photoMapDatas, mediaLibraryRdb);
}

int32_t RestoreMapCodeUtils::FileInfosToMapCode(const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
    const vector<FileInfo> &fileInfos)
{
    MEDIA_DEBUG_LOG("RestoreMapCodeUtils::FileInfosToMapCode fileInfos size %{public}zu", fileInfos.size());
    vector<PhotoMapData> photoMapDatas;
    int64_t rowNum = 0;
    for (const auto &fileInfo : fileInfos) {
        double longitude = fileInfo.longitude;
        double latitude = fileInfo.latitude;
        int32_t fileId = fileInfo.fileIdNew;
        if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
            fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
            PhotoMapData photoMapData(fileId, latitude, longitude);
            photoMapDatas.emplace_back(photoMapData);
        }
    }

    return PhotoMapCodeOperation::InsertPhotosMapCodes(photoMapDatas, mediaLibraryRdb);
}

int32_t RestoreMapCodeUtils::FileInfoToMapCode(const FileInfo &fileInfo,
    const std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb)
{
    MEDIA_DEBUG_LOG("RestoreMapCodeUtils::FileInfoToMapCode");
    vector<PhotoMapData> photoMapDatas;
    int64_t rowNum = 0;
    double longitude = fileInfo.longitude;
    double latitude = fileInfo.latitude;
    int32_t fileId = fileInfo.fileIdNew;
    if (fileId > 0 && fabs(longitude) > DOUBLE_EPSILON && fabs(latitude) > DOUBLE_EPSILON &&
        fabs(longitude) < MAX_LONGITUDE_EPSILON && fabs(latitude) < MAX_LATITUDE_EPSILON) {
        PhotoMapData photoMapData(fileId, latitude, longitude);
        photoMapDatas.emplace_back(photoMapData);
    }

    return PhotoMapCodeOperation::InsertPhotosMapCodes(photoMapDatas, mediaLibraryRdb);
}

int64_t RestoreMapCodeUtils::DeleteMapCodesByFileIds(const vector<string> &fileIds)
{
    return PhotoMapCodeOperation::RemovePhotosMapCodes(fileIds);
}
} // namespace Media
} // namespace OHOS
