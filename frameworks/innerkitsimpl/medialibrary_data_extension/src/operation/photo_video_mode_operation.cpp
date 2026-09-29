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
#define MLOG_TAG "PhotoVideoModeOperation"
#include "photo_video_mode_operation.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
namespace OHOS::Media {
int32_t PhotoVideoModeOperation::BatchUpdatePhotosVideoMode(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
                                                            const std::vector<std::string> &logFileIds)
{
    if (logFileIds.empty()) {
        MEDIA_DEBUG_LOG("BatchUpdatePhotosVideoMode has no data need to update.");
        return E_OK;
    }
    MEDIA_INFO_LOG("logFileIds size = %{public}d", static_cast<int32_t>(logFileIds.size()));
    NativeRdb::ValuesBucket updateLogPostBucket;
    updateLogPostBucket.Put(PhotoColumn::PHOTO_VIDEO_MODE, static_cast<int32_t>(VideoMode::LOG_VIDEO));
    NativeRdb::AbsRdbPredicates updateLogPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updateLogPredicates.In(MediaColumn::MEDIA_ID, logFileIds);
    int32_t changeRows = -1;
    int32_t logret = rdbStore->Update(changeRows, updateLogPostBucket, updateLogPredicates);
    CHECK_AND_RETURN_RET_LOG((logret == E_OK && changeRows > 0), E_FAIL,
                             "BatchUpdatePhotosVideoMode failed, logret: %{public}d, updateRows: %{public}d", logret,
                             changeRows);
    return E_OK;
}

int32_t PhotoVideoModeOperation::UpdatePhotosVideoMode(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
                                                       const int32_t videoMode, const int32_t fileId)
{
    MEDIA_INFO_LOG("UpdatePhotosVideoMode: videoMode=%{public}d, fileId=%{public}d", videoMode, fileId);
    std::string fileIdStr = std::to_string(fileId);
    std::vector<std::string> fileIds = {fileIdStr};
    NativeRdb::ValuesBucket updatePostBucket;
    updatePostBucket.Put(PhotoColumn::PHOTO_VIDEO_MODE, videoMode);
    NativeRdb::AbsRdbPredicates updatePredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, updatePostBucket, updatePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), E_FAIL,
                             "Failed to UpdatePhotosLog, ret: %{public}d, updateRows: %{public}d", ret, changeRows);
    return E_OK;
}
 
int32_t PhotoVideoModeOperation::GetMaxFileId(std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    std::string QUERY_MAX_FILE_ID = "SELECT MAX(file_id) as last_id FROM Photos";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_MAX_FILE_ID);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Query not match data fails");
    int columnIndex =  0;
    int32_t maxFileId = -1;
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_FAIL,
        "ResultSet go to first row fails");
    resultSet->GetInt(columnIndex, maxFileId);
    MEDIA_INFO_LOG("PhotoCustomRestoreOperation::BatchInsert fileId = %{public}d", maxFileId);
    return maxFileId;
}
}