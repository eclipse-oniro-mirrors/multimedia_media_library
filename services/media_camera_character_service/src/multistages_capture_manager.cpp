/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureManager"

#include "multistages_capture_manager.h"


#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "medialibrary_tracer.h"

using namespace std;

namespace OHOS {
namespace Media {

MultiStagesCaptureManager::MultiStagesCaptureManager() {}

MultiStagesCaptureManager::~MultiStagesCaptureManager() {}

void MultiStagesCaptureManager::RemovePhotos(const NativeRdb::AbsRdbPredicates &predicates,
    bool isRestorable)
{
    MEDIA_INFO_LOG("Remove photos enter, isRestorable is: %{public}d", isRestorable);
    if (predicates.GetTableName() != PhotoColumn::PHOTOS_TABLE) {
        MEDIA_INFO_LOG("Invalid table name: %{public}s", predicates.GetTableName().c_str());
        return;
    }

    NativeRdb::AbsRdbPredicates predicatesNew(predicates.GetTableName());
    string where = predicates.GetWhereClause() + " AND (" + PhotoColumn::PHOTO_QUALITY + "=" +
        to_string(static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) + " OR " +
        PhotoColumn::STAGE_VIDEO_TASK_STATUS + " = " +
        to_string(static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) + ")";

    predicatesNew.SetWhereClause(where);
    predicatesNew.SetWhereArgs(predicates.GetWhereArgs());
    vector<string> columns { MediaColumn::MEDIA_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_PHOTO_QUALITY,
        MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicatesNew, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("Result set is empty");
        return;
    }

    do {
        string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
        int32_t stageVideoTaskStatus = GetInt32Val(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, resultSet);
        // Moving photo remove video task
        if (stageVideoTaskStatus == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) {
            MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(photoId, isRestorable);
            continue;
        }

        int32_t photoQuality = GetInt32Val(MEDIA_DATA_DB_PHOTO_QUALITY, resultSet);
        if (photoId.empty() || photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
            MEDIA_DEBUG_LOG("photoId is empty or task status invalid ");
            continue;
        }

        int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, resultSet);
        switch (mediaType) {
            case MediaType::MEDIA_TYPE_IMAGE:
                MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(photoId, isRestorable);
                break;
            case MediaType::MEDIA_TYPE_VIDEO:
                MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(photoId, isRestorable);
                break;
            default:
                break;
        }
    } while (!resultSet->GoToNextRow());
}

void MultiStagesCaptureManager::RemovePhotosWithResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    bool isRestorable)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Result set is empty");
        return;
    }
    string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
    int32_t stageVideoTaskStatus = GetInt32Val(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, resultSet);
    // Moving photo remove video task
    if (stageVideoTaskStatus == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) {
        string mediaFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        int32_t photoSubType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(photoId, mediaFilePath, photoSubType,
            isRestorable);
        return;
    }
    int32_t photoQuality = GetInt32Val(MEDIA_DATA_DB_PHOTO_QUALITY, resultSet);
    if (photoId.empty() || photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
        MEDIA_DEBUG_LOG("photoId is empty or task status invalid ");
        return;
    }
    int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, resultSet);
    switch (mediaType) {
        case MediaType::MEDIA_TYPE_IMAGE:
            MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(photoId, isRestorable);
            break;
        case MediaType::MEDIA_TYPE_VIDEO:
            {
                string mediaFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
                int32_t photoSubType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
                MultiStagesVideoCaptureManager::GetInstance()
                    .RemoveVideo(photoId, mediaFilePath, photoSubType, isRestorable);
            }
            break;
        default:
            break;
    }
}

void MultiStagesCaptureManager::RestorePhotos(const NativeRdb::AbsRdbPredicates &predicates)
{
    MEDIA_INFO_LOG("Restore photos enter");
    if (predicates.GetTableName() != PhotoColumn::PHOTOS_TABLE) {
        MEDIA_INFO_LOG("Invalid table name: %{public}s", predicates.GetTableName().c_str());
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureManager::RestorePhotos");
    NativeRdb::AbsRdbPredicates predicatesNew(predicates.GetTableName());
    string where = predicates.GetWhereClause() + " AND " + PhotoColumn::PHOTO_QUALITY + "=" +
        to_string(static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    predicatesNew.SetWhereClause(where);
    predicatesNew.SetWhereArgs(predicates.GetWhereArgs());
    vector<string> columns { MediaColumn::MEDIA_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_PHOTO_QUALITY,
        MEDIA_DATA_DB_MEDIA_TYPE };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicatesNew, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_INFO_LOG(!cond, "Result set is empty");

    do {
        string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
        int32_t photoQuality = GetInt32Val(MEDIA_DATA_DB_PHOTO_QUALITY, resultSet);
        if (photoId.empty() || photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
            MEDIA_DEBUG_LOG("photoId is empty or full quality ");
            continue;
        }

        int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, resultSet);
        switch (mediaType) {
            case MediaType::MEDIA_TYPE_IMAGE:
                MultiStagesPhotoCaptureManager::GetInstance().RestoreImage(photoId);
                break;
            case MediaType::MEDIA_TYPE_VIDEO:
                MultiStagesVideoCaptureManager::GetInstance().RestoreVideo(photoId);
                break;
            default:
                break;
        }
    } while (!resultSet->GoToNextRow());
}

} // Media
} // OHOS