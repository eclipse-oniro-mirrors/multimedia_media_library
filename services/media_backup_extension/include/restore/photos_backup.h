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
#ifndef OHOS_MEDIA_PHOTOS_BACKUP
#define OHOS_MEDIA_PHOTOS_BACKUP

#include <string>
#include <vector>

#include "backup_const.h"
#include "photos_dao.h"
#include "rdb_open_callback.h"

namespace OHOS::Media {
class PhotosBackup {
public:
    struct BackupInfo {
        std::string suffix;
        int32_t photoCount {0};
        int32_t videoCount {0};
        int32_t cloudPhotoCount {0};
        int32_t cloudVideoCount {0};
        int32_t audioCount {0};
        size_t totalSize {0};
        int32_t lakePhotoCount{0};
        int32_t lakeVideoCount{0};
        size_t lakeTotalSize{0};
        std::string ToString() const;
    };

    PhotosBackup(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    int32_t CreateCloneFileInfoDb(AncoFileListClone ancoFileListClone,
        FileManagerFileListClone fileManagerFileListClone);
    std::string GetBackupInfo();

private:
    std::string GetBackupInfoOfMediaFile();
    std::string GetBackupInfoOfLakeFile();
    size_t GetBackupTotalSizeOfMediaFile();
    size_t GetBackupTotalSizeOfLakeFile();

    int32_t sceneCode_ {-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    PhotosDao photosDao_;
};
}  // namespace OHOS::Media
#endif