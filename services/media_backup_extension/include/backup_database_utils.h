/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef BACKUP_DATABASE_UTILS_H
#define BACKUP_DATABASE_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <type_traits>

#include "backup_const.h"
#include "rdb_helper.h"
#include "result_set.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "result_set_utils.h"
#include "media_config_info_column.h"

namespace OHOS {
namespace Media {
using FileIdPair = std::pair<int32_t, int32_t>;
using TagPairOpt = std::pair<std::optional<std::string>, std::optional<std::string>>;
constexpr int32_t TOTAL_TBL_FACE_ANALYSED = 2;
constexpr int32_t TOTAL_TBL_PET_ANALYSED = 1;
struct CloneVideoInfo {
    int32_t file_id;
    int32_t face;
    int32_t label;
    int32_t status;
};
class BackupDatabaseUtils {
public:
    using ConfigInfoType = std::unordered_map<ConfigInfoSceneId, std::unordered_map<std::string, std::string>>;
public:
    static int32_t InitDb(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &dbName,
        const std::string &dbPath, const std::string &bundleName, bool isMediaLibary,
            int32_t area = DEFAULT_AREA_VERSION);
    static int32_t InitReadOnlyRdb(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &dbName,
        const std::string &dbPath, const std::string &bundleName);
    static int32_t QueryInt(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &sql,
        const std::string &column, const std::vector<NativeRdb::ValueObject> &args = {});
    static int32_t Update(std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t &changeRows,
        NativeRdb::ValuesBucket &valuesBucket, std::unique_ptr<NativeRdb::AbsRdbPredicates> &predicates);
    static int32_t Delete(NativeRdb::AbsRdbPredicates &predicates, int32_t &changeRows,
        std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static int32_t InitGarbageAlbum(std::shared_ptr<NativeRdb::RdbStore> rdbStore, std::set<std::string> &cacheSet,
        std::unordered_map<std::string, std::string> &nickMap);
    static void QueryGalleryDuplicateDataCount(std::shared_ptr<NativeRdb::RdbStore> galleryRdb, int32_t &count,
        int32_t &total);
    static std::shared_ptr<NativeRdb::ResultSet> GetQueryResultSet(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
        const std::string &querySql, const std::vector<std::string> &sqlArgs = {});
    static std::unordered_map<std::string, std::string> GetColumnInfoMap(
        const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName);
    static void UpdateUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, int32_t number,
        const std::string &type);
    static int32_t QueryUniqueNumber(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &type);
    static std::string GarbleInfoName(const std::string &infoName);
    static void UpdateSelection(std::string &selection, const std::string &selectionToAdd, bool needWrap = false);
    static void UpdateSdWhereClause(std::string &querySql, bool shouldIncludeSd);
    static bool QueryThumbImage(NativeRdb::RdbStore &rdbStore,
        const std::string &keyValue, std::vector<uint8_t> &blob);
    static int32_t GetBlob(const std::string &columnName, std::shared_ptr<NativeRdb::ResultSet> resultSet,
        std::vector<uint8_t> &blobVal);
    static uint32_t GetUint32ValFromBytes(const std::vector<uint8_t> &bytes, size_t start);
    static void UpdateAnalysisTotalStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static void UpdateAnalysisFaceTagStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static void UpdateAnalysisPetTagStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static bool SetTagIdNew(PortraitAlbumInfo &portraitAlbumInfo,
        std::unordered_map<std::string, std::string> &tagIdMap);
    static bool SetFileIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, FileInfo> &fileInfoMap);
    static bool SetTagIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, std::string> &tagIdMap);
    static bool SetAlbumIdNew(FaceInfo &faceInfo, const std::unordered_map<std::string, int32_t> &albumIdMap);
    static void PrintErrorLog(const std::string &errorLog, int64_t start);
    static float GetLandmarksScale(int32_t width, int32_t height);
    static bool IsLandmarkValid(const FaceInfo &faceInfo, float landmarkX, float landmarkY);
    static bool IsValInBound(float val, float minVal, float maxVal);
    static std::vector<std::pair<std::string, std::string>> GetColumnInfoPairs(
        const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, const std::string &tableName);
    static std::vector<std::string> GetCommonColumnInfos(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb, std::string tableName);
    static std::vector<std::string> filterColumns(const std::vector<std::string>& allColumns,
        const std::vector<std::string>& excludedColumns);
    static std::vector<FileIdPair> CollectFileIdPairs(const std::vector<FileInfo>& fileInfos);
    static std::pair<std::vector<int32_t>, std::vector<int32_t>> UnzipFileIdPairs(const std::vector<FileIdPair>& pairs);
    static void UpdateAnalysisPhotoMapStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static std::vector<std::string> SplitString(const std::string& str, char delimiter);
    static void PrintQuerySql(const std::string& querySql);
    static int64_t QueryLong(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &sql,
        const std::string &columnName, const std::vector<NativeRdb::ValueObject> &args = {});
    static int64_t QueryMaxId(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::string& tableName, const std::string& idColumnName);
    static int ExecuteSQL(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string& sql,
        const std::vector<NativeRdb::ValueObject> &args = {});
    static bool DeleteDuplicatePortraitAlbum(int64_t maxAlbumId, const std::vector<std::string> &albumNames,
        const std::vector<std::string> &tagIds, std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void UpdateAnalysisTotalTblStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::vector<FileIdPair>& fileIdPair);
    static void UpdateFaceAnalysisTblStatus(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void DeleteExistingImageFaceData(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        const std::vector<FileIdPair>& fileIdPair);
    static void DeleteExistingPetFaceData(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        const std::vector<FileIdPair>& fileIdPair);
    static std::vector<TagPairOpt> QueryTagInfo(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void ParseFaceTagResultSet(const std::shared_ptr<NativeRdb::ResultSet>& resultSet,
        TagPairOpt& tagPair);
    static void UpdateGroupTagColumn(const std::vector<TagPairOpt>& updatedPairs,
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void UpdateFaceGroupTagsUnion(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void UpdateFaceGroupTagOfGallery(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb);
    static void UpdateTagPairs(std::vector<TagPairOpt>& updatedPairs, const std::string& newGroupTag,
        const std::vector<std::string>& tagIds);
    static void UpdateGroupTags(std::vector<TagPairOpt>& updatedPairs,
        const std::unordered_map<std::string, std::vector<std::string>>& groupTagMap);
    static void UpdateAssociateFileId(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::vector<FileInfo> &fileInfos);
    static void BatchUpdatePhotosToLocal(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        const std::vector<std::string> &inColumn);
    static int32_t BatchInsert(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string &tableName,
        std::vector<NativeRdb::ValuesBucket> &value, int64_t &rowNum);
    static std::string CheckDbIntegrity(std::shared_ptr<NativeRdb::RdbStore> rdbStore, int32_t sceneCode,
        const std::string &dbTag = "");
    static int32_t QueryLocalNoAstcCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static int32_t QueryReadyAstcCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static std::unordered_map<int32_t, int32_t> QueryMediaTypeCount(
        const std::shared_ptr<NativeRdb::RdbStore>& rdbStore, const std::string& querySql);
    static std::shared_ptr<NativeRdb::ResultSet> QuerySql(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::string &querySql, const std::vector<NativeRdb::ValueObject> &params = {});
    template <typename T>
    static std::string JoinValues(const std::vector<T>& values, std::string_view delimiter);
    template <typename T>
    static std::string JoinSQLValues(const std::vector<T>& values, std::string_view delimiter);
    template <typename T>
    static std::vector<T> LeftJoinValues(std::vector<T>& values,
        std::string_view delimiter);
    template <typename T>
    struct always_false : std::false_type {};
    template <typename T>
    static std::optional<T> GetOptionalValue(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        const std::string &columnName);
    template <typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
        const std::optional<T>& optionalValue);
    static void UpdateBurstPhotos(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static std::vector<int32_t> QueryIntVec(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::string& sql, const std::string& columnName);
    static std::unordered_map<int32_t, int32_t> QueryIntMap(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        const std::string& sql, const std::string& keyColumnName, const std::string& valueColumnName);
    static std::vector<CloneVideoInfo> QueryVideoInfo(std::string tableName,
        std::string VideofileIdOldInClause, std::shared_ptr<NativeRdb::RdbStore> rdbStore);
    static std::unordered_map<int32_t, int32_t> QueryOldNoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> oldRdbStore,
        const std::vector<int32_t>& oldFileIds);
    static void UpdateNewNoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
        const std::unordered_map<int32_t, int32_t>& oldFileIdToFaceMap, const std::vector<FileIdPair>& fileIdPair);
    static void UpdateAnalysisTotalTblNoFaceStatus(std::shared_ptr<NativeRdb::RdbStore> newRdbStore,
        std::shared_ptr<NativeRdb::RdbStore> oldRdbStore, const std::vector<FileIdPair>& fileIdPair);
    static bool isTableExist(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
        const std::string &tableName, bool& result);
    static ConfigInfoType QueryConfigInfo(
        const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static bool QueryPhotoUniqueSouthDeviceType(
        const std::shared_ptr<NativeRdb::RdbStore> &rdbStore, std::vector<SouthDeviceType>& uniqueSouthDeviceType);
    static bool ClearConfigInfo(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void ClearAnalysisVideoTotalTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void CheckFaceToAnalysisVideoTotalTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void UpdateStatusToAnalysisTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void UpdateFaceToAnalysisVideoTotalTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void UpdateFaceToAnalysisTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore,
        const std::vector<int32_t> &newFileIds);
    static void DeleteDirtytagIdFromFaceTagTable(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void UpdateVideoFaceTagId(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);
    static void UpdateVideoTotalFaceId(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore);

private:
    static std::string CloudSyncTriggerFunc(const std::vector<std::string> &args);
    static std::string IsCallerSelfFunc(const std::vector<std::string> &args);
    static std::string PhotoAlbumNotifyFunc(const std::vector<std::string>& args);
    static std::string BeginGenerateHighlightThumbnail(const std::vector<std::string>& args);
    static const ssize_t WAL_LIMIT_SIZE = 1024 * 1024 * 1024;
};

class RdbCallback : public NativeRdb::RdbOpenCallback {
public:
    virtual int32_t OnCreate(NativeRdb::RdbStore &rdb) override
    {
        return 0;
    }

    virtual int32_t OnUpgrade(NativeRdb::RdbStore &rdb, int32_t oldVersion,
        int32_t newVersion) override
    {
        return 0;
    }
};

template <typename T>
std::string BackupDatabaseUtils::JoinSQLValues(const std::vector<T>& values, std::string_view delimiter)
{
    std::stringstream ss;
    bool first = true;
    for (const auto& value : values) {
        if (!first) {
            ss << delimiter;
        }
        first = false;
        if constexpr (std::is_same_v<T, std::string>) {
            ss << "'" << value << "'";
        } else {
            ss << std::to_string(value);
        }
    }
    return ss.str();
}

template <typename T>
std::string BackupDatabaseUtils::JoinValues(const std::vector<T>& values, std::string_view delimiter)
{
    std::stringstream ss;
    bool first = true;
    for (const auto& value : values) {
        if (!first) {
            ss << delimiter;
        }
        first = false;
        if constexpr (std::is_same_v<T, std::string>) {
            ss << value;
        } else {
            ss << std::to_string(value);
        }
    }
    return ss.str();
}

template <typename T>
std::vector<T> BackupDatabaseUtils::LeftJoinValues(std::vector<T>& values, std::string_view delimiter)
{
    for (auto& value : values) {
        value.insert(0, delimiter);
    }
    return values;
}

template<typename T>
std::optional<T> BackupDatabaseUtils::GetOptionalValue(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    const std::string &columnName)
{
    int32_t columnIndex;
    int32_t err = resultSet->GetColumnIndex(columnName, columnIndex);
    if (err != E_OK) {
        return std::nullopt;
    }

    bool isNull = false;
    int32_t errCode = resultSet->IsColumnNull(columnIndex, isNull);
    if (errCode || isNull) {
        return std::nullopt;
    }

    T value;
    if constexpr (std::is_same_v<T, int32_t>) {
        errCode = resultSet->GetInt(columnIndex, value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        errCode = resultSet->GetLong(columnIndex, value);
    } else if constexpr (std::is_same_v<T, double>) {
        errCode = resultSet->GetDouble(columnIndex, value);
    } else if constexpr (std::is_same_v<T, std::string>) {
        errCode = resultSet->GetString(columnIndex, value);
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        errCode = resultSet->GetBlob(columnIndex, value);
    } else {
        static_assert(always_false<T>::value, "Unsupported type for GetOptionalValue");
    }

    return errCode ? std::nullopt : std::optional<T>(value);
}

template <typename T>
void BackupDatabaseUtils::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue)
{
    if (optionalValue.has_value()) {
        if constexpr (std::is_same_v<std::decay_t<T>, int32_t>) {
            values.PutInt(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>) {
            values.PutLong(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
            values.PutString(columnName, optionalValue.value());
        } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
            values.PutDouble(columnName, optionalValue.value());
        }  else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<uint8_t>>) {
            values.PutBlob(columnName, optionalValue.value());
        }
    }
}
} // namespace Media
} // namespace OHOS

#endif  // BACKUP_DATABASE_UTILS_H