/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "VisionOperation"

#include <thread>
#include "iservice_registry.h"
#include "media_actively_calling_analyse.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_vision_operations.h"
#include "rdb_utils.h"
#include "vision_aesthetics_score_column.h"
#include "vision_column.h"
#include "vision_total_column.h"
#include "vision_recommendation_column.h"

using namespace std;
using namespace OHOS::NativeRdb;
using Uri = OHOS::Uri;
using namespace OHOS::DataShare;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
static vector<int> NEED_UPDATE_TYPE = {
    PhotoAlbumSubType::CLASSIFY, PhotoAlbumSubType::PORTRAIT
};
int32_t MediaLibraryVisionOperations::InsertOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int64_t outRowId = -1;
    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK || outRowId < 0) {
        MEDIA_ERR_LOG("Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outRowId);
}

int32_t MediaLibraryVisionOperations::UpdateOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t updateRows = -1;
    int32_t errCode = rdbStore->Update(cmd, updateRows);
    if (errCode != NativeRdb::E_OK || updateRows < 0) {
        MEDIA_ERR_LOG("Update db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(updateRows);
}

int32_t MediaLibraryVisionOperations::DeleteOperation(MediaLibraryCommand &cmd)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int32_t deleteRows = -1;
    int32_t errCode = rdbStore->Delete(cmd, deleteRows);
    if (errCode != NativeRdb::E_OK || deleteRows < 0) {
        MEDIA_ERR_LOG("Delete db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(deleteRows);
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryVisionOperations::QueryOperation(MediaLibraryCommand &cmd,
    const std::vector<std::string> &columns)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return nullptr;
    }
    return rdbStore->Query(cmd, columns);
}

static int32_t UpdateAnalysisTotal(string &uriTotal, string &selection, const string &columnName)
{
    Uri uri = Uri(uriTotal);
    DataSharePredicates predicate;
    predicate.SetWhereClause(selection);
    MediaLibraryCommand cmdTotal(uri);
    DataShareValuesBucket valueBucket;
    valueBucket.Put(STATUS, 0);
    valueBucket.Put(columnName, 0);
    return MediaLibraryDataManager::GetInstance()->Update(cmdTotal, valueBucket, predicate);
}

static int32_t DeleteFromVisionTables(string &fileId, string &selectionTotal,
    const string &columnTotal, const string &tableName)
{
    string uriTotal = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_TOTAL;
    int32_t updateRows = UpdateAnalysisTotal(uriTotal, selectionTotal, columnTotal);
    MEDIA_DEBUG_LOG("Update %{public}d rows at total for edit commit to %{public}s", updateRows, columnTotal.c_str());
    if (updateRows <= 0) {
        return updateRows;
    }

    string uriTable = MEDIALIBRARY_DATA_URI + "/" + tableName;
    Uri uri = Uri(uriTable);
    DataSharePredicates predicate;
    predicate.EqualTo(FILE_ID, fileId);
    MediaLibraryCommand cmdTable(uri);
    return MediaLibraryDataManager::GetInstance()->Delete(cmdTable, predicate);
}

static void UpdateVisionTableForEdit(AsyncTaskData *taskData)
{
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("taskData is nullptr");
        return;
    }
    UpdateVisionAsyncTaskData* data = static_cast<UpdateVisionAsyncTaskData*>(taskData);
    if (data == nullptr) {
        MEDIA_ERR_LOG("UpdateVisionAsyncTaskData is nullptr");
        return;
    }
    string fileId = to_string(data->fileId_);

    string selectionTotal = FILE_ID + " = " + fileId + " AND " + LABEL + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, LABEL, PAH_ANA_LABEL);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + AESTHETICS_SCORE + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, AESTHETICS_SCORE, PAH_ANA_ATTS);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + OCR + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, OCR, PAH_ANA_OCR);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + SALIENCY + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, SALIENCY, PAH_ANA_SALIENCY);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + FACE + " IN (-2, 1, 2, 3, 4)";
    DeleteFromVisionTables(fileId, selectionTotal, FACE, PAH_ANA_FACE);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + OBJECT + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, OBJECT, PAH_ANA_OBJECT);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + RECOMMENDATION + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, RECOMMENDATION, PAH_ANA_RECOMMENDATION);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + SEGMENTATION + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, SEGMENTATION, PAH_ANA_SEGMENTATION);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + HEAD + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, HEAD, PAH_ANA_HEAD);

    selectionTotal = FILE_ID + " = " + fileId + " AND " + POSE + " = 1";
    DeleteFromVisionTables(fileId, selectionTotal, POSE, PAH_ANA_POSE);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdbstore");
        return;
    }
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByFile(rdbStore, {fileId}, NEED_UPDATE_TYPE);
}

int32_t MediaLibraryVisionOperations::EditCommitOperation(MediaLibraryCommand &cmd)
{
    if (cmd.GetOprnObject() != OperationObject::FILESYSTEM_PHOTO) {
        return E_SUCCESS;
    }
    const ValuesBucket &values = cmd.GetValueBucket();
    ValueObject valueObject;
    int32_t fileId;
    if (values.GetObject(PhotoColumn::MEDIA_ID, valueObject)) {
        valueObject.GetInt(fileId);
    } else {
        return E_HAS_DB_ERROR;
    }

    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker ==  nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        return E_ERR;
    }
    UpdateVisionAsyncTaskData* taskData = new (std::nothrow) UpdateVisionAsyncTaskData(fileId);
    shared_ptr<MediaLibraryAsyncTask> updateAsyncTask =
        make_shared<MediaLibraryAsyncTask>(UpdateVisionTableForEdit, taskData);
    if (updateAsyncTask != nullptr) {
        asyncWorker->AddTask(updateAsyncTask, true);
    } else {
        MEDIA_ERR_LOG("UpdateAnalysisDataForEdit fail");
    }
    return E_SUCCESS;
}

static void ActivelyStartAnalysisService(const int fileId)
{
    MEDIA_INFO_LOG("fileId is: %{public}d", fileId);
    int32_t code = MediaActivelyCallingAnalyse::ActivateServiceType::START_SERVICE_OCR;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInt32(static_cast<int32_t>(fileId));
    MediaActivelyCallingAnalyse mediaActivelyCallingAnalyse(nullptr);
    if (!mediaActivelyCallingAnalyse.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Actively Calling Analyse Fail");
    }
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryVisionOperations::DealWithActiveOcrTask(
    shared_ptr<NativeRdb::ResultSet> &queryResult, const DataShare::DataSharePredicates &predicates,
    const std::vector<std::string> &columns, MediaLibraryCommand &cmd)
{
    constexpr int32_t FIELD_IDX = 0;
    constexpr int32_t VALUE_IDX = 1;
    int32_t count = 0;
    int32_t ret = queryResult->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("GetRowCount failed ret:%{public}d", ret);
        return queryResult;
    }
    if (count > 0) {
        MEDIA_INFO_LOG("Active OCR Library: already has ocr");
        return queryResult;
    } else {
        int fileId = -1;
        auto operationItems = predicates.GetOperationList();
        for (DataShare::OperationItem item : operationItems) {
            if (static_cast<string>(item.GetSingle(FIELD_IDX)) == MediaColumn::MEDIA_ID) {
                fileId = std::stoi(static_cast<string>(item.GetSingle(VALUE_IDX)));
                MEDIA_INFO_LOG("Active OCR Library file id: %{public}d", fileId);
            }
        }
        ActivelyStartAnalysisService(fileId);
        queryResult = MediaLibraryRdbStore::Query(RdbUtils::ToPredicates(predicates, cmd.GetTableName()), columns);
    }
    return queryResult;
}
}
}
