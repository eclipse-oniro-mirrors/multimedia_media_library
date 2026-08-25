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

#define MLOG_TAG "Media_Cloud_Controller"

#include "cloud_media_album_controller_processor.h"

#include "media_log.h"
#include "cloud_file_data_vo.h"
#include "cloud_file_data_dto.h"
#include "photos_vo.h"
#include "photos_dto.h"

namespace OHOS::Media::CloudSync {
CloudMdkRecordPhotoAlbumVo CloudMediaAlbumControllerProcessor::ConvertRecordPoToVo(PhotoAlbumPo record)
{
    CloudMdkRecordPhotoAlbumVo recordVo;
    recordVo.albumId = record.albumId.value_or(0);
    recordVo.albumType = record.albumType.value_or(0);
    recordVo.albumName = record.albumName.value_or("");
    recordVo.lpath = record.lpath.value_or("");
    recordVo.cloudId = record.cloudId.value_or("");
    recordVo.albumSubtype = record.albumSubtype.value_or(0);
    recordVo.dateAdded = record.dateAdded.value_or(0);
    recordVo.dateModified = record.dateModified.value_or(0);
    recordVo.bundleName = record.bundleName.value_or("");
    recordVo.localLanguage = record.localLanguage.value_or("");
    recordVo.coverUriSource = record.coverUriSource.value_or(0);
    recordVo.coverCloudId = record.coverCloudId.value_or("");
    recordVo.uniqueId = record.uniqueId.value_or("");
    recordVo.shareAlbumOwner = record.shareAlbumOwner.value_or("");
    /* album_plugin columns */
    recordVo.albumPluginCloudId = record.albumPluginCloudId.value_or("");
    recordVo.albumNameEn = record.albumNameEn.value_or("");
    recordVo.dualAlbumName = record.dualAlbumName.value_or("");
    recordVo.priority = record.priority.value_or(0);
    recordVo.isInWhiteList = record.isInWhiteList.value_or(false);
    return recordVo;
}

PhotoAlbumDto CloudMediaAlbumControllerProcessor::ConvertToPhotoAlbumDto(
    const OnCreateRecordsAlbumReqBodyAlbumData &recordVo)
{
    PhotoAlbumDto record;
    record.cloudId = recordVo.cloudId;
    record.newCloudId = recordVo.newCloudId;
    record.isSuccess = recordVo.isSuccess;
    record.errorType = recordVo.errorType;
    record.serverErrorCode = recordVo.serverErrorCode;
    record.errorDetails = recordVo.errorDetails;
    return record;
}

PhotoAlbumDto CloudMediaAlbumControllerProcessor::ConvertToPhotoAlbumDto(
    const OnMdirtyAlbumRecord &recordVo)
{
    PhotoAlbumDto record;
    record.cloudId = recordVo.cloudId;
    record.isSuccess = recordVo.isSuccess;
    record.errorType = recordVo.errorType;
    record.serverErrorCode = recordVo.serverErrorCode;
    record.errorDetails = recordVo.errorDetails;
    return record;
}
}  // namespace OHOS::Media::CloudSync