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

#include "media_lcd_size_task.h"

#include "preferences.h"
#include "preferences_helper.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "media_string_utils.h"
#include "photos_po.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::ORM;

namespace OHOS::Media::Background {
constexpr int32_t BATCH_SIZE = 200;
constexpr int32_t MAX_MARK_COUNT = 1000;
constexpr int32_t CURSOR_COMPLETED = -1;
constexpr int32_t CURSOR_INITIAL = 0;
constexpr int32_t DIRTY_MDIRTY = 2;
constexpr char LCD_SIZE_SEPARATOR = ':';
const std::string LCD_SIZE_CURSOR_KEY = "lcd_size_cursor_key";
const std::string LCD_AGING_XML = "/data/storage/el2/base/preferences/lcd_aging.xml";

const std::string SQL_QUERY_DOWNLOAD_FILE ="\
    SELECT file_id, height, width, lcd_size \
    FROM Photos \
    WHERE clean_flag = 0 \
      AND position IN (2, 3) \
      AND file_id > ? \
      ORDER BY file_id ASC LIMIT ?;";

bool MediaLcdSizeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaLcdSizeTask::Execute()
{
    MEDIA_DEBUG_LOG("begin to HandleLcdSize");
    HandleLcdSize();
}

void MediaLcdSizeTask::SetCursorStatus(int32_t cursor)
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(LCD_AGING_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr, errCode: %{public}d", errCode);
    prefs->PutInt(std::string(LCD_SIZE_CURSOR_KEY), cursor);
    prefs->FlushSync();
    MEDIA_INFO_LOG("cursor set to: %{public}d", cursor);
}

int32_t MediaLcdSizeTask::GetCursorStatus()
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(LCD_AGING_XML, errCode);
    MEDIA_INFO_LOG("lcd_size_events prefs errCode: %{public}d", errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, CURSOR_COMPLETED, "prefs is nullptr, errCode: %{public}d", errCode);
    int32_t defaultVal = CURSOR_INITIAL;
    int32_t cursor = prefs->GetInt(std::string(LCD_SIZE_CURSOR_KEY), defaultVal);
    MEDIA_INFO_LOG("current cursor is %{public}d", cursor);
    return cursor;
}

int32_t MediaLcdSizeTask::QueryLcdAssets(const int32_t startFileId, std::vector<LcdAssetInfo> &lcdAssetInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");

    std::vector<NativeRdb::ValueObject> args = { startFileId, BATCH_SIZE };
    shared_ptr<ResultSet> resultSet = rdbStore->QuerySql(SQL_QUERY_DOWNLOAD_FILE, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Query lcd assets fails");

    vector<PhotosPo> photosPos;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "ReadRecords failed, ret: %{public}d", ret);

    for (const auto &photoPo : photosPos) {
        LcdAssetInfo assetInfo;
        assetInfo.fileId = photoPo.fileId.value_or(0);
        assetInfo.photoHeight = photoPo.height.value_or(0);
        assetInfo.photoWidth = photoPo.width.value_or(0);
        assetInfo.lcdSize = photoPo.lcdSize.value_or("");
        lcdAssetInfos.emplace_back(assetInfo);
    }
    return E_OK;
}

bool MediaLcdSizeTask::ParseLcdSize(const std::string &lcdSize, int32_t &lcdWidth, int32_t &lcdHeight)
{
    if (lcdSize.empty()) {
        MEDIA_DEBUG_LOG("lcdSize is empty");
        return false;
    }

    size_t separatorPos = lcdSize.find(LCD_SIZE_SEPARATOR);
    if (separatorPos == string::npos) {
        MEDIA_ERR_LOG("failed to parse lcdSize: %{public}s", lcdSize.c_str());
        return false;
    }

    string widthStr = lcdSize.substr(0, separatorPos);
    string heightStr = lcdSize.substr(separatorPos + 1);

    if (!MediaStringUtils::ConvertToInt(widthStr, lcdWidth)) {
        MEDIA_ERR_LOG("failed to parse lcdSize: %{public}s", lcdSize.c_str());
        return false;
    }

    if (!MediaStringUtils::ConvertToInt(heightStr, lcdHeight)) {
        MEDIA_ERR_LOG("failed to parse lcdSize: %{public}s", lcdSize.c_str());
        return false;
    }

    return true;
}

bool MediaLcdSizeTask::IsSpecialAsset(LcdAssetInfo &assetInfo)
{
    bool isValid = ParseLcdSize(assetInfo.lcdSize, assetInfo.lcdWidth, assetInfo.lcdHeight);
    CHECK_AND_RETURN_RET(isValid, false);

    isValid = !(assetInfo.photoWidth <= 0 || assetInfo.photoHeight <= 0 ||
        assetInfo.lcdWidth <= 0 || assetInfo.lcdHeight <= 0);
    CHECK_AND_RETURN_RET(isValid, false);

    bool condition1 = (assetInfo.photoWidth > assetInfo.photoHeight) && (assetInfo.lcdHeight > assetInfo.lcdWidth);
    bool condition2 = (assetInfo.photoHeight > assetInfo.photoWidth) && (assetInfo.lcdWidth > assetInfo.lcdHeight);

    return condition1 || condition2;
}

int32_t MediaLcdSizeTask::UpdateDirtyStatus(const std::vector<std::string> &fileIds)
{
    if (fileIds.empty()) {
        return E_ERR;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, fileIds);

    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));

    int32_t updatedRows = 0;
    int32_t ret = rdbStore->Update(updatedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && updatedRows > 0, E_ERR, "Update dirty status failed, ret: %{public}d", ret);
    MEDIA_INFO_LOG("Update dirty status success, updatedRows: %{public}d", updatedRows);
    return updatedRows;
}

void MediaLcdSizeTask::HandleLcdSize()
{
    int32_t cursor = GetCursorStatus();
    CHECK_AND_RETURN_INFO_LOG(cursor > CURSOR_COMPLETED, "no need handel lcdSize, cursor: %{public}d", cursor);

    int32_t startFileId = cursor;
    int32_t markedCount = 0;
    int32_t ret = E_ERR;
    while (markedCount < MAX_MARK_COUNT && cursor != CURSOR_COMPLETED) {
        if (!this->Accept()) {
            MEDIA_DEBUG_LOG("Lcd size task check condition failed, save cursor and exit");
            return;
        }

        std::vector<LcdAssetInfo> lcdAssetInfos;
        ret = QueryLcdAssets(startFileId, lcdAssetInfos);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "failed to QueryLcdAssets, ret: %{public}d", ret);
        if (lcdAssetInfos.empty()) {
            MEDIA_INFO_LOG("No more lcd assets to process, mark task as completed");
            SetCursorStatus(CURSOR_COMPLETED);
            cursor = CURSOR_COMPLETED;
            break;
        }
        startFileId = lcdAssetInfos.back().fileId;
        SetCursorStatus(startFileId);

        std::vector<std::string> needUpdateFileIds;
        for (auto &assetInfo : lcdAssetInfos) {
            CHECK_AND_CONTINUE(IsSpecialAsset(assetInfo));
            needUpdateFileIds.emplace_back(std::to_string(assetInfo.fileId));
            MEDIA_INFO_LOG("Special asset found, fileId: %{public}d, photoWidth: %{public}d, photoHeight: %{public}d, "
                "lcdWidth: %{public}d, lcdHeight: %{public}d",
                assetInfo.fileId, assetInfo.photoWidth, assetInfo.photoHeight, assetInfo.lcdWidth, assetInfo.lcdHeight);
        }

        if (!needUpdateFileIds.empty()) {
            CHECK_AND_CONTINUE_ERR_LOG(UpdateDirtyStatus(needUpdateFileIds),
                "Update dirty status failed");
            markedCount += static_cast<int32_t>(needUpdateFileIds.size());
            MEDIA_INFO_LOG("Marked %{public}d assets as mdirty in this batch, total marked: %{public}d",
                static_cast<int32_t>(needUpdateFileIds.size()), markedCount);
        }
    }
    MEDIA_INFO_LOG("MediaLcdSizeTask End, total marked: %{public}d, cursor: %{public}d", markedCount, cursor);
}
} // namespace OHOS::Media::Background
