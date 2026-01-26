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

#include "get_dirty_type_stat_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool GetDirtyTypeStatRespBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::Unmarshalling<uint64_t>(this->statList, parcel);
}

bool GetDirtyTypeStatRespBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::Marshalling<uint64_t>(this->statList, parcel);
}

std::string GetDirtyTypeStatRespBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync