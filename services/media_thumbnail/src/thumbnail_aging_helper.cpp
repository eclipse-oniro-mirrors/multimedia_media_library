/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_aging_helper.h"

#include "medialibrary_errno.h"
#include "media_log.h"
#include "thumbnail_const.h"
#include "thumbnail_file_utils.h"

using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
static void AgingLcd(AsyncTaskData *data)
{
    if (data == nullptr) {
        return;
    }
    AgingAsyncTaskData* taskData = static_cast<AgingAsyncTaskData*>(data);
    int32_t err = ThumbnailAgingHelper::ClearLcdFromFileTable(taskData->opts);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to ClearLcdFormFileTable %{public}d", err);
    }
}

int32_t ThumbnailAgingHelper::AgingLcdBatch(ThumbRdbOpt &opts)
{
    MEDIA_INFO_LOG("IN %{private}s", opts.table.c_str());
    if (opts.store == nullptr) {
        return E_HAS_DB_ERROR;
    }

    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        return E_ERR;
    }
    AgingAsyncTaskData* taskData = new (std::nothrow)AgingAsyncTaskData();
    if (taskData == nullptr) {
        return E_ERR;
    }
    taskData->opts = opts;
    shared_ptr<MediaLibraryAsyncTask> agingAsyncTask = make_shared<MediaLibraryAsyncTask>(AgingLcd, taskData);
    if (agingAsyncTask != nullptr) {
        asyncWorker->AddTask(agingAsyncTask, false);
    }
    return E_OK;
}

int32_t ThumbnailAgingHelper::GetAgingDataCount(const int64_t &time, const bool &before, ThumbRdbOpt &opts, int &count)
{
    int err = GetLcdCountByTime(time, before, opts, count);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetAgingDataCount %{public}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailAgingHelper::ClearLcdFromFileTable(ThumbRdbOpt &opts)
{
    int lcdCount = 0;
    int32_t err = GetLcdCount(opts, lcdCount);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetLcdCount %{public}d", err);
        return err;
    }
    MEDIA_DEBUG_LOG("lcdCount %{public}d", lcdCount);
    if (lcdCount <= THUMBNAIL_LCD_AGING_THRESHOLD) {
        MEDIA_INFO_LOG("Not need aging Lcd. lcdCount: %{public}d", lcdCount);
        return E_OK;
    }
    vector<ThumbnailData> infos;
    err = GetAgingLcdData(opts, lcdCount - THUMBNAIL_LCD_AGING_THRESHOLD, infos);
    if ((err != E_OK) || infos.empty()) {
        MEDIA_ERR_LOG("Failed to GetAgingLcdData %{public}d", err);
        return err;
    }

    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        return E_ERR;
    }
    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;
        if (ThumbnailFileUtils::DeleteThumbFile(infos[i], ThumbnailType::LCD)) {
            ThumbnailUtils::CleanThumbnailInfo(opts, false, true);
        }
    }
    MEDIA_INFO_LOG("Clear Lcd completed");
    return E_OK;
}

int32_t ThumbnailAgingHelper::GetLcdCount(ThumbRdbOpt &opts, int &outLcdCount)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err)) {
        MEDIA_ERR_LOG("Failed to QueryLcdCount %{public}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailAgingHelper::GetLcdCountByTime(const int64_t &time, const bool &before, ThumbRdbOpt &opts,
    int &outLcdCount)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryLcdCountByTime(time, before, opts, outLcdCount, err)) {
        MEDIA_ERR_LOG("Failed to QueryLcdCountByTime %{public}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailAgingHelper::GetAgingLcdData(ThumbRdbOpt &opts, int lcdLimit, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryAgingLcdInfos(opts, lcdLimit, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryAgingLcdInfos %{public}d", err);
        return err;
    }
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
