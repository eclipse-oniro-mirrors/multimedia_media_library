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

#ifndef OHOS_MEDIA_CLOUD_UPLOAD_DATA_CONVERT_H
#define OHOS_MEDIA_CLOUD_UPLOAD_DATA_CONVERT_H

#include <string>

#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "photo_album_dto.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "cloud_mdkrecord_photo_album_vo.h"

#include "media_column.h"
#include "photo_album_column.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"

namespace OHOS::Media::CloudSync {
class CloudUploadDataConvert {
public:
    static int32_t HandleThumbSize(
        std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    static int32_t HandleLcdSize(
        std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    static int32_t HandleFormattedDate(
        std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    static int32_t HandleUniqueFileds(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);

    static void HandleFileType(std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);

    static void HandlePosition(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    static void HandleRotate(std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotosVo &upLoadRecord);

    static void HandleProperties(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);

    /* attachments */
    static bool IsGraffiti(const CloudMdkRecordPhotosVo &upLoadRecord);

    static bool IsMovingPhoto(const CloudMdkRecordPhotosVo &upLoadRecord);

    static int32_t HandleContent(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);

    static void HandleThumbnail(
        std::map<std::string, MDKRecordField> &recordData, std::string &path, int32_t orientation);

    static void HandleLcd(std::map<std::string, MDKRecordField> &recordData, std::string &path, int32_t orientation);

    static void HandleAttachments(
        std::map<std::string, MDKRecordField> &recordData, const CloudMdkRecordPhotosVo &upLoadRecord);

    static int32_t HandleCompatibleFileds(
        std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotosVo &upLoadRecord);

    static MDKRecord ConvertToMdkRecord(const CloudMdkRecordPhotosVo &upLoadRecord);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_UPLOAD_DATA_CONVERT_H