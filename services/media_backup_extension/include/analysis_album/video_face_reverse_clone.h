/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef VIDEO_FACE_REVERSE_CLONE_H
#define VIDEO_FACE_REVERSE_CLONE_H

#include <string>
#include <vector>
#include <optional>
#include <type_traits>
#include <memory>

#include "backup_const.h"
#include "rdb_store.h"
#include "backup_database_utils.h"
#include "video_face_clone.h"

namespace OHOS {
namespace Media {

class VideoFaceReverseClone {
public:
    VideoFaceReverseClone(
        const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
        const std::shared_ptr<NativeRdb::RdbStore>& destRdb);

    void ReverseClone();

private:
    bool ShouldSkipClone();
    std::vector<VideoFaceTbl> QueryVideoFaceTbl(int32_t offset,
        const std::vector<std::string>& commonColumns);
    std::shared_ptr<NativeRdb::RdbStore> GetSourceRdb() { return sourceRdb_; }
    std::shared_ptr<NativeRdb::RdbStore> GetTargetRdb() { return destRdb_; }
    void ParseVideoFaceResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        VideoFaceTbl& tbl);
    int32_t InsertVideoFaceByTable(std::vector<VideoFaceTbl>& videoFaceTbl);
    NativeRdb::ValuesBucket CreateValuesBucketFromVideoFaceTbl(const VideoFaceTbl& videoFaceTbl);
    int32_t BatchInsertWithRetry(const std::string& tableName,
        std::vector<NativeRdb::ValuesBucket>& values, int64_t& rowNum);
    void UpdateVideoTotalTableFaceStatus();

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    static constexpr int32_t QUERY_COUNT = 200;
};

} // namespace Media
} // namespace OHOS

#endif // VIDEO_FACE_REVERSE_CLONE_H