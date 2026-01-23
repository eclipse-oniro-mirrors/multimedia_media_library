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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_ERROR_DETAIL_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_ERROR_DETAIL_VO_H

#include <string>

#include "i_media_parcelable.h"

#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudErrorDetail : public IPC::IMediaParcelable {
public:
    std::string domain;       // 描述:[域];
    std::string reason;       // 描述:[错误原因];
    std::string errorCode;    // 描述:[系统内部错误编码, 参见错误码规范]，26004977、31004913等;
    std::string description;  // 描述:[错误描述信息];
    std::string errorPos;     // 描述:[错误位置类型];
    std::string errorParam;   // 描述:[错误位置];
    int detailCode;           // 描述:[错误编码, 参见DKDetailErrorCode];

public:  // functions of Parcelable.
    virtual ~CloudErrorDetail() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H