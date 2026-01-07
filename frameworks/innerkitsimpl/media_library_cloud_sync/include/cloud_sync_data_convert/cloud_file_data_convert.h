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

#ifndef OHOS_MEDIA_CLOUD_FILE_DATA_CONVERT_H
#define OHOS_MEDIA_CLOUD_FILE_DATA_CONVERT_H

#include <string>

#include "cloud_media_sync_const.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "mdk_record_photos_data.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "medialibrary_type_const.h"
#include "on_copy_records_photos_vo.h"
#include "on_create_records_photos_vo.h"
#include "on_modify_file_dirty_vo.h"
#include "on_modify_records_photos_vo.h"
#include "on_fetch_photos_vo.h"
#include "photo_album_column.h"
#include "photos_po.h"
#include "userfile_manager_types.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class CloudFileDataConvert {
public:
    CloudFileDataConvert() = default;
    CloudFileDataConvert(CloudOperationType type, int32_t userId);
    ~CloudFileDataConvert() = default;
    std::string GetThumbPath(const std::string &path, const std::string &key);
    int32_t GetFileSize(const std::string &path, const std::string &thumbSuffix, int64_t &fileSize);
    int32_t HandleThumbSize(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleLcdSize(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleFormattedDate(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleUniqueFileds(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleFileType(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandlePosition(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleRotate(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleProperties(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleExifRotate(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    /* attachments */
    std::string GetLowerPath(const std::string &path);
    int32_t HandleEditData(std::map<std::string, MDKRecordField> &data, std::string &path, bool isMovingPhoto);
    int32_t HandleContent(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleThumbnail(std::map<std::string, MDKRecordField> &recordData, std::string &path, bool needUseExDir);
    std::string GetParentPath(const std::string &path);
    int32_t HandleLcd(std::map<std::string, MDKRecordField> &recordData, std::string &path, bool needUseExDir);
    int32_t HandleAttachments(
        std::map<std::string, MDKRecordField> &recordData, const CloudMdkRecordPhotosVo &upLoadRecord);

    int32_t SetSourceAlbum(MDKRecord &record, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t InsertAlbumIdChanges(
        MDKRecord &record, std::vector<MDKRecord> &records, const CloudMdkRecordPhotosVo &upLoadRecord);

    int32_t HandleCompatibleFileds(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t ConvertToMdkRecord(const CloudMdkRecordPhotosVo &upLoadRecord, MDKRecord &record);
    int32_t BuildCopyRecord(const std::string &cloudId, const MDKRecordOperResult &result, OnCopyRecord &record);
    int32_t BuildModifyRecord(const std::string &cloudId, const MDKRecordOperResult &result, OnModifyRecord &record);
    int32_t ConvertFdirtyRecord(
        const std::string &cloudId, const MDKRecordOperResult &result, OnFileDirtyRecord &record);
    int32_t ConvertToOnCreateRecord(
        const std::string &cloudId, const MDKRecordOperResult &result, OnCreateRecord &record);
    int32_t ConverMDKRecordToOnFetchPhotosVo(const MDKRecord &mdkRecord, OnFetchPhotosVo &OnFetchPhotoVo);

private:
    void ConvertErrorTypeDetails(const MDKRecordOperResult &result, std::vector<CloudErrorDetail> &errorDetails);
    int32_t HandleSize(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleWidthAndHeight(
        std::map<std::string, MDKRecordField> &properties, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleSourcePath(
        std::map<std::string, MDKRecordField> &properties, const CloudMdkRecordPhotosVo &upLoadRecord);
    int32_t HandleRawFile(std::map<std::string, MDKRecordField> &data, std::string &path, bool isMovingPhoto);
    void ConvertSourceAlbumIds(const MDKRecord &mdkRecord, OnFetchPhotosVo &onFetchPhotoVo);
    void ConvertAttributes(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo);
    void ConvertProperties(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo);
    int32_t HandleEditData(std::map<std::string, MDKRecordField> &data, std::string &path);
    int32_t HandleEditDataCamera(std::map<std::string, MDKRecordField> &data, std::string &path);
    int32_t CheckContentLivePhoto(const CloudMdkRecordPhotosVo &upLoadRecord, std::string &lowerPath);
    int32_t CheckContentFile(const CloudMdkRecordPhotosVo &upLoadRecord, const std::string &lowerPath);
    int32_t ExtractPosition(const std::string &position, double &latitude, double &longitude);
    int32_t ExtractPosition(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo);
    int32_t CheckFileSize(const std::string &thumbSuffix, const int64_t &fileSize);
    int32_t HandleAttributesHashMap(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &uploadRecord);
    void ConvertAttributesHashMap(MDKRecordPhotosData &data, OnFetchPhotosVo &onFetchPhotoVo);

private:
    /* identifier */
    int32_t userId_;
    std::string bundleName_;
    static const std::string recordType_;
    CloudOperationType type_;

    /* path */
    static std::string prefixLCD_;
    static std::string suffixLCD_;
    static std::string sandboxPrefix_;
    static std::string prefix_;
    static std::string suffix_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_FILE_DATA_CONVERT_H
