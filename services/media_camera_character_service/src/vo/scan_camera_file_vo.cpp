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

#define MLOG_TAG "MediaScanCameraFile"

#include "scan_camera_file_vo.h"

#include <sstream>

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool ScanCameraFileReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadBool(this->needUpdateAlbum);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadBool(this->needGenerateThumbnail);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadInt32(this->pathType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool ScanCameraFileReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->fileId);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteBool(this->needUpdateAlbum);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteBool(this->needGenerateThumbnail);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteInt32(this->pathType);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string ScanCameraFileReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(this->fileId) << "\","
       << "\"needUpdateAlbum\": \"" << std::to_string(static_cast<int32_t>(this->needUpdateAlbum)) << "\","
       << "\"needGenerateThumbnail\": \"" << std::to_string(static_cast<int32_t>(this->needGenerateThumbnail)) << "\","
       << "\"pathType\": \"" << std::to_string(this->pathType) << "\","
       << "}";
    return ss.str();
}
}  // namespace OHOS::Media