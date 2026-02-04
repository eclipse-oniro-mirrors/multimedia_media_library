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

#include "uri_convert_handler.h"

#include "media_column.h"
#include "media_string_utils.h"
#include "media_log.h"
#include "photo_album_column.h"

using namespace std;

namespace OHOS {
namespace Media {

using ChangeType = DataShare::DataShareObserver::ChangeType;
unordered_map<ChangeType, NotifyType> NotifyTypeChangeMap = {
    {ChangeType::INSERT, NotifyType::NOTIFY_ADD},
    {ChangeType::UPDATE, NotifyType::NOTIFY_UPDATE},
    {ChangeType::DELETE, NotifyType::NOTIFY_REMOVE},
};

static void AddNewNotify(CloudSyncHandleData &newHandleData,
    const list<pair<Uri, string>> &sendUris, const ChangeType &changeType)
{
    if (sendUris.size() <= 0) {
        return;
    }
    if (NotifyTypeChangeMap.find(changeType) == NotifyTypeChangeMap.end()) {
        MEDIA_WARN_LOG("can't find changetype: %{public}d", changeType);
        return;
    }
    if (changeType == ChangeType::INSERT) {
        return;
    }
    ChangeType sendType = static_cast<ChangeType>(NotifyTypeChangeMap[changeType]);
    if (newHandleData.notifyInfo.find(sendType) == newHandleData.notifyInfo.end()) {
        newHandleData.notifyInfo[sendType] = sendUris;
    } else {
        newHandleData.notifyInfo[sendType].insert(
            newHandleData.notifyInfo[sendType].end(), sendUris.begin(), sendUris.end());
    }
    return;
}

static string GetMediaUriWithRemark(const string &mediaUri, string &remark)
{
    if (!MediaStringUtils::StartsWith(mediaUri, PhotoColumn::PHOTO_URI_PREFIX)) {
        return mediaUri;
    }

    size_t pos = mediaUri.find("/", PhotoColumn::PHOTO_URI_PREFIX.length());
    if (pos == string::npos) {
        return mediaUri;
    }
    string sufffix = mediaUri.substr(pos + 1);
    if (sufffix == "meta" || sufffix == "asset") {
        remark = sufffix;
        return PhotoColumn::PHOTO_URI_PREFIX +
               mediaUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length(), pos - PhotoColumn::PHOTO_URI_PREFIX.length());
    }
    return mediaUri;
}

void UriConvertHandler::Handle(const CloudSyncHandleData &handleData)
{
    const string org_uri_prefix = "file://cloudsync/";
    const string gallery_uri_perfix = "file://cloudsync/gallery/";
    const string new_uri_prefix = "file://media/";
    CloudSyncNotifyInfo newNotifyInfo;
    CloudSyncHandleData newHandleData = handleData;

    if (handleData.orgInfo.type == ChangeType::OTHER) {
        AddNewNotify(newHandleData, { make_pair(Uri(PhotoColumn::PHOTO_URI_PREFIX), "") }, ChangeType::DELETE);
        AddNewNotify(newHandleData, { make_pair(Uri(PhotoAlbumColumns::ALBUM_URI_PREFIX), "") }, ChangeType::DELETE);
    } else {
        ChangeType type = handleData.orgInfo.type;
        list<pair<Uri, string>> sendUris;
        for (auto &uri : handleData.orgInfo.uris) {
            string uriString = uri.ToString();
            MEDIA_DEBUG_LOG("cloud_lake debug: uriString is %{public}s", uriString.c_str());
            size_t pos = uriString.find(gallery_uri_perfix);
            string mediaUriStr;
            if (pos == string::npos) {
                pos = uriString.find(org_uri_prefix);
                mediaUriStr = uriString.replace(pos, org_uri_prefix.length(), new_uri_prefix);
            } else {
                mediaUriStr = uriString.replace(pos, gallery_uri_perfix.length(), new_uri_prefix);
            }
            string remark = "";
            mediaUriStr = GetMediaUriWithRemark(mediaUriStr, remark);
            Uri newUri = Uri(mediaUriStr);
            MEDIA_DEBUG_LOG("cloud_lake debug: uri is %{public}s, remark is %{public}s",
                newUri.ToString().c_str(), remark.c_str());
            sendUris.push_back(make_pair(newUri, remark));
        }
        AddNewNotify(newHandleData, sendUris, type);
    }
 
    if (nextHandler_ != nullptr) {
        nextHandler_->Handle(newHandleData);
    }
    return ;
}
} //namespace Media
} //namespace OHOS
