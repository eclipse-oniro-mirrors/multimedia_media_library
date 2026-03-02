/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "photo_attachment_dto.h"

#include <sstream>

namespace OHOS::Media::CloudSync {
std::string PhotoAttachmentDto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": " << this->fileId << ", "
       << "\"attachmentSize\": " << this->attachementSize << ", "
       << "\"cloudPath\": " << this->cloudPath << ", ";
    ss << "\"attachments\": [";
    for (const auto &node : this->attachements) {
        ss << "\"" << node << "\", ";
    }
    ss << "], ";
    ss << "\"deletedAttachments\": [";
    for (const auto &node : this->deletedAttachments) {
        ss << "\"" << node << "\", ";
    }
    ss << "]"
       << "}";
    return ss.str();
}
} // namespace OHOS::Media::CloudSync