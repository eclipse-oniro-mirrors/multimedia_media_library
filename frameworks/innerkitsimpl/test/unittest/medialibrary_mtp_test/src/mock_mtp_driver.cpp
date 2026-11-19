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

#include "mtp_driver.h"
#include "media_log.h"
#include "mtp_packet_tools.h"
#include "mtp_test.h"

namespace OHOS {
namespace Media {
static std::vector<std::vector<uint8_t>> testDatas = {
    // MTP_OPERATION_OPEN_SESSION_CODE
    { 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },
};
constexpr int32_t MTP_SUCCESS = 0;
MtpDriver::MtpDriver()
{
}

MtpDriver::~MtpDriver()
{
}

int MtpDriver::OpenDriver()
{
    return 0;
}

int MtpDriver::CloseDriver()
{
    return 0;
}

int MtpDriver::Read(std::vector<uint8_t> &outBuffer, uint32_t &outReadSize)
{
    usbOpenFlag = false;
    std::vector<uint8_t>().swap(outBuffer);
    outBuffer.insert(outBuffer.end(), MtpTest::GetInstance()->testData_.begin(),
        MtpTest::GetInstance()->testData_.end());
    outReadSize = outBuffer.size();
    MEDIA_INFO_LOG("MtpDriver::Read");
    MtpPacketTool::Dump(outBuffer);
    return MTP_SUCCESS;
}

void MtpDriver::Write(std::vector<uint8_t> &buffer, uint32_t &bufferSize, int32_t &result)
{
    MEDIA_INFO_LOG("MtpDriver::Write");
    MtpTest::GetInstance()->setOutBuffer(buffer);
    MtpPacketTool::Dump(buffer);
    result = MTP_SUCCESS;
}

int MtpDriver::ReceiveObj(MtpFileRange &mfr)
{
    (void)mfr;
    // Return code is injected per test case through the MtpTest singleton; 0 by default
    return MtpTest::GetInstance()->receiveObjRet_;
}

int MtpDriver::SendObj(MtpFileRange &mfr)
{
    return 0;
}

int MtpDriver::WriteEvent(EventMtp &me)
{
    return 0;
}
} // namespace Media
} // namespace OHOS