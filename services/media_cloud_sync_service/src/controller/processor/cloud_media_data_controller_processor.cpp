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

#include "cloud_media_data_controller_processor.h"

#include "media_log.h"
#include "cloud_file_data_vo.h"
#include "cloud_file_data_dto.h"
#include "photos_vo.h"
#include "photos_dto.h"

namespace OHOS::Media::CloudSync {
PhotosVo CloudMediaDataControllerProcessor::ConvertPhotosDtoToPhotosVo(const PhotosDto &photosDto)
{
    MEDIA_DEBUG_LOG("ConvertPhotosDtoToPhotosVo, photosDto: %{public}s.", photosDto.ToString().c_str());
    PhotosVo photosVo;
    photosVo.fileId = photosDto.fileId;
    photosVo.cloudId = photosDto.cloudId;
    photosVo.size = photosDto.size;
    photosVo.path = photosDto.path;
    photosVo.fileName = photosDto.fileName;
    photosVo.type = photosDto.mediaType;
    photosVo.originalCloudId = photosDto.originalCloudId;
    photosVo.fileSourceType = photosDto.fileSourceType;
    photosVo.storagePath = photosDto.storagePath;
    for (auto &nodePair : photosDto.attachment) {
        CloudFileDataVo fileDataVo;
        fileDataVo.fileName = nodePair.second.fileName;
        fileDataVo.filePath = nodePair.second.path;
        fileDataVo.size = nodePair.second.size;
        photosVo.attachment[nodePair.first] = fileDataVo;
    }
    return photosVo;
}

std::vector<MediaOperateResultRespBodyResultNode> CloudMediaDataControllerProcessor::GetMediaOperateResult(
    const std::vector<MediaOperateResultDto> &mediaOperateResultDto)
{
    std::vector<MediaOperateResultRespBodyResultNode> res;
    for (auto &node : mediaOperateResultDto) {
        MediaOperateResultRespBodyResultNode resultNode;
        resultNode.cloudId = node.cloudId;
        resultNode.errorCode = node.errorCode;
        resultNode.errorMsg = node.errorMsg;
        res.emplace_back(resultNode);
    }
    return res;
}

void CloudMediaDataControllerProcessor::GetAgingFileQueryDto(
    const GetAgingFileReqBody &reqBody, AgingFileQueryDto &queryDto)
{
    queryDto.time = reqBody.time;
    queryDto.mediaType = reqBody.mediaType;
    queryDto.sizeLimit = reqBody.sizeLimit;
    queryDto.offset = reqBody.offset;
    return;
}
}  // namespace OHOS::Media::CloudSync