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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_MEDIA_SYNC_OBSERVER_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_MEDIA_SYNC_OBSERVER_H_

#include "datashare_helper.h"
#include "mtp_constants.h"
#include "media_column.h"
#include "mtp_packet_tools.h"
#include "mtp_operation_context.h"
#include "media_mtp_utils.h"
#include "medialibrary_async_worker.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {

struct MediaSyncNotifyInfo {
    std::list<Uri> uris;
    DataShare::DataShareObserver::ChangeType type;
};

class MediaSyncObserver : public DataShare::DataShareObserver {
public:
    MediaSyncObserver() = default;
    ~MediaSyncObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
    std::shared_ptr<MtpOperationContext> context_ = nullptr;
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_ = nullptr;
private:
    void SendEventPackets(uint32_t objectHandle, uint16_t eventCode);
    void SendEventPacketAlbum(uint32_t objectHandle, uint16_t eventCode);
    void SendPhotoEvent(ChangeType changeType, std::string suffixString);
    std::vector<int32_t> GetHandlesFromPhotosInfoBurstKeys(int32_t handle);
};

class MediaSyncNotifyData : public AsyncTaskData {
public:
    MediaSyncNotifyData(const MediaSyncNotifyInfo &info):notifyInfo_(info) {};
    virtual ~MediaSyncNotifyData() override = default;
    MediaSyncNotifyInfo notifyInfo_;
};
} // namespace Media
} // namespace OHOS

#endif //FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_MEDIA_SYNC_OBSERVER_H_
