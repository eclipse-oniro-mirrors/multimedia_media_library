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

#define MLOG_TAG "MediaAlbumsService"

#include "media_albums_service.h"

#include <string>

#include "medialibrary_album_operations.h"
#include "media_albums_rdb_operations.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdbstore.h"
#include "location_column.h"
#include "photo_album_column.h"
#include "story_album_column.h"
#include "media_file_utils.h"
#include "result_set_utils.h"
#include "medialibrary_analysis_album_operations.h"
#include "photo_map_operations.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_business_code.h"
#include "vision_photo_map_column.h"
#include "medialibrary_client_errno.h"
#include "rdb_utils.h"
#include "medialibrary_rdb_utils.h"
#include "photo_map_column.h"
#include "permission_utils.h"
#include "album_operation_uri.h"
#include "datashare_result_set.h"
#include "story_album_column.h"
#include "userfile_manager_types.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "vision_column_comm.h"
#include "vision_album_column.h"
#include "medialibrary_photo_operations.h"
#include "rdb_predicates.h"
#include "dfx_refresh_manager.h"
#include "media_file_utils.h"
#include "refresh_business_name.h"
#include "media_old_albums_column.h"
#include "medialibrary_unistore_manager.h"

using namespace std;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media {

struct HighlightAlbumInfo {
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};

const map<int32_t, struct HighlightAlbumInfo> GetHighlightAlbumInfoMap()
{
    static const std::map<int32_t, struct HighlightAlbumInfo> HIGHLIGHT_ALBUM_INFO_MAP = {
        { COVER_INFO, { PAH_QUERY_HIGHLIGHT_COVER, { ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            AI_ALBUM_ID, SUB_TITLE, CLUSTER_TYPE, CLUSTER_SUB_TYPE,
            CLUSTER_CONDITION, MIN_DATE_ADDED, MAX_DATE_ADDED, GENERATE_TIME, HIGHLIGHT_VERSION,
            REMARKS, HIGHLIGHT_STATUS, RATIO, BACKGROUND, FOREGROUND, WORDART, IS_COVERED, COLOR,
            RADIUS, SATURATION, BRIGHTNESS, BACKGROUND_COLOR_TYPE, SHADOW_LEVEL, TITLE_SCALE_X,
            TITLE_SCALE_Y, TITLE_RECT_WIDTH, TITLE_RECT_HEIGHT, BACKGROUND_SCALE_X, BACKGROUND_SCALE_Y,
            BACKGROUND_RECT_WIDTH, BACKGROUND_RECT_HEIGHT, LAYOUT_INDEX, COVER_ALGO_VERSION, COVER_KEY, COVER_STATUS,
            HIGHLIGHT_IS_MUTED, HIGHLIGHT_IS_FAVORITE, HIGHLIGHT_THEME,
            HIGHLIGHT_PIN_TIME, HIGHLIGHT_USE_SUBTITLE } } },
        { PLAY_INFO, { PAH_QUERY_HIGHLIGHT_PLAY, { ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            MUSIC, FILTER, HIGHLIGHT_PLAY_INFO, IS_CHOSEN, PLAY_INFO_VERSION, PLAY_INFO_ID } } },
        { ALBUM_INFO, { PAH_QUERY_HIGHLIGHT_ALBUM, {ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            HIGHLIGHT_STATUS, HIGHLIGHT_IS_VIEWED, HIGHLIGHT_NOTIFICATION_TIME } } },
    };
    return HIGHLIGHT_ALBUM_INFO_MAP;
}


MediaAlbumsService &MediaAlbumsService::GetInstance()
{
    static MediaAlbumsService service;
    return service;
}

int32_t MediaAlbumsService::DeleteHighlightAlbums(const vector<string>& albumIds)
{
    // Only Highlight albums can be deleted by this way
    MEDIA_INFO_LOG("Delete highlight albums");
    int32_t changedRows = this->rdbOperation_.DeleteHighlightAlbums(albumIds);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, E_HAS_DB_ERROR,
        "Delete highlight album failed, changedRows is %{private}d", changedRows);
    
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");

    if (changedRows > 0) {
        for (size_t i = 0; i < albumIds.size(); ++i) {
            watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX,
                albumIds[i]), NotifyType::NOTIFY_REMOVE);
        }
    }
    return changedRows;
}

int32_t MediaAlbumsService::DeletePhotoAlbums(const std::vector<std::string> &albumIds)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
    rdbPredicate.In(PhotoAlbumColumns::ALBUM_ID, albumIds);
    return MediaLibraryAlbumOperations::DeletePhotoAlbum(rdbPredicate);
}

int32_t MediaAlbumsService::CreatePhotoAlbum(const std::string& albumName)
{
    NativeRdb::ValuesBucket value;
    value.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    MediaLibraryCommand cmd(OperationObject::PHOTO_ALBUM, OperationType::CREATE, value);
    return MediaLibraryAlbumOperations::HandlePhotoAlbumOperations(cmd);
}

int32_t MediaAlbumsService::SetSubtitle(const string& highlightAlbumId, const string& albumSubtitle)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, highlightAlbumId);
    values.Put(SUB_TITLE, albumSubtitle);
    return MediaLibraryAlbumOperations::SetHighlightSubtitle(values, predicates);
}

int32_t MediaAlbumsService::SetHighlightUserActionData(const SetHighlightUserActionDataDto& dto)
{
    int32_t err = this->rdbOperation_.SetHighlightUserActionData(dto);
    return err;
}

int32_t MediaAlbumsService::SetPortraitAlbumName(const ChangeRequestSetAlbumNameDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_NAME, dto.albumName);
    return MediaLibraryAlbumOperations::SetAlbumName(values, predicates);
}

int32_t MediaAlbumsService::SetGroupAlbumName(const ChangeRequestSetAlbumNameDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_NAME, dto.albumName);
    return MediaLibraryAnalysisAlbumOperations::SetGroupAlbumName(values, predicates);
}

int32_t MediaAlbumsService::SetHighlightAlbumName(const ChangeRequestSetAlbumNameDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_NAME, dto.albumName);
    return MediaLibraryAlbumOperations::SetHighlightAlbumName(values, predicates);
}

int32_t MediaAlbumsService::RenameUserAlbum(const string& albumId, const string &newAlbumName)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.Put(PhotoAlbumColumns::ALBUM_NAME, newAlbumName);
    return MediaLibraryAlbumOperations::HandleSetAlbumNameRequest(values, predicates);
}

int32_t MediaAlbumsService::ChangeRequestSetAlbumName(const ChangeRequestSetAlbumNameDto& dto)
{
    switch (dto.albumSubtype) {
        case PhotoAlbumSubType::PORTRAIT:
        case PhotoAlbumSubType::PET:
            return SetPortraitAlbumName(dto);
        case PhotoAlbumSubType::GROUP_PHOTO:
            return SetGroupAlbumName(dto);
        case PhotoAlbumSubType::HIGHLIGHT:
        case PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS:
            return SetHighlightAlbumName(dto);
        default:
            break;
    }
    return RenameUserAlbum(dto.albumId, dto.albumName);
}

int32_t MediaAlbumsService::SetPortraitCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    return MediaLibraryAlbumOperations::SetCoverUri(values, predicates);
}

int32_t MediaAlbumsService::SetGroupAlbumCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    return MediaLibraryAnalysisAlbumOperations::SetGroupCoverUri(values, predicates);
}

int32_t MediaAlbumsService::SetHighlightAlbumCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    return MediaLibraryAlbumOperations::SetHighlightCoverUri(values, predicates);
}

int32_t MediaAlbumsService::SetUserAlbumCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, dto.albumSubtype);
    return MediaLibraryAlbumOperations::UpdateAlbumCoverUri(values, predicates, false);
}

int32_t MediaAlbumsService::SetSourceAlbumCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, dto.albumSubtype);
    return MediaLibraryAlbumOperations::UpdateAlbumCoverUri(values, predicates, false);
}

int32_t MediaAlbumsService::SetSystemAlbumCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, dto.albumId);
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, dto.coverUri);
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, dto.albumSubtype);
    return MediaLibraryAlbumOperations::UpdateAlbumCoverUri(values, predicates, true);
}

int32_t MediaAlbumsService::ChangeRequestSetCoverUri(const ChangeRequestSetCoverUriDto& dto)
{
    switch (dto.albumSubtype) {
        case PhotoAlbumSubType::PORTRAIT:
        case PhotoAlbumSubType::PET:
            return SetPortraitCoverUri(dto);
        case PhotoAlbumSubType::GROUP_PHOTO:
            return SetGroupAlbumCoverUri(dto);
        case PhotoAlbumSubType::HIGHLIGHT:
        case PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS:
            return SetHighlightAlbumCoverUri(dto);
        default:
            break;
    }
    return SetUserAlbumCoverUri(dto);
}

int32_t MediaAlbumsService::ChangeRequestSetDisplayLevel(int32_t displayLevelValue,
    int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    values.Put(USER_DISPLAY_LEVEL, displayLevelValue);
    return MediaLibraryAlbumOperations::SetDisplayLevel(values, predicates);
}

int32_t MediaAlbumsService::ChangeRequestSetIsMe(int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return MediaLibraryAlbumOperations::SetIsMe(values, predicates);
}

int32_t MediaAlbumsService::ChangeRequestDismiss(int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return MediaLibraryAnalysisAlbumOperations::DismissGroupPhotoAlbum(values, predicates);
}

int32_t MediaAlbumsService::ChangeRequestResetCoverUri(int32_t albumId, PhotoAlbumSubType albumSubType)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    values.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return MediaLibraryAlbumOperations::ResetCoverUri(values, predicates);
}

int32_t MediaAlbumsService::AlbumCommitModify(const AlbumCommitModifyDto& commitModifyDto, int32_t businessCode)
{
    DataShare::DataSharePredicates predicates;
    NativeRdb::ValuesBucket values;
    if (businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY)) {
        values.Put(PhotoAlbumColumns::ALBUM_NAME, commitModifyDto.albumName);
    }
    values.Put(PhotoAlbumColumns::ALBUM_COVER_URI, commitModifyDto.coverUri);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(commitModifyDto.albumId));

    int32_t changedRows = MediaLibraryAlbumOperations::UpdatePhotoAlbum(values, predicates);
    return changedRows;
}

static shared_ptr<NativeRdb::ResultSet> QueryPhoto(int32_t albumId, bool isSmartAlbum)
{
    NativeRdb::RdbPredicates querypred(PhotoAlbumColumns::TABLE);
    querypred.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    querypred.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);

    auto whereClause = querypred.GetWhereClause();
    CHECK_AND_RETURN_RET_LOG(MediaLibraryCommonUtils::CheckWhereClause(whereClause), nullptr,
        "illegal query whereClause input %{private}s", whereClause.c_str());

    vector<string> fetchColumn = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_COUNT,
    };
    if (!isSmartAlbum) {
        fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    }
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(const_cast<vector<string> &>(fetchColumn));
    return MediaLibraryRdbStore::QueryWithFilter(querypred, fetchColumn);
}

static int32_t FetchNewCount(shared_ptr<NativeRdb::ResultSet> resultSet, AlbumPhotoQueryRespBody& respBody)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet == nullptr");
        return E_ERR;
    }

    if (resultSet->GoToFirstRow() != 0) {
        MEDIA_ERR_LOG("go to first row failed");
        return E_ERR;
    }

    respBody.newCount =
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, resultSet, TYPE_INT32));
    respBody.newImageCount =
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, TYPE_INT32));
    respBody.newVideoCount =
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, TYPE_INT32));
    return E_OK;
}

int32_t MediaAlbumsService::AlbumAddAssets(const AlbumAddAssetsDto& addAssetsDto, AlbumPhotoQueryRespBody& respBody)
{
    vector<DataShare::DataShareValuesBucket> valuesBuckets;
    for (const auto &assetId : addAssetsDto.assetsArray) {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, addAssetsDto.albumId);
        valuesBucket.Put(PhotoColumn::MEDIA_ID, assetId);
        valuesBuckets.push_back(valuesBucket);
    }

    int32_t changedRows = PhotoMapOperations::AddPhotoAssets(valuesBuckets);
    if (changedRows < 0) {
        return changedRows;
    }

    auto resultSet = QueryPhoto(addAssetsDto.albumId, addAssetsDto.isSmartAlbum);
    if (FetchNewCount(resultSet, respBody) != E_OK) {
        return -1;
    }
    return changedRows;
}

int32_t MediaAlbumsService::AlbumRemoveAssets(const AlbumRemoveAssetsDto& removeAssetsDto,
    AlbumPhotoQueryRespBody& respBody)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(removeAssetsDto.albumId));
    predicates.And()->In(PhotoColumn::MEDIA_ID, removeAssetsDto.assetsArray);

    auto whereClause = predicates.GetWhereClause();
    CHECK_AND_RETURN_RET_LOG(MediaLibraryCommonUtils::CheckWhereClause(whereClause), E_SQL_CHECK_FAIL,
        "illegal query whereClause input %{private}s", whereClause.c_str());

    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoMap::TABLE);
    int32_t changedRows = PhotoMapOperations::RemovePhotoAssets(rdbPredicate);
    if (changedRows < 0) {
        return changedRows;
    }

    auto resultSet = QueryPhoto(removeAssetsDto.albumId, removeAssetsDto.isSmartAlbum);
    if (FetchNewCount(resultSet, respBody) != E_OK) {
        return -1;
    }
    return changedRows;
}

int32_t MediaAlbumsService::AlbumRecoverAssets(const AlbumRecoverAssetsDto& recoverAssetsDto)
{
    DataShare::DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, recoverAssetsDto.uris);

    return MediaLibraryAlbumOperations::RecoverPhotoAssets(predicates);
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAlbumsService::AlbumGetSelectedAssets(
    AlbumGetSelectedAssetsDto &dto)
{
    int curFileId = 0;
    double maxScore = 250;
    double minScore = 0;
    if (!dto.filter.empty()) {
        nlohmann::json filterJson = nlohmann::json::parse(dto.filter.c_str(), nullptr, false);
        std::string fileIdField = "currentFileId";
        bool cond =
            (filterJson.is_discarded() || !filterJson.contains(fileIdField) || !filterJson[fileIdField].is_string());
        CHECK_AND_RETURN_RET_LOG(!cond, nullptr, "failed to verify the filter format");
        std::string fileId = filterJson["currentFileId"].get<std::string>();
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(fileId), nullptr,
            "AlbumGetSelectedAssets get score fail");
        curFileId = std::stoi(fileId);
        maxScore = this->rdbOperation_.GetAssetScore(dto, curFileId);
    }
    minScore = this->rdbOperation_.GetLimitScore(dto);
    if (minScore >= maxScore) {
        MEDIA_ERR_LOG("AlbumGetSelectedAssets minScore >= maxScore");
        return std::make_shared<DataShare::DataShareResultSet>();
    }
    dto.minScore = minScore;
    dto.maxScore = maxScore;
    return this->rdbOperation_.GetSelectedAssets(dto);
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAlbumsService::AlbumGetAssets(AlbumGetAssetsDto &dto)
{
    auto startTime = MediaFileUtils::UTCTimeMilliSeconds();
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(dto.columns);
    NativeRdb::RdbPredicates predicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, PhotoColumn::PHOTOS_TABLE);
    auto resultSet = PhotoMapOperations::QueryPhotoAssets(predicates, dto.columns);
    auto totalCostTime = MediaFileUtils::UTCTimeMilliSeconds() - startTime;
    AccurateRefresh::DfxRefreshManager::QueryStatementReport(
        AccurateRefresh::GET_ASSETS_BUSSINESS_NAME, totalCostTime, predicates.GetStatement());
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to query album assets");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

static void LogQueryParams(const DataShare::DataSharePredicates &predicates, const vector<string> &fetchColumn)
{
    std::string sqlColumns;
    for (const auto &column : fetchColumn) {
        if (!sqlColumns.empty()) {
            sqlColumns += ",";
        }
        sqlColumns += column;
    }
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    std::string sqlWhere = rdbPredicate.GetWhereClause();
    for (const auto &whereArg : rdbPredicate.GetWhereArgs()) {
        sqlWhere.replace(sqlWhere.find("?"), 1, whereArg);
    }
    std::string sqlOrder = rdbPredicate.GetOrder();
    MEDIA_INFO_LOG("Predicates size = %{public}d", static_cast<int32_t>(predicates.GetOperationList().size()));
    MEDIA_INFO_LOG("sqlColumns: %{public}s", sqlColumns.c_str());
    MEDIA_INFO_LOG("sqlOrder: %{public}s", sqlOrder.c_str());
}

static bool CheckAlbumFetchColumns(const vector<string> &fetchColumn)
{
    for (const auto &column : fetchColumn) {
        if (!PhotoAlbumColumns::IsPhotoAlbumColumn(column)) {
            MEDIA_ERR_LOG("Invalid columns:%{public}s", column.c_str());
            return false;
        }
    }
    return true;
}

static void ReplaceFetchColumn(std::vector<std::string> &fetchColumn,
    const std::string &oldColumn, const std::string &newColumn)
{
    auto it = std::find(fetchColumn.begin(), fetchColumn.end(), oldColumn);
    if (it != fetchColumn.end()) {
        it->assign(newColumn);
    }
}

static void AddDefaultPhotoAlbumColumns(vector<string> &fetchColumn)
{
    auto columns = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    for (const auto &column : fetchColumn) {
        if (columns.count(column) == 0) {
            columns.insert(column);
        }
    }
    fetchColumn.assign(columns.begin(), columns.end());
}

static void AddNoSmartFetchColumns(std::vector<std::string> &fetchColumn)
{
    AddDefaultPhotoAlbumColumns(fetchColumn);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_LPATH);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
}

static void AddPhotoAlbumTypeFilter(DataShare::DataSharePredicates &predicates,
    int32_t albumType, int32_t albumSubType)
{
    if (albumType != PhotoAlbumType::INVALID) {
        predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    }
    if (albumSubType != PhotoAlbumSubType::ANY) {
        predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
        if (albumSubType == PhotoAlbumSubType::SHOOTING_MODE || albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
            predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_COUNT);
        }
    }
    if (PermissionUtils::IsSystemApp()) {
        predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));
    } else {
        predicates.And()->In(PhotoAlbumColumns::ALBUM_SUBTYPE, vector<string>({
            to_string(PhotoAlbumSubType::USER_GENERIC),
            to_string(PhotoAlbumSubType::FAVORITE),
            to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::IMAGE),
            to_string(PhotoAlbumSubType::SOURCE_GENERIC),
        }));
    }
}

static NativeRdb::RdbPredicates GetAllLocationPredicates(DataShare::DataSharePredicates &predicates)
{
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_LATITUDE, to_string(0));
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_LONGITUDE, to_string(0));
    return RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
}

static void ReplacePredicatesColumn(DataShare::DataSharePredicates& predicates,
    const std::string &oldColumn, const std::string &newColumn)
{
    constexpr int32_t fieldIdx = 0;
    auto& items = predicates.GetOperationList();
    std::vector<DataShare::OperationItem> tmpOperations = {};
    for (const DataShare::OperationItem& item : items) {
        if (item.singleParams.empty()) {
            tmpOperations.push_back(item);
            continue;
        }
        if (static_cast<string>(item.GetSingle(fieldIdx)) == oldColumn) {
            DataShare::OperationItem tmpItem = item;
            tmpItem.singleParams[fieldIdx] = newColumn;
            tmpOperations.push_back(tmpItem);
            continue;
        }
        tmpOperations.push_back(item);
    }
    predicates = DataShare::DataSharePredicates(move(tmpOperations));
}

static void AddHighlightAlbumPredicates(DataShare::DataSharePredicates& predicates, int32_t albumSubType)
{
    std::string analysisAlbumId = ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID;
    vector<string> onClause = {
        analysisAlbumId + " = " + HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
    };
    if (albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        onClause = {
            analysisAlbumId + " = " + HIGHLIGHT_ALBUM_TABLE + "." + AI_ALBUM_ID,
        };
    }
    predicates.InnerJoin(HIGHLIGHT_ALBUM_TABLE)->On(onClause);
    predicates.OrderByDesc(MAX_DATE_ADDED + ", " + GENERATE_TIME);
    ReplacePredicatesColumn(predicates, PhotoAlbumColumns::ALBUM_ID, analysisAlbumId);
}

static void GenerateAnalysisCmd(MediaLibraryCommand &cmd,
    DataShare::DataSharePredicates &predicates)
{
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());
    cmd.GetAbsRdbPredicates()->Limit(rdbPredicate.GetLimit());
    cmd.GetAbsRdbPredicates()->Offset(rdbPredicate.GetOffset());
}

int32_t MediaAlbumsService::QueryAlbums(QueryAlbumsDto &dto)
{
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    std::vector<std::string> &columns = dto.columns;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    auto startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::string sqlStr = "";
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY);
    if (dto.albumType == PhotoAlbumType::SMART) {
        if (dto.albumSubType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
            NativeRdb::RdbPredicates rdbPredicate = GetAllLocationPredicates(dto.predicates);
            const auto &locations = PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS;
            columns.insert(columns.end(), locations.begin(), locations.end());
            resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
            sqlStr = rdbPredicate.GetStatement();
        } else if (dto.albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
            columns = PhotoAlbumColumns::CITY_DEFAULT_FETCH_COLUMNS;
            std::string onClause = PhotoAlbumColumns::ALBUM_NAME  + " = " + CITY_ID;
            dto.predicates.InnerJoin(GEO_DICTIONARY_TABLE)->On({ onClause });
            dto.predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_COUNT, to_string(0));
            AddPhotoAlbumTypeFilter(dto.predicates, dto.albumType, dto.albumSubType);
            GenerateAnalysisCmd(cmd, dto.predicates);
            resultSet = MediaLibraryDataManager::QueryAnalysisAlbum(cmd, columns, dto.predicates);
            sqlStr = cmd.GetAbsRdbPredicates()->GetStatement();
        } else {
            AddDefaultPhotoAlbumColumns(columns);
            if (dto.albumSubType == PhotoAlbumSubType::HIGHLIGHT ||
                dto.albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
                AddHighlightAlbumPredicates(dto.predicates, dto.albumSubType);
                std::string analysisAlbumId = ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID +
                    " AS " + PhotoAlbumColumns::ALBUM_ID;
                ReplaceFetchColumn(columns, PhotoAlbumColumns::ALBUM_ID, analysisAlbumId);
            }
            AddPhotoAlbumTypeFilter(dto.predicates, dto.albumType, dto.albumSubType);
            GenerateAnalysisCmd(cmd, dto.predicates);
            resultSet = MediaLibraryDataManager::QueryAnalysisAlbum(cmd, columns, dto.predicates);
            sqlStr = cmd.GetAbsRdbPredicates()->GetStatement();
        }
    } else {
        AddNoSmartFetchColumns(columns);
        AddPhotoAlbumTypeFilter(dto.predicates, dto.albumType, dto.albumSubType);
        NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(dto.predicates, PhotoAlbumColumns::TABLE);
        if (rdbPredicate.GetOrder().empty()) {
            rdbPredicate.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
        }
        resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
        sqlStr = rdbPredicate.GetStatement();
    }
    int64_t totalCostTime = MediaFileUtils::UTCTimeMilliSeconds() - startTime;
    AccurateRefresh::DfxRefreshManager::QueryStatementReport(
        AccurateRefresh::DEAL_ALBUMS_BUSSINESS_NAME, totalCostTime, sqlStr);
    LogQueryParams(dto.predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet nullptr");
    auto bridge = RdbUtils::ToResultSetBridge(resultSet);
    dto.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    return E_OK;
}

int32_t MediaAlbumsService::QueryHiddenAlbums(QueryAlbumsDto &dto)
{
    std::vector<std::string> &columns = dto.columns;
    if (!CheckAlbumFetchColumns(columns)) {
        return E_INVALID_VALUES;
    }

    AddNoSmartFetchColumns(columns);
    if (dto.hiddenAlbumFetchMode == 1) {
        columns.push_back(PhotoAlbumColumns::HIDDEN_COUNT);
        columns.push_back(PhotoAlbumColumns::HIDDEN_COVER);
        dto.predicates.And()->EqualTo(PhotoAlbumColumns::CONTAINS_HIDDEN, to_string(1));
        dto.predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::HIDDEN);
        dto.predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::TRASH);
    } else {
        dto.predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::HIDDEN);
    }

    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(dto.predicates, PhotoAlbumColumns::TABLE);
    if (rdbPredicate.GetOrder().empty()) {
        rdbPredicate.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
    }

    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);

    LogQueryParams(dto.predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet nullptr");
    auto bridge = RdbUtils::ToResultSetBridge(resultSet);
    dto.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    return E_OK;
}

int32_t MediaAlbumsService::GetPhotoIndex(GetPhotoIndexReqBody &reqBody, QueryResultRespBody &respBody)
{
    DataShare::DataSharePredicates &predicates = reqBody.predicates;
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    const string &photoId = reqBody.photoId;
    const string &albumId = reqBody.albumId;
    MEDIA_INFO_LOG("GetPhotoIndex photoId:%{public}s, albumId:%{public}s", photoId.c_str(), albumId.c_str());

    MediaLibraryCommand cmd(predicates);
    shared_ptr<NativeRdb::ResultSet> resSet = nullptr;
    if (reqBody.isAnalysisAlbum) {
        resSet = MediaLibraryPhotoOperations::HandleAnalysisIndex(cmd, photoId, albumId);
    } else {
        NativeRdb::RdbPredicates predicates =
            RdbDataShareAdapter::RdbUtils::ToPredicates(cmd.GetDataSharePred(), PhotoColumn::PHOTOS_TABLE);
        resSet = MediaLibraryPhotoOperations::HandleIndexOfUri(cmd, predicates, photoId, albumId);
    }
    if (resSet == nullptr) {
        return E_FAIL;
    }
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_SUCCESS;
}

int32_t MediaAlbumsService::GetHighlightAlbumInfo(GetHighlightAlbumReqBody &reqBody, QueryResultRespBody &respBody)
{
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;

    auto infoMap = GetHighlightAlbumInfoMap();
    if (infoMap.find(reqBody.highlightAlbumInfoType) != infoMap.end()) {
        columns = infoMap.at(reqBody.highlightAlbumInfoType).fetchColumn;
        string tabStr;
        if (reqBody.highlightAlbumInfoType == COVER_INFO) {
            tabStr = HIGHLIGHT_COVER_INFO_TABLE;
        } else {
            tabStr = HIGHLIGHT_PLAY_INFO_TABLE;
        }
        vector<string> onClause = {
            tabStr + "." + PhotoAlbumColumns::ALBUM_ID + " = " + HIGHLIGHT_ALBUM_TABLE + "." + ID
        };
        CHECK_AND_EXECUTE(reqBody.highlightAlbumInfoType == ALBUM_INFO,
            predicates.InnerJoin(HIGHLIGHT_ALBUM_TABLE)->On(onClause));
    } else {
        MEDIA_ERR_LOG("Invalid highlightAlbumInfoType");
        return E_ERR;
    }
    int32_t albumId = reqBody.albumId;
    int32_t subType = reqBody.subType;
    if (subType == static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT)) {
        predicates.EqualTo(HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    } else if (subType == static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS)) {
        predicates.EqualTo(HIGHLIGHT_ALBUM_TABLE + "." + AI_ALBUM_ID, to_string(albumId));
    } else {
        MEDIA_ERR_LOG("Invalid highlight album subType");
        return E_ERR;
    }

    std::string tableName = "";
    if (reqBody.highlightAlbumInfoType == static_cast<int32_t>(HighlightAlbumInfoType::COVER_INFO)) {
        tableName = HIGHLIGHT_COVER_INFO_TABLE;
    } else if (reqBody.highlightAlbumInfoType == static_cast<int32_t>(HighlightAlbumInfoType::PLAY_INFO)) {
        tableName = HIGHLIGHT_PLAY_INFO_TABLE;
    } else if (reqBody.highlightAlbumInfoType == static_cast<int32_t>(HighlightAlbumInfoType::ALBUM_INFO)) {
        tableName = HIGHLIGHT_ALBUM_TABLE;
    } else {
        MEDIA_ERR_LOG("highlightAlbumInfoType not valid");
        return E_ERR;
    }

    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::QueryWithFilter(
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName), columns);

    CHECK_AND_RETURN_RET(resSet != nullptr, E_FAIL);
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_SUCCESS;
}

int32_t MediaAlbumsService::UpdatePhotoAlbumOrder(const SetPhotoAlbumOrderDto &setPhotoAlbumOrderDto)
{
    CHECK_AND_RETURN_RET_LOG(setPhotoAlbumOrderDto.CheckArray(), E_INNER_FAIL,
        "SetPhotoAlbumOrderDto can not be used to update album order");
    
    vector<NativeRdb::ValuesBucket> valuesBuckets;
    vector<NativeRdb::RdbPredicates> predicatesArray;
    for (size_t i = 0; i < setPhotoAlbumOrderDto.albumIds.size(); i++) {
        NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, setPhotoAlbumOrderDto.albumIds[i]);
        predicatesArray.push_back(predicates);

        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.Put(setPhotoAlbumOrderDto.albumOrderColumn, setPhotoAlbumOrderDto.albumOrders[i]);
        valuesBucket.Put(setPhotoAlbumOrderDto.orderSectionColumn, setPhotoAlbumOrderDto.orderSection[i]);
        valuesBucket.Put(setPhotoAlbumOrderDto.orderTypeColumn, setPhotoAlbumOrderDto.orderType[i]);
        valuesBucket.Put(setPhotoAlbumOrderDto.orderStatusColumn, setPhotoAlbumOrderDto.orderStatus[i]);
        valuesBuckets.push_back(valuesBucket);
    }

    int32_t changedRows = MediaLibraryAlbumOperations::UpdatePhotoAlbumOrder(valuesBuckets, predicatesArray);
    return changedRows;
}

int32_t MediaAlbumsService::SmartMoveAssets(ChangeRequestMoveAssetsDto &smartMoveAssetsDto)
{
    MEDIA_INFO_LOG("MediaAlbumsService::SmartMoveAssets");
    CHECK_AND_RETURN_RET_LOG(!smartMoveAssetsDto.assets.empty(), E_INNER_FAIL,
        "SmartMoveAssets assets is empery");

    vector<std::string> assets;
    for (const string asset : smartMoveAssetsDto.assets) {
        size_t pos = asset.find(PhotoColumn::PHOTO_URI_PREFIX);
        if (pos != string::npos) {
            string fileId = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(asset);
            CHECK_AND_CONTINUE(MediaFileUtils::IsValidInteger(fileId));
            assets.push_back(fileId);
        }
    }

    string albumId = to_string(smartMoveAssetsDto.albumId);
    string targetAlbumId = to_string(smartMoveAssetsDto.targetAlbumId);
    int32_t ret = PhotoMapOperations::SmartMoveAssets(albumId, targetAlbumId, assets);
    return ret;
}

int32_t MediaAlbumsService::MoveAssets(ChangeRequestMoveAssetsDto &moveAssetsDto)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(moveAssetsDto.albumId));
    predicates.And();
    predicates.In(PhotoColumn::MEDIA_ID, moveAssetsDto.assets);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates,
        PhotoColumn::PHOTOS_TABLE);

    NativeRdb::ValuesBucket value;
    value.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, moveAssetsDto.targetAlbumId);

    MediaLibraryCommand cmd(OperationObject::PAH_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    cmd.SetValueBucket(value);
    cmd.SetDataSharePred(predicates);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    int32_t ret = MediaLibraryPhotoOperations::BatchSetOwnerAlbumId(cmd);

    auto resultSet = this->rdbOperation_.MoveAssetsGetAlbumInfo(moveAssetsDto);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_ERR);
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        int32_t albumImageCount = GetInt32Val(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet);
        int32_t albumVideoCount = GetInt32Val(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet);
        int32_t albumCount = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
        if (albumId == moveAssetsDto.albumId) {
            moveAssetsDto.albumImageCount = albumImageCount;
            moveAssetsDto.albumVideoCount = albumVideoCount;
            moveAssetsDto.albumCount = albumCount;
            continue;
        }
        moveAssetsDto.targetAlbumImageCount = albumImageCount;
        moveAssetsDto.targetAlbumVideoCount = albumVideoCount;
        moveAssetsDto.targetAlbumCount = albumCount;
    }
    resultSet->Close();
    return ret;
}

int32_t MediaAlbumsService::AddAssets(ChangeRequestAddAssetsDto &addAssetsDto, ChangeRequestAddAssetsRespBody &respBody)
{
    std::vector<DataShare::DataShareValuesBucket> valuesBuckets;
    if (addAssetsDto.isHighlight) {
        for (auto asset : addAssetsDto.assets) {
            DataShare::DataShareValuesBucket pair;
            pair.Put(MAP_ALBUM, addAssetsDto.albumId);
            pair.Put(MAP_ASSET, asset);
            valuesBuckets.push_back(pair);
        }
        return PhotoMapOperations::AddHighlightPhotoAssets(valuesBuckets);
    }

    for (auto asset : addAssetsDto.assets) {
        DataShare::DataShareValuesBucket pair;
        pair.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, addAssetsDto.albumId);
        pair.Put(PhotoColumn::MEDIA_ID, asset);
        valuesBuckets.push_back(pair);
    }
    int32_t ret = PhotoMapOperations::AddPhotoAssets(valuesBuckets);

    auto resultSet = this->rdbOperation_.AddAssetsGetAlbumInfo(addAssetsDto);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_ERR);
    if (resultSet->GoToFirstRow() == E_OK) {
        if (!addAssetsDto.isHiddenOnly) {
            respBody.imageCount = GetInt32Val(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet);
            respBody.videoCount = GetInt32Val(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet);
        }
        respBody.albumCount = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    }
    resultSet->Close();
    return ret;
}

int32_t MediaAlbumsService::RemoveAssets(
    ChangeRequestRemoveAssetsDto &removeAssetsDto, ChangeRequestRemoveAssetsRespBody &respBody)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(removeAssetsDto.albumId));
    predicates.In(PhotoColumn::MEDIA_ID, removeAssetsDto.assets);
    NativeRdb::RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoMap::TABLE);
    int32_t ret = PhotoMapOperations::RemovePhotoAssets(rdbPredicate);

    auto resultSet = this->rdbOperation_.RemoveAssetsGetAlbumInfo(removeAssetsDto);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_ERR);
    if (resultSet->GoToFirstRow() == E_OK) {
        if (!removeAssetsDto.isHiddenOnly) {
            respBody.imageCount = GetInt32Val(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet);
            respBody.videoCount = GetInt32Val(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet);
        }
        respBody.albumCount = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
    }
    resultSet->Close();
    return ret;
}

int32_t MediaAlbumsService::RecoverAssets(ChangeRequestRecoverAssetsDto &recoverAssetsDto)
{
    // Get all recoverable fileIds
    std::vector<std::string> targetFileIds;
    int32_t ret =
        this->mediaAssetsRecoverService_.BatchMoveOutTrashAndMergeWithSameAsset(recoverAssetsDto.assets, targetFileIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "BatchMoveOutTrashAndMergeWithSameAsset failed, ret: %{public}d", ret);
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, targetFileIds);
    return MediaLibraryAlbumOperations::RecoverPhotoAssets(predicates);
}

int32_t MediaAlbumsService::DeleteAssets(ChangeRequestDeleteAssetsDto &deleteAssetsDto)
{
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, deleteAssetsDto.assets);
    predicates.GreaterThan(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    return MediaLibraryAlbumOperations::DeletePhotoAssets(predicates, false, false);
}

int32_t MediaAlbumsService::DismissAssets(ChangeRequestDismissAssetsDto &dismissAssetsDto)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MAP_ALBUM, to_string(dismissAssetsDto.albumId));
    predicates.In(MAP_ASSET, dismissAssetsDto.assets);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(dismissAssetsDto.photoAlbumSubType));
    NativeRdb::RdbPredicates rdbPredicate =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, ANALYSIS_PHOTO_MAP_TABLE);
    int32_t ret = PhotoMapOperations::DismissAssets(rdbPredicate);
    if (ret < 0) {
        MEDIA_ERR_LOG("DismissAssets Error, ret: %{public}d", ret);
        return ret;
    }
    if (dismissAssetsDto.photoAlbumSubType == PhotoAlbumSubType::PORTRAIT ||
        dismissAssetsDto.photoAlbumSubType == PhotoAlbumSubType::PET) {
        DataShare::DataShareValuesBucket updateValues;
        const int32_t PORTRAIT_REMOVED = -3;
        const std::string TAG_ID = "tag_id";
        updateValues.Put(TAG_ID, std::to_string(PORTRAIT_REMOVED));
        NativeRdb::ValuesBucket value = RdbDataShareAdapter::RdbUtils::ToValuesBucket(updateValues);
        DataShare::DataSharePredicates updatePredicates;
        std::string selection = std::to_string(dismissAssetsDto.albumId);
        for (size_t i = 0; i < dismissAssetsDto.assets.size(); ++i) {
            selection += "," + dismissAssetsDto.assets[i];
        }
        updatePredicates.SetWhereClause(selection);
        NativeRdb::RdbPredicates rdbPredicateUpdate =
            RdbDataShareAdapter::RdbUtils::ToPredicates(updatePredicates, VISION_IMAGE_FACE_TABLE);

        MediaLibraryCommand cmd(OperationObject::VISION_IMAGE_FACE, OperationType::UPDATE, MediaLibraryApi::API_10);
        cmd.SetValueBucket(value);
        cmd.SetDataSharePred(updatePredicates);
        cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicateUpdate.GetWhereClause());
        cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicateUpdate.GetWhereArgs());
        cmd.SetApiParam(MEDIA_OPERN_KEYWORD, UPDATE_DISMISS_ASSET);
        auto dataManager = MediaLibraryDataManager::GetInstance();
        dataManager->HandleAnalysisFaceUpdate(cmd, value, updatePredicates);
    }
    return ret;
}

int32_t MediaAlbumsService::MergeAlbum(ChangeRequestMergeAlbumDto &mergeAlbumDto)
{
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, mergeAlbumDto.albumId);
    valuesBucket.Put(TARGET_ALBUM_ID, mergeAlbumDto.targetAlbumId);
    NativeRdb::ValuesBucket value = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("MergeAlbum:Input parameter is invalid ");
        return E_INVALID_VALUES;
    }
    return MediaLibraryAlbumOperations::MergeAlbums(value);
}

int32_t MediaAlbumsService::PlaceBefore(ChangeRequestPlaceBeforeDto &placeBeforeDto)
{
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, placeBeforeDto.albumId);
    valuesBucket.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, placeBeforeDto.referenceAlbumId);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_TYPE, placeBeforeDto.albumType);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, placeBeforeDto.albumSubType);
    NativeRdb::ValuesBucket value = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("PlaceBefore:Input parameter is invalid ");
        return E_INVALID_VALUES;
    }

    return MediaLibraryAlbumOperations::OrderSingleAlbum(value);
}

int32_t MediaAlbumsService::GetAlbumsByIds(GetAlbumsByIdsDto &getAlbumsByIdsDto, GetAlbumsByIdsRespBody &respBody)
{
    std::vector<std::string> columns = getAlbumsByIdsDto.columns;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);

    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(getAlbumsByIdsDto.predicates, PhotoAlbumColumns::TABLE);
    if (rdbPredicates.GetOrder().empty()) {
        rdbPredicates.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
    }

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    if (resultSet != nullptr) {
        auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
        respBody.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    }

    return E_OK;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAlbumsService::GetClonedAlbumUris(GetClonedAlbumUrisDto &dto)
{
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(dto.columns);
    NativeRdb::RdbPredicates predicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, TabOldAlbumsColumn::OLD_ALBUM_TABLE);
    auto resultSet = PhotoMapOperations::QueryPhotoAssets(predicates, dto.columns);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to query album assets");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

int32_t MediaAlbumsService::GetPhotoAlbumObject(
    GetPhotoAlbumObjectDto &getPhotoAlbumObjectDto, GetPhotoAlbumObjectRespBody &respBody)
{
    std::vector<std::string> columns = getPhotoAlbumObjectDto.columns;
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(getPhotoAlbumObjectDto.predicates, PhotoAlbumColumns::TABLE);

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    if (resultSet != nullptr) {
        auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
        respBody.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    }
    return E_OK;
}

int32_t MediaAlbumsService::QueryAlbumsLpath(QueryAlbumsDto &dto)
{
    MEDIA_DEBUG_LOG("MediaAlbumsService::QueryAlbumsLpath Start");
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    std::vector<std::string> &columns = dto.columns;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(dto.predicates, PhotoAlbumColumns::TABLE);
    if (rdbPredicates.GetOrder().empty()) {
        rdbPredicates.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
    }
    resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);

    LogQueryParams(dto.predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet nullptr");
    auto bridge = RdbUtils::ToResultSetBridge(resultSet);
    dto.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    MEDIA_DEBUG_LOG("MediaAlbumsService::QueryAlbumsLpath End");
    return E_OK;
}

int32_t MediaAlbumsService::QueryAlbumsLpaths(QueryAlbumsDto &dto)
{
    MEDIA_DEBUG_LOG("MediaAlbumsService::QueryAlbumsLpaths Start");
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    std::vector<std::string> &columns = dto.columns;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    if (dto.albumType == PhotoAlbumType::USER || dto.albumType == PhotoAlbumType::SOURCE) {
        dto.predicates.And()
            ->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(dto.albumType))
            ->And()
            ->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, std::to_string(dto.albumSubType));
        NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(dto.predicates, PhotoAlbumColumns::TABLE);
        if (rdbPredicates.GetOrder().empty()) {
            rdbPredicates.OrderByAsc(PhotoAlbumColumns::ALBUM_ORDER);
        }
        resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    }

    LogQueryParams(dto.predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet nullptr");
    auto bridge = RdbUtils::ToResultSetBridge(resultSet);
    dto.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    MEDIA_DEBUG_LOG("MediaAlbumsService::QueryAlbumsLpaths End");
    return E_OK;
}

int32_t MediaAlbumsService::ChangeRequestSetHighlightAttribute(ChangeRequestSetHighlightAttributeDto &dto)
{
    MEDIA_DEBUG_LOG("MediaAlbumsService::ChangeRequestSetHighlightAttribute Start");
    int32_t ret = MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(dto.albumId,
        dto.highlightAlbumChangeAttribute, dto.highlightAlbumChangeAttributeValue);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
        "ChangeRequestSetHighlightAttribute failed, error id: %{public}d", ret);
    return E_OK;
}

int32_t MediaAlbumsService::ChangeRequestSetUploadStatus(const ChangeRequestSetUploadStatusDto &setUploadStatusDto)
{
    vector<string> albumIds;
    for (size_t i = 0; i < setUploadStatusDto.albumIds.size(); i++) {
        PhotoAlbumType photoAlbumType = static_cast<PhotoAlbumType>(setUploadStatusDto.photoAlbumTypes[i]);
        PhotoAlbumSubType photoAlbumSubType = static_cast<PhotoAlbumSubType>(setUploadStatusDto.photoAlbumSubtypes[i]);
        if (PhotoAlbum::IsUserPhotoAlbum(photoAlbumType, photoAlbumSubType) ||
            PhotoAlbum::IsSourceAlbum(photoAlbumType, photoAlbumSubType)) {
            albumIds.emplace_back(setUploadStatusDto.albumIds[i]);
        }
    }
    if (albumIds.size() == 0) {
        MEDIA_INFO_LOG("No userPhotoAlbum and IsourceAlbum");
        return E_OK;
    }
    int32_t changedRows = this->rdbOperation_.SetUploadStatus(albumIds, setUploadStatusDto.allowUpload);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, E_HAS_DB_ERROR,
        "setUploadStatus failed, changedRows is %{private}d", changedRows);
    return changedRows;
}

int32_t MediaAlbumsService::GetAlbumIdByLpathOrBundleName(GetAlbumIdByLpathDto &getAlbumIdByLpathDto,
    GetAlbumIdByLpathRespBody &respBody)
{
    std::vector<std::string> columns = getAlbumIdByLpathDto.columns;
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(getAlbumIdByLpathDto.predicates, PhotoAlbumColumns::TABLE);
    auto rdbResultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);
    if (rdbResultSet == nullptr) {
        MEDIA_ERR_LOG("GetAlbumIdByLpath Service rdbresultSet nullptr");
        return E_ERR;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(rdbResultSet);
    auto resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    if (resultSet == nullptr) {
        MEDIA_DEBUG_LOG("GetAlbumIdByLpath Service resultSet nullptr");
        return E_INNER_FAIL;
    }
    int albumId = -1;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int32_t columnIndex;
        if (resultSet->GetColumnIndex("album_id", columnIndex) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("GetColumnIndex  album_id failed");
            resultSet->Close();
            return E_INNER_FAIL;
        }
        if (resultSet->GetInt(columnIndex, albumId) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("GetInt for albumId failed");
            resultSet->Close();
            return E_INNER_FAIL;
        }
    } else {
        MEDIA_INFO_LOG("album_id column not found");
        albumId = -1;
    }
    resultSet->Close();
    respBody.albumId = albumId;
    return E_OK;
}

int32_t MediaAlbumsService::CreateAnalysisAlbum(CreateAnalysisAlbumDto &dto, CreateAnalysisAlbumRespBody &respBody)
{
    std::string albumName = dto.albumName;
    auto rowId = MediaLibraryAlbumOperations::CreatePortraitAlbum(albumName);
    respBody.albumId = rowId;
    return E_OK;
}
} // namespace OHOS::Media