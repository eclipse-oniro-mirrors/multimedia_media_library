/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "clone_source.h"
#define private public
#define protected public
#include "medialibrary_unistore.h"
#undef private
#undef protected
#include "media_column.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "vision_column.h"
#include "vision_db_sqls_more.h"
#include "media_config_info_column.h"
#include "media_audio_column.h"
#include "media_upgrade.h"

using namespace std;

namespace OHOS {
namespace Media {
const std::string SEGMENTATION_ANALYSIS_TABLE = "tab_analysis_segmentation";
const unordered_map<string, string> TABLE_CREATE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, PhotoUpgrade::CREATE_PHOTO_TABLE },
    { PhotoAlbumColumns::TABLE, PhotoAlbumColumns::CREATE_TABLE },
    { PhotoMap::TABLE, PhotoMap::CREATE_TABLE },
    { ANALYSIS_ALBUM_TABLE, CREATE_ANALYSIS_ALBUM_FOR_ONCREATE },
    { VISION_FACE_TAG_TABLE, CREATE_FACE_TAG_TBL_FOR_ONCREATE },
    { VISION_IMAGE_FACE_TABLE, CREATE_IMG_FACE_TBL_FOR_ONCREATE },
    { ANALYSIS_PHOTO_MAP_TABLE, CREATE_ANALYSIS_ALBUM_MAP },
    { AudioColumn::AUDIOS_TABLE, AudioColumn::CREATE_AUDIO_TABLE },
    { GEO_DICTIONARY_TABLE, CREATE_GEO_DICTIONARY_TABLE },
    { SEGMENTATION_ANALYSIS_TABLE, CREATE_SEGMENTATION_ANALYSIS_TABLE },
    { VISION_LABEL_TABLE, CREATE_TAB_ANALYSIS_LABEL },
    { VISION_VIDEO_LABEL_TABLE, CREATE_TAB_ANALYSIS_VIDEO_LABEL },
    { ANALYSIS_SEARCH_INDEX_TABLE, CREATE_SEARCH_INDEX_TBL },
    { GEO_KNOWLEDGE_TABLE, CREATE_GEO_KNOWLEDGE_TABLE },
    { VISION_TOTAL_TABLE, CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE },
    { ANALYSIS_BEAUTY_SCORE_TABLE, CREATE_AESTHETICS_SCORE_TBL },
    { ANALYSIS_VIDEO_FACE_TABLE, CREATE_VIDEO_FACE_TBL },
    { ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME, ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE },
    { VISION_VIDEO_TOTAL_TABLE, CREATE_TAB_ANALYSIS_VIDEO_TOTAL},
};
const unordered_map<string, InsertType> TABLE_INSERT_TYPE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, InsertType::PHOTOS },
    { PhotoAlbumColumns::TABLE, InsertType::PHOTO_ALBUM },
    { PhotoMap::TABLE, InsertType::PHOTO_MAP },
    { ANALYSIS_ALBUM_TABLE, InsertType::ANALYSIS_ALBUM },
    { VISION_FACE_TAG_TABLE, InsertType::FACE_TAG_TBL },
    { VISION_IMAGE_FACE_TABLE, InsertType::IMG_FACE_TBL },
    { ANALYSIS_PHOTO_MAP_TABLE, InsertType::ANALYSIS_PHOTO_MAP },
    { AudioColumn::AUDIOS_TABLE, InsertType::AUDIOS },
    { GEO_DICTIONARY_TABLE, InsertType::ANALYSIS_GEO_DICTIONARY },
    { SEGMENTATION_ANALYSIS_TABLE, InsertType::ANALYSIS_SEGMENTATION },
    { VISION_LABEL_TABLE, InsertType::TAB_ANALYSIS_LABEL },
    { VISION_VIDEO_LABEL_TABLE, InsertType::TAB_ANALYSIS_VIDEO_LABEL },
    { GEO_KNOWLEDGE_TABLE, InsertType::TAB_ANALYSIS_GEO_KNOWLEDGE },
    { VISION_TOTAL_TABLE, InsertType::TAB_ANALYSIS_TOTAL },
    { ANALYSIS_SEARCH_INDEX_TABLE, InsertType::ANALYSIS_SEARCH_INDEX },
    { ANALYSIS_BEAUTY_SCORE_TABLE, InsertType::BEAUTY_SCORE_TBL },
    { ANALYSIS_VIDEO_FACE_TABLE, InsertType::VIDEO_FACE_TBL },
    { ANALYSIS_VIDEO_TOTAL_TABLE, InsertType::TAB_ANALYSIS_VIDEO_TOTAL},
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
const string INSERT_PHOTO_ALBUM = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" + PhotoAlbumColumns::ALBUM_ID + ", " +
    PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
    PhotoAlbumColumns::ALBUM_NAME + ", " + PhotoAlbumColumns::ALBUM_DATE_MODIFIED + ", " +
    PhotoAlbumColumns::ALBUM_BUNDLE_NAME + ")";
const string INSERT_PHOTO_MAP = "INSERT INTO " + PhotoMap::TABLE + "(" + PhotoMap::ALBUM_ID + ", " +
    PhotoMap::ASSET_ID + ")";
const string INSERT_ANALYSIS_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE + "(" + PhotoAlbumColumns::ALBUM_ID + ", " +
    PhotoAlbumColumns::ALBUM_TYPE + ", " + PhotoAlbumColumns::ALBUM_SUBTYPE + ", " +
    PhotoAlbumColumns::ALBUM_NAME + ") ";
const string INSERT_ANALYSIS_PHOTO_MAP = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE + "(" + PhotoMap::ALBUM_ID + ", " +
    PhotoMap::ASSET_ID + ")";
const string INSERT_AUDIO = "INSERT INTO " + AudioColumn::AUDIOS_TABLE + "(" + AudioColumn::MEDIA_ID + ", " +
    AudioColumn::MEDIA_FILE_PATH + ", " + AudioColumn::MEDIA_SIZE + ", " + AudioColumn::MEDIA_TITLE + ", " +
    AudioColumn::MEDIA_NAME + ", " + AudioColumn::MEDIA_TYPE + ", " + AudioColumn::MEDIA_DATE_ADDED + ", "  +
    AudioColumn::MEDIA_DATE_MODIFIED + ", " + AudioColumn::MEDIA_DATE_TAKEN + ", " +
    AudioColumn::MEDIA_DURATION + ", " + AudioColumn::MEDIA_IS_FAV + ", " + AudioColumn::MEDIA_DATE_TRASHED + ", " +
    AudioColumn::AUDIO_ARTIST + ")";
const string INSERT_ANALYSIS_GEO_DICTIONARY = "INSERT INTO " + GEO_DICTIONARY_TABLE + "(" +
    CITY_ID + ", " + LANGUAGE + ", " + CITY_NAME + ")";
const string INSERT_TAB_ANALYSIS_LABEL = "INSERT INTO " + VISION_LABEL_TABLE + "(" +
    ID + ", " + FILE_ID + ", " + CATEGORY_ID + ", " + SUB_LABEL + ", " + PROB + ", " + FEATURE + ", " + SIM_RESULT +
    ", " + LABEL_VERSION + ", " + SALIENCY_SUB_PROB + ", " + ANALYSIS_VERSION + ")";
const string INSERT_TAB_ANALYSIS_VIDEO_LABEL = "INSERT INTO " + VISION_VIDEO_LABEL_TABLE + "(" +
    ID + ", " + FILE_ID + ", " + CATEGORY_ID + ", " + CONFIDENCE_PROBABILITY + ", " + SUB_CATEGORY + ", "
    + SUB_CONFIDENCE_PROB + ", " + SUB_LABEL + ", " + SUB_LABEL_PROB + ", " + SUB_LABEL_TYPE + ", " + TRACKS +
    ", " + VIDEO_PART_FEATURE + ", " + FILTER_TAG + ", " + ALGO_VERSION + ", " + ANALYSIS_VERSION + ", "
    + TRIGGER_GENERATE_THUMBNAIL + ")";
const string INSERT_TAB_ANALYSIS_GEO_KNOWLEDGE = "INSERT INTO " + GEO_KNOWLEDGE_TABLE + "(" +
    FILE_ID + "," + LATITUDE + "," + LONGITUDE + "," + LOCATION_KEY + "," + CITY_ID + "," + LANGUAGE + "," +
    COUNTRY + "," + ADMIN_AREA + "," + SUB_ADMIN_AREA + "," + LOCALITY + "," + SUB_LOCALITY + "," +
    THOROUGHFARE + "," + SUB_THOROUGHFARE + "," + FEATURE_NAME + "," + CITY_NAME + "," + ADDRESS_DESCRIPTION + "," +
    AOI + "," + POI + "," + FIRST_AOI + "," + FIRST_POI + "," + LOCATION_VERSION + "," +
    FIRST_AOI_CATEGORY + "," + FIRST_POI_CATEGORY + "," + LOCATION_TYPE + ")";
const string INSERT_TAB_ANALYSIS_TOTAL = "INSERT INTO " + VISION_TOTAL_TABLE + "(" +
    ID + "," + FILE_ID + "," + STATUS + "," + OCR + "," + LABEL + "," + AESTHETICS_SCORE + "," +
    FACE + "," + OBJECT + "," + RECOMMENDATION + "," + SEGMENTATION + "," + COMPOSITION + "," + SALIENCY + "," +
    HEAD + "," + POSE + "," + GEO + ")";

int32_t CloneOpenCall::OnCreate(NativeRdb::RdbStore &store)
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

int32_t CloneOpenCall::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}

void CloneOpenCall::Init(const vector<string> &tableList)
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

void CloneSource::Init(const string &dbPath, const vector<string> &tableList)
{
    NativeRdb::RdbStoreConfig config(dbPath);
    CloneOpenCall helper;
    helper.Init(tableList);
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    cloneStorePtr_ = store;
    Insert(tableList);
}

void CloneSource::Insert(const vector<string> &tableList)
{
    for (const auto &tableName : tableList) {
        if (TABLE_INSERT_TYPE_MAP.count(tableName) == 0) {
            MEDIA_INFO_LOG("Find value failed: %{public}s, skip", tableName.c_str());
            continue;
        }
        InsertType insertType = TABLE_INSERT_TYPE_MAP.at(tableName);
        InsertByTypeOne(insertType);
        InsertByTypeTwo(insertType);
    }
}

void CloneSource::InsertByTypeOne(InsertType insertType)
{
    switch (insertType) {
        case InsertType::PHOTOS: {
            InsertPhoto();
            break;
        }
        case InsertType::PHOTO_ALBUM: {
            InsertPhotoAlbum();
            break;
        }
        case InsertType::PHOTO_MAP: {
            InsertPhotoMap();
            break;
        }
        case InsertType::ANALYSIS_ALBUM: {
            InsertAnalysisAlbum();
            break;
        }
        case InsertType::FACE_TAG_TBL: {
            InsertFaceTag();
            break;
        }
        case InsertType::IMG_FACE_TBL: {
            InsertImgFaceTbl();
            break;
        }
        case InsertType::ANALYSIS_PHOTO_MAP: {
            InsertAnalysisPhotoMap();
            break;
        }
        case InsertType::AUDIOS: {
            InsertAudio();
            break;
        }
        case InsertType::ANALYSIS_GEO_DICTIONARY: {
            InsertAnalysisGeoDictionary();
            break;
        }
        case InsertType::TAB_ANALYSIS_LABEL: {
            InsertTabAnalysisLabel();
            break;
        }
        case InsertType::TAB_ANALYSIS_VIDEO_LABEL: {
            InsertTabAnalysisVideoLabel();
            break;
        }
        default:
            MEDIA_INFO_LOG("Invalid insert type: %{public}d", static_cast<int32_t>(insertType));
    }
}

void CloneSource::InsertByTypeTwo(InsertType insertType)
{
    switch (insertType) {
        case InsertType::TAB_ANALYSIS_GEO_KNOWLEDGE: {
            InsertTabAnalysisGeoKnowledge();
            break;
        }
        case InsertType::TAB_ANALYSIS_TOTAL: {
            InsertTabAnalysisTotal();
            break;
        }
        case InsertType::BEAUTY_SCORE_TBL: {
            InsertTabBeautyScore();
            break;
        }
        case InsertType::VIDEO_FACE_TBL: {
            InsertTabVideoFace();
            break;
        }
        case InsertType::ANALYSIS_SEGMENTATION: {
            InsertAnalysisSegmentation();
            break;
        }
        default:
            MEDIA_INFO_LOG("Invalid insert type: %{public}d", static_cast<int32_t>(insertType));
    }
}

void CloneSource::InsertPhoto()
{
    // file_id,
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "1, " +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 1501924205423, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1'" + VALUES_END); // cam, pic, shootingmode = 1
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "2, " +
        "'/storage/cloud/files/Photo/1/IMG_1501924307_001.jpg', 175397, 'cam_pic_del', 'cam_pic_del.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924207184, 1501924207286, 1501924207, 0, 0, 1501924271267, 0, " +
        "1280, 960, 0, ''" + VALUES_END); // cam, pic, trashed
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "3, " +
        "'/storage/cloud/files/Photo/16/VID_1501924310_000.mp4', 167055, 'cam_vid_fav', 'cam_vid_fav.mp4', 2, " +
        "'com.ohos.camera', '相机', 1501924210677, 1501924216550, 1501924210, 5450, 1, 0, 0, " +
        "480, 640, 0, ''" + VALUES_END); // cam, vid, favorite
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "4, " +
        "'/storage/cloud/files/Photo/2/IMG_1501924331_002.jpg', 505571, 'scr_pic_hid', 'scr_pic_hid.jpg', 1, " +
        "'com.ohos.screenshot', '截图', 1501924231249, 1501924231286, 1501924231, 0, 0, 0, 1, " +
        "1280, 720, 0, ''" + VALUES_END); // screenshot, pic, hidden
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "5, " +
        "'/storage/cloud/files/Photo/4/IMG_1501924357_004.jpg', 85975, 'scr_pic_edit', 'scr_pic_edit.jpg', 1, " +
        "'com.ohos.screenshot', '截图', 1501924257174, 1501924257583, 1501924257, 0, 0, 0, 0, " +
        "592, 720, 1501935124, ''" + VALUES_END); // screenshot, pic, edit
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "6, " +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_005.jpg', 0, 'size_0', 'size_0.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 1501924205423, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, ''" + VALUES_END); // cam, pic, size = 0
}

void CloneSource::InsertPhotoAlbum()
{
    // album_id, album_type, album_subtype, album_name, date_modified, bundle_name
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_ALBUM + VALUES_BEGIN + "8, 2048, 2049, '相机', 0, 'com.ohos.camera'" +
        VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_ALBUM + VALUES_BEGIN + "9, 2048, 2049, '截图', 0, 'com.ohos.screenshot'" +
        VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_ALBUM + VALUES_BEGIN + "11, 0, 1, '新建相册1', 1711540817842, NULL" +
        VALUES_END);
}

void CloneSource::InsertPhotoMap()
{
    // map_album, map_asset
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "8, 1" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "8, 2" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "8, 3" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "9, 4" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "9, 5" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "11, 1" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_PHOTO_MAP + VALUES_BEGIN + "11, 4" + VALUES_END);
}

void CloneSource::InsertAnalysisAlbum()
{
    // album_id, album_type, album_subtype, album_name
    cloneStorePtr_->ExecuteSql(INSERT_ANALYSIS_ALBUM + VALUES_BEGIN + "1, 4096, 4101, '1'" + VALUES_END);
    cloneStorePtr_->ExecuteSql("INSERT INTO " + ANALYSIS_ALBUM_TABLE + "("
        "album_type, album_subtype, album_name, tag_id, cover_uri, is_cover_satisfied) "
        "VALUES (4096, 4102, 'Test Portrait Album', 'test_tag_id', 'test_cover_uri', 1)");
    cloneStorePtr_->ExecuteSql("INSERT INTO " + ANALYSIS_ALBUM_TABLE + "("
        "album_type, album_subtype, album_name, tag_id, cover_uri, is_cover_satisfied) "
        "VALUES (4096, 4100, 'Test City Album', 'test_tag_id', 'test_cover_uri', 1)");
    cloneStorePtr_->ExecuteSql("INSERT INTO " + ANALYSIS_ALBUM_TABLE + "("
        "album_type, album_subtype, album_name, tag_id, cover_uri, is_cover_satisfied) "
        "VALUES (4096, 4097, 'Test Classify Album', 'test_tag_id', 'test_cover_uri', 1)");
}

void CloneSource::InsertFaceTag()
{
    // insert data into VISION_FACE_TAG_TABLE
    cloneStorePtr_->ExecuteSql(
        "INSERT INTO " + VISION_FACE_TAG_TABLE + "(tag_id, tag_name, center_features, tag_version, analysis_version) "
        "VALUES ('test_tag_id', 'Test Face Tag', 'test_center_features', 1, 1)");
}

void CloneSource::InsertImgFaceTbl()
{
    // insert data into VISION_IMAGE_FACE_TABLE
    cloneStorePtr_->ExecuteSql(
        "INSERT INTO " + VISION_IMAGE_FACE_TABLE + " (file_id, face_id, tag_id, scale_x, scale_y) "
        "VALUES (1, 'test_face_id', 'test_tag_id', 1.0, 1.0)");
}

void CloneSource::InsertAnalysisPhotoMap()
{
    // map_album, map_asset
    cloneStorePtr_->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 1" + VALUES_END);
}

void CloneSource::InsertAudio()
{
    // file_id,
    // data, size, title,
    // display_name, media_type, date_added, date_modified, date_taken, duration, is_favorite, date_trashed,
    // artist
    cloneStorePtr_->ExecuteSql(INSERT_AUDIO + VALUES_BEGIN + "1, " +
        "'/storage/cloud/files/Audio/16/AUD_1501924014_000.mp3', 4239718, 'Risk It All', " +
        "'A8_MUSIC_PRODUCTIONS_-_Risk_It_All.mp3', 3, 1501923914046, 1501923914090, 1704038400, 175490, 0, 0, " +
        "'A8 MUSIC PRODUCTIONS'" + VALUES_END);
    cloneStorePtr_->ExecuteSql(INSERT_AUDIO + VALUES_BEGIN + "2, " +
        "'/storage/cloud/files/Audio/1/AUD_1501924014_001.mp3', 5679616, 'Alone', " +
        "'Alone_-_Color_Out.mp3', 3, 1501923914157, 1501923914200, 1609430400, 245498, 0, 1501924213700, " +
        "'Color Out'" + VALUES_END); // trashed
    cloneStorePtr_->ExecuteSql(INSERT_AUDIO + VALUES_BEGIN + "3, " +
        "'/storage/cloud/files/Audio/2/AUD_1501924014_002.mp3', 2900316, 'Muito Love', " +
        "'Ed_Napoli_-_Muito_Love.mp3', 3, 1501923914301, 1501923914326, 1704038400, 120633, 1, 0, " +
        "'Ed Napoli'" + VALUES_END); // favorite
    cloneStorePtr_->ExecuteSql(INSERT_AUDIO + VALUES_BEGIN + "4, " +
        "'/storage/cloud/files/Audio/2/AUD_1501924014_003.mp3', 0, 'size_0', " +
        "'size_0.mp3', 3, 1501923914301, 1501923914326, 1704038400, 120633, 0, 0, " +
        "'Ed Napoli'" + VALUES_END); // size = 0
}

void CloneSource::InsertAnalysisGeoDictionary()
{
    //city_id, language, city_name,
    cloneStorePtr_->ExecuteSql(INSERT_ANALYSIS_GEO_DICTIONARY + VALUES_BEGIN + "'945032946426347352', " +
        "'zh-Hans', '武汉'" + VALUES_END);
}

void CloneSource::InsertAnalysisSegmentation()
{
    cloneStorePtr_->ExecuteSql("INSERT INTO tab_analysis_segmentation (file_id, segmentation_area, segmentation_name, "
        "prob, segmentation_version, analysis_version) VALUES ("
        "1, 'Lung', 1, 0.3349609375, '21', '1.1');");
}

void CloneSource::InsertTabAnalysisLabel()
{
    //id, file_id, category_id, sub_label, prob, feature,
    //sim_result, label_version, saliency_sub_prob, analysis_version
    cloneStorePtr_->ExecuteSql(INSERT_TAB_ANALYSIS_LABEL + VALUES_BEGIN + "1, 1, 2, '44', 1088, '5796', " +
        "'68', '1.5', '123', '123'" + VALUES_END);
}

void CloneSource::InsertTabAnalysisVideoLabel()
{
    //id, file_id, category_id, confidence_probability, sub_category, sub_confidence_prob,
    //sub_label, sub_label_prob, sub_label_type, tracks, video_part_feature, filter_tag, algo_version,
    //analysis_version, trigger_generate_thumbnail
    cloneStorePtr_->ExecuteSql(INSERT_TAB_ANALYSIS_VIDEO_LABEL + VALUES_BEGIN + "1, 1, '2', 9827, '103', 9827, " +
        "'153', 9827, 0, 'beginFrame', '', '1', '1.5', '123', 1" + VALUES_END);
}

void CloneSource::InsertTabAnalysisGeoKnowledge()
{
    // file_id, latitude, longitude, location_key, city_id, language,
    // country, admin_area, sub_admin_area, locality, sub_locality,
    // thoroughfare, sub_thoroughfare, feature_name, city_name, address_description,
    // aoi, poi, first_aoi, first_poi, location_version,
    // first_aoi_category, first_poi_category, location_type
    cloneStorePtr_->ExecuteSql(INSERT_TAB_ANALYSIS_GEO_KNOWLEDGE + VALUES_BEGIN +
        "1, 31.2, 121.5, 141115170378, '271527323140241011', 'zh-Hans', "
        "'C', 'AA', 'SAR', 'L', 'SL', "
        "'TF', 'STF', 'FN', 'CN', 'AD', "
        "'AOI', 'POI', 'FAOI', 'FPOI', 'LV', "
        "'FAIOC', 'FPOIC', 'LT'" + VALUES_END);
}

void CloneSource::InsertTabAnalysisTotal()
{
    // id, file_id, status, ocr, label, aesthetics_score,
    // face, object, recommendation, segmentation, composition, saliency,
    // head, pose, geo
    cloneStorePtr_->ExecuteSql(INSERT_TAB_ANALYSIS_TOTAL + VALUES_BEGIN +
        "1, 1, 0, 0, 1, 0, "
        "0, 0, 0, 0, 0, 0, "
        "0, 0, 2" + VALUES_END);
}

void CloneSource::InsertTabBeautyScore()
{
    const std::string INSERT_TAB_BEAUTY_SCORE = "INSERT INTO " + ANALYSIS_BEAUTY_SCORE_TABLE +
        " (file_id, aesthetics_score, aesthetics_version, prob, analysis_version, " +
        "selected_flag, selected_algo_version, selected_status, negative_flag, negative_algo_version) ";

    cloneStorePtr_->ExecuteSql(INSERT_TAB_BEAUTY_SCORE + VALUES_BEGIN + "10112, 11190, 'v1.1', 0.98, 'analysis_v1', " +
        "1, 'selected_v1', 0, 0, 'negative_v1'" + VALUES_END);
}

void CloneSource::InsertTabVideoFace()
{
    const std::string INSERT_TAB_VIDEO_FACE = "INSERT OR REPLACE INTO " + ANALYSIS_VIDEO_FACE_TABLE +
        " (file_id, face_id, tag_id, scale_x, scale_y, scale_width, scale_height, landmarks, pitch, yaw, roll, " +
        "prob, total_faces, frame_id, frame_timestamp, tracks, algo_version, features, analysis_version) ";

    // old file id is 10111
    cloneStorePtr_->ExecuteSql(INSERT_TAB_VIDEO_FACE + VALUES_BEGIN +
        "10111, 'face_id_src_001', 'tag_id_src_B', '0.2', "
        "'0.2', '0.3', '0.3', 'landmarks_src_1', '5.0', '10.0', '15.0', '0.95', 2, '5', '5000', 'tracks_src_1', "
        "'algo_v2', 'features_src_1', 'analysis_v2'" + VALUES_END);
}
} // namespace Media
} // namespace OHOS