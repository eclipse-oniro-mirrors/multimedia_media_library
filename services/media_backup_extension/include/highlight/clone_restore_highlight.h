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

#ifndef CLONE_RESTORE_HIGHLIGHT_H
#define CLONE_RESTORE_HIGHLIGHT_H

#include <sstream>
#include <string>

#include "backup_const.h"
#include "backup_database_utils.h"
#include "media_log.h"
#include "nlohmann/json.hpp"
#include "rdb_store.h"
#include "safe_map.h"

namespace OHOS::Media {
class CloneRestoreHighlight {
public:
    struct InitInfo {
        int32_t sceneCode{-1};
        std::string taskId;
        std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb;
        std::shared_ptr<NativeRdb::RdbStore> mediaRdb;
        std::string backupRestoreDir;
        std::unordered_map<int32_t, PhotoInfo> photoInfoMap;
    };

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

        std::optional<int32_t> highlightIdOld;
        std::optional<int32_t> highlightIdNew;

        std::string ToString() const
        {
            std::stringstream outputStr;
            outputStr << "HighlightAlbumInfo[" << "albumIdOld: ";
            if (albumIdOld.has_value()) { outputStr << albumIdOld.value(); }
            outputStr << ", albumIdNew: ";
            if (albumIdNew.has_value()) { outputStr << albumIdNew.value(); }
            outputStr << ", albumName: ";
            if (albumName.has_value()) { outputStr << albumName.value(); }
            outputStr << ", oldCoverUri: ";
            if (oldCoverUri.has_value()) { outputStr << oldCoverUri.value(); }
            outputStr << ", highlightIdNew: ";
            if (highlightIdNew.has_value()) { outputStr << highlightIdNew.value(); }
            outputStr << ", coverUri: " << coverUri << "]";
            return outputStr.str();
        }
    };

    void Init(const InitInfo &info);
    void Restore();
    void ReportCloneRestoreHighlightTask();
    int32_t GetNewHighlightAlbumId(int32_t oldId);
    int32_t GetNewHighlightPhotoId(int32_t oldId);
    std::string GetNewHighlightPhotoUri(int32_t oldId);
    bool IsCloneHighlight();
    std::string GetDefaultPlayInfo();
    void UpdateHighlightStatus(const std::vector<int32_t> &highlightIds);
    void UpdateRestoreTimeCost(int64_t timeCost);
    const std::vector<AnalysisAlbumInfo>& GetAnalysisInfos() const;

    template<typename T>
    static void PutIfPresent(NativeRdb::ValuesBucket &values, const std::string &columnName,
        const std::optional<T> &optionalValue);

    template<typename T>
    static void PutIfInIntersection(NativeRdb::ValuesBucket &values, const std::string &columnName,
        const std::optional<T> &optionalValue, const std::unordered_set<std::string> &intersection);

    template<typename T>
    static void GetIfInIntersection(const std::string &columnName, std::optional<T> &optionalValue,
        const std::unordered_set<std::string> &intersection, std::shared_ptr<NativeRdb::ResultSet> resultSet);

private:
    struct HighlightAlbumInfo {
        std::optional<int32_t> highlightIdOld;
        std::optional<int32_t> highlightIdNew;
        std::optional<int32_t> albumIdOld;
        std::optional<int32_t> aiAlbumIdOld;
        std::optional<int32_t> albumIdNew;
        std::optional<int32_t> aiAlbumIdNew;
        std::optional<std::string> subTitle;
        std::optional<std::string> clusterType;
        std::optional<std::string> clusterSubType;
        std::optional<std::string> clusterCondition;
        std::optional<int64_t> minDateAdded;
        std::optional<int64_t> maxDateAdded;
        std::optional<int64_t> generateTime;
        std::optional<int32_t> highlightVersion;
        std::optional<std::string> remarks;
        std::optional<int32_t> highlightStatus;

        std::optional<int32_t> insertPicCount;
        std::optional<int32_t> removePicCount;
        std::optional<int32_t> shareScreenshotCount;
        std::optional<int32_t> shareCoverCount;
        std::optional<int32_t> renameCount;
        std::optional<int32_t> changeCoverCount;
        std::optional<int32_t> renderViewedTimes;
        std::optional<int64_t> renderViewedDuration;
        std::optional<int32_t> artLayoutViewedTimes;
        std::optional<int64_t> artLayoutViewedDuration;
        std::optional<int32_t> musicEditCount;
        std::optional<int32_t> filterEditCount;
        std::optional<int32_t> isMuted;
        std::optional<int32_t> isFavorite;
        std::optional<std::string> theme;
        std::optional<int64_t> pinTime;
        std::optional<int32_t> useSubtitle;
        std::optional<std::string> highlightLocation;
        std::optional<int64_t> notificationTime;

        std::string ToString() const
        {
            std::stringstream outputStr;
            outputStr << "HighlightAlbumInfo[" << "albumIdNew: ";
            if (albumIdNew.has_value()) { outputStr << albumIdNew.value(); }
            outputStr << ", aiAlbumIdNew: ";
            if (aiAlbumIdNew.has_value()) { outputStr << aiAlbumIdNew.value(); }
            outputStr << ", subTitle: ";
            if (subTitle.has_value()) { outputStr << subTitle.value(); }
            outputStr << ", clusterType: ";
            if (clusterType.has_value()) { outputStr << clusterType.value(); }
            outputStr << ", clusterSubType: ";
            if (clusterSubType.has_value()) { outputStr << clusterSubType.value(); }
            outputStr << ", clusterCondition: ";
            if (clusterCondition.has_value()) { outputStr << clusterCondition.value(); }
            outputStr << ", highlightStatus: ";
            if (highlightStatus.has_value()) { outputStr << highlightStatus.value(); }
            outputStr << "]";
            return outputStr.str();
        }
    };

    struct HighlightCoverInfo {
        std::optional<int32_t> highlightIdNew;
        std::optional<std::string> ratio;
        std::optional<std::string> background;
        std::optional<std::string> foreground;
        std::optional<std::string> wordart;
        std::optional<int32_t> isCovered;
        std::optional<std::string> color;
        std::optional<int32_t> radius;
        std::optional<double> saturation;
        std::optional<double> brightness;
        std::optional<int32_t> backgroundColorType;
        std::optional<int32_t> shadowLevel;
        std::optional<double> scaleX;
        std::optional<double> scaleY;
        std::optional<double> rectWidth;
        std::optional<double> rectHeight;
        std::optional<double> bgrScaleX;
        std::optional<double> bgrScaleY;
        std::optional<double> bgrRectWidth;
        std::optional<double> bgrRectHeight;
        std::optional<int32_t> layoutIndex;
        std::optional<int32_t> coverAlgoVer;
        std::optional<int32_t> coverServiceVer;

        std::optional<int32_t> status;

        std::string ToString() const
        {
            std::stringstream outputStr;
            outputStr << "HighlightCoverInfo[" << "highlightIdNew: ";
            if (highlightIdNew.has_value()) { outputStr << highlightIdNew.value(); }
            outputStr << ", ratio: ";
            if (ratio.has_value()) { outputStr << ratio.value(); }
            outputStr << "]";
            return outputStr.str();
        }
    };

    struct HighlightPlayInfo {
        std::optional<int32_t> highlightIdNew;
        std::optional<int32_t> playId;
        std::optional<std::string> music;
        std::optional<int32_t> filter;
        std::optional<std::string> pInfo;
        std::optional<int32_t> isChosen;
        std::optional<int32_t> pInfoVer;
        std::optional<std::string> hAlgoVer;
        std::optional<std::string> cameraAlgoVer;
        std::optional<std::string> transAlgoVer;
        std::optional<std::string> playServiceVer;
        std::optional<int32_t> status;

        std::string ToString() const
        {
            std::stringstream outputStr;
            outputStr << "HighlightPlayInfo[" << "highlightIdNew: ";
            if (highlightIdNew.has_value()) { outputStr << highlightIdNew.value(); }
            outputStr << ", playId: ";
            if (playId.has_value()) { outputStr << playId.value(); }
            outputStr << "]";
            return outputStr.str();
        }
    };

    void Preprocess();
    void RestoreAlbums();
    void RestoreMaps();
    int32_t GetTotalNumberOfMaps();
    void RestoreMapsBatch();
    void UpdateAlbums();

    void GetAnalysisAlbumInfos();
    void GetAnalysisRowInfo(AnalysisAlbumInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void UpdateAlbumCoverUri(AnalysisAlbumInfo &info);
    void InsertIntoAnalysisAlbum();
    void GetAnalysisInsertValue(NativeRdb::ValuesBucket &value, const AnalysisAlbumInfo &info);
    void UpdateHighlightStatusMap(int32_t highlightStatus, int32_t highlightId,
        std::unordered_map<int32_t, std::vector<NativeRdb::ValueObject>> &highlightStatusMap);
    void UpdateHighlightStatusInDatabase(
        const std::unordered_map<int32_t, std::vector<NativeRdb::ValueObject>> &highlightStatusMap);
    int32_t GetMaxAlbumId(const std::string &tableName, const std::string &idName);
    void UpdateMapInsertValues(std::vector<NativeRdb::ValuesBucket> &values);
    void UpdateMapInsertValuesByAlbumId(std::vector<NativeRdb::ValuesBucket> &values,
        std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void InsertAnalysisPhotoMap(std::vector<NativeRdb::ValuesBucket> &values);
    int32_t BatchInsertWithRetry(const std::string &tableName, const std::vector<NativeRdb::ValuesBucket> &values,
        int64_t &rowNum);
    NativeRdb::ValuesBucket GetMapInsertValue(int32_t albumId, int32_t fileId, std::optional<int32_t> &order);
    void GetHighlightAlbumInfos();
    void GetHighlightNewAlbumId(HighlightAlbumInfo &info);
    void GetHighlightRowInfo(HighlightAlbumInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void InsertIntoHighlightAlbum();
    void GetHighlightInsertValue(NativeRdb::ValuesBucket &value, const HighlightAlbumInfo &info);
    void PutTempHighlightStatus(NativeRdb::ValuesBucket &value, const HighlightAlbumInfo &info);
    void MoveHighlightCovers();
    void MoveHighlightWordart(const AnalysisAlbumInfo &info, const std::string &srcDir);
    void MoveHighlightGround(const AnalysisAlbumInfo &info, const std::string &srcDir);
    int32_t MoveHighlightMusic(const std::string &srcDir, const std::string &dstDir);
    void GetHighlightCoverInfos();
    void GetCoverRowInfo(HighlightCoverInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void GetCoverGroundSourceInfo(HighlightCoverInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void InsertIntoHighlightCoverInfo();
    void GetCoverInsertValue(NativeRdb::ValuesBucket &value, const HighlightCoverInfo &info);
    void GetHighlightPlayInfos();
    void GetPlayRowInfo(HighlightPlayInfo &info, std::shared_ptr<NativeRdb::ResultSet> resultSet);
    void InsertIntoHighlightPlayInfo();
    void GetPlayInsertValue(NativeRdb::ValuesBucket &value, const HighlightPlayInfo &info);
    std::unordered_set<std::string> GetCommonColumns(const std::string &tableName);
    void ReportRestoreTaskOfTotal();
    void ReportRestoreTaskOfAlbumStats();
    void ReportRestoreTaskOfAlbumInfo();
    bool IsMapColumnOrderExist();
    void HighlightDeduplicate(const HighlightAlbumInfo &info);
    std::vector<NativeRdb::ValueObject> GetHighlightDuplicateIds(const HighlightAlbumInfo &info,
        std::string &duplicateAlbumName, std::unordered_set<int32_t> &duplicateAnalysisAlbumIdSet);
    void UpdateHighlightDuplicateRows(const std::vector<NativeRdb::ValueObject> &changeIds,
        const std::string &duplicateAlbumName);
    void DeleteAnalysisDuplicateRows(const std::unordered_set<int32_t> &duplicateAnalysisAlbumIdSet,
        const std::string &duplicateAlbumName);

private:
    int32_t sceneCode_{-1};
    std::string taskId_;
    std::string coverPath_;
    std::string musicDir_;
    std::string garblePath_;
    // old media_liabrary.db
    std::shared_ptr<NativeRdb::RdbStore> mediaRdb_;
    // new media_liabrary.db
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::unordered_map<int32_t, PhotoInfo> photoInfoMap_;
    std::unordered_map<std::string, std::unordered_set<std::string>> intersectionMap_;

    std::vector<AnalysisAlbumInfo> analysisInfos_;
    std::vector<HighlightAlbumInfo> highlightInfos_;
    std::vector<HighlightCoverInfo> coverInfos_;
    std::vector<HighlightPlayInfo> playInfos_;

    std::mutex counterMutex_;
    std::unordered_map<std::string, int32_t> albumPhotoCounter_;
    bool isMapOrder_{false};
    bool isCloneHighlight_{false};
    bool isHighlightDirExist_{false};
    int32_t maxIdOfAlbum_{0};
    int32_t maxIdOfHighlight_{0};
    int32_t lastIdOfMap_{0};
    int64_t albumSuccessCnt_{0};
    int64_t albumDuplicateCnt_{0};
    int64_t albumFailedCnt_{0};
    int64_t highlightSuccessCnt_{0};
    int64_t highlightDuplicateCnt_{0};
    int64_t highlightFailedCnt_{0};
    int64_t mapSuccessCnt_{0};
    int64_t mapFailedCnt_{0};
    int64_t coverInfoSuccessCnt_{0};
    int64_t coverInfoFailedCnt_{0};
    int64_t playInfoSuccessCnt_{0};
    int64_t playInfoFailedCnt_{0};
    int64_t restoreTimeCost_{0};
};

template<typename T>
void CloneRestoreHighlight::PutIfPresent(NativeRdb::ValuesBucket& values, const std::string& columnName,
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
        }
    }
}

template<typename T>
void CloneRestoreHighlight::PutIfInIntersection(NativeRdb::ValuesBucket& values, const std::string& columnName,
    const std::optional<T>& optionalValue, const std::unordered_set<std::string> &intersection)
{
    if (intersection.count(columnName) > 0) {
        PutIfPresent<T>(values, columnName, optionalValue);
        return;
    }
}

template<typename T>
void CloneRestoreHighlight::GetIfInIntersection(const std::string &columnName, std::optional<T> &optionalValue,
    const std::unordered_set<std::string> &intersection, std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (intersection.count(columnName) > 0) {
        optionalValue = BackupDatabaseUtils::GetOptionalValue<T>(resultSet, columnName);
        return;
    }
}
} // namespace OHOS::Media
#endif // CLONE_RESTORE_HIGHLIGHT_H