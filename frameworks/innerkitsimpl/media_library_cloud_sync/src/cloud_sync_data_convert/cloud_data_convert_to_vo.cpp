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
#define MLOG_TAG "Media_Client"

#include "cloud_data_convert_to_vo.h"
#include "cloud_meta_data.h"
#include "cloud_download_file_meta.h"

#include <string>
#include "media_log.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
CloudMetaData CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(const PhotosVo &photosVo)
{
    MEDIA_DEBUG_LOG("photosVo: %{public}s", photosVo.ToString().c_str());
    CloudMetaData cloudMetaData;
    cloudMetaData.fileId = photosVo.fileId;
    cloudMetaData.cloudId = photosVo.cloudId;
    cloudMetaData.size = photosVo.size;
    cloudMetaData.path = photosVo.path;
    cloudMetaData.fileName = photosVo.fileName;
    cloudMetaData.type = photosVo.type;
    cloudMetaData.modifiedTime = photosVo.modifiedTime;
    cloudMetaData.originalCloudId = photosVo.originalCloudId;
    cloudMetaData.fileSourceType = photosVo.fileSourceType;
    cloudMetaData.storagePath = photosVo.storagePath;
    for (const auto &nodePair : photosVo.attachment) {
        CloudFileData fileData;
        fileData.filePath = nodePair.second.filePath;
        fileData.fileName = nodePair.second.fileName;
        fileData.size = nodePair.second.size;
        cloudMetaData.attachment[nodePair.first] = fileData;
    }
    return cloudMetaData;
}

CloudDlFileMeta CloudDataConvertToVo::ConvertPhotosVoToLakeCloudMetaData(const PhotosVo &photosVo)
{
    MEDIA_INFO_LOG("ConvertPhotosVoToCloudMetaData, photosVo: %{public}s.", photosVo.ToString().c_str());
    CloudDlFileMeta cloudMetaData;
    cloudMetaData.fileId = photosVo.fileId;
    cloudMetaData.cloudId = photosVo.cloudId;
    cloudMetaData.size = photosVo.size;
    cloudMetaData.path = photosVo.path;
    cloudMetaData.fileName = photosVo.fileName;
    cloudMetaData.type = photosVo.type;
    cloudMetaData.modifiedTime = photosVo.modifiedTime;
    cloudMetaData.originalCloudId = photosVo.originalCloudId;
    cloudMetaData.fileSourceType = photosVo.fileSourceType;
    cloudMetaData.storagePath = photosVo.storagePath;
    cloudMetaData.hidden = photosVo.hidden;
    cloudMetaData.dateTrashed = photosVo.dateTrashed;
    for (const auto &nodePair : photosVo.attachment) {
        CloudFileData fileData;
        fileData.filePath = nodePair.second.filePath;
        fileData.fileName = nodePair.second.fileName;
        fileData.size = nodePair.second.size;
        cloudMetaData.attachment[nodePair.first] = fileData;
    }
    return cloudMetaData;
}
}  // namespace OHOS::Media::CloudSync