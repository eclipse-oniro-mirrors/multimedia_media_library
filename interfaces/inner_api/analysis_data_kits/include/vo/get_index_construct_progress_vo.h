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

#ifndef MEDIA_ANALYSIS_DATA_KITS_GET_INDEX_CONSTRUCT_PROGRESS_VO_H
#define MEDIA_ANALYSIS_DATA_KITS_GET_INDEX_CONSTRUCT_PROGRESS_VO_H

#include "i_media_parcelable.h"

namespace OHOS::Media {
class GetIndexConstructProgressRespBody : public IPC::IMediaParcelable {
public:
    std::string indexProgress;

    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
};
}  // namespace OHOS::Media
#endif  // MEDIA_ANALYSIS_DATA_KITS_GET_INDEX_CONSTRUCT_PROGRESS_VO_H