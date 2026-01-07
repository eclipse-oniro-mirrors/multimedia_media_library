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

#ifndef CLASSIFY_RESTORE_H
#define CLASSIFY_RESTORE_H

#include <string>

#include "backup_const.h"
#include "rdb_store.h"
#include "classify_aggregate_types.h"
// LCOV_EXCL_START
namespace OHOS::Media {
class ClassifyRestore {
public:
    void Init(int32_t sceneCode, const std::string &taskId,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    void RestoreClassify(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

private:
    struct GalleryLabelInfo {
        int32_t categoryId = -2;
        std::string subLabel;
        double prob = 0;
        std::string version;
        int32_t fileIdOld = -1;
        PhotoInfo photoInfo;
    };

    int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    void GetMaxIds();
    void ProcessLabelInfo(std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void ProcessImageCollectionInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap, std::string &hash);
    void ProcessGalleryMediaInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unordered_map<std::string, GalleryLabelInfo> &galleryLabelInfoMap);
    void RestoreLabel(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void ReportRestoreTask();
    void TransferLabelInfo(GalleryLabelInfo &info);
    void UpdateLabelInsertValues(std::vector<NativeRdb::ValuesBucket> &values,
        const GalleryLabelInfo &info);
    void UpdateStatus(std::vector<int32_t> &fileIds);
    std::vector<int32_t> ParseSubLabel(const std::string &subLabel) const;
    std::unordered_set<int32_t> GetAggregateTypes(const std::vector<int32_t> &labels) const;
    void CollectAlbumInfo(int32_t fileIdNew, int32_t categoryId, const std::vector<int32_t> &labels);
    int32_t EnsureClassifyAlbumId(const std::string &albumName);
    void InsertAlbumMappings(std::vector<NativeRdb::ValuesBucket> &values);
    void UpdateAlbumCounts(const std::unordered_set<int32_t> &albumIds);
    void CreateOrUpdateCategoryAlbums();
    void DeleteExistMapping(std::vector<int32_t> &fileIds);
    void EnsureSpecialAlbums();
    void EnsureSelfieAlbum();
    void EnsureUserCommentAlbum();
    void HandleOcr(const std::unordered_map<int32_t, std::vector<int32_t>> &subLabelMap);
    void HandleOcrHelper(const std::vector<int32_t> &fileIds);
    void AddIdCardAlbum(OcrAggregateType type, std::unordered_set<int32_t> &fileIdsToUpdateSet);
    void ProcessCategoryAlbums();
    int64_t GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

private:
    int32_t sceneCode_ {-1};
    int32_t maxIdOfLabel_ {0};
    std::atomic<int64_t> restoreTimeCost_ {0};
    std::atomic<int32_t> successInsertLabelCnt_ {0};
    std::atomic<int32_t> failInsertLabelCnt_ {0};
    std::atomic<int32_t> duplicateLabelCnt_ {0};
    std::atomic<int32_t> exitCode_ {-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::unordered_map<std::string, std::unordered_set<int32_t>> albumAssetMap_;
    std::unordered_map<std::string, int32_t> albumIdCache_;

private:
    const std::string QUERY_LABEL_SQL_PART_ONE = " \
        SELECT \
            category_id, \
            sub_label, \
            prob, \
            version, \
            hash \
        FROM image_collection \
        WHERE hash > ? \
        ORDER BY hash LIMIT ?;";
    
    const std::string QUERY_LABEL_SQL_PART_TWO = " \
        SELECT \
            _id, \
            hash \
        FROM ( \
            SELECT \
                gm._id, \
                gm.hash, \
                ROW_NUMBER() OVER ( \
                    PARTITION BY gm.hash \
                    ORDER BY \
                        CASE \
                            WHEN COALESCE(gm.recycleFlag, 0) NOT IN (2, -1, 1, -2, -4) \
                            AND COALESCE(gm.albumId, '') NOT IN (SELECT albumId FROM gallery_album WHERE hide =1) \
                            THEN 1 \
                            ELSE 2 \
                        END ASC \
                ) AS rn \
            FROM gallery_media gm \
            WHERE gm.hash IN ( \
                SELECT hash \
                FROM image_collection \
                WHERE hash > ? \
                ORDER BY hash \
                LIMIT ? \
            ) \
        ) sub \
        WHERE rn = 1;";

    const std::string QUERY_OCR_TEXT_SQL = "SELECT tab_analysis_ocr.file_id FROM tab_analysis_ocr WHERE ";
};
} // namespace OHOS::Media
// LCOV_EXCL_STOP
#endif // CLASSIFY_RESTORE_H