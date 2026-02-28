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
 
#ifndef OHOS_FOLDER_SCANNER_HELPER_H
#define OHOS_FOLDER_SCANNER_HELPER_H
 
#include <string>
 
namespace OHOS {
namespace Media {
 
class FolderScannerHelper {
public:
    FolderScannerHelper(int32_t albumId, std::string path);
    bool IsFolderModified();
    void UpdateFolderModified();
    bool IsSkipFolderFile();
 
private:
    void InitLakeAlbumInfo();
    void InitFolderInfo();
    void InsertFolderModified();
private:
    int32_t albumId_;
    std::string storagePath_ {""};
    int64_t albumInfoDateModified_;
    std::string albumInfoPath_ {""};
    int64_t folderDateModified_;
    bool isInsert_ {false};
};
 
} // namespace Media
} // namespace OHOS
#endif