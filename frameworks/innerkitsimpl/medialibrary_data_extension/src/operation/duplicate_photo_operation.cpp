/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "DuplicatePhotoOperation"

#include "duplicate_photo_operation.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "photo_query_filter.h"

namespace OHOS {
namespace Media {
std::once_flag DuplicatePhotoOperation::onceFlag_;

const std::string ASTERISK = "*";

const std::string SELECT_COLUMNS = "SELECT_COLUMNS";
const std::string NORMALIZED_PHOTOS = "NormalizedPhotos";
const std::string NORMALIZED_TITLE = "normalized_title";

const std::string LIMIT_CLAUSE = "LIMIT ?";
const std::string OFFSET_CLAUSE = "OFFSET ?";

const std::string IDX_DUPLICATE_ASSETS = "\
    CREATE INDEX \
    IF \
      NOT EXISTS idx_duplicate_assets ON Photos (title, size, orientation)";

static std::string GetNormalizedPhotosSubquery()
{
    static const std::string SQL_NORMALIZED_PHOTO_SUBQUERY = " \
        SELECT \
          *, \
          REGEXP_REPLACE(title, '(_[0-9]{2})?((_[0-9])|(\\([0-9]*\\)))*$', '') AS " + NORMALIZED_TITLE + " \
        FROM \
          Photos \
        WHERE " +
          PhotoQueryFilter::GetSqlWhereClause(PhotoQueryFilter::Option::FILTER_VISIBLE) + " ";

    return SQL_NORMALIZED_PHOTO_SUBQUERY;
}

static std::string GetAllDuplicateImageAssetsCTE()
{
    static const std::string SQL_ALL_DUPLICATE_IMG_ASSETS_CTE =
        NORMALIZED_PHOTOS + " \
        INNER JOIN ( \
            SELECT " +
                NORMALIZED_TITLE + ", \
                size, \
                orientation \
            FROM " +
                NORMALIZED_PHOTOS + " \
            WHERE \
                media_type = 1 \
            GROUP BY " +
                NORMALIZED_TITLE + ", \
                size, \
                orientation \
            HAVING \
                count(*) > 1 \
        ) AS DupImg ON " + NORMALIZED_PHOTOS + "." + NORMALIZED_TITLE + " = DupImg." + NORMALIZED_TITLE + " \
            AND " + NORMALIZED_PHOTOS + ".size = DupImg.size \
            AND " + NORMALIZED_PHOTOS + ".orientation = DupImg.orientation ";

    return SQL_ALL_DUPLICATE_IMG_ASSETS_CTE;
}

static std::string GetAllDuplicateVideoAssetsCTE()
{
    static const std::string SQL_ALL_DUPLICATE_VID_ASSETS_CTE =
        NORMALIZED_PHOTOS + " \
        INNER JOIN (\
            SELECT " +
                NORMALIZED_TITLE + ", \
                size, \
                orientation \
            FROM " +
                NORMALIZED_PHOTOS + " \
            WHERE \
                media_type = 2 \
            GROUP BY " +
                NORMALIZED_TITLE + ", \
                size \
            HAVING \
                count(*) > 1\
        ) AS DupVid ON " + NORMALIZED_PHOTOS + "." + NORMALIZED_TITLE + " = DupVid." + NORMALIZED_TITLE + " \
            AND " + NORMALIZED_PHOTOS + ".size = DupVid.size ";

    return SQL_ALL_DUPLICATE_VID_ASSETS_CTE;
}

static std::string GetQueryAllDuplicateAssetsCountSql()
{
    static const std::string SQL_QUERY_ALL_DUPLICATE_ASSETS_COUNT = "\
        WITH " + NORMALIZED_PHOTOS + " AS (" + GetNormalizedPhotosSubquery() + ") " + " \
        SELECT \
            count(*) \
        FROM (SELECT file_id FROM " + GetAllDuplicateImageAssetsCTE() +
            " UNION " +
            "SELECT file_id FROM " + GetAllDuplicateVideoAssetsCTE() + ") ";

    return SQL_QUERY_ALL_DUPLICATE_ASSETS_COUNT;
}

static std::string GetQueryAllDuplicateAssetsSql()
{
    static const std::string SQL_QUERY_ALL_DUPLICATE_ASSETS = "\
        WITH " + NORMALIZED_PHOTOS + " AS (" + GetNormalizedPhotosSubquery() + ") " + " \
        SELECT \
            * \
        FROM " + GetAllDuplicateImageAssetsCTE() + " \
        UNION \
        SELECT \
            * \
        FROM " + GetAllDuplicateVideoAssetsCTE() + " \
        ORDER BY \
            " + NORMALIZED_PHOTOS + "." + NORMALIZED_TITLE + ", \
            " + NORMALIZED_PHOTOS + ".size, \
            " + NORMALIZED_PHOTOS + ".orientation \
        ";

    return SQL_QUERY_ALL_DUPLICATE_ASSETS;
}

const std::string ALBUM_PRIORITY_EXPRESSION = "\
    CASE \
        WHEN lpath = '/DCIM/Camera' THEN \
        0 \
        WHEN lpath = '/Pictures/Screenshots' THEN \
        1 \
        WHEN lpath = '/Pictures/Screenrecords' THEN \
        2 \
        WHEN lpath = '/Pictures/WeiXin' THEN \
        3 \
        WHEN lpath IN ( '/Pictures/WeChat', '/tencent/MicroMsg/WeChat', '/Tencent/MicroMsg/WeiXin' ) THEN \
        4 \
        ELSE 5 \
    END ";

const std::string TITLE_PRIORITY_EXPRESSION = "\
    CASE \
        WHEN title = " + NORMALIZED_TITLE + " THEN \
        0 \
        ELSE 1 \
    END ";

static std::string GetDuplicateImageToDeleteCTE()
{
    static const std::string SQL_DUPLICATE_IMG_TO_DELETE_CTE = "\
      SELECT\
        " + SELECT_COLUMNS + ", \
        " + NORMALIZED_TITLE + ", \
        ROW_NUMBER( ) OVER (\
          PARTITION BY " + NORMALIZED_TITLE + ", \
          size, \
          orientation \
        ORDER BY \
        CASE \
          WHEN album_id != NULL THEN \
          0 ELSE 1 \
        END ASC, \
        " + ALBUM_PRIORITY_EXPRESSION + " ASC, \
        " + TITLE_PRIORITY_EXPRESSION + " ASC \
        ) AS row_num \
      FROM \
        " + NORMALIZED_PHOTOS + " \
        LEFT JOIN PhotoAlbum ON " + NORMALIZED_PHOTOS + ".owner_album_id = PhotoAlbum.album_id \
      WHERE \
        media_type = 1 ";

    return SQL_DUPLICATE_IMG_TO_DELETE_CTE;
}

static std::string GetDuplicateVideoToDeleteCTE()
{
    static const std::string SQL_DUPLICATE_VID_TO_DELETE_CTE = "\
      SELECT\
        " + SELECT_COLUMNS + ", \
        " + NORMALIZED_TITLE + ", \
        ROW_NUMBER( ) OVER (\
          PARTITION BY " + NORMALIZED_TITLE + ", \
          size \
        ORDER BY \
        CASE \
          WHEN album_id != NULL THEN \
          0 ELSE 1 \
        END ASC, \
        " + ALBUM_PRIORITY_EXPRESSION + " ASC, \
        " + TITLE_PRIORITY_EXPRESSION + " ASC \
        ) AS row_num \
      FROM \
        " + NORMALIZED_PHOTOS + " \
        LEFT JOIN PhotoAlbum ON " + NORMALIZED_PHOTOS + ".owner_album_id = PhotoAlbum.album_id \
      WHERE \
        media_type = 2 ";

    return SQL_DUPLICATE_VID_TO_DELETE_CTE;
}

static std::string GetDuplicateAssetsToDeleteSql()
{
    static const std::string SQL_QUERY_DUPLICATE_ASSETS_TO_DELETE = "\
        WITH " + NORMALIZED_PHOTOS + " AS (" + GetNormalizedPhotosSubquery() + ") " + " \
        SELECT \
            " + SELECT_COLUMNS + ", \
            " + NORMALIZED_TITLE + " \
        FROM ( " + GetDuplicateImageToDeleteCTE() + " ) \
        WHERE \
            row_num > 1 \
        UNION \
        SELECT \
            " + SELECT_COLUMNS + ", \
            " + NORMALIZED_TITLE + " \
        FROM ( " + GetDuplicateVideoToDeleteCTE() + " ) \
        WHERE \
            row_num > 1 \
        ORDER BY \
            " + NORMALIZED_TITLE + ", \
            size, \
            orientation \
    ";

    return SQL_QUERY_DUPLICATE_ASSETS_TO_DELETE;
}

static std::string GetDuplicateAssetsToDeleteCountSql()
{
    static const std::string SQL_QUERY_DUPLICATE_ASSETS_TO_DELETE = "\
        WITH " + NORMALIZED_PHOTOS + " AS (" + GetNormalizedPhotosSubquery() + ") " + " \
        SELECT \
            count(*) \
        FROM (SELECT file_id FROM (" + GetDuplicateImageToDeleteCTE() + ") WHERE row_num > 1 \
            UNION \
            SELECT file_id FROM (" + GetDuplicateVideoToDeleteCTE() + ") WHERE row_num > 1) ";

    return SQL_QUERY_DUPLICATE_ASSETS_TO_DELETE;
}

std::string DuplicatePhotoOperation::GetSelectColumns(const std::unordered_set<std::string> &columns)
{
    CHECK_AND_RETURN_RET(!columns.empty(), ASTERISK);

    std::string selectColumns;
    bool first = true;
    for (const std::string &column : columns) {
        if (!first) {
            selectColumns += ", ";
        } else {
            first = false;
        }
        selectColumns += column;
    }

    return selectColumns;
}

static void AppendLimitOffsetClause(std::string &sql, std::vector<NativeRdb::ValueObject>& bindArgs,
    int limit, int offset)
{
    if (limit >= 0) {
        sql += " " + LIMIT_CLAUSE;
        bindArgs.push_back(limit);
        if (offset >= 0) {
            sql += " " + OFFSET_CLAUSE;
            bindArgs.push_back(offset);
        }
    }
}

std::shared_ptr<NativeRdb::ResultSet> DuplicatePhotoOperation::GetAllDuplicateAssets(
    const NativeRdb::RdbPredicates& predicates, const std::vector<std::string>& columns)
{
    int limit = predicates.GetLimit();
    int offset = predicates.GetOffset();
    bool isQueryCount = find(columns.begin(), columns.end(), CONST_MEDIA_COLUMN_COUNT) != columns.end();
    MEDIA_INFO_LOG("Limit: %{public}d, Offset: %{public}d, isQueryCount: %{public}d", limit, offset, isQueryCount);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "GetAllDuplicateAssets failed, rdbStore is nullptr");

    MediaLibraryTracer tracer;
    if (isQueryCount) {
        tracer.Start("QueryAllDuplicateAssets_count");
        std::call_once(onceFlag_, [&]() { rdbStore->ExecuteSql(IDX_DUPLICATE_ASSETS); });
        return rdbStore->QueryByStep(GetQueryAllDuplicateAssetsCountSql());
    }

    tracer.Start("QueryAllDuplicateAssets_records");
    std::string sql = GetQueryAllDuplicateAssetsSql();
    std::vector<NativeRdb::ValueObject> bindArgs {};
    AppendLimitOffsetClause(sql, bindArgs, limit, offset);
    return rdbStore->QueryByStep(sql, bindArgs);
}

std::shared_ptr<NativeRdb::ResultSet> DuplicatePhotoOperation::GetDuplicateAssetsToDelete(
    const NativeRdb::RdbPredicates& predicates, const std::vector<std::string>& columns)
{
    int limit = predicates.GetLimit();
    int offset = predicates.GetOffset();
    bool isQueryCount = find(columns.begin(), columns.end(), CONST_MEDIA_COLUMN_COUNT) != columns.end();
    MEDIA_INFO_LOG("Limit: %{public}d, Offset: %{public}d, isQueryCount: %{public}d", limit, offset, isQueryCount);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "GetAllDuplicateAssets failed, rdbStore is nullptr");

    MediaLibraryTracer tracer;
    if (isQueryCount) {
        tracer.Start("QueryCanDelDuplicateAssets_count");
        return rdbStore->QueryByStep(GetDuplicateAssetsToDeleteCountSql());
    }

    tracer.Start("QueryCanDelDuplicateAssets_records");
    std::unordered_set<std::string> columnSet{ "file_id", "title", "size", "orientation" };
    columnSet.insert(columns.begin(), columns.end());
    std::string sql = GetDuplicateAssetsToDeleteSql();
    std::string selectColumns = GetSelectColumns(columnSet);
    MediaFileUtils::ReplaceAll(sql, SELECT_COLUMNS, selectColumns);
    std::vector<NativeRdb::ValueObject> bindArgs {};
    AppendLimitOffsetClause(sql, bindArgs, limit, offset);
    return rdbStore->QueryByStep(sql, bindArgs);
}
} // namespace Media
} // namespace OHOS
