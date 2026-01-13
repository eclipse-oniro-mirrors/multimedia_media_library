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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_INFO_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_INFO_DATA_H_
#include "payload_data.h"
#include "object_info.h"
namespace OHOS {
namespace Media {
class SendObjectInfoData : public PayloadData {
public:
    explicit SendObjectInfoData(std::shared_ptr<MtpOperationContext> &context);
    explicit SendObjectInfoData();
    ~SendObjectInfoData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;
    uint32_t CalculateSize() override;

    bool SetSetParam(uint32_t storageID, uint32_t parent, uint32_t handle);

private:
    int ParserData(const std::vector<uint8_t> &buffer, size_t &offset);
    int ParserDataForImageInfo(const std::vector<uint8_t> &buffer, size_t &offset);
    int ParserDataForFileInfo(const std::vector<uint8_t> &buffer, size_t &offset);

private:
    bool hasSetParam_ {false};
    uint32_t storageID_ {0};
    uint32_t parent_ {0};
    uint32_t handle_ {0};
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_SEND_OBJECT_INFO_DATA_H_
