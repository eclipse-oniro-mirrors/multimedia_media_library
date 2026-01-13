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

#include <utility>

#include "clone_highlight_source.h"
#define private public
#define protected public
#include "medialibrary_unistore.h"
#undef private
#undef protected
#include "media_log.h"
#include "vision_db_sqls_more.h"
#include "vision_photo_map_column.h"
#include "story_album_column.h"
#include "story_db_sqls.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "backup_const_column.h"
#include "vision_album_column.h"
#include "media_upgrade.h"
using namespace std;

namespace OHOS {
namespace Media {
const unordered_map<string, string> TABLE_CREATE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, PhotoUpgrade::CREATE_PHOTO_TABLE },
    { ANALYSIS_ALBUM_TABLE, CREATE_ANALYSIS_ALBUM_FOR_ONCREATE },
    { ANALYSIS_PHOTO_MAP_TABLE, CREATE_ANALYSIS_ALBUM_MAP },
    { HIGHLIGHT_ALBUM_TABLE, CREATE_HIGHLIGHT_ALBUM_TABLE },
    { HIGHLIGHT_COVER_INFO_TABLE, CREATE_HIGHLIGHT_COVER_INFO_TABLE },
    { HIGHLIGHT_PLAY_INFO_TABLE, CREATE_HIGHLIGHT_PLAY_INFO_TABLE },
    { ANALYSIS_ASSET_SD_MAP_TABLE, CREATE_ANALYSIS_ASSET_SD_MAP_TABLE },
    { ANALYSIS_ALBUM_ASSET_MAP_TABLE, CREATE_ANALYSIS_ALBUM_ASET_MAP_TABLE },
    { VISION_LABEL_TABLE, CREATE_TAB_ANALYSIS_LABEL },
    { VISION_RECOMMENDATION_TABLE, CREATE_TAB_ANALYSIS_RECOMMENDATION },
    { VISION_SALIENCY_TABLE, CREATE_TAB_ANALYSIS_SALIENCY_DETECT },
    { VISION_TOTAL_TABLE, CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE },
};
const unordered_map<string, InsertType> TABLE_INSERT_TYPE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, InsertType::PHOTOS },
    { ANALYSIS_ALBUM_TABLE, InsertType::ANALYSIS_ALBUM },
    { ANALYSIS_PHOTO_MAP_TABLE, InsertType::ANALYSIS_PHOTO_MAP },
    { HIGHLIGHT_ALBUM_TABLE, InsertType::TAB_HIGHLIGHT_ALBUM },
    { HIGHLIGHT_COVER_INFO_TABLE, InsertType::TAB_HIGHLIGHT_COVER_INFO },
    { HIGHLIGHT_PLAY_INFO_TABLE, InsertType::TAB_HIGHLIGHT_PLAY_INFO },
    { ANALYSIS_ASSET_SD_MAP_TABLE, InsertType::TAB_ANALYSIS_ASSET_SD_MAP },
    { ANALYSIS_ALBUM_ASSET_MAP_TABLE, InsertType::TAB_ANALYSIS_ALBUM_ASSET_MAP },
    { VISION_LABEL_TABLE, InsertType::TAB_ANALYSIS_LABEL },
    { VISION_RECOMMENDATION_TABLE, InsertType::TAB_ANALYSIS_RECOMMENDATION },
    { VISION_SALIENCY_TABLE, InsertType::TAB_ANALYSIS_SALIENCY_DETECT },
    { VISION_TOTAL_TABLE, InsertType::TAB_ANALYSIS_TOTAL },
};
const string VALUES_BEGIN = " VALUES (";
const string VALUES_END = ") ";
const string INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_ID + ", " +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
const string INSERT_ANALYSIS_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" + ANALYSIS_COL_ALBUM_ID + ", " +
    ANALYSIS_COL_ALBUM_TYPE + ", " + ANALYSIS_COL_ALBUM_SUBTYPE + ", " + ANALYSIS_COL_ALBUM_NAME + ", " +
    ANALYSIS_COL_COUNT + ", " + ANALYSIS_COL_DATE_MODIFIED + ")";
const string INSERT_ANALYSIS_PHOTO_MAP = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE + "(" + MAP_ALBUM + ", " +
    MAP_ASSET + ", " + ORDER_POSITION + ")";
const string INSERT_HIGHLIGHT_ALBUM_TABLE = "INSERT INTO " + HIGHLIGHT_ALBUM_TABLE + "(id, " + ALBUM_ID + ", " +
    AI_ALBUM_ID + ", " + SUB_TITLE + ", " + CLUSTER_TYPE + ", " + CLUSTER_SUB_TYPE + ", " + CLUSTER_CONDITION + ", " +
    MIN_DATE_ADDED + ", " + MAX_DATE_ADDED + ", " + GENERATE_TIME + ", " + HIGHLIGHT_VERSION + ", " +
    HIGHLIGHT_STATUS + ", " + HIGHLIGHT_INSERT_PIC_COUNT + ", " + HIGHLIGHT_REMOVE_PIC_COUNT + ", " +
    HIGHLIGHT_SHARE_SCREENSHOT_COUNT + ", " + HIGHLIGHT_SHARE_COVER_COUNT + ", " + HIGHLIGHT_RENAME_COUNT + ", " +
    HIGHLIGHT_CHANGE_COVER_COUNT + ", " + HIGHLIGHT_RENDER_VIEWED_TIMES + ", " +
    HIGHLIGHT_RENDER_VIEWED_DURATION + ", " + HIGHLIGHT_ART_LAYOUT_VIEWED_TIMES + ", " +
    HIGHLIGHT_ART_LAYOUT_VIEWED_DURATION + ", " + HIGHLIGHT_MUSIC_EDIT_COUNT + ", " +
    HIGHLIGHT_FILTER_EDIT_COUNT + ") ";
const string INSERT_HIGHLIGHT_COVER_INFO_TABLE = "INSERT INTO " + HIGHLIGHT_COVER_INFO_TABLE + "(" + ALBUM_ID + ", " +
    RATIO + ", " + COVER_SERVICE_VERSION + ", " + COVER_KEY + ", " + COVER_STATUS + ")";
const string INSERT_HIGHLIGHT_PLAY_INFO_TABLE = "INSERT INTO " + HIGHLIGHT_PLAY_INFO_TABLE + "(" + ALBUM_ID + ", " +
    PLAY_INFO_ID + ", " + MUSIC + ", " + FILTER + ", " + HIGHLIGHT_PLAY_INFO + ", " + IS_CHOSEN + ", " +
    PLAY_INFO_VERSION + ", " + HIGHLIGHTING_ALGO_VERSION + ", " + CAMERA_MOVEMENT_ALGO_VERSION + ", " +
    TRANSITION_ALGO_VERSION + ", " + PLAY_SERVICE_VERSION + ", " + PLAY_INFO_STATUS + ")";
const string INSERT_ANALYSIS_ASSET_SD_MAP_TABLE = "INSERT INTO " + ANALYSIS_ASSET_SD_MAP_TABLE + "(" +
    MAP_ASSET_SOURCE + ", " + MAP_ASSET_DESTINATION + ")";
const string INSERT_ANALYSIS_ALBUM_ASSET_MAP_TABLE = "INSERT INTO " + ANALYSIS_ALBUM_ASSET_MAP_TABLE + "(" +
    HIGHLIGHT_MAP_ALBUM + ", " + HIGHLIGHT_MAP_ASSET + ")";
const string INSERT_VISION_LABEL_TABLE = "INSERT INTO " + VISION_LABEL_TABLE + "(id, " +
    FILE_ID + ", " + CATEGORY_ID + ", " + SUB_LABEL + ", " + PROB + ", " + FEATURE + ", " + SIM_RESULT + ", " +
    LABEL_VERSION + ")";
const string INSERT_VISION_RECOMMENDATION_TABLE = "INSERT INTO " + VISION_RECOMMENDATION_TABLE + "(id, " +
    FILE_ID + ", " + RECOMMENDATION_ID + ", " + RECOMMENDATION_RESOLUTION + ", " + RECOMMENDATION_SCALE_X + ", " +
    RECOMMENDATION_SCALE_Y + ", " + RECOMMENDATION_SCALE_WIDTH + ", " + RECOMMENDATION_SCALE_HEIGHT + ", " +
    RECOMMENDATION_VERSION + ", " + SCALE_X + ", " + SCALE_Y + ", " + SCALE_WIDTH + ", " + SCALE_HEIGHT + ", " +
    ANALYSIS_VERSION + ")";
const string INSERT_VISION_SALIENCY_TABLE = "INSERT INTO " + VISION_SALIENCY_TABLE + "(id, " +
    FILE_ID + ", " + SALIENCY_X + ", " + SALIENCY_Y + ", " + SALIENCY_VERSION + ")";
const string INSERT_TAB_ANALYSIS_TOTAL = "INSERT INTO " + VISION_TOTAL_TABLE + "(" +
    ID + "," + FILE_ID + "," + STATUS + "," + OCR + "," + LABEL + "," + AESTHETICS_SCORE + "," +
    FACE + "," + OBJECT + "," + RECOMMENDATION + "," + SEGMENTATION + "," + COMPOSITION + "," + SALIENCY + "," +
    HEAD + "," + POSE + "," + GEO + ")";

int32_t CloneHighlightOpenCall::OnCreate(NativeRdb::RdbStore &store)
{
    for (const auto &createSql : createSqls_) {
        int32_t errCode = store.ExecuteSql(createSql);
        if (errCode != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("Execute %{public}s failed: %{public}d", createSql.c_str(), errCode);
            return errCode;
        }
    }
    return NativeRdb::E_OK;
}

int32_t CloneHighlightOpenCall::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

void CloneHighlightOpenCall::Init(const vector<string> &tableList)
{
    for (const auto &tableName : tableList) {
        if (TABLE_CREATE_MAP.count(tableName) == 0) {
            MEDIA_INFO_LOG("Find value failed: %{public}s, skip", tableName.c_str());
            continue;
        }
        string createSql = TABLE_CREATE_MAP.at(tableName);
        createSqls_.push_back(createSql);
    }
}

void CloneHighlightSource::Init(const string &dbPath, const vector<string> &tableList)
{
    NativeRdb::RdbStoreConfig config(dbPath);
    CloneHighlightOpenCall helper;
    helper.Init(tableList);
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    this->cloneStorePtr_ = store;
    Insert(tableList, this->cloneStorePtr_);
}

void CloneHighlightSource::Insert(const vector<string> &tableList, std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    for (const auto &tableName : tableList) {
        if (TABLE_INSERT_TYPE_MAP.count(tableName) == 0) {
            MEDIA_INFO_LOG("Find value failed: %{public}s, skip", tableName.c_str());
            continue;
        }
        InsertType insertType = TABLE_INSERT_TYPE_MAP.at(tableName);
        InsertByType(insertType, rdbPtr);
    }
}

void CloneHighlightSource::InsertByType(InsertType insertType, std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    switch (insertType) {
        case InsertType::PHOTOS: {
            InsertPhoto(rdbPtr);
            break;
        }
        case InsertType::ANALYSIS_ALBUM: {
            InsertAnalysisAlbum(rdbPtr);
            break;
        }
        case InsertType::ANALYSIS_PHOTO_MAP: {
            InsertAnalysisPhotoMap(rdbPtr);
            break;
        }
        case InsertType::TAB_HIGHLIGHT_ALBUM: {
            InsertHighlightAlbum(rdbPtr);
            break;
        }
        case InsertType::TAB_HIGHLIGHT_COVER_INFO: {
            InsertHighlightCover(rdbPtr);
            break;
        }
        case InsertType::TAB_HIGHLIGHT_PLAY_INFO: {
            InsertHighlightPlayInfo(rdbPtr);
            break;
        }
        case InsertType::TAB_ANALYSIS_ASSET_SD_MAP: {
            InsertSDMap(rdbPtr);
            break;
        }
        case InsertType::TAB_ANALYSIS_ALBUM_ASSET_MAP: {
            InsertAlbumMap(rdbPtr);
            break;
        }
        default:
            InsertAnalysisDataByType(insertType, rdbPtr);
    }
}

void CloneHighlightSource::InsertAnalysisDataByType(InsertType insertType, std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    switch (insertType) {
        case InsertType::TAB_ANALYSIS_LABEL: {
            InsertTabAnalysisLabel(rdbPtr);
            break;
        }
        case InsertType::TAB_ANALYSIS_RECOMMENDATION: {
            InsertTabAnalysisRecommendation(rdbPtr);
            break;
        }
        case InsertType::TAB_ANALYSIS_SALIENCY_DETECT: {
            InsertTabAnalysisSaliency(rdbPtr);
            break;
        }
        case InsertType::TAB_ANALYSIS_TOTAL: {
            InsertTabAnalysisTotal(rdbPtr);
            break;
        }
        default:
            MEDIA_INFO_LOG("Invalid insert type");
    }
}

void CloneHighlightSource::InsertPhoto(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // file_id,
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    rdbPtr->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "1, " +
        "'/storage/cloud/files/Photo/16/test.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 1501924205423, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1'" + VALUES_END); // cam, pic, shootingmode = 1
    rdbPtr->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "2, " +
        "'/storage/cloud/files/Photo/1/IMG_1501924307_001.jpg', 175397, 'cam_pic_del', 'cam_pic_del.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924207184, 1501924207286, 1501924207, 0, 0, 1501924271267, 0, " +
        "1280, 960, 0, ''" + VALUES_END); // cam, pic, trashed
}

void CloneHighlightSource::InsertAnalysisAlbum(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // album_id, album_type, album_subtype, album_name
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_ALBUM + VALUES_BEGIN +
        "1, 4096, 4104, 'test_highlight_album', 2, 123456" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_ALBUM + VALUES_BEGIN +
        "2, 4096, 4105, 'test_highlight_album', 2, 123456" + VALUES_END);
}

void CloneHighlightSource::InsertAnalysisPhotoMap(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // map_album, map_asset
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 1, 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 2, 2" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "2, 1, 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "2, 2, 2" + VALUES_END);
}

void CloneHighlightSource::InsertHighlightAlbum(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // id, album_id, ai_album_id, subtitle
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_ALBUM_TABLE + VALUES_BEGIN + "1, 1, 2, '2024.05.22', 'TYPE_DBSCAN', "
        "'Old_AOI_0', '[]', 1716307200000, 1716392070000, 1738992083745, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0" +
        VALUES_END);
}

void CloneHighlightSource::InsertHighlightCover(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // insert data into Highlight Cover
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, '1_1', 0, "
        "'test_highlight_album_1_1_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, '3_2', 0, "
        "'test_highlight_album_3_2_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, '3_4', 0, "
        "'test_highlight_album_3_4_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, 'microcard', 0, "
        "'test_highlight_album_microcard_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, 'medium_card', 0, "
        "'test_highlight_album_medium_card_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, 'big_card', 0, "
        "'test_highlight_album_big_card_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, 'screen_0_ver', 0, "
        "'test_highlight_album_screen_0_ver_file://media', 1" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_COVER_INFO_TABLE + VALUES_BEGIN + "1, 'screen_0_hor', 0, "
        "'test_highlight_album_screen_0_hor_file://media', 1" + VALUES_END);
}

void CloneHighlightSource::InsertHighlightPlayInfo(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // insert data into Highlight Play Info
    rdbPtr->ExecuteSql(INSERT_HIGHLIGHT_PLAY_INFO_TABLE + VALUES_BEGIN +
        "1, 0, '/test/files/test.mp3', 8, '{}', 1, 1, 'V1.0.2', 'V1.0.3', 'V1.0.2', 1, 0" + VALUES_END);
}

void CloneHighlightSource::InsertSDMap(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_ASSET_SD_MAP_TABLE + VALUES_BEGIN + "2, 2" + VALUES_END);
}

void CloneHighlightSource::InsertAlbumMap(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_ANALYSIS_ALBUM_ASSET_MAP_TABLE + VALUES_BEGIN + "1, 2" + VALUES_END);
}

void CloneHighlightSource::InsertTabAnalysisLabel(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_VISION_LABEL_TABLE + VALUES_BEGIN + "1, 1, 0, '[12]', 0, '[]', '[]', '1.5'" +
        VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_LABEL_TABLE + VALUES_BEGIN + "2, 2, 0, '[12]', 0, '[]', '[]', '1.5'" +
        VALUES_END);
}

void CloneHighlightSource::InsertTabAnalysisRecommendation(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "1, 1, 0, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "2, 1, 1, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "3, 1, 2, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "4, 1, 3, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "5, 2, 0, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "6, 2, 1, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "7, 2, 2, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_RECOMMENDATION_TABLE + VALUES_BEGIN + "8, 2, 3, '0*0', 185, 0, 295, 640,"
        " 1.0, 0, 0, 1, 1, '1.0'" + VALUES_END);
}

void CloneHighlightSource::InsertTabAnalysisSaliency(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    rdbPtr->ExecuteSql(INSERT_VISION_SALIENCY_TABLE + VALUES_BEGIN + "1, 1, 0.5, 0.5, '1.0'" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_VISION_SALIENCY_TABLE + VALUES_BEGIN + "2, 2, 0.5, 0.5, '1.0'" + VALUES_END);
}

void CloneHighlightSource::InsertTabAnalysisTotal(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    // id, file_id, status, ocr, label, aesthetics_score,
    // face, object, recommendation, segmentation, composition, saliency,
    // head, pose, geo
    rdbPtr->ExecuteSql(INSERT_TAB_ANALYSIS_TOTAL + VALUES_BEGIN +
        "1, 1, 0, 0, 0, 0, "
        "0, 0, 1, 0, 0, 1, "
        "0, 0, 0" + VALUES_END);
    rdbPtr->ExecuteSql(INSERT_TAB_ANALYSIS_TOTAL + VALUES_BEGIN +
        "2, 2, 0, 0, 0, 0, "
        "0, 0, 1, 0, 0, 1, "
        "0, 0, 0" + VALUES_END);
}
} // namespace Media
} // namespace OHOS