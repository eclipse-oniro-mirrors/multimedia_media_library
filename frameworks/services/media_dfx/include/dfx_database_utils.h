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


#ifndef OHOS_MEDIA_DFX_DATABASE_UTILS_H
#define OHOS_MEDIA_DFX_DATABASE_UTILS_H

#include <string>
#include <vector>
#include <map>
#include "rdb_helper.h"
#include "result_set.h"
#include "rdb_predicates.h"
#include "dfx_const.h"
#include "dfx_reporter.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

struct QuerySizeAndResolution;

struct DfxCompatibleInfo {
    bool highResolution = false;
    std::string encodings = "";
    int32_t futureField = 0;
};

struct DocsScanFolderStats {
    std::string dirPath;
    int32_t imageCount = 0;
    int32_t videoCount = 0;
    std::string formatDistribution = "{}";
    std::string sizeDistribution = "[]";
    int32_t atimeWithin30min = 0;
    int32_t atimeDiffSec = 0;
};

class DfxDatabaseUtils {
public:
    EXPORT static int32_t QueryFromPhotos(int32_t mediaType, int32_t position);
    EXPORT static int32_t QueryFileManagerFromPhotos(int32_t mediaType, int32_t position);
    EXPORT static AlbumInfo QueryAlbumInfoBySubtype(int32_t albumSubtype);
    EXPORT static std::vector<PhotoInfo> QueryDirtyCloudPhoto();
    EXPORT static int32_t QueryAnalysisVersion(const std::string &table, const std::string &column);
    EXPORT static int32_t QueryDownloadedAndGeneratedThumb(int32_t& downloadedThumb, int32_t& generatedThumb);
    EXPORT static int32_t QueryTotalCloudThumb(int32_t& totalDownload);
    EXPORT static int32_t QueryDbVersion();
    EXPORT static int32_t QueryPhotoRecordInfo(PhotoRecordInfo &photoRecordInfo);
    EXPORT static int32_t QueryASTCThumb(bool isLocal);
    EXPORT static int32_t QueryLCDThumb(bool isLocal);
    EXPORT static int32_t QueryOperationRecordInfo(OperationRecordInfo &operationRecordInfo);
    EXPORT static int32_t QueryPhotoErrorCount();
    EXPORT static void GetPhotoMimeType(std::string &photoMimeType);
    EXPORT static bool GetSizeAndResolutionInfo(QuerySizeAndResolution &queryInfo);
    EXPORT static int32_t QueryAncoPhotosFormatAndCount(AncoCountFormatInfo &reportData);
    EXPORT static std::vector<std::string> QueryAlbumNamesByUploadStatus(const int32_t uploadStatus);
    EXPORT static int32_t QueryAllCompatibleInfo(std::map<std::string, DfxCompatibleInfo>& infoMap);
    EXPORT static int32_t QueryLcdFromPhotos(std::vector<int32_t> thumbStatus, int32_t isFavorite);
    EXPORT static int32_t QueryAlbumCoverCount();
    EXPORT static int32_t QueryPhotoExtSmartCount();
    EXPORT static int32_t CreateDocsMediaScanTempTable();
    EXPORT static int32_t InsertDocsScanFolderStats(const DocsScanFolderStats &stats);
    EXPORT static bool IsDirPathInDocsScanTempTable(const std::string &dirPath);
    EXPORT static int32_t QueryDocsScanFolderStats(int32_t offset, int32_t limit,
        std::vector<DocsScanFolderStats> &results);
    EXPORT static int32_t QueryDocsScanTotalFolderCount(int32_t &count);
    EXPORT static int32_t DropDocsMediaScanTempTable();

private:
    static int32_t QueryInt(const NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
        const std::string &queryColumn, int32_t &value);
    static int32_t QueryDouble(const NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
        const std::string &queryColumn, double &value);
    static bool CheckChargingAndScreenOff(bool isReported);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_DATABASE_UTILS_H