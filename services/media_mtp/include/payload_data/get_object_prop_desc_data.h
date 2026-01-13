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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_PROP_DESC_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_PROP_DESC_DATA_H_
#include "mtp_constants.h"
#include "payload_data.h"
#include "property.h"
namespace OHOS {
namespace Media {
class GetObjectPropDescData : public PayloadData {
public:
    explicit GetObjectPropDescData(std::shared_ptr<MtpOperationContext> &context);
    ~GetObjectPropDescData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;
    uint32_t CalculateSize() override;

private:
    std::shared_ptr<Property> GetProp();
    std::shared_ptr<Property> GetPropInt();
    std::shared_ptr<Property> GetPropStr();
    std::shared_ptr<Property> GetPropForm();
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_OBJECT_PROP_DESC_DATA_H_
