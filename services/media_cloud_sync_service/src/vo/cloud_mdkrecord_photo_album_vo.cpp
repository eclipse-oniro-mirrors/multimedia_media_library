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

#define MLOG_TAG "Media_Cloud_Vo"

#include "cloud_mdkrecord_photo_album_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool CloudMdkRecordPhotoAlbumVo::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(albumId);
    parcel.WriteInt32(albumType);
    parcel.WriteString(albumName);
    parcel.WriteString(lpath);
    parcel.WriteString(cloudId);
    parcel.WriteInt32(albumSubtype);
    parcel.WriteInt64(dateAdded);
    parcel.WriteInt64(dateModified);
    parcel.WriteString(bundleName);
    parcel.WriteString(localLanguage);
    parcel.WriteString(albumPluginCloudId);
    parcel.WriteString(albumNameEn);
    parcel.WriteString(dualAlbumName);
    parcel.WriteInt32(priority);
    parcel.WriteBool(isInWhiteList);
    parcel.WriteInt32(coverUriSource);
    parcel.WriteString(coverCloudId);
    return true;
}

bool CloudMdkRecordPhotoAlbumVo::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(albumId);
    parcel.ReadInt32(albumType);
    parcel.ReadString(albumName);
    parcel.ReadString(lpath);
    parcel.ReadString(cloudId);
    parcel.ReadInt32(albumSubtype);
    parcel.ReadInt64(dateAdded);
    parcel.ReadInt64(dateModified);
    parcel.ReadString(bundleName);
    parcel.ReadString(localLanguage);
    parcel.ReadString(albumPluginCloudId);
    parcel.ReadString(albumNameEn);
    parcel.ReadString(dualAlbumName);
    parcel.ReadInt32(priority);
    parcel.ReadBool(isInWhiteList);
    parcel.ReadInt32(coverUriSource);
    parcel.ReadString(coverCloudId);
    return true;
}

std::string CloudMdkRecordPhotoAlbumVo::ToString() const
{
    return "";
}

bool CloudMdkRecordPhotoAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    bool status = parcel.ReadInt32(this->size);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.ReadBool(this->isCloudSpaceFull);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

bool CloudMdkRecordPhotoAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    bool status = parcel.WriteInt32(this->size);
    CHECK_AND_RETURN_RET(status, status);
    status = parcel.WriteBool(this->isCloudSpaceFull);
    CHECK_AND_RETURN_RET(status, status);
    return true;
}

std::string CloudMdkRecordPhotoAlbumReqBody::ToString() const
{
    return "";
}

std::vector<CloudMdkRecordPhotoAlbumVo> CloudMdkRecordPhotoAlbumRespBody::GetPhotoAlbumRecords()
{
    return this->baseAlbumUploadVo;
}

bool CloudMdkRecordPhotoAlbumRespBody::Marshalling(MessageParcel &parcel) const
{
    if (baseAlbumUploadVo.size() == 0) {
        return false;
    }
    CHECK_AND_RETURN_RET(parcel.WriteInt32(static_cast<int32_t>(baseAlbumUploadVo.size())), false);
    for (const auto &entry : baseAlbumUploadVo) {
        if (!entry.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

bool CloudMdkRecordPhotoAlbumRespBody::GetRecords(std::vector<CloudMdkRecordPhotoAlbumVo> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.clear();
    bool isValid;
    for (size_t i = 0; i < size; i++) {
        CloudMdkRecordPhotoAlbumVo nodeObj;
        isValid = nodeObj.Unmarshalling(parcel);
        CHECK_AND_RETURN_RET(isValid, false);
        val.emplace_back(nodeObj);
    }
    return true;
}
bool CloudMdkRecordPhotoAlbumRespBody::Unmarshalling(MessageParcel &parcel)
{
    return GetRecords(this->baseAlbumUploadVo, parcel);
}

std::string CloudMdkRecordPhotoAlbumRespBody::ToString() const
{
    return "";
}
}  // namespace OHOS::Media::CloudSync