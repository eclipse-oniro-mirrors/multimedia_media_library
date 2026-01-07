/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SCANNER_OBJ_H
#define MEDIA_SCANNER_OBJ_H

#include <algorithm>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <ftw.h>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <securec.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

#include "medialibrary_type_const.h"
#include "medialibrary_errno.h"
#include "media_scanner_const.h"
#include "media_scanner_db.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "scanner_utils.h"
#include "imedia_scanner_callback.h"
#include "iremote_object.h"
#include "userfile_manager_types.h"

#define FREE_MEMORY_AND_SET_NULL(fName)      \
    do {                                     \
        free(fName);                         \
        fName = nullptr;                     \
    } while (0)

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
/**
 * Media Scanner class for scanning files and folders in MediaLibrary Database
 * and updating the metadata for each media file
 *
 * @since 1.0
 * @version 1.0
 */
class MediaScannerObj {
public:
    enum ScanType {
        FILE,
        DIRECTORY,
        START,
        ERROR,
        SET_ERROR,
        CAMERA_SHOT_MOVING_PHOTO
    };

    EXPORT MediaScannerObj(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
        MediaScannerObj::ScanType type, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT MediaScannerObj(MediaScannerObj::ScanType type, MediaLibraryApi api = MediaLibraryApi::API_OLD);
    EXPORT virtual ~MediaScannerObj() = default;

    EXPORT void Scan();

    /* stop */
    EXPORT void SetStopFlag(std::shared_ptr<bool> &stopFlag);

    /* Record Error */
    void SetErrorPath(const std::string &path);

    /* set is Force Scan */
    void SetForceScan(bool isForceScan);

    void SetFileId(int32_t fileId);

    void SetIsSkipAlbumUpdate(bool isSkipAlbumUpdate);

private:
    /* file */
    EXPORT int32_t ScanFile();
    EXPORT int32_t ScanFileInternal();
    int32_t BuildFileInfo(const std::string &parent, int32_t parentId);
    int32_t BuildData(const struct stat &statInfo);
    EXPORT int32_t GetFileMetadata();
    EXPORT int32_t GetParentDirInfo(const std::string &parent, int32_t parentId);
    EXPORT int32_t GetMediaInfo();
#ifdef MEDIALIBRARY_COMPATIBILITY
    void SetPhotoSubType(const std::string &parent);
#endif

    /* dir */
    EXPORT int32_t ScanDir();
    EXPORT int32_t ScanDirInternal();
    EXPORT int32_t ScanFileInTraversal(const std::string &path, const std::string &parent, int32_t parentId);
    EXPORT int32_t WalkFileTree(const std::string &path, int32_t parentId);
    EXPORT int32_t CleanupDirectory();
    EXPORT int32_t InsertOrUpdateAlbumInfo(const std::string &albumPath, int32_t parentId,
        const std::string &albumName);

    /* error */
    int32_t Start();
    int32_t ScanError(bool isBoot = false);

    /* set error */
    int32_t SetError();

    /* db ops */
    EXPORT int32_t Commit();
    EXPORT int32_t AddToTransaction();
    EXPORT int32_t CommitTransaction();

    /* callback */
    EXPORT int32_t InvokeCallback(int32_t code);

    ScanType type_;
    std::string path_;
    std::string dir_;
    std::string uri_;
    std::string errorPath_;
    bool skipPhoto_ = true;
    std::unique_ptr<MediaScannerDb> mediaScannerDb_;
    std::shared_ptr<IMediaScannerCallback> callback_;
    std::shared_ptr<bool> stopFlag_;

    std::unique_ptr<Metadata> data_;
    std::unordered_map<std::string, Metadata> albumMap_;
    std::set<std::pair<std::string, int32_t>> scannedIds_;
    std::vector<std::unique_ptr<Metadata>> dataBuffer_;
    MediaLibraryApi api_;
    bool isForceScan_ = false;
    int32_t fileId_ = 0;
    bool isSkipAlbumUpdate_ = false;
    bool isCameraShotMovingPhoto_ = false;
    bool needUpdateAssetName_ = true;
};

class ScanErrCallback : public IMediaScannerCallback {
public:
    ScanErrCallback(const std::string &err) : err_(err) {};
    ~ScanErrCallback() = default;

    int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) override
    {
        if (status == E_OK) {
            return MediaScannerDb::GetDatabaseInstance()->DeleteError(err_);
        }

        return E_OK;
    }

private:
    std::string err_;
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_SCANNER_OBJ_H
