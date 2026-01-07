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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DRIVER_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DRIVER_H_
#include <memory>
#include <stdint.h>
#include <vector>
#include "mtp_constants.h"
#include "v1_0/iusbfn_mtp_interface.h"
namespace OHOS {
namespace Media {
class MtpDriver {
public:
    MtpDriver();
    ~MtpDriver();
    int OpenDriver();
    int CloseDriver();

    int Read(std::vector<uint8_t> &outBuffer, uint32_t &outReadSize);
    void Write(std::vector<uint8_t> &buffer, uint32_t &bufferSize, int32_t &result);

    int ReceiveObj(MtpFileRange &mfr);
    int SendObj(MtpFileRange &mfr);
    int WriteEvent(EventMtp &em);
private:
    bool usbOpenFlag {false};
    sptr<OHOS::HDI::Usb::Gadget::Mtp::V1_0::IUsbfnMtpInterface> usbfnMtpInterface = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DRIVER_H_
