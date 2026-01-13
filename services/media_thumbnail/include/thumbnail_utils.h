/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_UTILS_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_UTILS_H_

#include <mutex>
#include <condition_variable>

#include "fa_ability_context.h"
#include "avmetadatahelper.h"
#include "datashare_result_set.h"
#include "image_source.h"
#include "medialibrary_rdbstore.h"
#include "rdb_helper.h"
#include "rdb_predicates.h"
#include "single_kvstore.h"
#include "thumbnail_const.h"
#include "thumbnail_data.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class ThumbnailUtils {
public:
    EXPORT ThumbnailUtils() = delete;
    EXPORT ~ThumbnailUtils() = delete;
    // utils
    EXPORT static bool ResizeImage(const std::vector<uint8_t> &data, const Size &size,
        std::unique_ptr<PixelMap> &pixelMap);
    EXPORT static bool CompressImage(const std::shared_ptr<PixelMap> &pixelMap, std::vector<uint8_t> &data,
        bool isAstc = false, bool forceSdr = true, const ThumbnailQuality quality = ThumbnailQuality::DEFAULT);
    EXPORT static bool CompressPicture(ThumbnailData &data, const std::shared_ptr<Picture>& picture,
        const bool isSourceEx, std::string &tempOutputPath);
    EXPORT static bool CleanThumbnailInfo(ThumbRdbOpt &opts, bool withThumb, bool withLcd = false);

    // RDB Store Query
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryThumbnailInfo(ThumbRdbOpt &opts,
        ThumbnailData &data, int &err);

    EXPORT static bool DeleteAllThumbFilesAndAstc(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static bool DeleteThumbnailDirAndAstc(const ThumbRdbOpt &opts, const ThumbnailData &data);
    EXPORT static bool BatchDeleteThumbnailDirAndAstc(const ThumbRdbOpt &opts, const ThumbnailDataBatch &dataBatch);
    // Steps
    EXPORT static bool LoadSourceImage(ThumbnailData &data);
    EXPORT static bool GenTargetPixelmap(ThumbnailData &data, const Size &desiredSize);

    EXPORT static bool SaveAfterPacking(ThumbnailData &data, const bool isSourceEx, const std::string &tempOutputPath);
    EXPORT static void CancelAfterPacking(const std::string &tempOutputPath);
    EXPORT static int TrySaveFile(ThumbnailData &Data, ThumbnailType type);
    EXPORT static bool CacheLcdInfo(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static void CacheInvalidLcdInfo(ThumbnailData &data);
    EXPORT static bool UpdateHighlightInfo(ThumbRdbOpt &opts, ThumbnailData &data, int &err);
    EXPORT static bool UpdateLcdReadyStatus(ThumbRdbOpt &opts, ThumbnailData &data, int &err, LcdReady status);
    EXPORT static bool DoUpdateAstcDateTaken(ThumbRdbOpt &opts, ThumbnailData &data);

    // RDB Store generate and aging
    EXPORT static bool QueryLcdCount(ThumbRdbOpt &opts, int &outLcdCount, int &err);
    EXPORT static bool QueryDistributeLcdCount(ThumbRdbOpt &opts, int &outLcdCount, int &err);
    EXPORT static bool QueryAgingLcdInfos(ThumbRdbOpt &opts, int LcdLimit,
        std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryNoLcdInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryLocalNoLcdInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryNoThumbnailInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryUpgradeThumbnailInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos,
        bool isWifiConnected, int &err);
    EXPORT static bool QueryNoAstcInfosRestored(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err,
        const int32_t &restoreAstcCount);
    EXPORT static bool QueryLocalNoThumbnailInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryNoAstcInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryNewThumbnailCount(ThumbRdbOpt &opts, const int64_t &time, int &count, int &err);
    EXPORT static bool QueryNoHighlightInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos, int &err);
    EXPORT static bool QueryNoHighlightPath(ThumbRdbOpt &opts, ThumbnailData &data, int &err);
    EXPORT static bool QueryHighlightTriggerPath(ThumbRdbOpt &opts, ThumbnailData &data, int &err);
    EXPORT static std::string GetHighlightValue(const std::string &str, const std::string &key);
    EXPORT static bool GetHighlightTracks(ThumbRdbOpt &opts, std::vector<int> &trackInfos, int32_t &err);
    EXPORT static std::string CreateOutputPath(const ThumbnailData &data, const std::string &suffix);

    EXPORT static bool QueryLcdCountByTime(const int64_t &time, const bool &before, ThumbRdbOpt &opts, int &outLcdCount,
        int &err);
    EXPORT static bool ResizeThumb(int& width, int& height);
    EXPORT static bool ResizeLcd(int& width, int& height);
    EXPORT static void QueryThumbnailDataFromFileId(ThumbRdbOpt &opts, const std::string &id,
        ThumbnailData &data, int &err);
    EXPORT static bool CheckDateTaken(ThumbRdbOpt &opts, ThumbnailData &data);
    EXPORT static void GetThumbnailInfo(ThumbRdbOpt &opts, ThumbnailData &outData);
    EXPORT static bool ScaleThumbnailFromSource(ThumbnailData &data, const bool isSourceEx);
    EXPORT static bool ScaleTargetPixelMap(std::shared_ptr<PixelMap> &dataSource, const Size &targetSize,
        const AntiAliasingOption &option);
    EXPORT static bool CenterScaleEx(std::shared_ptr<PixelMap> &dataSource, const Size &desiredSize,
        const std::string path);

    EXPORT static void RecordStartGenerateStats(ThumbnailData::GenerateStats &stats, GenerateScene scene,
        LoadSourceType sourceType);
    EXPORT static void RecordCostTimeAndReport(ThumbnailData::GenerateStats &stats);

    EXPORT static bool GetLocalThumbSize(const ThumbnailData &data, const ThumbnailType& type, Size& size);
    EXPORT static void SetThumbnailSizeValue(NativeRdb::ValuesBucket& values, Size& size, const std::string& column);
    EXPORT static bool IsMobileNetworkEnabled();
    EXPORT static bool LoadVideoFrame(ThumbnailData &data, Size &desiredSize, int64_t timeStamp);
    EXPORT static bool CheckCloudThumbnailDownloadFinish(const std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);
    EXPORT static bool QueryOldKeyAstcInfos(const std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr,
        const std::string &table, std::vector<ThumbnailData> &infos);
    EXPORT static void StoreThumbnailSize(const ThumbRdbOpt& opts, const ThumbnailData& data);
    EXPORT static void DropThumbnailSize(const ThumbRdbOpt& opts, const ThumbnailData& data);
    EXPORT static void BatchDropThumbnailSize(const ThumbnailDataBatch& dataBatch);
    EXPORT static bool IsExCloudThumbnail(const ThumbnailData &data);
    EXPORT static bool NeedRotateThumbnail(const ThumbnailData &data);
    EXPORT static bool IsImageWithRotate(const ThumbnailData &data);
    EXPORT static bool IsUseRotatedSource(const ThumbnailData &data);
    EXPORT static void PostProcPixelMapSource(ThumbnailData &data);
    EXPORT static void PostProcPictureSource(ThumbnailData &data);

private:
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryThumbnailSet(ThumbRdbOpt &opts);
    EXPORT static int SaveThumbDataToLocalDir(ThumbnailData &data, const std::string &suffix,
        uint8_t *output, const int writeSize);
    EXPORT static int ToSaveFile(ThumbnailData &data, const std::string &fileName,
        uint8_t *output, const int &writeSize);
    EXPORT static int SaveFileCreateDir(const std::string &path, const std::string &suffix, std::string &fileName);
    EXPORT static int SaveFileCreateDirHighlight(const std::string &path, const std::string &suffix,
        std::string &fileName, const std::string &timeStamp);
    EXPORT static int32_t SetSource(std::shared_ptr<AVMetadataHelper> avMetadataHelper, const std::string &path);
    EXPORT static int64_t UTCTimeMilliSeconds();
    EXPORT static void ParseHighlightQueryResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
       ThumbnailData &data, int &err);

    // utils
    EXPORT static bool LoadImageFile(ThumbnailData &data, Size &desiredSize);
    EXPORT static bool LoadAudioFileInfo(std::shared_ptr<AVMetadataHelper> avMetadataHelper, ThumbnailData &data,
        Size &desiredSize, uint32_t &errCode);
    EXPORT static bool LoadAudioFile(ThumbnailData &data, Size &desiredSize);
    EXPORT static bool ConvertStrToInt32(const std::string &str, int32_t &ret);
    EXPORT static bool ParseVideoSize(std::shared_ptr<AVMetadataHelper> &avMetadataHelper,
        int32_t &videoWidth, int32_t &videoHeight);

    // scale
    EXPORT static bool ScaleFastThumb(ThumbnailData &data, const Size &size);

    EXPORT static int SaveAstcDataToKvStore(ThumbnailData &data, const ThumbnailType &type);
    EXPORT static bool UpdateAstcDateTakenFromKvStore(ThumbRdbOpt &opts, const ThumbnailData &data);
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_UTILS_H_
