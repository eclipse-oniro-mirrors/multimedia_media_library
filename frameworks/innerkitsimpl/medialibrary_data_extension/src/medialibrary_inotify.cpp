/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileInotify"
#include "medialibrary_inotify.h"

#include <string>
#include <thread>

#include "unistd.h"
#include "media_log.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_uripermission_operations.h"
#include "permission_utils.h"
#include "dfx_utils.h"

using namespace std;
namespace OHOS {
namespace Media {
std::shared_ptr<MediaLibraryInotify> MediaLibraryInotify::instance_ = nullptr;
std::mutex MediaLibraryInotify::mutex_;
const int32_t MAX_WATCH_LIST = 1000;
const int32_t SINGLE_BUNDLE_LIST = 200;
const int32_t MAX_AGING_WATCH_LIST = 100;

shared_ptr<MediaLibraryInotify> MediaLibraryInotify::GetInstance()
{
    if (instance_ != nullptr) {
        return instance_;
    }
    lock_guard<mutex> lock(mutex_);
    if (instance_ == nullptr) {
        instance_ = make_shared<MediaLibraryInotify>();
        if (instance_ == nullptr) {
            MEDIA_ERR_LOG("GetInstance nullptr");
            return instance_;
        }
        instance_->Init();
    }
    return instance_;
}

static string ConvertMediaPath(const std::string &path)
{
    // if input path is /storage/media/local/xxx, convert to /storage/cloud/xxx
    string mediaPath = path;
    string localPath = "/storage/media/local/";
    string cloudPath = "/storage/cloud/";
    if (mediaPath.find(localPath) != string::npos) {
        mediaPath.replace(mediaPath.find(localPath), localPath.length(), cloudPath);
    }
    return mediaPath;
}

void MediaLibraryInotify::WatchCallBack()
{
    string name("MediaLibraryInotify");
    pthread_setname_np(pthread_self(), name.c_str());
    MEDIA_INFO_LOG("MediaLibraryInotify start watchthread");
    const int32_t READ_LEN = 255;
    char data[READ_LEN] = {0};
    while (isWatching_) {
        int32_t len = read(inotifyFd_, data, READ_LEN);
        if (len < static_cast<int32_t>(sizeof(struct inotify_event))) {
            MEDIA_ERR_LOG("read event error, len: %{public}d, fd: %{public}d, error: %{public}d", len, inotifyFd_,
                errno);
            Restart();
            return;
        }
        int32_t index = 0;
        while (index < len) {
            struct inotify_event *event = reinterpret_cast<struct inotify_event *>(data + index);
            index += static_cast<int32_t>(sizeof(struct inotify_event)) + event->len;
            unique_lock<mutex> lock(mutex_);
            if (watchList_.count(event->wd) == 0) {
                continue;
            }
            auto &item = watchList_.at(event->wd);
            auto eventMask = event->mask;
            auto &meetEvent = item.meetEvent_;
            meetEvent = (eventMask & IN_MODIFY) ? (meetEvent | IN_MODIFY) : meetEvent;
            meetEvent = (eventMask & IN_CLOSE_WRITE) ? (meetEvent | IN_CLOSE_WRITE) : meetEvent;
            meetEvent = (eventMask & IN_CLOSE_NOWRITE) ? (meetEvent | IN_CLOSE_NOWRITE) : meetEvent;
            if (((meetEvent & IN_CLOSE_WRITE) && (meetEvent & IN_MODIFY)) ||
                ((meetEvent & IN_CLOSE_NOWRITE) && (meetEvent & IN_MODIFY))) {
                MEDIA_INFO_LOG("path:%{public}s, meetEvent:%{public}u, uri:%{public}s",
                    MediaFileUtils::DesensitizePath(item.path_).c_str(),
                    meetEvent,
                    MediaFileUtils::DesensitizeUri(item.uri_).c_str());
                string id = MediaFileUtils::GetIdFromUri(item.uri_);
                string itemPath = ConvertMediaPath(item.path_);
                string bundleName = item.bundleName_;
                MediaFileUri itemUri(item.uri_);
                MediaLibraryApi itemApi = item.api_;
                Remove(event->wd);
                lock.unlock();
                MediaLibraryObjectUtils::ScanFileAsync(itemPath, id, itemApi);
                UriPermissionOperations::DeleteBundlePermission(id, bundleName, itemUri.GetTableName());
            }
        }
    }
    MEDIA_INFO_LOG("MediaLibraryInotify end watchthread");
    isWatching_ = false;
}

void MediaLibraryInotify::Restart()
{
    for (auto iter = watchList_.begin(); iter != watchList_.end(); iter++) {
        if (inotify_rm_watch(inotifyFd_, iter->first) != 0) {
            MEDIA_ERR_LOG("rm watch fd: %{public}d, fail: %{public}d", iter->first, errno);
        }
    }
    isWatching_ = false;
    watchList_.clear();
    inotifyFd_ = 0;
    Init();
}

void MediaLibraryInotify::DoAging()
{
    lock_guard<mutex> lock(mutex_);
    if (watchList_.size() > MAX_AGING_WATCH_LIST) {
        MEDIA_DEBUG_LOG("watch list clear");
        watchList_.clear();
    }
}

void MediaLibraryInotify::DoStop()
{
    lock_guard<mutex> lock(mutex_);
    for (auto iter = watchList_.begin(); iter != watchList_.end(); iter++) {
        if (inotify_rm_watch(inotifyFd_, iter->first) != 0) {
            MEDIA_ERR_LOG("rm watch fd: %{public}d, fail: %{public}d", iter->first, errno);
        }
    }
    isWatching_ = false;
    watchList_.clear();
    inotifyFd_ = 0;
    MEDIA_INFO_LOG("stop success");
}

int32_t MediaLibraryInotify::RemoveByFileUri(const string &uri, MediaLibraryApi api)
{
    lock_guard<mutex> lock(mutex_);
    int32_t wd = -1;
    for (auto iter = watchList_.begin(); iter != watchList_.end(); iter++) {
        if (iter->second.uri_ == uri && iter->second.api_ == api) {
            wd = iter->first;
            MEDIA_DEBUG_LOG("remove uri:%{public}s wd:%{public}d path:%{public}s",
                iter->second.uri_.c_str(), wd, iter->second.path_.c_str());
            break;
        }
    }
    if (wd < 0) {
        MEDIA_DEBUG_LOG("remove uri:%s fail", uri.c_str());
        return E_FAIL;
    }
    return Remove(wd);
}

int32_t MediaLibraryInotify::Remove(int wd)
{
    watchList_.erase(wd);
    if (inotify_rm_watch(inotifyFd_, wd) != 0) {
        MEDIA_ERR_LOG("rm watch fd:%{public}d fail:%{public}d", wd, errno);
        return E_FAIL;
    }
    return E_SUCCESS;
}

int32_t MediaLibraryInotify::Init()
{
    if (inotifyFd_ <= 0) {
        inotifyFd_ = inotify_init();
        if (inotifyFd_ < 0) {
            MEDIA_ERR_LOG("add AddWatchList fail fd: %{public}d, error: %{public}d", inotifyFd_, errno);
            return E_FAIL;
        }
        MEDIA_INFO_LOG("init inotifyFd_: %{public}d success", inotifyFd_);
    }
    return E_SUCCESS;
}

int32_t MediaLibraryInotify::GetBundleCount(const std::string &bundleName)
{
    int count = 0;
    for (auto &pair : watchList_) {
        if (bundleName.compare(pair.second.bundleName_) == 0) {
            count++;
        }
    }
    MEDIA_DEBUG_LOG("MediaLibraryInotify GetBundleCount count:%{public}d", count);
    return count;
}

const string MediaLibraryInotify::BuildDfxInfo()
{
    unordered_map<string, WatchBundleInfo> bundleInfoMap;
    for (auto &pair : watchList_) {
        string bundleName = pair.second.bundleName_;
        if (bundleInfoMap.find(bundleName) != bundleInfoMap.end()) {
            auto it = bundleInfoMap.find(bundleName);
            int32_t count = ++it->second.count;
            int64_t firstTime = pair.second.currentTime_ < it->second.firstEntryTime ?
                pair.second.currentTime_ : it->second.firstEntryTime;
            string firstUri = pair.second.currentTime_ < it->second.firstEntryTime ?
                pair.second.uri_ : it->second.firstUri;
            WatchBundleInfo watchBundleInfo(count, firstTime, firstUri, bundleName);
            it->second = watchBundleInfo;
        } else {
            WatchBundleInfo watchBundleInfo(1, pair.second.currentTime_, pair.second.uri_, bundleName);
            bundleInfoMap.emplace(bundleName, watchBundleInfo);
        }
    }
    string dfxInfo;
    for (const auto &pair : bundleInfoMap) {
        dfxInfo.append(pair.second.Dump());
        dfxInfo.append(";");
    }
    return dfxInfo;
}

int32_t MediaLibraryInotify::AddWatchList(const string &path, const string &uri, MediaLibraryApi api)
{
    lock_guard<mutex> lock(mutex_);
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (watchList_.size() >= MAX_WATCH_LIST || GetBundleCount(bundleName) >= SINGLE_BUNDLE_LIST) {
        MEDIA_ERR_LOG("watch list full, add uri:%{public}s fail, bundleName:%{public}s, info:%{public}s",
            uri.c_str(), bundleName.c_str(), BuildDfxInfo().c_str());
        return E_FAIL;
    }
    int32_t wd = inotify_add_watch(inotifyFd_, path.c_str(), IN_CLOSE | IN_MODIFY);
    if (wd > 0) {
        int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
        struct WatchInfo item(path, uri, bundleName, api, currentTime);
        MEDIA_INFO_LOG("inotify emplace path:%{public}s, bundleName:%{public}s, watchSize:%{public}d,",
            DfxUtils::GetSafePath(path).c_str(), bundleName.c_str(), static_cast<int32_t>(watchList_.size()));
        watchList_.emplace(wd, item);
    }
    if (!isWatching_.load()) {
        isWatching_ = true;
        thread(&MediaLibraryInotify::WatchCallBack, this).detach();
    }
    return E_SUCCESS;
}
} // namespace Media
} // namespace OHOS