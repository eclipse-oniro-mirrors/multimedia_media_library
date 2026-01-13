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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_DEVICE_INFO_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_DEVICE_INFO_DATA_H_
#include "mtp_constants.h"
#include "payload_data.h"
#include "storage.h"
namespace OHOS {
namespace Media {
class GetDeviceInfoData : public PayloadData {
public:
    explicit GetDeviceInfoData(std::shared_ptr<MtpOperationContext> &context);
    explicit GetDeviceInfoData();
    ~GetDeviceInfoData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;

    uint32_t CalculateSize() override;
    void SetManufacturer(const std::string &manufacturer);
    void SetModel(const std::string &model);
    void SetVersion(const std::string &version);
    void SetSerialNum(const std::string &serialNum);

private:
    uint16_t standardVersion_ = 0;
    uint32_t vendorExtensionID_ = 0;
    uint16_t vendorExtensionVersion_ = 0;
    std::string vendorExtensionDesc_;
    uint16_t functionalMode_ = 0;
    std::vector<uint8_t> mOutBuffer;
    std::string manufacturer_;
    std::string model_;
    std::string version_;
    std::string serialNum_;

    std::string GetPropertyInner(const std::string &property, const std::string &value);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_DEVICE_INFO_DATA_H_
