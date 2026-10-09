/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_FIX_LCD_FILE_SIZE_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_FIX_LCD_FILE_SIZE_TASK_H

#include <vector>
#include <string>
#include <filesystem>

#include "i_media_background_task.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::Background {

struct LcdFileSizeInfo {
    int32_t id;
    std::string path;
};

class MediaFixLcdFileSizeTask : public IMediaBackGroundTask {
public:
    virtual ~MediaFixLcdFileSizeTask() = default;

    bool Accept() override;
    void Execute() override;

private:
    static constexpr int32_t BATCH_SIZE = 500;
    std::shared_ptr<NativeRdb::ResultSet> QueryInvalidLcdSizeFiles(int32_t limit, int32_t &lastProcessId);
    bool ParseFilesList(std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        std::vector<LcdFileSizeInfo>& filesList);
    bool ProcessLcdFileSize(const std::vector<LcdFileSizeInfo> &filesList);
    void FixLcdFileSize();
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_FIX_LCD_FILE_SIZE_TASK_H