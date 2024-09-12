/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H
#define OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H

#include "rdb_store.h"
#include "db_upgrade_utils.h"

namespace OHOS::Media {
namespace DataTransfer {
class GalleryDbUpgrade {
public:
    int32_t OnUpgrade(NativeRdb::RdbStore &store);

private:
    int32_t AddPhotoQualityOfGalleryMedia(NativeRdb::RdbStore &store);

private:
    const std::string SQL_GALLERY_MEDIA_TABLE_ADD_PHOTO_QUALITY = "\
        ALTER TABLE gallery_media ADD COLUMN photo_quality INTEGER DEFAULT 0;";

private:
    DbUpgradeUtils dbUpgradeUtils_;
};
}  // namespace DataTransfer
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_DATATRANSFER_GALLERY_DB_UPGRADE_H