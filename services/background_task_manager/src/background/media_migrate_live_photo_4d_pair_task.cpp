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

#include "media_migrate_live_photo_4d_pair_task.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {

bool MediaMigrateLivePhoto4dPairTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaMigrateLivePhoto4dPairTask::Execute()
{
    HandleMigrateLivePhoto4dPair();
}

void MediaMigrateLivePhoto4dPairTask::HandleMigrateLivePhoto4dPair()
{
    MEDIA_INFO_LOG("MigrateLivePhoto4dPair start");
    int32_t iteration = 0;
    while (iteration < MIGRATE_PAIR_MAX_ITERATION) {
        iteration++;

        if (!Accept()) {
            MEDIA_INFO_LOG("MigrateLivePhoto4dPair Accept check failed, return");
            return;
        }

        auto resultSet = QueryParentAssets();
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "MigrateLivePhoto4dPair query failed");

        std::vector<ParentPairData> dataList;
        bool ret = ParseParentData(resultSet, dataList);
        CHECK_AND_RETURN_LOG(ret, "MigrateLivePhoto4dPair parse data failed");

        if (dataList.empty()) {
            MEDIA_INFO_LOG("MigrateLivePhoto4dPair no more data, done");
            return;
        }

        std::map<std::string, std::string> pairMap;
        std::vector<int32_t> parentFileIds;
        for (const auto &item : dataList) {
            pairMap[item.latestPair] = item.uniqueId;
            parentFileIds.push_back(item.fileId);
        }

        int32_t updateRet = BatchUpdateChildPair(pairMap);
        if (updateRet != E_OK) {
            MEDIA_ERR_LOG("MigrateLivePhoto4dPair BatchUpdateChildPair failed");
        }

        int32_t clearRet = BatchClearParentPair(parentFileIds);
        if (clearRet != E_OK) {
            MEDIA_ERR_LOG("MigrateLivePhoto4dPair BatchClearParentPair failed");
        }

        MEDIA_INFO_LOG("MigrateLivePhoto4dPair processed batch, size: %{public}zu, iteration: %{public}d",
            dataList.size(), iteration);
    }
    MEDIA_INFO_LOG("MigrateLivePhoto4dPair reach max iteration");
}

std::shared_ptr<NativeRdb::ResultSet> MediaMigrateLivePhoto4dPairTask::QueryParentAssets()
{
    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, PhotoColumn::UNIQUE_ID,
        PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR};

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.IsNotNull(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR)
        ->And()->NotEqualTo(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, "")
        ->And()->GreaterThanOrEqualTo(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
            static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED))
        ->And()->LessThan(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
            static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D))
        ->OrderByDesc(PhotoColumn::MEDIA_ID)
        ->Limit(MIGRATE_PAIR_BATCH_NUM);

    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

bool MediaMigrateLivePhoto4dPairTask::ParseParentData(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::vector<ParentPairData> &dataList)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "MigrateLivePhoto4dPair resultSet is nullptr");

    int rowCount = 0;
    int32_t err = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(err == E_OK && rowCount >= 0, false,
        "MigrateLivePhoto4dPair GetRowCount failed, err:%{public}d", err);
    CHECK_AND_RETURN_RET_LOG(rowCount > 0, true, "MigrateLivePhoto4dPair no data");

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "MigrateLivePhoto4dPair GoToFirstRow failed %{public}d", err);

    do {
        ParentPairData data;

        data.fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
        data.uniqueId = GetStringVal(PhotoColumn::UNIQUE_ID, resultSet);
        data.latestPair = GetStringVal(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR, resultSet);

        if (data.latestPair.empty()) {
            MEDIA_WARN_LOG("MigrateLivePhoto4dPair skip invalid data, latestPair empty, fileId: %{public}d",
                data.fileId);
            continue;
        }

        if (data.uniqueId.empty() || data.uniqueId == "-1") {
            std::string newUniqueId = GenerateUniqueIdForAsset(data.fileId);
            if (newUniqueId.empty()) {
                MEDIA_WARN_LOG("MigrateLivePhoto4dPair generate uniqueId failed, skip fileId: %{public}d",
                    data.fileId);
                continue;
            }
            data.uniqueId = newUniqueId;
            MEDIA_INFO_LOG("MigrateLivePhoto4dPair generated uniqueId for fileId: %{public}d", data.fileId);
        }

        dataList.push_back(data);
    } while (resultSet->GoToNextRow() == E_OK);

    return true;
}

int32_t MediaMigrateLivePhoto4dPairTask::BatchUpdateChildPair(
    const std::map<std::string, std::string> &pairMap)
{
    if (pairMap.empty()) {
        return E_OK;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "MigrateLivePhoto4dPair rdbStore is null");

    std::string caseClauses;
    std::string inClauses;
    bool first = true;
    for (const auto &[childUid, parentUid] : pairMap) {
        if (!first) {
            caseClauses += " ";
            inClauses += ",";
        }
        caseClauses += "WHEN '" + childUid + "' THEN '" + parentUid + "'";
        inClauses += "'" + childUid + "'";
        first = false;
    }

    std::string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR + " = CASE " +
        PhotoColumn::UNIQUE_ID + " " + caseClauses + " END WHERE " +
        PhotoColumn::UNIQUE_ID + " IN (" + inClauses + ") AND " +
        PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS + " >= " +
        to_string(static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D));

    // DEBUG
    MEDIA_INFO_LOG("MigrateLivePhoto4dPair BatchUpdateChildPair sql: %{public}s", sql.c_str());
    // DEBUG

    int32_t ret = rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
        "MigrateLivePhoto4dPair BatchUpdateChildPair ExecuteSql failed, ret: %{public}d", ret);

    MEDIA_INFO_LOG("MigrateLivePhoto4dPair BatchUpdateChildPair success, count: %{public}zu", pairMap.size());
    return E_OK;
}

int32_t MediaMigrateLivePhoto4dPairTask::BatchClearParentPair(const std::vector<int32_t> &parentFileIds)
{
    if (parentFileIds.empty()) {
        return E_OK;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "MigrateLivePhoto4dPair rdbStore is null");

    ValuesBucket values;
    values.PutNull(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_LATEST_PAIR);

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> parentIdStrs;
    for (auto id : parentFileIds) {
        parentIdStrs.push_back(std::to_string(id));
    }
    predicates.In(PhotoColumn::MEDIA_ID, parentIdStrs);

    // DEBUG
    std::string fileIdsStr;
    for (size_t i = 0; i < parentFileIds.size(); i++) {
        if (i > 0) fileIdsStr += ",";
        fileIdsStr += to_string(parentFileIds[i]);
    }
    MEDIA_INFO_LOG("MigrateLivePhoto4dPair BatchClearParentPair fileIds: [%{public}s]", fileIdsStr.c_str());
    // DEBUG

    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret,
        "MigrateLivePhoto4dPair BatchClearParentPair failed, ret: %{public}d", ret);

    MEDIA_INFO_LOG("MigrateLivePhoto4dPair BatchClearParentPair success, changedRows: %{public}d", changedRows);
    return E_OK;
}

std::string MediaMigrateLivePhoto4dPairTask::GenerateUniqueIdForAsset(int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("MigrateLivePhoto4dPair GenerateUniqueId rdbStore is null");
        return "";
    }

    std::string newUniqueId = MediaFileUtils::GenerateUUID();

    ValuesBucket values;
    values.PutString(PhotoColumn::UNIQUE_ID, newUniqueId);

    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));
    predicates.And()->IsNull(PhotoColumn::UNIQUE_ID)
        ->Or()->EqualTo(PhotoColumn::UNIQUE_ID, "")
        ->Or()->EqualTo(PhotoColumn::UNIQUE_ID, "-1");

    int32_t changedRows = 0;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, "",
        "MigrateLivePhoto4dPair GenerateUniqueId Update failed, fileId: %{public}d, ret: %{public}d",
        fileId, ret);

    if (changedRows > 0) {
        MEDIA_INFO_LOG("MigrateLivePhoto4dPair GenerateUniqueId success, fileId: %{public}d", fileId);
        return newUniqueId;
    }

    // changedRows == 0: uniqueId may have been set by another process, re-query actual value
    MEDIA_INFO_LOG("MigrateLivePhoto4dPair GenerateUniqueId changedRows=0, re-query fileId: %{public}d", fileId);
    RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, {PhotoColumn::UNIQUE_ID});
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("MigrateLivePhoto4dPair GenerateUniqueId re-query failed, fileId: %{public}d", fileId);
        return "";
    }
    std::string actualUniqueId = GetStringVal(PhotoColumn::UNIQUE_ID, resultSet);
    resultSet = nullptr;
    if (MediaFileUtils::IsValidUuid(actualUniqueId)) {
        MEDIA_INFO_LOG("MigrateLivePhoto4dPair GenerateUniqueId use existing uniqueId, fileId: %{public}d", fileId);
        return actualUniqueId;
    }
    MEDIA_ERR_LOG("MigrateLivePhoto4dPair GenerateUniqueId re-query still invalid, fileId: %{public}d", fileId);
    return "";
}

}  // namespace OHOS::Media::Background
