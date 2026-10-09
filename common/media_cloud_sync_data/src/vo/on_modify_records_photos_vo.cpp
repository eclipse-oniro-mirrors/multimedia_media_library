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

#include "on_modify_records_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"

namespace OHOS::Media::CloudSync {
bool OnModifyRecord::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->path), false, "path");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->modifyTime), false, "modifyTime");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt64(this->version), false, "version");
    CHECK_AND_RETURN_RET_LOG(parcel.ReadBool(this->isSuccess), false, "isSuccess");
    int32_t copyRecordErrorType;
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(copyRecordErrorType), false, "copyRecordErrorType");
    this->errorType = static_cast<ErrorType>(copyRecordErrorType);
    CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(this->serverErrorCode), false, "serverErrorCode");
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::UnmarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel),
                             false,
                             "errorDetails");
    return true;
}

bool OnModifyRecord::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->cloudId), false, "cloudId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->path), false, "path");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteString(this->fileName), false, "fileName");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->fileId), false, "fileId");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->modifyTime), false, "modifyTime");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->metaDateModified), false, "metaDateModified");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt64(this->version), false, "version");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteBool(this->isSuccess), false, "isSuccess");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(static_cast<int32_t>(this->errorType)), false, "errorType");
    CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(this->serverErrorCode), false, "serverErrorCode");
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::MarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel),
                             false,
                             "errorDetails");
    return true;
}

std::string OnModifyRecord::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"fileId\": \"" << fileId << "\","
       << "\"modifyTime\": \"" << modifyTime << "\","
       << "\"metaDateModified\": \"" << metaDateModified << "\","
       << "\"path\": \"" << MediaFileUtils::DesensitizePath(path) << "\","
       << "\"isSuccess\": \"" << std::to_string(isSuccess) << "\","
       << "\"errorType\": \"" << static_cast<int32_t>(errorType) << "\","
       << "\"serverErrorCode\": " << serverErrorCode << ","
       << "[";
    for (uint32_t i = 0; i < errorDetails.size(); ++i) {
        ss << "{\"reason\": \"" << errorDetails[i].reason << "\","
           << "\"errorCode\": \"" << errorDetails[i].errorCode << "\","
           << "\"description\": \"" << errorDetails[i].description << "\","
           << "\"errorPos\": \"" << errorDetails[i].errorPos << "\","
           << "\"errorParam\": \"" << errorDetails[i].errorParam << "\","
           << "\"detailCode\": " << errorDetails[i].detailCode << "}";
        if (i != errorDetails.size() - 1) {
            ss << ",";
        }
    }
    ss << "]}";
    return ss.str();
}

bool OnModifyRecordsPhotosReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::UnmarshallingParcelable(this->records_, parcel), false, "records");
    return true;
}

bool OnModifyRecordsPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(IPC::ITypeMediaUtil::MarshallingParcelable(this->records_, parcel), false, "records");
    return true;
}

int32_t OnModifyRecordsPhotosReqBody::AddModifyRecord(const OnModifyRecord &record)
{
    this->records_.push_back(record);
    return E_OK;
}

std::vector<OnModifyRecord> OnModifyRecordsPhotosReqBody::GetModifyRecords()
{
    return records_;
}

std::string OnModifyRecordsPhotosReqBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync