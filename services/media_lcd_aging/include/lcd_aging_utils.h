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

#ifndef OHOS_MEDIA_LCD_AGING_UTILS_H
#define OHOS_MEDIA_LCD_AGING_UTILS_H

#include <vector>

#include "cloud_sync_common.h"
#include "lcd_aging_file_info.h"
#include "photos_po.h"

namespace OHOS::Media {
using namespace OHOS::Media::ORM;
using namespace OHOS::FileManagement::CloudSync;

class LcdAgingUtils {
public:
    EXPORT static int64_t GetMaxThresholdOfLcd();
    EXPORT static int64_t GetScaleThresholdOfLcd();
    EXPORT static std::vector<DentryFileInfo> ConvertAgingFileToDentryFile(
        const std::vector<LcdAgingFileInfo> &agingFileInfos);
    EXPORT static bool HasExThumbnail(const LcdAgingFileInfo &agingFileInfo);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LCD_AGING_UTILS_H