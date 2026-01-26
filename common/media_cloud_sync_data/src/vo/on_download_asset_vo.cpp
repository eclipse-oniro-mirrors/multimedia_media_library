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

#define MLOG_TAG "Media_Cloud_Vo"

#include "on_download_asset_vo.h"

#include <sstream>

#include "cloud_lake_info.h"
#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool OnDownloadAssetReqBody::Unmarshalling(MessageParcel &parcel)
{
    IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->cloudIds, parcel);
    AdditionFileInfo::Unmarshalling(this->lakeInfos, parcel);
    return true;
}

bool OnDownloadAssetReqBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::Marshalling<std::string>(this->cloudIds, parcel);
    AdditionFileInfo::Marshalling(this->lakeInfos, parcel);
    return true;
}

std::string OnDownloadAssetReqBody::ToString() const
{
    std::stringstream ss;
    ss << "[";
    for (uint32_t i = 0; i < cloudIds.size(); ++i) {
        ss << "\"" << this->cloudIds[i] << "\"";
        if (i != cloudIds.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync