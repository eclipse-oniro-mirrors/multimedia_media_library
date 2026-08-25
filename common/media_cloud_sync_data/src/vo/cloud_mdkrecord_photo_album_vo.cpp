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

#include "media_log.h"

namespace OHOS::Media::CloudSync {
bool CloudMdkRecordPhotoAlbumVo::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(albumId), false, "albumId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(albumType), false, "albumType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(albumName), false, "albumName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(lpath), false, "lpath");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(albumSubtype), false, "albumSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateAdded), false, "dateAdded");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(bundleName), false, "bundleName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(localLanguage), false, "localLanguage");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(albumPluginCloudId), false, "albumPluginCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(albumNameEn), false, "albumNameEn");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(dualAlbumName), false, "dualAlbumName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(priority), false, "priority");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(isInWhiteList), false, "isInWhiteList");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(coverUriSource), false, "coverUriSource");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(coverCloudId), false, "coverCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(shareAlbumOwner), false, "shareAlbumOwner");
    return true;
}

bool CloudMdkRecordPhotoAlbumVo::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(albumId), false, "albumId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(albumType), false, "albumType");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(albumName), false, "albumName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(lpath), false, "lpath");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(albumSubtype), false, "albumSubtype");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateAdded), false, "dateAdded");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(dateModified), false, "dateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(bundleName), false, "bundleName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(localLanguage), false, "localLanguage");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(albumPluginCloudId), false, "albumPluginCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(albumNameEn), false, "albumNameEn");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(dualAlbumName), false, "dualAlbumName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(priority), false, "priority");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(isInWhiteList), false, "isInWhiteList");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(coverUriSource), false, "coverUriSource");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(coverCloudId), false, "coverCloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(uniqueId), false, "uniqueId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(shareAlbumOwner), false, "shareAlbumOwner");
    return true;
}

std::string CloudMdkRecordPhotoAlbumVo::ToString() const
{
    return "";
}

bool CloudMdkRecordPhotoAlbumReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->isCloudSpaceFull), false, "isCloudSpaceFull");
    return true;
}

bool CloudMdkRecordPhotoAlbumReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->size), false, "size");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->isCloudSpaceFull), false, "isCloudSpaceFull");
    return true;
}

std::string CloudMdkRecordPhotoAlbumReqBody::ToString() const
{
    return "";
}

std::vector<CloudMdkRecordPhotoAlbumVo> CloudMdkRecordPhotoAlbumRespBody::GetPhotoAlbumRecords()
{
    return this->baseAlbumUploadVo_;
}

bool CloudMdkRecordPhotoAlbumRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(
        parcel.WriteInt32(static_cast<int32_t>(baseAlbumUploadVo_.size())), false, "baseAlbumUploadVo.size()");
    for (const auto &entry : baseAlbumUploadVo_) {
        CHECK_AND_RETURN_RET_LOG(entry.Marshalling(parcel), false, "CloudMdkRecordPhotoAlbumVo");
    }
    return true;
}

bool CloudMdkRecordPhotoAlbumRespBody::UnmarshallRecords(std::vector<CloudMdkRecordPhotoAlbumVo> &val,
                                                         MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(len >= 0, false, "len >= 0");
    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    bool isInValid = (size > readAbleSize) || (size > val.max_size());
    CHECK_AND_RETURN_RET_LOG(!isInValid, false, "size invalid");
    val.clear();
    for (size_t i = 0; i < size; i++) {
        CloudMdkRecordPhotoAlbumVo nodeObj;
        CHECK_AND_RETURN_RET_LOG(nodeObj.Unmarshalling(parcel), false, "CloudMdkRecordPhotoAlbumVo");
        val.emplace_back(nodeObj);
    }
    return true;
}

bool CloudMdkRecordPhotoAlbumRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(UnmarshallRecords(this->baseAlbumUploadVo_, parcel), false, "baseAlbumUploadVo_");
    return true;
}

std::string CloudMdkRecordPhotoAlbumRespBody::ToString() const
{
    return "";
}
}  // namespace OHOS::Media::CloudSync