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

#ifndef CLONE_RESTORE_CLASSIFY_BASE_H
#define CLONE_RESTORE_CLASSIFY_BASE_H

#include "values_bucket.h"
#include "backup_const_column.h"
#include "classify_restore_const.h"

namespace OHOS::Media {

const uint32_t BIT1 = 1u << 1;
const uint32_t BIT20 = 1u << 20;
const std::string ANALYSIS_LABEL_TABLE = "tab_analysis_label";
const std::string ANALYSIS_VIDEO_LABEL_TABLE = "tab_analysis_video_label";
const std::string ANALYSIS_TOTAL_TABLE = "tab_analysis_total";

class CloneRestoreClassifyBase {
public:
    virtual ~CloneRestoreClassifyBase() = default;

protected:
    void GetAnalysisAlbumInsertValue(NativeRdb::ValuesBucket &value,
        const ClassifyAlbumInfo &info);
    void ParseClassifyAlbumResultSet(ClassifyAlbumInfo &info,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetClassifyInfoFromResultSet(ClassifyCloneInfo &info,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetClassifyVideoInfoFromResultSet(ClassifyVideoCloneInfo &info,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetMapInsertValue(NativeRdb::ValuesBucket &value,
        ClassifyCloneInfo &info,
        const std::unordered_set<std::string> &intersection);
    void GetVideoMapInsertValue(NativeRdb::ValuesBucket &value,
        const ClassifyVideoCloneInfo &info,
        const std::unordered_set<std::string> &intersection);
    std::unordered_set<std::string> GetCommonColumns(const std::string &tableName);
    bool CheckTableColumns(const std::string &tableName,
        std::unordered_map<std::string, std::string> &columns);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum,
        std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    void UpdateScoreMask(int32_t fileId, uint32_t mask);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket& values,
        const std::string& columnName,
        const std::optional<T>& optionalValue,
        const std::unordered_set<std::string> &intersection)
    {
        if (intersection.count(columnName) > 0 && optionalValue.has_value()) {
            if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
                values.PutInt(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
                values.PutLong(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                values.PutString(columnName, optionalValue.value());
            } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
                values.PutDouble(columnName, optionalValue.value());
            }
        }
    }

    int32_t sceneCode_{-1};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::unordered_map<int32_t, uint32_t>* externalScoreMaskMap_{nullptr};
    std::unordered_map<int32_t, int32_t>* duplicateMap_{nullptr};
};

} // namespace OHOS::Media

#endif // CLONE_RESTORE_CLASSIFY_BASE_H