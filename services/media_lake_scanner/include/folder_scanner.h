/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#ifndef FOLDER_SCANNER_H
#define FOLDER_SCANNER_H
 
#include <string>
#include <vector>
#include <queue>
#include <unordered_set>

#include "file_parser.h"
#include "folder_parser.h"
#include "folder_scanner_utils.h"
#include "lake_const.h"
#include "media_lake_notify_info.h"
#include "medialibrary_notify.h"
#include "rdb_store.h"
#include "rdb_helper.h"
#include "scanner_utils.h"
#include "folder_scanner_helper.h"
 
namespace OHOS::Media {

class FolderScanner {
public:
    EXPORT FolderScanner(const std::string &path, LakeScanMode scanMode = LakeScanMode::INCREMENT);
    EXPORT FolderScanner(const MediaLakeNotifyInfo &fileInfo, LakeScanMode scanMode = LakeScanMode::INCREMENT);
    EXPORT virtual ~FolderScanner();

    int32_t Run();
    int32_t ScanCurrentDirectory(queue<std::string> &subDirQueue);

    int32_t GetAlbumId();
    void GetFileIds(std::vector<int32_t> &fileIds);
    bool IsScanFolderFile();
    int32_t GetAddCount();
    int32_t GetUpdateCount();

private:
    int32_t HandleNeedInsertedAssets(FileParser &fileParser,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    int32_t HandleNeedUpdateAssets(FileParser &fileParser);
    int32_t HandleFiles(std::string &currentFilePath,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    void UpdateAndNotifyAlbumInfos(bool isNeedUpdateSystemAlbum);
    int32_t BatchInsertAssets(const string &tableName, std::vector<NativeRdb::ValuesBucket> &values);
    void CheckSetFileScannerSkip(FolderOperationType type);
    void CheckUpdateFolderModified();
    bool IsIncrementScanConflict();
    void BatchInsertAssets(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    FolderScanner BuildSubDirFolderScanner(const std::string &currentSubDir);
    FileParser BuildFileParser(const std::string &currentFilePath);
    void BuildNotifyInfo(const std::string &targetPath, MediaLakeNotifyInfo &notifyInfo);

private:
    unordered_set<std::string> notifyAlbumIds_;
    unordered_set<std::string> notifyFileUris_;
    std::string rootPath_;
    MediaLakeNotifyInfo notifyFolderInfo_;
    FolderScannerInitialType fsInitialType_{FolderScannerInitialType::DEFAULT};
    std::vector<NativeRdb::ValuesBucket> insertFileBuckets_;
    FolderParser folderParser_;
    LakeAlbumInfo albumInfos_;
    std::shared_ptr<FolderScannerHelper> folderScannerHelperPtr;
    std::vector<int32_t> fileIds_;
    const LakeScanMode scanMode_;
    std::vector<std::string> inodes_;
    int32_t addCount_{0};
    int32_t updateCount_{0};
};

} // namespace OHOS::Media
#endif // FOLDER_SCANNER_H