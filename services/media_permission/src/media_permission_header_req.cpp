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
#define MLOG_TAG "MediaPermissionCheck"
#include "media_permission_header_req.h"

#include "media_column.h"

#include <sstream>

namespace OHOS::Media {
const std::string PermissionHeaderReq::FILE_ID_KEY = MediaColumn::MEDIA_ID;
const std::string PermissionHeaderReq::URI_TYPE_KEY = "uriType";
const std::string PermissionHeaderReq::OPEN_URI_KEY = "openUri";
const std::string PermissionHeaderReq::OPEN_MODE_KEY = "openMode";

PermissionHeaderReq::PermissionHeaderReq(const std::string& fileId, int32_t uriType)
    : fileId_(fileId), uriType_(uriType)
{
    headerReq_[FILE_ID_KEY] = fileId_;
    headerReq_[URI_TYPE_KEY] = std::to_string(uriType_);
}

PermissionHeaderReq::PermissionHeaderReq(const std::string& openUri, const std::string& openMode)
    : openUri_(openUri), openMode_(openMode)
{
    headerReq_[OPEN_URI_KEY] = openUri_;
    headerReq_[OPEN_MODE_KEY] = openMode_;
}

PermissionHeaderReq::PermissionHeaderReq(const std::string& fileId, int32_t uriType,
    const std::string& openUri, const std::string& openMode)
    : fileId_(fileId), uriType_(uriType), openUri_(openUri), openMode_(openMode)
{
    headerReq_[FILE_ID_KEY] = fileId_;
    headerReq_[URI_TYPE_KEY] = std::to_string(uriType_);
    headerReq_[OPEN_URI_KEY] = openUri_;
    headerReq_[OPEN_MODE_KEY] = openMode_;
}

static bool CanConvertToInt32(const std::string &str)
{
    std::istringstream iss(str);
    int32_t num = 0;
    iss >> num;
    return iss.eof() && !iss.fail();
}

PermissionHeaderReq PermissionHeaderReq::convertToPermissionHeaderReq(
    const std::unordered_map<std::string, std::string> &header, int32_t userId,
    std::vector<std::vector<PermissionType>> &permissionPolicy, bool isDBBypass)
{
    PermissionHeaderReq permissionHeaderReq;
    int32_t uriType = -1;
    if (header.count(URI_TYPE_KEY)) {
        if (CanConvertToInt32(header.at(URI_TYPE_KEY))) {
            uriType = static_cast<int32_t>(std::stoi(header.at(URI_TYPE_KEY)));
        }
    }
    if (header.count(FILE_ID_KEY) && header.count(URI_TYPE_KEY) &&
        header.count(OPEN_URI_KEY) && header.count(OPEN_MODE_KEY)) {
        permissionHeaderReq = PermissionHeaderReq(header.at(FILE_ID_KEY), uriType,
            header.at(OPEN_URI_KEY), header.at(OPEN_MODE_KEY));
    } else if (header.count(FILE_ID_KEY) && header.count(URI_TYPE_KEY)) {
        permissionHeaderReq = PermissionHeaderReq(header.at(FILE_ID_KEY), uriType);
    } else if (header.count(OPEN_URI_KEY) && header.count(OPEN_MODE_KEY)) {
        permissionHeaderReq = PermissionHeaderReq(header.at(OPEN_URI_KEY), header.at(OPEN_MODE_KEY));
    } else {
        permissionHeaderReq = PermissionHeaderReq();
    }
    permissionHeaderReq.setUserId(userId);
    permissionHeaderReq.setPermissionPolicy(permissionPolicy);
    permissionHeaderReq.setDBBypass(isDBBypass);
    return permissionHeaderReq;
}

std::string PermissionHeaderReq::getFileId() const
{
    return fileId_;
}

int32_t PermissionHeaderReq::getUriType() const
{
    return uriType_;
}

std::string PermissionHeaderReq::getOpenUri() const
{
    return openUri_;
}

std::string PermissionHeaderReq::getOpenMode() const
{
    return openMode_;
}

int32_t PermissionHeaderReq::getUserId() const
{
    return userId_;
}

void PermissionHeaderReq::setUserId(int32_t userId)
{
    userId_ = userId;
}

std::vector<std::vector<PermissionType>> PermissionHeaderReq::getPermissionPolicy() const
{
    return permissionPolicy_;
}

void PermissionHeaderReq::setPermissionPolicy(std::vector<std::vector<PermissionType>>& permissionPolicy)
{
    permissionPolicy_ = permissionPolicy;
}

bool PermissionHeaderReq::getIsDBBypass() const
{
    return isDBBypass_;
}

void PermissionHeaderReq::setDBBypass(bool isDBBypass)
{
    isDBBypass_ = isDBBypass;
}
} // namespace OHOS::Media
