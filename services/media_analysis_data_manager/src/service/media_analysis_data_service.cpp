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
 
#define MLOG_TAG "Analysis_Data_Service"
 
#include "media_analysis_data_service.h"
 
#include "media_log.h"
#include "medialibrary_errno.h"
#include "rdb_utils.h"
#include "medialibrary_vision_operations.h"
#include "medialibrary_search_operations.h"
#include "location_column.h"
#include "vision_column.h"
#include "vision_aesthetics_score_column.h"
#include "vision_composition_column.h"
#include "vision_face_tag_column.h"
#include "vision_head_column.h"
#include "vision_image_face_column.h"
#include "vision_label_column.h"
#include "vision_object_column.h"
#include "vision_ocr_column.h"
#include "vision_pet_face_column.h"
#include "vision_pose_column.h"
#include "vision_recommendation_column.h"
#include "vision_saliency_detect_column.h"
#include "vision_segmentation_column.h"
#include "vision_total_column.h"
#include "vision_video_label_column.h"
#include "vision_multi_crop_column.h"
#include "vision_column_comm.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "vision_video_face_column.h"
#include "media_column.h"
#include "get_asset_analysis_data_dto.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "vision_photo_map_column.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_client_errno.h"
#include "result_set_utils.h"
#include "media_analysis_progress_column.h"
#include "user_photography_info_column.h"
#include "medialibrary_notify.h"
#include "photo_map_operations.h"

 
namespace OHOS::Media::AnalysisData {
using namespace std;
MediaAnalysisDataService &MediaAnalysisDataService::GetInstance()
{
    static MediaAnalysisDataService service;
    return service;
}
 
struct AnalysisConfig {
    std::string tableName;
    std::string totalColumnName;
    std::vector<std::string> columns;
};

struct HighlightAlbumInfo {
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};
 
static const map<int32_t, struct AnalysisConfig> ANALYSIS_CONFIG_MAP = {
    { ANALYSIS_AESTHETICS_SCORE, { VISION_AESTHETICS_TABLE, AESTHETICS_SCORE, { AESTHETICS_SCORE, PROB } } },
    { ANALYSIS_LABEL, { VISION_LABEL_TABLE, LABEL, { CATEGORY_ID, SUB_LABEL, PROB, FEATURE,
        SIM_RESULT, SALIENCY_SUB_PROB } } },
    { ANALYSIS_VIDEO_LABEL, { VISION_VIDEO_LABEL_TABLE, VIDEO_LABEL, { CATEGORY_ID, CONFIDENCE_PROBABILITY,
        SUB_CATEGORY, SUB_CONFIDENCE_PROB, SUB_LABEL, SUB_LABEL_PROB, SUB_LABEL_TYPE,
        TRACKS, VIDEO_PART_FEATURE, FILTER_TAG} } },
    { ANALYSIS_OCR, { VISION_OCR_TABLE, OCR, { OCR_TEXT, OCR_TEXT_MSG, OCR_WIDTH, OCR_HEIGHT } } },
    { ANALYSIS_FACE, { VISION_IMAGE_FACE_TABLE, FACE, { FACE_ID, TAG_ID, SCALE_X, SCALE_Y,
        SCALE_WIDTH, SCALE_HEIGHT, LANDMARKS, PITCH, YAW, ROLL, PROB, TOTAL_FACES, FEATURES, FACE_OCCLUSION,
        BEAUTY_BOUNDER_X, BEAUTY_BOUNDER_Y, BEAUTY_BOUNDER_WIDTH, BEAUTY_BOUNDER_HEIGHT, FACE_AESTHETICS_SCORE,
        JOINT_BEAUTY_BOUNDER_X, JOINT_BEAUTY_BOUNDER_Y, JOINT_BEAUTY_BOUNDER_WIDTH, JOINT_BEAUTY_BOUNDER_HEIGHT } } },
    { ANALYSIS_OBJECT, { VISION_OBJECT_TABLE, OBJECT, { OBJECT_ID, OBJECT_LABEL, OBJECT_SCALE_X, OBJECT_SCALE_Y,
        OBJECT_SCALE_WIDTH, OBJECT_SCALE_HEIGHT, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
    { ANALYSIS_RECOMMENDATION, { VISION_RECOMMENDATION_TABLE, RECOMMENDATION, { RECOMMENDATION_ID,
        RECOMMENDATION_RESOLUTION, RECOMMENDATION_SCALE_X, RECOMMENDATION_SCALE_Y, RECOMMENDATION_SCALE_WIDTH,
        RECOMMENDATION_SCALE_HEIGHT, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
    { ANALYSIS_SEGMENTATION, { VISION_SEGMENTATION_TABLE, SEGMENTATION, { SEGMENTATION_AREA, SEGMENTATION_NAME,
        PROB } } },
    { ANALYSIS_COMPOSITION, { VISION_COMPOSITION_TABLE, COMPOSITION, { COMPOSITION_ID, COMPOSITION_RESOLUTION,
        CLOCK_STYLE, CLOCK_LOCATION_X, CLOCK_LOCATION_Y, CLOCK_COLOUR, COMPOSITION_SCALE_X, COMPOSITION_SCALE_Y,
        COMPOSITION_SCALE_WIDTH, COMPOSITION_SCALE_HEIGHT, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
    { ANALYSIS_SALIENCY, { VISION_SALIENCY_TABLE, SALIENCY, { SALIENCY_X, SALIENCY_Y } } },
    { ANALYSIS_DETAIL_ADDRESS, { PhotoColumn::PHOTOS_TABLE, DETAIL_ADDRESS, {
        PhotoColumn::PHOTOS_TABLE + "." + LATITUDE, PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, LANGUAGE, COUNTRY,
        ADMIN_AREA, SUB_ADMIN_AREA, LOCALITY, SUB_LOCALITY, THOROUGHFARE, SUB_THOROUGHFARE, FEATURE_NAME, CITY_NAME,
        ADDRESS_DESCRIPTION, LOCATION_TYPE, AOI, POI, FIRST_AOI, FIRST_POI, LOCATION_VERSION, FIRST_AOI_CATEGORY,
        FIRST_POI_CATEGORY} } },
    { ANALYSIS_HUMAN_FACE_TAG, { VISION_FACE_TAG_TABLE, FACE_TAG, { VISION_FACE_TAG_TABLE + "." + TAG_ID, TAG_NAME,
        USER_OPERATION, GROUP_TAG, RENAME_OPERATION, CENTER_FEATURES, USER_DISPLAY_LEVEL, TAG_ORDER, IS_ME, COVER_URI,
        COUNT, PORTRAIT_DATE_MODIFY, ALBUM_TYPE, IS_REMOVED } } },
    { ANALYSIS_HEAD_POSITION, { VISION_HEAD_TABLE, HEAD, { HEAD_ID, HEAD_LABEL, HEAD_SCALE_X, HEAD_SCALE_Y,
        HEAD_SCALE_WIDTH, HEAD_SCALE_HEIGHT, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
    { ANALYSIS_BONE_POSE, { VISION_POSE_TABLE, POSE, { POSE_ID, POSE_LANDMARKS, POSE_SCALE_X, POSE_SCALE_Y,
        POSE_SCALE_WIDTH, POSE_SCALE_HEIGHT, PROB, POSE_TYPE, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
    { ANALYSIS_MULTI_CROP, { VISION_RECOMMENDATION_TABLE, RECOMMENDATION, { MOVEMENT_CROP, MOVEMENT_VERSION } } },
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
 
int32_t MediaAnalysisDataService::GetAssetAnalysisData(GetAssetAnalysisDataDto &dto)
{
    MEDIA_INFO_LOG("fileId:%{public}d, analysisType:%{public}d, language:%{public}s, analysisTotal:%{public}d",
        dto.fileId, dto.analysisType, dto.language.c_str(), dto.analysisTotal);
    auto it = ANALYSIS_CONFIG_MAP.find(dto.analysisType);
    if (it == ANALYSIS_CONFIG_MAP.end()) {
        MEDIA_ERR_LOG("Invalid analysisType:%{public}d", dto.analysisType);
        return -EINVAL;
    }
 
    const AnalysisConfig &config = it->second;
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
    if (dto.analysisTotal) {
        NativeRdb::RdbPredicates predicate(VISION_TOTAL_TABLE);
        predicate.EqualTo(MediaColumn::MEDIA_ID, to_string(dto.fileId));
        resultSet = MediaLibraryRdbStore::QueryWithFilter(predicate, { config.totalColumnName });
    } else {
        NativeRdb::RdbPredicates rdbPredicate(config.tableName);
        if (dto.analysisType == ANALYSIS_FACE) {
            rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, to_string(dto.fileId));
            resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, config.columns);
        } else if (dto.analysisType == ANALYSIS_HUMAN_FACE_TAG) {
            string onClause = VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " = " + VISION_FACE_TAG_TABLE + "." + TAG_ID;
            rdbPredicate.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });
            rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, to_string(dto.fileId));
            resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, config.columns);
        } else if (dto.analysisType == ANALYSIS_DETAIL_ADDRESS) {
            string onClause = GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = \'" + dto.language + "\' AND " +
                PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID + " = " + GEO_KNOWLEDGE_TABLE + "." + FILE_ID;
            rdbPredicate.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On({ onClause });
            rdbPredicate.EqualTo(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID, to_string(dto.fileId));
            resultSet = MediaLibraryDataManager::QueryGeo(rdbPredicate, config.columns);
        } else {
            rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, to_string(dto.fileId));
            resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, config.columns);
        }
    }
 
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet nullptr");
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    dto.resultSet = make_shared<DataShare::DataShareResultSet>(bridge);
    return E_OK;
}
 
int32_t MediaAnalysisDataService::GetIndexConstructProgress(std::string &indexProgress)
{
    auto resultSet = MediaLibrarySearchOperations::QueryIndexConstructProgress();
    CHECK_AND_RETURN_RET_LOG(resultSet, E_FAIL, "Failed to query index construct progress");
 
    auto errCode = resultSet->GoToFirstRow();
    if (errCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ResultSet GotoFirstRow failed, errCode=%{public}d", errCode);
        return E_FAIL;
    }
 
    const vector<string> columns = {PHOTO_COMPLETE_NUM, PHOTO_TOTAL_NUM, VIDEO_COMPLETE_NUM, VIDEO_TOTAL_NUM};
    int32_t index = 0;
    string value = "";
    indexProgress = "{";
    for (const auto &item : columns) {
        if (resultSet->GetColumnIndex(item, index) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("ResultSet GetColumnIndex failed, progressObject=%{public}s", item.c_str());
            return E_FAIL;
        }
        if (resultSet->GetString(index, value) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("ResultSet GetString failed, progressObject=%{public}s", item.c_str());
            return E_FAIL;
        }
        indexProgress += "\"" + item + "\":" + value + ",";
    }
    indexProgress = indexProgress.substr(0, indexProgress.length() - 1);
    indexProgress += "}";
    MEDIA_DEBUG_LOG("GetProgressStr progress=%{public}s", indexProgress.c_str());
    return E_OK;
}
 
int32_t MediaAnalysisDataService::StartAssetAnalysis(const StartAssetAnalysisDto &dto,
    StartAssetAnalysisRespBody &respBody)
{
    Uri uri(dto.uri);
    MediaLibraryCommand cmd(uri);
    cmd.SetDataSharePred(dto.predicates);
    auto resultSet = MediaLibraryVisionOperations::HandleForegroundAnalysisOperation(cmd);
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAnalysisDataService::SetOrderPosition(ChangeRequestSetOrderPositionDto &setOrderPositionDto)
{
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_MAP, OperationType::UPDATE_ORDER, MediaLibraryApi::API_10);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ORDER_POSITION, setOrderPositionDto.orderString);
    valuesBucket.Put(ALBUM_ID, setOrderPositionDto.albumId);
    NativeRdb::ValuesBucket value = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    if (value.IsEmpty()) {
        MEDIA_ERR_LOG("SetOrderPosition:Input parameter is invalid ");
        return E_INVALID_VALUES;
    }

    const string mapTable = ANALYSIS_PHOTO_MAP_TABLE;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(mapTable + "." + MAP_ALBUM, setOrderPositionDto.albumId)
        ->And()
        ->In(mapTable + "." + MAP_ASSET, setOrderPositionDto.assetIds);
    NativeRdb::RdbPredicates rdbPredicate =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, ANALYSIS_PHOTO_MAP_TABLE);
    cmd.SetValueBucket(value);
    cmd.SetDataSharePred(predicates);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(cmd);
}

int32_t MediaAnalysisDataService::GetOrderPosition(const GetOrderPositionDto& getOrderPositionDto,
    GetOrderPositionRespBody& resp)
{
    NativeRdb::RdbPredicates predicates(ANALYSIS_PHOTO_MAP_TABLE);
    const string mapTable = ANALYSIS_PHOTO_MAP_TABLE;
    predicates.EqualTo(mapTable + "." + MAP_ALBUM, getOrderPositionDto.albumId)->
        And()->In(mapTable + "." + MAP_ASSET, getOrderPositionDto.assetIdArray);
    std::vector<std::string> fetchColumn{MAP_ASSET, ORDER_POSITION};

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, fetchColumn);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    int count = 0;
    int ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK || count <= 0) {
        MEDIA_ERR_LOG("GetRowCount failed, error code: %{public}d, count: %{public}d", ret, count);
        return JS_INNER_FAIL;
    }
    unordered_map<std::string, int32_t> idOrderMap;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t mapAsset = get<int32_t>(ResultSetUtils::GetValFromColumn(MAP_ASSET, resultSet, TYPE_INT32));
        int32_t orderPosition = get<int32_t>(ResultSetUtils::GetValFromColumn(ORDER_POSITION, resultSet, TYPE_INT32));
        idOrderMap[std::to_string(mapAsset)] = orderPosition;
    }
    resp.orderPositionArray.clear();
    for (const string& assetId : getOrderPositionDto.assetIdArray) {
        resp.orderPositionArray.push_back(idOrderMap[assetId]);
    }
    return E_OK;
}

int32_t MediaAnalysisDataService::SetPortraitRelationship(const int32_t albumId, const string& relationship,
    const int32_t isMe)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    values.Put(ALBUM_RELATIONSHIP, relationship);
    return MediaLibraryAlbumOperations::SetPortraitAlbumRelationship(values, predicates, isMe);
}

int32_t MediaAnalysisDataService::GetPortraitRelationship(const int32_t albumId, GetRelationshipRespBody& resp)
{
    MEDIA_INFO_LOG("GetPortraitRelationship start");
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    std::vector<std::string> fetchColumn{ALBUM_RELATIONSHIP};

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, fetchColumn);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    int count = 0;
    int ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK || count <= 0) {
        MEDIA_ERR_LOG("GetRowCount failed, error code: %{public}d, count: %{public}d", ret, count);
        return JS_INNER_FAIL;
    }
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resp.relationship = get<std::string>(ResultSetUtils::GetValFromColumn(
            ALBUM_RELATIONSHIP, resultSet, TYPE_STRING));
    } else {
        MEDIA_ERR_LOG("query resultSet fail");
        return E_ERR;
    }
    resultSet->Close();
    return E_OK;
}

int32_t MediaAnalysisDataService::GetAnalysisProcess(GetAnalysisProcessReqBody &reqBody,
    QueryResultRespBody &respBody)
{
    std::string tableName;
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> columns;
    if (reqBody.analysisType == static_cast<int32_t>(AnalysisType::ANALYSIS_INVALID)) {
        tableName = TAB_ANALYSIS_PROGRESS_TABLE;
        columns = { SEARCH_FINISH_CNT, LOCATION_FINISH_CNT, FACE_FINISH_CNT, OBJECT_FINISH_CNT, AESTHETIC_FINISH_CNT,
            OCR_FINISH_CNT, POSE_FINISH_CNT, SALIENCY_FINISH_CNT, RECOMMENDATION_FINISH_CNT, SEGMENTATION_FINISH_CNT,
            BEAUTY_AESTHETIC_FINISH_CNT, HEAD_DETECT_FINISH_CNT, LABEL_DETECT_FINISH_CNT, TOTAL_IMAGE_CNT,
            FULLY_ANALYZED_IMAGE_CNT, TOTAL_PROGRESS, CHECK_SPACE_FLAG };
    } else if (reqBody.analysisType == static_cast<int32_t>(AnalysisType::ANALYSIS_LABEL)) {
        tableName = VISION_TOTAL_TABLE;
        columns = {
            "COUNT(*) AS totalCount",
            "SUM(CASE WHEN ((aesthetics_score != 0 AND label != 0 AND ocr != 0 AND face != 0 AND face != 1 "
                "AND face != 2 "
                "AND saliency != 0 AND segmentation != 0 AND head != 0 AND Photos.media_type = 1) OR "
                "(label != 0 AND face != 0 AND Photos.media_type = 2)) THEN 1 ELSE 0 END) AS finishedCount",
            "SUM(CASE WHEN label != 0 THEN 1 ELSE 0 END) AS LabelCount"
        };
        string clause = VISION_TOTAL_TABLE + "." + MediaColumn::MEDIA_ID + " = " + PhotoColumn::PHOTOS_TABLE+ "." +
            MediaColumn::MEDIA_ID;
        predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On({ clause });
        predicates.EqualTo(PhotoColumn::PHOTO_HIDDEN_TIME, 0)->And()
            ->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, 0)->And()
            ->EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);
    } else if (reqBody.analysisType == static_cast<int32_t>(AnalysisType::ANALYSIS_FACE)) {
        tableName = USER_PHOTOGRAPHY_INFO_TABLE;
        std::vector<std::string> columns = {
            HIGHLIGHT_ANALYSIS_PROGRESS
        };
    } else if (reqBody.analysisType == static_cast<int32_t>(AnalysisType::ANALYSIS_HIGHLIGHT)) {
        tableName = HIGHLIGHT_ALBUM_TABLE;
        columns = {
            "SUM(CASE WHEN highlight_status = -3 THEN 1 ELSE 0 END) AS ClearCount",
            "SUM(CASE WHEN highlight_status = -2 THEN 1 ELSE 0 END) AS DeleteCount",
            "SUM(CASE WHEN highlight_status = -1 THEN 1 ELSE 0 END) AS NotProduceCount",
            "SUM(CASE WHEN highlight_status > 0 THEN 1 ELSE 0 END) AS ProduceCount",
            "SUM(CASE WHEN highlight_status = 1 THEN 1 ELSE 0 END) AS PushCount",
        };
    }
    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::QueryWithFilter(
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName), columns);
    if (resSet == nullptr) {
        return E_FAIL;
    }
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_SUCCESS;
}

int32_t MediaAnalysisDataService::GetFaceId(int32_t albumId, string& groupTag)
{
    return this->albumDao_.GetFaceIdByAlbumId(albumId, groupTag);
}

int32_t MediaAnalysisDataService::GetHighlightAlbumInfo(GetHighlightAlbumReqBody &reqBody,
    QueryResultRespBody &respBody)
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

int32_t MediaAnalysisDataService::SetHighlightUserActionData(const SetHighlightUserActionDataDto& dto)
{
    int32_t err = this->albumDao_.SetHighlightUserActionData(dto);
    return err;
}

int32_t MediaAnalysisDataService::SetSubtitle(const string& highlightAlbumId, const string& albumSubtitle)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, highlightAlbumId);
    values.Put(SUB_TITLE, albumSubtitle);
    return MediaLibraryAlbumOperations::SetHighlightSubtitle(values, predicates);
}

int32_t MediaAnalysisDataService::DeleteHighlightAlbums(const vector<string>& albumIds)
{
    // Only Highlight albums can be deleted by this way
    MEDIA_INFO_LOG("Delete highlight albums");
    int32_t changedRows = this->albumDao_.DeleteHighlightAlbums(albumIds);
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

int32_t MediaAnalysisDataService::ChangeRequestSetIsMe(int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return MediaLibraryAlbumOperations::SetIsMe(values, predicates);
}

int32_t MediaAnalysisDataService::ChangeRequestSetDisplayLevel(int32_t displayLevelValue,
    int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    values.Put(USER_DISPLAY_LEVEL, displayLevelValue);
    return MediaLibraryAlbumOperations::SetDisplayLevel(values, predicates);
}

int32_t MediaAnalysisDataService::DismissAssets(ChangeRequestDismissAssetsDto &dismissAssetsDto)
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

int32_t MediaAnalysisDataService::MergeAlbum(ChangeRequestMergeAlbumDto &mergeAlbumDto)
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

int32_t MediaAnalysisDataService::PlaceBefore(ChangeRequestPlaceBeforeDto &placeBeforeDto)
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

int32_t MediaAnalysisDataService::ChangeRequestDismiss(int32_t albumId)
{
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    return MediaLibraryAnalysisAlbumOperations::DismissGroupPhotoAlbum(values, predicates);
}
} // namespace OHOS::Media::AnalysisData