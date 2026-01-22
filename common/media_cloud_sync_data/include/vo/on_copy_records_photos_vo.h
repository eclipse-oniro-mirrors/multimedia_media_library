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

#ifndef OHOS_MEDIA_CLOUD_SYNC_ON_COPY_RECORDS_PHOTOS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_ON_COPY_RECORDS_PHOTOS_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "media_itypes_utils.h"
#include "cloud_media_sync_const.h"
#include "cloud_error_detail_vo.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT OnCopyRecord : public IPC::IMediaParcelable {
public:
    int32_t fileId;
    int32_t rotation;
    int32_t fileType;
    int64_t size;
    int64_t createTime;
    int64_t modifyTime;
    int64_t version;
    std::string cloudId;
    std::string path;
    std::string fileName;
    std::string sourcePath;
    std::vector<CloudErrorDetail> errorDetails;
    ErrorType errorType;
    int32_t serverErrorCode;
    bool isSuccess;

public:  // functions of Parcelable.
    virtual ~OnCopyRecord() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT OnCopyRecordsPhotosReqBody : public IPC::IMediaParcelable {
private:
    std::vector<OnCopyRecord> records;

public:  // functions of Parcelable.
    virtual ~OnCopyRecordsPhotosReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;
    int32_t AddCopyRecord(const OnCopyRecord &record);
    std::vector<OnCopyRecord> GetRecords();

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H