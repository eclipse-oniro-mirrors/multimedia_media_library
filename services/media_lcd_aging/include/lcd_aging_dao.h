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

#ifndef OHOS_MEDIA_LCD_AGING_DAO_H
#define OHOS_MEDIA_LCD_AGING_DAO_H

#include <string>
#include <vector>

#include "lcd_aging_file_info.h"
#include "photos_po.h"
#include "result_set.h"

namespace OHOS::Media {
using namespace OHOS::Media::ORM;

class LcdAgingDao {
public:
    int32_t GetCurrentNumberOfLcd(int64_t &lcdNumber);
    int32_t QueryAgingLcdDataTrashed(const int32_t size, const std::vector<std::string> &notAgingFileIds,
        std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList);
    int32_t QueryAgingLcdDataNotTrashed(const int32_t size, const std::vector<std::string> &notAgingFileIds,
        std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList);
    int32_t SetLcdNotDownloadStatus(const std::vector<std::string> &fileIds);
    int32_t RevertToLcdDownloadStatus(const std::vector<std::string> &fileIds);
    int32_t UpdateLcdFileSize(const std::vector<LcdAgingFileInfo> &agingFileInfos);
    int32_t QueryAgingLcdDataByFileIds(const std::vector<int64_t> &fileIds,
        std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList);
    int64_t GetAgingLcdCount();

private:
    int32_t QueryAgingLcdDataInternal(const int32_t size, const std::vector<std::string> &notAgingFileIds,
        const std::string &sql, const char *logPrefix, std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList);
    void ReadLcdAgingInfoFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        LcdAgingFileInfo &lcdAgingInfo);
    void FillLcdAgingInfoListFromResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::vector<LcdAgingFileInfo> &lcdAgingFileInfoList);
    bool CheckLocalLcd(LcdAgingFileInfo &agingFileInfo);
    int32_t RegenerateAstcWithLocal(const LcdAgingFileInfo &agingFileInfo);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LCD_AGING_DAO_H