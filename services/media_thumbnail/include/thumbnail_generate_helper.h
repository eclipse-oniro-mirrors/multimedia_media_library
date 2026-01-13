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
#include "thumbnail_generate_worker_manager.h"

namespace OHOS {
namespace Media {
class ThumbnailGenerateHelper {
public:
    ThumbnailGenerateHelper() = delete;
    virtual ~ThumbnailGenerateHelper() = delete;
    EXPORT static int32_t CreateThumbnailFileScaned(ThumbRdbOpt &opts, bool isSync);
    EXPORT static int32_t CreateThumbnailFileScanedWithPicture(ThumbRdbOpt &opts,
        std::shared_ptr<Picture> originalPhotoPicture, bool isSync);
    EXPORT static int32_t CreateThumbnailBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t CreateAstcBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t CreateAstcCloudDownload(ThumbRdbOpt &opts, bool isCloudInsertTaskPriorityHigh = false);
    EXPORT static int32_t CreateAstcMthAndYear(ThumbRdbOpt &opts);
    EXPORT static int32_t CreateLcdBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t GenerateHighlightThumbnailBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t RegenerateThumbnailFromCloud(ThumbRdbOpt &opts);
    EXPORT static int32_t TriggerHighlightThumbnail(ThumbRdbOpt &opts, std::string &id, std::string &tracks,
        std::string &trigger, std::string &genType);
    EXPORT static int32_t UpgradeThumbnailBackground(ThumbRdbOpt &opts, bool isWifiConnected);
    EXPORT static int32_t CreateLocalThumbnail(ThumbRdbOpt &opts);
    EXPORT static int32_t GetNewThumbnailCount(ThumbRdbOpt &opts, const int64_t &time, int &count);
    EXPORT static int32_t GetThumbnailPixelMap(ThumbnailData& data, ThumbRdbOpt &opts, ThumbnailType thumbType);
    EXPORT static int32_t GetKeyFrameThumbnailPixelMap(ThumbRdbOpt &opts, int32_t &timeStamp,
        int32_t &type);
    EXPORT static int32_t CheckLcdSizeAndUpdateStatus(ThumbRdbOpt &opts);
    EXPORT static int32_t RepairExifRotateBackground(ThumbRdbOpt &opts);
    EXPORT static int32_t FixThumbnailExifRotateAfterDownloadAsset(ThumbRdbOpt &opts, bool needDeleteFromVisionTables);
    EXPORT static int32_t DfxReportThumbnailDirAcl();

private:
    EXPORT static int32_t GetLcdCount(ThumbRdbOpt &opts, int &outLcdCount);
    EXPORT static int32_t GetNoLcdData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetLocalNoLcdData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetNoThumbnailData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetNoAstcData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetAvailableFile(ThumbRdbOpt &opts, ThumbnailData &data, ThumbnailType thumbType,
        std::string &fileName);
    EXPORT static std::string GetAvailablePath(const std::string &path, const ThumbnailType &thumbType);
    EXPORT static int32_t GetNoHighlightData(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas);
    EXPORT static int32_t GetAvailableKeyFrameFile(ThumbRdbOpt &opts, ThumbnailData &data, int32_t thumbType,
        std::string &fileName);
    EXPORT static int32_t GetThumbnailDataNeedUpgrade(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas,
        bool isWifiConnected);
    EXPORT static void CheckMonthAndYearKvStoreValid(ThumbRdbOpt &opts);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_GENERATE_HELPER_H_
