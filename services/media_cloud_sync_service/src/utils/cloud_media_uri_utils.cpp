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

#define MLOG_TAG "Media_Cloud_Utils"

#include "cloud_media_uri_utils.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "string_ex.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
int32_t CloudMediaUriUtils::GetFileIdFromUri(const std::string &uri, int32_t &fileUniqueId)
{
    static std::string mediaDirPath = "file://media/Photo/";
    size_t index = uri.find(mediaDirPath);
    if (index != 0) {
        MEDIA_INFO_LOG("Failed to get media dir from uri: %{public}s", MediaFileUtils::DesensitizePath(uri).c_str());
        return E_INVALID_VALUES;
    }
    std::string uriTails = uri.substr(index + mediaDirPath.length());
    index = uriTails.find('/');
    if (index == std::string::npos) {
        MEDIA_INFO_LOG("Failed to get fileId from uri: %{public}s", MediaFileUtils::DesensitizePath(uri).c_str());
        return E_INVALID_VALUES;
    }
    std::string fileId = uriTails.substr(0, index);
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        MEDIA_INFO_LOG("Invalid fileId from uri: %{public}s", fileId.c_str());
        return E_INVALID_VALUES;
    }
    if (!StrToInt(fileId, fileUniqueId)) {
        MEDIA_INFO_LOG("Invalid meida uri: %{public}s", MediaFileUtils::DesensitizePath(uri).c_str());
        return E_INVALID_VALUES;
    }
    return E_OK;
}

int32_t CloudMediaUriUtils::GetFileIds(const std::vector<std::string> &uris, std::vector<int32_t> &fileIds)
{
    fileIds.clear();
    for (auto &uri : uris) {
        int32_t fileId = 0;
        auto ret = CloudMediaUriUtils::GetFileIdFromUri(uri, fileId);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "Failed to get fileId from uri: %{public}s", uri.c_str());
        fileIds.push_back(fileId);
        MEDIA_DEBUG_LOG("GetFileIds uri:%{public}s, fileId:%{public}d", uri.c_str(), fileId);
    }
    return uris.size() == fileIds.size() ? E_OK : E_INVALID_VALUES;
}
}  // namespace OHOS::Media::CloudSync