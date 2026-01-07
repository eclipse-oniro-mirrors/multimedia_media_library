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

#ifndef CLONE_RESTORE_PORTRAIT_BASE_H
#define CLONE_RESTORE_PORTRAIT_BASE_H

#include <string>
#include <sstream>

#include "backup_const.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "rdb_store.h"

namespace OHOS::Media {
using CoverUriInfo = std::pair<std::string, std::pair<std::string, int32_t>>;
using FileIdPair = std::pair<int32_t, int32_t>;
constexpr int32_t INVALID_COVER_SATISFIED_STATUS = 0;
struct AnalysisAlbumInfo {
    std::optional<int32_t> albumIdOld;
    std::optional<int32_t> albumIdNew;
    std::optional<int32_t> albumType;
    std::optional<int32_t> albumSubtype;
    std::optional<std::string> albumName;
    std::optional<std::string> oldCoverUri;
    std::string coverUri = "";
    std::optional<int64_t> dateModified;

    std::optional<int32_t> rank;
    std::optional<std::string> tagId;
    std::optional<int32_t> userOperation;
    std::optional<std::string> groupTag;
    std::optional<int32_t> userDisplayLevel;
    std::optional<int32_t> isMe;
    std::optional<int32_t> isRemoved;
    std::optional<int32_t> renameOperation;
    std::optional<int32_t> isLocal;
    std::optional<int32_t> isCoverSatisfied;
    std::optional<std::string> relationship;
    std::optional<int32_t> editOperation;

    std::string GarbleCoverUri(const std::string coverUri) const
    {
        auto uriParts = BackupDatabaseUtils::SplitString(coverUri, '/');
        if (uriParts.size() >= COVER_URI_NUM) {
            std::string fileName = uriParts[uriParts.size() - 1];
            return BackupFileUtils::GarbleFileName(fileName);
        }
        return "Unknown";
    }

    std::string ToString() const
    {
        std::stringstream outputStr;
        outputStr << "AlbumInfo[" << "albumIdOld: ";
        if (albumIdOld.has_value()) { outputStr << albumIdOld.value(); }
        outputStr << ", albumIdNew: ";
        if (albumIdNew.has_value()) { outputStr << albumIdNew.value(); }
        outputStr << ", albumName: ";
        if (albumName.has_value()) { outputStr << albumName.value(); }
        outputStr << ", oldCoverUri: ";
        if (oldCoverUri.has_value()) { outputStr << GarbleCoverUri(oldCoverUri.value()); }
        outputStr << ", coverUri: " << GarbleCoverUri(coverUri) << "]";
        return outputStr.str();
    }
};

class CloneRestorePortraitBase {
public:
    void GetMaxAlbumId();
    void GetAnalysisAlbumInsertValue(NativeRdb::ValuesBucket &value, const AnalysisAlbumTbl &info);
    void ParseAlbumResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        AnalysisAlbumTbl &analysisAlbumTbl);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        const std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    void AppendExtraWhereClause(std::string& whereClause);
    void GenNewCoverUris(const std::vector<CoverUriInfo>& coverUriInfo);
    std::string GenCoverUriUpdateSql(const std::unordered_map<std::string, std::pair<std::string, int32_t>>&
        tagIdToCoverInfo, const std::unordered_map<std::string, int32_t>& oldToNewFileId,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap, std::vector<std::string>& tagIds);
    std::string ProcessUriAndGenNew(const std::string& tagId, const std::string& oldCoverUri,
        const std::unordered_map<std::string, int32_t>& oldToNewFileId,
        const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    bool GetFileInfoByFileId(int32_t fileId, const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap,
        PhotoInfo& outPhotoInfo);
    std::vector<FileIdPair> CollectFileIdPairs(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    bool IsMapColumnOrderExist();
public:
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    int32_t sceneCode_;
    std::string restoreInfo_;
    int32_t syncSwitchType_{0};
    int32_t maxAnalysisAlbumId_{0};
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    bool isCloudRestoreSatisfied_{false};
};
} // OHOS::Media
#endif