/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_INFO_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_INFO_DATA_H_
#include "payload_data.h"
#include "object_info.h"
namespace OHOS {
namespace Media {
class GetObjectInfoData : public PayloadData {
public:
    explicit GetObjectInfoData(std::shared_ptr<MtpOperationContext> &context);
    explicit GetObjectInfoData();
    ~GetObjectInfoData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;
    uint32_t CalculateSize() override;

    bool SetObjectInfo(std::shared_ptr<ObjectInfo> &objectInfo);

private:
    std::shared_ptr<ObjectInfo> GetObjectInfo();

private:
    bool hasSetObjectInfo_ {false};
    std::shared_ptr<ObjectInfo> objectInfo_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_INFO_DATA_H_
