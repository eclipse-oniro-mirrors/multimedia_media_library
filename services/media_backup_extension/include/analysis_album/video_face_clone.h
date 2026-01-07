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

#ifndef VIDEO_FACE_CLONE_H
#define VIDEO_FACE_CLONE_H

#include <string>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "backup_const.h"
#include "rdb_store.h"
#include "backup_database_utils.h"


namespace OHOS {
namespace Media {
struct VideoFaceTbl {
    std::optional<int32_t> id;
    std::optional<int32_t> file_id;
    std::optional<std::string> face_id;
    std::optional<std::string> tag_id;
    std::optional<std::string> scale_x;
    std::optional<std::string> scale_y;
    std::optional<std::string> scale_width;
    std::optional<std::string> scale_height;
    std::optional<std::string> landmarks;
    std::optional<std::string> pitch;
    std::optional<std::string> yaw;
    std::optional<std::string> roll;
    std::optional<std::string> prob;
    std::optional<int32_t> total_faces;
    std::optional<std::string> frame_id;
    std::optional<std::string> frame_timestamp;
    std::optional<std::string> tracks;
    std::optional<std::string> algo_version;
    std::optional<std::vector<uint8_t>> features;
    std::optional<std::string> analysis_version;
};

class VideoFaceClone {
public:
    VideoFaceClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap);

    bool CloneVideoFaceInfo();

    int64_t GetMigratedFaceCount() const { return migrateVideoFaceNum_; }
    int64_t GetMigratedFileCount() const { return migrateVideoFaceFileNumber_; }
    int64_t GetTotalTimeCost() const { return migrateVideoFaceTotalTimeCost_; }

private:
    std::vector<VideoFaceTbl> QueryVideoFaceTbl(int32_t offset, std::string &fileIdClause,
        const std::vector<std::string> &commonColumns);
    void ParseVideoFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        VideoFaceTbl& videoFaceTbl);
    std::vector<VideoFaceTbl> ProcessVideoFaceTbls(const std::vector<VideoFaceTbl>& videoFaceTbls);
    void BatchInsertVideoFaces(const std::vector<VideoFaceTbl>& videoFaceTbls);
    NativeRdb::ValuesBucket CreateValuesBucketFromVideoFaceTbl(const VideoFaceTbl& videoFaceTbl);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    void DeleteExistingVideoFaceData(const std::vector<int32_t>& newFileIds);
    bool CopyAnalysisVideoTotalTab(const std::string &tableName, const std::vector<int32_t> &oldFileIds);
    void UpdateAnalysisVideoTotalTblLabelAndFace(const std::vector<int32_t> &oldFileIds,
        const std::vector<int32_t> &newFileIds);
    void UpdateAnalysisVideoTotalTblFaceAndTagId();
    void WriteDataToAnaVideoTotalTab(std::vector<CloneVideoInfo> &updateDataList);
    void WriteDataToAnaVideoTotalTabSub(std::vector<CloneVideoInfo> &updateDataList);
    void StartCloneAnalysisVideoTotalTab(const std::vector<int32_t> &oldFileIds,
        const std::vector<int32_t> &newFileIds);

    template<typename T>
    void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue)
    {
        if (optionalValue.has_value()) {
            if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
                values.PutInt(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
                values.PutLong(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                values.PutString(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
                values.PutDouble(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint8_t>>) {
                values.PutBlob(columnName, optionalValue.value());
            }
        }
    }

private:
    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap_;

    int64_t migrateVideoFaceNum_ = 0;
    int64_t migrateVideoFaceFileNumber_ = 0;
    int64_t migrateVideoFaceTotalTimeCost_ = 0;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEO_FACE_CLONE_H