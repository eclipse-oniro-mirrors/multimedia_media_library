/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_ALBUM_DATA_CONVERT_H
#define OHOS_MEDIA_CLOUD_ALBUM_DATA_CONVERT_H

#include <string>

#include "cloud_media_sync_const.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "on_create_records_album_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "cloud_error_detail_vo.h"
#include "mdk_record.h"
#include "mdk_record_album_data.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "userfile_manager_types.h"

namespace OHOS::Media::CloudSync {
class CloudAlbumDataConvert {
public:
    CloudAlbumDataConvert() = default;
    CloudAlbumDataConvert(CloudAlbumOperationType type);
    ~CloudAlbumDataConvert() = default;
int32_t HandleAlbumName(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
/* properties - general */
int32_t HandleGeneral(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleProperties(std::shared_ptr<MDKRecord> &record, std::map<std::string, MDKRecordField> &data,
    const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleAttributes(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleAlbumLogicType(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleType(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleAlbumId(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandleRecordId(std::shared_ptr<MDKRecord> record, const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t HandlePath(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData);
/* record id */
int32_t FillRecordId(std::shared_ptr<MDKRecord> record, const CloudMdkRecordPhotoAlbumVo &albumData);
void HandleEmptyShow(std::shared_ptr<MDKRecord> record, std::map<std::string, MDKRecordField> &data,
    const CloudMdkRecordPhotoAlbumVo &albumData);
int32_t ConvertToDoubleScreenshot(std::shared_ptr<MDKRecord> record, std::map<std::string, MDKRecordField> &data);
std::shared_ptr<MDKRecord> ConvertToMdkRecord(const CloudMdkRecordPhotoAlbumVo &upLoadRecord);
int32_t ConvertToOnCreateRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnCreateRecordsAlbumReqBodyAlbumData &record);
int32_t BuildModifyRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnMdirtyAlbumRecord &record);
bool IsCloudSpaceFull();
void SetCloudSpaceFull(bool isCloudSpaceFull);

private:
    void ConvertErrorTypeDetails(const MDKRecordOperResult &result, std::vector<CloudErrorDetail> &errorDetails);
    std::string SanitizeAlbumName(const std::string &albumName);

private:
    bool isCloudSpaceFull_{false};
    CloudAlbumOperationType type_;
    static const std::string recordType_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_UPLOAD_DATA_CONVERT_H