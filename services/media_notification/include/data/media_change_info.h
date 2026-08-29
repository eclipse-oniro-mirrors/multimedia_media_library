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

#ifndef OHOS_MEDIA_CHANGE_INFO_H
#define OHOS_MEDIA_CHANGE_INFO_H

#include <string>
#include <list>
#include <variant>
#include <sstream>
#include "parcel.h"
#include "media_log.h"
#include "userfile_manager_types.h"
#include "accurate_common_data.h"
#include "album_change_info.h"
#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media {
namespace Notification {
using namespace Media::AccurateRefresh;

enum AccurateNotifyType {
    NOTIFY_ASSET_ADD,
    NOTIFY_ASSET_UPDATE,
    NOTIFY_ASSET_REMOVE,
    NOTIFY_ALBUM_ADD,
    NOTIFY_ALBUM_UPDATE,
    NOTIFY_ALBUM_REMOVE,
    NOTIFY_ASSET_YUV_READY,
    NOTIFY_ASSET_ADD_ANALYSIS,
    NOTIFY_ASSET_REMOVE_ANALYSIS,
    NOTIFY_ALBUM_ADD_ANALYSIS,
    NOTIFY_ALBUM_REMOVE_ANALYSIS,
};

enum DownloadAssetsNotifyType {
    DOWNLOAD_PROGRESS = 0,
    DOWNLOAD_FINISH = 1,
    DOWNLOAD_FAILED = 2,
    DOWNLOAD_ASSET_DELETE = 3,
    DOWNLOAD_AUTO_PAUSE = 4,
    DOWNLOAD_AUTO_RESUME = 5,
    DOWNLOAD_REFRESH = 6,
};

enum NotifyUriType {
    PHOTO_URI,
    HIDDEN_PHOTO_URI,
    TRASH_PHOTO_URI,
    PHOTO_ALBUM_URI,
    HIDDEN_ALBUM_URI,
    TRASH_ALBUM_URI,
    ANALYSIS_PHOTO_URI,
    ANALYSIS_ALBUM_URI,
    SINGLE_PHOTO_URI,
    SINGLE_PHOTO_ALBUM_URI,
    INVALID,
    BATCH_DOWNLOAD_PROGRESS_URI,
    USER_DEFINE_NOTIFY_URI,
    ANY,
    AVAILABILITY_URI,
    SHARE_PHOTO_URI,
    SHARE_PHOTO_ALBUM_URI,
};

class NotifyDetailInfo : public Parcelable {
public:
    NotifyUriType uri;
    std::vector<int32_t> indexs;
    AccurateNotifyType notifyType;

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "uri:" << static_cast<uint16_t>(uri) << ", notifyType:" << static_cast<uint16_t>(notifyType) << " , ";
        ss << "indexs: {";
        for (const auto& item: indexs) {
            ss << item << " ";
        }
        ss << "}";
        return ss.str();
    }

    static std::shared_ptr<NotifyDetailInfo> Unmarshalling(Parcel &parcel)
    {
        MEDIA_DEBUG_LOG("unmarshalling debug: std::shared_ptr<NotifyDetailInfo> Unmarshalling");
        NotifyDetailInfo* info = new (std::nothrow)NotifyDetailInfo();
        if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
            delete info;
            info = nullptr;
        }
        return std::shared_ptr<NotifyDetailInfo>(info);
    }

    bool Marshalling(Parcel &parcel) const override
    {
        parcel.WriteUint16(static_cast<uint16_t>(this->uri));
        parcel.WriteInt32Vector(this->indexs);
        parcel.WriteUint16(static_cast<uint16_t>(this->notifyType));
        return true;
    }

private:
    bool ReadFromParcel(Parcel &parcel)
    {
        MEDIA_INFO_LOG("unmarshalling debug: NotifyDetailInfo::ReadFromParcel");
        this->uri = static_cast<NotifyUriType>(parcel.ReadUint16());
        parcel.ReadInt32Vector(&this->indexs);
        this->notifyType = static_cast<AccurateNotifyType>(parcel.ReadUint16());
        return true;
    }
};

struct MarshallingVisitor {
    Parcel &parcel;
    bool isSystem;
    bool operator()(const PhotoAssetChangeData &data) const
    {
        return data.Marshalling(parcel, isSystem);
    }
    bool operator()(const AlbumChangeData &data) const
    {
        return data.Marshalling(parcel, isSystem);
    }
};

struct ToStringVisitor {
    bool isDetail = false;
    std::string operator()(const PhotoAssetChangeData &data) const
    {
        return data.ToString(isDetail);
    }
    std::string operator()(const AlbumChangeData &data) const
    {
        return data.ToString(isDetail);
    }
};

class AssetManagerNotifyInfo : public Parcelable {
public:
    NotifyUriType notifyUri;
    DownloadAssetsNotifyType downloadAssetNotifyType;
    int32_t fileId;
    int32_t percent;
    int32_t autoPauseReason;
 
public:
    std::string ToString(bool isDetail = false) const
    {
        std::stringstream ss;
        ss << "notifyUri:" << static_cast<uint16_t>(notifyUri)
            << ", downloadAssetNotifyType:" << static_cast<uint16_t>(downloadAssetNotifyType)
            << ", fileId:" << fileId
            << ", percent:" << percent
            << ", autoPauseReason:" << autoPauseReason;
        return ss.str();
    }

    static std::shared_ptr<AssetManagerNotifyInfo> Unmarshalling(Parcel &parcel)
    {
        AssetManagerNotifyInfo* info = new (std::nothrow)AssetManagerNotifyInfo();
        if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
            delete info;
            info = nullptr;
        }
        return std::shared_ptr<AssetManagerNotifyInfo>(info);
    }

    bool Marshalling(Parcel &parcel) const override
    {
        parcel.WriteUint16(static_cast<uint16_t>(notifyUri));
        parcel.WriteUint16(static_cast<uint16_t>(downloadAssetNotifyType));
        parcel.WriteInt32(fileId);
        parcel.WriteInt32(percent);
        parcel.WriteInt32(autoPauseReason);
        MEDIA_INFO_LOG("Marshalling parcel size is: %{public}d", (int)parcel.GetDataSize());
        return true;
    }

    bool ReadFromParcel(Parcel &parcel)
    {
        this->notifyUri = static_cast<NotifyUriType>(parcel.ReadUint16());
        this->downloadAssetNotifyType = static_cast<DownloadAssetsNotifyType>(parcel.ReadUint16());
        this->fileId = parcel.ReadInt32();
        this->percent = parcel.ReadInt32();
        this->autoPauseReason = parcel.ReadInt32();
        MEDIA_INFO_LOG("ReadFromParcel notifyUri %{public}d", static_cast<int32_t>(this->notifyUri));
        return true;
    }
};

class MediaChangeInfo : public Parcelable {
public:
    std::vector<std::variant<PhotoAssetChangeData, AlbumChangeData>> changeInfos;
    bool isForRecheck;
    NotifyUriType notifyUri;
    AccurateNotifyType notifyType;
    bool isSystem;
 
public:
    std::string ToString(bool isDetail = false) const
    {
        std::stringstream ss;
        ss << "isForRecheck: " << isForRecheck <<", notifyUri:" << static_cast<uint16_t>(notifyUri)
            << ", notifyType:" << static_cast<uint16_t>(notifyType) << ", isSystem:" << isSystem <<".";
        for (size_t i = 0; i < changeInfos.size(); ++i) {
            ss << "changeInfo[" << i << "]: " << std::visit(ToStringVisitor{isDetail}, changeInfos[i]) << ", ";
        }
        return ss.str();
    }

    static std::shared_ptr<MediaChangeInfo> Unmarshalling(Parcel &parcel)
    {
        MediaChangeInfo* info = new (std::nothrow)MediaChangeInfo();
        if ((info != nullptr) && (!info->ReadFromParcel(parcel))) {
            delete info;
            info = nullptr;
        }
        return std::shared_ptr<MediaChangeInfo>(info);
    }
    bool Marshalling(Parcel &parcel, bool isSystem) const
    {
        size_t photoMaxLimit = 100;
        size_t photoAlbumMaxLimit = 50;
        size_t assetChangeDataSize = 0;
        size_t albumChangeDataSize = 0;
        size_t changeInfoSize = changeInfos.size();
        parcel.WriteUint32(changeInfoSize);
        for (const auto& item: changeInfos) {
            if (std::holds_alternative<PhotoAssetChangeData>(item) && assetChangeDataSize < photoMaxLimit) {
                assetChangeDataSize++;
                parcel.WriteBool(true);
            } else if (std::holds_alternative<AlbumChangeData>(item) && albumChangeDataSize < photoAlbumMaxLimit) {
                albumChangeDataSize++;
                parcel.WriteBool(false);
            } else {
                MEDIA_ERR_LOG("assetChangeData or lbumChangeData size exceeds the maximum limit.");
                return false;
            }
            std::visit(MarshallingVisitor{parcel, isSystem}, item);
        }
        parcel.WriteBool(isForRecheck);
        parcel.WriteUint16(static_cast<uint16_t>(notifyUri));
        parcel.WriteUint16(static_cast<uint16_t>(notifyType));
        parcel.WriteBool(isSystem);
        MEDIA_INFO_LOG("parcel size is: %{public}d", (int)parcel.GetDataSize());
        return true;
    }

    bool Marshalling(Parcel &parcel) const override
    {
        return Marshalling(parcel, false);
    }

    bool ReadFromParcelInMultiMode(Parcel &parcel)
    {
        this->isForRecheck = parcel.ReadBool();
        this->notifyUri = static_cast<NotifyUriType>(parcel.ReadUint16());
        this->notifyType = static_cast<AccurateNotifyType>(parcel.ReadUint16());
        this->isSystem = parcel.ReadBool();
        bool validFlag = parcel.ReadBool();
        while (validFlag) {
            bool type = parcel.ReadBool();
            if (type) {
                std::shared_ptr<PhotoAssetChangeData> item = PhotoAssetChangeData::Unmarshalling(parcel);
                if (item == nullptr) {
                    MEDIA_ERR_LOG("item is nullptr");
                    return false;
                }
                this->changeInfos.push_back(*item);
            } else {
                std::shared_ptr<AlbumChangeData> item = AlbumChangeData::Unmarshalling(parcel);
                if (item == nullptr) {
                    MEDIA_ERR_LOG("item is nullptr");
                    return false;
                }
                this->changeInfos.push_back(*item);
            }
            validFlag = parcel.ReadBool();
        }
        return true;
    }

private:
    bool ReadFromParcel(Parcel &parcel)
    {
        uint32_t size = -1;
        bool ret = parcel.ReadUint32(size);
        if (!ret) {
            MEDIA_ERR_LOG("failed to Unmarshalling notifyDetails");
            return false;
        }

        for (uint32_t i = 0; i < size; i++) {
            bool type = parcel.ReadBool();
            if (type) {
                std::shared_ptr<PhotoAssetChangeData> item = PhotoAssetChangeData::Unmarshalling(parcel);
                if (item == nullptr) {
                    MEDIA_ERR_LOG("item is nullptr");
                    return false;
                }
                this->changeInfos.push_back(*item);
            } else {
                std::shared_ptr<AlbumChangeData> item = AlbumChangeData::Unmarshalling(parcel);
                if (item == nullptr) {
                    MEDIA_ERR_LOG("item is nullptr");
                    return false;
                }
                this->changeInfos.push_back(*item);
            }
        }

        this->isForRecheck = parcel.ReadBool();
        this->notifyUri = static_cast<NotifyUriType>(parcel.ReadUint16());
        this->notifyType = static_cast<AccurateNotifyType>(parcel.ReadUint16());
        this->isSystem = parcel.ReadBool();
        return true;
    }
};

class DbAvailabilityData {
public:
    NotifyUriType notifyType;
    std::string status;
    std::string reason;

    bool WriteToParcel(Parcel &parcel) const
    {
        if (!parcel.WriteUint16(static_cast<uint16_t>(notifyType))) return false;
        if (!parcel.WriteString(status)) return false;
        if (!parcel.WriteString(reason)) return false;
        return true;
    }

    bool Unmarshalling(Parcel &parcel)
    {
        uint16_t type = 0;
        if (!parcel.ReadUint16(type)) return false;
        notifyType = static_cast<NotifyUriType>(type);
        if (notifyType != NotifyUriType::AVAILABILITY_URI) return false;
        if (!parcel.ReadString(status)) return false;
        if (!parcel.ReadString(reason)) return false;
        return true;
    }
};
} // namespace Notification
} // namespace Media
} // namespace OHOS
 
#endif  // OHOS_MEDIA_CHANGE_INFO_H