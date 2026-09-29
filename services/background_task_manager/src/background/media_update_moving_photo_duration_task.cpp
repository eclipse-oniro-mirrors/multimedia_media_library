/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Background"

#include "media_update_moving_photo_duration_task.h"

#include "rdb_predicates.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_edit_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "thumbnail_service.h"
#include "medialibrary_db_const.h"
#include "result_set_utils.h"
#include "moving_photo_file_utils.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
static const int32_t DURATION_PROCESS_NUM = 100;
static const int32_t INVALID_DURATION = -1;
static const int32_t MAX_ITERATION = 500;


bool MediaUpdateMovingPhotoDurationTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaUpdateMovingPhotoDurationTask::Execute()
{
    this->UpdateMovingPhotoDuration();
    return;
}

std::shared_ptr<NativeRdb::ResultSet> MediaUpdateMovingPhotoDurationTask::QueryInvalidDurationFiles()
{
    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    predicates.EqualTo(PhotoColumn::MEDIA_DURATION, 0);
    predicates.NotEqualTo(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    predicates.Or();
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    predicates.EndWrap();
    predicates.OrderByDesc(PhotoColumn::MEDIA_ID);
    predicates.Limit(DURATION_PROCESS_NUM);
    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

bool MediaUpdateMovingPhotoDurationTask::ParseUpdateFilesList(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::vector<UpdateDurationFileInfo> &updateFilesList)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "UpdateMovingPhotoDuration resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId =
            std::get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        std::string filePath =
            std::get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet,
            TYPE_STRING));
        updateFilesList.emplace_back(UpdateDurationFileInfo{
            .id = fileId,
            .path = filePath
        });
        MEDIA_DEBUG_LOG("UpdateMovingPhotoDuration handle file id %{public}d", fileId);
    }
    return true;
}

bool MediaUpdateMovingPhotoDurationTask::ProcessDuration(const std::vector<UpdateDurationFileInfo> &updateFilesList)
{
    MEDIA_INFO_LOG("UpdateMovingPhotoDuration process duration start, num: %{public}zu", updateFilesList.size());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr!");

    for (const auto& file : updateFilesList) {
        std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(file.path);
        int32_t duration = MovingPhotoFileUtils::GetMovingPhotoVideoDuration(videoPath);
        if (duration <= 0) {
            MEDIA_ERR_LOG("Get duration failed or invalid duration of moving photo video: %{public}d ms", duration);
            duration = INVALID_DURATION;
        }
        AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, file.id);
        ValuesBucket values;
        values.PutInt(PhotoColumn::MEDIA_DURATION, duration);

        int32_t changeRows = -1;
        int32_t ret = rdbStore->Update(changeRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), false,
            "failed to update duration, ret = %{public}d", ret);
        MEDIA_DEBUG_LOG("UpdateMovingPhotoDuration process duration done id: %{public}d, path: %{public}s", file.id,
            DfxUtils::GetSafePath(file.path).c_str());
    }
    return true;
}

void MediaUpdateMovingPhotoDurationTask::UpdateMovingPhotoDuration()
{
    MEDIA_INFO_LOG("UpdateMovingPhotoDuration start");
    int32_t iteration = 0;
    while (iteration < MAX_ITERATION) {
        iteration++;

        if (!this->Accept()) {
            MEDIA_INFO_LOG("UpdateMovingPhotoDuration check condition failed End");
            return;
        }

        auto resultSet = QueryInvalidDurationFiles();
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "UpdateMovingPhotoDuration query failed End");

        std::vector<UpdateDurationFileInfo> updateFilesList;
        bool ret = ParseUpdateFilesList(resultSet, updateFilesList);
        CHECK_AND_RETURN_LOG(ret, "UpdateMovingPhotoDuration parse files list failed End");

        if (updateFilesList.empty()) {
            MEDIA_INFO_LOG("UpdateMovingPhotoDuration End, No more files to update");
            return;
        }

        ret = ProcessDuration(updateFilesList);
        CHECK_AND_RETURN_LOG(ret, "UpdateMovingPhotoDuration process duration failed End");
    }
    MEDIA_INFO_LOG("UpdateMovingPhotoDuration reach max iteration End");
}
}  // namespace OHOS::Media::Background