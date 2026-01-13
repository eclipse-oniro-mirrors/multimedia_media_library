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

#ifndef OHOS_MEDIA_CLOUD_SYNC_PHOTOS_DTO_H
#define OHOS_MEDIA_CLOUD_SYNC_PHOTOS_DTO_H

#include <string>
#include <map>
#include <sstream>

#include "medialibrary_errno.h"
#include "media_column.h"
#include "cloud_file_data_dto.h"
#include "cloud_media_sync_const.h"
#include "on_copy_records_photos_vo.h"
#include "userfile_manager_types.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT PhotosDto {
public:
    std::string data;
    std::string path;
    int64_t size;
    int64_t dateModified;
    int32_t dirty;
    int64_t dateTrashed;
    int32_t position;
    int32_t localId;
    std::string cloudId;
    std::string originalCloudId;
    std::string dkRecordId;
    int64_t cloudVersion;
    int32_t fileId;
    int32_t fileType;
    std::string relativePath;
    int64_t dateAdded;
    int64_t dateTaken;
    int32_t ownerAlbumId;
    int64_t metaDateModified;
    int64_t editedTimeMs;
    int32_t syncStatus;
    int32_t thumbStatus;
    std::string displayName;
    std::string fileName;
    int32_t orientation;
    int32_t subtype;
    int32_t movingPhotoEffectMode;
    int32_t originalSubtype;
    std::string sourcePath;
    std::string livePhotoCachePath;
    std::string mimeType;
    int32_t mediaType;
    int32_t attributesMediaType;
    int32_t serverErrorCode;
    ErrorType errorType;
    std::vector<CloudErrorDetail> errorDetails;
    std::string cloudAlbumId;
    int64_t modifiedTime;
    int64_t createTime;
    int32_t rotation;
    int64_t version;
    bool isSuccess;
    int32_t videoMode;
    int32_t fileSourceType;
    std::string storagePath;
    std::string localPath;
    int32_t hidden;
    std::map<std::string, CloudFileDataDto> attachment;

private:
    void GetAttachment(std::stringstream &ss) const;
    void GetBasicInfo(std::stringstream &ss) const;
    void GetAttributesInfo(std::stringstream &ss) const;
    void GetPropertiesInfo(std::stringstream &ss) const;

public:  // basic functions
    std::string ToString() const;
};

struct DownloadAssetData {
    int32_t fileId;
    std::string cloudId;
    int64_t fileSize;
    int32_t mediaType;
    std::string originalCloudId;
    std::string path;
    int64_t editTime;
    int32_t effectMode;
    int32_t orientation;
    int32_t fileSourceType;
    std::string storagePath;
    int32_t hidden;
    int64_t dateTrashed;
    int32_t subtype;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_PHOTOS_DTO_H