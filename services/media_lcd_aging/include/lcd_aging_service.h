/*
* Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_LCD_AGING_SERVICE_H
#define OHOS_LCD_AGING_SERVICE_H

#include <stdint.h>
#include <string>
#include <memory>

#include "media_lake_clone_event_manager.h"
#include "medialibrary_restore.h"
#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class LcdAgingService {
public:
    EXPORT static LcdAgingService &GetInstance();

    int32_t HandleCanPerformDeepOptimizeSpace(bool &result);
    int32_t HandleGetDeepOptimizableSpace(int64_t &space);

public:
    void SetMarkingLcdStatus(bool status);
    bool IsMarkingLcdStatus();
    EXPORT int32_t MarkRecentLcdPhotos(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);

private:
    LcdAgingService() = default;
    ~LcdAgingService() = default;
    LcdAgingService(const LcdAgingService &) = delete;
    LcdAgingService &operator=(const LcdAgingService &) = delete;

    int64_t GetLcdImageCount();
    bool IsCloningOrRestoring();
    bool IsCleaningLcd();
    bool HasReleasableLcdImages();

    static const std::string SQL_GET_TOTAL_NUMBER_OF_LCD;
    static const std::string SQL_GET_CAN_OPTIMIZE_OF_LCD;
    static const std::string SQL_MARK_RECENT_LCD_PHOTOS;
    static std::atomic<bool> isMarkingLcdStatus_;
};
}  // namespace OHOS::Media
#endif  // OHOS_LCD_AGING_SERVICE_H