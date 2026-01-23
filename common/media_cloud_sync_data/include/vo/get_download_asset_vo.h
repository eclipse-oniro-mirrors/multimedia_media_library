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

#ifndef OHOS_MEDIA_CLOUD_SYNC_GET_DOWNLOAD_ASSET_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_GET_DOWNLOAD_ASSET_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "photos_vo.h"
#include "media_itypes_utils.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT GetDownloadAssetReqBody : public IPC::IMediaParcelable {
public:
    std::vector<std::string> pathList;
    std::vector<std::string> fileKeyList;

public:  // functions of Parcelable.
    virtual ~GetDownloadAssetReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT GetDownloadAssetRespBody : public IPC::IMediaParcelable {
public:
    std::vector<PhotosVo> photos;

public:  // functions of Parcelable.
    virtual ~GetDownloadAssetRespBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_GET_DOWNLOAD_ASSET_VO_H