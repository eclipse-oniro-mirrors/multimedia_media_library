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
#ifndef MEDIA_LIBRARY_MEDIA_LAKE_MONITOR_RDB_UTILS_H
#define MEDIA_LIBRARY_MEDIA_LAKE_MONITOR_RDB_UTILS_H

#include <string>

#include "asset_accurate_refresh.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS {
namespace Media {
struct LakeMonitorQueryResultData {
    int32_t fileId = -1;
    int32_t albumId = -1;
    int64_t dateTaken = 0;
    std::string photoPath;
};

template<typename T>
struct ColumnValueParser;

template<>
struct ColumnValueParser<int32_t> {
    static const std::string typeName;
    static int ParseValue(NativeRdb::ResultSet& rs, int index, int32_t& value);
};

template<>
struct ColumnValueParser<int64_t> {
    static const std::string typeName;
    static int ParseValue(NativeRdb::ResultSet& rs, int index, int64_t& value);
};

template<>
struct ColumnValueParser<std::string> {
    static const std::string typeName;
    static int ParseValue(NativeRdb::ResultSet& rs, int index, std::string& value);
};

class MediaLakeMonitorRdbUtils {
public:
    static bool QueryDataByDeletedStoragePath(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::string& storagePath, LakeMonitorQueryResultData &data);
    static bool QueryAlbumIdsByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        const std::string &lPath, std::vector<int32_t> &albumIds);
    static bool QueryAlbumByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        const std::string &lPath, std::vector<int32_t> &albumIds, std::unordered_map<int32_t, int32_t> &albumCounts);
    static bool QueryDataListByAlbumIds(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<int32_t> &albumIds, std::vector<LakeMonitorQueryResultData> &dataList);
    static bool DeleteAssetByStoragePath(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh,
        const std::string& filePath);
    static bool DeleteAssetsByOwnerAlbumIds(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::vector<int32_t> &albumIds);
    static bool DeleteEmptyAlbumsByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        const std::string &lPath);
    static bool UpdateAlbumInfo(std::shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t albumId = -1);
    static bool DeleteDirByLakePath(const std::string &path, std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        int32_t *delNum = nullptr);
    static bool DeleteLakeDirByLakePath(const std::string &path, std::shared_ptr<MediaLibraryRdbStore> &rdbStore);
    static bool DeleteLakeAlbums(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        std::unordered_map<int32_t, int32_t> &albumCounts, std::vector<LakeMonitorQueryResultData> &dataList);
    static void DeleteRelatedResource(const std::string &photoPath, const std::string &fileId,
        const std::string &dateTaken);
    static void NotifyAnalysisAlbum(const std::vector<std::string>& albumIds);

private:
    template <typename T>
    static T GetColumnValue(const std::shared_ptr<NativeRdb::ResultSet> &rs, const std::string &colName,
        const T &defaultValue = T{});
    static bool FillQueryResultData(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        LakeMonitorQueryResultData &data);
    static bool CheckValidData(const LakeMonitorQueryResultData &data);
    static NativeRdb::RdbPredicates BuildDeletePredicatesByStoragePath(const std::string &storagePath);
    static NativeRdb::RdbPredicates BuildQueryPredicatesByAlbumIds(const std::vector<int32_t> &albumIds);
};
}
}
#endif // MEDIA_LIBRARY_MEDIA_LAKE_MONITOR_RDB_UTILS_H