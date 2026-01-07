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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_STORAGE_IDS_DATA_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_STORAGE_IDS_DATA_H_

#include <vector>
#include "storage.h"
#include "payload_data.h"

namespace OHOS {
namespace Media {
class GetStorageIdsData : public PayloadData {
public:
    explicit GetStorageIdsData(std::shared_ptr<MtpOperationContext> &context);
    GetStorageIdsData();
    ~GetStorageIdsData() override;

    int Parser(const std::vector<uint8_t> &buffer, int32_t readSize) override;
    int Maker(std::vector<uint8_t> &outBuffer) override;

    uint32_t CalculateSize() override;
    void SetStorages(const std::vector<std::shared_ptr<Storage>> &storages);
private:
    std::vector<std::shared_ptr<Storage>> storages_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_GET_STORAGE_IDS_DATA_H_
