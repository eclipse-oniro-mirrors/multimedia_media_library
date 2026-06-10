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

#include <chrono>
#include <vector>
#include <string>
#include <thread>

#include "cloud_sync_observer.h"

#include "cloud_media_asset_manager.h"
#include "cloud_sync_notify_handler.h"
#include "media_analysis_helper.h"
#include "media_file_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "albums_refresh_manager.h"
#include "photo_album_column.h"
#include "albums_refresh_notify.h"
#include "notify_responsibility_chain_factory.h"
#include "post_event_utils.h"

using namespace std;

namespace OHOS {
namespace Media {
constexpr int32_t SYNC_INTERVAL = 10000;
static void HandleCloudNotify(AsyncTaskData *data)
{
    auto* taskData = static_cast<CloudSyncNotifyData*>(data);
    shared_ptr<CloudSyncNotifyHandler> notifyHandler = make_shared<CloudSyncNotifyHandler>(taskData->notifyInfo_);
    notifyHandler->MakeResponsibilityChain();
}

void CloudSyncObserver::DealCloudSync(const ChangeInfo &changeInfo)
{
    SyncNotifyInfo info;
    info.uris = changeInfo.uris_;
    std::string dataString = (const char *)changeInfo.data_;
    CHECK_AND_RETURN_WARN_LOG(nlohmann::json::accept(dataString),
        "Failed to verify the meataData format, metaData is: %{public}s", dataString.c_str());
    nlohmann::json jsonData = nlohmann::json::parse(dataString);
    bool isValid = jsonData.contains("taskType") && jsonData["taskType"].is_number_unsigned();
    CHECK_AND_EXECUTE(!isValid, info.taskType = jsonData["taskType"]);
    isValid = jsonData.contains("syncId") && jsonData["syncId"].is_string();
    CHECK_AND_EXECUTE(!isValid, info.syncId = jsonData["syncId"]);
    isValid = jsonData.contains("syncType") && jsonData["syncType"].is_number_unsigned();
    CHECK_AND_EXECUTE(!isValid, info.syncType = jsonData["syncType"]);
    isValid = jsonData.contains("totalAssets") && jsonData["totalAssets"].is_number_unsigned();
    CHECK_AND_EXECUTE(!isValid, info.totalAssets = jsonData["totalAssets"]);
    isValid = jsonData.contains("totalAlbums") && jsonData["totalAlbums"].is_number_unsigned();
    CHECK_AND_EXECUTE(!isValid, info.totalAlbums = jsonData["totalAlbums"]);

    if (info.taskType == TIME_BEGIN_SYNC) {
        PostEventUtils::GetInstance().CreateCloudDownloadSyncStat(info.syncId);
        VariantMap map = {
            {KEY_START_DOWNLOAD_TIME, MediaFileUtils::UTCTimeMilliSeconds()}, {KEY_DOWNLOAD_TYPE, info.syncType}};
        PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
        AlbumsRefreshManager::GetInstance().AddAlbumRefreshTask(info);
    } else if (info.taskType == TIME_END_SYNC) {
        AlbumsRefreshManager::GetInstance().AddAlbumRefreshTask(info);
    }
}
 
void CloudSyncObserver::DealAlbumGallery(CloudSyncNotifyInfo &notifyInfo)
{
    SyncNotifyInfo info = AlbumsRefreshManager::GetInstance().GetSyncNotifyInfo(notifyInfo, ALBUM_URI_TYPE);
    AlbumsRefreshManager::GetInstance().AddAlbumRefreshTask(info);
    VariantMap map;
    if (info.notifyType == NOTIFY_ADD) {
        map = {{KEY_TOTAL_ALBUM_NUM, info.urisSize}, {KEY_ADD_ALBUM_NUM, info.urisSize}};
    } else if (info.notifyType == NOTIFY_UPDATE) {
        map = {{KEY_TOTAL_ALBUM_NUM, info.urisSize}, {KEY_UPDATE_ALBUM_NUM, info.urisSize}};
    } else if (info.notifyType == NOTIFY_REMOVE) {
        map = {{KEY_TOTAL_ALBUM_NUM, info.urisSize}, {KEY_DELETE_ALBUM_NUM, info.urisSize}};
    }
    PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
}

void CloudSyncObserver::DealPhotoGallery(CloudSyncNotifyInfo &notifyInfo)
{
    if (notifyInfo.type == ChangeType::UPDATE || notifyInfo.type == ChangeType::OTHER ||
        notifyInfo.type == ChangeType::DELETE) {
        CloudSyncHandleData handleData;
        handleData.orgInfo = notifyInfo;
        shared_ptr<BaseHandler> chain = NotifyResponsibilityChainFactory::CreateChain(GALLERY_PHOTO_DELETE);
        if (chain != nullptr) {
            chain->Handle(handleData);
        } else {
            MEDIA_ERR_LOG("uri OR type is Invalid");
        }
    }
    SyncNotifyInfo info = AlbumsRefreshManager::GetInstance().GetSyncNotifyInfo(notifyInfo, PHOTO_URI_TYPE);
    AlbumsRefreshManager::GetInstance().AddAlbumRefreshTask(info);
    VariantMap map;
    if (info.notifyType == NOTIFY_ADD) {
        map = {{KEY_TOTAL_ASSET_NUM, info.urisSize}, {KEY_ADD_ASSET_NUM, info.urisSize}};
    } else if (info.notifyType == NOTIFY_UPDATE) {
        map = {{KEY_TOTAL_ASSET_NUM, info.urisSize}, {KEY_UPDATE_ASSET_NUM, info.urisSize}};
    } else if (info.notifyType == NOTIFY_REMOVE) {
        map = {{KEY_TOTAL_ASSET_NUM, info.urisSize}, {KEY_DELETE_ASSET_NUM, info.urisSize}};
    }
    PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
    if (notifyInfo.type == ChangeType::DELETE || notifyInfo.type == ChangeType::INSERT) {
        CloudMediaAssetManager::GetInstance().SetIsThumbnailUpdate();
    }
}

void CloudSyncObserver::DealGalleryDownload(CloudSyncNotifyInfo &notifyInfo)
{
    if (notifyInfo.type == ChangeType::OTHER) {
        CHECK_AND_RETURN_LOG(!notifyInfo.uris.empty(), "gallery download notify uri empty");
        string uriString = notifyInfo.uris.front().ToString();
        string downloadString = "gallery/download";
        string::size_type pos = uriString.find(downloadString);
        CHECK_AND_RETURN_LOG(pos != string::npos, "gallery download notify uri err");
        auto it = notifyInfo.uris.begin();
        *it = Uri(uriString.replace(pos, downloadString.length(), "Photo"));
        notifyInfo.type = ChangeType::UPDATE;
        CloudSyncHandleData handleData;
        handleData.orgInfo = notifyInfo;
        shared_ptr<BaseHandler> chain = NotifyResponsibilityChainFactory::CreateChain(TRANSPARENT);
        CHECK_AND_EXECUTE(chain == nullptr, chain->Handle(handleData));
    }
}

void CloudSyncObserver::OnChange(const ChangeInfo &changeInfo)
{
    CHECK_AND_RETURN_LOG(!changeInfo.uris_.empty(), "OnChange notify uri empty");
    CloudSyncNotifyInfo notifyInfo = {changeInfo.uris_, changeInfo.changeType_, changeInfo.data_};
    string uriString = notifyInfo.uris.front().ToString();
    MEDIA_DEBUG_LOG("#uriString: %{public}s, #uriSize: %{public}zu changeType: %{public}d",
        uriString.c_str(), changeInfo.uris_.size(), changeInfo.changeType_);
 
    if (uriString.find(PhotoAlbumColumns::PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX) != string::npos) {
        DealCloudSync(changeInfo);
        return;
    }

    if (uriString.find(PhotoColumn::PHOTO_CLOUD_URI_PREFIX) != string::npos && notifyInfo.type == ChangeType::OTHER) {
        lock_guard<mutex> lock(syncMutex_);
        if (!isPending_) {
            MEDIA_INFO_LOG("set timer handle index");
            std::thread([this]() { this->HandleIndex(); }).detach();
            isPending_ = true;
        }
    }

    if (uriString.find(PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX) != string::npos) {
        DealAlbumGallery(notifyInfo);
        return;
    }
    
    if (uriString.find(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX) != string::npos) {
        DealPhotoGallery(notifyInfo);
        return;
    }
    
    if (uriString.find(PhotoAlbumColumns::PHOTO_GALLERY_DOWNLOAD_URI_PREFIX) != string::npos) {
        DealGalleryDownload(notifyInfo);
        return;
    }

    auto *taskData = new (nothrow) CloudSyncNotifyData(notifyInfo);
    CHECK_AND_RETURN_LOG(taskData != nullptr, "Failed to new taskData");
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_ERR_LOG("Can not get asyncWorker");
        delete taskData;
        return;
    }
    shared_ptr<MediaLibraryAsyncTask> notifyHandleAsyncTask = make_shared<MediaLibraryAsyncTask>(
        HandleCloudNotify, taskData);
    CHECK_AND_EXECUTE(notifyHandleAsyncTask == nullptr, asyncWorker->AddTask(notifyHandleAsyncTask, true));
}

void CloudSyncObserver::HandleIndex()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SYNC_INTERVAL));
    lock_guard<mutex> lock(syncMutex_);
    std::vector<std::string> idToDeleteIndex;
    MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_DELETE_INDEX), idToDeleteIndex);

    //update index
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "uniStore is nullptr!");
    const std::string queryIdToUpdateIndex = "SELECT file_id FROM tab_analysis_search_index WHERE photo_status = 2";
    auto resultSetUpdateIndex = uniStore->QuerySql(queryIdToUpdateIndex);
    CHECK_AND_RETURN_LOG(resultSetUpdateIndex != nullptr, "resultSetUpdateIndex is nullptr!");
    std::vector<std::string> idToUpdateIndex;
    while (resultSetUpdateIndex->GoToNextRow() == NativeRdb::E_OK) {
        idToUpdateIndex.push_back(to_string(GetInt32Val("file_id", resultSetUpdateIndex)));
    }

    MEDIA_INFO_LOG("HandleIndex idToUpdateIndex size: %{public}zu", idToUpdateIndex.size());
    CHECK_AND_EXECUTE(idToUpdateIndex.empty(), MediaAnalysisHelper::AsyncStartMediaAnalysisService(
        static_cast<int32_t>(MediaAnalysisProxy::ActivateServiceType::START_UPDATE_INDEX), idToUpdateIndex));
    isPending_ = false;
}
} // namespace Media
} // namespace OHOS
