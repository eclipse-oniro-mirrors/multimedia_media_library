/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_DFX_REPORTER_H
#define OHOS_MEDIA_DFX_REPORTER_H

#include <string>

#include "thumbnail_data.h"

namespace OHOS {
namespace Media {

struct AlbumFusionDfxDataPoint {
    int64_t albumFusionTag;
    int64_t reportTimeStamp;
    int32_t albumFusionState; // 1: Before fusion starts 2: After fusion successfully finished 3: when fusion fails
    int32_t imageAssetCount;
    int32_t videoAssetCount;
    int32_t numberOfSourceAlbum;
    int32_t numberOfUserAlbum;
    int32_t totalAssetsInSourceAlbums;
    int32_t totalAssetsInUserAlbums;
    std::string albumDetails;
    std::string hiddenAssetInfo;
};

struct PhotoStatistics {
    int32_t localImageCount;   // 纯本地照片数量
    int32_t localVideoCount;   // 纯本地视频数量
    int32_t cloudImageCount;   // 纯云端图片数量
    int32_t cloudVideoCount;   // 纯云端视频数量
    int32_t sharedImageCount;  // 端云共存照片数量
    int32_t sharedVideoCount;  // 端云共存视频数量
};

class DfxReporter {
public:
    DfxReporter();
    ~DfxReporter();

    void ReportTimeOutOperation(std::string &bundleName, int32_t type, int32_t object, int32_t time);
    int32_t ReportHighMemoryImageThumbnail(std::string &path, std::string &suffix, int32_t width, int32_t height);
    int32_t ReportHighMemoryVideoThumbnail(std::string &path, std::string &suffix, int32_t width, int32_t height);
    void ReportThumbnailError();
    void ReportCommonBehavior();
    void ReportDeleteStatistic();
    void ReportDeleteBehavior(std::string bundleName, int32_t type, std::string path);
    void ReportThumbnailGeneration(const ThumbnailData::GenerateStats &stats);
    void ReportPhotoInfo(const PhotoStatistics& stats);
    void ReportAlbumInfo(const std::string &albumName, int32_t albumImageCount, int32_t albumVideoCount,
        bool isLocal);
    void ReportDirtyCloudPhoto(const std::string &data, int32_t dirty, int32_t cloudVersion);
    void ReportCommonVersion(int32_t dbVersion);
    void ReportAnalysisVersion(const std::string &analysisName, int32_t version);
    void ReportAdaptationToMovingPhoto();
    static int32_t ReportCloudSyncThumbGenerationStatus(const int32_t& downloadedThumb, const int32_t& generatedThumb,
        const int32_t& totalDownload);
    static void ReportStartResult(int32_t scene, int32_t errorCode, int32_t startTime);
    void ReportPhotoRecordInfo();
    static int32_t ReportMedialibraryAPI(const std::string& callerPackage, const std::string& saveUri);
    static int32_t ReportAlbumFusion(const AlbumFusionDfxDataPoint& reportData);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_REPORTER_H