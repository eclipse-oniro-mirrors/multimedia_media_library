/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureDfxFirstVisit"

#include "multistages_capture_dfx_first_visit.h"

#include <memory>

#include "database_adapter.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager_utils.h"
#include "post_event_utils.h"
#include "result_set_utils.h"
#include "values_bucket.h"

using namespace std;

namespace OHOS {
namespace Media {
MultiStagesCaptureDfxFirstVisit::MultiStagesCaptureDfxFirstVisit() {}

MultiStagesCaptureDfxFirstVisit::~MultiStagesCaptureDfxFirstVisit() {}

MultiStagesCaptureDfxFirstVisit& MultiStagesCaptureDfxFirstVisit::GetInstance()
{
    static MultiStagesCaptureDfxFirstVisit instance;
    return instance;
}

static void ReportInternal(AsyncTaskData *taskData)
{
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("taskData is nullptr");
        return;
    }
    FirstVisitAsyncTaskData *task = static_cast<FirstVisitAsyncTaskData*>(taskData);
    if (task == nullptr) {
        MEDIA_ERR_LOG("task is nullptr");
        return;
    }

    VariantMap map = {{KEY_PHOTO_ID, task->photoId_}, {KEY_TIME_INTERVAL, task->visitTime_ - task->startTime_}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_FIRST_VISIT_STAT, map);

    // update first_visit_time in Photos table
    NativeRdb::ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_FIRST_VISIT_TIME, task->visitTime_);
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, values);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, task->fileId_);
    auto result = DatabaseAdapter::Update(updateCmd);
    MEDIA_INFO_LOG("ReportInternal exit result: %{public}d", result);
}

void MultiStagesCaptureDfxFirstVisit::Report(const string &photoId, const int32_t fileId)
{
    if (photoId.empty() || fileId <= 0) {
        MEDIA_INFO_LOG("Report photoId is empty or fileId is invalid: %{public}s, %{public}d",
            photoId.c_str(), fileId);
        return;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_FIRST_VISIT_TIME,
        PhotoColumn::PHOTO_LAST_VISIT_TIME };
    
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        MEDIA_INFO_LOG("result set is empty");
        return;
    }

    int64_t firstVisitTime = GetInt64Val(PhotoColumn::PHOTO_FIRST_VISIT_TIME, resultSet);
    if (firstVisitTime > 0) {
        // had reported, do not need to report again
        return;
    }

    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_INFO_LOG("can not get async worker");
        return;
    }

    int64_t lastVisitTime = GetInt64Val(PhotoColumn::PHOTO_LAST_VISIT_TIME, resultSet);
    FirstVisitAsyncTaskData *taskData = new (std::nothrow) FirstVisitAsyncTaskData(fileId, photoId, lastVisitTime,
        MediaFileUtils::UTCTimeMilliSeconds());
    shared_ptr<MediaLibraryAsyncTask> asyncTask = make_shared<MediaLibraryAsyncTask>(ReportInternal, taskData);
    if (asyncTask == nullptr) {
        MEDIA_INFO_LOG("report first visit failed");
        return;
    }

    asyncWorker->AddTask(asyncTask, true);
}

} // namespace Media
} // namespace OHOS