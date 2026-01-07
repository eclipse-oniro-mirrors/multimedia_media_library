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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_RDB_UTILS_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_RDB_UTILS_H_

#include "thumbnail_data.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ThumbnailRdbUtils {
public:
    ThumbnailRdbUtils() = delete;
    virtual ~ThumbnailRdbUtils() = delete;

    static bool QueryThumbnailDataInfos(std::shared_ptr<MediaLibraryRdbStore> store,
        NativeRdb::RdbPredicates &rdbPredicates, const std::vector<std::string> &column,
        std::vector<ThumbnailData> &datas);
    static bool QueryThumbnailDataInfos(std::shared_ptr<MediaLibraryRdbStore> store,
        NativeRdb::RdbPredicates &rdbPredicates, const std::vector<std::string> &column,
        std::vector<ThumbnailData> &datas, int &err);
    static bool QueryThumbnailDataInfos(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        const std::vector<std::string> &column, std::vector<ThumbnailData> &datas, int &err);
    static bool QueryThumbnailDataInfo(std::shared_ptr<MediaLibraryRdbStore> store,
        NativeRdb::RdbPredicates &rdbPredicates, const std::vector<std::string> &column, ThumbnailData &data);
    static bool QueryThumbnailDataInfo(std::shared_ptr<MediaLibraryRdbStore> store,
        NativeRdb::RdbPredicates &rdbPredicates, const std::vector<std::string> &column, ThumbnailData &data, int &err);
    static bool QueryThumbnailDataInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        const std::vector<std::string> &column, ThumbnailData &data, int &err);
    static bool CheckResultSetCount(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int &err);
    static void ParseQueryResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, ThumbnailData &data,
        int &err, const std::vector<std::string> &column);
    static void ParseStringResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int index, std::string &data);
    static void ParseInt32Result(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int index, int32_t &data);
    static void ParseInt64Result(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int index, int64_t &data);

    static bool QueryLocalNoExifRotateInfos(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos);
    static int32_t UpdateExifRotateAndDirty(const ThumbnailData &data, DirtyType dirtyType,
        bool needRegenerateThumbnail);
    static int32_t UpdateRdbStoreById(ThumbRdbOpt &opts, const std::string &id, const NativeRdb::ValuesBucket &values);

private:
    static void HandleId(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleFilePath(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleDateAdded(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleDisplayName(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleDateTaken(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleDateModified(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int idx, ThumbnailData &data);
    static void HandleMediaType(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleOrientation(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleExifRotate(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandlePosition(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandlePhotoHeight(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandlePhotoWidth(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleDirty(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleReady(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static void HandleLcdVisitTime(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int idx, ThumbnailData &data);
    static void HandleThumbStatus(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        int idx, ThumbnailData &data);

    using HandleFunc = void(*)(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
    static const std::unordered_map<std::string, HandleFunc> RESULT_SET_HANDLER;
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_RDB_UTILS_H_