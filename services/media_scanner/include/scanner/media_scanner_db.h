/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SCANNER_DB_H
#define MEDIA_SCANNER_DB_H

#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "medialibrary_command.h"
#include "medialibrary_db_const.h"
#include "medialibrary_type_const.h"
#include "metadata.h"
#include "datashare_values_bucket.h"
#include "rdb_errno.h"
#include "result_set.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "want.h"
#include "datashare_result_set.h"
#include "asset_accurate_refresh.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaScannerDb {
public:
    EXPORT MediaScannerDb();
    EXPORT MediaScannerDb(MediaScannerDb &other) = delete;
    EXPORT void operator=(const MediaScannerDb &) = delete;
    EXPORT ~MediaScannerDb() = default;

    EXPORT static std::unique_ptr<MediaScannerDb> GetDatabaseInstance();
    EXPORT bool DeleteMetadata(const std::vector<std::string> &idList, const std::string &tableName);
    EXPORT void NotifyDatabaseChange(const MediaType mediaType);
    EXPORT void SetRdbHelper(void);

    EXPORT std::string InsertMetadata(const Metadata &metadata, std::string &tableName,
        MediaLibraryApi api = MediaLibraryApi::API_OLD,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> refresh = nullptr);
    EXPORT std::string UpdateMetadata(const Metadata &metadata, std::string &tableName,
        MediaLibraryApi api = MediaLibraryApi::API_OLD, bool skipPhoto = true,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> refresh = nullptr);
    EXPORT std::string GetFileDBUriFromPath(const std::string &path);
    EXPORT int32_t InsertAlbum(const Metadata &metadata);
    EXPORT int32_t UpdateAlbum(const Metadata &metadata);
    EXPORT int32_t ReadAlbums(const std::string &path, std::unordered_map<std::string, Metadata> &albumMap);
    EXPORT std::unordered_map<int32_t, MediaType> GetIdsFromFilePath(const std::string &path,
        const std::string &tableName, const std::string &whitePath = "");

    EXPORT int32_t GetIdFromPath(const std::string &path);
    EXPORT int32_t GetFileBasicInfo(const std::string &path, std::unique_ptr<Metadata> &ptr,
        MediaLibraryApi api = MediaLibraryApi::API_OLD, int32_t fileId = 0);

    EXPORT int32_t RecordError(const std::string &err);
    EXPORT std::set<std::string> ReadError();
    EXPORT int32_t DeleteError(const std::string &err);
    static void UpdateAlbumInfo(const std::vector<std::string> &subtypes = {},
        const std::vector<std::string> &userAlbumIds = {}, const std::vector<std::string> &sourceAlbumIds = {});
    static void UpdateAlbumInfoByMetaData(const Metadata &metadata);

private:
    int32_t FillMetadata(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unique_ptr<Metadata> &ptr);
    int32_t GetFileSet(MediaLibraryCommand &cmd, const vector<string> &columns,
        std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void ExtractMetaFromColumn(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::unique_ptr<Metadata> &metadata, const std::string &col);
    bool InsertData(const NativeRdb::ValuesBucket values, const std::string &tableName, int64_t &rowNum,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> refresh = nullptr);
    std::string MakeFileUri(const std::string &mediaTypeUri, const Metadata &metadata);
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SCANNER_DB_H
