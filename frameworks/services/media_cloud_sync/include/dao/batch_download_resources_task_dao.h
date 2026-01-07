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

#ifndef OHOS_MEDIA_BATCH_DOWNLOAD_RESOURCE_TASK_DAO_H
#define OHOS_MEDIA_BATCH_DOWNLOAD_RESOURCE_TASK_DAO_H

#include <string>
#include <vector>
#include <map>

#include "download_resources_po.h"
#include "abs_rdb_predicates.h"
#include "value_object.h"
#include "medialibrary_rdb_transaction.h"
#include "datashare_predicates.h"
#include "rdb_store.h"
#include "media_column.h"
#include "download_resources_column.h"

namespace OHOS {
namespace Media {
using namespace OHOS::Media::ORM;
#define EXPORT __attribute__ ((visibility ("default")))

const std::vector<std::string> PULL_QUERY_DOWNLOAD_COLUMNS = {
    PhotoColumn::MEDIA_ID,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::MEDIA_SIZE,
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
};

const std::vector<std::string> PULL_QUERY_DOWNLOAD_STATUS_COLUMNS = {
    DownloadResourcesColumn::MEDIA_ID,
    DownloadResourcesColumn::MEDIA_NAME,
    DownloadResourcesColumn::MEDIA_SIZE,
    DownloadResourcesColumn::MEDIA_URI,
    DownloadResourcesColumn::MEDIA_DATE_ADDED,
    DownloadResourcesColumn::MEDIA_DATE_FINISH,
    DownloadResourcesColumn::MEDIA_DOWNLOAD_STATUS,
    DownloadResourcesColumn::MEDIA_PERCENT,
    DownloadResourcesColumn::MEDIA_AUTO_PAUSE_REASON,
    DownloadResourcesColumn::MEDIA_COVER_LEVEL,
};

class BatchDownloadResourcesTaskDao {
public:
    BatchDownloadResourcesTaskDao() = default;
    ~BatchDownloadResourcesTaskDao() = default;

    EXPORT int32_t FromUriToAllFileIds(const std::vector<std::string> &uris, std::vector<std::string> &fileIds);
    EXPORT int32_t AddOtherBurstIdsToFileIds(std::vector<std::string> &fileIds);
    EXPORT void CloudMediaBatchDownloadResourcesStatusToTaskPo(std::shared_ptr<NativeRdb::ResultSet> resultSet,
        std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks);

    // add
    EXPORT int32_t QueryValidBatchDownloadPoFromPhotos(std::vector<std::string> &fileIds,
        std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks);
    EXPORT int32_t BatchInsert(int64_t &outRowId, const std::string &table,
        std::vector<NativeRdb::ValuesBucket> &initialBatchValues);
    EXPORT int32_t ClassifyExistedDownloadTasks(std::vector<std::string> &fileIds,
        std::vector<std::string> &newIds, std::vector<std::string> &existedIds);
    EXPORT int32_t ClassifyInvalidDownloadTasks(std::vector<std::string> &newIds,
        std::vector<std::string> &invalidIds);
    EXPORT int32_t UpdateExistedTasksStatus(
        std::vector<std::string> &fileIds, const int32_t status, const bool isUpdateTimeStamp);
    EXPORT int32_t HandleDuplicateAddTask(std::vector<DownloadResourcesTaskPo> &taskPos);
    EXPORT int32_t HandleAddExistedDownloadTasks(std::vector<std::string> &fileIds);

    // resume
    EXPORT int32_t UpdateResumeDownloadResourcesInfo(const std::vector<std::string> &fileIds);
    EXPORT int32_t UpdateStatusFailedToWaiting(const std::vector<std::string> &fileIds);
    EXPORT int32_t UpdateStatusPauseToWaiting(const std::vector<std::string> &fileIds);
    EXPORT int32_t UpdateStatusPauseToDownloading(const std::vector<std::string> &fileIds);

    EXPORT int32_t UpdateResumeAllDownloadResourcesInfo();
    EXPORT int32_t UpdateAllStatusFailedToWaiting();
    EXPORT int32_t UpdateAllStatusPauseToWaiting();
    EXPORT int32_t UpdateAllStatusPauseToDownloading();

    // pause
    EXPORT int32_t UpdatePauseDownloadResourcesInfo(const std::vector<std::string> &fileIds);
    EXPORT int32_t UpdateAllPauseDownloadResourcesInfo();
    EXPORT int32_t QueryPauseDownloadingStatusResources(std::vector<std::string> &fileIds,
        std::vector<std::string> &fileIdsDownloading, std::vector<std::string> &fileIdsNotInDownloading);

    // Cancel
    EXPORT int32_t QueryCancelDownloadingStatusResources(std::vector<std::string> &fileIds,
        std::vector<std::string> &fileIdsDownloading, std::vector<std::string> &fileIdsNotInDownloading);
    EXPORT int32_t DeleteCancelStateDownloadResources(const std::vector<std::string> &fileIds);
    EXPORT int32_t DeleteAllDownloadResourcesInfo();

    // get
    EXPORT int32_t QueryCloudMediaBatchDownloadResourcesStatus(
        NativeRdb::RdbPredicates &predicates, std::vector<DownloadResourcesTaskPo> &downloadResourcesTasks);
    EXPORT int32_t QueryCloudMediaBatchDownloadResourcesCount(
        NativeRdb::RdbPredicates &predicates, int32_t &count);
};
} // namespace Media
} // namespace OHOS
#endif  // OHOS_MEDIA_BATCH_DOWNLOAD_RESOURCE_TASK_DAO_H
