/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "PhotoMimetypeOperation"

#include "photo_mimetype_operation.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "values_bucket.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const int32_t BATCH_QUERY_NUMBER = 1000;
const std::string PREFIX_IMAGE = "image/";
const std::string COLUMN_COUNT = "count(*)";

static int32_t GetCountOfAllAssets(int32_t &assetsCount)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetCountOfAllAssets failed. rdbStore is null.");
    std::vector<std::string> columns = { COLUMN_COUNT };
    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.NotLike(MediaColumn::MEDIA_MIME_TYPE, "image/%");
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetCountOfAllAssets failed. rdbStore is null.");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetsCount = GetInt32Val(COLUMN_COUNT, resultSet);
        return E_OK;
    }
    return E_ERR;
}

int32_t PhotoMimetypeOperation::UpdateInvalidMimeType()
{
    MEDIA_INFO_LOG("enter UpdateInvalidMimeType.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "HasDataForUpdate failed. rdbStore is null.");
    int32_t assetsCount = 0;
    CHECK_AND_RETURN_RET_LOG(GetCountOfAllAssets(assetsCount) == E_OK, E_ERR, "Failed to GetCountOfAllAssets.");
    CHECK_AND_RETURN_RET_INFO_LOG(assetsCount > 0, E_OK, "no Invalid mimeType.");
    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.NotLike(MediaColumn::MEDIA_MIME_TYPE, "image/%");
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME };

    int32_t startCount = 0;
    while (MedialibrarySubscriber::IsCurrentStatusOn() && startCount < assetsCount) {
        predicates.Limit(BATCH_QUERY_NUMBER);
        auto resultSet = rdbStore->Query(predicates, columns);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "UpdateMimeType failed. resultSet is null.");

        std::unordered_map<std::string, std::vector<std::string>> invalidMimeTypeMap;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
            std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            std::string mimeType = MediaFileUtils::GetMimeTypeFromDisplayName(displayName);
            if (fileId.empty() || mimeType.empty() || !MediaFileUtils::StartsWith(mimeType, PREFIX_IMAGE)) {
                MEDIA_WARN_LOG("Invalid, fileId: %{public}s, mimeType: %{public}s.", fileId.c_str(), mimeType.c_str());
                continue;
            }
            invalidMimeTypeMap[mimeType].emplace_back(fileId);
        }
        resultSet->Close();
        if (!invalidMimeTypeMap.empty()) {
            CHECK_AND_RETURN_RET_LOG(HandleUpdateInvalidMimeType(rdbStore, invalidMimeTypeMap) == E_OK, E_ERR,
                "Failed to HandleUpdateInvalidMimeType.");
        }
        startCount += BATCH_QUERY_NUMBER;
    }
    MEDIA_INFO_LOG("end UpdateInvalidMimeType.");
    return E_OK;
}

int32_t PhotoMimetypeOperation::HandleUpdateInvalidMimeType(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::unordered_map<std::string, std::vector<std::string>> &invalidMimeTypeMap)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr.");
    MEDIA_INFO_LOG("start to update invalid mimeType.");
    for (const auto &invalidPair : invalidMimeTypeMap) {
        ValuesBucket values;
        values.Put(MediaColumn::MEDIA_MIME_TYPE, invalidPair.first);
        AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(MediaColumn::MEDIA_ID, invalidPair.second);
        int32_t totalCount = static_cast<int32_t>(invalidPair.second.size());
        int32_t changedRows = -1;
        int32_t ret = rdbStore->Update(changedRows, values, predicates);
        if (ret != E_OK || changedRows != totalCount) {
            MEDIA_ERR_LOG("update failed, mimeType: %{public}s, totalCount: %{public}d, changedRows: %{public}d.",
                invalidPair.first.c_str(), totalCount, changedRows);
            continue;
        }
        MEDIA_INFO_LOG("update successfully, mimeType: %{public}s, totalCount: %{public}d, changedRows: %{public}d.",
            invalidPair.first.c_str(), totalCount, changedRows);
    }
    MEDIA_INFO_LOG("end to update invalid mimeType.");
    return E_OK;
}
}  // namespace OHOS::Media