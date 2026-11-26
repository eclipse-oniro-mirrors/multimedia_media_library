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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_TEST_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_TEST_H_
#include <mutex>
#include <vector>
namespace OHOS {
namespace Media {
class MtpTest {
public:
    MtpTest();
    ~MtpTest() = default;
    static std::shared_ptr<MtpTest> GetInstance();
    void setOutBuffer(std::vector<uint8_t> testData);
    void getOutBuffer(std::vector<uint8_t> &outBuffer);
    
    std::vector<uint8_t> testData_;
	// Injection point for MtpDriver::ReceiveObj return code in mock_mtp_driver.cpp; 0 means success
    int32_t receiveObjRet_ = 0;
    
private:
    static std::shared_ptr<MtpTest> mtpTestInstance_;
    static std::mutex instanceLock_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_SERVICE_H_