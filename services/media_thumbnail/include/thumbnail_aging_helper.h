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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_AGING_HELPER_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_AGING_HELPER_H_

#include "medialibrary_async_worker.h"
#include "rdb_helper.h"
#include "single_kvstore.h"
#include "thumbnail_utils.h"

namespace OHOS {
namespace Media {
class AgingAsyncTaskData : public AsyncTaskData {
public:
    AgingAsyncTaskData() = default;
    virtual ~AgingAsyncTaskData() override = default;
    ThumbRdbOpt opts;
    ThumbnailData thumbnailData;
};

class ThumbnailAgingHelper {
public:
    ThumbnailAgingHelper() = delete;
    virtual ~ThumbnailAgingHelper() = delete;

    EXPORT static int32_t AgingLcdBatch(ThumbRdbOpt &opts);

    EXPORT static int32_t ClearLcdFromFileTable(ThumbRdbOpt &opts);
    EXPORT static int32_t GetAgingDataCount(const int64_t &time, const bool &before, ThumbRdbOpt &opts, int &count);
private:
    EXPORT static int32_t GetLcdCount(ThumbRdbOpt &opts, int &outLcdCount);
    EXPORT static int32_t GetAgingLcdData(ThumbRdbOpt &opts, int LcdLimit, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetDistributeLcdCount(ThumbRdbOpt &opts, int &outLcdCount);
    EXPORT static int32_t GetAgingDistributeLcdData(ThumbRdbOpt &opts,
        int LcdLimit, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetLcdCountByTime(const int64_t &time, const bool &before, ThumbRdbOpt &opts,
        int &outLcdCount);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_AGING_HELPER_H_
