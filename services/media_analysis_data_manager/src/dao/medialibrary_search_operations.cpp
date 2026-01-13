/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "SearchOperation"

#include "medialibrary_search_operations.h"

#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "search_column.h"
#include "vision_total_column.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const std::string notTrashedAndHiddenCondition = MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " +
        MediaColumn::MEDIA_HIDDEN + " = 0 AND " + MediaColumn::MEDIA_TIME_PENDING + " = 0 AND " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = 0 AND " + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 AND ";
const std::string analysisCompleteCondition = "(" + TBL_SEARCH_CV_STATUS + " = 1 AND (" + OCR + " != 0 AND " + FACE +
    " NOT IN (0,1,2) OR " + MediaColumn::MEDIA_TYPE + " = 2) AND " + LABEL + " != 0) AND (" + TBL_SEARCH_GEO_STATUS +
    " = 1 OR (" + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE + " = 0 AND " +
    PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE + " = 0) OR (" +
    PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE + " IS NULL) OR (" +
    PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LONGITUDE + " IS NULL))";
const std::string selectAnalysisCompletedPhoto = "SELECT COUNT(case when " + notTrashedAndHiddenCondition +
    TBL_SEARCH_PHOTO_STATUS + " > 1 AND " + MediaColumn::MEDIA_TYPE + " = 1 AND " + analysisCompleteCondition +
    " then 1 end) as " + PHOTO_COMPLETE_NUM + ",";
const std::string mediaPhotoTotal = "COUNT(case when " + notTrashedAndHiddenCondition + MediaColumn::MEDIA_TYPE +
    " = 1 then 1 end) as " + PHOTO_TOTAL_NUM + ",";
const std::string selectAnalysisCompletedVideo = "COUNT(case when " + notTrashedAndHiddenCondition +
    TBL_SEARCH_PHOTO_STATUS + " > 1 AND " + MediaColumn::MEDIA_TYPE + " = 2 AND " + analysisCompleteCondition +
    " then 1 end) as " + VIDEO_COMPLETE_NUM + ",";
const std::string mediaVideoTotal = "COUNT(case when " + notTrashedAndHiddenCondition + MediaColumn::MEDIA_TYPE +
    " = 2 then 1 end) as " + VIDEO_TOTAL_NUM;
const std::string mediaPhotosQuery = selectAnalysisCompletedPhoto + mediaPhotoTotal + selectAnalysisCompletedVideo +
    mediaVideoTotal + " FROM " + PhotoColumn::PHOTOS_TABLE + " Inner JOIN " + SEARCH_TOTAL_TABLE + " ON " +
    PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + "=" + SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID +
    " Inner JOIN " + VISION_TOTAL_TABLE + " ON " + SEARCH_TOTAL_TABLE + "." + TBL_SEARCH_FILE_ID + "=" +
    VISION_TOTAL_TABLE + "." + MediaColumn::MEDIA_ID;

int32_t MediaLibrarySearchOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int64_t outRowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK || outRowId < 0) {
        MEDIA_ERR_LOG("Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outRowId);
}

int32_t MediaLibrarySearchOperations::UpdateOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(cmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(updateRows);
}

int32_t MediaLibrarySearchOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    if (errCode != NativeRdb::E_OK || deleteRows < 0) {
        MEDIA_ERR_LOG("Delete db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(deleteRows);
}

shared_ptr<NativeRdb::ResultSet> MediaLibrarySearchOperations::QueryOperation(MediaLibraryCommand &cmd,
    const std::vector<std::string> &columns)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(cmd, columns);
}

shared_ptr<ResultSet> MediaLibrarySearchOperations::QueryIndexConstructProgress()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr!");
        return nullptr;
    }

    return rdbStore->QuerySql(mediaPhotosQuery);
}
}
}
