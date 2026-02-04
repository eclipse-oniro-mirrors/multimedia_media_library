/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "cloud_media_data_client_handler_processor.h"

#include <string>
#include <vector>

#include "cloud_data_convert_to_vo.h"

namespace OHOS::Media::CloudSync {
std::vector<CloudMetaData> CloudMediaDataClientHandlerProcessor::GetCloudNewData(const std::vector<PhotosVo> &newDatas)
{
    std::vector<CloudMetaData> newCloudDatas;
    if (newDatas.size() <= 0) {
        return newCloudDatas;
    }
    for (const auto &newData : newDatas) {
        CloudMetaData newCloudData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(newData);
        newCloudDatas.emplace_back(newCloudData);
    }
    return newCloudDatas;
}

std::vector<CloudMetaData> CloudMediaDataClientHandlerProcessor::GetCloudFdirtyData(
    const std::vector<PhotosVo> &fdirtyDatas)
{
    std::vector<CloudMetaData> fdirtyCloudDatas;
    if (fdirtyDatas.size() <= 0) {
        return fdirtyCloudDatas;
    }
    for (const auto &fdirtyData : fdirtyDatas) {
        CloudMetaData fdirtyCloudData = CloudDataConvertToVo::ConvertPhotosVoToCloudMetaData(fdirtyData);
        fdirtyCloudDatas.emplace_back(fdirtyCloudData);
    }
    return fdirtyCloudDatas;
}
}  // namespace OHOS::Media::CloudSync