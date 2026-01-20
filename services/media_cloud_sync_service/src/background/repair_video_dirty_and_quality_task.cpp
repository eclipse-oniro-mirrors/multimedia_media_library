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

#define MLOG_TAG "Media_Background"

#include "repair_video_dirty_and_quality_task.h"

#include "abs_rdb_predicates.h"
#include "cloud_media_dao_utils.h"
#include "datashare_abs_result_set.h"
#include "media_log.h"
#include "media_column.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "power_efficiency_manager.h"
#include "result_set_utils.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media::Background {
// LCOV_EXCL_START
bool RepairVideoDirtyAndQualityTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void RepairVideoDirtyAndQualityTask::Execute()
{
    this->HandleRepairVideoDirtyAndQuality();
    return;
}

int32_t RepairVideoDirtyAndQualityTask::UpdateVideoDirtyAndQuality(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<std::string> &fileIdVec)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, fileIdVec);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_VIDEO));
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, -1);
    predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    predicates.NotEqualTo(PhotoColumn::PHOTO_SUBTYPE, CAMERA_SUBTYPE);

    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));

    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), E_ERR,
        "Failed to UpdateVideoDirtyAndQuality, ret: %{public}d, changeRows: %{public}d", ret, changeRows);
    MEDIA_INFO_LOG("UpdateVideoDirtyAndQuality success, changeRows: %{public}d.", changeRows);
    return ret;
}

int32_t RepairVideoDirtyAndQualityTask::HandleRepairVideoDirtyAndQuality()
{
    MEDIA_INFO_LOG("Begin HandleRepairVideoDirtyAndQuality");
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");

    std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_VIDEO));
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, -1);
    predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    predicates.NotEqualTo(PhotoColumn::PHOTO_SUBTYPE, CAMERA_SUBTYPE);
    predicates.Limit(BATCH_QUERY_NUMBER);

    bool nextUpdate = true;
    while (nextUpdate && Accept()) {
        auto resultSet = rdbStore->Query(predicates, columns);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "resultSet is nullptr");

        int32_t rowCount = 0;
        int32_t ret = resultSet->GetRowCount(rowCount);
        CHECK_AND_RETURN_RET_LOG((ret == E_OK && rowCount >= 0), E_ERR, "Failed to query resultSet.");

        if (rowCount == 0) {
            MEDIA_ERR_LOG("No need to HandleRepairVideoDirtyAndQuality");
            return E_OK;
        }
        if (rowCount < BATCH_QUERY_NUMBER) {
            nextUpdate = false;
        }

        std::vector<std::string> fileIdVec;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
            fileIdVec.push_back(fileId);
        }
        resultSet->Close();

        CHECK_AND_RETURN_RET_LOG(UpdateVideoDirtyAndQuality(rdbStore, fileIdVec) == E_OK,
            E_ERR, "Failed to UpdateVideoDirtyAndQuality.");
    }
    MEDIA_INFO_LOG("End HandleRepairVideoDirtyAndQuality");
    return E_OK;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::Background