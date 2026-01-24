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

#define MLOG_TAG "AlbumsRefreshWorker"

#include "albums_refresh_worker.h"

#include "media_log.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "albums_refresh_manager.h"
#include "cloud_album_handler.h"
#include "photo_album_column.h"
#include "post_event_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"

using namespace std;

namespace OHOS {
namespace Media {
static constexpr int32_t WAIT_TIME = 30;
static constexpr int32_t WAIT_RELEASE = 50;
static constexpr int32_t CPU_9 = 9;
static constexpr int32_t MAX_ADD_FUSION_URIS = 1500;
static constexpr int32_t MAX_OTHER_FUSION_URIS = 500;

static void SetCpu(thread &t)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(CPU_9, &cpuset);
    pthread_setaffinity_np(t.native_handle(), sizeof(cpuset), &cpuset);
}

AlbumsRefreshWorker::AlbumsRefreshWorker()
{
    stop = false;
    isThreadAlive = true;
    thread taskThread(&AlbumsRefreshWorker::DealWithTasks, this);
    SetCpu(taskThread);
    taskThread.detach();
}

AlbumsRefreshWorker::~AlbumsRefreshWorker()
{
    stop = true;
    condVar_.notify_all();
    unique_lock<mutex> lock(releaseMutex_);
    releaseVar_.wait_for(lock, chrono::milliseconds(WAIT_RELEASE), [this]() { return isThreadAlive == false; });
}

void AlbumsRefreshWorker::StartConsumerThread()
{
    if (!isThreadAlive && !stop) {
        isThreadAlive = true;
        thread taskThread(&AlbumsRefreshWorker::DealWithTasks, this);
        SetCpu(taskThread);
        taskThread.detach();
    }
}

void AlbumsRefreshWorker::AddAlbumRefreshTask(SyncNotifyInfo &info)
{
    {
        lock_guard<mutex> lock(queueMutex_);
        taskQueue_.push(info);
        StartConsumerThread();
    }
    condVar_.notify_one();
}

static string extractIdByPhotoUriString(const string &input)
{
    string out = input;
    string prefix = PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX;
    size_t pos = out.find(prefix);
    if (pos == string::npos) {
        return "";
    }
    out.replace(pos, prefix.length(), "");
    // 全量排查验证cloudsync uri解析
    if (MediaFileUtils::EndsWith(out, "/meta") || MediaFileUtils::EndsWith(out, "/asset")) {
        size_t lastSlashPos = out.find_last_of('/');
        out = out.substr(0, lastSlashPos);
    }
    return out;
}

static string extractIdByAlbumUriString(const string &input)
{
    string out = input;
    string prefix = PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX;
    size_t pos = out.find(prefix);
    if (pos == string::npos) {
        return "";
    }
    out.replace(pos, prefix.length(), "");
    return out;
}

static void PrintSyncInfo(SyncNotifyInfo &info)
{
    MEDIA_DEBUG_LOG(
        "#test info.taskType: %{public}d, info.syncType: %{public}d, info.notifyType: %{public}d, info.syncId: "
        "%{public}s, info.totalAssets: %{public}d, info.totalAlbums: %{public}d, info.urisSize: %{public}d",
        info.taskType,
        info.syncType,
        info.notifyType,
        info.syncId.c_str(),
        info.totalAssets,
        info.totalAlbums,
        info.urisSize);
}

void AlbumsRefreshWorker::TaskFusion(SyncNotifyInfo &info)
{
    SyncNotifyInfo firstTask = taskQueue_.front();
    info.forceRefreshType = firstTask.forceRefreshType;
    info.taskType = firstTask.taskType;
    info.syncId = firstTask.syncId;
    if (info.forceRefreshType != ForceRefreshType::NONE || info.taskType != TIME_IN_SYNC) {
        CHECK_AND_EXECUTE(firstTask.taskType != TIME_IN_SYNC_NO_REFRESH, info = firstTask);
        taskQueue_.pop();
        return;
    }
    PrintSyncInfo(firstTask);
    for (auto it = firstTask.uris.begin(); it != firstTask.uris.end(); ++it) {
        string uriString = (*it).ToString();
        string uriId = extractIdByPhotoUriString(uriString);
        info.uriIds.insert(uriId);
        MEDIA_DEBUG_LOG("#testfusion1 uriString: %{public}s, uriId: %{public}s, info.uriIds.size: %{public}zu",
            uriString.c_str(),
            uriId.c_str(),
            info.uriIds.size());
    }
    taskQueue_.pop();
    int32_t maxFusionUriSize = (firstTask.notifyType == NOTIFY_ADD) ? MAX_ADD_FUSION_URIS : MAX_OTHER_FUSION_URIS;
    while (!taskQueue_.empty()) {
        SyncNotifyInfo nextTask = taskQueue_.front();
        PrintSyncInfo(nextTask);
        if (nextTask.uriType != firstTask.uriType || nextTask.notifyType != firstTask.notifyType ||
            static_cast<int32_t>(info.uriIds.size()) > maxFusionUriSize ||
            nextTask.forceRefreshType != ForceRefreshType::NONE || nextTask.taskType != TIME_IN_SYNC) {
            break;
        }
        for (auto it = nextTask.uris.begin(); it != nextTask.uris.end(); ++it) {
            string uriString = (*it).ToString();
            string uriId = extractIdByPhotoUriString(uriString);
            info.uriIds.insert(uriId);
            MEDIA_DEBUG_LOG("#testfusion2 uriString: %{public}s, uriId: %{public}s, info.uriIds.size: %{public}zu",
                uriString.c_str(),
                uriId.c_str(),
                info.uriIds.size());
        }
        taskQueue_.pop();
    }
    info.urisSize = info.uriIds.size();
    info.notifyType = firstTask.notifyType;
    PrintSyncInfo(info);
}

static inline bool CheckCloudIdIsEmpty(string uriString)
{
    return extractIdByAlbumUriString(uriString).empty();
}

void AlbumsRefreshWorker::GetSystemAlbumIds(SyncNotifyInfo &info, std::vector<std::string> &albumIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdb");
    vector<string> cloudIds;
    for (auto uri : info.uris) {
        string uriString = uri.ToString();
        string cloudId = extractIdByAlbumUriString(uriString);
        if (cloudId.empty()) {
            continue;
        }
        cloudIds.emplace_back(cloudId);
    }
    CHECK_AND_RETURN(!cloudIds.empty());
    auto resultSet = AlbumsRefreshManager::GetInstance().CovertCloudId2AlbumId(rdbStore, cloudIds);
    CHECK_AND_RETURN(resultSet != nullptr);
    do {
        int32_t ablumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        albumIds.push_back(std::to_string(ablumId));
    } while (resultSet->GoToNextRow() == E_OK);
    resultSet->Close();
}

void AlbumsRefreshWorker::TryDeleteAlbum(SyncNotifyInfo &info, std::vector<std::string> &albumIds)
{
    bool cond = (info.notifyType != NOTIFY_REMOVE || CheckCloudIdIsEmpty(info.uris.front().ToString()));
    CHECK_AND_RETURN(!cond);
    CloudAlbumHandler::DeleteOrUpdateCloudAlbums(albumIds);
}

void AlbumsRefreshWorker::TaskExecute(SyncNotifyInfo &info)
{
    if (info.taskType == TIME_END_SYNC) {
        VariantMap map = {{KEY_END_DOWNLOAD_TIME, MediaFileUtils::UTCTimeMilliSeconds()}};
        PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
        PostEventUtils::GetInstance().PostCloudDownloadSyncStat(info.syncId);
    }
    AlbumsRefreshManager::GetInstance().RefreshPhotoAlbums(info);
}

void AlbumsRefreshWorker::TaskNotify(SyncNotifyInfo &info)
{
    CHECK_AND_RETURN(info.refreshResult == E_SUCCESS);
    if (info.notifyAssets) {
        AlbumsRefreshNotify::SendBatchUris(info.notifyType, info.uris, info.extraUris);
    }
    if (info.notifyAlbums) {
        list<Uri> emptyUris;
        AlbumsRefreshNotify::SendBatchUris(info.notifyType, info.extraUris, emptyUris);
    }
}

void AlbumsRefreshWorker::DealWithTasks()
{
    MEDIA_INFO_LOG("albums refresh consumer thread start");
    bool loopCondition = true;
    while (loopCondition && !stop) {
        bool needExecute = false;
        SyncNotifyInfo info;
        {
            unique_lock<mutex> lock(queueMutex_);
            if (condVar_.wait_for(lock, chrono::seconds(WAIT_TIME), [this]() { return stop || !taskQueue_.empty(); })) {
                if (stop || taskQueue_.empty()) {
                    loopCondition = false;
                    break;
                }
                SyncNotifyInfo task = taskQueue_.front();
                if (task.uriType == ALBUM_URI_TYPE) {
                    AlbumsRefreshManager::GetInstance().NotifyPhotoAlbums(task);
                    taskQueue_.pop();
                } else {
                    TaskFusion(info);
                    needExecute = true;
                }
            } else {
                loopCondition = false;
                break;
            }
        }
        if (needExecute) {
            TaskExecute(info);
            TaskNotify(info);
        }
    }
    MEDIA_INFO_LOG("albums refresh worker thread task queue is empty for %{public}d seconds", WAIT_TIME);
    isThreadAlive = false;
}
}  // namespace Media
}  // namespace OHOS
