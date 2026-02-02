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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_enhance_dao.h"

#include <string>
#include <utime.h>
#include <vector>

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "result_set.h"
#include "result_set_utils.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaEnhanceDao::GetCloudSyncUnPreparedDataCount(int32_t &result)
{
    MEDIA_INFO_LOG("enter GetCloudSyncUnPreparedDataCount");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_QUALITY,
        std::to_string(static_cast<int32_t>(MultiStagesPhotoQuality::LOW)));
    queryPredicates.EqualTo(MediaColumn::MEDIA_TYPE,
        std::to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)));
    queryPredicates.And()->IsNotNull(PhotoColumn::PHOTO_ID);
    vector<string> queryColums = {"COUNT(1) AS count"};
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "resultSet is null");
    bool isValid = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_EXECUTE(isValid, resultSet->Close());
    CHECK_AND_RETURN_RET_LOG(isValid, E_RDB, "failed to get row");
    result = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("GetCloudSyncUnPreparedDataCount end %{public}d", result);
    return E_OK;
}

std::tuple<std::string, std::string> CloudMediaEnhanceDao::GetNextUnPreparedData()
{
    MEDIA_INFO_LOG("enter GetNextUnPreparedData");
    auto ret = std::make_tuple<std::string, std::string>("", "");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ret, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_QUALITY,
        std::to_string(static_cast<int32_t>(MultiStagesPhotoQuality::LOW)));
    queryPredicates.EqualTo(MediaColumn::MEDIA_TYPE,
        std::to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)));
    queryPredicates.And()->IsNotNull(PhotoColumn::PHOTO_ID);
    queryPredicates.Limit(1);
    vector<string> queryColums = { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ID };
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, ret, "Failed to query.");

    int32_t rowCount = 0;
    int32_t retRow = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG((retRow == 0 && rowCount >= 0), ret, "Failed to Get Count.");

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::get<0>(ret) = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        std::get<1>(ret) = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    }
    resultSet->Close();
    
    return ret;
}
}  // namespace OHOS::Media::CloudSync