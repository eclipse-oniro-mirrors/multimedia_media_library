/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
 
#ifndef OHOS_MEDIA_CLOUD_SYNC_SCA_DETAIL_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_SCA_DETAIL_VO_H
 
#include <map>
#include <sstream>
#include <string>
#include <vector>
 
#include "cloud_media_define.h"
#include "i_media_parcelable.h"
 
namespace OHOS::Media::CloudSync {
// MDKScadetail 对应的 VO 结构体, 客户端解析后传递到服务端处理
class EXPORT ScaDetailVo : public IPC::IMediaParcelable {
public:
    std::string usage; // 风控对应的类型
    int32_t riskResult{0};
 
public: // functions of Parcelable.
    virtual ~ScaDetailVo() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
 
public: // basic functions
    std::string ToString() const;
};
 
// 照片下行共享风控详情: 当前用户角色判定 + scaDetail 列表
class EXPORT SharePhotoDetailVo : public IPC::IMediaParcelable {
public:
    std::string currentUserId;
    std::string mediaCreateId;
    std::vector<ScaDetailVo> scaDetailList;
 
public: // functions of Parcelable.
    virtual ~SharePhotoDetailVo() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
 
public: // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_SCA_DETAIL_VO_H