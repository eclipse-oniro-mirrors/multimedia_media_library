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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_GET_CHECK_RECORDS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_GET_CHECK_RECORDS_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "media_log.h"
#include "media_itypes_utils.h"
#include "cloud_file_data_vo.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT GetCheckRecordsReqBody : public IPC::IMediaParcelable {
public:
    std::vector<std::string> cloudIds;

public:  // functions of Parcelable.
    virtual ~GetCheckRecordsReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT GetCheckRecordsRespBodyCheckData : public IPC::IMediaParcelable {
public:
    GetCheckRecordsRespBodyCheckData() = default;
    virtual ~GetCheckRecordsRespBodyCheckData() = default;

public:
    std::string cloudId;
    int64_t size;             // 原文件大小
    std::string data;         // 原文件路径
    std::string displayName;  // 原文件名称
    std::string fileName;
    int32_t mediaType;     // 原文件类型（图片/视频）, 1:图片, 2:视频
    int32_t cloudVersion;  // 本地记录云端版本
    int32_t position;      // 本地数据库记录文件位置
    int64_t dateModified;
    int32_t dirty;
    int32_t thmStatus;
    int32_t syncStatus;
    std::map<std::string, CloudFileDataVo> attachment;
    int32_t fileSourceType;
    std::string storagePath;

public:  // functions of Parcelable.
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT GetCheckRecordsRespBody : public IPC::IMediaParcelable {
public:
    std::unordered_map<std::string, GetCheckRecordsRespBodyCheckData> checkDataList;

public:  // functions of Parcelable.
    virtual ~GetCheckRecordsRespBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_GET_CHECK_RECORDS_VO_H