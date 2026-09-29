/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTOS_TABLE_CONFIG_H
#define OHOS_MEDIA_PHOTOS_TABLE_CONFIG_H

#include <string>

#include "field_config/clone_field_meta.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

inline CloneTableMeta BuildPhotosTableMeta()
{
    using FP = FieldPolicy;
    using FT = CloneFieldType;
    const std::string posLocal = PhotoColumn::PHOTO_POSITION + " IN (1, 3) ";
    const std::string posCloud = PhotoColumn::PHOTO_POSITION + " IN (1, 2, 3) ";
    const std::string syncVisible = PhotoColumn::PHOTO_SYNC_STATUS + " = " +
        std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    const std::string cleanNot = PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
        std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    const std::string timePending0 = MediaColumn::MEDIA_TIME_PENDING + " = 0";
    const std::string isTemp0 = PhotoColumn::PHOTO_IS_TEMP + " = 0";
    const std::string srcType01 = PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " IN (0, 3)";

    auto defUuid = CloneFieldDefault{ true, [](NativeRdb::ValuesBucket &b, const std::string &c) {
        b.PutString(c, MediaFileUtils::GenerateUUID());
    } };

    CloneTableMeta t{ PhotoColumn::PHOTOS_TABLE, "" };
    t.fields = {
        CloneFieldMeta{ MediaColumn::MEDIA_ID,        FT::INT32,  {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_FILE_PATH, FT::STRING, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_SIZE,     FT::INT64,  {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_TYPE,     FT::INT32,  {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_NAME,     FT::STRING, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_TITLE,    FT::STRING, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_DATE_ADDED,    FT::INT64, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_DATE_MODIFIED, FT::INT64, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_ORIENTATION, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_SUBTYPE,     FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_DATE_TRASHED, FT::INT64, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ MediaColumn::MEDIA_HIDDEN,       FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },

        CloneFieldMeta{ PhotoColumn::PHOTO_POSITION, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, posLocal, posCloud },
        CloneFieldMeta{ PhotoColumn::PHOTO_SYNC_STATUS, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, syncVisible, syncVisible },
        CloneFieldMeta{ PhotoColumn::PHOTO_CLEAN_FLAG, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, cleanNot, cleanNot },
        CloneFieldMeta{ MediaColumn::MEDIA_TIME_PENDING, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, timePending0, timePending0 },
        CloneFieldMeta{ PhotoColumn::PHOTO_IS_TEMP, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, isTemp0, isTemp0 },
        CloneFieldMeta{ PhotoColumn::PHOTO_FILE_SOURCE_TYPE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, srcType01, srcType01 },

        CloneFieldMeta{ PhotoColumn::PHOTO_CLOUD_ID, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_DIRTY, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_CLOUD_VERSION, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_THUMB_STATUS, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_THUMBNAIL_READY, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_LCD_VISIT_TIME, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_CE_AVAILABLE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_CE_STATUS_CODE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_METADATA_FLAGS, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_EXIF_ROTATE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_TRANSCODE_TIME, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_FILE_INODE, FT::INT64, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_STORAGE_PATH, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },

        CloneFieldMeta{ PhotoColumn::UNIQUE_ID, FT::STRING, defUuid, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_RISK_STATUS, FT::INT32,
            { true, [](NativeRdb::ValuesBucket &b, const std::string &c) {
                b.PutInt(c, static_cast<int32_t>(PhotoRiskStatus::UNIDENTIFIED));
            } },
            FP::INHERIT_SOURCE, FP::INHERIT_SOURCE, FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
    };
    return t;
}

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_PHOTOS_TABLE_CONFIG_H
