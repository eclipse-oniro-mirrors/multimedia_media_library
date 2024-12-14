/*
 * Copyright (C) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_ITHUMBNAIL_HELPER_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_ITHUMBNAIL_HELPER_H_

#include <map>
#include <memory>
#include <shared_mutex>

#include "ability_connect_callback_stub.h"
#include "fa_ability_context.h"
#include "datashare_proxy.h"
#include "datashare_values_bucket.h"
#include "medialibrary_sync_operation.h"
#include "result_set_bridge.h"
#include "thumbnail_const.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_utils.h"
#include "pixel_map.h"

namespace OHOS {
namespace Media {
enum WaitStatus {
    INSERT,
    WAIT_SUCCESS,
    WAIT_CONTINUE,
    WAIT_FAILED,
    TIMEOUT,
};

enum CloudLoadType {
    NONE,
    CLOUD_READ_THUMB,
    CLOUD_READ_LCD,
    CLOUD_DOWNLOAD,
};

enum CloudReadStatus {
    START,
    SUCCESS,
    FAIL,
};

class ThumbnailSyncStatus {
public:
    std::condition_variable cond_;
    std::mutex mtx_;
    bool isSyncComplete_{false};
    bool isCreateThumbnailSuccess_{false};
    std::atomic<CloudReadStatus> CloudLoadThumbnailStatus_{START};
    std::atomic<CloudReadStatus> CloudLoadLcdStatus_{START};
    std::atomic<CloudLoadType> cloudLoadType_{NONE};
};

using ThumbnailMap = std::map<std::string, std::shared_ptr<ThumbnailSyncStatus>>;
class ThumbnailWait {
public:
    ThumbnailWait(bool release);
    ~ThumbnailWait();

    WaitStatus InsertAndWait(const std::string &id, ThumbnailType type);
    WaitStatus CloudInsertAndWait(const std::string &id, CloudLoadType cloudLoadType);
    void CheckAndWait(const std::string &id, bool isLcd);
    void UpdateThumbnailMap();
    void UpdateCloudLoadThumbnailMap(CloudLoadType cloudLoadType, bool isLoadSuccess);

private:
    void Notify();
    std::string id_;
    bool needRelease_{false};
    static ThumbnailMap thumbnailMap_;
    static std::shared_mutex mutex_;
};

class IThumbnailHelper {
public:
    IThumbnailHelper() = default;
    virtual ~IThumbnailHelper() = default;
    EXPORT static void CreateLcdAndThumbnail(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static bool DoCreateLcdAndThumbnail(ThumbRdbOpt &opts, ThumbnailData &data, WaitStatus &ret);
    EXPORT static void CreateLcd(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static void CreateThumbnail(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static void CreateAstc(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static void CreateAstcEx(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static void AddThumbnailGenerateTask(ThumbnailGenerateExecute executor, ThumbRdbOpt &opts,
        ThumbnailData &thumbData, const ThumbnailTaskType &taskType, const ThumbnailTaskPriority &priority);
    EXPORT static void AddThumbnailGenBatchTask(ThumbnailGenerateExecute executor,
        ThumbRdbOpt &opts, ThumbnailData &thumbData, int32_t requestId = 0);
    EXPORT static std::unique_ptr<PixelMap> GetPixelMap(const std::vector<uint8_t> &image, Size &size);
    EXPORT static bool DoCreateLcd(ThumbRdbOpt &opts, ThumbnailData &data, WaitStatus &ret);
    EXPORT static bool DoCreateThumbnail(ThumbRdbOpt &opts, ThumbnailData &data, WaitStatus &ret);
    EXPORT static bool DoCreateAstc(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool DoCreateAstcEx(ThumbRdbOpt &opts, ThumbnailData &data, WaitStatus &ret);
    EXPORT static bool DoRotateThumbnail(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool DoRotateThumbnailEx(ThumbRdbOpt &opts, ThumbnailData &data, int32_t fd, ThumbnailType thumbType);
    EXPORT static bool IsPureCloudImage(ThumbRdbOpt &opts);
    EXPORT static void DeleteMonthAndYearAstc(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static void UpdateAstcDateTaken(std::shared_ptr<ThumbnailTaskData> &data);
    EXPORT static bool UpdateThumbnailState(const ThumbRdbOpt &opts, ThumbnailData &data, const bool isSuccess);
    EXPORT static void UpdateHighlightDbState(ThumbRdbOpt &opts, ThumbnailData &data);
private:
    EXPORT static bool GenThumbnail(ThumbRdbOpt &opts, ThumbnailData &data, const ThumbnailType type);
    EXPORT static bool GenThumbnailEx(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool TryLoadSource(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool GenMonthAndYearAstcData(ThumbnailData &data, const ThumbnailType type);
    EXPORT static bool UpdateSuccessState(const ThumbRdbOpt &opts, const ThumbnailData &data);
    EXPORT static bool UpdateFailState(const ThumbRdbOpt &opts, const ThumbnailData &data);
    EXPORT static int32_t UpdateThumbDbState(const ThumbRdbOpt &opts, const ThumbnailData &data);
    EXPORT static bool IsCreateThumbnailSuccess(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool IsCreateThumbnailExSuccess(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool IsCreateLcdSuccess(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool IsCreateLcdExSuccess(ThumbRdbOpt &opts, ThumbnailData &data);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_ITHUMBNAIL_HELPER_H_
