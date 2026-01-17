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

#ifndef OHOS_MEDIA_PORTRAIT_ALBUM_DAO_H
#define OHOS_MEDIA_PORTRAIT_ALBUM_DAO_H

#include <string>

namespace OHOS::Media {
class PortraitAlbumDao {
public:
    PortraitAlbumDao() = default;
    ~PortraitAlbumDao() = default;
private:
    const int32_t AROUSAL_STRONG = 9;
    const int32_t AROUSAL_MODERATE = 5;
    const int32_t AROUSAL_WEAK = 1;

    const int32_t VALENCE_POSTIVE = 1;

    const int32_t STRONG_BONUS = 100;
    const int32_t MODERATE_BONUS = 50;
    const int32_t WEAK_BONUS = 30;

    const int32_t AESTHETICS_SCORE_THRESHOLD = 40;
    const int32_t IS_EXCLUDED_THRESHOLD = 3203;
public:
    // Scoring method used as a column in SQL queries
    const std::string SQL_PORTRAIT_ALBUM_COLUM_TOTAL_SCORE = "\
        (CASE \
            WHEN tab_analysis_affective.valence = " + std::to_string(VALENCE_POSTIVE) +
            " AND tab_analysis_affective.arousal = " + std::to_string(AROUSAL_STRONG) +
            " THEN " + std::to_string(STRONG_BONUS) + " \
            WHEN tab_analysis_affective.valence = " + std::to_string(VALENCE_POSTIVE) +
            " AND tab_analysis_affective.arousal = " + std::to_string(AROUSAL_MODERATE) +
            " THEN " + std::to_string(MODERATE_BONUS) + " \
            WHEN tab_analysis_affective.valence = " + std::to_string(VALENCE_POSTIVE) +
            " AND tab_analysis_affective.arousal = " +
            std::to_string(AROUSAL_WEAK) + " THEN " + std::to_string(WEAK_BONUS) + " \
            ELSE 0 \
        END) + \
        (CASE \
            WHEN Photos.is_favorite = 1 THEN " + std::to_string(MODERATE_BONUS) + " \
            ELSE 0 \
        END) + \
        tab_analysis_image_face.aesthetics_score ";

    // talbe join
    const std::string SQL_PORTRAIT_ALBUM_TABLE_JOIN = "\
        Photos \
        INNER JOIN tab_analysis_image_face ON ( \
            tab_analysis_image_face.file_id = Photos.file_id \
        ) \
        INNER JOIN AnalysisAlbum ON ( \
            tab_analysis_image_face.tag_id = AnalysisAlbum.tag_id \
            AND tab_analysis_image_face.tag_id LIKE 'ser%' \
        ) \
        INNER JOIN ( \
            SELECT group_tag \
            FROM AnalysisAlbum \
            WHERE album_id = ? \
        ) ag ON ( \
            ag.group_tag = AnalysisAlbum.group_tag \
        ) \
        LEFT OUTER JOIN tab_analysis_affective ON ( \
            tab_analysis_affective.file_id = Photos.file_id \
        )";

    // filter condition
    const std::string SQL_PORTRAIT_ALBUM_CONDITION = "\
        Photos.sync_status = 0 \
        AND Photos.clean_flag = 0 \
        AND media_type = 1 \
        AND file_source_type <> 4 \
        AND date_trashed = 0 \
        AND hidden = 0 \
        AND time_pending = 0 \
        AND is_temp = 0 \
        AND burst_cover_level = 1 \
        AND tab_analysis_image_face.aesthetics_score > " + std::to_string(AESTHETICS_SCORE_THRESHOLD) + " \
        AND (tab_analysis_image_face.is_excluded & " + std::to_string(IS_EXCLUDED_THRESHOLD) +
        ") = " + std::to_string(IS_EXCLUDED_THRESHOLD);

    // get assets that meet the conditions
    const std::string SQL_PORTRAIT_ALBUM_GET_SELECTED_ASSETS = "\
        SELECT Photos.burst_key, Photos.ce_available, Photos.cover_position, Photos.data, Photos.date_added, \
            Photos.date_day, Photos.date_modified, Photos.date_taken, Photos.date_trashed, Photos.detail_time, \
            Photos.display_name, Photos.duration, Photos.dynamic_range_type, Photos.file_id, Photos.front_camera, \
            Photos.height, Photos.hidden, Photos.is_auto, Photos.is_favorite, Photos.lcd_size, Photos.media_type, \
            Photos.mime_type, Photos.moving_photo_effect_mode, Photos.orientation, Photos.original_subtype, \
            Photos.photo_quality,Photos.position, Photos.shooting_mode_tag, Photos.size, Photos.subtype, \
            Photos.thumb_size, Photos.thumbnail_ready, Photos.title, Photos.width, " +
            SQL_PORTRAIT_ALBUM_COLUM_TOTAL_SCORE + " AS total_score \
        FROM " + SQL_PORTRAIT_ALBUM_TABLE_JOIN + " \
        WHERE " + SQL_PORTRAIT_ALBUM_CONDITION + " \
            AND total_score >= ? \
            AND total_score <= ? \
        ORDER BY total_score DESC, \
            Photos.file_id \
        LIMIT ? \
        OFFSET 0";

    // get the score of the asset at the 30th percentile among assets that meet the conditions
    const std::string SQL_PORTRAIT_ALBUM_GET_LIMIT_SCORE = "\
        SELECT * FROM ( \
            SELECT \
                ROW_NUMBER() OVER (ORDER BY " + SQL_PORTRAIT_ALBUM_COLUM_TOTAL_SCORE + " DESC ) AS rank, \
                Photos.file_id, \
                COUNT(1) OVER () AS count, \
                " + SQL_PORTRAIT_ALBUM_COLUM_TOTAL_SCORE + " AS total_score \
            FROM " + SQL_PORTRAIT_ALBUM_TABLE_JOIN + " \
            WHERE " + SQL_PORTRAIT_ALBUM_CONDITION + " \
        ) WHERE rank = ROUND(count * 0.3 + 0.49);";

    // get the score of the asset with the  file_id
    const std::string SQL_PORTRAIT_ALBUM_GET_ASSET_SCORE = "\
        SELECT " + SQL_PORTRAIT_ALBUM_COLUM_TOTAL_SCORE + " AS total_score \
        FROM " + SQL_PORTRAIT_ALBUM_TABLE_JOIN + " \
        WHERE " + SQL_PORTRAIT_ALBUM_CONDITION + " \
        AND Photos.file_id = ?;";
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PORTRAIT_ALBUM_DAO_H