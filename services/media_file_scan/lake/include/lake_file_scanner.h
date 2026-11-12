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
 

#ifndef LAKE_FILE_SCANNER_H
#define LAKE_FILE_SCANNER_H

#include <vector>
#include <string>

#include "file_scanner.h"

namespace OHOS::Media {

class LakeFileParser;

class LakeFileScanner : public FileScanner {
public:
    LakeFileScanner(ScanMode scanMode = ScanMode::INCREMENT);
    ~LakeFileScanner() override = default;

private:
    void HandleFiles(MediaNotifyInfo& fileInfos) override;
    bool IsIncrementScanConflict(std::vector<MediaNotifyInfo> fileInfos) override;
    bool IsSkipDirectory(const std::string &dir) override;
    std::shared_ptr<FolderParser> BuildFolderParser(const std::string &path) override;

private:
    void RefreshTrashedAssetInfo(LakeFileParser &lakefileParser);
    void RefreshNeedReoverAssetInfo(LakeFileParser &lakefileParser);
};
}

#endif