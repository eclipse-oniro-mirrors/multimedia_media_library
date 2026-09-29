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

#include "batch_update_metadata_modified_vo.h"

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media {

bool BatchUpdateMetaDataModifiedReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->fileIds, parcel);
}

bool BatchUpdateMetaDataModifiedReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::Marshalling<std::string>(this->fileIds, parcel);
}
} // namespace OHOS::Media