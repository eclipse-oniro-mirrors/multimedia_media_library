/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_REPAIR_FUTURE_DATE_TASK_H
#define OHOS_MEDIA_BACKGROUND_REPAIR_FUTURE_DATE_TASK_H

#include <vector>
#include <mutex>

#include "cloud_media_scan_service.h"
#include "i_media_background_task.h"
#include "photos_po.h"

namespace OHOS::Media::Background {
using namespace OHOS::Media::ORM;
using namespace OHOS::Media::CloudSync;

class RepairFutureDateTask : public IMediaBackGroundTask {
public:
    virtual ~RepairFutureDateTask() = default;
    bool Accept() override;
    void Execute() override;

private:
    int32_t GetRepairDateData(const int32_t lastRecord, std::vector<PhotosPo> &photos);
    void UpdateFutureDate(
        const CloudMediaScanService::ScanResult &scanResult, const int32_t fileId, const int32_t position);
    void RepairPhotoDate(int32_t &currentRecord, bool &terminate, const std::vector<PhotosPo> &photos);

private:
    std::mutex repairDateMutex_;
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_REPAIR_FUTURE_DATE_TASK_H