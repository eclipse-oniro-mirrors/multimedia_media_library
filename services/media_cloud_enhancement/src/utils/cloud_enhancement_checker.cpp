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

#define MLOG_TAG "CloudEnhancementChecker"

#include "cloud_enhancement_checker.h"

#include "media_column.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "enhancement_database_operations.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_rdb_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
static const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";
static const std::string PERMISSION_ADDED_FILE_ID = "permission_added_file_id";

static void GetMaxFileId(int32_t& maxFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "get rdb store failed");
    string queryMaxSql = "SELECT Max(file_id) FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = rdbStore->QuerySql(queryMaxSql);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_LOG(!cond, "Failed to get max file_id!");
    resultSet->GetInt(0, maxFileId);
    resultSet->Close();
    return;
}

void CloudEnhancementChecker::AddPermissionForCloudEnhancement()
{
    MEDIA_INFO_LOG("start to add permission for cloud enhancement photo!");
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t curFileId = prefs->GetInt(PERMISSION_ADDED_FILE_ID, 0);
    MEDIA_INFO_LOG("start file id: %{public}d", curFileId);
    vector<string> columns = { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ASSOCIATE_FILE_ID };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, curFileId);
    predicates.And();
    predicates.EqualTo(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        to_string(static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT)));
    auto resultSet = MediaLibraryRdbStore::StepQueryWithoutCheck(predicates, columns);
    errCode = (resultSet == nullptr) ? E_ERR : E_OK;
    CHECK_AND_EXECUTE(resultSet == nullptr, errCode = (resultSet->GoToFirstRow() != E_OK) ? E_ERR : E_OK);
    if (errCode == E_ERR) {
        int32_t maxFileId = 0;
        GetMaxFileId(maxFileId);
        prefs->PutInt(PERMISSION_ADDED_FILE_ID, maxFileId);
        prefs->FlushSync();
        MEDIA_INFO_LOG("no cloud enhancement photo need to add permission, current max id:%{public}d",
            maxFileId);
        return;
    }

    do {
        int32_t sourceId = GetInt32Val(PhotoColumn::PHOTO_ASSOCIATE_FILE_ID, resultSet);
        int32_t targetId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        EnhancementDatabaseOperations::InsertCloudEnhancementPerm(sourceId, targetId);
        prefs->PutInt(PERMISSION_ADDED_FILE_ID, targetId);
        prefs->FlushSync();
    } while (resultSet->GoToNextRow() == E_OK);
    MEDIA_INFO_LOG("end add permission for cloud enhancement photo!");
}

static int BuildFileIdPredicatesForBatch(int currentFileId, int maxFileId, RdbPredicates& predicates)
{
    const int batchSize = 1000;
    int32_t startId = currentFileId;
    int32_t endId = std::min(startId + batchSize, maxFileId);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, startId);
    predicates.LessThanOrEqualTo(MediaColumn::MEDIA_ID, endId);
    return endId;
}

static int QueryMaxFileId()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "get rdb store failed");
    string queryMaxSql = "SELECT Max(file_id) FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = rdbStore->QuerySql(queryMaxSql);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), E_ERR, "Query max file_id failed");
    int32_t maxFileId = -1;
    maxFileId = GetInt32Val("Max(file_id)", resultSet);
    return maxFileId;
}

void CloudEnhancementChecker::RecognizeCloudEnhancementPhotosByDisplayName()
{
    MEDIA_INFO_LOG("start to recognize cloud enhancement photos by display name");
    int32_t errCode = E_OK;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "get rdb store failed");
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    const string fileIdProgressKeyName = "recognize_ce_photos_file_id_progress";
    int32_t currentFileId = prefs->GetInt(fileIdProgressKeyName, 0);
    int maxFileId = QueryMaxFileId();
    bool needRefreshAlbum = false;
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");
    MEDIA_INFO_LOG("start from file id: %{public}d, max file id is %{public}d", currentFileId, maxFileId);

    while (currentFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn()) {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        int batchEndId = BuildFileIdPredicatesForBatch(currentFileId, maxFileId, predicates);
        predicates.EqualTo(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
            static_cast<int32_t>(StrongAssociationType::NORMAL));
        const string enhancedSuffix = "%_enhanced";
        predicates.Like(MediaColumn::MEDIA_TITLE, enhancedSuffix);
        predicates.EqualTo(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
        ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
            static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT));
        values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::FINISH));
        int updatedRow = -1;
        int ret = rdbStore->Update(updatedRow, values, predicates);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("update cloud enhancement attribute failed, errCode: %{public}d", ret);
            break;
        }
        currentFileId = batchEndId;
        prefs->PutInt(fileIdProgressKeyName, currentFileId);
        prefs->FlushSync();
        if (updatedRow > 0) {
            needRefreshAlbum = true;
        }
    }

    if (needRefreshAlbum) {
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore,
            {to_string(static_cast<int32_t>(PhotoAlbumSubType::CLOUD_ENHANCEMENT))});
        MediaLibraryRdbUtils::UpdateSysAlbumHiddenState(rdbStore,
            {to_string(static_cast<int32_t>(PhotoAlbumSubType::CLOUD_ENHANCEMENT))});
    }
    MEDIA_INFO_LOG("end recognize cloud enhancement photos by display name. File id progress: %{public}d",
        currentFileId);
}

}  // namespace Media
}  // namespace OHOS
