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

#define MLOG_TAG "AccurateRefresh::AssetDataManager"

#include <sstream>

#include "asset_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "accurate_debug_log.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

AssetDataManager::~AssetDataManager()
{
    ACCURATE_DEBUG("multiThreadAssetIds_ size: %{public}zu", multiThreadAssetIds_.size());
    ClearMultiThreadChangeDatas();
}

int32_t AssetDataManager::UpdateModifiedDatas()
{
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetDataManager::PostProcessModifiedDatas(const std::vector<int32_t> &keys)
{
    MEDIA_DEBUG_LOG("keys size: %{public}zu", keys.size());
    for (auto key : keys) {
        auto iter = changeDatas_.find(key);
        if (iter == changeDatas_.end()) {
            MEDIA_WARN_LOG("not init info for update common modified data.");
            continue;
        }
        PhotoAssetChangeData &assetChangeData = iter->second;
        assetChangeData.thumbnailChangeStatus_ = UpdateThumbnailChangeStatus(assetChangeData);
        assetChangeData.isContentChanged_ =
            assetChangeData.thumbnailChangeStatus_ == ThumbnailChangeStatus::THUMBNAIL_UPDATE;
    }
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetDataManager::UpdateThumbnailChangeStatus(PhotoAssetChangeData &assetChangeData)
{
    int32_t visibleAfter = assetChangeData.infoAfterChange_.thumbnailVisible_;
    int32_t visibleBefore = assetChangeData.infoBeforeChange_.thumbnailVisible_;
    int64_t readyBefore = assetChangeData.infoBeforeChange_.thumbnailReady_;
    int64_t readyAfter = assetChangeData.infoAfterChange_.thumbnailReady_;
    MEDIA_DEBUG_LOG("UpdateThumbnailChangeStatus visibleBefore: %{public}d, visibleAfter: %{public}d, "
        "readyBefore: %{public}" PRId64 ", readyAfter: %{public}" PRId64,
        visibleBefore, visibleAfter, readyBefore, readyAfter);
    if (visibleAfter == 0 || visibleAfter == INVALID_INT32_VALUE) {
        return ThumbnailChangeStatus::THUMBNAIL_NOT_EXISTS;
    }
    if (visibleBefore != visibleAfter) {
        return ThumbnailChangeStatus::THUMBNAIL_ADD;
    }
    if (readyBefore == readyAfter) {
        return ThumbnailChangeStatus::THUMBNAIL_NOT_CHANGE;
    } else {
        return ThumbnailChangeStatus::THUMBNAIL_UPDATE;
    }
}

void AssetDataManager::SetAlbumIdFromChangeDates()
{
    for (const auto &[key, changeInfo] : this->changeDatas_) {
        this->uniqueAlbumIds_.insert(changeInfo.infoBeforeChange_.ownerAlbumId_);
        this->uniqueAlbumIds_.insert(changeInfo.infoAfterChange_.ownerAlbumId_);
        int32_t fileId = changeInfo.GetFileId();
        CHECK_AND_CONTINUE(fileId != INVALID_INT32_VALUE);
        this->uniqueFileIds_.insert(fileId);
    }
}

void AssetDataManager::SetAlbumIdByChangeInfos(const vector<PhotoAssetChangeInfo> &changeInfos)
{
    for (const PhotoAssetChangeInfo &changeInfo : changeInfos) {
        this->uniqueAlbumIds_.insert(changeInfo.ownerAlbumId_);
        CHECK_AND_CONTINUE(changeInfo.fileId_ != INVALID_INT32_VALUE);
        this->uniqueFileIds_.insert(changeInfo.fileId_);
    }
}

bool AssetDataManager::CheckIsExceed(bool isLengthChanged)
{
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::CheckIsExceed(const AbsRdbPredicates &predicates, bool isLengthChanged)
{
    SetAlbumIdsByPredicates(predicates);
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::CheckIsExceed(const string &sql,
    const vector<ValueObject> &bindArgs, bool isLengthChanged)
{
    SetAlbumIdsBySql(sql, bindArgs);
    if (!isLengthChanged) {
        return isExceed_;
    }

    if (isExceed_) {
        return true;
    }

    isExceed_ = this->changeDatas_.size() >= MAX_DATA_LENGTH;
    if (isExceed_) {
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::CheckIsExceed(size_t length)
{
    if (length >= MAX_DATA_LENGTH) {
        isExceed_ = true;
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::CheckIsExceed(const vector<int32_t> &keys)
{
    SetAlbumIdsByFileds(keys);
    if (keys.size() >= MAX_DATA_LENGTH) {
        isExceed_ = true;
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::CheckIsExceedInMultiThread(const vector<int32_t> &keys)
{
    SetAlbumIdsByFileds(keys);
    if (MultiThreadAssetChangeInfoMgr::GetInstance().CheckIsExceed()) {
        isExceed_ = true;
        this->SetAlbumIdFromChangeDates();
        this->changeDatas_.clear();
    }
    return isExceed_;
}

bool AssetDataManager::GetIsExceedStatus()
{
    return isExceed_;
}

int32_t AssetDataManager::GetChangeInfoKey(const PhotoAssetChangeInfo &changeInfo)
{
    return changeInfo.fileId_;
}

std::vector<PhotoAssetChangeInfo> AssetDataManager::GetInfoByKeys(const std::vector<int32_t> &fileIds)
{
    AbsRdbPredicates rbdPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> fileIdStrs;
    for (const auto &fileId : fileIds) {
        fileIdStrs.push_back(to_string(fileId));
    }
    rbdPredicates.In(PhotoColumn::MEDIA_ID, fileIdStrs);
    return GetInfosByPredicates(rbdPredicates);
}

int32_t AssetDataManager::SetAlbumIdsByPredicates(const AbsRdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetDataManager::SetAlbumIdsByPredicates");
    shared_ptr<ResultSet> resultSet;
    if (CanTransOperate()) {
        resultSet = trans_->QueryByStep(predicates, {PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, NativeRdb::E_ERROR, "rdbStore null");
        resultSet = rdbStore->QueryByStep(predicates, {PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "resultSet null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        auto albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet,
            PhotoAssetChangeInfo::GetDataType(PhotoColumn::PHOTO_OWNER_ALBUM_ID)));
        uniqueAlbumIds_.insert(albumId);
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t AssetDataManager::SetAlbumIdsBySql(const std::string &sql,
    const std::vector<ValueObject> &bindArgs)
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetDataManager::SetAlbumIdsBySql");
    shared_ptr<ResultSet> resultSet;
    if (CanTransOperate()) {
        resultSet = trans_->QueryByStep(sql, bindArgs);
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ACCURATE_REFRESH_RDB_NULL, "rdbStore null");
        resultSet = rdbStore->QueryByStep(sql, bindArgs);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, NativeRdb::E_ERROR, "resultSet null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        uniqueAlbumIds_.insert(get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet,
            PhotoAssetChangeInfo::GetDataType(PhotoColumn::PHOTO_OWNER_ALBUM_ID))));
    }
    resultSet->Close();
    return NativeRdb::E_OK;
}

int32_t AssetDataManager::SetAlbumIdsByFileds(const std::vector<int32_t> &fileIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetDataManager::SetAlbumIdsByFileds");
    AbsRdbPredicates rbdPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> fileIdStrs;
    for (const auto &fileId : fileIds) {
        fileIdStrs.push_back(to_string(fileId));
    }
    rbdPredicates.In(PhotoColumn::MEDIA_ID, fileIdStrs);
    return SetAlbumIdsByPredicates(rbdPredicates);
}

bool AssetDataManager::CheckIsForRecheck()
{
    return isForRecheck_ || CheckIsExceed();
}

vector<PhotoAssetChangeInfo> AssetDataManager::GetInfosByPredicates(const AbsRdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("AssetDataManager::GetInfosByPredicates");
    shared_ptr<ResultSet> resultSet;
    if (CanTransOperate()) {
        resultSet = trans_->QueryByStep(predicates, PhotoAssetChangeInfo::GetPhotoAssetColumns());
    } else {
        auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, vector<PhotoAssetChangeInfo>(), "rdbStore null");
        resultSet = rdbStore->QueryByStep(predicates, PhotoAssetChangeInfo::GetPhotoAssetColumns());
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, vector<PhotoAssetChangeInfo>(), "resultSet null");
    auto ret = GetInfosByResult(resultSet);
    resultSet->Close();
    return ret;
}

vector<PhotoAssetChangeInfo> AssetDataManager::GetInfosByResult(const shared_ptr<ResultSet> &resultSet)
{
    // 根据resultSet转PhotoAssetChangeInfo
    return PhotoAssetChangeInfo::GetInfoFromResult(resultSet);
}

vector<int32_t> AssetDataManager::GetInitKeys()
{
    stringstream ss;
    ss << "GetInitKeys:";
    vector<int32_t> fileIds;
    for (auto &changeData : changeDatas_) {
        fileIds.push_back(changeData.first);
        ss << " " << changeData.first;
    }

    ACCURATE_DEBUG("%{public}s", ss.str().c_str());
    return fileIds;
}

int32_t AssetDataManager::SetContentChanged(int32_t fileId, bool isChanged)
{
    auto iter = contentInfos.find(fileId);
    if (iter == contentInfos.end()) {
        PhotoAssetContentInfo contentInfo;
        contentInfo.isContentChanged_ = isChanged;
        contentInfos.emplace(fileId, contentInfo);
        ACCURATE_DEBUG("add fileId: %{public}d, isChanged %{public}d", fileId, isChanged);
    } else {
        iter->second.isContentChanged_ = isChanged;
        ACCURATE_DEBUG("update fileId: %{public}d, isChanged %{public}d", fileId, isChanged);
    }
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetDataManager::SetThumbnailStatus(int32_t fileId, int32_t status)
{
    auto iter = contentInfos.find(fileId);
    if (iter == contentInfos.end()) {
        PhotoAssetContentInfo contentInfo;
        contentInfo.thumbnailChangeStatus_ = status;
        contentInfos.emplace(fileId, contentInfo);
        ACCURATE_DEBUG("add fileId: %{public}d, status: %{public}d", fileId, status);
    } else {
        iter->second.thumbnailChangeStatus_ = status;
        ACCURATE_DEBUG("update fileId: %{public}d, status: %{public}d", fileId, status);
    }
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AssetDataManager::UpdateNotifyInfo()
{
    for (auto const &contentInfo : contentInfos) {
        auto fileId = contentInfo.first;
        const PhotoAssetContentInfo &content = contentInfo.second;
        auto iter = changeDatas_.find(fileId);
        if (iter == changeDatas_.end()) {
            MEDIA_ERR_LOG("no changeDatas: %{public}d.", fileId);
            continue;
        }
        iter->second.isContentChanged_ = content.isContentChanged_;
        iter->second.thumbnailChangeStatus_ = content.thumbnailChangeStatus_;
        ACCURATE_DEBUG("update fileId:%{public}d, content(%{public}d, %{public}d)", fileId, content.isContentChanged_,
            content.thumbnailChangeStatus_);
    }
    return ACCURATE_REFRESH_RET_OK;
}

void AssetDataManager::PostInsertBeforeData(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo)
{
    UpdatePendingInfo(changeData, pendingInfo);
    if (changeData.infoBeforeChange_.fileId_ == INVALID_INT32_VALUE) {
        MEDIA_ERR_LOG("invalid fileId");
        return;
    }

    MultiThreadAssetChangeInfoMgr::GetInstance().CheckInsertBeforeInfo(changeData.infoBeforeChange_);
    multiThreadAssetIds_.insert(changeData.infoBeforeChange_.fileId_);
}

void AssetDataManager::PostInsertAfterData(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo, bool isAdd,
    int fileIdBeforeRemove)
{
    UpdatePendingInfo(changeData, pendingInfo);
    int fileId =
        changeData.infoAfterChange_.fileId_ == INVALID_INT32_VALUE && fileIdBeforeRemove != INVALID_INT32_VALUE ?
        fileIdBeforeRemove : changeData.infoAfterChange_.fileId_;
    if (fileId == INVALID_INT32_VALUE) {
        MEDIA_ERR_LOG("invalid fileId");
        return;
    }
    if (MultiThreadAssetChangeInfoMgr::GetInstance().CheckInsertAfterInfo(changeData.infoAfterChange_, isAdd,
        fileIdBeforeRemove)) {
        multiThreadAssetIds_.insert(fileId);
        changeData.isMultiOperation_ = true;
    } else {
        multiThreadAssetIds_.erase(fileId);
    }
}

bool AssetDataManager::CheckUpdateDataForMultiThread(PhotoAssetChangeData &changeData)
{
    if (!changeData.isMultiOperation_) {
        return false;
    }
    auto infos = MultiThreadAssetChangeInfoMgr::GetInstance().GetAndUpdateAssetChangeData(changeData.GetFileId());
    changeData.infoBeforeChange_ = infos.first;
    changeData.infoAfterChange_ = infos.second;
    changeData.isMultiOperation_ = false;
    ClearMultiThreadChangeData(changeData.GetFileId());
    return true;
}

void AssetDataManager::ClearMultiThreadChangeData(int32_t fileId)
{
    MultiThreadAssetChangeInfoMgr::GetInstance().ClearMultiThreadChangeData(fileId);
    multiThreadAssetIds_.erase(fileId);
}

void AssetDataManager::ClearMultiThreadChangeDatas()
{
    if (multiThreadAssetIds_.empty()) {
        return;
    }
    for (auto fileId : multiThreadAssetIds_) {
        MultiThreadAssetChangeInfoMgr::GetInstance().ClearMultiThreadChangeData(fileId);
    }
    multiThreadAssetIds_.clear();
}

void AssetDataManager::UpdatePendingInfo(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo)
{
    if (pendingInfo.start_ != INVALID_INT64_VALUE && changeData.infoBeforeChange_.timestamp_ == INVALID_INT64_VALUE) {
        changeData.infoBeforeChange_.timestamp_ = pendingInfo.start_;
    }
    if (pendingInfo.end_ != INVALID_INT64_VALUE && changeData.infoAfterChange_.timestamp_ == INVALID_INT64_VALUE) {
        changeData.infoAfterChange_.timestamp_ = pendingInfo.end_;
    }
}

} // namespace Media
} // namespace OHOS
