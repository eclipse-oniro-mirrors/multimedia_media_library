/*
 * Copyright (C) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef OCR_RESTORE_H
#define OCR_RESTORE_H

#include <atomic>
#include <mutex>
#include <sstream>
#include <string>

#include "backup_const.h"
#include "nlohmann/json.hpp"
#include "rdb_store.h"

namespace OHOS::Media {

struct GalleryOCRInfo {
    PhotoInfo photoInfo;
    int fileIdOld;
    string hash;
    string ocrText;
    int ocrVersion;
    int width;
    int height;
};

struct OcrDataProfile {
    // 查询1: 源表数据分析
    int64_t ocrTotal {0};
    int64_t ocrValidHash {0};
    int64_t distinctOcrValidHash {0};
    int64_t mediaTotal {0};
    int64_t mediaValidHash {0};
    int64_t distinctMediaValidHash {0};
    int64_t matchedHash {0};
    // 查询2: 异常数据分析
    int64_t hashEmpty {0};
    int64_t textEmpty {0};
    int64_t duplicateHashCount {0};
    int64_t maxDuplicateTimes {0};
    int64_t hashNotExist {0};
};

class OCRRestore {
public:
    void Init(int32_t sceneCode, const std::string& taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    void RestoreOCR(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);

private:
    void UpdateOcrInsertValues(std::vector<NativeRdb::ValuesBucket> &values, const GalleryOCRInfo &ocrInfo);
    void RestoreOCRInfos(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, bool isCloudRestoreSatisfied);
    void RestoreOCRTotal(const std::vector<int32_t> &fileIds);
    bool CheckBatchTimeout(int64_t currentTime, int64_t shouldEndTime, int64_t startTime, bool firstBatch);

    void BatchInsertAndReport(const std::vector<NativeRdb::ValuesBucket> &values);
    int32_t BatchInsertWithRetry(
        const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);

    int64_t GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    int64_t GetTotalGalleryOcrRecords();
    void GetMaxId();
    void ReportRestoreTask();
    void CollectOcrDataProfile();
    void QueryOcrTableAnalysis();
    void QueryOcrAnomalyAnalysis();

    // Task context
    int32_t sceneCode_ {-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;

    // Statistics for task report
    int64_t maxId_ {0};
    std::atomic<int64_t> restoreTimeCost_ {0};
    std::atomic<int32_t> exitCode_ {-1};
    std::atomic<int64_t> totalGalleryOcrRecords_ {0};
    std::atomic<int64_t> successCnt_ {0};
    std::atomic<int64_t> failCnt_ {0};
    OcrDataProfile ocrDataProfile_ {};

    const std::string SQL_OCR_TABLE_ANALYSIS = "\
        SELECT \
            (SELECT COUNT(*) FROM t_ocr_result) AS ocr_total, \
            (SELECT COUNT(hash) FROM t_ocr_result WHERE hash IS NOT NULL AND hash != '') AS ocr_valid_hash, \
            (SELECT COUNT(DISTINCT hash) FROM t_ocr_result WHERE hash IS NOT NULL AND hash != '') \
                AS distinct_ocr_valid_hash, \
            (SELECT COUNT(*) FROM gallery_media) AS media_total, \
            (SELECT COUNT(hash) FROM gallery_media WHERE hash IS NOT NULL AND hash != '') AS media_valid_hash, \
            (SELECT COUNT(DISTINCT hash) FROM gallery_media WHERE hash IS NOT NULL AND hash != '') \
                AS distinct_media_valid_hash, \
            (SELECT COUNT(*) FROM ( \
                SELECT hash FROM t_ocr_result WHERE hash IS NOT NULL AND hash != '' \
                INTERSECT \
                SELECT hash FROM gallery_media WHERE hash IS NOT NULL AND hash != '' \
            )) AS matched_hash;";
    const std::string SQL_OCR_ANOMALY_ANALYSIS = "\
        SELECT \
            COALESCE(SUM(CASE WHEN hash IS NULL OR hash = '' THEN 1 ELSE 0 END), 0) AS hash_empty, \
            COALESCE(SUM(CASE WHEN ocr_text IS NULL OR ocr_text = '' THEN 1 ELSE 0 END), 0) AS text_empty, \
            (SELECT COUNT(*) FROM ( \
                SELECT hash FROM t_ocr_result WHERE hash IS NOT NULL AND hash != '' \
                GROUP BY hash HAVING COUNT(*) > 1 \
            )) AS duplicate_hash_count, \
            (SELECT COALESCE(MAX(cnt), 0) FROM \
                (SELECT COUNT(*) AS cnt FROM t_ocr_result WHERE hash IS NOT NULL AND hash != '' \
                GROUP BY hash)) AS max_duplicate_times, \
            (SELECT COUNT(*) FROM t_ocr_result t \
                LEFT JOIN gallery_media g ON t.hash = g.hash \
                WHERE g.hash IS NULL AND t.hash IS NOT NULL AND t.hash != '') AS hash_not_exist \
        FROM t_ocr_result;";
};
}  // namespace OHOS::Media

#endif  // OCR_RESTORE_H