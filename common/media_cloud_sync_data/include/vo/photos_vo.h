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

#ifndef OHOS_MEDIA_CLOUD_SYNC_PHOTOS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_PHOTOS_VO_H

#include <string>
#include <map>
#include <sstream>

#include "i_media_parcelable.h"
#include "cloud_file_data_vo.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT PhotosVo : public IPC::IMediaParcelable {
public:
    int32_t fileId;
    std::string cloudId;
    int64_t size{0};
    std::string path;
    std::string fileName;
    std::string localPath;
    std::string originalCloudId;  // 复制体还未上行的下载
    int32_t type;
    int32_t orientation;
    int64_t modifiedTime;
    std::map<std::string, CloudFileDataVo> attachment;
    int32_t fileSourceType;
    std::string storagePath;
    int32_t hidden;
    int64_t dateTrashed;
    int32_t attributesMediaType;

public:  // functions of Parcelable.
    virtual ~PhotosVo() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:
    static bool Marshalling(const std::vector<PhotosVo> &result, MessageParcel &parcel);
    static bool Unmarshalling(std::vector<PhotosVo> &val, MessageParcel &parcel);

private:
    std::string ToString(std::map<std::string, CloudFileDataVo> dataMap) const;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_PHOTOS_VO_H