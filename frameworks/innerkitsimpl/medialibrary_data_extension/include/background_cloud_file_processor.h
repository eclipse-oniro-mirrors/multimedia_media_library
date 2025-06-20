/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_H
#define OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_H

#include "abs_shared_result_set.h"
#include "background_cloud_file_download_callback.h"
#include "medialibrary_async_worker.h"
#include "metadata.h"
#include "rdb_predicates.h"
#include "timer.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "media_file_uri.h"
#include "medialibrary_base_bg_processor.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
constexpr int32_t DOWNLOAD_INTERVAL = 1 * 60 * 1000;  // 1 minute
constexpr int32_t DOWNLOAD_DURATION = 10 * 1000; // 10 seconds
constexpr int32_t DOWNLOAD_FAIL_MAX_TIMES = 5; // 5 times

typedef struct {
    bool isCloud;
    bool isVideo;
} QueryOption;

class EXPORT BackgroundCloudFileProcessor : public MediaLibraryBaseBgProcessor {
public:
    EXPORT static void StartTimer();
    EXPORT static void StopTimer(bool isReportSchedule = false);
    EXPORT static void SetDownloadLatestFinished(bool downloadLatestFinished);
    EXPORT static bool GetDownloadLatestFinished();
    EXPORT static void HandleSuccessCallback(const DownloadProgressObj &progress);
    EXPORT static void HandleFailedCallback(const DownloadProgressObj &progress);
    EXPORT static void HandleStoppedCallback(const DownloadProgressObj &progress);
    EXPORT static void ProcessCloudData();
    EXPORT static void StopUpdateData();

    int32_t Start(const std::string &taskExtra) override;
    int32_t Stop(const std::string &taskExtra) override;

private:
    typedef struct {
        std::vector<std::string> uris;
        MediaType mediaType;
    } DownloadFiles;

    typedef struct {
        int32_t fileId;
        std::string path;
        std::string displayName;
        int64_t size;
        int32_t width;
        int32_t height;
        std::string mimeType;
        std::string mediaSuffix;
        int32_t duration;
        MediaType mediaType;
        bool isCloud;
        bool isVideo;
    } AbnormalData;

    typedef struct {
        std::vector<AbnormalData> abnormalData;
    } UpdateData;

    enum DownloadStatus : int32_t {
        INIT = 0,
        SUCCESS,
        NETWORK_UNAVAILABLE,
        STORAGE_FULL,
        STOPPED,
        UNKNOWN,
    };

    class DownloadCloudFilesData : public AsyncTaskData {
    public:
        DownloadCloudFilesData(DownloadFiles downloadFiles) : downloadFiles_(downloadFiles){};
        ~DownloadCloudFilesData() override = default;

        DownloadFiles downloadFiles_;
    };

    static void DownloadCloudFiles();
    static bool DownloadCloudFilesSync();
    static bool GetStorageFreeRatio(double &freeRatio);
    static void SetLastDownloadMilliSecond(int64_t lastDownloadMilliSecond);
    static int64_t GetLastDownloadMilliSecond();
    static void ClearDownloadCnt();
    static void UpdateDownloadCnt(std::string path, int64_t cnt);
    static int64_t GetDownloadCnt(std::string path);
    static std::shared_ptr<NativeRdb::ResultSet> QueryCloudFiles(double freeRatio);
    static void CheckAndUpdateDownloadCnt(std::string path, int64_t cnt);
    static void GetDownloadNum(int64_t &downloadNum);
    static void DownloadLatestFinished();
    static void ParseDownloadFiles(std::shared_ptr<NativeRdb::ResultSet> &resultSet, DownloadFiles &downloadFiles);
    static void removeFinishedResult(const std::vector<std::string>& downloadingPaths);
    static int32_t AddDownloadTask(const DownloadFiles &downloadFiles);
    static void DownloadCloudFilesExecutor(AsyncTaskData *data);
    static void StopDownloadFiles();
    static void UpdateCloudData();
    static void UpdateAbnormalDayMonthYear();
    static std::shared_ptr<NativeRdb::ResultSet> QueryUpdateData(bool isCloud, bool isVideo);
    static void SetPredicates(NativeRdb::RdbPredicates &predicates, bool isCloud, bool isVideo);
    static void ParseUpdateData(std::shared_ptr<NativeRdb::ResultSet> &resultSet, UpdateData &updateData,
        bool isCloud, bool isVideo);
    static void UpdateCloudDataExecutor(const UpdateData &updateData);
    static void UpdateAbnormaldata(std::unique_ptr<Metadata> &metadata, const std::string &tableName);
    static void GetSizeAndMimeType(std::unique_ptr<Metadata> &metadata);
    static int32_t GetExtractMetadata(std::unique_ptr<Metadata> &metadata);
    static void UpdateCurrentOffset(bool isCloud, bool isVideo);

    static int32_t downloadInterval_;
    static int32_t downloadDuration_;
    static std::recursive_mutex mutex_;
    static Utils::Timer timer_;
    static uint32_t startTimerId_;
    static uint32_t stopTimerId_;
    static bool isUpdating_;
    static int32_t cloudUpdateOffset_;
    static int32_t localImageUpdateOffset_;
    static int32_t localVideoUpdateOffset_;
    static int32_t cloudRetryCount_;
    static std::mutex downloadResultMutex_;
    static std::unordered_map<std::string, DownloadStatus> downloadResult_;
    static int64_t downloadId_;

    static const std::string taskName_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_H