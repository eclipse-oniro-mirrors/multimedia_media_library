/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_SCANNER_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_SCANNER_H

#include <string>

#include "asset_accurate_refresh.h"
#include "metadata.h"
#include "medialibrary_rdbstore.h"
#include "custom_restore_types.h"
#ifdef MEDIALIBRARY_FEATURE_CUSTOM_RESTORE
#include "photo_custom_restore_operation.h"
#endif

namespace OHOS::Media {

class MediaFileInterworkScanner {
public:
    EXPORT virtual ~MediaFileInterworkScanner() = default;
    EXPORT static MediaFileInterworkScanner* GetInstance();
    EXPORT void ScanFileManager();
private:
    MediaFileInterworkScanner() = default;
    std::mutex asyncTaskMutex_;  // 异步任务互斥锁
    std::atomic<bool> isAsyncTaskRunning_{false};  // 异步任务运行状态标志
    std::thread taskThread_;
    map<std::string, int32_t> albumCache_;

    int32_t GetTaskStatus();
    int32_t SetTaskStatus(int32_t status);
    bool CheckSystemConditions();
    int32_t ScanDirectory(const std::string &path, std::vector<std::string> &files);
    bool IsImageOrVideoFile(const std::string &filePath);
    bool IsValidFileName(const std::string &fileName);
    bool ShouldSkipDirectory(const std::string &dirPath);
    int32_t ExecutePhaseOne();

    int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    void SetTimeInfo(const std::unique_ptr<Metadata> &data, RestoreFileInfo &info, NativeRdb::ValuesBucket &value);
    int32_t InsertFileBatch(const std::vector<std::string> &files);
    int32_t InsertOrUpdateAlbum(const std::string &albumPath, int32_t &albumId);
    int32_t CleanTabFileOptTable();
    int32_t FillMetadata(const RestoreFileInfo &fileInfo, std::unique_ptr<Metadata> &data);
    NativeRdb::ValuesBucket GetInsertValue(RestoreFileInfo &fileInfo);
    int32_t UpdateUniqueNumber(UniqueNumber &uniqueNumber);
    vector<RestoreFileInfo> SetDestinationPath(vector<RestoreFileInfo> &restoreFiles, UniqueNumber &uniqueNumber);
    vector<RestoreFileInfo> GetFileInfos(const std::vector<std::string> &filePathVector, UniqueNumber &uniqueNumber);
    std::vector<string> GetPhotosNotExists(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<std::string> &files);
    int32_t SetRestoreFileAlbumId(std::vector<RestoreFileInfo> &destRestoreFiles);
    std::shared_ptr<NativeRdb::ResultSet> GetOptFile(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);
    int32_t UpdateOptStatus(const std::shared_ptr<MediaLibraryRdbStore> rdbStore, std::vector<std::string> fileBatch);
    int32_t BatchUpdateTimePending(const vector<RestoreFileInfo> &restoreFiles,
        AccurateRefresh::AssetAccurateRefresh &assetRefresh);
    int32_t ProcessPhaseTwoRecords();

    int32_t BatchInsert(std::vector<RestoreFileInfo> &files);
    int32_t HandlePhotosRestore(const std::vector<std::string> &files);
    int32_t ExecutePhaseTwo();
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_SCANNER_H