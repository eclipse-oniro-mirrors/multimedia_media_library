/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloneToAlbumService"

#include "clone_to_album_service.h"

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <charconv>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "clone_to_album_callback_proxy.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "media_file_utils.h"
#include "media_edit_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "result_set_utils.h"
#include "iremote_object.h"
#include "iremote_proxy.h"
#include "rdb_utils.h"
#include "medialibrary_album_fusion_utils.h"
#include "media_file_uri.h"
#include "medialibrary_data_manager_utils.h"
#include "asset_operation_info.h"
#include "media_file_access_utils.h"
#include "photo_file_utils.h"
#include "media_file_notify_info.h"
#include "file_manager_scanner.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_tracer.h"
#include "file_management_utils.h"
#include "photo_album.h"
#include "photo_day_month_year_operation.h"
#include "moving_photo_file_utils.h"

namespace OHOS {
namespace Media {

constexpr size_t COPY_WORKER_TIME_INTERVAL = 1000;
constexpr int32_t E_CANCELLED = -10086;
constexpr int32_t NOT_SUPPORT_RENAME = 1;
constexpr int64_t MIN_FREE_SPACE_FOR_SHARE_CLONE = 5LL * 1024 * 1024 * 1024;
const std::string TARGET_DIR = "/storage/media/local/files";
const std::string DOCS_DIR = "/storage/media/local/files/Docs";
const std::string DOCS_LPATH = "/FromDocs";
constexpr int32_t DOCS_LPATH_LENGTH = 9;
const std::string CLONE_FILE_ROOT_LPATH = "/FromDocs/";
const std::string CLONE_FILE_ROOT_ALBUM = "根目录";
const std::string  RELATIVE_PATH = "../";
constexpr int32_t SHARED_ASSET_FLAG = 1;


shared_ptr<NativeRdb::ResultSet> QueryGetAlbumByAlbumId(const int32_t &albumId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    return rdbStore->Query(predicates, {});
}

int32_t CloneToAlbumService::ValidateRequest(CloneToAlbumReqBody &reqBody)
{
    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("assetsArray is empty");
        return E_ERR;
    }
    if (reqBody.albumId <= 0) {
        MEDIA_ERR_LOG("albumId is invalid");
        return E_ERR;
    }
    if (!reqBody.progressCallback) {
        MEDIA_ERR_LOG("callback is invalid");
        return E_ERR;
    }

    auto resultSet = QueryGetAlbumByAlbumId(reqBody.albumId);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INNER_FAIL, "resultSet is nullptr");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        reqBody.albumLpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        reqBody.albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        reqBody.albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
        resultSet->Close();
        return E_OK;
    }
    resultSet->Close();
    MEDIA_ERR_LOG("query album info failed");
    return E_ERR;
}

static std::string GetThumbnailPathFromOrignalPath(std::string srcPath)
{
    if (srcPath.empty()) {
        MEDIA_ERR_LOG("source file invalid!");
        return "";
    }
    std::string photoRelativePath = "/Photo/";
    std::string thumbRelativePath = "/.thumbs/Photo/";
    size_t pos = srcPath.find(photoRelativePath);
    std::string thumbnailPath = "";
    if (pos != std::string::npos) {
        thumbnailPath = srcPath.replace(pos, photoRelativePath.length(), thumbRelativePath);
    }
    return thumbnailPath;
}

int32_t CheckFileName(CloneAssetInfo &cloneAssetInfo, std::unordered_set<std::string> &occupiedPaths)
{
    std::string targetPath;
    if (cloneAssetInfo.albumSubType == static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER)) {
        if (cloneAssetInfo.albumLpath == CLONE_FILE_ROOT_LPATH) {
            targetPath = DOCS_DIR + cloneAssetInfo.albumLpath.substr(DOCS_LPATH_LENGTH) + cloneAssetInfo.displayName;
        } else {
            targetPath = DOCS_DIR + cloneAssetInfo.albumLpath.substr(DOCS_LPATH_LENGTH) + "/" +
                cloneAssetInfo.displayName;
        }
        std::string renamePath;
        std::string renameTitle;
        std::string renameDisplayName;
        auto conflictChecker = [&occupiedPaths](const std::string &path) {
            return MediaFileUtils::IsFileExists(path) || occupiedPaths.find(path) != occupiedPaths.end();
        };
        if (cloneAssetInfo.burstKey.empty()) {
            MediaFileAccessUtils::HandleSameNameRename(targetPath, renamePath, renameTitle, renameDisplayName,
                conflictChecker);
        } else {
            MediaFileAccessUtils::HandleBurstSameNameRename(targetPath, renamePath, renameTitle, renameDisplayName,
                conflictChecker);
        }
        if (targetPath != renamePath && cloneAssetInfo.mode == NOT_SUPPORT_RENAME) {
            MEDIA_ERR_LOG("HandleSameName error");
            return E_SCENE_HAS_RENAMED;
        } else {
            cloneAssetInfo.targetFilePath = renamePath;
            cloneAssetInfo.targetFileTitle = renameTitle;
            cloneAssetInfo.targetDisplayName = renameDisplayName;
            occupiedPaths.insert(renamePath);
        }
    } else {
        cloneAssetInfo.targetDisplayName = cloneAssetInfo.displayName;
    }
    return E_OK;
}

static void FillBurstCloneAssetInfo(CloneAssetInfo &burstCloneAssetInfo,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    burstCloneAssetInfo.fileId = GetInt32Val(PhotoColumn::MEDIA_ID, resultSet);
    burstCloneAssetInfo.filePath = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    burstCloneAssetInfo.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    burstCloneAssetInfo.mediaType = GetInt32Val(PhotoColumn::MEDIA_TYPE, resultSet);
    burstCloneAssetInfo.size = GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSet);
    burstCloneAssetInfo.hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    burstCloneAssetInfo.dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    burstCloneAssetInfo.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    burstCloneAssetInfo.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    burstCloneAssetInfo.sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    burstCloneAssetInfo.burstKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
    burstCloneAssetInfo.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    burstCloneAssetInfo.burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
}

int32_t CloneToAlbumService::QueryBurstAssetInfo(CloneAssetInfo &cloneAssetInfo, uint64_t &displayTotalSize,
    uint64_t &actualTotalSize, bool isShareAlbumTarget)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::MEDIA_ID, cloneAssetInfo.fileId);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, cloneAssetInfo.burstKey);
    std::vector<std::string> columns = {
    PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME, PhotoColumn::MEDIA_TYPE,
    PhotoColumn::MEDIA_SIZE, MediaColumn::MEDIA_HIDDEN, MediaColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_SOURCE_PATH,
    PhotoColumn::PHOTO_BURST_KEY, PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL
    };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow()!= NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Asset not found, fileId:%{public}" PRId64, cloneAssetInfo.fileId);
        return E_SCENE_HAS_RENAMED;
    }
    struct stat editStatInfo {};
    struct stat thumStatInfo {};
    do {
        CloneAssetInfo burstCloneAssetInfo;
        FillBurstCloneAssetInfo(burstCloneAssetInfo, resultSet);
        actualTotalSize += static_cast<uint64_t>(burstCloneAssetInfo.size);
        if (!isShareAlbumTarget) {
            std::string editDataPath = MediaEditUtils::GetEditDataPath(burstCloneAssetInfo.filePath);
            if (stat(editDataPath.c_str(), &editStatInfo) == E_OK) {
                actualTotalSize += static_cast<uint64_t>(editStatInfo.st_size);
            }
        }
        std::string thumbnailPath = GetThumbnailPathFromOrignalPath(burstCloneAssetInfo.filePath);
        if (stat(thumbnailPath.c_str(), &thumStatInfo) == E_OK) {
            actualTotalSize += static_cast<uint64_t>(thumStatInfo.st_size);
        }
        displayTotalSize += static_cast<uint64_t>(burstCloneAssetInfo.size);
        burstCloneAssetInfo.albumId = cloneAssetInfo.albumId;
        burstCloneAssetInfo.targetDisplayName = burstCloneAssetInfo.displayName;
        cloneAssetInfo.burstCloneAssetList.push_back(burstCloneAssetInfo);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    return E_OK;
}

static void AddEditDataSize(const CloneAssetInfo &info, bool isShareAlbumTarget, uint64_t &actualTotalSize)
{
    struct stat editStatInfo {};
    if (!isShareAlbumTarget) {
        std::string editDataPath = MediaEditUtils::GetEditDataPath(info.filePath);
        if (stat(editDataPath.c_str(), &editStatInfo) == E_OK) {
            actualTotalSize += static_cast<uint64_t>(editStatInfo.st_size);
        }
    } else {
        bool isMovingPhoto = MovingPhotoFileUtils::IsMovingPhoto(
            info.photoSubType, info.movingPhotoEffectMode, 0);
        if (isMovingPhoto) {
            std::string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(info.filePath);
            if (stat(extraDataPath.c_str(), &editStatInfo) == E_OK) {
                actualTotalSize += static_cast<uint64_t>(editStatInfo.st_size);
            }
        }
    }
}

int32_t CloneToAlbumService::QueryAllAssetsInfo(const CloneToAlbumReqBody &reqBody,
    CloneTaskInfo &assets, uint64_t &displayTotalSize, uint64_t &actualTotalSize, bool isShareAlbumTarget)
{
    std::unordered_set<std::string> occupiedPaths;
    for (const auto &id : reqBody.assetsArray) {
        auto fileId = std::to_string(MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(id));
        CloneAssetInfo info;
        struct stat thumStatInfo {};
        int32_t ret = QueryAssetInfo(fileId, info);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("QueryAssetInfo failed, id=%{public}s", id.c_str());
            return ret;
        }
        info.mode = reqBody.mode;
        info.albumLpath = reqBody.albumLpath;
        info.albumId = reqBody.albumId;
        info.albumSubType = reqBody.albumSubType;
        info.albumType = reqBody.albumType;
        info.requestId = reqBody.requestId;
        ret = CheckFileName(info, occupiedPaths);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("check name error");
            return ret;
        }
        actualTotalSize += static_cast<uint64_t>(info.size);
        displayTotalSize += static_cast<uint64_t>(info.size);
        AddEditDataSize(info, isShareAlbumTarget, actualTotalSize);
        std::string thumbnailPath = GetThumbnailPathFromOrignalPath(info.filePath);
        if (stat(thumbnailPath.c_str(), &thumStatInfo) == E_OK) {
            actualTotalSize += static_cast<uint64_t>(thumStatInfo.st_size);
        }
        if (!info.burstKey.empty()) {
            ret = QueryBurstAssetInfo(info, displayTotalSize, actualTotalSize, isShareAlbumTarget);
            if (ret != E_OK) {
                MEDIA_ERR_LOG("QueryBurstAssetInfo failed, id=%{public}" PRId64, info.fileId);
                return ret;
            }
        }
        assets.cloneAssetInfo.push_back(info);
    }
    assets.requestId = reqBody.requestId;
    return E_OK;
}

bool CheckFreeSpace(int32_t needFreeSize)
{
    int64_t freeSize = MediaFileUtils::GetFreeSize();
    CHECK_AND_RETURN_RET_LOG(freeSize > 0, false, "Get free size failed, freeSize:%{public}" PRId64, freeSize);
    CHECK_AND_RETURN_RET_LOG(freeSize > needFreeSize, false,
        "Check free size failed, freeSize:%{public}" PRId64 ", "
        "needFreeSize:%{public}d", freeSize, needFreeSize);
    return true;
}

int32_t CloneToAlbumService::HandleAssetClone(const CloneAssetInfo &cloneAssetInfo,
    std::string &newFileId, std::atomic<uint64_t> &processedSize,
    std::atomic<uint32_t> &processedCount, const CloneCallbackType &cloneCallbackType)
{
    auto progressCb = [&processedSize](uint64_t copiedSize) {
        processedSize.fetch_add(copiedSize);
    };
    int32_t intCloneCallbackType = static_cast<int32_t>(cloneCallbackType);
    int32_t result = MediaLibraryAlbumFusionUtils::CloneProgressAsset(cloneAssetInfo,
        cloneAssetInfo.albumId, newFileId, progressCb, intCloneCallbackType);
    if (result != E_OK) {
        MEDIA_INFO_LOG("clone error result %{public}d", result);
        return result;
    }

    processedCount.fetch_add(1);
    if (!cloneAssetInfo.burstKey.empty() && cloneCallbackType == CloneCallbackType::PHOTOASSET) {
        int32_t ret = DoBurstAssetsClone(cloneAssetInfo, progressCb);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed toDoBurstAssetsClone.");
    }
    return E_OK;
}

int32_t CloneToAlbumService::DoBurstAssetsClone(const CloneAssetInfo &cloneAssetInfo,
    std::function<void(uint64_t)> progressCallback)
{
    for (const auto &asset : cloneAssetInfo.burstCloneAssetList) {
        std::string newFileId = "";
        int32_t result = MediaLibraryAlbumFusionUtils::CloneProgressAsset(asset,
        asset.albumId, newFileId, progressCallback, static_cast<int32_t>(CloneCallbackType::PHOTOASSET));
        if (result != E_OK) {
            MEDIA_ERR_LOG("clone error result %{public}d", result);
            if (asset.burstCoverLevel == 1) {
                return result;
            }
            return E_OK;
        }
    }
    return E_OK;
}

static int32_t GetUriFromResult(std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
    const std::vector<std::string> &resultFileId, std::vector<std::string> &resultUris,
    CloneCallbackType cloneCallbackType)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetUriFromResult failed");
        return E_SCENE_PARAM_INVALID;
    }
    std::unordered_map<std::string, std::string> idUriMap;
    while (resultSet->GoToNextRow() == E_OK) {
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string fileId = to_string(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
        string fileDisplayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string fileStorage = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        string newUri = MediaFileUri::GetPhotoUri(fileId, filePath, fileDisplayName);
        if (cloneCallbackType == CloneCallbackType::FILEPATH) {
            size_t pos = fileStorage.find(TARGET_DIR);
            if (pos != string::npos) {
                newUri = fileStorage.substr(pos + TARGET_DIR.length());
            }
        }
        idUriMap[fileId] = newUri;
    }
    for (const auto &fileId : resultFileId) {
        auto iter = idUriMap.find(fileId);
        if (iter == idUriMap.end()) {
            MEDIA_WARN_LOG("new file id not found, fileId=%{public}s", fileId.c_str());
            continue;
        }
        resultUris.push_back(iter->second);
    }
    return E_OK;
}

static void NotifyCloneResult(int32_t ret, const CloneTaskInfo &cloneTaskInfo,
    const sptr<CloneToAlbumCallbackProxy> &callback, uint64_t totalSize, uint32_t totalCount)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, cloneTaskInfo.resultFileIds);
    auto resultSet = rdbStore->Query(predicates, std::vector<std::string>{});
    std::vector<std::string> resultUris;
    int32_t result = GetUriFromResult(resultSet, cloneTaskInfo.resultFileIds, resultUris,
        cloneTaskInfo.cloneCallbackType);
    if (callback != nullptr && cloneTaskInfo.cloneCallbackType == CloneCallbackType::PHOTOASSET) {
        auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
        auto dataShareresult = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
        callback->OnProgress(cloneTaskInfo.processedSize.load(), totalSize,
            cloneTaskInfo.processedCount.load(), totalCount);
        callback->OnComplete(ret, result == E_OK ? resultUris : std::vector<std::string>(), dataShareresult);
    } else if (callback != nullptr) {
        callback->OnProgress(cloneTaskInfo.processedSize.load(), totalSize,
            cloneTaskInfo.processedCount.load(), totalCount);
        std::shared_ptr<DataShare::DataShareResultSet> nullResultSet = nullptr;
        callback->OnComplete(ret, result == E_OK ? resultUris : std::vector<std::string>(), nullResultSet);
    }
}

int32_t CloneToAlbumService::StartCopy(uint64_t totalSize, uint32_t totalCount, CloneTaskInfo &cloneTaskInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneStartCopy");
    int32_t ret = E_INNER_FAIL;
    auto callback = iface_cast<CloneToAlbumCallbackProxy>(cloneTaskInfo.progressCallback);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ret, "Failed to callback");
    Utils::Timer::TimerCallback timerCallback = [callback,
        &cloneTaskInfo, totalSize, totalCount]() {
        uint64_t size = cloneTaskInfo.processedSize.load();
        uint32_t count = cloneTaskInfo.processedCount.load();
        if (callback) {
            callback->OnProgress(size, totalSize, count, totalCount);
        }
    };
    Utils::Timer timer{"copyWorker"};
    timer.Setup();
    uint32_t timerId = timer.Register(timerCallback, COPY_WORKER_TIME_INTERVAL, false);

    for (const auto &asset : cloneTaskInfo.cloneAssetInfo) {
        if (cloneTaskInfo.isShareAlbumTarget) {
            int64_t freeSize = MediaFileUtils::GetFreeSize();
            if (freeSize < MIN_FREE_SPACE_FOR_SHARE_CLONE) {
                MEDIA_ERR_LOG("Insufficient free space during copy, freeSize=%{public}" PRId64, freeSize);
                ret = E_SCENE_NO_ENOUGH_SPACE;
                break;
            }
        }

        std::string newFileId = "";
        ret = HandleAssetClone(asset, newFileId, cloneTaskInfo.processedSize,
            cloneTaskInfo.processedCount, cloneTaskInfo.cloneCallbackType);
        CHECK_AND_BREAK(ret == E_OK);
        cloneTaskInfo.resultFileIds.push_back(newFileId);
    }
    timer.Unregister(timerId);

    NotifyCloneResult(ret, cloneTaskInfo, callback, totalSize, totalCount);
    timer.Shutdown();
    return E_OK;
}

int32_t CheckSharedAssetsNotSupported(const std::vector<std::string> &assetsArray)
{
    std::vector<std::string> fileIds;
    for (const auto &uri : assetsArray) {
        int32_t fileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(uri);
        if (fileId >= 0) {
            fileIds.push_back(std::to_string(fileId));
        }
    }
    if (fileIds.empty()) {
        return E_OK;
    }
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.In(MediaColumn::MEDIA_ID, fileIds);
    std::vector<std::string> columns = { PhotoColumn::PHOTO_IS_SHARED };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, columns);
    if (resultSet == nullptr) {
        return E_OK;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t isShared = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_IS_SHARED,
            resultSet, TYPE_INT32));
        if (isShared == SHARED_ASSET_FLAG) {
            MEDIA_ERR_LOG("CloneToAlbum does not support shared album asset");
            return E_OPERATION_NOT_SUPPORT;
        }
    }
    resultSet->Close();
    return E_OK;
}

int32_t CloneToAlbumService::CloneToAlbum(CloneToAlbumReqBody &reqBody)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneToAlbum");
    MEDIA_INFO_LOG("CloneToAlbum start, assets=%{public}zu, albumId=%{public}d, requestId=%{public}d",
        reqBody.assetsArray.size(), reqBody.albumId,  reqBody.requestId);
    int32_t ret = ValidateRequest(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "validate request failed.");
        MEDIA_INFO_LOG("ValidateRequest error");
        return E_SCENE_PARAM_INVALID;
    }

    int32_t sharedRet = CheckSharedAssetsNotSupported(reqBody.assetsArray);
    if (sharedRet != E_OK) {
        return sharedRet;
    }

    CloneTaskInfo cloneTaskInfo;
    uint64_t totalSize = 0;
    uint64_t actualTotalSize = 0;
    ret = QueryAllAssetsInfo(reqBody, cloneTaskInfo, totalSize, actualTotalSize);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_SCENE_HAS_RENAMED, E_SCENE_HAS_RENAMED, "not support rename");
        return E_SCENE_PARAM_INVALID;
    }
    if (!CheckFreeSpace(actualTotalSize)) {
        MEDIA_ERR_LOG("CheckFreeSpace FAIL");
        return E_SCENE_PARAM_INVALID;
    }

    uint32_t totalCount = static_cast<uint32_t>(cloneTaskInfo.cloneAssetInfo.size());
    if (cloneTaskInfo.cloneAssetInfo.size() != reqBody.assetsArray.size()) {
        MEDIA_ERR_LOG("size error");
        return E_SCENE_PARAM_INVALID;
    }
    cloneTaskInfo.progressCallback = reqBody.progressCallback;
    cloneTaskInfo.cloneCallbackType = CloneCallbackType::PHOTOASSET;
    // 创建线程
    std::thread([this, totalSize, totalCount, taskInfo = cloneTaskInfo ]() {
        CloneTaskInfo cloneTaskInfotmp = taskInfo;
        this->StartCopy(totalSize, totalCount, cloneTaskInfotmp);
    }).detach();
    return E_OK;
}

shared_ptr<NativeRdb::ResultSet> QueryGetAlbumByLPath(const string &lpath)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lpath);
    return rdbStore->Query(predicates, {});
}

int32_t InsertAlbumByLPath(const string &lpath)
{
    FileAlbumInfo insertAlbumInfo;
    insertAlbumInfo.lpath = lpath;
    string albumName = "";
    size_t lastSlashPos = lpath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        insertAlbumInfo.albumName = lpath.substr(lastSlashPos + 1);
    }
    if (lpath == CLONE_FILE_ROOT_LPATH) {
        insertAlbumInfo.albumName = CLONE_FILE_ROOT_ALBUM;
    }
    int32_t albumId = FileManagementUtils::InsertFileAlbum(insertAlbumInfo);
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_ERR, "InsertFileAlbum failed.");
    return albumId;
}

int32_t GetAlbumByLPath(CloneToAlbumReqBody &reqBody)
{
    auto resultSet = QueryGetAlbumByLPath(reqBody.albumLpath);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INNER_FAIL, "resultSet is nullptr.");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        reqBody.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        reqBody.albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        reqBody.albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
        resultSet->Close();
        return E_OK;
    }
    resultSet->Close();
    auto ret = InsertAlbumByLPath(reqBody.albumLpath);
    if (ret <= 0) {
        MEDIA_ERR_LOG("album add err");
        return E_ERR;
    }
    reqBody.albumId = ret;
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER);
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SOURCE);
    return E_OK;
}

int32_t ValidateRequestForDir(CloneToAlbumReqBody &reqBody)
{
    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("dir assetsArray is empty");
        return E_ERR;
    }
    if (reqBody.targetDir.empty()) {
        MEDIA_ERR_LOG("dir targetDir is empty");
        return E_ERR;
    }
    if (reqBody.targetDir.find(RELATIVE_PATH) != std::string::npos) {
        MEDIA_ERR_LOG("dir targetDir contains invalid relative path");
        return E_ERR;
    }
    if (!reqBody.progressCallback) {
        MEDIA_ERR_LOG("callback is invalid");
        return E_ERR;
    }
    reqBody.targetDir = TARGET_DIR + reqBody.targetDir;
    if (!MediaFileUtils::IsDirectory(reqBody.targetDir)) {
        MEDIA_ERR_LOG("targetDir is not directory");
        return E_SCENE_ALBUM_NOT_EXIST;
    }
    if (!PhotoFileUtils::CheckFileManagerRealPath(reqBody.targetDir)) {
        MEDIA_ERR_LOG("targetDir is not file manager %{public}s", reqBody.targetDir.c_str());
        return E_ERR;
    }
    if (reqBody.targetDir.substr(0, DOCS_DIR.length()) == DOCS_DIR) {
        reqBody.albumLpath = DOCS_LPATH + reqBody.targetDir.substr(DOCS_DIR.length());
        MEDIA_INFO_LOG("albumLpath is %{public}s", reqBody.albumLpath.c_str());
    }
    int32_t ret = GetAlbumByLPath(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "GetAlbumByLPath inner fail");
        MEDIA_ERR_LOG("LPath is not file manager");
        return E_ERR;
    }
    if (reqBody.albumSubType != static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER)) {
        MEDIA_ERR_LOG("SubType is not file manager");
        return E_ERR;
    }
    return E_OK;
}

int32_t CloneToAlbumService::CloneToDir(CloneToAlbumReqBody &reqBody)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneToDir");
    MEDIA_INFO_LOG("CloneToDir start, assets=%{public}zu",
        reqBody.assetsArray.size());
    int32_t ret = ValidateRequestForDir(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "ValidateRequestForDir inner fail");
        MEDIA_ERR_LOG("ValidateRequestForDir error");
        return E_SCENE_PARAM_INVALID;
    }

    CloneTaskInfo cloneTaskInfo;
    uint64_t totalSize = 0;
    uint64_t actualTotalSize = 0;
    ret = QueryAllAssetsInfo(reqBody, cloneTaskInfo, totalSize, actualTotalSize);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_SCENE_HAS_RENAMED, E_SCENE_HAS_RENAMED, "not support rename");
        return E_SCENE_PARAM_INVALID;
    }

    if (!CheckFreeSpace(actualTotalSize)) {
        MEDIA_ERR_LOG("CheckFreeSpace FAIL");
        return E_SCENE_PARAM_INVALID;
    }

    uint32_t totalCount = static_cast<uint32_t>(cloneTaskInfo.cloneAssetInfo.size());
    if (cloneTaskInfo.cloneAssetInfo.size() != reqBody.assetsArray.size()) {
        MEDIA_INFO_LOG("size error");
        return E_SCENE_PARAM_INVALID;
    }
    cloneTaskInfo.progressCallback = reqBody.progressCallback;
    cloneTaskInfo.cloneCallbackType = CloneCallbackType::FILEPATH;
    // 创建线程
    std::thread([this, totalSize, totalCount, taskInfo = cloneTaskInfo ]() {
        CloneTaskInfo cloneTaskInfotmp = taskInfo;
        this->StartCopy(totalSize, totalCount, cloneTaskInfotmp);
    }).detach();

    return E_OK;
}

shared_ptr<NativeRdb::ResultSet> QueryAssetByStoragePaths(const std::vector<string> &paths)
{
    CHECK_AND_RETURN_RET_LOG(!paths.empty(), nullptr, "Empty paths.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_STORAGE_PATH, paths);
    return rdbStore->Query(predicates, {});
}

static int32_t BuildPathUriMap(shared_ptr<NativeRdb::ResultSet> &resultSet,
    std::unordered_map<std::string, std::string> &pathUriMap)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Empty resultSet.");
    while (resultSet->GoToNextRow() == E_OK) {
        string storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        pathUriMap[storagePath] = MediaFileUri::GetPhotoUri(to_string(fileId), filePath, displayName);
    }
    return E_OK;
}

static int32_t ScanFileForPath(const std::vector<std::string> &filePaths)
{
    CHECK_AND_RETURN_RET_LOG(!filePaths.empty(), E_ERR, "Empty filePaths.");
    std::vector<MediaNotifyInfo> input;
    for (const auto &filePath : filePaths) {
        MediaNotifyInfo info {
            .beforePath = filePath,
            .afterPath  = filePath,
            .objType    = FileNotifyObjectType::FILE,
            .optType    = FileNotifyOperationType::MOD
        };
        input.push_back(info);
    }
    FileManagerScanner scanner;
    auto ret = scanner.Run(input);
    if (ret != E_SUCCESS) {
        MEDIA_ERR_LOG("scanner failed");
        return E_INNER_FAIL;
    }
    return E_OK;
}

static void CollectMissingPaths(const std::vector<std::string> &paths,
    const std::unordered_map<std::string, std::string> &pathUriMap, std::vector<std::string> &missingPaths)
{
    for (const auto &assetPath : paths) {
        if (pathUriMap.find(assetPath) == pathUriMap.end()) {
            missingPaths.push_back(assetPath);
        }
    }
}

static int32_t ConvertPathsToUris(const std::vector<std::string> &paths,
    const std::unordered_map<std::string, std::string> &pathUriMap, std::vector<std::string> &assetsUri)
{
    for (const auto &assetPath : paths) {
        auto iter = pathUriMap.find(assetPath);
        if (iter == pathUriMap.end()) {
            MEDIA_ERR_LOG("assetPath is error, %{public}s", assetPath.c_str());
            return E_ERR;
        }
        assetsUri.push_back(iter->second);
    }
    return E_OK;
}

static int32_t ConvertAssetPathsToUris(std::vector<std::string> &assetsUri, const std::vector<std::string> &assetsArray)
{
    std::vector<string> assetsPath;
    assetsPath.reserve(assetsArray.size());
    for (const auto &filePath : assetsArray) {
        string assetPath = TARGET_DIR + filePath;
        if (!PhotoFileUtils::CheckFileManagerRealPath(assetPath)) {
            MEDIA_ERR_LOG("targetDir is not file manager");
            return E_ERR;
        }
        if (!MediaFileUtils::IsFileExists(assetPath)) {
            MEDIA_ERR_LOG("assetPath is not exists %{public}s", assetPath.c_str());
            return E_ERR;
        }
        assetsPath.push_back(assetPath);
    }
    auto resultSet = QueryAssetByStoragePaths(assetsPath);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Failed to query asset by storage paths.");
    std::unordered_map<std::string, std::string> pathUriMap;
    pathUriMap.reserve(assetsPath.size());
    int32_t ret = BuildPathUriMap(resultSet, pathUriMap);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to build path uri map.");
    resultSet->Close();

    std::vector<string> scanFilePaths;
    scanFilePaths.reserve(assetsPath.size());
    CollectMissingPaths(assetsPath, pathUriMap, scanFilePaths);
    if (!scanFilePaths.empty()) {
        ret = ScanFileForPath(scanFilePaths);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to scan file for path.");
        resultSet = QueryAssetByStoragePaths(scanFilePaths);
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "Failed to query asset by storage paths.");
        ret = BuildPathUriMap(resultSet, pathUriMap);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to build path uri map.");
        resultSet->Close();
    }

    assetsUri.reserve(assetsPath.size());
    // Rebuild URIs strictly in the original request order carried by assetsArray
    ret = ConvertPathsToUris(assetsPath, pathUriMap, assetsUri);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to convert paths to uris.");
    return E_OK;
}

int32_t ValidateRequestByPath(CloneToAlbumReqBody &reqBody)
{
    if (reqBody.albumId <= 0) {
        MEDIA_ERR_LOG("albumId is invalid");
        return E_ERR;
    }

    if (!reqBody.progressCallback) {
        MEDIA_ERR_LOG("callback is invalid");
        return E_ERR;
    }

    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("path assetsArray is empty");
        return E_ERR;
    }
    std::vector<std::string> assetsUri;
    int32_t ret = ConvertAssetPathsToUris(assetsUri, reqBody.assetsArray);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to get uri by path.");
    reqBody.assetsArray = assetsUri;

    auto resultSet = QueryGetAlbumByAlbumId(reqBody.albumId);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INNER_FAIL, "resultSet is nullptr");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        reqBody.albumLpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
        reqBody.albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        reqBody.albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
        resultSet->Close();
        return E_OK;
    }
    resultSet->Close();
    MEDIA_ERR_LOG("query album info failed");
    return E_ERR;
}

int32_t CloneToAlbumService::CloneAssetByPath(CloneToAlbumReqBody &reqBody)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneAssetByPath");
    MEDIA_INFO_LOG("CloneAssetByPath start");
    int32_t ret = ValidateRequestByPath(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "ValidateRequestByPath inner error");
        MEDIA_ERR_LOG("ValidateRequestByPath error");
        return E_SCENE_PARAM_INVALID;
    }

    CloneTaskInfo cloneTaskInfo;
    uint64_t totalSize = 0;
    uint64_t actualTotalSize = 0;
    ret = QueryAllAssetsInfo(reqBody, cloneTaskInfo, totalSize, actualTotalSize);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_SCENE_HAS_RENAMED, E_SCENE_HAS_RENAMED, "not support rename");
        MEDIA_ERR_LOG("QueryAllAssetsInfo error");
        return E_SCENE_PARAM_INVALID;
    }
    if (!CheckFreeSpace(actualTotalSize)) {
        MEDIA_ERR_LOG("CheckFreeSpace FAIL");
        return E_SCENE_NO_ENOUGH_SPACE;
    }

    uint32_t totalCount = static_cast<uint32_t>(cloneTaskInfo.cloneAssetInfo.size());
    if (cloneTaskInfo.cloneAssetInfo.size() != reqBody.assetsArray.size()) {
        MEDIA_INFO_LOG("size error");
        return E_SCENE_PARAM_INVALID;
    }
    cloneTaskInfo.progressCallback = reqBody.progressCallback;
    cloneTaskInfo.cloneCallbackType = CloneCallbackType::URI;
    // 创建线程
    std::thread([this, totalSize, totalCount, taskInfo = cloneTaskInfo ]() {
        CloneTaskInfo cloneTaskInfotmp = taskInfo;
        this->StartCopy(totalSize, totalCount, cloneTaskInfotmp);
    }).detach();

    return E_OK;
}

int32_t CloneToAlbumService::QueryAssetInfo(const std::string &fileId, CloneAssetInfo &info)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    std::vector<std::string> columns = {
        PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::MEDIA_NAME, PhotoColumn::MEDIA_TYPE,
        PhotoColumn::MEDIA_SIZE, MediaColumn::MEDIA_HIDDEN, MediaColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_SOURCE_PATH,
        PhotoColumn::PHOTO_BURST_KEY, PhotoColumn::PHOTO_IS_SHARED, PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE
    };

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Asset not found, fileId=%{public}s", fileId.c_str());
        return E_ERR;
    }

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(fileId), E_ERR, "fileId is invailed");
    info.fileId = std::stoi(fileId);
    info.filePath = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    info.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    info.mediaType = GetInt32Val(PhotoColumn::MEDIA_TYPE, resultSet);
    info.size = GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSet);
    info.hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    info.dateTrashed = GetInt64Val(PhotoColumn::MEDIA_DATE_TRASHED, resultSet);
    info.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    info.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    info.sourcePath = GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
    info.burstKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
    info.photoSubType = GetInt64Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    info.movingPhotoEffectMode = GetInt64Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    info.isShared = GetInt32Val(PhotoColumn::PHOTO_IS_SHARED, resultSet);
    resultSet->Close();
    return E_OK;
}

int32_t CloneToAlbumService::CloneToAlbumCancel(const CloneToAlbumReqBody &reqBody)
{
    MEDIA_INFO_LOG("CloneToAlbumCancel start, requestId=%{public}d",
        reqBody.requestId);
    MediaFileUtils::CloneToAlbumCancel(std::to_string(reqBody.requestId));
    return E_OK;
}

int32_t CloneToAlbumService::ValidateShareAlbumBasicParam(const CloneToAlbumReqBody &reqBody)
{
    if (reqBody.assetsArray.empty()) {
        MEDIA_ERR_LOG("assetsArray is empty");
        return E_ERR;
    }
    if (reqBody.albumId <= 0) {
        MEDIA_ERR_LOG("albumId is invalid");
        return E_ERR;
    }
    if (!reqBody.progressCallback) {
        MEDIA_ERR_LOG("callback is invalid");
        return E_ERR;
    }
    if (reqBody.owner.empty()) {
        MEDIA_ERR_LOG("owner is empty");
        return E_ERR;
    }
    return E_OK;
}

int32_t CloneToAlbumService::CheckAssetCloudAndShared(const std::string &id, bool &hasSharedAsset)
{
    auto fileId = std::to_string(MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(id));
    CloneAssetInfo info;
    int32_t ret = QueryAssetInfo(fileId, info);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryAssetInfo failed for asset, id=%{public}s", id.c_str());
        return E_SCENE_PARAM_INVALID;
    }
    if (info.position == static_cast<int32_t>(PhotoPositionType::CLOUD)) {
        MEDIA_ERR_LOG("Asset is pure cloud, cannot clone");
        return E_SCENE_PARAM_INVALID;
    }
    if (info.isShared == 1) {
        hasSharedAsset = true;
    }
    return E_OK;
}

static bool IsShareAlbumMember(const std::string &owner, int32_t albumId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore");
        return false;
    }
    NativeRdb::RdbPredicates predicates("tab_share_album_member");
    predicates.EqualTo("album_id", albumId);
    predicates.EqualTo("share_member", owner);
    auto resultSet = rdbStore->Query(predicates, std::vector<std::string>{"COUNT(*)"});
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query share album member failed");
        return false;
    }
    int32_t count = 0;
    resultSet->GetInt(0, count);
    resultSet->Close();
    return count > 0;
}

int32_t CloneToAlbumService::ValidateShareAlbumAssets(const CloneToAlbumReqBody &reqBody,
    bool isTargetShareAlbum)
{
    if (isTargetShareAlbum) {
        if (reqBody.owner != reqBody.shareAlbumOwner &&
            !IsShareAlbumMember(reqBody.owner, reqBody.albumId)) {
            MEDIA_ERR_LOG("Owner is not the album manager or member");
            return E_SCENE_PARAM_INVALID;
        }
        for (const auto &id : reqBody.assetsArray) {
            bool unused = false;
            int32_t ret = CheckAssetCloudAndShared(id, unused);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CheckAssetCloudAndShared failed");
        }
    } else {
        for (const auto &id : reqBody.assetsArray) {
            bool isSharedAsset = false;
            int32_t ret = CheckAssetCloudAndShared(id, isSharedAsset);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CheckAssetCloudAndShared failed");
            if (!isSharedAsset) {
                MEDIA_ERR_LOG("Cannot clone non-share asset to non-share album, asset=%{public}s", id.c_str());
                return E_SCENE_PARAM_INVALID;
            }
        }
    }
    return E_OK;
}

int32_t CloneToAlbumService::ValidateShareAlbumRequest(CloneToAlbumReqBody &reqBody)
{
    int32_t ret = ValidateShareAlbumBasicParam(reqBody);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "ValidateShareAlbumBasicParam failed");

    auto resultSet = QueryGetAlbumByAlbumId(reqBody.albumId);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INNER_FAIL, "resultSet is nullptr");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_ERR_LOG("query album info failed");
        return E_SCENE_PARAM_INVALID;
    }
    reqBody.albumLpath = GetStringVal(PhotoAlbumColumns::ALBUM_LPATH, resultSet);
    reqBody.albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    reqBody.albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    reqBody.shareAlbumOwner = GetStringVal(PhotoAlbumColumns::ALBUM_SHARE_ALBUM_OWNER, resultSet);
    resultSet->Close();

    bool isTargetShareAlbum = PhotoAlbum::IsShareAlbum(
        static_cast<PhotoAlbumType>(reqBody.albumType),
        static_cast<PhotoAlbumSubType>(reqBody.albumSubType));
    return ValidateShareAlbumAssets(reqBody, isTargetShareAlbum);
}

void CloneToAlbumService::FillShareAlbumFields(CloneTaskInfo &cloneTaskInfo,
    const CloneToAlbumReqBody &reqBody)
{
    auto now = std::chrono::system_clock::now();
    int64_t nowMs = MediaFileUtils::UTCTimeMilliSeconds();
    std::string dateDay = MediaFileUtils::StrCreateTimeByMilliseconds(
        PhotoColumn::PHOTO_DATE_DAY_FORMAT, nowMs);
    int64_t dateDayValue = 0;
    std::from_chars(dateDay.data(), dateDay.data() + dateDay.size(), dateDayValue);
    for (auto &asset : cloneTaskInfo.cloneAssetInfo) {
        asset.isShared = 1;
        asset.shareOwnerInfo = reqBody.owner;
        asset.shareAlbumOwner = reqBody.shareAlbumOwner;
        asset.shareDateDay = dateDayValue;
        asset.shareGroup = reqBody.shareGroup;
        asset.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_SHARE_ALBUM);
        asset.isShareAlbumTarget = true;
        for (auto &burstAsset : asset.burstCloneAssetList) {
            burstAsset.isShared = 1;
            burstAsset.shareOwnerInfo = reqBody.owner;
            burstAsset.shareAlbumOwner = reqBody.shareAlbumOwner;
            burstAsset.shareDateDay = dateDayValue;
            burstAsset.shareGroup = reqBody.shareGroup;
            burstAsset.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_SHARE_ALBUM);
            burstAsset.isShareAlbumTarget = true;
        }
    }
}

void CloneToAlbumService::SetupCloneTaskInfo(CloneTaskInfo &cloneTaskInfo,
    const CloneToAlbumReqBody &reqBody, bool isTargetShareAlbum)
{
    if (isTargetShareAlbum) {
        FillShareAlbumFields(cloneTaskInfo, reqBody);
    }
    cloneTaskInfo.progressCallback = reqBody.progressCallback;
    cloneTaskInfo.cloneCallbackType = CloneCallbackType::PHOTOASSET;
    cloneTaskInfo.isShareAlbumTarget = isTargetShareAlbum;
    cloneTaskInfo.owner = reqBody.owner;
    cloneTaskInfo.shareGroup = reqBody.shareGroup;
    cloneTaskInfo.shareAlbumOwner = reqBody.shareAlbumOwner;
}

int32_t CloneToAlbumService::CloneWithShareAlbum(CloneToAlbumReqBody &reqBody)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneWithShareAlbum");
    MEDIA_INFO_LOG("CloneWithShareAlbum start, assets=%{public}zu, albumId=%{public}d",
        reqBody.assetsArray.size(), reqBody.albumId);

    int32_t ret = ValidateShareAlbumRequest(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "validate share album request failed.");
        return E_SCENE_PARAM_INVALID;
    }

    bool isTargetShareAlbum = PhotoAlbum::IsShareAlbum(
        static_cast<PhotoAlbumType>(reqBody.albumType),
        static_cast<PhotoAlbumSubType>(reqBody.albumSubType));

    CloneTaskInfo cloneTaskInfo;
    uint64_t totalSize = 0;
    uint64_t actualTotalSize = 0;
    ret = QueryAllAssetsInfo(reqBody, cloneTaskInfo, totalSize, actualTotalSize, isTargetShareAlbum);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK || ret == E_SCENE_HAS_RENAMED, ret, "QueryAllAssetsInfo failed");
    if (ret == E_SCENE_HAS_RENAMED) {
        return E_SCENE_HAS_RENAMED;
    }

    int64_t freeSize = MediaFileUtils::GetFreeSize();
    if (freeSize < MIN_FREE_SPACE_FOR_SHARE_CLONE) {
        MEDIA_ERR_LOG("Insufficient free space, freeSize=%{public}" PRId64, freeSize);
        return E_SCENE_NO_ENOUGH_SPACE;
    }
    if (cloneTaskInfo.cloneAssetInfo.size() != reqBody.assetsArray.size()) {
        MEDIA_ERR_LOG("size error");
        return E_SCENE_PARAM_INVALID;
    }

    SetupCloneTaskInfo(cloneTaskInfo, reqBody, isTargetShareAlbum);

    uint32_t totalCount = static_cast<uint32_t>(cloneTaskInfo.cloneAssetInfo.size());
    std::thread([this, totalSize, totalCount, taskInfo = cloneTaskInfo]() {
        CloneTaskInfo cloneTaskInfotmp = taskInfo;
        this->StartCopy(totalSize, totalCount, cloneTaskInfotmp);
    }).detach();
    return E_OK;
}

} // namespace Media
} // namespace OHOS
