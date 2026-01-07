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
#ifndef OHOS_MEDIA_PHOTOS_DATA_HANDLER
#define OHOS_MEDIA_PHOTOS_DATA_HANDLER

#include <string>

#include "photos_dao.h"

namespace OHOS::Media {
class PhotosDataHandler {
public:
    void OnStart(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    void HandleDirtyFiles(bool isRestore);

private:
    PhotosDataHandler &SetSceneCode(int32_t sceneCode);
    PhotosDataHandler &SetTaskId(const std::string &taskId);
    PhotosDataHandler &SetMediaLibraryRdb(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    void HandleDirtyFilesBatch(int32_t offset);
    int32_t CleanDirtyFiles(const std::vector<PhotosDao::PhotosRowData> &dirtyFiles);
    int32_t DeleteDirtyFilesInDb();
    int32_t SetVisibleFilesInDb();
    bool DeleteDirtyFile(const PhotosDao::PhotosRowData &dirtyFile);
    bool ShouldSetVisible(const PhotosDao::PhotosRowData &dirtyFile);
    void AddToCleanFailedFiles(const PhotosDao::PhotosRowData &dirtyFile);
    void AddToSetVisibleFiles(const PhotosDao::PhotosRowData &dirtyFile);
    bool IsLocalFileExist(const PhotosDao::PhotosRowData &dirtyFile);
    bool IsFileExist(const PhotosDao::PhotosRowData &dirtyFile);
    void ReportDirtyFile(const std::string &errorCode, const PhotosDao::PhotosRowData &dirtyFile);

private:
    int32_t sceneCode_ {-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::mutex cleanFailedFilesMutex_;
    std::mutex setVisibleFilesMutex_;
    std::vector<std::string> cleanFailedFiles_;
    std::vector<std::string> setVisibleFiles_;
    PhotosDao photosDao_;
    std::atomic<int32_t> dirtyFileCleanNumber_ {0};
    std::atomic<int32_t> failedDirtyFileCleanNumber_ {0};
};
}  // namespace OHOS::Media

#endif  // OHOS_MEDIA_PHOTOS_DATA_HANDLER
