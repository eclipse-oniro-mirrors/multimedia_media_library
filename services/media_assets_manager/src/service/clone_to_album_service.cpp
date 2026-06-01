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
#include <thread>

#include "clone_to_album_callback_proxy.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
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

namespace OHOS {
namespace Media {

constexpr size_t COPY_WORKER_TIME_INTERVAL = 1000;
constexpr int32_t E_CANCELLED = -10086;
constexpr int32_t NOT_SUPPORT_RENAME = 1;
const std::string TARGET_DIR = "/storage/media/local/files";
const std::string DOCS_DIR = "/storage/media/local/files/Docs";
const std::string DOCS_LPATH = "/FromDocs";
constexpr int32_t DOCS_LPATH_LENGTH = 9;
const std::string CLONE_FILE_ROOT_LPATH = "/FromDocs/";
const std::string CLONE_FILE_ROOT_ALBUM = "根目录";


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
    if (pos != string::npos) {
        thumbnailPath = srcPath.replace(pos, photoRelativePath.length(), thumbRelativePath);
    }
    return thumbnailPath;
}

int32_t CheckFileName(CloneAssetInfo &cloneAssetInfo)
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
        if (cloneAssetInfo.burstKey.empty()) {
            MediaFileAccessUtils::HandleSameNameRename(targetPath, renamePath, renameTitle, renameDisplayName);
        } else {
            MediaFileAccessUtils::HandleBurstSameNameRename(targetPath, renamePath, renameTitle, renameDisplayName);
        }
        if (targetPath != renamePath && cloneAssetInfo.mode == NOT_SUPPORT_RENAME) {
            MEDIA_ERR_LOG("HandleSameName error");
            return E_SCENE_HAS_RENAMED;
        } else {
            cloneAssetInfo.targetFilePath = renamePath;
            cloneAssetInfo.targetFileTitle = renameTitle;
            cloneAssetInfo.targetDisplayName = renameDisplayName;
        }
    } else {
        cloneAssetInfo.targetDisplayName = cloneAssetInfo.displayName;
    }
    return E_OK;
}

int32_t CloneToAlbumService::QueryBurstAssetInfo(CloneAssetInfo &cloneAssetInfo, uint64_t &displayTotalSize,
    uint64_t &actualTotalSize)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.NotEqualTo(PhotoColumn::MEDIA_ID, cloneAssetInfo.fileId);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, cloneAssetInfo.burstKey);
    std::vector<std::string> columns = {
    PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_NAME, PhotoColumn::MEDIA_TYPE,
    PhotoColumn::MEDIA_SIZE, MediaColumn::MEDIA_HIDDEN, MediaColumn::MEDIA_DATE_TRASHED,
    PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_STORAGE_PATH, PhotoColumn::PHOTO_SOURCE_PATH,
    PhotoColumn::PHOTO_BURST_KEY, PhotoColumn::PHOTO_OWNER_ALBUM_ID, PhotoColumn::PHOTO_FILE_SOURCE_TYPE
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
        actualTotalSize += static_cast<uint64_t>(burstCloneAssetInfo.size);
        std::string editDataPath = MediaEditUtils::GetEditDataPath(burstCloneAssetInfo.filePath);
        if (stat(editDataPath.c_str(), &editStatInfo) == E_OK) {
            actualTotalSize += editStatInfo.st_size;
        }
        std::string thumbnailPath = GetThumbnailPathFromOrignalPath(burstCloneAssetInfo.filePath);
        if (stat(thumbnailPath.c_str(), &thumStatInfo) == E_OK) {
            actualTotalSize += thumStatInfo.st_size;
        }
        displayTotalSize += static_cast<uint64_t>(burstCloneAssetInfo.size);
        burstCloneAssetInfo.albumId = cloneAssetInfo.albumId;
        burstCloneAssetInfo.targetDisplayName = burstCloneAssetInfo.displayName;
        cloneAssetInfo.burstCloneAssetList.push_back(burstCloneAssetInfo);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    return E_OK;
}

int32_t CloneToAlbumService::QueryAllAssetsInfo(const CloneToAlbumReqBody &reqBody,
    CloneTaskInfo &assets, uint64_t &displayTotalSize, uint64_t &actualTotalSize)
{
    for (const auto &id : reqBody.assetsArray) {
        auto fileId = std::to_string(MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(id));
        CloneAssetInfo info;
        struct stat editStatInfo {};
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
        ret = CheckFileName(info);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("check name error");
            return ret;
        }
        actualTotalSize += static_cast<uint64_t>(info.size);
        displayTotalSize += static_cast<uint64_t>(info.size);
        std::string editDataPath = MediaEditUtils::GetEditDataPath(info.filePath);
        if (stat(editDataPath.c_str(), &editStatInfo) == E_OK) {
            actualTotalSize += editStatInfo.st_size;
        }
        std::string thumbnailPath = GetThumbnailPathFromOrignalPath(info.filePath);
        if (stat(thumbnailPath.c_str(), &thumStatInfo) == E_OK) {
            actualTotalSize += thumStatInfo.st_size;
        }
        if (!info.burstKey.empty()) {
            ret = QueryBurstAssetInfo(info, displayTotalSize, actualTotalSize);
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
    std::atomic<uint32_t> &processedCount, CloneCallbackType &cloneCallbackType)
{
    auto progressCb = [&processedSize](uint64_t copiedSize) {
        processedSize.fetch_add(copiedSize);
    };
    int32_t result = MediaLibraryAlbumFusionUtils::CloneProgressAsset(cloneAssetInfo,
        cloneAssetInfo.albumId, newFileId, progressCb);
    if (result != E_OK) {
        MEDIA_INFO_LOG("clone error result %{public}d", result);
        return result;
    }

    processedCount.fetch_add(1);
    //如果是连拍
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
        asset.albumId, newFileId, progressCallback);
        if (result != E_OK) {
            MEDIA_ERR_LOG("clone error result %{public}d", result);
            return result;
        }
    }
    return E_OK;
}

int32_t CloneToAlbumService::GetUriFromResult(std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
    std::vector<std::string> &resultUris, CloneCallbackType cloneCallbackType)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetUriFromResult failed");
        return E_SCENE_PARAM_INVALID;
    }
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
        resultUris.push_back(newUri);
    }
    return E_OK;
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

    std::vector<std::string> resultFileId;
    for (const auto &asset : cloneTaskInfo.cloneAssetInfo) {
        std::string newFileId = "";
        ret = HandleAssetClone(asset, newFileId, cloneTaskInfo.processedSize,
            cloneTaskInfo.processedCount, cloneTaskInfo.cloneCallbackType);
        CHECK_AND_BREAK(ret == E_OK);
        resultFileId.push_back(newFileId);
    }
    timer.Unregister(timerId);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, resultFileId);

    std::vector<std::string> columns = {};
    auto resultSet = rdbStore->Query(predicates, columns);
    std::vector<std::string> resultUris;
    int32_t result = GetUriFromResult(resultSet, resultUris, cloneTaskInfo.cloneCallbackType);
    if (callback != nullptr && cloneTaskInfo.cloneCallbackType == CloneCallbackType::PHOTOASSET) {
        auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
        auto dataShareresult = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
        callback->OnProgress(cloneTaskInfo.processedSize.load(), totalSize,
            cloneTaskInfo.processedCount.load(), totalCount);
        callback->OnComplete(ret, result == E_OK ? resultUris : std::vector<std::string>(), dataShareresult);
    } else if (callback != nullptr) {
        callback->OnProgress(cloneTaskInfo.processedSize.load(), totalSize,
            cloneTaskInfo.processedCount.load(), totalCount);
        std::shared_ptr<DataShare::DataShareResultSet> resultSet = nullptr;
        callback->OnComplete(ret, result == E_OK ? resultUris : std::vector<std::string>(), resultSet);
    }
    timer.Shutdown();
    return E_OK;
}

int32_t CloneToAlbumService::CloneToAlbum(CloneToAlbumReqBody &reqBody)
{
    MEDIA_INFO_LOG("CloneToAlbum start, assets=%{public}zu, albumId=%{public}d, requestId=%{public}d",
        reqBody.assetsArray.size(), reqBody.albumId,  reqBody.requestId);
    int32_t ret = ValidateRequest(reqBody);
    if (ret != E_OK) {
        CHECK_AND_RETURN_RET_LOG(ret != E_INNER_FAIL, E_INNER_FAIL, "validate request failed.");
        MEDIA_INFO_LOG("ValidateRequest error");
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
    string albumName = "";
    size_t lastSlashPos = lpath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        albumName = lpath.substr(lastSlashPos + 1);
    }
    if (lpath == CLONE_FILE_ROOT_LPATH) {
        albumName = CLONE_FILE_ROOT_ALBUM;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int64_t rowNum = 0;
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SOURCE));
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE,
        static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER));
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, lpath);
    int32_t result = rdbStore->Insert(rowNum, PhotoAlbumColumns::TABLE, values);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Insert error! ");
        return E_ERR;
    }
    return rowNum;
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

shared_ptr<NativeRdb::ResultSet> QueryAssetByStoragePath(const string &path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), nullptr, "Empty path.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_STORAGE_PATH, path);
    return rdbStore->Query(predicates, {});
}

int32_t GetUriByPath(const string &filePath, string &fileUri)
{
    fileUri = TARGET_DIR + filePath;
    if (!PhotoFileUtils::CheckFileManagerRealPath(fileUri)) {
        MEDIA_ERR_LOG("targetDir is not file manager");
        return E_ERR;
    }
    if (!MediaFileUtils::IsFileExists(fileUri)) {
        MEDIA_ERR_LOG("fileUri is not exists %{public}s", fileUri.c_str());
        return E_ERR;
    }
    //根据资产路径查询资产对象,资产存在则直接返回
    auto resultSet = QueryAssetByStoragePath(fileUri);
    int32_t count = 0;
    if (resultSet != nullptr && resultSet->GetRowCount(count) == NativeRdb::E_OK && count > 0) {
        resultSet->GoToFirstRow();
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        fileUri = MediaFileUri::GetPhotoUri(to_string(fileId), filePath, displayName);
        return E_OK;
    }
    MediaNotifyInfo info {
        .beforePath = fileUri,
        .afterPath  = fileUri,
        .objType    = FileNotifyObjectType::FILE,
        .optType    = FileNotifyOperationType::MOD
    };
    std::vector<MediaNotifyInfo> input = {info};
    FileManagerScanner scanner;
    auto ret = scanner.Run(input);
    if (ret != E_SUCCESS) {
        MEDIA_ERR_LOG("scanner failed");
        return E_INNER_FAIL;
    }
    resultSet = QueryAssetByStoragePath(fileUri);
    if (resultSet != nullptr && resultSet->GetRowCount(count) == NativeRdb::E_OK && count > 0) {
        resultSet->GoToFirstRow();
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
            MediaFileUtils::GetExtraUri(displayName, filePath));
        fileUri = uri;
        return E_OK;
    }
    return E_ERR;
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
    for (const auto &filePath : reqBody.assetsArray) {
        string fileUri = "";
        auto ret = GetUriByPath(filePath, fileUri);
        if (ret != E_OK || fileUri.empty()) {
            MEDIA_ERR_LOG("filePath is not file manager, %{public}s", fileUri.c_str());
            return ret;
        }
        assetsUri.push_back(fileUri);
    }
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
        PhotoColumn::PHOTO_BURST_KEY,
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

} // namespace Media
} // namespace OHOS
