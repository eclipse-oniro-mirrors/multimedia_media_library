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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_HELPER_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_HELPER_H_

#include "rdb_helper.h"
#include "rdb_predicates.h"
#include "single_kvstore.h"
#include "thumbnail_utils.h"

namespace OHOS {
namespace Media {
class ThumbnailGenerateHelper {
public:
    ThumbnailGenerateHelper() = delete;
    virtual ~ThumbnailGenerateHelper() = delete;
    static int32_t CreateThumbnailFileScaned(ThumbRdbOpt &opts, bool isSync);
    static int32_t CreateThumbnailBackground(ThumbRdbOpt &opts);
    static int32_t CreateAstcBackground(ThumbRdbOpt &opts);
    static int32_t CreateAstcCloudDownload(ThumbRdbOpt &opts);
    static int32_t CreateLcdBackground(ThumbRdbOpt &opts);
    static int32_t CreateAstcBatchOnDemand(ThumbRdbOpt &opts, NativeRdb::RdbPredicates &predicate, int32_t requestId);
    EXPORT static int32_t UpgradeThumbnailBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t RestoreAstcDualFrame(ThumbRdbOpt &opts);
    static int32_t GetNewThumbnailCount(ThumbRdbOpt &opts, const int64_t &time, int &count);
    EXPORT static int32_t GetThumbnailPixelMap(ThumbRdbOpt &opts, ThumbnailType thumbType);

private:
    static int32_t GetLcdCount(ThumbRdbOpt &opts, int &outLcdCount);
    static int32_t GetNoLcdData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    static int32_t GetNoThumbnailData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    static int32_t GetNoAstcData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    static int32_t GetAvailableFile(ThumbRdbOpt &opts, ThumbnailData &data, ThumbnailType thumbType,
        std::string &fileName);
    static int32_t GetThumbnailDataNeedUpgrade(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    static void CheckMonthAndYearKvStoreValid(ThumbRdbOpt &opts);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_HELPER_H_
