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
 

#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include <vector>
#include <string>

#include "file_parser.h"
#include "folder_parser.h"
#include "values_bucket.h"
#include "media_lake_notify_info.h"

namespace OHOS::Media {

const int32_t RET_SUCCESS = 0;

struct FolderParserInfo {
    shared_ptr<FolderParser> folderParser_;
    std::string currentFolderPath_;
    FolderOperationType currentOptType_{FolderOperationType::SKIP};
    bool isSkipFolder = false;
};

class FileScanner {
public:
    FileScanner(LakeScanMode scanMode = LakeScanMode::INCREMENT);
    int32_t Run(std::vector<MediaLakeNotifyInfo> fileInfos);

private:
    std::string GetFolder(std::string file);
    void RefreshUpdateAssetInfo(FileParser &fileParser);
    void GetInsertAssetInfo(MediaLakeNotifyInfo &fileInfo, FileParser &fileParser);
    void RefreshInsertAssetInfo();
    void UpdateAlbum();
    void RefreshUpdateAssetAlbumInfo(MediaLakeNotifyInfo &fileInfo, FileParser &fileParser);
    bool CheckUpdateFolderParserInfo(MediaLakeNotifyInfo &fileInfo);
    bool IsIncrementScanConflict(std::vector<MediaLakeNotifyInfo> fileInfos);
    void RefreshAssetInfoForSkipFile(FileParser &fileParser, FileUpdateType type);
    void DeleteRelatedResource(InnerFileInfo &fileIinfo);

private:
    vector<NativeRdb::ValuesBucket> insertFileInfos_;
    vector<std::string> inodes_;
    std::vector<std::string> albumIds_;
    FolderParserInfo folderParserInfo_;
    const LakeScanMode scanMode_;
    int32_t addCount_{0};
    int32_t updateCount_{0};
};
}

#endif