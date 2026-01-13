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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_PROCESSOR_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_PROCESSOR_H

#include <string>
#include <vector>

#include "photos_dto.h"
#include "photos_vo.h"
#include "get_check_records_vo.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "on_fetch_photos_vo.h"
#include "cloud_media_pull_data_dto.h"
#include "on_create_records_photos_vo.h"
#include "on_modify_file_dirty_vo.h"
#include "on_modify_records_photos_vo.h"
#include "report_failure_vo.h"
#include "report_failure_dto.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaPhotoControllerProcessor {
public:
    std::vector<PhotosVo> SetFdirtyDataVoFromDto(std::vector<PhotosDto> &fdirtyDataDtos);
    std::vector<PhotosVo> SetNewDataVoFromDto(std::vector<PhotosDto> &newDataDtos);
    std::unordered_map<std::string, GetCheckRecordsRespBodyCheckData> GetCheckRecordsRespBody(
        std::vector<PhotosDto> photosDtoVec);
    CloudMdkRecordPhotosVo ConvertRecordPoToVo(const PhotosPo &record);
    CloudMediaPullDataDto ConvertToCloudMediaPullData(const OnFetchPhotosVo &photosVo);
    PhotosDto ConvertToPhotoDto(const OnCreateRecord &recordVo);
    void ConvertToPhotosDto(const OnFileDirtyRecord &recordVo, PhotosDto &dto);
    void ConvertToPhotosDto(const OnModifyRecord &recordVo, PhotosDto &dto);
    ReportFailureDto GetReportFailureDto(const ReportFailureReqBody &reqBody);

private:
    // functions for ConvertRecordPoToVo
    bool GetBasicInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);
    bool GetAttributesInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);
    bool GetPropertiesInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);
    bool GetCloudInfo(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);
    // functions for ConvertToCloudMediaPullData
    bool GetBasicInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool GetAttributesInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool GetPropertiesInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool GetCloudInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool GetAlbumInfo(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool GetAttributesHashMap(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);
    bool GetAttributesHashMap(const OnFetchPhotosVo &photosVo, CloudMediaPullDataDto &data);
    bool HandleUserComment(const PhotosPo &record, CloudMdkRecordPhotosVo &photosVo);

private:
    static constexpr int32_t USER_COMMENT_LIMIT_SIZE = 1024;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTO_CONTROLLER_PROCESSOR_H