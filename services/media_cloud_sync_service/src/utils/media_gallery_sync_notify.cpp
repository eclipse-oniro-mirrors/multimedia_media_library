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
#define MLOG_TAG "Media_Cloud_Utils"

#include "media_gallery_sync_notify.h"

#include <string>
#include <vector>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "cloud_media_sync_const.h"
#include "media_column.h"
#include "photo_album_column.h"

namespace OHOS::Media::CloudSync {
using namespace std;
using ChangeType = AAFwk::ChangeInfo::ChangeType;
using NotifyDataMap = unordered_map<ChangeType, list<Uri>>;
unordered_map<ChangeType, list<Uri>> MediaGallerySyncNotify::notifyListMap_ = {};
int32_t MediaGallerySyncNotify::recordAdded_ = 0;
std::mutex MediaGallerySyncNotify::mtx_{};
constexpr int NOTIFY_INTERVALS = 50;
const std::string GALLERY_PROGRESS_URI = PhotoAlbumColumns::PHOTO_GALLERY_CLOUD_SYNC_INFO_URI_PREFIX;

MediaGallerySyncNotify &MediaGallerySyncNotify::GetInstance()
{
    static MediaGallerySyncNotify instance;
    return instance;
}

static void PrintUriList(ChangeType changeType, const list<Uri> &uris)
{
    if (uris.size() > 0) {
        MEDIA_INFO_LOG("NotifyChange notify changeType = %{public}d, size = %{public}zu, uri = %{public}s",
            changeType,
            uris.size(),
            uris.front().ToString().c_str());
    } else {
        MEDIA_INFO_LOG("NotifyChange notify changeType = %{public}d, size = %{public}zu", changeType, uris.size());
    }
}

static int32_t TryNotifyChange()
{
    MediaGallerySyncNotify::recordAdded_++;
    if (MediaGallerySyncNotify::recordAdded_ % NOTIFY_INTERVALS == 0) {
        auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
        if (obsMgrClient == nullptr) {
            MEDIA_ERR_LOG("TryNotifyChange %{public}s obsMgrClient is nullptr", __func__);
            return E_IPC_SA_LOAD_FAILED;
        }
        for (auto it = MediaGallerySyncNotify::notifyListMap_.begin();
                  it != MediaGallerySyncNotify::notifyListMap_.end(); ++it) {
            obsMgrClient->NotifyChangeExt({it->first, it->second});
            PrintUriList(it->first, it->second);
        }
        MediaGallerySyncNotify::notifyListMap_.clear();
        MediaGallerySyncNotify::recordAdded_ = 0;
    }
    return E_OK;
}

static int32_t NotifyFileAssetChange(bool notify, const std::string &uri, const ChangeType changeType)
{
    std::lock_guard<mutex> lock(MediaGallerySyncNotify::mtx_);
    auto iterator = MediaGallerySyncNotify::notifyListMap_.find(changeType);
    if (iterator != MediaGallerySyncNotify::notifyListMap_.end()) {
        iterator->second.emplace_back(Uri(uri));
    } else {
        list<Uri> newList;
        newList.emplace_back(Uri(uri));
        MediaGallerySyncNotify::notifyListMap_.insert(make_pair(changeType, newList));
    }
    if (!notify) {
        return E_OK;
    }
    return TryNotifyChange();
}

static int32_t NotifyAlbumChange(const bool notify, const std::string &uri, const ChangeType changeType)
{
    return NotifyFileAssetChange(notify, uri, changeType);
}

static int32_t NotifyAlbumMapChange(const std::string &uri, const ChangeType changeType, const std::string &fileAssetId)
{
    return E_OK;
}

static int32_t AddAndNotify(
    const bool notify, const std::string &uri, const ChangeType changeType, const std::string &fileAssetId)
{
    int ret = E_FAIL;
    if (uri.find(PhotoColumn::PHOTO_CLOUD_URI_PREFIX) != std::string::npos) {
        ret = NotifyFileAssetChange(notify, uri, changeType);
    } else if (uri.find(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX) != std::string::npos) {
        ret = NotifyFileAssetChange(notify, uri, changeType);
    } else if ((uri.find(PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX) != std::string::npos) &&
               (fileAssetId == "0")) {
        ret = NotifyAlbumChange(notify, uri, changeType);
    } else if ((uri.find(PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX) != std::string::npos) ||
               (uri.find(PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX) != std::string::npos)) {
        ret = NotifyAlbumChange(notify, uri + fileAssetId, changeType);
    } else if (uri.find(PhotoAlbumColumns::PHOTO_GALLERY_DOWNLOAD_URI_PREFIX) != std::string::npos) {
        ret = NotifyFileAssetChange(notify, uri + fileAssetId, changeType);
    } else {
        ret = NotifyAlbumMapChange(uri, changeType, fileAssetId);
    }
    return ret;
}

int32_t MediaGallerySyncNotify::AddNotify(
    const std::string &uri, const ChangeType changeType, const std::string &fileAssetId)
{
    MEDIA_INFO_LOG(
        "AddNotify uri:%{public}s, type:%{public}d, fileId:%{public}s", uri.c_str(), changeType, fileAssetId.c_str());
    return AddAndNotify(false, uri, changeType, fileAssetId);
}

int32_t MediaGallerySyncNotify::TryNotify(
    const std::string &uri, const ChangeType changeType, const std::string &fileAssetId)
{
    MEDIA_INFO_LOG(
        "TryNotify uri:%{public}s, type:%{public}d, fileId:%{public}s", uri.c_str(), changeType, fileAssetId.c_str());
    return AddAndNotify(true, uri, changeType, fileAssetId);
}

int32_t MediaGallerySyncNotify::FinalNotify()
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("FinalNotify %{public}s obsMgrClient is nullptr", __func__);
        return E_IPC_SA_LOAD_FAILED;
    }
    std::lock_guard<mutex> lock(MediaGallerySyncNotify::mtx_);
    for (auto it = MediaGallerySyncNotify::notifyListMap_.begin(); it != MediaGallerySyncNotify::notifyListMap_.end();
         ++it) {
        obsMgrClient->NotifyChangeExt({it->first, it->second});
        PrintUriList(it->first, it->second);
    }
    MediaGallerySyncNotify::notifyListMap_.clear();
    MediaGallerySyncNotify::recordAdded_ = 0;
    return E_OK;
}

int32_t MediaGallerySyncNotify::NotifyProgress(NotifyTaskType taskType, const std::string &syncId,
    NotifySyncType syncType, uint32_t totalAlbums, uint32_t totalAssets)
{
    std::string params = "{\"syncId\":\"" + syncId + "\",";
    params += "\"syncType\":" + to_string(syncType) + ",";
    params += "\"taskType\":" + to_string(taskType) + ",";
    params += "\"totalAlbums\":" + to_string(totalAlbums) + ",";
    params += "\"totalAssets\":" + to_string(totalAssets) + "}";

    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("NotifyProgress %{public}s obsMgrClient is nullptr", __func__);
        return E_IPC_SA_LOAD_FAILED;
    }
    AAFwk::ChangeInfo changeInfo;
    changeInfo.uris_.push_back(Uri(GALLERY_PROGRESS_URI));
    changeInfo.data_ = (void *)params.c_str();
    changeInfo.size_ = params.length();
    MEDIA_INFO_LOG("NotifyProgress params = %{public}s, size is %{public}zu", params.c_str(), params.length());
    obsMgrClient->NotifyChangeExt(changeInfo);

    return E_OK;
}

void MediaGallerySyncNotify::NotifyProgressBegin()
{
    if (!syncId_.empty()) {
        return;
    }
    std::time_t timeStamp = std::time(nullptr);
    syncId_ = std::to_string(timeStamp);
    NotifyProgress(NotifyTaskType::NOTIFY_BEGIN, syncId_, NotifySyncType::NOTIFY_INCREMENTAL_SYNC, 0, 0);
}

void MediaGallerySyncNotify::NotifyProgressEnd()
{
    NotifyProgress(NotifyTaskType::NOTIFY_END, syncId_, NotifySyncType::NOTIFY_INCREMENTAL_SYNC, 0, 0);
    syncId_ = "";
}
}  // namespace OHOS::Media::CloudSync
