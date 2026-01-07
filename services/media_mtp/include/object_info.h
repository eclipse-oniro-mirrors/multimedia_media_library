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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_OBJECT_INFO_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_OBJECT_INFO_H_
#include <stdint.h>
#include <time.h>
#include <string>
namespace OHOS {
namespace Media {
class ObjectInfo {
public:
    uint32_t handle {0};
    uint32_t storageID {0};
    uint16_t format {0};
    uint16_t protectionStatus {0};
    uint32_t compressedSize {0};
    uint32_t size {0};
    uint16_t thumbFormat {0};
    uint32_t thumbCompressedSize {0};
    uint32_t thumbPixelWidth {0};
    uint32_t thumbPixelHeight {0};
    uint32_t imagePixelWidth {0};
    uint32_t imagePixelHeight {0};
    uint32_t imagePixelDepth {0};
    uint32_t parent {0};
    uint16_t associationType {0};
    uint32_t associationDesc {0};
    uint32_t sequenceNumber {0};
    std::string name;
    time_t dateCreated {0};
    time_t dateModified {0};
    std::string keywords;

    explicit ObjectInfo(uint32_t h);
    virtual ~ObjectInfo();

    void Dump();
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_OBJECT_INFO_H_
