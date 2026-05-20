/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCreateCameraFileFd"

#include "create_camera_file_fd_vo.h"

#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool CreateCameraFileFdReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadString(this->mode);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->pathType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool CreateCameraFileFdReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteString(this->mode);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->pathType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string CreateCameraFileFdReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(this->fileId) << "\","
       << "\"mode\": \"" << this->mode << "\","
       << "\"pathType\": \"" << std::to_string(this->pathType)
       << "}";
    return ss.str();
}

bool CreateCameraFileFdRespBody::Unmarshalling(MessageParcel &parcel)
{
    this->fd = parcel.ReadFileDescriptor();
    return true;
}

bool CreateCameraFileFdRespBody::Marshalling(MessageParcel &parcel) const
{
    if (this->fd < 0) {
        MEDIA_ERR_LOG("invalid fd: %{public}d.", this->fd);
        return false;
    }
    bool status = parcel.WriteFileDescriptor(this->fd);
    if (!status) {
        MEDIA_ERR_LOG("fail to WriteFileDescriptor fd: %{public}d.", this->fd);
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

std::string CreateCameraFileFdRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fd\": \"" << std::to_string(this->fd)
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media