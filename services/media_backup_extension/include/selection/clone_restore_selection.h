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

#ifndef CLONE_RESTORE_SELECTION_H
#define CLONE_RESTORE_SELECTION_H

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <sstream>

#include "backup_const.h"
#include "backup_file_utils.h"
#include "rdb_store.h"
#include "clone_restore_analysis_total.h"

namespace OHOS::Media {
struct SelectionInfo {
    std::optional<int32_t> fileId;
    std::optional<int32_t> monthFlag;
    std::optional<int32_t> yearFlag;
    std::optional<std::string> selectionVersion;
    std::optional<int32_t> eventId;

    std::string ToString() const
    {
        std::stringstream outputStr;
        outputStr << "SelectionInfo[";
        if (fileId.has_value()) {
            outputStr << "fileId: " << fileId.value();
        }
        if (monthFlag.has_value()) {
            outputStr << ", monthFlag: " << monthFlag.value();
        }
        if (yearFlag.has_value()) {
            outputStr << ", yearFlag: " << yearFlag.value();
        }
        if (eventId.has_value()) {
            outputStr << ", eventId: " << eventId.value();
        }
        outputStr << "]";
        return outputStr.str();
    }
};

struct AtomEventInfo {
    std::optional<int32_t> eventId;
    std::optional<int64_t> minDate;
    std::optional<int64_t> maxDate;
    std::optional<int32_t> count;
    std::optional<int32_t> dateDay;
    std::optional<int32_t> dateMonth;
    std::optional<int32_t> dateType;
    std::optional<int32_t> eventScore;
    std::optional<std::string> eventVersion;
    std::optional<int32_t> eventStatus;

    std::string ToString() const
    {
        std::stringstream outputStr;
        outputStr << "AtomEventInfo[";
        if (eventId.has_value()) {
            outputStr << "eventId: " << eventId.value();
        }
        if (minDate.has_value()) {
            outputStr << ", minDate: " << minDate.value();
        }
        if (maxDate.has_value()) {
            outputStr << ", maxDate: " << maxDate.value();
        }
        if (count.has_value()) {
            outputStr << ", count: " << count.value();
        }
        outputStr << "]";
        return outputStr.str();
    }
};

class CloneRestoreSelection {
public:
    void Init(int32_t sceneCode, const std::string &taskId, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb, const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
        bool isCloudRestoreSatisfied);
    bool Preprocess();
    void Restore();
    void RestoreAnalysisTotalSelectionStatus();
    bool RefreshTotalNumber();

protected:
    std::atomic<uint64_t> migrateSelectionNumber_{0};
    std::atomic<uint64_t> migrateAtomEventNumber_{0};
    std::atomic<uint64_t> migrateSelectionTotalTimeCost_{0};

private:
    void DeleteExistingSelectionInfos();
    void DeleteExistingSelectionTable();
    void DeleteExistingAtomEventTable();
    void ClearTotalTableSelectionFields();
    void RestoreSelectionData();
    void RestoreAtomEventData();
    void AppendExtraWhereClause(std::string &whereClause);
    std::vector<SelectionInfo> QuerySelectionTbl(int32_t lastFileId);
    void BatchInsertSelectionData(const std::vector<SelectionInfo> &selectionInfos);
    NativeRdb::ValuesBucket CreateValuesBucketFromSelectionInfo(const SelectionInfo &info);
    std::vector<AtomEventInfo> QueryAtomEventTbl(int32_t lastEventId);
    void BatchInsertAtomEventData(const std::vector<AtomEventInfo> &atomEventInfos);
    NativeRdb::ValuesBucket CreateValuesBucketFromAtomEventInfo(const AtomEventInfo &info);
    int32_t BatchInsertWithRetry(
        const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    int64_t GetShouldEndTime();

private:
    int32_t sceneCode_;
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    bool isCloudRestoreSatisfied_;

    int32_t totalSelectionNumber_{0};
    int32_t totalAtomEventNumber_{0};
};
}  // namespace OHOS::Media
#endif  // CLONE_RESTORE_SELECTION_H