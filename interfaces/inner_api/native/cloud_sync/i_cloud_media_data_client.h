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

#ifndef OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_CLIENT_H
#define OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_CLIENT_H

#include <string>
#include <vector>

#include "media_column.h"
#include "cloud_file_data.h"
#include "cloud_download_thum_para.h"
#include "cloud_meta_data.h"
#include "cloud_download_file_meta.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "media_operate_result.h"
#include "datashare_predicates.h"
#include "cloud_lake_info.h"
#include "datashare_values_bucket.h"

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
class EXPORT ICloudMediaDataClient {
public:  // getter & setter
    virtual void SetTraceId(const std::string &traceId) = 0;
    virtual std::string GetTraceId() const = 0;
    virtual void SetUserId(const int32_t &userId) = 0;
    virtual void SetCloudType(const int32_t cloudType) = 0;

public:
    // 核查
    virtual int32_t UpdateDirty(const std::string &cloudId, DirtyTypes dirtyType) = 0;
    virtual int32_t UpdatePosition(const std::vector<std::string> &cloudIds, int32_t position) = 0;
    virtual int32_t UpdatePosWithType(const std::vector<std::string> &cloudIds,
        int32_t position, int32_t fileSourceType) = 0;
    virtual int32_t UpdateFileSourceType(const std::vector<std::string> &cloudIds,
        int32_t fileSourceType) = 0;
    virtual int32_t UpdateThmStatus(const std::string &cloudId, int32_t thmStatus) = 0;
    virtual int32_t GetAgingFile(const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset,
        std::vector<CloudMetaData> &metaData) = 0;
    virtual int32_t GetActiveAgingFile(const int64_t time, int32_t mediaType, int32_t sizeLimit, int32_t offset,
        std::vector<CloudMetaData> &metaData) = 0;
    // 下载
    virtual int32_t GetDownloadAsset(
        const std::vector<std::string> &uris, std::vector<CloudMetaData> &cloudMetaDataVec) = 0;
    virtual int32_t GetDownloadLakeAsset(
        const std::vector<std::string> &uris, std::vector<CloudDlFileMeta> &cloudMetaDataVec) = 0;
    virtual int32_t GetDownloadThmsByUri(
        const std::vector<std::string> &uri, int32_t type, std::vector<CloudMetaData> &metaData) = 0;
    virtual int32_t OnDownloadAsset(
        const std::vector<std::string> &cloudIds, std::vector<MediaOperateResult> &result) = 0;
    virtual int32_t OnDownloadLakeAsset(
        const std::unordered_map<std::string, AdditionFileInfo> &lakeInfos,
        std::vector<MediaOperateResult> &result) = 0;
    virtual int32_t GetDownloadThms(std::vector<CloudMetaData> &cloudMetaDataVec, const DownloadThumPara &param) = 0;
    virtual int32_t OnDownloadThms(const std::unordered_map<std::string, int32_t> &resMap, int32_t &failSize) = 0;
    virtual int32_t GetDownloadThmNum(int32_t &totalNum, int32_t type) = 0;
    // 缓存视频
    virtual int32_t GetVideoToCache(std::vector<CloudMetaData> &cloudMetaDataVec, int32_t size) = 0;
    // 大数据
    virtual int32_t GetFilePosStat(std::vector<uint64_t> &filePosStat) = 0;
    virtual int32_t GetCloudThmStat(std::vector<uint64_t> &cloudThmStat) = 0;
    virtual int32_t GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat) = 0;
    virtual int32_t UpdateLocalFileDirty(std::vector<MDKRecord> &records) = 0;
    virtual int32_t UpdateSyncStatus(const std::string &cloudId, int32_t syncStatus) = 0;

    virtual int32_t GetCloudSyncUnPreparedData(int32_t &result) = 0;
    virtual int32_t SubmitCloudSyncPreparedDataTask() = 0;

    virtual int32_t CheckAndFixAlbum() = 0;
    virtual int32_t QueryData(const DataShare::DataSharePredicates &predicates,
            const std::vector<std::string> &columnNames, const std::string &tableName,
            std::vector<std::unordered_map<std::string, std::string>> &results) = 0;
    virtual int32_t UpdateData(const std::string &tableName, const DataShare::DataSharePredicates &predicates,
            const DataShare::DataShareValuesBucket &value, const std::string &operateName) = 0;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_I_CLOUD_MEDIA_DATA_CLIENT_H