/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_BACKUP_REPORT_DATA_TYPE_H
#define OHOS_MEDIA_BACKUP_REPORT_DATA_TYPE_H

#include <string>
#include <sstream>

namespace OHOS::Media {
struct AlbumMediaStatisticInfo {
    int32_t sceneCode{-1};
    std::string taskId;
    std::string albumName;
    int32_t totalCount{0};
    int32_t imageCount{0};
    int32_t videoCount{0};
    int32_t hiddenCount{0};
    int32_t trashedCount{0};
    int32_t cloudCount{0};
    int32_t favoriteCount{0};
    int32_t burstCoverCount{0};
    int32_t burstTotalCount{0};
    int32_t innerEastLakeCount{0};
    // non-event data members
    std::string lPath;
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "AlbumMediaStatisticInfo["
            << "sceneCode: " << sceneCode << ", "
            << "taskId: " << taskId << ", "
            << "albumName: " << albumName << ", "
            << "totalCount: " << totalCount << ", "
            << "imageCount: " << imageCount << ", "
            << "videoCount: " << videoCount << ", "
            << "hiddenCount: " << hiddenCount << ", "
            << "trashedCount: " << trashedCount << ", "
            << "cloudCount: " << cloudCount << ", "
            << "favoriteCount: " << favoriteCount << ", "
            << "burstTotalCount: " << burstTotalCount << ", "
            << "burstCoverCount: " << burstCoverCount << ", "
            << "innerEastLakeCount: " << innerEastLakeCount << "]";
        return ss.str();
    }
};

class MediaRestoreResultInfo {
public:
    int32_t sceneCode{-1};
    std::string taskId;
    std::string errorCode;
    std::string errorInfo;
    std::string type;
    std::string backupInfo;
    int duplicateCount{0};
    int failedCount{0};
    int successCount{0};

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "MediaRestoreResultInfo["
           << "sceneCode: " << this->sceneCode << ", taskId: " << this->taskId << ", errorCode: " << this->errorCode
           << ", errorInfo: " << this->errorInfo << ", type: " << this->type << ", backupInfo: " << this->backupInfo
           << ", duplicateCount: " << this->duplicateCount << ", failedCount: " << this->failedCount
           << ", successCount: " << this->successCount << "]";
        return ss.str();
    }
};

class CallbackBackupInfo {
public:
    std::string backupInfo;
    std::string details;
    int duplicateCount;
    int failedCount;
    int successCount;

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "BackupInfo[ "
           << "backupInfo: " << this->backupInfo << ", "
           << "details: " << this->details << ", "
           << "duplicateCount: " << this->duplicateCount << ", "
           << "failedCount: " << this->failedCount << ", "
           << "successCount: " << this->successCount << " ]";
        return ss.str();
    }
};

class CallbackResultInfo {
public:
    std::string errorCode;
    std::string errorInfo;
    std::string type;

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "ResultInfo[ "
           << "errorCode: " << this->errorCode << ", "
           << "errorInfo: " << this->errorInfo << ", "
           << "type: " << this->type << " ]";
        return ss.str();
    }
};

class CallbackResultData {
public:
    CallbackResultInfo resultInfo;
    std::vector<CallbackBackupInfo> infos;

private:
    std::string ToString(const std::vector<CallbackBackupInfo> &infoList) const
    {
        std::stringstream ss;
        ss << "infos[ ";
        for (const auto &info : infoList) {
            ss << info.ToString() << ", ";
        }
        ss << " ]";
        return ss.str();
    }

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "ResultData[ " << this->resultInfo.ToString() << ", " << this->ToString(this->infos) << " ]";
        return ss.str();
    }
};

class AlbumNameInfo {
private:
    std::string albumName_;
    std::string lPath_;
    int64_t costTime_;
    int32_t period_;
    int32_t dbType_;

public:
    AlbumNameInfo &SetAlbumName(const std::string &albumName)
    {
        this->albumName_ = albumName;
        return *this;
    }
    AlbumNameInfo &SetLPath(const std::string &lPath)
    {
        this->lPath_ = lPath;
        return *this;
    }
    AlbumNameInfo &SetCostTime(int64_t costTime)
    {
        this->costTime_ = costTime;
        return *this;
    }
    /**
     * @brief Set period. 0 - BEFORE, 1 - AFTER
     */
    AlbumNameInfo &SetPeriod(int32_t period)
    {
        this->period_ = period;
        return *this;
    }
    /**
     * @brief Set db type. 0 - GALLERY, 1 - MEDIA
     */
    AlbumNameInfo &SetDbType(int32_t dbType)
    {
        this->dbType_ = dbType;
        return *this;
    }
    /**
     * @brief Convert AlbumNameInfo to string. Format : albumName_lPath_dbTypeName_periodName_costTime
     */
    std::string ToString() const
    {
        std::string dbTypeName = this->dbType_ == 0 ? "GALLERY" : "MEDIA";
        std::string periodName = this->period_ == 0 ? "BEFORE" : (this->period_ == 1 ? "AFTER" : "OLD");
        std::stringstream ss;
        ss << this->albumName_ << "_" << this->lPath_ << "_" << dbTypeName << "_" << periodName << "_"
           << this->costTime_;
        return ss.str();
    }
};

struct AlbumStatisticInfo {
    std::string lPath;
    int32_t count;
    std::string albumName;
};

enum {
    DUAL_MEDIA_TYPE_ALL = 0,
    DUAL_MEDIA_TYPE_IMAGE = 1,
    DUAL_MEDIA_TYPE_VIDEO = 3,
    DUAL_SEARCH_TYPE_ALL = 0,
    DUAL_SEARCH_TYPE_CLOUD = 1,
    DUAL_HIDDEN_TYPE_SKIP = -1,
    DUAL_HIDDEN_TYPE_NOT_HIDDEN = 0,
    DUAL_HIDDEN_TYPE_HIDDEN = 1,
    DUAL_TRASHED_TYPE_SKIP = -1,
    DUAL_TRASHED_TYPE_NOT_TRASHED = 0,
    DUAL_TRASHED_TYPE_TRASHED = 1,
    DUAL_CLOUD_TYPE_SKIP = -1,
    DUAL_CLOUD_TYPE_NOT_CLOUD = 0,
    DUAL_CLOUD_TYPE_CLOUD = 1,
    DUAL_FAVORITE_TYPE_SKIP = -1,
    DUAL_FAVORITE_TYPE_ALL = 0,
    DUAL_FAVORITE_TYPE_FAVORITE = 1,
    DUAL_BURST_TYPE_SKIP = -1,
    DUAL_BURST_TYPE_ALL = 0,
    DUAL_BURST_TYPE_COVER = 1,
    DUAL_LAKE_TYPE_SKIP = -1,
    DUAL_LAKE_TYPE_INNER = 3,
};

enum {
    SINGLE_MEDIA_TYPE_ALL = 0,
    SINGLE_MEDIA_TYPE_IMAGE = 1,
    SINGLE_MEDIA_TYPE_VIDEO = 2,
    SINGLE_SEARCH_TYPE_ALL = 0,
    SINGLE_SEARCH_TYPE_CLOUD = 2,
    SINGLE_HIDDEN_TYPE_SKIP = -1,
    SINGLE_HIDDEN_TYPE_NOT_HIDDEN = 0,
    SINGLE_HIDDEN_TYPE_HIDDEN = 1,
    SINGLE_TRASHED_TYPE_SKIP = -1,
    SINGLE_TRASHED_TYPE_NOT_TRASHED = 0,
    SINGLE_TRASHED_TYPE_TRASHED = 1,
    SINGLE_CLOUD_TYPE_SKIP = -1,
    SINGLE_CLOUD_TYPE_NOT_CLOUD = 0,
    SINGLE_CLOUD_TYPE_CLOUD = 1,
    SINGLE_FAVORITE_TYPE_SKIP = -1,
    SINGLE_FAVORITE_TYPE_ALL = 0,
    SINGLE_FAVORITE_TYPE_FAVORITE = 1,
    SINGLE_BURST_TYPE_SKIP = -1,
    SINGLE_BURST_TYPE_ALL = 0,
    SINGLE_BURST_TYPE_COVER = 1,
    SINGLE_LAKE_TYPE_SKIP = -1,
    SINGLE_LAKE_TYPE_INNER = 3,
};

class SearchCondition {
private:
    int32_t mediaType_ = DUAL_MEDIA_TYPE_ALL;
    int32_t hiddenType_ = DUAL_HIDDEN_TYPE_SKIP;
    int32_t trashedType_ = DUAL_TRASHED_TYPE_SKIP;
    int32_t cloudType_ = DUAL_CLOUD_TYPE_SKIP;
    int32_t favoriteType_ = DUAL_FAVORITE_TYPE_SKIP;
    int32_t burstType_ = DUAL_BURST_TYPE_SKIP;
    int32_t lakeType_ = DUAL_LAKE_TYPE_SKIP;

public:
    int32_t GetMediaType()
    {
        return this->mediaType_;
    }
    SearchCondition &SetMediaType(int32_t mediaType)
    {
        this->mediaType_ = mediaType;
        return *this;
    }
    int32_t GetHiddenType()
    {
        return this->hiddenType_;
    }
    SearchCondition &SetHiddenType(int32_t hiddenType)
    {
        this->hiddenType_ = hiddenType;
        return *this;
    }
    int32_t GetTrashedType()
    {
        return this->trashedType_;
    }
    SearchCondition &SetTrashedType(int32_t trashedType)
    {
        this->trashedType_ = trashedType;
        return *this;
    }
    int32_t GetCloudType()
    {
        return this->cloudType_;
    }
    SearchCondition &SetCloudType(int32_t cloudType)
    {
        this->cloudType_ = cloudType;
        return *this;
    }
    int32_t GetFavoriteType()
    {
        return this->favoriteType_;
    }
    SearchCondition &SetFavoriteType(int32_t favoriteType)
    {
        this->favoriteType_ = favoriteType;
        return *this;
    }
    int32_t GetBurstType()
    {
        return this->burstType_;
    }
    SearchCondition &SetBurstType(int32_t burstType)
    {
        this->burstType_ = burstType;
        return *this;
    }
    int32_t GetLakeType()
    {
        return this->lakeType_;
    }
    SearchCondition &SetLakeType(int32_t lakeType)
    {
        this->lakeType_ = lakeType;
        return *this;
    }
};

struct ErrorInfo {
    int32_t error{-1};
    int32_t count{0};
    std::string status;
    std::string extend;
    ErrorInfo(int32_t error, int32_t count, int32_t errorCode)
        : error(error), count(count), status(std::to_string(errorCode)) {}
    ErrorInfo(int32_t error, int32_t count, const std::string &status, const std::string &extend)
        : error(error), count(count), status(status), extend(extend) {}
};

struct FileDbCheckInfo {
    int32_t dbType{-1};
    int32_t dbStatus{-1};
    int32_t fileStatus{-1};
    FileDbCheckInfo(int32_t dbType, int32_t dbStatus, int32_t fileStatus)
        : dbType(dbType), dbStatus(dbStatus), fileStatus(fileStatus) {}
};

struct RestoreTaskInfo {
    std::string type;
    std::string errorCode;
    std::string errorInfo;
    int32_t successCount{0};
    int32_t failedCount{0};
    int32_t duplicateCount{0};
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_REPORT_DATA_TYPE_H