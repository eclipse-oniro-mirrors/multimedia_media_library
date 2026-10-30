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

#include "analysis_album_refresh_db.h"

#include <algorithm>
#include <functional>
#include <map>
#include <unordered_set>

#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "shooting_mode_column.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
namespace {
using NativeRdb::RdbPredicates;

std::string AddPlaceholders(size_t count)
{
    std::string result;
    for (size_t index = 0; index < count; ++index) {
        result += result.empty() ? "?" : ",?";
    }
    return result;
}

std::vector<std::string> BatchArguments(const std::vector<int32_t> &ids, size_t offset)
{
    const size_t end = std::min(offset + ANALYSIS_REFRESH_QUERY_BATCH_SIZE, ids.size());
    std::vector<std::string> args;
    for (size_t index = offset; index < end; ++index) {
        args.push_back(std::to_string(ids[index]));
    }
    return args;
}

bool ReadIdRows(const std::shared_ptr<NativeRdb::RdbStore> &rdb, const std::vector<int32_t> &ids,
    const std::string &prefix, const std::function<bool(const std::shared_ptr<NativeRdb::ResultSet> &)> &consume,
    const std::string &suffix = ")")
{
    CHECK_AND_RETURN_RET(rdb != nullptr, false);
    for (size_t offset = 0; offset < ids.size(); offset += ANALYSIS_REFRESH_QUERY_BATCH_SIZE) {
        const auto args = BatchArguments(ids, offset);
        auto rows = rdb->QuerySql(prefix + AddPlaceholders(args.size()) + suffix, args);
        CHECK_AND_RETURN_RET(ResultSetUtils::ReadRows(rows, consume), false);
    }
    return true;
}

const std::string ALBUM_QUERY = "SELECT album_id, album_subtype, count, album_name, cover_uri, "
    "is_cover_satisfied, group_tag FROM AnalysisAlbum ORDER BY album_id";

bool ReadAlbum(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, AnalysisAlbumMetadata &metadata)
{
    int32_t satisfied = -1;
    return resultSet != nullptr && ResultSetUtils::TryGetValue(ANALYSIS_COL_ALBUM_ID, resultSet, metadata.albumId) &&
        metadata.albumId > 0 && ResultSetUtils::TryGetValue(ANALYSIS_COL_ALBUM_SUBTYPE, resultSet, metadata.subtype) &&
        ResultSetUtils::TryGetValue(ANALYSIS_COL_COUNT, resultSet, metadata.count) &&
        ResultSetUtils::TryGetValue(ANALYSIS_COL_ALBUM_NAME, resultSet, metadata.albumName) &&
        ResultSetUtils::TryGetValue(ANALYSIS_COL_COVER_URI, resultSet, metadata.coverUri) &&
        ResultSetUtils::TryGetValue(ANALYSIS_COL_IS_COVER_SATISFIED, resultSet, satisfied) && satisfied >= 0 &&
        satisfied <= static_cast<int32_t>(CoverSatisfiedType::BEAUTY_SETTING_EDITE) &&
        ((metadata.coverSatisfied = static_cast<uint8_t>(satisfied)), true);
}

} // namespace

int32_t AnalysisAlbumRefreshDb::QueryAlbums(std::vector<AnalysisAlbumMetadata> &albums, bool supportedOnly,
    AnalysisAlbumGroupMembers *members) const
{
    albums.clear();
    CHECK_AND_RETURN_RET(rdb_ != nullptr, E_INVALID_ARGUMENTS);
    std::vector<AnalysisAlbumMetadata> queried;
    AnalysisAlbumGroupMembers groups;
    const bool complete = ResultSetUtils::ReadRows(rdb_->QuerySql(ALBUM_QUERY), [&](const auto &row) {
        AnalysisAlbumMetadata album;
        if (members != nullptr) {
            bool isNull = true;
            int32_t groupTagColumn = -1;
            std::string tag;
            CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(ANALYSIS_COL_ALBUM_ID, row, album.albumId) &&
                album.albumId > 0 &&
                row->GetColumnIndex(ANALYSIS_COL_GROUP_TAG, groupTagColumn) == E_OK &&
                row->IsColumnNull(groupTagColumn, isNull) == E_OK, false);
            if (!isNull) {
                CHECK_AND_RETURN_RET(row->GetString(groupTagColumn, tag) == E_OK, false);
                groups.albumGroups.emplace(album.albumId, tag);
                groups.groups[tag].push_back(album.albumId);
            }
        }
        CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(ANALYSIS_COL_ALBUM_SUBTYPE, row, album.subtype), false);
        CHECK_AND_RETURN_RET(!supportedOnly || IsSupportedAnalysisSubtype(album.subtype), true);
        CHECK_AND_RETURN_RET(ReadAlbum(row, album), false);
        queried.push_back(std::move(album));
        return true;
    });
    CHECK_AND_RETURN_RET(complete, E_HAS_DB_ERROR);
    albums = std::move(queried);
    if (members != nullptr) {
        *members = std::move(groups);
    }
    return E_OK;
}

int32_t AnalysisAlbumRefreshDb::QueryCloudCleanupAssetIds(std::vector<int32_t> &fileIds) const
{
    fileIds.clear();
    CHECK_AND_RETURN_RET(rdb_ != nullptr, E_HAS_DB_ERROR);
    // Cloud marking changes clean_flag; the cleaner also deletes pre-existing dirty=-1 rows.
    // Include already marked cloud rows so partial cleanup is evaluated from the final database.
    auto rows = rdb_->QuerySql("SELECT file_id FROM Photos WHERE position = 2 OR dirty = -1",
        std::vector<std::string> {});
    std::vector<int32_t> queried;
    const bool complete = ResultSetUtils::ReadRows(rows, [&queried](const auto &row) {
        int32_t id = 0;
        CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_ID, row, id) && id > 0, false);
        queried.push_back(id);
        return true;
    });
    CHECK_AND_RETURN_RET(complete, E_HAS_DB_ERROR);
    fileIds = std::move(queried);
    return E_OK;
}

int32_t AnalysisAlbumRefreshDb::QueryAssetIds(const std::vector<int32_t> &ids,
    AnalysisAssetQuery selection, std::vector<int32_t> &fileIds) const
{
    fileIds.clear();
    std::string prefix = "SELECT file_id FROM Photos WHERE file_id IN (";
    std::string suffix = ")";
    if (selection == AnalysisAssetQuery::ALBUM_MEMBERS) {
        prefix = "SELECT DISTINCT map_asset AS file_id FROM AnalysisPhotoMap WHERE map_album IN (";
    } else if (selection == AnalysisAssetQuery::DELETION_WITH_BURST) {
        prefix = "WITH requested AS (SELECT file_id, burst_key FROM Photos WHERE file_id IN (";
        suffix = ")) SELECT file_id FROM Photos WHERE file_id IN (SELECT file_id FROM requested)"
            " OR burst_key IN (SELECT burst_key FROM requested WHERE burst_key IS NOT NULL AND burst_key != '')";
    }
    std::unordered_set<int32_t> queried;
    const bool complete = ReadIdRows(rdb_, ids, prefix, [&queried](const auto &row) {
        int32_t id = 0;
        CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_ID, row, id) && id > 0, false);
        queried.insert(id);
        return true;
    }, suffix);
    CHECK_AND_RETURN_RET(complete, E_HAS_DB_ERROR);
    fileIds.assign(queried.begin(), queried.end());
    return E_OK;
}

namespace {
struct ContributionQuery {
    const std::shared_ptr<NativeRdb::RdbStore> &rdb;
    const std::function<void(int32_t, int32_t)> &consume;
    bool membershipOnly;
    std::unordered_set<int32_t> fileSet;
    std::vector<int32_t> files;
    const AnalysisAlbumQueryDirectory &directory;
    const AnalysisAlbumGroupMembers &members;
    size_t relationQueries = 0;
    size_t shootingQueries = 0;
    std::unordered_set<AnalysisAlbumAssetKey, AnalysisAlbumAssetKeyHash> groupedAssets = {};
};

bool ReadRelationContribution(const std::shared_ptr<NativeRdb::ResultSet> &row,
    ContributionQuery &query, const AnalysisAlbumGroupMembers &members)
{
    int32_t album = 0;
    int32_t id = 0;
    CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue("map_album", row, album) &&
        ResultSetUtils::TryGetValue("map_asset", row, id) && query.fileSet.count(id) > 0, false);
    if (query.directory.ordinary.count(album) > 0) {
        query.consume(album, id);
    }
    const auto found = members.albumGroups.find(album);
    CHECK_AND_RETURN_RET(found != members.albumGroups.end(), true);
    const auto &group = members.groups.at(found->second);
    // Each asset may have several relations in the same group. Expand it only once per batch.
    CHECK_AND_RETURN_RET(query.groupedAssets.insert({group.front(), id}).second, true);
    for (int32_t root : group) {
        CHECK_AND_CONTINUE(query.directory.grouped.count(root) > 0);
        query.consume(root, id);
    }
    return true;
}

bool QueryRelationContributions(ContributionQuery &query)
{
    CHECK_AND_RETURN_RET(!query.directory.ordinary.empty() || !query.directory.grouped.empty(), true);
    const auto &members = query.members;
    const std::string filter = query.membershipOnly ? "" : " AND " +
        MediaLibraryRdbUtils::GetAnalysisPhotoCountClause();
    for (size_t offset = 0; offset < query.files.size(); offset += ANALYSIS_REFRESH_QUERY_BATCH_SIZE) {
        query.groupedAssets.clear();
        const auto args = BatchArguments(query.files, offset);
        ++query.relationQueries;
        auto rows = query.rdb->QuerySql("SELECT DISTINCT m.map_album, m.map_asset"
            " FROM AnalysisPhotoMap m INNER JOIN Photos ON Photos.file_id = m.map_asset WHERE m.map_asset IN (" +
            AddPlaceholders(args.size()) + ")" + filter, args);
        const bool complete = ResultSetUtils::ReadRows(rows, [&query, &members](const auto &row) {
            return ReadRelationContribution(row, query, members);
        });
        CHECK_AND_RETURN_RET(complete, false);
    }
    return true;
}

bool ReadShootingMembership(const std::shared_ptr<NativeRdb::ResultSet> &row, ContributionQuery &query)
{
    int32_t id = 0;
    int32_t subtype = 0;
    int32_t effect = 0;
    std::string mime;
    std::string front;
    std::string mode;
    CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_ID, row, id) && query.fileSet.count(id) > 0 &&
        ResultSetUtils::TryGetValue(PhotoColumn::PHOTO_SUBTYPE, row, subtype) &&
        ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_MIME_TYPE, row, mime) &&
        ResultSetUtils::TryGetValue(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, row, effect) &&
        ResultSetUtils::TryGetValue(PhotoColumn::PHOTO_FRONT_CAMERA, row, front) &&
        ResultSetUtils::TryGetValue(PhotoColumn::PHOTO_SHOOTING_MODE, row, mode), false);
    const auto modes = ShootingModeAlbum::GetShootingModeAlbumOfAsset(subtype, mime, effect, front, mode);
    for (const auto &batch : query.directory.shootingQueries) {
        for (size_t index = 0; index < batch.modes.size(); ++index) {
            const auto type = static_cast<ShootingModeAlbumType>(batch.modes[index]);
            CHECK_AND_CONTINUE(std::find(modes.begin(), modes.end(), type) != modes.end());
            for (int32_t album : batch.albums[index]) {
                query.consume(album, id);
            }
        }
    }
    return true;
}

bool QueryShootingMemberships(ContributionQuery &query)
{
    for (size_t offset = 0; offset < query.files.size(); offset += ANALYSIS_REFRESH_QUERY_BATCH_SIZE) {
        const auto args = BatchArguments(query.files, offset);
        ++query.shootingQueries;
        auto rows = query.rdb->QuerySql("SELECT file_id, subtype, mime_type, moving_photo_effect_mode, "
            "front_camera, shooting_mode FROM Photos WHERE file_id IN (" + AddPlaceholders(args.size()) + ")", args);
        const bool complete = ResultSetUtils::ReadRows(rows, [&query](const auto &row) {
            return ReadShootingMembership(row, query);
        });
        CHECK_AND_RETURN_RET(complete, false);
    }
    return true;
}

bool PrepareShootingQueries(const std::map<ShootingModeAlbumType, std::vector<int32_t>> &modes,
    std::vector<AnalysisAlbumShootingQuery> &batches)
{
    for (const auto &mode : modes) {
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        ShootingModeAlbum::GetShootingModeAlbumPredicates(mode.first, predicates, false);
        const auto &where = predicates.GetWhereClause();
        const auto &args = predicates.GetWhereArgs();
        CHECK_AND_RETURN_RET(!where.empty() && args.size() < ANALYSIS_REFRESH_SQL_VARIABLE_LIMIT, false);
        // Keep at least one bind for file_id, and bound the projected columns independently.
        if (batches.empty() || batches.back().arguments.size() + args.size() >= ANALYSIS_REFRESH_SQL_VARIABLE_LIMIT ||
            batches.back().albums.size() >= ANALYSIS_REFRESH_QUERY_BATCH_SIZE) {
            batches.emplace_back();
        }
        auto &batch = batches.back();
        batch.projection += ", CASE WHEN (" + where + ") THEN 1 ELSE 0 END AS mode_" +
            std::to_string(batch.albums.size());
        batch.arguments.insert(batch.arguments.end(), args.begin(), args.end());
        batch.modes.push_back(static_cast<int32_t>(mode.first));
        batch.albums.push_back(mode.second);
    }
    return true;
}

bool ReadShootingContribution(const std::shared_ptr<NativeRdb::ResultSet> &row,
    ContributionQuery &query, const AnalysisAlbumShootingQuery &batch, std::vector<int32_t> &modeColumns)
{
    int32_t id = 0;
    CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_ID, row, id) &&
        query.fileSet.count(id) > 0, false);
    for (size_t index = 0; index < batch.albums.size(); ++index) {
        if (modeColumns[index] < 0) {
            int32_t column = -1;
            CHECK_AND_RETURN_RET(row->GetColumnIndex("mode_" + std::to_string(index), column) == E_OK, false);
            modeColumns[index] = column;
        }
        int32_t counted = 0;
        CHECK_AND_RETURN_RET(row->GetInt(modeColumns[index], counted) == NativeRdb::E_OK, false);
        CHECK_AND_CONTINUE(counted == 1);
        for (int32_t albumId : batch.albums[index]) {
            query.consume(albumId, id);
        }
    }
    return true;
}

bool QueryShootingContributions(ContributionQuery &query)
{
    CHECK_AND_RETURN_RET(!query.directory.shootingQueries.empty(), true);
    CHECK_AND_RETURN_RET(!query.membershipOnly, QueryShootingMemberships(query));
    for (const auto &batch : query.directory.shootingQueries) {
        const size_t size = std::min(ANALYSIS_REFRESH_QUERY_BATCH_SIZE,
            ANALYSIS_REFRESH_SQL_VARIABLE_LIMIT - batch.arguments.size());
        for (size_t offset = 0; offset < query.files.size(); offset += size) {
            auto args = batch.arguments;
            const size_t end = std::min(query.files.size(), offset + size);
            for (size_t index = offset; index < end; ++index) {
                args.push_back(std::to_string(query.files[index]));
            }
            ++query.shootingQueries;
            auto rows = query.rdb->QuerySql("SELECT file_id" + batch.projection +
                " FROM Photos WHERE file_id IN (" + AddPlaceholders(end - offset) + ")", args);
            std::vector<int32_t> modeColumns(batch.albums.size(), -1);
            const bool complete = ResultSetUtils::ReadRows(rows, [&query, &batch, &modeColumns](const auto &row) {
                return ReadShootingContribution(row, query, batch, modeColumns);
            });
            CHECK_AND_RETURN_RET(complete, false);
        }
    }
    return true;
}
} // namespace

bool AnalysisAlbumRefreshDb::QueryMigratedPhotoIds(const std::unordered_map<int32_t, int32_t> &expected,
    std::unordered_map<int32_t, int32_t> &mappings) const
{
    CHECK_AND_RETURN_RET(rdb_ != nullptr, false);
    std::vector<int32_t> ids;
    std::unordered_set<int32_t> unique;
    for (const auto &entry : expected) {
        CHECK_AND_RETURN_RET(entry.first > 0 && entry.second > 0 && unique.insert(entry.second).second, false);
        ids.push_back(entry.second);
    }
    std::unordered_map<int32_t, int32_t> queried;
    CHECK_AND_RETURN_RET(ReadIdRows(rdb_, ids, "SELECT old_file_id, file_id, clone_sequence FROM tab_cloned_old_photos "
        "WHERE file_id IN (", [&queried, &expected](const auto &row) {
            int32_t oldId = 0;
            int32_t finalId = 0;
            int32_t sequence = 0;
            CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(ASSET_MAP_COL_OLD_FILE_ID, row, oldId) && oldId > 0 &&
                ResultSetUtils::TryGetValue(ASSET_MAP_COL_FILE_ID, row, finalId) &&
                ResultSetUtils::TryGetValue(ASSET_MAP_COL_CLONE_SEQUENCE, row, sequence), false);
            const auto wanted = expected.find(oldId);
            return sequence == 1 && wanted != expected.end() && wanted->second == finalId &&
                queried.emplace(oldId, finalId).second;
        }), false);
    mappings = std::move(queried);
    return true;
}

bool AnalysisAlbumRefreshDb::PrepareQueryDirectory(const std::vector<AnalysisAlbumQueryScope> &scopes,
    AnalysisAlbumQueryDirectory &directory)
{
    AnalysisAlbumQueryDirectory prepared;
    std::unordered_map<int32_t, AnalysisAlbumQueryScope> unique;
    std::map<ShootingModeAlbumType, std::vector<int32_t>> modes;
    for (const auto &scope : scopes) {
        const auto [it, inserted] = unique.emplace(scope.albumId, scope);
        CHECK_AND_RETURN_RET(scope.albumId > 0 && scope.subtype > 0 && (inserted ||
            (it->second.subtype == scope.subtype && it->second.albumName == scope.albumName)), false);
        CHECK_AND_CONTINUE(inserted);
        if (scope.subtype == static_cast<int32_t>(PhotoAlbumSubType::SHOOTING_MODE)) {
            ShootingModeAlbumType type {};
            CHECK_AND_RETURN_RET(ShootingModeAlbum::AlbumNameToShootingModeAlbumType(scope.albumName, type), false);
            modes[type].push_back(scope.albumId);
        } else if (IsSingleSideAnalysisSubtype(scope.subtype)) {
            prepared.grouped.insert(scope.albumId);
        } else {
            prepared.ordinary.insert(scope.albumId);
        }
    }
    CHECK_AND_RETURN_RET(PrepareShootingQueries(modes, prepared.shootingQueries), false);
    directory = std::move(prepared);
    return true;
}

int32_t AnalysisAlbumRefreshDb::QueryContributions(
    const AnalysisAlbumQueryDirectory &fixed,
    const std::vector<int32_t> &fileIds, const std::function<void(int32_t, int32_t)> &consume,
    const AnalysisAlbumGroupMembers &members,
    bool membershipOnly) const
{
    CHECK_AND_RETURN_RET(rdb_ != nullptr && consume, E_INVALID_ARGUMENTS);
    ContributionQuery query {rdb_, consume, membershipOnly, {}, {}, fixed, members};
    query.fileSet.insert(fileIds.begin(), fileIds.end());
    CHECK_AND_RETURN_RET(std::none_of(query.fileSet.begin(), query.fileSet.end(),
        [](int32_t id) { return id <= 0; }), E_INVALID_ARGUMENTS);
    query.files.assign(query.fileSet.begin(), query.fileSet.end());
    CHECK_AND_RETURN_RET(!query.files.empty(), E_OK);
    const bool complete = QueryRelationContributions(query) && QueryShootingContributions(query);
    MEDIA_INFO_LOG("Analysis contribution ids=%{public}zu relation_sql=%{public}zu mode_sql=%{public}zu "
        "membership=%{public}d complete=%{public}d", query.files.size(), query.relationQueries,
        query.shootingQueries, membershipOnly, complete);
    return complete ? E_OK : E_HAS_DB_ERROR;
}

int32_t AnalysisAlbumRefreshDb::QueryPhotoUris(
    const std::vector<int32_t> &fileIds,
    std::unordered_map<int32_t, std::string> &photos, std::unordered_set<int32_t> *validCoverPhotos) const
{
    photos.clear();
    CHECK_AND_EXECUTE(validCoverPhotos == nullptr, validCoverPhotos->clear());
    CHECK_AND_RETURN_RET(rdb_ != nullptr, E_INVALID_ARGUMENTS);
    const std::unordered_set<int32_t> requested(fileIds.begin(), fileIds.end());
    CHECK_AND_RETURN_RET(std::none_of(requested.begin(), requested.end(),
        [](int32_t id) { return id <= 0; }), E_INVALID_ARGUMENTS);
    std::unordered_map<int32_t, std::string> queried;
    std::unordered_set<int32_t> eligible;
    const std::string eligibility = validCoverPhotos == nullptr ? "" :
        ", CASE WHEN " + MediaLibraryRdbUtils::GetAnalysisPhotoCountClause() + " THEN 1 ELSE 0 END AS cover_eligible";
    CHECK_AND_RETURN_RET(ReadIdRows(rdb_, {requested.begin(), requested.end()},
        "SELECT file_id, data, display_name" + eligibility + " FROM Photos WHERE file_id IN (", [&](const auto &row) {
            int32_t fileId = 0;
            CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_ID, row, fileId) &&
                requested.count(fileId) > 0, false);
            std::string path;
            std::string name;
            CHECK_AND_RETURN_RET(ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_FILE_PATH, row, path) &&
                !path.empty() &&
                ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_NAME, row, name) && !name.empty(), false);
            int32_t valid = 0;
            CHECK_AND_RETURN_RET(validCoverPhotos == nullptr ||
                ResultSetUtils::TryGetValue("cover_eligible", row, valid), false);
            CHECK_AND_EXECUTE(valid == 0, eligible.insert(fileId));
            const std::string uri = MediaFileUri::GetPhotoUri(std::to_string(fileId), path, name);
            return !uri.empty() && queried.emplace(fileId, uri).second;
        }), E_HAS_DB_ERROR);
    // Empty metadata records prove a successful query found no photo; failures publish nothing.
    for (int32_t id : requested) {
        queried.try_emplace(id);
    }
    photos = std::move(queried);
    CHECK_AND_EXECUTE(validCoverPhotos == nullptr, *validCoverPhotos = std::move(eligible));
    return E_OK;
}

namespace {
std::shared_ptr<NativeRdb::ResultSet> QueryDefaultCoverRows(
    const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const AnalysisAlbumQueryScope &scope)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> columns = {"Photos.file_id", "Photos.data", "Photos.display_name"};
    if (IsSingleSideAnalysisSubtype(scope.subtype)) {
        MediaLibraryRdbUtils::GetPortraitAlbumCoverPredicates(std::to_string(scope.albumId), predicates);
    } else {
        if (scope.subtype == PhotoAlbumSubType::SHOOTING_MODE) {
            ShootingModeAlbumType type {};
            CHECK_AND_RETURN_RET(ShootingModeAlbum::AlbumNameToShootingModeAlbumType(scope.albumName, type), nullptr);
        }
        UpdateAlbumData album {};
        album.albumId = scope.albumId;
        album.albumSubtype = scope.subtype;
        album.albumName = scope.albumName;
        MediaLibraryRdbUtils::GetAlbumCountAndCoverPredicates(album, predicates, false, true);
        MediaLibraryRdbUtils::DetermineQueryOrder(predicates, album, false, columns);
        predicates.Limit(1);
    }
    return rdbStore->Query(predicates, columns);
}

} // namespace

int32_t AnalysisAlbumRefreshDb::QueryDefaultCover(
    const AnalysisAlbumQueryScope &scope,
    std::string &coverUri) const
{
    coverUri.clear();
    CHECK_AND_RETURN_RET(rdb_ != nullptr && scope.albumId > 0, E_INVALID_ARGUMENTS);
    auto rows = QueryDefaultCoverRows(rdb_, scope);
    std::string uri;
    const bool complete = ResultSetUtils::ReadRows(rows, [&uri, &scope](const auto &row) {
        bool emptyAggregate = false;
        int32_t idColumn = -1;
        CHECK_AND_RETURN_RET(row->GetColumnIndex(PhotoColumn::MEDIA_ID, idColumn) == E_OK &&
            row->IsColumnNull(idColumn, emptyAggregate) == E_OK, false);
        if (emptyAggregate) {
            return scope.subtype == PhotoAlbumSubType::SHOOTING_MODE;
        }
        int32_t id = 0;
        std::string path;
        std::string name;
        CHECK_AND_RETURN_RET(uri.empty() && row->GetInt(idColumn, id) == E_OK && id > 0 &&
            ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_FILE_PATH, row, path) && !path.empty() &&
            ResultSetUtils::TryGetValue(PhotoColumn::MEDIA_NAME, row, name) && !name.empty(), false);
        uri = MediaFileUri::GetPhotoUri(std::to_string(id), path, name);
        return !uri.empty();
    });
    CHECK_AND_RETURN_RET(complete, E_HAS_DB_ERROR);
    coverUri = std::move(uri);
    return E_OK;
}

int32_t AnalysisAlbumRefreshDb::UpdateAlbum(int32_t albumId, NativeRdb::ValuesBucket &values)
{
    CHECK_AND_RETURN_RET(rdb_ != nullptr, E_HAS_DB_ERROR);
    CHECK_AND_RETURN_RET(!values.IsEmpty(), E_OK);
    TransactionOperations transaction("AnalysisAlbumContributionUpdate");
    transaction.SetBackupRdbStore(rdb_);
    std::function<int()> apply = [&]() -> int {
        RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(albumId));
        int32_t changedRows = 0;
        return transaction.Update(changedRows, values, predicates) == E_OK && changedRows == 1 ? E_OK : E_HAS_DB_ERROR;
    };
    return transaction.RetryTrans(apply, true) == E_OK ? E_OK : E_HAS_DB_ERROR;
}

} // namespace OHOS::Media
