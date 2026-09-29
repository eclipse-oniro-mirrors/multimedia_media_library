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

#define MLOG_TAG "Media_Background"

#include "media_fix_lcd_file_size_task.h"

#include "rdb_predicates.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "thumbnail_source_loading.h"

using namespace OHOS::NativeRdb;
using namespace std;

namespace OHOS::Media::Background {

bool MediaFixLcdFileSizeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaFixLcdFileSizeTask::Execute()
{
    this->FixLcdFileSize();
    return;
}

std::shared_ptr<NativeRdb::ResultSet> MediaFixLcdFileSizeTask::QueryInvalidLcdSizeFiles(int32_t limit,
    int32_t &lastProcessId)
{
    int32_t lcdFileSize = 0;
    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH};
 
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_LCD_FILE_SIZE, lcdFileSize);
    predicates.OrderByAsc(PhotoColumn::MEDIA_ID);

    predicates.GreaterThan(PhotoColumn::MEDIA_ID, lastProcessId);
    predicates.Limit(limit);
    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

bool MediaFixLcdFileSizeTask::ParseFilesList(std::shared_ptr<NativeRdb::ResultSet>& resultSet,
    std::vector<LcdFileSizeInfo>& filesList)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "FixLcdFileSize resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        std::string filePath =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        filesList.emplace_back(LcdFileSizeInfo{
            .id = fileId,
            .path = filePath
        });
        MEDIA_DEBUG_LOG("FixLcdFileSize handle file id %{public}d", fileId);
    }
    return true;
}

bool MediaFixLcdFileSizeTask::ProcessLcdFileSize(const std::vector<LcdFileSizeInfo> &filesList)
{
    MEDIA_INFO_LOG("FixLcdFileSize process start, num: %{public}zu", filesList.size());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    std::string suffixes[] = {THUMBNAIL_LCD_EX_SUFFIX, THUMBNAIL_LCD_SUFFIX};
    bool hasLcdFile = false;
    
    for (const auto& file : filesList) {
        size_t lcdFileSize = 0;
        for (const auto& suffix : suffixes) {
            std::string lcdCloudPath = GetThumbnailPath(file.path, suffix);
            if (!lcdCloudPath.empty() && MediaFileUtils::GetFileSize(lcdCloudPath, lcdFileSize)) {
                hasLcdFile = true;
                break;
            }
        }
        if (!hasLcdFile) {
            continue;
        }

        AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, file.id);
        ValuesBucket values;
        values.PutLong(PhotoColumn::PHOTO_LCD_FILE_SIZE, static_cast<int64_t>(hasLcdFile ? lcdFileSize : 0));

        int32_t changeRows = -1;
        int32_t ret = rdbStore->Update(changeRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG((ret == E_OK && changeRows > 0), false,
            "failed to update lcd_file_size, ret = %{public}d", ret);
        MEDIA_DEBUG_LOG("FixLcdFileSize process done id: %{public}d, path: %{public}s, size: %{public}zu", file.id,
            DfxUtils::GetSafePath(file.path).c_str(), lcdFileSize);
    }
    return true;
}

void MediaFixLcdFileSizeTask::FixLcdFileSize()
{
    MEDIA_INFO_LOG("FixLcdFileSize Start");
    int32_t lastProcessId = 0;
    while (this->Accept()) {
        auto resultSet = QueryInvalidLcdSizeFiles(BATCH_SIZE, lastProcessId);
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "FixLcdFileSize query failed End");

        std::vector<LcdFileSizeInfo> filesList;
        bool ret = ParseFilesList(resultSet, filesList);
        CHECK_AND_RETURN_LOG(ret, "FixLcdFileSize parse files list failed End");

        if (filesList.empty()) {
            MEDIA_INFO_LOG("FixLcdFileSize End, No more files to update");
            return;
        }

        ret = ProcessLcdFileSize(filesList);
        lastProcessId = filesList.back().id;
        CHECK_AND_RETURN_LOG(ret, "FixLcdFileSize process failed End");
    }
}  // namespace OHOS::Media::Background
}  // namespace OHOS::Media::Background