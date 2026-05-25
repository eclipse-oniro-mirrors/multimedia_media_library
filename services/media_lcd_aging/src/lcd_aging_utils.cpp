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
#define MLOG_TAG "Lcd_Aging"

#include "lcd_aging_utils.h"

#include <cinttypes>
#include <map>

#include "exif_rotate_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
constexpr double SCALE_FACTOR = 0.8;
constexpr int64_t MAX_LCD_NUMBER = 50000;
constexpr int64_t SCALE_LCD_NUMBER = static_cast<int64_t>(MAX_LCD_NUMBER * SCALE_FACTOR);

const std::string DENTRY_INFO_LCD_EX = "THM_EX/LCD";
const std::string DENTRY_INFO_LCD = "LCD";
const std::string FILE_NAME_LCD = "LCD.jpg";
const int64_t THUMB_DENTRY_SIZE = 2 * 1024 * 1024;

int64_t LcdAgingUtils::GetMaxThresholdOfLcd()
{
    return MAX_LCD_NUMBER;
}

int64_t LcdAgingUtils::GetScaleThresholdOfLcd()
{
    return SCALE_LCD_NUMBER;
}

std::vector<DentryFileInfo> LcdAgingUtils::ConvertAgingFileToDentryFile(
    const std::vector<LcdAgingFileInfo> &agingFileInfos)
{
    std::vector<DentryFileInfo> dentryFileInfos;
    for (auto &agingFileInfo : agingFileInfos) {
        DentryFileInfo dentryFileInfo;
        dentryFileInfo.cloudId = agingFileInfo.cloudId;
        dentryFileInfo.modifiedTime = agingFileInfo.dateModified;
        dentryFileInfo.path = agingFileInfo.path;
        dentryFileInfo.size = THUMB_DENTRY_SIZE;
        dentryFileInfo.fileType = agingFileInfo.hasExThumbnail ? DENTRY_INFO_LCD_EX : DENTRY_INFO_LCD;

        dentryFileInfo.fileName = FILE_NAME_LCD;
        dentryFileInfos.emplace_back(dentryFileInfo);
    }
    return dentryFileInfos;
}

bool LcdAgingUtils::HasExThumbnail(const LcdAgingFileInfo &agingFileInfo)
{
    return (agingFileInfo.mediaType == MediaType::MEDIA_TYPE_IMAGE) &&
        (agingFileInfo.orientation != 0 || agingFileInfo.exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT));
}
}  // namespace OHOS::Media