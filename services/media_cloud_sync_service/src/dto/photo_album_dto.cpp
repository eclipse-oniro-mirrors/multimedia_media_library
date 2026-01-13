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

#define MLOG_TAG "MEDIA_CLOUD_DTO"

#include "photo_album_dto.h"

#include <sstream>

namespace OHOS::Media::CloudSync {
std::string PhotoAlbumDto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"albumId\": \"" << this->albumId << "\", "
       << "\"albumType\": " << this->albumType << ", "
       << "\"albumSubType\": " << this->albumSubType << ","
       << "\"lPath\": \"" << this->lPath << "\","
       << "\"bundleName\": \"" << this->bundleName << "\","
       << "\"priority\": " << this->priority << ","
       << "\"cloudId\": " << this->cloudId << ","
       << "\"newCloudId\": " << this->newCloudId << ","
       << "\"localLanguage\": \"" << this->localLanguage << "\","
       << "\"albumDateCreated\": " << this->albumDateCreated << ","
       << "\"albumDateModified\": " << this->albumDateModified << ","
       << "\"isDelete\": " << std::to_string(this->isDelete) << ","
       << "\"isSuccess\": " << std::to_string(this->isSuccess) << ","
       << "\"coverUriSource\": " << std::to_string(this->coverUriSource) << ","
       << "\"coverCloudId\": " << this->coverCloudId << ","
       << "\"serverErrorCode\": " << serverErrorCode << ","
       << "\"errorType\": \"" << static_cast<int32_t>(errorType) << "\","
       << "\"errorDetails\": [";
    for (uint32_t i = 0; i < errorDetails.size(); ++i) {
        ss << errorDetails[i].ToString();
        if (i != errorDetails.size() - 1) {
            ss << ",";
        }
    }
    ss << "]}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync