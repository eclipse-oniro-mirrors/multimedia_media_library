/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AccurateRefresh::AlbumDataManager"

#include "media_file_utils.h"
#include "album_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "accurate_debug_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_tracer.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

int32_t AlbumDataManager::InitAlbumInfos(const vector<int> &albumIds)
{
    if (albumIds.empty()) {
        MEDIA_WARN_LOG("albumIds empty.");
        return ACCURATE_REFRESH_INPUT_PARA_ERR;
    }

    MediaLibraryTracer tracer;
    tracer.Start("AlbumDataManager::InitAlbumInfos");

    auto initDatas = GetAlbumInfos(albumIds);
    if (initDatas.empty()) {
        MEDIA_WARN_LOG("initDatas empty.");
    } else {
        InsertInitChangeInfos(initDatas);
    }

    return ACCURATE_REFRESH_RET_OK;
}


int32_t AlbumDataManager::UpdateModifiedDatas()
{
    return ACCURATE_REFRESH_RET_OK;
}

PhotoAssetChangeInfo AlbumDataManager::GetPhotoAssetInfo(int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumDataManager::GetPhotoAssetInfo");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, PhotoAssetChangeInfo(), "rdbStore null");
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    auto resultSet = rdbStore->QueryByStep(predicates, PhotoAssetChangeInfo::GetPhotoAssetColumns());
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, PhotoAssetChangeInfo(), "resultSet null");
    auto changeInfos = PhotoAssetChangeInfo::GetInfoFromResult(resultSet);
    resultSet->Close();
    if (changeInfos.size() != 1) {
        MEDIA_WARN_LOG("changeInfos[%{public}d] size[%{public}zu] wrong.", fileId, changeInfos.size());
        return PhotoAssetChangeInfo();
    }
    return changeInfos[0];
}

int32_t AlbumDataManager::PostProcessModifiedDatas(const std::vector<int32_t> &keys)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumDataManager::PostProcessModifiedDatas");
    for (auto &key : keys) {
        auto item = changeDatas_.find(key);
        if (item == changeDatas_.end()) {
            MEDIA_WARN_LOG("no data, albumId[%{public}d].", key);
            continue;
        }
        auto &before = item->second.infoBeforeChange_;
        auto &after = item->second.infoAfterChange_;
        // 更新cover相关信息
        if (before.coverUri_ != after.coverUri_) {
            after.isCoverChange_ = true;
            if (after.coverInfo_.fileId_ == INVALID_INT32_VALUE && !after.coverUri_.empty()) {
                auto coverFileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(after.coverUri_);
                after.coverInfo_ = GetPhotoAssetInfo(coverFileId);
            }
        }
 
        // 更新hidden cover相关信息
        if (before.hiddenCoverUri_ != after.hiddenCoverUri_) {
            after.isHiddenCoverChange_ = true;
            if (after.hiddenCoverInfo_.fileId_ == INVALID_INT32_VALUE && !after.hiddenCoverUri_.empty()) {
                auto coverFileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(after.hiddenCoverUri_);
                after.hiddenCoverInfo_ = GetPhotoAssetInfo(coverFileId);
            }
        }
        MEDIA_DEBUG_LOG("coverChange[%{public}d]:[%{public}s] hiddenCoverChange[%{public}d]:[%{public}s]",
            after.isCoverChange_, after.coverInfo_.ToString().c_str(), after.isHiddenCoverChange_,
            after.hiddenCoverInfo_.ToString().c_str());
    }
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumDataManager::GetChangeInfoKey(const AlbumChangeInfo &changeInfo)
{
    return changeInfo.albumId_;
}

vector<AlbumChangeInfo> AlbumDataManager::GetInfoByKeys(const vector<int32_t> &albumIds)
{
    return GetAlbumInfos(albumIds);
}

vector<AlbumChangeInfo> AlbumDataManager::GetInfosByPredicates(const AbsRdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumDataManager::GetInfosByPredicates");
    shared_ptr<ResultSet> resultSet;
    if (CanTransOperate()) {
        resultSet = trans_->QueryByStep(predicates, AlbumChangeInfo::GetAlbumInfoColumns());
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, vector<AlbumChangeInfo>(), "rdbStore null");
        resultSet = rdbStore->QueryByStep(predicates, AlbumChangeInfo::GetAlbumInfoColumns());
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, vector<AlbumChangeInfo>(), "resultSet null");
    
    // 根据resultSet转AlbumChangeInfo
    auto albumInfos = GetInfosByResult(resultSet);
    resultSet->Close();
    
    return albumInfos;
}

vector<AlbumChangeInfo> AlbumDataManager::GetInfosByResult(const shared_ptr<ResultSet> &resultSet)
{
    return AlbumChangeInfo::GetInfoFromResult(resultSet);
}

vector<AlbumChangeInfo> AlbumDataManager::GetAlbumInfos(const vector<int32_t> &albumIds,
    const vector<string> systemTypes)
{
    vector<AlbumChangeInfo> albumInfos;
    if (!systemTypes.empty()) {
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, systemTypes);
        albumInfos = GetInfosByPredicates(predicates);
    }

    if (!albumIds.empty()) {
        vector<string> albumIdStrs;
        for (auto const &albumId : albumIds) {
            albumIdStrs.push_back(to_string(static_cast<int> (albumId)));
        }
        vector<AlbumChangeInfo> albumIdInfos;
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIdStrs);
        albumIdInfos = GetInfosByPredicates(predicates);
        albumInfos.insert(albumInfos.end(), albumIdInfos.begin(), albumIdInfos.end());
    }
    return albumInfos;
}

unordered_map<int32_t, AlbumChangeInfo> AlbumDataManager::GetInitAlbumInfos()
{
    unordered_map<int32_t, AlbumChangeInfo> initAlbumInfos;
    for (auto &item : changeDatas_) {
        initAlbumInfos.emplace(item.first, item.second.infoBeforeChange_);
    }
    return initAlbumInfos;
}

vector<int32_t> AlbumDataManager::GetInitKeys()
{
    stringstream ss;
    ss << "GetInitKeys:";
    vector<int32_t> albumIds;
    for (auto &changeData : changeDatas_) {
        albumIds.push_back(changeData.first);
        ss << " " << changeData.first;
    }

    ACCURATE_DEBUG("%{public}s", ss.str().c_str());
    return albumIds;
}

vector<AlbumChangeData> AlbumDataManager::GetAlbumDatasFromAddAlbum(const vector<string> &albumIdsStr)
{
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIdsStr);
    if (albumIdsStr.size() > 0) {
        ACCURATE_DEBUG("albumId: %{public}s", albumIdsStr[0].c_str());
    } else {
        ACCURATE_DEBUG("albumId empty");
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    vector<AlbumChangeData> albumChangeDatas;
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, albumChangeDatas, "rdbStore null");
    auto resultSet = rdbStore->QueryByStep(predicates, AlbumChangeInfo::GetAlbumInfoColumns());
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumChangeDatas, "resultSet null");
    auto albumIdInfos = AlbumChangeInfo::GetInfoFromResult(resultSet);
    resultSet->Close();
    for (auto &albumInfo : albumIdInfos) {
        AlbumChangeData changeData;
        changeData.operation_ = RDB_OPERATION_ADD;
        changeData.version_ = MediaFileUtils::UTCTimeMilliSeconds();
        changeData.infoAfterChange_ = albumInfo;
        albumChangeDatas.push_back(changeData);
        ACCURATE_DEBUG("albumInfo: %{public}s", albumInfo.ToString().c_str());
    }
    return albumChangeDatas;
}

vector<AlbumChangeData> AlbumDataManager::GetAlbumDatasForUpdateNoChange(const vector<int32_t> &albumIds)
{
    vector<string> albumIdStrs;
    for (auto const &albumId : albumIds) {
        albumIdStrs.push_back(to_string(static_cast<int>(albumId)));
    }
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIdStrs);
    if (!albumIds.empty()) {
        ACCURATE_DEBUG("albumId: %{public}d", albumIds[0]);
    } else {
        ACCURATE_DEBUG("albumId empty");
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    vector<AlbumChangeData> albumChangeDatas;
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, albumChangeDatas, "rdbStore null");
    auto resultSet = rdbStore->QueryByStep(predicates, AlbumChangeInfo::GetAlbumInfoColumns());
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumChangeDatas, "resultSet null");
    auto albumIdInfos = AlbumChangeInfo::GetInfoFromResult(resultSet);
    resultSet->Close();
    for (auto &albumInfo : albumIdInfos) {
        AlbumChangeData changeData;
        changeData.operation_ = RDB_OPERATION_UPDATE;
        changeData.version_ = MediaFileUtils::UTCTimeMilliSeconds();
        changeData.infoBeforeChange_ = albumInfo;
        changeData.infoAfterChange_ = albumInfo;
        albumChangeDatas.push_back(changeData);
        ACCURATE_DEBUG("albumInfo: %{public}s", albumInfo.ToString().c_str());
    }
    return albumChangeDatas;
}

int32_t AlbumDataManager::SetAlbumIdsByPredicates(const AbsRdbPredicates &predicates)
{
    // 直接更新操作相册的，不用存相册id
    return NativeRdb::E_OK;
}

int32_t AlbumDataManager::SetAlbumIdsBySql(const std::string &sql,
    const std::vector<ValueObject> &bindArgs)
{
    // 直接更新操作相册的，不用存相册id
    return NativeRdb::E_OK;
}

int32_t AlbumDataManager::SetAlbumIdsByFileds(const std::vector<int32_t> &fileIds)
{
    // 直接更新操作相册的，不用存相册id
    return NativeRdb::E_OK;
}

void AlbumDataManager::ClearChangeInfos()
{
    this->changeDatas_.clear();
}

bool AlbumDataManager::CheckIsExceed(bool isLengthChanged)
{
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->changeDatas_.clear();
    }
    return isExceed_;
};

bool AlbumDataManager::CheckIsExceed(const AbsRdbPredicates &predicates, bool isLengthChanged)
{
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->changeDatas_.clear();
    }
    return isExceed_;
};

bool AlbumDataManager::CheckIsExceed(const string &sql,
    const vector<ValueObject> &bindArgs, bool isLengthChanged)
{
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->changeDatas_.clear();
    }
    return isExceed_;
};

bool AlbumDataManager::CheckIsExceed(size_t length)
{
    if (length >= MAX_DATA_LENGTH) {
        isExceed_ = true;
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AlbumDataManager::CheckIsExceed(const vector<int32_t> &keys)
{
    if (keys.size() >= MAX_DATA_LENGTH) {
        isExceed_ = true;
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AlbumDataManager::CheckIsForRecheck()
{
    return isForRecheck_ || CheckIsExceed();
}

} // namespace Media
} // namespace OHOS
