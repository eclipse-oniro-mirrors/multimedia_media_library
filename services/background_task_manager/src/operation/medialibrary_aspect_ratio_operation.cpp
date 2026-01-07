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

#include <fcntl.h>
#include <sys/stat.h>

#include "medialibrary_aspect_ratio_operation.h"

#include "directory_ex.h"

#include "abs_rdb_predicates.h"
#include "cmath"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "result_set_utils.h"
#include "scanner_utils.h"
#include "values_bucket.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const std::int32_t BATCH_SIZE = 500;
const std::string SQL_PHOTOS_TABLE_UNFILLED_ASPECT_RATIO_COUNT = "SELECT"
                                                                " COUNT( * ) AS Count "
                                                                "FROM"
                                                                " Photos "
                                                                "WHERE"
                                                                " aspect_ratio = -2;";
const std::string SQL_PHOTOS_TABLE_ASSET_SIZE = "SELECT"
                                                " file_id, width, height "
                                                "FROM"
                                                " Photos "
                                                "WHERE"
                                                " aspect_ratio = -2"
                                                " LIMIT ?;";

std::atomic<bool> MediaLibraryAspectRatioOperation::isContinue_{true};

void MediaLibraryAspectRatioOperation::Stop()
{
    isContinue_.store(false);
}

std::vector<AssetAspectRatio> MediaLibraryAspectRatioOperation::GetUnfilledValues()
{
    std::vector<AssetAspectRatio> assetInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, assetInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {BATCH_SIZE};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_ASSET_SIZE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, assetInfos, "resultSet is null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_DEBUG_LOG("resultSet count is 0");
        resultSet->Close();
        return assetInfos;
    }

    do {
        AssetAspectRatio assetInfo;
        int32_t width =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_WIDTH, resultSet, TYPE_INT32));
        int32_t height =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_HEIGHT, resultSet, TYPE_INT32));
        assetInfo.fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        assetInfo.aspect_ratio = MediaFileUtils::CalculateAspectRatio(height, width);
        assetInfos.push_back(assetInfo);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load() &&
        resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return assetInfos;
}

int32_t MediaLibraryAspectRatioOperation::QueryUnfilledValueCount()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_UNFILLED_ASPECT_RATIO_COUNT);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "resultSet is null");
    int32_t count = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        count = get<int32_t>(ResultSetUtils::GetValFromColumn("Count", resultSet, TYPE_INT32));
    } else {
        MEDIA_DEBUG_LOG("No assets which aspect_ratio = -2.");
    }
    resultSet->Close();
    return count;
}

void MediaLibraryAspectRatioOperation::UpdateAspectRatioValue()
{
    isContinue_.store(true);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    while (MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load()) {
        CHECK_AND_BREAK_INFO_LOG(QueryUnfilledValueCount() > 0,
            "No unfilled aspect_ratio value need to handle");
        MEDIA_INFO_LOG("handle unfilled aspect_ratio value");
        std::vector<AssetAspectRatio> assetInfos = GetUnfilledValues();
        HandleAspectRatio(assetInfos);
    }
    MEDIA_INFO_LOG("end handle unfilled aspect_ratio value asset! cost: %{public}" PRId64,
        MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return;
}

void MediaLibraryAspectRatioOperation::HandleAspectRatio(const std::vector<AssetAspectRatio> &assetInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    for (const AssetAspectRatio &assetInfo : assetInfos) {
        CHECK_AND_BREAK_INFO_LOG(MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load(),
            "current status is off, break");
        std::string updat_sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
                                " SET " + PhotoColumn::PHOTO_ASPECT_RATIO + " = " + to_string(assetInfo.aspect_ratio) +
                                " WHERE " + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID + " = " +
                                to_string(assetInfo.fileId) + ";";
        int ret = rdbStore->ExecuteSql(updat_sql);
        CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "Execute update ASPECT_RATIO sql failed");
    }
}
}  // namespace OHOS::Media