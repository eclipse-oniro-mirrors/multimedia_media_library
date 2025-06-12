/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "MediaAssetsRdbOperations"
 
#include "media_assets_rdb_operations.h"

#include <string>

#include "abs_rdb_predicates.h"
#include "media_log.h"
#include "form_map.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_rdbstore.h"
#include "media_file_uri.h"
#include "medialibrary_errno.h"
#include "media_facard_photos_column.h"
#include "medialibrary_facard_operations.h"
#include "ipc_skeleton.h"
#include "medialibrary_type_const.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_photo_operations.h"
#include "vision_column.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "dfx_manager.h"
#include "multistages_capture_manager.h"
#include "enhancement_manager.h"
#include "media_analysis_helper.h"
#include "story_album_column.h"
#include "media_app_uri_permission_column.h"
#include "media_file_utils.h"
#include "mimetype_utils.h"
#include "permission_utils.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include "photo_map_column.h"
#include "photo_file_operation.h"
#include "medialibrary_rdb_utils.h"
#include "thumbnail_service.h"
#include "photo_asset_copy_operation.h"
#include "medialibrary_photo_operations.h"
#include "file_asset.h"
#include "medialibrary_app_uri_sensitive_operations.h"
#include "medialibrary_app_uri_permission_operations.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
constexpr int32_t PHOTO_HIDDEN_FLAG = 1;
constexpr int32_t POSITION_CLOUD_FLAG = 2;
constexpr int32_t CLOUD_COPY_DIRTY_FLAG = 7;

std::mutex MediaAssetsRdbOperations::facardMutex_;
static const string STATUS = "status";
static const string LABEL = "label";
static const string AESTHETICS_SCORE = "aesthetics_score";
static const string OCR = "ocr";
static const string SALIENCY = "saliency";
static const string FACE = "face";
static const string OBJECT = "object";
static const string RECOMMENDATION = "recommendation";
static const string SEGMENTATION = "segmentation";
static const string HEAD = "head";
static const string POSE = "pose";

static vector<int> NEED_UPDATE_TYPE = {
    PhotoAlbumSubType::CLASSIFY, PhotoAlbumSubType::PORTRAIT
};
MediaAssetsRdbOperations::MediaAssetsRdbOperations() {}
bool MediaAssetsRdbOperations::QueryFileIdIfExists(const string& fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("file id queried already exists!");
        return true;
    }
    return false;
}

bool MediaAssetsRdbOperations::QueryFormIdIfExists(const string& formId)
{
    NativeRdb::RdbPredicates rdbPredicate(FormMap::FORM_MAP_TABLE);
    rdbPredicate.EqualTo(FormMap::FORMMAP_FORM_ID, formId);
    vector<string> columns = { FormMap::FORMMAP_FORM_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        MEDIA_INFO_LOG("form id queried already exists!");
        return true;
    }
    return false;
}

int32_t MediaAssetsRdbOperations::RemoveFormInfo(const string& formId)
{
    MEDIA_INFO_LOG("MediaAssetsRdbOperation::RemoveFormInfo enter");
    NativeRdb::RdbPredicates rdbPredicate(FormMap::FORM_MAP_TABLE);
    rdbPredicate.EqualTo(FormMap::FORMMAP_FORM_ID, formId);
    int32_t deleteRows = MediaLibraryRdbStore::Delete(rdbPredicate);
    CHECK_AND_RETURN_RET_WARN_LOG(deleteRows > 0, deleteRows, "deleteRows: %{public}d", deleteRows);
    return deleteRows;
}

int32_t MediaAssetsRdbOperations::RemoveGalleryFormInfo(const string& formId)
{
    lock_guard<mutex> lock(facardMutex_);
    NativeRdb::RdbPredicates rdbPredicate(TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE);
    rdbPredicate.EqualTo(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, formId);
    MediaLibraryFaCardOperations::UnregisterObserver(formId);
    return MediaLibraryRdbStore::Delete(rdbPredicate);
}

int32_t MediaAssetsRdbOperations::SaveFormInfo(const string& formId, const string& uri)
{
    ValuesBucket value;
    value.PutString(FormMap::FORMMAP_URI, uri);

    if (!uri.empty()) {
        MediaFileUri mediaUri(uri);
        CHECK_AND_RETURN_RET_LOG(QueryFileIdIfExists(mediaUri.GetFileId()),
            E_GET_PRAMS_FAIL, "the fileId is not exist");
        vector<int64_t> formIds = { std::stoll(formId) };
        MediaLibraryFormMapOperations::PublishedChange(uri, formIds, true);
    }
    if (QueryFormIdIfExists(formId)) {
        lock_guard<mutex> lock(facardMutex_);
        RdbPredicates predicates(FormMap::FORM_MAP_TABLE);
        predicates.EqualTo(FormMap::FORMMAP_FORM_ID, formId);
        return MediaLibraryRdbStore::UpdateWithDateTime(value, predicates);
    }

    value.PutString(FormMap::FORMMAP_FORM_ID, formId);
    int64_t outRowId = -1;
    lock_guard<mutex> lock(facardMutex_);
    int32_t errCode = MediaLibraryRdbStore::InsertInternal(outRowId, FormMap::FORM_MAP_TABLE, value);
    bool cond = (errCode != NativeRdb::E_OK || outRowId < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "Insert into db failed, errCode = %{public}d", errCode);
    return static_cast<int32_t>(outRowId);
}

int32_t MediaAssetsRdbOperations::SaveGalleryFormInfo(const vector<string>& formIds,
    const vector<string>& fileUris)
{
    lock_guard<mutex> lock(facardMutex_);
    vector<ValuesBucket> values;
    for (size_t i = 0; i < formIds.size(); i++) {
        ValuesBucket value;
        value.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, formIds[i]);
        value.PutString(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, fileUris[i]);
        values.emplace_back(value);
    }
    int64_t outRowId = 0;
    int32_t errCode = MediaLibraryRdbStore::BatchInsert(outRowId, TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE,
        values);
    bool cond = (errCode != NativeRdb::E_OK || outRowId < 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "Batch Insert into db failed, errCode = %{public}d", errCode);
    for (size_t i = 0; i < formIds.size(); i++) {
        MEDIA_INFO_LOG("formId = %{public}s, assetRegisterUri = %{public}s", formIds[i].c_str(),
            fileUris[i].c_str());
        MediaLibraryFaCardOperations::RegisterObserver(formIds[i], fileUris[i]);
    }
    return static_cast<int32_t>(outRowId);
}

int32_t MediaAssetsRdbOperations::CommitEditInsert(const string& editData, int32_t fileId)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    vector<string> columns = {
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::MEDIA_TIME_PENDING,
        PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        MediaColumn::MEDIA_DATE_TAKEN,
    };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_INVALID_VALUES);
    
    shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetAssetFromResultSet(resultSet, columns);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset Failed, fileId=%{public}d", fileId);
        PhotoEditingRecord::GetInstance()->EndCommitEdit(fileId);
        return E_INVALID_VALUES;
    }
    fileAsset->SetId(fileId);
    int32_t ret = MediaLibraryPhotoOperations::CommitEditInsertExecute(fileAsset, editData);
    PhotoEditingRecord::GetInstance()->EndCommitEdit(fileId);
    MEDIA_INFO_LOG("commit edit finished, fileId=%{public}d", fileId);
    return ret;
}

static int32_t UpdateAnalysisTotal(const string& column, const string& selection)
{
    ValuesBucket value;
    value.PutInt(STATUS, 0);
    value.PutString(column, "0");
    NativeRdb::RdbPredicates rdbPredicate(VISION_TOTAL_TABLE);
    rdbPredicate.SetWhereClause(selection);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Can not get rdbstore");
        return E_ERR;
    }
    int32_t changeRows = 0;
    int32_t err = rdbStore->Update(changeRows, value, rdbPredicate);
    CHECK_AND_RETURN_RET(err == NativeRdb::E_OK, err);
    return changeRows;
}

static void DeleteFromVisionTable(const string& fileId, const string& column, const string& table,
    const string& selection)
{
    int32_t updateRows = UpdateAnalysisTotal(column, selection);
    if (updateRows <= 0) {
        return;
    }

    NativeRdb::RdbPredicates rdbPredicate(table);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    MediaLibraryRdbStore::Delete(rdbPredicate);
}

static void DeleteFromVisionTablesForVideo(const string& fileId, const string& table)
{
    NativeRdb::RdbPredicates rdbPredicate(table);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    MediaLibraryRdbStore::Delete(rdbPredicate);
}

void MediaAssetsRdbOperations::DeleteFromVisionTables(const string& fileId)
{
    string selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND label = 1";
    DeleteFromVisionTable(fileId, LABEL, VISION_LABEL_TABLE, selectionTotal);
    DeleteFromVisionTablesForVideo(fileId, VISION_VIDEO_LABEL_TABLE);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND aesthetics_score = 1";
    DeleteFromVisionTable(fileId, AESTHETICS_SCORE, VISION_AESTHETICS_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND ocr = 1";
    DeleteFromVisionTable(fileId, OCR, VISION_OCR_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND saliency = 1";
    DeleteFromVisionTable(fileId, SALIENCY, VISION_SALIENCY_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND face IN (-2, 1, 2, 3, 4)";
    DeleteFromVisionTable(fileId, FACE, VISION_IMAGE_FACE_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND object = 1";
    DeleteFromVisionTable(fileId, OBJECT, VISION_OBJECT_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND recommendation = 1";
    DeleteFromVisionTable(fileId, RECOMMENDATION, VISION_RECOMMENDATION_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND segmentation = 1";
    DeleteFromVisionTable(fileId, SEGMENTATION, VISION_SEGMENTATION_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND head = 1";
    DeleteFromVisionTable(fileId, HEAD, VISION_HEAD_TABLE, selectionTotal);

    selectionTotal = MediaColumn::MEDIA_ID + " = " + fileId + " AND pose = 1";
    DeleteFromVisionTable(fileId, POSE, VISION_POSE_TABLE, selectionTotal);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdbstore");
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByFile(rdbStore, {fileId}, NEED_UPDATE_TYPE);
}

bool MediaAssetsRdbOperations::QueryAlbumIdIfExists(const std::string& albumId)
{
    int32_t count = 0;
    NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
    rdbPredicate.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = { PhotoAlbumColumns::ALBUM_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet != nullptr) {
        resultSet->GetRowCount(count);
        resultSet->Close();
    }
    return count == 1;
}

void MediaAssetsRdbOperations::QueryAssetsUri(const std::vector<std::string> &fileIds,
    std::vector<std::string> &uris)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.In(MediaColumn::MEDIA_ID, fileIds);

    std::vector<std::string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH
    };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        std::string assetPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);

        string extrUri = MediaFileUtils::GetExtraUri(displayName, assetPath);
        uris.push_back(MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, fileId, extrUri));
    }
    resultSet->Close();
}

static bool CheckOprnObject(OperationObject object)
{
    const set<OperationObject> validOprnObjectet = {
        OperationObject::FILESYSTEM_PHOTO,
        OperationObject::FILESYSTEM_AUDIO
    };
    if (validOprnObjectet.find(object) == validOprnObjectet.end()) {
        MEDIA_ERR_LOG("input OperationObject %{public}d error!", object);
        return false;
    }
    return true;
}

static shared_ptr<FileAsset> FetchFileAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    int32_t count = 0;
    int32_t currentRowIndex = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowCount(count) == NativeRdb::E_OK, nullptr, "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowIndex(currentRowIndex) == NativeRdb::E_OK, nullptr, "Cannot get row index of resultset");
    CHECK_AND_RETURN_RET_LOG(currentRowIndex >= 0 && currentRowIndex < count, nullptr, "Invalid row index");

    auto fileAsset = make_shared<FileAsset>();
    auto &map = fileAsset->GetMemberMap();
    for (const auto &column : columns) {
        int32_t columnIndex = 0;
        CHECK_AND_RETURN_RET_LOG(resultSet->GetColumnIndex(column, columnIndex) == NativeRdb::E_OK,
            nullptr, "Can not get column %{private}s index", column.c_str());
        CHECK_AND_RETURN_RET_LOG(FILEASSET_MEMBER_MAP.find(column) != FILEASSET_MEMBER_MAP.end(), nullptr,
            "Can not find column %{private}s from member map", column.c_str());
        switch (FILEASSET_MEMBER_MAP.at(column)) {
            case MEMBER_TYPE_INT32: {
                int32_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetInt(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get int value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_INT64: {
                int64_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetLong(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get long value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_STRING: {
                string value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetString(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get string value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_DOUBLE: {
                double value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetDouble(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get double value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
        }
    }
    return fileAsset;
}

static shared_ptr<FileAsset> GetAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, nullptr,
        "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(count == 1, nullptr, "ResultSet count is %{public}d, not 1", count);
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, nullptr, "Cannot go to first row");
    return FetchFileAssetFromResultSet(resultSet, columns);
}

shared_ptr<FileAsset> MediaAssetsRdbOperations::GetFileAssetFromDb(const string &column,
    const string &value, OperationObject oprnObject, const vector<string> &columns, const string &networkId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryAssetOperations::GetFileAssetFromDb");
    if (!CheckOprnObject(oprnObject) || column.empty() || value.empty()) {
        return nullptr;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return nullptr;
    }

    MediaLibraryCommand cmd(oprnObject, OperationType::QUERY, networkId);
    cmd.GetAbsRdbPredicates()->EqualTo(column, value);

    auto resultSet = rdbStore->Query(cmd, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    return GetAssetFromResultSet(resultSet, columns);
}

const static vector<string> EDITED_COLUMN_VECTOR = {
    PhotoColumn::MEDIA_FILE_PATH,
    PhotoColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_EDIT_TIME,
    PhotoColumn::MEDIA_TIME_PENDING,
    PhotoColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    MediaColumn::MEDIA_DATE_TAKEN,
};

int32_t MediaAssetsRdbOperations::RevertToOrigin(const int32_t &fileId)
{
    shared_ptr<FileAsset> fileAsset = GetFileAssetFromDb(PhotoColumn::MEDIA_ID,
        to_string(fileId), OperationObject::FILESYSTEM_PHOTO, EDITED_COLUMN_VECTOR);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Get FileAsset From Uri Failed, fileId:%{public}d", fileId);
        return E_INVALID_VALUES;
    }
    fileAsset->SetId(fileId);
    CHECK_AND_RETURN_RET(PhotoEditingRecord::GetInstance()->StartRevert(fileId), E_IS_IN_COMMIT);

    int32_t errCode = MediaLibraryPhotoOperations::DoRevertEdit(fileAsset);
    PhotoEditingRecord::GetInstance()->EndRevert(fileId);
    return errCode;
}

int32_t MediaAssetsRdbOperations::GrantPhotoUriPermission(MediaLibraryCommand &cmd)
{
    int32_t ret = MediaLibraryAppUriSensitiveOperations::HandleInsertOperation(cmd);
    CHECK_AND_RETURN_RET(ret == MediaLibraryAppUriSensitiveOperations::SUCCEED, ret);
    return MediaLibraryAppUriPermissionOperations::HandleInsertOperation(cmd);
}

int32_t MediaAssetsRdbOperations::GrantPhotoUrisPermission(
    MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    int32_t ret = MediaLibraryAppUriSensitiveOperations::BatchInsert(cmd, values);
    CHECK_AND_RETURN_RET(ret == MediaLibraryAppUriSensitiveOperations::SUCCEED, ret);
    CHECK_AND_RETURN_RET(!MediaLibraryAppUriSensitiveOperations::BeForceSensitive(cmd, values), ret);
    return MediaLibraryAppUriPermissionOperations::BatchInsert(cmd, values);
}

int32_t MediaAssetsRdbOperations::CancelPhotoUriPermission(NativeRdb::RdbPredicates &rdbPredicate)
{
    return MediaLibraryAppUriPermissionOperations::DeleteOperation(rdbPredicate);
}

int32_t MediaAssetsRdbOperations::StartThumbnailCreationTask(NativeRdb::RdbPredicates &rdbPredicate, int32_t requestId)
{
    MEDIA_INFO_LOG("MediaAssetsRdbOperations::StartThumbnailCreationTask requestId:%{public}d", requestId);
    return ThumbnailService::GetInstance()->CreateAstcBatchOnDemand(rdbPredicate, requestId);
}

int32_t MediaAssetsRdbOperations::StopThumbnailCreationTask(int32_t requestId)
{
    MEDIA_INFO_LOG("MediaAssetsRdbOperations::StopThumbnailCreationTask requestId:%{public}d", requestId);
    ThumbnailService::GetInstance()->CancelAstcBatchTask(requestId);
    return E_OK;
}

int32_t MediaAssetsRdbOperations::QueryEnhancementTaskState(const string& photoUri,
    QueryCloudEnhancementTaskStateDto& dto)
{
    vector<string> columns = {
        MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_CE_STATUS_CODE
    };
    MediaLibraryCommand cmd(PhotoColumn::PHOTOS_TABLE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    cmd.SetDataSharePred(predicates);
    auto resultSet = EnhancementManager::GetInstance().HandleQueryOperation(cmd, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, -E_HAS_DB_ERROR, "Failed to query album!");
    dto.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    dto.photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    dto.ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    dto.ceErrorCode = GetInt32Val(PhotoColumn::PHOTO_CE_STATUS_CODE, resultSet);
    resultSet->Close();
    return E_OK;
}
} // namespace OHOS::Media