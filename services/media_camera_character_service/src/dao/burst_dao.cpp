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

#define MLOG_TAG "BurstDao"

#include "burst_dao.h"

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "userfile_manager_types.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "cloud_media_common.h"

namespace OHOS::Media {
/**
 * SQL语句：
 * SELECT p.file_id
 * FROM photos p
 * INNER JOIN (
 *     SELECT DISTINCT p1.photo_burst_key, p1.owner_album_id
 *     FROM photos p1
 *     WHERE p1.file_id IN (输入的fileIds)
 *     AND p1.photo_burst_cover_level = 1
 *     AND p1.photo_subtype = 4
 * ) cover_info
 * ON p.photo_burst_key = cover_info.photo_burst_key
 * AND p.owner_album_id = cover_info.owner_album_id
 * WHERE p.photo_burst_cover_level = 2
 */
void BurstDao::CompleteBurstFileIds(std::vector<std::string> &fileIds, std::vector<std::string> &uris)
{
    CHECK_AND_RETURN_LOG(!fileIds.empty(), "CompleteBurstFileIds fileIds is empty");
    
    std::string inClause = CloudMediaCommon::ToStringWithComma(fileIds);
    MEDIA_DEBUG_LOG("input fileIds: %{public}s.", inClause.c_str());
    
    std::string sql = "SELECT p." + MediaColumn::MEDIA_ID + ", p." + MediaColumn::MEDIA_FILE_PATH
        + ", p." + MediaColumn::MEDIA_TYPE + ", p." + MediaColumn::MEDIA_NAME
        + " FROM " + PhotoColumn::PHOTOS_TABLE + " p "
        + " INNER JOIN ("
        + " SELECT DISTINCT p1." + PhotoColumn::PHOTO_BURST_KEY
        + ", p1." + PhotoColumn::PHOTO_OWNER_ALBUM_ID
        + " FROM " + PhotoColumn::PHOTOS_TABLE + " p1 "
        + " WHERE p1." + MediaColumn::MEDIA_ID + " IN (" + inClause + ") "
        + " AND p1." + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = "
        + std::to_string(static_cast<int32_t>(BurstCoverLevelType::COVER))
        + " AND p1." + PhotoColumn::PHOTO_SUBTYPE + " = "
        + std::to_string(static_cast<int32_t>(PhotoSubType::BURST))
        + " ) cover_info "
        + " ON p." + PhotoColumn::PHOTO_BURST_KEY + " = cover_info." + PhotoColumn::PHOTO_BURST_KEY
        + " AND p." + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = cover_info." + PhotoColumn::PHOTO_OWNER_ALBUM_ID
        + " WHERE p." + PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = "
        + std::to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "get rdb store fail");
    
    auto resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query selected files!");
    
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId =
            std::get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        fileIds.push_back(std::to_string(fileId));

        std::string displayName =
            std::get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_NAME, resultSet, TYPE_STRING));
        std::string filePath =
            std::get<string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        int32_t mediaType =
            std::get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet, TYPE_INT32));
        auto extrUri = MediaFileUtils::GetUriByExtrConditions(
            CONST_ML_FILE_URI_PREFIX +
            MediaFileUri::GetMediaTypeUri(static_cast<MediaType>(mediaType), MEDIA_API_VERSION_V10) + "/",
            to_string(fileId), MediaFileUtils::GetExtraUri(displayName, filePath));
        uris.push_back(extrUri);
    }
    resultSet->Close();
}
}  // namespace OHOS::Media