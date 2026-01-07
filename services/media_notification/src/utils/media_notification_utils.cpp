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

#include "media_notification_utils.h"

#include <thread>
#include <securec.h>
#include "media_log.h"
#include "parcel.h"
#include "parameters.h"
#include "media_change_info.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
using namespace Notification;
const size_t MAX_PARCEL_SIZE = 200 * 1024 * 0.95;
const uint64_t INTERVAL_TIME_MS = 10;

const std::string PARAM_NEED_DC_BASE_QUOTA_ANALYSIS = "persist.multimedia.media_analysis.dc_base_quota_analysis";
const std::string NO_NEED_DC_BASE_QUOTA_ANALYSIS = "0";
const std::string NEED_DC_BASE_QUOTA_ANALYSIS = "1";

const std::string PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS = "persist.multimedia.media_analysis.dc_extra_quota_analysis";
const std::string NO_NEED_DC_EXTRA_QUOTA_ANALYSIS = "0";
const std::string NEED_DC_EXTRA_QUOTA_ANALYSIS = "1";

const std::string PARAM_NEED_DC_PROACTIVE_ANALYSIS = "persist.multimedia.media_analysis.dc_proactive_analysis";
const std::string NO_NEED_DC_PROACTIVE_ANALYSIS = "0";
const std::string NEED_DC_PROACTIVE_ANALYSIS = "1";

struct MarshallingPtrVisitor {
    std::shared_ptr<Parcel> &parcel;
    bool isSystem;
    bool operator()(const PhotoAssetChangeData &data) const
    {
        return data.Marshalling(*parcel, isSystem);
    }
    bool operator()(const AlbumChangeData &data) const
    {
        return data.Marshalling(*parcel, isSystem);
    }
};

std::shared_ptr<AssetManagerNotifyInfo> NotificationUtils::UnmarshalAssetManagerNotify(Parcel &parcel)
{
    AssetManagerNotifyInfo* info = new (std::nothrow)AssetManagerNotifyInfo();
    if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
        delete info;
        info = nullptr;
    }
    return std::shared_ptr<AssetManagerNotifyInfo>(info);
}

std::shared_ptr<MediaChangeInfo> NotificationUtils::UnmarshalInMultiMode(Parcel &parcel)
{
    MediaChangeInfo* info = new (std::nothrow)MediaChangeInfo();
    if ((info != nullptr) && (!info->ReadFromParcelInMultiMode(parcel))) {
        delete info;
        info = nullptr;
    }
    return std::shared_ptr<MediaChangeInfo>(info);
}

bool NotificationUtils::Marshalling(const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo,
    std::vector<std::shared_ptr<Parcel>> &parcels)
{
    size_t index = 0;
    do {
        std::shared_ptr<Parcel> parcel = std::make_shared<Parcel>();
        bool validFlag = true;
        // 在每个包的每一个ChangeData前增加一个标志位，该标志位为false表示为parcel的最后一帧ChangeData。不在压缩和读取ChangeData
        parcel->WriteBool(mediaChangeInfo->isForRecheck);
        parcel->WriteUint16(static_cast<uint16_t>(mediaChangeInfo->notifyUri));
        parcel->WriteUint16(static_cast<uint16_t>(mediaChangeInfo->notifyType));
        parcel->WriteBool(mediaChangeInfo->isSystem);

        for (size_t i = index; i < mediaChangeInfo->changeInfos.size(); i++) {
            size_t currentDataSize = parcel->GetDataSize();
            if (currentDataSize > MAX_PARCEL_SIZE) { // 待补充,动态内存大小难以确定
                validFlag = false;
                parcel->WriteBool(validFlag);
                MEDIA_WARN_LOG("assetChangeData or lbumChangeData size exceeds the maximum limit.");
                break;
            } else {
                validFlag = true;
                ++index;
                parcel->WriteBool(validFlag);
            }

            if (std::holds_alternative<PhotoAssetChangeData>(mediaChangeInfo->changeInfos[i])) {
                parcel->WriteBool(true);
            } else if (std::holds_alternative<AlbumChangeData>(mediaChangeInfo->changeInfos[i])) {
                parcel->WriteBool(false);
            } else {
                MEDIA_ERR_LOG("fail to marshalling.");
                return false;
            }
            std::visit(MarshallingPtrVisitor{parcel, mediaChangeInfo->isSystem}, mediaChangeInfo->changeInfos[i]);
        }
        parcel->WriteBool(false);
        parcels.push_back(parcel);
    } while (index < mediaChangeInfo->changeInfos.size());
    return true;
}

bool NotificationUtils::WriteToChangeInfo(const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo,
    std::vector<std::shared_ptr<AAFwk::ChangeInfo>> &changeInfos)
{
    std::vector<std::shared_ptr<Parcel>> parcels;
    bool ret = Marshalling(mediaChangeInfo, parcels);
    if (!ret) {
        MEDIA_INFO_LOG("fail to marshlling");
        return false;
    }
    for (auto &item : parcels) {
        uintptr_t buf = item->GetData();
        if (item->GetDataSize() == 0) {
            MEDIA_ERR_LOG("fail to marshalling sercerParcel");
            return false;
        }
        auto *uBuf = new (std::nothrow) uint8_t[item->GetDataSize()];
        if (uBuf == nullptr) {
            MEDIA_ERR_LOG("parcel->GetDataSize is null");
            return false;
        }
        int ret = memcpy_s(uBuf, item->GetDataSize(), reinterpret_cast<uint8_t *>(buf), item->GetDataSize());
        if (ret != 0) {
            MEDIA_ERR_LOG("Parcel data copy failed, err = %{public}d", ret);
            if (uBuf != nullptr) {
                delete[] uBuf;
                uBuf = nullptr;
            }
            return false;
        }
        std::shared_ptr<AAFwk::ChangeInfo> serverChangeInfo = std::make_shared<AAFwk::ChangeInfo>();
        serverChangeInfo->data_ = uBuf;
        serverChangeInfo->size_ = item->GetDataSize();
        MEDIA_INFO_LOG("serverChangeInfo->size_ is: %{public}d", (int)item->GetDataSize());
        changeInfos.push_back(serverChangeInfo);
    }
    return true;
}

int32_t NotificationUtils::SendNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
    const std::shared_ptr<MediaChangeInfo> &mediaChangeInfo)
{
    if (dataObserver == nullptr || mediaChangeInfo == nullptr) {
        MEDIA_ERR_LOG("dataObserver or mediaChangeInfo is nullptr");
        return E_ERR;
    }
    std::shared_ptr<NotificationUtils> utilsHandle = std::make_shared<NotificationUtils>();
    std::vector<std::shared_ptr<AAFwk::ChangeInfo>> changeInfos;
    bool ret = utilsHandle->WriteToChangeInfo(mediaChangeInfo, changeInfos);
    if (!ret || changeInfos.size() == 0) {
        MEDIA_ERR_LOG("fail to write changeInfo");
        return E_ERR;
    }

    for (auto &changeInfo : changeInfos) {
        MEDIA_INFO_LOG("start send notification at an interval of 10 milliseconds");
        dataObserver->OnChangeExt(*changeInfo);
        if (changeInfo->data_ != nullptr) {
            delete[] static_cast<uint8_t*>(changeInfo->data_);
            changeInfo->data_ = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_TIME_MS));
    }
    return E_OK;
}

int32_t NotificationUtils::SendDownloadProgressInfoNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
    const std::shared_ptr<AAFwk::ChangeInfo> &changeInfo)
{
    MEDIA_INFO_LOG("SendDownloadProgressInfoNotification OnChangeExt");
    CHECK_AND_RETURN_RET_LOG(dataObserver != nullptr && changeInfo != nullptr, E_ERR,
        "dataObserver or changeInfo is nullptr");
    dataObserver->OnChangeExt(*changeInfo);
    return E_OK;
}

void NotificationUtils::UpdateNotificationProp()
{
    if (system::GetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NO_NEED_DC_BASE_QUOTA_ANALYSIS) !=
        NEED_DC_BASE_QUOTA_ANALYSIS) {
        bool isSet = system::SetParameter(PARAM_NEED_DC_BASE_QUOTA_ANALYSIS, NEED_DC_BASE_QUOTA_ANALYSIS);
        if (isSet) {
            MEDIA_DEBUG_LOG("set %{public}s success", PARAM_NEED_DC_BASE_QUOTA_ANALYSIS.c_str());
        } else {
            MEDIA_WARN_LOG("set %{public}s failed", PARAM_NEED_DC_BASE_QUOTA_ANALYSIS.c_str());
        }
    }

    if (system::GetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NO_NEED_DC_EXTRA_QUOTA_ANALYSIS) !=
        NEED_DC_EXTRA_QUOTA_ANALYSIS) {
        bool isSet = system::SetParameter(PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS, NEED_DC_EXTRA_QUOTA_ANALYSIS);
        if (isSet) {
            MEDIA_DEBUG_LOG("set %{public}s success", PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS.c_str());
        } else {
            MEDIA_WARN_LOG("set %{public}s failed", PARAM_NEED_DC_EXTRA_QUOTA_ANALYSIS.c_str());
        }
    }

    if (system::GetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NO_NEED_DC_PROACTIVE_ANALYSIS) !=
        NEED_DC_PROACTIVE_ANALYSIS) {
        bool isSet = system::SetParameter(PARAM_NEED_DC_PROACTIVE_ANALYSIS, NEED_DC_PROACTIVE_ANALYSIS);
        if (isSet) {
            MEDIA_DEBUG_LOG("set %{public}s success", PARAM_NEED_DC_PROACTIVE_ANALYSIS.c_str());
        } else {
            MEDIA_WARN_LOG("set %{public}s failed", PARAM_NEED_DC_PROACTIVE_ANALYSIS.c_str());
        }
    }
}

int32_t NotificationUtils::SendUserDefineNotification(const sptr<AAFwk::IDataAbilityObserver> &dataObserver,
    const std::shared_ptr<AAFwk::ChangeInfo> &changeInfo)
{
    MEDIA_INFO_LOG("SendUserDefineNotification OnChangeExt");
    CHECK_AND_RETURN_RET_LOG(dataObserver != nullptr && changeInfo != nullptr, E_ERR,
        "dataObserver or changeInfo is nullptr");
    dataObserver->OnChangeExt(*changeInfo);
    return E_OK;
}
} // OHOS::Media