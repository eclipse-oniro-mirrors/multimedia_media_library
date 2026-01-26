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

#ifndef OHOS_MEDIA_CLOUD_SYNC_ON_MODIFY_RECORDS_PHOTOS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_ON_MODIFY_RECORDS_PHOTOS_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "media_itypes_utils.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_define.h"
#include "cloud_error_detail_vo.h"

namespace OHOS::Media::CloudSync {
class EXPORT OnModifyRecord : public IPC::IMediaParcelable {
public:
    std::string cloudId;
    std::string path;
    std::string fileName;
    int32_t fileId;
    int64_t modifyTime;
    int64_t metaDateModified;
    int64_t version;
    bool isSuccess;
    ErrorType errorType;
    int32_t serverErrorCode;
    std::vector<CloudErrorDetail> errorDetails;

public:  // functions of Parcelable.
    virtual ~OnModifyRecord() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT OnModifyRecordsPhotosReqBody : public IPC::IMediaParcelable {
private:
    std::vector<OnModifyRecord> records;

public:
    int32_t AddModifyRecord(const OnModifyRecord &record);
    std::vector<OnModifyRecord> GetModifyRecords();

public:  // functions of Parcelable.
    virtual ~OnModifyRecordsPhotosReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H