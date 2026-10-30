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

#ifndef OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_H
#define OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "userfile_manager_types.h"

namespace OHOS::NativeRdb {
class RdbStore;
}

namespace OHOS::Media {
bool IsSupportedAnalysisSubtype(int32_t subtype);

inline bool IsSingleSideAnalysisSubtype(int32_t subtype)
{
    return subtype == static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT) ||
        subtype == static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO);
}

inline constexpr size_t ANALYSIS_REFRESH_SQL_VARIABLE_LIMIT = 999;
inline constexpr size_t ANALYSIS_REFRESH_QUERY_BATCH_SIZE = 400;
struct AnalysisAlbumGroupMembers {
    std::unordered_map<int32_t, std::string> albumGroups;
    std::unordered_map<std::string, std::vector<int32_t>> groups;
};

enum class AnalysisAssetQuery : uint8_t { ALBUM_MEMBERS, EXISTING, DELETION_WITH_BURST };

enum class AnalysisAlbumDatabaseSide : uint8_t { SOURCE = 0, TARGET };
enum class AnalysisAlbumContributionState : uint8_t { UNKNOWN = 0, NOT_COUNTED, COUNTED };
struct AnalysisAlbumAssetKey {
    int32_t albumId = 0;
    int32_t fileId = 0;
    bool operator==(const AnalysisAlbumAssetKey &other) const
    {
        return albumId == other.albumId && fileId == other.fileId;
    }
};

struct AnalysisAlbumAssetKeyHash {
    size_t operator()(const AnalysisAlbumAssetKey &key) const
    {
        size_t value = std::hash<int32_t> {}(key.albumId);
        return value ^ (std::hash<int32_t> {}(key.fileId) + 0x9e3779b9U + (value << 6) + (value >> 2));
    }
};

using AnalysisAlbumContributions = std::unordered_map<AnalysisAlbumAssetKey,
    AnalysisAlbumContributionState, AnalysisAlbumAssetKeyHash>;

struct AnalysisAlbumBinding {
    int32_t finalAlbumId = 0;
    bool includeBaseline = true;
};

struct AnalysisAlbumBaseline {
    int64_t count = 0;
    std::string albumName;
    int32_t coverFileId = 0;
    uint8_t coverSatisfied = 0;
    int32_t subtype = 0;
    bool coverDeleted = false;
    bool coverUnknown = false;
    AnalysisAlbumBinding binding;
    bool registered = false;
    bool complete = true;
    int64_t capturedCount = 0;
};

struct AnalysisAlbumOperation {
    int32_t sourceId = 0;
    int32_t targetId = 0;
    int32_t replacedTargetId = 0;
    int32_t subtype = 0;
    bool complete = false;
};

using AnalysisAlbumRestoreObserver = std::function<void(const AnalysisAlbumOperation &)>;

struct AnalysisAlbumQueryScope {
    int32_t albumId = 0;
    int32_t subtype = 0;
    std::string albumName;
};

struct AnalysisAlbumShootingQuery {
    std::string projection;
    std::vector<std::string> arguments;
    std::vector<int32_t> modes;
    std::vector<std::vector<int32_t>> albums;
};

struct AnalysisAlbumQueryDirectory {
    std::unordered_set<int32_t> ordinary;
    std::unordered_set<int32_t> grouped;
    std::vector<AnalysisAlbumShootingQuery> shootingQueries;
};

enum class AnalysisBeforeCoverage : uint8_t { UNCAPTURED, CAPTURED, UNAVAILABLE };
struct AnalysisAlbumContextSnapshot {
    std::unordered_map<int32_t, AnalysisAlbumBaseline> sourceAlbums;
    std::unordered_map<int32_t, AnalysisAlbumBaseline> targetAlbums;
    std::unordered_set<int32_t> invalidFinalAlbums;
    AnalysisAlbumGroupMembers targetGroups;
    AnalysisAlbumQueryDirectory targetDirectory;
    std::unordered_map<int32_t, AnalysisBeforeCoverage> targetCoverage;
    bool targetDirectoryReady = false;
    bool taskComplete = true;
};

struct AnalysisAlbumCountResult {
    bool complete = false;
    int32_t finalCount = 0;
};

struct AnalysisAlbumRefreshResult {
    int32_t error = 0;
    bool fullRefresh = false;
    std::unordered_set<int32_t> changedAlbumIds;
    std::unordered_set<int32_t> failedAlbumIds;
};

struct AnalysisAlbumMetadata {
    int32_t albumId = 0;
    int32_t subtype = 0;
    int32_t count = 0;
    std::string albumName;
    std::string coverUri;
    uint8_t coverSatisfied = 0;
    int32_t CoverFileId() const;
};


class AnalysisAlbumRefreshDb;

// Owned by the restore task. Worker access is limited to the externally serialized before-capture path.
// Batch facts are merged only after the original worker wait; freeze is a phase boundary, not a writer lock.
class AnalysisAlbumRefreshTask final {
public:
    AnalysisAlbumRefreshTask() = default;
    AnalysisAlbumRefreshTask(const AnalysisAlbumRefreshTask &) = delete;
    AnalysisAlbumRefreshTask &operator=(const AnalysisAlbumRefreshTask &) = delete;
    bool InitializeOriginalAlbums(const std::shared_ptr<NativeRdb::RdbStore> &rdb, AnalysisAlbumDatabaseSide side);
    bool PrepareTargetDirectory(AnalysisAlbumGroupMembers groups);
    bool CaptureBefore(const std::shared_ptr<NativeRdb::RdbStore> &rdb, const std::vector<int32_t> &fileIds);
    static AnalysisAlbumCountResult CalculateFinalCount(int64_t fixedBase, int64_t beforeSum, int64_t afterSum);
    AnalysisAlbumRefreshResult Refresh(const std::shared_ptr<NativeRdb::RdbStore> &targetRdb);
    void Clear();
    bool RecordOriginalAlbums(const std::vector<AnalysisAlbumMetadata> &albums, AnalysisAlbumDatabaseSide side);
    void RecordTargetAlbumMerge(int32_t originalId, int32_t finalId);
    void RecordAlbumResult(const AnalysisAlbumOperation &result);
    void MarkTargetAlbumIncomplete(int32_t albumId);
    const AnalysisAlbumContextSnapshot &GetFacts() const;
    const AnalysisAlbumBaseline *FindOriginalAlbum(AnalysisAlbumDatabaseSide side, int32_t albumId) const;
    bool MigrateTargetAlbums(int64_t offset, int64_t upperBound);
    bool MigrateTargetAssets(const std::unordered_map<int32_t, int32_t> &mappings);
    void RecordDeletedTargets(const std::vector<int32_t> &ids);
    void RecordPhotoResults(const std::vector<int32_t> &committedIds,
        const std::unordered_map<int32_t, int32_t> &replacements,
        const std::unordered_set<int32_t> &conflictingDonors = {});
    void MarkTaskIncomplete();

    const AnalysisAlbumContextSnapshot &Seal();
    bool IsTaskComplete() const;

private:
    AnalysisBeforeCoverage QueryBefore(AnalysisAlbumRefreshDb &db, const std::vector<int32_t> &ids,
        std::unordered_map<int32_t, int64_t> &counts);
    void RecordCoverReplacement(int32_t originalId, int32_t finalId, bool conflicting);
    // Borrowed catalog nodes remain stable through unordered_map rehash and node extraction.
    std::unordered_map<int32_t, std::vector<AnalysisAlbumBaseline *>> targetCoverOwners_;
    AnalysisAlbumContextSnapshot data_;
    bool sealed_ = false;
    bool targetResultsStarted_ = false;
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_H
