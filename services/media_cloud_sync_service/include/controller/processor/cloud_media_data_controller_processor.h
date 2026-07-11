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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTROLLER_PROCESSOR_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTROLLER_PROCESSOR_H

#include <string>
#include <vector>

#include "photos_dto.h"
#include "photos_vo.h"

#include "media_operate_result_vo.h"
#include "media_operate_result_dto.h"
#include "aging_file_query_dto.h"
#include "get_aging_file_vo.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDataControllerProcessor {
public:
    PhotosVo ConvertPhotosDtoToPhotosVo(const PhotosDto &photosDto);
    std::vector<MediaOperateResultRespBodyResultNode> GetMediaOperateResult(
        const std::vector<MediaOperateResultDto> &mediaOperateResultDto);
    void GetAgingFileQueryDto(const GetAgingFileReqBody &reqBody, AgingFileQueryDto &queryDto);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTROLLER_PROCESSOR_H