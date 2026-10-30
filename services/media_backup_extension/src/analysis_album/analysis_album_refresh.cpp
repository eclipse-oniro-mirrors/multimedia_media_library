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

#include "analysis_album_refresh.h"

#include <algorithm>
#include <charconv>
#include <iterator>
#include <limits>
#include <tuple>

#include "analysis_album_refresh_db.h"
#include "backup_const_column.h"
#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"

namespace OHOS::Media {

bool IsSupportedAnalysisSubtype(int32_t subtype)
{
    switch (static_cast<PhotoAlbumSubType>(subtype)) {
        case PhotoAlbumSubType::CLASSIFY:
        case PhotoAlbumSubType::GEOGRAPHY_LOCATION:
        case PhotoAlbumSubType::GEOGRAPHY_CITY:
        case PhotoAlbumSubType::SHOOTING_MODE:
        case PhotoAlbumSubType::PORTRAIT:
        case PhotoAlbumSubType::GROUP_PHOTO:
        case PhotoAlbumSubType::HIGHLIGHT:
        case PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS:
            return true;
        default:
            return false;
    }
}

int32_t AnalysisAlbumMetadata::CoverFileId() const
{
    const auto value = MediaLibraryDataManagerUtils::GetFileIdFromPhotoUri(coverUri);
    int32_t id = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), id);
    return result.ec == std::errc() && result.ptr == value.data() + value.size() && id > 0 ? id : 0;
}

bool AnalysisAlbumRefreshTask::IsTaskComplete() const
{
    return data_.taskComplete;
}

void AnalysisAlbumRefreshTask::Clear()
{
    targetCoverOwners_.clear();
    data_ = {};
    sealed_ = false;
    targetResultsStarted_ = false;
}

bool AnalysisAlbumRefreshTask::RecordOriginalAlbums(const std::vector<AnalysisAlbumMetadata> &albums,
    AnalysisAlbumDatabaseSide side)
{
    CHECK_AND_RETURN_RET(!sealed_, false);
    std::unordered_set<int32_t> ids;
    for (const auto &album : albums) {
        CHECK_AND_RETURN_RET(album.albumId > 0 && IsSupportedAnalysisSubtype(album.subtype) &&
            FindOriginalAlbum(side, album.albumId) == nullptr && ids.insert(album.albumId).second, false);
        CHECK_AND_RETURN_RET(side != AnalysisAlbumDatabaseSide::TARGET || !data_.targetDirectoryReady, false);
    }
    for (const auto &album : albums) {
        const int32_t cover = album.CoverFileId();
        auto &catalog = side == AnalysisAlbumDatabaseSide::SOURCE ? data_.sourceAlbums : data_.targetAlbums;
        auto &record = catalog[album.albumId];
        record = AnalysisAlbumBaseline {album.count, album.albumName, cover, album.coverSatisfied,
            album.subtype, false, false, record.binding, true, record.complete};
        if (side == AnalysisAlbumDatabaseSide::TARGET && cover > 0) {
            targetCoverOwners_[cover].push_back(&record);
            data_.targetCoverage.try_emplace(cover, AnalysisBeforeCoverage::UNCAPTURED);
        }
    }
    return true;
}

void AnalysisAlbumRefreshTask::RecordTargetAlbumMerge(int32_t originalId, int32_t finalId)
{
    CHECK_AND_RETURN(!sealed_);
    if (originalId <= 0 || finalId <= 0) {
        data_.taskComplete = false;
        return;
    }
    // A binding is published only by a confirmed match or after the real operation commits.
    data_.targetAlbums[originalId].binding.finalAlbumId = finalId;
}

void AnalysisAlbumRefreshTask::MarkTargetAlbumIncomplete(int32_t albumId)
{
    CHECK_AND_RETURN(!sealed_);
    data_.targetAlbums[albumId].complete = false;
}

void AnalysisAlbumRefreshTask::RecordAlbumResult(const AnalysisAlbumOperation &result)
{
    CHECK_AND_RETURN(!sealed_);
    auto &source = data_.sourceAlbums[result.sourceId];
    MEDIA_DEBUG_LOG("AccAnalysisRef binding source=%{public}d target=%{public}d replaced=%{public}d "
        "subtype=%{public}d complete=%{public}d", result.sourceId, result.targetId,
        result.replacedTargetId, result.subtype, result.complete);
    const int32_t previousId = source.binding.finalAlbumId;
    const bool include = !IsSingleSideAnalysisSubtype(result.subtype) ||
        FindOriginalAlbum(AnalysisAlbumDatabaseSide::TARGET, result.targetId) == nullptr;
    if (result.complete && result.targetId > 0 && source.registered && source.subtype == result.subtype) {
        source.binding = {result.targetId, include};
        if (result.replacedTargetId > 0) {
            RecordTargetAlbumMerge(result.replacedTargetId, result.targetId);
        }
        if (!include || source.complete) {
            return;
        }
    }
    if (!source.registered) {
        data_.taskComplete = false;
    }
    source.complete = false;
    for (int32_t id : {previousId, result.replacedTargetId, result.targetId}) {
        if (id > 0) {
            data_.invalidFinalAlbums.insert(id);
            MarkTargetAlbumIncomplete(id);
        }
    }
}

const AnalysisAlbumContextSnapshot &AnalysisAlbumRefreshTask::GetFacts() const
{
    return data_;
}

const AnalysisAlbumBaseline *AnalysisAlbumRefreshTask::FindOriginalAlbum(
    AnalysisAlbumDatabaseSide side, int32_t albumId) const
{
    const auto &albums = side == AnalysisAlbumDatabaseSide::SOURCE ? data_.sourceAlbums : data_.targetAlbums;
    const auto album = albums.find(albumId);
    return album == albums.end() || !album->second.registered ? nullptr : &album->second;
}

namespace {
int32_t MappedId(int32_t id, const std::unordered_map<int32_t, int32_t> &mappings)
{
    const auto found = mappings.find(id);
    return found == mappings.end() ? id : found->second;
}

AnalysisAlbumGroupMembers MigratedGroups(const AnalysisAlbumGroupMembers &original, int64_t offset, int64_t upperBound)
{
    AnalysisAlbumGroupMembers result;
    for (const auto &entry : original.albumGroups) {
        const int64_t id = entry.first <= upperBound ? entry.first + offset : entry.first;
        if (id <= 0 || id > INT32_MAX) {
            return {};
        }
        result.albumGroups.emplace(static_cast<int32_t>(id), entry.second);
        result.groups[entry.second].push_back(static_cast<int32_t>(id));
    }
    for (auto &entry : result.groups) {
        std::sort(entry.second.begin(), entry.second.end());
    }
    return result;
}

} // namespace

bool AnalysisAlbumRefreshTask::MigrateTargetAssets(const std::unordered_map<int32_t, int32_t> &mappings)
{
    CHECK_AND_RETURN_RET(!sealed_ && !targetResultsStarted_, false);
    std::unordered_map<int32_t, AnalysisBeforeCoverage> coverage;
    for (const auto &entry : data_.targetCoverage) {
        const int32_t id = MappedId(entry.first, mappings);
        CHECK_AND_RETURN_RET(id > 0 && coverage.emplace(id, entry.second).second, false);
    }
    decltype(targetCoverOwners_) owners;
    for (const auto &entry : targetCoverOwners_) {
        const int32_t id = MappedId(entry.first, mappings);
        for (auto *album : entry.second) {
            album->coverFileId = id;
        }
        owners.emplace(id, entry.second);
    }
    data_.targetCoverage = std::move(coverage);
    targetCoverOwners_ = std::move(owners);
    return true;
}

void AnalysisAlbumRefreshTask::RecordDeletedTargets(const std::vector<int32_t> &ids)
{
    CHECK_AND_RETURN(!sealed_);
    if (targetResultsStarted_) {
        data_.taskComplete = false;
        return;
    }
    for (int32_t id : ids) {
        data_.targetCoverage.erase(id);
        const auto owners = targetCoverOwners_.find(id);
        CHECK_AND_CONTINUE(owners != targetCoverOwners_.end());
        for (auto *album : owners->second) {
            album->coverFileId = 0;
            album->coverDeleted = true;
        }
        targetCoverOwners_.erase(owners);
    }
}

bool AnalysisAlbumRefreshTask::MigrateTargetAlbums(int64_t offset, int64_t upperBound)
{
    CHECK_AND_RETURN_RET(!sealed_ && !targetResultsStarted_, false);
    CHECK_AND_RETURN_RET(
        std::none_of(data_.sourceAlbums.begin(), data_.sourceAlbums.end(), [](const auto &album) {
            return album.second.binding.finalAlbumId != 0;
        }), false);
    std::unordered_map<int32_t, int32_t> mappings;
    std::unordered_set<int32_t> destinations;
    for (const auto &album : data_.targetAlbums) {
        CHECK_AND_RETURN_RET(album.second.binding.finalAlbumId == 0, false);
        const int64_t original = album.first;
        CHECK_AND_CONTINUE(album.second.registered);
        CHECK_AND_RETURN_RET(original > upperBound ||
            (offset > -original && offset <= std::numeric_limits<int32_t>::max() - original), false);
        const int32_t id = original <= upperBound ? original + offset : original;
        CHECK_AND_RETURN_RET(destinations.insert(id).second, false);
        mappings.emplace(original, id);
    }
    auto groups = MigratedGroups(data_.targetGroups, offset, upperBound);
    CHECK_AND_RETURN_RET(groups.albumGroups.size() == data_.targetGroups.albumGroups.size(), false);
    data_.targetGroups = std::move(groups);
    auto originalAlbums = std::move(data_.targetAlbums);
    data_.targetAlbums.clear();
    while (!originalAlbums.empty()) {
        auto node = originalAlbums.extract(originalAlbums.begin());
        node.key() = MappedId(node.key(), mappings);
        const auto existing = data_.targetAlbums.find(node.key());
        if (existing != data_.targetAlbums.end()) {
            node.mapped().complete = node.mapped().complete && existing->second.complete;
            if (existing->second.registered) {
                existing->second.complete = node.mapped().complete;
                continue;
            }
            data_.targetAlbums.erase(existing);
        }
        data_.targetAlbums.insert(std::move(node));
    }
    return true;
}

const AnalysisAlbumContextSnapshot &AnalysisAlbumRefreshTask::Seal()
{
    sealed_ = true;
    return data_;
}

void AnalysisAlbumRefreshTask::RecordCoverReplacement(int32_t originalId, int32_t finalId, bool conflicting)
{
    const auto owners = targetCoverOwners_.find(originalId);
    CHECK_AND_RETURN(owners != targetCoverOwners_.end());
    for (auto *album : owners->second) {
        album->coverUnknown = album->coverUnknown || conflicting ||
            (album->coverFileId != originalId && album->coverFileId != finalId);
        if (!album->coverUnknown) {
            album->coverFileId = finalId;
        }
    }
}

void AnalysisAlbumRefreshTask::RecordPhotoResults(const std::vector<int32_t> &committedIds,
    const std::unordered_map<int32_t, int32_t> &replacements,
    const std::unordered_set<int32_t> &conflictingDonors)
{
    CHECK_AND_RETURN(!sealed_);
    const auto captured = [this](int32_t id) {
        const auto found = data_.targetCoverage.find(id);
        return id > 0 && found != data_.targetCoverage.end() && found->second != AnalysisBeforeCoverage::UNCAPTURED;
    };
    for (int32_t id : committedIds) {
        data_.taskComplete = data_.taskComplete && captured(id);
    }
    targetResultsStarted_ = targetResultsStarted_ || !committedIds.empty();
    for (const auto &mapping : replacements) {
        CHECK_AND_CONTINUE(mapping.first > 0 && mapping.second > 0 && mapping.first != mapping.second);
        data_.taskComplete = data_.taskComplete && captured(mapping.first) && captured(mapping.second);
        RecordCoverReplacement(mapping.first, mapping.second, conflictingDonors.count(mapping.first) > 0);
        targetResultsStarted_ = true;
    }
}

void AnalysisAlbumRefreshTask::MarkTaskIncomplete()
{
    if (sealed_) {
        return;
    }
    data_.taskComplete = false;
}


bool AnalysisAlbumRefreshTask::PrepareTargetDirectory(AnalysisAlbumGroupMembers groups)
{
    CHECK_AND_RETURN_RET(!sealed_, false);
    std::vector<AnalysisAlbumQueryScope> scopes;
    for (const auto &album : data_.targetAlbums) {
        CHECK_AND_CONTINUE(album.second.registered);
        scopes.push_back({album.first, album.second.subtype, album.second.albumName});
    }
    AnalysisAlbumQueryDirectory directory;
    if (!AnalysisAlbumRefreshDb::PrepareQueryDirectory(scopes, directory)) {
        MarkTaskIncomplete();
        return false;
    }
    data_.targetDirectory = std::move(directory);
    data_.targetGroups = std::move(groups);
    data_.targetDirectoryReady = true;
    return true;
}

bool AnalysisAlbumRefreshTask::InitializeOriginalAlbums(const std::shared_ptr<NativeRdb::RdbStore> &rdb,
    AnalysisAlbumDatabaseSide side)
{
    AnalysisAlbumRefreshDb db(rdb);
    std::vector<AnalysisAlbumMetadata> albums;
    AnalysisAlbumGroupMembers groups;
    if (db.QueryAlbums(albums, true,
        side == AnalysisAlbumDatabaseSide::TARGET ? &groups : nullptr) != E_OK ||
        !RecordOriginalAlbums(albums, side)) {
        MarkTaskIncomplete();
        return false;
    }
    return side != AnalysisAlbumDatabaseSide::TARGET || PrepareTargetDirectory(std::move(groups));
}

AnalysisBeforeCoverage AnalysisAlbumRefreshTask::QueryBefore(AnalysisAlbumRefreshDb &db,
    const std::vector<int32_t> &ids, std::unordered_map<int32_t, int64_t> &counts)
{
    const auto count = [&counts](int32_t album, int32_t) { ++counts[album]; };
    if (db.QueryContributions(data_.targetDirectory, ids, count, data_.targetGroups) == E_OK) {
        return AnalysisBeforeCoverage::CAPTURED;
    }
    counts.clear();
    if (db.QueryContributions(data_.targetDirectory, ids, count, data_.targetGroups, true) != E_OK) {
        data_.taskComplete = false;
    } else {
        for (const auto &entry : counts) {
            data_.targetAlbums.at(entry.first).complete = false;
        }
    }
    MEDIA_WARN_LOG("Analysis before-query failed; ranges=%{public}zu located=%{public}d",
        counts.size(), data_.taskComplete);
    counts.clear();
    return AnalysisBeforeCoverage::UNAVAILABLE;
}

bool AnalysisAlbumRefreshTask::CaptureBefore(const std::shared_ptr<NativeRdb::RdbStore> &rdb,
    const std::vector<int32_t> &fileIds)
{
    CHECK_AND_RETURN_RET(!sealed_ && data_.taskComplete, false);
    data_.taskComplete = data_.targetDirectoryReady;
    CHECK_AND_RETURN_RET(data_.taskComplete, false);
    std::vector<int32_t> ids;
    for (int32_t id : fileIds) {
        if (id > 0 && data_.targetCoverage[id] == AnalysisBeforeCoverage::UNCAPTURED) {
            ids.push_back(id);
        }
    }
    CHECK_AND_RETURN_RET(!ids.empty(), true);
    AnalysisAlbumRefreshDb db(rdb);
    std::unordered_map<int32_t, int64_t> counts;
    auto coverage = QueryBefore(db, ids, counts);
    for (const auto &entry : counts) {
        if (data_.targetAlbums.at(entry.first).capturedCount > INT64_MAX - entry.second) {
            data_.taskComplete = false;
            coverage = AnalysisBeforeCoverage::UNAVAILABLE;
        }
    }
    if (coverage == AnalysisBeforeCoverage::CAPTURED) {
        for (const auto &entry : counts) {
            data_.targetAlbums.at(entry.first).capturedCount += entry.second;
        }
    }
    for (int32_t id : ids) {
        data_.targetCoverage.at(id) = coverage;
    }
    return data_.taskComplete;
}

namespace {
enum class CoverState { UNRESOLVED, VALID, ERROR };

struct CoverCandidate {
    int32_t fileId = 0;
    CoverState state = CoverState::UNRESOLVED;
    AnalysisAlbumContributionState contribution = AnalysisAlbumContributionState::UNKNOWN;
    std::string uri;
    uint8_t satisfied = 0;
    std::tuple<bool, bool, bool, int32_t> priority;
};

struct AnalysisAlbumScope {
    AnalysisAlbumMetadata current;
    bool hasSource = false;
    std::vector<CoverCandidate> candidates;
    bool hasBaseline = false;
    bool hasManual = false;
    bool sourceRetained = false;
    bool complete = true;
    bool affected = false;
    bool hasTarget = false;
    bool currentCoverUsable = true;
    int64_t base = 0;
    int64_t before = 0;
    int64_t after = 0;
    int32_t finalCount = 0;
    const char *coverAction = "candidate";
    size_t nextCover = 0;
    CoverCandidate cover;
};

std::vector<int32_t> AffectedFinalIds(const AnalysisAlbumContextSnapshot &snapshot)
{
    std::vector<int32_t> ids;
    for (const auto &entry : snapshot.targetCoverage) {
        if (entry.second != AnalysisBeforeCoverage::UNCAPTURED) {
            ids.push_back(entry.first);
        }
    }
    return ids;
}

bool IsManualCover(uint8_t satisfied)
{
    return satisfied == static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING) ||
        satisfied == static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING_EDITE);
}

bool UsesSpecializedCover(int32_t subtype)
{
    return subtype == static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT) ||
        subtype == static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS);
}

CoverCandidate ResolveCover(const AnalysisAlbumBaseline &baseline, const AnalysisAlbumScope &scope,
    bool source, int32_t originalId)
{
    CoverCandidate candidate;
    candidate.satisfied = baseline.coverSatisfied;
    // Among retained candidates, prefer manual covers, then the new device (SOURCE).
    candidate.priority = {!IsManualCover(candidate.satisfied), !source,
        !source && scope.current.albumId != originalId, originalId};
    int32_t fileId = baseline.coverFileId;
    CHECK_AND_RETURN_RET(fileId > 0, candidate);
    candidate.state = baseline.coverUnknown ? CoverState::ERROR : CoverState::UNRESOLVED;
    CHECK_AND_RETURN_RET(!IsSingleSideAnalysisSubtype(scope.current.subtype) ||
        candidate.satisfied != static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING), candidate);
    candidate.fileId = fileId;
    return candidate;
}
} // namespace

AnalysisAlbumCountResult AnalysisAlbumRefreshTask::CalculateFinalCount(
    int64_t fixedBase, int64_t beforeSum, int64_t afterSum)
{
    // Validate subtraction first, then bound addition without overflowing int64.
    if (fixedBase < 0 || beforeSum < 0 || beforeSum > fixedBase || afterSum < 0 ||
        fixedBase - beforeSum > INT32_MAX || afterSum > INT32_MAX - (fixedBase - beforeSum)) {
        return {};
    }
    return {true, static_cast<int32_t>(fixedBase - beforeSum + afterSum)};
}

namespace {
struct RefreshData {
    const AnalysisAlbumContextSnapshot &snapshot;
    std::unordered_map<int32_t, AnalysisAlbumScope> scopes;
    AnalysisAlbumContributions finalStates;
    std::unordered_map<int32_t, std::string> photos;
    std::unordered_set<int32_t> validCoverPhotos;
    size_t coverReused = 0;
    size_t coverQueried = 0;
    size_t defaultReselections = 0;
    AnalysisAlbumGroupMembers finalGroups;
};

bool PrepareQueryDirectory(const RefreshData &data, AnalysisAlbumQueryDirectory &directory)
{
    std::vector<AnalysisAlbumQueryScope> queries;
    for (const auto &entry : data.scopes) {
        const auto &scope = entry.second;
        CHECK_AND_CONTINUE(scope.complete && scope.hasBaseline);
        queries.push_back({scope.current.albumId, scope.current.subtype, scope.current.albumName});
    }
    return AnalysisAlbumRefreshDb::PrepareQueryDirectory(queries, directory);
}

int32_t ResolveAlbumDestination(int32_t id, const AnalysisAlbumContextSnapshot &facts,
    std::unordered_map<int32_t, int32_t> &resolved)
{
    std::vector<int32_t> path;
    int32_t finalId = 0;
    while (id > 0) {
        if (!resolved.emplace(id, 0).second) {
            break;
        }
        path.push_back(id);
        const auto next = facts.targetAlbums.find(id);
        if (next == facts.targetAlbums.end() || next->second.binding.finalAlbumId == 0 ||
            next->second.binding.finalAlbumId == id) {
            finalId = id;
            break;
        }
        id = next->second.binding.finalAlbumId;
    }
    if (finalId == 0 && id > 0) {
        finalId = resolved.at(id);
    }
    for (int32_t original : path) {
        resolved.at(original) = finalId;
    }
    return finalId;
}

void LogScopeDecision(const AnalysisAlbumScope &scope, const char *action, const char *reason)
{
    MEDIA_DEBUG_LOG("AccAnalysisRef decision album=%{public}d subtype=%{public}d action=%{public}s "
        "reason=%{public}s old_count=%{public}d base=%{public}lld before=%{public}lld after=%{public}lld "
        "complete=%{public}d baseline=%{public}d source=%{public}d target=%{public}d affected=%{public}d",
        scope.current.albumId, scope.current.subtype, action, reason, scope.current.count,
        static_cast<long long>(scope.base), static_cast<long long>(scope.before),
        static_cast<long long>(scope.after), scope.complete, scope.hasBaseline,
        scope.hasSource, scope.hasTarget, scope.affected);
}

void AddOriginalAlbum(RefreshData &data, const AnalysisAlbumBaseline &album, bool source,
    int32_t originalId, int32_t finalId)
{
    const auto current = data.scopes.find(finalId);
    CHECK_AND_RETURN(source || current != data.scopes.end());
    const bool exists = current != data.scopes.end();
    auto &scope = data.scopes[finalId];
    scope.current.albumId = finalId;
    scope.complete = scope.complete && exists && scope.current.subtype == album.subtype &&
        data.snapshot.invalidFinalAlbums.count(finalId) == 0 &&
        (album.subtype != PhotoAlbumSubType::SHOOTING_MODE || album.albumName == scope.current.albumName);
    scope.affected = scope.affected || (!source && album.coverDeleted);
    scope.currentCoverUsable = scope.currentCoverUsable && (source || !album.coverDeleted);
    scope.hasSource = scope.hasSource || source;
    MEDIA_DEBUG_LOG("AccAnalysisRef baseline album=%{public}d original=%{public}d source=%{public}d "
        "subtype=%{public}d count=%{public}lld captured=%{public}lld complete=%{public}d included=%{public}d",
        finalId, originalId, source, album.subtype, static_cast<long long>(album.count),
        static_cast<long long>(album.capturedCount), album.complete,
        album.binding.includeBaseline && (source || !scope.sourceRetained));
    CHECK_AND_RETURN(album.binding.includeBaseline && (source || !scope.sourceRetained));
    scope.hasBaseline = true;
    scope.hasManual = scope.hasManual || IsManualCover(album.coverSatisfied);
    scope.sourceRetained = scope.sourceRetained || (source && IsSingleSideAnalysisSubtype(album.subtype));
    auto candidate = ResolveCover(album, scope, source, originalId);
    if (candidate.fileId > 0 || candidate.state == CoverState::ERROR) {
        data.finalStates[{finalId, candidate.fileId}] = AnalysisAlbumContributionState::UNKNOWN;
        scope.candidates.push_back(std::move(candidate));
    }
    if (!album.complete || album.count < 0 || scope.base > INT64_MAX - album.count) {
        scope.complete = false;
        return;
    }
    scope.affected = scope.affected || (source ? album.count : album.capturedCount) > 0;
    if (!source) {
        scope.base += album.count;
        scope.before += album.capturedCount;
        scope.hasTarget = true;
    }
}

void BuildFinalScopes(RefreshData &data)
{
    const auto &facts = data.snapshot;
    std::unordered_map<int32_t, int32_t> destinations;
    // Select the retained source side before accumulating target baselines.
    for (auto side : {AnalysisAlbumDatabaseSide::SOURCE, AnalysisAlbumDatabaseSide::TARGET}) {
        const auto &albums = side == AnalysisAlbumDatabaseSide::SOURCE ? facts.sourceAlbums : facts.targetAlbums;
        for (const auto &item : albums) {
            const auto &binding = item.second.binding;
            CHECK_AND_CONTINUE(item.second.registered &&
                (side == AnalysisAlbumDatabaseSide::TARGET || binding.finalAlbumId > 0));
            const int32_t id = ResolveAlbumDestination(binding.finalAlbumId == 0 ?
                item.first : binding.finalAlbumId, facts, destinations);
            if (id > 0) {
                AddOriginalAlbum(data, item.second, side == AnalysisAlbumDatabaseSide::SOURCE, item.first, id);
                continue;
            }
            for (int32_t unresolved : {item.first, binding.finalAlbumId}) {
                const auto scope = data.scopes.find(unresolved);
                if (scope != data.scopes.end()) {
                    scope->second.complete = false;
                }
            }
        }
    }
    for (auto &entry : data.scopes) {
        entry.second.complete = entry.second.complete && entry.second.hasBaseline;
    }
}

void ValidateFinalGroups(RefreshData &data)
{
    CHECK_AND_RETURN(data.snapshot.targetDirectoryReady);
    const auto &original = data.snapshot.targetGroups;
    std::unordered_set<int32_t> changed;
    for (const auto &group : original.groups) {
        const auto final = data.finalGroups.groups.find(group.first);
        CHECK_AND_CONTINUE(final == data.finalGroups.groups.end() || final->second != group.second);
        changed.insert(group.second.begin(), group.second.end());
        if (final != data.finalGroups.groups.end()) {
            changed.insert(final->second.begin(), final->second.end());
        }
    }
    for (const auto &group : data.finalGroups.groups) {
        CHECK_AND_CONTINUE(original.groups.count(group.first) == 0 &&
            std::any_of(group.second.begin(), group.second.end(), [&data](int32_t id) {
                const auto album = data.snapshot.targetAlbums.find(id);
                return album != data.snapshot.targetAlbums.end() && album->second.registered;
            }));
        changed.insert(group.second.begin(), group.second.end());
    }
    for (auto &scope : data.scopes) {
        if (IsSingleSideAnalysisSubtype(scope.second.current.subtype) && changed.count(scope.first) > 0) {
            LogScopeDecision(scope.second, "fallback", "group_members_changed");
            scope.second.complete = false;
        }
    }
}

void AccumulateCount(RefreshData &data, int32_t albumId, int32_t fileId)
{
    auto &scope = data.scopes.at(albumId);
    ++scope.after;
    scope.affected = true;
    if (scope.hasTarget) {
        const auto target = data.snapshot.targetCoverage.find(fileId);
        if (scope.complete && (!data.snapshot.targetDirectoryReady ||
            target == data.snapshot.targetCoverage.end() || target->second != AnalysisBeforeCoverage::CAPTURED)) {
            MEDIA_WARN_LOG("AccAnalysisRef coverage album=%{public}d asset=%{public}d reason=missing_before",
                albumId, fileId);
        }
        scope.complete = scope.complete && data.snapshot.targetDirectoryReady &&
            target != data.snapshot.targetCoverage.end() && target->second == AnalysisBeforeCoverage::CAPTURED;
    }
    const auto cover = data.finalStates.find({albumId, fileId});
    if (cover != data.finalStates.end()) {
        cover->second = AnalysisAlbumContributionState::COUNTED;
    }
}

bool QueryRefreshData(AnalysisAlbumRefreshDb &db, RefreshData &data)
{
    std::vector<AnalysisAlbumMetadata> albums;
    CHECK_AND_RETURN_RET(db.QueryAlbums(albums, false, &data.finalGroups) == E_OK, false);
    for (auto &album : albums) {
        data.finalStates[{album.albumId, album.CoverFileId()}] = AnalysisAlbumContributionState::UNKNOWN;
        data.scopes[album.albumId].current = std::move(album);
    }
    BuildFinalScopes(data);
    AnalysisAlbumQueryDirectory targetDirectory;
    CHECK_AND_RETURN_RET(PrepareQueryDirectory(data, targetDirectory), false);
    const auto finalIds = AffectedFinalIds(data.snapshot);
    const bool targetComplete = db.QueryContributions(targetDirectory, finalIds,
        [&data](int32_t album, int32_t file) { AccumulateCount(data, album, file); }, data.finalGroups) == E_OK;
    CHECK_AND_RETURN_RET(targetComplete, false);
    ValidateFinalGroups(data);
    const std::unordered_set<int32_t> queried(finalIds.begin(), finalIds.end());
    for (auto &entry : data.finalStates) {
        if (queried.count(entry.first.fileId) > 0 &&
            entry.second == AnalysisAlbumContributionState::UNKNOWN) {
            entry.second = AnalysisAlbumContributionState::NOT_COUNTED;
        }
    }
    for (auto it = data.scopes.begin(); it != data.scopes.end();) {
        const auto &scope = it->second;
        const bool untouched = scope.complete && !scope.hasSource && !scope.affected &&
            scope.current.count == scope.base &&
            std::none_of(scope.candidates.begin(), scope.candidates.end(), [&queried](const auto &cover) {
                return queried.count(cover.fileId) > 0;
            });
        if (untouched) {
            LogScopeDecision(scope, "skip", "unaffected");
        }
        it = untouched ? data.scopes.erase(it) : std::next(it);
    }
    MEDIA_INFO_LOG("Analysis refresh target_ids=%{public}zu tracked_targets=%{public}zu",
        finalIds.size(), data.snapshot.targetCoverage.size());
    return true;
}

void PrepareCovers(AnalysisAlbumScope &scope)
{
    if (scope.finalCount == 0 || UsesSpecializedCover(scope.current.subtype)) {
        scope.coverAction = UsesSpecializedCover(scope.current.subtype) ? "specialized_keep" : "clear_empty";
        scope.cover.state = CoverState::VALID;
        return;
    }
    // Merged albums and portrait/group albums retain an established cover while it remains valid.
    if (scope.affected && !scope.hasManual && (!scope.hasSource || !scope.hasTarget) &&
        !IsSingleSideAnalysisSubtype(scope.current.subtype)) {
        scope.candidates.clear();
        return;
    }
    std::sort(scope.candidates.begin(), scope.candidates.end(), [](const auto &left, const auto &right) {
        return left.priority < right.priority;
    });
    CHECK_AND_RETURN(scope.currentCoverUsable);
    CoverCandidate current;
    current.satisfied = scope.current.coverSatisfied;
    if (!IsSingleSideAnalysisSubtype(scope.current.subtype) ||
        current.satisfied != static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING)) {
        current.fileId = scope.current.CoverFileId();
    }
    scope.candidates.push_back(std::move(current));
}

std::vector<AnalysisAlbumScope *> CalculateScopes(RefreshData &data, std::unordered_set<int32_t> &failed)
{
    std::vector<AnalysisAlbumScope *> results;
    for (auto &item : data.scopes) {
        auto &scope = item.second;
        const auto count = AnalysisAlbumRefreshTask::CalculateFinalCount(scope.base, scope.before, scope.after);
        if (scope.complete && count.complete) {
            scope.finalCount = count.finalCount;
            scope.affected = scope.affected || !scope.hasSource;
            PrepareCovers(scope);
            results.push_back(&scope);
        } else {
            LogScopeDecision(scope, "fallback", scope.complete ? "invalid_count_arithmetic" : "incomplete_scope");
            failed.insert(scope.current.albumId);
        }
    }
    return results;
}

CoverCandidate CachedDefaultCover(AnalysisAlbumRefreshDb &db,
    const AnalysisAlbumScope &scope, const AnalysisAlbumGroupMembers &groups,
    std::unordered_map<std::string, CoverCandidate> &cache)
{
    int32_t id = scope.current.albumId;
    if (IsSingleSideAnalysisSubtype(scope.current.subtype)) {
        const auto group = groups.albumGroups.find(id);
        CHECK_AND_EXECUTE(group == groups.albumGroups.end(),
            id = groups.groups.at(group->second).front());
    }
    const std::string key = std::to_string(scope.current.subtype) + ":" + std::to_string(id);
    const auto found = cache.find(key);
    CHECK_AND_RETURN_RET(found == cache.end(), found->second);
    CoverCandidate cover;
    cover.state = db.QueryDefaultCover(
        {scope.current.albumId, scope.current.subtype, scope.current.albumName}, cover.uri) == E_OK ?
        CoverState::VALID : CoverState::ERROR;
    cover.satisfied = static_cast<uint8_t>(cover.uri.empty() ? CoverSatisfiedType::NO_SETTING :
        CoverSatisfiedType::DEFAULT_SETTING);
    CHECK_AND_PRINT_LOG(cover.state == CoverState::ERROR || !cover.uri.empty(),
        "No eligible analysis cover subtype=%{public}d",
        scope.current.subtype);
    return cache.emplace(key, std::move(cover)).first->second;
}

bool QueryRoundRelations(AnalysisAlbumRefreshDb &db,
    const std::vector<AnalysisAlbumScope *> &round, RefreshData &data)
{
    std::vector<AnalysisAlbumQueryScope> scopes;
    std::vector<int32_t> ids;
    std::unordered_map<int32_t, CoverCandidate *> missing;
    for (auto *scope : round) {
        auto &cover = scope->cover;
        cover.contribution = data.finalStates.at({scope->current.albumId, cover.fileId});
        if (cover.contribution != AnalysisAlbumContributionState::UNKNOWN) {
            ++data.coverReused;
            continue;
        }
        cover.contribution = AnalysisAlbumContributionState::NOT_COUNTED;
        scopes.push_back({scope->current.albumId, scope->current.subtype, scope->current.albumName});
        ids.push_back(cover.fileId);
        missing.emplace(scope->current.albumId, &cover);
    }
    CHECK_AND_RETURN_RET(!missing.empty(), true);
    AnalysisAlbumQueryDirectory directory;
    data.coverQueried += missing.size();
    return AnalysisAlbumRefreshDb::PrepareQueryDirectory(scopes, directory) &&
        db.QueryContributions(directory, ids, [&missing](int32_t album, int32_t file) {
            const auto found = missing.find(album);
            if (found != missing.end() && found->second->fileId == file) {
                found->second->contribution = AnalysisAlbumContributionState::COUNTED;
            }
        }, data.finalGroups) == E_OK;
}

void ValidateCoverRound(AnalysisAlbumRefreshDb &db,
    const std::vector<AnalysisAlbumScope *> &round, RefreshData &data)
{
    bool complete = QueryRoundRelations(db, round, data);
    std::unordered_set<int32_t> missing;
    for (const auto *scope : round) {
        const auto &cover = scope->cover;
        if (cover.contribution == AnalysisAlbumContributionState::COUNTED && data.photos.count(cover.fileId) == 0) {
            missing.insert(cover.fileId);
        }
    }
    std::unordered_map<int32_t, std::string> photos;
    std::unordered_set<int32_t> validCoverPhotos;
    complete = complete && db.QueryPhotoUris({missing.begin(), missing.end()}, photos, &validCoverPhotos) == E_OK;
    data.photos.merge(photos);
    data.validCoverPhotos.merge(validCoverPhotos);
    for (auto *scope : round) {
        auto &cover = scope->cover;
        const auto photo = data.photos.find(cover.fileId);
        const bool valid = cover.contribution == AnalysisAlbumContributionState::COUNTED &&
            photo != data.photos.end() && !photo->second.empty() &&
            (!IsSingleSideAnalysisSubtype(scope->current.subtype) || data.validCoverPhotos.count(cover.fileId) > 0);
        cover.state = !complete ? CoverState::ERROR : (valid ? CoverState::VALID : CoverState::UNRESOLVED);
        MEDIA_DEBUG_LOG("AccAnalysisRef candidate album=%{public}d asset=%{public}d query_ok=%{public}d "
            "counted=%{public}d uri_present=%{public}d eligible=%{public}d accepted=%{public}d",
            scope->current.albumId, cover.fileId, complete,
            cover.contribution == AnalysisAlbumContributionState::COUNTED,
            photo != data.photos.end() && !photo->second.empty(),
            data.validCoverPhotos.count(cover.fileId) > 0, complete && valid);
        if (complete) {
            data.finalStates.at({scope->current.albumId, cover.fileId}) = cover.contribution;
        }
        CHECK_AND_EXECUTE(cover.state != CoverState::VALID, cover.uri = photo->second);
    }
}

void ResolveScopeCovers(AnalysisAlbumRefreshDb &db,
    RefreshData &data, const std::vector<AnalysisAlbumScope *> &results)
{
    std::unordered_map<std::string, CoverCandidate> defaults;
    bool hasUnresolved = true;
    while (hasUnresolved) {
        std::vector<AnalysisAlbumScope *> round;
        for (auto *result : results) {
            CHECK_AND_CONTINUE(result->cover.state == CoverState::UNRESOLVED);
            if (result->nextCover == result->candidates.size()) {
                result->coverAction = "default_select";
                ++data.defaultReselections;
                result->cover = CachedDefaultCover(db, *result, data.finalGroups, defaults);
                continue;
            }
            result->cover = result->candidates[result->nextCover++];
            CHECK_AND_EXECUTE(result->cover.state == CoverState::ERROR || result->cover.fileId <= 0,
                round.push_back(result));
        }
        if (!round.empty()) {
            ValidateCoverRound(db, round, data);
        }
        hasUnresolved = std::any_of(results.begin(), results.end(), [](const auto *r) {
            return r->cover.state == CoverState::UNRESOLVED;
        });
    }
}

void LogPreciseResult(const AnalysisAlbumScope &scope, bool writeNeeded, int32_t ret)
{
    const auto &current = scope.current;
    AnalysisAlbumMetadata proposed;
    const bool keepCover = UsesSpecializedCover(current.subtype) ||
        (scope.finalCount > 0 && scope.cover.state != CoverState::VALID);
    proposed.coverUri = keepCover ? current.coverUri : scope.cover.uri;
    MEDIA_DEBUG_LOG("AccAnalysisRef precise album=%{public}d subtype=%{public}d ret=%{public}d "
        "write=%{public}d old_count=%{public}d proposed_count=%{public}d base=%{public}lld "
        "before=%{public}lld after=%{public}lld cover_action=%{public}s cover_valid=%{public}d "
        "old_cover_id=%{public}d proposed_cover_id=%{public}d uri_changed=%{public}d "
        "old_satisfied=%{public}d proposed_satisfied=%{public}d",
        current.albumId, current.subtype, ret, writeNeeded, current.count, scope.finalCount,
        static_cast<long long>(scope.base), static_cast<long long>(scope.before),
        static_cast<long long>(scope.after), scope.coverAction, scope.cover.state == CoverState::VALID,
        current.CoverFileId(), proposed.CoverFileId(), current.coverUri != proposed.coverUri,
        static_cast<int32_t>(current.coverSatisfied), keepCover ?
        static_cast<int32_t>(current.coverSatisfied) : static_cast<int32_t>(scope.cover.satisfied));
}

bool CommitScope(AnalysisAlbumRefreshDb &db, const AnalysisAlbumScope &scope,
    std::unordered_set<int32_t> &changed)
{
    const auto &current = scope.current;
    const auto &cover = scope.cover;
    const int32_t count = scope.finalCount;
    NativeRdb::ValuesBucket values;
    CHECK_AND_EXECUTE(current.count == count, values.PutInt(PhotoAlbumColumns::ALBUM_COUNT, count));
    if (!UsesSpecializedCover(current.subtype)) {
        if (count == 0 && (!current.coverUri.empty() || current.coverSatisfied != 0)) {
            values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, "");
            values.PutInt(ANALYSIS_COL_IS_COVER_SATISFIED, static_cast<int32_t>(CoverSatisfiedType::NO_SETTING));
        } else if (count > 0 && cover.state == CoverState::VALID &&
            (current.coverUri != cover.uri || current.coverSatisfied != cover.satisfied)) {
            values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, cover.uri);
            values.PutInt(ANALYSIS_COL_IS_COVER_SATISFIED, cover.satisfied);
        }
    }
    const int32_t updateRet = db.UpdateAlbum(current.albumId, values);
    LogPreciseResult(scope, !values.IsEmpty(), updateRet);
    if (updateRet != E_OK || cover.state != CoverState::VALID) {
        LogScopeDecision(scope, "fallback", updateRet != E_OK ? "write_failed" : "cover_query_failed");
    }
    CHECK_AND_RETURN_RET(updateRet == E_OK, false);
    CHECK_AND_EXECUTE(current.count == count, changed.insert(current.albumId));
    return cover.state == CoverState::VALID;
}

} // namespace

AnalysisAlbumRefreshResult AnalysisAlbumRefreshTask::Refresh(
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb)
{
    const auto &facts = Seal();
    AnalysisAlbumRefreshDb db(targetRdb);
    AnalysisAlbumRefreshResult refresh;
    RefreshData data {facts};
    if (targetRdb == nullptr || !facts.taskComplete ||
        !QueryRefreshData(db, data)) {
        MEDIA_WARN_LOG("AccAnalysisRef task action=fallback_all reason=%{public}s source_albums=%{public}zu "
            "target_albums=%{public}zu", targetRdb == nullptr ? "null_database" :
            (!facts.taskComplete ? "incomplete_task" : "refresh_data_query_failed"),
            facts.sourceAlbums.size(), facts.targetAlbums.size());
        refresh.error = E_HAS_DB_ERROR;
        refresh.fullRefresh = true;
        return refresh;
    }
    auto &failedAlbumIds = refresh.failedAlbumIds;
    auto results = CalculateScopes(data, failedAlbumIds);
    ResolveScopeCovers(db, data, results);
    for (const auto *scope : results) {
        if (!CommitScope(db, *scope, refresh.changedAlbumIds)) {
            failedAlbumIds.insert(scope->current.albumId);
        }
    }
    // Keep the log field stable; default_queries counts reselections, including cache hits.
    MEDIA_INFO_LOG("Analysis covers reused_keys=%{public}zu queried_keys=%{public}zu photos=%{public}zu "
        "default_queries=%{public}zu", data.coverReused, data.coverQueried, data.photos.size(),
        data.defaultReselections);
    refresh.error = failedAlbumIds.empty() ? E_OK : E_HAS_DB_ERROR;
    return refresh;
}

} // namespace OHOS::Media
