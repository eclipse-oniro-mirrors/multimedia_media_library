/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "PhotoMapOperation"

#include "photo_map_operations.h"

#include "media_analysis_helper.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_db_const.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "value_object.h"
#include "vision_column.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "vision_album_column.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"
#include "vision_photo_map_column.h"
#include "dfx_manager.h"
#include "dfx_const.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;

constexpr int32_t ALBUM_IS_REMOVED = 1;
constexpr int32_t USER_OPERATION_MODIFYED = 1;
unordered_map<string, string> dateTypeSecondsMap = {
    {MEDIA_DATA_DB_DATE_ADDED_TO_SECOND, "CAST(P.date_added / 1000 AS BIGINT) AS date_added_s"},
    {MEDIA_DATA_DB_DATE_TRASHED_TO_SECOND, "CAST(P.date_trashed / 1000 AS BIGINT) AS date_trashed_s"},
    {MEDIA_DATA_DB_DATE_MODIFIED_TO_SECOND, "CAST(P.date_modified / 1000 AS BIGINT) AS date_modified_s"},
    {MEDIA_DATA_DB_DATE_TAKEN_TO_SECOND, "CAST(P.date_taken / 1000 AS BIGINT) AS date_taken_s"}
};

static int32_t InsertAnalysisAsset(const DataShareValuesBucket &value,
    std::shared_ptr<TransactionOperations> trans)
{
    CHECK_AND_RETURN_RET_LOG(trans != nullptr, -EINVAL, "transactionOperations is null");
    /**
     * Build insert sql:
     * INSERT INTO AnalysisPhotoMap (map_album, map_asset, order_position) SELECT
     * ?, ?, ?
     * WHERE
     *     (NOT EXISTS (SELECT * FROM AnalysisPhotoMap WHERE map_album = ? AND map_asset = ?))
     *     AND (EXISTS (SELECT file_id FROM Photos WHERE file_id = ?))
     *     AND (EXISTS (SELECT album_id FROM AnalysisAlbum WHERE album_id = ?));
     */
    static const std::string INSERT_MAP_SQL = "INSERT OR IGNORE INTO " + ANALYSIS_PHOTO_MAP_TABLE +
        " (" + PhotoMap::ALBUM_ID + ", " + PhotoMap::ASSET_ID + ", " + ORDER_POSITION + ") " +
        "SELECT ?, ?, ? WHERE " +
        "(NOT EXISTS (SELECT 1 FROM " + ANALYSIS_PHOTO_MAP_TABLE + " WHERE " +
            PhotoMap::ALBUM_ID + " = ? AND " + PhotoMap::ASSET_ID + " = ?)) " +
        "AND (EXISTS (SELECT 1 FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
            MediaColumn::MEDIA_ID + " = ?)) " +
        "AND (EXISTS (SELECT 1 FROM " + ANALYSIS_ALBUM_TABLE +
            " WHERE " + PhotoAlbumColumns::ALBUM_ID + " = ? ));";
    bool isValid = false;
    int32_t albumId = value.Get(PhotoMap::ALBUM_ID, isValid);
    CHECK_AND_RETURN_RET(isValid, -EINVAL);
    int32_t assetId = value.Get(PhotoMap::ASSET_ID, isValid);
    CHECK_AND_RETURN_RET(isValid, -EINVAL);
    const int defaultValue = -1;
    int32_t orderPosition = value.Get(ORDER_POSITION, isValid);
    if (!isValid) {
        orderPosition = defaultValue;
    }

    vector<ValueObject> bindArgs = { albumId, assetId, orderPosition, albumId, assetId, assetId, albumId};
    return trans->ExecuteForLastInsertedRowId(INSERT_MAP_SQL, bindArgs);
}

int32_t PhotoMapOperations::AddPhotoAssets(const vector<DataShareValuesBucket> &values)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }

    int32_t changedRows = 0;
    vector<int32_t> updateIds;
    if (!values.empty()) {
        bool isValid = false;
        int32_t albumId = values[0].Get(PhotoColumn::PHOTO_OWNER_ALBUM_ID, isValid);
        if (!isValid || albumId <= 0) {
            MEDIA_WARN_LOG("Ignore failure on get album id when add assets. isValid: %{public}d, albumId: %{public}d",
                isValid, albumId);
            return changedRows;
        }
        bool hidden = false;
        changedRows = MediaLibraryRdbUtils::UpdateOwnerAlbumId(rdbStore, values, updateIds, hidden);
        MediaLibraryRdbUtils::UpdateUserAlbumInternal(rdbStore, { to_string(albumId) }, false, !hidden);
        MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, {
            to_string(PhotoAlbumSubType::IMAGE), to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::FAVORITE), to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT)
        });

        MEDIA_INFO_LOG("AddPhotoAssets idToUpdateIndex size: %{public}zu", updateIds.size());
        if (!updateIds.empty()) {
            vector<string> idToUpdateIndex;
            for (size_t i = 0; i < updateIds.size(); i++) {
                idToUpdateIndex.push_back(to_string(updateIds[i]));
            }
            MediaAnalysisHelper::AsyncStartMediaAnalysisService(
                static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), idToUpdateIndex);
        }

        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        for (const auto &id : updateIds) {
            string notifyUri = PhotoColumn::PHOTO_URI_PREFIX + to_string(id);
            watch->Notify(MediaFileUtils::Encode(notifyUri), NotifyType::NOTIFY_ALBUM_ADD_ASSET, albumId);
            watch->Notify(MediaFileUtils::Encode(notifyUri), NotifyType::NOTIFY_ALBUM_ADD_ASSET,
                watch->GetAlbumIdBySubType(PhotoAlbumSubType::IMAGE));
            watch->Notify(MediaFileUtils::Encode(notifyUri), NotifyType::NOTIFY_ALBUM_ADD_ASSET,
                watch->GetAlbumIdBySubType(PhotoAlbumSubType::VIDEO));
            watch->Notify(MediaFileUtils::Encode(notifyUri), NotifyType::NOTIFY_ALBUM_ADD_ASSET,
                watch->GetAlbumIdBySubType(PhotoAlbumSubType::FAVORITE));
            watch->Notify(MediaFileUtils::Encode(notifyUri), NotifyType::NOTIFY_ADD);
        }
    }
    return changedRows;
}

static int32_t GetPortraitAlbumIds(const string &albumId, vector<string> &portraitAlbumIds)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr! failed query album order");
    const std::string queryPortraitAlbumIds = "SELECT " + ALBUM_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        GROUP_TAG + " IN(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + ALBUM_ID + " = " + albumId + " AND " + ALBUM_SUBTYPE + " = " + to_string(PORTRAIT) +")";

    auto resultSet = uniStore->QuerySql(queryPortraitAlbumIds);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_DB_FAIL);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        portraitAlbumIds.push_back(to_string(GetInt32Val(ALBUM_ID, resultSet)));
    }
    return E_OK;
}

int32_t PhotoMapOperations::AddHighlightPhotoAssets(const vector<DataShareValuesBucket> &values)
{
    MEDIA_INFO_LOG("Add highlight assets start");
    CHECK_AND_RETURN_RET(!values.empty(), E_DB_FAIL);

    bool isValid = false;
    int32_t albumId = values[0].Get(PhotoMap::ALBUM_ID, isValid);
    bool cond = (!isValid || albumId <= 0);
    CHECK_AND_RETURN_RET(!cond, E_DB_FAIL);

    MEDIA_INFO_LOG("Add highlight assets, id is %{puiblic}d", albumId);
    vector<string> uris;
    std::vector<ValuesBucket> insertValues;
    for (auto value : values) {
        bool isValidValue = false;
        std::string assetUri = value.Get(PhotoMap::ASSET_ID, isValidValue);
        if (MediaLibraryDataManagerUtils::IsNumber(MediaFileUri::GetPhotoId(assetUri))) {
            MEDIA_INFO_LOG("invalid photo id: %{public}s", MediaFileUri::GetPhotoId(assetUri).c_str());
            continue;
        }
        uris.push_back(assetUri);
        int32_t photoId = std::stoi(MediaFileUri::GetPhotoId(assetUri));
        DataShare::DataShareValuesBucket pair;
        pair.Put(PhotoMap::ALBUM_ID, albumId);
        pair.Put(PhotoMap::ASSET_ID, photoId);
        ValuesBucket valueInsert = RdbDataShareAdapter::RdbUtils::ToValuesBucket(pair);
        insertValues.push_back(valueInsert);
    }

    int64_t outRowId = -1;
    int32_t ret = MediaLibraryRdbStore::BatchInsert(outRowId, ANALYSIS_PHOTO_MAP_TABLE, insertValues);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Batch insert highlight assets fail, err = %{public}d", ret);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);

    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, { to_string(albumId) });
    auto watch = MediaLibraryNotify::GetInstance();
    for (auto uri : uris) {
        watch->Notify(MediaFileUtils::Encode(uri), NotifyType::NOTIFY_ALBUM_ADD_ASSET, albumId);
    }
    return ret;
}

int32_t PhotoMapOperations::AddAnaLysisPhotoAssets(const vector<DataShareValuesBucket> &values)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET(rdbStore != nullptr, E_HAS_DB_ERROR);
    if (values.empty()) {
        return 0;
    }
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    int32_t changedRows = 0;
    int32_t err = NativeRdb::E_OK;
    std::function<int(void)> func = [&]()->int {
        for (const auto &value : values) {
            int ret =  InsertAnalysisAsset(value, trans);
            if (ret == E_HAS_DB_ERROR) {
                MEDIA_WARN_LOG("InsertAnalysisAsset for db error, changedRows now: %{public}d", changedRows);
                return ret;
            }
            if (ret > 0) {
                changedRows++;
            }
        }
        return NativeRdb::E_OK;
    };
    err = trans->RetryTrans(func);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "AddAnaLysisPhotoAssets: trans retry fail!, ret:%{public}d", err);

    bool isValid = false;
    std::vector<string> albumIdList;
    for (const auto &value : values) {
        int32_t albumId = value.Get(PhotoMap::ALBUM_ID, isValid);
        if (!isValid || albumId <= 0) {
            MEDIA_WARN_LOG("Ignore failure on get album id when add assets. isValid: %{public}d, albumId: %{public}d",
                isValid, albumId);
            continue;
        }
        albumIdList.push_back(to_string(albumId));
    }
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdList);
    return changedRows;
}

int32_t handleSqlParam(string &sqlParam, const vector<string> &assetsArray)
{
    CHECK_AND_RETURN_RET_LOG(!assetsArray.empty(), E_ERR, "assetsArray is empty ! ");
    for (size_t i = 0; i < assetsArray.size(); i++) {
        sqlParam += assetsArray[i];
        if (i < assetsArray.size() - 1) {
            sqlParam +=",";
        }
    }
    return E_OK;
}

static int32_t ClassfiyAssets(const vector<string> &assetsArray, MergedAlbumInfo &mergedAlbumInfo)
{
    string allSourceAlbumIdsParam = "";
    string allTargetAlbumIdsParam = "";
    string allAssetParam = "";
    CHECK_AND_RETURN_RET_LOG(handleSqlParam(allSourceAlbumIdsParam, mergedAlbumInfo.sourceAlbumIds),
        E_ERR, "handleSqlParam allSourceAlbumIdsParam fail ! ");
    CHECK_AND_RETURN_RET_LOG(handleSqlParam(allTargetAlbumIdsParam, mergedAlbumInfo.targetAlbumIds),
        E_ERR, "handleSqlParam allTargetAlbumIdsParam fail ! ");
    CHECK_AND_RETURN_RET_LOG(handleSqlParam(allAssetParam, assetsArray),
        E_ERR, "handleSqlParam allAssetParam fail ! ");

    const std::string queryPortraitAlbumIds = "SELECT " + MAP_ASSET +
        " FROM ( SELECT " + MAP_ASSET + " FROM " + ANALYSIS_PHOTO_MAP_TABLE + " WHERE " + MAP_ALBUM +
        " IN(" + allSourceAlbumIdsParam + ") AND " + MAP_ASSET + " IN( " + allAssetParam + " ) " +
        " INTERSECT " +
        " SELECT " + MAP_ASSET + " FROM " + ANALYSIS_PHOTO_MAP_TABLE + " WHERE " + MAP_ALBUM +
        " IN(" + allTargetAlbumIdsParam + ") AND " + MAP_ASSET + " IN( " + allAssetParam + " )) ";
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr! ");
    auto resultSet = uniStore->QuerySql(queryPortraitAlbumIds);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr! ");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        mergedAlbumInfo.commonAssets.push_back(GetStringVal(MAP_ASSET, resultSet));
    }
    resultSet->Close();
    for (const auto &item : assetsArray) {
        if (find(mergedAlbumInfo.commonAssets.begin(),
        mergedAlbumInfo.commonAssets.end(), item) == mergedAlbumInfo.commonAssets.end()) {
            mergedAlbumInfo.unCommonAssets.push_back(item);
        }
    }
    return E_OK;
}

static int32_t QueryTagID(vector<string> albumIds, vector<string> &tagId,
    const string &targetAlbumeId, bool isTargetAlbumId)
{
    MEDIA_INFO_LOG("QueryTagID begin ");
    string sql;
    if (isTargetAlbumId) {
        sql = "SELECT " + TAG_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ALBUM_ID + " = " + targetAlbumeId;
    } else {
        string albumIdParams;
        handleSqlParam(albumIdParams, albumIds);
        sql = "SELECT " + TAG_ID + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " +
        ALBUM_ID + " IN(" + albumIdParams + ")";
    }

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr!");
    auto resultSet = uniStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr!");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        tagId.push_back(GetStringVal(TAG_ID, resultSet));
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(!tagId.empty(), E_ERR, "QueryTagID fail. tagId is empty ! ");
    return E_OK;
}

static int32_t HandleImageFaceMoveAsset(const string &albumId, const string &targetAlbumId,
    const vector<string> &assetIds, MergedAlbumInfo &mergedAlbumInfo)
{
    MEDIA_INFO_LOG("HandleImageFaceMoveAsset start");
    vector<string> targetTagId;
    vector<string> tagId;
    QueryTagID(mergedAlbumInfo.targetAlbumIds, targetTagId, targetAlbumId, true);
    QueryTagID(mergedAlbumInfo.sourceAlbumIds, tagId, albumId, false);

    NativeRdb::RdbPredicates rdbPredicate { VISION_IMAGE_FACE_TABLE };
    rdbPredicate.In(TAG_ID, tagId);
    rdbPredicate.And()->In(MediaColumn::MEDIA_ID, assetIds);
    ValuesBucket values;
    CHECK_AND_RETURN_RET_LOG(!targetTagId.empty(), E_ERR, "targetTagId is empty ! ");
    values.PutString(TAG_ID, targetTagId[0]);
    int32_t changedRows = -1;

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "HandleImageFace Failed to get rdbStore.");
    rdbStore->Update(changedRows, values, rdbPredicate);
    return changedRows;
}

static int32_t HandleAnalysisEditOperationMoveAsset(MergedAlbumInfo &mergedAlbumInfo)
{
    ValuesBucket value;
    int32_t changedRows = -1;

    NativeRdb::RdbPredicates rdbPredicate { ANALYSIS_ALBUM_TABLE };
    vector<string> updateAlbumIds = mergedAlbumInfo.sourceAlbumIds;
    for (auto idItem : mergedAlbumInfo.targetAlbumIds) {
        updateAlbumIds.emplace_back(idItem);
    }
    rdbPredicate.In(ALBUM_ID, updateAlbumIds);
    value.PutInt(EDIT_OPERATION, USER_OPERATION_MODIFYED);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t ret = rdbStore->Update(changedRows, value, rdbPredicate);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_ERR, "EditOperationMoveAsset fail.row %{public}d ", changedRows);
    return changedRows;
}

static int32_t HandleCommonAsset(MergedAlbumInfo &mergedAlbumInfo)
{
    MEDIA_INFO_LOG("HandleCommonAsset start");
    vector<std::string> updateMapAlbumIds;
    int32_t deleteRow = -1;
    NativeRdb::RdbPredicates rdbPredicate { ANALYSIS_PHOTO_MAP_TABLE };
    rdbPredicate.In(MAP_ALBUM, mergedAlbumInfo.sourceAlbumIds);
    rdbPredicate.And()->In(MAP_ASSET, mergedAlbumInfo.commonAssets);
    deleteRow = MediaLibraryRdbStore::Delete(rdbPredicate);
    return deleteRow;
}

static int32_t HandleUnCommonAsset(const string &targetAlbumId, MergedAlbumInfo &mergedAlbumInfo)
{
    MEDIA_INFO_LOG("HandleCommonAsset start");
    ValuesBucket value;
    int32_t changedRows = -1;
    NativeRdb::RdbPredicates rdbPredicate { ANALYSIS_PHOTO_MAP_TABLE };
    rdbPredicate.In(MAP_ALBUM, mergedAlbumInfo.sourceAlbumIds);
    rdbPredicate.And()->In(MAP_ASSET, mergedAlbumInfo.unCommonAssets);
    value.PutString(MAP_ALBUM, targetAlbumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t ret = rdbStore->Update(changedRows, value, rdbPredicate);
    return changedRows;
}

static int32_t HandleAlbumMapMoveAsset(const string &targetAlbumId,
    const vector<string> &assetIds, MergedAlbumInfo &mergedAlbumInfo)
{
    if (!mergedAlbumInfo.commonAssets.empty()) {
        int32_t changedRows = HandleCommonAsset(mergedAlbumInfo);
        CHECK_AND_RETURN_RET_LOG(changedRows >= 0, E_ERR, "HandleCommonAsset Fail ");
    }
    int32_t changedRows = HandleUnCommonAsset(targetAlbumId, mergedAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, E_ERR, "HandleUnCommonAsset Fail ");
    return changedRows;
}

int32_t DoSmartMoveAssets(const string &albumId, const string targetAlbumId,
    const vector<string> &assetIds, MergedAlbumInfo &mergedAlbumInfo)
{
    MEDIA_INFO_LOG("DoSmartMoveAssets start ");
    int32_t changedImageFaceRows = HandleImageFaceMoveAsset(albumId, targetAlbumId, assetIds, mergedAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(changedImageFaceRows >= 0, E_ERR, "HandleImageFaceMoveAsset Fail ");

    int32_t changedAlbumMapRows = HandleAlbumMapMoveAsset(targetAlbumId, assetIds, mergedAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(changedAlbumMapRows >= 0, E_ERR, "HandleAlbumMapMoveAsset Fail ");

    int32_t changedAnalysisAlbumRows = HandleAnalysisEditOperationMoveAsset(mergedAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(changedAnalysisAlbumRows >= 0, E_ERR, "HandleAnalysisEditOperationMoveAsset Fail ");

    vector<string> updateMapAlbumIds = mergedAlbumInfo.sourceAlbumIds;
    for (auto idItem : mergedAlbumInfo.targetAlbumIds) {
        updateMapAlbumIds.emplace_back(idItem);
    }
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), updateMapAlbumIds);
    return E_OK;
}

int32_t PhotoMapOperations::SmartMoveAssets(const string &albumId,
    const string &targetAlbumId, const vector<std::string> &assetsArray)
{
    MEDIA_INFO_LOG("SmartMoveAssets start ");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(albumId), E_ERR, "albumId is invail");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(targetAlbumId), E_ERR, "targetAlbumId is invail");
    MergedAlbumInfo mergedAlbumInfo;
    GetPortraitAlbumIds(albumId, mergedAlbumInfo.sourceAlbumIds);
    GetPortraitAlbumIds(targetAlbumId, mergedAlbumInfo.targetAlbumIds);
    ClassfiyAssets(assetsArray, mergedAlbumInfo);

    int32_t ret = DoSmartMoveAssets(albumId, targetAlbumId, assetsArray, mergedAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "SmartMoveAssets Fail ");
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    for (size_t i = 0; i < assetsArray.size(); i++) {
        std::string UpdateUri = PhotoColumn::PHOTO_URI_PREFIX + assetsArray[i];
        watch->Notify(MediaFileUtils::Encode(UpdateUri),
            NotifyType::NOTIFY_ALBUM_DISMISS_ASSET, std::stoi(albumId));
    }
    for (size_t i = 0; i < mergedAlbumInfo.unCommonAssets.size(); i++) {
        std::string UpdateUri = PhotoColumn::PHOTO_URI_PREFIX + mergedAlbumInfo.unCommonAssets[i];
        watch->Notify(MediaFileUtils::Encode(UpdateUri),
            NotifyType::NOTIFY_ALBUM_ADD_ASSET, std::stoi(targetAlbumId));
    }
    return ret;
}

static void GetDismissAssetsPredicates(NativeRdb::RdbPredicates &rdbPredicate, vector<string> &updateAlbumIds,
    PhotoAlbumSubType subtype, const string &albumId, const vector<string> &assetsArray)
{
    if (subtype == PhotoAlbumSubType::PORTRAIT) {
        GetPortraitAlbumIds(albumId, updateAlbumIds);
        rdbPredicate.In(MAP_ALBUM, updateAlbumIds);
        rdbPredicate.And()->In(MAP_ASSET, assetsArray);
    } else {
        rdbPredicate.EqualTo(MAP_ALBUM, albumId);
        rdbPredicate.And()->In(MAP_ASSET, assetsArray);
        updateAlbumIds.push_back(albumId);
    }
}

int32_t DoDismissAssets(int32_t subtype, const string &albumId, const vector<string> &assetIds)
{
    int32_t deleteRow = 0;
    if (subtype == PhotoAlbumSubType::GROUP_PHOTO) {
        NativeRdb::RdbPredicates mapTablePredicate { ANALYSIS_PHOTO_MAP_TABLE };
        mapTablePredicate.EqualTo(MAP_ALBUM, albumId);
        mapTablePredicate.And()->In(MAP_ASSET, assetIds);
        deleteRow = MediaLibraryRdbStore::Delete(mapTablePredicate);
        if (deleteRow != 0 && MediaLibraryDataManagerUtils::IsNumber(albumId)) {
            MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(
                MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), {albumId});
        }
        return deleteRow;
    }

    if (subtype == PhotoAlbumSubType::HIGHLIGHT || subtype == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        if (rdbStore == nullptr) {
            MEDIA_ERR_LOG("RdbStore is nullptr");
        } else {
            int32_t updateHighlight = MediaLibraryRdbUtils::UpdateHighlightPlayInfo(rdbStore, albumId);
            CHECK_AND_PRINT_LOG(updateHighlight >= 0, "Update highlight playinfo fail");
        }
    }
    vector<string> updateAlbumIds;
    NativeRdb::RdbPredicates rdbPredicate { ANALYSIS_PHOTO_MAP_TABLE };
    GetDismissAssetsPredicates(rdbPredicate, updateAlbumIds,
        static_cast<PhotoAlbumSubType>(subtype), albumId, assetIds);
    deleteRow = MediaLibraryRdbStore::Delete(rdbPredicate);
    if (deleteRow <= 0) {
        return deleteRow;
    }
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(
        MediaLibraryUnistoreManager::GetInstance().GetRdbStore(), updateAlbumIds);
    return deleteRow;
}

int32_t PhotoMapOperations::DismissAssets(NativeRdb::RdbPredicates &predicates)
{
    vector<string> whereArgsUri = predicates.GetWhereArgs();
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    
    const vector<string> &whereArgsId = predicates.GetWhereArgs();
    if (whereArgsId.size() == 0 || whereArgsUri.size() == 0) {
        MEDIA_ERR_LOG("No fileAsset to delete");
        return E_INVALID_ARGUMENTS;
    }
    string strAlbumId = whereArgsId[0];
    if (strAlbumId.empty()) {
        MEDIA_ERR_LOG("Failed to get albumId");
        return E_INVALID_ARGUMENTS;
    }

    int32_t albumId = atoi(strAlbumId.c_str());
    if (albumId <= 0) {
        MEDIA_WARN_LOG("Ignore failure on get album id when remove assets, album updating would be lost");
        return E_INVALID_ARGUMENTS;
    }
    string strSubtype = whereArgsId[whereArgsId.size() - 1];
    int32_t subtype = atoi(strSubtype.c_str());
    if (subtype != PhotoAlbumSubType::CLASSIFY && subtype != PhotoAlbumSubType::PORTRAIT &&
        subtype != PhotoAlbumSubType::GROUP_PHOTO && subtype != PhotoAlbumSubType::HIGHLIGHT &&
        subtype != PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        MEDIA_ERR_LOG("Invalid album subtype: %{public}d", subtype);
        return E_INVALID_ARGUMENTS;
    }

    vector<string> assetsArray;
    for (size_t i = 1; i < whereArgsId.size() - 1; i++) {
        assetsArray.push_back(whereArgsId[i]);
    }

    int32_t deleteRow = DoDismissAssets(subtype, strAlbumId, assetsArray);
    if (deleteRow > 0) {
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        for (size_t i = 1; i < whereArgsUri.size() - 1; i++) {
            watch->Notify(MediaFileUtils::Encode(whereArgsUri[i]), NotifyType::NOTIFY_ALBUM_DISMISS_ASSET, albumId);
        }
    }
    return deleteRow;
}

int32_t PhotoMapOperations::RemovePhotoAssets(RdbPredicates &predicates)
{
    vector<string> uriWhereArgs = predicates.GetWhereArgs();
    int32_t deleteRow = 0;
    CHECK_AND_RETURN_RET_LOG(!uriWhereArgs.empty(), deleteRow, "Remove photo assets failed: args is empty");
    MediaLibraryRdbStore::ReplacePredicatesUriToId(predicates);
    vector<string> idWhereArgs = predicates.GetWhereArgs();
    string strAlbumId = idWhereArgs[0];
    int32_t albumId = atoi(strAlbumId.c_str());
    CHECK_AND_WARN_LOG(albumId > 0, "Invalid album Id: %{public}s", strAlbumId.c_str());
    idWhereArgs.erase(idWhereArgs.begin());
    if (idWhereArgs.empty()) {
        MEDIA_WARN_LOG("No photo assets to remove");
        return deleteRow;
    }

    MEDIA_INFO_LOG("Remove %{public}zu photo assets from album %{public}d", idWhereArgs.size(), albumId);

    RdbPredicates predicatesPhotos(PhotoColumn::PHOTOS_TABLE);
    predicatesPhotos.In(MediaColumn::MEDIA_ID, idWhereArgs);
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::REMOTE_ASSETS_BUSSINESS_NAME);
    MediaLibraryPhotoOperations::UpdateSourcePath(idWhereArgs);
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    assetRefresh.Update(deleteRow, values, predicatesPhotos);
    CHECK_AND_RETURN_RET_LOG(deleteRow > 0, E_HAS_DB_ERROR,
        "Update Removed Asset to Trash failed, ret: %{public}d", deleteRow);

    assetRefresh.RefreshAlbum();

    uriWhereArgs.erase(uriWhereArgs.begin());
    MediaAnalysisHelper::StartMediaAnalysisServiceAsync(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), uriWhereArgs);

    auto watch = MediaLibraryNotify::GetInstance();
    if (watch != nullptr) {
        int trashAlbumId = watch->GetAlbumIdBySubType(PhotoAlbumSubType::TRASH);
        CHECK_AND_PRINT_LOG(trashAlbumId > 0,
            "Trash album id error: %{public}d, trash album notification unavailable", trashAlbumId);
        for (size_t i = 0; i < uriWhereArgs.size(); i++) {
            watch->Notify(MediaFileUtils::Encode(uriWhereArgs[i]), NotifyType::NOTIFY_REMOVE);
            watch->Notify(MediaFileUtils::Encode(uriWhereArgs[i]), NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, albumId);
            watch->Notify(MediaFileUtils::Encode(uriWhereArgs[i]), NotifyType::NOTIFY_ALBUM_ADD_ASSET, trashAlbumId);
        }
    } else {
        MEDIA_ERR_LOG("Failed to get notify instance, notification unavailable");
    }
    assetRefresh.Notify();

    DfxManager::GetInstance()->HandleDeleteBehavior(DfxType::ALBUM_REMOVE_PHOTOS, deleteRow, uriWhereArgs);
    return deleteRow;
}

bool IsQueryGroupPhotoAlbumAssets(const string &albumId, string &tagId, int32_t &isRemoved)
{
    bool cond = (albumId.empty() || !MediaLibraryDataManagerUtils::IsNumber(albumId));
    CHECK_AND_RETURN_RET(!cond, false);
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    vector<string> columns = {PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumColumns::ALBUM_SUBTYPE, TAG_ID, IS_REMOVED};
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    cond = (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK);
    CHECK_AND_RETURN_RET(!cond, false);

    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    int32_t albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    tagId = GetStringVal(TAG_ID, resultSet);
    isRemoved = GetInt32Val(IS_REMOVED, resultSet);
    return albumType == PhotoAlbumType::SMART && albumSubtype == PhotoAlbumSubType::GROUP_PHOTO;
}

shared_ptr<OHOS::NativeRdb::ResultSet> QueryGroupPhotoAlbumAssets(const string &albumId, const string &tagId,
    const vector<string> &columns)
{
    string strColumns;
    for (size_t i = 0; i < columns.size(); i++) {
        string column = columns[i];
        if (dateTypeSecondsMap.find(column) != dateTypeSecondsMap.end()) {
            strColumns.append(dateTypeSecondsMap[column]);
        } else {
            strColumns.append("P." + column);
        }
        if (i != columns.size() - 1) {
            strColumns.append(", ");
        }
    }
    string strTags = "'";
    int32_t albumTagCount = 1;
    for (char c : tagId) {
        if (c == ',') {
            strTags.append("', '");
            albumTagCount++;
        } else {
            strTags.push_back(c);
        }
    }
    strTags.append("'");
    string sql = "SELECT " + strColumns + " FROM " + VISION_IMAGE_FACE_TABLE + " F INNER JOIN " +
        ANALYSIS_ALBUM_TABLE + " AA ON F." + TAG_ID + " = AA." + TAG_ID + " AND AA." + GROUP_TAG + " IN (" + strTags +
        ") INNER JOIN " + ANALYSIS_PHOTO_MAP_TABLE + " ON " + MAP_ALBUM + " = AA." + PhotoAlbumColumns::ALBUM_ID +
        " AND " + MAP_ASSET + " = F." + MediaColumn::MEDIA_ID + " INNER JOIN " + PhotoColumn::PHOTOS_TABLE +
        " P ON P." + MediaColumn::MEDIA_ID + " = F." + MediaColumn::MEDIA_ID + " AND " +
        MediaColumn::MEDIA_DATE_TRASHED + " = 0 AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0 AND " +
        PhotoColumn::PHOTO_BURST_COVER_LEVEL + " = 1 AND " + MediaColumn::MEDIA_HIDDEN + " = 0 AND " +
        MediaColumn::MEDIA_TIME_PENDING + " = 0 GROUP BY P." + MediaColumn::MEDIA_ID +
        " HAVING COUNT(" + GROUP_TAG + ") = " + TOTAL_FACES + " AND " +
        " COUNT(DISTINCT " + GROUP_TAG +") = " + to_string(albumTagCount) + ";";
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "rdbstore is nullptr");
    return rdbStore->QuerySql(sql);
}
shared_ptr<OHOS::NativeRdb::ResultSet> PhotoMapOperations::QueryPhotoAssets(const RdbPredicates &rdbPredicate,
    const vector<string> &columns)
{
    if (rdbPredicate.GetWhereArgs().size() <= 0) {
        return nullptr;
    }
    return MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
}
} // namespace OHOS::Media