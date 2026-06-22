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
 

#ifndef FILE_MANAGER_SCANNER_H
#define FILE_MANAGER_SCANNER_H

#include "file_manager_parser.h"
#include "file_scanner.h"
#include "keyd_mutex.h"

namespace OHOS::Media {

const std::unordered_set<std::string> FILE_MANAGER_BLOCKED_DIRS = {
    "HO_DATA_EXT_MISC", ".Trash", ".thumbs", ".Recent", ".backup", ".VMDocs", ".ohpm", "PCEngine", "appdata",
};

class FileManagerScanner : public FileScanner {
public:
    FileManagerScanner(ScanMode scanMode = ScanMode::INCREMENT);
    ~FileManagerScanner() override = default;

    static bool IsSkipFileManagerDirectory(const std::string &currentDir);
private:
    void HandleFiles(MediaNotifyInfo& fileInfos) override;
    std::shared_ptr<FolderParser> BuildFolderParser(const std::string &path) override;
    bool IsIncrementScanConflict(std::vector<MediaNotifyInfo> fileInfos) override;
    bool IsSkipDirectory(const std::string &dir) override;
    
    bool IsSkipCurrentFile(const std::string &filePath);
    void RefreshTrashedAssetInfo(FileManagerParser &fileManagerParser);
    void RefreshNeedReoverAssetInfo(FileManagerParser &fileManagerParser);
    void RefreshInsertAssetInfo() override;
};
}

#endif